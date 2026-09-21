/**
 * @file     control_gate.c
 * @brief    首次解锁与普通控制唯一准入状态机实现
 * @details  纯 typed reducer 串行绑定请求身份，并隔离 no-op、写入与未确认普通控制。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "control_gate.h"

#include <math.h>
#include <string.h>

/** control_gate 初始稳定码。 */
#define CONTROL_GATE_AUTH_REQUIRED_CODE "CLOUD_AUTH_REQUIRED"
/** control_gate 查询中稳定码。 */
#define CONTROL_GATE_QUERYING_CODE "CLOUD_RENTAL_QUERYING"
/** control_gate 等待支付复查稳定码。 */
#define CONTROL_GATE_WAITING_PAYMENT_CODE "CLOUD_RENTAL_UNPAID"
/** control_gate 等待网络退避稳定码。 */
#define CONTROL_GATE_RETRY_WAIT_CODE "CLOUD_RETRY_WAIT"
/** control_gate paid 终态稳定码。 */
#define CONTROL_GATE_PAID_CODE "CLOUD_RENTAL_PAID"
/** BLE 一次性写入进行中稳定码。 */
#define CONTROL_GATE_UNLOCK_WRITING_CODE "BLE_UNLOCK_WRITING"
/** BLE 解锁状态证据确认中稳定码。 */
#define CONTROL_GATE_UNLOCK_CONFIRMING_CODE "BLE_UNLOCK_CONFIRMING"
/** BLE 解锁闭环成功稳定码。 */
#define CONTROL_GATE_UNLOCKED_CODE "BLE_UNLOCKED"
/** BLE 链路变化取消事务稳定码。 */
#define CONTROL_GATE_LINK_CHANGED_CODE "BLE_UNLOCK_LINK_CHANGED"
/** BLE 写入失败稳定码。 */
#define CONTROL_GATE_WRITE_FAILED_CODE "BLE_UNLOCK_WRITE_FAILED"
/** BLE 状态证据超时稳定码。 */
#define CONTROL_GATE_CONFIRM_TIMEOUT_CODE "BLE_UNLOCK_CONFIRM_TIMEOUT"
/** 普通控制 no-op 稳定码。 */
#define CONTROL_GATE_CONTROL_NO_OP_CODE "BLE_CONTROL_NO_OP"
/** 普通控制初始写入中稳定码。 */
#define CONTROL_GATE_CONTROL_WRITING_CODE "BLE_CONTROL_WRITING"
/** 普通控制写 procedure 已接受但未确认稳定码。 */
#define CONTROL_GATE_CONTROL_SUBMITTED_CODE "BLE_CONTROL_SUBMITTED_UNCONFIRMED"
/** 普通控制写入被拒绝稳定码。 */
#define CONTROL_GATE_CONTROL_WRITE_REJECTED_CODE "BLE_CONTROL_WRITE_REJECTED"
/** 普通控制状态证据已确认稳定码。 */
#define CONTROL_GATE_CONTROL_APPLIED_CODE "BLE_CONTROL_APPLIED"
/** 普通控制因链路事实变化而结束稳定码。 */
#define CONTROL_GATE_CONTROL_BLOCKED_CODE "BLE_CONTROL_BLOCKED"
/** 普通控制正在重试稳定码。 */
#define CONTROL_GATE_CONTROL_RETRYING_CODE "BLE_CONTROL_RETRYING"
/** 普通控制三次失败锁止稳定码。 */
#define CONTROL_GATE_CONTROL_RETRY_EXHAUSTED_CODE \
    "BLE_CONTROL_RETRY_EXHAUSTED"
/** 一次性关机等待产品结果稳定码。 */
#define CONTROL_GATE_POWEROFF_PENDING_CODE "BLE_POWEROFF_PENDING"
/** 普通控制真实重连后解除锁止稳定码。 */
#define CONTROL_GATE_CONTROL_RECONNECTED_CODE "BLE_CONTROL_RECONNECTED"
/** 关机窗口内观察到离线证据稳定码。 */
#define CONTROL_GATE_POWEROFF_ASSUMED_OFF_CODE "BLE_POWEROFF_ASSUMED_OFF"
/** 关机结果窗口到期且证据不足稳定码。 */
#define CONTROL_GATE_POWEROFF_TIMEOUT_UNKNOWN_CODE \
    "BLE_POWEROFF_TIMEOUT_UNKNOWN"
/** 关机后同一设备仍在线稳定码。 */
#define CONTROL_GATE_POWEROFF_FAILED_RECONNECTED_CODE \
    "BLE_POWEROFF_FAILED_RECONNECTED"

static void clear_action(control_gate_action_t *action);
static void set_action(control_gate_action_t *action,
                       control_gate_action_type_t type,
                       const control_gate_t *gate);
static void clear_control_action(control_gate_control_action_t *action);
static void set_control_action(control_gate_control_action_t *action,
                               const control_gate_t *gate);
static void make_update(control_gate_t *gate,
                        watch_rental_phase_t phase,
                        watch_control_block_reason_t block_reason,
                        watch_rental_result_t result,
                        bool pending,
                        bool unlocked,
                        const char *error_code,
                        watch_control_update_t *update);
static void make_control_update(control_gate_t *gate,
                                watch_control_phase_t phase,
                                watch_control_result_t result,
                                bool pending,
                                const char *error_code,
                                watch_control_update_t *update);
static watch_control_block_reason_t admission_block_reason(
    const watch_state_snapshot_t *snapshot,
    uint32_t link_generation);
static bool transaction_matches(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation);
static bool rental_phase_accepts_result(watch_rental_phase_t phase);
static bool rental_result_is_transient(
    const control_gate_rental_result_t *result);
static watch_control_block_reason_t rental_block_reason(
    watch_rental_result_t result);
static const char *rental_error_code(const control_gate_rental_result_t *result);
static bool control_intent_is_valid(const control_gate_control_intent_t *intent);
static bool control_snapshot_is_valid(const watch_state_snapshot_t *snapshot);
static watch_control_kind_t control_kind_for_request(
    const watch_state_snapshot_t *snapshot,
    const control_gate_control_intent_t *intent);
static bool control_target_is_no_op(
    const watch_state_snapshot_t *snapshot,
    const control_gate_control_intent_t *intent);
static uint8_t protocol_gear(float gear);
static bool control_transaction_matches(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_control_kind_t kind,
    uint8_t target);
static bool status_sequence_is_after(uint32_t candidate, uint32_t baseline);
static bool actual_state_matches_target(
    const watch_exoskeleton_state_t *actual_state,
    watch_control_kind_t kind,
    uint8_t target);
static bool poweroff_request_matches(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY]);
static void finish_poweroff(control_gate_t *gate,
                            watch_poweroff_result_t result,
                            uint32_t observed_link_generation,
                            bool preserve_session,
                            const char *error_code,
                            watch_control_update_t *update);

void control_gate_init(control_gate_t *gate)
{
    if (gate == NULL)
    {
        return;
    }
    memset(gate, 0, sizeof(*gate));
    gate->phase = WATCH_RENTAL_PHASE_WAITING_AUTH;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
}

