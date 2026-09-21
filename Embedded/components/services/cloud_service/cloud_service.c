/**
 * @file     cloud_service.c
 * @brief    租赁授权与 telemetry 云服务实现
 * @details  由固定 cloud_task 复用 modem HTTPS transport，并提供单次产品快照到有界 JSON 的严格映射。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "cloud_service.h"

#include "cloud_payment_schedule.h"

#include <ctype.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "time_service.h"

#ifndef CLOUD_SERVICE_CODEC_ONLY
#include <stdatomic.h>

#include "ble_service.h"
#include "config_service.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "gps_service.h"
#include "modem_service.h"
#include "state_service.h"
#endif

#ifndef CLOUD_SERVICE_CODEC_ONLY
static const char *TAG = "SVC_CLOUD";

typedef enum
{
    CLOUD_ACTIVE_NONE = 0,         /**< 当前没有 modem 请求。 */
    CLOUD_ACTIVE_RENTAL_WAIT_MODEM, /**< 租赁查询正等待 4G 显式联网。 */
    CLOUD_ACTIVE_RENTAL,           /**< 当前 modem 请求属于租赁查询。 */
    CLOUD_ACTIVE_TELEMETRY,        /**< 当前 modem 请求属于 telemetry 上报。 */
} cloud_active_kind_t;

typedef struct
{
    cloud_active_kind_t active_kind;              /**< 当前唯一 modem 请求类型。 */
    uint32_t active_request_id;                   /**< 当前唯一 modem 请求 ID。 */
    cloud_rental_request_t active_rental;         /**< 当前租赁请求的不可变值。 */
    uint64_t rental_wait_deadline_ms;             /**< 支付等待 4G 联网的单调毫秒截止。 */
    bool active_rental_context_valid;             /**< 等待期间租赁事务身份是否仍有效。 */
    cloud_rental_request_t rental_session;        /**< 当前自动支付会话的固定逻辑身份。 */
    cloud_payment_schedule_t rental_schedule;     /**< FAST/SLOW/BACKOFF 纯 deadline 状态。 */
    bool rental_session_valid;                    /**< 是否存在可继续复查的支付会话。 */
    bool rental_lease_held;                       /**< 当前是否持有支付 RF lease。 */
    cloud_rental_request_t authorized_session;    /**< 当前保持 4G 在线的已授权 BLE 链路身份。 */
    bool authorized_online_hold;                  /**< 当前授权链路是否持续持有支付 RF lease。 */
    bool gps_pause_requested;                     /**< 是否已为当前支付 deadline 请求 GPS 让出 RF。 */
    bool telemetry_lease_held;                    /**< 当前是否持有遥测 RF lease。 */
    uint64_t telemetry_wait_deadline_ms;          /**< 本 telemetry 窗口等待 4G 就绪的硬截止时间。 */
    cloud_telemetry_snapshot_t latest;            /**< 唯一可覆盖 telemetry 最新槽。 */
    bool latest_valid;                            /**< 最新槽是否含当前绑定的完整快照。 */
    bool latest_pending_send;                     /**< 最新快照是否等待提交给 modem。 */
    bool active_telemetry_identity_valid;         /**< 活动 telemetry 是否仍属于当前绑定。 */
    uint32_t active_telemetry_binding_generation; /**< 活动 telemetry 捕获的 BLE 物理链路代次。 */
    bool binding_tracked;                         /**< 是否已为合法绑定建立周期锚点。 */
    bool cloud_configured;                        /**< 最近一次产品事实是否允许云请求。 */
    bool modem_ready;                             /**< 最近一次产品事实中的 modem 就绪状态。 */
    char bound_mac[WATCH_BLE_MAC_CAPACITY];       /**< 当前周期锚定的规范绑定 MAC。 */
    uint64_t next_window_ms;                      /**< 下一固定相位窗口的单调毫秒。 */
} cloud_owner_state_t;

typedef struct
{
    int64_t server_time_ms; /**< 待提交的服务器 UTC 毫秒。 */
    uint64_t monotonic_ms;  /**< 同一终态捕获的单调毫秒。 */
    int64_t offset_ms;      /**< 身份原子授权后计算出的 boot 云时间偏移。 */
} cloud_time_commit_context_t;
#endif

/** 当前 boot 云时间是否已从合法响应同步。 */
static bool s_boot_time_synchronized;
/** 当前 boot 的 UTC 毫秒减单调启动毫秒偏移。 */
static int64_t s_boot_cloud_offset_ms;

#ifndef CLOUD_SERVICE_CODEC_ONLY
/** cloud_task 的 typed 请求队列。 */
static QueueHandle_t s_request_queue;
/** cloud_task 的 typed 结果队列。 */
static QueueHandle_t s_result_queue;
/** cloud_task 的单槽事件唤醒队列，不承载业务请求。 */
static QueueHandle_t s_wake_queue;
/** 服务停止锁存，队列满时也不会丢失。 */
static atomic_bool s_stop_requested;
/** cloud_task 是否正在拥有队列与事务。 */
static atomic_bool s_owner_running;
/** BLE owner 锁存的支付取消请求 ID。 */
static atomic_uint s_rental_cancel_request_id;
/** BLE owner 锁存的已结束授权物理链路代次。 */
static atomic_uint s_authorized_link_end_generation;
/** UI/BLE owner 锁存的支付立即检查请求 ID。 */
static atomic_uint s_rental_accelerate_request_id;
/** 当前可同步接受一次人工加速的支付 request ID。 */
static atomic_uint s_rental_accelerate_eligible_request_id;
/** 当前已取得调度槽并正在联网或查询的支付 request ID。 */
static atomic_uint s_rental_in_flight_request_id;
/** 结果队列满时保留的唯一终态。 */
static cloud_rental_result_t s_pending_result;
/** 是否存在尚未送达 ble_task 的终态。 */
static bool s_pending_result_valid;
/** cloud_task 诊断更新单调序号。 */
static uint32_t s_cloud_update_sequence;
/** state queue 暂满时保留的最新 cloud 诊断真值。 */
static watch_cloud_update_t s_pending_cloud_update;
/** 是否存在尚未入队的 cloud 诊断真值。 */
static bool s_pending_cloud_update_dirty;
/** telemetry 诊断更新单调序号。 */
static uint32_t s_telemetry_update_sequence;
/** state_service 暂忙时保留的唯一 owned telemetry 终态。 */
static watch_telemetry_update_t s_pending_telemetry_update;
/** 是否存在尚未交付 state_service 的 telemetry 终态。 */
static bool s_pending_telemetry_update_dirty;
/** 固定 cloud_task 的单 owner telemetry 与请求编排状态。 */
static cloud_owner_state_t s_owner;
/** cloud 启动准备阶段的产品状态工作区，避免完整快照占用 main 任务栈。 */
static watch_state_snapshot_t s_prepare_product_state;
/** 租赁事务校验使用的配置工作区，由 cloud owner 独占。 */
static config_service_snapshot_t s_rental_config_scratch;
/** 租赁事务校验使用的产品状态工作区，由 cloud owner 独占。 */
static watch_state_snapshot_t s_rental_state_scratch;
/** cloud 单 owner 的 HTTPS 请求工作区，避免按值正文占用 cloud_task 栈。 */
static ml307r_https_post_request_t s_transport_request_scratch;
/** cloud 单 owner 的 HTTPS 响应工作区，避免按值响应占用 cloud_task 栈。 */
static ml307r_https_response_t s_transport_response_scratch;
/** 当前 boot 内跨端点、跨 cloud owner 周期单调的 transport 请求 ID。 */
static uint32_t s_next_transport_request_id;
#ifdef CLOUD_SERVICE_TEST
/** production-C 测试最近一次实际路由到 telemetry handler 的完整终态。 */
static ml307r_https_response_t s_test_last_telemetry_terminal;
/** production-C 测试是否已观察到 telemetry handler 终态。 */
static bool s_test_last_telemetry_terminal_valid;
/** production-C owner 测试是否启用依赖注入。 */
static bool s_test_dependencies_enabled;
/** production-C owner 测试注入的产品状态。 */
static watch_state_snapshot_t s_test_watch_state;
/** production-C owner 测试注入的产品状态读取结果。 */
static esp_err_t s_test_watch_state_result;
/** production-C owner 测试注入的云配置。 */
static config_service_snapshot_t s_test_config;
/** production-C owner 测试注入的云配置读取结果。 */
static esp_err_t s_test_config_result;
/** production-C owner 测试注入的 modem 提交结果。 */
static esp_err_t s_test_modem_submit_result;
/** production-C owner 测试累计的 modem 提交次数。 */
static uint32_t s_test_modem_submit_count;
/** production-C 测试注入的 4G 显式联网请求结果。 */
static esp_err_t s_test_modem_connect_result;
/** production-C 测试累计的 4G 显式联网请求次数。 */
static uint32_t s_test_modem_connect_count;
/** production-C 测试累计的 RF lease 归还次数。 */
static uint32_t s_test_modem_release_count;
/** production-C 测试一次性注入的 modem 终态返回值。 */
static esp_err_t s_test_modem_receive_result;
/** production-C 测试一次性注入的 modem 终态。 */
static ml307r_https_response_t s_test_modem_response;
#endif
#endif

static bool canonical_mac_is_valid(const char *mac, size_t capacity);
static size_t json_field_count(const cJSON *object, const char *name);
static bool parse_server_time(const cJSON *item, int64_t *server_time_ms);
static void set_telemetry_result(cloud_telemetry_response_t *result,
                                 cloud_telemetry_result_t telemetry_result,
                                 bool accepted,
                                 int64_t server_time_ms,
                                 const char *error_code);
static bool parse_json_object(const uint8_t *body,
                              size_t body_length,
                              cJSON **root);
static bool json_body_contains_embedded_nul(const uint8_t *body,
                                            size_t body_length);
static bool remote_string_is_bounded(const cJSON *item, size_t max_length);
static const char *telemetry_http_error_code(uint16_t http_status);
static void initialize_result(const cloud_rental_request_t *request,
                              uint16_t http_status,
                              cloud_rental_result_t *result);
static void set_result(cloud_rental_result_t *result,
                       watch_rental_result_t rental_result,
                       bool can_unlock,
                       const char *error_code,
                       const char *user_reason);
static bool consume_json_tail(const char *cursor, const char *end);

#ifndef CLOUD_SERVICE_CODEC_ONLY
static cloud_transport_result_t map_transport_result(ml307r_https_error_t error);
static watch_telemetry_result_t map_telemetry_result(
    cloud_telemetry_result_t result);
static bool execute_rental_request(const cloud_rental_request_t *request,
                                   uint32_t transport_request_id);
static bool deliver_result(const cloud_rental_result_t *result);
static void publish_cloud_diagnostic(const cloud_rental_result_t *result);
static void publish_telemetry_diagnostic(
    const cloud_telemetry_response_t *response,
    uint32_t request_id,
    uint64_t monotonic_ms,
    bool time_synchronized,
    int64_t offset_ms);
static void clear_latest_telemetry(void);
static void reset_owner_state(void);
static void observe_binding(const watch_state_snapshot_t *state,
                            uint64_t monotonic_ms);
static void advance_telemetry_window(uint64_t monotonic_ms);
static void capture_due_telemetry(uint64_t monotonic_ms);
static uint32_t next_transport_request_id(void);
static const char *telemetry_submit_error_code(esp_err_t error);
static bool start_telemetry_request(uint64_t monotonic_ms);
static bool rental_state_matches_request(
    const watch_state_snapshot_t *state,
    const cloud_rental_request_t *request);
static bool rental_requests_match(const cloud_rental_request_t *first,
                                  const cloud_rental_request_t *second);
static void process_rental_control_signals(uint64_t monotonic_ms);
static void cancel_rental_session(const char *reason);
static void refresh_rental_accelerate_eligibility(void);
static void release_rental_lease(void);
static void release_authorized_online_hold(const char *reason);
static bool authorized_online_hold_matches_state(
    const watch_state_snapshot_t *state);
static void release_telemetry_lease(void);
static void note_telemetry_lease_acquired(uint64_t monotonic_ms);
static void defer_telemetry_to_next_window(const char *reason);
static void release_network_leases_for_result_backpressure(void);
static void cancel_active_telemetry(const char *reason);
static cloud_payment_outcome_t classify_payment_outcome(
    const cloud_rental_result_t *result);
static void finish_payment_attempt(const cloud_rental_result_t *result,
                                   uint64_t monotonic_ms);
static bool load_rental_context(
    const cloud_rental_request_t *request,
    config_service_snapshot_t *config,
    watch_state_snapshot_t *state);
static void publish_rental_failure(
    const cloud_rental_request_t *request,
    watch_rental_result_t result,
    const char *error_code,
    const char *user_reason);
static bool try_start_rental_request(uint64_t monotonic_ms,
                                     uint64_t next_cloud_window_ms);
static void advance_rental_modem_wait(uint64_t monotonic_ms);
static void finish_active_operation(void);
static void handle_rental_terminal(const ml307r_https_response_t *response,
                                   esp_err_t receive_error,
                                   uint64_t monotonic_ms);
static esp_err_t owner_note_and_persist_cloud_time(
    int64_t server_time_ms,
    uint64_t monotonic_ms);
static esp_err_t commit_cloud_time_if_authorized(void *context);
static bool active_telemetry_identity_is_current(void);
static void handle_telemetry_terminal(const ml307r_https_response_t *response,
                                      esp_err_t receive_error);
static void process_modem_terminal(uint64_t monotonic_ms);
static esp_err_t owner_begin(void);
static esp_err_t owner_step(uint64_t monotonic_ms);
static TickType_t owner_next_wait_ticks(uint64_t monotonic_ms);
static void wake_owner(void);
static void retry_pending_telemetry_diagnostic(void);
static void owner_end(void);
static esp_err_t owner_watch_state_snapshot(
    watch_state_snapshot_t *snapshot,
    TickType_t timeout_ticks);
static esp_err_t owner_config_snapshot(config_service_snapshot_t *snapshot,
                                       TickType_t timeout_ticks);
static esp_err_t owner_modem_submit(
    const ml307r_https_post_request_t *request,
    TickType_t timeout_ticks);
static esp_err_t owner_modem_request_connect(void);
static esp_err_t owner_modem_request_telemetry_connect(void);
static void owner_modem_release(modem_network_lease_t lease);
static void owner_modem_cancel(uint32_t request_id);
static esp_err_t owner_modem_receive(ml307r_https_response_t *response,
                                     TickType_t timeout_ticks);
