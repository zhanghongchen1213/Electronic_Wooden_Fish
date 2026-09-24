/**
 * @file     test_sync_response_policy.c
 * @brief    同步响应收敛纯逻辑主机测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_response_policy.h"

#include <assert.h>
#include <stdio.h>

static void test_advance_and_noop(void)
{
    ewf_sync_response_input_t input = {
        .code = 0,
        .request_acked_total = 10U,
        .response_acked_total = 15U,
        .command_revision = 1U,
        .applied_revision = 0U,
        .transport_ok = true,
    };
    ewf_sync_response_decision_t d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_ADVANCE_ACKED);
    assert(d.new_acked_total == 15U);
    assert(d.mark_synced);
    assert(d.apply_pending_command);

    input.response_acked_total = 10U;
    d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_NOOP);
    assert(d.mark_synced);

    input.response_acked_total = 8U;
    d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_NOOP);
}

static void test_conflict_and_version(void)
{
    ewf_sync_response_input_t input = {
        .code = EWF_SYNC_CODE_DEVICE_RESET_CONFLICT,
        .request_acked_total = 1U,
        .response_acked_total = 99U,
        .transport_ok = true,
    };
    ewf_sync_response_decision_t d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_CONFLICT);
    assert(d.mark_conflict);
    assert(!d.mark_synced);

    input.code = EWF_SYNC_CODE_SCRIPTURE_MISMATCH;
    d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_STOP_PROGRESS);
    assert(d.mark_sync_failed);
}

static void test_20004_ack_with_gate(void)
{
    ewf_sync_response_input_t input = {
        .code = EWF_SYNC_CODE_PENDING_COMPLETION,
        .request_acked_total = 1U,
        .response_acked_total = 5U,
        .transport_ok = true,
    };
    ewf_sync_response_decision_t d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_ACK_WITH_GATE);
    assert(d.mark_synced);
}

static void test_transport_fail_not_synced(void)
{
    ewf_sync_response_input_t input = {.transport_ok = false};
    ewf_sync_response_decision_t d = ewf_sync_response_policy_apply(&input);
    assert(d.action == EWF_SYNC_RESPONSE_TRANSPORT_FAIL);
    assert(!d.mark_synced);
    assert(d.mark_sync_failed);
}

int main(void)
{
    test_advance_and_noop();
    test_conflict_and_version();
    test_20004_ack_with_gate();
    test_transport_fail_not_synced();
    printf("PASS test_sync_response_policy\n");
    return 0;
}
