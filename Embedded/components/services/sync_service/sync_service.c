/**
 * @file     sync_service.c
 * @brief    活动窗口同步服务实现。
 * @details  默认 mock 传输；真机 Air780 可编译切换。不阻塞 tap/progress/feedback。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_service.h"

#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "device_nav_service.h"
#include "device_settings_policy.h"
#include "esp_log.h"
#include "feedback_service.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "power_core_path_policy.h"
#include "progress_service.h"
#include "sync_config.h"
#include "sync_https_mock.h"
#include "ewf_scripture_canonical.h"

#if EWF_SYNC_TRANSPORT_AIR780
#include "air780egp_bsp.h"
#endif

static const char *TAG = "SVC_SYNC";

#define SYNC_POLL_MS 200U
#define SYNC_RESPONSE_CAPACITY 4096U

static SemaphoreHandle_t s_mutex;
static atomic_bool s_stop_requested;
static atomic_bool s_running;
static bool s_contracts_ready;
static bool s_prepared;

static ewf_sync_window_state_t s_window;
static ewf_sync_fact_t s_sync_fact = EWF_SYNC_FACT_LOCAL_RECORDED;
static ewf_sync_network_mode_t s_network_mode = EWF_SYNC_NETWORK_DISABLED;
static uint32_t s_last_business_code;
static uint32_t s_window_count;
static bool s_conflict;
static bool s_identity_ready;
/** 无有效样本前占位 50；有有效样本后上报真实 0–100。 */
static uint8_t s_battery_percent = 50U;
static bool s_battery_hardware_pending = true;

static char s_device_id[EWF_SYNC_DEVICE_ID_CAPACITY];
static char s_firmware_version[EWF_SYNC_FIRMWARE_VERSION_CAPACITY];
static char s_scripture_version[EWF_SYNC_SCRIPTURE_VERSION_CAPACITY];
static uint32_t s_audio_config_version = EWF_SYNC_AUDIO_CONFIG_VERSION_DEFAULT;

static char s_response_buffer[SYNC_RESPONSE_CAPACITY];

static bool identity_fields_ok(void);
static esp_err_t run_one_window(uint64_t now_ms, bool immediate);
static void publish_network_mode(ewf_sync_network_mode_t mode);
static const char *network_mode_wire(ewf_sync_network_mode_t mode);
static bool core_path_allows_new_sync_window(uint8_t power_level);

esp_err_t sync_service_init_contracts(void)
{
    if (s_contracts_ready) {
        return ESP_OK;
    }
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_running, false);
    ewf_sync_window_state_init(&s_window);
    (void)snprintf(s_device_id, sizeof(s_device_id), "%s",
                   EWF_SYNC_DEVICE_ID_DEFAULT);
    (void)snprintf(s_firmware_version, sizeof(s_firmware_version), "%s",
                   EWF_SYNC_FIRMWARE_VERSION_DEFAULT);
    (void)snprintf(s_scripture_version, sizeof(s_scripture_version), "%s",
                   EWF_SCRIPTURE_VERSION);
    s_contracts_ready = true;
    return ESP_OK;
}

esp_err_t sync_service_prepare_run(void)
{
    if (!s_contracts_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    s_identity_ready = identity_fields_ok();
    if (!s_identity_ready) {
        ESP_LOGE(TAG,
                 "同步身份未就绪：fail-closed，不开窗假成功（device_id/firmware/"
                 "scripture 缺省）");
        s_network_mode = EWF_SYNC_NETWORK_DISABLED;
        s_prepared = true;
        return ESP_OK;
    }
    if (s_battery_hardware_pending) {
        ESP_LOGW(TAG,
                 "battery_percent 占位=%u（尚无有效 CW2015 样本；有样本后上报真实值）",
                 (unsigned)s_battery_percent);
    }

#if EWF_SYNC_TRANSPORT_AIR780
    const esp_err_t modem_err = ewf_air780_start();
    if (modem_err != ESP_OK) {
        ESP_LOGW(TAG, "Air780 start 失败：%s；网络事实=disabled",
                 esp_err_to_name(modem_err));
        s_network_mode = EWF_SYNC_NETWORK_DISABLED;
    } else {
        s_network_mode = EWF_SYNC_NETWORK_NO_SIGNAL;
    }
#else
    /* mock 路径：可观测为 connected，便于主机钉住 network_mode。 */
    s_network_mode = EWF_SYNC_NETWORK_CONNECTED;
    ESP_LOGI(TAG, "同步传输=MOCK（Epic 2 边界，不依赖真实 backend）");
#endif
    s_prepared = true;
    return ESP_OK;
}

void sync_service_cancel_prepared_run(void)
{
    s_prepared = false;
}

esp_err_t sync_service_deinit_contracts(void)
{
    if (atomic_load(&s_running)) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_mutex != NULL) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }
    s_contracts_ready = false;
    s_prepared = false;
    return ESP_OK;
}

