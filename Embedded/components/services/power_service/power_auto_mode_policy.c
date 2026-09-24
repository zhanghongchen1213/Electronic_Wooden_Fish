/**
 * @file     power_auto_mode_policy.c
 * @brief    BOOT0 自动模式启停与周期投递纯逻辑实现
 * @details  只维护内存态与决策；不访问 GPIO、定时器、NVS、队列或日志。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "power_auto_mode_policy.h"

#include <stddef.h>

static ewf_power_auto_mode_decision_t make_decision(
    const ewf_power_auto_mode_state_t *state,
    ewf_power_auto_action_t action,
    uint32_t emit_at_ms)
{
    ewf_power_auto_mode_decision_t decision = {
        .action = action,
        .enabled = false,
        .next_due_ms = 0U,
        .emit_count = 0U,
        .emit_at_ms = emit_at_ms,
    };
    if (state != NULL)
    {
        decision.enabled = state->enabled;
        decision.next_due_ms = state->next_due_ms;
        decision.emit_count = state->emit_count;
    }
    return decision;
}

static void clear_schedule(ewf_power_auto_mode_state_t *state)
{
    state->enabled = false;
    state->anchor_ms = 0U;
    state->next_due_ms = 0U;
}

void ewf_power_auto_mode_reset(ewf_power_auto_mode_state_t *state)
{
    if (state == NULL)
    {
        return;
    }
    clear_schedule(state);
    state->emit_count = 0U;
}

ewf_power_auto_mode_decision_t ewf_power_auto_mode_on_runtime_tap(
    ewf_power_auto_mode_state_t *state,
    uint32_t at_ms)
{
    if (state == NULL)
    {
        return make_decision(NULL, EWF_POWER_AUTO_ACTION_NOOP, 0U);
    }

    if (state->enabled)
    {
        /* 再次短按：退出，停发，已入队事件不回滚。 */
        clear_schedule(state);
        return make_decision(state, EWF_POWER_AUTO_ACTION_DISABLE, 0U);
    }

    /* 进入：锚点=释放确认；首拍在锚点+PERIOD，当下零 emit。 */
    state->enabled = true;
    state->anchor_ms = at_ms;
    state->next_due_ms = at_ms + EWF_POWER_AUTO_MODE_PERIOD_MS;
    return make_decision(state, EWF_POWER_AUTO_ACTION_ENABLE, 0U);
}

ewf_power_auto_mode_decision_t ewf_power_auto_mode_on_period_due(
    ewf_power_auto_mode_state_t *state,
    uint32_t now_ms,
    bool completed)
{
    if (state == NULL)
    {
        return make_decision(NULL, EWF_POWER_AUTO_ACTION_NOOP, 0U);
    }

    /* 完成锁定必须停模式+清定时器意图，即使本周期已到期也不再 emit。 */
    if (completed)
    {
        if (!state->enabled)
        {
            return make_decision(state, EWF_POWER_AUTO_ACTION_NOOP, 0U);
        }
        clear_schedule(state);
        return make_decision(state, EWF_POWER_AUTO_ACTION_DISABLE, 0U);
    }

    if (!state->enabled)
    {
        return make_decision(state, EWF_POWER_AUTO_ACTION_NOOP, 0U);
    }

    if (now_ms < state->next_due_ms)
    {
        return make_decision(state, EWF_POWER_AUTO_ACTION_NOOP, 0U);
    }

    const uint32_t emit_at_ms = state->next_due_ms;
    /* 按固定周期推进，避免以 now_ms 重锚造成逻辑漂移。 */
    state->next_due_ms = state->next_due_ms + EWF_POWER_AUTO_MODE_PERIOD_MS;
    ++state->emit_count;
    return make_decision(state, EWF_POWER_AUTO_ACTION_EMIT_TAP, emit_at_ms);
}

ewf_power_auto_mode_decision_t ewf_power_auto_mode_force_disable(
    ewf_power_auto_mode_state_t *state,
    uint32_t now_ms)
{
    (void)now_ms;
    if (state == NULL)
    {
        return make_decision(NULL, EWF_POWER_AUTO_ACTION_NOOP, 0U);
    }
    if (!state->enabled)
    {
        return make_decision(state, EWF_POWER_AUTO_ACTION_NOOP, 0U);
    }
    clear_schedule(state);
    return make_decision(state, EWF_POWER_AUTO_ACTION_DISABLE, 0U);
}
