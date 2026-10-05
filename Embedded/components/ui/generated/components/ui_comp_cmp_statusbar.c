// SquareLine Studio 1.6.1 / ewf-device — cmp_statusbar
// UI 对齐批次：4 信号 lucide 位图（18×18，A8 白模板，运行时 recolor）+ sync 圆点
// + 同步文案 + 电量数值 + 电池位图（HTML statusbar master 几何）。
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

/* A8 白模板图标：recolor 上色；禁用/无信号态由 bindings 换 src + recolor。 */
static lv_obj_t * make_signal_icon(lv_obj_t * parent, int x, const lv_img_dsc_t * src,
                                   uint32_t recolor_hex)
{
    lv_obj_t * img = lv_img_create(parent);
    lv_img_set_src(img, src);
    lv_obj_set_pos(img, x, 3);
    lv_obj_set_style_img_recolor(img, lv_color_hex(recolor_hex), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_img_recolor_opa(img, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
    return img;
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
    lv_obj_t * icon_4g = make_signal_icon(cui_statusbar, 8, &ui_img_ic_signal_zero, 0x81786a);
    lv_obj_t * icon_wifi = make_signal_icon(cui_statusbar, 36, &ui_img_ic_wifi_off, 0x5f564b);
    lv_obj_t * icon_ble = make_signal_icon(cui_statusbar, 64, &ui_img_ic_bluetooth_off, 0x5f564b);
    lv_obj_t * icon_gps = make_signal_icon(cui_statusbar, 92, &ui_img_ic_navigation_off, 0x5f564b);

    lv_obj_t * dot_sync = lv_obj_create(cui_statusbar);
    lv_obj_set_size(dot_sync, 7, 7);
    lv_obj_set_pos(dot_sync, 140, 8);
    lv_obj_set_style_radius(dot_sync, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(dot_sync, lv_color_hex(0xb8c7a4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(dot_sync, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(dot_sync, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * txt_sync = lv_label_create(cui_statusbar);
    lv_label_set_text(txt_sync, "待同步");
    lv_obj_set_style_text_font(txt_sync, &ui_font_ns500_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(txt_sync, lv_color_hex(0xcfc4b0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(txt_sync, 153, 1);

    lv_obj_t * txt_battery = lv_label_create(cui_statusbar);
    lv_label_set_text(txt_battery, "--%");
    lv_obj_set_width(txt_battery, 36);
    lv_obj_set_height(txt_battery, 22);
    lv_obj_set_style_text_font(txt_battery, &ui_font_ns500_13, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(txt_battery, lv_color_hex(0xf5efe2), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(txt_battery, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(txt_battery, 246, 1);

    lv_obj_t * icon_battery = make_signal_icon(cui_statusbar, 290, &ui_img_ic_battery_medium, 0xc9a66a);

    kids->kids[UI_COMP_STATUSBAR_STATUSBAR] = cui_statusbar;
    kids->kids[UI_COMP_STATUSBAR_ICON_4G] = icon_4g;
    kids->kids[UI_COMP_STATUSBAR_ICON_WIFI] = icon_wifi;
    kids->kids[UI_COMP_STATUSBAR_ICON_BLE] = icon_ble;
    kids->kids[UI_COMP_STATUSBAR_ICON_GPS] = icon_gps;
    kids->kids[UI_COMP_STATUSBAR_DOT_SYNC] = dot_sync;
    kids->kids[UI_COMP_STATUSBAR_TXT_SYNC] = txt_sync;
    kids->kids[UI_COMP_STATUSBAR_TXT_BATTERY] = txt_battery;
    kids->kids[UI_COMP_STATUSBAR_ICON_BATTERY] = icon_battery;

    lv_obj_add_event_cb(cui_statusbar, statusbar_event_cb, LV_EVENT_ALL, kids);
    ui_comp_statusbar_create_hook(cui_statusbar);
    return cui_statusbar;
}
