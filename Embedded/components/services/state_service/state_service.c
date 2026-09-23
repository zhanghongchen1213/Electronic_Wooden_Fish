/**
 * @file     state_service.c
 * @brief    类型化状态聚合服务实现
 * @details  在固定 state_task 中串行消费产品更新，校验授权与普通控制 typed 载荷并提供 apply-ACK。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "state_service.h"

#include <ctype.h>
#include <stdatomic.h>
#include <string.h>

#include "esp_log.h"
#include "config_service.h"
#include "cw2015_bsp.h"

static const char *TAG = "SVC_STATE";

/** 电量计 canonical 稳定错误码；原属 power_service.h，Story 1.1 由本服务自己拥有。 */
#define STATE_SERVICE_BATTERY_OK_CODE "POWER_BATTERY_OK"
/** 电量计不可用的 canonical 稳定错误码。 */
#define STATE_SERVICE_BATTERY_UNAVAILABLE_CODE "POWER_BATTERY_UNAVAILABLE"
/** apply ACK 最高位表示 state_task 未成功应用更新。 */
#define STATE_SERVICE_ACK_FAILURE_BIT (1UL << 31)
/** apply ACK 序号使用的低 31 位。 */
#define STATE_SERVICE_ACK_ID_MASK (STATE_SERVICE_ACK_FAILURE_BIT - 1UL)
/** BLE latest-value mailbox 短临界区有界尝试次数。 */
#define STATE_SERVICE_BLE_MAILBOX_LOCK_ATTEMPTS 4U
/** 配置 latest-value mailbox 短临界区有界尝试次数。 */
#define STATE_SERVICE_CONFIG_MAILBOX_LOCK_ATTEMPTS 4U
/** modem latest-value mailbox 短临界区有界尝试次数。 */
#define STATE_SERVICE_MODEM_MAILBOX_LOCK_ATTEMPTS 4U
/** GPS latest-value mailbox 短临界区有界尝试次数。 */
#define STATE_SERVICE_GPS_MAILBOX_LOCK_ATTEMPTS 4U
/** telemetry terminal 保留槽短临界区有界尝试次数。 */
#define STATE_SERVICE_TELEMETRY_MAILBOX_LOCK_ATTEMPTS 4U
/** 配置重投遇到瞬态锁超时时的下一次尝试间隔。 */
#define STATE_SERVICE_CONFIG_RETRY_DELAY_MS 50U

/** 自检 terminal apply ACK 的启动周期内序号。 */
static atomic_uint s_next_apply_ack_id;
/** 当前 state_task 句柄，仅供发布成功后直接唤醒 owner。 */
static _Atomic(TaskHandle_t) s_owner_task;
/** 保护 BLE latest-value mailbox 按值副本的短临界区。 */
static atomic_flag s_ble_mailbox_lock = ATOMIC_FLAG_INIT;
/** state_task 尚未取走的最新完整 BLE 真值。 */
static watch_ble_update_t s_ble_mailbox;
/** BLE latest-value mailbox 是否存在待应用真值。 */
static bool s_ble_mailbox_dirty;
/** 保护配置 latest-value mailbox 按值副本的短临界区。 */
static atomic_flag s_config_mailbox_lock = ATOMIC_FLAG_INIT;
/** state_task 尚未取走的最新无凭据配置真值。 */
static watch_config_update_t s_config_mailbox;
/** 配置 latest-value mailbox 是否存在待应用真值。 */
static bool s_config_mailbox_dirty;
/** 保护 modem latest-value mailbox 的短临界区。 */
static atomic_flag s_modem_mailbox_lock = ATOMIC_FLAG_INIT;
/** state_task 尚未取走的最新 modem 真值。 */
static watch_modem_update_t s_modem_mailbox;
/** modem mailbox 是否存在待应用真值。 */
static bool s_modem_mailbox_dirty;
/** 保护 GPS latest-value mailbox 的短临界区。 */
static atomic_flag s_gps_mailbox_lock = ATOMIC_FLAG_INIT;
/** state_task 尚未取走的最新完整 GPS 真值。 */
static watch_gps_update_t s_gps_mailbox;
/** GPS mailbox 是否存在待应用真值。 */
static bool s_gps_mailbox_dirty;
/** 保护 telemetry owned terminal 保留槽的短临界区。 */
static atomic_flag s_telemetry_mailbox_lock = ATOMIC_FLAG_INIT;
/** state_task 尚未取走且不得被覆盖的 telemetry 终态。 */
static watch_telemetry_update_t s_telemetry_mailbox;
/** telemetry 保留槽是否存在待应用终态。 */
static bool s_telemetry_mailbox_dirty;

static const char *canonical_power_error_code(const char *error_code);
static const char *canonical_driver_error_code(const char *error_code);
static bool canonical_mac_is_valid(
    const char mac[WATCH_BLE_MAC_CAPACITY]);
static bool lock_ble_mailbox(void);
static esp_err_t store_ble_mailbox(const watch_ble_update_t *update);
static bool take_ble_mailbox(watch_ble_update_t *update);
static void restore_ble_mailbox_if_empty(const watch_ble_update_t *update);
static bool stop_and_take_ble_mailbox(watch_ble_update_t *update);
static bool lock_config_mailbox(void);
static esp_err_t store_config_mailbox(const watch_config_update_t *update);
static bool take_config_mailbox(watch_config_update_t *update);
static bool restore_config_mailbox(const watch_config_update_t *update);
static bool stop_and_take_config_mailbox(watch_config_update_t *update);
static bool lock_modem_mailbox(void);
static esp_err_t store_modem_mailbox(const watch_modem_update_t *update);
static bool take_modem_mailbox(watch_modem_update_t *update);
static bool restore_modem_mailbox(const watch_modem_update_t *update);
static bool stop_and_take_modem_mailbox(watch_modem_update_t *update);
static bool lock_gps_mailbox(void);
static esp_err_t store_gps_mailbox(const watch_gps_update_t *update);
static bool take_gps_mailbox(watch_gps_update_t *update);
static bool restore_gps_mailbox(const watch_gps_update_t *update);
static bool stop_and_take_gps_mailbox(watch_gps_update_t *update);
static bool lock_telemetry_mailbox(void);
static esp_err_t store_telemetry_mailbox(
    const watch_telemetry_update_t *update);
static bool take_telemetry_mailbox(watch_telemetry_update_t *update);
static bool restore_telemetry_mailbox(
    const watch_telemetry_update_t *update);
static bool stop_and_take_telemetry_mailbox(
    watch_telemetry_update_t *update);
