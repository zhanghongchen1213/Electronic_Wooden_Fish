// SquareLine Studio 1.6.1 / LVGL 8.3.11 export shape — ewf-device
#ifndef _EWF_DEVICE_UI_HELPERS_H
#define _EWF_DEVICE_UI_HELPERS_H
#ifdef __cplusplus
extern "C" {
#endif
#include "ui.h"
#define _UI_TEMPORARY_STRING_BUFFER_SIZE 32
#define _UI_BAR_PROPERTY_VALUE 0
#define _UI_BAR_PROPERTY_VALUE_WITH_ANIM 1
void _ui_bar_set_property(lv_obj_t * target, int id, int val);
void _ui_basic_set_property(lv_obj_t * target, int id, int val);
void _ui_label_set_property(lv_obj_t * target, int id, const char * val);
void _ui_screen_change(lv_obj_t ** target, lv_scr_load_anim_t fademode, int spd, int delay, void (*target_init)(void));
void _ui_screen_delete(lv_obj_t ** target);
#ifdef __cplusplus
} /*extern "C"*/
#endif
#endif
