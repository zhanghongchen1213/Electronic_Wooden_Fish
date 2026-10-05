/**
 * @file     ui_runtime_binding.c
 * @brief    EWF 壳层投影实现：导航快照 → pager/设置/状态栏。
 * @details  去掉 legbot exo/BLE/等遗留投影。Wi-Fi/BLE/GPS 默认 disabled。
 *           同步槽默认「待同步」，禁止伪造「已同步成功」。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_runtime_binding.h"

#include "ewf_ui_geometry.h"
#include "ui_shell_policy.h"
#include "ui_shezhi_projection.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "UI_BIND";

static ewf_ui_signal_state_t s_inj_4g = EWF_UI_SIGNAL_NO_SIGNAL;
static ewf_ui_signal_state_t s_inj_wifi = EWF_UI_SIGNAL_DISABLED;
static ewf_ui_signal_state_t s_inj_ble = EWF_UI_SIGNAL_DISABLED;
static ewf_ui_signal_state_t s_inj_gps = EWF_UI_SIGNAL_DISABLED;
static ewf_ui_sync_phrase_t s_inj_sync = EWF_UI_SYNC_PHRASE_PENDING;
static int s_inj_battery = -1;
static bool s_ready;
/** 最近一次已投影的设置开闭，避免每轮 poll 重复 load_scr。 */
static bool s_applied_settings_open;
/** 是否已完成至少一次设置屏投影。 */
static bool s_settings_projection_primed;

/* 状态栏状态色（设计对照板锚定：信号 connected=#b8c7a4 / no-signal=#81786a /
   disabled=#5f564b；电池 #c9a66a、低电 #c77b55；sync 文案 #cfc4b0、电量值 #f5efe2）。
   sync 圆点四态 HTML 只锚定 ok=#b8c7a4，其余按产品语义映射（契约允许运行时补丁）。 */
#define EWF_UI_SB_SIGNAL_CONNECTED_HEX 0xB8C7A4U
#define EWF_UI_SB_SIGNAL_NO_SIGNAL_HEX 0x81786AU
#define EWF_UI_SB_SIGNAL_DISABLED_HEX 0x5F564BU
#define EWF_UI_SB_BATTERY_HEX 0xC9A66AU
#define EWF_UI_SB_BATTERY_LOW_HEX 0xC77B55U
#define EWF_UI_SB_DOT_OK_HEX 0xB8C7A4U
#define EWF_UI_SB_DOT_BUSY_HEX 0xD9A441U
#define EWF_UI_SB_DOT_PENDING_HEX 0x81786AU
#define EWF_UI_SB_DOT_FAIL_HEX 0xC77B55U

/** 电量图标低电阈值（<30% 切 low 变体与红色）。 */
#define EWF_UI_SB_BATTERY_LOW_PERCENT 30

static const char *sync_text(ewf_ui_sync_phrase_t phrase)
{
    switch (phrase) {
    case EWF_UI_SYNC_PHRASE_BUSY:
        return "同步中";
    case EWF_UI_SYNC_PHRASE_FAIL:
        return "同步失败";
    case EWF_UI_SYNC_PHRASE_OK:
        return "已同步";
    case EWF_UI_SYNC_PHRASE_PENDING:
    default:
        return "待同步";
    }
}

static uint32_t signal_color(ewf_ui_signal_state_t st)
{
    switch (st) {
    case EWF_UI_SIGNAL_CONNECTED:
        return EWF_UI_SB_SIGNAL_CONNECTED_HEX;
    case EWF_UI_SIGNAL_NO_SIGNAL:
        return EWF_UI_SB_SIGNAL_NO_SIGNAL_HEX;
    case EWF_UI_SIGNAL_DISABLED:
    default:
        return EWF_UI_SB_SIGNAL_DISABLED_HEX;
    }
}

