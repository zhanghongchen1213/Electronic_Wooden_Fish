/**
 * @file     at_core.h
 * @brief    ML307R AT 单读者事务核心接口
 * @details  提供 UART2 初始化、有界事务队列、增量响应解析和独立 URC 分发，由固定 modem_task 唯一驱动。
 * @author   ZHC
 * @date     2026-07-13
 */

#ifndef LEGBOT_AT_CORE_H
#define LEGBOT_AT_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/uart.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** AT 命令最大长度，包含内容但不包含自动追加的 CRLF。 */
#define LEGBOT_AT_COMMAND_MAX_LENGTH 256U
/** AT 数据模式按值载荷最大长度。 */
#define LEGBOT_AT_DATA_MAX_LENGTH 2304U
/** AT 响应存储容量；文本响应预留字符串终止符，原始响应可占满。 */
#define LEGBOT_AT_RESPONSE_MAX_LENGTH 1024U
/** AT 预期响应前缀容量，包含字符串终止符。 */
#define LEGBOT_AT_EXPECTED_PREFIX_CAPACITY 32U
/** 单条 URC 文本容量，包含字符串终止符。 */
#define LEGBOT_AT_URC_MAX_LENGTH 192U
/** 单条解析行容量，包含字符串终止符。 */
#define LEGBOT_AT_LINE_MAX_LENGTH 256U

    typedef enum
    {
        AT_CORE_PHASE_COMMAND = 0, /**< 等待普通命令响应。 */
        AT_CORE_PHASE_DATA_PROMPT, /**< 等待大于号数据提示并发送按值载荷。 */
        AT_CORE_PHASE_FINAL,       /**< 数据已发送，等待最终响应。 */
    } at_core_transaction_phase_t;

    typedef enum
    {
        AT_CORE_RESULT_OK = 0,            /**< 收到 OK 终态。 */
        AT_CORE_RESULT_ERROR,             /**< 收到 ERROR 终态。 */
        AT_CORE_RESULT_CME_ERROR,         /**< 收到 +CME ERROR 终态。 */
        AT_CORE_RESULT_TIMEOUT,           /**< ESP32-S3 绝对截止时间已到。 */
        AT_CORE_RESULT_CANCELLED,         /**< owner 显式取消。 */
        AT_CORE_RESULT_UART_ERROR,        /**< UART 读取或写入失败。 */
        AT_CORE_RESULT_MODEM_RESTARTED,   /**< 收到 +MATREADY，旧事务代次作废。 */
        AT_CORE_RESULT_RESPONSE_OVERFLOW, /**< 响应超过有界容量。 */
        AT_CORE_RESULT_PROMPT,            /**< 内部已识别数据提示，不作为事务终态发布。 */
    } at_core_result_status_t;

    typedef struct
    {
        uint32_t request_id;                                      /**< 启动周期内非零请求 ID。 */
        at_core_transaction_phase_t phase;                        /**< 事务初始阶段。 */
        TickType_t deadline_ticks;                                /**< owner 业务单调 tick 绝对截止时间。 */
        TickType_t response_timeout_ticks;                        /**< UART 真正发送后的响应预算，0 仅使用业务截止。 */
        char command[LEGBOT_AT_COMMAND_MAX_LENGTH + 1U];          /**< 按值复制的命令，不含 CRLF。 */
        char expected_prefix[LEGBOT_AT_EXPECTED_PREFIX_CAPACITY]; /**< 可选匹配响应前缀。 */
        uint8_t data[LEGBOT_AT_DATA_MAX_LENGTH];                  /**< prompt 后发送的二进制安全按值数据。 */
        uint16_t data_length;                                     /**< data 中的有效字节数。 */
        uint16_t raw_response_length;                             /**< 匹配 expected_prefix 后按精确长度接收的原始响应字节数。 */
        uint8_t raw_response_inline_delimiters;                   /**< 非零时，在前缀行内跳过指定数量逗号后接收原始响应。 */
        bool sensitive;                                           /**< 完成后必须清零命令、数据及响应 staging。 */
        QueueHandle_t completion_queue;                           /**< 可选专用完成通道，NULL 使用公共结果队列。 */
    } legbot_at_transaction_t;

    typedef struct
    {
        uint32_t request_id;                          /**< 对应事务请求 ID。 */
        at_core_result_status_t status;               /**< 明确 AT 终态。 */
        char response[LEGBOT_AT_RESPONSE_MAX_LENGTH]; /**< 有界响应；raw 模式按显式长度解释且不保证 NUL 终止。 */
        uint16_t response_length;                     /**< response 有效字节数。 */
        uint32_t rx_byte_count;                       /**< 本事务活动期间收到的 UART 字节饱和计数。 */
    } legbot_at_result_t;

    typedef struct
    {
        uint32_t sequence;                   /**< URC 发布单调序号，达到上限后饱和。 */
        char line[LEGBOT_AT_URC_MAX_LENGTH]; /**< 已终止的有界 URC 行。 */
    } legbot_at_urc_t;

    typedef struct
    {
        uint32_t rx_byte_count;           /**< UART 接收字节饱和累计值，不包含响应正文。 */
        uint32_t tx_byte_count;           /**< UART 驱动已接受发送字节饱和累计值，包含命令结束符与数据载荷。 */
        uint32_t transaction_count;       /**< 已向 UART 驱动提交发送的 AT 事务饱和累计值。 */
        uint32_t timeout_count;           /**< AT 事务超时终态饱和累计值。 */
        uint32_t resync_count;            /**< 超时或取消后进入重同步的饱和累计值。 */
        uint32_t line_overflow_count;      /**< 超长行丢弃计数。 */
        uint32_t rx_error_count;           /**< UART 读取错误计数。 */
        uint32_t command_queue_full_count; /**< 命令队列满计数。 */
        uint32_t result_queue_full_count;  /**< 完成队列满计数。 */
        uint32_t urc_overflow_count;       /**< URC 队列满计数。 */
        TickType_t last_rx_ticks;          /**< 最近一次收到任意 UART 字节的单调 tick。 */
        TickType_t last_tx_ticks;          /**< 最近一次成功发送 UART 字节的单调 tick。 */
        char last_error[40];               /**< 最近 MODEM_ 稳定错误。 */
    } at_core_diagnostics_t;

    /**
     * @brief 初始化 UART2 与 AT 有界队列
     * @details 不创建长期任务；固定 modem_task 必须调用 at_core_owner_poll()。
     * @return ESP_OK 成功，其他值表示 UART 或内存初始化失败
     */
    esp_err_t at_core_init(void);

    /**
     * @brief 查询 AT 核心是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool at_core_is_initialized(void);

    /**
     * @brief 获取 AT 核心使用的 BSP UART 端口
     * @return BSP 定义的 ML307R UART2 端口
     */
    uart_port_t at_core_uart_port(void);

    /**
     * @brief 运行时切换 UART2 波特率
     * @details 禁止在 AT 事务进行中调用。会等待 TX 完成、flush RX 缓冲、清除重同步状态。
     * @param baud_rate 目标正整数波特率
     * @return ESP_OK 切换成功
     *         ESP_ERR_INVALID_STATE 核心未初始化或当前有活跃事务
     *         ESP_ERR_INVALID_ARG 波特率为 0
     *         ESP_FAIL UART 驱动切换失败
     */
    esp_err_t at_core_update_baud_rate(uint32_t baud_rate);

    /**
     * @brief 查询当前 UART2 运行时波特率
     * @return 当前波特率；未初始化时返回 0
     */
    uint32_t at_core_baud_rate(void);

    /**
     * @brief 按值提交一条 AT transaction
     * @param transaction 有界事务值
     * @param timeout_ticks 等待命令队列空间的有界 tick 数
     * @return ESP_OK 已入队，其他值表示参数、状态或队列错误
     */
    esp_err_t at_core_submit(const legbot_at_transaction_t *transaction,
                             TickType_t timeout_ticks);

    /**
     * @brief 由 modem_task 唯一调用，持续读取 UART 并推进事务
     * @param now_ticks 当前单调 tick，用于绝对截止判定
     * @param read_wait_ticks 本次 UART 读取最大等待 tick
     * @return ESP_OK 正常推进，其他值表示 UART 或 owner 状态错误
     */
    esp_err_t at_core_owner_poll(TickType_t now_ticks, TickType_t read_wait_ticks);

    /**
     * @brief 向增量解析器输入已读取字节
     * @details 仅供唯一 owner 与 production-C host harness 使用。
     * @param bytes 输入字节
     * @param length 输入长度
     * @param now_ticks 当前单调 tick
     * @return ESP_OK 已处理，其他值表示参数或状态错误
     */
    esp_err_t at_core_owner_feed(const uint8_t *bytes,
                                 size_t length,
                                 TickType_t now_ticks);

    /**
     * @brief 从公共完成队列接收 AT 结果
     * @param result 结果输出
     * @param timeout_ticks 有界等待 tick
     * @return ESP_OK 已收到，其他值表示参数、状态或超时
     */
    esp_err_t at_core_receive_result(legbot_at_result_t *result,
                                     TickType_t timeout_ticks);

    /**
     * @brief 从独立 URC 队列接收一条通知
     * @param urc URC 输出
     * @param timeout_ticks 有界等待 tick
     * @return ESP_OK 已收到，其他值表示参数、状态或超时
     */
    esp_err_t at_core_receive_urc(legbot_at_urc_t *urc,
                                  TickType_t timeout_ticks);

    /**
     * @brief 取消当前匹配事务
     * @param request_id 非零请求 ID
     * @return ESP_OK 已取消，ESP_ERR_NOT_FOUND 表示当前事务不匹配
     */
    esp_err_t at_core_cancel(uint32_t request_id);

    /**
     * @brief 作废当前事务并清空待执行命令
     * @param modem_restarted true 表示模组重启，false 表示 STOP/普通取消
     */
    void at_core_abort_all(bool modem_restarted);

    /**
     * @brief 读取 AT 饱和诊断快照
     * @param diagnostics 输出快照
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 参数无效
     */
    esp_err_t at_core_diagnostics_snapshot(at_core_diagnostics_t *diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_AT_CORE_H */
