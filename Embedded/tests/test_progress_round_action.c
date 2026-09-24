/**
 * @file     test_progress_round_action.c
 * @brief    跨轮 restart/exit 与完成锁定 gate 主机回归（Story 3.6）。
 * @author   ZHC
 * @date     2026-09-24
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ewf_scripture_canonical.h"
#include "progress_transaction.h"

static ewf_progress_transaction_t fresh(void)
{
    ewf_progress_transaction_t tx = {0};
    ewf_progress_transaction_init(&tx, EWF_SCRIPTURE_VERSION);
    return tx;
}

static void reach_completion(ewf_progress_transaction_t *tx)
{
    for (uint32_t i = 0U; i < EWF_SCRIPTURE_CONSUMABLE_COUNT; ++i) {
        (void)ewf_progress_apply_valid_tap(tx, EWF_SCRIPTURE_CONSUMABLE_COUNT);
    }
    assert(tx->pending_completion);
    assert(tx->round_state == EWF_PROGRESS_ROUND_STATE_COMPLETED);
}

static void test_gate_pending_or_completed(void)
{
    ewf_progress_transaction_t tx = fresh();
    ewf_tap_gate_state_t gate = {0};

    ewf_progress_fill_gate(&tx, &gate);
    assert(!gate.completed);

    reach_completion(&tx);
    ewf_progress_fill_gate(&tx, &gate);
    assert(gate.completed);

    /* exit：清 pending，gate 仍靠 round_state=completed 冻结。 */
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_EXIT,
                                           "exit-1",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_ACCEPTED);
    assert(!tx.pending_completion);
    assert(tx.round_state == EWF_PROGRESS_ROUND_STATE_COMPLETED);
    ewf_progress_fill_gate(&tx, &gate);
    assert(gate.completed);
    assert(!ewf_progress_apply_valid_tap(&tx, EWF_SCRIPTURE_CONSUMABLE_COUNT));
}

static void test_restart_new_round_keeps_totals(void)
{
    ewf_progress_transaction_t tx = fresh();
    reach_completion(&tx);
    const uint32_t local = tx.local_total;
    const uint32_t acked = tx.acked_total;
    const uint32_t old_round = tx.round_id;

    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_RESTART,
                                           "restart-1",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_ACCEPTED);
    assert(tx.round_id == old_round + 1U);
    assert(tx.round_cursor == 0U);
    assert(tx.round_state == EWF_PROGRESS_ROUND_STATE_IN_PROGRESS);
    assert(!tx.pending_completion);
    assert(tx.local_total == local);
    assert(tx.acked_total == acked);

    ewf_tap_gate_state_t gate = {0};
    ewf_progress_fill_gate(&tx, &gate);
    assert(!gate.completed);
}

static void test_idempotent_same_action_id(void)
{
    ewf_progress_transaction_t tx = fresh();
    reach_completion(&tx);
    const uint32_t round_before = tx.round_id;

    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_RESTART,
                                           "same-id",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_ACCEPTED);
    assert(tx.round_id == round_before + 1U);

    /* 同 id 重放：不二次 +1。 */
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_RESTART,
                                           "same-id",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_IDEMPOTENT);
    assert(tx.round_id == round_before + 1U);
}

static void test_reject_when_not_locked(void)
{
    ewf_progress_transaction_t tx = fresh();
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_RESTART,
                                           "early",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_REJECTED);
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_EXIT,
                                           "early-exit",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_REJECTED);
}

static void test_overflow_reject(void)
{
    ewf_progress_transaction_t tx = fresh();
    reach_completion(&tx);
    tx.round_id = UINT32_MAX;
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_RESTART,
                                           "overflow",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_REJECTED);
    assert(tx.round_id == UINT32_MAX);
}

static void test_same_id_cross_action_rejected(void)
{
    ewf_progress_transaction_t tx = fresh();
    reach_completion(&tx);
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_EXIT,
                                           "collide-id",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_ACCEPTED);
    /* 同键改 restart：状态仍为 completed，不得误报 IDEMPOTENT。 */
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_RESTART,
                                           "collide-id",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_REJECTED);
    assert(tx.round_state == EWF_PROGRESS_ROUND_STATE_COMPLETED);
}

static void test_exit_idempotent_replay(void)
{
    ewf_progress_transaction_t tx = fresh();
    reach_completion(&tx);
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_EXIT,
                                           "exit-idem",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_ACCEPTED);
    assert(ewf_progress_apply_round_action(&tx,
                                           EWF_PROGRESS_ROUND_ACTION_EXIT,
                                           "exit-idem",
                                           EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_ROUND_ACTION_IDEMPOTENT);
}

int main(void)
{
    test_gate_pending_or_completed();
    test_restart_new_round_keeps_totals();
    test_idempotent_same_action_id();
    test_reject_when_not_locked();
    test_overflow_reject();
    test_same_id_cross_action_rejected();
    test_exit_idempotent_replay();
    printf("progress round_action: 全部通过\n");
    return 0;
}