control_gate_request_status_t control_gate_request_first_unlock(
    control_gate_t *gate,
    const watch_state_snapshot_t *snapshot,
    uint32_t link_generation,
    control_gate_action_t *action,
    watch_control_update_t *update)
{
    clear_action(action);
    if (gate == NULL || snapshot == NULL || action == NULL || update == NULL)
    {
        return CONTROL_GATE_BLOCKED;
    }
    if (gate->pending || gate->control_pending || snapshot->pending_control)
    {
        watch_control_block_reason_t block_reason = WATCH_CONTROL_BLOCK_BUSY;
        const char *error_code = gate->phase == WATCH_RENTAL_PHASE_CONFIRMING
                                     ? CONTROL_GATE_UNLOCK_CONFIRMING_CODE
                                     : CONTROL_GATE_QUERYING_CODE;
        if (gate->phase == WATCH_RENTAL_PHASE_WAITING_PAYMENT)
        {
            block_reason = WATCH_CONTROL_BLOCK_UNPAID;
            error_code = CONTROL_GATE_WAITING_PAYMENT_CODE;
        }
        else if (gate->phase == WATCH_RENTAL_PHASE_RETRY_WAIT)
        {
            block_reason = WATCH_CONTROL_BLOCK_CLOUD;
            error_code = CONTROL_GATE_RETRY_WAIT_CODE;
        }
        make_update(gate,
                    gate->phase,
                    block_reason,
                    gate->last_result,
                    true,
                    false,
                    error_code,
                    update);
        return CONTROL_GATE_BUSY;
    }
    const watch_control_block_reason_t blocked =
        admission_block_reason(snapshot, link_generation);
    if (!gate->session_invalidated && snapshot->unlock_session_valid &&
        blocked == WATCH_CONTROL_BLOCK_NONE)
    {
        make_update(gate,
                    WATCH_RENTAL_PHASE_UNLOCKED,
                    WATCH_CONTROL_BLOCK_NONE,
                    snapshot->last_rental_result,
                    false,
                    true,
                    CONTROL_GATE_UNLOCKED_CODE,
                    update);
        return CONTROL_GATE_ALREADY_UNLOCKED;
    }
    if (blocked != WATCH_CONTROL_BLOCK_NONE ||
        snapshot->ble_link_generation != link_generation)
    {
        make_update(gate,
                    WATCH_RENTAL_PHASE_WAITING_AUTH,
                    blocked,
                    WATCH_RENTAL_RESULT_NONE,
                    false,
                    false,
                    CONTROL_GATE_AUTH_REQUIRED_CODE,
                    update);
        return CONTROL_GATE_BLOCKED;
    }

    do
    {
        ++gate->next_request_id;
    } while (gate->next_request_id == 0U);
    gate->request_id = gate->next_request_id;
    memcpy(gate->exoskeleton_mac,
           snapshot->bound_exoskeleton_mac,
           sizeof(gate->exoskeleton_mac));
    gate->link_generation = link_generation;
    gate->phase = WATCH_RENTAL_PHASE_QUERYING;
    gate->last_result = WATCH_RENTAL_RESULT_NONE;
    gate->pending = true;
    gate->status_sequence_at_write = 0U;
    gate->confirm_started_ms = 0U;
    set_action(action, CONTROL_GATE_ACTION_RENTAL_QUERY, gate);
    make_update(gate,
                WATCH_RENTAL_PHASE_QUERYING,
                WATCH_CONTROL_BLOCK_BUSY,
                WATCH_RENTAL_RESULT_NONE,
                true,
                false,
                CONTROL_GATE_QUERYING_CODE,
                update);
    return CONTROL_GATE_ACCEPTED;
}

control_gate_control_request_status_t control_gate_request_control(
    control_gate_t *gate,
    const watch_state_snapshot_t *snapshot,
    uint32_t link_generation,
    const control_gate_control_intent_t *intent,
    control_gate_control_action_t *action,
    watch_control_update_t *update)
{
    clear_control_action(action);
    if (gate == NULL || snapshot == NULL || intent == NULL || action == NULL ||
        update == NULL || !control_intent_is_valid(intent))
    {
        return CONTROL_GATE_CONTROL_INVALID;
    }
    if (gate->control_locked_out)
    {
        return CONTROL_GATE_CONTROL_LOCKED_OUT;
    }
    if (gate->pending || gate->control_pending || snapshot->pending_control)
    {
        return CONTROL_GATE_CONTROL_BUSY;
    }
    const watch_control_block_reason_t blocked =
        admission_block_reason(snapshot, link_generation);
    if (blocked != WATCH_CONTROL_BLOCK_NONE ||
        snapshot->ble_link_generation != link_generation ||
        !control_snapshot_is_valid(snapshot))
    {
        return CONTROL_GATE_CONTROL_BLOCKED;
    }
    if (intent->kind == WATCH_CONTROL_KIND_SCENE_MODE &&
        !watch_exoskeleton_scene_config_allows_control(
            snapshot->exoskeleton_state.scene_config,
            intent->target))
    {
        return CONTROL_GATE_CONTROL_BLOCKED;
    }
    if (intent->kind == WATCH_CONTROL_KIND_GEAR &&
        intent->target >
            watch_exoskeleton_scene_config_max_gear(snapshot->exoskeleton_state.scene_config))
    {
        return CONTROL_GATE_CONTROL_BLOCKED;
    }
    if (gate->session_invalidated || !snapshot->unlock_session_valid ||
        snapshot->rental_phase != WATCH_RENTAL_PHASE_UNLOCKED ||
        snapshot->last_rental_result != WATCH_RENTAL_RESULT_PAID)
    {
        return CONTROL_GATE_CONTROL_AUTH_REQUIRED;
    }

    do
    {
        ++gate->next_request_id;
    } while (gate->next_request_id == 0U);
    gate->request_id = gate->next_request_id;
    memcpy(gate->exoskeleton_mac,
           snapshot->bound_exoskeleton_mac,
           sizeof(gate->exoskeleton_mac));
    gate->link_generation = link_generation;
    gate->phase = WATCH_RENTAL_PHASE_UNLOCKED;
    gate->last_result = WATCH_RENTAL_RESULT_PAID;
    gate->control_kind = control_kind_for_request(snapshot, intent);
    gate->control_target = intent->target;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
    gate->control_last_result = WATCH_CONTROL_RESULT_NONE;
    gate->control_status_sequence_at_write = 0U;
    gate->control_attempt = 1U;
    gate->control_ack_deadline_ms = 0U;
    gate->poweroff_result = WATCH_POWEROFF_RESULT_NONE;
    gate->poweroff_deadline_ms = 0U;

    if (control_target_is_no_op(snapshot, intent))
    {
        gate->control_pending = false;
        gate->control_last_result = WATCH_CONTROL_RESULT_NO_OP;
        set_control_action(action, gate);
        action->type = CONTROL_GATE_CONTROL_ACTION_NONE;
        make_control_update(gate,
                            WATCH_CONTROL_PHASE_IDLE,
                            WATCH_CONTROL_RESULT_NO_OP,
                            false,
                            CONTROL_GATE_CONTROL_NO_OP_CODE,
                            update);
        return CONTROL_GATE_CONTROL_NO_OP;
    }

    gate->control_pending = true;
    gate->control_phase = WATCH_CONTROL_PHASE_WRITING;
    set_control_action(action, gate);
    make_control_update(gate,
                        WATCH_CONTROL_PHASE_WRITING,
                        WATCH_CONTROL_RESULT_NONE,
                        true,
                        CONTROL_GATE_CONTROL_WRITING_CODE,
                        update);
    return CONTROL_GATE_CONTROL_ACCEPTED;
}

