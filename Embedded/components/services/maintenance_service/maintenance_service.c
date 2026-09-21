/**
 * @file     maintenance_service.c
 * @brief    最小维护与诊断服务实现
 * @details  投影现有 watch_state，并管理有界 BLE 候选和按值选择命令，不创建独立任务。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "maintenance_service.h"

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "gps_service.h"
#include "legbot_services.h"
#include "modem_service.h"
#include "selftest_service.h"

#if !defined(LEGBOT_CAP_GPS_TIME) || \
    (LEGBOT_CAP_GPS_TIME != 0 && LEGBOT_CAP_GPS_TIME != 1)
#error "LEGBOT_CAP_GPS_TIME must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_GPS_CONTINUOUS_LOCATION)
#error "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION must be defined."
#endif
#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "SVC_MAINT";

/** 无可用 typed 错误时的稳定显示值。 */
#define MAINTENANCE_NO_ERROR_CODE "NONE"
/** 尚未产生有效电量结果时的初始状态码。 */
#define MAINTENANCE_POWER_NOT_UPDATED "POWER_BATTERY_NOT_UPDATED"
/** 尚未产生有效电量驱动结果时的初始状态码。 */
#define MAINTENANCE_POWER_DRIVER_NOT_READY "DRV_CW2015_NOT_READY"

/** 保护候选列表、代次和维护版本的互斥锁。 */
static SemaphoreHandle_t s_mutex;
/** 未来 ble_service 消费的 typed 选择队列。 */
static QueueHandle_t s_selection_queue;
/** 仅由 UI 独立确认页写入、ble_task 消费的清绑队列。 */
static QueueHandle_t s_clear_queue;
/** 当前按值候选列表。 */
static maintenance_ble_candidate_list_t s_candidate_list;
/** 当前候选代次的 ble_task 最终选择反馈。 */
static maintenance_ble_selection_feedback_t s_selection_feedback;
/** 维护瞬态模型版本。 */
static uint64_t s_snapshot_version;
/** 当前代次是否已经接受过一次选择。 */
static uint32_t s_accepted_generation;
/** 维护契约是否完整初始化。 */
static bool s_initialized;

static void cleanup_resources(void);
static bool normalize_mac(const char input[MAINTENANCE_BLE_MAC_CAPACITY],
                          char output[MAINTENANCE_BLE_MAC_CAPACITY]);
static bool is_stable_error_code(const char *code);
static bool copy_error_code(char destination[MAINTENANCE_ERROR_CODE_CAPACITY],
                            const char *source,
                            size_t source_capacity);
static const char *audio_error_code(watch_audio_error_t error);
static void select_latest_error(maintenance_snapshot_t *snapshot,
                                const watch_state_snapshot_t *watch_snapshot);

esp_err_t maintenance_service_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    /* 上一次若停在半初始化路径，先恢复为可重试的空状态。 */
    cleanup_resources();
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL)
    {
        cleanup_resources();
        return ESP_ERR_NO_MEM;
    }

    s_selection_queue = xQueueCreate(MAINTENANCE_SELECTION_QUEUE_DEPTH,
                                     sizeof(maintenance_ble_selection_command_t));
    if (s_selection_queue == NULL)
    {
        cleanup_resources();
        return ESP_ERR_NO_MEM;
    }
    s_clear_queue = xQueueCreate(MAINTENANCE_CLEAR_QUEUE_DEPTH,
                                 sizeof(maintenance_ble_clear_command_t));
    if (s_clear_queue == NULL)
    {
        cleanup_resources();
        return ESP_ERR_NO_MEM;
    }

    memset(&s_candidate_list, 0, sizeof(s_candidate_list));
    memset(&s_selection_feedback, 0, sizeof(s_selection_feedback));
    s_snapshot_version = 1U;
    s_accepted_generation = 0U;
    s_initialized = true;
    ESP_LOGI(TAG, "维护契约已初始化，未创建额外任务");
    return ESP_OK;
}

