/**
 * @file     ui_muyu_done_policy.c
 * @brief    木鱼完成遮罩 modal-done 纯逻辑实现。
 * @details  摘要格式固定：「心经进度 {cursor} / 260 字」（完成时常为 260/260）。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_muyu_done_policy.h"

#include <stdio.h>
#include <string.h>

bool ewf_ui_muyu_done_is_locked(const ewf_ui_muyu_done_input_t *in)
{
    if (in == NULL) {
        return false;
    }
    return in->pending_completion ||
           in->round_state == 1U; /* EWF_PROGRESS_ROUND_STATE_COMPLETED */
}

void ewf_ui_muyu_done_project(const ewf_ui_muyu_done_input_t *in,
                              ewf_ui_muyu_done_view_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    (void)snprintf(out->title, sizeof(out->title), "%s", EWF_UI_MUYU_DONE_TITLE);
    out->overlay_visible = ewf_ui_muyu_done_is_locked(in);
    out->restart_enabled = out->overlay_visible;
    out->exit_enabled = out->overlay_visible;

    uint32_t cursor = 0U;
    if (in != NULL) {
        cursor = in->round_cursor;
        if (cursor > EWF_UI_MUYU_PROGRESS_DENOM) {
            cursor = EWF_UI_MUYU_PROGRESS_DENOM;
        }
    }
    (void)snprintf(out->summary,
                   sizeof(out->summary),
                   "心经进度 %lu / %u 字",
                   (unsigned long)cursor,
                   (unsigned)EWF_UI_MUYU_PROGRESS_DENOM);
}

bool ewf_ui_muyu_done_text_allowed(const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return false;
    }
    if (strcmp(text, EWF_UI_MUYU_DONE_TITLE) == 0 ||
        strcmp(text, EWF_UI_MUYU_DONE_RESTART) == 0 ||
        strcmp(text, EWF_UI_MUYU_DONE_EXIT) == 0) {
        return true;
    }
    /* 摘要：固定前缀「心经进度 」+ 数字/斜杠/空格 +「 字」。 */
    static const char k_prefix[] = "心经进度 ";
    static const char k_suffix[] = " 字";
    const size_t prefix_len = sizeof(k_prefix) - 1U;
    const size_t suffix_len = sizeof(k_suffix) - 1U;
    const size_t len = strlen(text);
    if (len <= prefix_len + suffix_len) {
        return false;
    }
    if (strncmp(text, k_prefix, prefix_len) != 0) {
        return false;
    }
    if (strcmp(text + (len - suffix_len), k_suffix) != 0) {
        return false;
    }
    return true;
}
