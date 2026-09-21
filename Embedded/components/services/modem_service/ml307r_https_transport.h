/**
 * @file     ml307r_https_transport.h
 * @brief    ML307R 原生 HTTPS POST transport 接口
 * @details  定义异步、单 in-flight、有界请求响应与固定超时重试合同，不包含租赁或 telemetry 业务语义。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_ML307R_HTTPS_TRANSPORT_H
#define LEGBOT_ML307R_HTTPS_TRANSPORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "at_core.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/** 单次 HTTPS attempt 总预算。 */
#define ML307R_HTTPS_ATTEMPT_TIMEOUT_MS 20000U
/** 连接与 TLS 子阶段预算。 */
#define ML307R_HTTPS_CONNECT_TIMEOUT_MS 10000U
/** 失败后 TERM/DELETE 与 AT 重同步的独立清理预算。 */
#define ML307R_HTTPS_CLEANUP_TIMEOUT_MS 8000U
/** 无 CA 时关闭服务端身份认证。 */
#define ML307R_HTTPS_AUTH_NONE 0
/** 配置 CA 时启用服务端单向认证。 */
#define ML307R_HTTPS_AUTH_SERVER 1
/** endpoint path 容量，包含终止符。 */
#define ML307R_HTTPS_PATH_CAPACITY 256U
/** 单个 header 名称容量，包含终止符。 */
#define ML307R_HTTPS_HEADER_NAME_CAPACITY 40U
/** 单个 header 值容量，包含终止符。 */
#define ML307R_HTTPS_HEADER_VALUE_CAPACITY 160U
/** 自定义 header 上限。 */
#define ML307R_HTTPS_HEADER_COUNT 4U
/** 请求 body 最大长度。 */
#define ML307R_HTTPS_BODY_CAPACITY 2048U
/** 响应 header 最大长度。 */
#define ML307R_HTTPS_RESPONSE_HEADER_CAPACITY 1024U
/** 响应 body 最大长度。 */
#define ML307R_HTTPS_RESPONSE_BODY_CAPACITY 4096U
/** transport 稳定错误容量。 */
#define ML307R_HTTPS_ERROR_CODE_CAPACITY 40U

typedef enum
{
    ML307R_HTTPS_ERROR_NONE = 0,           /**< transport 成功。 */
    ML307R_HTTPS_ERROR_BUSY,               /**< 已有单 in-flight 请求。 */
    ML307R_HTTPS_ERROR_INVALID_REQUEST,    /**< path/header/body 非法。 */
    ML307R_HTTPS_ERROR_CONFIG,             /**< 云配置预检或凭据复制失败。 */
    ML307R_HTTPS_ERROR_URL,                /**< base URL 无法安全分解。 */
    ML307R_HTTPS_ERROR_QUEUE,              /**< AT 队列拒绝。 */
    ML307R_HTTPS_ERROR_CONNECT_TIMEOUT,    /**< 连接/TLS 超过 10 秒。 */
    ML307R_HTTPS_ERROR_ATTEMPT_TIMEOUT,    /**< attempt 超过 20 秒。 */
    ML307R_HTTPS_ERROR_TLS,                /**< CA、SNI 或服务端验证失败。 */
    ML307R_HTTPS_ERROR_HTTP,               /**< 原生 HTTP 实例或非 2xx 状态失败。 */
    ML307R_HTTPS_ERROR_RESPONSE_TOO_LARGE, /**< 模组报告的响应超过容量。 */
    ML307R_HTTPS_ERROR_CANCELLED,          /**< STOP 或调用方取消。 */
    ML307R_HTTPS_ERROR_MODEM_RESTARTED,    /**< MATREADY 作废当前 generation。 */
    ML307R_HTTPS_ERROR_AT_UNRESPONSIVE,    /**< 模组 AT 通道无响应或清理后不可用。 */
} ml307r_https_error_t;

typedef struct
{
    char name[ML307R_HTTPS_HEADER_NAME_CAPACITY];   /**< header 名称。 */
    char value[ML307R_HTTPS_HEADER_VALUE_CAPACITY]; /**< header 值。 */
} ml307r_https_header_t;

