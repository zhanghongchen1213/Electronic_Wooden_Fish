/**
 * @file     device_settings_policy.c
 * @brief    设备设置字段校验与默认值实现。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_settings_policy.h"

#include <string.h>

void ewf_device_settings_defaults(ewf_device_settings_record_t *record)
{
    if (record == NULL) {
        return;
    }
    record->schema_version = EWF_DEVICE_SETTINGS_SCHEMA_VERSION;
    record->volume = EWF_DEVICE_SETTINGS_VOLUME_DEFAULT;
    record->brightness = EWF_BRIGHTNESS_MID;
    record->timeout_s = EWF_DEVICE_SETTINGS_TIMEOUT_DEFAULT;
    record->applied_revision = EWF_DEVICE_SETTINGS_APPLIED_REVISION_DEFAULT;
}

bool ewf_device_settings_volume_valid(uint8_t volume)
{
    return volume <= 100U;
}

bool ewf_device_settings_timeout_valid(uint32_t timeout_s)
{
    return timeout_s == 5U || timeout_s == 15U || timeout_s == 30U;
}

ewf_settings_result_t ewf_device_settings_validate(
    const ewf_device_settings_record_t *record)
{
    if (record == NULL) {
        return EWF_SETTINGS_NULL;
    }
    if (record->schema_version == 0U) {
        return EWF_SETTINGS_SCHEMA;
    }
    if (!ewf_device_settings_volume_valid(record->volume)) {
        return EWF_SETTINGS_VOLUME;
    }
    if ((unsigned)record->brightness >= (unsigned)EWF_BRIGHTNESS_COUNT) {
        return EWF_SETTINGS_BRIGHTNESS;
    }
    if (!ewf_device_settings_timeout_valid(record->timeout_s)) {
        return EWF_SETTINGS_TIMEOUT;
    }
    return EWF_SETTINGS_OK;
}

bool ewf_device_settings_brightness_parse(const char *name,
                                          ewf_brightness_t *out)
{
    if (name == NULL || out == NULL) {
        return false;
    }
    if (strcmp(name, "low") == 0) {
        *out = EWF_BRIGHTNESS_LOW;
        return true;
    }
    if (strcmp(name, "mid") == 0) {
        *out = EWF_BRIGHTNESS_MID;
        return true;
    }
    if (strcmp(name, "high") == 0) {
        *out = EWF_BRIGHTNESS_HIGH;
        return true;
    }
    /* medium 是 UX 传说，不得写入存储。 */
    return false;
}

const char *ewf_device_settings_brightness_name(ewf_brightness_t value)
{
    switch (value) {
    case EWF_BRIGHTNESS_LOW:
        return "low";
    case EWF_BRIGHTNESS_MID:
        return "mid";
    case EWF_BRIGHTNESS_HIGH:
        return "high";
    default:
        return "mid";
    }
}

uint16_t ewf_device_settings_brightness_raw(ewf_brightness_t value)
{
    switch (value) {
    case EWF_BRIGHTNESS_LOW:
        return EWF_BRIGHTNESS_RAW_LOW;
    case EWF_BRIGHTNESS_HIGH:
        return EWF_BRIGHTNESS_RAW_HIGH;
    case EWF_BRIGHTNESS_MID:
    default:
        return EWF_BRIGHTNESS_RAW_MID;
    }
}

bool ewf_device_settings_revision_ok(uint32_t previous, uint32_t next)
{
    return next >= previous;
}
