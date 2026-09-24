/**
 * @file     ui_jingwen_projection.c
 * @brief    经文页 13 槽流 LVGL 投影与锚回流尾。
 * @details  所有 lv_* / scroll API 仅在 ui_task 调用。滚动失败只打中文日志，
 *           不调用 progress/tap API，不阻塞输入队列。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_jingwen_projection.h"

#include "esp_log.h"
#include "ui.h"
#include "ui_jingwen_constants.h"
#include "ui_jingwen_stream_policy.h"

#include <string.h>

static const char *TAG = "UI_JINGWEN";

static bool s_ready;
static uint32_t s_prev_cursor;
static uint32_t s_prev_round_id;
static bool s_cursor_primed;
static uint32_t s_visible_rows;

static void style_confirmed(lv_obj_t *label);
static void style_current(lv_obj_t *label);
static void clear_row_labels(uint32_t row);
static void ensure_row_labels(uint32_t row_count);
static void sync_row_visibility(uint32_t row_count);
static void anchor_to_tail(void);

esp_err_t ui_jingwen_projection_init(void)
{
    s_ready = true;
    s_prev_cursor = 0U;
    s_prev_round_id = 0U;
    s_cursor_primed = false;
    s_visible_rows = 0U;
    return ESP_OK;
}

static void style_confirmed(lv_obj_t *label)
{
    if (label == NULL) {
        return;
    }
    lv_obj_set_style_text_font(label, &ui_font_serif500_26, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label,
                                lv_color_hex(EWF_UI_JINGWEN_CONFIRMED_COLOR_HEX),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void style_current(lv_obj_t *label)
{
    if (label == NULL) {
        return;
    }
    lv_obj_set_style_text_font(label, &ui_font_serif700_52, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label,
                                lv_color_hex(EWF_UI_MUYU_AMBER_400_HEX),
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void clear_row_labels(uint32_t row)
{
    if (row >= EWF_UI_JINGWEN_MAX_ROWS) {
        return;
    }
    for (uint32_t col = 0U; col < EWF_UI_JINGWEN_ROW_SLOTS; ++col) {
        if (ui_jingwen_row_slots[row][col] != NULL) {
            lv_label_set_text(ui_jingwen_row_slots[row][col], "");
            style_confirmed(ui_jingwen_row_slots[row][col]);
        }
    }
}

static void ensure_row_labels(uint32_t row_count)
{
    if (ui_jingwen_history_content == NULL) {
        return;
    }
    /* destroy 后句柄被置空时重置计数，避免跳过重建。 */
    if (s_visible_rows > 0U &&
        (ui_jingwen_row_boxes[0] == NULL || ui_jingwen_row_slots[0][0] == NULL)) {
        s_visible_rows = 0U;
    }
    while (s_visible_rows < row_count && s_visible_rows < EWF_UI_JINGWEN_MAX_ROWS) {
        const uint32_t row = s_visible_rows;
        lv_obj_t *row_box = lv_obj_create(ui_jingwen_history_content);
        if (row_box == NULL) {
            ESP_LOGW(TAG, "经文行盒创建失败：row=%lu", (unsigned long)row);
            break;
        }
        lv_obj_set_size(row_box, EWF_UI_JINGWEN_HISTORY_W - 16, EWF_UI_JINGWEN_ROW_HEIGHT);
        lv_obj_set_style_bg_opa(row_box, LV_OPA_TRANSP, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(row_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(row_box, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_clear_flag(row_box, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        ui_jingwen_row_boxes[row] = row_box;

        for (uint32_t col = 0U; col < EWF_UI_JINGWEN_ROW_SLOTS; ++col) {
            lv_obj_t *label = lv_label_create(row_box);
            if (label == NULL) {
                ESP_LOGW(TAG, "经文槽标签创建失败：row=%lu col=%lu",
                         (unsigned long)row, (unsigned long)col);
                continue;
            }
            lv_label_set_text(label, "");
            style_confirmed(label);
            lv_obj_set_width(label, EWF_UI_JINGWEN_SLOT_W);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_pos(label,
                           (int)(col * (EWF_UI_JINGWEN_SLOT_W + EWF_UI_JINGWEN_SLOT_GAP)),
                           0);
            ui_jingwen_row_slots[row][col] = label;
        }
        ++s_visible_rows;
    }
}

static void sync_row_visibility(uint32_t row_count)
{
    for (uint32_t row = 0U; row < s_visible_rows; ++row) {
        lv_obj_t *row_box = ui_jingwen_row_boxes[row];
        if (row_box == NULL) {
            continue;
        }
        if (row < row_count) {
            lv_obj_clear_flag(row_box, LV_OBJ_FLAG_HIDDEN);
        } else {
            clear_row_labels(row);
            lv_obj_add_flag(row_box, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void anchor_to_tail(void)
{
    if (ui_jingwen_history == NULL) {
        ESP_LOGW(TAG, "经文锚回流尾失败：history 对象为空");
        return;
    }
    lv_obj_update_layout(ui_jingwen_history);
    if (ui_jingwen_glyph_current != NULL) {
        lv_obj_scroll_to_view(ui_jingwen_glyph_current, LV_ANIM_OFF);
        return;
    }
    const lv_coord_t bottom = lv_obj_get_scroll_bottom(ui_jingwen_history);
    if (bottom > 0) {
        const lv_coord_t y = lv_obj_get_scroll_y(ui_jingwen_history) + bottom;
        lv_obj_scroll_to_y(ui_jingwen_history, y, LV_ANIM_OFF);
    }
}

void ui_jingwen_projection_apply(const watch_state_snapshot_t *snapshot)
{
    if (!s_ready) {
        return;
    }

    uint32_t cursor = 0U;
    uint32_t round_id = 0U;
    if (snapshot != NULL) {
        cursor = snapshot->tap_round_cursor;
        round_id = snapshot->tap_round_id;
    }

    /* 游标与轮次均不变时可跳过重排（脏标记）。 */
    const bool unchanged = s_cursor_primed && (cursor == s_prev_cursor) &&
                           (round_id == s_prev_round_id);
    if (unchanged) {
        return;
    }

    ewf_ui_jingwen_stream_view_t view;
    ewf_ui_jingwen_stream_project(cursor, round_id, &view);

    ensure_row_labels(view.row_count);
    sync_row_visibility(view.row_count);

    ui_jingwen_glyph_current = NULL;
    for (uint32_t row = 0U; row < view.row_count; ++row) {
        for (uint32_t col = 0U; col < EWF_UI_JINGWEN_ROW_SLOTS; ++col) {
            lv_obj_t *label = ui_jingwen_row_slots[row][col];
            if (label == NULL) {
                continue;
            }
            if (col < view.row_filled[row]) {
                lv_label_set_text(label, view.rows[row][col]);
                const bool is_current =
                    ((int)row == view.glyph_row) && ((int)col == view.glyph_col);
                if (is_current) {
                    style_current(label);
                    ui_jingwen_glyph_current = label;
                } else {
                    style_confirmed(label);
                }
            } else {
                lv_label_set_text(label, "");
                style_confirmed(label);
            }
        }
    }

    if (ui_jingwen_scripture_progress != NULL) {
        lv_label_set_text(ui_jingwen_scripture_progress, view.progress_text);
    }
    if (ui_jingwen_round_index != NULL) {
        lv_label_set_text(ui_jingwen_round_index,
                          view.round_index_text[0] != '\0' ? view.round_index_text : "");
    }

    const bool cursor_grew = s_cursor_primed && (cursor > s_prev_cursor);
    if (!s_cursor_primed || cursor_grew || cursor < s_prev_cursor) {
        /* 裁决 E：新字（或首帧/回退）锚回流尾。回看只改 scroll，数据层不变。 */
        if (ewf_ui_jingwen_should_anchor_tail(true, cursor_grew) || !s_cursor_primed ||
            cursor < s_prev_cursor) {
            anchor_to_tail();
        }
    }

    s_prev_cursor = cursor;
    s_prev_round_id = round_id;
    s_cursor_primed = true;
}