typedef struct
{
    uint32_t request_id;                                      /**< 调用方非零请求 ID。 */
    TickType_t deadline_ticks;                                /**< 整个逻辑请求组绝对截止时间。 */
    char path[ML307R_HTTPS_PATH_CAPACITY];                    /**< 相对 endpoint path。 */
    ml307r_https_header_t headers[ML307R_HTTPS_HEADER_COUNT]; /**< 可选额外 headers。 */
    uint8_t header_count;                                     /**< headers 有效数量。 */
    uint8_t max_retries;                                      /**< 本逻辑请求允许的内部重试次数，支付轮询固定为 0。 */
    uint8_t body[ML307R_HTTPS_BODY_CAPACITY];                 /**< POST body 按值载荷。 */
    uint16_t body_length;                                     /**< body 有效字节数。 */
} ml307r_https_post_request_t;

typedef struct
{
    uint32_t request_id;                                   /**< 对应逻辑请求。 */
    uint32_t generation;                                   /**< transport generation。 */
    uint16_t http_status;                                  /**< HTTP 状态码，transport 前失败为 0。 */
    ml307r_https_error_t error;                            /**< typed transport 终态。 */
    uint16_t header_length;                                /**< 响应 header 有效长度。 */
    uint16_t body_length;                                  /**< 响应 body 有效长度。 */
    uint8_t header[ML307R_HTTPS_RESPONSE_HEADER_CAPACITY]; /**< 显式长度 header。 */
    uint8_t body[ML307R_HTTPS_RESPONSE_BODY_CAPACITY];     /**< 显式长度 body。 */
    char error_code[ML307R_HTTPS_ERROR_CODE_CAPACITY];     /**< 稳定 MODEM_/CLOUD_ 错误。 */
} ml307r_https_response_t;

/**
 * @brief 在 modem 启动阶段预分配 transport 的 PSRAM 常驻状态
 * @return ESP_OK 已就绪，ESP_ERR_NO_MEM 表示外部工作区申请失败
 */
esp_err_t ml307r_https_transport_prepare(void);

/**
 * @brief 启动一个异步 HTTPS POST 请求
 * @param request 有界请求值
 * @param now_ticks 当前单调 tick
 * @return ESP_OK 已接受，其他值表示忙、参数或配置错误
 */
esp_err_t ml307r_https_transport_start(const ml307r_https_post_request_t *request,
                                       TickType_t now_ticks);

/**
 * @brief 推进 transport 状态机与绝对截止/退避
 * @param now_ticks 当前单调 tick
 * @return ESP_OK 正常推进，其他值表示 AT 队列暂时错误
 */
esp_err_t ml307r_https_transport_step(TickType_t now_ticks);

/**
 * @brief 向 transport 交付唯一 AT owner 的完成结果
 * @param result AT 完成结果
 * @param now_ticks 当前单调 tick
 * @return true 结果属于当前 transport，false 应由生命周期状态机处理
 */
bool ml307r_https_transport_handle_at_result(const legbot_at_result_t *result,
                                             TickType_t now_ticks);

/**
 * @brief 向 transport 交付独立 URC
 * @param urc AT core 已解析 URC
 * @param now_ticks 当前单调 tick
 * @return true URC 被 transport 消费，false 为其他生命周期 URC
 */
bool ml307r_https_transport_handle_urc(const legbot_at_urc_t *urc,
                                       TickType_t now_ticks);

/**
 * @brief 立即取消 active generation 并清敏感 staging
 * @param modem_restarted true 表示模组重启，false 表示 STOP/调用方取消
 */
void ml307r_https_transport_cancel(bool modem_restarted);

/**
 * @brief 仅在活动请求 ID 匹配时取消当前 HTTPS generation
 * @param request_id 调用方 transport 请求 ID
 * @return true 已取消匹配活动请求，false 当前无匹配请求
 */
bool ml307r_https_transport_cancel_request(uint32_t request_id);