esp_err_t maintenance_service_deinit(void)
{
    cleanup_resources();
    return ESP_OK;
}

esp_err_t maintenance_service_snapshot(maintenance_snapshot_t *snapshot,
                                       TickType_t timeout_ticks)
{
    if (!s_initialized || s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    maintenance_ble_candidate_list_t candidate_list = {0};
    maintenance_ble_selection_feedback_t selection_feedback = {0};
    uint64_t version = 0U;
    if (xSemaphoreTake(s_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    candidate_list = s_candidate_list;
    selection_feedback = s_selection_feedback;
    version = s_snapshot_version;
    xSemaphoreGive(s_mutex);

    /* 不持有维护 mutex 时只读取一次 watch_state，避免跨模块嵌套锁。 */
    watch_state_snapshot_t watch_snapshot = {0};
    esp_err_t err = watch_state_snapshot(&watch_snapshot, timeout_ticks);
    if (err != ESP_OK)
    {
        return err;
    }

    /* 直接投影到调用者提供的工作区，避免再创建一个约 4 KiB 的栈上临时值。 */
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->version = version;
    snapshot->ble.status = MAINTENANCE_CAPABILITY_AVAILABLE;
    snapshot->ble.observed_value = watch_snapshot.ble_connected;
    snapshot->has_bound_exoskeleton = watch_snapshot.has_bound_exoskeleton;
    snapshot->ble_transaction = watch_snapshot.ble_transaction;
    snapshot->gatt_ready = watch_snapshot.gatt_ready;
    snapshot->mtu_ready = watch_snapshot.mtu_ready;
    snapshot->negotiated_mtu = watch_snapshot.negotiated_mtu;
    snapshot->notify_ready = watch_snapshot.notify_ready;
    snapshot->device_state_available = watch_snapshot.device_state_available;
    snapshot->dataReady = watch_snapshot.dataReady;
    snapshot->status_fresh = watch_snapshot.status_fresh;
    snapshot->last_status_rx_ms = watch_snapshot.last_status_rx_ms;
    snapshot->exoskeleton_state = watch_snapshot.exoskeleton_state;
    snapshot->link_control_ready = watch_snapshot.link_control_ready;
    snapshot->unlock_session_valid = watch_snapshot.unlock_session_valid;
    snapshot->rental_phase = watch_snapshot.rental_phase;
    snapshot->control_block_reason = watch_snapshot.control_block_reason;
    snapshot->last_rental_result = watch_snapshot.last_rental_result;
    snapshot->pending_control = watch_snapshot.pending_control;
    snapshot->ble_diagnostics = watch_snapshot.ble_diagnostics;
    snapshot->gps.status = watch_snapshot.gps.status == WATCH_GPS_STATUS_UNAVAILABLE
                               ? MAINTENANCE_CAPABILITY_UNAVAILABLE
                               : MAINTENANCE_CAPABILITY_AVAILABLE;
    snapshot->gps.observed_value = watch_snapshot.gps.status == WATCH_GPS_STATUS_FIXED;
    snapshot->modem.status = watch_snapshot.modem.modem_update_sequence > 0U
                                 ? MAINTENANCE_CAPABILITY_AVAILABLE
                                 : MAINTENANCE_CAPABILITY_UNAVAILABLE;
    snapshot->modem.observed_value =
        watch_modem_network_connected(&watch_snapshot.modem);
    snapshot->modem_diagnostics = watch_snapshot.modem;
    snapshot->last_telemetry_result = watch_snapshot.last_telemetry_result;
    snapshot->last_telemetry_http_status = watch_snapshot.last_telemetry_http_status;
    snapshot->last_telemetry_attempt_monotonic_ms =
        watch_snapshot.last_telemetry_attempt_monotonic_ms;
    snapshot->last_telemetry_success_server_time_ms =
        watch_snapshot.last_telemetry_success_server_time_ms;
    snapshot->cloud_time_offset_ms = watch_snapshot.cloud_time_offset_ms;
    snapshot->last_telemetry_success_server_time_valid =
        watch_snapshot.last_telemetry_success_server_time_valid;
    snapshot->cloud_time_synchronized = watch_snapshot.cloud_time_synchronized;
    snapshot->cloud_config.status = watch_snapshot.cloud_configured
                                        ? MAINTENANCE_CAPABILITY_AVAILABLE
                                        : MAINTENANCE_CAPABILITY_UNAVAILABLE;
    snapshot->cloud_config.observed_value = watch_snapshot.cloud_configured;
    snapshot->cloud_status = watch_snapshot.cloud_status;
    snapshot->base_url_status = watch_snapshot.base_url_status;
    snapshot->apn_mode = watch_snapshot.apn_mode;
    snapshot->token_status = watch_snapshot.token_status;
    snapshot->certificate_status = watch_snapshot.certificate_status;
    snapshot->config_schema_status = watch_snapshot.config_schema_status;
    snapshot->battery_valid = watch_snapshot.battery_valid;
    snapshot->battery_percent = watch_snapshot.battery_percent;
    snapshot->audio_state = watch_snapshot.audio_state;
    snapshot->audio_error = watch_snapshot.audio_error;
    snapshot->selftest = watch_snapshot.selftest;
    snapshot->ble_candidates = candidate_list;
    snapshot->ble_selection_feedback = selection_feedback;
    if (watch_snapshot.has_bound_exoskeleton)
    {
        memcpy(snapshot->bound_exoskeleton_mac,
               watch_snapshot.bound_exoskeleton_mac,
               sizeof(snapshot->bound_exoskeleton_mac));
    }
    memcpy(snapshot->watch_id, watch_snapshot.watch_id, sizeof(snapshot->watch_id));
    if (watch_snapshot.token_status == WATCH_CONFIG_VALUE_CONFIGURED)
    {
        memcpy(snapshot->token_mask,
               CONFIG_SERVICE_TOKEN_MASK,
               sizeof(CONFIG_SERVICE_TOKEN_MASK));
    }
    (void)copy_error_code(snapshot->cloud_error_code,
                          watch_snapshot.cloud_error_code,
                          sizeof(watch_snapshot.cloud_error_code));
    (void)copy_error_code(snapshot->control_error_code,
                          watch_snapshot.control_error_code,
                          sizeof(watch_snapshot.control_error_code));
    (void)copy_error_code(snapshot->ble_latest_error_code,
                          watch_snapshot.ble_diagnostics.last_error_code,
                          sizeof(watch_snapshot.ble_diagnostics.last_error_code));
    (void)copy_error_code(snapshot->modem_error_code,
                          watch_snapshot.modem.last_error,
                          sizeof(watch_snapshot.modem.last_error));
    (void)copy_error_code(snapshot->telemetry_error_code,
                          watch_snapshot.telemetry_error_code,
                          sizeof(watch_snapshot.telemetry_error_code));
    select_latest_error(snapshot, &watch_snapshot);
    return ESP_OK;
}

esp_err_t maintenance_service_ble_ui_snapshot(
    maintenance_ble_ui_snapshot_t *snapshot,
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *snapshot = (maintenance_ble_ui_snapshot_t){
        .version = s_snapshot_version,
        .ble_candidates = s_candidate_list,
        .ble_selection_feedback = s_selection_feedback,
    };
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

maintenance_result_t maintenance_service_request_selftest(void)
{
    return maintenance_service_request_selftest_with_gps_context(false);
}

maintenance_result_t maintenance_service_request_modem_connect(void)
{
#if !LEGBOT_CAP_MODEM
    return MAINTENANCE_RESULT_NOT_READY;
#else
    if (!s_initialized)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    const esp_err_t error =
        modem_service_request_connect(MODEM_CONNECT_TRIGGER_MANUAL);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG, "维护页 4G 手动联网请求已提交");
        return MAINTENANCE_RESULT_OK;
    }
    if (error == ESP_ERR_INVALID_STATE || error == ESP_ERR_TIMEOUT)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    ESP_LOGE(TAG,
             "维护页提交 4G 手动联网失败，错误=0x%x",
             (unsigned)error);
    return MAINTENANCE_RESULT_ERROR;
#endif
}

maintenance_result_t maintenance_service_request_gps_acquisition(void)
{
#if !LEGBOT_CAP_GPS_TIME
    return MAINTENANCE_RESULT_NOT_READY;
#else
    if (!s_initialized)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    const gps_acquisition_purpose_t purpose =
#if LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        GPS_PURPOSE_LOCATION;
#else
        GPS_PURPOSE_TIME_SYNC;
#endif
    const esp_err_t error = gps_service_request_acquisition(
        purpose, GPS_TRIGGER_MANUAL, 0);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG, "维护页 GPS 手动搜星请求已提交");
        return MAINTENANCE_RESULT_OK;
    }
    if (error == ESP_ERR_INVALID_STATE || error == ESP_ERR_TIMEOUT)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    ESP_LOGE(TAG,
             "维护页提交 GPS 手动搜星失败，错误=0x%x",
             (unsigned)error);
    return MAINTENANCE_RESULT_ERROR;
#endif
}

