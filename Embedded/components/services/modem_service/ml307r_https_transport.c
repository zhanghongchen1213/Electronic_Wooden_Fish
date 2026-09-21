/**
 * @file     ml307r_https_transport.c
 * @brief    ML307R 原生 HTTPS POST transport 实现
 * @details  使用 MHTTP/SSL 长度数据模式、逐条 Header、缓存响应、显式 host 截止、固定重试退避与完整敏感 staging 清理。
 * @author   ZHC
 * @date     2026-08-18
 */

#include "ml307r_https_transport.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "config_service.h"
#include "exoskeleton_scene_config.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/task.h"

static const char *TAG = "SVC_MODEM";

/** URL host 容量。 */
#define ML307R_HTTPS_HOST_CAPACITY 128U
/** URL base path 容量。 */
#define ML307R_HTTPS_BASE_PATH_CAPACITY 192U
/** 单条 header staging 容量，包含名称、分隔符、值和终止符。 */
#define ML307R_HTTPS_HEADER_STAGING_CAPACITY \
    (ML307R_HTTPS_HEADER_NAME_CAPACITY + ML307R_HTTPS_HEADER_VALUE_CAPACITY + 1U)
/** transport 自动注入的固定 header 数量。 */
#define ML307R_HTTPS_FIXED_HEADER_COUNT 2U
/** AT 子命令默认预算。 */
#define ML307R_HTTPS_AT_TIMEOUT_MS 3000U
/** ML307R 固定 SSL context。 */
#define ML307R_HTTPS_SSL_CONTEXT 0
/** ML307R HTTPS 实例必须显式启用 SSL。 */
#define ML307R_HTTPS_SSL_ENABLE 1
/** ML307R 普通模式 content 已全部输入。 */
#define ML307R_HTTPS_CONTENT_EOF 0
/** ML307R HTTP POST 方法编号。 */
#define ML307R_HTTPS_METHOD_POST 2
/** ML307R 缓存响应头读取类型。 */
#define ML307R_HTTPS_READ_HEADER 0
/** ML307R 缓存响应正文读取类型。 */
#define ML307R_HTTPS_READ_CONTENT 1
/** MHTTPREAD 元数据后进入原始数据所需跳过的逗号数。 */
#define ML307R_HTTPS_READ_INLINE_DELIMITERS 4U

typedef enum
{
    HTTPS_STAGE_IDLE = 0,    /**< 无请求。 */
    HTTPS_STAGE_CERT_WRITE,  /**< 写入 CA PEM。 */
    HTTPS_STAGE_SSL_CERT,    /**< 选择 CA 引用。 */
    HTTPS_STAGE_SSL_AUTH,    /**< 按 CA 配置选择认证方式。 */
    HTTPS_STAGE_CREATE,      /**< 创建 HTTPS 实例并取得 httpid。 */
    HTTPS_STAGE_CFG_CACHED,  /**< 开启缓存响应。 */
    HTTPS_STAGE_CFG_TIMEOUT, /**< 配置模组下层 timeout。 */
    HTTPS_STAGE_CFG_SSL,     /**< 绑定 SSL context。 */
    HTTPS_STAGE_HEADER,      /**< 逐条以 more/length prompt 写 header。 */
    HTTPS_STAGE_CONTENT,     /**< 以长度 prompt 写 body。 */
    HTTPS_STAGE_REQUEST,     /**< 发起 POST。 */
    HTTPS_STAGE_WAIT_URC,    /**< 等待 recv/err URC。 */
    HTTPS_STAGE_READ,        /**< 按报告长度 MHTTPREAD。 */
    HTTPS_STAGE_TERM,        /**< 超时/取消 best-effort 终止。 */
    HTTPS_STAGE_DELETE,      /**< 删除实例与配置。 */
    HTTPS_STAGE_RETRY_WAIT,  /**< reader 继续工作的定时退避。 */
} https_stage_t;

typedef enum
{
    HTTPS_COMMAND_CREATE = 0,   /**< 创建 HTTPS 实例。 */
    HTTPS_COMMAND_TIMEOUT,      /**< 配置连接、响应与输入超时。 */
    HTTPS_COMMAND_SSL,          /**< 为实例启用并绑定 SSL context。 */
    HTTPS_COMMAND_CONTENT,      /**< 输入完整 POST body。 */
    HTTPS_COMMAND_REQUEST,      /**< 发起 POST path 请求。 */
    HTTPS_COMMAND_READ_HEADER,  /**< 读取缓存响应头。 */
    HTTPS_COMMAND_READ_CONTENT, /**< 读取缓存响应正文。 */
} https_command_t;

typedef struct
{
    bool active;                                                  /**< 请求组是否活动。 */
    bool result_ready;                                            /**< typed 终态是否待领取。 */
    bool waiting_at;                                              /**< 是否等待当前 AT request id。 */
    bool cleanup_for_failure;                                     /**< cleanup 后重试或结束失败。 */
    bool response_complete;                                       /**< 已按报告长度取得完整 HTTP 终态。 */
    bool pending_http_urc_valid;                                  /**< REQUEST 的 OK 前已收到匹配 HTTP URC。 */
    bool pending_http_received;                                   /**< 暂存 URC 是否为 recv。 */
    uint32_t generation;                                          /**< 每个逻辑请求代次。 */
    uint32_t awaited_generation;                                  /**< 当前 HTTP URC 允许完成的代次。 */
    uint32_t next_at_request_id;                                  /**< transport AT 请求 ID。 */
    uint32_t active_at_request_id;                                /**< 当前 AT 请求 ID。 */
    uint32_t late_urc_count;                                      /**< 迟到/不匹配 URC 饱和计数。 */
    uint8_t retry_count;                                          /**< 已执行重试数。 */
    uint8_t header_index;                                         /**< 当前待提交 header 索引。 */
    int httpid;                                                   /**< ML307R HTTP 实例 ID。 */
    uint16_t response_header_length;                              /**< 模组报告 header 长度。 */
    uint16_t response_length;                                     /**< 模组报告 body 长度。 */
    uint16_t response_capacity;                                   /**< 当前读取目标容量。 */
    uint16_t read_offset;                                         /**< 分段 MHTTPREAD 当前偏移。 */
    uint16_t read_chunk_length;                                   /**< 当前 raw 分段期望长度。 */
    int pending_http_status;                                      /**< OK 前暂存的 HTTP status。 */
    int pending_http_header_length;                               /**< OK 前暂存的 header 长度。 */
    int pending_http_content_length;                              /**< OK 前暂存的 body 长度。 */
    TickType_t group_deadline_ticks;                              /**< 调用方逻辑请求截止。 */
    TickType_t attempt_deadline_ticks;                            /**< 当前 attempt 20 秒截止。 */
    TickType_t connect_deadline_ticks;                            /**< 当前连接/TLS 10 秒截止。 */
    TickType_t cleanup_deadline_ticks;                            /**< TERM/DELETE 独立清理截止。 */
    TickType_t retry_at_ticks;                                    /**< 退避结束绝对时间。 */
    https_stage_t stage;                                          /**< 当前 transport 阶段。 */
    ml307r_https_error_t pending_error;                           /**< cleanup 后固化的错误。 */
    ml307r_https_post_request_t request;                          /**< 按值请求 staging。 */
    config_service_cloud_credentials_t credentials;               /**< 临时敏感配置副本。 */
    char host[ML307R_HTTPS_HOST_CAPACITY];                        /**< 已验证 SNI host。 */
    uint16_t port;                                                /**< HTTPS 端口。 */
    char base_path[ML307R_HTTPS_BASE_PATH_CAPACITY];              /**< base URL path。 */
    char endpoint[ML307R_HTTPS_PATH_CAPACITY];                    /**< 合成 endpoint。 */
    uint8_t header_staging[ML307R_HTTPS_HEADER_STAGING_CAPACITY]; /**< 敏感 header。 */
    uint16_t header_staging_length;                               /**< header 有效长度。 */
    ml307r_https_response_t result;                               /**< 待领取 typed 终态。 */
} https_transport_t;