static esp_err_t owner_publish_telemetry(
    const watch_telemetry_update_t *update,
    TickType_t timeout_ticks);
static void secure_zero(void *buffer, size_t length);
#endif

esp_err_t cloud_service_build_rental_http(
    const cloud_rental_request_t *request,
    const char watch_id[WATCH_STATE_WATCH_ID_CAPACITY],
    int64_t timestamp_ms,
    cloud_rental_http_request_t *http)
{
    if (request == NULL || http == NULL || request->request_id == 0U ||
        request->link_generation == 0U || timestamp_ms < 0 ||
        !canonical_mac_is_valid(request->exoskeleton_mac,
                                sizeof(request->exoskeleton_mac)) ||
        !canonical_mac_is_valid(watch_id, WATCH_STATE_WATCH_ID_CAPACITY))
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(http, 0, sizeof(*http));
    memcpy(http->path,
           CLOUD_RENTAL_CHECK_PATH,
           sizeof(CLOUD_RENTAL_CHECK_PATH));
    const int written = snprintf(
        (char *)http->body,
        sizeof(http->body),
        "{\"watch_id\":\"%s\",\"exoskeleton_mac\":\"%s\",\"timestamp_ms\":%" PRId64 "}",
        watch_id,
        request->exoskeleton_mac,
        timestamp_ms);
    if (written < 0 || (size_t)written >= sizeof(http->body))
    {
        memset(http, 0, sizeof(*http));
        return ESP_ERR_INVALID_SIZE;
    }
    http->body_length = (uint16_t)written;
    return ESP_OK;
}

esp_err_t cloud_service_capture_telemetry(
    uint64_t monotonic_ms,
    cloud_telemetry_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    if (monotonic_ms > (uint64_t)INT64_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }

    watch_state_snapshot_t product_state = {0};
#ifdef CLOUD_SERVICE_CODEC_ONLY
    const esp_err_t state_error = watch_state_snapshot(&product_state, 0U);
#else
    const esp_err_t state_error =
        owner_watch_state_snapshot(&product_state, 0U);
#endif
    if (state_error != ESP_OK)
    {
        return state_error;
    }
    if (!canonical_mac_is_valid(product_state.watch_id,
                                sizeof(product_state.watch_id)) ||
        !product_state.has_bound_exoskeleton ||
        !canonical_mac_is_valid(product_state.bound_exoskeleton_mac,
                                sizeof(product_state.bound_exoskeleton_mac)) ||
        !product_state.battery_valid || product_state.battery_percent > 100U ||
        product_state.gps.status < WATCH_GPS_STATUS_OFF ||
        product_state.gps.status > WATCH_GPS_STATUS_UNAVAILABLE)
    {
        return ESP_ERR_INVALID_ARG;
    }

    const bool exoskeleton_data_valid =
        product_state.ble_connected && product_state.device_state_available &&
        product_state.dataReady && product_state.status_fresh &&
        product_state.last_status_rx_ms != 0U &&
        monotonic_ms >= product_state.last_status_rx_ms &&
        monotonic_ms - product_state.last_status_rx_ms < STATUS_STALE_MS;
    if (!exoskeleton_data_valid ||
        !isfinite(product_state.exoskeleton_state.steps) ||
        product_state.exoskeleton_state.steps < 0.0F ||
        product_state.exoskeleton_state.battery_level > 100U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    watch_gps_status_t telemetry_gps_status = product_state.gps.status;
    bool coordinates_valid = false;
    if (product_state.gps.status == WATCH_GPS_STATUS_FIXED)
    {
        if (!product_state.gps.coordinates_valid ||
            !isfinite(product_state.gps.latitude) ||
            !isfinite(product_state.gps.longitude) ||
            product_state.gps.latitude < -90.0 ||
            product_state.gps.latitude > 90.0 ||
            product_state.gps.longitude < -180.0 ||
            product_state.gps.longitude > 180.0)
        {
            return ESP_ERR_INVALID_ARG;
        }
        coordinates_valid = product_state.gps.last_fix_rx_ms != 0U &&
                            monotonic_ms >= product_state.gps.last_fix_rx_ms &&
                            monotonic_ms - product_state.gps.last_fix_rx_ms <
                                WATCH_GPS_FIX_FRESHNESS_MS;
        if (!coordinates_valid)
        {
            telemetry_gps_status = WATCH_GPS_STATUS_SEARCHING;
        }
    }
    else if (product_state.gps.coordinates_valid)
    {
        return ESP_ERR_INVALID_ARG;
    }

    cloud_telemetry_snapshot_t captured = {
        .binding_generation = product_state.ble_link_generation,
        .timestamp_ms = cloud_service_timestamp_ms(monotonic_ms),
        .gps_status = telemetry_gps_status,
        .watch_battery = product_state.battery_percent,
        .exoskeleton_data_valid = exoskeleton_data_valid,
        .coordinates_valid = coordinates_valid,
    };
    if (exoskeleton_data_valid)
    {
        captured.steps = product_state.exoskeleton_state.steps;
        captured.exoskeleton_battery =
            product_state.exoskeleton_state.battery_level;
    }
    if (coordinates_valid)
    {
        captured.latitude = product_state.gps.latitude;
        captured.longitude = product_state.gps.longitude;
    }
    memcpy(captured.watch_id,
           product_state.watch_id,
           sizeof(captured.watch_id));
    memcpy(captured.exoskeleton_mac,
           product_state.bound_exoskeleton_mac,
           sizeof(captured.exoskeleton_mac));
    *snapshot = captured;
    return ESP_OK;
}

const char *cloud_service_telemetry_gps_status_name(
    watch_gps_status_t status)
{
    switch (status)
    {
    case WATCH_GPS_STATUS_FIXED:
        return "fixed";
    case WATCH_GPS_STATUS_SEARCHING:
        return "searching";
    case WATCH_GPS_STATUS_UNAVAILABLE:
        return "unavailable";
    case WATCH_GPS_STATUS_OFF:
        return "off";
    default:
        return NULL;
    }
}

esp_err_t cloud_service_build_telemetry_http(
    const cloud_telemetry_snapshot_t *snapshot,
    size_t body_capacity,
    cloud_telemetry_http_request_t *http)
{
    if (http == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(http, 0, sizeof(*http));
    const char *gps_status_text =
        snapshot == NULL
            ? NULL
            : cloud_service_telemetry_gps_status_name(snapshot->gps_status);
    if (snapshot == NULL || body_capacity == 0U ||
        body_capacity > sizeof(http->body) ||
        !canonical_mac_is_valid(snapshot->watch_id,
                                sizeof(snapshot->watch_id)) ||
        !canonical_mac_is_valid(snapshot->exoskeleton_mac,
                                sizeof(snapshot->exoskeleton_mac)) ||
        snapshot->timestamp_ms < 0 ||
        snapshot->timestamp_ms > CLOUD_SERVER_TIME_MAX_MS ||
        snapshot->watch_battery > 100U ||
        gps_status_text == NULL ||
        snapshot->coordinates_valid !=
            (snapshot->gps_status == WATCH_GPS_STATUS_FIXED) ||
        (snapshot->exoskeleton_data_valid &&
         (!isfinite(snapshot->steps) || snapshot->steps < 0.0F ||
          snapshot->exoskeleton_battery > 100U)) ||
        (snapshot->coordinates_valid &&
         (!isfinite(snapshot->latitude) ||
          !isfinite(snapshot->longitude) || snapshot->latitude < -90.0 ||
          snapshot->latitude > 90.0 || snapshot->longitude < -180.0 ||
          snapshot->longitude > 180.0)))
    {
        return ESP_ERR_INVALID_ARG;
    }

    char steps_text[32] = "null";
    char exoskeleton_battery_text[8] = "null";
    char latitude_text[32] = "null";
    char longitude_text[32] = "null";
    if (snapshot->exoskeleton_data_valid)
    {
        const unsigned exoskeleton_battery =
            (unsigned)snapshot->exoskeleton_battery;
        const int steps_written = snprintf(steps_text,
                                           sizeof(steps_text),
                                           "%.9g",
                                           (double)snapshot->steps);
        const int battery_written = snprintf(exoskeleton_battery_text,
                                             sizeof(exoskeleton_battery_text),
                                             "%u",
                                             exoskeleton_battery);
        if (steps_written < 0 ||
            (size_t)steps_written >= sizeof(steps_text) ||
            battery_written < 0 ||
            (size_t)battery_written >= sizeof(exoskeleton_battery_text))
        {
            return ESP_ERR_INVALID_SIZE;
        }
    }
    if (snapshot->coordinates_valid)
    {
        const int latitude_written = snprintf(latitude_text,
                                              sizeof(latitude_text),
                                              "%.6f",
                                              snapshot->latitude);
        const int longitude_written = snprintf(longitude_text,
                                               sizeof(longitude_text),
                                               "%.6f",
                                               snapshot->longitude);
        if (latitude_written < 0 ||
            (size_t)latitude_written >= sizeof(latitude_text) ||
            longitude_written < 0 ||
            (size_t)longitude_written >= sizeof(longitude_text))
        {
            return ESP_ERR_INVALID_SIZE;
        }
    }

    memcpy(http->path, CLOUD_TELEMETRY_PATH, sizeof(CLOUD_TELEMETRY_PATH));
    const int body_written = snprintf(
        (char *)http->body,
        body_capacity,
        "{\"watch_id\":\"%s\",\"exoskeleton_mac\":\"%s\",\"timestamp_ms\":%" PRId64
        ",\"steps\":%s,\"exoskeleton_battery\":%s,\"watch_battery\":%u,"
        "\"gps_status\":\"%s\",\"latitude\":%s,\"longitude\":%s}",
        snapshot->watch_id,
        snapshot->exoskeleton_mac,
        snapshot->timestamp_ms,
        steps_text,
        exoskeleton_battery_text,
        (unsigned)snapshot->watch_battery,
        gps_status_text,
        latitude_text,
        longitude_text);
    if (body_written < 0 || (size_t)body_written >= body_capacity ||
        (size_t)body_written > UINT16_MAX)
    {
        memset(http, 0, sizeof(*http));
        return ESP_ERR_INVALID_SIZE;
    }
    http->body_length = (uint16_t)body_written;
    return ESP_OK;
}

bool cloud_service_response_matches_request(
    const cloud_rental_request_t *request,
    uint32_t response_request_id)
{
    return request != NULL && request->request_id != 0U &&
           response_request_id == request->request_id;
}

esp_err_t cloud_service_parse_rental_http(
    const cloud_rental_request_t *request,
    uint16_t http_status,
    cloud_transport_result_t transport,
    const uint8_t *body,
    size_t body_length,
    cloud_rental_result_t *result)
{
    if (request == NULL || result == NULL || request->request_id == 0U ||
        request->link_generation == 0U ||
        !canonical_mac_is_valid(request->exoskeleton_mac,
                                sizeof(request->exoskeleton_mac)) ||
        transport < CLOUD_TRANSPORT_OK ||
        transport > CLOUD_TRANSPORT_INTERNAL ||
        (body == NULL && body_length != 0U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    initialize_result(request, http_status, result);
    if (transport != CLOUD_TRANSPORT_OK)
    {
        if (transport == CLOUD_TRANSPORT_TLS)
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_TLS_ERROR,
                       false,
                       "CLOUD_TLS_FAILED",
                       "配置或证书错误");
        }
        else if (transport == CLOUD_TRANSPORT_TIMEOUT)
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_TIMEOUT,
                       false,
                       "CLOUD_TIMEOUT",
                       "云端请求超时");
        }
        else
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
                       false,
                       "CLOUD_UNREACHABLE",
                       "云端不可达");
        }
        return ESP_OK;
    }
    if (http_status < 200U || http_status >= 300U)
    {
        if (http_status == 401U)
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_TOKEN_INVALID,
                       false,
                       "CLOUD_TOKEN_INVALID",
                       "设备鉴权配置错误");
        }
        else if (http_status == 400U)
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_REQUEST_INVALID,
                       false,
                       "CLOUD_REQUEST_INVALID",
                       "授权请求无效");
        }
        else if (http_status >= 500U)
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_SERVER_ERROR,
                       false,
                       "CLOUD_SERVER_ERROR",
                       "云端服务异常");
        }
        else
        {
            set_result(result,
                       WATCH_RENTAL_RESULT_HTTP_ERROR,
                       false,
                       "CLOUD_HTTP_ERROR",
                       "云端响应异常");
        }
        return ESP_OK;
    }
    if (body == NULL || body_length == 0U)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_PARSE_ERROR,
                   false,
                   "CLOUD_RESPONSE_INVALID",
                   "云端响应无法解析");
        return ESP_ERR_INVALID_ARG;
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts((const char *)body,
                                            body_length,
                                            &parse_end,
                                            0);
    const char *body_end = (const char *)body + body_length;
    if (root == NULL || !cJSON_IsObject(root) || parse_end == NULL ||
        !consume_json_tail(parse_end, body_end))
    {
        cJSON_Delete(root);
        set_result(result,
                   WATCH_RENTAL_RESULT_PARSE_ERROR,
                   false,
                   "CLOUD_RESPONSE_INVALID",
                   "云端响应无法解析");
        return ESP_ERR_INVALID_ARG;
    }
    const cJSON *can_unlock =
        cJSON_GetObjectItemCaseSensitive(root, "can_unlock");
    const cJSON *reason = cJSON_GetObjectItemCaseSensitive(root, "reason");
    const cJSON *server_time =
        cJSON_GetObjectItemCaseSensitive(root, "server_time_ms");
    int64_t server_time_ms = 0;
    if (json_field_count(root, "can_unlock") != 1U ||
        json_field_count(root, "reason") != 1U ||
        json_field_count(root, "server_time_ms") != 1U ||
        !cJSON_IsBool(can_unlock) || !cJSON_IsString(reason) ||
        reason->valuestring == NULL || !parse_server_time(server_time, &server_time_ms))
    {
        cJSON_Delete(root);
        set_result(result,
                   WATCH_RENTAL_RESULT_PARSE_ERROR,
                   false,
                   "CLOUD_RESPONSE_INVALID",
                   "云端响应字段无效");
        return ESP_ERR_INVALID_ARG;
    }
    const bool allowed = cJSON_IsTrue(can_unlock);
    const char *reason_text = reason->valuestring;
    esp_err_t parse_result = ESP_OK;
    if (allowed && strcmp(reason_text, "paid") == 0)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_PAID,
                   true,
                   "CLOUD_OK",
                   "租赁授权通过");
    }
    else if (!allowed && strcmp(reason_text, "unpaid") == 0)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_UNPAID,
                   false,
                   "CLOUD_RENTAL_UNPAID",
                   "租赁未支付");
    }
    else if (!allowed && strcmp(reason_text, "disabled") == 0)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_DISABLED,
                   false,
                   "CLOUD_DEVICE_DISABLED",
                   "设备禁用");
    }
    else if (!allowed && strcmp(reason_text, "unknown_device") == 0)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_UNKNOWN_DEVICE,
                   false,
                   "CLOUD_UNKNOWN_DEVICE",
                   "云端未登记设备");
    }
    else if (!allowed && strcmp(reason_text, "request_invalid") == 0)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_REQUEST_INVALID,
                   false,
                   "CLOUD_REQUEST_INVALID",
                   "授权请求无效");
    }
    else if (!allowed && strcmp(reason_text, "server_error") == 0)
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_SERVER_ERROR,
                   false,
                   "CLOUD_SERVER_ERROR",
                   "云端服务异常");
    }
    else
    {
        set_result(result,
                   WATCH_RENTAL_RESULT_PARSE_ERROR,
                   false,
                   "CLOUD_RESPONSE_INVALID",
                   "云端响应合同矛盾");
        parse_result = ESP_ERR_INVALID_ARG;
    }
    result->server_time_ms = server_time_ms;
    cJSON_Delete(root);
    return parse_result;
}

