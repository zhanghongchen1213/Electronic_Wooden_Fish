/**
 * @file     ui_muyu_projection.c
 * @brief    木鱼页字带/进度/三环/完成遮罩 LVGL 投影实现。
 * @details  所有 lv_* 与 timer 仅在 ui_task 上下文调用。
 *           今日区与 TONGJI 共用 ewf_ui_today_value_format（裁决 G）。
 *           完成遮罩与 gate.completed 对齐（裁决 A）；投影失败只打日志不阻塞落盘。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_muyu_projection.h"

#include "ui_muyu_belt_policy.h"
#include "ui_muyu_constants.h"
#include "ui_muyu_done_policy.h"
#include "ui_tongji_stats_policy.h"
#include "ui.h"

#include <string.h>

static bool s_ready;
static uint32_t s_prev_cursor;
static bool s_cursor_primed;
static ewf_ui_tap_rings_state_t s_rings;
static lv_timer_t *s_flash_timer;
static ui_muyu_woodfish_click_fn s_on_click;
static ewf_ui_muyu_done_view_t s_prev_done;
static bool s_done_primed;

static void apply_ring_idle(void);
static void apply_ring_flash(void);
static void flash_timer_cb(lv_timer_t *timer);
static void woodfish_clicked_cb(lv_event_t *e);
static void style_glyph_slot(lv_obj_t *label, bool filled, bool is_current);
static ewf_ui_tap_rings_gate_t gate_from_snapshot(const watch_state_snapshot_t *snapshot);
static void restart_flash_timer(void);
static void apply_done_overlay(const watch_state_snapshot_t *snapshot);

esp_err_t ui_muyu_projection_init(ui_muyu_woodfish_click_fn on_woodfish_click)
{
    s_ready = true;
    s_prev_cursor = 0U;
    s_cursor_primed = false;
    memset(&s_rings, 0, sizeof(s_rings));
    memset(&s_prev_done, 0, sizeof(s_prev_done));
    s_done_primed = false;
    s_flash_timer = NULL;
    s_on_click = on_woodfish_click;
    apply_ring_idle();

    if (ui_muyu_woodfish != NULL) {
        lv_obj_add_flag(ui_muyu_woodfish, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(ui_muyu_woodfish, woodfish_clicked_cb, LV_EVENT_CLICKED, NULL);
    }
    if (ui_muyu_modal_done != NULL) {
        lv_obj_add_flag(ui_muyu_modal_done, LV_OBJ_FLAG_HIDDEN);
    }
    return ESP_OK;
}

static void woodfish_clicked_cb(lv_event_t *e)
{
    (void)e;
    if (s_on_click != NULL) {
        s_on_click();
    }
}

static ewf_ui_tap_rings_gate_t gate_from_snapshot(const watch_state_snapshot_t *snapshot)
{
    ewf_ui_tap_rings_gate_t gate = {
        .pending_completion = false,
        .fault_locked = false,
        .screen_on = true,
        .active_page_muyu = true,
    };
    if (snapshot == NULL) {
        return gate;
    }
    /* 完成锁定：优先用 gate.completed（pending||completed），与 rings/遮罩一致。 */
    gate.pending_completion = snapshot->tap_completed ||
                              snapshot->tap_pending_completion ||
                              (snapshot->tap_round_state == 1U);
    gate.fault_locked = snapshot->tap_fault_locked;
    gate.screen_on = (snapshot->screen_state == WATCH_SCREEN_STATE_ON);
    gate.active_page_muyu = (snapshot->nav.active_page == WATCH_NAV_PAGE_MUYU);
    return gate;
}

