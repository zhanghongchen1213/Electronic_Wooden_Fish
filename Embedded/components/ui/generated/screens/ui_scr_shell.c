// SquareLine Studio 1.6.1 / ewf-device — screen_shell
// Story 3.2：MUYU 补 7 槽字带 / scripture-progress / tap-rings（布局空桩；业务在 bindings）
// Story 3.3：JINGWEN 补 scripture-history 滚动容器 / 进度 / round-index（布局空桩；业务在 bindings）
// Story 3.4：TONGJI 补双 stat-card + reading-progress（布局空桩；业务在 bindings；裁决 A/H）
// Story 3.6：MUYU 页内 modal-done（布局空桩；业务在 bindings；禁止顶层 OVERLAY Screen）
#include "../ui.h"
#include "ui_muyu_constants.h"
#include "ui_runtime_events.h"

#include <stdint.h>

lv_obj_t * ui_scr_shell = NULL;
lv_obj_t * ui_shell_pager = NULL;
lv_obj_t * ui_page_muyu_root = NULL;
lv_obj_t * ui_page_jingwen_root = NULL;
lv_obj_t * ui_page_tongji_root = NULL;
lv_obj_t * ui_muyu_statusbar = NULL;
lv_obj_t * ui_jingwen_statusbar = NULL;
lv_obj_t * ui_tongji_statusbar = NULL;
lv_obj_t * ui_muyu_woodfish = NULL;
lv_obj_t * ui_muyu_title = NULL;
lv_obj_t * ui_jingwen_title = NULL;
lv_obj_t * ui_tongji_title = NULL;
lv_obj_t * ui_muyu_glyph_slots[7] = {NULL};
lv_obj_t * ui_muyu_scripture_progress = NULL;
lv_obj_t * ui_muyu_today_hint = NULL;
lv_obj_t * ui_muyu_tap_rings[3] = {NULL};
lv_obj_t * ui_muyu_modal_done = NULL;
lv_obj_t * ui_muyu_modal_card = NULL;
lv_obj_t * ui_muyu_modal_title = NULL;
lv_obj_t * ui_muyu_modal_summary = NULL;
lv_obj_t * ui_muyu_modal_btn_restart = NULL;
lv_obj_t * ui_muyu_modal_btn_exit = NULL;

lv_obj_t * ui_jingwen_history = NULL;
lv_obj_t * ui_jingwen_history_content = NULL;
lv_obj_t * ui_jingwen_scripture_progress = NULL;
lv_obj_t * ui_jingwen_round_index = NULL;
lv_obj_t * ui_jingwen_glyph_current = NULL;
lv_obj_t * ui_jingwen_row_boxes[24] = {NULL};
lv_obj_t * ui_jingwen_row_slots[24][13] = {{NULL}};

lv_obj_t * ui_tongji_divider = NULL;
lv_obj_t * ui_tongji_today_card = NULL;
lv_obj_t * ui_tongji_today_value = NULL;
lv_obj_t * ui_tongji_total_card = NULL;
lv_obj_t * ui_tongji_total_value = NULL;
lv_obj_t * ui_tongji_progress_card = NULL;
lv_obj_t * ui_tongji_progress_arc = NULL;
lv_obj_t * ui_tongji_round_label = NULL;
lv_obj_t * ui_tongji_progress_value = NULL;
lv_obj_t * ui_tongji_progress_percent = NULL;
lv_obj_t * ui_tongji_round_index = NULL;
lv_obj_t * ui_tongji_round_status = NULL;