esp_err_t cloud_service_parse_telemetry_http(
    uint16_t http_status,
    cloud_transport_result_t transport,
    const uint8_t *body,
    size_t body_length,
    cloud_telemetry_response_t *result)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(result, 0, sizeof(*result));
    if (transport < CLOUD_TRANSPORT_OK ||
        transport > CLOUD_TRANSPORT_INTERNAL ||
        (body == NULL && body_length != 0U))
    {
        return ESP_ERR_INVALID_ARG;
    }
    result->http_status = http_status;

    if (transport != CLOUD_TRANSPORT_OK)
    {
        const char *error_code = "CLOUD_INTERNAL";
        if (transport == CLOUD_TRANSPORT_UNREACHABLE)
        {
            error_code = "CLOUD_UNREACHABLE";
        }
        else if (transport == CLOUD_TRANSPORT_TLS)
        {
            error_code = "CLOUD_TLS_FAILED";
        }
        else if (transport == CLOUD_TRANSPORT_TIMEOUT)
        {
            error_code = "CLOUD_TRANSPORT_TIMEOUT";
        }
        else if (transport == CLOUD_TRANSPORT_CANCELLED)
        {
            error_code = "CLOUD_CANCELLED";
        }
        set_telemetry_result(result,
                             CLOUD_TELEMETRY_RESULT_TRANSPORT_ERROR,
                             false,
                             0,
                             error_code);
        return ESP_OK;
    }

    const bool http_success = http_status >= 200U && http_status < 300U;
    if (!http_success)
    {
        set_telemetry_result(result,
                             CLOUD_TELEMETRY_RESULT_HTTP_ERROR,
                             false,
                             0,
                             telemetry_http_error_code(http_status));
    }
    if (body == NULL || body_length == 0U ||
        body_length > CLOUD_TELEMETRY_RESPONSE_BODY_CAPACITY)
    {
        if (http_success)
        {
            set_telemetry_result(result,
                                 CLOUD_TELEMETRY_RESULT_RESPONSE_INVALID,
                                 false,
                                 0,
                                 "CLOUD_RESPONSE_INVALID");
        }
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *root = NULL;
    if (!parse_json_object(body, body_length, &root))
    {
        if (http_success)
        {
            set_telemetry_result(result,
                                 CLOUD_TELEMETRY_RESULT_RESPONSE_INVALID,
                                 false,
                                 0,
                                 "CLOUD_RESPONSE_INVALID");
        }
        return ESP_ERR_INVALID_ARG;
    }

    const cJSON *server_time =
        cJSON_GetObjectItemCaseSensitive(root, "server_time_ms");
    int64_t server_time_ms = 0;
    bool valid = json_field_count(root, "server_time_ms") == 1U &&
                 parse_server_time(server_time, &server_time_ms);
    if (http_success)
    {
        const cJSON *accepted =
            cJSON_GetObjectItemCaseSensitive(root, "accepted");
        const cJSON *message =
            cJSON_GetObjectItemCaseSensitive(root, "message");
        valid = valid && json_field_count(root, "accepted") == 1U &&
                json_field_count(root, "message") == 1U &&
                cJSON_IsBool(accepted) && cJSON_IsTrue(accepted) &&
                remote_string_is_bounded(message,
                                         CLOUD_REMOTE_MESSAGE_MAX_LENGTH) &&
                strcmp(message->valuestring, "ok") == 0;
        if (valid)
        {
            set_telemetry_result(result,
                                 CLOUD_TELEMETRY_RESULT_ACCEPTED,
                                 true,
                                 server_time_ms,
                                 "CLOUD_OK");
        }
        else
        {
            set_telemetry_result(result,
                                 CLOUD_TELEMETRY_RESULT_RESPONSE_INVALID,
                                 false,
                                 0,
                                 "CLOUD_RESPONSE_INVALID");
        }
    }
    else
    {
        const cJSON *code = cJSON_GetObjectItemCaseSensitive(root, "code");
        const cJSON *message =
            cJSON_GetObjectItemCaseSensitive(root, "message");
        valid = valid && json_field_count(root, "code") == 1U &&
                json_field_count(root, "message") == 1U &&
                remote_string_is_bounded(code,
                                         CLOUD_REMOTE_ERROR_CODE_MAX_LENGTH) &&
                remote_string_is_bounded(message,
                                         CLOUD_REMOTE_MESSAGE_MAX_LENGTH);
    }
    cJSON_Delete(root);
    return valid ? ESP_OK : ESP_ERR_INVALID_ARG;
}

void cloud_service_reset_boot_time(void)
{
    s_boot_time_synchronized = false;
    s_boot_cloud_offset_ms = 0;
}

esp_err_t cloud_service_note_server_time(int64_t server_time_ms,
                                         uint64_t monotonic_ms,
                                         int64_t *offset_ms)
{
    if (offset_ms == NULL || server_time_ms < 0 ||
        server_time_ms > CLOUD_SERVER_TIME_MAX_MS ||
        monotonic_ms > (uint64_t)INT64_MAX)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const int64_t offset = server_time_ms - (int64_t)monotonic_ms;
    const esp_err_t time_error =
        time_service_submit_sample(TIME_SOURCE_CLOUD,
                                   server_time_ms,
                                   monotonic_ms);
    if (time_error != ESP_OK)
    {
        return time_error;
    }
    s_boot_cloud_offset_ms = offset;
    s_boot_time_synchronized = true;
    *offset_ms = offset;
    return ESP_OK;
}

int64_t cloud_service_timestamp_ms(uint64_t monotonic_ms)
{
    int64_t timestamp = 0;
    return time_service_now(monotonic_ms, &timestamp, NULL) == ESP_OK
               ? timestamp
               : 0;
}

#ifndef CLOUD_SERVICE_CODEC_ONLY
esp_err_t cloud_service_prepare(void)
{
    if (atomic_load(&s_owner_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const bool any_queue_prepared = s_request_queue != NULL ||
                                    s_result_queue != NULL ||
                                    s_wake_queue != NULL;
    const bool all_queues_prepared = s_request_queue != NULL &&
                                     s_result_queue != NULL &&
                                     s_wake_queue != NULL;
    if (any_queue_prepared && !all_queues_prepared)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_request_queue == NULL)
    {
        s_request_queue = xQueueCreate(CLOUD_RENTAL_REQUEST_QUEUE_DEPTH,
                                       sizeof(cloud_rental_request_t));
        if (s_request_queue == NULL)
        {
            return ESP_ERR_NO_MEM;
        }
        s_result_queue = xQueueCreate(CLOUD_RENTAL_RESULT_QUEUE_DEPTH,
                                      sizeof(cloud_rental_result_t));
        if (s_result_queue == NULL)
        {
            vQueueDelete(s_request_queue);
            s_request_queue = NULL;
            return ESP_ERR_NO_MEM;
        }
        s_wake_queue = xQueueCreate(1U, sizeof(uint8_t));
        if (s_wake_queue == NULL)
        {
            vQueueDelete(s_result_queue);
            s_result_queue = NULL;
            vQueueDelete(s_request_queue);
            s_request_queue = NULL;
            return ESP_ERR_NO_MEM;
        }
    }
    else
    {
        (void)xQueueReset(s_request_queue);
        (void)xQueueReset(s_result_queue);
        (void)xQueueReset(s_wake_queue);
    }
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_owner_running, false);
    atomic_store(&s_rental_cancel_request_id, 0U);
    atomic_store(&s_authorized_link_end_generation, 0U);
    atomic_store(&s_rental_accelerate_request_id, 0U);
    atomic_store(&s_rental_accelerate_eligible_request_id, 0U);
    s_pending_result_valid = false;
    secure_zero(&s_pending_result, sizeof(s_pending_result));
    reset_owner_state();
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    secure_zero(&s_transport_response_scratch,
                sizeof(s_transport_response_scratch));
    memset(&s_prepare_product_state, 0, sizeof(s_prepare_product_state));
    const esp_err_t state_error = owner_watch_state_snapshot(
        &s_prepare_product_state,
        pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS));
    if (state_error != ESP_OK)
    {
        vQueueDelete(s_wake_queue);
        s_wake_queue = NULL;
        vQueueDelete(s_result_queue);
        s_result_queue = NULL;
        vQueueDelete(s_request_queue);
        s_request_queue = NULL;
        memset(&s_prepare_product_state, 0, sizeof(s_prepare_product_state));
        return state_error;
    }
    s_cloud_update_sequence = s_prepare_product_state.cloud_update_sequence;
    memset(&s_pending_cloud_update, 0, sizeof(s_pending_cloud_update));
    s_pending_cloud_update_dirty = false;
    s_telemetry_update_sequence =
        s_prepare_product_state.telemetry_update_sequence;
    memset(&s_prepare_product_state, 0, sizeof(s_prepare_product_state));
    memset(&s_pending_telemetry_update, 0, sizeof(s_pending_telemetry_update));
    s_pending_telemetry_update_dirty = false;
    cloud_service_reset_boot_time();
    /* 旧 offset 只验证 typed NVS 可读性，不建立当前 boot 同步证明。 */
    int64_t persisted_offset = 0;
    (void)config_service_cloud_offset_read(&persisted_offset,
                                           pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS));
    return ESP_OK;
}

void cloud_service_cancel_prepared_run(void)
{
    if (atomic_load(&s_owner_running))
    {
        return;
    }
    if (s_request_queue != NULL)
    {
        vQueueDelete(s_request_queue);
        s_request_queue = NULL;
    }
    if (s_result_queue != NULL)
    {
        vQueueDelete(s_result_queue);
        s_result_queue = NULL;
    }
    if (s_wake_queue != NULL)
    {
        vQueueDelete(s_wake_queue);
        s_wake_queue = NULL;
    }
    s_pending_result_valid = false;
    atomic_store(&s_rental_cancel_request_id, 0U);
    atomic_store(&s_authorized_link_end_generation, 0U);
    atomic_store(&s_rental_accelerate_request_id, 0U);
    atomic_store(&s_rental_accelerate_eligible_request_id, 0U);
    secure_zero(&s_pending_result, sizeof(s_pending_result));
    reset_owner_state();
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    secure_zero(&s_transport_response_scratch,
                sizeof(s_transport_response_scratch));
    memset(&s_pending_cloud_update, 0, sizeof(s_pending_cloud_update));
    s_pending_cloud_update_dirty = false;
    memset(&s_pending_telemetry_update, 0, sizeof(s_pending_telemetry_update));
    s_pending_telemetry_update_dirty = false;
    cloud_service_reset_boot_time();
}