esp_err_t control_gate_apply_control_write_result(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_control_kind_t kind,
    uint8_t target,
    bool success,
    uint32_t status_sequence,
    watch_control_update_t *update)
{
    control_gate_control_action_t action = {0};
    return control_gate_apply_control_write_result_at(
        gate,
        request_id,
        exoskeleton_mac,
        link_generation,
        kind,
        target,
        success,
        status_sequence,
        0U,
        &action,
        update);
}

esp_err_t control_gate_apply_control_write_result_at(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_control_kind_t kind,
    uint8_t target,
    bool success,
    uint32_t status_sequence,
    uint64_t now_ms,
    control_gate_control_action_t *action,
    watch_control_update_t *update)
{
    if (gate == NULL || action == NULL || update == NULL ||
        !gate->control_pending ||
        gate->control_phase != WATCH_CONTROL_PHASE_WRITING ||
        !control_transaction_matches(gate,
                                     request_id,
                                     exoskeleton_mac,
                                     link_generation,
                                     kind,
                                     target))
    {
        return gate == NULL || action == NULL || update == NULL
                   ? ESP_ERR_INVALID_ARG
                   : ESP_ERR_NOT_FOUND;
    }
    clear_control_action(action);
    if (!success)
    {
        gate->control_ack_deadline_ms = 0U;
        gate->poweroff_deadline_ms = 0U;
        if (kind != WATCH_CONTROL_KIND_POWEROFF &&
            gate->control_attempt < CONTROL_MAX_ATTEMPTS)
        {
            ++gate->control_attempt;
            gate->control_phase = WATCH_CONTROL_PHASE_WRITING;
            gate->control_last_result = WATCH_CONTROL_RESULT_RETRYING;
            set_control_action(action, gate);
            make_control_update(gate,
                                WATCH_CONTROL_PHASE_WRITING,
                                WATCH_CONTROL_RESULT_RETRYING,
                                true,
                                CONTROL_GATE_CONTROL_RETRYING_CODE,
                                update);
            return ESP_OK;
        }
        gate->control_pending = false;
        gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
        if (kind == WATCH_CONTROL_KIND_POWEROFF)
        {
            gate->control_last_result = WATCH_CONTROL_RESULT_WRITE_REJECTED;
        }
        else
        {
            gate->control_last_result = WATCH_CONTROL_RESULT_RETRY_EXHAUSTED;
            gate->control_locked_out = true;
            gate->lockout_link_generation = gate->link_generation;
            gate->lockout_disconnect_seen = false;
        }
        make_control_update(gate,
                            WATCH_CONTROL_PHASE_IDLE,
                            gate->control_last_result,
                            false,
                            kind == WATCH_CONTROL_KIND_POWEROFF
                                ? CONTROL_GATE_CONTROL_WRITE_REJECTED_CODE
                                : CONTROL_GATE_CONTROL_RETRY_EXHAUSTED_CODE,
                            update);
        return ESP_OK;
    }

    gate->control_status_sequence_at_write = status_sequence;
    gate->control_last_result = WATCH_CONTROL_RESULT_SUBMITTED_UNCONFIRMED;
    if (kind == WATCH_CONTROL_KIND_POWEROFF)
    {
        gate->control_phase = WATCH_CONTROL_PHASE_POWEROFF_PENDING;
        gate->poweroff_deadline_ms =
            now_ms > UINT64_MAX - POWEROFF_RESULT_TIMEOUT_MS
                ? UINT64_MAX
                : now_ms + POWEROFF_RESULT_TIMEOUT_MS;
        gate->control_ack_deadline_ms = 0U;
        make_control_update(gate,
                            WATCH_CONTROL_PHASE_POWEROFF_PENDING,
                            WATCH_CONTROL_RESULT_SUBMITTED_UNCONFIRMED,
                            true,
                            CONTROL_GATE_POWEROFF_PENDING_CODE,
                            update);
        return ESP_OK;
    }

    gate->control_phase = WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED;
    gate->control_ack_deadline_ms =
        now_ms > UINT64_MAX - CONTROL_ACK_TIMEOUT_MS
            ? UINT64_MAX
            : now_ms + CONTROL_ACK_TIMEOUT_MS;
    gate->poweroff_deadline_ms = 0U;
    make_control_update(gate,
                        WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED,
                        WATCH_CONTROL_RESULT_SUBMITTED_UNCONFIRMED,
                        true,
                        CONTROL_GATE_CONTROL_SUBMITTED_CODE,
                        update);
    return ESP_OK;
}

esp_err_t control_gate_apply_control_status_evidence(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_control_kind_t kind,
    uint8_t target,
    uint32_t status_sequence,
    const watch_exoskeleton_state_t *actual_state,
    bool status_fresh,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL || actual_state == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->control_pending ||
        gate->control_phase != WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED ||
        kind == WATCH_CONTROL_KIND_POWEROFF || !status_fresh ||
        !control_transaction_matches(gate, request_id, exoskeleton_mac,
                                     link_generation, kind, target) ||
        !status_sequence_is_after(status_sequence,
                                  gate->control_status_sequence_at_write) ||
        !actual_state_matches_target(actual_state, kind, target))
    {
        return ESP_ERR_NOT_FOUND;
    }
    gate->control_pending = false;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
    gate->control_last_result = WATCH_CONTROL_RESULT_APPLIED;
    gate->control_ack_deadline_ms = 0U;
    make_control_update(gate, WATCH_CONTROL_PHASE_IDLE,
                        WATCH_CONTROL_RESULT_APPLIED, false,
                        CONTROL_GATE_CONTROL_APPLIED_CODE, update);
    return ESP_OK;
}

esp_err_t control_gate_check_control_timeout(
    control_gate_t *gate,
    uint64_t now_ms,
    control_gate_control_action_t *action,
    watch_control_update_t *update)
{
    clear_control_action(action);
    if (gate == NULL || action == NULL || update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->control_pending ||
        gate->control_phase != WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED ||
        gate->control_kind == WATCH_CONTROL_KIND_POWEROFF ||
        gate->control_ack_deadline_ms == 0U ||
        now_ms < gate->control_ack_deadline_ms)
    {
        return ESP_OK;
    }

    gate->control_ack_deadline_ms = 0U;
    if (gate->control_attempt < CONTROL_MAX_ATTEMPTS)
    {
        ++gate->control_attempt;
        gate->control_phase = WATCH_CONTROL_PHASE_WRITING;
        gate->control_last_result = WATCH_CONTROL_RESULT_RETRYING;
        set_control_action(action, gate);
        make_control_update(gate,
                            WATCH_CONTROL_PHASE_WRITING,
                            WATCH_CONTROL_RESULT_RETRYING,
                            true,
                            CONTROL_GATE_CONTROL_RETRYING_CODE,
                            update);
        return ESP_ERR_TIMEOUT;
    }

    gate->control_pending = false;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
    gate->control_last_result = WATCH_CONTROL_RESULT_RETRY_EXHAUSTED;
    gate->control_locked_out = true;
    gate->lockout_link_generation = gate->link_generation;
    gate->lockout_disconnect_seen = false;
    make_control_update(gate,
                        WATCH_CONTROL_PHASE_IDLE,
                        WATCH_CONTROL_RESULT_RETRY_EXHAUSTED,
                        false,
                        CONTROL_GATE_CONTROL_RETRY_EXHAUSTED_CODE,
                        update);
    return ESP_ERR_TIMEOUT;
}

