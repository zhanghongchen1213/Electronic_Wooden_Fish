/**
 * @file     ble_control_packet.c
 * @brief    外骨骼 32 字节完整控制包 builder 实现
 * @details  从有效 mirror 复制开放字段、应用 typed override，并为所有非开放字段写入 v1 安全值。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "ble_control_packet.h"

#include "app_state.h"

#include <math.h>
#include <string.h>

/** 控制包固定首字节。 */
#define BLE_CONTROL_HEADER_FIRST 0x44U
/** 控制包固定第二字节。 */
#define BLE_CONTROL_HEADER_SECOND 0xAAU

_Static_assert(sizeof(float) == sizeof(uint32_t),
               "BLE control protocol requires 32-bit float values.");

static bool mirror_is_valid(const ble_status_frame_t *mirror);
static bool override_is_valid(const ble_control_packet_override_t *override);
static bool override_is_compatible(
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override);
static uint8_t gear_to_u8(float value);
static void write_float32_le(uint8_t destination[4], float value);

bool ble_control_packet_build(
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override,
    uint8_t packet[BLE_CONTROL_PACKET_LENGTH])
{
    static const uint8_t reserved_data[BLE_CONTROL_RESERVED_DATA_LENGTH] = {
        0x00U,
        0x00U,
        0x32U,
        0x32U,
        0x32U,
        0x32U,
        0x32U,
        0x32U,
        0x32U,
    };
    if (packet == NULL || !mirror_is_valid(mirror) ||
        !override_is_valid(override) ||
        !override_is_compatible(mirror, override))
    {
        return false;
    }

    uint8_t built[BLE_CONTROL_PACKET_LENGTH] = {0};
    built[0] = BLE_CONTROL_HEADER_FIRST;
    built[1] = BLE_CONTROL_HEADER_SECOND;
    built[2] = mirror->motor_enable;
    built[3] = gear_to_u8(mirror->left_gear);
    built[4] = gear_to_u8(mirror->right_gear);
    built[5] = mirror->scene_mode;
    memcpy(&built[10], reserved_data, sizeof(reserved_data));
    built[BLE_CONTROL_DEVICE_CHECK_OFFSET] = 1U;
    built[21] = mirror->standby_time;
    write_float32_le(&built[26], mirror->balance_factor);

    switch (override->kind)
    {
    case BLE_CONTROL_OVERRIDE_NONE:
        break;
    case BLE_CONTROL_OVERRIDE_MOTOR_ENABLE:
        built[2] = override->target;
        break;
    case BLE_CONTROL_OVERRIDE_GEAR:
        built[3] = override->target;
        built[4] = override->target;
        break;
    case BLE_CONTROL_OVERRIDE_SCENE_MODE:
        built[5] = override->target;
        break;
    case BLE_CONTROL_OVERRIDE_POWEROFF:
        built[BLE_CONTROL_SHUTDOWN_OFFSET] = 1U;
        break;
    case BLE_CONTROL_OVERRIDE_GEAR_SCENE_BOUNDARY:
        built[3] = override->target;
        built[4] = override->target;
        built[5] = override->target == 11U ? 3U : 1U;
        break;
    default:
        return false;
    }

    memcpy(packet, built, sizeof(built));
    return true;
}

static bool mirror_is_valid(const ble_status_frame_t *mirror)
{
    if (mirror == NULL ||
        memchr(mirror->version, '\0', sizeof(mirror->version)) == NULL)
    {
        return false;
    }
    const watch_exoskeleton_scene_config_t config = mirror->scene_config;
    const float max_gear = (float)watch_exoskeleton_scene_config_max_gear(config);
    uint8_t normalized_scene_mode = 0U;
    return mirror->motor_enable <= 1U &&
           isfinite(mirror->left_gear) &&
           mirror->left_gear >= (float)WATCH_EXOSKELETON_GEAR_MIN &&
           mirror->left_gear <= max_gear && isfinite(mirror->right_gear) &&
           mirror->right_gear >= (float)WATCH_EXOSKELETON_GEAR_MIN &&
           mirror->right_gear <= max_gear &&
           isfinite(mirror->balance_factor) &&
           mirror->balance_factor >= -1.0F &&
           mirror->balance_factor <= 1.0F &&
           watch_exoskeleton_scene_mode_from_protocol_config(
               config,
               mirror->scene_mode,
               &normalized_scene_mode);
}

static bool override_is_valid(const ble_control_packet_override_t *override)
{
    if (override == NULL)
    {
        return false;
    }
    switch (override->kind)
    {
    case BLE_CONTROL_OVERRIDE_NONE:
        return override->target == 0U;
    case BLE_CONTROL_OVERRIDE_MOTOR_ENABLE:
        return override->target <= 1U;
    case BLE_CONTROL_OVERRIDE_GEAR:
        return override->target >= WATCH_EXOSKELETON_GEAR_MIN &&
               override->target <= WATCH_EXOSKELETON_GEAR_MAX;
    case BLE_CONTROL_OVERRIDE_SCENE_MODE:
        return override->target >= 1U && override->target <= 4U;
    case BLE_CONTROL_OVERRIDE_POWEROFF:
        return override->target == 1U;
    case BLE_CONTROL_OVERRIDE_GEAR_SCENE_BOUNDARY:
        return override->target == 10U || override->target == 11U;
    default:
        return false;
    }
}

static bool override_is_compatible(
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override)
{
    if (mirror == NULL || override == NULL)
    {
        return false;
    }
    const watch_exoskeleton_scene_config_t config = mirror->scene_config;
    switch (override->kind)
    {
    case BLE_CONTROL_OVERRIDE_GEAR:
        return override->target <=
               watch_exoskeleton_scene_config_max_gear(config);
    case BLE_CONTROL_OVERRIDE_SCENE_MODE:
    {
        uint8_t normalized_scene_mode = 0U;
        return watch_exoskeleton_scene_mode_from_protocol_config(
            config,
            override->target,
            &normalized_scene_mode) &&
               watch_exoskeleton_scene_config_allows_control(
                   config,
                   normalized_scene_mode);
    }
    case BLE_CONTROL_OVERRIDE_GEAR_SCENE_BOUNDARY:
    {
        const uint8_t current_left_gear = gear_to_u8(mirror->left_gear);
        return config == WATCH_EXOSKELETON_SCENE_CONFIG_FULL &&
               ((current_left_gear == 10U && override->target == 11U) ||
                (current_left_gear == 11U && override->target == 10U));
    }
    case BLE_CONTROL_OVERRIDE_NONE:
    case BLE_CONTROL_OVERRIDE_MOTOR_ENABLE:
    case BLE_CONTROL_OVERRIDE_POWEROFF:
        return true;
    default:
        return false;
    }
}

static uint8_t gear_to_u8(float value)
{
    return (uint8_t)(value + 0.5F);
}

static void write_float32_le(uint8_t destination[4], float value)
{
    uint32_t bits = 0U;
    memcpy(&bits, &value, sizeof(bits));
    destination[0] = (uint8_t)(bits & 0xFFU);
    destination[1] = (uint8_t)((bits >> 8U) & 0xFFU);
    destination[2] = (uint8_t)((bits >> 16U) & 0xFFU);
    destination[3] = (uint8_t)((bits >> 24U) & 0xFFU);
}