esp_err_t cloud_service_request_stop(TickType_t timeout_ticks)
{
    if (s_request_queue == NULL || s_result_queue == NULL ||
        s_wake_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    atomic_store(&s_stop_requested, true);
    wake_owner();
    return ESP_OK;
}

esp_err_t cloud_service_rental_submit(const cloud_rental_request_t *request,
                                      TickType_t timeout_ticks)
{
    if (request == NULL || request->request_id == 0U ||
        request->link_generation == 0U ||
        !canonical_mac_is_valid(request->exoskeleton_mac,
                                sizeof(request->exoskeleton_mac)))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_request_queue == NULL || atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_request_queue, request, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    wake_owner();
    return ESP_OK;
}

esp_err_t cloud_service_rental_cancel(uint32_t request_id)
{
    if (request_id == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_request_queue == NULL || atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_rental_cancel_request_id, request_id);
    wake_owner();
    return ESP_OK;
}

esp_err_t cloud_service_authorized_link_end(uint32_t link_generation)
{
    if (link_generation == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_request_queue == NULL || atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_authorized_link_end_generation, link_generation);
    wake_owner();
    return ESP_OK;
}

esp_err_t cloud_service_rental_accelerate(uint32_t request_id,
                                          bool *joined_in_flight)
{
    if (request_id == 0U || joined_in_flight == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *joined_in_flight = false;
    if (s_request_queue == NULL || atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    unsigned int expected_request_id = request_id;
    if (!atomic_compare_exchange_strong(
            &s_rental_accelerate_eligible_request_id,
            &expected_request_id,
            0U))
    {
        if (atomic_load(&s_rental_in_flight_request_id) == request_id)
        {
            *joined_in_flight = true;
            return ESP_OK;
        }
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_rental_accelerate_request_id, request_id);
    wake_owner();
    return ESP_OK;
}

esp_err_t cloud_service_rental_receive(cloud_rental_result_t *result,
                                       TickType_t timeout_ticks)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueReceive(s_result_queue, result, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t cloud_service_run(void)
{
    const esp_err_t begin_error = owner_begin();
    if (begin_error != ESP_OK)
    {
        return begin_error;
    }
    while (!atomic_load(&s_stop_requested))
    {
        const uint64_t monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000);
        (void)owner_step(monotonic_ms);
        if (atomic_load(&s_stop_requested))
        {
            break;
        }
        uint8_t wake = 0U;
        (void)xQueueReceive(s_wake_queue,
                            &wake,
                            owner_next_wait_ticks(monotonic_ms));
    }
    owner_end();
    return ESP_OK;
}

#ifdef CLOUD_SERVICE_TEST
void cloud_service_test_set_watch_state(
    const watch_state_snapshot_t *snapshot,
    esp_err_t result)
{
    if (snapshot != NULL)
    {
        s_test_watch_state = *snapshot;
    }
    else
    {
        secure_zero(&s_test_watch_state, sizeof(s_test_watch_state));
    }
    s_test_watch_state_result = result;
    s_test_dependencies_enabled = true;
}

void cloud_service_test_set_config(
    const config_service_snapshot_t *snapshot,
    esp_err_t result)
{
    if (snapshot != NULL)
    {
        s_test_config = *snapshot;
    }
    else
    {
        secure_zero(&s_test_config, sizeof(s_test_config));
    }
    s_test_config_result = result;
    s_test_dependencies_enabled = true;
}

void cloud_service_test_set_modem_submit_result(esp_err_t result)
{
    s_test_modem_submit_result = result;
    s_test_dependencies_enabled = true;
}

void cloud_service_test_set_modem_connect_result(esp_err_t result)
{
    s_test_modem_connect_result = result;
    s_test_dependencies_enabled = true;
}

void cloud_service_test_set_modem_terminal(
    const ml307r_https_response_t *response,
    esp_err_t result)
{
    if (response != NULL)
    {
        s_test_modem_response = *response;
    }
    else
    {
        secure_zero(&s_test_modem_response,
                    sizeof(s_test_modem_response));
    }
    s_test_modem_receive_result = result;
    s_test_dependencies_enabled = true;
}

esp_err_t cloud_service_test_owner_begin(void)
{
    return owner_begin();
}

esp_err_t cloud_service_test_owner_step(uint64_t monotonic_ms)
{
    if (!atomic_load(&s_owner_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    return owner_step(monotonic_ms);
}

void cloud_service_test_owner_end(void)
{
    if (atomic_load(&s_owner_running))
    {
        owner_end();
    }
    s_test_dependencies_enabled = false;
    secure_zero(&s_test_watch_state, sizeof(s_test_watch_state));
    s_test_watch_state_result = ESP_ERR_INVALID_STATE;
    secure_zero(&s_test_config, sizeof(s_test_config));
    s_test_config_result = ESP_ERR_INVALID_STATE;
    s_test_modem_submit_result = ESP_ERR_INVALID_STATE;
    s_test_modem_connect_result = ESP_ERR_INVALID_STATE;
}

esp_err_t cloud_service_test_latest_telemetry(
    cloud_telemetry_snapshot_t *snapshot,
    bool *valid,
    bool *pending_send)
{
    if (snapshot == NULL || valid == NULL || pending_send == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *snapshot = s_owner.latest;
    *valid = s_owner.latest_valid;
    *pending_send = s_owner.latest_pending_send;
    return ESP_OK;
}

uint64_t cloud_service_test_next_window_ms(void)
{
    return s_owner.next_window_ms;
}

uint32_t cloud_service_test_modem_submit_count(void)
{
    return s_test_modem_submit_count;
}

uint32_t cloud_service_test_modem_connect_count(void)
{
    return s_test_modem_connect_count;
}

uint32_t cloud_service_test_modem_release_count(void)
{
    return s_test_modem_release_count;
}

bool cloud_service_test_telemetry_lease_held(void)
{
    return s_owner.telemetry_lease_held;
}

bool cloud_service_test_rental_lease_held(void)
{
    return s_owner.rental_lease_held;
}

uint32_t cloud_service_test_active_transport_request_id(void)
{
    return s_owner.active_request_id;
}

esp_err_t cloud_service_test_acquire_telemetry_lease(void)
{
    if (s_owner.telemetry_lease_held)
    {
        return ESP_OK;
    }
    const esp_err_t error = owner_modem_request_telemetry_connect();
    if (error == ESP_OK)
    {
        note_telemetry_lease_acquired(0U);
    }
    return error;
}

esp_err_t cloud_service_test_fill_result_queue(void)
{
    if (s_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const cloud_rental_result_t occupied = {
        .request_id = UINT32_MAX,
    };
    for (uint32_t index = 0U;
         index < CLOUD_RENTAL_RESULT_QUEUE_DEPTH;
         ++index)
    {
        if (xQueueSend(s_result_queue, &occupied, 0U) != pdTRUE)
        {
            return ESP_ERR_INVALID_STATE;
        }
    }
    return ESP_OK;
}

esp_err_t cloud_service_test_set_rental_active(bool active)
{
    if (active)
    {
        if (s_owner.active_kind != CLOUD_ACTIVE_NONE)
        {
            return ESP_ERR_INVALID_STATE;
        }
        s_owner.active_kind = CLOUD_ACTIVE_RENTAL;
        return ESP_OK;
    }
    if (s_owner.active_kind != CLOUD_ACTIVE_RENTAL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    finish_active_operation();
    return ESP_OK;
}

bool cloud_service_test_active_telemetry_identity_valid(void)
{
    return s_owner.active_kind == CLOUD_ACTIVE_TELEMETRY &&
           s_owner.active_telemetry_identity_valid;
}

esp_err_t cloud_service_test_last_telemetry_terminal(
    ml307r_https_response_t *response)
{
    if (response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_test_last_telemetry_terminal_valid)
    {
        return ESP_ERR_NOT_FOUND;
    }
    *response = s_test_last_telemetry_terminal;
    return ESP_OK;
}
#endif
#endif

static bool canonical_mac_is_valid(const char *mac, size_t capacity)
{
    if (mac == NULL || capacity != 18U || strnlen(mac, capacity) != capacity - 1U)
    {
        return false;
    }
    for (size_t index = 0U; index < capacity - 1U; ++index)
    {
        const bool separator = index == 2U || index == 5U || index == 8U ||
                               index == 11U || index == 14U;
        if ((separator && mac[index] != ':') ||
            (!separator &&
             !((mac[index] >= '0' && mac[index] <= '9') ||
               (mac[index] >= 'A' && mac[index] <= 'F'))))
        {
            return false;
        }
    }
    return true;
}

static size_t json_field_count(const cJSON *object, const char *name)
{
    if (!cJSON_IsObject(object) || name == NULL)
    {
        return 0U;
    }
    size_t count = 0U;
    for (const cJSON *item = object->child; item != NULL; item = item->next)
    {
        if (item->string != NULL && strcmp(item->string, name) == 0)
        {
            ++count;
        }
    }
    return count;
}

static bool parse_server_time(const cJSON *item, int64_t *server_time_ms)
{
    if (!cJSON_IsNumber(item) || server_time_ms == NULL ||
        !isfinite(item->valuedouble) || item->valuedouble < 0.0 ||
        item->valuedouble > (double)CLOUD_SERVER_TIME_MAX_MS ||
        floor(item->valuedouble) != item->valuedouble)
    {
        return false;
    }
    *server_time_ms = (int64_t)item->valuedouble;
    return true;
}

static void set_telemetry_result(cloud_telemetry_response_t *result,
                                 cloud_telemetry_result_t telemetry_result,
                                 bool accepted,
                                 int64_t server_time_ms,
                                 const char *error_code)
{
    result->result = telemetry_result;
    result->accepted = accepted;
    result->server_time_ms = server_time_ms;
    memset(result->error_code, 0, sizeof(result->error_code));
    size_t length = 0U;
    while (length < sizeof(result->error_code) &&
           error_code[length] != '\0')
    {
        ++length;
    }
    if (length < sizeof(result->error_code))
    {
        memcpy(result->error_code, error_code, length + 1U);
    }
}

static bool parse_json_object(const uint8_t *body,
                              size_t body_length,
                              cJSON **root)
{
    if (body == NULL || body_length == 0U || root == NULL ||
        json_body_contains_embedded_nul(body, body_length))
    {
        return false;
    }
    const char *parse_end = NULL;
    *root = cJSON_ParseWithLengthOpts((const char *)body,
                                      body_length,
                                      &parse_end,
                                      0);
    const char *body_end = (const char *)body + body_length;
    if (*root == NULL || !cJSON_IsObject(*root) || parse_end == NULL ||
        !consume_json_tail(parse_end, body_end))
    {
        cJSON_Delete(*root);
        *root = NULL;
        return false;
    }
    return true;
}

static bool json_body_contains_embedded_nul(const uint8_t *body,
                                            size_t body_length)
{
    if (body == NULL)
    {
        return false;
    }
    for (size_t index = 0U; index < body_length; ++index)
    {
        if (body[index] == 0U)
        {
            return true;
        }
        if (index + 5U >= body_length)
        {
            continue;
        }
        if (body[index] != '\\' || body[index + 1U] != 'u' ||
            body[index + 2U] != '0' || body[index + 3U] != '0' ||
            body[index + 4U] != '0' || body[index + 5U] != '0')
        {
            continue;
        }
        size_t slash_count = 1U;
        size_t cursor = index;
        while (cursor > 0U && body[cursor - 1U] == '\\')
        {
            ++slash_count;
            --cursor;
        }
        if ((slash_count & 1U) != 0U)
        {
            return true;
        }
    }
    return false;
}

static bool remote_string_is_bounded(const cJSON *item, size_t max_length)
{
    return cJSON_IsString(item) && item->valuestring != NULL &&
           strnlen(item->valuestring, max_length + 1U) <= max_length;
}

static const char *telemetry_http_error_code(uint16_t http_status)
{
    if (http_status == 400U)
    {
        return "CLOUD_REQUEST_INVALID";
    }
    if (http_status == 401U)
    {
        return "CLOUD_TOKEN_INVALID";
    }
    if (http_status == 404U)
    {
        return "CLOUD_ENDPOINT_NOT_FOUND";
    }
    if (http_status == 408U)
    {
        return "CLOUD_HTTP_TIMEOUT";
    }
    if (http_status == 429U)
    {
        return "CLOUD_RATE_LIMITED";
    }
    if (http_status >= 500U && http_status <= 599U)
    {
        return "CLOUD_HTTP_5XX";
    }
    return "CLOUD_HTTP_ERROR";
}

static void initialize_result(const cloud_rental_request_t *request,
                              uint16_t http_status,
                              cloud_rental_result_t *result)
{
    memset(result, 0, sizeof(*result));
    result->request_id = request->request_id;
    result->link_generation = request->link_generation;
    result->http_status = http_status;
    memcpy(result->exoskeleton_mac,
           request->exoskeleton_mac,
           sizeof(result->exoskeleton_mac));
}

static void set_result(cloud_rental_result_t *result,
                       watch_rental_result_t rental_result,
                       bool can_unlock,
                       const char *error_code,
                       const char *user_reason)
{
    result->result = rental_result;
    result->can_unlock = can_unlock;
    memset(result->error_code, 0, sizeof(result->error_code));
    memset(result->user_reason, 0, sizeof(result->user_reason));
    size_t error_length = 0U;
    while (error_length < sizeof(result->error_code) &&
           error_code[error_length] != '\0')
    {
        ++error_length;
    }
    size_t reason_length = 0U;
    while (reason_length < sizeof(result->user_reason) &&
           user_reason[reason_length] != '\0')
    {
        ++reason_length;
    }
    if (error_length < sizeof(result->error_code))
    {
        memcpy(result->error_code, error_code, error_length + 1U);
    }
    if (reason_length < sizeof(result->user_reason))
    {
        memcpy(result->user_reason, user_reason, reason_length + 1U);
    }
}

static bool consume_json_tail(const char *cursor, const char *end)
{
    if (cursor == NULL || end == NULL || cursor > end)
    {
        return false;
    }
    while (cursor < end && isspace((unsigned char)*cursor))
    {
        ++cursor;
    }
    return cursor == end;
}

#ifndef CLOUD_SERVICE_CODEC_ONLY
static cloud_transport_result_t map_transport_result(ml307r_https_error_t error)
{
    if (error == ML307R_HTTPS_ERROR_NONE || error == ML307R_HTTPS_ERROR_HTTP)
    {
        return CLOUD_TRANSPORT_OK;
    }
    if (error == ML307R_HTTPS_ERROR_TLS)
    {
        return CLOUD_TRANSPORT_TLS;
    }
    if (error == ML307R_HTTPS_ERROR_CONNECT_TIMEOUT ||
        error == ML307R_HTTPS_ERROR_ATTEMPT_TIMEOUT)
    {
        return CLOUD_TRANSPORT_TIMEOUT;
    }
    if (error == ML307R_HTTPS_ERROR_CANCELLED ||
        error == ML307R_HTTPS_ERROR_MODEM_RESTARTED)
    {
        return CLOUD_TRANSPORT_CANCELLED;
    }
    if (error == ML307R_HTTPS_ERROR_QUEUE ||
        error == ML307R_HTTPS_ERROR_AT_UNRESPONSIVE)
    {
        return CLOUD_TRANSPORT_UNREACHABLE;
    }
    return CLOUD_TRANSPORT_INTERNAL;
}

static watch_telemetry_result_t map_telemetry_result(
    cloud_telemetry_result_t result)
{
    switch (result)
    {
    case CLOUD_TELEMETRY_RESULT_ACCEPTED:
        return WATCH_TELEMETRY_RESULT_ACCEPTED;
    case CLOUD_TELEMETRY_RESULT_HTTP_ERROR:
        return WATCH_TELEMETRY_RESULT_HTTP_ERROR;
    case CLOUD_TELEMETRY_RESULT_TRANSPORT_ERROR:
        return WATCH_TELEMETRY_RESULT_TRANSPORT_ERROR;
    case CLOUD_TELEMETRY_RESULT_RESPONSE_INVALID:
        return WATCH_TELEMETRY_RESULT_RESPONSE_INVALID;
    default:
        return WATCH_TELEMETRY_RESULT_RESPONSE_INVALID;
    }
}

static void clear_latest_telemetry(void)
{
    secure_zero(&s_owner.latest, sizeof(s_owner.latest));
    s_owner.latest_valid = false;
    s_owner.latest_pending_send = false;
}

static void reset_owner_state(void)
{
    secure_zero(&s_owner, sizeof(s_owner));
    atomic_store(&s_authorized_link_end_generation, 0U);
    atomic_store(&s_rental_accelerate_eligible_request_id, 0U);
    atomic_store(&s_rental_in_flight_request_id, 0U);
#ifdef CLOUD_SERVICE_TEST
    secure_zero(&s_test_last_telemetry_terminal,
                sizeof(s_test_last_telemetry_terminal));
    s_test_last_telemetry_terminal_valid = false;
    s_test_modem_submit_count = 0U;
    s_test_modem_connect_count = 0U;
    s_test_modem_release_count = 0U;
    s_test_modem_receive_result = ESP_ERR_TIMEOUT;
    secure_zero(&s_test_modem_response,
                sizeof(s_test_modem_response));
#endif
}

static void observe_binding(const watch_state_snapshot_t *state,
                            uint64_t monotonic_ms)
{
    if (s_owner.authorized_online_hold &&
        !authorized_online_hold_matches_state(state))
    {
        release_authorized_online_hold("授权链路、绑定或云配置已变化");
    }
    if (s_owner.rental_session_valid &&
        !rental_state_matches_request(state, &s_owner.rental_session))
    {
        cancel_rental_session("绑定、链路代次或控制准入已变化");
    }
    if (s_owner.active_kind == CLOUD_ACTIVE_RENTAL_WAIT_MODEM)
    {
        s_owner.active_rental_context_valid =
            rental_state_matches_request(state, &s_owner.active_rental);
    }
    const bool binding_valid =
        state != NULL && state->has_bound_exoskeleton &&
        canonical_mac_is_valid(state->bound_exoskeleton_mac,
                               sizeof(state->bound_exoskeleton_mac));
    if (!binding_valid)
    {
        clear_latest_telemetry();
        cancel_active_telemetry("绑定已清除");
        release_telemetry_lease();
        s_owner.binding_tracked = false;
        s_owner.cloud_configured = false;
        s_owner.modem_ready = false;
        s_owner.next_window_ms = 0U;
        memset(s_owner.bound_mac, 0, sizeof(s_owner.bound_mac));
        return;
    }

    const bool binding_changed =
        !s_owner.binding_tracked ||
        memcmp(s_owner.bound_mac,
               state->bound_exoskeleton_mac,
               sizeof(s_owner.bound_mac)) != 0;
    if (binding_changed)
    {
        clear_latest_telemetry();
        cancel_active_telemetry("绑定身份已变化");
        release_telemetry_lease();
        s_owner.binding_tracked = true;
        memcpy(s_owner.bound_mac,
               state->bound_exoskeleton_mac,
               sizeof(s_owner.bound_mac));
        s_owner.next_window_ms =
            monotonic_ms > UINT64_MAX - CLOUD_TELEMETRY_INTERVAL_MS
                ? UINT64_MAX
                : monotonic_ms + CLOUD_TELEMETRY_INTERVAL_MS;
        ESP_LOGI(TAG,
                 "检测到新绑定，首条 telemetry 将在三分钟云窗口采集");
    }
    if (s_owner.active_kind == CLOUD_ACTIVE_TELEMETRY &&
        state->ble_link_generation !=
            s_owner.active_telemetry_binding_generation)
    {
        cancel_active_telemetry("BLE 链路代次已变化");
    }
    s_owner.cloud_configured = state->cloud_configured;
    s_owner.modem_ready = state->modem_ready;
    if (!s_owner.cloud_configured)
    {
        clear_latest_telemetry();
        cancel_active_telemetry("云配置已失效");
        release_telemetry_lease();
    }
}

static void advance_telemetry_window(uint64_t monotonic_ms)
{
    if (!s_owner.binding_tracked || monotonic_ms < s_owner.next_window_ms)
    {
        return;
    }
    const uint64_t elapsed = monotonic_ms - s_owner.next_window_ms;
    const uint64_t windows = elapsed / CLOUD_TELEMETRY_INTERVAL_MS + 1U;
    const uint64_t remaining = UINT64_MAX - s_owner.next_window_ms;
    if (windows > remaining / CLOUD_TELEMETRY_INTERVAL_MS)
    {
        s_owner.next_window_ms = UINT64_MAX;
    }
    else
    {
        s_owner.next_window_ms += windows * CLOUD_TELEMETRY_INTERVAL_MS;
    }
}

static void capture_due_telemetry(uint64_t monotonic_ms)
{
    if (!s_owner.binding_tracked || monotonic_ms < s_owner.next_window_ms)
    {
        return;
    }
    advance_telemetry_window(monotonic_ms);
    if (!s_owner.cloud_configured)
    {
        clear_latest_telemetry();
        release_telemetry_lease();
        return;
    }

    cloud_telemetry_snapshot_t captured = {0};
    if (cloud_service_capture_telemetry(monotonic_ms, &captured) != ESP_OK ||
        memcmp(captured.exoskeleton_mac,
               s_owner.bound_mac,
               sizeof(captured.exoskeleton_mac)) != 0)
    {
        clear_latest_telemetry();
        if (s_owner.active_kind == CLOUD_ACTIVE_TELEMETRY)
        {
            cancel_active_telemetry("telemetry 采集失败");
        }
        else
        {
            release_telemetry_lease();
        }
        secure_zero(&captured, sizeof(captured));
        return;
    }
    s_owner.latest = captured;
    s_owner.latest_valid = true;
    s_owner.latest_pending_send = true;
    secure_zero(&captured, sizeof(captured));
}

static uint32_t next_transport_request_id(void)
{
    do
    {
        ++s_next_transport_request_id;
    } while (s_next_transport_request_id == 0U);
    return s_next_transport_request_id;
}

static const char *telemetry_submit_error_code(esp_err_t error)
{
    return error == ESP_ERR_TIMEOUT ? "CLOUD_TRANSPORT_TIMEOUT"
                                    : "CLOUD_INTERNAL";
}

static bool start_telemetry_request(uint64_t monotonic_ms)
{
    if (!s_owner.latest_valid || !s_owner.binding_tracked ||
        memcmp(s_owner.latest.exoskeleton_mac,
               s_owner.bound_mac,
               sizeof(s_owner.bound_mac)) != 0)
    {
        clear_latest_telemetry();
        release_telemetry_lease();
        return false;
    }
    if (!s_owner.latest_pending_send || s_pending_telemetry_update_dirty)
    {
        return false;
    }
    config_service_snapshot_t config = {0};
    watch_state_snapshot_t state = {0};
    if (owner_config_snapshot(&config,
                              pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) != ESP_OK ||
        owner_watch_state_snapshot(
            &state,
            pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) != ESP_OK)
    {
        defer_telemetry_to_next_window("telemetry 上下文采集失败");
        return false;
    }
    if (!config.cloud_configured || !state.cloud_configured ||
        !state.has_bound_exoskeleton || !state.ble_connected ||
        !state.device_state_available || !state.dataReady ||
        !state.status_fresh ||
        !canonical_mac_is_valid(config.watch_id, sizeof(config.watch_id)) ||
        !canonical_mac_is_valid(state.watch_id, sizeof(state.watch_id)) ||
        memcmp(config.watch_id,
               s_owner.latest.watch_id,
               sizeof(config.watch_id)) != 0 ||
        memcmp(state.watch_id,
               s_owner.latest.watch_id,
               sizeof(state.watch_id)) != 0 ||
        memcmp(state.bound_exoskeleton_mac,
               s_owner.bound_mac,
               sizeof(s_owner.bound_mac)) != 0 ||
        state.ble_link_generation != s_owner.latest.binding_generation)
    {
        clear_latest_telemetry();
        release_telemetry_lease();
        return false;
    }
    if (state.gps.acquisition_state == WATCH_GPS_ACQUISITION_SEARCHING ||
        state.gps.acquisition_state == WATCH_GPS_ACQUISITION_TRACKING)
    {
        defer_telemetry_to_next_window("GPS 正在占用 RF");
        return false;
    }
    if (!s_owner.telemetry_lease_held)
    {
        if (owner_modem_request_telemetry_connect() != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "telemetry 无法取得 RF lease，等待下一云窗口");
            s_owner.latest_pending_send = false;
            release_telemetry_lease();
            return false;
        }
        note_telemetry_lease_acquired(monotonic_ms);
    }
    if (s_owner.telemetry_wait_deadline_ms != 0U &&
        monotonic_ms >= s_owner.telemetry_wait_deadline_ms)
    {
        defer_telemetry_to_next_window("4G 联网等待超过 65 秒");
        return false;
    }
    if (!state.modem_ready)
    {
        s_owner.modem_ready = false;
        return false;
    }
    s_owner.telemetry_wait_deadline_ms = 0U;

    cloud_telemetry_http_request_t http = {0};
    if (cloud_service_build_telemetry_http(&s_owner.latest,
                                           CLOUD_TELEMETRY_BODY_CAPACITY,
                                           &http) != ESP_OK)
    {
        clear_latest_telemetry();
        release_telemetry_lease();
        return false;
    }
    const uint32_t request_id = next_transport_request_id();
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    s_transport_request_scratch.request_id = request_id;
    s_transport_request_scratch.max_retries = CLOUD_REQUEST_RETRY_MAX;
    s_transport_request_scratch.deadline_ticks =
        xTaskGetTickCount() + pdMS_TO_TICKS(CLOUD_REQUEST_GROUP_TIMEOUT_MS);
    s_transport_request_scratch.body_length = http.body_length;
    memcpy(s_transport_request_scratch.path,
           http.path,
           strlen(http.path) + 1U);
    memcpy(s_transport_request_scratch.body, http.body, http.body_length);
    const esp_err_t submit_error = owner_modem_submit(
        &s_transport_request_scratch,
        pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS));
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    secure_zero(&http, sizeof(http));
    s_owner.latest_pending_send = false;
    if (submit_error != ESP_OK)
    {
        const char *error_code = telemetry_submit_error_code(submit_error);
        cloud_telemetry_response_t failure = {0};
        set_telemetry_result(&failure,
                             CLOUD_TELEMETRY_RESULT_TRANSPORT_ERROR,
                             false,
                             0,
                             error_code);
        publish_telemetry_diagnostic(&failure,
                                     request_id,
                                     monotonic_ms,
                                     false,
                                     0);
        ESP_LOGW(TAG,
                 "telemetry 请求未被 modem 接受，请求=%lu，稳定码=%s，等待下一窗口",
                 (unsigned long)request_id,
                 error_code);
        release_telemetry_lease();
        return false;
    }
    s_owner.active_kind = CLOUD_ACTIVE_TELEMETRY;
    s_owner.active_request_id = request_id;
    s_owner.active_telemetry_identity_valid = true;
    s_owner.active_telemetry_binding_generation =
        s_owner.latest.binding_generation;
    return true;
}

static bool rental_state_matches_request(
    const watch_state_snapshot_t *state,
    const cloud_rental_request_t *request)
{
    const bool phase_accepts_follow_up =
        state != NULL &&
        (state->rental_phase == WATCH_RENTAL_PHASE_QUERYING ||
         state->rental_phase == WATCH_RENTAL_PHASE_WAITING_PAYMENT ||
         state->rental_phase == WATCH_RENTAL_PHASE_RETRY_WAIT);
    return state != NULL && request != NULL &&
           state->cloud_configured &&
           state->has_bound_exoskeleton && state->ble_connected &&
           state->ble_link_generation != 0U &&
           state->ble_link_generation == request->link_generation &&
           state->pending_control &&
           phase_accepts_follow_up &&
           state->control_request_id == request->request_id &&
           state->control_link_generation == request->link_generation &&
           state->gatt_ready && state->notify_ready &&
           state->device_state_available &&
           state->dataReady && state->status_fresh &&
           state->link_control_ready &&
           !state->unlock_session_valid &&
           memcmp(state->bound_exoskeleton_mac,
                  request->exoskeleton_mac,
                  sizeof(state->bound_exoskeleton_mac)) == 0 &&
           memcmp(state->control_exoskeleton_mac,
                  request->exoskeleton_mac,
                  sizeof(state->control_exoskeleton_mac)) == 0;
}

static bool rental_requests_match(const cloud_rental_request_t *first,
                                  const cloud_rental_request_t *second)
{
    return first != NULL && second != NULL &&
           first->request_id != 0U &&
           first->request_id == second->request_id &&
           first->link_generation == second->link_generation &&
           memcmp(first->exoskeleton_mac,
                  second->exoskeleton_mac,
                  sizeof(first->exoskeleton_mac)) == 0;
}

static void release_rental_lease(void)
{
    if (!s_owner.rental_lease_held)
    {
        return;
    }
    owner_modem_release(MODEM_NETWORK_LEASE_PAYMENT);
    s_owner.rental_lease_held = false;
}

static void release_authorized_online_hold(const char *reason)
{
    if (!s_owner.authorized_online_hold)
    {
        return;
    }
    const uint32_t request_id = s_owner.authorized_session.request_id;
    const uint32_t link_generation =
        s_owner.authorized_session.link_generation;
    s_owner.authorized_online_hold = false;
    secure_zero(&s_owner.authorized_session,
                sizeof(s_owner.authorized_session));
    release_rental_lease();
    ESP_LOGI(TAG,
             "当前授权 BLE 链路已释放 4G 在线持有，请求=%lu，链路代次=%lu，原因=%s",
             (unsigned long)request_id,
             (unsigned long)link_generation,
             reason != NULL ? reason : "未知");
}

static bool authorized_online_hold_matches_state(
    const watch_state_snapshot_t *state)
{
    return state != NULL && s_owner.authorized_online_hold &&
           state->cloud_configured && state->has_bound_exoskeleton &&
           state->ble_connected && state->ble_link_generation != 0U &&
           state->ble_link_generation ==
               s_owner.authorized_session.link_generation &&
           memcmp(state->bound_exoskeleton_mac,
                  s_owner.authorized_session.exoskeleton_mac,
                  sizeof(state->bound_exoskeleton_mac)) == 0;
}

static void release_telemetry_lease(void)
{
    if (s_owner.telemetry_lease_held)
    {
        owner_modem_release(MODEM_NETWORK_LEASE_TELEMETRY);
    }
    s_owner.telemetry_lease_held = false;
    s_owner.telemetry_wait_deadline_ms = 0U;
}

static void note_telemetry_lease_acquired(uint64_t monotonic_ms)
{
    s_owner.telemetry_lease_held = true;
    s_owner.telemetry_wait_deadline_ms =
        monotonic_ms > UINT64_MAX - CLOUD_TELEMETRY_MODEM_WAIT_MS
            ? UINT64_MAX
            : monotonic_ms + CLOUD_TELEMETRY_MODEM_WAIT_MS;
}

static void defer_telemetry_to_next_window(const char *reason)
{
    if (s_owner.latest_valid)
    {
        s_owner.latest_pending_send = false;
    }
    release_telemetry_lease();
    ESP_LOGI(TAG,
             "telemetry 已保留 latest-only 并延后到下一云窗口，原因=%s",
             reason != NULL ? reason : "未知");
}

static void release_network_leases_for_result_backpressure(void)
{
    release_telemetry_lease();
    if (s_owner.authorized_online_hold)
    {
        ESP_LOGI(TAG,
                 "支付成功结果暂未交付，当前授权 BLE 链路继续保持 4G 在线");
    }
    else
    {
        release_rental_lease();
        ESP_LOGI(TAG,
                 "支付结果暂未交付，已释放支付与 telemetry RF lease 并保留 latest-only");
    }
}

static void cancel_active_telemetry(const char *reason)
{
    if (s_owner.active_kind != CLOUD_ACTIVE_TELEMETRY)
    {
        return;
    }
    const uint32_t request_id = s_owner.active_request_id;
    s_owner.active_telemetry_identity_valid = false;
    if (request_id != 0U)
    {
        owner_modem_cancel(request_id);
    }
    finish_active_operation();
    release_telemetry_lease();
    ESP_LOGI(TAG,
             "已取消失效 telemetry 并释放 RF，请求=%lu，原因=%s",
             (unsigned long)request_id,
             reason != NULL ? reason : "未知");
}

static void cancel_rental_session(const char *reason)
{
    if (!s_owner.rental_session_valid &&
        s_owner.active_kind != CLOUD_ACTIVE_RENTAL &&
        s_owner.active_kind != CLOUD_ACTIVE_RENTAL_WAIT_MODEM)
    {
        return;
    }
    const uint32_t request_id = s_owner.rental_session_valid
                                    ? s_owner.rental_session.request_id
                                    : s_owner.active_rental.request_id;
    if (s_owner.active_kind == CLOUD_ACTIVE_RENTAL &&
        s_owner.active_request_id != 0U)
    {
        owner_modem_cancel(s_owner.active_request_id);
    }
    if (s_owner.active_kind == CLOUD_ACTIVE_RENTAL ||
        s_owner.active_kind == CLOUD_ACTIVE_RENTAL_WAIT_MODEM)
    {
        finish_active_operation();
    }
    release_rental_lease();
    atomic_store(&s_rental_accelerate_request_id, 0U);
    atomic_store(&s_rental_accelerate_eligible_request_id, 0U);
    atomic_store(&s_rental_in_flight_request_id, 0U);
    cloud_payment_schedule_reset(&s_owner.rental_schedule);
    secure_zero(&s_owner.rental_session, sizeof(s_owner.rental_session));
    s_owner.rental_session_valid = false;
    ESP_LOGI(TAG,
             "支付自动复查会话已取消，请求=%lu，原因=%s",
             (unsigned long)request_id,
             reason != NULL ? reason : "未知");
}

static void process_rental_control_signals(uint64_t monotonic_ms)
{
    const uint32_t ended_link_generation =
        atomic_exchange(&s_authorized_link_end_generation, 0U);
    if (ended_link_generation != 0U &&
        s_owner.authorized_online_hold &&
        s_owner.authorized_session.link_generation ==
            ended_link_generation)
    {
        release_authorized_online_hold("BLE 物理链路已结束");
    }

    const uint32_t cancel_request_id =
        atomic_exchange(&s_rental_cancel_request_id, 0U);
    if (cancel_request_id != 0U &&
        s_owner.authorized_online_hold &&
        s_owner.authorized_session.request_id == cancel_request_id)
    {
        release_authorized_online_hold("BLE 授权流程已取消");
    }
    if (cancel_request_id != 0U &&
        ((s_owner.rental_session_valid &&
          s_owner.rental_session.request_id == cancel_request_id) ||
         (s_owner.active_rental.request_id == cancel_request_id)))
    {
        cancel_rental_session("BLE 链路或绑定事实变化");
    }

    const uint32_t accelerate_request_id =
        atomic_exchange(&s_rental_accelerate_request_id, 0U);
    if (accelerate_request_id != 0U &&
        s_owner.rental_session_valid &&
        s_owner.rental_session.request_id == accelerate_request_id)
    {
        const bool accepted = cloud_payment_schedule_accelerate(
            &s_owner.rental_schedule,
            monotonic_ms);
        ESP_LOGI(TAG,
                 "支付立即检查%s，请求=%lu",
                 accepted ? "已接受" : "已忽略",
                 (unsigned long)accelerate_request_id);
        refresh_rental_accelerate_eligibility();
    }
}

static void refresh_rental_accelerate_eligibility(void)
{
    uint32_t eligible_request_id = 0U;
    if (s_owner.rental_session_valid &&
        s_owner.rental_schedule.active &&
        !s_owner.rental_schedule.in_flight &&
        !s_owner.rental_schedule.manual_bypass_used &&
        (s_owner.rental_schedule.first_unpaid_valid ||
         s_owner.rental_schedule.modem_available_ms != 0U))
    {
        eligible_request_id = s_owner.rental_session.request_id;
    }
    atomic_store(&s_rental_accelerate_eligible_request_id,
                 eligible_request_id);
}

static cloud_payment_outcome_t classify_payment_outcome(
    const cloud_rental_result_t *result)
{
    if (result == NULL)
    {
        return CLOUD_PAYMENT_OUTCOME_TRANSIENT;
    }
    if (result->result == WATCH_RENTAL_RESULT_PAID && result->can_unlock)
    {
        return CLOUD_PAYMENT_OUTCOME_PAID;
    }
    if (result->result == WATCH_RENTAL_RESULT_UNPAID)
    {
        return CLOUD_PAYMENT_OUTCOME_UNPAID;
    }
    if (result->result == WATCH_RENTAL_RESULT_SERVER_ERROR ||
        result->result == WATCH_RENTAL_RESULT_TRANSPORT_ERROR ||
        result->result == WATCH_RENTAL_RESULT_TIMEOUT ||
        (result->result == WATCH_RENTAL_RESULT_HTTP_ERROR &&
         (result->http_status == 408U || result->http_status == 429U ||
          (result->http_status >= 500U && result->http_status <= 599U))))
    {
        return CLOUD_PAYMENT_OUTCOME_TRANSIENT;
    }
    return CLOUD_PAYMENT_OUTCOME_FATAL;
}

static void finish_payment_attempt(const cloud_rental_result_t *result,
                                   uint64_t monotonic_ms)
{
    if (!s_owner.rental_session_valid)
    {
        return;
    }
    atomic_store(&s_rental_in_flight_request_id, 0U);
    const cloud_payment_outcome_t outcome = classify_payment_outcome(result);
    cloud_payment_schedule_finish_attempt(&s_owner.rental_schedule,
                                          outcome,
                                          monotonic_ms,
                                          s_owner.next_window_ms);
    const cloud_payment_mode_t mode = cloud_payment_schedule_mode(
        &s_owner.rental_schedule,
        monotonic_ms);
    if (outcome == CLOUD_PAYMENT_OUTCOME_PAID)
    {
        if (s_owner.rental_lease_held)
        {
            s_owner.authorized_session = s_owner.rental_session;
            s_owner.authorized_online_hold = true;
            ESP_LOGI(TAG,
                     "支付成功，4G 将保持在线至当前 BLE 链路结束，请求=%lu，链路代次=%lu",
                     (unsigned long)s_owner.authorized_session.request_id,
                     (unsigned long)s_owner.authorized_session.link_generation);
        }
        else
        {
            ESP_LOGE(TAG,
                     "支付成功但支付 RF lease 已缺失，无法建立授权链路在线持有，请求=%lu",
                     (unsigned long)s_owner.rental_session.request_id);
        }
    }
    const bool release_payment_lease =
        outcome != CLOUD_PAYMENT_OUTCOME_PAID &&
        (outcome == CLOUD_PAYMENT_OUTCOME_TRANSIENT ||
         outcome == CLOUD_PAYMENT_OUTCOME_FATAL ||
         mode == CLOUD_PAYMENT_MODE_SLOW ||
         mode == CLOUD_PAYMENT_MODE_COMPLETE ||
         mode == CLOUD_PAYMENT_MODE_BLOCKED);
    const bool telemetry_can_share_payment_window =
        s_owner.latest_pending_send &&
        !s_pending_telemetry_update_dirty &&
        !s_owner.telemetry_lease_held &&
        s_owner.rental_lease_held &&
        s_owner.modem_ready;
    if (telemetry_can_share_payment_window)
    {
        const esp_err_t bridge_error =
            owner_modem_request_telemetry_connect();
        if (bridge_error == ESP_OK)
        {
            note_telemetry_lease_acquired(monotonic_ms);
            ESP_LOGI(TAG,
                     "支付与 telemetry 已在同一 RF 窗口完成 lease 交接");
        }
        else
        {
            ESP_LOGW(TAG,
                     "支付窗口无法预接 telemetry RF lease，错误=0x%x",
                     (unsigned)bridge_error);
            defer_telemetry_to_next_window("RF lease 交接失败");
        }
    }
    else if (release_payment_lease && s_owner.latest_pending_send &&
             !s_owner.telemetry_lease_held)
    {
        defer_telemetry_to_next_window("支付终态无可复用在线 RF 窗口");
    }
    if (release_payment_lease)
    {
        release_rental_lease();
    }
    if (outcome == CLOUD_PAYMENT_OUTCOME_PAID ||
        outcome == CLOUD_PAYMENT_OUTCOME_FATAL)
    {
        secure_zero(&s_owner.rental_session,
                    sizeof(s_owner.rental_session));
        s_owner.rental_session_valid = false;
    }
    refresh_rental_accelerate_eligibility();
}

static bool load_rental_context(
    const cloud_rental_request_t *request,
    config_service_snapshot_t *config,
    watch_state_snapshot_t *state)
{
    return request != NULL && config != NULL && state != NULL &&
           owner_config_snapshot(
               config,
               pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) == ESP_OK &&
           owner_watch_state_snapshot(
               state,
               pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) == ESP_OK &&
           config->cloud_configured &&
           rental_state_matches_request(state, request);
}

static void publish_rental_failure(
    const cloud_rental_request_t *request,
    watch_rental_result_t result,
    const char *error_code,
    const char *user_reason)
{
    cloud_rental_result_t failure = {0};
    initialize_result(request, 0U, &failure);
    set_result(&failure,
               result,
               false,
               error_code,
               user_reason);
    finish_payment_attempt(
        &failure,
        (uint64_t)(esp_timer_get_time() / 1000));
    publish_cloud_diagnostic(&failure);
    if (!deliver_result(&failure))
    {
        s_pending_result = failure;
        s_pending_result_valid = true;
        release_network_leases_for_result_backpressure();
    }
}

static bool try_start_rental_request(uint64_t monotonic_ms,
                                     uint64_t next_cloud_window_ms)
{
    if (s_owner.active_kind != CLOUD_ACTIVE_NONE || s_pending_result_valid)
    {
        return false;
    }
    bool consumed_request = false;
    cloud_rental_request_t queued_request = {0};
    if (xQueueReceive(s_request_queue, &queued_request, 0) == pdTRUE)
    {
        consumed_request = true;
        if (queued_request.request_id == 0U)
        {
            secure_zero(&queued_request, sizeof(queued_request));
            return true;
        }
        if (s_owner.authorized_online_hold)
        {
            if (rental_requests_match(&queued_request,
                                      &s_owner.authorized_session))
            {
                ESP_LOGI(TAG,
                         "已授权链路的重复支付请求已忽略，请求=%lu",
                         (unsigned long)queued_request.request_id);
                secure_zero(&queued_request, sizeof(queued_request));
                return true;
            }
            release_authorized_online_hold("新的 BLE 链路开始支付验证");
        }
        if (!s_owner.rental_session_valid)
        {
            s_owner.rental_session = queued_request;
            s_owner.rental_session_valid = true;
            cloud_payment_schedule_start(&s_owner.rental_schedule,
                                         monotonic_ms);
            refresh_rental_accelerate_eligibility();
            ESP_LOGI(TAG,
                     "支付自动复查会话已建立，请求=%lu，链路代次=%lu",
                     (unsigned long)queued_request.request_id,
                     (unsigned long)queued_request.link_generation);
        }
        else if (rental_requests_match(&queued_request,
                                       &s_owner.rental_session))
        {
            ESP_LOGI(TAG,
                     "支付重复请求已按同一会话忽略，请求=%lu",
                     (unsigned long)queued_request.request_id);
        }
        else
        {
            ESP_LOGW(TAG,
                     "支付请求与活动会话身份不符，已丢弃，请求=%lu，活动=%lu",
                     (unsigned long)queued_request.request_id,
                     (unsigned long)s_owner.rental_session.request_id);
        }
        secure_zero(&queued_request, sizeof(queued_request));
    }

    if (!s_owner.rental_session_valid)
    {
        return consumed_request;
    }
    watch_state_snapshot_t scheduling_state = {0};
    if (owner_watch_state_snapshot(&scheduling_state, 0) == ESP_OK &&
        (scheduling_state.gps.acquisition_state ==
             WATCH_GPS_ACQUISITION_SEARCHING ||
         scheduling_state.gps.acquisition_state ==
             WATCH_GPS_ACQUISITION_TRACKING))
    {
#ifndef CLOUD_SERVICE_TEST
        if (!s_owner.gps_pause_requested)
        {
            const esp_err_t pause_error = gps_service_pause_for_radio(0);
            s_owner.gps_pause_requested = pause_error == ESP_OK;
            if (pause_error != ESP_OK)
            {
                ESP_LOGW(TAG,
                         "支付查询暂停 GPS 请求暂未入队，错误=0x%x，将重试",
                         (unsigned)pause_error);
            }
        }
#endif
        ESP_LOGI(TAG,
                 "支付查询等待 GPS 进入待机，请求=%lu",
                 (unsigned long)s_owner.rental_session.request_id);
        return consumed_request;
    }
    s_owner.gps_pause_requested = false;
    const cloud_payment_mode_t mode_before = cloud_payment_schedule_mode(
        &s_owner.rental_schedule,
        monotonic_ms);
    if (!cloud_payment_schedule_take_due(&s_owner.rental_schedule,
                                         monotonic_ms,
                                         next_cloud_window_ms))
    {
        const cloud_payment_mode_t mode_after = cloud_payment_schedule_mode(
            &s_owner.rental_schedule,
            monotonic_ms);
        if (mode_before == CLOUD_PAYMENT_MODE_FAST &&
            mode_after == CLOUD_PAYMENT_MODE_SLOW)
        {
            release_rental_lease();
        }
        return consumed_request;
    }
    atomic_store(&s_rental_in_flight_request_id,
                 s_owner.rental_session.request_id);
    refresh_rental_accelerate_eligibility();

    s_owner.active_rental = s_owner.rental_session;
    secure_zero(&s_rental_config_scratch,
                sizeof(s_rental_config_scratch));
    secure_zero(&s_rental_state_scratch,
                sizeof(s_rental_state_scratch));
    if (!load_rental_context(&s_owner.active_rental,
                             &s_rental_config_scratch,
                             &s_rental_state_scratch))
    {
        publish_rental_failure(&s_owner.active_rental,
                               WATCH_RENTAL_RESULT_CONFIG_ERROR,
                               "CLOUD_CONFIG_INVALID",
                               "配置或证书错误");
        secure_zero(&s_rental_config_scratch,
                    sizeof(s_rental_config_scratch));
        secure_zero(&s_rental_state_scratch,
                    sizeof(s_rental_state_scratch));
        secure_zero(&s_owner.active_rental,
                    sizeof(s_owner.active_rental));
        return true;
    }
    const bool modem_ready = s_rental_state_scratch.modem_ready;
    secure_zero(&s_rental_config_scratch,
                sizeof(s_rental_config_scratch));
    secure_zero(&s_rental_state_scratch,
                sizeof(s_rental_state_scratch));
    if (!s_owner.rental_lease_held)
    {
        if (owner_modem_request_connect() != ESP_OK)
        {
            publish_rental_failure(&s_owner.active_rental,
                                   WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
                                   "CLOUD_UNREACHABLE",
                                   "云端不可达");
            secure_zero(&s_owner.active_rental,
                        sizeof(s_owner.active_rental));
            return true;
        }
        s_owner.rental_lease_held = true;
    }
    if (!modem_ready)
    {
        s_owner.active_kind = CLOUD_ACTIVE_RENTAL_WAIT_MODEM;
        s_owner.rental_wait_deadline_ms =
            monotonic_ms + CLOUD_MODEM_WAKE_WAIT_MS;
        s_owner.active_rental_context_valid = true;
        ESP_LOGI(TAG,
                 "支付请求已触发 4G 显式联网，请求=%lu，等待上限=%u ms",
                 (unsigned long)s_owner.active_rental.request_id,
                 CLOUD_MODEM_WAKE_WAIT_MS);
        return true;
    }
    const uint32_t transport_request_id = next_transport_request_id();
    if (execute_rental_request(&s_owner.active_rental,
                               transport_request_id))
    {
        s_owner.active_kind = CLOUD_ACTIVE_RENTAL;
        s_owner.active_request_id = transport_request_id;
    }
    else
    {
        secure_zero(&s_owner.active_rental, sizeof(s_owner.active_rental));
    }
    return true;
}

static void advance_rental_modem_wait(uint64_t monotonic_ms)
{
    if (s_owner.active_kind != CLOUD_ACTIVE_RENTAL_WAIT_MODEM)
    {
        return;
    }
    if (!s_owner.active_rental_context_valid ||
        !s_owner.cloud_configured)
    {
        ESP_LOGW(TAG,
                 "支付等待 4G 期间事务已失效，请求=%lu",
                 (unsigned long)s_owner.active_rental.request_id);
        cancel_rental_session("等待联网期间支付身份失效");
        return;
    }
    if (s_owner.modem_ready)
    {
        const uint32_t transport_request_id = next_transport_request_id();
        if (execute_rental_request(&s_owner.active_rental,
                                   transport_request_id))
        {
            s_owner.active_kind = CLOUD_ACTIVE_RENTAL;
            s_owner.active_request_id = transport_request_id;
            s_owner.rental_wait_deadline_ms = 0U;
            ESP_LOGI(TAG,
                     "支付等待 4G 已就绪，开始云查询，请求=%lu",
                     (unsigned long)s_owner.active_rental.request_id);
        }
        else
        {
            finish_active_operation();
        }
        return;
    }
    if (monotonic_ms < s_owner.rental_wait_deadline_ms)
    {
        return;
    }
    ESP_LOGW(TAG,
             "支付等待 4G 联网超时，请求=%lu，等待=%u ms",
             (unsigned long)s_owner.active_rental.request_id,
             CLOUD_MODEM_WAKE_WAIT_MS);
    publish_rental_failure(&s_owner.active_rental,
                           WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
                           "CLOUD_UNREACHABLE",
                           "云端不可达");
    finish_active_operation();
}

static void finish_active_operation(void)
{
    secure_zero(&s_owner.active_rental, sizeof(s_owner.active_rental));
    s_owner.active_kind = CLOUD_ACTIVE_NONE;
    s_owner.active_request_id = 0U;
    s_owner.rental_wait_deadline_ms = 0U;
    s_owner.active_rental_context_valid = false;
    s_owner.active_telemetry_identity_valid = false;
    s_owner.active_telemetry_binding_generation = 0U;
}

static void handle_rental_terminal(const ml307r_https_response_t *response,
                                   esp_err_t receive_error,
                                   uint64_t monotonic_ms)
{
    cloud_rental_result_t result = {0};
    if (receive_error != ESP_OK || response == NULL)
    {
        initialize_result(&s_owner.active_rental, 0U, &result);
        set_result(&result,
                   WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
                   false,
                   "CLOUD_TRANSPORT_RESULT_INVALID",
                   "云端不可达");
    }
    else
    {
        const esp_err_t parse_error = cloud_service_parse_rental_http(
            &s_owner.active_rental,
            response->http_status,
            map_transport_result(response->error),
            response->body,
            response->body_length,
            &result);
        if (parse_error == ESP_OK && result.server_time_ms >= 0 &&
            response->http_status >= 200U && response->http_status < 300U)
        {
            if (owner_note_and_persist_cloud_time(result.server_time_ms,
                                                  monotonic_ms) != ESP_OK)
            {
                set_result(&result,
                           WATCH_RENTAL_RESULT_CONFIG_ERROR,
                           false,
                           "CLOUD_TIME_PERSIST_FAILED",
                           "云时间配置保存失败");
            }
        }
        if (response->error == ML307R_HTTPS_ERROR_MODEM_RESTARTED ||
            response->error == ML307R_HTTPS_ERROR_CANCELLED)
        {
            clear_latest_telemetry();
        }
    }
    ESP_LOGI(TAG,
             "租赁查询已结束，请求=%lu，HTTP=%u，正文长度=%u，稳定码=%s",
             (unsigned long)s_owner.active_rental.request_id,
             response != NULL ? (unsigned)response->http_status : 0U,
             response != NULL ? (unsigned)response->body_length : 0U,
             result.error_code);
    finish_payment_attempt(&result, monotonic_ms);
    publish_cloud_diagnostic(&result);
    if (!deliver_result(&result))
    {
        s_pending_result = result;
        s_pending_result_valid = true;
        release_network_leases_for_result_backpressure();
        ESP_LOGW(TAG,
                 "租赁结果队列暂满，已保留终态，请求=%lu",
                 (unsigned long)result.request_id);
    }
}

static esp_err_t owner_note_and_persist_cloud_time(
    int64_t server_time_ms,
    uint64_t monotonic_ms)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        if (server_time_ms < 0 ||
            server_time_ms > CLOUD_SERVER_TIME_MAX_MS ||
            monotonic_ms > (uint64_t)INT64_MAX)
        {
            return ESP_ERR_INVALID_ARG;
        }
        s_boot_cloud_offset_ms =
            server_time_ms - (int64_t)monotonic_ms;
        s_boot_time_synchronized = true;
        return ESP_OK;
    }
#endif
    int64_t offset_ms = 0;
    const esp_err_t note_error =
        cloud_service_note_server_time(server_time_ms,
                                       monotonic_ms,
                                       &offset_ms);
    if (note_error != ESP_OK)
    {
        return note_error;
    }
    return state_service_persist_cloud_offset(
        offset_ms,
        pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS));
}

static bool active_telemetry_identity_is_current(void)
{
    if (!s_owner.active_telemetry_identity_valid ||
        s_owner.active_kind != CLOUD_ACTIVE_TELEMETRY)
    {
        return false;
    }
    watch_state_snapshot_t state = {0};
    return watch_state_snapshot(&state,
                                pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) == ESP_OK &&
           state.cloud_configured && state.has_bound_exoskeleton &&
           canonical_mac_is_valid(state.bound_exoskeleton_mac,
                                  sizeof(state.bound_exoskeleton_mac)) &&
           memcmp(state.bound_exoskeleton_mac,
                  s_owner.bound_mac,
                  sizeof(s_owner.bound_mac)) == 0 &&
           state.ble_link_generation ==
               s_owner.active_telemetry_binding_generation;
}

static esp_err_t commit_cloud_time_if_authorized(void *context)
{
    cloud_time_commit_context_t *commit =
        (cloud_time_commit_context_t *)context;
    if (commit == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return cloud_service_note_server_time(commit->server_time_ms,
                                          commit->monotonic_ms,
                                          &commit->offset_ms);
}

static void handle_telemetry_terminal(const ml307r_https_response_t *response,
                                      esp_err_t receive_error)
{
#ifdef CLOUD_SERVICE_TEST
    if (response != NULL)
    {
        s_test_last_telemetry_terminal = *response;
        s_test_last_telemetry_terminal_valid = true;
    }
#endif
    if (response != NULL &&
        (response->error == ML307R_HTTPS_ERROR_MODEM_RESTARTED ||
         response->error == ML307R_HTTPS_ERROR_CANCELLED))
    {
        clear_latest_telemetry();
    }
    if (!active_telemetry_identity_is_current())
    {
        s_owner.active_telemetry_identity_valid = false;
        ESP_LOGW(TAG,
                 "telemetry 已丢弃旧绑定终态，请求=%lu，HTTP=%u，正文长度=%u",
                 (unsigned long)(response != NULL ? response->request_id
                                                  : s_owner.active_request_id),
                 response != NULL ? (unsigned)response->http_status : 0U,
                 response != NULL ? (unsigned)response->body_length : 0U);
        return;
    }
    const uint64_t monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000);
    cloud_telemetry_response_t parsed = {0};
    if (receive_error != ESP_OK || response == NULL)
    {
        (void)cloud_service_parse_telemetry_http(
            0U,
            CLOUD_TRANSPORT_INTERNAL,
            NULL,
            0U,
            &parsed);
    }
    else
    {
        (void)cloud_service_parse_telemetry_http(
            response->http_status,
            map_transport_result(response->error),
            response->body,
            response->body_length,
            &parsed);
    }

    bool time_synchronized = false;
    int64_t offset_ms = 0;
    if (parsed.result == CLOUD_TELEMETRY_RESULT_ACCEPTED && parsed.accepted)
    {
        cloud_time_commit_context_t commit = {
            .server_time_ms = parsed.server_time_ms,
            .monotonic_ms = monotonic_ms,
        };
        if (watch_state_run_if_binding_current(
                s_owner.bound_mac,
                s_owner.active_telemetry_binding_generation,
                commit_cloud_time_if_authorized,
                &commit,
                pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) != ESP_OK)
        {
            s_owner.active_telemetry_identity_valid = false;
            ESP_LOGW(TAG,
                     "telemetry 成功终态在校时提交前绑定已变化，请求=%lu，稳定码=CLOUD_IDENTITY_STALE",
                     (unsigned long)s_owner.active_request_id);
            return;
        }
        offset_ms = commit.offset_ms;
        time_synchronized = true;
        if (state_service_persist_cloud_offset(
                offset_ms,
                pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS)) != ESP_OK)
        {
            memset(parsed.error_code, 0, sizeof(parsed.error_code));
            memcpy(parsed.error_code,
                   "CLOUD_TIME_PERSIST_FAILED",
                   sizeof("CLOUD_TIME_PERSIST_FAILED"));
            ESP_LOGW(TAG,
                     "telemetry 云时间偏移保存失败，请求=%lu，稳定码=%s",
                     (unsigned long)s_owner.active_request_id,
                     parsed.error_code);
        }
    }
    publish_telemetry_diagnostic(&parsed,
                                 s_owner.active_request_id,
                                 monotonic_ms,
                                 time_synchronized,
                                 offset_ms);
    ESP_LOGI(TAG,
             "telemetry 请求组已结束，请求=%lu，HTTP=%u，正文长度=%u，稳定码=%s",
             (unsigned long)s_owner.active_request_id,
             response != NULL ? (unsigned)response->http_status : 0U,
             response != NULL ? (unsigned)response->body_length : 0U,
             parsed.error_code);
}

static void process_modem_terminal(uint64_t monotonic_ms)
{
    secure_zero(&s_transport_response_scratch,
                sizeof(s_transport_response_scratch));
    const esp_err_t receive_error = owner_modem_receive(
        &s_transport_response_scratch,
        0);
    if (receive_error == ESP_ERR_TIMEOUT || receive_error == ESP_ERR_NOT_FOUND)
    {
        return;
    }
    if (s_owner.active_kind == CLOUD_ACTIVE_NONE ||
        s_owner.active_kind == CLOUD_ACTIVE_RENTAL_WAIT_MODEM)
    {
        ESP_LOGW(TAG,
                 "cloud_task 已丢弃无归属 modem 终态，请求=%lu",
                 (unsigned long)s_transport_response_scratch.request_id);
        secure_zero(&s_transport_response_scratch,
                    sizeof(s_transport_response_scratch));
        return;
    }
    if (receive_error == ESP_OK &&
        s_transport_response_scratch.request_id != s_owner.active_request_id)
    {
        ESP_LOGW(TAG,
                 "cloud_task 已丢弃迟到 modem 终态，请求=%lu，当前=%lu，类型=%u",
                 (unsigned long)s_transport_response_scratch.request_id,
                 (unsigned long)s_owner.active_request_id,
                 (unsigned)s_owner.active_kind);
        secure_zero(&s_transport_response_scratch,
                    sizeof(s_transport_response_scratch));
        return;
    }
    if (s_owner.active_kind == CLOUD_ACTIVE_RENTAL)
    {
        handle_rental_terminal(
            receive_error == ESP_OK ? &s_transport_response_scratch : NULL,
            receive_error,
            monotonic_ms);
    }
    else
    {
        handle_telemetry_terminal(
            receive_error == ESP_OK ? &s_transport_response_scratch : NULL,
            receive_error);
        release_telemetry_lease();
    }
    finish_active_operation();
    secure_zero(&s_transport_response_scratch,
                sizeof(s_transport_response_scratch));
}

static esp_err_t owner_begin(void)
{
    if (s_request_queue == NULL || s_result_queue == NULL ||
        s_wake_queue == NULL ||
        atomic_load(&s_stop_requested) ||
        atomic_exchange(&s_owner_running, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    reset_owner_state();
    return ESP_OK;
}

/**
 * @brief 在独立栈帧中捕获产品状态并更新 owner binding 信息。
 * @details 独立函数确保 product state 快照 (~1800 字节) 在返回后释放，
 *          避免与后续 capture_due_telemetry/start_telemetry_request/
 *          execute_rental_request 内部的 state 副本同时驻留栈中。
 */
static void owner_step_observe_binding(uint64_t monotonic_ms)
{
    watch_state_snapshot_t state = {0};
    if (owner_watch_state_snapshot(&state, 0) == ESP_OK)
    {
        observe_binding(&state, monotonic_ms);
    }
}

static esp_err_t owner_step(uint64_t monotonic_ms)
{
    if (s_pending_cloud_update_dirty &&
        state_service_publish_cloud(&s_pending_cloud_update, 0) == ESP_OK)
    {
        s_pending_cloud_update_dirty = false;
    }
    if (s_pending_telemetry_update_dirty &&
        owner_publish_telemetry(&s_pending_telemetry_update, 0) ==
            ESP_OK)
    {
        s_pending_telemetry_update_dirty = false;
    }
    if (s_pending_result_valid && deliver_result(&s_pending_result))
    {
        secure_zero(&s_pending_result, sizeof(s_pending_result));
        s_pending_result_valid = false;
    }

    process_rental_control_signals(monotonic_ms);
    owner_step_observe_binding(monotonic_ms);
    const uint64_t current_cloud_window_ms = s_owner.next_window_ms;
    /* 先固定 latest 槽，支付终态才能在释放 lease 前完成同窗交接。 */
    capture_due_telemetry(monotonic_ms);
    process_modem_terminal(monotonic_ms);
    advance_rental_modem_wait(monotonic_ms);

    bool rental_consumed = false;
    if (s_owner.active_kind == CLOUD_ACTIVE_NONE &&
        !s_pending_result_valid)
    {
        rental_consumed = try_start_rental_request(monotonic_ms,
                                                   current_cloud_window_ms);
    }
    if (s_owner.active_kind == CLOUD_ACTIVE_NONE &&
        !s_pending_result_valid && !rental_consumed &&
        s_owner.latest_pending_send)
    {
        (void)start_telemetry_request(monotonic_ms);
    }
    return ESP_OK;
}

static TickType_t owner_next_wait_ticks(uint64_t monotonic_ms)
{
    if (s_owner.active_kind != CLOUD_ACTIVE_NONE ||
        s_pending_result_valid || s_pending_cloud_update_dirty ||
        s_pending_telemetry_update_dirty)
    {
        return pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS);
    }
    if (s_owner.gps_pause_requested || s_owner.latest_pending_send)
    {
        return pdMS_TO_TICKS(CLOUD_OWNER_DEFERRED_WAIT_MS);
    }

    uint64_t next_deadline_ms = 0U;
    if (s_owner.rental_session_valid && s_owner.rental_schedule.active)
    {
        next_deadline_ms = s_owner.rental_schedule.manual_due
                               ? monotonic_ms
                               : s_owner.rental_schedule.next_deadline_ms;
        if (!s_owner.rental_schedule.manual_due &&
            s_owner.rental_schedule.modem_available_ms > next_deadline_ms)
        {
            next_deadline_ms =
                s_owner.rental_schedule.modem_available_ms;
        }
    }
    if (s_owner.binding_tracked && s_owner.next_window_ms != 0U &&
        (next_deadline_ms == 0U ||
         s_owner.next_window_ms < next_deadline_ms))
    {
        next_deadline_ms = s_owner.next_window_ms;
    }
    if (next_deadline_ms == 0U)
    {
        return portMAX_DELAY;
    }
    if (next_deadline_ms <= monotonic_ms)
    {
        return 0U;
    }
    const uint64_t wait_ms = next_deadline_ms - monotonic_ms;
    if (wait_ms >= (uint64_t)UINT32_MAX)
    {
        return pdMS_TO_TICKS(UINT32_MAX);
    }
    const TickType_t wait_ticks = pdMS_TO_TICKS((uint32_t)wait_ms);
    return wait_ticks == 0U ? 1U : wait_ticks;
}

static void wake_owner(void)
{
    if (s_wake_queue == NULL)
    {
        return;
    }
    const uint8_t wake = 1U;
    (void)xQueueSend(s_wake_queue, &wake, 0U);
}

static void retry_pending_telemetry_diagnostic(void)
{
    for (uint32_t attempt = 0U;
         attempt < CLOUD_TELEMETRY_FINAL_PUBLISH_ATTEMPTS &&
         s_pending_telemetry_update_dirty;
         ++attempt)
    {
        if (owner_publish_telemetry(&s_pending_telemetry_update, 0) ==
            ESP_OK)
        {
            s_pending_telemetry_update_dirty = false;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS));
    }
    if (s_pending_telemetry_update_dirty)
    {
        ESP_LOGE(TAG,
                 "cloud_task 停止前 telemetry 终态仍未交付，稳定码=CLOUD_STATE_APPLY_FAILED");
    }
}

static void owner_end(void)
{
    retry_pending_telemetry_diagnostic();
    cancel_rental_session("cloud service 停止");
    release_authorized_online_hold("cloud service 停止");
    release_telemetry_lease();
    reset_owner_state();
    s_pending_result_valid = false;
    secure_zero(&s_pending_result, sizeof(s_pending_result));
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    secure_zero(&s_transport_response_scratch,
                sizeof(s_transport_response_scratch));
    if (s_request_queue != NULL)
    {
        (void)xQueueReset(s_request_queue);
    }
    if (s_result_queue != NULL)
    {
        (void)xQueueReset(s_result_queue);
    }
    if (s_wake_queue != NULL)
    {
        (void)xQueueReset(s_wake_queue);
    }
    cloud_service_reset_boot_time();
    atomic_store(&s_owner_running, false);
}

static esp_err_t owner_watch_state_snapshot(
    watch_state_snapshot_t *snapshot,
    TickType_t timeout_ticks)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        if (snapshot == NULL)
        {
            return ESP_ERR_INVALID_ARG;
        }
        if (s_test_watch_state_result == ESP_OK)
        {
            *snapshot = s_test_watch_state;
        }
        return s_test_watch_state_result;
    }
#endif
    return watch_state_snapshot(snapshot, timeout_ticks);
}

