/**
 * @file     ui_shell_policy.h
 * @brief    DEVICE-01 壳层纯逻辑策略（无 LVGL，可主机测）。
 * @author   ZHC
 * @date     2026-09-24
 */
#ifndef EWF_UI_SHELL_POLICY_H
#define EWF_UI_SHELL_POLICY_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define EWF_UI_SHELL_CANVAS_W 410
#define EWF_UI_SHELL_CANVAS_H 502
#define EWF_UI_SHELL_CORNER_RADIUS 110
#define EWF_UI_SHELL_MARGIN_PX 16
#define EWF_UI_SHELL_STATUSBAR_H 24
#define EWF_UI_SHELL_PAGER_CONTENT_W 1230
typedef enum {
    EWF_UI_SHELL_PAGE_MUYU = 0,
    EWF_UI_SHELL_PAGE_JINGWEN,
    EWF_UI_SHELL_PAGE_TONGJI,
    EWF_UI_SHELL_PAGE_COUNT
} ewf_ui_shell_page_t;
int ewf_ui_shell_page_offset_x(ewf_ui_shell_page_t page);
bool ewf_ui_shell_settings_is_ondemand(bool settings_open, int pager_page_count);
bool ewf_ui_shell_wifi_ble_gps_default_disabled(void);
#ifdef __cplusplus
}
#endif
#endif
