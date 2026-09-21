/**
 * @file     ble_status_frame.c
 * @brief    67 字节 BLE 状态帧纯解析器
 * @details  按显式小端偏移解码 float32，并以 fail-closed 规则保护关键字段。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "ble_status_frame.h"

#include "app_state.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

/** 无历史镜像时用于替代成对未初始化档位的协议安全默认值。 */
#define BLE_STATUS_DEFAULT_GEAR 5.0F

_Static_assert(sizeof(float) == sizeof(uint32_t),
               "BLE status protocol requires 32-bit float values.");

static uint32_t read_u32_le(const uint8_t *bytes);
static float read_float32_le(const uint8_t *bytes);
static bool copy_version(const uint8_t source[BLE_STATUS_VERSION_FIELD_LENGTH],
                         char output[BLE_STATUS_VERSION_CAPACITY]);
static bool decode_with_gear_history(
    const uint8_t frame[BLE_STATUS_FRAME_LENGTH],
    const ble_status_frame_t *gear_history,
    ble_status_frame_t *output,
    bool *version_sanitized,
    ble_status_decode_error_t *decode_error);
static ble_status_decode_error_t validate_critical_fields(
    const ble_status_frame_t *candidate);
static bool canonical_token_mac_is_valid(
    const char mac[BLE_STATUS_MAC_CAPACITY]);
static bool owner_tokens_equal(const ble_status_owner_token_t *left,
                               const ble_status_owner_token_t *right);
static void increment_saturated(uint32_t *counter);

bool ble_status_frame_decode(const uint8_t frame[BLE_STATUS_FRAME_LENGTH],
                             ble_status_frame_t *output,
                             bool *version_sanitized)
{
    return ble_status_frame_decode_detailed(frame,
                                            output,
                                            version_sanitized,
                                            NULL);
}

bool ble_status_frame_decode_detailed(
    const uint8_t frame[BLE_STATUS_FRAME_LENGTH],
    ble_status_frame_t *output,
    bool *version_sanitized,
    ble_status_decode_error_t *decode_error)
{
    return decode_with_gear_history(frame,
                                    NULL,
                                    output,
                                    version_sanitized,
                                    decode_error);
}

const char *ble_status_decode_error_name(
    ble_status_decode_error_t decode_error)
{
    static const char *const names[] = {
        [BLE_STATUS_DECODE_OK] = "OK",
        [BLE_STATUS_DECODE_HEADER_INVALID] = "HEADER_INVALID",
        [BLE_STATUS_DECODE_CURRENT_MODE_NONFINITE] = "CURRENT_MODE_NONFINITE",
        [BLE_STATUS_DECODE_STEPS_NONFINITE] = "STEPS_NONFINITE",
        [BLE_STATUS_DECODE_STEP_FREQUENCY_NONFINITE] =
            "STEP_FREQUENCY_NONFINITE",
        [BLE_STATUS_DECODE_LEFT_GEAR_NONFINITE] = "LEFT_GEAR_NONFINITE",
        [BLE_STATUS_DECODE_RIGHT_GEAR_NONFINITE] = "RIGHT_GEAR_NONFINITE",
        [BLE_STATUS_DECODE_BALANCE_FACTOR_NONFINITE] =
            "BALANCE_FACTOR_NONFINITE",
        [BLE_STATUS_DECODE_STEPS_OUT_OF_RANGE] = "STEPS_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_VERSION_UNSUPPORTED] = "VERSION_UNSUPPORTED",
        [BLE_STATUS_DECODE_LEFT_GEAR_OUT_OF_RANGE] =
            "LEFT_GEAR_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_RIGHT_GEAR_OUT_OF_RANGE] =
            "RIGHT_GEAR_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_BALANCE_FACTOR_OUT_OF_RANGE] =
            "BALANCE_FACTOR_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_MOTOR_ENABLE_OUT_OF_RANGE] =
            "MOTOR_ENABLE_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_BATTERY_OUT_OF_RANGE] = "BATTERY_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_SCENE_MODE_OUT_OF_RANGE] =
            "SCENE_MODE_OUT_OF_RANGE",
        [BLE_STATUS_DECODE_SCENE_MODE_MODEL_INCOMPATIBLE] =
            "SCENE_MODE_MODEL_INCOMPATIBLE",
    };
    if ((size_t)decode_error >= sizeof(names) / sizeof(names[0]) ||
        names[decode_error] == NULL)
    {
        return "UNKNOWN";
    }
    return names[decode_error];
}

