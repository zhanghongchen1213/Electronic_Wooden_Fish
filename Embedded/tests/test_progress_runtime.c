/**
 * @file     test_progress_runtime.c
 * @brief    高水位/轮次 owner 服务的主机集成回归。
 * @details  编译实际生产 C 源码（progress_service、state_service、watch_state），
 *           仅替代 RTOS、持久化介质和下游总线；验证事件消费接线、恢复序列、
 *           积压 gate 事实发布、确认收敛与持久化失败 typed 上抛。
 * @author   ZHC
 * @date     2026-09-23
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "event_bus.h"
#include "ewf_scripture_canonical.h"
#include "progress_service.h"
#include "progress_store.h"
#include "progress_transaction.h"
#include "state_service.h"
#include "tap_input_service.h"

void host_platform_init(void);
void host_state_owner_start(void);
esp_err_t host_state_apply_one(void);
void host_progress_store_reset(void);
void host_progress_store_fail_save(bool fail);
void host_progress_store_preload(const ewf_progress_transaction_t *tx);
uint32_t host_progress_store_save_count(void);
bool host_progress_store_has_stored(void);
const ewf_progress_transaction_t *host_progress_store_stored(void);
bool host_progress_store_contains_key(const char *key);

static uint32_t s_event_sequence;

static void drain_state_updates(void)
{
    /* 每轮敲击至多产生 progress + gate 两条更新，全部应用到不可变快照。 */
    while (host_state_apply_one() == ESP_OK)
    {
    }
}

static void reset_gate_snapshot(void)
{
    /* 清除上一用例残留的 gate 事实；必须走 owner 入口保证单调序号一致。 */
    assert(state_service_update_tap_gate_owner(false, false, 0) == ESP_OK);
    drain_state_updates();
}

static void reset_owner(void)
{
    host_platform_init();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_progress_store_reset();
    assert(progress_service_init_contracts() == ESP_OK);
    assert(progress_service_prepare_run() == ESP_OK);
    drain_state_updates();
    reset_gate_snapshot();
}

static void restart_owner(void)
{
    /* 模拟重启：重建 owner 内存并从持久化介质恢复；快照序号继续单调。 */
    assert(progress_service_prepare_run() == ESP_OK);
    drain_state_updates();
}

static void submit_valid_event(void)
{
    const legbot_event_t event = {
        .source = LEGBOT_EVENT_SOURCE_STATE,
        .type = LEGBOT_EVENT_TYPE_SIGNAL,
        .code = EWF_TAP_EVENT_VALID,
        .value = ++s_event_sequence,
    };
    assert(xQueueSend(event_bus_queue(), &event, 0) == pdTRUE);
}

static void pump_owner(void)
{
    /* owner 消费至队列排空后停止请求生效退出（主机 stub 非阻塞）。 */
    assert(progress_service_request_stop(0) == ESP_OK);
    assert(progress_service_run() == ESP_OK);
    drain_state_updates();
}

static void ensure_prepared(void)
{
    /* run() 退出后服务框架语义上已回收任务；主机逐轮重建 prepared 状态。 */
    assert(progress_service_init_contracts() == ESP_OK);
    assert(progress_service_prepare_run() == ESP_OK);
    drain_state_updates();
}

static void tap_once(void)
{
    ensure_prepared();
    submit_valid_event();
    pump_owner();
}

static watch_state_snapshot_t snapshot(void)
{
    watch_state_snapshot_t result = {0};
    assert(watch_state_snapshot(&result, 0) == ESP_OK);
    return result;
}

static bool read_gate(bool *queue_full)
{
    bool ready = false;
    bool completed = false;
    bool fault_locked = false;
    assert(state_service_read_tap_gate(&ready, &completed, &fault_locked,
                                       queue_full) == ESP_OK);
    return completed;
}

static bool read_fault_locked(void)
{
    bool ready = false;
    bool completed = false;
    bool fault_locked = false;
    bool queue_full = false;
    assert(state_service_read_tap_gate(&ready, &completed, &fault_locked,
                                       &queue_full) == ESP_OK);
    return fault_locked;
}

static void pump_idle_recovery(void)
{
    /* 空闲超时触发落盘健康探测后退出。 */
    ensure_prepared();
    assert(progress_service_request_stop(0) == ESP_OK);
    assert(progress_service_run() == ESP_OK);
    drain_state_updates();
}

