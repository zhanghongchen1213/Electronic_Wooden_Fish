/**
 * @file     ui_jingwen_stream_policy.c
 * @brief    13 槽 append-only 流布局与锚点策略实现。
 * @details  裁决 B/E/F：全文换行非尾窗；新字强制锚尾；进度复用 muyu 公式。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_jingwen_stream_policy.h"

#include "ui_muyu_belt_policy.h"
#include "ui_scripture_display_expand.h"

#include <stdio.h>
#include <string.h>

void ewf_ui_jingwen_stream_project(uint32_t round_cursor,
                                   uint32_t tap_round_id,
                                   ewf_ui_jingwen_stream_view_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->glyph_row = -1;
    out->glyph_col = -1;

    ewf_ui_scripture_char_t stream[EWF_UI_JINGWEN_DISPLAY_CAP];
    const size_t total = ewf_ui_scripture_expand_consumed(
        round_cursor, stream, EWF_UI_JINGWEN_DISPLAY_CAP);
    out->display_char_count = (uint32_t)total;

    if (total == 0U) {
        out->progress_percent = ewf_ui_muyu_progress_percent(round_cursor);
        (void)snprintf(out->progress_text,
                       sizeof(out->progress_text),
                       "心经进度 0 / %u · 0%%",
                       (unsigned)EWF_UI_MUYU_PROGRESS_DENOM);
        if (tap_round_id > 0U) {
            (void)snprintf(out->round_index_text,
                           sizeof(out->round_index_text),
                           "第 %lu 次诵读",
                           (unsigned long)tap_round_id);
        }
        return;
    }

    size_t index = 0U;
    uint32_t row = 0U;
    while (index < total && row < EWF_UI_JINGWEN_MAX_ROWS) {
        uint8_t filled = 0U;
        while (filled < EWF_UI_JINGWEN_ROW_SLOTS && index < total) {
            (void)snprintf(out->rows[row][filled],
                           EWF_UI_MUYU_SLOT_BYTES,
                           "%s",
                           stream[index].utf8);
            ++filled;
            ++index;
        }
        out->row_filled[row] = filled;
        ++row;
    }
    out->row_count = row;

    /* 流尾唯一 glyph-current。 */
    if (row > 0U) {
        out->glyph_row = (int)(row - 1U);
        out->glyph_col = (int)out->row_filled[row - 1U] - 1;
    }

    out->progress_percent = ewf_ui_muyu_progress_percent(round_cursor);
    uint32_t shown = round_cursor;
    if (shown > EWF_UI_MUYU_PROGRESS_DENOM) {
        shown = EWF_UI_MUYU_PROGRESS_DENOM;
    }
    (void)snprintf(out->progress_text,
                   sizeof(out->progress_text),
                   "心经进度 %lu / %u · %lu%%",
                   (unsigned long)shown,
                   (unsigned)EWF_UI_MUYU_PROGRESS_DENOM,
                   (unsigned long)out->progress_percent);
    if (tap_round_id > 0U) {
        (void)snprintf(out->round_index_text,
                       sizeof(out->round_index_text),
                       "第 %lu 次诵读",
                       (unsigned long)tap_round_id);
    }
}

bool ewf_ui_jingwen_should_anchor_tail(bool reviewing, bool cursor_grew)
{
    (void)reviewing;
    /* 裁决 E：新字到达一律强制锚尾（即使此前在回看）。 */
    return cursor_grew;
}
