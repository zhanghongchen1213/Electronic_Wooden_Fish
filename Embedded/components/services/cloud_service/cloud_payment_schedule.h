/**
 * @file     cloud_payment_schedule.h
 * @brief    云支付有限自动复查的纯 deadline 策略接口
 * @details  在不依赖任务、队列或 modem 的前提下维护 FAST、SLOW 与网络退避节拍，确保错过窗口不突发补查。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_CLOUD_PAYMENT_SCHEDULE_H
#define LEGBOT_CLOUD_PAYMENT_SCHEDULE_H

#include <stdbool.h>
#include <stdint.h>

#include "exoskeleton_scene_config.h"

typedef enum
{
    CLOUD_PAYMENT_MODE_IDLE = 0, /**< 当前没有支付会话。 */
    CLOUD_PAYMENT_MODE_INITIAL,  /**< 完整 BLE 准入后的立即首查。 */
    CLOUD_PAYMENT_MODE_FAST,     /**< 首个 UNPAID 后三分钟快速窗口。 */
    CLOUD_PAYMENT_MODE_SLOW,     /**< 快速窗口后与 telemetry 合并慢查。 */
    CLOUD_PAYMENT_MODE_BACKOFF,  /**< 网络暂错，等待 modem 可用时刻。 */
    CLOUD_PAYMENT_MODE_COMPLETE, /**< 已支付并结束云复查。 */
    CLOUD_PAYMENT_MODE_BLOCKED,  /**< 致命错误终止会话。 */
} cloud_payment_mode_t;

typedef enum
{
    CLOUD_PAYMENT_OUTCOME_UNPAID = 0, /**< 有效未支付响应。 */
    CLOUD_PAYMENT_OUTCOME_PAID,       /**< 有效已支付响应。 */
    CLOUD_PAYMENT_OUTCOME_TRANSIENT,  /**< 网络、408、429 或 5xx 暂态错误。 */
    CLOUD_PAYMENT_OUTCOME_FATAL,      /**< 配置、鉴权或协议致命错误。 */
    CLOUD_PAYMENT_OUTCOME_CANCELLED,  /**< 断链、解绑或换代取消。 */
} cloud_payment_outcome_t;

typedef struct
{
    bool active;                    /**< 会话是否仍允许调度 HTTP。 */
    bool in_flight;                 /**< 是否已有唯一活动 HTTP。 */
    bool first_unpaid_valid;        /**< 是否已锚定首个有效 UNPAID。 */
    bool manual_bypass_used;        /**< 当前会话是否已消费一次立即检查。 */
    bool manual_due;                /**< 是否存在一次人工加速 deadline。 */
    cloud_payment_mode_t cadence;   /**< 不含暂态退避覆盖的基础节拍。 */
    uint64_t first_unpaid_ms;       /**< 首个有效 UNPAID 单调毫秒。 */
    uint64_t next_deadline_ms;      /**< 下一支付节拍 deadline。 */
    uint64_t modem_available_ms;    /**< 网络退避后 modem 最早可用时刻。 */
    uint16_t http_attempt_count;    /**< 当前逻辑会话已启动 HTTP 次数。 */
    uint8_t next_fast_slot;         /**< 下一快速槽，范围 1～11。 */
    uint8_t backoff_index;          /**< 下一暂态错误使用的退避档位。 */
} cloud_payment_schedule_t;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 清空支付调度状态
     * @param schedule 支付调度状态
     */
    void cloud_payment_schedule_reset(cloud_payment_schedule_t *schedule);

    /**
     * @brief 建立一次立即首查的支付会话
     * @param schedule 支付调度状态
     * @param now_ms 当前单调毫秒
     */
    void cloud_payment_schedule_start(cloud_payment_schedule_t *schedule,
                                      uint64_t now_ms);

    /**
     * @brief 尝试消费当前唯一到期 HTTP 执行权
     * @details 快速期跨过多个槽时只消费最新一个，绝不补发错过的多个请求；180 秒边界不再快速查询。
     * @param schedule 支付调度状态
     * @param now_ms 当前单调毫秒
     * @param next_cloud_window_ms telemetry 下一固定云窗口
     * @return true 本次可启动一个 HTTP，false 尚未到期或会话不可执行
     */
    bool cloud_payment_schedule_take_due(
        cloud_payment_schedule_t *schedule,
        uint64_t now_ms,
        uint64_t next_cloud_window_ms);

    /**
     * @brief 应用一次 HTTP 终态并计算后续 deadline
     * @param schedule 支付调度状态
     * @param outcome 本次支付 HTTP 分类结果
     * @param now_ms 终态发生的单调毫秒
     * @param next_cloud_window_ms telemetry 下一固定云窗口
     */
    void cloud_payment_schedule_finish_attempt(
        cloud_payment_schedule_t *schedule,
        cloud_payment_outcome_t outcome,
        uint64_t now_ms,
        uint64_t next_cloud_window_ms);

    /**
     * @brief 消费本会话唯一一次人工立即检查加速权
     * @param schedule 支付调度状态
     * @param now_ms 当前单调毫秒
     * @return true 已把下一次检查拉到当前时刻，false 已使用、在途或会话无效
     */
    bool cloud_payment_schedule_accelerate(
        cloud_payment_schedule_t *schedule,
        uint64_t now_ms);

    /**
     * @brief 返回当前对外诊断模式
     * @param schedule 支付调度状态
     * @param now_ms 当前单调毫秒
     * @return FAST、SLOW、BACKOFF 或终态模式
     */
    cloud_payment_mode_t cloud_payment_schedule_mode(
        const cloud_payment_schedule_t *schedule,
        uint64_t now_ms);

    /**
     * @brief 取得指定网络退避档位的毫秒数
     * @param index 退避档位，超出范围返回封顶值
     * @return 退避毫秒数
     */
    uint32_t cloud_payment_schedule_backoff_ms(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CLOUD_PAYMENT_SCHEDULE_H */