maintenance_result_t maintenance_service_request_selftest_with_gps_context(
    bool indoor_confirmed)
{
    if (!s_initialized)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }

    esp_err_t err = legbot_selftest_service_start_with_gps_context(indoor_confirmed);
    if (err == ESP_OK)
    {
        return MAINTENANCE_RESULT_OK;
    }
    if (err == ESP_ERR_INVALID_STATE || err == ESP_ERR_TIMEOUT)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    ESP_LOGE(TAG, "维护入口启动自检失败，错误=0x%x", (unsigned)err);
    return MAINTENANCE_RESULT_ERROR;
}

maintenance_result_t maintenance_service_request_selftest_retry(
    uint32_t run_id,
    selftest_item_id_t item_id,
    bool indoor_confirmed)
{
    if (!s_initialized)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    if (run_id == 0U || item_id < SELFTEST_ITEM_AMOLED || item_id >= SELFTEST_ITEM_COUNT)
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }
    watch_state_snapshot_t snapshot = {0};
    esp_err_t err = watch_state_snapshot(&snapshot, 0);
    if (err == ESP_ERR_TIMEOUT)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    if (err != ESP_OK)
    {
        return MAINTENANCE_RESULT_ERROR;
    }
    if (snapshot.selftest.run_id != run_id ||
        snapshot.selftest.state != SELFTEST_RUN_FINISHED ||
        snapshot.selftest.selftest_results[item_id].outcome != SELFTEST_OUTCOME_FAIL)
    {
        return MAINTENANCE_RESULT_STALE;
    }

    err = selftest_service_request_retry_item(
        run_id,
        item_id,
        indoor_confirmed
            ? SELFTEST_GPS_FIX_CONTEXT_INDOOR_CONFIRMED
            : SELFTEST_GPS_FIX_CONTEXT_DEFAULT,
        0);
    if (err == ESP_OK)
    {
        return MAINTENANCE_RESULT_OK;
    }
    if (err == ESP_ERR_INVALID_STATE)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    if (err == ESP_ERR_TIMEOUT)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    ESP_LOGE(TAG,
             "维护入口重试自检失败，run_id=%lu，item=%d，错误=0x%x",
             (unsigned long)run_id,
             (int)item_id,
             (unsigned)err);
    return MAINTENANCE_RESULT_ERROR;
}

