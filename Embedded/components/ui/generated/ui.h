// SquareLine Studio 1.6.1 / LVGL 8.3.11 / Project: ewf-device
#ifndef _EWF_DEVICE_UI_H
#define _EWF_DEVICE_UI_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"
#include "ewf_ui_geometry.h"
#include "ui_helpers.h"
#include "components/ui_comp.h"
#include "components/ui_comp_hook.h"
#include "ui_events.h"
#include "screens/ui_scr_shell.h"
#include "screens/ui_scr_shezhi.h"
#include "images/ui_images.h"

extern lv_obj_t * ui____initial_actions0;

LV_FONT_DECLARE(ui_font_ns600_16);
LV_FONT_DECLARE(ui_font_ns700_22);
LV_FONT_DECLARE(ui_font_serif700_52);
LV_FONT_DECLARE(ui_font_ns500_11);
LV_FONT_DECLARE(ui_font_ns500_12);
LV_FONT_DECLARE(ui_font_ns500_13);
LV_FONT_DECLARE(ui_font_ns600_14);
LV_FONT_DECLARE(ui_font_ns500_15);
LV_FONT_DECLARE(ui_font_ns600_18);
LV_FONT_DECLARE(ui_font_ns600_20);
LV_FONT_DECLARE(ui_font_ns600_22);
LV_FONT_DECLARE(ui_font_ns700_26);
LV_FONT_DECLARE(ui_font_serif600_22);
LV_FONT_DECLARE(ui_font_serif700_24);
LV_FONT_DECLARE(ui_font_serif500_24);
LV_FONT_DECLARE(ui_font_serif700_48);

void ui_init(void);
void ui_destroy(void);
#ifdef __cplusplus
}
#endif
#endif
