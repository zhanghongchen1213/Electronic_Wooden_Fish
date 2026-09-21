/**
 * @file     gps_acquisition_policy.h
 * @brief    GPS 自动搜星、退避与失锁纯策略接口
 * @details  提供无任务、无队列和无 GPIO 副作用的搜星预算、退避阶段、亮屏机会与失锁时间判断。
 * @author   ZHC
 * @date     2026-07-31
 */

#ifndef LEGBOT_GPS_ACQUISITION_POLICY_H
#define LEGBOT_GPS_ACQUISITION_POLICY_H

#include <stdbool.h>
#include <stdint.h>

/** GPS 任一自动搜星会话的最大工作预算。 */
#define GPS_ACQUISITION_INITIAL_BUDGET_MS 180000U
/** GPS 首次失败后的单次重试搜星预算。 */
#define GPS_ACQUISITION_RETRY_BUDGET_MS 180000U
/** GPS 自动退避阶段数量，最后一档循环使用。 */
#define GPS_ACQUISITION_BACKOFF_STAGE_COUNT 4U
/** GPS v2 运动持续时的定位间隔。 */
#define GPS_ACQUISITION_MOTION_INTERVAL_MS 600000U
/** GPS v2 最近运动事件后的静止宽限。 */
#define GPS_ACQUISITION_MOTION_GRACE_MS 120000U
/** GPS 手动或亮屏提前搜星后的主动冷却时间。 */
#define GPS_ACQUISITION_ACTIVE_TRIGGER_COOLDOWN_MS 300000U

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 计算本次搜星会话的最大工作预算
     * @param initial_completed 首次启动会话是否已经结束
     * @param boot_trigger 本次请求是否由启动流程触发
     * @return 任一会话均返回 3 分钟硬上限
     */
    uint32_t gps_acquisition_policy_session_budget_ms(
        bool initial_completed,
        bool boot_trigger);

    /**
     * @brief 取得指定退避阶段的等待时间
     * @param stage 当前退避阶段，超出范围时使用 60 分钟封顶值
     * @return 10、20、40 或 60 分钟的等待毫秒数
     */
    uint32_t gps_acquisition_policy_backoff_delay_ms(uint8_t stage);

    /**
     * @brief 计算一次失败后的下一退避阶段
     * @param stage 当前退避阶段
     * @return 下一阶段，达到 60 分钟档后保持不变
     */
    uint8_t gps_acquisition_policy_next_backoff_stage(uint8_t stage);

    /**
     * @brief 判断当前退避窗口能否消费一次亮屏提前搜星机会
     * @param stage 当前退避阶段
     * @param opportunity_used 当前窗口是否已经消费机会
     * @param now_ms 当前单调毫秒
     * @param cooldown_deadline_ms 主动触发冷却截止单调毫秒
     * @return true 表示处于 20/40/60 分钟长退避且机会与冷却均允许
     */
    bool gps_acquisition_policy_screen_opportunity_allowed(
        uint8_t stage,
        bool opportunity_used,
        uint64_t now_ms,
        uint64_t cooldown_deadline_ms);

    /**
     * @brief 判断最近运动事件是否仍处于两分钟活动宽限
     * @param last_motion_ms 最近运动事件单调毫秒，零表示尚无运动证据
     * @param now_ms 当前单调毫秒
     * @return true 表示仍允许自动定位
     */
    bool gps_acquisition_policy_motion_active(uint64_t last_motion_ms,
                                              uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_GPS_ACQUISITION_POLICY_H */
