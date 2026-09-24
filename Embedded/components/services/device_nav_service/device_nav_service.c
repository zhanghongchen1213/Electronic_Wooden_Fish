/**
 * @file     device_nav_service.c
 * @brief    设备导航、设置与立即同步请求服务实现。
 * @details  经 state_service 发布 typed 导航事实；显示开关只经 CO5300 BSP；
 *           不驱动 KILL、不模拟软件关机。立即同步无客户端时停在 pending。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_nav_service.h"

#include <stdatomic.h>
#include <string.h>

#include "co5300_bsp.h"
#include "device_settings_store.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "state_service.h"

static const char *TAG = "SVC_NAV";

#define DEVICE_NAV_QUEUE_DEPTH 8U
#define DEVICE_NAV_IDLE_POLL_MS 200U
#define DEVICE_NAV_PUBLISH_TIMEOUT_MS 50U

typedef enum {
    DEVICE_NAV_MSG_STOP = 0,
    DEVICE_NAV_MSG_INPUT,
    DEVICE_NAV_MSG_TICK,
} device_nav_msg_kind_t;

typedef struct {
    device_nav_msg_kind_t kind;
    ewf_nav_input_kind_t input_kind;
    uint32_t duration_ms;
    uint64_t now_ms;
} device_nav_message_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_mutex;
static bool s_prepared;
static atomic_bool s_running;
static ewf_nav_state_t s_nav;
static ewf_device_settings_record_t s_settings;
static ewf_sync_status_t s_sync_status;
static uint32_t s_sync_request_id;
static bool s_settings_persist_error;
static uint32_t s_screen_transition_sequence;

static uint64_t now_ms(void);
static esp_err_t apply_input_locked(const ewf_nav_input_t *input,
                                    TickType_t timeout_ticks);
static esp_err_t publish_nav_locked(watch_nav_reason_t reason,
                                    TickType_t timeout_ticks);
static esp_err_t persist_settings_locked(void);
static esp_err_t apply_display_locked(bool on);
static esp_err_t apply_brightness_locked(void);
static watch_nav_page_t to_watch_page(ewf_nav_page_t page);
static watch_nav_reason_t to_watch_reason(ewf_nav_reason_t reason);
static watch_brightness_t to_watch_brightness(ewf_brightness_t value);
static watch_sync_status_t to_watch_sync(ewf_sync_status_t status);

esp_err_t device_nav_service_init_contracts(void)
{
    if (s_queue != NULL && s_mutex != NULL) {
        return ESP_OK;
    }
    s_queue = xQueueCreate(DEVICE_NAV_QUEUE_DEPTH, sizeof(device_nav_message_t));
    s_mutex = xSemaphoreCreateMutex();
    if (s_queue == NULL || s_mutex == NULL) {
        if (s_queue != NULL) {
            vQueueDelete(s_queue);
            s_queue = NULL;
        }
        if (s_mutex != NULL) {
            vSemaphoreDelete(s_mutex);
            s_mutex = NULL;
        }
        return ESP_ERR_NO_MEM;
    }
    ewf_device_settings_defaults(&s_settings);
    ewf_nav_state_init(&s_nav, s_settings.timeout_s, 0U);
    s_sync_status = EWF_SYNC_STATUS_IDLE;
    s_sync_request_id = 0U;
    s_settings_persist_error = false;
    s_screen_transition_sequence = 0U;
    atomic_store(&s_running, false);
    return ESP_OK;
}

esp_err_t device_nav_service_prepare_run(void)
{
    if (s_queue == NULL || s_mutex == NULL || atomic_load(&s_running)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    ewf_device_settings_record_t loaded = {0};
    ewf_settings_store_status_t status = EWF_SETTINGS_STORE_ERROR;
    const esp_err_t load_error =
        ewf_device_settings_store_load(&loaded, &status);
    if (load_error != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return load_error;
    }
    if (status == EWF_SETTINGS_STORE_EMPTY) {
        /* load 在 EMPTY 路径已填 defaults，并可带迁移音量。 */
        s_settings = loaded;
        if (ewf_device_settings_validate(&s_settings) != EWF_SETTINGS_OK) {
            ewf_device_settings_defaults(&s_settings);
        }
        const esp_err_t save_error = ewf_device_settings_store_save(&s_settings);
        if (save_error != ESP_OK) {
            ESP_LOGE(TAG, "首启设置初始落盘失败，错误=%s",
                     esp_err_to_name(save_error));
            s_settings_persist_error = true;
        } else {
            s_settings_persist_error = false;
        }
    } else if (status == EWF_SETTINGS_STORE_ERROR) {
        ESP_LOGE(TAG, "设置恢复失败，保持内存默认值");
        s_settings_persist_error = true;
        ewf_device_settings_defaults(&s_settings);
    } else {
        s_settings = loaded;
        s_settings_persist_error = false;
    }
    ewf_nav_state_init(&s_nav, s_settings.timeout_s, now_ms());
    (void)apply_brightness_locked();
    (void)publish_nav_locked(WATCH_NAV_REASON_NONE,
                             pdMS_TO_TICKS(DEVICE_NAV_PUBLISH_TIMEOUT_MS));
    s_prepared = true;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

