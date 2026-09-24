/**
 * @file     progress_transaction.c
 * @brief    本地高水位/轮次单事务组的纯逻辑实现。
 * @details  owner 是唯一累计入口：每个有效事件只推进一次 local_total 与可消费汉字
 *           游标；acked_total 只能单调推进；末字置位 pending_completion 并本地确认
 *           round_state=completed（Story 3.6 裁决 B）；写集不含自动模式标志。
 *           gate.completed = pending || round_state==completed（裁决 A）。
 *           跨轮 restart/exit 经 ewf_progress_apply_round_action（裁决 C–F）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "progress_transaction.h"

#include <string.h>

/** 事务组字段容量检查使用的内部工具。 */
static void copy_bounded(char *dst, size_t capacity, const char *src);

void ewf_progress_transaction_init(ewf_progress_transaction_t *tx,
                                   const char *scripture_version)
{
    if (tx == NULL)
    {
        return;
    }
    memset(tx, 0, sizeof(*tx));
    tx->schema_version = EWF_PROGRESS_SCHEMA_VERSION;
    tx->local_total = 0U;
    tx->acked_total = 0U;
    tx->round_id = 1U;
    tx->round_cursor = 0U;
    tx->round_state = EWF_PROGRESS_ROUND_STATE_IN_PROGRESS;
    tx->pending_completion = false;
    (void)copy_bounded(tx->scripture_version,
                       sizeof(tx->scripture_version),
                       scripture_version != NULL ? scripture_version : "");
    tx->action_id[0] = '\0';
}

ewf_progress_validate_result_t ewf_progress_transaction_validate(
    const ewf_progress_transaction_t *tx,
    const char *expected_scripture_version,
    uint32_t consumable_count)
{
    if (tx == NULL || expected_scripture_version == NULL)
    {
        return EWF_PROGRESS_VALIDATE_NULL;
    }
    if (consumable_count == 0U)
    {
        return EWF_PROGRESS_VALIDATE_FIELD;
    }
    if (tx->schema_version != EWF_PROGRESS_SCHEMA_VERSION)
    {
        return EWF_PROGRESS_VALIDATE_SCHEMA;
    }
    if (tx->acked_total > tx->local_total ||
        tx->round_id == 0U ||
        (unsigned)tx->round_state > (unsigned)EWF_PROGRESS_ROUND_STATE_COMPLETED)
    {
        return EWF_PROGRESS_VALIDATE_FIELD;
    }
    /* 游标必须停在可消费序列内；超出即高水位、轮次与游标不匹配。 */
    if (tx->round_cursor > consumable_count)
    {
        return EWF_PROGRESS_VALIDATE_FIELD;
    }
    /* 完成锁定置位时游标必须已覆盖全部可消费汉字。 */
    if (tx->pending_completion && tx->round_cursor != consumable_count)
    {
        return EWF_PROGRESS_VALIDATE_FIELD;
    }
    if (strncmp(tx->scripture_version,
                expected_scripture_version,
                sizeof(tx->scripture_version)) != 0)
    {
        return EWF_PROGRESS_VALIDATE_VERSION;
    }
    return EWF_PROGRESS_VALIDATE_OK;
}

uint32_t ewf_progress_backlog_count(const ewf_progress_transaction_t *tx)
{
    if (tx == NULL || tx->acked_total > tx->local_total)
    {
        return 0U;
    }
    return tx->local_total - tx->acked_total;
}

bool ewf_progress_backlog_is_full(const ewf_progress_transaction_t *tx)
{
    /* 契约 §12：差值达到 1000 即拒绝，不是“大于 1000”。 */
    return ewf_progress_backlog_count(tx) >= EWF_PROGRESS_BACKLOG_LIMIT;
}

bool ewf_progress_apply_valid_tap(ewf_progress_transaction_t *tx,
                                  uint32_t consumable_count)
{
    if (tx == NULL || consumable_count == 0U)
    {
        return false;
    }
    /* 完成锁定期间不再推进（pending 或 round_state=completed）；与 gate 双保险。 */
    if (ewf_progress_is_completion_locked(tx))
    {
        return false;
    }
    if (tx->round_cursor >= consumable_count)
    {
        return false;
    }
    ++tx->local_total;
    ++tx->round_cursor;
    if (tx->round_cursor >= consumable_count)
    {
        /*
         * 裁决 B：末字本地可见完成 → round_state=completed 且保持 pending=true，
         * 直至 sync 确认或用户 exit/restart 显式清理；gate 靠 pending||completed。
         */
        tx->pending_completion = true;
        tx->round_state = EWF_PROGRESS_ROUND_STATE_COMPLETED;
        return true;
    }
    return false;
}

