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

static const char *signal_text(ewf_ui_signal_state_t st, const char *on_label)
{
    switch (st) {
    case EWF_UI_SIGNAL_CONNECTED:
        return on_label;
    case EWF_UI_SIGNAL_NO_SIGNAL:
        return "--";
    case EWF_UI_SIGNAL_DISABLED:
    default:
        return "-";
    }
}

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

static void patch_statusbar(lv_obj_t *bar, const ewf_ui_shell_model_t *model)
{
    if (bar == NULL || model == NULL) {
        return;
    }
    lv_obj_t *icon_4g = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_4G);
    lv_obj_t *icon_wifi = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_WIFI);
    lv_obj_t *icon_ble = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_BLE);
    lv_obj_t *icon_gps = ui_comp_get_child(bar, UI_COMP_STATUSBAR_ICON_GPS);
    lv_obj_t *txt_sync = ui_comp_get_child(bar, UI_COMP_STATUSBAR_TXT_SYNC);
    lv_obj_t *txt_bat = ui_comp_get_child(bar, UI_COMP_STATUSBAR_TXT_BATTERY);

    if (icon_4g) {
        lv_label_set_text(icon_4g, signal_text(model->signal_4g, "4G"));
    }
    if (icon_wifi) {
        lv_label_set_text(icon_wifi, signal_text(model->signal_wifi, "Wi"));
    }
    if (icon_ble) {
        lv_label_set_text(icon_ble, signal_text(model->signal_ble, "BT"));
    }
    if (icon_gps) {
        lv_label_set_text(icon_gps, signal_text(model->signal_gps, "GP"));
    }
    if (txt_sync) {
        lv_label_set_text(txt_sync, sync_text(model->sync_phrase));
    }
    if (txt_bat) {
        char buf[8];
        if (model->battery_percent < 0 || model->battery_percent > 100) {
            lv_label_set_text(txt_bat, "--%");
        } else {
            (void)snprintf(buf, sizeof(buf), "%d%%", model->battery_percent);
            lv_label_set_text(txt_bat, buf);
        }
    }
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
