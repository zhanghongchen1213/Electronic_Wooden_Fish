/**
 * @file     progress_transaction.c
 * @brief    本地高水位/轮次单事务组的纯逻辑实现。
 * @details  owner 是唯一累计入口：每个有效事件只推进一次 local_total 与可消费汉字
 *           游标；acked_total 只能单调推进；末字置位 pending_completion；写集不含
 *           自动模式标志。所有判定不硬编码经文字数，取值域由调用方传入 canonical
 *           可消费总数（EWF_SCRIPTURE_CONSUMABLE_COUNT）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "progress_transaction.h"

#include <string.h>

/** 事务组字段容量与版本字符串长度检查使用的内部工具。 */
static bool string_fits(const char *scripture_version);
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
    /* 完成锁定期间不再推进；gate 层已忽略输入，这里保持防御一致。 */
    if (tx->pending_completion)
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
        /* 到达末字：本地完成锁定置位；完成确认与新轮次属 Story 3.6/5.2 域。 */
        tx->pending_completion = true;
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

void ewf_progress_fill_gate(const ewf_progress_transaction_t *tx,
                            ewf_tap_gate_state_t *gate)
{
    if (gate == NULL)
    {
        return;
    }
    gate->service_ready = true;
    gate->completed = tx != NULL && tx->pending_completion;
    gate->fault_locked = false;
    gate->queue_full = ewf_progress_backlog_is_full(tx);
}

static bool string_fits(const char *scripture_version)
{
    return scripture_version != NULL &&
           strlen(scripture_version) < EWF_PROGRESS_SCRIPTURE_VERSION_CAPACITY;
}

static void copy_bounded(char *dst, size_t capacity, const char *src)
{
    if (dst == NULL || capacity == 0U)
    {
        return;
    }
    if (!string_fits(src))
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