/** modem task 独占的 PSRAM transport 常驻状态。 */
static https_transport_t *s_transport_storage;
/** transport 常驻状态的稳定别名。 */
#define s_transport (*s_transport_storage)
/** 启动周期内跨请求单调 generation，避免迟到 URC 完成新请求。 */
static uint32_t s_next_generation;
/** 启动周期内跨请求单调 AT request id。 */
static uint32_t s_next_transport_at_id;

static void secure_clear(void *buffer, size_t length);
static void saturating_increment(uint32_t *value);
static bool tick_reached(TickType_t now_ticks, TickType_t deadline_ticks);
static TickType_t earlier_deadline(TickType_t first, TickType_t second);
static TickType_t select_command_deadline(
    bool cleanup,
    TickType_t now_ticks,
    TickType_t attempt_deadline_ticks,
    TickType_t cleanup_deadline_ticks);
static TickType_t new_cleanup_deadline(TickType_t now_ticks);
static TickType_t command_deadline(TickType_t now_ticks);
static ml307r_https_error_t classify_at_failure(
    at_core_result_status_t status,
    uint32_t rx_byte_count,
    bool tls_stage);
static const char *stage_name(https_stage_t stage);
static bool valid_text(const char *text, size_t capacity, bool allow_slash);
static bool valid_header_name(const char *text, size_t capacity);
static bool header_name_is_reserved(const char *name);
static esp_err_t encode_protocol_command(https_command_t command_kind,
                                         const char *host,
                                         uint16_t port,
                                         int httpid,
                                         uint16_t length,
                                         char *command,
                                         size_t command_capacity);
static bool parse_https_url(const char *url,
                            char host[ML307R_HTTPS_HOST_CAPACITY],
                            uint16_t *port,
                            char base_path[ML307R_HTTPS_BASE_PATH_CAPACITY]);
static bool join_endpoint_path(const char *base_path,
                               const char *path,
                               char endpoint[ML307R_HTTPS_PATH_CAPACITY]);
static bool current_header(const char **name, const char **value);
static esp_err_t encode_header(const char *name,
                               const char *value,
                               int httpid,
                               bool has_more,
                               char *command,
                               size_t command_capacity,
                               uint8_t *data,
                               size_t data_capacity,
                               uint16_t *data_length);
static uint32_t next_at_request_id(void);
static esp_err_t submit_stage_command(const char *command,
                                      const char *prefix,
                                      const void *data,
                                      size_t data_length,
                                      uint16_t raw_response_length,
                                      bool sensitive,
                                      TickType_t now_ticks);
static esp_err_t queue_current_stage(TickType_t now_ticks);
static void begin_attempt(TickType_t now_ticks);
static void advance_after_ok(const legbot_at_result_t *result, TickType_t now_ticks);
static void fail_attempt(ml307r_https_error_t error, TickType_t now_ticks);
static void schedule_retry_or_finish(TickType_t now_ticks);
static void finish(ml307r_https_error_t error, const char *code);
static bool http_status_is_retryable(uint16_t status);
static bool copy_read_chunk(const uint8_t *data, uint16_t length);
static void apply_http_urc(bool received,
                           int status,
                           int header_length,
                           int content_length,
                           TickType_t now_ticks);
static const char *error_code(ml307r_https_error_t error);
static bool parse_http_urc(const char *line,
                           bool *received,
                           int *httpid,
                           int *status,
                           int *header_length,
                           int *content_length);

esp_err_t ml307r_https_transport_prepare(void)
{
    if (s_transport_storage != NULL)
    {
        return ESP_OK;
    }
    s_transport_storage = heap_caps_calloc(
        1U,
        sizeof(*s_transport_storage),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_transport_storage == NULL)
    {
        ESP_LOGE(TAG,
                 "HTTPS transport PSRAM 常驻状态申请失败，字节=%lu",
                 (unsigned long)sizeof(*s_transport_storage));
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG,
             "HTTPS transport PSRAM 常驻状态已就绪，字节=%lu",
             (unsigned long)sizeof(*s_transport_storage));
    return ESP_OK;
}

esp_err_t ml307r_https_transport_start(const ml307r_https_post_request_t *request,
                                       TickType_t now_ticks)
{
    if (s_transport_storage == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_transport.active || s_transport.result_ready)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (request == NULL || request->request_id == 0U ||
        request->deadline_ticks == 0U || tick_reached(now_ticks, request->deadline_ticks) ||
        request->header_count > ML307R_HTTPS_HEADER_COUNT ||
        request->max_retries > CLOUD_REQUEST_RETRY_MAX ||
        request->body_length > ML307R_HTTPS_BODY_CAPACITY ||
        !valid_text(request->path, sizeof(request->path), true) ||
        request->path[0] != '/')
    {
        return ESP_ERR_INVALID_ARG;
    }
    for (size_t index = 0; index < request->header_count; ++index)
    {
        if (!valid_header_name(request->headers[index].name,
                               sizeof(request->headers[index].name)) ||
            header_name_is_reserved(request->headers[index].name) ||
            !valid_text(request->headers[index].value,
                        sizeof(request->headers[index].value), true))
        {
            return ESP_ERR_INVALID_ARG;
        }
    }
    config_service_snapshot_t config = {0};
    if (config_service_snapshot(&config, pdMS_TO_TICKS(20U)) != ESP_OK ||
        config_service_cloud_preflight(&config) != CLOUD_ERROR_NONE)
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t generation = ++s_next_generation;
    if (generation == 0U)
    {
        generation = ++s_next_generation;
    }
    memset(&s_transport, 0, sizeof(s_transport));
    if (config_service_cloud_credentials_copy(&s_transport.credentials,
                                              pdMS_TO_TICKS(20U)) != ESP_OK)
    {
        secure_clear(&s_transport, sizeof(s_transport));
        return ESP_ERR_INVALID_STATE;
    }
    if (!parse_https_url(s_transport.credentials.base_url,
                         s_transport.host,
                         &s_transport.port,
                         s_transport.base_path) ||
        !join_endpoint_path(s_transport.base_path,
                            request->path,
                            s_transport.endpoint))
    {
        secure_clear(&s_transport, sizeof(s_transport));
        return ESP_ERR_INVALID_ARG;
    }
    s_transport.request = *request;
    s_transport.generation = generation;
    s_transport.active = true;
    s_transport.httpid = -1;
    s_transport.group_deadline_ticks = request->deadline_ticks;
    begin_attempt(now_ticks);
    ESP_LOGI(TAG,
             "HTTPS POST 已接受，请求=%lu，代次=%lu，正文长度=%u",
             (unsigned long)request->request_id,
             (unsigned long)s_transport.generation,
             (unsigned)request->body_length);
    return ESP_OK;
}

esp_err_t ml307r_https_transport_step(TickType_t now_ticks)
{
    if (s_transport_storage == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_transport.active)
    {
        return ESP_OK;
    }
    if (s_transport.cleanup_for_failure)
    {
        if (tick_reached(now_ticks, s_transport.cleanup_deadline_ticks))
        {
            if (s_transport.waiting_at)
            {
                (void)at_core_cancel(s_transport.active_at_request_id);
                s_transport.waiting_at = false;
            }
            ESP_LOGW(TAG,
                     "HTTPS 失败清理截止，请求=%lu，阶段=%s，错误=%s",
                     (unsigned long)s_transport.request.request_id,
                     stage_name(s_transport.stage),
                     error_code(s_transport.pending_error));
            schedule_retry_or_finish(now_ticks);
        }
    }
    else if (tick_reached(now_ticks, s_transport.group_deadline_ticks) ||
             (s_transport.stage != HTTPS_STAGE_RETRY_WAIT &&
              tick_reached(now_ticks, s_transport.attempt_deadline_ticks)))
    {
        fail_attempt(ML307R_HTTPS_ERROR_ATTEMPT_TIMEOUT, now_ticks);
    }
    else if (s_transport.stage <= HTTPS_STAGE_CFG_SSL &&
             tick_reached(now_ticks, s_transport.connect_deadline_ticks))
    {
        fail_attempt(ML307R_HTTPS_ERROR_CONNECT_TIMEOUT, now_ticks);
    }
    if (!s_transport.active || s_transport.waiting_at ||
        s_transport.stage == HTTPS_STAGE_WAIT_URC)
    {
        return ESP_OK;
    }
    if (s_transport.stage == HTTPS_STAGE_RETRY_WAIT)
    {
        if (!tick_reached(now_ticks, s_transport.retry_at_ticks))
        {
            return ESP_OK;
        }
        begin_attempt(now_ticks);
    }
    return queue_current_stage(now_ticks);
}