maintenance_result_t maintenance_service_publish_ble_candidates(
    uint32_t generation,
    const maintenance_ble_candidate_t candidates[],
    size_t candidate_count,
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_mutex == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    if (generation == 0U || candidate_count > MAINTENANCE_BLE_CANDIDATE_MAX_COUNT ||
        (candidate_count == 0U && candidates != NULL) ||
        (candidate_count > 0U && candidates == NULL))
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }

    maintenance_ble_candidate_list_t normalized_list = {
        .generation = generation,
        .count = (uint8_t)candidate_count,
    };
    for (size_t index = 0; index < candidate_count; ++index)
    {
        char *normalized = normalized_list.candidates[index].mac;
        if (!normalize_mac(candidates[index].mac, normalized))
        {
            return MAINTENANCE_RESULT_INVALID_ARGUMENT;
        }
        const size_t advertised_name_length = strnlen(candidates[index].advertised_name, MAINTENANCE_BLE_NAME_CAPACITY);
        if (advertised_name_length >= MAINTENANCE_BLE_NAME_CAPACITY)
        {
            return MAINTENANCE_RESULT_INVALID_ARGUMENT;
        }
        memcpy(normalized_list.candidates[index].advertised_name, candidates[index].advertised_name,
               advertised_name_length + 1U);
        normalized_list.candidates[index].rssi = candidates[index].rssi;
        for (size_t previous = 0; previous < index; ++previous)
        {
            if (strcmp(normalized, normalized_list.candidates[previous].mac) == 0)
            {
                return MAINTENANCE_RESULT_INVALID_ARGUMENT;
            }
        }
    }

    if (xSemaphoreTake(s_mutex, timeout_ticks) != pdTRUE)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    const bool stale_generation = generation < s_candidate_list.generation ||
                                  (generation == s_candidate_list.generation && s_accepted_generation == generation);
    if (stale_generation)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_STALE;
    }
    s_candidate_list = normalized_list;
    memset(&s_selection_feedback, 0, sizeof(s_selection_feedback));
    s_accepted_generation = 0U;
    if (s_snapshot_version < UINT64_MAX)
    {
        ++s_snapshot_version;
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG,
             "已发布 BLE 候选，代次=%lu，数量=%u",
             (unsigned long)generation,
             (unsigned)candidate_count);
    return MAINTENANCE_RESULT_OK;
}

