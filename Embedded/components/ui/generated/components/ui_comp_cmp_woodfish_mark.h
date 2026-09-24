#ifndef UI_COMP_CMP_WOODFISH_MARK_H
#define UI_COMP_CMP_WOODFISH_MARK_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"
enum {
    UI_COMP_WOODFISH_MARK_ROOT = 0,
    UI_COMP_WOODFISH_MARK_PLACEHOLDER,
    _UI_COMP_WOODFISH_MARK_NUM
};
lv_obj_t * ui_woodfish_mark_create(lv_obj_t * comp_parent);
#ifdef __cplusplus
}
#endif
#endif
