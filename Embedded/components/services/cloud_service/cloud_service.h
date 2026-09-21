/**
 * @file     cloud_service.h
 * @brief    租赁授权与 telemetry 云服务公共接口
 * @details  定义 cloud_task 的有界请求/结果、不可变 telemetry 投影、严格 JSON 合同与当前 boot 云时间接口。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_CLOUD_SERVICE_H
#define LEGBOT_CLOUD_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifndef CLOUD_SERVICE_CODEC_ONLY
#include "legbot_services.h"
#ifdef CLOUD_SERVICE_TEST
#include "config_service.h"
#include "ml307r_https_transport.h"
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef CLOUD_SERVICE_CODEC_ONLY
/** 云端服务在统一服务表中的固定 ID。 */
#define LEGBOT_CLOUD_SERVICE_ID LEGBOT_SERVICE_CLOUD
#endif
/** 租赁授权请求队列深度。 */
#define CLOUD_RENTAL_REQUEST_QUEUE_DEPTH 2U
/** 租赁授权结果队列深度。 */
#define CLOUD_RENTAL_RESULT_QUEUE_DEPTH 2U
/** 租赁请求 JSON 最大字节数。 */
#define CLOUD_RENTAL_BODY_CAPACITY 256U
/** Telemetry 请求 JSON 最大字节数。 */
#define CLOUD_TELEMETRY_BODY_CAPACITY 384U
/** Telemetry 单个云窗口等待 4G 联网的硬截止时间。 */
#define CLOUD_TELEMETRY_MODEM_WAIT_MS 65000U
/** Telemetry 仅保留最新快照的固定槽容量。 */
#define CLOUD_TELEMETRY_LATEST_SLOT_CAPACITY 1U
/** cloud owner 退出前最终重投 telemetry 终态的有界次数。 */
#define CLOUD_TELEMETRY_FINAL_PUBLISH_ATTEMPTS 4U
/** cloud_task 仅在 HTTPS、联网等待或待交付终态活动期间使用的短轮询周期。 */
#define CLOUD_OWNER_POLL_MS 20U
/** cloud_task 等待 GPS 让出 RF 或 latest-only 槽可提交时的活动退让周期。 */
#define CLOUD_OWNER_DEFERRED_WAIT_MS 250U
/** 支付请求等待 4G 完成显式联网的有界时间。 */
#define CLOUD_MODEM_WAKE_WAIT_MS 65000U
/** 本地 CLOUD_ 稳定码容量。 */
#define CLOUD_ERROR_CODE_CAPACITY WATCH_STATE_ERROR_CODE_CAPACITY
/** 用户可理解中文原因容量。 */
#define CLOUD_USER_REASON_CAPACITY 64U
/** cJSON double 可安全精确表达的最大整数毫秒。 */
#define CLOUD_SERVER_TIME_MAX_MS 9007199254740991LL
/** Telemetry 响应正文允许解析的最大显式长度。 */
#define CLOUD_TELEMETRY_RESPONSE_BODY_CAPACITY 512U
/** 云错误对象 code 的最大合同长度，不含字符串终止符。 */
#define CLOUD_REMOTE_ERROR_CODE_MAX_LENGTH 32U
/** 云响应 message 的最大合同长度，不含字符串终止符。 */
#define CLOUD_REMOTE_MESSAGE_MAX_LENGTH 128U

    typedef enum
    {
        CLOUD_TRANSPORT_OK = 0,      /**< modem transport 成功并提供 HTTP 终态。 */
        CLOUD_TRANSPORT_UNREACHABLE, /**< 网络、modem 或队列不可达。 */
        CLOUD_TRANSPORT_TLS,         /**< 证书、SNI 或 TLS 校验失败。 */
        CLOUD_TRANSPORT_TIMEOUT,     /**< 连接或单次请求总预算超时。 */
        CLOUD_TRANSPORT_CANCELLED,   /**< 服务停止或 modem 重启取消。 */
        CLOUD_TRANSPORT_INTERNAL,    /**< 本地请求、队列或响应容量错误。 */
    } cloud_transport_result_t;

    typedef enum
    {
        CLOUD_TELEMETRY_RESULT_NEVER_SENT = 0,   /**< 当前 boot 尚无 telemetry 终态。 */
        CLOUD_TELEMETRY_RESULT_ACCEPTED,         /**< 云合同严格接受本次 telemetry。 */
        CLOUD_TELEMETRY_RESULT_HTTP_ERROR,       /**< 已收到非 2xx HTTP 终态。 */
        CLOUD_TELEMETRY_RESULT_TRANSPORT_ERROR,  /**< HTTPS transport 未形成可用 HTTP 成功终态。 */
        CLOUD_TELEMETRY_RESULT_RESPONSE_INVALID, /**< 2xx 响应不满足 telemetry 合同。 */
    } cloud_telemetry_result_t;

    typedef struct
    {
        uint32_t request_id;                          /**< control_gate 当前启动周期非零请求 ID。 */
        char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY]; /**< 事务绑定规范 MAC。 */
        uint32_t link_generation;                     /**< 事务绑定 BLE 物理链路代次。 */
    } cloud_rental_request_t;

    typedef struct
    {
        char path[64];                            /**< 固定租赁 endpoint path。 */
        uint8_t body[CLOUD_RENTAL_BODY_CAPACITY]; /**< 无 token 的 JSON 正文。 */
        uint16_t body_length;                     /**< JSON 正文显式有效长度。 */
    } cloud_rental_http_request_t;

    typedef struct
    {
        char watch_id[WATCH_STATE_WATCH_ID_CAPACITY]; /**< 同一次产品快照中的规范手环身份。 */
        char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY]; /**< 已提交绑定的规范外骨骼身份。 */
        uint32_t binding_generation;                  /**< 捕获快照时的 BLE 物理链路代次，仅用于终态归属。 */
        int64_t timestamp_ms;                         /**< 当前 boot 已同步 UTC 毫秒，未同步为 0。 */
        double latitude;                              /**< 坐标有效时的 WGS-84 纬度。 */
        double longitude;                             /**< 坐标有效时的 WGS-84 经度。 */
        float steps;                                  /**< 外骨骼状态有效时的协议镜像步数。 */
        watch_gps_status_t gps_status;                /**< 四态 GPS 合同值。 */
        uint8_t exoskeleton_battery;                  /**< 外骨骼状态有效时的电量百分比。 */
        uint8_t watch_battery;                        /**< 当前有效手环电量百分比。 */
        bool exoskeleton_data_valid;                  /**< 步数与外骨骼电量是否同时可序列化。 */
        bool coordinates_valid;                       /**< 纬度与经度是否同时可序列化。 */
    } cloud_telemetry_snapshot_t;

    typedef struct
    {
        char path[64];                               /**< 固定 telemetry endpoint path。 */
        uint8_t body[CLOUD_TELEMETRY_BODY_CAPACITY]; /**< 无 token 的九字段 JSON 正文。 */
        uint16_t body_length;                        /**< JSON 正文显式有效长度。 */
    } cloud_telemetry_http_request_t;

    typedef struct
    {
        cloud_telemetry_result_t result;            /**< 固件本地 telemetry 终态分类。 */
        uint16_t http_status;                       /**< transport 前失败为 0。 */
        bool accepted;                              /**< 仅严格成功合同为 true。 */
        int64_t server_time_ms;                     /**< 仅严格成功合同携带服务器 UTC 毫秒。 */
        char error_code[CLOUD_ERROR_CODE_CAPACITY]; /**< 本地有界 CLOUD_ 稳定码。 */
    } cloud_telemetry_response_t;

    typedef struct
    {
        uint32_t request_id;                          /**< 对应 control_gate 请求 ID。 */
        char exoskeleton_mac[WATCH_BLE_MAC_CAPACITY]; /**< 对应规范 MAC。 */
        uint32_t link_generation;                     /**< 对应 BLE 链路代次。 */
        watch_rental_result_t result;                 /**< 固件本地有界租赁终态。 */
        uint16_t http_status;                         /**< transport 前失败为 0。 */
        bool can_unlock;                              /**< 仅 HTTP 2xx + paid 合同可为 true。 */
        int64_t server_time_ms;                       /**< 合法响应中的非负 UTC 毫秒。 */
        char error_code[CLOUD_ERROR_CODE_CAPACITY];   /**< CLOUD_ 稳定码。 */
        char user_reason[CLOUD_USER_REASON_CAPACITY]; /**< 本地中文可理解原因。 */
    } cloud_rental_result_t;

    /**
     * @brief 生成只含三个合同字段的租赁请求 JSON
     * @param request request id、绑定 MAC 与链路代次
     * @param watch_id 当前 config/watch_state 的规范手环 ID
     * @param timestamp_ms 当前 boot 已同步时的 UTC 毫秒，未同步为 0
     * @param http 固定 path 与显式长度正文输出
     * @return ESP_OK 成功，其他值表示字段或容量非法
     */
    esp_err_t cloud_service_build_rental_http(
        const cloud_rental_request_t *request,
        const char watch_id[WATCH_STATE_WATCH_ID_CAPACITY],
        int64_t timestamp_ms,
        cloud_rental_http_request_t *http);

    /**
     * @brief 从唯一一次 watch_state 快照捕获 telemetry 不可变投影
     * @param monotonic_ms 本次捕获共用的单调启动毫秒
     * @param snapshot 按值、有界 telemetry 投影输出
     * @return ESP_OK 成功，其他值表示快照、身份或必填电量无效
     */
    esp_err_t cloud_service_capture_telemetry(
        uint64_t monotonic_ms,
        cloud_telemetry_snapshot_t *snapshot);

    /**
     * @brief 把 typed GPS 四态映射为 telemetry 合同字符串
     * @param status typed GPS 状态
     * @return 固定合同字符串，非法枚举返回 NULL
     */
    const char *cloud_service_telemetry_gps_status_name(
        watch_gps_status_t status);

    /**
     * @brief 生成只含九个合同字段的 telemetry HTTP path 与 JSON 正文
     * @param snapshot 已捕获的不可变 telemetry 投影
     * @param body_capacity 调用方允许使用的正文容量，不得超过固定缓冲
     * @param http 固定 path 与显式长度正文输出
     * @return ESP_OK 成功，其他值表示投影、容量或格式化无效
     */
    esp_err_t cloud_service_build_telemetry_http(
        const cloud_telemetry_snapshot_t *snapshot,
        size_t body_capacity,
        cloud_telemetry_http_request_t *http);

    /**
     * @brief 校验 modem 终态是否属于当前唯一租赁请求
     * @param request 当前 cloud_task 活动请求
     * @param response_request_id modem 终态携带的请求 ID
     * @return true 可继续解析，false 表示迟到或不匹配终态
     */
    bool cloud_service_response_matches_request(
        const cloud_rental_request_t *request,
        uint32_t response_request_id);

    /**
     * @brief 严格解析租赁 HTTP 终态并映射本地原因
     * @param request 原始 typed 请求
     * @param http_status HTTP 状态，transport 前失败为 0
     * @param transport 本地 transport 终态
     * @param body 显式长度响应正文，可非 NUL 结尾
     * @param body_length 正文有效长度
     * @param result 完整有界本地终态输出
     * @return ESP_OK 已得到合同或错误终态，解析失败返回 ESP_ERR_INVALID_ARG
     */
    esp_err_t cloud_service_parse_rental_http(
        const cloud_rental_request_t *request,
        uint16_t http_status,
        cloud_transport_result_t transport,
        const uint8_t *body,
        size_t body_length,
        cloud_rental_result_t *result);

    /**
     * @brief 严格解析 telemetry HTTP 终态并映射本地稳定诊断
     * @param http_status HTTP 状态，transport 前失败为 0
     * @param transport 本地 transport 终态
     * @param body 显式长度响应正文，可非 NUL 结尾
     * @param body_length 正文有效长度
     * @param result 完整清零后写入的有界 telemetry 终态
     * @return ESP_OK 合同有效，ESP_ERR_INVALID_ARG 表示参数或响应合同无效
     */
    esp_err_t cloud_service_parse_telemetry_http(
        uint16_t http_status,
        cloud_transport_result_t transport,
        const uint8_t *body,
        size_t body_length,
        cloud_telemetry_response_t *result);

    /**
     * @brief 清除当前 boot 的云时间同步证据
     */
    void cloud_service_reset_boot_time(void);

    /**
     * @brief 使用合法 server_time_ms 建立当前 boot UTC 偏移
     * @param server_time_ms 云端 UTC 毫秒
     * @param monotonic_ms 当前 esp_timer 单调启动毫秒
     * @param offset_ms 计算得到的 i64 偏移输出
     * @return ESP_OK 成功，其他值表示时间越界或时基非法
     */
    esp_err_t cloud_service_note_server_time(int64_t server_time_ms,
                                             uint64_t monotonic_ms,
                                             int64_t *offset_ms);

    /**
     * @brief 获取当前 boot 的 UTC 毫秒
     * @param monotonic_ms 当前 esp_timer 单调启动毫秒
     * @return 已同步时返回 UTC 毫秒，未同步或溢出返回 0
     */
    int64_t cloud_service_timestamp_ms(uint64_t monotonic_ms);

