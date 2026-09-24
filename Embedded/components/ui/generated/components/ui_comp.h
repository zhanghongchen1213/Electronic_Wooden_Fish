#ifndef _EWF_UI_COMP_H
#define _EWF_UI_COMP_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"
#include "ui_comp_cmp_statusbar.h"
#include "ui_comp_cmp_woodfish_mark.h"
typedef struct {
    uint32_t child_idx;
    lv_obj_t * child;
} ui_comp_get_child_t;
lv_obj_t * ui_comp_get_child(lv_obj_t * comp, uint32_t child_idx);
extern uint32_t LV_EVENT_GET_COMP_CHILD;
#ifdef __cplusplus
}
#endif
#endif