void ble_status_sync_reset(ble_status_sync_t *sync)
{
    if (sync != NULL)
    {
        memset(sync, 0, sizeof(*sync));
    }
}

bool ble_status_sync_push(ble_status_sync_t *sync,
                          uint8_t value,
                          uint8_t *discarded_increment)
{
    if (discarded_increment != NULL)
    {
        *discarded_increment = 0U;
    }
    if (sync == NULL || sync->length >= BLE_STATUS_FRAME_LENGTH)
    {
        return false;
    }
    if (sync->length == 0U)
    {
        if (value == BLE_STATUS_HEADER_FIRST)
        {
            sync->bytes[0] = value;
            sync->length = 1U;
        }
        else if (discarded_increment != NULL)
        {
            *discarded_increment = 1U;
        }
        return false;
    }
    if (sync->length == 1U)
    {
        if (value == BLE_STATUS_HEADER_SECOND)
        {
            sync->bytes[1] = value;
            sync->length = 2U;
        }
        else if (value == BLE_STATUS_HEADER_FIRST)
        {
            if (discarded_increment != NULL)
            {
                *discarded_increment = 1U;
            }
        }
        else
        {
            sync->length = 0U;
            if (discarded_increment != NULL)
            {
                *discarded_increment = 2U;
            }
        }
        return false;
    }

    sync->bytes[sync->length] = value;
    ++sync->length;
    return sync->length == BLE_STATUS_FRAME_LENGTH;
}

void ble_status_sync_finish(ble_status_sync_t *sync, bool frame_valid)
{
    if (sync == NULL || sync->length != BLE_STATUS_FRAME_LENGTH)
    {
        return;
    }
    (void)frame_valid;
    /* 已收满的 67 字节无论合法与否都按整帧消费，非法帧不得保留帧内后缀。 */
    sync->length = 0U;
}

void ble_status_owner_reset_link(ble_status_owner_state_t *state)
{
    if (state == NULL)
    {
        return;
    }
    ble_status_sync_reset(&state->sync);
    memset(&state->partial_token, 0, sizeof(state->partial_token));
    state->partial_token_valid = false;
    memset(&state->evidence_token, 0, sizeof(state->evidence_token));
    state->evidence_published = false;
}

void ble_status_owner_clear_mirror(ble_status_owner_state_t *state)
{
    if (state == NULL)
    {
        return;
    }
    memset(&state->mirror, 0, sizeof(state->mirror));
    memset(&state->mirror_token, 0, sizeof(state->mirror_token));
    state->last_status_rx_ms = 0U;
    state->mirror_valid = false;
}

void ble_status_owner_reset_all(ble_status_owner_state_t *state)
{
    if (state != NULL)
    {
        memset(state, 0, sizeof(*state));
    }
}