static void patch_signal_icon(lv_obj_t *icon,
                              ewf_ui_signal_state_t st,
                              const lv_img_dsc_t *connected_src,
                              const lv_img_dsc_t *off_src,
                              const lv_img_dsc_t *zero_src)
{
    if (icon == NULL) {
        return;
    }
    const lv_img_dsc_t *src = off_src;
    switch (st) {
    case EWF_UI_SIGNAL_CONNECTED:
        src = connected_src;
        break;
    case EWF_UI_SIGNAL_NO_SIGNAL:
        src = zero_src;
        break;
    case EWF_UI_SIGNAL_DISABLED:
    default:
        src = off_src;
        break;
    }
    lv_img_set_src(icon, src);
    lv_obj_set_style_img_recolor(icon, lv_color_hex(signal_color(st)),
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void patch_sync_dot(lv_obj_t *dot, ewf_ui_sync_phrase_t phrase)
{
    if (dot == NULL) {
        return;
    }
    uint32_t hex = EWF_UI_SB_DOT_PENDING_HEX;
    switch (phrase) {
    case EWF_UI_SYNC_PHRASE_OK:
        hex = EWF_UI_SB_DOT_OK_HEX;
        break;
    case EWF_UI_SYNC_PHRASE_BUSY:
        hex = EWF_UI_SB_DOT_BUSY_HEX;
        break;
    case EWF_UI_SYNC_PHRASE_FAIL:
        hex = EWF_UI_SB_DOT_FAIL_HEX;
        break;
    case EWF_UI_SYNC_PHRASE_PENDING:
    default:
        hex = EWF_UI_SB_DOT_PENDING_HEX;
        break;
    }
    lv_obj_set_style_bg_color(dot, lv_color_hex(hex), LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void patch_battery(lv_obj_t *icon, lv_obj_t *txt, int percent)
{
    const bool invalid = (percent < 0 || percent > 100);
    const bool low = (!invalid && percent < EWF_UI_SB_BATTERY_LOW_PERCENT);
    if (icon != NULL) {
        const lv_img_dsc_t *src = &ui_img_ic_battery_medium;
        if (invalid) {
            src = &ui_img_ic_battery_medium;
        } else if (low) {
            src = &ui_img_ic_battery_low;
        } else if (percent >= 80) {
            src = &ui_img_ic_battery_full;
        }
        lv_img_set_src(icon, src);
        lv_obj_set_style_img_recolor(icon,
                                     lv_color_hex(low ? EWF_UI_SB_BATTERY_LOW_HEX
                                                      : EWF_UI_SB_BATTERY_HEX),
                                     LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (txt != NULL) {
        char buf[8];
        if (invalid) {
            lv_label_set_text(txt, "--%");
        } else {
            (void)snprintf(buf, sizeof(buf), "%d%%", percent);
            lv_label_set_text(txt, buf);
        }
    }
}

static void patch_statusbar(lv_obj_t *bar, const ewf_ui_shell_model_t *model)
{
    if (bar == NULL || model == NULL) {
        return;
    }
    lv_obj_t *icon_4g = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_4G);
    lv_obj_t *icon_wifi = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_WIFI);
    lv_obj_t *icon_ble = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_BLE);
    lv_obj_t *icon_gps = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_GPS);
    lv_obj_t *dot_sync = ui_comp_get_child(bar, UI_COMP_STATUSBAR_DOT_SYNC);
    lv_obj_t *txt_sync = ui_comp_get_child(bar, UI_COMP_STATUSBAR_TXT_SYNC);
    lv_obj_t *txt_bat = ui_comp_get_child(bar, UI_COMP_STATUSBAR_TXT_BATTERY);
    lv_obj_t *icon_bat = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_BATTERY);

    patch_signal_icon(icon_4g, model->signal_4g,
                      &ui_img_ic_signal, &ui_img_ic_signal_zero, &ui_img_ic_signal_zero);
    patch_signal_icon(icon_wifi, model->signal_wifi,
                      &ui_img_ic_wifi, &ui_img_ic_wifi_off, &ui_img_ic_wifi_off);
    patch_signal_icon(icon_ble, model->signal_ble,
                      &ui_img_ic_bluetooth, &ui_img_ic_bluetooth_off, &ui_img_ic_bluetooth_off);
    patch_signal_icon(icon_gps, model->signal_gps,
                      &ui_img_ic_navigation, &ui_img_ic_navigation_off, &ui_img_ic_navigation_off);
    patch_sync_dot(dot_sync, model->sync_phrase);
    if (txt_sync) {
        lv_label_set_text(txt_sync, sync_text(model->sync_phrase));
    }
    patch_battery(icon_bat, txt_bat, model->battery_percent);
}

esp_err_t ui_runtime_binding_init(void)
{
    s_ready = true;
    s_settings_projection_primed = false;
    s_applied_settings_open = false;
    return ESP_OK;
}

int ui_runtime_binding_page_offset_x(watch_nav_page_t page)
{
    return ewf_ui_shell_page_offset_x((ewf_ui_shell_page_t)page);
}

void ui_runtime_binding_build_model(const watch_state_snapshot_t *snapshot,
                                    ewf_ui_shell_model_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->signal_wifi = s_inj_wifi;
    out->signal_ble = s_inj_ble;
    out->signal_gps = s_inj_gps;
    out->signal_4g = s_inj_4g;
    out->sync_phrase = s_inj_sync;
    out->battery_percent = s_inj_battery;
    out->display_on_request = true;
    if (snapshot == NULL) {
        out->active_page = WATCH_NAV_PAGE_MUYU;
        out->settings_open = false;
        out->screen_state = WATCH_SCREEN_STATE_ON;
        return;
    }
    out->active_page = snapshot->nav.active_page;
    if (out->active_page >= WATCH_NAV_PAGE_COUNT) {
        out->active_page = WATCH_NAV_PAGE_MUYU;
    }
    out->settings_open = snapshot->nav.settings_open;
    out->screen_state = snapshot->screen_state;
    out->display_on_request = (snapshot->screen_state == WATCH_SCREEN_STATE_ON);
    if (snapshot->battery_valid) {
        out->battery_percent = (int)snapshot->battery_percent;
    }
}

esp_err_t ui_runtime_binding_apply(const ewf_ui_shell_model_t *model)
{
    if (!s_ready || model == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (ui_shell_pager != NULL) {
        lv_obj_set_x(ui_shell_pager, -ui_runtime_binding_page_offset_x(model->active_page));
    }

    const bool settings_changed =
        (!s_settings_projection_primed) || (s_applied_settings_open != model->settings_open);
    if (settings_changed) {
        if (model->settings_open) {
            if (ui_scr_shezhi == NULL) {
                ui_scr_shezhi_screen_init();
            }
            if (ui_scr_shezhi != NULL) {
                lv_disp_load_scr(ui_scr_shezhi);
            }
        } else {
            if (ui_scr_shell != NULL) {
                lv_disp_load_scr(ui_scr_shell);
            }
            if (ui_scr_shezhi != NULL) {
                ui_scr_shezhi_screen_destroy();
                /* 按需 Screen 销毁后必须失效脏标记，否则重开会跳过投影。 */
                ui_shezhi_projection_invalidate();
            }
        }
        s_applied_settings_open = model->settings_open;
        s_settings_projection_primed = true;
    }

    patch_statusbar(ui_muyu_statusbar, model);
    patch_statusbar(ui_jingwen_statusbar, model);
    patch_statusbar(ui_tongji_statusbar, model);
    (void)TAG;
    return ESP_OK;
}

void ui_runtime_binding_inject_statusbar(ewf_ui_signal_state_t signal_4g,
                                         ewf_ui_sync_phrase_t sync_phrase,
                                         int battery_percent)
{
    s_inj_4g = signal_4g;
    s_inj_sync = sync_phrase;
    s_inj_battery = battery_percent;
}

void ui_runtime_binding_get_statusbar_defaults(ewf_ui_signal_state_t *wifi,
                                               ewf_ui_signal_state_t *ble,
                                               ewf_ui_signal_state_t *gps)
{
    if (wifi) {
        *wifi = EWF_UI_SIGNAL_DISABLED;
    }
    if (ble) {
        *ble = EWF_UI_SIGNAL_DISABLED;
    }
    if (gps) {
        *gps = EWF_UI_SIGNAL_DISABLED;
    }
}
