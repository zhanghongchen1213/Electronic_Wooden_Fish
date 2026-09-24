/**
 * @file     ui_muyu_belt_policy.c
 * @brief    七字带右对齐尾窗与心经进度投影实现。
 * @details  裁决 A/B/C：显示字符槽（含标点占槽）；尾窗右侧=最新=glyph-current；
 *           只读 round_cursor，禁止本地累加。经文唯一源为 ewf_scripture_canonical.h。
 *           Story 3.3：展开逻辑抽至 ui_scripture_display_expand（保持 7 槽尾窗语义）。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_muyu_belt_policy.h"

#include "ewf_scripture_canonical.h"
#include "ui_scripture_display_expand.h"

#include <stdio.h>
#include <string.h>

/** 显示流最大字符数（canonical totalChars）。 */
#define EWF_UI_MUYU_DISPLAY_CAP ((size_t)EWF_SCRIPTURE_TOTAL_CHARS)

uint32_t ewf_ui_muyu_progress_percent(uint32_t round_cursor)
{
    uint32_t cursor = round_cursor;
    if (cursor >= EWF_UI_MUYU_PROGRESS_DENOM) {
        return 100U;
    }
    return (cursor * 100U) / EWF_UI_MUYU_PROGRESS_DENOM;
}

void ewf_ui_muyu_belt_project(uint32_t round_cursor, ewf_ui_muyu_belt_view_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    out->glyph_current_index = -1;

    ewf_ui_scripture_char_t stream[EWF_UI_MUYU_DISPLAY_CAP];
    const size_t total = ewf_ui_scripture_expand_consumed(
        round_cursor, stream, EWF_UI_MUYU_DISPLAY_CAP);
    out->display_char_count = (uint32_t)total;

    const size_t window =
        total > EWF_UI_MUYU_BELT_SLOTS ? EWF_UI_MUYU_BELT_SLOTS : total;
    const size_t start = total - window;
    const size_t left_pad = EWF_UI_MUYU_BELT_SLOTS - window;

    for (size_t i = 0U; i < EWF_UI_MUYU_BELT_SLOTS; ++i) {
        if (i < left_pad) {
            (void)snprintf(out->slots[i],
                           EWF_UI_MUYU_SLOT_BYTES,
                           "%s",
                           EWF_UI_MUYU_PLACEHOLDER_GLYPH);
            out->filled[i] = false;
        } else {
            const size_t src = start + (i - left_pad);
            (void)snprintf(out->slots[i],
                           EWF_UI_MUYU_SLOT_BYTES,
                           "%s",
                           stream[src].utf8);
            out->filled[i] = true;
            out->glyph_current_index = (int)i;
        }
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
}