bool control_gate_note_disconnect(
    control_gate_t *gate,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation)
{
    if (gate == NULL || !gate->control_locked_out ||
        gate->lockout_disconnect_seen ||
        link_generation == 0U ||
        link_generation != gate->lockout_link_generation ||
        exoskeleton_mac == NULL ||
        memcmp(exoskeleton_mac,
               gate->exoskeleton_mac,
               sizeof(gate->exoskeleton_mac)) != 0)
    {
        return false;
    }
    gate->lockout_disconnect_seen = true;
    return true;
}

bool control_gate_release_lockout_after_reconnect(
    control_gate_t *gate,
    const watch_state_snapshot_t *snapshot,
    uint32_t link_generation,
    watch_control_update_t *update)
{
    if (gate == NULL || snapshot == NULL || update == NULL ||
        !gate->control_locked_out || !gate->lockout_disconnect_seen ||
        link_generation == 0U ||
        link_generation == gate->lockout_link_generation ||
        snapshot->ble_link_generation != link_generation ||
        !snapshot->control_locked_out || !snapshot->unlock_session_valid ||
        snapshot->rental_phase != WATCH_RENTAL_PHASE_UNLOCKED ||
        snapshot->last_rental_result != WATCH_RENTAL_RESULT_PAID ||
        admission_block_reason(snapshot, link_generation) !=
            WATCH_CONTROL_BLOCK_NONE ||
        memcmp(snapshot->bound_exoskeleton_mac,
               gate->exoskeleton_mac,
               sizeof(gate->exoskeleton_mac)) != 0)
    {
        return false;
    }

    gate->link_generation = link_generation;
    gate->phase = WATCH_RENTAL_PHASE_UNLOCKED;
    gate->last_result = WATCH_RENTAL_RESULT_PAID;
    gate->control_kind = WATCH_CONTROL_KIND_NONE;
    gate->control_target = 0U;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
    gate->control_last_result = WATCH_CONTROL_RESULT_NONE;
    gate->control_pending = false;
    gate->control_status_sequence_at_write = 0U;
    gate->control_attempt = 0U;
    gate->control_ack_deadline_ms = 0U;
    gate->control_locked_out = false;
    gate->lockout_link_generation = 0U;
    gate->lockout_disconnect_seen = false;
    gate->poweroff_result = WATCH_POWEROFF_RESULT_NONE;
    gate->poweroff_deadline_ms = 0U;
    make_update(gate,
                WATCH_RENTAL_PHASE_UNLOCKED,
                WATCH_CONTROL_BLOCK_NONE,
                WATCH_RENTAL_RESULT_PAID,
                false,
                true,
                CONTROL_GATE_CONTROL_RECONNECTED_CODE,
                update);
    return true;
}

esp_err_t control_gate_apply_poweroff_offline_evidence(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    uint64_t now_ms,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->control_pending ||
        gate->control_phase != WATCH_CONTROL_PHASE_POWEROFF_PENDING ||
        gate->control_kind != WATCH_CONTROL_KIND_POWEROFF ||
        !transaction_matches(gate,
                             request_id,
                             exoskeleton_mac,
                             link_generation) ||
        gate->poweroff_deadline_ms == 0U ||
        now_ms >= gate->poweroff_deadline_ms)
    {
        return ESP_ERR_NOT_FOUND;
    }
    finish_poweroff(gate,
                    WATCH_POWEROFF_RESULT_ASSUMED_OFF,
                    link_generation,
                    false,
                    CONTROL_GATE_POWEROFF_ASSUMED_OFF_CODE,
                    update);
    return ESP_OK;
}

esp_err_t control_gate_apply_poweroff_online_evidence(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t observed_link_generation,
    uint32_t status_sequence,
    bool status_fresh,
    bool continuity_proven,
    uint64_t now_ms,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->control_pending ||
        gate->control_phase != WATCH_CONTROL_PHASE_POWEROFF_PENDING ||
        gate->control_kind != WATCH_CONTROL_KIND_POWEROFF ||
        !poweroff_request_matches(gate, request_id, exoskeleton_mac) ||
        observed_link_generation == 0U || !status_fresh ||
        !status_sequence_is_after(status_sequence,
                                  gate->control_status_sequence_at_write) ||
        gate->poweroff_deadline_ms == 0U ||
        now_ms >= gate->poweroff_deadline_ms)
    {
        return ESP_ERR_NOT_FOUND;
    }
    finish_poweroff(gate,
                    WATCH_POWEROFF_RESULT_FAILED_RECONNECTED,
                    observed_link_generation,
                    continuity_proven,
                    CONTROL_GATE_POWEROFF_FAILED_RECONNECTED_CODE,
                    update);
    return ESP_OK;
}

esp_err_t control_gate_check_poweroff_timeout(
    control_gate_t *gate,
    uint64_t now_ms,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->control_pending ||
        gate->control_phase != WATCH_CONTROL_PHASE_POWEROFF_PENDING ||
        gate->poweroff_deadline_ms == 0U ||
        now_ms < gate->poweroff_deadline_ms)
    {
        return ESP_OK;
    }
    finish_poweroff(gate,
                    WATCH_POWEROFF_RESULT_TIMEOUT_UNKNOWN,
                    gate->link_generation,
                    false,
                    CONTROL_GATE_POWEROFF_TIMEOUT_UNKNOWN_CODE,
                    update);
    return ESP_ERR_TIMEOUT;
}

esp_err_t control_gate_prepare_control_status_evidence(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_control_kind_t kind,
    uint8_t target,
    uint32_t status_sequence,
    const watch_exoskeleton_state_t *actual_state,
    bool status_fresh,
    control_gate_t *prepared_gate,
    watch_control_update_t *update)
{
    if (gate == NULL || prepared_gate == NULL || update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *prepared_gate = *gate;
    const esp_err_t error = control_gate_apply_control_status_evidence(
        prepared_gate,
        request_id,
        exoskeleton_mac,
        link_generation,
        kind,
        target,
        status_sequence,
        actual_state,
        status_fresh,
        update);
    if (error != ESP_OK)
    {
        memset(prepared_gate, 0, sizeof(*prepared_gate));
    }
    return error;
}

esp_err_t control_gate_apply_rental_result(
    control_gate_t *gate,
    const control_gate_rental_result_t *result,
    control_gate_action_t *action,
    watch_control_update_t *update)
{
    clear_action(action);
    if (gate == NULL || result == NULL || action == NULL || update == NULL ||
        result->result <= WATCH_RENTAL_RESULT_NONE ||
        result->result > WATCH_RENTAL_RESULT_CONFIG_ERROR)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->pending || !rental_phase_accepts_result(gate->phase) ||
        !transaction_matches(gate,
                             result->request_id,
                             result->exoskeleton_mac,
                             result->link_generation))
    {
        return ESP_ERR_NOT_FOUND;
    }
    gate->last_result = result->result;
    if (result->result == WATCH_RENTAL_RESULT_PAID)
    {
        gate->phase = WATCH_RENTAL_PHASE_UNLOCK_WRITING;
        set_action(action, CONTROL_GATE_ACTION_BLE_UNLOCK, gate);
        make_update(gate,
                    WATCH_RENTAL_PHASE_UNLOCK_WRITING,
                    WATCH_CONTROL_BLOCK_BUSY,
                    result->result,
                    true,
                    false,
                    CONTROL_GATE_UNLOCK_WRITING_CODE,
                    update);
        return ESP_OK;
    }

    if (result->result == WATCH_RENTAL_RESULT_UNPAID)
    {
        gate->phase = WATCH_RENTAL_PHASE_WAITING_PAYMENT;
        make_update(gate,
                    WATCH_RENTAL_PHASE_WAITING_PAYMENT,
                    WATCH_CONTROL_BLOCK_UNPAID,
                    result->result,
                    true,
                    false,
                    rental_error_code(result),
                    update);
        return ESP_OK;
    }

    if (rental_result_is_transient(result))
    {
        gate->phase = WATCH_RENTAL_PHASE_RETRY_WAIT;
        make_update(gate,
                    WATCH_RENTAL_PHASE_RETRY_WAIT,
                    WATCH_CONTROL_BLOCK_CLOUD,
                    result->result,
                    true,
                    false,
                    rental_error_code(result),
                    update);
        return ESP_OK;
    }

    gate->phase = WATCH_RENTAL_PHASE_BLOCKED;
    gate->pending = false;
    make_update(gate,
                WATCH_RENTAL_PHASE_BLOCKED,
                rental_block_reason(result->result),
                result->result,
                false,
                false,
                rental_error_code(result),
                update);
    return ESP_OK;
}

