/**
 * @file     control_ui_binding.c
 * @brief    控制页纯模型与 typed intent 实现
 * @details  仅执行按值状态投影与 typed 动作转换，不创建、持有或更新任何 LVGL 对象。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "control_ui_binding.h"

#include <math.h>
#include <string.h>

static bool action_target_is_valid(control_ui_action_t action, uint8_t target);
static bool actual_state_is_valid(const watch_state_snapshot_t *snapshot);
static uint8_t protocol_gear(float gear);
static control_ui_block_reason_t auth_block_reason(
    const watch_state_snapshot_t *snapshot);

esp_err_t control_ui_model_from_snapshot(const watch_state_snapshot_t *snapshot,
                                         bool snapshot_valid,
                                         uint64_t now_ms,
                                         control_ui_model_t *model)
{
    if (model == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(model, 0, sizeof(*model));
    if (!snapshot_valid || snapshot == NULL)
    {
        model->block_reason = CONTROL_UI_BLOCK_SNAPSHOT_FAILED;
        return ESP_OK;
    }

    if (snapshot->device_state_available && actual_state_is_valid(snapshot))
    {
        model->actual_available = true;
        model->actual_current =
            snapshot->ble_connected && snapshot->dataReady &&
            snapshot->status_fresh && snapshot->last_status_rx_ms != 0U &&
            now_ms >= snapshot->last_status_rx_ms &&
            now_ms - snapshot->last_status_rx_ms < STATUS_STALE_MS;
        model->actual_motor_enable = snapshot->exoskeleton_state.motor_enable;
        model->actual_gear = protocol_gear(snapshot->exoskeleton_state.left_gear);
        model->actual_scene_mode = snapshot->exoskeleton_state.scene_mode;
    }

    model->poweroff_committed =
        snapshot->control_kind == WATCH_CONTROL_KIND_POWEROFF &&
        (snapshot->control_phase == WATCH_CONTROL_PHASE_POWEROFF_PENDING ||
         (snapshot->control_last_result ==
              WATCH_CONTROL_RESULT_POWEROFF_TERMINAL &&
          snapshot->poweroff_result != WATCH_POWEROFF_RESULT_NONE));

    if (snapshot->control_locked_out)
    {
        model->block_reason = CONTROL_UI_BLOCK_RETRY_EXHAUSTED;
    }
    else if (!snapshot->has_bound_exoskeleton ||
             snapshot->bound_exoskeleton_mac[0] == '\0')
    {
        model->block_reason = CONTROL_UI_BLOCK_UNBOUND;
    }
    else if (!snapshot->ble_connected)
    {
        model->block_reason = CONTROL_UI_BLOCK_BLE_DISCONNECTED;
    }
    else if (!snapshot->gatt_ready)
    {
        model->block_reason = CONTROL_UI_BLOCK_GATT_NOT_READY;
    }
    else if (!snapshot->notify_ready)
    {
        model->block_reason = CONTROL_UI_BLOCK_NOTIFY_NOT_READY;
    }
    else if (!snapshot->device_state_available || !snapshot->dataReady ||
             !actual_state_is_valid(snapshot))
    {
        model->block_reason = CONTROL_UI_BLOCK_DATA_NOT_READY;
    }
    else if (!snapshot->status_fresh || snapshot->last_status_rx_ms == 0U ||
             now_ms < snapshot->last_status_rx_ms ||
             now_ms - snapshot->last_status_rx_ms >= STATUS_STALE_MS)
    {
        model->block_reason = CONTROL_UI_BLOCK_STATUS_STALE;
    }
    else if (!snapshot->link_control_ready)
    {
        model->block_reason = CONTROL_UI_BLOCK_LINK_NOT_READY;
    }
    else if (snapshot->pending_control ||
             snapshot->rental_phase == WATCH_RENTAL_PHASE_QUERYING ||
             snapshot->rental_phase == WATCH_RENTAL_PHASE_UNLOCK_WRITING ||
             snapshot->rental_phase == WATCH_RENTAL_PHASE_CONFIRMING ||
             snapshot->control_phase == WATCH_CONTROL_PHASE_WRITING ||
             snapshot->control_phase ==
                 WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED ||
             snapshot->control_phase == WATCH_CONTROL_PHASE_POWEROFF_PENDING)
    {
        model->block_reason = CONTROL_UI_BLOCK_PENDING;
    }
    else if (!snapshot->unlock_session_valid ||
             snapshot->rental_phase != WATCH_RENTAL_PHASE_UNLOCKED ||
             snapshot->last_rental_result != WATCH_RENTAL_RESULT_PAID)
    {
        model->block_reason = auth_block_reason(snapshot);
    }
    else
    {
        model->block_reason = CONTROL_UI_BLOCK_NONE;
        model->controls_enabled = true;
    }
    if (model->poweroff_committed)
    {
        model->controls_enabled = false;
    }
    return ESP_OK;
}

bool control_ui_poweroff_submit_allowed(
    const watch_state_snapshot_t *snapshot,
    bool snapshot_valid,
    uint64_t now_ms)
{
    control_ui_model_t model = {0};
    return control_ui_model_from_snapshot(snapshot,
                                          snapshot_valid,
                                          now_ms,
                                          &model) == ESP_OK &&
           model.controls_enabled && model.block_reason == CONTROL_UI_BLOCK_NONE;
}

esp_err_t control_ui_intent_from_action(control_ui_action_t action,
                                        uint8_t target,
                                        control_ui_intent_t *intent)
{
    if (intent == NULL || !action_target_is_valid(action, target))
    {
        return ESP_ERR_INVALID_ARG;
    }

    watch_control_kind_t kind;
    switch (action)
    {
    case CONTROL_UI_ACTION_MOTOR:
        kind = WATCH_CONTROL_KIND_MOTOR_ENABLE;
        break;
    case CONTROL_UI_ACTION_GEAR:
        kind = WATCH_CONTROL_KIND_GEAR;
        break;
    case CONTROL_UI_ACTION_SCENE_MODE:
        kind = WATCH_CONTROL_KIND_SCENE_MODE;
        break;
    case CONTROL_UI_ACTION_POWEROFF:
        kind = WATCH_CONTROL_KIND_POWEROFF;
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }

    *intent = (control_ui_intent_t){
        .kind = kind,
        .target = target,
    };
    return ESP_OK;
}

static bool action_target_is_valid(control_ui_action_t action, uint8_t target)
{
    switch (action)
    {
    case CONTROL_UI_ACTION_MOTOR:
        return target <= 1U;
    case CONTROL_UI_ACTION_GEAR:
        return target >= WATCH_EXOSKELETON_GEAR_MIN &&
               target <= WATCH_EXOSKELETON_GEAR_MAX;
    case CONTROL_UI_ACTION_SCENE_MODE:
        return target >= WATCH_EXOSKELETON_SCENE_MODE_MIN &&
               target <= WATCH_EXOSKELETON_SCENE_MODE_MAX;
    case CONTROL_UI_ACTION_POWEROFF:
        return target == 1U;
    default:
        return false;
    }
}

static bool actual_state_is_valid(const watch_state_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return false;
    }
    const float max_gear = (float)watch_exoskeleton_scene_config_max_gear(
        snapshot->exoskeleton_state.scene_config);
    return snapshot->exoskeleton_state.motor_enable <= 1U &&
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

static uint8_t protocol_gear(float gear)
{
    return (uint8_t)(gear + 0.5F);
}

static control_ui_block_reason_t auth_block_reason(
    const watch_state_snapshot_t *snapshot)
{
    if (snapshot->control_block_reason == WATCH_CONTROL_BLOCK_UNPAID ||
        snapshot->last_rental_result == WATCH_RENTAL_RESULT_UNPAID)
    {
        return CONTROL_UI_BLOCK_RENTAL_UNPAID;
    }
    if (snapshot->control_block_reason == WATCH_CONTROL_BLOCK_DISABLED ||
        snapshot->last_rental_result == WATCH_RENTAL_RESULT_DISABLED)
    {
        return CONTROL_UI_BLOCK_DEVICE_DISABLED;
    }
    if (snapshot->control_block_reason == WATCH_CONTROL_BLOCK_UNKNOWN_DEVICE ||
        snapshot->last_rental_result == WATCH_RENTAL_RESULT_UNKNOWN_DEVICE)
    {
        return CONTROL_UI_BLOCK_UNKNOWN_DEVICE;
    }
    if (snapshot->control_block_reason == WATCH_CONTROL_BLOCK_CLOUD)
    {
        return CONTROL_UI_BLOCK_CLOUD_UNAVAILABLE;
    }
    return CONTROL_UI_BLOCK_AUTH_REQUIRED;
}
