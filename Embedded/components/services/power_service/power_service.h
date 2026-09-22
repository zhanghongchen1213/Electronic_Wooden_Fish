/**
 * @file     power_service.h
 * @brief    PWR_INT/BOOT0 板级输入边界服务接口
 * @details  服务在任务上下文完成去抖、低电平时长测量和 typed 结论交接；ISR 只记录时间戳并投递
 *           ISR-safe 消息。本服务不检测充电状态、不驱动 LTC2954 KILL，也不提供软件关机。
 * @author   ZHC
 * @date     2026-07-09
 */

#ifndef LEGBOT_POWER_SERVICE_H
#define LEGBOT_POWER_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"
#include "power_boot_policy.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 服务在统一服务表中的固定 ID。 */
#define LEGBOT_POWER_SERVICE_ID LEGBOT_SERVICE_POWER

/** power_task 私有 typed 命令队列深度。 */
#define POWER_SERVICE_QUEUE_DEPTH 8U

/** power_service 的可观测不可变快照。 */
    typedef struct
    {
        uint8_t pwr_level;                       /**< PWR_INT 原始逻辑电平，不解释极性。 */
        uint8_t boot_level;                      /**< BOOT0 原始逻辑电平，不解释极性。 */
        bool pwr_low;                            /**< 是否处于已确认的持续低电平区间。 */
        uint32_t pwr_low_duration_ms;            /**< 当前低电平区间已持续的单调毫秒。 */
        uint32_t pwr_interval_count;              /**< 已交接的合法 PWR 按压区间数。 */
        uint32_t boot_tap_count;                 /**< 已交接的运行态 BOOT0 短按数。 */
        ewf_power_boot_event_kind_t last_event;  /**< 最近一次 typed 结论。 */
        uint32_t last_event_duration_ms;         /**< 最近一次结论的区间时长。 */
        bool boot0_runtime_armed;                /**< 启动采样窗口是否已结束。 */
        bool isr_registered;                     /**< PWR/BOOT 边沿中断是否已登记。 */
    } power_service_snapshot_t;

    /**
     * @brief 创建 power_task 私有 typed 命令队列
     * @return ESP_OK 成功或已创建，ESP_ERR_NO_MEM 队列创建失败
     */
    esp_err_t power_service_init_contracts(void);

    /**
     * @brief 在框架创建 power_task 前进入 prepared 状态
     * @return ESP_OK 已准备，ESP_ERR_INVALID_STATE 队列不可用或任务已活动
     */
    esp_err_t power_service_prepare_run(void);

    /** @brief 回滚未成功创建的 power_task prepared 状态。 */
    void power_service_cancel_prepared_run(void);

    /**
     * @brief 删除未运行 power_task 的 typed 命令队列
     * @return ESP_OK 成功或未创建，ESP_ERR_INVALID_STATE 任务正在活动
     */
    esp_err_t power_service_deinit_contracts(void);

    /**
     * @brief 进入 power_task 循环
     * @details 登记 PWR_INT/BOOT0 任意边沿 ISR，结束启动采样窗口，串行完成去抖与时长交接。
     * @return ESP_OK 已安全退出，其他值表示框架级错误
     */
    esp_err_t power_service_run(void);

    /**
     * @brief 请求停止 power_task
     * @param timeout_ticks 等待命令队列空间的有界 tick 数
     * @return ESP_OK 已入队
     *         ESP_ERR_INVALID_STATE 任务未活动
     *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
     */
    esp_err_t power_service_request_stop(TickType_t timeout_ticks);

    /**
     * @brief 复制当前不可变快照
     * @param snapshot 快照输出
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效，ESP_ERR_INVALID_STATE 尚未启动
     */
    esp_err_t power_service_snapshot(power_service_snapshot_t *snapshot);

    /**
     * @brief 获取 power_task 私有 typed 命令队列
     * @return 队列句柄，尚未创建时为 NULL
     */
    QueueHandle_t power_service_queue(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_POWER_SERVICE_H */