esp_err_t control_gate_apply_write_result(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    bool success,
    uint32_t status_sequence,
    uint64_t now_ms,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL || !gate->pending ||
        gate->phase != WATCH_RENTAL_PHASE_UNLOCK_WRITING ||
        !transaction_matches(gate,
                             request_id,
                             exoskeleton_mac,
                             link_generation))
    {
        return gate == NULL || update == NULL ? ESP_ERR_INVALID_ARG
                                              : ESP_ERR_NOT_FOUND;
    }
    if (!success)
    {
        gate->phase = WATCH_RENTAL_PHASE_BLOCKED;
        gate->pending = false;
        make_update(gate,
                    WATCH_RENTAL_PHASE_BLOCKED,
                    WATCH_CONTROL_BLOCK_BLE_WRITE,
                    gate->last_result,
                    false,
                    false,
                    CONTROL_GATE_WRITE_FAILED_CODE,
                    update);
        return ESP_OK;
    }
    gate->phase = WATCH_RENTAL_PHASE_CONFIRMING;
    gate->status_sequence_at_write = status_sequence;
    gate->confirm_started_ms = now_ms;
    make_update(gate,
                WATCH_RENTAL_PHASE_CONFIRMING,
                WATCH_CONTROL_BLOCK_BUSY,
                gate->last_result,
                true,
                false,
                CONTROL_GATE_UNLOCK_CONFIRMING_CODE,
                update);
    return ESP_OK;
}

esp_err_t control_gate_apply_status_evidence(
    control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    uint32_t status_sequence,
    uint64_t now_ms,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL || !gate->pending ||
        gate->phase != WATCH_RENTAL_PHASE_CONFIRMING ||
        !transaction_matches(gate,
                             request_id,
                             exoskeleton_mac,
                             link_generation) ||
        status_sequence <= gate->status_sequence_at_write)
    {
        return gate == NULL || update == NULL ? ESP_ERR_INVALID_ARG
                                              : ESP_ERR_NOT_FOUND;
    }
    if (now_ms < gate->confirm_started_ms ||
        now_ms - gate->confirm_started_ms >
            CONTROL_GATE_UNLOCK_CONFIRM_TIMEOUT_MS)
    {
        gate->phase = WATCH_RENTAL_PHASE_BLOCKED;
        gate->pending = false;
        make_update(gate,
                    WATCH_RENTAL_PHASE_BLOCKED,
                    WATCH_CONTROL_BLOCK_CONFIRM_TIMEOUT,
                    WATCH_RENTAL_RESULT_TIMEOUT,
                    false,
                    false,
                    CONTROL_GATE_CONFIRM_TIMEOUT_CODE,
                    update);
        return ESP_ERR_TIMEOUT;
    }
    gate->phase = WATCH_RENTAL_PHASE_UNLOCKED;
    gate->pending = false;
    gate->session_invalidated = false;
    make_update(gate,
                WATCH_RENTAL_PHASE_UNLOCKED,
                WATCH_CONTROL_BLOCK_NONE,
                gate->last_result,
                false,
                true,
                CONTROL_GATE_UNLOCKED_CODE,
                update);
    return ESP_OK;
}

esp_err_t control_gate_check_timeout(control_gate_t *gate,
                                     uint64_t now_ms,
                                     watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!gate->pending || gate->phase != WATCH_RENTAL_PHASE_CONFIRMING ||
        (now_ms >= gate->confirm_started_ms &&
         now_ms - gate->confirm_started_ms <=
             CONTROL_GATE_UNLOCK_CONFIRM_TIMEOUT_MS))
    {
        return ESP_OK;
    }
    gate->phase = WATCH_RENTAL_PHASE_BLOCKED;
    gate->pending = false;
    make_update(gate,
                WATCH_RENTAL_PHASE_BLOCKED,
                WATCH_CONTROL_BLOCK_CONFIRM_TIMEOUT,
                WATCH_RENTAL_RESULT_TIMEOUT,
                false,
                false,
                CONTROL_GATE_CONFIRM_TIMEOUT_CODE,
                update);
    return ESP_ERR_TIMEOUT;
}

esp_err_t control_gate_fail_authorization_entry(
    control_gate_t *gate,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_rental_result_t result,
    const char *error_code,
    watch_control_update_t *update)
{
    if (gate == NULL || exoskeleton_mac == NULL || error_code == NULL ||
        update == NULL || link_generation == 0U ||
        strnlen(exoskeleton_mac, CONTROL_GATE_MAC_CAPACITY) !=
            CONTROL_GATE_MAC_CAPACITY - 1U ||
        result <= WATCH_RENTAL_RESULT_PAID ||
        result > WATCH_RENTAL_RESULT_CONFIG_ERROR ||
        strnlen(error_code, WATCH_STATE_ERROR_CODE_CAPACITY) >=
            WATCH_STATE_ERROR_CODE_CAPACITY ||
        (strncmp(error_code, "CLOUD_", 6U) != 0 &&
         strncmp(error_code, "BLE_", 4U) != 0))
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (gate->pending || gate->control_pending ||
        (!gate->session_invalidated &&
         gate->phase == WATCH_RENTAL_PHASE_UNLOCKED &&
         gate->last_result == WATCH_RENTAL_RESULT_PAID))
    {
        return ESP_ERR_INVALID_STATE;
    }

    do
    {
        ++gate->next_request_id;
    } while (gate->next_request_id == 0U);
    gate->request_id = gate->next_request_id;
    memcpy(gate->exoskeleton_mac,
           exoskeleton_mac,
           sizeof(gate->exoskeleton_mac));
    gate->link_generation = link_generation;
    gate->phase = WATCH_RENTAL_PHASE_BLOCKED;
    gate->last_result = result;
    gate->pending = false;
    gate->status_sequence_at_write = 0U;
    gate->confirm_started_ms = 0U;
    make_update(gate,
                WATCH_RENTAL_PHASE_BLOCKED,
                rental_block_reason(result),
                result,
                false,
                false,
                error_code,
                update);
    return ESP_OK;
}

