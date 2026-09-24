/**
 * @file     watch_state.c
 * @brief    手环运行状态存储实现
 * @details  使用 FreeRTOS 互斥锁保护全局快照，并校验授权与普通控制域的独立状态不变量。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "app_state.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "freertos/semphr.h"

/** 电量尚未读取时的稳定状态码。 */
#define WATCH_STATE_BATTERY_NOT_UPDATED "POWER_BATTERY_NOT_UPDATED"
/** 电量驱动尚未读取时的稳定状态码。 */
#define WATCH_STATE_BATTERY_DRIVER_NOT_UPDATED "DRV_CW2015_NOT_READY"
/** BLE 服务尚未发布状态时的稳定码。 */
#define WATCH_STATE_BLE_NOT_STARTED "BLE_NOT_STARTED"
/** 配置 initial truth 尚未投递时的稳定状态码。 */
#define WATCH_STATE_CLOUD_NOT_PUBLISHED "CLOUD_CONFIG_NOT_PUBLISHED"
/** modem owner 尚未发布状态时的稳定码。 */
#define WATCH_STATE_MODEM_NOT_STARTED "MODEM_NOT_STARTED"
/** GPS owner 尚未启动时的稳定码。 */
#define WATCH_STATE_GPS_OFF "GPS_OFF"
/** 首次解锁尚未取得授权时的稳定码。 */
#define WATCH_STATE_CONTROL_AUTH_REQUIRED "CLOUD_AUTH_REQUIRED"
/** 活动控制事务因 BLE 链路变化而取消的稳定码。 */
#define WATCH_STATE_CONTROL_LINK_CHANGED "BLE_UNLOCK_LINK_CHANGED"
/** 活动控制事务因状态过期而取消的稳定码。 */
#define WATCH_STATE_CONTROL_STATUS_STALE "BLE_STATUS_STALE"
/** 仅供旧版 host fixture 兼容；固件构建固定关闭无身份活动状态。 */
#ifdef TEST_EXPORT
#define WATCH_STATE_LEGACY_HOST_FIXTURE_ALLOWED 1
#else
#define WATCH_STATE_LEGACY_HOST_FIXTURE_ALLOWED 0
#endif

/** 全局状态互斥锁，保护 s_state 的并发读写。 */
static SemaphoreHandle_t s_state_mutex;

/** 当前运行状态快照。 */
static watch_state_snapshot_t s_state;

static esp_err_t copy_error_code(char *destination,
                                 size_t capacity,
                                 const char *source);
static bool canonical_mac_is_valid(
    const char mac[WATCH_BLE_MAC_CAPACITY]);
static const char *expected_cloud_error_code(const watch_config_update_t *update);
static uint8_t config_status_safety_rank(watch_cloud_config_status_t status);
static bool control_update_is_consistent(const watch_control_update_t *update);
static bool sequence_is_after(uint32_t candidate, uint32_t baseline);
static bool ordinary_control_target_is_valid(watch_control_kind_t kind,
                                             uint8_t target);
static bool control_block_result_is_consistent(
    watch_control_block_reason_t block_reason,
    watch_rental_result_t result);
static bool rental_result_can_retry(watch_rental_result_t result);
static bool exoskeleton_state_is_valid(const watch_exoskeleton_state_t *state);
static bool version_matches_model(const char *version,
                                  const char *prefix);
static bool consume_decimal_component(const char **cursor);
static bool modem_ip_is_valid(const char *text);
static bool modem_ipv4_is_valid(const char *text);
static bool modem_ipv6_is_valid(const char *text);
static void reset_selftest_summary(uint32_t run_id,
                                   selftest_item_id_t first_item,
                                   TickType_t started_at_ticks);

esp_err_t watch_state_init(void)
{
    if (s_state_mutex != NULL)
    {
        return ESP_OK;
    }

    s_state_mutex = xSemaphoreCreateMutex();
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    memset(&s_state, 0, sizeof(s_state));
    s_state.boot_ready = true;
    (void)copy_error_code(s_state.battery_error_code,
                          sizeof(s_state.battery_error_code),
                          WATCH_STATE_BATTERY_NOT_UPDATED);
    (void)copy_error_code(s_state.battery_driver_error_code,
                          sizeof(s_state.battery_driver_error_code),
                          WATCH_STATE_BATTERY_DRIVER_NOT_UPDATED);
    (void)copy_error_code(s_state.ble_error_code,
                          sizeof(s_state.ble_error_code),
                          WATCH_STATE_BLE_NOT_STARTED);
    (void)copy_error_code(s_state.ble_diagnostics.last_error_code,
                          sizeof(s_state.ble_diagnostics.last_error_code),
                          WATCH_STATE_BLE_NOT_STARTED);
    (void)copy_error_code(s_state.cloud_error_code,
                          sizeof(s_state.cloud_error_code),
                          WATCH_STATE_CLOUD_NOT_PUBLISHED);
    (void)copy_error_code(s_state.rental_cloud_error_code,
                          sizeof(s_state.rental_cloud_error_code),
                          "CLOUD_RENTAL_NOT_QUERIED");
    (void)copy_error_code(s_state.telemetry_error_code,
                          sizeof(s_state.telemetry_error_code),
                          "CLOUD_TELEMETRY_NOT_SENT");
    (void)copy_error_code(s_state.modem.last_error,
                          sizeof(s_state.modem.last_error),
                          WATCH_STATE_MODEM_NOT_STARTED);
    s_state.gps.status = WATCH_GPS_STATUS_OFF;
    (void)copy_error_code(s_state.gps.error_code,
                          sizeof(s_state.gps.error_code),
                          WATCH_STATE_GPS_OFF);
    s_state.cloud_status = WATCH_CLOUD_UNCONFIGURED;
    s_state.base_url_status = WATCH_CONFIG_VALUE_UNCONFIGURED;
    s_state.apn_mode = WATCH_APN_AUTO;
    s_state.token_status = WATCH_CONFIG_VALUE_UNCONFIGURED;
    s_state.certificate_status = WATCH_CONFIG_VALUE_UNCONFIGURED;
    s_state.config_schema_status = WATCH_CONFIG_SCHEMA_CORRUPT;
    s_state.ble_transaction = WATCH_BLE_TRANSACTION_UNBOUND;
    s_state.audio_state = WATCH_AUDIO_STATE_IDLE;
    s_state.audio_error = WATCH_AUDIO_ERROR_NOT_READY;
    s_state.power_level = WATCH_POWER_LEVEL_NORMAL;
    s_state.screen_state = WATCH_SCREEN_STATE_ON;
    s_state.power_error = WATCH_POWER_ERROR_NONE;
    s_state.rental_phase = WATCH_RENTAL_PHASE_WAITING_AUTH;
    s_state.control_block_reason = WATCH_CONTROL_BLOCK_UNBOUND;
    (void)copy_error_code(s_state.control_error_code,
                          sizeof(s_state.control_error_code),
                          WATCH_STATE_CONTROL_AUTH_REQUIRED);
    reset_selftest_summary(0U, SELFTEST_ITEM_COUNT, 0U);
    s_state.selftest.state = SELFTEST_RUN_IDLE;
    return ESP_OK;
}

watch_exoskeleton_model_t watch_exoskeleton_model_from_version(
    const char *version)
{
    if (version_matches_model(version, "v1.") ||
        version_matches_model(version, "Mini-v1."))
    {
        return WATCH_EXOSKELETON_MODEL_MINI;
    }
    if (version_matches_model(version, "v2.") ||
        version_matches_model(version, "Pro-v2."))
    {
        return WATCH_EXOSKELETON_MODEL_PRO;
    }
    if (version_matches_model(version, "v3.") ||
        version_matches_model(version, "Max-v3."))
    {
        return WATCH_EXOSKELETON_MODEL_MAX;
    }
    if (version_matches_model(version, "v4.") ||
        version_matches_model(version, "MiniY-v4."))
    {
        return WATCH_EXOSKELETON_MODEL_MINI_Y;
    }
    if (version_matches_model(version, "v5.") ||
        version_matches_model(version, "ProY-v5."))
    {
        return WATCH_EXOSKELETON_MODEL_PRO_Y;
    }
    if (version_matches_model(version, "v6.") ||
        version_matches_model(version, "MaxY-v6."))
    {
        return WATCH_EXOSKELETON_MODEL_MAX_Y;
    }
    if (version_matches_model(version, "v7.") ||
        version_matches_model(version, "OldA-v7."))
    {
        return WATCH_EXOSKELETON_MODEL_OLD_A;
    }
    if (version_matches_model(version, "v8.") ||
        version_matches_model(version, "OldC-v8."))
    {
        return WATCH_EXOSKELETON_MODEL_OLD_C;
    }
    if (version_matches_model(version, "v9.") ||
        version_matches_model(version, "Child-v9."))
    {
        return WATCH_EXOSKELETON_MODEL_CHILD;
    }
    return WATCH_EXOSKELETON_MODEL_UNKNOWN;
}

uint8_t watch_exoskeleton_model_max_gear(
    watch_exoskeleton_model_t model)
{
    const watch_exoskeleton_scene_config_t config = watch_exoskeleton_model_scene_config(model);
    return watch_exoskeleton_scene_config_max_gear(config);
}

uint8_t watch_exoskeleton_scene_config_max_gear(
    watch_exoskeleton_scene_config_t config)
{
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_INVALID)
    {
        return 0U;
    }
    return config == WATCH_EXOSKELETON_SCENE_CONFIG_FULL
               ? WATCH_EXOSKELETON_GEAR_MAX
               : WATCH_EXOSKELETON_NON_EXTREME_GEAR_MAX;
}

watch_exoskeleton_scene_config_t watch_exoskeleton_model_scene_config(
    watch_exoskeleton_model_t model)
{
    switch (model)
    {
    case WATCH_EXOSKELETON_MODEL_MINI:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V1_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_PRO:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V2_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_MAX:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V3_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_MINI_Y:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V4_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_PRO_Y:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V5_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_MAX_Y:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V6_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_OLD_A:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V7_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_OLD_C:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V8_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_CHILD:
        return (watch_exoskeleton_scene_config_t)
            WATCH_EXOSKELETON_V9_SCENE_CONFIG;
    case WATCH_EXOSKELETON_MODEL_UNKNOWN:
    default:
        return WATCH_EXOSKELETON_SCENE_CONFIG_INVALID;
    }
}