static void test_first_boot_zero_values(void)
{
    reset_owner();
    const watch_state_snapshot_t snap = snapshot();
    assert(snap.tap_local_total == 0U);
    assert(snap.tap_acked_total == 0U);
    assert(snap.tap_round_id == 1U);
    assert(snap.tap_round_cursor == 0U);
    assert(snap.tap_round_state == (uint32_t)EWF_PROGRESS_ROUND_STATE_IN_PROGRESS);
    assert(snap.tap_pending_completion == false);
    assert(snap.tap_backlog_count == 0U);
    /* 首启零值初始事务组已落盘。 */
    assert(host_progress_store_save_count() == 1U);
    assert(host_progress_store_has_stored());
}

static void test_recovery_roundtrip(void)
{
    reset_owner();
    for (uint32_t index = 0; index < 3U; ++index)
    {
        tap_once();
    }
    const ewf_progress_transaction_t *stored = host_progress_store_stored();
    assert(stored->local_total == 3U);
    assert(stored->round_cursor == 3U);
    assert(stored->acked_total == 0U);

    restart_owner();
    const watch_state_snapshot_t snap = snapshot();
    assert(snap.tap_local_total == 3U);
    assert(snap.tap_round_cursor == 3U);
    assert(snap.tap_round_id == 1U);
    /* 重启后继续推进，高水位与游标不重置、不重复。 */
    tap_once();
    assert(snapshot().tap_local_total == 4U);
    assert(snapshot().tap_round_cursor == 4U);
}

static void test_scripture_version_fail_closed(void)
{
    host_platform_init();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_progress_store_reset();
    ewf_progress_transaction_t corrupt = {0};
    ewf_progress_transaction_init(&corrupt, EWF_SCRIPTURE_VERSION);
    (void)strcpy(corrupt.scripture_version, "HS-0.0.1");
    corrupt.local_total = 77U;
    host_progress_store_preload(&corrupt);
    assert(progress_service_init_contracts() == ESP_OK);
    assert(progress_service_prepare_run() == ESP_OK);
    drain_state_updates();

    tap_once();
    /* 版本不一致属配置错误：fail-closed，不推进、不覆盖介质、不静默回退。 */
    assert(snapshot().tap_local_total == 77U);
    assert(host_progress_store_stored()->local_total == 77U);
    assert(progress_service_advance_acked_total(78U, 0) == ESP_ERR_INVALID_STATE);
}

static void test_backlog_boundary_and_recovery(void)
{
    reset_owner();
    /* 直接预置积压 999，一次敲击到达 1000 阈值。 */
    ewf_progress_transaction_t preload = {0};
    ewf_progress_transaction_init(&preload, EWF_SCRIPTURE_VERSION);
    preload.local_total = 999U;
    host_progress_store_preload(&preload);
    restart_owner();

    tap_once();
    watch_state_snapshot_t snap = snapshot();
    assert(snap.tap_local_total == 1000U);
    assert(snap.tap_backlog_count == 1000U);

    /* 达到 1000 后统一 gate 拒绝：owner 兜底忽略 + 快照 queue_full 事实。 */
    tap_once();
    snap = snapshot();
    assert(snap.tap_local_total == 1000U);
    assert(snap.tap_backlog_count == 1000U);
    bool queue_full = false;
    const bool completed = read_gate(&queue_full);
    assert(!completed);
    assert(queue_full);

    /* 确认收敛使差值回落后立即恢复接受。 */
    ensure_prepared();
    assert(progress_service_advance_acked_total(1U, 0) == ESP_OK);
    drain_state_updates();
    queue_full = false;
    assert(!read_gate(&queue_full));
    tap_once();
    snap = snapshot();
    assert(snap.tap_local_total == 1001U);
    assert(snap.tap_acked_total == 1U);
    assert(snap.tap_backlog_count == 1000U);
    /* 重新达到上限：再次拒绝。 */
    tap_once();
    assert(snapshot().tap_local_total == 1001U);
}

static void test_end_of_round_pending_completion(void)
{
    reset_owner();
    ewf_progress_transaction_t preload = {0};
    ewf_progress_transaction_init(&preload, EWF_SCRIPTURE_VERSION);
    preload.local_total = (uint32_t)EWF_SCRIPTURE_CONSUMABLE_COUNT - 1U;
    preload.round_cursor = (uint32_t)EWF_SCRIPTURE_CONSUMABLE_COUNT - 1U;
    host_progress_store_preload(&preload);
    restart_owner();

    tap_once();
    watch_state_snapshot_t snap = snapshot();
    assert(snap.tap_round_cursor == (uint32_t)EWF_SCRIPTURE_CONSUMABLE_COUNT);
    assert(snap.tap_pending_completion);
    bool queue_full = false;
    assert(read_gate(&queue_full));
    /* 本地完成锁定期间输入经统一 gate 忽略，不计数。 */
    assert(read_gate(&queue_full));
    tap_once();
    snap = snapshot();
    assert(snap.tap_local_total == (uint32_t)EWF_SCRIPTURE_CONSUMABLE_COUNT);
    assert(snap.tap_pending_completion);
}