static void apply_done_overlay(const watch_state_snapshot_t *snapshot)
{
    ewf_ui_muyu_done_input_t in = {
        .pending_completion = false,
        .round_state = 0U,
        .round_cursor = 0U,
    };
    if (snapshot != NULL) {
        in.pending_completion = snapshot->tap_pending_completion;
        in.round_state = snapshot->tap_round_state;
        in.round_cursor = snapshot->tap_round_cursor;
    }

    ewf_ui_muyu_done_view_t view;
    ewf_ui_muyu_done_project(&in, &view);
    if (s_done_primed && memcmp(&view, &s_prev_done, sizeof(view)) == 0) {
        return;
    }
    s_prev_done = view;
    s_done_primed = true;

    if (ui_muyu_modal_done != NULL) {
        if (view.overlay_visible) {
            lv_obj_clear_flag(ui_muyu_modal_done, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ui_muyu_modal_done, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (ui_muyu_modal_title != NULL) {
        lv_label_set_text(ui_muyu_modal_title, view.title);
    }
    if (ui_muyu_modal_summary != NULL) {
        lv_label_set_text(ui_muyu_modal_summary, view.summary);
    }
    if (ui_muyu_modal_btn_restart != NULL) {
        if (view.restart_enabled) {
            lv_obj_clear_flag(ui_muyu_modal_btn_restart, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_muyu_modal_btn_restart, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_obj_clear_flag(ui_muyu_modal_btn_restart, LV_OBJ_FLAG_CLICKABLE);
        }
    }
    if (ui_muyu_modal_btn_exit != NULL) {
        if (view.exit_enabled) {
            lv_obj_clear_flag(ui_muyu_modal_btn_exit, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ui_muyu_modal_btn_exit, LV_OBJ_FLAG_CLICKABLE);
        } else {
            lv_obj_clear_flag(ui_muyu_modal_btn_exit, LV_OBJ_FLAG_CLICKABLE);
        }
    }
}

static void style_glyph_slot(lv_obj_t *label, bool filled, bool is_current)
{
    if (label == NULL) {
        return;
    }
    if (!filled) {
        lv_obj_set_style_text_font(label, &ui_font_serif400_26, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label,
                                    lv_color_hex(EWF_UI_MUYU_PLACEHOLDER_COLOR_HEX),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    if (is_current) {
        lv_obj_set_style_text_font(label, &ui_font_serif700_52, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(label,
                                    lv_color_hex(EWF_UI_MUYU_AMBER_400_HEX),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        return;
    }
    lv_obj_set_style_text_font(label, &ui_font_serif500_26, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label,
                                lv_color_hex(EWF_UI_MUYU_CHAR_COLOR_HEX),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void apply_ring_idle(void)
{
    static const uint32_t colors[EWF_UI_MUYU_TAP_RING_COUNT] = {
        EWF_UI_MUYU_RING_IDLE0_HEX,
        EWF_UI_MUYU_RING_IDLE1_HEX,
        EWF_UI_MUYU_RING_IDLE2_HEX,
    };
    static const lv_opa_t opas[EWF_UI_MUYU_TAP_RING_COUNT] = {
        EWF_UI_MUYU_RING_IDLE0_OPA,
        EWF_UI_MUYU_RING_IDLE1_OPA,
        EWF_UI_MUYU_RING_IDLE2_OPA,
    };
    for (uint32_t i = 0U; i < EWF_UI_MUYU_TAP_RING_COUNT; ++i) {
        if (ui_muyu_tap_rings[i] == NULL) {
            continue;
        }
        lv_obj_set_style_border_color(ui_muyu_tap_rings[i],
                                      lv_color_hex(colors[i]),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_muyu_tap_rings[i],
                                    opas[i],
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void apply_ring_flash(void)
{
    for (uint32_t i = 0U; i < EWF_UI_MUYU_TAP_RING_COUNT; ++i) {
        if (ui_muyu_tap_rings[i] == NULL) {
            continue;
        }
        lv_obj_set_style_border_color(ui_muyu_tap_rings[i],
                                      lv_color_hex(EWF_UI_MUYU_AMBER_300_HEX),
                                      LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(ui_muyu_tap_rings[i],
                                    LV_OPA_COVER,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void flash_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (s_rings.flashing) {
        (void)ewf_ui_tap_rings_on_tick(&s_rings, s_rings.flash_deadline_ms);
    }
    apply_ring_idle();
    if (s_flash_timer != NULL) {
        lv_timer_pause(s_flash_timer);
    }
}

static void restart_flash_timer(void)
{
    apply_ring_flash();
    if (s_flash_timer == NULL) {
        s_flash_timer = lv_timer_create(flash_timer_cb, EWF_UI_MUYU_TAP_FLASH_MS, NULL);
        if (s_flash_timer == NULL) {
            s_rings.flashing = false;
            s_rings.flash_deadline_ms = 0U;
            apply_ring_idle();
            return;
        }
        lv_timer_set_repeat_count(s_flash_timer, -1);
    }
    lv_timer_set_period(s_flash_timer, EWF_UI_MUYU_TAP_FLASH_MS);
    lv_timer_reset(s_flash_timer);
    lv_timer_resume(s_flash_timer);
}

void ui_muyu_projection_apply(const watch_state_snapshot_t *snapshot,
                              ewf_ui_tap_origin_t last_tap_origin,
                              uint32_t now_ms)
{
    if (!s_ready) {
        return;
    }

    uint32_t cursor = 0U;
    ewf_ui_tap_rings_gate_t gate = gate_from_snapshot(snapshot);
    if (snapshot != NULL) {
        cursor = snapshot->tap_round_cursor;
    }

    ewf_ui_muyu_belt_view_t belt;
    ewf_ui_muyu_belt_project(cursor, &belt);

    for (uint32_t i = 0U; i < EWF_UI_MUYU_BELT_SLOTS; ++i) {
        if (ui_muyu_glyph_slots[i] == NULL) {
            continue;
        }
        const bool is_current = ((int)i == belt.glyph_current_index);
        lv_label_set_text(ui_muyu_glyph_slots[i], belt.slots[i]);
        style_glyph_slot(ui_muyu_glyph_slots[i], belt.filled[i], is_current);
    }

    if (ui_muyu_scripture_progress != NULL) {
        lv_label_set_text(ui_muyu_scripture_progress, belt.progress_text);
    }
    if (ui_muyu_today_hint != NULL) {
        char today_text[24];
        const bool trusted =
            (snapshot != NULL) ? snapshot->time_synchronized : false;
        const uint32_t today_count =
            (snapshot != NULL) ? snapshot->tap_today_count : 0U;
        ewf_ui_today_value_format(trusted, today_count, today_text, sizeof(today_text));
        lv_label_set_text(ui_muyu_today_hint, today_text);
    }

    apply_done_overlay(snapshot);

    if (!s_cursor_primed) {
        s_prev_cursor = cursor;
        s_cursor_primed = true;
        return;
    }

    if (cursor > s_prev_cursor) {
        if (ewf_ui_tap_rings_on_event(&s_rings, now_ms, last_tap_origin, &gate)) {
            restart_flash_timer();
        }
        s_prev_cursor = cursor;
    } else if (cursor < s_prev_cursor) {
        s_prev_cursor = cursor;
    }
}