bool ml307r_https_transport_handle_at_result(const legbot_at_result_t *result,
                                             TickType_t now_ticks)
{
    if (s_transport_storage == NULL || result == NULL ||
        !s_transport.active || !s_transport.waiting_at ||
        result->request_id != s_transport.active_at_request_id)
    {
        return false;
    }
    s_transport.waiting_at = false;
    if (result->status == AT_CORE_RESULT_MODEM_RESTARTED)
    {
        finish(ML307R_HTTPS_ERROR_MODEM_RESTARTED, "MODEM_RESTARTED");
    }
    else if (result->status != AT_CORE_RESULT_OK)
    {
        ESP_LOGW(TAG,
                 "HTTPS AT 阶段失败，请求=%lu，阶段=%s，状态=%u，RX=%lu",
                 (unsigned long)s_transport.request.request_id,
                 stage_name(s_transport.stage),
                 (unsigned)result->status,
                 (unsigned long)result->rx_byte_count);
        if (s_transport.cleanup_for_failure &&
            result->status == AT_CORE_RESULT_TIMEOUT &&
            result->rx_byte_count == 0U)
        {
            s_transport.pending_error = ML307R_HTTPS_ERROR_AT_UNRESPONSIVE;
        }
        if (s_transport.cleanup_for_failure && s_transport.stage == HTTPS_STAGE_TERM)
        {
            s_transport.stage = HTTPS_STAGE_DELETE;
        }
        else if (s_transport.cleanup_for_failure && s_transport.stage == HTTPS_STAGE_DELETE)
        {
            schedule_retry_or_finish(now_ticks);
        }
        else if (s_transport.stage == HTTPS_STAGE_DELETE &&
                 s_transport.response_complete)
        {
            ESP_LOGW(TAG, "HTTP 响应已完成但实例删除失败，不重放 POST");
            finish(s_transport.pending_error, error_code(s_transport.pending_error));
        }
        else
        {
            fail_attempt(classify_at_failure(
                             result->status,
                             result->rx_byte_count,
                             s_transport.stage <= HTTPS_STAGE_CFG_SSL),
                         now_ticks);
        }
    }
    else
    {
        advance_after_ok(result, now_ticks);
    }
    return true;
}

bool ml307r_https_transport_handle_urc(const legbot_at_urc_t *urc,
                                       TickType_t now_ticks)
{
    if (s_transport_storage == NULL || urc == NULL ||
        strncmp(urc->line, "+MHTTPURC:", strlen("+MHTTPURC:")) != 0)
    {
        return false;
    }
    bool received = false;
    int httpid = -1;
    int status = 0;
    int header_length = 0;
    int content_length = 0;
    if (!s_transport.active ||
        !parse_http_urc(urc->line, &received, &httpid, &status,
                        &header_length, &content_length) ||
        httpid != s_transport.httpid)
    {
        saturating_increment(&s_transport.late_urc_count);
        return true;
    }
    if (s_transport.stage == HTTPS_STAGE_REQUEST && s_transport.waiting_at)
    {
        if (!s_transport.pending_http_urc_valid)
        {
            s_transport.pending_http_urc_valid = true;
            s_transport.pending_http_received = received;
            s_transport.pending_http_status = status;
            s_transport.pending_http_header_length = header_length;
            s_transport.pending_http_content_length = content_length;
        }
        return true;
    }
    if (s_transport.stage != HTTPS_STAGE_WAIT_URC ||
        s_transport.awaited_generation != s_transport.generation)
    {
        saturating_increment(&s_transport.late_urc_count);
        return true;
    }
    apply_http_urc(received, status, header_length, content_length, now_ticks);
    return true;
}

void ml307r_https_transport_cancel(bool modem_restarted)
{
    if (s_transport_storage == NULL || !s_transport.active)
    {
        return;
    }
    const int httpid = s_transport.httpid;
    at_core_abort_all(modem_restarted);
    secure_clear(&s_transport.credentials, sizeof(s_transport.credentials));
    secure_clear(s_transport.header_staging, sizeof(s_transport.header_staging));
    secure_clear(s_transport.request.headers, sizeof(s_transport.request.headers));
    secure_clear(s_transport.request.path, sizeof(s_transport.request.path));
    secure_clear(s_transport.request.body, sizeof(s_transport.request.body));
    s_transport.request.header_count = 0U;
    s_transport.request.body_length = 0U;
    secure_clear(s_transport.result.header, sizeof(s_transport.result.header));
    secure_clear(s_transport.result.body, sizeof(s_transport.result.body));
    s_transport.result.header_length = 0U;
    s_transport.result.body_length = 0U;
    s_transport.result.http_status = 0U;
    s_transport.response_complete = false;
    s_transport.pending_http_urc_valid = false;
    if (modem_restarted || httpid < 0)
    {
        finish(modem_restarted ? ML307R_HTTPS_ERROR_MODEM_RESTARTED
                               : ML307R_HTTPS_ERROR_CANCELLED,
               modem_restarted ? "MODEM_RESTARTED" : "MODEM_HTTPS_CANCELLED");
        return;
    }
    const TickType_t now_ticks = xTaskGetTickCount();
    s_transport.pending_error = ML307R_HTTPS_ERROR_CANCELLED;
    s_transport.cleanup_for_failure = true;
    s_transport.waiting_at = false;
    s_transport.awaited_generation = 0U;
    s_transport.stage = HTTPS_STAGE_TERM;
    s_transport.cleanup_deadline_ticks = new_cleanup_deadline(now_ticks);
    s_transport.group_deadline_ticks = s_transport.cleanup_deadline_ticks;
}

bool ml307r_https_transport_cancel_request(uint32_t request_id)
{
    if (s_transport_storage == NULL || request_id == 0U ||
        !s_transport.active ||
        s_transport.request.request_id != request_id)
    {
        return false;
    }
    ml307r_https_transport_cancel(false);
    return true;
}

bool ml307r_https_transport_active(void)
{
    return s_transport_storage != NULL && s_transport.active;
}

esp_err_t ml307r_https_transport_take_response(ml307r_https_response_t *response)
{
    if (response == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_transport_storage == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_transport.result_ready)
    {
        return ESP_ERR_NOT_FOUND;
    }
    *response = s_transport.result;
    secure_clear(&s_transport, sizeof(s_transport));
    return ESP_OK;
}