static esp_err_t owner_config_snapshot(config_service_snapshot_t *snapshot,
                                       TickType_t timeout_ticks)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        if (snapshot == NULL)
        {
            return ESP_ERR_INVALID_ARG;
        }
        if (s_test_config_result == ESP_OK)
        {
            *snapshot = s_test_config;
        }
        return s_test_config_result;
    }
#endif
    return config_service_snapshot(snapshot, timeout_ticks);
}

static esp_err_t owner_modem_submit(
    const ml307r_https_post_request_t *request,
    TickType_t timeout_ticks)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        if (request == NULL)
        {
            return ESP_ERR_INVALID_ARG;
        }
        ++s_test_modem_submit_count;
        return s_test_modem_submit_result;
    }
#endif
    return modem_service_https_post_async(request, timeout_ticks);
}

static esp_err_t owner_modem_request_connect(void)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        ++s_test_modem_connect_count;
        return s_test_modem_connect_result;
    }
#endif
    return modem_service_network_acquire(MODEM_NETWORK_LEASE_PAYMENT);
}

static esp_err_t owner_modem_request_telemetry_connect(void)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        ++s_test_modem_connect_count;
        return s_test_modem_connect_result;
    }
#endif
    return modem_service_network_acquire(MODEM_NETWORK_LEASE_TELEMETRY);
}

