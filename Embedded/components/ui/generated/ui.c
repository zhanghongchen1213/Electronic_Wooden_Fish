// SquareLine Studio 1.6.1 / LVGL 8.3.11 / Project: ewf-device
#include "ui.h"
#include "ui_helpers.h"

lv_obj_t * ui____initial_actions0;

#if LV_COLOR_DEPTH != 16
#error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio settings"
#endif
#if LV_COLOR_16_SWAP !=1
#error "LV_COLOR_16_SWAP should be 1 to match SquareLine Studio settings"
#endif

void ui_init(void)
{
    LV_EVENT_GET_COMP_CHILD = lv_event_register_id();
    lv_disp_t * dispp = lv_disp_get_default();
    lv_theme_t * theme = lv_theme_basic_init(dispp);
    lv_disp_set_theme(dispp, theme);
    ui_scr_shell_screen_init();
    ui____initial_actions0 = lv_obj_create(NULL);
    lv_disp_load_scr(ui_scr_shell);
}

void ui_destroy(void)
{
    ui_scr_shezhi_screen_destroy();
    ui_scr_shell_screen_destroy();
}