ewf_progress_ack_result_t ewf_progress_apply_acked_total(
    ewf_progress_transaction_t *tx,
    uint32_t new_acked_total)
{
    if (tx == NULL)
    {
        return EWF_PROGRESS_ACK_REJECTED;
    }
    if (new_acked_total == tx->acked_total)
    {
        /* 重复同步的同值确认是 no-op，不得重复计数（AD-3）。 */
        return EWF_PROGRESS_ACK_NOOP;
    }
    if (new_acked_total < tx->acked_total || new_acked_total > tx->local_total)
    {
        /* 倒退提交被拒绝；确认值不可能高于本地已记录值。 */
        return EWF_PROGRESS_ACK_REJECTED;
    }
    tx->acked_total = new_acked_total;
    return EWF_PROGRESS_ACK_ACCEPTED;
}

bool ewf_progress_is_completion_locked(const ewf_progress_transaction_t *tx)
{
    if (tx == NULL)
    {
        return false;
    }
    /* 裁决 A：完成锁定 = pending || round_state==completed。 */
    return tx->pending_completion ||
           tx->round_state == EWF_PROGRESS_ROUND_STATE_COMPLETED;
}

void ewf_progress_fill_gate(const ewf_progress_transaction_t *tx,
                            ewf_tap_gate_state_t *gate)
{
    if (gate == NULL)
    {
        return;
    }
    gate->service_ready = true;
    gate->completed = ewf_progress_is_completion_locked(tx);
    gate->fault_locked = false;
    gate->queue_full = ewf_progress_backlog_is_full(tx);
}

ewf_progress_round_action_result_t ewf_progress_apply_round_action(
    ewf_progress_transaction_t *tx,
    ewf_progress_round_action_t action,
    const char *action_id,
    uint32_t consumable_count)
{
    (void)consumable_count;
    if (tx == NULL || action_id == NULL || action_id[0] == '\0' ||
        strlen(action_id) >= EWF_PROGRESS_ACTION_ID_CAPACITY)
    {
        return EWF_PROGRESS_ROUND_ACTION_REJECTED;
    }
    if (action != EWF_PROGRESS_ROUND_ACTION_RESTART &&
        action != EWF_PROGRESS_ROUND_ACTION_EXIT)
    {
        return EWF_PROGRESS_ROUND_ACTION_REJECTED;
    }

    /* 同 action_id 短路径：仅当当前状态已与该动作首次结果一致才幂等；
     * 否则视为跨动作碰撞或陈旧键，拒绝（避免重启后序号归零撞键误吞）。 */
    if (strncmp(tx->action_id, action_id, sizeof(tx->action_id)) == 0)
    {
        if (action == EWF_PROGRESS_ROUND_ACTION_RESTART &&
            tx->round_state == EWF_PROGRESS_ROUND_STATE_IN_PROGRESS &&
            !tx->pending_completion)
        {
            return EWF_PROGRESS_ROUND_ACTION_IDEMPOTENT;
        }
        if (action == EWF_PROGRESS_ROUND_ACTION_EXIT &&
            tx->round_state == EWF_PROGRESS_ROUND_STATE_COMPLETED &&
            !tx->pending_completion)
        {
            return EWF_PROGRESS_ROUND_ACTION_IDEMPOTENT;
        }
        return EWF_PROGRESS_ROUND_ACTION_REJECTED;
    }

    if (!ewf_progress_is_completion_locked(tx))
    {
        /* 裁决 D：未完成锁定时 restart/exit 均拒绝（对齐 5.2）。 */
        return EWF_PROGRESS_ROUND_ACTION_REJECTED;
    }

    if (action == EWF_PROGRESS_ROUND_ACTION_RESTART)
    {
        /* 裁决 F：UINT32_MAX 拒绝，不回绕到 0/1。 */
        if (tx->round_id == UINT32_MAX)
        {
            return EWF_PROGRESS_ROUND_ACTION_REJECTED;
        }
        ++tx->round_id;
        tx->round_cursor = 0U;
        tx->round_state = EWF_PROGRESS_ROUND_STATE_IN_PROGRESS;
        tx->pending_completion = false;
        copy_bounded(tx->action_id, sizeof(tx->action_id), action_id);
        /* 裁决 H/AD-3：不清零 local_total/acked_total；不触碰 auto_mode。 */
        return EWF_PROGRESS_ROUND_ACTION_ACCEPTED;
    }

    /* exit：保留 completed 与当前 round_id/cursor；清 pending；累计保留。 */
    tx->round_state = EWF_PROGRESS_ROUND_STATE_COMPLETED;
    tx->pending_completion = false;
    copy_bounded(tx->action_id, sizeof(tx->action_id), action_id);
    return EWF_PROGRESS_ROUND_ACTION_ACCEPTED;
}

static void copy_bounded(char *dst, size_t capacity, const char *src)
{
    if (dst == NULL || capacity == 0U)
    {
        return;
    }
    if (src == NULL || strlen(src) >= capacity)
    {
        dst[0] = '\0';
        return;
    }
    /* 容量已验证，拷贝含终止符安全。 */
    size_t index = 0U;
    while (src[index] != '\0' && index + 1U < capacity)
    {
        dst[index] = src[index];
        ++index;
    }
    dst[index] = '\0';
}
