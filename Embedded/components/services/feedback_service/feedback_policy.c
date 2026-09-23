/**
 * @file     feedback_policy.c
 * @brief    统一反馈服务的纯逻辑策略实现。
 * @details  全部决策只消费事件到达时刻与策略状态：音量域校验、持久化记录打包、
 *           限速合并节奏、RGB 节流与故障降级状态机。决策不含任何 ESP-IDF 依赖，
 *           计数/游标/持久化/同步路径在本模块中不存在（AD-12）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "feedback_policy.h"

#include <stddef.h>

bool ewf_feedback_volume_valid(uint8_t volume)
{
    return volume >= EWF_FEEDBACK_VOLUME_MIN && volume <= EWF_FEEDBACK_VOLUME_MAX;
}

ewf_feedback_volume_result_t ewf_feedback_volume_record_pack(
    uint8_t volume,
    ewf_feedback_volume_record_t *record)
{
    if (record == NULL)
    {
        return EWF_FEEDBACK_VOLUME_NULL;
    }
    if (!ewf_feedback_volume_valid(volume))
    {
        /* 越界写入被拒绝：保持原值（调用方不动介质），并以中文日志上报（服务层）。 */
        return EWF_FEEDBACK_VOLUME_OUT_OF_RANGE;
    }
    record->schema_version = EWF_FEEDBACK_VOLUME_SCHEMA_VERSION;
    record->volume = volume;
    return EWF_FEEDBACK_VOLUME_OK;
}

ewf_feedback_record_result_t ewf_feedback_volume_record_validate(
    const ewf_feedback_volume_record_t *record)
{
    if (record == NULL)
    {
        return EWF_FEEDBACK_RECORD_NULL;
    }
    if (record->schema_version != EWF_FEEDBACK_VOLUME_SCHEMA_VERSION)
    {
        return EWF_FEEDBACK_RECORD_SCHEMA;
    }
    if (!ewf_feedback_volume_valid(record->volume))
    {
        return EWF_FEEDBACK_RECORD_VOLUME;
    }
    return EWF_FEEDBACK_RECORD_OK;
}

void ewf_feedback_policy_init(ewf_feedback_policy_state_t *state)
{
    if (state == NULL)
    {
        return;
    }
    state->last_audio_ms = 0U;
    state->has_audio = false;
    state->last_rgb_ms = 0U;
    state->has_rgb = false;
    state->audio_fault_active = false;
    state->rgb_fault_active = false;
    state->last_audio_fault_ms = 0U;
    state->has_audio_fault_log = false;
    state->last_rgb_fault_ms = 0U;
    state->has_rgb_fault_log = false;
    state->audio_fault_count = 0U;
    state->rgb_fault_count = 0U;
}

ewf_feedback_audio_decision_t ewf_feedback_audio_decide(
    const ewf_feedback_policy_state_t *state,
    uint32_t now_ms)
{
    if (state == NULL)
    {
        return EWF_FEEDBACK_AUDIO_MERGE_SKIP;
    }
    if (!state->has_audio ||
        (uint32_t)(now_ms - state->last_audio_ms) >= EWF_FEEDBACK_AUDIO_MIN_INTERVAL_MS)
    {
        return EWF_FEEDBACK_AUDIO_PLAY;
    }
    return EWF_FEEDBACK_AUDIO_MERGE_SKIP;
}

ewf_feedback_rgb_decision_t ewf_feedback_rgb_decide(
    const ewf_feedback_policy_state_t *state,
    uint32_t now_ms)
{
    if (state == NULL)
    {
        return EWF_FEEDBACK_RGB_SKIP;
    }
    if (!state->has_rgb ||
        (uint32_t)(now_ms - state->last_rgb_ms) >= EWF_FEEDBACK_RGB_MIN_INTERVAL_MS)
    {
        return EWF_FEEDBACK_RGB_UPDATE;
    }
    return EWF_FEEDBACK_RGB_SKIP;
}

bool ewf_feedback_note_audio_result(ewf_feedback_policy_state_t *state,
                                    uint32_t now_ms,
                                    uint32_t output_ms,
                                    bool succeeded)
{
    if (state == NULL)
    {
        return false;
    }
    if (succeeded)
    {
        /* 故障恢复后事实自动回到正常态，并重置限频窗口。 */
        state->audio_fault_active = false;
        state->has_audio_fault_log = false;
        state->last_audio_ms = output_ms;
        state->has_audio = true;
        return false;
    }
    state->audio_fault_active = true;
    if (state->audio_fault_count < 0xFFFFFFFFU)
    {
        ++state->audio_fault_count;
    }
    bool should_log = !state->has_audio_fault_log ||
                      (uint32_t)(now_ms - state->last_audio_fault_ms) >=
                          EWF_FEEDBACK_FAULT_LOG_MIN_INTERVAL_MS;
    if (should_log)
    {
        state->last_audio_fault_ms = now_ms;
        state->has_audio_fault_log = true;
    }
    return should_log;
}

bool ewf_feedback_note_rgb_result(ewf_feedback_policy_state_t *state,
                                  uint32_t now_ms,
                                  uint32_t output_ms,
                                  bool succeeded)
{
    if (state == NULL)
    {
        return false;
    }
    if (succeeded)
    {
        state->rgb_fault_active = false;
        state->has_rgb_fault_log = false;
        state->last_rgb_ms = output_ms;
        state->has_rgb = true;
        return false;
    }
    state->rgb_fault_active = true;
    if (state->rgb_fault_count < 0xFFFFFFFFU)
    {
        ++state->rgb_fault_count;
    }
    bool should_log = !state->has_rgb_fault_log ||
                      (uint32_t)(now_ms - state->last_rgb_fault_ms) >=
                          EWF_FEEDBACK_FAULT_LOG_MIN_INTERVAL_MS;
    if (should_log)
    {
        state->last_rgb_fault_ms = now_ms;
        state->has_rgb_fault_log = true;
    }
    return should_log;
}
