#ifndef UI_COMP_CMP_STATUSBAR_H
#define UI_COMP_CMP_STATUSBAR_H
#ifdef __cplusplus
extern "C" {
#endif
#include "lvgl/lvgl.h"
enum {
    UI_COMP_STATUSBAR_STATUSBAR = 0,
    UI_COMP_STATUSBAR_ICON_4G,
    UI_COMP_STATUSBAR_ICON_WIFI,
    UI_COMP_STATUSBAR_ICON_BLE,
    UI_COMP_STATUSBAR_ICON_GPS,
    UI_COMP_STATUSBAR_TXT_SYNC,
    UI_COMP_STATUSBAR_TXT_BATTERY,
    UI_COMP_STATUSBAR_ICON_BATTERY,
    _UI_COMP_STATUSBAR_NUM
};
lv_obj_t * ui_statusbar_create(lv_obj_t * comp_parent);
#ifdef __cplusplus
}
#endif
#endif
