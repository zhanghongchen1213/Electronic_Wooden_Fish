// SquareLine Studio 1.6.1 / ewf-device — SHEZHI on-demand settings screen
// generated 仅布局/空桩；选中态与 sync action 由 bindings 投影。
#include "../ui.h"
#include "ui_runtime_events.h"

#include <stdint.h>

lv_obj_t *ui_scr_shezhi = NULL;
lv_obj_t *ui_shezhi_title = NULL;
lv_obj_t *ui_shezhi_divider = NULL;
lv_obj_t *ui_shezhi_viewport = NULL;
lv_obj_t *ui_shezhi_content = NULL;

lv_obj_t *ui_shezhi_row_brightness = NULL;
lv_obj_t *ui_shezhi_brightness_label = NULL;
lv_obj_t *ui_shezhi_brightness_caps[3] = {NULL, NULL, NULL};
lv_obj_t *ui_shezhi_brightness_cap_labels[3] = {NULL, NULL, NULL};

lv_obj_t *ui_shezhi_row_timeout = NULL;
lv_obj_t *ui_shezhi_timeout_label = NULL;
lv_obj_t *ui_shezhi_timeout_caps[3] = {NULL, NULL, NULL};
lv_obj_t *ui_shezhi_timeout_cap_labels[3] = {NULL, NULL, NULL};

lv_obj_t *ui_shezhi_row_volume = NULL;
lv_obj_t *ui_shezhi_volume_label = NULL;
lv_obj_t *ui_shezhi_volume_slider = NULL;
lv_obj_t *ui_shezhi_volume_value = NULL;

lv_obj_t *ui_shezhi_row_sync = NULL;
lv_obj_t *ui_shezhi_sync_title = NULL;
lv_obj_t *ui_shezhi_sync_action = NULL;
lv_obj_t *ui_shezhi_sync_action_label = NULL;

lv_obj_t *ui_shezhi_row_version = NULL;
lv_obj_t *ui_shezhi_version_label = NULL;
lv_obj_t *ui_shezhi_version_value = NULL;
lv_obj_t *ui_shezhi_row_device_id = NULL;
lv_obj_t *ui_shezhi_device_id_label = NULL;
lv_obj_t *ui_shezhi_device_id_value = NULL;

