#include "ui_helpers.h"
void _ui_bar_set_property(lv_obj_t * target, int id, int val) {
    if(id == _UI_BAR_PROPERTY_VALUE_WITH_ANIM) lv_bar_set_value(target, val, LV_ANIM_ON);
    if(id == _UI_BAR_PROPERTY_VALUE) lv_bar_set_value(target, val, LV_ANIM_OFF);
}
void _ui_basic_set_property(lv_obj_t * target, int id, int val) { (void)target; (void)id; (void)val; }
void _ui_label_set_property(lv_obj_t * target, int id, const char * val) {
    if(id == 0) lv_label_set_text(target, val);
}
void _ui_screen_change(lv_obj_t ** target, lv_scr_load_anim_t fademode, int spd, int delay, void (*target_init)(void)) {
    if(*target == NULL) target_init();
    lv_scr_load_anim(*target, fademode, spd, delay, false);
}
void _ui_screen_delete(lv_obj_t ** target) {
    if(*target == NULL) return;
    lv_obj_del(*target);
    *target = NULL;
}