bool ble_status_owner_consume_byte(ble_status_owner_state_t *state,
                                   uint8_t value,
                                   const ble_status_owner_token_t *link_token,
                                   uint64_t now_ms,
                                   ble_status_owner_outcome_t *outcome)
{
    if (state == NULL || link_token == NULL || outcome == NULL ||
        !canonical_token_mac_is_valid(link_token->mac))
    {
        return false;
    }
    memset(outcome, 0, sizeof(*outcome));
    if (state->partial_token_valid &&
        !owner_tokens_equal(&state->partial_token, link_token))
    {
        ble_status_owner_reset_link(state);
    }
    if (state->evidence_published &&
        !owner_tokens_equal(&state->evidence_token, link_token))
    {
        memset(&state->evidence_token, 0, sizeof(state->evidence_token));
        state->evidence_published = false;
    }

    const bool token_was_valid = state->partial_token_valid;
    uint8_t discarded_increment = 0U;
    const bool frame_ready = ble_status_sync_push(&state->sync,
                                                  value,
                                                  &discarded_increment);
    for (uint8_t count = 0U; count < discarded_increment; ++count)
    {
        increment_saturated(&state->diagnostics.discarded_byte_count);
    }
    if (!token_was_valid && state->sync.length > 0U)
    {
        state->partial_token = *link_token;
        state->partial_token_valid = true;
    }
    else if (state->sync.length == 0U)
    {
        state->partial_token_valid = false;
    }
    if (!frame_ready)
    {
        return true;
    }

    outcome->frame_completed = true;
    ble_status_frame_t candidate = {0};
    outcome->frame_valid = decode_with_gear_history(
        state->sync.bytes,
        state->mirror_valid &&
                state->evidence_published &&
                owner_tokens_equal(&state->mirror_token, link_token)
            ? &state->mirror
            : NULL,
        &candidate,
        &outcome->version_sanitized,
        &outcome->decode_error);
    if (outcome->version_sanitized)
    {
        increment_saturated(&state->diagnostics.version_sanitized_count);
    }
    if (!outcome->frame_valid)
    {
        increment_saturated(&state->diagnostics.invalid_frame_count);
        ble_status_sync_finish(&state->sync, false);
        state->partial_token_valid = state->sync.length > 0U;
        return true;
    }

    state->mirror = candidate;
    state->mirror_token = *link_token;
    state->last_status_rx_ms = now_ms;
    state->mirror_valid = true;
    increment_saturated(&state->diagnostics.valid_frame_count);
    ble_status_sync_finish(&state->sync, true);
    state->partial_token_valid = false;
    if (!state->evidence_published ||
        !owner_tokens_equal(&state->evidence_token, link_token))
    {
        state->evidence_published = true;
        state->evidence_token = *link_token;
        outcome->first_valid_evidence = true;
    }
    return true;
}

bool ble_status_is_fresh(bool current_link_valid,
                         bool device_state_available,
                         bool data_ready,
                         uint64_t now_ms,
                         uint64_t last_status_rx_ms,
                         uint64_t stale_ms)
{
    return current_link_valid && device_state_available && data_ready &&
           last_status_rx_ms != 0U && stale_ms != 0U &&
           now_ms >= last_status_rx_ms &&
           now_ms - last_status_rx_ms < stale_ms;
}

bool ble_status_freshness_accept_frame(
    bool current_link_valid,
    bool device_state_available,
    bool transport_ready,
    uint64_t received_at_ms,
    uint64_t now_ms,
    uint64_t stale_ms,
    ble_status_freshness_state_t *state)
{
    if (state == NULL || !current_link_valid || !device_state_available ||
        received_at_ms == 0U || stale_ms == 0U)
    {
        return false;
    }
    state->data_ready = true;
    state->status_fresh = ble_status_is_fresh(current_link_valid,
                                              device_state_available,
                                              true,
                                              now_ms,
                                              received_at_ms,
                                              stale_ms);
    state->link_control_ready = transport_ready && state->status_fresh;
    return true;
}

bool ble_status_freshness_expire(
    bool current_link_valid,
    bool device_state_available,
    uint64_t now_ms,
    uint64_t last_status_rx_ms,
    uint64_t stale_ms,
    ble_status_freshness_state_t *state)
{
    if (state == NULL || !state->status_fresh ||
        ble_status_is_fresh(current_link_valid,
                            device_state_available,
                            state->data_ready,
                            now_ms,
                            last_status_rx_ms,
                            stale_ms))
    {
        return false;
    }
    state->status_fresh = false;
    state->link_control_ready = false;
    return true;
}

