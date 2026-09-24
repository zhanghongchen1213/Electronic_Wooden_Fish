#ifndef _EWF_UI_COMP_HOOK_H
#define _EWF_UI_COMP_HOOK_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"
void ui_comp_statusbar_create_hook(lv_obj_t * comp);
void ui_comp_woodfish_mark_create_hook(lv_obj_t * comp);
#ifdef __cplusplus
}
#endif
#endif
