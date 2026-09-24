/**
 * @file     sync_response_policy.c
 * @brief    同步响应收敛纯逻辑实现。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "sync_response_policy.h"

#include <stddef.h>

ewf_sync_response_decision_t ewf_sync_response_policy_apply(
    const ewf_sync_response_input_t *input)
{
    ewf_sync_response_decision_t decision = {
        .action = EWF_SYNC_RESPONSE_TRANSPORT_FAIL,
        .new_acked_total = 0U,
        .apply_pending_command = false,
        .mark_synced = false,
        .mark_pending_sync = false,
        .mark_sync_failed = true,
        .mark_conflict = false,
    };

    if (input == NULL || !input->transport_ok) {
        return decision;
    }

    decision.mark_sync_failed = false;

    if (input->code == EWF_SYNC_CODE_DEVICE_RESET_CONFLICT) {
        decision.action = EWF_SYNC_RESPONSE_CONFLICT;
        decision.mark_conflict = true;
        decision.mark_pending_sync = true;
        return decision;
    }

    if (input->code == EWF_SYNC_CODE_SCRIPTURE_MISMATCH) {
        decision.action = EWF_SYNC_RESPONSE_STOP_PROGRESS;
        decision.mark_sync_failed = true;
        return decision;
    }

    if (input->code != EWF_SYNC_CODE_OK &&
        input->code != EWF_SYNC_CODE_PENDING_COMPLETION) {
        decision.action = EWF_SYNC_RESPONSE_REJECT;
        decision.mark_sync_failed = true;
        decision.mark_pending_sync = true;
        return decision;
    }

    if (input->response_acked_total < input->request_acked_total) {
        decision.action = EWF_SYNC_RESPONSE_NOOP;
        decision.mark_synced = true;
    } else if (input->response_acked_total == input->request_acked_total) {
        decision.action = EWF_SYNC_RESPONSE_NOOP;
        decision.mark_synced = true;
    } else {
        decision.action = (input->code == EWF_SYNC_CODE_PENDING_COMPLETION)
                              ? EWF_SYNC_RESPONSE_ACK_WITH_GATE
                              : EWF_SYNC_RESPONSE_ADVANCE_ACKED;
        decision.new_acked_total = input->response_acked_total;
        decision.mark_synced = true;
    }

    if (input->command_revision > input->applied_revision) {
        decision.apply_pending_command = true;
    }

    return decision;
}