bool control_gate_abort_pending(control_gate_t *gate,
                                watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return false;
    }
    if (gate->control_pending)
    {
        if (gate->control_phase == WATCH_CONTROL_PHASE_POWEROFF_PENDING &&
            gate->control_kind == WATCH_CONTROL_KIND_POWEROFF)
        {
            finish_poweroff(gate,
                            WATCH_POWEROFF_RESULT_TIMEOUT_UNKNOWN,
                            gate->link_generation,
                            false,
                            CONTROL_GATE_POWEROFF_TIMEOUT_UNKNOWN_CODE,
                            update);
            return true;
        }
        gate->control_pending = false;
        gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
        gate->control_last_result = WATCH_CONTROL_RESULT_BLOCKED;
        gate->control_ack_deadline_ms = 0U;
        gate->poweroff_deadline_ms = 0U;
        make_control_update(gate,
                            WATCH_CONTROL_PHASE_IDLE,
                            WATCH_CONTROL_RESULT_BLOCKED,
                            false,
                            CONTROL_GATE_CONTROL_BLOCKED_CODE,
                            update);
        return true;
    }
    if (!gate->pending)
    {
        return false;
    }
    gate->phase = WATCH_RENTAL_PHASE_BLOCKED;
    gate->pending = false;
    gate->control_pending = false;
    gate->control_kind = WATCH_CONTROL_KIND_NONE;
    gate->control_target = 0U;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
    gate->control_last_result = WATCH_CONTROL_RESULT_NONE;
    make_update(gate,
                WATCH_RENTAL_PHASE_BLOCKED,
                WATCH_CONTROL_BLOCK_BLE_LINK,
                gate->last_result,
                false,
                false,
                CONTROL_GATE_LINK_CHANGED_CODE,
                update);
    return true;
}

bool control_gate_abort_and_invalidate_session(
    control_gate_t *gate,
    watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return false;
    }
    const bool aborted = control_gate_abort_pending(gate, update);
    control_gate_invalidate_session(gate, update);
    return aborted;
}

void control_gate_invalidate_session(control_gate_t *gate,
                                     watch_control_update_t *update)
{
    if (gate == NULL || update == NULL)
    {
        return;
    }
    gate->phase = WATCH_RENTAL_PHASE_WAITING_AUTH;
    gate->last_result = WATCH_RENTAL_RESULT_NONE;
    gate->pending = false;
    gate->session_invalidated = true;
    gate->status_sequence_at_write = 0U;
    gate->confirm_started_ms = 0U;
    make_update(gate,
                WATCH_RENTAL_PHASE_WAITING_AUTH,
                WATCH_CONTROL_BLOCK_BLE_LINK,
                WATCH_RENTAL_RESULT_NONE,
                false,
                false,
                CONTROL_GATE_AUTH_REQUIRED_CODE,
                update);
}

static void clear_action(control_gate_action_t *action)
{
    if (action != NULL)
    {
        memset(action, 0, sizeof(*action));
    }
}

static void set_action(control_gate_action_t *action,
                       control_gate_action_type_t type,
                       const control_gate_t *gate)
{
    if (action == NULL || gate == NULL)
    {
        return;
    }
    *action = (control_gate_action_t){
        .type = type,
        .request_id = gate->request_id,
        .link_generation = gate->link_generation,
    };
    memcpy(action->exoskeleton_mac,
           gate->exoskeleton_mac,
           sizeof(action->exoskeleton_mac));
}

static void clear_control_action(control_gate_control_action_t *action)
{
    if (action != NULL)
    {
        memset(action, 0, sizeof(*action));
    }
}

static void set_control_action(control_gate_control_action_t *action,
                               const control_gate_t *gate)
{
    if (action == NULL || gate == NULL)
    {
        return;
    }
    *action = (control_gate_control_action_t){
        .type = CONTROL_GATE_CONTROL_ACTION_BLE_WRITE,
        .request_id = gate->request_id,
        .link_generation = gate->link_generation,
        .kind = gate->control_kind,
        .target = gate->control_target,
        .attempt = gate->control_attempt,
    };
    memcpy(action->exoskeleton_mac,
           gate->exoskeleton_mac,
           sizeof(action->exoskeleton_mac));
}

static void make_update(control_gate_t *gate,
                        watch_rental_phase_t phase,
                        watch_control_block_reason_t block_reason,
                        watch_rental_result_t result,
                        bool pending,
                        bool unlocked,
                        const char *error_code,
                        watch_control_update_t *update)
{
    if (gate == NULL || update == NULL || error_code == NULL)
    {
        return;
    }
    do
    {
        ++gate->control_update_sequence;
    } while (gate->control_update_sequence == 0U);
    memset(update, 0, sizeof(*update));
    update->control_update_sequence = gate->control_update_sequence;
    update->request_id = gate->request_id;
    update->link_generation = gate->link_generation;
    memcpy(update->exoskeleton_mac,
           gate->exoskeleton_mac,
           sizeof(update->exoskeleton_mac));
    update->rental_phase = phase;
    update->block_reason = block_reason;
    update->last_rental_result = result;
    update->pending_control = pending;
    update->unlock_session_valid = unlocked;
    const size_t length = strnlen(error_code, sizeof(update->control_error_code));
    if (length < sizeof(update->control_error_code))
    {
        memcpy(update->control_error_code, error_code, length + 1U);
    }
    else
    {
        memcpy(update->control_error_code,
               CONTROL_GATE_AUTH_REQUIRED_CODE,
               sizeof(CONTROL_GATE_AUTH_REQUIRED_CODE));
    }
}

static void make_control_update(control_gate_t *gate,
                                watch_control_phase_t phase,
                                watch_control_result_t result,
                                bool pending,
                                const char *error_code,
                                watch_control_update_t *update)
{
    if (gate == NULL || update == NULL || error_code == NULL)
    {
        return;
    }
    do
    {
        ++gate->control_update_sequence;
    } while (gate->control_update_sequence == 0U);
    memset(update, 0, sizeof(*update));
    update->control_update_sequence = gate->control_update_sequence;
    update->request_id = gate->request_id;
    update->link_generation = gate->link_generation;
    memcpy(update->exoskeleton_mac,
           gate->exoskeleton_mac,
           sizeof(update->exoskeleton_mac));
    update->rental_phase = WATCH_RENTAL_PHASE_UNLOCKED;
    update->control_kind = gate->control_kind;
    update->control_target = gate->control_target;
    update->control_phase = phase;
    update->control_last_result = result;
    update->control_attempt = gate->control_attempt;
    update->control_ack_deadline_ms = gate->control_ack_deadline_ms;
    update->control_locked_out = gate->control_locked_out;
    update->poweroff_result = gate->poweroff_result;
    update->poweroff_deadline_ms = gate->poweroff_deadline_ms;
    update->block_reason = pending
                               ? WATCH_CONTROL_BLOCK_BUSY
                               : (result == WATCH_CONTROL_RESULT_WRITE_REJECTED
                                      ? WATCH_CONTROL_BLOCK_BLE_WRITE
                                      : (result ==
                                                 WATCH_CONTROL_RESULT_RETRY_EXHAUSTED
                                             ? WATCH_CONTROL_BLOCK_RETRY_EXHAUSTED
                                             : (result == WATCH_CONTROL_RESULT_BLOCKED
                                                    ? WATCH_CONTROL_BLOCK_BLE_LINK
                                                    : WATCH_CONTROL_BLOCK_NONE)));
    update->last_rental_result = WATCH_RENTAL_RESULT_PAID;
    update->pending_control = pending;
    update->unlock_session_valid = true;
    const size_t length = strnlen(error_code, sizeof(update->control_error_code));
    if (length < sizeof(update->control_error_code))
    {
        memcpy(update->control_error_code, error_code, length + 1U);
    }
    else
    {
        memcpy(update->control_error_code,
               CONTROL_GATE_CONTROL_BLOCKED_CODE,
               sizeof(CONTROL_GATE_CONTROL_BLOCKED_CODE));
    }
}