maintenance_result_t maintenance_service_clear_ble_candidate_session(
    uint32_t generation,
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_mutex == NULL || s_selection_queue == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    if (generation == 0U)
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }
    if (xSemaphoreTake(s_mutex, timeout_ticks) != pdTRUE)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    if (generation < s_candidate_list.generation)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_STALE;
    }

    s_candidate_list = (maintenance_ble_candidate_list_t){
        .generation = generation,
    };
    memset(&s_selection_feedback, 0, sizeof(s_selection_feedback));
    s_accepted_generation = 0U;
    maintenance_ble_selection_command_t discarded_command = {0};
    while (xQueueReceive(s_selection_queue, &discarded_command, 0) == pdTRUE)
    {
        memset(&discarded_command, 0, sizeof(discarded_command));
    }
    if (s_snapshot_version < UINT64_MAX)
    {
        ++s_snapshot_version;
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG,
             "已关闭 BLE 候选会话并清空瞬态选择，代次=%lu",
             (unsigned long)generation);
    return MAINTENANCE_RESULT_OK;
}

maintenance_result_t maintenance_service_select_ble_candidate(
    uint32_t generation,
    const char mac[MAINTENANCE_BLE_MAC_CAPACITY],
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_mutex == NULL || s_selection_queue == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    maintenance_ble_selection_command_t command = {
        .generation = generation,
    };
    if (!normalize_mac(mac, command.mac))
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }

    if (xSemaphoreTake(s_mutex, timeout_ticks) != pdTRUE)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    if (generation != s_candidate_list.generation)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_STALE;
    }

    bool found = false;
    for (size_t index = 0; index < s_candidate_list.count; ++index)
    {
        if (strcmp(command.mac, s_candidate_list.candidates[index].mac) == 0)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_STALE;
    }
    if (s_accepted_generation == generation)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_BUSY;
    }
    if (xQueueSend(s_selection_queue, &command, 0) != pdTRUE)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_BUSY;
    }

    s_accepted_generation = generation;
    if (s_snapshot_version < UINT64_MAX)
    {
        ++s_snapshot_version;
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG,
             "BLE 候选选择命令已入队，代次=%lu，MAC=%s",
             (unsigned long)generation,
             command.mac);
    return MAINTENANCE_RESULT_OK;
}