static lv_obj_t * make_page_root(lv_obj_t * pager, int x, const char * name)
{
    lv_obj_t * page = lv_obj_create(pager);
    lv_obj_set_size(page, EWF_UI_CANVAS_W, EWF_UI_CANVAS_H);
    lv_obj_set_pos(page, x, 0);
    lv_obj_set_style_radius(page, EWF_UI_CORNER_RADIUS, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x0B0B0B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(page, EWF_UI_MARGIN_PX, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    (void)name;
    return page;
}

static lv_obj_t * make_tap_ring(lv_obj_t * parent, int size, int border_w)
{
    lv_obj_t * ring = lv_obj_create(parent);
    lv_obj_set_size(ring, size, size);
    lv_obj_center(ring);
    lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ring, border_w, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ring, lv_color_hex(0xFBFAF0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return ring;
}

static lv_obj_t * make_stat_card(lv_obj_t * parent, int y, const char * label)
{
    lv_obj_t * card = lv_obj_create(parent);
    lv_obj_set_size(card, 378, 64);
    lv_obj_set_pos(card, 16, y);
    lv_obj_set_style_radius(card, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x121212), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * accent = lv_obj_create(card);
    lv_obj_set_size(accent, 4, 40);
    lv_obj_set_pos(accent, 18, 12);
    lv_obj_set_style_radius(accent, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(accent, lv_color_hex(0xd9a441), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(accent, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(accent, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t * lab = lv_label_create(card);
    lv_label_set_text(lab, label);
    lv_obj_set_style_text_font(lab, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(lab, lv_color_hex(0xa6a29a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(lab, 38, 7);

    lv_obj_t * unit = lv_label_create(card);
    lv_label_set_text(unit, "次");
    lv_obj_set_style_text_font(unit, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(unit, lv_color_hex(0xcfc4b0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(unit, 300, 28);

    return card;
}

void ui_scr_shell_screen_init(void)
{
    ui_scr_shell = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_scr_shell, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_scr_shell, EWF_UI_CANVAS_W, EWF_UI_CANVAS_H);
    lv_obj_set_style_radius(ui_scr_shell, EWF_UI_CORNER_RADIUS, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_scr_shell, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_scr_shell, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(ui_scr_shell, true, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_shell_pager = lv_obj_create(ui_scr_shell);
    lv_obj_set_size(ui_shell_pager, EWF_UI_PAGER_CONTENT_W, EWF_UI_CANVAS_H);
    lv_obj_set_pos(ui_shell_pager, 0, 0);
    lv_obj_set_style_bg_opa(ui_shell_pager, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_shell_pager, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_shell_pager, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_shell_pager, LV_OBJ_FLAG_SCROLLABLE);

    ui_page_muyu_root = make_page_root(ui_shell_pager, EWF_UI_PAGE_MUYU_X, "MUYU");
    ui_page_jingwen_root = make_page_root(ui_shell_pager, EWF_UI_PAGE_JINGWEN_X, "JINGWEN");
    ui_page_tongji_root = make_page_root(ui_shell_pager, EWF_UI_PAGE_TONGJI_X, "TONGJI");

    ui_muyu_statusbar = ui_statusbar_create(ui_page_muyu_root);
    ui_jingwen_statusbar = ui_statusbar_create(ui_page_jingwen_root);
    ui_tongji_statusbar = ui_statusbar_create(ui_page_tongji_root);

    ui_muyu_title = lv_label_create(ui_page_muyu_root);
    lv_label_set_text(ui_muyu_title, "木鱼");
    lv_obj_set_style_text_font(ui_muyu_title, &ui_font_serif700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_title, 16, 56);

    /* 七字带：内容区宽=画布-2×边距；槽宽收敛使 belt_w≤内容宽，槽间距净空 ≥8。 */
    const int belt_y = 100;
    const int slot_w = 47;
    const int slot_gap = 8;
    const int content_w = EWF_UI_CANVAS_W - 2 * EWF_UI_MARGIN_PX;
    const int belt_w = 7 * slot_w + 6 * slot_gap;
    const int belt_x = (content_w - belt_w) / 2;
    for(int i = 0; i < 7; ++i) {
        ui_muyu_glyph_slots[i] = lv_label_create(ui_page_muyu_root);
        lv_label_set_text(ui_muyu_glyph_slots[i], "·");
        lv_obj_set_style_text_font(ui_muyu_glyph_slots[i], &ui_font_serif400_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(ui_muyu_glyph_slots[i], lv_color_hex(0x6B6B6B), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_width(ui_muyu_glyph_slots[i], slot_w);
        lv_obj_set_style_text_align(ui_muyu_glyph_slots[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_pos(ui_muyu_glyph_slots[i], belt_x + i * (slot_w + slot_gap), belt_y);
    }

    ui_muyu_scripture_progress = lv_label_create(ui_page_muyu_root);
    lv_label_set_text(ui_muyu_scripture_progress, "心经进度 0 / 260 · 0%");
    lv_obj_set_style_text_font(ui_muyu_scripture_progress, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_scripture_progress, 16, 156);

    ui_muyu_today_hint = lv_label_create(ui_page_muyu_root);
    lv_label_set_text(ui_muyu_today_hint, "待校时");
    lv_obj_set_style_text_font(ui_muyu_today_hint, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_today_hint, 16, 178);

    ui_muyu_woodfish = ui_woodfish_mark_create(ui_page_muyu_root);
    lv_obj_set_pos(ui_muyu_woodfish, 95, 200);

    /* tap-rings：挂在 woodfish 容器内，idle 白半透明描边。 */
    ui_muyu_tap_rings[0] = make_tap_ring(ui_muyu_woodfish, 200, 2);
    lv_obj_set_style_border_opa(ui_muyu_tap_rings[0], 0x66, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_muyu_tap_rings[1] = make_tap_ring(ui_muyu_woodfish, 170, 2);
    lv_obj_set_style_border_opa(ui_muyu_tap_rings[1], 0x44, LV_PART_MAIN | LV_STATE_DEFAULT);
    ui_muyu_tap_rings[2] = make_tap_ring(ui_muyu_woodfish, 140, 2);
    lv_obj_set_style_border_opa(ui_muyu_tap_rings[2], 0x33, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* modal-done：帧内浮层；默认隐藏；禁止另建顶层 Screen。 */
    ui_muyu_modal_done = lv_obj_create(ui_page_muyu_root);
    lv_obj_set_size(ui_muyu_modal_done, EWF_UI_MUYU_MODAL_OVERLAY_W, EWF_UI_MUYU_MODAL_OVERLAY_H);
    lv_obj_set_pos(ui_muyu_modal_done, 0, 0);
    lv_obj_set_style_bg_color(ui_muyu_modal_done, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_muyu_modal_done, EWF_UI_MUYU_MODAL_OVERLAY_OPA, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_muyu_modal_done, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_muyu_modal_done, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_muyu_modal_done, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_muyu_modal_done, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(ui_muyu_modal_done, LV_OBJ_FLAG_HIDDEN);

    ui_muyu_modal_card = lv_obj_create(ui_muyu_modal_done);
    lv_obj_set_size(ui_muyu_modal_card, EWF_UI_MUYU_MODAL_CARD_W, EWF_UI_MUYU_MODAL_CARD_H);
    lv_obj_set_pos(ui_muyu_modal_card, EWF_UI_MUYU_MODAL_CARD_X, EWF_UI_MUYU_MODAL_CARD_Y);
    lv_obj_set_style_radius(ui_muyu_modal_card, EWF_UI_MUYU_MODAL_CARD_RADIUS, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_muyu_modal_card, lv_color_hex(EWF_UI_MUYU_MODAL_CARD_BG_HEX), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_muyu_modal_card, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_muyu_modal_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_muyu_modal_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_muyu_modal_card, LV_OBJ_FLAG_SCROLLABLE);

    ui_muyu_modal_title = lv_label_create(ui_muyu_modal_card);
    lv_label_set_text(ui_muyu_modal_title, "本轮完成");
    lv_obj_set_style_text_font(ui_muyu_modal_title, &ui_font_serif700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_muyu_modal_title, lv_color_hex(EWF_UI_MUYU_MODAL_TITLE_HEX), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_modal_title, 24, 24);

    ui_muyu_modal_summary = lv_label_create(ui_muyu_modal_card);
    lv_label_set_text(ui_muyu_modal_summary, "心经进度 260 / 260 字");
    lv_obj_set_style_text_font(ui_muyu_modal_summary, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_muyu_modal_summary, lv_color_hex(EWF_UI_MUYU_MODAL_MUTED_HEX), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_modal_summary, 24, 72);

    ui_muyu_modal_btn_restart = lv_label_create(ui_muyu_modal_card);
    lv_label_set_text(ui_muyu_modal_btn_restart, "从头开始");
    lv_obj_set_style_text_font(ui_muyu_modal_btn_restart, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_muyu_modal_btn_restart, lv_color_hex(EWF_UI_MUYU_MODAL_RESTART_HEX), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_modal_btn_restart, 24, 128);
    lv_obj_add_flag(ui_muyu_modal_btn_restart, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_muyu_modal_btn_restart, ui_runtime_events_on_generated, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)EWF_UI_MUYU_DONE_EVT_RESTART);

    ui_muyu_modal_btn_exit = lv_label_create(ui_muyu_modal_card);
    lv_label_set_text(ui_muyu_modal_btn_exit, "退出");
    lv_obj_set_style_text_font(ui_muyu_modal_btn_exit, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_muyu_modal_btn_exit, lv_color_hex(EWF_UI_MUYU_MODAL_MUTED_HEX), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_muyu_modal_btn_exit, 180, 128);
    lv_obj_add_flag(ui_muyu_modal_btn_exit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(ui_muyu_modal_btn_exit, ui_runtime_events_on_generated, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)EWF_UI_MUYU_DONE_EVT_EXIT);

    ui_jingwen_title = lv_label_create(ui_page_jingwen_root);
    lv_label_set_text(ui_jingwen_title, "经文");
    lv_obj_set_style_text_font(ui_jingwen_title, &ui_font_serif700_28, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_jingwen_title, 16, 56);

    /* scripture-history：362×286 @ (24,102)；面色 #121212、圆角 18；禁止挂 woodfish/tap-rings。 */
    ui_jingwen_history = lv_obj_create(ui_page_jingwen_root);
    lv_obj_set_size(ui_jingwen_history, 362, 286);
    lv_obj_set_pos(ui_jingwen_history, 24, 102);
    lv_obj_set_style_radius(ui_jingwen_history, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_jingwen_history, lv_color_hex(0x121212), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_jingwen_history, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_jingwen_history, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_dir(ui_jingwen_history, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_jingwen_history, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui_jingwen_history, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_jingwen_history, LV_OBJ_FLAG_CLICK_FOCUSABLE);

    ui_jingwen_history_content = lv_obj_create(ui_jingwen_history);
    lv_obj_set_size(ui_jingwen_history_content, 346, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(ui_jingwen_history_content, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_jingwen_history_content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_jingwen_history_content, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_flex_flow(ui_jingwen_history_content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ui_jingwen_history_content, 8, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_jingwen_history_content, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ui_jingwen_scripture_progress = lv_label_create(ui_page_jingwen_root);
    lv_label_set_text(ui_jingwen_scripture_progress, "心经进度 0 / 260 · 0%");
    lv_obj_set_style_text_font(ui_jingwen_scripture_progress, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_jingwen_scripture_progress, 16, 410);

    ui_jingwen_round_index = lv_label_create(ui_page_jingwen_root);
    lv_label_set_text(ui_jingwen_round_index, "");
    lv_obj_set_style_text_font(ui_jingwen_round_index, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_jingwen_round_index, 16, 432);

    ui_tongji_title = lv_label_create(ui_page_tongji_root);
    lv_label_set_text(ui_tongji_title, "统计");
    lv_obj_set_style_text_font(ui_tongji_title, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_title, 16, 56);

    /* TONGJI：分割线 + 今日/累计卡 + reading-progress；禁止挂木鱼触区与七日/三十日控件。 */
    ui_tongji_divider = lv_obj_create(ui_page_tongji_root);
    lv_obj_set_size(ui_tongji_divider, 378, 2);
    lv_obj_set_pos(ui_tongji_divider, 16, 99);
    lv_obj_set_style_bg_color(ui_tongji_divider, lv_color_hex(0x30343B), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_tongji_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(ui_tongji_divider, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_tongji_divider, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ui_tongji_today_card = make_stat_card(ui_page_tongji_root, 112, "今日敲击");
    ui_tongji_today_value = lv_label_create(ui_tongji_today_card);
    lv_label_set_text(ui_tongji_today_value, "待校时");
    lv_obj_set_style_text_font(ui_tongji_today_value, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_today_value, lv_color_hex(0xa6a29a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_today_value, 38, 24);

    ui_tongji_total_card = make_stat_card(ui_page_tongji_root, 184, "累计敲击");
    ui_tongji_total_value = lv_label_create(ui_tongji_total_card);
    lv_label_set_text(ui_tongji_total_value, "0");
    lv_obj_set_style_text_font(ui_tongji_total_value, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_total_value, lv_color_hex(0xfbfaf0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_total_value, 38, 24);

    ui_tongji_progress_card = lv_obj_create(ui_page_tongji_root);
    lv_obj_set_size(ui_tongji_progress_card, 378, 176);
    lv_obj_set_pos(ui_tongji_progress_card, 16, 264);
    lv_obj_set_style_radius(ui_tongji_progress_card, 18, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_tongji_progress_card, lv_color_hex(0x121212), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_tongji_progress_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(ui_tongji_progress_card, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(ui_tongji_progress_card, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ui_tongji_progress_arc = lv_arc_create(ui_tongji_progress_card);
    lv_obj_set_size(ui_tongji_progress_arc, 120, 120);
    lv_obj_set_pos(ui_tongji_progress_arc, 28, 28);
    lv_arc_set_rotation(ui_tongji_progress_arc, 270);
    lv_arc_set_bg_angles(ui_tongji_progress_arc, 0, 360);
    lv_arc_set_angles(ui_tongji_progress_arc, 0, 360);
    lv_arc_set_range(ui_tongji_progress_arc, 0, 100);
    lv_arc_set_value(ui_tongji_progress_arc, 0);
    lv_obj_remove_style(ui_tongji_progress_arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(ui_tongji_progress_arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_color(ui_tongji_progress_arc, lv_color_hex(0x252525), LV_PART_MAIN);
    lv_obj_set_style_arc_width(ui_tongji_progress_arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui_tongji_progress_arc, lv_color_hex(0xd9a441), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui_tongji_progress_arc, 8, LV_PART_INDICATOR);

    ui_tongji_round_label = lv_label_create(ui_tongji_progress_card);
    lv_label_set_text(ui_tongji_round_label, "本次诵读");
    lv_obj_set_style_text_font(ui_tongji_round_label, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_round_label, lv_color_hex(0xa6a29a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_round_label, 180, 40);

    ui_tongji_progress_value = lv_label_create(ui_tongji_progress_card);
    lv_label_set_text(ui_tongji_progress_value, "0 / 260 字");
    lv_obj_set_style_text_font(ui_tongji_progress_value, &ui_font_ns700_22, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_progress_value, lv_color_hex(0xfbfaf0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_progress_value, 180, 62);

    ui_tongji_progress_percent = lv_label_create(ui_tongji_progress_card);
    lv_label_set_text(ui_tongji_progress_percent, "已完成 0%");
    lv_obj_set_style_text_font(ui_tongji_progress_percent, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_progress_percent, lv_color_hex(0xcfc4b0), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_progress_percent, 180, 98);

    ui_tongji_round_index = lv_label_create(ui_tongji_progress_card);
    lv_label_set_text(ui_tongji_round_index, "");
    lv_obj_set_style_text_font(ui_tongji_round_index, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_round_index, lv_color_hex(0xa6a29a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_round_index, 180, 124);

    ui_tongji_round_status = lv_label_create(ui_tongji_progress_card);
    lv_label_set_text(ui_tongji_round_status, "");
    lv_obj_set_style_text_font(ui_tongji_round_status, &ui_font_ns600_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_tongji_round_status, lv_color_hex(0xa6a29a), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(ui_tongji_round_status, 180, 146);
}

void ui_scr_shell_screen_destroy(void)
{
    if(ui_scr_shell) lv_obj_del(ui_scr_shell);
    ui_scr_shell = NULL;
    ui_shell_pager = NULL;
    ui_page_muyu_root = NULL;
    ui_page_jingwen_root = NULL;
    ui_page_tongji_root = NULL;
    ui_muyu_statusbar = NULL;
    ui_jingwen_statusbar = NULL;
    ui_tongji_statusbar = NULL;
    ui_muyu_woodfish = NULL;
    ui_muyu_title = NULL;
    ui_jingwen_title = NULL;
    ui_tongji_title = NULL;
    ui_muyu_scripture_progress = NULL;
    ui_muyu_today_hint = NULL;
    for(int i = 0; i < 7; ++i) ui_muyu_glyph_slots[i] = NULL;
    for(int i = 0; i < 3; ++i) ui_muyu_tap_rings[i] = NULL;
    ui_muyu_modal_done = NULL;
    ui_muyu_modal_card = NULL;
    ui_muyu_modal_title = NULL;
    ui_muyu_modal_summary = NULL;
    ui_muyu_modal_btn_restart = NULL;
    ui_muyu_modal_btn_exit = NULL;
    ui_jingwen_history = NULL;
    ui_jingwen_history_content = NULL;
    ui_jingwen_scripture_progress = NULL;
    ui_jingwen_round_index = NULL;
    ui_jingwen_glyph_current = NULL;
    for(int r = 0; r < 24; ++r) {
        ui_jingwen_row_boxes[r] = NULL;
        for(int c = 0; c < 13; ++c) ui_jingwen_row_slots[r][c] = NULL;
    }
    ui_tongji_divider = NULL;
    ui_tongji_today_card = NULL;
    ui_tongji_today_value = NULL;
    ui_tongji_total_card = NULL;
    ui_tongji_total_value = NULL;
    ui_tongji_progress_card = NULL;
    ui_tongji_progress_arc = NULL;
    ui_tongji_round_label = NULL;
    ui_tongji_progress_value = NULL;
    ui_tongji_progress_percent = NULL;
    ui_tongji_round_index = NULL;
    ui_tongji_round_status = NULL;
}