static watch_control_block_reason_t admission_block_reason(
    const watch_state_snapshot_t *snapshot,
    uint32_t link_generation)
{
    if (!snapshot->has_bound_exoskeleton ||
        snapshot->bound_exoskeleton_mac[0] == '\0')
    {
        return WATCH_CONTROL_BLOCK_UNBOUND;
    }
    if (link_generation == 0U || !snapshot->ble_connected ||
        !snapshot->gatt_ready || !snapshot->notify_ready ||
        !snapshot->link_control_ready)
    {
        return WATCH_CONTROL_BLOCK_BLE_LINK;
    }
    if (!snapshot->device_state_available || !snapshot->dataReady)
    {
        return WATCH_CONTROL_BLOCK_DATA_NOT_READY;
    }
    if (!snapshot->status_fresh)
    {
        return WATCH_CONTROL_BLOCK_STATUS_STALE;
    }
    return WATCH_CONTROL_BLOCK_NONE;
}

static bool transaction_matches(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation)
{
    return gate != NULL && exoskeleton_mac != NULL && request_id != 0U &&
           request_id == gate->request_id &&
           link_generation == gate->link_generation &&
           strnlen(exoskeleton_mac, CONTROL_GATE_MAC_CAPACITY) ==
               CONTROL_GATE_MAC_CAPACITY - 1U &&
           memcmp(exoskeleton_mac,
                  gate->exoskeleton_mac,
                  CONTROL_GATE_MAC_CAPACITY) == 0;
}

static bool rental_phase_accepts_result(watch_rental_phase_t phase)
{
    return phase == WATCH_RENTAL_PHASE_QUERYING ||
           phase == WATCH_RENTAL_PHASE_WAITING_PAYMENT ||
           phase == WATCH_RENTAL_PHASE_RETRY_WAIT;
}

static bool rental_result_is_transient(
    const control_gate_rental_result_t *result)
{
    if (result == NULL)
    {
        return false;
    }
    if (result->result == WATCH_RENTAL_RESULT_SERVER_ERROR ||
        result->result == WATCH_RENTAL_RESULT_TRANSPORT_ERROR ||
        result->result == WATCH_RENTAL_RESULT_TIMEOUT)
    {
        return true;
    }
    return result->result == WATCH_RENTAL_RESULT_HTTP_ERROR &&
           (result->http_status == 408U || result->http_status == 429U ||
            (result->http_status >= 500U && result->http_status <= 599U));
}

static watch_control_block_reason_t rental_block_reason(
    watch_rental_result_t result)
{
    if (result == WATCH_RENTAL_RESULT_UNPAID)
    {
        return WATCH_CONTROL_BLOCK_UNPAID;
    }
    if (result == WATCH_RENTAL_RESULT_DISABLED)
    {
        return WATCH_CONTROL_BLOCK_DISABLED;
    }
    if (result == WATCH_RENTAL_RESULT_UNKNOWN_DEVICE)
    {
        return WATCH_CONTROL_BLOCK_UNKNOWN_DEVICE;
    }
    return WATCH_CONTROL_BLOCK_CLOUD;
}

static const char *rental_error_code(const control_gate_rental_result_t *result)
{
    static const char *const fallback_codes[] = {
        [WATCH_RENTAL_RESULT_UNPAID] = "CLOUD_RENTAL_UNPAID",
        [WATCH_RENTAL_RESULT_DISABLED] = "CLOUD_DEVICE_DISABLED",
        [WATCH_RENTAL_RESULT_UNKNOWN_DEVICE] = "CLOUD_UNKNOWN_DEVICE",
        [WATCH_RENTAL_RESULT_REQUEST_INVALID] = "CLOUD_REQUEST_INVALID",
        [WATCH_RENTAL_RESULT_SERVER_ERROR] = "CLOUD_SERVER_ERROR",
        [WATCH_RENTAL_RESULT_TOKEN_INVALID] = "CLOUD_TOKEN_INVALID",
        [WATCH_RENTAL_RESULT_HTTP_ERROR] = "CLOUD_HTTP_ERROR",
        [WATCH_RENTAL_RESULT_TRANSPORT_ERROR] = "CLOUD_UNREACHABLE",
        [WATCH_RENTAL_RESULT_TLS_ERROR] = "CLOUD_TLS_FAILED",
        [WATCH_RENTAL_RESULT_TIMEOUT] = "CLOUD_TIMEOUT",
        [WATCH_RENTAL_RESULT_PARSE_ERROR] = "CLOUD_RESPONSE_INVALID",
        [WATCH_RENTAL_RESULT_CONFIG_ERROR] = "CLOUD_CONFIG_INVALID",
    };
    if (result->error_code[0] != '\0' &&
        strnlen(result->error_code, sizeof(result->error_code)) <
            sizeof(result->error_code) &&
        strncmp(result->error_code, "CLOUD_", 6U) == 0)
    {
        return result->error_code;
    }
    if (result->result > WATCH_RENTAL_RESULT_NONE &&
        result->result <= WATCH_RENTAL_RESULT_CONFIG_ERROR &&
        fallback_codes[result->result] != NULL)
    {
        return fallback_codes[result->result];
    }
    return CONTROL_GATE_AUTH_REQUIRED_CODE;
}

static bool control_intent_is_valid(const control_gate_control_intent_t *intent)
{
    if (intent == NULL)
    {
        return false;
    }
    switch (intent->kind)
    {
    case WATCH_CONTROL_KIND_MOTOR_ENABLE:
        return intent->target <= 1U;
    case WATCH_CONTROL_KIND_GEAR:
        return intent->target >= WATCH_EXOSKELETON_GEAR_MIN &&
               intent->target <= WATCH_EXOSKELETON_GEAR_MAX;
    case WATCH_CONTROL_KIND_SCENE_MODE:
        return intent->target >= WATCH_EXOSKELETON_SCENE_MODE_MIN &&
               intent->target <= WATCH_EXOSKELETON_SCENE_MODE_MAX;
    case WATCH_CONTROL_KIND_POWEROFF:
        return intent->target == 1U;
    default:
        return false;
    }
}

