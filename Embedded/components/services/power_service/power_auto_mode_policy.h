/**
 * @file     power_auto_mode_policy.h
 * @brief    BOOT0 自动模式启停与周期投递的纯逻辑策略
 * @details  不含任何 ESP-IDF 依赖，可由主机测试重复运行。策略只决定 enable/disable/
 *           emit_tap/noop 与下一到期时刻；统一 gate 仍由 tap_input_service 执行。
 *           短按切换当下不立即 emit；首拍在锚点 + 3000ms。完成锁定强制停表。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_POWER_AUTO_MODE_POLICY_H
#define EWF_POWER_AUTO_MODE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 自动模式周期单调毫秒（FR-E-011）。 */
#define EWF_POWER_AUTO_MODE_PERIOD_MS 3000U
/** 真机连续周期验收容差单调毫秒（NFR；主机逻辑时钟期望 0 误差）。 */
#define EWF_POWER_AUTO_MODE_PERIOD_TOLERANCE_MS 100U

    /** 策略输出动作。 */
    typedef enum
    {
        EWF_POWER_AUTO_ACTION_NOOP = 0,  /**< 无状态变化、不投递。 */
        EWF_POWER_AUTO_ACTION_ENABLE,    /**< 进入自动模式（启定时器意图）。 */
        EWF_POWER_AUTO_ACTION_DISABLE,   /**< 退出自动模式（停定时器意图）。 */
        EWF_POWER_AUTO_ACTION_EMIT_TAP,  /**< 本周期应投递一个 automatic_tap。 */
    } ewf_power_auto_action_t;

    /** 自动模式运行内存态（不落盘）。 */
    typedef struct
    {
        bool enabled;           /**< 自动模式是否开启。 */
        uint32_t anchor_ms;     /**< 进入时释放确认锚点单调毫秒；关闭时为 0。 */
        uint32_t next_due_ms;   /**< 下一周期到期单调毫秒；关闭时为 0。 */
        uint32_t emit_count;    /**< 已决策 emit 次数（诊断）。 */
    } ewf_power_auto_mode_state_t;

    /** 一次策略决策。 */
    typedef struct
    {
        ewf_power_auto_action_t action; /**< 输出动作。 */
        bool enabled;                   /**< 决策后模式是否开启。 */
        uint32_t next_due_ms;           /**< 决策后下一到期时刻。 */
        uint32_t emit_count;            /**< 决策后累计 emit 次数。 */
        uint32_t emit_at_ms;            /**< EMIT_TAP 时的投递时刻；否则 0。 */
    } ewf_power_auto_mode_decision_t;

    /**
     * @brief 复位自动模式状态为默认关闭
     * @param state 策略实例
     */
    void ewf_power_auto_mode_reset(ewf_power_auto_mode_state_t *state);

    /**
     * @brief 处理一次运行态 BOOT0 短按（释放确认）
     * @details 每次翻转一次；进入时以 at_ms 为锚点，首拍在 at_ms+PERIOD；
     *          当下不 emit。退出时清除锚点与下一到期。
     * @param state 策略实例
     * @param at_ms 释放确认单调毫秒
     * @return 决策（ENABLE 或 DISABLE）
     */
    ewf_power_auto_mode_decision_t ewf_power_auto_mode_on_runtime_tap(
        ewf_power_auto_mode_state_t *state,
        uint32_t at_ms);

    /**
     * @brief 处理一次周期到期核对
     * @details completed==true 时强制 DISABLE 且不 emit；未开启或未到期则 NOOP；
     *          到期则 EMIT_TAP 并将 next_due 推进一个周期（逻辑时钟无累积漂移）。
     *          queue_full/fault 不在此层拦截（由 tap_input gate 拒绝）。
     * @param state 策略实例
     * @param now_ms 当前单调毫秒
     * @param completed 完成锁定 gate 快照
     * @return 决策
     */
    ewf_power_auto_mode_decision_t ewf_power_auto_mode_on_period_due(
        ewf_power_auto_mode_state_t *state,
        uint32_t now_ms,
        bool completed);

    /**
     * @brief 强制因完成锁定停表
     * @param state 策略实例
     * @param now_ms 当前单调毫秒（仅记入诊断上下文，不用于延迟）
     * @return DISABLE 决策；已关闭时为 NOOP
     */
    ewf_power_auto_mode_decision_t ewf_power_auto_mode_force_disable(
        ewf_power_auto_mode_state_t *state,
        uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* EWF_POWER_AUTO_MODE_POLICY_H */