static void owner_modem_release(modem_network_lease_t lease)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        (void)lease;
        ++s_test_modem_release_count;
        return;
    }
#endif
    (void)modem_service_network_release(lease);
}

static void owner_modem_cancel(uint32_t request_id)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        (void)request_id;
        return;
    }
#endif
    (void)modem_service_https_cancel(request_id);
}

static esp_err_t owner_modem_receive(ml307r_https_response_t *response,
                                     TickType_t timeout_ticks)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        (void)timeout_ticks;
        const esp_err_t result = s_test_modem_receive_result;
        if (result == ESP_OK && response != NULL)
        {
            *response = s_test_modem_response;
        }
        s_test_modem_receive_result = ESP_ERR_TIMEOUT;
        secure_zero(&s_test_modem_response,
                    sizeof(s_test_modem_response));
        return result;
    }
#endif
    return modem_service_https_receive(response, timeout_ticks);
}

static esp_err_t owner_publish_telemetry(
    const watch_telemetry_update_t *update,
    TickType_t timeout_ticks)
{
#ifdef CLOUD_SERVICE_TEST
    if (s_test_dependencies_enabled)
    {
        (void)timeout_ticks;
        return update != NULL ? ESP_OK : ESP_ERR_INVALID_ARG;
    }
#endif
    return state_service_publish_telemetry(update, timeout_ticks);
}