void device_nav_service_cancel_prepared_run(void)
{
    if (!atomic_load(&s_running)) {
        s_prepared = false;
    }
}

esp_err_t device_nav_service_deinit_contracts(void)
{
    if (atomic_load(&s_running)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_queue != NULL) {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }
    if (s_mutex != NULL) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }
    s_prepared = false;
    return ESP_OK;
}

esp_err_t device_nav_service_run(void)
{
    if (s_queue == NULL || !s_prepared) {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_running, true);
    device_nav_message_t message = {0};
    bool stop = false;
    while (!stop) {
        const BaseType_t got = xQueueReceive(
            s_queue, &message, pdMS_TO_TICKS(DEVICE_NAV_IDLE_POLL_MS));
        if (got == pdTRUE) {
            if (message.kind == DEVICE_NAV_MSG_STOP) {
                stop = true;
                continue;
            }
            if (message.kind == DEVICE_NAV_MSG_INPUT) {
                if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                    const ewf_nav_input_t input = {
                        .kind = message.input_kind,
                        .duration_ms = message.duration_ms,
                        .now_ms = message.now_ms,
                    };
                    (void)apply_input_locked(
                        &input, pdMS_TO_TICKS(DEVICE_NAV_PUBLISH_TIMEOUT_MS));
                    xSemaphoreGive(s_mutex);
                }
            }
        } else {
            /* 空闲超时轮询：只发熄屏意图，不关机。 */
            if (xSemaphoreTake(s_mutex, 0) == pdTRUE) {
                const ewf_nav_input_t input = {
                    .kind = EWF_NAV_INPUT_IDLE_TIMEOUT,
                    .now_ms = now_ms(),
                };
                (void)apply_input_locked(
                    &input, pdMS_TO_TICKS(DEVICE_NAV_PUBLISH_TIMEOUT_MS));
                xSemaphoreGive(s_mutex);
            }
        }
    }
    atomic_store(&s_running, false);
    s_prepared = false;
    return ESP_OK;
}

esp_err_t device_nav_service_request_stop(TickType_t timeout_ticks)
{
    if (s_queue == NULL || !s_prepared) {
        return ESP_ERR_INVALID_STATE;
    }
    const device_nav_message_t stop = {.kind = DEVICE_NAV_MSG_STOP};
    return xQueueSend(s_queue, &stop, timeout_ticks) == pdTRUE ? ESP_OK
                                                               : ESP_ERR_TIMEOUT;
}

static esp_err_t enqueue_input(ewf_nav_input_kind_t kind, uint32_t duration_ms,
                               uint64_t at_ms)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    /* 任务未进入 run 循环时同步应用，便于主机接线测试与启动前交接。 */
    if (s_queue == NULL || !s_prepared || !atomic_load(&s_running)) {
        if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
            return ESP_ERR_TIMEOUT;
        }
        const ewf_nav_input_t input = {
            .kind = kind,
            .duration_ms = duration_ms,
            .now_ms = at_ms,
        };
        const esp_err_t error = apply_input_locked(
            &input, pdMS_TO_TICKS(DEVICE_NAV_PUBLISH_TIMEOUT_MS));
        xSemaphoreGive(s_mutex);
        return error;
    }
    const device_nav_message_t message = {
        .kind = DEVICE_NAV_MSG_INPUT,
        .input_kind = kind,
        .duration_ms = duration_ms,
        .now_ms = at_ms,
    };
    return xQueueSend(s_queue, &message, pdMS_TO_TICKS(10U)) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t device_nav_service_on_pwr_interval(uint32_t duration_ms,
                                             uint64_t now_ms_value)
{
    return enqueue_input(EWF_NAV_INPUT_PWR_INTERVAL, duration_ms, now_ms_value);
}