/**
 * @brief 查询 transport 是否有 active 或退避中的请求
 * @return true 请求组尚未终结，false 空闲
 */
bool ml307r_https_transport_active(void);

/**
 * @brief 领取一次 typed 请求终态
 * @param response 输出响应
 * @return ESP_OK 已领取，ESP_ERR_NOT_FOUND 表示暂无终态
 */
esp_err_t ml307r_https_transport_take_response(ml307r_https_response_t *response);

#ifdef ML307R_HTTPS_TRANSPORT_TEST
typedef struct
{
    char create[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];       /**< 创建 HTTPS 实例命令。 */
    char timeout[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];      /**< timeout 配置命令。 */
    char ssl[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];          /**< SSL 使能与绑定命令。 */
    char content[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];      /**< POST body 输入命令。 */
    char request[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];      /**< POST path 请求命令。 */
    char read_header[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];  /**< 缓存响应头读取命令。 */
    char read_content[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U]; /**< 缓存响应正文读取命令。 */
    uint8_t read_inline_delimiters;                       /**< MHTTPREAD 数据前元数据逗号数。 */
} ml307r_https_test_protocol_commands_t;

/**
 * @brief 为合同测试编码一条 ML307R Header transaction
 * @param httpid ML307R HTTP 实例 ID
 * @param has_more true 表示后续仍有 Header
 * @param name Header 名称
 * @param value Header 值
 * @param transaction 输出的敏感 prompt transaction
 * @return ESP_OK 编码成功，其他值表示参数或容量错误
 */
esp_err_t ml307r_https_transport_test_encode_header(
    int httpid,
    bool has_more,
    const char *name,
    const char *value,
    legbot_at_transaction_t *transaction);

/**
 * @brief 判断额外 Header 名称是否允许由调用方提交
 * @param name Header 名称
 * @return true 名称合法且不与 transport 自动 Header 重复
 */
bool ml307r_https_transport_test_header_name_allowed(const char *name);

/**
 * @brief 计算测试场景下普通命令或失败清理命令的绝对截止
 * @param cleanup true 表示失败清理命令
 * @param now_ticks 当前单调 tick
 * @param attempt_deadline_ticks 已有 attempt 截止
 * @param cleanup_deadline_ticks 已有独立清理截止
 * @return 该 AT transaction 应使用的绝对截止
 */
TickType_t ml307r_https_transport_test_command_deadline(
    bool cleanup,
    TickType_t now_ticks,
    TickType_t attempt_deadline_ticks,
    TickType_t cleanup_deadline_ticks);

/**
 * @brief 计算不受业务请求截止截断的失败清理截止
 * @param now_ticks 进入失败清理的当前单调 tick
 * @return 独立 TERM/DELETE 清理截止
 */
TickType_t ml307r_https_transport_test_cleanup_deadline(
    TickType_t now_ticks);

/**
 * @brief 按 AT 结果和阶段分类 transport 错误
 * @param status AT 完成状态
 * @param rx_byte_count 当前命令收到的字节数
 * @param tls_stage true 表示失败发生在 SSL/HTTPS 配置阶段
 * @return transport typed 错误
 */
ml307r_https_error_t ml307r_https_transport_test_classify_at_failure(
    at_core_result_status_t status,
    uint32_t rx_byte_count,
    bool tls_stage);

/**
 * @brief 编码一组 ML307R HTTPS 协议合同命令
 * @param host 服务器域名
 * @param port HTTPS 端口
 * @param httpid HTTP 实例 ID
 * @param body_length POST body 长度
 * @param path_length endpoint path 长度
 * @param read_length 单次缓存读取长度
 * @param commands 输出命令组
 * @return ESP_OK 编码成功，其他值表示参数或容量错误
 */
esp_err_t ml307r_https_transport_test_encode_protocol_commands(
    const char *host,
    uint16_t port,
    int httpid,
    uint16_t body_length,
    uint16_t path_length,
    uint16_t read_length,
    ml307r_https_test_protocol_commands_t *commands);
#endif

#endif /* LEGBOT_ML307R_HTTPS_TRANSPORT_H */