static bool execute_rental_request(const cloud_rental_request_t *request,
                                   uint32_t transport_request_id)
{
    config_service_snapshot_t config = {0};
    watch_state_snapshot_t state = {0};
    cloud_rental_result_t failure = {0};
    initialize_result(request, 0U, &failure);
    if (!load_rental_context(request, &config, &state))
    {
        set_result(&failure,
                   WATCH_RENTAL_RESULT_CONFIG_ERROR,
                   false,
                   "CLOUD_CONFIG_INVALID",
                   "配置或证书错误");
        finish_payment_attempt(&failure,
                               (uint64_t)(esp_timer_get_time() / 1000));
        publish_cloud_diagnostic(&failure);
        if (!deliver_result(&failure))
        {
            s_pending_result = failure;
            s_pending_result_valid = true;
            release_network_leases_for_result_backpressure();
        }
        return false;
    }
    const uint64_t monotonic_ms = (uint64_t)(esp_timer_get_time() / 1000);
    cloud_rental_http_request_t http = {0};
    if (cloud_service_build_rental_http(
            request,
            config.watch_id,
            cloud_service_timestamp_ms(monotonic_ms),
            &http) != ESP_OK)
    {
        set_result(&failure,
                   WATCH_RENTAL_RESULT_CONFIG_ERROR,
                   false,
                   "CLOUD_REQUEST_INVALID",
                   "授权请求无效");
        finish_payment_attempt(&failure, monotonic_ms);
        publish_cloud_diagnostic(&failure);
        if (!deliver_result(&failure))
        {
            s_pending_result = failure;
            s_pending_result_valid = true;
            release_network_leases_for_result_backpressure();
        }
        return false;
    }
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    s_transport_request_scratch.request_id = transport_request_id;
    s_transport_request_scratch.max_retries = 0U;
    s_transport_request_scratch.deadline_ticks =
        xTaskGetTickCount() + pdMS_TO_TICKS(CLOUD_REQUEST_GROUP_TIMEOUT_MS);
    s_transport_request_scratch.body_length = http.body_length;
    memcpy(s_transport_request_scratch.path,
           http.path,
           strlen(http.path) + 1U);
    memcpy(s_transport_request_scratch.body, http.body, http.body_length);
    const esp_err_t submit_error = owner_modem_submit(
        &s_transport_request_scratch,
        pdMS_TO_TICKS(CLOUD_OWNER_POLL_MS));
    secure_zero(&s_transport_request_scratch,
                sizeof(s_transport_request_scratch));
    secure_zero(&http, sizeof(http));
    if (submit_error != ESP_OK)
    {
        set_result(&failure,
                   WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
                   false,
                   "CLOUD_UNREACHABLE",
                   "云端不可达");
        finish_payment_attempt(&failure, monotonic_ms);
        publish_cloud_diagnostic(&failure);
        if (!deliver_result(&failure))
        {
            s_pending_result = failure;
            s_pending_result_valid = true;
            release_network_leases_for_result_backpressure();
        }
        return false;
    }
    return true;
}