#ifndef CLOUD_SERVICE_CODEC_ONLY
    /**
     * @brief 为固定 cloud_task 创建有界 typed 队列
     * @return ESP_OK 成功，其他值表示内存或状态错误
     */
    esp_err_t cloud_service_prepare(void);

    /**
     * @brief 取消尚未运行的 cloud service 准备状态
     */
    void cloud_service_cancel_prepared_run(void);

    /**
     * @brief 请求 cloud_task 有界停止并作废易失事务
     * @param timeout_ticks 保留统一停止接口的有界等待参数
     * @return ESP_OK 已锁存，ESP_ERR_INVALID_STATE 表示未准备
     */
    esp_err_t cloud_service_request_stop(TickType_t timeout_ticks);

    /**
     * @brief 向 cloud_task 投递单个租赁授权查询
     * @param request request id、规范 MAC 与链路代次
     * @param timeout_ticks 等待请求队列空间的有界 tick
     * @return ESP_OK 已入队，其他值表示参数、状态或队列满
     */
    esp_err_t cloud_service_rental_submit(const cloud_rental_request_t *request,
                                          TickType_t timeout_ticks);

    /**
     * @brief 取消指定支付会话并使其迟到结果失效
     * @param request_id 当前逻辑支付请求 ID
     * @return ESP_OK 已锁存取消，其他值表示参数或服务状态无效
     */
    esp_err_t cloud_service_rental_cancel(uint32_t request_id);

    /**
     * @brief 通知 cloud owner 某条 BLE 物理链路已经结束
     * @param link_generation 已结束的非零 BLE 物理链路代次
     * @return ESP_OK 已锁存，其他值表示参数或服务状态无效
     */
    esp_err_t cloud_service_authorized_link_end(uint32_t link_generation);

    /**
     * @brief 请求指定支付会话执行唯一一次立即检查加速
     * @param request_id 当前逻辑支付请求 ID
     * @param joined_in_flight 是否已幂等加入同一会话的在途真实查询
     * @return ESP_OK 已锁存加速或已加入在途查询，其他值表示参数或服务状态无效
     */
    esp_err_t cloud_service_rental_accelerate(uint32_t request_id,
                                              bool *joined_in_flight);

    /**
     * @brief 从 cloud_task 领取一个租赁查询终态
     * @param result 终态输出
     * @param timeout_ticks 有界等待 tick
     * @return ESP_OK 已领取，其他值表示参数、状态或超时
     */
    esp_err_t cloud_service_rental_receive(cloud_rental_result_t *result,
                                           TickType_t timeout_ticks);

    /**
     * @brief 运行固定 cloud_task 的异步 modem 编排循环
     * @return ESP_OK 正常停止，其他值表示 owner 初始化或队列错误
     */
    esp_err_t cloud_service_run(void);

