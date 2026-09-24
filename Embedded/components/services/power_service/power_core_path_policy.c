/**
 * @file     power_core_path_policy.c
 * @brief    低电/落盘优先核心路径调度与电量迟滞实现
 * @details  只做纯逻辑裁决；不访问 GPIO、NVS、网络、日志或关机 API。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "power_core_path_policy.h"

#include <stddef.h>

/** 与 app_state.h watch_power_level_t 数值对齐，避免本模块依赖 ESP 头。 */
#define EWF_PWR_LVL_NORMAL 0U
#define EWF_PWR_LVL_WARN 1U
#define EWF_PWR_LVL_CRITICAL 2U

void ewf_power_battery_level_reset(ewf_power_battery_level_state_t *state)
{
    if (state == NULL)
    {
        return;
    }
    state->level = EWF_PWR_LVL_NORMAL;
}

uint8_t ewf_power_battery_level_update(ewf_power_battery_level_state_t *state,
                                       uint8_t percent)
{
    if (state == NULL)
    {
        return EWF_PWR_LVL_NORMAL;
    }
    if (percent > 100U)
    {
        percent = 100U;
    }

    uint8_t level = state->level;
    if (level > EWF_PWR_LVL_CRITICAL)
    {
        level = EWF_PWR_LVL_NORMAL;
    }

    switch (level)
    {
    case EWF_PWR_LVL_NORMAL:
        if (percent <= EWF_POWER_SOC_CRITICAL_ENTER_PERCENT)
        {
            level = EWF_PWR_LVL_CRITICAL;
        }
        else if (percent <= EWF_POWER_SOC_WARN_ENTER_PERCENT)
        {
            level = EWF_PWR_LVL_WARN;
        }
        break;
    case EWF_PWR_LVL_WARN:
        if (percent <= EWF_POWER_SOC_CRITICAL_ENTER_PERCENT)
        {
            level = EWF_PWR_LVL_CRITICAL;
        }
        else if (percent >= EWF_POWER_SOC_WARN_EXIT_PERCENT)
        {
            level = EWF_PWR_LVL_NORMAL;
        }
        break;
    case EWF_PWR_LVL_CRITICAL:
        if (percent >= EWF_POWER_SOC_WARN_EXIT_PERCENT)
        {
            level = EWF_PWR_LVL_NORMAL;
        }
        else if (percent >= EWF_POWER_SOC_CRITICAL_EXIT_PERCENT)
        {
            level = EWF_PWR_LVL_WARN;
        }
        break;
    default:
        level = EWF_PWR_LVL_NORMAL;
        break;
    }

    state->level = level;
    return level;
}

ewf_power_core_path_decision_t ewf_power_core_path_decide(
    const ewf_power_core_path_input_t *input)
{
    ewf_power_core_path_decision_t decision = {
        .primary_action = EWF_POWER_CORE_ACTION_FLUSH_PROGRESS,
        .allow_flush = true,
        .allow_feedback = true,
        .allow_sync = true,
        .defer_noncritical = false,
        .suggest_brightness_cap = false,
        .brightness_cap = 100U,
        .software_poweroff_intent = false,
    };

    if (input == NULL)
    {
        decision.allow_feedback = false;
        decision.allow_sync = false;
        decision.defer_noncritical = true;
        decision.primary_action = EWF_POWER_CORE_ACTION_DEFER_NONCRITICAL;
        return decision;
    }

    const bool persist_busy = input->persist_pending || input->persist_inflight;
    if (persist_busy)
    {
        /* 落盘冲突：先 FLUSH，推迟一切非关键反馈与新开窗。 */
        decision.allow_feedback = false;
        decision.allow_sync = false;
        decision.defer_noncritical = true;
        decision.primary_action = EWF_POWER_CORE_ACTION_FLUSH_PROGRESS;
    }
    else if (input->feedback_pending)
    {
        decision.primary_action = EWF_POWER_CORE_ACTION_ALLOW_FEEDBACK;
    }
    else if (input->sync_window_requested)
    {
        decision.primary_action = EWF_POWER_CORE_ACTION_ALLOW_SYNC;
    }

    if (input->power_level == EWF_PWR_LVL_CRITICAL)
    {
        decision.suggest_brightness_cap = true;
        decision.brightness_cap = EWF_POWER_CRITICAL_BRIGHTNESS_CAP;
        /* CRITICAL 不主动断网；落盘优先已由 persist_busy 分支保证。 */
    }

    /* LOW_BATTERY_WARN：不主动断网、不额外切断反馈/同步（除非落盘冲突）。 */
    return decision;
}
