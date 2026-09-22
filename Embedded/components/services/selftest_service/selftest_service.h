/**
 * @file     selftest_service.h
 * @brief    typed BSP 自检服务接口
 * @details  定义有界注册表、当前启用自检项目元数据、串行结果引擎和证据类别校验的公共契约。
 *           自检只覆盖电源、启动绑带、复位观察、板级资源与初始化阶段；设计输入和编译成功
 *           都不能升级为 hardware_verified。
 * @author   ZHC
 * @date     2026-07-14
 */

#ifndef LEGBOT_SELFTEST_SERVICE_H
#define LEGBOT_SELFTEST_SERVICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "legbot_services.h"

/** 自检服务在统一服务表中的固定 ID。 */
#define LEGBOT_SELFTEST_SERVICE_ID LEGBOT_SERVICE_SELFTEST
/** 当前启用项与后续 Story 扩展的有界注册容量。 */
#define SELFTEST_REGISTRY_CAPACITY 8U
/** PRD 已声明项目的最长允许超时。 */
#define SELFTEST_MAX_TIMEOUT_MS 180000U
/** selftest_task 私有 typed 命令队列深度。 */
#define SELFTEST_SERVICE_QUEUE_DEPTH 2U
/** selftest_task 空闲与取消轮询的有界片段。 */
#define SELFTEST_SERVICE_POLL_MS 10U
/**
 * 板级回执是否可用。Story 1.1 无样机，因此恒为 0；
 * 只有 Story 7.6 归档示波器/实板回执后，才允许把该常量改为 1。
 */
#define SELFTEST_SERVICE_BOARD_RECEIPT_AVAILABLE 0U

typedef enum
{
    SELFTEST_SERVICE_COMMAND_RUN_ALL = 0, /**< 按固定顺序执行完整清单。 */
    SELFTEST_SERVICE_COMMAND_STOP,        /**< 取消当前项并退出 selftest_task。 */
} selftest_service_command_type_t;

typedef struct
{
    selftest_service_command_type_t type; /**< typed 命令类型。 */
    uint32_t requested_at_ticks;          /**< 命令入队时的单调 tick。 */
} selftest_service_command_t;

typedef struct
{
    uint32_t stack_high_water_bytes; /**< selftest_task 历史最小剩余栈，IDF 口径为字节。 */
} selftest_service_metrics_t;

typedef struct
{
    selftest_item_id_t item_id; /**< 稳定项目 ID。 */
    const char *name;           /**< 编译期固定项目名称。 */
    uint32_t timeout_ms;        /**< PRD 固定超时。 */
    bool required_in_story;     /**< true 表示本 Story 必须实现，不可以未注册跳过。 */
} selftest_item_metadata_t;

typedef struct
{
    uint32_t run_id;          /**< 当前运行 ID。 */
    selftest_item_id_t item_id; /**< 当前项目 ID。 */
    uint32_t timeout_ms;      /**< 当前项目有效超时。 */
    void *handler_context;    /**< 注册时提供的长生命周期上下文。 */
} selftest_handler_context_t;

/**
 * @brief 单项 handler 类型
 * @details handler 必须在 context->timeout_ms 内自行返回，并在小步轮询中检查 selftest_service_cancel_requested()；引擎不会强制终止占用硬件资源的调用栈。
 * @param context 本次运行的 typed 上下文
 * @param result 由 handler 填充的定长结果
 * @return ESP_OK 已生成有效终态，其他错误将由引擎转为 fail
 */
typedef esp_err_t (*selftest_item_handler_t)(const selftest_handler_context_t *context,
                                             selftest_item_result_t *result);

/**
 * @brief 注册一个有界 selftest handler
 * @param item_id 稳定项目 ID
 * @param timeout_ms 非零且不超过 SELFTEST_MAX_TIMEOUT_MS 的超时
 * @param handler 单项 handler
 * @param handler_context 长生命周上下文，可为 NULL
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_ARG ID、超时或 handler 无效，或 ID 重复
 *         ESP_ERR_INVALID_STATE 自检正在运行
 *         ESP_ERR_NO_MEM 注册表已满
 */
esp_err_t selftest_service_register(selftest_item_id_t item_id,
                                    uint32_t timeout_ms,
                                    selftest_item_handler_t handler,
                                    void *handler_context);

/**
 * @brief 在未运行时清空注册表
 * @return ESP_OK 已清空，ESP_ERR_INVALID_STATE 自检正在运行
 */
esp_err_t selftest_service_registry_reset(void);

/**
 * @brief 获取当前启用自检项目的固定顺序与超时元数据
 * @param count 输出元数据数量，可为 NULL
 * @return 编译期固定元数据表
 */
const selftest_item_metadata_t *selftest_service_canonical_items(size_t *count);

/**
 * @brief 创建 selftest_task 私有 typed 命令队列
 * @return ESP_OK 成功或已创建，ESP_ERR_NO_MEM 队列创建失败
 */
esp_err_t selftest_service_init_contracts(void);

/**
 * @brief 在统一框架创建 selftest_task 前进入 prepared 状态
 * @return ESP_OK 已准备，ESP_ERR_INVALID_STATE 队列不可用或 task 已活动
 */
esp_err_t selftest_service_prepare_run(void);

/** @brief 回滚未成功创建的 selftest_task prepared 状态。 */
void selftest_service_cancel_prepared_run(void);

/**
 * @brief 删除未运行 selftest_task 的 typed 命令队列
 * @return ESP_OK 成功或未创建，ESP_ERR_INVALID_STATE task 正在活动
 */
esp_err_t selftest_service_deinit_contracts(void);

/**
 * @brief 请求执行一次完整自检
 * @param timeout_ticks 等待命令队列空间的有界 tick 数
 * @return ESP_OK 已入队
 *         ESP_ERR_INVALID_STATE task 未活动或已有运行/排队请求
 *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
 */
esp_err_t selftest_service_request_run(TickType_t timeout_ticks);

/**
 * @brief 请求取消当前项并退出 selftest_task
 * @param timeout_ticks 等待命令队列空间的有界 tick 数
 * @return ESP_OK 已入队
 *         ESP_ERR_INVALID_STATE task 未活动
 *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
 */
esp_err_t selftest_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 进入统一服务框架拥有的 selftest_task 循环
 * @details 仅消费 RUN_ALL/STOP typed 命令，使用有界等待并串行执行单项。
 * @return ESP_OK 已安全退出，其他值表示框架级错误
 */
esp_err_t selftest_service_run(void);

/**
 * @brief 查询当前 handler 是否应尽快取消
 * @return true 已收到 STOP，false 继续执行
 */
bool selftest_service_cancel_requested(void);

/**
 * @brief 获取 selftest_task 栈高水位快照
 * @param metrics 指标输出
 * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效
 */
esp_err_t selftest_service_metrics_snapshot(selftest_service_metrics_t *metrics);

/**
 * @brief 获取 selftest_task 私有 typed 命令队列
 * @return 队列句柄，尚未创建时为 NULL
 */
QueueHandle_t selftest_service_queue(void);

#endif /* LEGBOT_SELFTEST_SERVICE_H */