maintenance_result_t maintenance_service_receive_ble_selection(
    maintenance_ble_selection_command_t *command,
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_selection_queue == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    if (command == NULL)
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }
    return xQueueReceive(s_selection_queue, command, timeout_ticks) == pdTRUE
               ? MAINTENANCE_RESULT_OK
               : MAINTENANCE_RESULT_EMPTY;
}

maintenance_result_t maintenance_service_publish_ble_selection_feedback(
    uint32_t generation,
    const char mac[MAINTENANCE_BLE_MAC_CAPACITY],
    maintenance_ble_selection_result_t result,
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_mutex == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    maintenance_ble_selection_feedback_t feedback = {
        .generation = generation,
        .result = result,
    };
    if (generation == 0U ||
        result < MAINTENANCE_BLE_SELECTION_CONNECTING ||
        result > MAINTENANCE_BLE_SELECTION_ERROR ||
        !normalize_mac(mac, feedback.mac))
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }
    if (xSemaphoreTake(s_mutex, timeout_ticks) != pdTRUE)
    {
        return MAINTENANCE_RESULT_BUSY;
    }
    const bool asynchronous_error = result == MAINTENANCE_BLE_SELECTION_ERROR;
    if (generation != s_candidate_list.generation)
    {
        if (asynchronous_error)
        {
            ESP_LOGW(TAG,
                     "BLE 旧代次异步选择错误已拒绝，代次=%lu，当前=%lu",
                     (unsigned long)generation,
                     (unsigned long)s_candidate_list.generation);
        }
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_STALE;
    }
    bool found = false;
    for (size_t index = 0; index < s_candidate_list.count; ++index)
    {
        if (strcmp(feedback.mac, s_candidate_list.candidates[index].mac) == 0)
        {
            found = true;
            break;
        }
    }
    if (!found)
    {
        xSemaphoreGive(s_mutex);
        return MAINTENANCE_RESULT_STALE;
    }
    s_selection_feedback = feedback;
    if (s_snapshot_version < UINT64_MAX)
    {
        ++s_snapshot_version;
    }
    xSemaphoreGive(s_mutex);
    return MAINTENANCE_RESULT_OK;
}

maintenance_result_t maintenance_service_confirm_clear_ble_binding(
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_clear_queue == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    const maintenance_ble_clear_command_t command = {
        .confirmed = true,
    };
    return xQueueSend(s_clear_queue, &command, timeout_ticks) == pdTRUE
               ? MAINTENANCE_RESULT_OK
               : MAINTENANCE_RESULT_BUSY;
}

maintenance_result_t maintenance_service_receive_ble_clear(
    maintenance_ble_clear_command_t *command,
    TickType_t timeout_ticks)
{
    if (!s_initialized || s_clear_queue == NULL)
    {
        return MAINTENANCE_RESULT_NOT_READY;
    }
    if (command == NULL)
    {
        return MAINTENANCE_RESULT_INVALID_ARGUMENT;
    }
    return xQueueReceive(s_clear_queue, command, timeout_ticks) == pdTRUE
               ? MAINTENANCE_RESULT_OK
               : MAINTENANCE_RESULT_EMPTY;
}

static void cleanup_resources(void)
{
    /* 动态 mutex 也是 queue 内核对象，统一使用 vQueueDelete 回收。 */
    if (s_selection_queue != NULL)
    {
        vQueueDelete(s_selection_queue);
        s_selection_queue = NULL;
    }
    if (s_clear_queue != NULL)
    {
        vQueueDelete(s_clear_queue);
        s_clear_queue = NULL;
    }
    if (s_mutex != NULL)
    {
        vQueueDelete(s_mutex);
        s_mutex = NULL;
    }
    memset(&s_candidate_list, 0, sizeof(s_candidate_list));
    memset(&s_selection_feedback, 0, sizeof(s_selection_feedback));
    s_snapshot_version = 0U;
    s_accepted_generation = 0U;
    s_initialized = false;
}