static void test_acked_advance_contract(void)
{
    reset_owner();
    for (uint32_t index = 0; index < 5U; ++index)
    {
        tap_once();
    }
    assert(snapshot().tap_local_total == 5U);
    ensure_prepared();
    assert(progress_service_advance_acked_total(3U, 0) == ESP_OK);
    drain_state_updates();
    assert(snapshot().tap_acked_total == 3U);
    /* 重复同步的同值确认 no-op：确认值不变。 */
    assert(progress_service_advance_acked_total(3U, 0) == ESP_OK);
    assert(snapshot().tap_acked_total == 3U);
    /* 倒退提交拒绝。 */
    assert(progress_service_advance_acked_total(2U, 0) == ESP_ERR_INVALID_ARG);
    assert(snapshot().tap_acked_total == 3U);
    tap_once();
    assert(snapshot().tap_acked_total == 3U);
    assert(snapshot().tap_local_total == 6U);
}

static void test_persist_failure_typed_and_no_silent_loss(void)
{
    reset_owner();
    tap_once();
    assert(snapshot().tap_local_total == 1U);
    host_progress_store_fail_save(true);
    tap_once();
    /* 落盘失败：内存与介质一致地保持旧高水位，不静默丢失也不伪造推进。 */
    watch_state_snapshot_t snap = snapshot();
    assert(snap.tap_local_total == 1U);
    assert(snap.tap_persist_error);
    bool pending = false;
    bool inflight = false;
    assert(progress_service_get_persist_status(&pending, &inflight) == ESP_OK);
    assert(pending);
    assert(!inflight);
    host_progress_store_fail_save(false);
    tap_once();
    snap = snapshot();
    assert(snap.tap_local_total == 2U);
    assert(!snap.tap_persist_error);
    assert(progress_service_get_persist_status(&pending, &inflight) == ESP_OK);
    assert(!pending);
}

static void test_fault_lock_after_persist_failures_and_idle_unlock(void)
{
    reset_owner();
    tap_once();
    assert(!read_fault_locked());

    /* 同一 prepared 会话内连续三次落盘失败，避免 prepare_run 复位 fault 计数。 */
    host_progress_store_fail_save(true);
    ensure_prepared();
    submit_valid_event();
    submit_valid_event();
    submit_valid_event();
    pump_owner();
    assert(read_fault_locked());
    bool pending = false;
    bool inflight = false;
    assert(progress_service_get_persist_status(&pending, &inflight) == ESP_OK);
    assert(pending);

    /* gate owner 更新 completed/queue_full 不得清掉 fault_locked。 */
    assert(state_service_update_tap_gate_owner(true, false, 0) == ESP_OK);
    drain_state_updates();
    assert(read_fault_locked());
    bool ready = false;
    bool completed = false;
    bool fault_locked = false;
    bool queue_full = false;
    assert(state_service_read_tap_gate(&ready, &completed, &fault_locked,
                                       &queue_full) == ESP_OK);
    assert(completed);

    /* 介质恢复后空闲探测成功落盘 → unlock + clear pending。 */
    host_progress_store_fail_save(false);
    pump_idle_recovery();
    assert(!read_fault_locked());
    assert(progress_service_get_persist_status(&pending, &inflight) == ESP_OK);
    assert(!pending);
}

static void test_write_set_has_no_automatic_mode_flag(void)
{
    reset_owner();
    tap_once();
    /* AC 2：持久化写集不含自动模式标志；写集 key 名单固定且无 auto 字段。 */
    assert(!host_progress_store_contains_key("automatic_mode"));
    assert(!host_progress_store_contains_key("auto"));
    const ewf_progress_transaction_t *stored = host_progress_store_stored();
    assert(strstr(stored->scripture_version, "auto") == NULL);
    assert(stored->action_id[0] == '\0');
}

int main(void)
{
    test_first_boot_zero_values();
    test_recovery_roundtrip();
    test_scripture_version_fail_closed();
    test_backlog_boundary_and_recovery();
    test_end_of_round_pending_completion();
    test_acked_advance_contract();
    test_persist_failure_typed_and_no_silent_loss();
    test_fault_lock_after_persist_failures_and_idle_unlock();
    test_write_set_has_no_automatic_mode_flag();
    printf("progress runtime: 全部通过\n");
    return 0;
}