static bool decode_with_gear_history(
    const uint8_t frame[BLE_STATUS_FRAME_LENGTH],
    const ble_status_frame_t *gear_history,
    ble_status_frame_t *output,
    bool *version_sanitized,
    ble_status_decode_error_t *decode_error)
{
    if (version_sanitized != NULL)
    {
        *version_sanitized = false;
    }
    if (decode_error != NULL)
    {
        *decode_error = BLE_STATUS_DECODE_HEADER_INVALID;
    }
    if (frame == NULL || output == NULL ||
        frame[0] != BLE_STATUS_HEADER_FIRST ||
        frame[1] != BLE_STATUS_HEADER_SECOND)
    {
        return false;
    }

    ble_status_frame_t candidate = {0};
    memcpy(candidate.reserved_header, &frame[2], sizeof(candidate.reserved_header));
    candidate.current_mode = read_float32_le(&frame[4]);
    candidate.steps = read_float32_le(&frame[8]);
    candidate.step_frequency = read_float32_le(&frame[12]);
    candidate.left_gear = read_float32_le(&frame[16]);
    candidate.right_gear = read_float32_le(&frame[20]);
    if (candidate.left_gear == candidate.right_gear &&
        (candidate.left_gear == -1.0F || candidate.left_gear == 0.0F))
    {
        candidate.left_gear = gear_history != NULL
                                  ? gear_history->left_gear
                                  : BLE_STATUS_DEFAULT_GEAR;
        candidate.right_gear = gear_history != NULL
                                   ? gear_history->right_gear
                                   : BLE_STATUS_DEFAULT_GEAR;
    }
    candidate.balance_factor = read_float32_le(&frame[24]);
    candidate.motor_enable = frame[28];
    candidate.battery_level = frame[29];
    candidate.scene_mode = frame[30];
    candidate.firmware_update_flag = frame[31];
    memcpy(candidate.reserved_data, &frame[32], sizeof(candidate.reserved_data));
    candidate.scene_config = frame[38];
    candidate.left_motor_position = (int8_t)frame[41];
    candidate.right_motor_position = (int8_t)frame[42];
    candidate.standby_time = frame[43];
    candidate.temp_alarm = frame[44];
    memcpy(candidate.padding, &frame[45], sizeof(candidate.padding));
    const bool sanitized = copy_version(&frame[47], candidate.version);
    memcpy(candidate.timestamp, &frame[63], sizeof(candidate.timestamp));
    if (version_sanitized != NULL)
    {
        *version_sanitized = sanitized;
    }
    const ble_status_decode_error_t validation_error =
        validate_critical_fields(&candidate);
    if (decode_error != NULL)
    {
        *decode_error = validation_error;
    }
    if (validation_error != BLE_STATUS_DECODE_OK)
    {
        return false;
    }
    *output = candidate;
    return true;
}

static uint32_t read_u32_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) |
           ((uint32_t)bytes[3] << 24U);
}

