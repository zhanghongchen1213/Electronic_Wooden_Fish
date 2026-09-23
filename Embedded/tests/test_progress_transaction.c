/**
 * @file     test_progress_transaction.c
 * @brief    高水位/轮次单事务组纯逻辑主机回归。
 * @details  覆盖积压 999/1000/1001 边界与差值回落恢复、acked_total 单调收敛、
 *           末字完成置位、事务组校验与 scripture 版本 fail-closed 语义；
 *           不依赖 ESP-IDF。
 * @author   ZHC
 * @date     2026-09-23
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

static void set_backlog(ewf_progress_transaction_t *tx, uint32_t backlog)
{
    /* 通过差值构造积压：acked 固定为 0，local 等于目标积压。 */
    tx->acked_total = 0U;
    tx->local_total = backlog;
}

static void test_init_defaults(void)
{
    const ewf_progress_transaction_t tx = fresh();
    assert(tx.schema_version == EWF_PROGRESS_SCHEMA_VERSION);
    assert(tx.local_total == 0U);
    assert(tx.acked_total == 0U);
    assert(tx.round_id == 1U);
    assert(tx.round_cursor == 0U);
    assert(tx.round_state == EWF_PROGRESS_ROUND_STATE_IN_PROGRESS);
    assert(tx.pending_completion == false);
    assert(strcmp(tx.scripture_version, EWF_SCRIPTURE_VERSION) == 0);
    assert(tx.action_id[0] == '\0');
}

static void test_validate_ok(void)
{
    ewf_progress_transaction_t tx = fresh();
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_OK);
}

static void test_validate_schema(void)
{
    ewf_progress_transaction_t tx = fresh();
    tx.schema_version = EWF_PROGRESS_SCHEMA_VERSION + 1U;
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_SCHEMA);
}

static void test_validate_field(void)
{
    ewf_progress_transaction_t tx = fresh();
    tx.acked_total = tx.local_total + 1U;
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_FIELD);

    tx = fresh();
    tx.round_id = 0U;
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_FIELD);

    tx = fresh();
    tx.round_cursor = EWF_SCRIPTURE_CONSUMABLE_COUNT + 1U;
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_FIELD);

    tx = fresh();
    tx.pending_completion = true;
    tx.round_cursor = EWF_SCRIPTURE_CONSUMABLE_COUNT - 1U;
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_FIELD);
}

static void test_validate_version_fail_closed(void)
{
    ewf_progress_transaction_t tx = fresh();
    assert(ewf_progress_transaction_validate(
               &tx, "HS-0.0.1", EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_VERSION);
    tx.scripture_version[0] = '\0';
    assert(ewf_progress_transaction_validate(
               &tx, EWF_SCRIPTURE_VERSION, EWF_SCRIPTURE_CONSUMABLE_COUNT) ==
           EWF_PROGRESS_VALIDATE_VERSION);
}

static void test_backlog_boundaries(void)
{
    ewf_progress_transaction_t tx = fresh();
    set_backlog(&tx, 999U);
    assert(ewf_progress_backlog_count(&tx) == 999U);
    assert(!ewf_progress_backlog_is_full(&tx));

    set_backlog(&tx, 1000U);
    assert(ewf_progress_backlog_count(&tx) == 1000U);
    /* 契约 §12：差值达到 1000 即拒绝。 */
    assert(ewf_progress_backlog_is_full(&tx));

    set_backlog(&tx, 1001U);
    assert(ewf_progress_backlog_count(&tx) == 1001U);
    assert(ewf_progress_backlog_is_full(&tx));
}

static void test_backlog_recovery(void)
{
    ewf_progress_transaction_t tx = fresh();
    set_backlog(&tx, 1000U);
    assert(ewf_progress_backlog_is_full(&tx));
    /* 确认收敛使差值回落后立即恢复接受。 */
    assert(ewf_progress_apply_acked_total(&tx, 1U) == EWF_PROGRESS_ACK_ACCEPTED);
    assert(ewf_progress_backlog_count(&tx) == 999U);
    assert(!ewf_progress_backlog_is_full(&tx));

    ewf_tap_gate_state_t gate = {0};
    ewf_progress_fill_gate(&tx, &gate);
    assert(gate.queue_full == false);
    assert(gate.completed == false);
    assert(gate.fault_locked == false);
}

static void test_apply_valid_tap_and_completion(void)
{
    ewf_progress_transaction_t tx = fresh();
    bool completed_now = false;
    for (uint32_t index = 0; index < EWF_SCRIPTURE_CONSUMABLE_COUNT; ++index)
    {
        const bool expect_completion =
            index + 1U == EWF_SCRIPTURE_CONSUMABLE_COUNT;
        completed_now = ewf_progress_apply_valid_tap(
            &tx, EWF_SCRIPTURE_CONSUMABLE_COUNT);
        assert(tx.local_total == index + 1U);
        assert(tx.round_cursor == index + 1U);
        assert(completed_now == expect_completion);
    }
    assert(tx.pending_completion);
    ewf_tap_gate_state_t gate = {0};
    ewf_progress_fill_gate(&tx, &gate);
    assert(gate.completed);

    /* 完成锁定期间任何输入不计数。 */
    assert(!ewf_progress_apply_valid_tap(&tx, EWF_SCRIPTURE_CONSUMABLE_COUNT));
    assert(tx.local_total == EWF_SCRIPTURE_CONSUMABLE_COUNT);
}

static void test_acked_monotonic(void)
{
    ewf_progress_transaction_t tx = fresh();
    tx.local_total = 10U;
    assert(ewf_progress_apply_acked_total(&tx, 5U) == EWF_PROGRESS_ACK_ACCEPTED);
    assert(tx.acked_total == 5U);
    /* 重复同步的同值确认 no-op。 */
    assert(ewf_progress_apply_acked_total(&tx, 5U) == EWF_PROGRESS_ACK_NOOP);
    assert(tx.acked_total == 5U);
    /* 倒退提交拒绝。 */
    assert(ewf_progress_apply_acked_total(&tx, 4U) == EWF_PROGRESS_ACK_REJECTED);
    assert(tx.acked_total == 5U);
    /* 确认值不可能高于本地已记录值。 */
    assert(ewf_progress_apply_acked_total(&tx, 11U) == EWF_PROGRESS_ACK_REJECTED);
    assert(tx.acked_total == 5U);
}

static void test_gate_projection(void)
{
    ewf_progress_transaction_t tx = fresh();
    set_backlog(&tx, 1000U);
    ewf_tap_gate_state_t gate = {0};
    ewf_progress_fill_gate(&tx, &gate);
    assert(gate.queue_full);
    assert(!gate.completed);
}

int main(void)
{
    test_init_defaults();
    test_validate_ok();
    test_validate_schema();
    test_validate_field();
    test_validate_version_fail_closed();
    test_backlog_boundaries();
    test_backlog_recovery();
    test_apply_valid_tap_and_completion();
    test_acked_monotonic();
    test_gate_projection();
    printf("progress transaction: 全部通过\n");
    return 0;
}
