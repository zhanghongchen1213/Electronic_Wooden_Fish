// SquareLine Studio 1.6.1 / ewf-device — cmp_statusbar
#include "../ui.h"

typedef struct {
    lv_obj_t * kids[_UI_COMP_STATUSBAR_NUM];
} statusbar_kids_t;

static void statusbar_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * target = lv_event_get_target(e);
    statusbar_kids_t * kids = (statusbar_kids_t *)lv_event_get_user_data(e);
    if(code == LV_EVENT_GET_COMP_CHILD && kids != NULL) {
        ui_comp_get_child_t * info = (ui_comp_get_child_t *)lv_event_get_param(e);
        if(info != NULL && info->child_idx < _UI_COMP_STATUSBAR_NUM) {
            info->child = kids->kids[info->child_idx];
        }
        return;
    }
    if(code == LV_EVENT_DELETE && kids != NULL) {
        lv_mem_free(kids);
        lv_obj_remove_event_cb(target, statusbar_event_cb);
    }
}

lv_obj_t * ui_statusbar_create(lv_obj_t * comp_parent)
{
    statusbar_kids_t * kids = lv_mem_alloc(sizeof(statusbar_kids_t));
    lv_memset_00(kids, sizeof(statusbar_kids_t));

    lv_obj_t * cui_statusbar = lv_obj_create(comp_parent);
    lv_obj_set_width(cui_statusbar, 314);
    lv_obj_set_height(cui_statusbar, EWF_UI_STATUSBAR_H);
    lv_obj_set_x(cui_statusbar, 48);
    lv_obj_set_y(cui_statusbar, 20);
    lv_obj_set_style_bg_opa(cui_statusbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(cui_statusbar, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(cui_statusbar, LV_OBJ_FLAG_SCROLLABLE);

    /* 空闲默认与 bindings 一致：4G=no-signal，Wi-Fi/BLE/GPS=disabled（裁决 E）。 */
    lv_obj_t * icon_4g = lv_label_create(cui_statusbar);
    lv_label_set_text(icon_4g, "--");
    lv_obj_set_style_text_font(icon_4g, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(icon_4g, 0, 2);

    lv_obj_t * icon_wifi = lv_label_create(cui_statusbar);
    lv_label_set_text(icon_wifi, "-");
    lv_obj_set_style_text_font(icon_wifi, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(icon_wifi, 36, 2);

    lv_obj_t * icon_ble = lv_label_create(cui_statusbar);
    lv_label_set_text(icon_ble, "-");
    lv_obj_set_style_text_font(icon_ble, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(icon_ble, 68, 2);

    lv_obj_t * icon_gps = lv_label_create(cui_statusbar);
    lv_label_set_text(icon_gps, "-");
    lv_obj_set_style_text_font(icon_gps, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(icon_gps, 100, 2);

    lv_obj_t * txt_sync = lv_label_create(cui_statusbar);
    lv_label_set_text(txt_sync, "待同步");
    lv_obj_set_style_text_font(txt_sync, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(txt_sync, 140, 2);

    lv_obj_t * txt_battery = lv_label_create(cui_statusbar);
    lv_label_set_text(txt_battery, "--%");
    lv_obj_set_style_text_font(txt_battery, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(txt_battery, 240, 2);

    lv_obj_t * icon_battery = lv_label_create(cui_statusbar);
    lv_label_set_text(icon_battery, "电");
    lv_obj_set_style_text_font(icon_battery, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(icon_battery, 290, 2);

    kids->kids[UI_COMP_STATUSBAR_STATUSBAR] = cui_statusbar;
    kids->kids[UI_COMP_STATUSBAR_ICON_4G] = icon_4g;
    kids->kids[UI_COMP_STATUSBAR_ICON_WIFI] = icon_wifi;
    kids->kids[UI_COMP_STATUSBAR_ICON_BLE] = icon_ble;
    kids->kids[UI_COMP_STATUSBAR_ICON_GPS] = icon_gps;
    kids->kids[UI_COMP_STATUSBAR_TXT_SYNC] = txt_sync;
    kids->kids[UI_COMP_STATUSBAR_TXT_BATTERY] = txt_battery;
    kids->kids[UI_COMP_STATUSBAR_ICON_BATTERY] = icon_battery;

    lv_obj_add_event_cb(cui_statusbar, statusbar_event_cb, LV_EVENT_ALL, kids);
    ui_comp_statusbar_create_hook(cui_statusbar);
    return cui_statusbar;
}
