/**
 * @file     ui_shezhi_settings_policy.c
 * @brief    设置页投影纯逻辑实现。
 * @details  禁止输出 medium；禁止非词表 sync action；五态只改文案/色。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_shezhi_settings_policy.h"

#include <stdio.h>
#include <string.h>

static void copy_identity(char *out,
                          size_t out_len,
                          const char *value,
                          bool configured,
                          bool *muted)
{
    if (out == NULL || out_len == 0U) {
        return;
    }
    if (!configured || value == NULL || value[0] == '\0') {
        (void)snprintf(out, out_len, "%s", EWF_UI_SHEZHI_IDENTITY_PLACEHOLDER);
        if (muted != NULL) {
            *muted = true;
        }
        return;
    }
    /* 未配置默认串不得伪装已绑定。 */
    if (strcmp(value, "ewf-unconfigured") == 0 ||
        strcmp(value, "0.0.0-dev") == 0) {
        (void)snprintf(out, out_len, "%s", EWF_UI_SHEZHI_IDENTITY_PLACEHOLDER);
        if (muted != NULL) {
            *muted = true;
        }
        return;
    }
    (void)snprintf(out, out_len, "%s", value);
    if (muted != NULL) {
        *muted = false;
    }
}

void ewf_ui_shezhi_sync_btn_from_status(uint8_t sync_status,
                                        ewf_ui_shezhi_sync_btn_view_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->action_color_hex = EWF_UI_SHEZHI_ACTION_HEX;
    switch (sync_status) {
    case EWF_UI_SHEZHI_SYNC_STATUS_BUSY:
        out->component_state = EWF_UI_SHEZHI_SYNC_BTN_BUSY;
        (void)snprintf(out->action_text, sizeof(out->action_text), "%s", "同步中");
        break;
    case EWF_UI_SHEZHI_SYNC_STATUS_OK:
        out->component_state = EWF_UI_SHEZHI_SYNC_BTN_OK;
        (void)snprintf(out->action_text, sizeof(out->action_text), "%s", "已同步");
        break;
    case EWF_UI_SHEZHI_SYNC_STATUS_PENDING:
        out->component_state = EWF_UI_SHEZHI_SYNC_BTN_PENDING;
        (void)snprintf(out->action_text, sizeof(out->action_text), "%s", "待同步");
        break;
    case EWF_UI_SHEZHI_SYNC_STATUS_FAIL:
        out->component_state = EWF_UI_SHEZHI_SYNC_BTN_FAIL;
        (void)snprintf(out->action_text, sizeof(out->action_text), "%s", "同步失败");
        break;
    case EWF_UI_SHEZHI_SYNC_STATUS_IDLE:
    default:
        out->component_state = EWF_UI_SHEZHI_SYNC_BTN_BASE;
        (void)snprintf(out->action_text, sizeof(out->action_text), "%s", "立即同步");
        break;
    }
}

int ewf_ui_shezhi_brightness_index(uint8_t brightness)
{
    if (brightness <= 2U) {
        return (int)brightness;
    }
    return 1; /* mid 默认 */
}

int ewf_ui_shezhi_timeout_index(uint32_t timeout_s)
{
    if (timeout_s == 5U) {
        return 0;
    }
    if (timeout_s == 30U) {
        return 2;
    }
    return 1; /* 15 默认 */
}

void ewf_ui_shezhi_volume_geometry(uint8_t volume,
                                   uint16_t *fill_w,
                                   int16_t *knob_x)
{
    uint8_t v = volume;
    if (v > 100U) {
        v = 100U;
    }
    const uint16_t track = (uint16_t)EWF_UI_SHEZHI_VOLUME_TRACK_W;
    const uint16_t fill =
        (uint16_t)(((uint32_t)v * (uint32_t)track) / 100U);
    if (fill_w != NULL) {
        *fill_w = fill;
    }
    if (knob_x != NULL) {
        int16_t x = (int16_t)(EWF_UI_SHEZHI_VOLUME_TRACK_X + (int)fill -
                              (EWF_UI_SHEZHI_VOLUME_KNOB_SIZE / 2));
        const int16_t min_x = (int16_t)EWF_UI_SHEZHI_VOLUME_TRACK_X;
        const int16_t max_x =
            (int16_t)(EWF_UI_SHEZHI_VOLUME_TRACK_X + track -
                      EWF_UI_SHEZHI_VOLUME_KNOB_SIZE);
        if (x < min_x) {
            x = min_x;
        }
        if (x > max_x) {
            x = max_x;
        }
        *knob_x = x;
    }
}

bool ewf_ui_shezhi_sync_action_allowed(const char *text)
{
    if (text == NULL) {
        return false;
    }
    return strcmp(text, "立即同步") == 0 || strcmp(text, "同步中") == 0 ||
           strcmp(text, "已同步") == 0 || strcmp(text, "待同步") == 0 ||
           strcmp(text, "同步失败") == 0;
}

void ewf_ui_shezhi_settings_project(const ewf_ui_shezhi_settings_input_t *in,
                                    ewf_ui_shezhi_settings_view_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->page1_row_count_ok = true;
    out->brightness_index = 1;
    out->timeout_index = 1;
    out->volume = 50U;
    ewf_ui_shezhi_volume_geometry(out->volume, &out->volume_fill_w,
                                  &out->volume_knob_x);
    (void)snprintf(out->volume_text, sizeof(out->volume_text), "50");
    ewf_ui_shezhi_sync_btn_from_status(EWF_UI_SHEZHI_SYNC_STATUS_IDLE, &out->sync_btn);
    bool muted = true;
    copy_identity(out->firmware_text, sizeof(out->firmware_text), NULL, false,
                  &muted);
    copy_identity(out->device_id_text, sizeof(out->device_id_text), NULL, false,
                  &muted);
    out->identity_muted = true;

    if (in == NULL) {
        return;
    }

    out->brightness_index = ewf_ui_shezhi_brightness_index(in->brightness);
    out->timeout_index = ewf_ui_shezhi_timeout_index(in->timeout_s);
    out->volume = (in->volume > 100U) ? 100U : in->volume;
    ewf_ui_shezhi_volume_geometry(out->volume, &out->volume_fill_w,
                                  &out->volume_knob_x);
    (void)snprintf(out->volume_text, sizeof(out->volume_text), "%u",
                   (unsigned)out->volume);
    ewf_ui_shezhi_sync_btn_from_status(in->sync_status, &out->sync_btn);

    bool fw_muted = false;
    bool id_muted = false;
    copy_identity(out->firmware_text, sizeof(out->firmware_text),
                  in->firmware_version, in->identity_configured, &fw_muted);
    copy_identity(out->device_id_text, sizeof(out->device_id_text), in->device_id,
                  in->identity_configured, &id_muted);
    out->identity_muted = fw_muted || id_muted;
}
