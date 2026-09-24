/**
 * @file     sync_window_policy.c
 * @brief    活动窗口与退避纯逻辑实现。
 * @details  无差值且无立即同步/待应用命令时跳过；失败单调抬升退避档。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_window_policy.h"

#include <stddef.h>

const uint32_t EWF_SYNC_BACKOFF_MS[EWF_SYNC_BACKOFF_STAGE_COUNT] = {
    5000U,
    15000U,
    30000U,
    60000U,
};

/* 编译期：首档>0，后档≥前档。 */
typedef char ewf_sync_backoff_stage0_positive_check[
    (5000U > 0U) ? 1 : -1];
typedef char ewf_sync_backoff_mono_1_check[(15000U >= 5000U) ? 1 : -1];
typedef char ewf_sync_backoff_mono_2_check[(30000U >= 15000U) ? 1 : -1];
typedef char ewf_sync_backoff_mono_3_check[(60000U >= 30000U) ? 1 : -1];

static ewf_sync_window_decision_t make_decision(
    ewf_sync_window_action_t action,
    uint8_t stage,
    bool schedule_retry,
    bool enter_recovery,
    ewf_sync_trigger_t trigger)
{
    const ewf_sync_window_decision_t decision = {
        .action = action,
        .next_backoff_stage = stage,
        .backoff_delay_ms = ewf_sync_backoff_delay_ms(stage),
        .schedule_retry = schedule_retry,
        .enter_recovery = enter_recovery,
        .accepted_trigger = trigger,
    };
    return decision;
}

static void bump_backoff(ewf_sync_window_state_t *state, uint64_t now_ms)
{
    if (state->backoff_stage + 1U < EWF_SYNC_BACKOFF_STAGE_COUNT) {
        ++state->backoff_stage;
    }
    state->next_eligible_ms =
        now_ms + ewf_sync_backoff_delay_ms(state->backoff_stage);
}

uint32_t ewf_sync_backoff_delay_ms(uint8_t stage)
{
    if (stage >= EWF_SYNC_BACKOFF_STAGE_COUNT) {
        return EWF_SYNC_BACKOFF_MS[EWF_SYNC_BACKOFF_STAGE_COUNT - 1U];
    }
    return EWF_SYNC_BACKOFF_MS[stage];
}

void ewf_sync_window_state_init(ewf_sync_window_state_t *state)
{
    if (state == NULL) {
        return;
    }
    state->window_open = false;
    state->backoff_stage = 0U;
    state->consecutive_at_timeouts = 0U;
    state->next_eligible_ms = 0U;
    state->transport_busy = false;
}

ewf_sync_window_decision_t ewf_sync_window_policy_apply(
    ewf_sync_window_state_t *state,
    const ewf_sync_window_input_t *input)
{
    if (state == NULL || input == NULL) {
        return make_decision(EWF_SYNC_WINDOW_SKIP, 0U, false, false,
                             EWF_SYNC_TRIGGER_NONE);
    }

    if (input->force_close) {
        state->window_open = false;
        state->transport_busy = false;
        return make_decision(EWF_SYNC_WINDOW_CLOSE, state->backoff_stage, false,
                             false, EWF_SYNC_TRIGGER_NONE);
    }

    /* 传输结果回写：成功清退避并关窗；超时累计；其它失败抬升退避。 */
    if (input->transport_ok) {
        state->backoff_stage = 0U;
        state->consecutive_at_timeouts = 0U;
        state->next_eligible_ms = 0U;
        state->transport_busy = false;
        state->window_open = false;
    } else if (input->transport_timeout) {
        if (state->consecutive_at_timeouts < 255U) {
            ++state->consecutive_at_timeouts;
        }
        if (state->consecutive_at_timeouts >=
            EWF_SYNC_AT_TIMEOUT_RECOVERY_THRESHOLD) {
            bump_backoff(state, input->now_ms);
            state->consecutive_at_timeouts = 0U;
            state->window_open = false;
            state->transport_busy = false;
            return make_decision(EWF_SYNC_WINDOW_CLOSE, state->backoff_stage,
                                 true, true, input->trigger);
        }
        state->transport_busy = false;
        /* 单次超时只告警：不抬档。 */
    } else if (input->transport_failed) {
        bump_backoff(state, input->now_ms);
        state->window_open = false;
        state->transport_busy = false;
        return make_decision(EWF_SYNC_WINDOW_CLOSE, state->backoff_stage, true,
                             false, EWF_SYNC_TRIGGER_NONE);
    }

    const bool has_backlog = input->local_total > input->acked_total;
    const bool has_pending_command =
        input->command_revision > input->applied_revision;
    ewf_sync_trigger_t effective = input->trigger;
    if (effective == EWF_SYNC_TRIGGER_NONE) {
        if (input->immediate_pending) {
            effective = EWF_SYNC_TRIGGER_IMMEDIATE_SYNC;
        } else if (has_pending_command) {
            effective = EWF_SYNC_TRIGGER_PENDING_COMMAND;
        } else if (has_backlog) {
            effective = EWF_SYNC_TRIGGER_BACKLOG;
        }
    }

    if (effective == EWF_SYNC_TRIGGER_NONE) {
        return make_decision(EWF_SYNC_WINDOW_SKIP, state->backoff_stage, false,
                             false, EWF_SYNC_TRIGGER_NONE);
    }

    /* 退避窗口内：合并意图，不立即开窗。 */
    if (input->now_ms < state->next_eligible_ms) {
        return make_decision(EWF_SYNC_WINDOW_MERGE, state->backoff_stage, true,
                             false, effective);
    }

    if (state->transport_busy || state->window_open) {
        return make_decision(EWF_SYNC_WINDOW_MERGE, state->backoff_stage, false,
                             false, effective);
    }

    state->window_open = true;
    state->transport_busy = true;
    return make_decision(EWF_SYNC_WINDOW_OPEN, state->backoff_stage, false,
                         false, effective);
}