bool watch_exoskeleton_model_supports_extreme(
    watch_exoskeleton_model_t model)
{
    return watch_exoskeleton_model_scene_config(model) ==
           WATCH_EXOSKELETON_SCENE_CONFIG_FULL;
}

bool watch_exoskeleton_model_has_mode_switch(
    watch_exoskeleton_model_t model)
{
    const watch_exoskeleton_scene_config_t config =
        watch_exoskeleton_model_scene_config(model);
    return config == WATCH_EXOSKELETON_SCENE_CONFIG_FULL ||
           config == WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME ||
           config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_FITNESS ||
           config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP;
}

bool watch_exoskeleton_model_supports_scene_mode(
    watch_exoskeleton_model_t model,
    uint8_t scene_mode)
{
    const watch_exoskeleton_scene_config_t config =
        watch_exoskeleton_model_scene_config(model);
    if (scene_mode < WATCH_EXOSKELETON_SCENE_MODE_MIN ||
        scene_mode > WATCH_EXOSKELETON_SCENE_MODE_MAX ||
        config == WATCH_EXOSKELETON_SCENE_CONFIG_INVALID)
    {
        return false;
    }
    switch (config)
    {
    case WATCH_EXOSKELETON_SCENE_CONFIG_FULL:
        return scene_mode <= 4U;
    case WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME:
        return scene_mode <= 4U && scene_mode != 3U;
    case WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_FITNESS:
        return scene_mode <= 2U;
    case WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_ONLY:
        return scene_mode == 1U;
    case WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP:
        return scene_mode == 1U ||
               scene_mode == WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP;
    case WATCH_EXOSKELETON_SCENE_CONFIG_INVALID:
    default:
        return false;
    }
}

bool watch_exoskeleton_scene_config_supports_mode(
    watch_exoskeleton_scene_config_t config,
    uint8_t scene_mode)
{
    if (!WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(config) || scene_mode < 1U || scene_mode > 5U)
        return false;
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_FULL) return scene_mode <= 4U;
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME) return scene_mode == 1U || scene_mode == 2U || scene_mode == 4U;
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_FITNESS) return scene_mode <= 2U;
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_ONLY) return scene_mode == 1U;
    return scene_mode == 1U || scene_mode == WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP;
}

bool watch_exoskeleton_scene_config_allows_control(
    watch_exoskeleton_scene_config_t config, uint8_t scene_mode)
{
    return config != WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_ONLY &&
           watch_exoskeleton_scene_config_supports_mode(config, scene_mode);
}

bool watch_exoskeleton_model_allows_scene_control(
    watch_exoskeleton_model_t model,
    uint8_t scene_mode)
{
    return watch_exoskeleton_model_has_mode_switch(model) &&
           watch_exoskeleton_model_supports_scene_mode(model, scene_mode);
}

bool watch_exoskeleton_scene_mode_to_protocol(
    watch_exoskeleton_model_t model,
    uint8_t scene_mode,
    uint8_t *protocol_scene_mode)
{
    if (protocol_scene_mode == NULL ||
        !watch_exoskeleton_model_allows_scene_control(model, scene_mode))
    {
        return false;
    }
    const watch_exoskeleton_scene_config_t config =
        watch_exoskeleton_model_scene_config(model);
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME &&
        scene_mode == 4U)
    {
        *protocol_scene_mode = 3U;
        return true;
    }
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP &&
        scene_mode == WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP)
    {
        *protocol_scene_mode = 2U;
        return true;
    }
    *protocol_scene_mode = scene_mode;
    return true;
}

bool watch_exoskeleton_scene_mode_from_protocol(
    watch_exoskeleton_model_t model,
    uint8_t protocol_scene_mode,
    uint8_t *scene_mode)
{
    if (scene_mode == NULL)
    {
        return false;
    }
    return watch_exoskeleton_scene_mode_from_protocol_config(
        watch_exoskeleton_model_scene_config(model), protocol_scene_mode, scene_mode);
}

bool watch_exoskeleton_scene_mode_from_protocol_config(
    watch_exoskeleton_scene_config_t config,
    uint8_t protocol_scene_mode,
    uint8_t *scene_mode)
{
    if (scene_mode == NULL)
    {
        return false;
    }
    switch (config)
    {
    case WATCH_EXOSKELETON_SCENE_CONFIG_FULL:
        if (protocol_scene_mode < 1U || protocol_scene_mode > 4U)
        {
            return false;
        }
        *scene_mode = protocol_scene_mode;
        return true;
    case WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME:
        if (protocol_scene_mode < 1U || protocol_scene_mode > 3U)
        {
            return false;
        }
        *scene_mode = protocol_scene_mode == 3U
                          ? 4U
                          : protocol_scene_mode;
        return true;
    case WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_FITNESS:
        if (protocol_scene_mode < 1U || protocol_scene_mode > 2U)
        {
            return false;
        }
        *scene_mode = protocol_scene_mode;
        return true;
    case WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_ONLY:
        if (protocol_scene_mode != 1U)
        {
            return false;
        }
        *scene_mode = 1U;
        return true;
    case WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP:
        if (protocol_scene_mode < 1U || protocol_scene_mode > 2U)
        {
            return false;
        }
        *scene_mode = protocol_scene_mode == 2U
                          ? WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP
                          : 1U;
        return true;
    case WATCH_EXOSKELETON_SCENE_CONFIG_INVALID:
    default:
        return false;
    }
}

