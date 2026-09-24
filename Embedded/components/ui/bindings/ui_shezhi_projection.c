/**
 * @file     ui_shezhi_projection.c
 * @brief    设置页 LVGL 投影实现。
 * @details  所有 lv_* 仅 ui_task；五态只覆写 sync-btn action；无 woodfish。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_shezhi_projection.h"

#include "esp_log.h"
#include "ui.h"
#include "ui_shezhi_constants.h"
#include "ui_shezhi_settings_policy.h"

#include <string.h>

static const char *TAG = "UI_SHEZHI";

static bool s_ready;
static bool s_primed;
static bool s_applying;
static ewf_ui_shezhi_settings_view_t s_prev;
static int16_t s_scroll_y;

static void style_capsule(lv_obj_t *cap, lv_obj_t *label, bool selected)
{
    if (cap == NULL || label == NULL) {
        return;
    }
    const uint32_t bg =
        selected ? EWF_UI_SHEZHI_SELECTED_BG_HEX : EWF_UI_SHEZHI_SEG_BG_HEX;
    const uint32_t fg =
        selected ? EWF_UI_SHEZHI_SELECTED_FG_HEX : EWF_UI_SHEZHI_UNSELECTED_FG_HEX;
    lv_obj_set_style_bg_color(cap, lv_color_hex(bg), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(fg),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void apply_view(const ewf_ui_shezhi_settings_view_t *view)
{
    if (view == NULL) {
        return;
    }

    /* 抑制 lv_slider_set_value 触发的 VALUE_CHANGED → 音量回写。 */
    s_applying = true;

    for (int i = 0; i < 3; ++i) {
        style_capsule(ui_shezhi_brightness_caps[i],
                      ui_shezhi_brightness_cap_labels[i],
                      i == view->brightness_index);
        style_capsule(ui_shezhi_timeout_caps[i], ui_shezhi_timeout_cap_labels[i],
                      i == view->timeout_index);
    }

    if (ui_shezhi_volume_slider != NULL) {
        lv_slider_set_value(ui_shezhi_volume_slider, (int32_t)view->volume,
                            LV_ANIM_OFF);
    }
    if (ui_shezhi_volume_value != NULL) {
        lv_label_set_text(ui_shezhi_volume_value, view->volume_text);
    }

    if (ui_shezhi_sync_action_label != NULL) {
        lv_label_set_text(ui_shezhi_sync_action_label, view->sync_btn.action_text);
        lv_obj_set_style_text_color(
            ui_shezhi_sync_action_label,
            lv_color_hex(view->sync_btn.action_color_hex),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    const uint32_t id_color =
        view->identity_muted ? EWF_UI_SHEZHI_MUTED_HEX : EWF_UI_SHEZHI_LABEL_HEX;
    if (ui_shezhi_version_value != NULL) {
        lv_label_set_text(ui_shezhi_version_value, view->firmware_text);
        lv_obj_set_style_text_color(ui_shezhi_version_value, lv_color_hex(id_color),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_shezhi_device_id_value != NULL) {
        lv_label_set_text(ui_shezhi_device_id_value, view->device_id_text);
        lv_obj_set_style_text_color(ui_shezhi_device_id_value, lv_color_hex(id_color),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    /* 不在每次 apply 强制 scroll：保留用户原生滚动；注入走 set_scroll_y。 */
    s_applying = false;
    (void)TAG;
}

esp_err_t ui_shezhi_projection_init(void)
{
    s_ready = true;
    s_primed = false;
    s_applying = false;
    s_scroll_y = 0;
    memset(&s_prev, 0, sizeof(s_prev));
    return ESP_OK;
}

void ui_shezhi_projection_invalidate(void)
{
    s_primed = false;
    s_applying = false;
    s_scroll_y = 0;
    memset(&s_prev, 0, sizeof(s_prev));
}

bool ui_shezhi_projection_is_applying(void)
{
    return s_applying;
}

void ui_shezhi_projection_set_scroll_y(int16_t scroll_y)
{
    if (scroll_y < 0) {
        scroll_y = 0;
    }
    s_scroll_y = scroll_y;
    if (ui_shezhi_viewport != NULL) {
        lv_obj_scroll_to_y(ui_shezhi_viewport, s_scroll_y, LV_ANIM_OFF);
    }
}

int16_t ui_shezhi_projection_get_scroll_y(void)
{
    if (ui_shezhi_viewport != NULL) {
        return (int16_t)lv_obj_get_scroll_y(ui_shezhi_viewport);
    }
    return s_scroll_y;
}

void ui_shezhi_projection_apply(const watch_state_snapshot_t *snapshot,
                                const char *firmware_version,
                                const char *device_id,
                                bool identity_configured)
{
    if (!s_ready || ui_scr_shezhi == NULL) {
        return;
    }

    ewf_ui_shezhi_settings_input_t in;
    memset(&in, 0, sizeof(in));
    if (snapshot != NULL) {
        in.volume = snapshot->nav.volume;
        in.brightness = (uint8_t)snapshot->nav.brightness;
        in.timeout_s = snapshot->nav.timeout_s;
        in.sync_status = (uint8_t)snapshot->nav.sync_status;
    }
    in.firmware_version = firmware_version;
    in.device_id = device_id;
    in.identity_configured = identity_configured;

    ewf_ui_shezhi_settings_view_t view;
    ewf_ui_shezhi_settings_project(&in, &view);

    if (s_primed && memcmp(&view, &s_prev, sizeof(view)) == 0) {
        return;
    }
    apply_view(&view);
    s_prev = view;
    s_primed = true;
}
