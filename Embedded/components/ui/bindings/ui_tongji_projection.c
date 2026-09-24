/**
 * @file     ui_tongji_projection.c
 * @brief    统计页双卡 + reading-progress LVGL 投影实现。
 * @details  所有 lv_* 仅 ui_task；今日与 MUYU 共用 ewf_ui_today_value_format。
 *           本页无 woodfish/tap-rings，触摸不提交 device_touch。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_tongji_projection.h"

#include "esp_log.h"
#include "ui.h"
#include "ui_tongji_constants.h"
#include "ui_tongji_stats_policy.h"

#include <string.h>

static const char *TAG = "UI_TONGJI";

static bool s_ready;
static bool s_primed;
static ewf_ui_tongji_stats_view_t s_prev;

static void apply_view(const ewf_ui_tongji_stats_view_t *view);

esp_err_t ui_tongji_projection_init(void)
{
    s_ready = true;
    s_primed = false;
    memset(&s_prev, 0, sizeof(s_prev));
    return ESP_OK;
}

static void apply_view(const ewf_ui_tongji_stats_view_t *view)
{
    if (view == NULL) {
        return;
    }

    if (ui_tongji_today_value != NULL) {
        lv_label_set_text(ui_tongji_today_value, view->today_value);
        const uint32_t color = view->today_untrusted ? EWF_UI_TONGJI_MUTED_HEX
                                                     : EWF_UI_TONGJI_VALUE_HEX;
        lv_obj_set_style_text_color(ui_tongji_today_value,
                                    lv_color_hex(color),
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_tongji_total_value != NULL) {
        lv_label_set_text(ui_tongji_total_value, view->total_value);
    }
    if (ui_tongji_progress_value != NULL) {
        lv_label_set_text(ui_tongji_progress_value, view->progress_value);
    }
    if (ui_tongji_progress_percent != NULL) {
        lv_label_set_text(ui_tongji_progress_percent, view->progress_percent_text);
    }
    if (ui_tongji_round_label != NULL) {
        lv_label_set_text(ui_tongji_round_label, view->round_label);
    }
    if (ui_tongji_round_index != NULL) {
        lv_label_set_text(ui_tongji_round_index, view->round_index_text);
    }
    if (ui_tongji_round_status != NULL) {
        lv_label_set_text(ui_tongji_round_status, view->round_status_text);
    }
    if (ui_tongji_progress_arc != NULL) {
        lv_arc_set_value(ui_tongji_progress_arc, (int16_t)view->arc_value);
    }
    (void)TAG;
}

void ui_tongji_projection_apply(const watch_state_snapshot_t *snapshot)
{
    if (!s_ready) {
        return;
    }

    ewf_ui_tongji_stats_input_t in;
    memset(&in, 0, sizeof(in));
    if (snapshot != NULL) {
        in.time_synchronized = snapshot->time_synchronized;
        in.today_count = snapshot->tap_today_count;
        in.local_total = snapshot->tap_local_total;
        in.acked_total = snapshot->tap_acked_total;
        in.round_cursor = snapshot->tap_round_cursor;
        in.round_id = snapshot->tap_round_id;
        in.round_state = snapshot->tap_round_state;
        in.pending_completion = snapshot->tap_pending_completion;
    }

    ewf_ui_tongji_stats_view_t view;
    ewf_ui_tongji_stats_project(&in, &view);

    if (s_primed && memcmp(&view, &s_prev, sizeof(view)) == 0) {
        return;
    }
    apply_view(&view);
    s_prev = view;
    s_primed = true;
}