static float read_float32_le(const uint8_t *bytes)
{
    const uint32_t bits = read_u32_le(bytes);
    float value = 0.0F;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static bool copy_version(const uint8_t source[BLE_STATUS_VERSION_FIELD_LENGTH],
                         char output[BLE_STATUS_VERSION_CAPACITY])
{
    size_t length = 0U;
    while (length < BLE_STATUS_VERSION_FIELD_LENGTH && source[length] != 0U)
    {
        if (source[length] < 0x20U || source[length] > 0x7EU)
        {
            output[0] = '\0';
            return true;
        }
        output[length] = (char)source[length];
        ++length;
    }
    output[length] = '\0';
    return false;
}

static ble_status_decode_error_t validate_critical_fields(
    const ble_status_frame_t *candidate)
{
    if (!isfinite(candidate->current_mode))
    {
        return BLE_STATUS_DECODE_CURRENT_MODE_NONFINITE;
    }
    if (!isfinite(candidate->steps))
    {
        return BLE_STATUS_DECODE_STEPS_NONFINITE;
    }
    if (!isfinite(candidate->step_frequency))
    {
        return BLE_STATUS_DECODE_STEP_FREQUENCY_NONFINITE;
    }
    if (!isfinite(candidate->left_gear))
    {
        return BLE_STATUS_DECODE_LEFT_GEAR_NONFINITE;
    }
    if (!isfinite(candidate->right_gear))
    {
        return BLE_STATUS_DECODE_RIGHT_GEAR_NONFINITE;
    }
    if (!isfinite(candidate->balance_factor))
    {
        return BLE_STATUS_DECODE_BALANCE_FACTOR_NONFINITE;
    }
    if (candidate->steps < 0.0F)
    {
        return BLE_STATUS_DECODE_STEPS_OUT_OF_RANGE;
    }
    const watch_exoskeleton_scene_config_t config =
        (watch_exoskeleton_scene_config_t)candidate->scene_config;
    if (!WATCH_EXOSKELETON_SCENE_CONFIG_IS_VALID(config))
    {
        return BLE_STATUS_DECODE_SCENE_CONFIG_INVALID;
    }
    const float max_gear = config == WATCH_EXOSKELETON_SCENE_CONFIG_FULL
                               ? (float)WATCH_EXOSKELETON_GEAR_MAX
                               : (float)WATCH_EXOSKELETON_NON_EXTREME_GEAR_MAX;
    if (candidate->left_gear < (float)WATCH_EXOSKELETON_GEAR_MIN ||
        candidate->left_gear > max_gear)
    {
        return BLE_STATUS_DECODE_LEFT_GEAR_OUT_OF_RANGE;
    }
    if (candidate->right_gear < (float)WATCH_EXOSKELETON_GEAR_MIN ||
        candidate->right_gear > max_gear)
    {
        return BLE_STATUS_DECODE_RIGHT_GEAR_OUT_OF_RANGE;
    }
    if (candidate->balance_factor < -1.0F ||
        candidate->balance_factor > 1.0F)
    {
        return BLE_STATUS_DECODE_BALANCE_FACTOR_OUT_OF_RANGE;
    }
    if (candidate->motor_enable > 1U)
    {
        return BLE_STATUS_DECODE_MOTOR_ENABLE_OUT_OF_RANGE;
    }
    if (candidate->battery_level > 100U)
    {
        return BLE_STATUS_DECODE_BATTERY_OUT_OF_RANGE;
    }
    if (candidate->scene_mode < 1U || candidate->scene_mode > 4U)
    {
        return BLE_STATUS_DECODE_SCENE_MODE_OUT_OF_RANGE;
    }
    uint8_t normalized_scene_mode = 0U;
    if (!watch_exoskeleton_scene_mode_from_protocol_config(
            config,
            candidate->scene_mode,
            &normalized_scene_mode))
    {
        return BLE_STATUS_DECODE_SCENE_MODE_MODEL_INCOMPATIBLE;
    }
    return BLE_STATUS_DECODE_OK;
}

static bool owner_tokens_equal(const ble_status_owner_token_t *left,
                               const ble_status_owner_token_t *right)
{
    return left != NULL && right != NULL &&
           left->conn_handle == right->conn_handle &&
           left->link_generation == right->link_generation &&
           memcmp(left->mac, right->mac, BLE_STATUS_MAC_CAPACITY) == 0;
}

static bool canonical_token_mac_is_valid(
    const char mac[BLE_STATUS_MAC_CAPACITY])
{
    if (mac == NULL ||
        strnlen(mac, BLE_STATUS_MAC_CAPACITY) != BLE_STATUS_MAC_CAPACITY - 1U)
    {
        return false;
    }
    for (size_t index = 0U; index < BLE_STATUS_MAC_CAPACITY - 1U; ++index)
    {
        if ((index + 1U) % 3U == 0U)
        {
            if (mac[index] != ':')
            {
                return false;
            }
        }
        else if (!isdigit((unsigned char)mac[index]) &&
                 (mac[index] < 'A' || mac[index] > 'F'))
        {
            return false;
        }
    }
    return true;
}

static void increment_saturated(uint32_t *counter)
{
    if (counter != NULL && *counter < UINT32_MAX)
    {
        ++(*counter);
    }
}