static lv_obj_t *make_row(lv_obj_t *parent, lv_coord_t y)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(row, 378, 80);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_style_radius(row, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(row, lv_color_hex(0x16181C),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return row;
}

static lv_obj_t *make_icon_block(lv_obj_t *parent)
{
    /* hardware_pending：lucide SVG 像素级对拍；此处简化色块。 */
    lv_obj_t *icon = lv_obj_create(parent);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(icon, 30, 30);
    lv_obj_set_pos(icon, 20, 25);
    lv_obj_set_style_radius(icon, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(icon, lv_color_hex(0xE6BD69),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(icon, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    return icon;
}

static void make_segment_caps(lv_obj_t *parent,
                              lv_obj_t **caps,
                              lv_obj_t **labels,
                              const char *const texts[3],
                              uintptr_t base_evt)
{
    lv_obj_t *seg = lv_obj_create(parent);
    lv_obj_clear_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(seg, 164, 48);
    lv_obj_set_pos(seg, 198, 16);
    lv_obj_set_style_radius(seg, 24, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(seg, lv_color_hex(0x262A31),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(seg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(seg, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    static const lv_coord_t xs[3] = {2, 56, 110};
    for (int i = 0; i < 3; ++i) {
        caps[i] = lv_obj_create(seg);
        lv_obj_clear_flag(caps[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(caps[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(caps[i], 52, 44);
        lv_obj_set_pos(caps[i], xs[i], 2);
        lv_obj_set_style_radius(caps[i], 22, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(caps[i], lv_color_hex(0x262A31),
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(caps[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(caps[i], 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_event_cb(caps[i], ui_runtime_events_on_generated,
                            LV_EVENT_CLICKED, (void *)(base_evt + (uintptr_t)i));

        labels[i] = lv_label_create(caps[i]);
        lv_label_set_text(labels[i], texts[i]);
        lv_obj_set_style_text_font(labels[i], &ui_font_ns600_16,
                                   LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(labels[i], lv_color_hex(0xA6A29A),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_center(labels[i]);
    }
}

void ui_scr_shezhi_screen_init(void)
{
    ui_scr_shezhi = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_scr_shezhi, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_scr_shezhi, EWF_UI_CANVAS_W, EWF_UI_CANVAS_H);
    lv_obj_set_style_radius(ui_scr_shezhi, EWF_UI_CORNER_RADIUS,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scr_shezhi, lv_color_hex(0x050505),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_scr_shezhi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_scr_shezhi, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_shezhi_title = lv_label_create(ui_scr_shezhi);
    lv_label_set_text(ui_shezhi_title, "设置");
    lv_obj_set_style_text_font(ui_shezhi_title, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_title, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_title, 16, 16);
    lv_obj_set_width(ui_shezhi_title, 378);
    lv_obj_set_style_text_align(ui_shezhi_title, LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_shezhi_divider = lv_obj_create(ui_scr_shezhi);
    lv_obj_clear_flag(ui_shezhi_divider, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_shezhi_divider, 378, 2);
    lv_obj_set_pos(ui_shezhi_divider, 16, 66);
    lv_obj_set_style_bg_color(ui_shezhi_divider, lv_color_hex(0x30343B),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_shezhi_divider, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_shezhi_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_shezhi_viewport = lv_obj_create(ui_scr_shezhi);
    lv_obj_set_size(ui_shezhi_viewport, 378, 344);
    lv_obj_set_pos(ui_shezhi_viewport, 16, 72);
    lv_obj_set_style_bg_opa(ui_shezhi_viewport, LV_OPA_TRANSP,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_shezhi_viewport, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_shezhi_viewport, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_dir(ui_shezhi_viewport, LV_DIR_VER);
    lv_obj_add_flag(ui_shezhi_viewport, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_shezhi_viewport, LV_OBJ_FLAG_SCROLL_ELASTIC);

    ui_shezhi_content = lv_obj_create(ui_shezhi_viewport);
    lv_obj_clear_flag(ui_shezhi_content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_shezhi_content, 378, 520);
    lv_obj_set_pos(ui_shezhi_content, 0, 0);
    lv_obj_set_style_bg_opa(ui_shezhi_content, LV_OPA_TRANSP,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_shezhi_content, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_shezhi_content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* page-1：亮度 / 熄屏 / 音量 / 立即同步（恰好 4 行）。 */
    ui_shezhi_row_brightness = make_row(ui_shezhi_content, 0);
    (void)make_icon_block(ui_shezhi_row_brightness);
    ui_shezhi_brightness_label = lv_label_create(ui_shezhi_row_brightness);
    lv_label_set_text(ui_shezhi_brightness_label, "屏幕亮度");
    lv_obj_set_style_text_font(ui_shezhi_brightness_label, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_brightness_label, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_brightness_label, 66, 12);
    {
        static const char *const texts[3] = {"低", "中", "高"};
        make_segment_caps(ui_shezhi_row_brightness, ui_shezhi_brightness_caps,
                          ui_shezhi_brightness_cap_labels, texts,
                          (uintptr_t)EWF_UI_SHEZHI_EVT_BRIGHTNESS_LOW);
    }

    ui_shezhi_row_timeout = make_row(ui_shezhi_content, 88);
    (void)make_icon_block(ui_shezhi_row_timeout);
    ui_shezhi_timeout_label = lv_label_create(ui_shezhi_row_timeout);
    lv_label_set_text(ui_shezhi_timeout_label, "自动熄屏");
    lv_obj_set_style_text_font(ui_shezhi_timeout_label, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_timeout_label, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_timeout_label, 66, 12);
    {
        static const char *const texts[3] = {"5秒", "15秒", "30秒"};
        make_segment_caps(ui_shezhi_row_timeout, ui_shezhi_timeout_caps,
                          ui_shezhi_timeout_cap_labels, texts,
                          (uintptr_t)EWF_UI_SHEZHI_EVT_TIMEOUT_5);
    }

    ui_shezhi_row_volume = make_row(ui_shezhi_content, 176);
    (void)make_icon_block(ui_shezhi_row_volume);
    ui_shezhi_volume_label = lv_label_create(ui_shezhi_row_volume);
    lv_label_set_text(ui_shezhi_volume_label, "音量");
    lv_obj_set_style_text_font(ui_shezhi_volume_label, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_volume_label, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_volume_label, 66, 12);

    ui_shezhi_volume_slider = lv_slider_create(ui_shezhi_row_volume);
    lv_obj_set_size(ui_shezhi_volume_slider, 112, 18);
    lv_obj_set_pos(ui_shezhi_volume_slider, 198, 31);
    lv_slider_set_range(ui_shezhi_volume_slider, 0, 100);
    lv_slider_set_value(ui_shezhi_volume_slider, 50, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui_shezhi_volume_slider, lv_color_hex(0x252525),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_shezhi_volume_slider, lv_color_hex(0xD9A441),
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_shezhi_volume_slider, lv_color_hex(0xD9A441),
                              LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_shezhi_volume_slider, 4,
                             LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_shezhi_volume_slider, ui_runtime_events_on_generated,
                        LV_EVENT_VALUE_CHANGED,
                        (void *)(uintptr_t)EWF_UI_SHEZHI_EVT_VOLUME_CHANGED);

    ui_shezhi_volume_value = lv_label_create(ui_shezhi_row_volume);
    lv_label_set_text(ui_shezhi_volume_value, "50");
    lv_obj_set_style_text_font(ui_shezhi_volume_value, &ui_font_ns600_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_volume_value, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_volume_value, 318, 20);

    ui_shezhi_row_sync = make_row(ui_shezhi_content, 264);
    (void)make_icon_block(ui_shezhi_row_sync);
    ui_shezhi_sync_title = lv_label_create(ui_shezhi_row_sync);
    lv_label_set_text(ui_shezhi_sync_title, "立即同步");
    lv_obj_set_style_text_font(ui_shezhi_sync_title, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_sync_title, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_sync_title, 66, 12);

    ui_shezhi_sync_action = lv_obj_create(ui_shezhi_row_sync);
    lv_obj_clear_flag(ui_shezhi_sync_action, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_shezhi_sync_action, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(ui_shezhi_sync_action, 146, 48);
    lv_obj_set_pos(ui_shezhi_sync_action, 216, 16);
    lv_obj_set_style_radius(ui_shezhi_sync_action, 24,
                            LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_shezhi_sync_action, lv_color_hex(0x262A31),
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_shezhi_sync_action, 0,
                                  LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_event_cb(ui_shezhi_sync_action, ui_runtime_events_on_generated,
                        LV_EVENT_CLICKED,
                        (void *)(uintptr_t)EWF_UI_SHEZHI_EVT_SYNC_CLICK);

    ui_shezhi_sync_action_label = lv_label_create(ui_shezhi_sync_action);
    lv_label_set_text(ui_shezhi_sync_action_label, "立即同步");
    lv_obj_set_style_text_font(ui_shezhi_sync_action_label, &ui_font_ns600_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_sync_action_label,
                                lv_color_hex(0xE6BD69),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_center(ui_shezhi_sync_action_label);

    /* page-2 @ top=344：木鱼版本 / 木鱼ID。 */
    ui_shezhi_row_version = make_row(ui_shezhi_content, 344);
    (void)make_icon_block(ui_shezhi_row_version);
    ui_shezhi_version_label = lv_label_create(ui_shezhi_row_version);
    lv_label_set_text(ui_shezhi_version_label, "木鱼版本");
    lv_obj_set_style_text_font(ui_shezhi_version_label, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_version_label, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_version_label, 66, 12);
    ui_shezhi_version_value = lv_label_create(ui_shezhi_row_version);
    lv_label_set_text(ui_shezhi_version_value, "-");
    lv_obj_set_style_text_font(ui_shezhi_version_value, &ui_font_ns600_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_version_value, lv_color_hex(0xA6A29A),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_version_value, 242, 12);
    lv_obj_set_width(ui_shezhi_version_value, 120);
    lv_obj_set_style_text_align(ui_shezhi_version_value, LV_TEXT_ALIGN_RIGHT,
                                LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_shezhi_row_device_id = make_row(ui_shezhi_content, 432);
    (void)make_icon_block(ui_shezhi_row_device_id);
    ui_shezhi_device_id_label = lv_label_create(ui_shezhi_row_device_id);
    lv_label_set_text(ui_shezhi_device_id_label, "木鱼ID");
    lv_obj_set_style_text_font(ui_shezhi_device_id_label, &ui_font_ns700_22,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_device_id_label, lv_color_hex(0xFBFAF0),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_device_id_label, 66, 12);
    ui_shezhi_device_id_value = lv_label_create(ui_shezhi_row_device_id);
    lv_label_set_text(ui_shezhi_device_id_value, "-");
    lv_obj_set_style_text_font(ui_shezhi_device_id_value, &ui_font_ns600_16,
                               LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_shezhi_device_id_value, lv_color_hex(0xA6A29A),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_shezhi_device_id_value, 242, 12);
    lv_obj_set_width(ui_shezhi_device_id_value, 120);
    lv_obj_set_style_text_align(ui_shezhi_device_id_value, LV_TEXT_ALIGN_RIGHT,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

void ui_scr_shezhi_screen_destroy(void)
{
    if (ui_scr_shezhi) {
        lv_obj_del(ui_scr_shezhi);
    }
    ui_scr_shezhi = NULL;
    ui_shezhi_title = NULL;
    ui_shezhi_divider = NULL;
    ui_shezhi_viewport = NULL;
    ui_shezhi_content = NULL;
    ui_shezhi_row_brightness = NULL;
    ui_shezhi_brightness_label = NULL;
    ui_shezhi_row_timeout = NULL;
    ui_shezhi_timeout_label = NULL;
    ui_shezhi_row_volume = NULL;
    ui_shezhi_volume_label = NULL;
    ui_shezhi_volume_slider = NULL;
    ui_shezhi_volume_value = NULL;
    ui_shezhi_row_sync = NULL;
    ui_shezhi_sync_title = NULL;
    ui_shezhi_sync_action = NULL;
    ui_shezhi_sync_action_label = NULL;
    ui_shezhi_row_version = NULL;
    ui_shezhi_version_label = NULL;
    ui_shezhi_version_value = NULL;
    ui_shezhi_row_device_id = NULL;
    ui_shezhi_device_id_label = NULL;
    ui_shezhi_device_id_value = NULL;
    for (int i = 0; i < 3; ++i) {
        ui_shezhi_brightness_caps[i] = NULL;
        ui_shezhi_brightness_cap_labels[i] = NULL;
        ui_shezhi_timeout_caps[i] = NULL;
        ui_shezhi_timeout_cap_labels[i] = NULL;
    }
}