esp_err_t device_nav_service_on_nav_input(ewf_nav_input_kind_t kind,
                                          uint64_t now_ms_value)
{
    if (kind == EWF_NAV_INPUT_NONE || kind == EWF_NAV_INPUT_PWR_INTERVAL) {
        return ESP_ERR_INVALID_ARG;
    }
    return enqueue_input(kind, 0U, now_ms_value);
}

esp_err_t device_nav_service_note_activity(uint64_t now_ms_value)
{
    return enqueue_input(EWF_NAV_INPUT_ACTIVITY, 0U, now_ms_value);
}

ewf_nav_page_t device_nav_service_active_page(void)
{
    /* 互斥失败时 fail-closed：非木鱼页，避免误开木鱼计数门禁。 */
    ewf_nav_page_t page = EWF_NAV_PAGE_JINGWEN;
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
        page = s_nav.active_page;
        xSemaphoreGive(s_mutex);
    }
    return page;
}

bool device_nav_service_screen_on(void)
{
    /* 互斥失败时 fail-closed：视为熄屏，避免误计数。 */
    bool on = false;
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
        on = s_nav.screen_on;
        xSemaphoreGive(s_mutex);
    }
    return on;
}

bool device_nav_service_settings_open(void)
{
    /* 互斥失败时 fail-closed：视为设置打开，禁止木鱼计数。 */
    bool open = true;
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
        open = s_nav.settings_open;
        xSemaphoreGive(s_mutex);
    }
    return open;
}

esp_err_t device_nav_service_set_volume(uint8_t volume,
                                        TickType_t timeout_ticks)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!ewf_device_settings_volume_valid(volume)) {
        ESP_LOGW(TAG, "音量写入被拒绝：取值域恰为 0-100，提交值=%u",
                 (unsigned)volume);
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    if (volume == s_settings.volume) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }
    const uint8_t previous = s_settings.volume;
    s_settings.volume = volume;
    const esp_err_t save_error = persist_settings_locked();
    if (save_error != ESP_OK) {
        s_settings.volume = previous;
        s_settings_persist_error = true;
        (void)publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "音量落盘失败，错误=%s；保持原值=%u",
                 esp_err_to_name(save_error), (unsigned)previous);
        return save_error;
    }
    s_settings_persist_error = false;
    s_nav.last_activity_ms = now_ms();
    const esp_err_t publish_error =
        publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
    xSemaphoreGive(s_mutex);
    return publish_error;
}

esp_err_t device_nav_service_set_brightness(ewf_brightness_t brightness,
                                            TickType_t timeout_ticks)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if ((unsigned)brightness >= (unsigned)EWF_BRIGHTNESS_COUNT) {
        ESP_LOGW(TAG, "亮度写入被拒绝：非法枚举=%u", (unsigned)brightness);
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    if (brightness == s_settings.brightness) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }
    const ewf_brightness_t previous = s_settings.brightness;
    s_settings.brightness = brightness;
    const esp_err_t save_error = persist_settings_locked();
    if (save_error != ESP_OK) {
        s_settings.brightness = previous;
        s_settings_persist_error = true;
        (void)publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
        xSemaphoreGive(s_mutex);
        return save_error;
    }
    s_settings_persist_error = false;
    (void)apply_brightness_locked();
    s_nav.last_activity_ms = now_ms();
    const esp_err_t publish_error =
        publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
    xSemaphoreGive(s_mutex);
    return publish_error;
}

