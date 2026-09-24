/**
 * @file     power_core_path_policy.h
 * @brief    低电/落盘优先核心路径调度与电量迟滞纯逻辑
 * @details  零 ESP-IDF 依赖。裁决：任何 persist_pending|inflight 时禁止新开
 *           HTTPS 活动窗口与新的非关键音频/RGB；CRITICAL 仍先落盘并可建议亮度上限。
 *           永不产出软件关机意图，不实现充电检测分支。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_POWER_CORE_PATH_POLICY_H
#define EWF_POWER_CORE_PATH_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 低电 WARN 进入阈值（含），样机定标前 hardware_pending，非产品承诺。 */
#define EWF_POWER_SOC_WARN_ENTER_PERCENT 20U
/** 低电 WARN 退出阈值（含）。 */
#define EWF_POWER_SOC_WARN_EXIT_PERCENT 23U
/** 严重低电 CRITICAL 进入阈值（含）。 */
#define EWF_POWER_SOC_CRITICAL_ENTER_PERCENT 10U
/** 严重低电 CRITICAL 退出阈值（含）。 */
#define EWF_POWER_SOC_CRITICAL_EXIT_PERCENT 13U
/** CW2015 产品轮询周期单调毫秒（30–60s 建议区间；样机可定标）。 */
#define EWF_POWER_BATTERY_POLL_PERIOD_MS 30000U
/** CRITICAL 建议亮度上限（经 settings 只读建议，不直改 LVGL；非擅自改产品默认）。 */
#define EWF_POWER_CRITICAL_BRIGHTNESS_CAP 30U
/** PRD §14 收敛旋钮注释锚点：活动窗口最大时长毫秒（缩短须经产品确认）。 */
#define EWF_POWER_PRD14_SYNC_WINDOW_MAX_MS 60000U

    /** 核心路径有序动作意图（互不表示软件关机）。 */
    typedef enum
    {
        EWF_POWER_CORE_ACTION_FLUSH_PROGRESS = 0, /**< 优先完成累计/轮次落盘。 */
        EWF_POWER_CORE_ACTION_ALLOW_FEEDBACK,     /**< 允许非关键反馈输出。 */
        EWF_POWER_CORE_ACTION_ALLOW_SYNC,         /**< 允许新开 HTTPS 活动窗口。 */
        EWF_POWER_CORE_ACTION_DEFER_NONCRITICAL,  /**< 推迟非关键反馈/同步。 */
    } ewf_power_core_action_t;

    /** 策略输入快照。 */
    typedef struct
    {
        uint8_t power_level;          /**< watch_power_level_t 数值，避免依赖 app_state。 */
        bool persist_pending;         /**< 事务组待落盘或上次失败待重试。 */
        bool persist_inflight;        /**< 事务组正在落盘。 */
        bool sync_window_requested;   /**< 调用方意图：评估是否允许开窗。 */
        bool feedback_pending;        /**< 调用方意图：评估是否允许非关键反馈。 */
    } ewf_power_core_path_input_t;

    /** 策略输出。 */
    typedef struct
    {
        ewf_power_core_action_t primary_action; /**< 首要动作意图。 */
        bool allow_flush;                       /**< 始终允许落盘（含 CRITICAL）。 */
        bool allow_feedback;                    /**< 是否允许启动新的非关键反馈。 */
        bool allow_sync;                        /**< 是否允许新开活动窗口。 */
        bool defer_noncritical;                 /**< 是否因落盘冲突推迟非关键路径。 */
        bool suggest_brightness_cap;            /**< CRITICAL 时建议限制亮度。 */
        uint8_t brightness_cap;                 /**< 建议亮度上限；未建议时为 100。 */
        bool software_poweroff_intent;          /**< 恒为 false：禁止软件关机意图。 */
    } ewf_power_core_path_decision_t;

    /** 电量迟滞状态（仅内存，不落盘）。 */
    typedef struct
    {
        uint8_t level; /**< 当前 watch_power_level_t 数值。 */
    } ewf_power_battery_level_state_t;

    /**
     * @brief 复位电量等级迟滞状态为 NORMAL
     * @param state 迟滞状态
     */
    void ewf_power_battery_level_reset(ewf_power_battery_level_state_t *state);

    /**
     * @brief 按 SOC 百分比更新闭集电量等级（含迟滞）
     * @param state 迟滞状态
     * @param percent 有效 SOC 0–100
     * @return 更新后的等级数值
     */
    uint8_t ewf_power_battery_level_update(ewf_power_battery_level_state_t *state,
                                           uint8_t percent);

    /**
     * @brief 裁决核心路径优先序
     * @param input 输入快照；NULL 时返回安全 defer（仍允许 flush）
     * @return 决策；software_poweroff_intent 恒 false
     */
    ewf_power_core_path_decision_t ewf_power_core_path_decide(
        const ewf_power_core_path_input_t *input);

#ifdef __cplusplus
}
#endif

#endif /* EWF_POWER_CORE_PATH_POLICY_H */