esp_err_t sync_service_request_stop(TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (!s_contracts_ready) {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, true);
    return ESP_OK;
}

esp_err_t sync_service_snapshot(ewf_sync_service_snapshot_t *snapshot)
{
    if (snapshot == NULL || s_mutex == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    snapshot->sync_fact = s_sync_fact;
    snapshot->network_mode = s_network_mode;
    snapshot->backoff_stage = s_window.backoff_stage;
    snapshot->last_business_code = s_last_business_code;
    snapshot->window_count = s_window_count;
    snapshot->conflict = s_conflict;
    snapshot->identity_ready = s_identity_ready;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t sync_service_set_identity(const char *device_id,
                                    const char *firmware_version,
                                    const char *scripture_version,
                                    uint32_t audio_config_version)
{
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    if (device_id != NULL) {
        (void)snprintf(s_device_id, sizeof(s_device_id), "%s", device_id);
    }
    if (firmware_version != NULL) {
        (void)snprintf(s_firmware_version, sizeof(s_firmware_version), "%s",
                       firmware_version);
    }
    if (scripture_version != NULL) {
        (void)snprintf(s_scripture_version, sizeof(s_scripture_version), "%s",
                       scripture_version);
    }
    s_audio_config_version = audio_config_version;
    s_identity_ready = identity_fields_ok();
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t sync_service_set_battery_percent(uint8_t percent)
{
    if (percent > 100U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    s_battery_percent = percent;
    /* 有效样本到达后清除占位；无样本前 prepare 仍可发 hardware_pending 日志。 */
    s_battery_hardware_pending = false;
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static bool core_path_allows_new_sync_window(uint8_t power_level)
{
    bool persist_pending = false;
    bool persist_inflight = false;
    /* 读失败 fail-closed：视为落盘忙，禁止新开窗。 */
    if (progress_service_get_persist_status(&persist_pending, &persist_inflight) !=
        ESP_OK) {
        ESP_LOGW(TAG, "无法读取落盘状态，fail-closed 推迟同步开窗");
        return false;
    }

    const ewf_power_core_path_input_t core_input = {
        .power_level = power_level,
        .persist_pending = persist_pending,
        .persist_inflight = persist_inflight,
        .sync_window_requested = true,
        .feedback_pending = false,
    };
    const ewf_power_core_path_decision_t core =
        ewf_power_core_path_decide(&core_input);
    if (core.software_poweroff_intent) {
        /* 合同：策略永不产出软件关机；若漂移则 fail-closed 不开窗。 */
        ESP_LOGE(TAG, "核心路径非法产出软件关机意图，拒绝开窗");
        return false;
    }
    if (!core.allow_sync) {
        ESP_LOGW(TAG,
                 "核心路径推迟同步开窗：persist_pending=%d inflight=%d "
                 "（保持 pending/busy，不假成功推进 acked_total）",
                 (int)persist_pending, (int)persist_inflight);
        return false;
    }
    return true;
}

esp_err_t sync_service_poll_once(uint64_t now_ms)
{
    if (!s_contracts_ready || !s_prepared) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_identity_ready) {
        return ESP_ERR_INVALID_STATE;
    }

    const ewf_sync_status_t sync_status = device_nav_service_sync_status();
    const bool immediate = (sync_status == EWF_SYNC_STATUS_PENDING);

    watch_state_snapshot_t state = {0};
    const esp_err_t snap_err = watch_state_snapshot(&state, 0);
    if (snap_err != ESP_OK) {
        return snap_err;
    }

    ewf_device_settings_record_t settings = {0};
    (void)device_nav_service_get_settings(&settings);

    ewf_sync_window_input_t input = {
        .trigger = EWF_SYNC_TRIGGER_NONE,
        .local_total = state.tap_local_total,
        .acked_total = state.tap_acked_total,
        .command_revision = 0U,
        .applied_revision = settings.applied_revision,
        .immediate_pending = immediate,
        .transport_ok = false,
        .transport_timeout = false,
        .force_close = false,
        .now_ms = now_ms,
    };

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    const ewf_sync_window_decision_t decision =
        ewf_sync_window_policy_apply(&s_window, &input);
    xSemaphoreGive(s_mutex);

    if (decision.action == EWF_SYNC_WINDOW_OPEN) {
        if (!core_path_allows_new_sync_window((uint8_t)state.power_level)) {
            /* 策略已把 window_open/transport_busy 置真；必须回滚，否则后续永久 MERGE。 */
            if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                ewf_sync_window_input_t force = {.force_close = true,
                                                 .now_ms = now_ms};
                (void)ewf_sync_window_policy_apply(&s_window, &force);
                xSemaphoreGive(s_mutex);
            }
            /* 不推进 acked；立即同步保持 pending/busy 语义。 */
            if (immediate) {
                (void)device_nav_service_complete_sync(EWF_SYNC_STATUS_BUSY, 0);
            }
            return ESP_OK;
        }
        return run_one_window(now_ms, immediate);
    }
    if (immediate && decision.action == EWF_SYNC_WINDOW_MERGE) {
        (void)device_nav_service_complete_sync(EWF_SYNC_STATUS_BUSY, 0);
    }
    return ESP_OK;
}

esp_err_t sync_service_run(void)
{
    if (!s_contracts_ready || !s_prepared) {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_running, true);
    atomic_store(&s_stop_requested, false);
    ESP_LOGI(TAG, "同步服务已进入活动窗口轮询");

    while (!atomic_load(&s_stop_requested)) {
        const uint64_t now_ms =
            (uint64_t)pdTICKS_TO_MS(xTaskGetTickCount());
        (void)sync_service_poll_once(now_ms);
        vTaskDelay(pdMS_TO_TICKS(SYNC_POLL_MS));
    }

#if EWF_SYNC_TRANSPORT_AIR780
    (void)ewf_air780_suspend();
#endif
    atomic_store(&s_running, false);
    return ESP_OK;
}

static bool identity_fields_ok(void)
{
    if (s_device_id[0] == '\0' ||
        strcmp(s_device_id, EWF_SYNC_DEVICE_ID_DEFAULT) == 0) {
        return false;
    }
    if (s_firmware_version[0] == '\0' || s_scripture_version[0] == '\0') {
        return false;
    }
    return true;
}

static const char *network_mode_wire(ewf_sync_network_mode_t mode)
{
    switch (mode) {
    case EWF_SYNC_NETWORK_CONNECTED:
        return "connected";
    case EWF_SYNC_NETWORK_NO_SIGNAL:
        return "no_signal";
    case EWF_SYNC_NETWORK_DISABLED:
    default:
        return "disabled";
    }
}

static void publish_network_mode(ewf_sync_network_mode_t mode)
{
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_network_mode = mode;
        xSemaphoreGive(s_mutex);
    }
}

static esp_err_t run_one_window(uint64_t now_ms, bool immediate)
{
    (void)now_ms;
    if (immediate) {
        (void)device_nav_service_complete_sync(EWF_SYNC_STATUS_BUSY, 0);
    }

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    s_sync_fact = EWF_SYNC_FACT_SYNCING;
    ++s_window_count;
    xSemaphoreGive(s_mutex);

    watch_state_snapshot_t state = {0};
    if (watch_state_snapshot(&state, 0) != ESP_OK) {
        if (immediate) {
            (void)device_nav_service_complete_sync(EWF_SYNC_STATUS_FAIL, 0);
        }
        return ESP_ERR_INVALID_STATE;
    }
    ewf_device_settings_record_t settings = {0};
    (void)device_nav_service_get_settings(&settings);

    ewf_sync_report_request_t request;
    memset(&request, 0, sizeof(request));
    (void)snprintf(request.device_id, sizeof(request.device_id), "%s",
                   s_device_id);
    (void)snprintf(request.scripture_version, sizeof(request.scripture_version),
                   "%s", s_scripture_version);
    request.local_total = state.tap_local_total;
    request.acked_total = state.tap_acked_total;
    request.round_id = state.tap_round_id == 0U ? 1U : state.tap_round_id;
    (void)snprintf(request.round_state, sizeof(request.round_state), "%s",
                   state.tap_round_state == 1U ? "completed" : "in_progress");
    request.round_cursor = state.tap_round_cursor;
    request.pending_completion = state.tap_pending_completion;
    request.applied_revision = settings.applied_revision;
    request.battery_percent = s_battery_percent;
    (void)snprintf(request.network_mode, sizeof(request.network_mode), "%s",
                   network_mode_wire(s_network_mode));
    request.audio_config_version = s_audio_config_version;
    (void)snprintf(request.firmware_version, sizeof(request.firmware_version),
                   "%s", s_firmware_version);
    (void)snprintf(request.action_id, sizeof(request.action_id),
                   "sync-%lu", (unsigned long)device_nav_service_sync_request_id());

    char request_json[1024];
    const size_t req_len =
        ewf_sync_https_encode_request(&request, request_json,
                                      sizeof(request_json));
    if (req_len == 0U) {
        ESP_LOGE(TAG, "请求编码失败");
        goto fail_close;
    }

    {
        char redact[64];
        ewf_sync_https_redact_field("url", EWF_SYNC_HTTPS_URL, redact,
                                    sizeof(redact));
        ESP_LOGI(TAG, "开窗上报：%s body(len=%lu)", redact,
                 (unsigned long)req_len);
        ewf_sync_https_redact_field("token", EWF_SYNC_DEVICE_TOKEN, redact,
                                    sizeof(redact));
        ESP_LOGI(TAG, "鉴权字段：%s", redact);
    }

    bool transport_ok = false;
    bool transport_timeout = false;
    size_t response_len = 0U;

#if EWF_SYNC_TRANSPORT_MOCK
    {
        uint16_t http_status = 0U;
        response_len = ewf_sync_https_mock_build_response(
            &request, s_response_buffer, sizeof(s_response_buffer),
            &http_status);
        transport_ok = response_len > 0U;
        (void)http_status;
    }
#elif EWF_SYNC_TRANSPORT_AIR780
    {
        const esp_err_t post_err = ewf_air780_https_post_json(
            EWF_SYNC_HTTPS_URL, EWF_SYNC_DEVICE_TOKEN, request_json,
            s_response_buffer, sizeof(s_response_buffer), &response_len,
            EWF_AIR780_HTTPS_DEADLINE_MS);
        transport_ok = (post_err == ESP_OK && response_len > 0U);
        if (post_err == ESP_ERR_TIMEOUT) {
            transport_timeout = true;
        }
        (void)ewf_air780_suspend();
    }
#else
#error "必须定义 EWF_SYNC_TRANSPORT_MOCK 或 EWF_SYNC_TRANSPORT_AIR780"
#endif

    ewf_sync_report_response_t response;
    memset(&response, 0, sizeof(response));
    const bool decoded =
        transport_ok &&
        ewf_sync_https_decode_response(s_response_buffer, &response);

    ewf_sync_response_input_t resp_input = {
        .code = decoded ? response.code : -1,
        .request_acked_total = request.acked_total,
        .response_acked_total = decoded ? response.acked_total : 0U,
        .command_revision = decoded ? response.command_revision : 0U,
        .applied_revision = settings.applied_revision,
        .transport_ok = decoded,
    };
    const ewf_sync_response_decision_t resp_decision =
        ewf_sync_response_policy_apply(&resp_input);

    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_last_business_code =
            decoded ? (uint32_t)response.code : 0U;
        s_conflict = resp_decision.mark_conflict;
        if (resp_decision.mark_synced) {
            s_sync_fact = EWF_SYNC_FACT_SYNCED;
        } else if (resp_decision.mark_conflict) {
            s_sync_fact = EWF_SYNC_FACT_CONFLICT;
        } else if (resp_decision.mark_sync_failed) {
            s_sync_fact = EWF_SYNC_FACT_SYNC_FAILED;
        } else if (resp_decision.mark_pending_sync) {
            s_sync_fact = EWF_SYNC_FACT_PENDING_SYNC;
        }
        xSemaphoreGive(s_mutex);
    }

    if (resp_decision.action == EWF_SYNC_RESPONSE_ADVANCE_ACKED ||
        resp_decision.action == EWF_SYNC_RESPONSE_ACK_WITH_GATE) {
        const esp_err_t ack_err = progress_service_advance_acked_total(
            resp_decision.new_acked_total, 0);
        if (ack_err != ESP_OK) {
            ESP_LOGE(TAG, "acked_total 推进失败：%s", esp_err_to_name(ack_err));
        }
    } else if (resp_decision.action == EWF_SYNC_RESPONSE_STOP_PROGRESS) {
        ESP_LOGE(TAG, "经文版本不一致(20005)：停止推进，不得伪装已同步");
    } else if (resp_decision.action == EWF_SYNC_RESPONSE_CONFLICT) {
        ESP_LOGW(TAG, "设备重置冲突(20003)：进入可恢复冲突态，不回退 local_total");
    }

    if (resp_decision.apply_pending_command && response.has_command_payload) {
        ewf_brightness_t brightness = EWF_BRIGHTNESS_MID;
        if (response.brightness == 0U) {
            brightness = EWF_BRIGHTNESS_LOW;
        } else if (response.brightness == 2U) {
            brightness = EWF_BRIGHTNESS_HIGH;
        } else if (response.brightness == 1U) {
            brightness = EWF_BRIGHTNESS_MID;
        }
        const esp_err_t apply_err = device_nav_service_apply_command_revision(
            response.command_revision, response.volume, brightness,
            response.timeout_s != 0U ? response.timeout_s : 15U, 0);
        if (apply_err == ESP_OK) {
            (void)feedback_service_set_volume(response.volume, 0);
            /* 待应用命令抬升后，视需要再开窗回传由后续 poll 触发。 */
            ESP_LOGI(TAG, "已应用命令修订=%lu",
                     (unsigned long)response.command_revision);
        }
    }

    {
        /* 只回写传输结果，不携带 backlog/立即同步触发，避免用陈旧 acked 误开窗。 */
        ewf_sync_window_input_t close_input = {
            .transport_ok = resp_decision.mark_synced,
            .transport_timeout = transport_timeout,
            .transport_failed = !resp_decision.mark_synced && !transport_timeout,
            .now_ms = now_ms,
        };
        if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
            (void)ewf_sync_window_policy_apply(&s_window, &close_input);
            ewf_sync_window_input_t force = {.force_close = true,
                                             .now_ms = now_ms};
            (void)ewf_sync_window_policy_apply(&s_window, &force);
            xSemaphoreGive(s_mutex);
        }
    }

    publish_network_mode(resp_decision.mark_synced
                             ? EWF_SYNC_NETWORK_CONNECTED
                             : EWF_SYNC_NETWORK_NO_SIGNAL);

    if (immediate) {
        const ewf_sync_status_t final_status =
            resp_decision.mark_synced ? EWF_SYNC_STATUS_OK
                                      : EWF_SYNC_STATUS_FAIL;
        (void)device_nav_service_complete_sync(final_status, 0);
    }

    if (resp_decision.mark_synced) {
        return ESP_OK;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        if (state.tap_local_total > state.tap_acked_total) {
            s_sync_fact = EWF_SYNC_FACT_PENDING_SYNC;
        }
        xSemaphoreGive(s_mutex);
    }
    return ESP_FAIL;

fail_close:
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
        s_sync_fact = EWF_SYNC_FACT_SYNC_FAILED;
        ewf_sync_window_input_t force = {.force_close = true, .now_ms = now_ms};
        (void)ewf_sync_window_policy_apply(&s_window, &force);
        xSemaphoreGive(s_mutex);
    }
    if (immediate) {
        (void)device_nav_service_complete_sync(EWF_SYNC_STATUS_FAIL, 0);
    }
    return ESP_FAIL;
}