static bool deliver_result(const cloud_rental_result_t *result)
{
    if (result == NULL || s_result_queue == NULL ||
        xQueueSend(s_result_queue, result, 0) != pdTRUE)
    {
        return false;
    }
    (void)ble_service_notify_cloud_result_ready();
    return true;
}

static void publish_cloud_diagnostic(const cloud_rental_result_t *result)
{
    if (result == NULL || result->request_id == 0U)
    {
        return;
    }
    do
    {
        ++s_cloud_update_sequence;
    } while (s_cloud_update_sequence == 0U);
    s_pending_cloud_update = (watch_cloud_update_t){
        .cloud_update_sequence = s_cloud_update_sequence,
        .request_id = result->request_id,
        .result = result->result,
        .http_status = result->http_status,
    };
    memcpy(s_pending_cloud_update.error_code,
           result->error_code,
           sizeof(s_pending_cloud_update.error_code));
    s_pending_cloud_update_dirty = true;
    if (state_service_publish_cloud(&s_pending_cloud_update, 0) == ESP_OK)
    {
        s_pending_cloud_update_dirty = false;
    }
}

static void publish_telemetry_diagnostic(
    const cloud_telemetry_response_t *response,
    uint32_t request_id,
    uint64_t monotonic_ms,
    bool time_synchronized,
    int64_t offset_ms)
{
    if (response == NULL || request_id == 0U)
    {
        return;
    }
    do
    {
        ++s_telemetry_update_sequence;
    } while (s_telemetry_update_sequence == 0U);
    s_pending_telemetry_update = (watch_telemetry_update_t){
        .telemetry_update_sequence = s_telemetry_update_sequence,
        .request_id = request_id,
        .binding_generation =
            s_owner.active_kind == CLOUD_ACTIVE_TELEMETRY
                ? s_owner.active_telemetry_binding_generation
                : s_owner.latest.binding_generation,
        .result = map_telemetry_result(response->result),
        .http_status = response->http_status,
        .last_attempt_monotonic_ms = monotonic_ms,
        .last_success_server_time_ms = time_synchronized
                                           ? response->server_time_ms
                                           : 0,
        .cloud_offset_ms = time_synchronized ? offset_ms : 0,
        .last_success_server_time_valid = time_synchronized,
        .cloud_time_synchronized = time_synchronized,
    };
    memcpy(s_pending_telemetry_update.exoskeleton_mac,
           s_owner.bound_mac,
           sizeof(s_pending_telemetry_update.exoskeleton_mac));
    memcpy(s_pending_telemetry_update.error_code,
           response->error_code,
           sizeof(s_pending_telemetry_update.error_code));
    s_pending_telemetry_update_dirty = true;
    if (owner_publish_telemetry(&s_pending_telemetry_update, 0) ==
        ESP_OK)
    {
        s_pending_telemetry_update_dirty = false;
    }
}

static void secure_zero(void *buffer, size_t length)
{
    volatile uint8_t *bytes = (volatile uint8_t *)buffer;
    while (length > 0U)
    {
        *bytes++ = 0U;
        --length;
    }
}
#endif