static bool control_snapshot_is_valid(const watch_state_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return false;
    }
    const float max_gear = (float)watch_exoskeleton_scene_config_max_gear(
        snapshot->exoskeleton_state.scene_config);
    return strnlen(snapshot->bound_exoskeleton_mac,
                   sizeof(snapshot->bound_exoskeleton_mac)) ==
               sizeof(snapshot->bound_exoskeleton_mac) - 1U &&
           snapshot->exoskeleton_state.motor_enable <= 1U &&
           isfinite(snapshot->exoskeleton_state.left_gear) &&
           snapshot->exoskeleton_state.left_gear >=
               (float)WATCH_EXOSKELETON_GEAR_MIN &&
           snapshot->exoskeleton_state.left_gear <= max_gear &&
           isfinite(snapshot->exoskeleton_state.right_gear) &&
           snapshot->exoskeleton_state.right_gear >=
               (float)WATCH_EXOSKELETON_GEAR_MIN &&
           snapshot->exoskeleton_state.right_gear <= max_gear &&
           watch_exoskeleton_scene_config_supports_mode(
               snapshot->exoskeleton_state.scene_config,
               snapshot->exoskeleton_state.scene_mode);
}

static watch_control_kind_t control_kind_for_request(
    const watch_state_snapshot_t *snapshot,
    const control_gate_control_intent_t *intent)
{
    if (snapshot == NULL || intent == NULL ||
        intent->kind != WATCH_CONTROL_KIND_GEAR ||
        !watch_exoskeleton_model_supports_extreme(
            snapshot->exoskeleton_model))
    {
        return intent != NULL ? intent->kind : WATCH_CONTROL_KIND_NONE;
    }
    const uint8_t current_gear =
        protocol_gear(snapshot->exoskeleton_state.left_gear);
    if ((current_gear == 10U && intent->target == 11U) ||
        (current_gear == 11U && intent->target == 10U))
    {
        return WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY;
    }
    return intent->kind;
}

static bool control_target_is_no_op(
    const watch_state_snapshot_t *snapshot,
    const control_gate_control_intent_t *intent)
{
    switch (intent->kind)
    {
    case WATCH_CONTROL_KIND_MOTOR_ENABLE:
        return snapshot->exoskeleton_state.motor_enable == intent->target;
    case WATCH_CONTROL_KIND_GEAR:
        return protocol_gear(snapshot->exoskeleton_state.left_gear) ==
                   intent->target &&
               protocol_gear(snapshot->exoskeleton_state.right_gear) ==
                   intent->target;
    case WATCH_CONTROL_KIND_SCENE_MODE:
        return snapshot->exoskeleton_state.scene_mode == intent->target;
    case WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY:
    case WATCH_CONTROL_KIND_POWEROFF:
    default:
        return false;
    }
}

static uint8_t protocol_gear(float gear)
{
    return (uint8_t)(gear + 0.5F);
}

static bool control_transaction_matches(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY],
    uint32_t link_generation,
    watch_control_kind_t kind,
    uint8_t target)
{
    return transaction_matches(gate,
                               request_id,
                               exoskeleton_mac,
                               link_generation) &&
           kind == gate->control_kind && target == gate->control_target;
}

static bool poweroff_request_matches(
    const control_gate_t *gate,
    uint32_t request_id,
    const char exoskeleton_mac[CONTROL_GATE_MAC_CAPACITY])
{
    return gate != NULL && exoskeleton_mac != NULL && request_id != 0U &&
           request_id == gate->request_id &&
           strnlen(exoskeleton_mac, CONTROL_GATE_MAC_CAPACITY) ==
               CONTROL_GATE_MAC_CAPACITY - 1U &&
           memcmp(exoskeleton_mac,
                  gate->exoskeleton_mac,
                  CONTROL_GATE_MAC_CAPACITY) == 0;
}

static void finish_poweroff(control_gate_t *gate,
                            watch_poweroff_result_t result,
                            uint32_t observed_link_generation,
                            bool preserve_session,
                            const char *error_code,
                            watch_control_update_t *update)
{
    gate->link_generation = observed_link_generation;
    gate->control_pending = false;
    gate->control_phase = WATCH_CONTROL_PHASE_IDLE;
    gate->control_last_result = WATCH_CONTROL_RESULT_POWEROFF_TERMINAL;
    gate->control_ack_deadline_ms = 0U;
    gate->poweroff_result = result;
    gate->poweroff_deadline_ms = 0U;
    gate->phase = preserve_session ? WATCH_RENTAL_PHASE_UNLOCKED
                                   : WATCH_RENTAL_PHASE_WAITING_AUTH;
    gate->last_result = preserve_session ? WATCH_RENTAL_RESULT_PAID
                                         : WATCH_RENTAL_RESULT_NONE;
    gate->session_invalidated = !preserve_session;
    make_control_update(gate,
                        WATCH_CONTROL_PHASE_IDLE,
                        WATCH_CONTROL_RESULT_POWEROFF_TERMINAL,
                        false,
                        error_code,
                        update);
    update->rental_phase = gate->phase;
    update->block_reason = preserve_session ? WATCH_CONTROL_BLOCK_NONE
                                            : WATCH_CONTROL_BLOCK_BLE_LINK;
    update->last_rental_result = gate->last_result;
    update->unlock_session_valid = preserve_session;
}

static bool status_sequence_is_after(uint32_t candidate, uint32_t baseline)
{
    return (int32_t)(candidate - baseline) > 0;
}

static bool actual_state_matches_target(
    const watch_exoskeleton_state_t *actual_state,
    watch_control_kind_t kind,
    uint8_t target)
{
    if (actual_state == NULL)
    {
        return false;
    }
    switch (kind)
    {
    case WATCH_CONTROL_KIND_MOTOR_ENABLE:
        return actual_state->motor_enable == target;
    case WATCH_CONTROL_KIND_GEAR:
        return isfinite(actual_state->left_gear) &&
               actual_state->left_gear >=
                   (float)WATCH_EXOSKELETON_GEAR_MIN &&
               actual_state->left_gear <=
                   (float)WATCH_EXOSKELETON_GEAR_MAX &&
               isfinite(actual_state->right_gear) &&
               actual_state->right_gear >=
                   (float)WATCH_EXOSKELETON_GEAR_MIN &&
               actual_state->right_gear <=
                   (float)WATCH_EXOSKELETON_GEAR_MAX &&
               protocol_gear(actual_state->left_gear) == target &&
               protocol_gear(actual_state->right_gear) == target;
    case WATCH_CONTROL_KIND_SCENE_MODE:
        return actual_state->scene_mode == target;
    case WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY:
        return memchr(actual_state->version,
                      '\0',
                      sizeof(actual_state->version)) != NULL &&
               watch_exoskeleton_model_supports_extreme(
                   watch_exoskeleton_model_from_version(
                       actual_state->version)) &&
               isfinite(actual_state->left_gear) &&
               actual_state->left_gear >=
                   (float)WATCH_EXOSKELETON_GEAR_MIN &&
               actual_state->left_gear <=
                   (float)WATCH_EXOSKELETON_GEAR_MAX &&
               isfinite(actual_state->right_gear) &&
               actual_state->right_gear >=
                   (float)WATCH_EXOSKELETON_GEAR_MIN &&
               actual_state->right_gear <=
                   (float)WATCH_EXOSKELETON_GEAR_MAX &&
               protocol_gear(actual_state->left_gear) == target &&
               protocol_gear(actual_state->right_gear) == target &&
               actual_state->scene_mode == (target == 11U ? 3U : 1U);
    default:
        return false;
    }
}