static bool control_update_is_valid(const watch_control_update_t *update);
static bool control_target_is_valid(watch_control_kind_t kind, uint8_t target);
static void wake_owner(void);
static esp_err_t publish_queue_update(QueueHandle_t queue,
                                      const state_service_update_t *update,
                                      TickType_t timeout_ticks);
static esp_err_t apply_update(const state_service_update_t *update);
static esp_err_t apply_update_with_retry(const state_service_update_t *update);
static esp_err_t publish_terminal_update(state_service_update_t *update,
                                         TickType_t timeout_ticks);
static uint32_t next_apply_ack_id(void);
static esp_err_t wait_for_apply_ack(uint32_t ack_id);
static void acknowledge_update(const state_service_update_t *update, esp_err_t apply_error);
static void observe_applied_update(const state_service_update_t *update);
static void finish_run(QueueHandle_t queue);

bool state_service_update_requires_model_refresh(
    state_service_update_type_t type)
{
    return type == STATE_SERVICE_UPDATE_BATTERY ||
           type == STATE_SERVICE_UPDATE_BLE ||
           type == STATE_SERVICE_UPDATE_POWER ||
           type == STATE_SERVICE_UPDATE_MODEM ||
           type == STATE_SERVICE_UPDATE_GPS ||
           type == STATE_SERVICE_UPDATE_CONTROL ||
           type == STATE_SERVICE_UPDATE_CLOUD ||
           type == STATE_SERVICE_UPDATE_TELEMETRY;
}