esp_err_t device_nav_service_set_timeout_s(uint32_t timeout_s,
                                           TickType_t timeout_ticks)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!ewf_device_settings_timeout_valid(timeout_s)) {
        ESP_LOGW(TAG, "熄屏时长写入被拒绝：仅允许 5/15/30，提交值=%lu",
                 (unsigned long)timeout_s);
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    if (timeout_s == s_settings.timeout_s) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }
    const uint32_t previous = s_settings.timeout_s;
    s_settings.timeout_s = timeout_s;
    const esp_err_t save_error = persist_settings_locked();
    if (save_error != ESP_OK) {
        s_settings.timeout_s = previous;
        s_settings_persist_error = true;
        (void)publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
        xSemaphoreGive(s_mutex);
        return save_error;
    }
    s_settings_persist_error = false;
    s_nav.idle_timeout_s = timeout_s;
    s_nav.last_activity_ms = now_ms();
    const esp_err_t publish_error =
        publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
    xSemaphoreGive(s_mutex);
    return publish_error;
}

esp_err_t device_nav_service_get_settings(ewf_device_settings_record_t *record)
{
    if (record == NULL || s_mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    *record = s_settings;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t device_nav_service_request_sync(TickType_t timeout_ticks)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    ++s_sync_request_id;
    if (s_sync_request_id == 0U) {
        ++s_sync_request_id;
    }
    /* 无 Story 2.5 通信客户端时不得假装 HTTPS 成功，停在 pending。 */
    s_sync_status = EWF_SYNC_STATUS_PENDING;
    ESP_LOGI(TAG,
             "立即同步已请求：request_id=%lu，等待通信客户端（状态=pending）",
             (unsigned long)s_sync_request_id);
    s_nav.last_activity_ms = now_ms();
    const esp_err_t error =
        publish_nav_locked(WATCH_NAV_REASON_SYNC_REQUEST, timeout_ticks);
    xSemaphoreGive(s_mutex);
    return error;
}

ewf_sync_status_t device_nav_service_sync_status(void)
{
    ewf_sync_status_t status = EWF_SYNC_STATUS_IDLE;
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
        status = s_sync_status;
        xSemaphoreGive(s_mutex);
    }
    return status;
}

uint32_t device_nav_service_sync_request_id(void)
{
    uint32_t id = 0U;
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
        id = s_sync_request_id;
        xSemaphoreGive(s_mutex);
    }
    return id;
}

esp_err_t device_nav_service_apply_command_revision(
    uint32_t command_revision,
    uint8_t volume,
    ewf_brightness_t brightness,
    uint32_t timeout_s,
    TickType_t timeout_ticks)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!ewf_device_settings_volume_valid(volume) ||
        (unsigned)brightness >= (unsigned)EWF_BRIGHTNESS_COUNT ||
        !ewf_device_settings_timeout_valid(timeout_s)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    if (command_revision < s_settings.applied_revision) {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "命令修订倒退被拒绝：当前=%lu 提交=%lu",
                 (unsigned long)s_settings.applied_revision,
                 (unsigned long)command_revision);
        return ESP_ERR_INVALID_ARG;
    }
    if (command_revision == s_settings.applied_revision) {
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }

    const ewf_device_settings_record_t previous = s_settings;
    s_settings.volume = volume;
    s_settings.brightness = brightness;
    s_settings.timeout_s = timeout_s;
    s_settings.applied_revision = command_revision;
    const esp_err_t save_error = persist_settings_locked();
    if (save_error != ESP_OK) {
        s_settings = previous;
        s_settings_persist_error = true;
        (void)publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
        xSemaphoreGive(s_mutex);
        return save_error;
    }
    s_settings_persist_error = false;
    (void)apply_brightness_locked();
    s_nav.idle_timeout_s = timeout_s;
    s_nav.last_activity_ms = now_ms();
    const esp_err_t publish_error =
        publish_nav_locked(WATCH_NAV_REASON_SETTINGS_WRITE, timeout_ticks);
    xSemaphoreGive(s_mutex);
    return publish_error;
}

