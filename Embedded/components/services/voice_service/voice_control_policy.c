/**
 * @file     voice_control_policy.c
 * @brief    离线语音控制纯门禁与命令映射实现
 * @details  严格复用控制页 controls_enabled 和 typed action，补充亮屏、自检、严重低电及语音候选阈值约束。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "voice_control_policy.h"

#include <math.h>

/** 所有语音命令最高候选必须严格超过的概率。 */
#define VOICE_CONTROL_MIN_PROBABILITY 0.40F

static bool voice_control_gate_allows_internal(
    const watch_state_snapshot_t *snapshot,
    bool snapshot_valid,
    uint64_t now_ms,
    bool require_screen_on);

bool voice_control_entry_gate_allows(
    const watch_state_snapshot_t *snapshot,
    bool snapshot_valid,
    uint64_t now_ms)
{
    return voice_control_gate_allows_internal(snapshot,
                                              snapshot_valid,
                                              now_ms,
                                              false);
}

bool voice_control_gate_allows(
    const watch_state_snapshot_t *snapshot,
    bool snapshot_valid,
    uint64_t now_ms)
{
    return voice_control_gate_allows_internal(snapshot,
                                              snapshot_valid,
                                              now_ms,
                                              true);
}

bool voice_control_candidate_accepts(
    int command_id,
    float top_probability,
    float second_probability)
{
    if (command_id < VOICE_COMMAND_AI_MODE ||
        command_id > VOICE_COMMAND_GEAR_10 ||
        !isfinite(top_probability) || !isfinite(second_probability) ||
        top_probability < 0.0F || top_probability > 1.0F ||
        second_probability < 0.0F || second_probability > 1.0F ||
        top_probability < second_probability)
    {
        return false;
    }
    return top_probability > VOICE_CONTROL_MIN_PROBABILITY;
}

bool voice_button_long_press_due(
    bool pressed,
    bool already_evaluated,
    uint64_t elapsed_ms)
{
    return pressed && !already_evaluated &&
           elapsed_ms >= VOICE_CONTROL_BOOT_LONG_PRESS_MS;
}

bool voice_capture_auto_stop_due(
    bool capture_started,
    bool already_requested,
    uint64_t elapsed_ms)
{
    return capture_started && !already_requested &&
           elapsed_ms >= VOICE_CONTROL_CAPTURE_WINDOW_MS;
}

esp_err_t voice_control_resolve_command(
    int command_id,
    const watch_state_snapshot_t *snapshot,
    const control_ui_model_t *model,
    control_ui_action_t *action,
    uint8_t *target,
    bool *should_submit)
{
    if (snapshot == NULL || model == NULL || action == NULL ||
        target == NULL || should_submit == NULL ||
        command_id < VOICE_COMMAND_AI_MODE ||
        command_id > VOICE_COMMAND_GEAR_10)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *should_submit = true;
    if (command_id >= VOICE_COMMAND_GEAR_1 &&
        command_id <= VOICE_COMMAND_GEAR_10)
    {
        if (!model->actual_available)
        {
            return ESP_ERR_INVALID_STATE;
        }
        *action = CONTROL_UI_ACTION_GEAR;
        *target = (uint8_t)(command_id - VOICE_COMMAND_GEAR_1 + 1);
        return ESP_OK;
    }
    switch ((voice_command_id_t)command_id)
    {
    case VOICE_COMMAND_AI_MODE:
        *action = CONTROL_UI_ACTION_SCENE_MODE;
        *target = 1U;
        break;
    case VOICE_COMMAND_EXTREME_MODE:
        *action = CONTROL_UI_ACTION_SCENE_MODE;
        *target = 3U;
        break;
    case VOICE_COMMAND_FITNESS_MODE:
        if (snapshot->exoskeleton_state.scene_config ==
            WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP)
        {
            return ESP_ERR_NOT_SUPPORTED;
        }
        *action = CONTROL_UI_ACTION_SCENE_MODE;
        *target = 2U;
        break;
    case VOICE_COMMAND_DOWNHILL_MODE:
        *action = CONTROL_UI_ACTION_SCENE_MODE;
        *target = 4U;
        break;
    case VOICE_COMMAND_POWEROFF:
        *action = CONTROL_UI_ACTION_POWEROFF;
        *target = 1U;
        break;
    case VOICE_COMMAND_ASSIST_ON:
    case VOICE_COMMAND_ASSIST_OFF:
        if (!model->actual_available)
        {
            return ESP_ERR_INVALID_STATE;
        }
        *action = CONTROL_UI_ACTION_MOTOR;
        *target = command_id == VOICE_COMMAND_ASSIST_ON ? 1U : 0U;
        *should_submit = model->actual_motor_enable != *target;
        break;
    case VOICE_COMMAND_GEAR_UP:
    case VOICE_COMMAND_GEAR_DOWN:
        if (!model->actual_available)
        {
            return ESP_ERR_INVALID_STATE;
        }
        const uint8_t max_gear = watch_exoskeleton_scene_config_max_gear(
            snapshot->exoskeleton_state.scene_config);
        if (max_gear < WATCH_EXOSKELETON_GEAR_MIN)
        {
            return ESP_ERR_NOT_SUPPORTED;
        }
        *action = CONTROL_UI_ACTION_GEAR;
        if (command_id == VOICE_COMMAND_GEAR_UP)
        {
            *should_submit = model->actual_gear < max_gear;
            *target = *should_submit
                          ? (uint8_t)(model->actual_gear + 1U)
                          : max_gear;
        }
        else
        {
            *should_submit = model->actual_gear > 1U;
            *target = *should_submit
                          ? (uint8_t)(model->actual_gear - 1U)
                          : 1U;
        }
        break;
    default:
        return ESP_ERR_INVALID_ARG;
    }
    if (*action == CONTROL_UI_ACTION_SCENE_MODE &&
        !watch_exoskeleton_scene_config_allows_control(
            snapshot->exoskeleton_state.scene_config,
            *target))
    {
        return ESP_ERR_NOT_SUPPORTED;
    }
    return ESP_OK;
}

static bool voice_control_gate_allows_internal(
    const watch_state_snapshot_t *snapshot,
    bool snapshot_valid,
    uint64_t now_ms,
    bool require_screen_on)
{
    if (!snapshot_valid || snapshot == NULL ||
        snapshot->screen_state < WATCH_SCREEN_STATE_ON ||
        snapshot->screen_state >= WATCH_SCREEN_STATE_COUNT ||
        (require_screen_on &&
         snapshot->screen_state != WATCH_SCREEN_STATE_ON) ||
        snapshot->power_level == WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY ||
        snapshot->selftest.state == SELFTEST_RUN_RUNNING ||
        snapshot->selftest.retry_active)
    {
        return false;
    }
    control_ui_model_t model = {0};
    return control_ui_model_from_snapshot(snapshot,
                                          true,
                                          now_ms,
                                          &model) == ESP_OK &&
           model.controls_enabled &&
           model.block_reason == CONTROL_UI_BLOCK_NONE;
}