esp_err_t state_service_publish_battery(const watch_battery_update_t *update,
                                        TickType_t timeout_ticks)
{
    if (update == NULL || update->power_error_code == NULL ||
        update->driver_error_code == NULL ||
        update->power_level < WATCH_POWER_LEVEL_NORMAL ||
        update->power_level >= WATCH_POWER_LEVEL_COUNT ||
        (update->valid && update->percent > 100U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    const char *power_error_code = canonical_power_error_code(update->power_error_code);
    const char *driver_error_code = canonical_driver_error_code(update->driver_error_code);
    if (power_error_code == NULL || driver_error_code == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_BATTERY,
        .payload.battery = {
            .valid = update->valid,
            .percent = update->percent,
            .power_level = update->power_level,
            .attempted_at_ticks = update->attempted_at_ticks,
            .power_error_code = power_error_code,
            .driver_error_code = driver_error_code,
        },
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_publish_power(const watch_power_update_t *update,
                                      TickType_t timeout_ticks)
{
    if (update == NULL ||
        update->screen_state < WATCH_SCREEN_STATE_ON ||
        update->screen_state >= WATCH_SCREEN_STATE_COUNT ||
        update->transition_sequence == 0U ||
        update->error < WATCH_POWER_ERROR_NONE ||
        update->error >= WATCH_POWER_ERROR_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_POWER,
        .payload.power = *update,
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_publish_audio(const watch_audio_update_t *update,
                                      TickType_t timeout_ticks)
{
    if (update == NULL || update->state < WATCH_AUDIO_STATE_IDLE ||
        update->state > WATCH_AUDIO_STATE_FAILED ||
        update->error < WATCH_AUDIO_ERROR_NONE ||
        update->error > WATCH_AUDIO_ERROR_PA_FAILED)
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_AUDIO,
        .payload.audio = *update,
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_publish_ble(const watch_ble_update_t *update,
                                    TickType_t timeout_ticks)
{
    if (update == NULL ||
        update->transaction < WATCH_BLE_TRANSACTION_UNBOUND ||
        update->transaction > WATCH_BLE_TRANSACTION_ERROR ||
        (update->transaction == WATCH_BLE_TRANSACTION_UNBOUND &&
         (update->has_bound_exoskeleton || update->ble_connected)) ||
        (update->transaction == WATCH_BLE_TRANSACTION_BOUND_READY &&
         !update->has_bound_exoskeleton) ||
        (!update->has_bound_exoskeleton &&
         update->bound_exoskeleton_mac[0] != '\0') ||
        (!update->ble_connected &&
         (update->gatt_ready || update->mtu_ready || update->negotiated_mtu != 0U ||
          update->notify_ready || update->dataReady || update->status_fresh ||
          update->link_control_ready)) ||
        (!update->mtu_ready && update->negotiated_mtu != 0U) ||
        (update->mtu_ready &&
         (!update->gatt_ready || update->negotiated_mtu == 0U)) ||
        (update->notify_ready && !update->gatt_ready) ||
        (!update->device_state_available &&
         (update->dataReady || update->status_fresh || update->last_status_rx_ms != 0U)) ||
        (update->device_state_available &&
         (!update->has_bound_exoskeleton || update->last_status_rx_ms == 0U ||
          memchr(update->exoskeleton_state.version, '\0',
                 sizeof(update->exoskeleton_state.version)) == NULL)) ||
        (update->dataReady && (!update->ble_connected || !update->device_state_available)) ||
        (update->status_fresh &&
         (!update->device_state_available || !update->dataReady ||
          update->last_status_rx_ms == 0U)) ||
        (update->link_control_ready &&
         (!update->ble_connected || !update->gatt_ready ||
          !update->notify_ready || !update->dataReady ||
          !update->status_fresh)) ||
        (!update->link_control_ready && !update->clear_pending_control) ||
        memchr(update->ble_error_code, '\0', sizeof(update->ble_error_code)) == NULL ||
        strncmp(update->ble_error_code, "BLE_", 4U) != 0 ||
        (update->has_bound_exoskeleton &&
         !canonical_mac_is_valid(update->bound_exoskeleton_mac)))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (legbot_state_service_queue() == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    const esp_err_t error = store_ble_mailbox(update);
    if (error == ESP_OK)
    {
        wake_owner();
    }
    return error;
}

esp_err_t state_service_publish_config(const watch_config_update_t *update,
                                       TickType_t timeout_ticks)
{
    const esp_err_t validation_error = watch_config_update_validate(update);
    if (validation_error != ESP_OK)
    {
        return validation_error;
    }
    if (legbot_state_service_queue() == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    const esp_err_t error = store_config_mailbox(update);
    if (error == ESP_OK || error == ESP_ERR_TIMEOUT)
    {
        /* config owner 保留失败真值；超时时也唤醒 state_task 安排有限重投。 */
        wake_owner();
    }
    return error;
}

esp_err_t state_service_publish_control(const watch_control_update_t *update,
                                        TickType_t timeout_ticks)
{
    if (!control_update_is_valid(update))
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_CONTROL,
        .payload.control = *update,
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_publish_control_confirmed(
    const watch_control_update_t *update,
    TickType_t timeout_ticks)
{
    if (!control_update_is_valid(update))
    {
        return ESP_ERR_INVALID_ARG;
    }
    state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_CONTROL,
        .payload.control = *update,
    };
    return publish_terminal_update(&message, timeout_ticks);
}

#ifdef STATE_SERVICE_CONTROL_TEST
/**
 * @brief host 回归中执行 production apply 与 ACK 路径
 * @param update 已由测试队列接收的 typed 更新
 * @return production apply_update 的真实结果
 */
esp_err_t state_service_test_apply_queued_update(
    const state_service_update_t *update)
{
    const esp_err_t error = apply_update(update);
    acknowledge_update(update, error);
    return error;
}

void state_service_test_reset_gps_mailbox(void)
{
    while (!lock_gps_mailbox())
    {
        taskYIELD();
    }
    memset(&s_gps_mailbox, 0, sizeof(s_gps_mailbox));
    s_gps_mailbox_dirty = false;
    atomic_flag_clear(&s_gps_mailbox_lock);
}

bool state_service_test_take_gps_mailbox(watch_gps_update_t *update)
{
    return take_gps_mailbox(update);
}

int state_service_test_telemetry_mailbox_contract(void)
{
    while (!lock_telemetry_mailbox())
    {
        taskYIELD();
    }
    memset(&s_telemetry_mailbox, 0, sizeof(s_telemetry_mailbox));
    s_telemetry_mailbox_dirty = false;
    atomic_flag_clear(&s_telemetry_mailbox_lock);
    watch_telemetry_update_t first = {
        .telemetry_update_sequence = 1U,
        .request_id = 10U,
        .result = WATCH_TELEMETRY_RESULT_HTTP_ERROR,
        .http_status = 429U,
    };
    memcpy(first.error_code,
           "CLOUD_RATE_LIMITED",
           sizeof("CLOUD_RATE_LIMITED"));
    watch_telemetry_update_t second = first;
    second.telemetry_update_sequence = 2U;
    second.request_id = 11U;
    if (store_telemetry_mailbox(&first) != ESP_OK ||
        store_telemetry_mailbox(&second) != ESP_ERR_TIMEOUT)
    {
        return 1;
    }
    watch_telemetry_update_t received = {0};
    if (!take_telemetry_mailbox(&received) || received.request_id != 10U ||
        store_telemetry_mailbox(&second) != ESP_OK ||
        !take_telemetry_mailbox(&received) || received.request_id != 11U)
    {
        return 2;
    }
    return 0;
}
#endif

esp_err_t state_service_publish_cloud(const watch_cloud_update_t *update,
                                      TickType_t timeout_ticks)
{
    if (update == NULL || update->cloud_update_sequence == 0U ||
        update->request_id == 0U ||
        update->result <= WATCH_RENTAL_RESULT_NONE ||
        update->result > WATCH_RENTAL_RESULT_CONFIG_ERROR ||
        memchr(update->error_code, '\0', sizeof(update->error_code)) == NULL ||
        strncmp(update->error_code, "CLOUD_", 6U) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_CLOUD,
        .payload.cloud = *update,
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_publish_telemetry(
    const watch_telemetry_update_t *update,
    TickType_t timeout_ticks)
{
    const esp_err_t validation_error =
        watch_telemetry_update_validate(update);
    if (validation_error != ESP_OK)
    {
        return validation_error;
    }
    if (legbot_state_service_queue() == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    const esp_err_t error = store_telemetry_mailbox(update);
    if (error == ESP_OK)
    {
        wake_owner();
    }
    return error;
}

esp_err_t state_service_persist_cloud_offset(
    int64_t offset_ms,
    TickType_t timeout_ticks)
{
    state_service_update_t update = {
        .type = STATE_SERVICE_UPDATE_CLOUD_OFFSET_PERSIST,
        .payload.cloud_offset_ms = offset_ms,
    };
    return publish_terminal_update(&update, timeout_ticks);
}

esp_err_t state_service_publish_modem(const watch_modem_update_t *update,
                                      TickType_t timeout_ticks)
{
    if (watch_modem_update_validate(update) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (legbot_state_service_queue() == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    const esp_err_t error = store_modem_mailbox(update);
    if (error == ESP_OK)
    {
        wake_owner();
    }
    return error;
}

esp_err_t state_service_publish_gps(const watch_gps_update_t *update,
                                    TickType_t timeout_ticks)
{
    const esp_err_t validation_error = watch_gps_update_validate(update);
    if (validation_error != ESP_OK)
    {
        return validation_error;
    }
    if (legbot_state_service_queue() == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    const esp_err_t error = store_gps_mailbox(update);
    if (error == ESP_OK)
    {
        wake_owner();
    }
    return error;
}

esp_err_t state_service_publish_selftest_begin(const watch_selftest_begin_update_t *update,
                                               TickType_t timeout_ticks)
{
    if (update == NULL || update->run_id == 0U ||
        update->first_item < SELFTEST_ITEM_BSP_RESOURCE_TABLE || update->first_item >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_SELFTEST_BEGIN,
        .payload.selftest_begin = *update,
    };
    return publish_terminal_update(&message, timeout_ticks);
}

esp_err_t state_service_publish_selftest_item(const watch_selftest_item_update_t *update,
                                              TickType_t timeout_ticks)
{
    if (update == NULL || update->run_id == 0U ||
        update->result.item_id < SELFTEST_ITEM_BSP_RESOURCE_TABLE ||
        update->result.item_id >= SELFTEST_ITEM_COUNT ||
        (update->result.outcome != SELFTEST_OUTCOME_PASS &&
         update->result.outcome != SELFTEST_OUTCOME_FAIL &&
         update->result.outcome != SELFTEST_OUTCOME_SKIP &&
         update->result.outcome != SELFTEST_OUTCOME_INCONCLUSIVE) ||
        memchr(update->result.detail_code, '\0', sizeof(update->result.detail_code)) == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_SELFTEST_ITEM,
        .payload.selftest_item = *update,
    };
    return publish_terminal_update(&message, timeout_ticks);
}

esp_err_t state_service_publish_selftest_finish(const watch_selftest_finish_update_t *update,
                                                TickType_t timeout_ticks)
{
    if (update == NULL || update->run_id == 0U ||
        (update->state != SELFTEST_RUN_FINISHED &&
         update->state != SELFTEST_RUN_CANCELLED))
    {
        return ESP_ERR_INVALID_ARG;
    }
    state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_SELFTEST_FINISH,
        .payload.selftest_finish = *update,
    };
    return publish_terminal_update(&message, timeout_ticks);
}

esp_err_t state_service_publish_selftest_retry_begin(
    const watch_selftest_retry_begin_update_t *update,
    TickType_t timeout_ticks)
{
    if (update == NULL || update->run_id == 0U ||
        update->item_id < SELFTEST_ITEM_BSP_RESOURCE_TABLE || update->item_id >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_SELFTEST_RETRY_BEGIN,
        .payload.selftest_retry_begin = *update,
    };
    return publish_terminal_update(&message, timeout_ticks);
}

esp_err_t state_service_publish_selftest_retry_finish(
    const watch_selftest_retry_finish_update_t *update,
    TickType_t timeout_ticks)
{
    if (update == NULL || update->run_id == 0U ||
        update->result.item_id < SELFTEST_ITEM_BSP_RESOURCE_TABLE ||
        update->result.item_id >= SELFTEST_ITEM_COUNT ||
        (update->result.outcome != SELFTEST_OUTCOME_PASS &&
         update->result.outcome != SELFTEST_OUTCOME_FAIL &&
         update->result.outcome != SELFTEST_OUTCOME_SKIP &&
         update->result.outcome != SELFTEST_OUTCOME_INCONCLUSIVE) ||
        memchr(update->result.detail_code, '\0', sizeof(update->result.detail_code)) == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_SELFTEST_RETRY_FINISH,
        .payload.selftest_retry_finish = *update,
    };
    return publish_terminal_update(&message, timeout_ticks);
}

esp_err_t state_service_request_stop(TickType_t timeout_ticks)
{
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_STOP,
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_publish_tap_gate(
    const watch_tap_gate_update_t *update,
    TickType_t timeout_ticks)
{
    if (update == NULL || update->update_sequence == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const state_service_update_t message = {
        .type = STATE_SERVICE_UPDATE_TAP_GATE,
        .payload.tap_gate = *update,
    };
    return publish_queue_update(queue, &message, timeout_ticks);
}

esp_err_t state_service_read_tap_gate(bool *service_ready,
                                      bool *completed,
                                      bool *fault_locked)
{
    if (service_ready == NULL || completed == NULL || fault_locked == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    *service_ready = false;
    *completed = false;
    *fault_locked = false;

    /* state_task 是敲击 gate 的唯一事实源入口；调用方不得传入可伪造的状态。 */
    if (atomic_load(&s_owner_task) == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    watch_state_snapshot_t snapshot = {0};
    const esp_err_t error = watch_state_snapshot(&snapshot, 0);
    if (error != ESP_OK)
    {
        return error;
    }

    *completed = snapshot.tap_completed;
    *fault_locked = snapshot.tap_fault_locked;
    *service_ready = true;
    return ESP_OK;
}

void state_service_run(void)
{
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        ESP_LOGE(TAG, "state_task 消息队列未初始化，无法启动状态聚合");
        return;
    }

    atomic_store(&s_owner_task, xTaskGetCurrentTaskHandle());
    bool prefer_config_mailbox = true;
    bool prefer_ble_mailbox = true;
    bool prefer_modem_mailbox = true;
    bool prefer_gps_mailbox = true;
    for (;;)
    {
        /* 只有配置重投的瞬态锁超时会创建有限 deadline。 */
        const esp_err_t config_retry_error =
            config_service_retry_pending_change();
        const TickType_t owner_wait_ticks =
            config_retry_error == ESP_ERR_TIMEOUT
                ? pdMS_TO_TICKS(STATE_SERVICE_CONFIG_RETRY_DELAY_MS)
                : portMAX_DELAY;
        state_service_update_t update = {0};
        bool update_received = false;
        if (take_telemetry_mailbox(&update.payload.telemetry))
        {
            update.type = STATE_SERVICE_UPDATE_TELEMETRY;
            update_received = true;
        }
        else if (prefer_gps_mailbox && take_gps_mailbox(&update.payload.gps))
        {
            update.type = STATE_SERVICE_UPDATE_GPS;
            update_received = true;
        }
        else if (prefer_modem_mailbox && take_modem_mailbox(&update.payload.modem))
        {
            update.type = STATE_SERVICE_UPDATE_MODEM;
            update_received = true;
        }
        else if (prefer_config_mailbox && take_config_mailbox(&update.payload.config))
        {
            update.type = STATE_SERVICE_UPDATE_CONFIG;
            update_received = true;
        }
        else if (prefer_ble_mailbox && take_ble_mailbox(&update.payload.ble))
        {
            update.type = STATE_SERVICE_UPDATE_BLE;
            update_received = true;
        }
        else if (xQueueReceive(queue, &update, 0) == pdTRUE)
        {
            update_received = true;
        }
        else if (!prefer_config_mailbox && take_config_mailbox(&update.payload.config))
        {
            update.type = STATE_SERVICE_UPDATE_CONFIG;
            update_received = true;
        }
        else if (!prefer_ble_mailbox && take_ble_mailbox(&update.payload.ble))
        {
            update.type = STATE_SERVICE_UPDATE_BLE;
            update_received = true;
        }
        else if (!prefer_modem_mailbox && take_modem_mailbox(&update.payload.modem))
        {
            update.type = STATE_SERVICE_UPDATE_MODEM;
            update_received = true;
        }
        else if (!prefer_gps_mailbox && take_gps_mailbox(&update.payload.gps))
        {
            update.type = STATE_SERVICE_UPDATE_GPS;
            update_received = true;
        }
        if (!update_received)
        {
            (void)ulTaskNotifyTake(pdTRUE, owner_wait_ticks);
            continue;
        }
        if (update.type == STATE_SERVICE_UPDATE_STOP)
        {
            watch_config_update_t final_config_update = {0};
            if (stop_and_take_config_mailbox(&final_config_update))
            {
                const state_service_update_t final_update = {
                    .type = STATE_SERVICE_UPDATE_CONFIG,
                    .payload.config = final_config_update,
                };
                esp_err_t final_error = ESP_ERR_TIMEOUT;
                const TickType_t final_started_at = xTaskGetTickCount();
                do
                {
                    final_error = apply_update_with_retry(&final_update);
                    if (final_error == ESP_ERR_TIMEOUT)
                    {
                        ESP_LOGE(TAG, "state_task 停止前最终配置真值暂时繁忙，继续重试");
                        taskYIELD();
                    }
                } while (final_error == ESP_ERR_TIMEOUT &&
                         xTaskGetTickCount() - final_started_at <
                             pdMS_TO_TICKS(STATE_SERVICE_APPLY_ACK_TIMEOUT_MS));
                if (final_error == ESP_ERR_TIMEOUT)
                {
                    (void)restore_config_mailbox(&final_update.payload.config);
                    ESP_LOGE(TAG, "state_task 停止前配置真值仍繁忙，已保留至下次启动");
                }
                else if (final_error != ESP_OK)
                {
                    ESP_LOGE(TAG, "state_task 停止前最终配置真值非法，已拒绝应用");
                }
            }
            watch_ble_update_t final_ble_update = {0};
            if (stop_and_take_ble_mailbox(&final_ble_update))
            {
                const state_service_update_t final_update = {
                    .type = STATE_SERVICE_UPDATE_BLE,
                    .payload.ble = final_ble_update,
                };
                esp_err_t final_error = ESP_ERR_TIMEOUT;
                do
                {
                    final_error = apply_update_with_retry(&final_update);
                    if (final_error != ESP_OK)
                    {
                        ESP_LOGE(TAG,
                                 "state_task 停止前最终 BLE 真值应用失败，错误=0x%x",
                                 (unsigned)final_error);
                        if (final_error == ESP_ERR_TIMEOUT)
                        {
                            taskYIELD();
                        }
                    }
                } while (final_error == ESP_ERR_TIMEOUT);
                if (final_error == ESP_OK)
                {
                    observe_applied_update(&final_update);
                }
                else
                {
                    ESP_LOGE(TAG,
                             "state_task 停止前最终 BLE 真值存在不可恢复错误，已拒绝无限重试");
                }
            }
            watch_modem_update_t final_modem_update = {0};
            if (stop_and_take_modem_mailbox(&final_modem_update))
            {
                const state_service_update_t final_update = {
                    .type = STATE_SERVICE_UPDATE_MODEM,
                    .payload.modem = final_modem_update,
                };
                esp_err_t final_error = apply_update_with_retry(&final_update);
                if (final_error == ESP_ERR_TIMEOUT)
                {
                    (void)restore_modem_mailbox(&final_update.payload.modem);
                    ESP_LOGE(TAG, "state_task 停止前最终 modem 真值应用失败");
                }
            }
            watch_gps_update_t final_gps_update = {0};
            if (stop_and_take_gps_mailbox(&final_gps_update))
            {
                const state_service_update_t final_update = {
                    .type = STATE_SERVICE_UPDATE_GPS,
                    .payload.gps = final_gps_update,
                };
                const esp_err_t final_error = apply_update_with_retry(&final_update);
                if (final_error == ESP_ERR_TIMEOUT)
                {
                    (void)restore_gps_mailbox(&final_update.payload.gps);
                    ESP_LOGE(TAG, "state_task 停止前最终 GPS 真值应用失败");
                }
            }
            watch_telemetry_update_t final_telemetry_update = {0};
            if (stop_and_take_telemetry_mailbox(&final_telemetry_update))
            {
                const state_service_update_t final_update = {
                    .type = STATE_SERVICE_UPDATE_TELEMETRY,
                    .payload.telemetry = final_telemetry_update,
                };
                const esp_err_t final_error =
                    apply_update_with_retry(&final_update);
                if (final_error == ESP_ERR_TIMEOUT)
                {
                    (void)restore_telemetry_mailbox(
                        &final_update.payload.telemetry);
                    ESP_LOGE(TAG,
                             "state_task 停止前 telemetry 终态应用失败，稳定码=CLOUD_STATE_APPLY_FAILED");
                }
            }
            finish_run(queue);
            return;
        }
        prefer_config_mailbox = update.type != STATE_SERVICE_UPDATE_CONFIG;
        prefer_ble_mailbox = update.type != STATE_SERVICE_UPDATE_BLE;
        prefer_modem_mailbox = update.type != STATE_SERVICE_UPDATE_MODEM;
        prefer_gps_mailbox = update.type != STATE_SERVICE_UPDATE_GPS;
        esp_err_t err = apply_update_with_retry(&update);
        if (err != ESP_OK)
        {
            if (update.type == STATE_SERVICE_UPDATE_BLE)
            {
                restore_ble_mailbox_if_empty(&update.payload.ble);
            }
            else if (update.type == STATE_SERVICE_UPDATE_MODEM && err == ESP_ERR_TIMEOUT)
            {
                (void)restore_modem_mailbox(&update.payload.modem);
            }
            else if (update.type == STATE_SERVICE_UPDATE_CONFIG && err == ESP_ERR_TIMEOUT)
            {
                (void)restore_config_mailbox(&update.payload.config);
            }
            else if (update.type == STATE_SERVICE_UPDATE_GPS && err == ESP_ERR_TIMEOUT)
            {
                (void)restore_gps_mailbox(&update.payload.gps);
            }
            else if (update.type == STATE_SERVICE_UPDATE_TELEMETRY &&
                     err == ESP_ERR_TIMEOUT)
            {
                (void)restore_telemetry_mailbox(&update.payload.telemetry);
            }
            const UBaseType_t stack_free = uxTaskGetStackHighWaterMark(NULL);
            ESP_LOGE(TAG,
                     "类型化状态快照更新失败，类型=%d，错误=0x%x，state 栈余量=%u 字节",
                     (int)update.type,
                     (unsigned)err,
                     (unsigned)stack_free);
        }
        else
        {
            /* 只有 watch_state 已成功 apply 后才记录本次 typed 更新。 */
            observe_applied_update(&update);
        }
        acknowledge_update(&update, err);
    }
}

static esp_err_t apply_update(const state_service_update_t *update)
{
    if (update->type == STATE_SERVICE_UPDATE_BATTERY)
    {
        return watch_state_apply_battery_update(
            &update->payload.battery,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_POWER)
    {
        return watch_state_apply_power_update(
            &update->payload.power,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_AUDIO)
    {
        return watch_state_apply_audio_update(
            &update->payload.audio,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_BLE)
    {
        return watch_state_apply_ble_update(
            &update->payload.ble,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_CONFIG)
    {
        return watch_state_apply_config_update(
            &update->payload.config,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_MODEM)
    {
        return watch_state_apply_modem_update(
            &update->payload.modem,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_GPS)
    {
        return watch_state_apply_gps_update(
            &update->payload.gps,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_CONTROL)
    {
        return watch_state_apply_control_update(
            &update->payload.control,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_CLOUD)
    {
        return watch_state_apply_cloud_update(
            &update->payload.cloud,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_TELEMETRY)
    {
        return watch_state_apply_telemetry_update(
            &update->payload.telemetry,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_CLOUD_OFFSET_PERSIST)
    {
        return config_service_cloud_offset_write(
            update->payload.cloud_offset_ms,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_SELFTEST_BEGIN)
    {
        return watch_state_apply_selftest_begin(
            &update->payload.selftest_begin,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_SELFTEST_ITEM)
    {
        return watch_state_apply_selftest_item(
            &update->payload.selftest_item,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_SELFTEST_FINISH)
    {
        return watch_state_apply_selftest_finish(
            &update->payload.selftest_finish,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_SELFTEST_RETRY_BEGIN)
    {
        return watch_state_apply_selftest_retry_begin(
            &update->payload.selftest_retry_begin,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_SELFTEST_RETRY_FINISH)
    {
        return watch_state_apply_selftest_retry_finish(
            &update->payload.selftest_retry_finish,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    if (update->type == STATE_SERVICE_UPDATE_TAP_GATE)
    {
        return watch_state_apply_tap_gate_update(
            &update->payload.tap_gate,
            pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    }
    return ESP_ERR_INVALID_ARG;
}

static bool control_update_is_valid(const watch_control_update_t *update)
{
    return update != NULL && update->control_update_sequence != 0U &&
           update->rental_phase >= WATCH_RENTAL_PHASE_WAITING_AUTH &&
           update->rental_phase <= WATCH_RENTAL_PHASE_RETRY_WAIT &&
           update->control_kind >= WATCH_CONTROL_KIND_NONE &&
           update->control_kind <= WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY &&
           update->control_phase >= WATCH_CONTROL_PHASE_IDLE &&
           update->control_phase <= WATCH_CONTROL_PHASE_POWEROFF_PENDING &&
           update->control_last_result >= WATCH_CONTROL_RESULT_NONE &&
           update->control_last_result <= WATCH_CONTROL_RESULT_POWEROFF_TERMINAL &&
           update->poweroff_result >= WATCH_POWEROFF_RESULT_NONE &&
           update->poweroff_result <=
               WATCH_POWEROFF_RESULT_FAILED_RECONNECTED &&
           control_target_is_valid(update->control_kind,
                                   update->control_target) &&
           update->block_reason >= WATCH_CONTROL_BLOCK_NONE &&
           update->block_reason <= WATCH_CONTROL_BLOCK_RETRY_EXHAUSTED &&
           update->last_rental_result >= WATCH_RENTAL_RESULT_NONE &&
           update->last_rental_result <= WATCH_RENTAL_RESULT_CONFIG_ERROR &&
           memchr(update->control_error_code,
                  '\0',
                  sizeof(update->control_error_code)) != NULL &&
           (strncmp(update->control_error_code, "CLOUD_", 6U) == 0 ||
            strncmp(update->control_error_code, "BLE_", 4U) == 0);
}

static bool control_target_is_valid(watch_control_kind_t kind, uint8_t target)
{
    switch (kind)
    {
    case WATCH_CONTROL_KIND_NONE:
        return target == 0U;
    case WATCH_CONTROL_KIND_MOTOR_ENABLE:
        return target <= 1U;
    case WATCH_CONTROL_KIND_GEAR:
        return target >= WATCH_EXOSKELETON_GEAR_MIN &&
               target <= WATCH_EXOSKELETON_GEAR_MAX;
    case WATCH_CONTROL_KIND_SCENE_MODE:
        return target >= WATCH_EXOSKELETON_SCENE_MODE_MIN &&
               target <= WATCH_EXOSKELETON_SCENE_MODE_MAX;
    case WATCH_CONTROL_KIND_POWEROFF:
        return target == 1U;
    case WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY:
        return target == 10U || target == 11U;
    default:
        return false;
    }
}

static void wake_owner(void)
{
    TaskHandle_t owner = atomic_load(&s_owner_task);
    if (owner != NULL && owner != xTaskGetCurrentTaskHandle())
    {
        xTaskNotifyGive(owner);
    }
}

static esp_err_t publish_queue_update(QueueHandle_t queue,
                                      const state_service_update_t *update,
                                      TickType_t timeout_ticks)
{
    if (xQueueSend(queue, update, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    wake_owner();
    return ESP_OK;
}

static esp_err_t apply_update_with_retry(const state_service_update_t *update)
{
    esp_err_t err = ESP_ERR_TIMEOUT;
    for (uint32_t attempt = 0U;
         attempt < STATE_SERVICE_APPLY_RETRY_COUNT;
         ++attempt)
    {
        taskYIELD();
        err = apply_update(update);
        if (err == ESP_OK ||
            (err != ESP_ERR_TIMEOUT &&
             update->type != STATE_SERVICE_UPDATE_CLOUD_OFFSET_PERSIST))
        {
            break;
        }
    }
    return err;
}

static esp_err_t publish_terminal_update(state_service_update_t *update,
                                         TickType_t timeout_ticks)
{
    QueueHandle_t queue = legbot_state_service_queue();
    if (queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    if (current_task == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t stale_notification = 0U;
    while (xTaskNotifyWait(UINT32_MAX,
                           UINT32_MAX,
                           &stale_notification,
                           0) == pdTRUE)
    {
    }
    update->apply_ack_task = current_task;
    update->apply_ack_id = next_apply_ack_id();
    for (uint32_t attempt = 0; attempt < STATE_SERVICE_TERMINAL_PUBLISH_RETRY_COUNT; ++attempt)
    {
        if (publish_queue_update(queue, update, timeout_ticks) == ESP_OK)
        {
            return wait_for_apply_ack(update->apply_ack_id);
        }
        taskYIELD();
    }
    ESP_LOGE(TAG,
             "需要 ACK 的更新有界重试后仍未入队，类型=%d",
             (int)update->type);
    return ESP_ERR_TIMEOUT;
}

static uint32_t next_apply_ack_id(void)
{
    uint32_t ack_id = 0U;
    do
    {
        ack_id = (atomic_fetch_add(&s_next_apply_ack_id, 1U) + 1U) &
                 STATE_SERVICE_ACK_ID_MASK;
    } while (ack_id == 0U);
    return ack_id;
}

static esp_err_t wait_for_apply_ack(uint32_t ack_id)
{
    const TickType_t started_at = xTaskGetTickCount();
    const TickType_t budget = pdMS_TO_TICKS(STATE_SERVICE_APPLY_ACK_TIMEOUT_MS);
    for (;;)
    {
        const TickType_t elapsed = xTaskGetTickCount() - started_at;
        if (elapsed >= budget)
        {
            return ESP_ERR_TIMEOUT;
        }
        uint32_t notification = 0U;
        if (xTaskNotifyWait(0,
                            UINT32_MAX,
                            &notification,
                            budget - elapsed) != pdTRUE)
        {
            return ESP_ERR_TIMEOUT;
        }
        if ((notification & STATE_SERVICE_ACK_ID_MASK) != ack_id)
        {
            continue;
        }
        return (notification & STATE_SERVICE_ACK_FAILURE_BIT) == 0U
                   ? ESP_OK
                   : ESP_FAIL;
    }
}

static void acknowledge_update(const state_service_update_t *update, esp_err_t apply_error)
{
    if (update->apply_ack_task == NULL || update->apply_ack_id == 0U)
    {
        return;
    }
    const uint32_t notification = update->apply_ack_id |
                                  (apply_error == ESP_OK
                                       ? 0U
                                       : STATE_SERVICE_ACK_FAILURE_BIT);
    if (xTaskNotify(update->apply_ack_task,
                    notification,
                    eSetValueWithOverwrite) != pdPASS)
    {
        ESP_LOGE(TAG,
                 "自检 terminal apply ACK 发送失败，ack_id=%lu",
                 (unsigned long)update->apply_ack_id);
    }
}

/**
 * @brief 记录一次已成功 apply 的类型化状态更新
 * @details Story 1.1 没有 UI、音频或充电门控消费者，因此这里只做可追溯观测，
 *          不向任何旧服务投递唤醒或刷新信号。
 * @param update 已成功 apply 的 typed 更新
 */
static void observe_applied_update(const state_service_update_t *update)
{
    uint32_t run_id = 0U;
    if (update->type == STATE_SERVICE_UPDATE_SELFTEST_BEGIN)
    {
        run_id = update->payload.selftest_begin.run_id;
    }
    else if (update->type == STATE_SERVICE_UPDATE_SELFTEST_ITEM)
    {
        run_id = update->payload.selftest_item.run_id;
    }
    else if (update->type == STATE_SERVICE_UPDATE_SELFTEST_FINISH)
    {
        run_id = update->payload.selftest_finish.run_id;
    }
    else if (update->type == STATE_SERVICE_UPDATE_SELFTEST_RETRY_BEGIN)
    {
        run_id = update->payload.selftest_retry_begin.run_id;
    }
    else if (update->type == STATE_SERVICE_UPDATE_SELFTEST_RETRY_FINISH)
    {
        run_id = update->payload.selftest_retry_finish.run_id;
    }
    else
    {
        return;
    }
    ESP_LOGD(TAG,
             "自检快照已应用：run_id=%lu；当前无 UI/电源消费者需要唤醒",
             (unsigned long)run_id);
}

static void finish_run(QueueHandle_t queue)
{
    atomic_store(&s_owner_task, NULL);
    (void)xQueueReset(queue);
}

static const char *canonical_power_error_code(const char *error_code)
{
    if (strcmp(error_code, STATE_SERVICE_BATTERY_OK_CODE) == 0)
    {
        return STATE_SERVICE_BATTERY_OK_CODE;
    }
    if (strcmp(error_code, STATE_SERVICE_BATTERY_UNAVAILABLE_CODE) == 0)
    {
        return STATE_SERVICE_BATTERY_UNAVAILABLE_CODE;
    }
    return NULL;
}

static const char *canonical_driver_error_code(const char *error_code)
{
    static const char *const stable_codes[] = {
        CW2015_ERROR_OK,
        CW2015_ERROR_NOT_READY,
        CW2015_ERROR_DEVICE_MISSING,
        CW2015_ERROR_BUS_BUSY,
        CW2015_ERROR_INIT_FAILED,
        CW2015_ERROR_READ_FAILED,
    };
    for (size_t index = 0; index < sizeof(stable_codes) / sizeof(stable_codes[0]); ++index)
    {
        if (strcmp(error_code, stable_codes[index]) == 0)
        {
            return stable_codes[index];
        }
    }
    return NULL;
}

static bool canonical_mac_is_valid(
    const char mac[WATCH_BLE_MAC_CAPACITY])
{
    if (mac == NULL ||
        strnlen(mac, WATCH_BLE_MAC_CAPACITY) != WATCH_BLE_MAC_CAPACITY - 1U)
    {
        return false;
    }
    for (size_t index = 0U; index < WATCH_BLE_MAC_CAPACITY - 1U; ++index)
    {
        if ((index + 1U) % 3U == 0U)
        {
            if (mac[index] != ':')
            {
                return false;
            }
        }
        else if (!isdigit((unsigned char)mac[index]) &&
                 (mac[index] < 'A' || mac[index] > 'F'))
        {
            return false;
        }
    }
    return true;
}

static bool lock_ble_mailbox(void)
{
    for (uint32_t attempt = 0U;
         attempt < STATE_SERVICE_BLE_MAILBOX_LOCK_ATTEMPTS;
         ++attempt)
    {
        if (!atomic_flag_test_and_set(&s_ble_mailbox_lock))
        {
            return true;
        }
        taskYIELD();
    }
    return false;
}

static esp_err_t store_ble_mailbox(const watch_ble_update_t *update)
{
    if (update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!lock_ble_mailbox())
    {
        return ESP_ERR_TIMEOUT;
    }
    s_ble_mailbox = *update;
    s_ble_mailbox_dirty = true;
    atomic_flag_clear(&s_ble_mailbox_lock);
    return ESP_OK;
}

static bool take_ble_mailbox(watch_ble_update_t *update)
{
    if (update == NULL || !lock_ble_mailbox())
    {
        return false;
    }
    const bool dirty = s_ble_mailbox_dirty;
    if (dirty)
    {
        *update = s_ble_mailbox;
        s_ble_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_ble_mailbox_lock);
    return dirty;
}

static void restore_ble_mailbox_if_empty(const watch_ble_update_t *update)
{
    if (update == NULL)
    {
        return;
    }
    while (!lock_ble_mailbox())
    {
        taskYIELD();
    }
    if (!s_ble_mailbox_dirty)
    {
        s_ble_mailbox = *update;
        s_ble_mailbox_dirty = true;
    }
    atomic_flag_clear(&s_ble_mailbox_lock);
}

static bool stop_and_take_ble_mailbox(watch_ble_update_t *update)
{
    while (!lock_ble_mailbox())
    {
        taskYIELD();
    }
    const bool dirty = s_ble_mailbox_dirty && update != NULL;
    if (dirty)
    {
        *update = s_ble_mailbox;
        s_ble_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_ble_mailbox_lock);
    return dirty;
}

static bool lock_config_mailbox(void)
{
    for (uint32_t attempt = 0U;
         attempt < STATE_SERVICE_CONFIG_MAILBOX_LOCK_ATTEMPTS;
         ++attempt)
    {
        if (!atomic_flag_test_and_set(&s_config_mailbox_lock))
        {
            return true;
        }
        taskYIELD();
    }
    return false;
}

static esp_err_t store_config_mailbox(const watch_config_update_t *update)
{
    if (update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!lock_config_mailbox())
    {
        return ESP_ERR_TIMEOUT;
    }
    const esp_err_t err = watch_config_update_store_latest(&s_config_mailbox,
                                                           &s_config_mailbox_dirty,
                                                           update);
    atomic_flag_clear(&s_config_mailbox_lock);
    return err;
}

static bool take_config_mailbox(watch_config_update_t *update)
{
    if (update == NULL || !lock_config_mailbox())
    {
        return false;
    }
    const bool dirty = s_config_mailbox_dirty;
    if (dirty)
    {
        *update = s_config_mailbox;
        s_config_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_config_mailbox_lock);
    return dirty;
}

static bool restore_config_mailbox(const watch_config_update_t *update)
{
    if (update == NULL || !lock_config_mailbox())
    {
        return false;
    }
    (void)watch_config_update_store_latest(&s_config_mailbox,
                                           &s_config_mailbox_dirty,
                                           update);
    atomic_flag_clear(&s_config_mailbox_lock);
    return true;
}

static bool stop_and_take_config_mailbox(watch_config_update_t *update)
{
    if (!lock_config_mailbox())
    {
        return false;
    }
    const bool dirty = s_config_mailbox_dirty && update != NULL;
    if (dirty)
    {
        *update = s_config_mailbox;
        s_config_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_config_mailbox_lock);
    return dirty;
}

static bool lock_modem_mailbox(void)
{
    for (uint32_t attempt = 0U;
         attempt < STATE_SERVICE_MODEM_MAILBOX_LOCK_ATTEMPTS;
         ++attempt)
    {
        if (!atomic_flag_test_and_set(&s_modem_mailbox_lock))
        {
            return true;
        }
        taskYIELD();
    }
    return false;
}

static esp_err_t store_modem_mailbox(const watch_modem_update_t *update)
{
    if (!lock_modem_mailbox())
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!s_modem_mailbox_dirty ||
        update->modem_update_sequence > s_modem_mailbox.modem_update_sequence)
    {
        s_modem_mailbox = *update;
        s_modem_mailbox_dirty = true;
    }
    atomic_flag_clear(&s_modem_mailbox_lock);
    return ESP_OK;
}

static bool take_modem_mailbox(watch_modem_update_t *update)
{
    if (update == NULL || !lock_modem_mailbox())
    {
        return false;
    }
    const bool dirty = s_modem_mailbox_dirty;
    if (dirty)
    {
        *update = s_modem_mailbox;
        s_modem_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_modem_mailbox_lock);
    return dirty;
}

static bool restore_modem_mailbox(const watch_modem_update_t *update)
{
    if (update == NULL || !lock_modem_mailbox())
    {
        return false;
    }
    if (!s_modem_mailbox_dirty ||
        update->modem_update_sequence > s_modem_mailbox.modem_update_sequence)
    {
        s_modem_mailbox = *update;
        s_modem_mailbox_dirty = true;
    }
    atomic_flag_clear(&s_modem_mailbox_lock);
    return true;
}

static bool stop_and_take_modem_mailbox(watch_modem_update_t *update)
{
    if (!lock_modem_mailbox())
    {
        return false;
    }
    const bool dirty = s_modem_mailbox_dirty && update != NULL;
    if (dirty)
    {
        *update = s_modem_mailbox;
        s_modem_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_modem_mailbox_lock);
    return dirty;
}

static bool lock_gps_mailbox(void)
{
    for (uint32_t attempt = 0U;
         attempt < STATE_SERVICE_GPS_MAILBOX_LOCK_ATTEMPTS;
         ++attempt)
    {
        if (!atomic_flag_test_and_set(&s_gps_mailbox_lock))
        {
            return true;
        }
        taskYIELD();
    }
    return false;
}

static esp_err_t store_gps_mailbox(const watch_gps_update_t *update)
{
    if (!lock_gps_mailbox())
    {
        return ESP_ERR_TIMEOUT;
    }
    const esp_err_t error = watch_gps_update_store_latest(&s_gps_mailbox,
                                                          &s_gps_mailbox_dirty,
                                                          update);
    atomic_flag_clear(&s_gps_mailbox_lock);
    return error;
}

static bool take_gps_mailbox(watch_gps_update_t *update)
{
    if (update == NULL || !lock_gps_mailbox())
    {
        return false;
    }
    const bool dirty = s_gps_mailbox_dirty;
    if (dirty)
    {
        *update = s_gps_mailbox;
        s_gps_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_gps_mailbox_lock);
    return dirty;
}

static bool restore_gps_mailbox(const watch_gps_update_t *update)
{
    if (update == NULL || !lock_gps_mailbox())
    {
        return false;
    }
    const esp_err_t error = watch_gps_update_store_latest(&s_gps_mailbox,
                                                          &s_gps_mailbox_dirty,
                                                          update);
    atomic_flag_clear(&s_gps_mailbox_lock);
    return error == ESP_OK || error == ESP_ERR_INVALID_STATE;
}

static bool stop_and_take_gps_mailbox(watch_gps_update_t *update)
{
    if (!lock_gps_mailbox())
    {
        return false;
    }
    const bool dirty = s_gps_mailbox_dirty && update != NULL;
    if (dirty)
    {
        *update = s_gps_mailbox;
        s_gps_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_gps_mailbox_lock);
    return dirty;
}

static bool lock_telemetry_mailbox(void)
{
    for (uint32_t attempt = 0U;
         attempt < STATE_SERVICE_TELEMETRY_MAILBOX_LOCK_ATTEMPTS;
         ++attempt)
    {
        if (!atomic_flag_test_and_set(&s_telemetry_mailbox_lock))
        {
            return true;
        }
        taskYIELD();
    }
    return false;
}

static esp_err_t store_telemetry_mailbox(
    const watch_telemetry_update_t *update)
{
    if (update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!lock_telemetry_mailbox())
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_telemetry_mailbox_dirty)
    {
        atomic_flag_clear(&s_telemetry_mailbox_lock);
        return ESP_ERR_TIMEOUT;
    }
    s_telemetry_mailbox = *update;
    s_telemetry_mailbox_dirty = true;
    atomic_flag_clear(&s_telemetry_mailbox_lock);
    return ESP_OK;
}

static bool take_telemetry_mailbox(watch_telemetry_update_t *update)
{
    if (update == NULL || !lock_telemetry_mailbox())
    {
        return false;
    }
    const bool dirty = s_telemetry_mailbox_dirty;
    if (dirty)
    {
        *update = s_telemetry_mailbox;
        s_telemetry_mailbox_dirty = false;
    }
    atomic_flag_clear(&s_telemetry_mailbox_lock);
    return dirty;
}

static bool restore_telemetry_mailbox(
    const watch_telemetry_update_t *update)
{
    if (update == NULL || !lock_telemetry_mailbox())
    {
        return false;
    }
    if (!s_telemetry_mailbox_dirty)
    {
        s_telemetry_mailbox = *update;
        s_telemetry_mailbox_dirty = true;
    }
    atomic_flag_clear(&s_telemetry_mailbox_lock);
    return true;
}

static bool stop_and_take_telemetry_mailbox(
    watch_telemetry_update_t *update)
{
    return take_telemetry_mailbox(update);
}