esp_err_t device_nav_service_complete_sync(ewf_sync_status_t status,
                                           TickType_t timeout_ticks)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (status != EWF_SYNC_STATUS_OK && status != EWF_SYNC_STATUS_FAIL &&
        status != EWF_SYNC_STATUS_BUSY && status != EWF_SYNC_STATUS_PENDING) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    /*
     * 飞行中请求只允许回写终端态或 busy；禁止无匹配请求时写入
     * pending/busy 造成 UI 假飞态。
     */
    if (status == EWF_SYNC_STATUS_PENDING || status == EWF_SYNC_STATUS_BUSY) {
        if (s_sync_status != EWF_SYNC_STATUS_PENDING &&
            s_sync_status != EWF_SYNC_STATUS_BUSY) {
            xSemaphoreGive(s_mutex);
            ESP_LOGW(TAG, "complete_sync 拒绝：无飞行中请求却写入非终端态=%d",
                     (int)status);
            return ESP_ERR_INVALID_STATE;
        }
    } else if (s_sync_status != EWF_SYNC_STATUS_PENDING &&
               s_sync_status != EWF_SYNC_STATUS_BUSY) {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "complete_sync 拒绝：当前非飞行态=%d，提交=%d",
                 (int)s_sync_status, (int)status);
        return ESP_ERR_INVALID_STATE;
    }
    s_sync_status = status;
    const esp_err_t error =
        publish_nav_locked(WATCH_NAV_REASON_SYNC_REQUEST, timeout_ticks);
    xSemaphoreGive(s_mutex);
    return error;
}

static uint64_t now_ms(void)
{
    return (uint64_t)pdTICKS_TO_MS(xTaskGetTickCount());
}

static esp_err_t apply_input_locked(const ewf_nav_input_t *input,
                                    TickType_t timeout_ticks)
{
    const bool prev_screen_on = s_nav.screen_on;
    const ewf_nav_decision_t decision = ewf_nav_policy_apply(&s_nav, input);
    if (!decision.changed) {
        return ESP_OK;
    }
    if (decision.wake_display) {
        if (apply_display_locked(true) != ESP_OK) {
            s_nav.screen_on = false;
            ESP_LOGW(TAG, "亮屏 BSP 失败，回滚 screen_on");
        }
    }
    if (decision.blank_display) {
        if (apply_display_locked(false) != ESP_OK) {
            s_nav.screen_on = prev_screen_on;
            ESP_LOGW(TAG, "熄屏 BSP 失败，回滚 screen_on");
        }
    }
    return publish_nav_locked(to_watch_reason(decision.reason), timeout_ticks);
}

static esp_err_t publish_nav_locked(watch_nav_reason_t reason,
                                    TickType_t timeout_ticks)
{
    watch_nav_update_t facts = {
        .update_sequence = 0U,
        .screen_state =
            s_nav.screen_on ? WATCH_SCREEN_STATE_ON : WATCH_SCREEN_STATE_OFF,
        .active_page = to_watch_page(s_nav.active_page),
        .settings_open = s_nav.settings_open,
        .last_reason = reason,
        .volume = s_settings.volume,
        .brightness = to_watch_brightness(s_settings.brightness),
        .timeout_s = s_settings.timeout_s,
        .applied_revision = s_settings.applied_revision,
        .sync_status = to_watch_sync(s_sync_status),
        .sync_request_id = s_sync_request_id,
        .settings_persist_error = s_settings_persist_error,
    };
    /*
     * 仅在熄亮屏变化时推进 power 通道序号，避免设置写入把序号重置后
     * 撞上 watch_state 中已采用的更高序号。
     */
    if (reason == WATCH_NAV_REASON_PWR_WAKE ||
        reason == WATCH_NAV_REASON_TOUCH_WAKE ||
        reason == WATCH_NAV_REASON_IDLE_OFF ||
        reason == WATCH_NAV_REASON_NONE) {
        watch_power_snapshot_t power_snap = {0};
        if (watch_state_power_snapshot(&power_snap, 0) == ESP_OK) {
            if (s_screen_transition_sequence <=
                power_snap.screen_transition_sequence) {
                s_screen_transition_sequence =
                    power_snap.screen_transition_sequence;
            }
        }
        ++s_screen_transition_sequence;
        if (s_screen_transition_sequence == 0U) {
            ++s_screen_transition_sequence;
        }
        const watch_power_update_t power = {
            .screen_state = facts.screen_state,
            .last_local_activity_ms = s_nav.last_activity_ms,
            .transition_sequence = s_screen_transition_sequence,
            .error = WATCH_POWER_ERROR_NONE,
        };
        (void)state_service_publish_power(&power, timeout_ticks);
    }
    return state_service_update_nav_owner(&facts, timeout_ticks);
}

