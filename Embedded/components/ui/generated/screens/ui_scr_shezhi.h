/**
 * @file     ui_scr_shezhi.h
 * @brief    SHEZHI 按需设置屏对象声明（generated 布局层）。
 */
#ifndef UI_SCR_SHEZHI_H
#define UI_SCR_SHEZHI_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"

extern void ui_scr_shezhi_screen_init(void);
extern void ui_scr_shezhi_screen_destroy(void);

extern lv_obj_t *ui_scr_shezhi;
extern lv_obj_t *ui_shezhi_title;
extern lv_obj_t *ui_shezhi_divider;
extern lv_obj_t *ui_shezhi_viewport;
extern lv_obj_t *ui_shezhi_content;

extern lv_obj_t *ui_shezhi_row_brightness;
extern lv_obj_t *ui_shezhi_brightness_label;
extern lv_obj_t *ui_shezhi_brightness_caps[3];
extern lv_obj_t *ui_shezhi_brightness_cap_labels[3];

extern lv_obj_t *ui_shezhi_row_timeout;
extern lv_obj_t *ui_shezhi_timeout_label;
extern lv_obj_t *ui_shezhi_timeout_caps[3];
extern lv_obj_t *ui_shezhi_timeout_cap_labels[3];

extern lv_obj_t *ui_shezhi_row_volume;
extern lv_obj_t *ui_shezhi_volume_label;
extern lv_obj_t *ui_shezhi_volume_slider;
extern lv_obj_t *ui_shezhi_volume_value;

extern lv_obj_t *ui_shezhi_row_sync;
extern lv_obj_t *ui_shezhi_sync_title;
extern lv_obj_t *ui_shezhi_sync_action;
extern lv_obj_t *ui_shezhi_sync_action_label;

extern lv_obj_t *ui_shezhi_row_version;
extern lv_obj_t *ui_shezhi_version_label;
extern lv_obj_t *ui_shezhi_version_value;
extern lv_obj_t *ui_shezhi_row_device_id;
extern lv_obj_t *ui_shezhi_device_id_label;
extern lv_obj_t *ui_shezhi_device_id_value;

#ifdef __cplusplus
}
#endif
#endif
