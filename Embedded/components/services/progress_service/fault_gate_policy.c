/**
 * @file     fault_gate_policy.c
 * @brief    fault_locked 生产裁决纯逻辑实现
 * @details  不访问 NVS、日志、队列或 UI；只维护连续失败计数与 lock 位。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "fault_gate_policy.h"

#include <stddef.h>

static ewf_fault_gate_decision_t make_decision(const ewf_fault_gate_state_t *state,
                                               bool previous_locked)
{
    ewf_fault_gate_decision_t decision = {
        .locked = false,
        .locked_changed = false,
        .unlock_on_flush = false,
    };
    if (state != NULL)
    {
        decision.locked = state->locked;
        decision.locked_changed = (state->locked != previous_locked);
    }
    return decision;
}

void ewf_fault_gate_reset(ewf_fault_gate_state_t *state)
{
    if (state == NULL)
    {
        return;
    }
    state->consecutive_persist_errors = 0U;
    state->locked = false;
    state->display_fault = false;
}

ewf_fault_gate_decision_t ewf_fault_gate_on_persist_result(
    ewf_fault_gate_state_t *state,
    bool persist_ok)
{
    if (state == NULL)
    {
        return make_decision(NULL, false);
    }

    const bool previous = state->locked;
    if (persist_ok)
    {
        state->consecutive_persist_errors = 0U;
        /* 成功落盘解锁 persist 触发的 lock；display_fault 仍可保持 lock。 */
        if (!state->display_fault)
        {
            state->locked = false;
        }
        ewf_fault_gate_decision_t decision = make_decision(state, previous);
        decision.unlock_on_flush = (previous && !state->locked);
        return decision;
    }

    if (state->consecutive_persist_errors < 0xFFFFFFFFu)
    {
        ++state->consecutive_persist_errors;
    }
    if (state->consecutive_persist_errors >= EWF_FAULT_GATE_PERSIST_FAIL_THRESHOLD)
    {
        state->locked = true;
    }
    return make_decision(state, previous);
}

ewf_fault_gate_decision_t ewf_fault_gate_on_display_fault(
    ewf_fault_gate_state_t *state,
    bool display_fault)
{
    if (state == NULL)
    {
        return make_decision(NULL, false);
    }

    const bool previous = state->locked;
    state->display_fault = display_fault;
    if (display_fault)
    {
        state->locked = true;
    }
    else if (state->consecutive_persist_errors < EWF_FAULT_GATE_PERSIST_FAIL_THRESHOLD)
    {
        state->locked = false;
    }
    return make_decision(state, previous);
}

bool ewf_fault_gate_feedback_or_sync_failure_locks(void)
{
    return false;
}