esp_err_t watch_state_apply_update(const watch_state_update_t *update, TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->field == WATCH_STATE_FIELD_BLE_CONNECTED ||
        update->field == WATCH_STATE_FIELD_MODEM_READY ||
        update->field == WATCH_STATE_FIELD_GPS_FIX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* 状态快照是跨服务共享数据，更新前必须取得互斥锁。 */
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    switch (update->field)
    {
    case WATCH_STATE_FIELD_BOOT_READY:
        s_state.boot_ready = update->value != 0;
        break;
    default:
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_tap_gate_update(
    const watch_tap_gate_update_t *update,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->update_sequence == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_state.tap_gate_update_sequence != 0U &&
        update->update_sequence <= s_state.tap_gate_update_sequence)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.tap_gate_update_sequence = update->update_sequence;
    s_state.tap_completed = update->completed;
    s_state.tap_fault_locked = update->fault_locked;
    s_state.tap_queue_full = update->queue_full;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_tap_progress_update(
    const watch_tap_progress_update_t *update,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->update_sequence == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_state.tap_progress_update_sequence != 0U &&
        update->update_sequence <= s_state.tap_progress_update_sequence)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.tap_progress_update_sequence = update->update_sequence;
    s_state.tap_local_total = update->local_total;
    s_state.tap_acked_total = update->acked_total;
    s_state.tap_round_id = update->round_id;
    s_state.tap_round_cursor = update->round_cursor;
    s_state.tap_round_state = update->round_state;
    s_state.tap_pending_completion = update->pending_completion;
    s_state.tap_backlog_count = update->backlog_count;
    s_state.tap_persist_error = update->persist_error;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_feedback_update(
    const watch_feedback_update_t *update,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->update_sequence == 0U ||
        update->volume > 100U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t index = 0; index < (size_t)WATCH_FEEDBACK_CHANNEL_COUNT; ++index)
    {
        const watch_feedback_channel_update_t *channel = &update->channels[index];
        if ((unsigned)channel->channel >= (unsigned)WATCH_FEEDBACK_CHANNEL_COUNT ||
            (unsigned)channel->state > (unsigned)WATCH_FEEDBACK_CHANNEL_FAILED ||
            (unsigned)channel->error > (unsigned)WATCH_FEEDBACK_ERROR_RGB_FAILED ||
            (unsigned)channel->channel != index)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_state.feedback.update_sequence != 0U &&
        update->update_sequence <= s_state.feedback.update_sequence)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.feedback = *update;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_feedback_snapshot(watch_feedback_update_t *snapshot,
                                        TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL || snapshot == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *snapshot = s_state.feedback;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_nav_update(const watch_nav_update_t *update,
                                       TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->update_sequence == 0U ||
        update->volume > 100U ||
        (unsigned)update->active_page >= (unsigned)WATCH_NAV_PAGE_COUNT ||
        (unsigned)update->brightness >= (unsigned)WATCH_BRIGHTNESS_COUNT ||
        (update->timeout_s != 5U && update->timeout_s != 15U &&
         update->timeout_s != 30U) ||
        (unsigned)update->sync_status >= (unsigned)WATCH_SYNC_STATUS_COUNT ||
        (unsigned)update->screen_state >= (unsigned)WATCH_SCREEN_STATE_COUNT ||
        (unsigned)update->last_reason >= (unsigned)WATCH_NAV_REASON_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_state.nav.update_sequence != 0U &&
        update->update_sequence <= s_state.nav.update_sequence)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.nav = *update;
    /* 导航 owner 同时维护正交 screen_state，避免多处私写。 */
    s_state.screen_state = update->screen_state;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_nav_snapshot(watch_nav_update_t *snapshot,
                                   TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL || snapshot == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *snapshot = s_state.nav;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_battery_update(const watch_battery_update_t *update,
                                           TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->power_error_code == NULL ||
        update->driver_error_code == NULL ||
        update->power_level < WATCH_POWER_LEVEL_NORMAL ||
        update->power_level >= WATCH_POWER_LEVEL_COUNT ||
        (update->valid && update->percent > 100U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (strlen(update->power_error_code) >= sizeof(s_state.battery_error_code) ||
        strlen(update->driver_error_code) >= sizeof(s_state.battery_driver_error_code))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    /* 失败更新只改变有效性和诊断，保留最近一次有效百分比及其时间。 */
    s_state.battery_valid = update->valid;
    s_state.battery_last_attempt_ticks = update->attempted_at_ticks;
    (void)copy_error_code(s_state.battery_error_code,
                          sizeof(s_state.battery_error_code),
                          update->power_error_code);
    (void)copy_error_code(s_state.battery_driver_error_code,
                          sizeof(s_state.battery_driver_error_code),
                          update->driver_error_code);
    if (update->valid)
    {
        s_state.battery_percent = update->percent;
        s_state.battery_has_valid_sample = true;
        s_state.battery_last_valid_ticks = update->attempted_at_ticks;
        s_state.power_level = update->power_level;
    }

    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_power_update(const watch_power_update_t *update,
                                         TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL ||
        update->screen_state < WATCH_SCREEN_STATE_ON ||
        update->screen_state >= WATCH_SCREEN_STATE_COUNT ||
        update->transition_sequence == 0U ||
        update->error < WATCH_POWER_ERROR_NONE ||
        update->error >= WATCH_POWER_ERROR_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!sequence_is_after(update->transition_sequence,
                           s_state.screen_transition_sequence))
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    s_state.screen_state = update->screen_state;
    s_state.last_local_activity_ms = update->last_local_activity_ms;
    s_state.screen_transition_sequence = update->transition_sequence;
    s_state.power_error = update->error;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_audio_update(const watch_audio_update_t *update,
                                         TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->state < WATCH_AUDIO_STATE_IDLE ||
        update->state > WATCH_AUDIO_STATE_FAILED ||
        update->error < WATCH_AUDIO_ERROR_NONE ||
        update->error > WATCH_AUDIO_ERROR_PA_FAILED)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    s_state.audio_resource_available = update->resource_available;
    s_state.audio_state = update->state;
    s_state.audio_error = update->error;
    s_state.audio_updated_at_ticks = update->updated_at_ticks;
    ++s_state.audio_update_sequence;

    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_modem_update(const watch_modem_update_t *update,
                                         TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (watch_modem_update_validate(update) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (update->modem_update_sequence <= s_state.modem.modem_update_sequence)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.modem = *update;
    s_state.modem_ready = update->pdp_status == WATCH_MODEM_PDP_UP &&
                          (update->lifecycle == WATCH_MODEM_LIFECYCLE_HTTPS_READY ||
                           update->lifecycle == WATCH_MODEM_LIFECYCLE_REQUEST_ACTIVE);
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_modem_update_validate(const watch_modem_update_t *update)
{
    if (update == NULL || update->modem_update_sequence == 0U ||
        update->lifecycle < WATCH_MODEM_LIFECYCLE_BOOTING ||
        update->lifecycle > WATCH_MODEM_LIFECYCLE_BACKOFF ||
        update->sim_status < WATCH_MODEM_SIM_UNKNOWN ||
        update->sim_status > WATCH_MODEM_SIM_READY ||
        update->signal_status < WATCH_MODEM_SIGNAL_UNKNOWN ||
        update->signal_status > WATCH_MODEM_SIGNAL_PRESENT ||
        update->registration < WATCH_MODEM_REGISTRATION_UNKNOWN ||
        update->registration > WATCH_MODEM_REGISTRATION_DENIED ||
        update->pdp_status < WATCH_MODEM_PDP_DOWN ||
        update->pdp_status > WATCH_MODEM_PDP_UP ||
        update->request_status < WATCH_MODEM_REQUEST_IDLE ||
        update->request_status > WATCH_MODEM_REQUEST_OK ||
        memchr(update->imei, '\0', sizeof(update->imei)) == NULL ||
        memchr(update->serial_number, '\0', sizeof(update->serial_number)) == NULL ||
        memchr(update->operator_name, '\0', sizeof(update->operator_name)) == NULL ||
        memchr(update->ip_address, '\0', sizeof(update->ip_address)) == NULL ||
        memchr(update->last_error, '\0', sizeof(update->last_error)) == NULL ||
        strncmp(update->last_error, "MODEM_", 6U) != 0 ||
        (update->pdp_status == WATCH_MODEM_PDP_UP &&
         !modem_ip_is_valid(update->ip_address)) ||
        (update->pdp_status == WATCH_MODEM_PDP_DOWN && update->ip_address[0] != '\0') ||
        (update->lifecycle == WATCH_MODEM_LIFECYCLE_REQUEST_ACTIVE &&
         update->request_status != WATCH_MODEM_REQUEST_ACTIVE) ||
        (update->lifecycle != WATCH_MODEM_LIFECYCLE_REQUEST_ACTIVE &&
         update->request_status == WATCH_MODEM_REQUEST_ACTIVE) ||
        ((update->lifecycle == WATCH_MODEM_LIFECYCLE_PDP_READY ||
          update->lifecycle == WATCH_MODEM_LIFECYCLE_HTTPS_READY ||
          update->lifecycle == WATCH_MODEM_LIFECYCLE_REQUEST_ACTIVE) &&
         (update->pdp_status != WATCH_MODEM_PDP_UP ||
          (update->registration != WATCH_MODEM_REGISTRATION_HOME &&
           update->registration != WATCH_MODEM_REGISTRATION_ROAMING) ||
          update->sim_status != WATCH_MODEM_SIM_READY)) ||
        ((update->lifecycle == WATCH_MODEM_LIFECYCLE_BOOTING ||
          update->lifecycle == WATCH_MODEM_LIFECYCLE_SIM_CHECK ||
          update->lifecycle == WATCH_MODEM_LIFECYCLE_REGISTERING ||
          update->lifecycle == WATCH_MODEM_LIFECYCLE_RECOVERING ||
          update->lifecycle == WATCH_MODEM_LIFECYCLE_BACKOFF) &&
         update->pdp_status != WATCH_MODEM_PDP_DOWN) ||
        (update->sim_status != WATCH_MODEM_SIM_READY &&
         update->pdp_status == WATCH_MODEM_PDP_UP) ||
        ((update->sim_status == WATCH_MODEM_SIM_MISSING ||
          update->sim_status == WATCH_MODEM_SIM_NOT_READY) &&
         (update->registration == WATCH_MODEM_REGISTRATION_HOME ||
          update->registration == WATCH_MODEM_REGISTRATION_ROAMING)) ||
        (update->registration != WATCH_MODEM_REGISTRATION_HOME &&
         update->registration != WATCH_MODEM_REGISTRATION_ROAMING &&
         update->pdp_status == WATCH_MODEM_PDP_UP) ||
        !((update->csq_rssi >= 0 && update->csq_rssi <= 31) ||
          update->csq_rssi == 99) ||
        !((update->csq_ber >= 0 && update->csq_ber <= 7) ||
          update->csq_ber == 99) ||
        !((update->cesq_rsrq >= 0 && update->cesq_rsrq <= 34) ||
          update->cesq_rsrq == 255) ||
        !((update->cesq_rsrp >= 0 && update->cesq_rsrp <= 97) ||
          update->cesq_rsrp == 255))
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

bool watch_modem_network_connected(
    const watch_modem_snapshot_t *snapshot)
{
    return snapshot != NULL &&
           snapshot->modem_update_sequence != 0U &&
           snapshot->sim_status == WATCH_MODEM_SIM_READY &&
           (snapshot->registration == WATCH_MODEM_REGISTRATION_HOME ||
            snapshot->registration == WATCH_MODEM_REGISTRATION_ROAMING) &&
           snapshot->pdp_status == WATCH_MODEM_PDP_UP &&
           snapshot->ip_address[0] != '\0';
}

esp_err_t watch_gps_update_validate(const watch_gps_update_t *update)
{
    if (update == NULL || update->update_sequence == 0U ||
        update->status < WATCH_GPS_STATUS_OFF ||
        update->status > WATCH_GPS_STATUS_UNAVAILABLE ||
        update->acquisition_state < WATCH_GPS_ACQUISITION_STANDBY ||
        update->acquisition_state > WATCH_GPS_ACQUISITION_UNAVAILABLE ||
        update->purpose < WATCH_GPS_PURPOSE_TIME_SYNC ||
        update->purpose > WATCH_GPS_PURPOSE_SELFTEST ||
        update->trigger < WATCH_GPS_TRIGGER_BOOT ||
        update->trigger > WATCH_GPS_TRIGGER_MOTION ||
        update->backoff_stage > 2U ||
        update->time_source < WATCH_TIME_SOURCE_NONE ||
        update->time_source > WATCH_TIME_SOURCE_CLOUD ||
        update->time_synchronized !=
            (update->time_source != WATCH_TIME_SOURCE_NONE) ||
        (!update->time_synchronized && update->time_offset_ms != 0) ||
        memchr(update->error_code, '\0', sizeof(update->error_code)) == NULL ||
        (strncmp(update->error_code, "GPS_", 4U) != 0 &&
         strncmp(update->error_code, "DRV_", 4U) != 0))
    {
        return ESP_ERR_INVALID_ARG;
    }

    const bool fixed = update->status == WATCH_GPS_STATUS_FIXED;
    if (fixed != update->coordinates_valid)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (fixed)
    {
        if (!isfinite(update->latitude) || !isfinite(update->longitude) ||
            update->latitude < -90.0 || update->latitude > 90.0 ||
            update->longitude < -180.0 || update->longitude > 180.0 ||
            update->last_fix_rx_ms > update->updated_at_ms ||
            update->updated_at_ms - update->last_fix_rx_ms >=
                WATCH_GPS_FIX_FRESHNESS_MS)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    else if (update->latitude != 0.0 || update->longitude != 0.0 ||
             update->last_fix_rx_ms != 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t watch_state_apply_gps_update(const watch_gps_update_t *update,
                                       TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t validation_error = watch_gps_update_validate(update);
    if (validation_error != ESP_OK)
    {
        return validation_error;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!sequence_is_after(update->update_sequence,
                           s_state.gps.update_sequence))
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    /* 坐标、nullable 语义和派生兼容标志在同一锁内整体替换。 */
    s_state.gps = *update;
    s_state.gps_fix = update->status == WATCH_GPS_STATUS_FIXED;
    if (update->time_synchronized &&
        s_state.time_source != WATCH_TIME_SOURCE_CLOUD)
    {
        s_state.time_source = update->time_source;
        s_state.time_synchronized = true;
        s_state.time_offset_ms = update->time_offset_ms;
    }
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_gps_update_store_latest(watch_gps_update_t *slot,
                                        bool *dirty,
                                        const watch_gps_update_t *latest)
{
    if (slot == NULL || dirty == NULL ||
        watch_gps_update_validate(latest) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (*dirty &&
        !sequence_is_after(latest->update_sequence,
                           slot->update_sequence))
    {
        return ESP_ERR_INVALID_STATE;
    }
    *slot = *latest;
    *dirty = true;
    return ESP_OK;
}

esp_err_t watch_config_update_validate(const watch_config_update_t *update)
{
    if (update == NULL || update->revision == 0U ||
        !canonical_mac_is_valid(update->watch_id) ||
        update->cloud_status < WATCH_CLOUD_UNCONFIGURED ||
        update->cloud_status > WATCH_CLOUD_INVALID ||
        update->base_url_status < WATCH_CONFIG_VALUE_UNCONFIGURED ||
        update->base_url_status > WATCH_CONFIG_VALUE_INVALID ||
        update->apn_mode < WATCH_APN_AUTO || update->apn_mode > WATCH_APN_INVALID ||
        update->token_status < WATCH_CONFIG_VALUE_UNCONFIGURED ||
        update->token_status > WATCH_CONFIG_VALUE_INVALID ||
        update->certificate_status < WATCH_CONFIG_VALUE_UNCONFIGURED ||
        update->certificate_status > WATCH_CONFIG_VALUE_INVALID ||
        update->schema_status < WATCH_CONFIG_SCHEMA_VALID ||
        update->schema_status > WATCH_CONFIG_SCHEMA_CORRUPT ||
        memchr(update->cloud_error_code, '\0', sizeof(update->cloud_error_code)) == NULL ||
        strncmp(update->cloud_error_code, "CLOUD_", 6U) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const bool has_invalid = update->base_url_status == WATCH_CONFIG_VALUE_INVALID ||
                             update->apn_mode == WATCH_APN_INVALID ||
                             update->token_status == WATCH_CONFIG_VALUE_INVALID ||
                             update->certificate_status == WATCH_CONFIG_VALUE_INVALID ||
                             update->schema_status == WATCH_CONFIG_SCHEMA_CORRUPT;
    const bool has_unconfigured =
        update->base_url_status == WATCH_CONFIG_VALUE_UNCONFIGURED ||
        update->token_status == WATCH_CONFIG_VALUE_UNCONFIGURED;
    const watch_cloud_config_status_t expected_status =
        has_invalid ? WATCH_CLOUD_INVALID
                    : (has_unconfigured ? WATCH_CLOUD_UNCONFIGURED
                                        : WATCH_CLOUD_CONFIGURED);
    if (update->cloud_status != expected_status ||
        update->cloud_configured != (expected_status == WATCH_CLOUD_CONFIGURED) ||
        strcmp(update->cloud_error_code, expected_cloud_error_code(update)) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t watch_state_apply_config_update(const watch_config_update_t *update,
                                          TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t validation_error = watch_config_update_validate(update);
    if (validation_error != ESP_OK)
    {
        return validation_error;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (update->revision < s_state.config_revision ||
        (update->revision == s_state.config_revision &&
         s_state.cloud_status != WATCH_CLOUD_CONFIGURED &&
         update->cloud_status == WATCH_CLOUD_CONFIGURED))
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    s_state.config_revision = update->revision;
    memcpy(s_state.watch_id, update->watch_id, sizeof(s_state.watch_id));
    s_state.cloud_configured = update->cloud_configured;
    s_state.cloud_status = update->cloud_status;
    s_state.base_url_status = update->base_url_status;
    s_state.apn_mode = update->apn_mode;
    s_state.token_status = update->token_status;
    s_state.certificate_status = update->certificate_status;
    s_state.config_schema_status = update->schema_status;
    (void)copy_error_code(s_state.cloud_error_code,
                          sizeof(s_state.cloud_error_code),
                          update->cloud_error_code);
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_config_update_store_latest(watch_config_update_t *slot,
                                           bool *dirty,
                                           const watch_config_update_t *latest)
{
    if (slot == NULL || dirty == NULL || latest == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!*dirty || latest->revision > slot->revision ||
        (latest->revision == slot->revision &&
         config_status_safety_rank(latest->cloud_status) >
             config_status_safety_rank(slot->cloud_status)))
    {
        *slot = *latest;
        *dirty = true;
    }
    return ESP_OK;
}

esp_err_t watch_ble_update_store_latest(watch_ble_update_t *slot,
                                        bool *dirty,
                                        const watch_ble_update_t *latest)
{
    if (slot == NULL || dirty == NULL || latest == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *slot = *latest;
    *dirty = true;
    return ESP_OK;
}

esp_err_t watch_state_apply_control_update(const watch_control_update_t *update,
                                           TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->control_update_sequence == 0U ||
        update->rental_phase < WATCH_RENTAL_PHASE_WAITING_AUTH ||
        update->rental_phase > WATCH_RENTAL_PHASE_RETRY_WAIT ||
        update->control_kind < WATCH_CONTROL_KIND_NONE ||
        update->control_kind > WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY ||
        update->control_phase < WATCH_CONTROL_PHASE_IDLE ||
        update->control_phase > WATCH_CONTROL_PHASE_POWEROFF_PENDING ||
        update->control_last_result < WATCH_CONTROL_RESULT_NONE ||
        update->control_last_result >
            WATCH_CONTROL_RESULT_POWEROFF_TERMINAL ||
        update->poweroff_result < WATCH_POWEROFF_RESULT_NONE ||
        update->poweroff_result >
            WATCH_POWEROFF_RESULT_FAILED_RECONNECTED ||
        update->block_reason < WATCH_CONTROL_BLOCK_NONE ||
        update->block_reason > WATCH_CONTROL_BLOCK_RETRY_EXHAUSTED ||
        update->last_rental_result < WATCH_RENTAL_RESULT_NONE ||
        update->last_rental_result > WATCH_RENTAL_RESULT_CONFIG_ERROR ||
        strnlen(update->control_error_code,
                sizeof(update->control_error_code)) >=
            sizeof(update->control_error_code) ||
        (strncmp(update->control_error_code, "CLOUD_", 6U) != 0 &&
         strncmp(update->control_error_code, "BLE_", 4U) != 0) ||
        !control_update_is_consistent(update))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    const bool has_identity = update->request_id != 0U;
    const bool binding_mismatch =
        has_identity &&
        (!s_state.has_bound_exoskeleton ||
         memcmp(update->exoskeleton_mac,
                s_state.bound_exoskeleton_mac,
                sizeof(update->exoskeleton_mac)) != 0);
    const bool active_link_mismatch =
        (update->unlock_session_valid || update->pending_control) &&
        (!s_state.has_bound_exoskeleton || !s_state.ble_connected ||
         (has_identity
              ? (s_state.ble_link_generation == 0U ||
                 update->link_generation != s_state.ble_link_generation)
              : (!WATCH_STATE_LEGACY_HOST_FIXTURE_ALLOWED ||
                 s_state.ble_link_generation != 0U)));
    if (!sequence_is_after(update->control_update_sequence,
                           s_state.control_update_sequence) ||
        binding_mismatch || active_link_mismatch)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.control_update_sequence = update->control_update_sequence;
    s_state.control_request_id = update->request_id;
    s_state.control_link_generation = update->link_generation;
    memcpy(s_state.control_exoskeleton_mac,
           update->exoskeleton_mac,
           sizeof(s_state.control_exoskeleton_mac));
    s_state.rental_phase = update->rental_phase;
    s_state.control_kind = update->control_kind;
    s_state.control_target = update->control_target;
    s_state.control_phase = update->control_phase;
    s_state.control_last_result = update->control_last_result;
    s_state.control_attempt = update->control_attempt;
    s_state.control_ack_deadline_ms = update->control_ack_deadline_ms;
    s_state.control_locked_out = update->control_locked_out;
    s_state.poweroff_result = update->poweroff_result;
    s_state.poweroff_deadline_ms = update->poweroff_deadline_ms;
    s_state.control_block_reason = update->block_reason;
    s_state.last_rental_result = update->last_rental_result;
    s_state.pending_control = update->pending_control;
    s_state.unlock_session_valid = update->unlock_session_valid;
    (void)copy_error_code(s_state.control_error_code,
                          sizeof(s_state.control_error_code),
                          update->control_error_code);
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_cloud_update(const watch_cloud_update_t *update,
                                         TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->cloud_update_sequence == 0U ||
        update->request_id == 0U ||
        update->result <= WATCH_RENTAL_RESULT_NONE ||
        update->result > WATCH_RENTAL_RESULT_CONFIG_ERROR ||
        strnlen(update->error_code, sizeof(update->error_code)) >=
            sizeof(update->error_code) ||
        strncmp(update->error_code, "CLOUD_", 6U) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (update->cloud_update_sequence <= s_state.cloud_update_sequence)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.cloud_update_sequence = update->cloud_update_sequence;
    s_state.last_cloud_request_id = update->request_id;
    s_state.last_cloud_result = update->result;
    s_state.last_cloud_http_status = update->http_status;
    (void)copy_error_code(s_state.rental_cloud_error_code,
                          sizeof(s_state.rental_cloud_error_code),
                          update->error_code);
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_telemetry_update_validate(
    const watch_telemetry_update_t *update)
{
    if (update == NULL || update->telemetry_update_sequence == 0U ||
        update->request_id == 0U ||
        !canonical_mac_is_valid(update->exoskeleton_mac) ||
        update->result <= WATCH_TELEMETRY_RESULT_NEVER_SENT ||
        update->result > WATCH_TELEMETRY_RESULT_RESPONSE_INVALID ||
        strnlen(update->error_code, sizeof(update->error_code)) >=
            sizeof(update->error_code) ||
        strncmp(update->error_code, "CLOUD_", 6U) != 0)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const bool accepted = update->result == WATCH_TELEMETRY_RESULT_ACCEPTED;
    if (accepted)
    {
        if (update->http_status < 200U || update->http_status >= 300U ||
            !update->last_success_server_time_valid ||
            !update->cloud_time_synchronized ||
            update->last_success_server_time_ms < 0 ||
            update->last_success_server_time_ms >
                WATCH_CLOUD_SERVER_TIME_MAX_MS ||
            update->last_attempt_monotonic_ms > (uint64_t)INT64_MAX ||
            update->cloud_offset_ms !=
                update->last_success_server_time_ms -
                    (int64_t)update->last_attempt_monotonic_ms ||
            (strcmp(update->error_code, "CLOUD_OK") != 0 &&
             strcmp(update->error_code, "CLOUD_TIME_PERSIST_FAILED") != 0))
        {
            return ESP_ERR_INVALID_ARG;
        }
        return ESP_OK;
    }

    if (update->last_success_server_time_valid ||
        update->cloud_time_synchronized ||
        update->last_success_server_time_ms != 0 ||
        update->cloud_offset_ms != 0 ||
        strcmp(update->error_code, "CLOUD_OK") == 0 ||
        strcmp(update->error_code, "CLOUD_TIME_PERSIST_FAILED") == 0)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (update->result == WATCH_TELEMETRY_RESULT_HTTP_ERROR)
    {
        if (update->http_status < 100U || update->http_status > 599U ||
            (update->http_status >= 200U && update->http_status < 300U))
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    else if (update->result == WATCH_TELEMETRY_RESULT_TRANSPORT_ERROR)
    {
        if (update->http_status != 0U)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    else if (update->result == WATCH_TELEMETRY_RESULT_RESPONSE_INVALID &&
             (update->http_status < 200U ||
              update->http_status >= 300U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t watch_state_run_if_binding_current(
    const char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY],
    uint32_t binding_generation,
    watch_state_binding_action_t action,
    void *context,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!canonical_mac_is_valid(exoskeleton_mac) || action == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!s_state.has_bound_exoskeleton ||
        s_state.ble_link_generation != binding_generation ||
        memcmp(s_state.bound_exoskeleton_mac,
               exoskeleton_mac,
               WATCH_BLE_MAC_CAPACITY) != 0)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t action_error = action(context);
    xSemaphoreGive(s_state_mutex);
    return action_error;
}

esp_err_t watch_state_apply_telemetry_update(
    const watch_telemetry_update_t *update,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t validation_error =
        watch_telemetry_update_validate(update);
    if (validation_error != ESP_OK)
    {
        return validation_error;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!s_state.has_bound_exoskeleton ||
        s_state.ble_link_generation != update->binding_generation ||
        memcmp(s_state.bound_exoskeleton_mac,
               update->exoskeleton_mac,
               sizeof(update->exoskeleton_mac)) != 0)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    if (!sequence_is_after(update->telemetry_update_sequence,
                           s_state.telemetry_update_sequence))
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    s_state.telemetry_update_sequence = update->telemetry_update_sequence;
    s_state.last_telemetry_request_id = update->request_id;
    s_state.last_telemetry_result = update->result;
    s_state.last_telemetry_http_status = update->http_status;
    s_state.last_telemetry_attempt_monotonic_ms =
        update->last_attempt_monotonic_ms;
    if (update->last_success_server_time_valid)
    {
        s_state.last_telemetry_success_server_time_ms =
            update->last_success_server_time_ms;
        s_state.last_telemetry_success_server_time_valid = true;
        s_state.cloud_time_offset_ms = update->cloud_offset_ms;
        s_state.cloud_time_synchronized = update->cloud_time_synchronized;
        s_state.time_source = WATCH_TIME_SOURCE_CLOUD;
        s_state.time_synchronized = true;
        s_state.time_offset_ms = update->cloud_offset_ms;
    }
    (void)copy_error_code(s_state.telemetry_error_code,
                          sizeof(s_state.telemetry_error_code),
                          update->error_code);
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_ble_update(const watch_ble_update_t *update,
                                       TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL ||
        update->transaction < WATCH_BLE_TRANSACTION_UNBOUND ||
        update->transaction > WATCH_BLE_TRANSACTION_ERROR ||
        (update->transaction == WATCH_BLE_TRANSACTION_UNBOUND &&
         (update->has_bound_exoskeleton || update->ble_connected)) ||
        (update->transaction == WATCH_BLE_TRANSACTION_BOUND_READY &&
         !update->has_bound_exoskeleton) ||
        (update->ble_connected && update->link_generation == 0U &&
         !WATCH_STATE_LEGACY_HOST_FIXTURE_ALLOWED) ||
        (!update->has_bound_exoskeleton &&
         update->bound_exoskeleton_mac[0] != '\0') ||
        (!update->ble_connected &&
         (update->link_generation != 0U || update->gatt_ready ||
          update->mtu_ready || update->negotiated_mtu != 0U ||
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
          !exoskeleton_state_is_valid(&update->exoskeleton_state))) ||
        (update->dataReady && (!update->ble_connected || !update->device_state_available)) ||
        (update->status_fresh &&
         (!update->device_state_available || !update->dataReady ||
          update->last_status_rx_ms == 0U)) ||
        (update->link_control_ready &&
         (!update->ble_connected || !update->gatt_ready ||
          !update->notify_ready || !update->dataReady ||
          !update->status_fresh)) ||
        (!update->link_control_ready && !update->clear_pending_control) ||
        strnlen(update->ble_error_code, sizeof(update->ble_error_code)) >=
            sizeof(update->ble_error_code) ||
        strncmp(update->ble_error_code, "BLE_", 4U) != 0 ||
        strnlen(update->diagnostics.last_error_code,
                sizeof(update->diagnostics.last_error_code)) >=
            sizeof(update->diagnostics.last_error_code) ||
        (update->diagnostics.last_error_code[0] != '\0' &&
         strncmp(update->diagnostics.last_error_code, "BLE_", 4U) != 0) ||
        (update->diagnostics_reset &&
         (update->transaction != WATCH_BLE_TRANSACTION_UNBOUND ||
          update->has_bound_exoskeleton || update->ble_connected ||
          update->diagnostics.valid_frame_count != 0U ||
          update->diagnostics.invalid_frame_count != 0U ||
          update->diagnostics.discarded_byte_count != 0U ||
          update->diagnostics.version_sanitized_count != 0U ||
          update->diagnostics.reconnect_attempt_count != 0U ||
          update->diagnostics.reconnect_success_count != 0U)))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (update->has_bound_exoskeleton &&
        !canonical_mac_is_valid(update->bound_exoskeleton_mac))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!update->diagnostics_reset &&
        (update->diagnostics.valid_frame_count <
             s_state.ble_diagnostics.valid_frame_count ||
         update->diagnostics.invalid_frame_count <
             s_state.ble_diagnostics.invalid_frame_count ||
         update->diagnostics.discarded_byte_count <
             s_state.ble_diagnostics.discarded_byte_count ||
         update->diagnostics.version_sanitized_count <
             s_state.ble_diagnostics.version_sanitized_count ||
         update->diagnostics.reconnect_attempt_count <
             s_state.ble_diagnostics.reconnect_attempt_count ||
         update->diagnostics.reconnect_success_count <
             s_state.ble_diagnostics.reconnect_success_count))
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_ARG;
    }

    const bool binding_changed =
        s_state.has_bound_exoskeleton != update->has_bound_exoskeleton ||
        (s_state.has_bound_exoskeleton && update->has_bound_exoskeleton &&
         strcmp(s_state.bound_exoskeleton_mac,
                update->bound_exoskeleton_mac) != 0);
    if (binding_changed)
    {
        s_state.exoskeleton_model = WATCH_EXOSKELETON_MODEL_UNKNOWN;
        s_state.exoskeleton_low_battery = false;
    }
    s_state.has_bound_exoskeleton = update->has_bound_exoskeleton;
    memset(s_state.bound_exoskeleton_mac, 0, sizeof(s_state.bound_exoskeleton_mac));
    if (update->has_bound_exoskeleton)
    {
        memcpy(s_state.bound_exoskeleton_mac,
               update->bound_exoskeleton_mac,
               sizeof(s_state.bound_exoskeleton_mac));
    }
    s_state.ble_connected = update->ble_connected;
    s_state.ble_link_generation = update->link_generation;
    s_state.ble_transaction = update->transaction;
    s_state.gatt_ready = update->gatt_ready;
    s_state.mtu_ready = update->mtu_ready;
    s_state.negotiated_mtu = update->negotiated_mtu;
    s_state.notify_ready = update->notify_ready;
    s_state.device_state_available = update->device_state_available;
    s_state.dataReady = update->dataReady;
    s_state.status_fresh = update->status_fresh;
    if (update->device_state_available)
    {
        s_state.last_status_rx_ms = update->last_status_rx_ms;
        s_state.exoskeleton_state = update->exoskeleton_state;
        s_state.exoskeleton_model =
            watch_exoskeleton_model_from_version(update->exoskeleton_state.version);
        if (update->exoskeleton_state.battery_level <=
            WATCH_EXOSKELETON_LOW_BATTERY_ENTER_PERCENT)
        {
            s_state.exoskeleton_low_battery = true;
        }
        else if (update->exoskeleton_state.battery_level >
                 WATCH_EXOSKELETON_LOW_BATTERY_EXIT_PERCENT)
        {
            s_state.exoskeleton_low_battery = false;
        }
    }
    else
    {
        s_state.last_status_rx_ms = 0U;
        memset(&s_state.exoskeleton_state, 0, sizeof(s_state.exoskeleton_state));
        /* 设备状态失效时不能继续把上一帧的低电锁存投影到提醒页；
         * 下一帧有效状态会按迟滞阈值重新决定是否进入提醒。 */
        s_state.exoskeleton_low_battery = false;
    }
    s_state.link_control_ready = update->link_control_ready;
    if (binding_changed)
    {
        s_state.unlock_session_valid = false;
        s_state.pending_control = false;
        s_state.control_request_id = 0U;
        s_state.control_link_generation = 0U;
        memset(s_state.control_exoskeleton_mac,
               0,
               sizeof(s_state.control_exoskeleton_mac));
        s_state.rental_phase = WATCH_RENTAL_PHASE_WAITING_AUTH;
        s_state.control_kind = WATCH_CONTROL_KIND_NONE;
        s_state.control_target = 0U;
        s_state.control_phase = WATCH_CONTROL_PHASE_IDLE;
        s_state.control_last_result = WATCH_CONTROL_RESULT_NONE;
        s_state.control_attempt = 0U;
        s_state.control_ack_deadline_ms = 0U;
        s_state.control_locked_out = false;
        s_state.poweroff_result = WATCH_POWEROFF_RESULT_NONE;
        s_state.poweroff_deadline_ms = 0U;
        s_state.control_block_reason = update->has_bound_exoskeleton
                                           ? WATCH_CONTROL_BLOCK_BLE_LINK
                                           : WATCH_CONTROL_BLOCK_UNBOUND;
        s_state.last_rental_result = WATCH_RENTAL_RESULT_NONE;
        (void)copy_error_code(s_state.control_error_code,
                              sizeof(s_state.control_error_code),
                              WATCH_STATE_CONTROL_AUTH_REQUIRED);
    }
    else if (update->clear_pending_control && s_state.pending_control)
    {
        s_state.pending_control = false;
        if (s_state.control_phase == WATCH_CONTROL_PHASE_WRITING ||
            s_state.control_phase ==
                WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED)
        {
            s_state.control_phase = WATCH_CONTROL_PHASE_IDLE;
            s_state.control_last_result = WATCH_CONTROL_RESULT_BLOCKED;
            s_state.control_ack_deadline_ms = 0U;
            s_state.poweroff_deadline_ms = 0U;
            const bool status_stale = update->ble_connected &&
                                      update->dataReady &&
                                      !update->status_fresh;
            s_state.control_block_reason = status_stale
                                               ? WATCH_CONTROL_BLOCK_STATUS_STALE
                                               : WATCH_CONTROL_BLOCK_BLE_LINK;
            (void)copy_error_code(s_state.control_error_code,
                                  sizeof(s_state.control_error_code),
                                  status_stale
                                      ? WATCH_STATE_CONTROL_STATUS_STALE
                                      : "BLE_CONTROL_BLOCKED");
        }
        else if (s_state.rental_phase == WATCH_RENTAL_PHASE_QUERYING ||
                 s_state.rental_phase ==
                     WATCH_RENTAL_PHASE_WAITING_PAYMENT ||
                 s_state.rental_phase == WATCH_RENTAL_PHASE_RETRY_WAIT ||
                 s_state.rental_phase == WATCH_RENTAL_PHASE_UNLOCK_WRITING ||
                 s_state.rental_phase == WATCH_RENTAL_PHASE_CONFIRMING)
        {
            s_state.rental_phase = WATCH_RENTAL_PHASE_BLOCKED;
            const bool status_stale = update->ble_connected &&
                                      update->dataReady &&
                                      !update->status_fresh;
            s_state.control_block_reason = status_stale
                                               ? WATCH_CONTROL_BLOCK_STATUS_STALE
                                               : WATCH_CONTROL_BLOCK_BLE_LINK;
            (void)copy_error_code(
                s_state.control_error_code,
                sizeof(s_state.control_error_code),
                status_stale ? WATCH_STATE_CONTROL_STATUS_STALE
                             : WATCH_STATE_CONTROL_LINK_CHANGED);
        }
    }
    (void)copy_error_code(s_state.ble_error_code,
                          sizeof(s_state.ble_error_code),
                          update->ble_error_code);
    s_state.ble_diagnostics = update->diagnostics;
    if (s_state.ble_diagnostics.last_error_code[0] == '\0')
    {
        (void)copy_error_code(s_state.ble_diagnostics.last_error_code,
                              sizeof(s_state.ble_diagnostics.last_error_code),
                              update->ble_error_code);
    }

    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_audio_snapshot(watch_audio_snapshot_t *snapshot,
                                     TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    *snapshot = (watch_audio_snapshot_t){
        .resource_available = s_state.audio_resource_available,
        .state = s_state.audio_state,
        .error = s_state.audio_error,
        .updated_at_ticks = s_state.audio_updated_at_ticks,
        .update_sequence = s_state.audio_update_sequence,
    };
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_power_snapshot(watch_power_snapshot_t *snapshot,
                                     TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    *snapshot = (watch_power_snapshot_t){
        .power_level = s_state.power_level,
        .screen_state = s_state.screen_state,
        .battery_percent = s_state.battery_percent,
        .battery_has_valid_sample = s_state.battery_has_valid_sample,
        .selftest_active =
            s_state.selftest.state == SELFTEST_RUN_RUNNING ||
            s_state.selftest.retry_active,
        .screen_transition_sequence =
            s_state.screen_transition_sequence,
    };
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_selftest_begin(const watch_selftest_begin_update_t *update,
                                           TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->run_id == 0U ||
        update->first_item < SELFTEST_ITEM_BSP_RESOURCE_TABLE || update->first_item >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    reset_selftest_summary(update->run_id, update->first_item, update->started_at_ticks);
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_selftest_item(const watch_selftest_item_update_t *update,
                                          TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->run_id == 0U ||
        update->result.item_id < SELFTEST_ITEM_BSP_RESOURCE_TABLE ||
        update->result.item_id >= SELFTEST_ITEM_COUNT ||
        update->next_item < SELFTEST_ITEM_BSP_RESOURCE_TABLE || update->next_item > SELFTEST_ITEM_COUNT ||
        (update->result.outcome != SELFTEST_OUTCOME_PASS &&
         update->result.outcome != SELFTEST_OUTCOME_FAIL &&
         update->result.outcome != SELFTEST_OUTCOME_SKIP &&
         update->result.outcome != SELFTEST_OUTCOME_INCONCLUSIVE) ||
        memchr(update->result.detail_code, '\0', sizeof(update->result.detail_code)) == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    selftest_item_result_t *destination =
        &s_state.selftest.selftest_results[update->result.item_id];
    const selftest_item_id_t expected_next =
        (selftest_item_id_t)(update->result.item_id + 1);
    if (s_state.selftest.run_id != update->run_id ||
        s_state.selftest.state != SELFTEST_RUN_RUNNING ||
        s_state.selftest.current_item != update->result.item_id ||
        update->next_item != expected_next ||
        destination->outcome != SELFTEST_OUTCOME_NOT_RUN)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    *destination = update->result;
    ++s_state.selftest.completed_count;
    if (update->result.outcome == SELFTEST_OUTCOME_PASS)
    {
        ++s_state.selftest.pass_count;
    }
    else if (update->result.outcome == SELFTEST_OUTCOME_FAIL)
    {
        ++s_state.selftest.fail_count;
    }
    else if (update->result.outcome == SELFTEST_OUTCOME_SKIP)
    {
        ++s_state.selftest.skip_count;
    }
    else
    {
        ++s_state.selftest.inconclusive_count;
    }
    s_state.selftest.current_item = update->next_item;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_selftest_finish(const watch_selftest_finish_update_t *update,
                                            TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->run_id == 0U ||
        (update->state != SELFTEST_RUN_FINISHED &&
         update->state != SELFTEST_RUN_CANCELLED))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_state.selftest.run_id != update->run_id ||
        s_state.selftest.state != SELFTEST_RUN_RUNNING ||
        (update->state == SELFTEST_RUN_FINISHED &&
         s_state.selftest.completed_count != SELFTEST_ITEM_COUNT))
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    s_state.selftest.state = update->state;
    s_state.selftest.current_item = SELFTEST_ITEM_COUNT;
    s_state.selftest.finished_at_ticks = update->finished_at_ticks;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_selftest_retry_begin(
    const watch_selftest_retry_begin_update_t *update,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (update == NULL || update->run_id == 0U ||
        update->item_id < SELFTEST_ITEM_BSP_RESOURCE_TABLE || update->item_id >= SELFTEST_ITEM_COUNT)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    const selftest_item_result_t *current_result =
        &s_state.selftest.selftest_results[update->item_id];
    if (s_state.selftest.run_id != update->run_id ||
        s_state.selftest.state != SELFTEST_RUN_FINISHED ||
        s_state.selftest.completed_count != SELFTEST_ITEM_COUNT ||
        s_state.selftest.retry_active ||
        current_result->outcome != SELFTEST_OUTCOME_FAIL)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    s_state.selftest.state = SELFTEST_RUN_RUNNING;
    s_state.selftest.current_item = update->item_id;
    s_state.selftest.retry_active = true;
    s_state.selftest.retry_item = update->item_id;
    s_state.selftest.retry_started_at_ticks = update->started_at_ticks;
    s_state.selftest.retry_finished_at_ticks = 0U;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_apply_selftest_retry_finish(
    const watch_selftest_retry_finish_update_t *update,
    TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
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
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    selftest_item_result_t *destination =
        &s_state.selftest.selftest_results[update->result.item_id];
    if (s_state.selftest.run_id != update->run_id ||
        s_state.selftest.state != SELFTEST_RUN_RUNNING ||
        !s_state.selftest.retry_active ||
        s_state.selftest.retry_item != update->result.item_id ||
        s_state.selftest.current_item != update->result.item_id ||
        destination->outcome != SELFTEST_OUTCOME_FAIL ||
        s_state.selftest.fail_count == 0U)
    {
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }

    --s_state.selftest.fail_count;
    if (update->result.outcome == SELFTEST_OUTCOME_PASS)
    {
        ++s_state.selftest.pass_count;
    }
    else if (update->result.outcome == SELFTEST_OUTCOME_FAIL)
    {
        ++s_state.selftest.fail_count;
    }
    else if (update->result.outcome == SELFTEST_OUTCOME_SKIP)
    {
        ++s_state.selftest.skip_count;
    }
    else
    {
        ++s_state.selftest.inconclusive_count;
    }
    *destination = update->result;
    s_state.selftest.state = SELFTEST_RUN_FINISHED;
    s_state.selftest.current_item = SELFTEST_ITEM_COUNT;
    s_state.selftest.retry_active = false;
    s_state.selftest.retry_finished_at_ticks = update->finished_at_ticks;
    s_state.selftest.finished_at_ticks = update->finished_at_ticks;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

esp_err_t watch_state_snapshot(watch_state_snapshot_t *snapshot, TickType_t timeout_ticks)
{
    if (s_state_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    /* 读取快照时复制完整结构，避免调用者看到更新中的中间状态。 */
    if (xSemaphoreTake(s_state_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }

    *snapshot = s_state;
    xSemaphoreGive(s_state_mutex);
    return ESP_OK;
}

static esp_err_t copy_error_code(char *destination,
                                 size_t capacity,
                                 const char *source)
{
    if (destination == NULL || source == NULL || capacity == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const size_t length = strlen(source);
    if (length >= capacity)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memcpy(destination, source, length + 1U);
    return ESP_OK;
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

static const char *expected_cloud_error_code(const watch_config_update_t *update)
{
    if (update->schema_status == WATCH_CONFIG_SCHEMA_CORRUPT)
    {
        return "CLOUD_SCHEMA_CORRUPT";
    }
    if (update->base_url_status == WATCH_CONFIG_VALUE_UNCONFIGURED)
    {
        return "CLOUD_BASE_URL_UNCONFIGURED";
    }
    if (update->base_url_status == WATCH_CONFIG_VALUE_INVALID)
    {
        return "CLOUD_BASE_URL_INVALID";
    }
    if (update->apn_mode == WATCH_APN_INVALID)
    {
        return "CLOUD_APN_INVALID";
    }
    if (update->token_status == WATCH_CONFIG_VALUE_UNCONFIGURED)
    {
        return "CLOUD_TOKEN_UNCONFIGURED";
    }
    if (update->token_status == WATCH_CONFIG_VALUE_INVALID)
    {
        return "CLOUD_TOKEN_INVALID";
    }
    if (update->certificate_status == WATCH_CONFIG_VALUE_INVALID)
    {
        return "CLOUD_CERT_INVALID";
    }
    return "CLOUD_OK";
}

static uint8_t config_status_safety_rank(watch_cloud_config_status_t status)
{
    if (status == WATCH_CLOUD_INVALID)
    {
        return 2U;
    }
    if (status == WATCH_CLOUD_UNCONFIGURED)
    {
        return 1U;
    }
    return 0U;
}

static bool control_update_is_consistent(const watch_control_update_t *update)
{
    if (update == NULL)
    {
        return false;
    }
    const bool has_any_identity = update->request_id != 0U ||
                                  update->link_generation != 0U ||
                                  update->exoskeleton_mac[0] != '\0';
    const bool has_complete_identity =
        update->request_id != 0U && update->link_generation != 0U &&
        canonical_mac_is_valid(update->exoskeleton_mac);
    if (has_any_identity != has_complete_identity)
    {
        return false;
    }

    const bool active_identity_is_usable =
        has_complete_identity ||
        (WATCH_STATE_LEGACY_HOST_FIXTURE_ALLOWED && !has_any_identity);
    const bool has_control_operation =
        update->control_kind != WATCH_CONTROL_KIND_NONE ||
        update->control_phase != WATCH_CONTROL_PHASE_IDLE ||
        update->control_last_result != WATCH_CONTROL_RESULT_NONE ||
        update->control_attempt != 0U || update->control_locked_out ||
        update->control_ack_deadline_ms != 0U ||
        update->poweroff_result != WATCH_POWEROFF_RESULT_NONE ||
        update->poweroff_deadline_ms != 0U;
    const bool has_extended_contract =
        update->control_attempt != 0U || update->control_locked_out ||
        update->control_ack_deadline_ms != 0U ||
        update->poweroff_result != WATCH_POWEROFF_RESULT_NONE ||
        update->poweroff_deadline_ms != 0U;
    if (has_control_operation)
    {
        if (!active_identity_is_usable ||
            !ordinary_control_target_is_valid(update->control_kind,
                                              update->control_target) ||
            (has_extended_contract &&
             (update->control_attempt == 0U ||
              update->control_attempt > WATCH_CONTROL_MAX_ATTEMPTS)) ||
            (update->control_ack_deadline_ms != 0U &&
             update->poweroff_deadline_ms != 0U))
        {
            return false;
        }
        if (update->control_kind == WATCH_CONTROL_KIND_POWEROFF)
        {
            if ((has_extended_contract && update->control_attempt != 1U) ||
                update->control_locked_out ||
                update->control_ack_deadline_ms != 0U)
            {
                return false;
            }
            if (update->control_phase == WATCH_CONTROL_PHASE_WRITING)
            {
                return update->pending_control &&
                       update->control_last_result ==
                           WATCH_CONTROL_RESULT_NONE &&
                       update->poweroff_result == WATCH_POWEROFF_RESULT_NONE &&
                       update->poweroff_deadline_ms == 0U &&
                       update->rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
                       update->unlock_session_valid &&
                       update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
                       update->block_reason == WATCH_CONTROL_BLOCK_BUSY;
            }
            if (update->control_phase ==
                WATCH_CONTROL_PHASE_POWEROFF_PENDING)
            {
                return update->pending_control &&
                       update->control_last_result ==
                           WATCH_CONTROL_RESULT_SUBMITTED_UNCONFIRMED &&
                       update->poweroff_result == WATCH_POWEROFF_RESULT_NONE &&
                       update->poweroff_deadline_ms != 0U &&
                       update->rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
                       update->unlock_session_valid &&
                       update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
                       update->block_reason == WATCH_CONTROL_BLOCK_BUSY;
            }
            if (update->control_phase != WATCH_CONTROL_PHASE_IDLE ||
                update->pending_control ||
                update->poweroff_deadline_ms != 0U)
            {
                return false;
            }
            if (update->control_last_result ==
                WATCH_CONTROL_RESULT_WRITE_REJECTED)
            {
                return update->poweroff_result == WATCH_POWEROFF_RESULT_NONE &&
                       update->rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
                       update->unlock_session_valid &&
                       update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
                       update->block_reason == WATCH_CONTROL_BLOCK_BLE_WRITE;
            }
            if (update->control_last_result == WATCH_CONTROL_RESULT_BLOCKED)
            {
                return update->poweroff_result == WATCH_POWEROFF_RESULT_NONE &&
                       update->rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
                       update->unlock_session_valid &&
                       update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
                       update->block_reason == WATCH_CONTROL_BLOCK_BLE_LINK;
            }
            if (update->control_last_result !=
                    WATCH_CONTROL_RESULT_POWEROFF_TERMINAL ||
                update->poweroff_result == WATCH_POWEROFF_RESULT_NONE)
            {
                return false;
            }
            if (update->poweroff_result ==
                    WATCH_POWEROFF_RESULT_FAILED_RECONNECTED &&
                update->unlock_session_valid)
            {
                return update->rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
                       update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
                       update->block_reason == WATCH_CONTROL_BLOCK_NONE;
            }
            return !update->unlock_session_valid &&
                   update->rental_phase == WATCH_RENTAL_PHASE_WAITING_AUTH &&
                   update->last_rental_result == WATCH_RENTAL_RESULT_NONE &&
                   update->block_reason == WATCH_CONTROL_BLOCK_BLE_LINK;
        }
        if (update->poweroff_result != WATCH_POWEROFF_RESULT_NONE ||
            update->poweroff_deadline_ms != 0U ||
            update->rental_phase != WATCH_RENTAL_PHASE_UNLOCKED ||
            !update->unlock_session_valid ||
            update->last_rental_result != WATCH_RENTAL_RESULT_PAID)
        {
            return false;
        }
        switch (update->control_phase)
        {
        case WATCH_CONTROL_PHASE_WRITING:
            return update->pending_control &&
                   update->control_ack_deadline_ms == 0U &&
                   !update->control_locked_out &&
                   (update->control_last_result == WATCH_CONTROL_RESULT_NONE ||
                    (update->control_attempt > 1U &&
                     update->control_last_result ==
                         WATCH_CONTROL_RESULT_RETRYING)) &&
                   update->block_reason == WATCH_CONTROL_BLOCK_BUSY;
        case WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED:
            return update->pending_control &&
                   !update->control_locked_out &&
                   (!has_extended_contract ||
                    update->control_ack_deadline_ms != 0U) &&
                   update->control_last_result ==
                       WATCH_CONTROL_RESULT_SUBMITTED_UNCONFIRMED &&
                   update->block_reason == WATCH_CONTROL_BLOCK_BUSY;
        case WATCH_CONTROL_PHASE_IDLE:
            if (update->pending_control ||
                update->control_ack_deadline_ms != 0U)
            {
                return false;
            }
            if (update->control_locked_out)
            {
                return update->control_attempt == WATCH_CONTROL_MAX_ATTEMPTS &&
                       update->control_last_result ==
                           WATCH_CONTROL_RESULT_RETRY_EXHAUSTED &&
                       update->block_reason ==
                           WATCH_CONTROL_BLOCK_RETRY_EXHAUSTED;
            }
            if (update->control_last_result == WATCH_CONTROL_RESULT_NO_OP)
            {
                return update->block_reason == WATCH_CONTROL_BLOCK_NONE;
            }
            if (update->control_last_result == WATCH_CONTROL_RESULT_APPLIED)
            {
                return update->block_reason == WATCH_CONTROL_BLOCK_NONE;
            }
            if (update->control_last_result ==
                WATCH_CONTROL_RESULT_WRITE_REJECTED)
            {
                return update->block_reason == WATCH_CONTROL_BLOCK_BLE_WRITE;
            }
            if (update->control_last_result == WATCH_CONTROL_RESULT_BLOCKED)
            {
                return update->block_reason == WATCH_CONTROL_BLOCK_BLE_LINK ||
                       update->block_reason ==
                           WATCH_CONTROL_BLOCK_STATUS_STALE;
            }
            return false;
        default:
            return false;
        }
    }
    if (has_extended_contract)
    {
        return false;
    }
    switch (update->rental_phase)
    {
    case WATCH_RENTAL_PHASE_WAITING_AUTH:
        return !update->pending_control && !update->unlock_session_valid &&
               update->last_rental_result == WATCH_RENTAL_RESULT_NONE &&
               update->block_reason != WATCH_CONTROL_BLOCK_NONE &&
               update->block_reason != WATCH_CONTROL_BLOCK_BUSY;
    case WATCH_RENTAL_PHASE_QUERYING:
        return active_identity_is_usable && update->pending_control &&
               !update->unlock_session_valid &&
               update->last_rental_result == WATCH_RENTAL_RESULT_NONE &&
               update->block_reason == WATCH_CONTROL_BLOCK_BUSY;
    case WATCH_RENTAL_PHASE_WAITING_PAYMENT:
        return active_identity_is_usable && update->pending_control &&
               !update->unlock_session_valid &&
               update->last_rental_result == WATCH_RENTAL_RESULT_UNPAID &&
               update->block_reason == WATCH_CONTROL_BLOCK_UNPAID;
    case WATCH_RENTAL_PHASE_RETRY_WAIT:
        return active_identity_is_usable && update->pending_control &&
               !update->unlock_session_valid &&
               rental_result_can_retry(update->last_rental_result) &&
               update->block_reason == WATCH_CONTROL_BLOCK_CLOUD;
    case WATCH_RENTAL_PHASE_UNLOCK_WRITING:
    case WATCH_RENTAL_PHASE_CONFIRMING:
        return active_identity_is_usable && update->pending_control &&
               !update->unlock_session_valid &&
               update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
               update->block_reason == WATCH_CONTROL_BLOCK_BUSY;
    case WATCH_RENTAL_PHASE_UNLOCKED:
        return active_identity_is_usable && !update->pending_control &&
               update->unlock_session_valid &&
               update->last_rental_result == WATCH_RENTAL_RESULT_PAID &&
               update->block_reason == WATCH_CONTROL_BLOCK_NONE;
    case WATCH_RENTAL_PHASE_BLOCKED:
        return !update->pending_control && !update->unlock_session_valid &&
               update->block_reason != WATCH_CONTROL_BLOCK_NONE &&
               update->block_reason != WATCH_CONTROL_BLOCK_BUSY &&
               control_block_result_is_consistent(update->block_reason,
                                                  update->last_rental_result);
    default:
        return false;
    }
}

static bool sequence_is_after(uint32_t candidate, uint32_t baseline)
{
    return candidate != 0U &&
           (baseline == 0U || (int32_t)(candidate - baseline) > 0);
}

static bool ordinary_control_target_is_valid(watch_control_kind_t kind,
                                             uint8_t target)
{
    switch (kind)
    {
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

static bool control_block_result_is_consistent(
    watch_control_block_reason_t block_reason,
    watch_rental_result_t result)
{
    switch (block_reason)
    {
    case WATCH_CONTROL_BLOCK_BLE_LINK:
    case WATCH_CONTROL_BLOCK_STATUS_STALE:
        return result >= WATCH_RENTAL_RESULT_NONE &&
               result <= WATCH_RENTAL_RESULT_CONFIG_ERROR;
    case WATCH_CONTROL_BLOCK_UNPAID:
        return result == WATCH_RENTAL_RESULT_UNPAID;
    case WATCH_CONTROL_BLOCK_DISABLED:
        return result == WATCH_RENTAL_RESULT_DISABLED;
    case WATCH_CONTROL_BLOCK_UNKNOWN_DEVICE:
        return result == WATCH_RENTAL_RESULT_UNKNOWN_DEVICE;
    case WATCH_CONTROL_BLOCK_CLOUD:
        return result >= WATCH_RENTAL_RESULT_REQUEST_INVALID &&
               result <= WATCH_RENTAL_RESULT_CONFIG_ERROR;
    case WATCH_CONTROL_BLOCK_BLE_WRITE:
        return result == WATCH_RENTAL_RESULT_PAID;
    case WATCH_CONTROL_BLOCK_CONFIRM_TIMEOUT:
        return result == WATCH_RENTAL_RESULT_TIMEOUT;
    default:
        return false;
    }
}

static bool rental_result_can_retry(watch_rental_result_t result)
{
    return result == WATCH_RENTAL_RESULT_SERVER_ERROR ||
           result == WATCH_RENTAL_RESULT_HTTP_ERROR ||
           result == WATCH_RENTAL_RESULT_TRANSPORT_ERROR ||
           result == WATCH_RENTAL_RESULT_TIMEOUT;
}

static bool exoskeleton_state_is_valid(const watch_exoskeleton_state_t *state)
{
    if (state == NULL ||
        memchr(state->version, '\0', sizeof(state->version)) == NULL)
    {
        return false;
    }
    const watch_exoskeleton_scene_config_t config = state->scene_config;
    const float max_gear = (float)watch_exoskeleton_scene_config_max_gear(config);
    return isfinite(state->current_mode) &&
           isfinite(state->steps) && state->steps >= 0.0F &&
           isfinite(state->step_frequency) &&
           isfinite(state->left_gear) &&
           state->left_gear >= (float)WATCH_EXOSKELETON_GEAR_MIN &&
           state->left_gear <= max_gear &&
           isfinite(state->right_gear) &&
           state->right_gear >= (float)WATCH_EXOSKELETON_GEAR_MIN &&
           state->right_gear <= max_gear &&
           isfinite(state->balance_factor) &&
           state->balance_factor >= -1.0F &&
           state->balance_factor <= 1.0F &&
           state->motor_enable <= 1U &&
           state->battery_level <= 100U &&
           watch_exoskeleton_scene_config_supports_mode(config,
                                                        state->scene_mode);
}

static bool version_matches_model(const char *version,
                                  const char *prefix)
{
    if (version == NULL || prefix == NULL)
    {
        return false;
    }
    const size_t prefix_length = strlen(prefix);
    if (strncmp(version, prefix, prefix_length) != 0)
    {
        return false;
    }
    const char *cursor = version + prefix_length;
    if (!consume_decimal_component(&cursor) || *cursor != '.')
    {
        return false;
    }
    ++cursor;
    return consume_decimal_component(&cursor) && *cursor == '\0';
}

static bool consume_decimal_component(const char **cursor)
{
    if (cursor == NULL || *cursor == NULL ||
        **cursor < '0' || **cursor > '9')
    {
        return false;
    }
    do
    {
        ++(*cursor);
    } while (**cursor >= '0' && **cursor <= '9');
    return true;
}

static bool modem_ip_is_valid(const char *text)
{
    return modem_ipv4_is_valid(text) || modem_ipv6_is_valid(text);
}

static bool modem_ipv4_is_valid(const char *text)
{
    unsigned octets = 0U;
    const char *cursor = text;
    while (*cursor != '\0')
    {
        unsigned value = 0U;
        unsigned digits = 0U;
        while (*cursor >= '0' && *cursor <= '9')
        {
            value = value * 10U + (unsigned)(*cursor - '0');
            ++digits;
            ++cursor;
        }
        if (digits == 0U || digits > 3U || value > 255U)
        {
            return false;
        }
        ++octets;
        if (*cursor == '\0')
        {
            break;
        }
        if (*cursor != '.' || octets >= 4U)
        {
            return false;
        }
        ++cursor;
    }
    return octets == 4U && strcmp(text, "0.0.0.0") != 0;
}

static bool modem_ipv6_is_valid(const char *text)
{
    unsigned groups = 0U;
    bool compressed = false;
    const char *cursor = text;
    if (*cursor == '\0')
    {
        return false;
    }
    while (*cursor != '\0')
    {
        if (*cursor == ':')
        {
            if (cursor[1] != ':' || compressed)
            {
                return false;
            }
            compressed = true;
            cursor += 2;
            if (*cursor == '\0')
            {
                break;
            }
            continue;
        }
        unsigned digits = 0U;
        while ((*cursor >= '0' && *cursor <= '9') ||
               (*cursor >= 'a' && *cursor <= 'f') ||
               (*cursor >= 'A' && *cursor <= 'F'))
        {
            ++digits;
            ++cursor;
        }
        if (digits == 0U || digits > 4U || ++groups > 8U)
        {
            return false;
        }
        if (*cursor == '\0')
        {
            break;
        }
        if (*cursor != ':')
        {
            return false;
        }
        if (cursor[1] == ':')
        {
            if (compressed)
            {
                return false;
            }
            compressed = true;
            cursor += 2;
        }
        else
        {
            if (cursor[1] == '\0')
            {
                return false;
            }
            ++cursor;
        }
    }
    return groups >= 1U && (compressed ? groups < 8U : groups == 8U) &&
           strcmp(text, "::") != 0;
}

static void reset_selftest_summary(uint32_t run_id,
                                   selftest_item_id_t first_item,
                                   TickType_t started_at_ticks)
{
    memset(&s_state.selftest, 0, sizeof(s_state.selftest));
    s_state.selftest.run_id = run_id;
    s_state.selftest.state = SELFTEST_RUN_RUNNING;
    s_state.selftest.current_item = first_item;
    s_state.selftest.started_at_ticks = started_at_ticks;
    s_state.selftest.retry_item = SELFTEST_ITEM_COUNT;
    for (size_t index = 0; index < SELFTEST_ITEM_COUNT; ++index)
    {
        selftest_item_result_t *item = &s_state.selftest.selftest_results[index];
        item->item_id = (selftest_item_id_t)index;
        item->outcome = SELFTEST_OUTCOME_NOT_RUN;
        item->reason = SELFTEST_REASON_NONE;
        (void)copy_error_code(item->detail_code,
                              sizeof(item->detail_code),
                              "DRV_SELFTEST_PENDING");
    }
}