static esp_err_t persist_settings_locked(void)
{
    s_settings.schema_version = EWF_DEVICE_SETTINGS_SCHEMA_VERSION;
    return ewf_device_settings_store_save(&s_settings);
}

static esp_err_t apply_display_locked(bool on)
{
    const esp_err_t error = co5300_bsp_set_display(on);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "显示开关失败：on=%d，错误=%s", (int)on,
                 esp_err_to_name(error));
    }
    return error;
}

static esp_err_t apply_brightness_locked(void)
{
    const uint16_t raw =
        ewf_device_settings_brightness_raw(s_settings.brightness);
    const esp_err_t error = co5300_bsp_set_brightness(raw);
    if (error != ESP_OK) {
        ESP_LOGW(TAG, "亮度应用失败：raw=%u，错误=%s", (unsigned)raw,
                 esp_err_to_name(error));
    }
    return error;
}

static watch_nav_page_t to_watch_page(ewf_nav_page_t page)
{
    switch (page) {
    case EWF_NAV_PAGE_JINGWEN:
        return WATCH_NAV_PAGE_JINGWEN;
    case EWF_NAV_PAGE_TONGJI:
        return WATCH_NAV_PAGE_TONGJI;
    case EWF_NAV_PAGE_MUYU:
    default:
        return WATCH_NAV_PAGE_MUYU;
    }
}

static watch_nav_reason_t to_watch_reason(ewf_nav_reason_t reason)
{
    switch (reason) {
    case EWF_NAV_REASON_PWR_CYCLE_PAGE:
        return WATCH_NAV_REASON_PWR_CYCLE_PAGE;
    case EWF_NAV_REASON_PWR_CLOSE_SETTINGS:
        return WATCH_NAV_REASON_PWR_CLOSE_SETTINGS;
    case EWF_NAV_REASON_PWR_WAKE:
        return WATCH_NAV_REASON_PWR_WAKE;
    case EWF_NAV_REASON_SWIPE_PAGE:
        return WATCH_NAV_REASON_SWIPE_PAGE;
    case EWF_NAV_REASON_OPEN_SETTINGS:
        return WATCH_NAV_REASON_OPEN_SETTINGS;
    case EWF_NAV_REASON_CLOSE_SETTINGS:
        return WATCH_NAV_REASON_CLOSE_SETTINGS;
    case EWF_NAV_REASON_TOUCH_WAKE:
        return WATCH_NAV_REASON_TOUCH_WAKE;
    case EWF_NAV_REASON_IDLE_OFF:
        return WATCH_NAV_REASON_IDLE_OFF;
    case EWF_NAV_REASON_ACTIVITY_RESET:
        return WATCH_NAV_REASON_ACTIVITY;
    default:
        return WATCH_NAV_REASON_NONE;
    }
}

static watch_brightness_t to_watch_brightness(ewf_brightness_t value)
{
    switch (value) {
    case EWF_BRIGHTNESS_LOW:
        return WATCH_BRIGHTNESS_LOW;
    case EWF_BRIGHTNESS_HIGH:
        return WATCH_BRIGHTNESS_HIGH;
    case EWF_BRIGHTNESS_MID:
    default:
        return WATCH_BRIGHTNESS_MID;
    }
}

static watch_sync_status_t to_watch_sync(ewf_sync_status_t status)
{
    switch (status) {
    case EWF_SYNC_STATUS_PENDING:
        return WATCH_SYNC_STATUS_PENDING;
    case EWF_SYNC_STATUS_BUSY:
        return WATCH_SYNC_STATUS_BUSY;
    case EWF_SYNC_STATUS_OK:
        return WATCH_SYNC_STATUS_OK;
    case EWF_SYNC_STATUS_FAIL:
        return WATCH_SYNC_STATUS_FAIL;
    case EWF_SYNC_STATUS_IDLE:
    default:
        return WATCH_SYNC_STATUS_IDLE;
    }
}