#ifdef ML307R_HTTPS_TRANSPORT_TEST
esp_err_t ml307r_https_transport_test_encode_header(
    int httpid,
    bool has_more,
    const char *name,
    const char *value,
    legbot_at_transaction_t *transaction)
{
    if (transaction == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(transaction, 0, sizeof(*transaction));
    esp_err_t err = encode_header(name,
                                  value,
                                  httpid,
                                  has_more,
                                  transaction->command,
                                  sizeof(transaction->command),
                                  transaction->data,
                                  sizeof(transaction->data),
                                  &transaction->data_length);
    if (err == ESP_OK)
    {
        transaction->phase = AT_CORE_PHASE_DATA_PROMPT;
        transaction->sensitive = true;
    }
    return err;
}

bool ml307r_https_transport_test_header_name_allowed(const char *name)
{
    return name != NULL &&
           valid_header_name(name, ML307R_HTTPS_HEADER_NAME_CAPACITY) &&
           !header_name_is_reserved(name);
}

TickType_t ml307r_https_transport_test_command_deadline(
    bool cleanup,
    TickType_t now_ticks,
    TickType_t attempt_deadline_ticks,
    TickType_t cleanup_deadline_ticks)
{
    return select_command_deadline(
        cleanup,
        now_ticks,
        attempt_deadline_ticks,
        cleanup_deadline_ticks);
}

TickType_t ml307r_https_transport_test_cleanup_deadline(
    TickType_t now_ticks)
{
    return new_cleanup_deadline(now_ticks);
}

ml307r_https_error_t ml307r_https_transport_test_classify_at_failure(
    at_core_result_status_t status,
    uint32_t rx_byte_count,
    bool tls_stage)
{
    return classify_at_failure(status, rx_byte_count, tls_stage);
}

esp_err_t ml307r_https_transport_test_encode_protocol_commands(
    const char *host,
    uint16_t port,
    int httpid,
    uint16_t body_length,
    uint16_t path_length,
    uint16_t read_length,
    ml307r_https_test_protocol_commands_t *commands)
{
    if (commands == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(commands, 0, sizeof(*commands));
    esp_err_t err = encode_protocol_command(
        HTTPS_COMMAND_CREATE,
        host,
        port,
        httpid,
        0U,
        commands->create,
        sizeof(commands->create));
    if (err == ESP_OK)
    {
        err = encode_protocol_command(
            HTTPS_COMMAND_TIMEOUT,
            NULL,
            0U,
            httpid,
            0U,
            commands->timeout,
            sizeof(commands->timeout));
    }
    if (err == ESP_OK)
    {
        err = encode_protocol_command(
            HTTPS_COMMAND_SSL,
            NULL,
            0U,
            httpid,
            0U,
            commands->ssl,
            sizeof(commands->ssl));
    }
    if (err == ESP_OK)
    {
        err = encode_protocol_command(
            HTTPS_COMMAND_CONTENT,
            NULL,
            0U,
            httpid,
            body_length,
            commands->content,
            sizeof(commands->content));
    }
    if (err == ESP_OK)
    {
        err = encode_protocol_command(
            HTTPS_COMMAND_REQUEST,
            NULL,
            0U,
            httpid,
            path_length,
            commands->request,
            sizeof(commands->request));
    }
    if (err == ESP_OK)
    {
        err = encode_protocol_command(
            HTTPS_COMMAND_READ_HEADER,
            NULL,
            0U,
            httpid,
            read_length,
            commands->read_header,
            sizeof(commands->read_header));
    }
    if (err == ESP_OK)
    {
        err = encode_protocol_command(
            HTTPS_COMMAND_READ_CONTENT,
            NULL,
            0U,
            httpid,
            read_length,
            commands->read_content,
            sizeof(commands->read_content));
    }
    commands->read_inline_delimiters =
        ML307R_HTTPS_READ_INLINE_DELIMITERS;
    return err;
}
#endif

static void secure_clear(void *buffer, size_t length)
{
    volatile uint8_t *bytes = (volatile uint8_t *)buffer;
    while (length-- > 0U)
    {
        *bytes++ = 0U;
    }
}

static void saturating_increment(uint32_t *value)
{
    if (*value < UINT32_MAX)
    {
        ++(*value);
    }
}

static bool tick_reached(TickType_t now_ticks, TickType_t deadline_ticks)
{
    return (int32_t)(now_ticks - deadline_ticks) >= 0;
}

static TickType_t earlier_deadline(TickType_t first, TickType_t second)
{
    return (int32_t)(first - second) <= 0 ? first : second;
}

static TickType_t select_command_deadline(
    bool cleanup,
    TickType_t now_ticks,
    TickType_t attempt_deadline_ticks,
    TickType_t cleanup_deadline_ticks)
{
    const TickType_t owner_deadline = cleanup
                                          ? cleanup_deadline_ticks
                                          : attempt_deadline_ticks;
    return earlier_deadline(
        owner_deadline,
        now_ticks + pdMS_TO_TICKS(ML307R_HTTPS_AT_TIMEOUT_MS));
}

static TickType_t command_deadline(TickType_t now_ticks)
{
    return select_command_deadline(
        s_transport.cleanup_for_failure,
        now_ticks,
        s_transport.attempt_deadline_ticks,
        s_transport.cleanup_deadline_ticks);
}

static TickType_t new_cleanup_deadline(TickType_t now_ticks)
{
    return now_ticks + pdMS_TO_TICKS(ML307R_HTTPS_CLEANUP_TIMEOUT_MS);
}

static ml307r_https_error_t classify_at_failure(
    at_core_result_status_t status,
    uint32_t rx_byte_count,
    bool tls_stage)
{
    if ((status == AT_CORE_RESULT_TIMEOUT && rx_byte_count == 0U) ||
        status == AT_CORE_RESULT_UART_ERROR)
    {
        return ML307R_HTTPS_ERROR_AT_UNRESPONSIVE;
    }
    return tls_stage ? ML307R_HTTPS_ERROR_TLS
                     : ML307R_HTTPS_ERROR_HTTP;
}

static const char *stage_name(https_stage_t stage)
{
    static const char *const names[] = {
        [HTTPS_STAGE_IDLE] = "idle",
        [HTTPS_STAGE_CERT_WRITE] = "cert_write",
        [HTTPS_STAGE_SSL_CERT] = "ssl_cert",
        [HTTPS_STAGE_SSL_AUTH] = "ssl_auth",
        [HTTPS_STAGE_CREATE] = "create",
        [HTTPS_STAGE_CFG_CACHED] = "cfg_cached",
        [HTTPS_STAGE_CFG_TIMEOUT] = "cfg_timeout",
        [HTTPS_STAGE_CFG_SSL] = "cfg_ssl",
        [HTTPS_STAGE_HEADER] = "header",
        [HTTPS_STAGE_CONTENT] = "content",
        [HTTPS_STAGE_REQUEST] = "request",
        [HTTPS_STAGE_WAIT_URC] = "wait_urc",
        [HTTPS_STAGE_READ] = "read",
        [HTTPS_STAGE_TERM] = "term",
        [HTTPS_STAGE_DELETE] = "delete",
        [HTTPS_STAGE_RETRY_WAIT] = "retry_wait",
    };
    return stage <= HTTPS_STAGE_RETRY_WAIT ? names[stage] : "unknown";
}

static bool valid_text(const char *text, size_t capacity, bool allow_slash)
{
    const size_t length = strnlen(text, capacity);
    if (length == 0U || length >= capacity)
    {
        return false;
    }
    for (size_t index = 0; index < length; ++index)
    {
        const unsigned char value = (unsigned char)text[index];
        if (value < 0x21U || value == 0x7fU || value == '\r' || value == '\n' ||
            value == ',' || value == '"' || (!allow_slash && value == '/'))
        {
            return false;
        }
    }
    return true;
}

static bool valid_header_name(const char *text, size_t capacity)
{
    static const char separators[] = "()<>@,;:\\\"/[]?={} \t";
    const size_t length = strnlen(text, capacity);
    if (length == 0U || length >= capacity)
    {
        return false;
    }
    for (size_t index = 0U; index < length; ++index)
    {
        const unsigned char value = (unsigned char)text[index];
        if (value <= 0x20U || value >= 0x7fU || strchr(separators, value) != NULL)
        {
            return false;
        }
    }
    return true;
}

static bool header_name_is_reserved(const char *name)
{
    return strcasecmp(name, "Content-Type") == 0 ||
           strcasecmp(name, "X-Device-Token") == 0;
}

static esp_err_t encode_protocol_command(https_command_t command_kind,
                                         const char *host,
                                         uint16_t port,
                                         int httpid,
                                         uint16_t length,
                                         char *command,
                                         size_t command_capacity)
{
    if (command == NULL || command_capacity == 0U ||
        (command_kind == HTTPS_COMMAND_CREATE &&
         (host == NULL || host[0] == '\0')) ||
        (command_kind != HTTPS_COMMAND_CREATE && httpid < 0))
    {
        return ESP_ERR_INVALID_ARG;
    }
    int written = -1;
    switch (command_kind)
    {
    case HTTPS_COMMAND_CREATE:
        written = snprintf(command,
                           command_capacity,
                           "AT+MHTTPCREATE=\"https://%s:%u\"",
                           host,
                           (unsigned)port);
        break;
    case HTTPS_COMMAND_TIMEOUT:
        written = snprintf(command,
                           command_capacity,
                           "AT+MHTTPCFG=\"timeout\",%d,10,20,10",
                           httpid);
        break;
    case HTTPS_COMMAND_SSL:
        written = snprintf(command,
                           command_capacity,
                           "AT+MHTTPCFG=\"ssl\",%d,%d,%d",
                           httpid,
                           ML307R_HTTPS_SSL_ENABLE,
                           ML307R_HTTPS_SSL_CONTEXT);
        break;
    case HTTPS_COMMAND_CONTENT:
        written = snprintf(command,
                           command_capacity,
                           "AT+MHTTPCONTENT=%d,%d,%u",
                           httpid,
                           ML307R_HTTPS_CONTENT_EOF,
                           (unsigned)length);
        break;
    case HTTPS_COMMAND_REQUEST:
        written = snprintf(command,
                           command_capacity,
                           "AT+MHTTPREQUEST=%d,%d,%u",
                           httpid,
                           ML307R_HTTPS_METHOD_POST,
                           (unsigned)length);
        break;
    case HTTPS_COMMAND_READ_HEADER:
    case HTTPS_COMMAND_READ_CONTENT:
        written = snprintf(
            command,
            command_capacity,
            "AT+MHTTPREAD=%d,%d,%u",
            httpid,
            command_kind == HTTPS_COMMAND_READ_HEADER
                ? ML307R_HTTPS_READ_HEADER
                : ML307R_HTTPS_READ_CONTENT,
            (unsigned)length);
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }
    return written > 0 && (size_t)written < command_capacity
               ? ESP_OK
               : ESP_ERR_INVALID_SIZE;
}

static bool parse_https_url(const char *url,
                            char host[ML307R_HTTPS_HOST_CAPACITY],
                            uint16_t *port,
                            char base_path[ML307R_HTTPS_BASE_PATH_CAPACITY])
{
    static const char scheme[] = "https://";
    if (strncmp(url, scheme, sizeof(scheme) - 1U) != 0)
    {
        return false;
    }
    const char *authority = url + sizeof(scheme) - 1U;
    const char *path = strchr(authority, '/');
    const char *authority_end = path == NULL ? authority + strlen(authority) : path;
    if (authority == authority_end || memchr(authority, '@', (size_t)(authority_end - authority)))
    {
        return false;
    }
    const char *colon = memchr(authority, ':', (size_t)(authority_end - authority));
    const char *host_end = colon == NULL ? authority_end : colon;
    const size_t host_length = (size_t)(host_end - authority);
    if (host_length == 0U || host_length >= ML307R_HTTPS_HOST_CAPACITY)
    {
        return false;
    }
    for (size_t index = 0; index < host_length; ++index)
    {
        const char value = authority[index];
        if (!((value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
              (value >= '0' && value <= '9') || value == '.' || value == '-'))
        {
            return false;
        }
    }
    memcpy(host, authority, host_length);
    host[host_length] = '\0';
    *port = 443U;
    if (colon != NULL)
    {
        char port_text[6] = {0};
        const size_t port_length = (size_t)(authority_end - colon - 1);
        if (port_length == 0U || port_length >= sizeof(port_text))
        {
            return false;
        }
        memcpy(port_text, colon + 1, port_length);
        const long parsed = strtol(port_text, NULL, 10);
        if (parsed <= 0L || parsed > 65535L)
        {
            return false;
        }
        *port = (uint16_t)parsed;
    }
    const char *base = path == NULL ? "/" : path;
    const size_t base_length = strnlen(base, ML307R_HTTPS_BASE_PATH_CAPACITY);
    if (base_length == 0U || base_length >= ML307R_HTTPS_BASE_PATH_CAPACITY)
    {
        return false;
    }
    memcpy(base_path, base, base_length + 1U);
    return true;
}

static bool join_endpoint_path(const char *base_path,
                               const char *path,
                               char endpoint[ML307R_HTTPS_PATH_CAPACITY])
{
    const bool root = strcmp(base_path, "/") == 0;
    size_t base_length = strlen(base_path);
    while (base_length > 1U && base_path[base_length - 1U] == '/')
    {
        --base_length;
    }
    const int written = root
                            ? snprintf(endpoint, ML307R_HTTPS_PATH_CAPACITY,
                                       "%s", path)
                            : snprintf(endpoint, ML307R_HTTPS_PATH_CAPACITY,
                                       "%.*s%s", (int)base_length, base_path, path);
    return written > 0 && written < (int)ML307R_HTTPS_PATH_CAPACITY &&
           valid_text(endpoint, ML307R_HTTPS_PATH_CAPACITY, true);
}

static bool current_header(const char **name, const char **value)
{
    if (name == NULL || value == NULL)
    {
        return false;
    }
    if (s_transport.header_index == 0U)
    {
        *name = "Content-Type";
        *value = "application/json";
    }
    else if (s_transport.header_index == 1U)
    {
        *name = "X-Device-Token";
        *value = s_transport.credentials.token;
    }
    else
    {
        const size_t request_index =
            (size_t)s_transport.header_index - ML307R_HTTPS_FIXED_HEADER_COUNT;
        if (request_index >= s_transport.request.header_count)
        {
            return false;
        }
        *name = s_transport.request.headers[request_index].name;
        *value = s_transport.request.headers[request_index].value;
    }
    return true;
}

static esp_err_t encode_header(const char *name,
                               const char *value,
                               int httpid,
                               bool has_more,
                               char *command,
                               size_t command_capacity,
                               uint8_t *data,
                               size_t data_capacity,
                               uint16_t *data_length)
{
    if (name == NULL || value == NULL || httpid < 0 ||
        command == NULL || command_capacity == 0U ||
        data == NULL || data_capacity == 0U || data_length == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!valid_header_name(name, ML307R_HTTPS_HEADER_NAME_CAPACITY) ||
        !valid_text(value, ML307R_HTTPS_HEADER_VALUE_CAPACITY, true))
    {
        return ESP_ERR_INVALID_ARG;
    }
    const int written = snprintf((char *)data,
                                 data_capacity,
                                 "%s: %s",
                                 name,
                                 value);
    if (written <= 0 || (size_t)written >= data_capacity ||
        (size_t)written > UINT16_MAX)
    {
        secure_clear(data, data_capacity);
        return ESP_ERR_INVALID_SIZE;
    }
    const int command_length = snprintf(command,
                                        command_capacity,
                                        "AT+MHTTPHEADER=%d,%u,%u",
                                        httpid,
                                        has_more ? 1U : 0U,
                                        (unsigned)written);
    if (command_length <= 0 || (size_t)command_length >= command_capacity)
    {
        secure_clear(data, data_capacity);
        return ESP_ERR_INVALID_SIZE;
    }
    *data_length = (uint16_t)written;
    return ESP_OK;
}

static uint32_t next_at_request_id(void)
{
    do
    {
        ++s_next_transport_at_id;
    } while (s_next_transport_at_id == 0U);
    s_transport.next_at_request_id = s_next_transport_at_id;
    return s_next_transport_at_id;
}

static esp_err_t submit_stage_command(const char *command,
                                      const char *prefix,
                                      const void *data,
                                      size_t data_length,
                                      uint16_t raw_response_length,
                                      bool sensitive,
                                      TickType_t now_ticks)
{
    legbot_at_transaction_t transaction = {
        .request_id = next_at_request_id(),
        .phase = data_length == 0U ? AT_CORE_PHASE_COMMAND : AT_CORE_PHASE_DATA_PROMPT,
        .deadline_ticks = command_deadline(now_ticks),
        .data_length = (uint16_t)data_length,
        .raw_response_length = raw_response_length,
        .raw_response_inline_delimiters =
            raw_response_length == 0U
                ? 0U
                : ML307R_HTTPS_READ_INLINE_DELIMITERS,
        .sensitive = sensitive,
    };
    size_t command_length = 0U;
    while (command_length < sizeof(transaction.command) &&
           command[command_length] != '\0')
    {
        ++command_length;
    }
    size_t prefix_length = 0U;
    while (prefix_length < sizeof(transaction.expected_prefix) &&
           prefix[prefix_length] != '\0')
    {
        ++prefix_length;
    }
    if (command_length == 0U || command_length >= sizeof(transaction.command) ||
        prefix_length >= sizeof(transaction.expected_prefix) ||
        data_length > sizeof(transaction.data))
    {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(transaction.command, command, command_length + 1U);
    memcpy(transaction.expected_prefix, prefix, prefix_length + 1U);
    if (data_length != 0U)
    {
        memcpy(transaction.data, data, data_length);
    }
    esp_err_t err = at_core_submit(&transaction, 0);
    if (err == ESP_OK)
    {
        s_transport.active_at_request_id = transaction.request_id;
        s_transport.waiting_at = true;
    }
    secure_clear(&transaction, sizeof(transaction));
    return err;
}

static esp_err_t queue_current_stage(TickType_t now_ticks)
{
    char command[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U] = {0};
    const void *data = NULL;
    size_t data_length = 0U;
    const char *prefix = "";
    bool sensitive = false;
    switch (s_transport.stage)
    {
    case HTTPS_STAGE_CERT_WRITE:
        if (s_transport.credentials.ca_pem[0] == '\0')
        {
            s_transport.stage = s_transport.credentials.ca_ref[0] == '\0'
                                    ? HTTPS_STAGE_SSL_AUTH
                                    : HTTPS_STAGE_SSL_CERT;
            return ESP_OK;
        }
        (void)snprintf(command, sizeof(command), "AT+MSSLCERTWR=\"legbot_ca\",%u",
                       (unsigned)strlen(s_transport.credentials.ca_pem));
        data = s_transport.credentials.ca_pem;
        data_length = strlen(s_transport.credentials.ca_pem);
        sensitive = true;
        break;
    case HTTPS_STAGE_SSL_CERT:
        if (s_transport.credentials.ca_pem[0] == '\0' &&
            s_transport.credentials.ca_ref[0] == '\0')
        {
            s_transport.stage = HTTPS_STAGE_SSL_AUTH;
            return ESP_OK;
        }
        (void)snprintf(command, sizeof(command), "AT+MSSLCFG=\"cert\",%d,\"%s\"",
                       ML307R_HTTPS_SSL_CONTEXT,
                       s_transport.credentials.ca_pem[0] == '\0'
                           ? s_transport.credentials.ca_ref
                           : "legbot_ca");
        sensitive = true;
        break;
    case HTTPS_STAGE_SSL_AUTH:
        (void)snprintf(
            command,
            sizeof(command),
            "AT+MSSLCFG=\"auth\",%d,%d",
            ML307R_HTTPS_SSL_CONTEXT,
            s_transport.credentials.ca_pem[0] == '\0' &&
                    s_transport.credentials.ca_ref[0] == '\0'
                ? ML307R_HTTPS_AUTH_NONE
                : ML307R_HTTPS_AUTH_SERVER);
        break;
    case HTTPS_STAGE_CREATE:
        (void)encode_protocol_command(
            HTTPS_COMMAND_CREATE,
            s_transport.host,
            s_transport.port,
            s_transport.httpid,
            0U,
            command,
            sizeof(command));
        prefix = "+MHTTPCREATE:";
        break;
    case HTTPS_STAGE_CFG_CACHED:
        (void)snprintf(command, sizeof(command), "AT+MHTTPCFG=\"cached\",%d,1", s_transport.httpid);
        break;
    case HTTPS_STAGE_CFG_TIMEOUT:
        (void)encode_protocol_command(
            HTTPS_COMMAND_TIMEOUT,
            NULL,
            0U,
            s_transport.httpid,
            0U,
            command,
            sizeof(command));
        break;
    case HTTPS_STAGE_CFG_SSL:
        (void)encode_protocol_command(
            HTTPS_COMMAND_SSL,
            NULL,
            0U,
            s_transport.httpid,
            0U,
            command,
            sizeof(command));
        break;
    case HTTPS_STAGE_HEADER:
    {
        const char *name = NULL;
        const char *value = NULL;
        if (!current_header(&name, &value))
        {
            secure_clear(command, sizeof(command));
            fail_attempt(ML307R_HTTPS_ERROR_INVALID_REQUEST, now_ticks);
            return ESP_ERR_INVALID_SIZE;
        }
        const uint8_t header_count =
            (uint8_t)(ML307R_HTTPS_FIXED_HEADER_COUNT +
                      s_transport.request.header_count);
        const unsigned has_more =
            (unsigned)(s_transport.header_index + 1U < header_count);
        secure_clear(s_transport.header_staging, sizeof(s_transport.header_staging));
        esp_err_t header_error = encode_header(
            name,
            value,
            s_transport.httpid,
            has_more != 0U,
            command,
            sizeof(command),
            s_transport.header_staging,
            sizeof(s_transport.header_staging),
            &s_transport.header_staging_length);
        if (header_error != ESP_OK)
        {
            secure_clear(command, sizeof(command));
            fail_attempt(ML307R_HTTPS_ERROR_INVALID_REQUEST, now_ticks);
            return header_error;
        }
        data = s_transport.header_staging;
        data_length = s_transport.header_staging_length;
        sensitive = true;
        break;
    }
    case HTTPS_STAGE_CONTENT:
        (void)encode_protocol_command(
            HTTPS_COMMAND_CONTENT,
            NULL,
            0U,
            s_transport.httpid,
            s_transport.request.body_length,
            command,
            sizeof(command));
        data = s_transport.request.body;
        data_length = s_transport.request.body_length;
        sensitive = true;
        break;
    case HTTPS_STAGE_REQUEST:
        (void)encode_protocol_command(
            HTTPS_COMMAND_REQUEST,
            NULL,
            0U,
            s_transport.httpid,
            (uint16_t)strlen(s_transport.endpoint),
            command,
            sizeof(command));
        data = s_transport.endpoint;
        data_length = strlen(s_transport.endpoint);
        break;
    case HTTPS_STAGE_READ:
    {
        const bool reading_header =
            s_transport.read_offset < s_transport.response_header_length;
        const uint16_t section_end = reading_header
                                         ? s_transport.response_header_length
                                         : s_transport.response_capacity;
        s_transport.read_chunk_length =
            (uint16_t)(section_end - s_transport.read_offset);
        if (s_transport.read_chunk_length > LEGBOT_AT_RESPONSE_MAX_LENGTH)
        {
            s_transport.read_chunk_length = LEGBOT_AT_RESPONSE_MAX_LENGTH;
        }
        (void)encode_protocol_command(
            reading_header
                ? HTTPS_COMMAND_READ_HEADER
                : HTTPS_COMMAND_READ_CONTENT,
            NULL,
            0U,
            s_transport.httpid,
            s_transport.read_chunk_length,
            command,
            sizeof(command));
        prefix = "+MHTTPREAD:";
        break;
    }
    case HTTPS_STAGE_TERM:
        (void)snprintf(command, sizeof(command), "AT+MHTTPTERM=%d", s_transport.httpid);
        break;
    case HTTPS_STAGE_DELETE:
        (void)snprintf(command, sizeof(command), "AT+MHTTPDEL=%d", s_transport.httpid);
        break;
    default:
        return ESP_OK;
    }
    esp_err_t err = submit_stage_command(command, prefix, data, data_length,
                                         s_transport.stage == HTTPS_STAGE_READ
                                             ? s_transport.read_chunk_length
                                             : 0U,
                                         sensitive, now_ticks);
    secure_clear(command, sizeof(command));
    if (err != ESP_OK)
    {
        if (s_transport.cleanup_for_failure && s_transport.stage == HTTPS_STAGE_TERM)
        {
            s_transport.stage = HTTPS_STAGE_DELETE;
        }
        else if (s_transport.cleanup_for_failure && s_transport.stage == HTTPS_STAGE_DELETE)
        {
            schedule_retry_or_finish(now_ticks);
        }
        else if (s_transport.stage == HTTPS_STAGE_DELETE &&
                 s_transport.response_complete)
        {
            ESP_LOGW(TAG, "HTTP 响应已完成但实例删除未入队，不重放 POST");
            finish(s_transport.pending_error, error_code(s_transport.pending_error));
        }
        else
        {
            fail_attempt(ML307R_HTTPS_ERROR_QUEUE, now_ticks);
        }
    }
    return err;
}

static void begin_attempt(TickType_t now_ticks)
{
    s_transport.attempt_deadline_ticks = earlier_deadline(
        s_transport.group_deadline_ticks,
        now_ticks + pdMS_TO_TICKS(ML307R_HTTPS_ATTEMPT_TIMEOUT_MS));
    s_transport.connect_deadline_ticks = earlier_deadline(
        s_transport.attempt_deadline_ticks,
        now_ticks + pdMS_TO_TICKS(ML307R_HTTPS_CONNECT_TIMEOUT_MS));
    s_transport.cleanup_deadline_ticks = 0U;
    s_transport.httpid = -1;
    s_transport.response_header_length = 0U;
    s_transport.response_length = 0U;
    s_transport.response_capacity = 0U;
    s_transport.read_offset = 0U;
    s_transport.read_chunk_length = 0U;
    s_transport.header_index = 0U;
    s_transport.header_staging_length = 0U;
    secure_clear(s_transport.header_staging, sizeof(s_transport.header_staging));
    s_transport.pending_error = ML307R_HTTPS_ERROR_NONE;
    s_transport.response_complete = false;
    s_transport.pending_http_urc_valid = false;
    s_transport.pending_http_received = false;
    s_transport.pending_http_status = 0;
    s_transport.pending_http_header_length = 0;
    s_transport.pending_http_content_length = 0;
    secure_clear(s_transport.result.header, sizeof(s_transport.result.header));
    secure_clear(s_transport.result.body, sizeof(s_transport.result.body));
    s_transport.result.header_length = 0U;
    s_transport.result.body_length = 0U;
    s_transport.result.http_status = 0U;
    s_transport.stage = HTTPS_STAGE_CERT_WRITE;
    s_transport.waiting_at = false;
    s_transport.cleanup_for_failure = false;
}

static void advance_after_ok(const legbot_at_result_t *result, TickType_t now_ticks)
{
    if (s_transport.stage == HTTPS_STAGE_CREATE)
    {
        const char *prefix = strstr(result->response, "+MHTTPCREATE:");
        if (prefix == NULL || sscanf(prefix, "+MHTTPCREATE: %d", &s_transport.httpid) != 1 ||
            s_transport.httpid < 0)
        {
            fail_attempt(ML307R_HTTPS_ERROR_HTTP, now_ticks);
            return;
        }
    }
    else if (s_transport.stage == HTTPS_STAGE_HEADER)
    {
        ++s_transport.header_index;
        const uint8_t header_count =
            (uint8_t)(ML307R_HTTPS_FIXED_HEADER_COUNT +
                      s_transport.request.header_count);
        if (s_transport.header_index >= header_count)
        {
            s_transport.stage = HTTPS_STAGE_CONTENT;
        }
        return;
    }
    else if (s_transport.stage == HTTPS_STAGE_REQUEST)
    {
        s_transport.awaited_generation = s_transport.generation;
        s_transport.stage = HTTPS_STAGE_WAIT_URC;
        if (s_transport.pending_http_urc_valid)
        {
            const bool received = s_transport.pending_http_received;
            const int status = s_transport.pending_http_status;
            const int header_length = s_transport.pending_http_header_length;
            const int content_length = s_transport.pending_http_content_length;
            s_transport.pending_http_urc_valid = false;
            apply_http_urc(received, status, header_length, content_length, now_ticks);
        }
        return;
    }
    else if (s_transport.stage == HTTPS_STAGE_READ)
    {
        if (result->response_length != s_transport.read_chunk_length ||
            !copy_read_chunk((const uint8_t *)result->response,
                             result->response_length))
        {
            fail_attempt(ML307R_HTTPS_ERROR_HTTP, now_ticks);
            return;
        }
        s_transport.read_offset = (uint16_t)(s_transport.read_offset +
                                             result->response_length);
        if (s_transport.read_offset < s_transport.response_capacity)
        {
            return;
        }
        s_transport.result.header_length = s_transport.response_header_length;
        s_transport.result.body_length = s_transport.response_length;
        s_transport.response_complete = true;
        s_transport.pending_error =
            s_transport.result.http_status >= 200U && s_transport.result.http_status < 300U
                ? ML307R_HTTPS_ERROR_NONE
                : ML307R_HTTPS_ERROR_HTTP;
        s_transport.cleanup_for_failure =
            s_transport.pending_error == ML307R_HTTPS_ERROR_HTTP &&
            http_status_is_retryable(s_transport.result.http_status);
        s_transport.stage = HTTPS_STAGE_DELETE;
        return;
    }
    else if (s_transport.stage == HTTPS_STAGE_TERM)
    {
        s_transport.stage = HTTPS_STAGE_DELETE;
        return;
    }
    else if (s_transport.stage == HTTPS_STAGE_DELETE)
    {
        if (s_transport.cleanup_for_failure)
        {
            schedule_retry_or_finish(now_ticks);
        }
        else
        {
            finish(s_transport.pending_error, error_code(s_transport.pending_error));
        }
        return;
    }
    s_transport.stage = (https_stage_t)(s_transport.stage + 1);
}

static void fail_attempt(ml307r_https_error_t error, TickType_t now_ticks)
{
    if (!s_transport.active || s_transport.cleanup_for_failure)
    {
        return;
    }
    s_transport.pending_error = error;
    s_transport.cleanup_for_failure = true;
    s_transport.cleanup_deadline_ticks = new_cleanup_deadline(now_ticks);
    ESP_LOGW(TAG,
             "HTTPS attempt 失败并开始清理，请求=%lu，阶段=%s，错误=%s，httpid=%d",
             (unsigned long)s_transport.request.request_id,
             stage_name(s_transport.stage),
             error_code(error),
             s_transport.httpid);
    if (s_transport.waiting_at)
    {
        (void)at_core_cancel(s_transport.active_at_request_id);
    }
    s_transport.waiting_at = false;
    s_transport.awaited_generation = 0U;
    if (s_transport.httpid >= 0)
    {
        s_transport.stage = error == ML307R_HTTPS_ERROR_ATTEMPT_TIMEOUT ||
                                    error == ML307R_HTTPS_ERROR_CONNECT_TIMEOUT ||
                                    error == ML307R_HTTPS_ERROR_CANCELLED ||
                                    error == ML307R_HTTPS_ERROR_AT_UNRESPONSIVE
                                ? HTTPS_STAGE_TERM
                                : HTTPS_STAGE_DELETE;
    }
    else
    {
        schedule_retry_or_finish(now_ticks);
    }
}

static void schedule_retry_or_finish(TickType_t now_ticks)
{
    const bool http_retryable =
        s_transport.response_complete &&
        s_transport.pending_error == ML307R_HTTPS_ERROR_HTTP &&
        http_status_is_retryable(s_transport.result.http_status);
    const bool transport_retryable =
        !s_transport.response_complete &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_NONE &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_BUSY &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_INVALID_REQUEST &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_CONFIG &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_URL &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_CANCELLED &&
        s_transport.pending_error != ML307R_HTTPS_ERROR_MODEM_RESTARTED;
    const bool retryable = http_retryable || transport_retryable;
    if (retryable &&
        s_transport.retry_count < s_transport.request.max_retries &&
        !tick_reached(now_ticks, s_transport.group_deadline_ticks))
    {
        const uint32_t delay_ms = s_transport.retry_count == 0U
                                      ? CLOUD_REQUEST_RETRY_FIRST_MS
                                      : CLOUD_REQUEST_RETRY_SECOND_MS;
        ++s_transport.retry_count;
        s_transport.retry_at_ticks = now_ticks + pdMS_TO_TICKS(delay_ms);
        s_transport.stage = HTTPS_STAGE_RETRY_WAIT;
        s_transport.cleanup_for_failure = false;
        s_transport.cleanup_deadline_ticks = 0U;
        s_transport.httpid = -1;
        return;
    }
    finish(s_transport.pending_error, error_code(s_transport.pending_error));
}

static bool http_status_is_retryable(uint16_t status)
{
    return status == 408U || status == 429U ||
           (status >= 500U && status <= 599U);
}

static void finish(ml307r_https_error_t error, const char *code)
{
    const uint32_t request_id = s_transport.request.request_id;
    const uint32_t generation = s_transport.generation;
    if (!s_transport.response_complete)
    {
        secure_clear(s_transport.result.header, sizeof(s_transport.result.header));
        secure_clear(s_transport.result.body, sizeof(s_transport.result.body));
        s_transport.result.header_length = 0U;
        s_transport.result.body_length = 0U;
        s_transport.result.http_status = 0U;
    }
    secure_clear(&s_transport.credentials, sizeof(s_transport.credentials));
    secure_clear(s_transport.header_staging, sizeof(s_transport.header_staging));
    secure_clear(&s_transport.request, sizeof(s_transport.request));
    s_transport.result.request_id = request_id;
    s_transport.result.generation = generation;
    s_transport.result.error = error;
    memset(s_transport.result.error_code, 0, sizeof(s_transport.result.error_code));
    const size_t code_length = strnlen(code, sizeof(s_transport.result.error_code));
    if (code_length < sizeof(s_transport.result.error_code))
    {
        memcpy(s_transport.result.error_code, code, code_length + 1U);
    }
    s_transport.active = false;
    s_transport.result_ready = true;
    s_transport.waiting_at = false;
    s_transport.stage = HTTPS_STAGE_IDLE;
}

static bool copy_read_chunk(const uint8_t *data, uint16_t length)
{
    uint16_t source_offset = 0U;
    uint16_t target_offset = s_transport.read_offset;
    if ((uint32_t)target_offset + length > s_transport.response_capacity)
    {
        return false;
    }
    if (target_offset < s_transport.response_header_length)
    {
        uint16_t header_length = (uint16_t)(s_transport.response_header_length -
                                            target_offset);
        if (header_length > length)
        {
            header_length = length;
        }
        memcpy(&s_transport.result.header[target_offset], data, header_length);
        target_offset = (uint16_t)(target_offset + header_length);
        source_offset = header_length;
    }
    if (source_offset < length)
    {
        const uint16_t body_offset = (uint16_t)(target_offset -
                                                s_transport.response_header_length);
        const uint16_t body_length = (uint16_t)(length - source_offset);
        if ((uint32_t)body_offset + body_length > sizeof(s_transport.result.body))
        {
            return false;
        }
        memcpy(&s_transport.result.body[body_offset], &data[source_offset], body_length);
    }
    return true;
}

static void apply_http_urc(bool received,
                           int status,
                           int header_length,
                           int content_length,
                           TickType_t now_ticks)
{
    if (!received)
    {
        fail_attempt(ML307R_HTTPS_ERROR_HTTP, now_ticks);
        return;
    }
    if (status < 100 || status > 599 || header_length < 0 || content_length < 0 ||
        header_length > (int)ML307R_HTTPS_RESPONSE_HEADER_CAPACITY ||
        content_length > (int)ML307R_HTTPS_RESPONSE_BODY_CAPACITY ||
        header_length + content_length > UINT16_MAX)
    {
        fail_attempt(ML307R_HTTPS_ERROR_RESPONSE_TOO_LARGE, now_ticks);
        return;
    }
    s_transport.result.http_status = (uint16_t)status;
    s_transport.pending_error = status >= 200 && status < 300
                                    ? ML307R_HTTPS_ERROR_NONE
                                    : ML307R_HTTPS_ERROR_HTTP;
    s_transport.response_header_length = (uint16_t)header_length;
    s_transport.response_length = (uint16_t)content_length;
    s_transport.response_capacity = (uint16_t)(header_length + content_length);
    s_transport.read_offset = 0U;
    s_transport.response_complete = s_transport.response_capacity == 0U;
    s_transport.cleanup_for_failure =
        s_transport.response_complete &&
        s_transport.pending_error == ML307R_HTTPS_ERROR_HTTP &&
        http_status_is_retryable(s_transport.result.http_status);
    s_transport.stage = s_transport.response_complete
                            ? HTTPS_STAGE_DELETE
                            : HTTPS_STAGE_READ;
}

static const char *error_code(ml307r_https_error_t error)
{
    static const char *const codes[] = {
        [ML307R_HTTPS_ERROR_NONE] = "MODEM_HTTPS_OK",
        [ML307R_HTTPS_ERROR_BUSY] = "MODEM_HTTPS_BUSY",
        [ML307R_HTTPS_ERROR_INVALID_REQUEST] = "MODEM_HTTPS_INVALID",
        [ML307R_HTTPS_ERROR_CONFIG] = "CLOUD_CONFIG_INVALID",
        [ML307R_HTTPS_ERROR_URL] = "CLOUD_URL_INVALID",
        [ML307R_HTTPS_ERROR_QUEUE] = "MODEM_HTTPS_QUEUE",
        [ML307R_HTTPS_ERROR_CONNECT_TIMEOUT] = "MODEM_HTTPS_CONNECT_TIMEOUT",
        [ML307R_HTTPS_ERROR_ATTEMPT_TIMEOUT] = "MODEM_HTTPS_TIMEOUT",
        [ML307R_HTTPS_ERROR_TLS] = "CLOUD_TLS_FAILED",
        [ML307R_HTTPS_ERROR_HTTP] = "MODEM_HTTP_FAILED",
        [ML307R_HTTPS_ERROR_RESPONSE_TOO_LARGE] = "MODEM_RESPONSE_TOO_LARGE",
        [ML307R_HTTPS_ERROR_CANCELLED] = "MODEM_HTTPS_CANCELLED",
        [ML307R_HTTPS_ERROR_MODEM_RESTARTED] = "MODEM_RESTARTED",
        [ML307R_HTTPS_ERROR_AT_UNRESPONSIVE] = "MODEM_AT_UNRESPONSIVE",
    };
    return error <= ML307R_HTTPS_ERROR_AT_UNRESPONSIVE
               ? codes[error]
               : "MODEM_HTTPS_UNKNOWN";
}

static bool parse_http_urc(const char *line,
                           bool *received,
                           int *httpid,
                           int *status,
                           int *header_length,
                           int *content_length)
{
    if (strstr(line, "\"recv\"") != NULL)
    {
        *received = true;
        return sscanf(line,
                      "+MHTTPURC: \"recv\",%d,%d,%d,%d",
                      httpid, status, header_length, content_length) == 4;
    }
    if (strstr(line, "\"err\"") != NULL)
    {
        *received = false;
        *status = 0;
        *header_length = 0;
        *content_length = 0;
        return sscanf(line, "+MHTTPURC: \"err\",%d", httpid) == 1;
    }
    return false;
}
