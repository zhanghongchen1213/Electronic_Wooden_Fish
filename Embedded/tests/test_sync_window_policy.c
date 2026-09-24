/**
 * @file     test_sync_window_policy.c
 * @brief    活动窗口与退避纯逻辑主机测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_window_policy.h"

#include <assert.h>
#include <stdio.h>

static void test_backoff_table_mono(void)
{
    assert(EWF_SYNC_BACKOFF_MS[0] == 5000U);
    assert(EWF_SYNC_BACKOFF_MS[1] == 15000U);
    assert(EWF_SYNC_BACKOFF_MS[2] == 30000U);
    assert(EWF_SYNC_BACKOFF_MS[3] == 60000U);
    assert(ewf_sync_backoff_delay_ms(0) == 5000U);
    assert(ewf_sync_backoff_delay_ms(99) == 60000U);
}

static void test_open_on_backlog(void)
{
    ewf_sync_window_state_t state;
    ewf_sync_window_state_init(&state);
    ewf_sync_window_input_t input = {
        .local_total = 10U,
        .acked_total = 3U,
        .now_ms = 100U,
    };
    ewf_sync_window_decision_t d = ewf_sync_window_policy_apply(&state, &input);
    assert(d.action == EWF_SYNC_WINDOW_OPEN);
    assert(d.accepted_trigger == EWF_SYNC_TRIGGER_BACKLOG);
    assert(state.window_open);
}

static void test_merge_when_busy(void)
{
    ewf_sync_window_state_t state;
    ewf_sync_window_state_init(&state);
    state.window_open = true;
    state.transport_busy = true;
    ewf_sync_window_input_t input = {
        .immediate_pending = true,
        .now_ms = 100U,
    };
    ewf_sync_window_decision_t d = ewf_sync_window_policy_apply(&state, &input);
    assert(d.action == EWF_SYNC_WINDOW_MERGE);
}

static void test_timeout_recovery_after_three(void)
{
    ewf_sync_window_state_t state;
    ewf_sync_window_state_init(&state);
    state.transport_busy = true;
    ewf_sync_window_input_t input = {
        .transport_timeout = true,
        .now_ms = 0U,
    };
    ewf_sync_window_decision_t d;
    d = ewf_sync_window_policy_apply(&state, &input);
    assert(d.action == EWF_SYNC_WINDOW_SKIP || d.enter_recovery == false);
    assert(state.backoff_stage == 0U);

    input.now_ms = 1U;
    state.transport_busy = true;
    d = ewf_sync_window_policy_apply(&state, &input);
    assert(state.backoff_stage == 0U);

    input.now_ms = 2U;
    state.transport_busy = true;
    d = ewf_sync_window_policy_apply(&state, &input);
    assert(d.enter_recovery);
    assert(d.action == EWF_SYNC_WINDOW_CLOSE);
    assert(state.backoff_stage == 1U);
    assert(state.next_eligible_ms == 2U + 15000U);
}

static void test_skip_without_trigger(void)
{
    ewf_sync_window_state_t state;
    ewf_sync_window_state_init(&state);
    ewf_sync_window_input_t input = {
        .local_total = 5U,
        .acked_total = 5U,
        .now_ms = 10U,
    };
    ewf_sync_window_decision_t d = ewf_sync_window_policy_apply(&state, &input);
    assert(d.action == EWF_SYNC_WINDOW_SKIP);
}

static void test_transport_ok_closes_window(void)
{
    ewf_sync_window_state_t state;
    ewf_sync_window_state_init(&state);
    state.window_open = true;
    state.transport_busy = true;
    state.backoff_stage = 2U;
    ewf_sync_window_input_t input = {
        .transport_ok = true,
        .now_ms = 50U,
    };
    ewf_sync_window_decision_t d = ewf_sync_window_policy_apply(&state, &input);
    assert(d.action == EWF_SYNC_WINDOW_SKIP);
    assert(!state.window_open);
    assert(!state.transport_busy);
    assert(state.backoff_stage == 0U);
}

int main(void)
{
    test_backoff_table_mono();
    test_open_on_backlog();
    test_merge_when_busy();
    test_timeout_recovery_after_three();
    test_skip_without_trigger();
    test_transport_ok_closes_window();
    printf("PASS test_sync_window_policy\n");
    return 0;
}