#ifdef CLOUD_SERVICE_TEST
    /**
     * @brief 为 production-C owner 测试注入产品状态读取结果
     * @param snapshot 成功读取时返回的完整产品状态
     * @param result 模拟 watch_state_snapshot 的返回值
     */
    void cloud_service_test_set_watch_state(
        const watch_state_snapshot_t *snapshot,
        esp_err_t result);

    /**
     * @brief 为 production-C owner 测试注入云配置读取结果
     * @param snapshot 成功读取时返回的无凭据配置
     * @param result 模拟 config_service_snapshot 的返回值
     */
    void cloud_service_test_set_config(
        const config_service_snapshot_t *snapshot,
        esp_err_t result);

    /**
     * @brief 为 production-C owner 测试注入 modem 提交结果
     * @param result 模拟 modem_service_https_post_async 的返回值
     */
    void cloud_service_test_set_modem_submit_result(esp_err_t result);

    /**
     * @brief 注入支付请求触发 4G 显式联网的返回值
     * @param result modem 显式联网请求返回值
     */
    void cloud_service_test_set_modem_connect_result(esp_err_t result);

    /**
     * @brief 为 production-C owner 注入一次性 modem 终态
     * @param response result 为 ESP_OK 时的完整终态，其他情况可为 NULL
     * @param result 模拟 modem_service_https_receive 的返回值
     */
    void cloud_service_test_set_modem_terminal(
        const ml307r_https_response_t *response,
        esp_err_t result);

    /**
     * @brief 为 production-C 测试启动同一 cloud owner 状态机
     * @return ESP_OK 已启动，其他值表示未 prepare 或 owner 已运行
     */
    esp_err_t cloud_service_test_owner_begin(void);

    /**
     * @brief 用注入的单调毫秒推进一次 production cloud owner
     * @param monotonic_ms 测试注入的单调启动毫秒
     * @return ESP_OK 已有界推进，其他值表示 owner 状态非法
     */
    esp_err_t cloud_service_test_owner_step(uint64_t monotonic_ms);

    /**
     * @brief 结束 production-C 测试 owner 并清除易失事务
     */
    void cloud_service_test_owner_end(void);

    /**
     * @brief 读取 production-C 测试 owner 的单槽 telemetry 状态
     * @param snapshot 当前 latest 槽按值输出
     * @param valid latest 槽是否有效
     * @param pending_send latest 是否仍等待提交给 modem
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 表示输出指针非法
     */
    esp_err_t cloud_service_test_latest_telemetry(
        cloud_telemetry_snapshot_t *snapshot,
        bool *valid,
        bool *pending_send);

    /**
     * @brief 读取 production-C owner 的下一固定 telemetry 窗口
     * @return 下一窗口的单调毫秒
     */
    uint64_t cloud_service_test_next_window_ms(void);

    /**
     * @brief 读取 production-C 测试中实际提交给 modem 的次数
     * @return modem 提交调用累计次数
     */
    uint32_t cloud_service_test_modem_submit_count(void);

    /**
     * @brief 读取 production-C 测试中支付触发 4G 联网的次数
     * @return 显式联网请求累计次数
     */
    uint32_t cloud_service_test_modem_connect_count(void);

    /**
     * @brief 读取 production-C 测试中 RF lease 归还次数
     * @return modem lease release 累计次数
     */
    uint32_t cloud_service_test_modem_release_count(void);

    /**
     * @brief 查询 cloud owner 当前是否持有 telemetry RF lease
     * @return true 表示持有，false 表示已释放
     */
    bool cloud_service_test_telemetry_lease_held(void);

    /**
     * @brief 查询 cloud owner 当前是否持有支付 RF lease
     * @return true 表示持有，false 表示已释放
     */
    bool cloud_service_test_rental_lease_held(void);

    /**
     * @brief 读取 cloud owner 当前活动 transport 请求 ID
     * @return 非零活动请求 ID，无活动请求时为 0
     */
    uint32_t cloud_service_test_active_transport_request_id(void);

    /**
     * @brief 通过 production wrapper 为失败路径测试取得 telemetry lease
     * @return ESP_OK 已持有，其他值表示取得失败
     */
    esp_err_t cloud_service_test_acquire_telemetry_lease(void);

    /**
     * @brief 填满租赁结果队列以模拟结果交付背压
     * @return ESP_OK 已填满，其他值表示队列状态无效
     */
    esp_err_t cloud_service_test_fill_result_queue(void);

    /**
     * @brief 模拟租赁请求占用或释放唯一 modem 请求槽
     * @param active true 表示租赁占用，false 表示释放
     * @return ESP_OK 已切换，ESP_ERR_INVALID_STATE 表示存在其他活动请求
     */
    esp_err_t cloud_service_test_set_rental_active(bool active);

    /**
     * @brief 查询活动 telemetry 是否仍属于当前明确绑定
     * @return true 活动 telemetry 身份仍有效，false 表示无活动请求或身份已失效
     */
    bool cloud_service_test_active_telemetry_identity_valid(void);

    /**
     * @brief 读取最近一次实际路由到 telemetry handler 的完整终态
     * @param response transport 终态按值输出
     * @return ESP_OK 已观察到终态，ESP_ERR_NOT_FOUND 表示暂无终态
     */
    esp_err_t cloud_service_test_last_telemetry_terminal(
        ml307r_https_response_t *response);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CLOUD_SERVICE_H */