static bool normalize_mac(const char input[MAINTENANCE_BLE_MAC_CAPACITY],
                          char output[MAINTENANCE_BLE_MAC_CAPACITY])
{
    if (input == NULL || output == NULL)
    {
        return false;
    }
    const size_t length = strnlen(input, MAINTENANCE_BLE_MAC_CAPACITY);
    if (length != MAINTENANCE_BLE_MAC_CAPACITY - 1U)
    {
        return false;
    }
    static const size_t separator_positions[] = {2U, 5U, 8U, 11U, 14U};
    for (size_t index = 0; index < MAINTENANCE_BLE_MAC_CAPACITY - 1U; ++index)
    {
        bool separator = false;
        for (size_t separator_index = 0;
             separator_index < sizeof(separator_positions) / sizeof(separator_positions[0]);
             ++separator_index)
        {
            separator = separator || index == separator_positions[separator_index];
        }
        if (separator)
        {
            if (input[index] != ':')
            {
                return false;
            }
            output[index] = ':';
        }
        else
        {
            unsigned char value = (unsigned char)input[index];
            if (!isxdigit(value))
            {
                return false;
            }
            output[index] = (char)toupper(value);
        }
    }
    output[MAINTENANCE_BLE_MAC_CAPACITY - 1U] = '\0';
    return true;
}

