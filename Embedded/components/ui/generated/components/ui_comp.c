#include "../ui.h"
uint32_t LV_EVENT_GET_COMP_CHILD;
lv_obj_t * ui_comp_get_child(lv_obj_t * comp, uint32_t child_idx) {
    ui_comp_get_child_t info = { .child_idx = child_idx, .child = NULL };
    lv_event_send(comp, LV_EVENT_GET_COMP_CHILD, &info);
    return info.child;
}