static bool is_stable_error_code(const char *code)
{
    if (code == NULL)
    {
        return false;
    }
    static const char *const prefixes[] = {
        "DRV_", "POWER_", "AUDIO_", "UI_", "BLE_", "CLOUD_", "MODEM_", "GPS_"};
    for (size_t index = 0; index < sizeof(prefixes) / sizeof(prefixes[0]); ++index)
    {
        if (strncmp(code, prefixes[index], strlen(prefixes[index])) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool copy_error_code(char destination[MAINTENANCE_ERROR_CODE_CAPACITY],
                            const char *source,
                            size_t source_capacity)
{
    if (destination == NULL || source == NULL || source_capacity == 0U)
    {
        return false;
    }
    const size_t length = strnlen(source, source_capacity);
    if (length == 0U || length >= source_capacity ||
        length >= MAINTENANCE_ERROR_CODE_CAPACITY)
    {
        return false;
    }
    memcpy(destination, source, length + 1U);
    return true;
}

static const char *audio_error_code(watch_audio_error_t error)
{
    static const char *const codes[] = {
        [WATCH_AUDIO_ERROR_NONE] = "AUDIO_OK",
        [WATCH_AUDIO_ERROR_NOT_READY] = "AUDIO_NOT_READY",
        [WATCH_AUDIO_ERROR_RESOURCE_MISSING] = "AUDIO_RESOURCE_MISSING",
        [WATCH_AUDIO_ERROR_CORRUPT] = "AUDIO_RESOURCE_CORRUPT",
        [WATCH_AUDIO_ERROR_UNSUPPORTED] = "AUDIO_RESOURCE_UNSUPPORTED",
        [WATCH_AUDIO_ERROR_IO_FAILED] = "AUDIO_IO_FAILED",
        [WATCH_AUDIO_ERROR_BUSY] = "AUDIO_BUSY",
        [WATCH_AUDIO_ERROR_CODEC_FAILED] = "AUDIO_CODEC_FAILED",
        [WATCH_AUDIO_ERROR_I2S_FAILED] = "AUDIO_I2S_FAILED",
        [WATCH_AUDIO_ERROR_PA_FAILED] = "AUDIO_PA_FAILED",
    };
    return error >= WATCH_AUDIO_ERROR_NONE && error <= WATCH_AUDIO_ERROR_PA_FAILED
               ? codes[error]
               : "AUDIO_UNKNOWN";
}

static void select_latest_error(maintenance_snapshot_t *snapshot,
                                const watch_state_snapshot_t *watch_snapshot)
{
    char candidate[MAINTENANCE_ERROR_CODE_CAPACITY] = {0};
    snapshot->has_latest_error = false;
    (void)copy_error_code(snapshot->latest_error_code,
                          MAINTENANCE_NO_ERROR_CODE,
                          sizeof(MAINTENANCE_NO_ERROR_CODE));

    for (size_t index = 0; index < SELFTEST_ITEM_COUNT; ++index)
    {
        const selftest_item_result_t *item = &watch_snapshot->selftest.selftest_results[index];
        if (item->outcome == SELFTEST_OUTCOME_FAIL &&
            copy_error_code(candidate, item->detail_code, sizeof(item->detail_code)) &&
            is_stable_error_code(candidate) &&
            copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
        {
            snapshot->has_latest_error = true;
        }
    }
    if (snapshot->has_latest_error)
    {
        return;
    }

    if (watch_snapshot->last_telemetry_result !=
            WATCH_TELEMETRY_RESULT_NEVER_SENT &&
        copy_error_code(candidate,
                        watch_snapshot->telemetry_error_code,
                        sizeof(watch_snapshot->telemetry_error_code)) &&
        !(watch_snapshot->last_telemetry_result ==
              WATCH_TELEMETRY_RESULT_ACCEPTED &&
          strcmp(candidate, "CLOUD_OK") == 0) &&
        is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code,
                        candidate,
                        sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }

    if (copy_error_code(candidate,
                        watch_snapshot->gps.error_code,
                        sizeof(watch_snapshot->gps.error_code)) &&
        strcmp(candidate, "GPS_OK") != 0 &&
        strcmp(candidate, "GPS_OFF") != 0 &&
        strcmp(candidate, "GPS_SEARCHING") != 0 &&
        is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }

    if (copy_error_code(candidate,
                        watch_snapshot->modem.last_error,
                        sizeof(watch_snapshot->modem.last_error)) &&
        strcmp(candidate, "MODEM_OK") != 0 &&
        strcmp(candidate, "MODEM_NOT_STARTED") != 0 &&
        is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }

    if (copy_error_code(candidate,
                        watch_snapshot->cloud_error_code,
                        sizeof(watch_snapshot->cloud_error_code)) &&
        strcmp(candidate, "CLOUD_OK") != 0 && is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }

    if (copy_error_code(candidate,
                        watch_snapshot->ble_error_code,
                        sizeof(watch_snapshot->ble_error_code)) &&
        strcmp(candidate, "BLE_OK") != 0 &&
        strcmp(candidate, "BLE_NOT_STARTED") != 0 &&
        is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }

    const char *power_code = watch_snapshot->battery_error_code;
    if (copy_error_code(candidate,
                        power_code,
                        sizeof(watch_snapshot->battery_error_code)) &&
        strcmp(candidate, MAINTENANCE_POWER_NOT_UPDATED) != 0 &&
        strcmp(candidate, "POWER_OK") != 0 && is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }
    const char *driver_code = watch_snapshot->battery_driver_error_code;
    if (copy_error_code(candidate,
                        driver_code,
                        sizeof(watch_snapshot->battery_driver_error_code)) &&
        strcmp(candidate, MAINTENANCE_POWER_DRIVER_NOT_READY) != 0 &&
        is_stable_error_code(candidate) &&
        copy_error_code(snapshot->latest_error_code, candidate, sizeof(candidate)))
    {
        snapshot->has_latest_error = true;
        return;
    }

    const char *audio_code = audio_error_code(watch_snapshot->audio_error);
    if (watch_snapshot->audio_error != WATCH_AUDIO_ERROR_NONE &&
        watch_snapshot->audio_error != WATCH_AUDIO_ERROR_NOT_READY &&
        copy_error_code(snapshot->latest_error_code,
                        audio_code,
                        MAINTENANCE_ERROR_CODE_CAPACITY))
    {
        snapshot->has_latest_error = true;
    }
}
