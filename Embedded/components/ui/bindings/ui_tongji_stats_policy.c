/**
 * @file     ui_tongji_stats_policy.c
 * @brief    统计页今日/累计/进度投影实现。
 * @details  不输出七日/三十日/连续天数字段；pending 只标记布尔+同步词表短语。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_tongji_stats_policy.h"

#include "ui_muyu_belt_policy.h"

#include <stdio.h>
#include <string.h>

static void format_uint_thousands(uint32_t value, char *out, size_t out_len)
{
    if (out == NULL || out_len == 0U) {
        return;
    }
    char raw[16];
    (void)snprintf(raw, sizeof(raw), "%lu", (unsigned long)value);
    const size_t len = strlen(raw);
    if (len == 0U) {
        out[0] = '\0';
        return;
    }
    size_t o = 0U;
    for (size_t i = 0U; i < len && o + 1U < out_len; ++i) {
        const size_t remain = len - i;
        if (i > 0U && (remain % 3U) == 0U) {
            out[o++] = ',';
            if (o + 1U >= out_len) {
                break;
            }
        }
        out[o++] = raw[i];
    }
    out[o < out_len ? o : out_len - 1U] = '\0';
}

void ewf_ui_today_value_format(bool time_synchronized,
                               uint32_t today_count,
                               char *out,
                               size_t out_len)
{
    if (out == NULL || out_len == 0U) {
        return;
    }
    if (!time_synchronized) {
        (void)snprintf(out, out_len, "%s", "待校时");
        return;
    }
    (void)snprintf(out, out_len, "%lu", (unsigned long)today_count);
}

void ewf_ui_tongji_stats_project(const ewf_ui_tongji_stats_input_t *in,
                                 ewf_ui_tongji_stats_view_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    (void)snprintf(out->round_label, sizeof(out->round_label), "%s", "本次诵读");

    if (in == NULL) {
        out->today_untrusted = true;
        ewf_ui_today_value_format(false, 0U, out->today_value, sizeof(out->today_value));
        (void)snprintf(out->total_value, sizeof(out->total_value), "0");
        (void)snprintf(out->progress_value, sizeof(out->progress_value),
                       "0 / %u 字", (unsigned)EWF_UI_TONGJI_PROGRESS_DENOM);
        (void)snprintf(out->progress_percent_text, sizeof(out->progress_percent_text),
                       "已完成 0%%");
        return;
    }

    out->today_untrusted = !in->time_synchronized;
    ewf_ui_today_value_format(in->time_synchronized,
                              in->today_count,
                              out->today_value,
                              sizeof(out->today_value));

    format_uint_thousands(in->local_total, out->total_value, sizeof(out->total_value));

    uint32_t shown = in->round_cursor;
    if (shown > EWF_UI_TONGJI_PROGRESS_DENOM) {
        shown = EWF_UI_TONGJI_PROGRESS_DENOM;
    }
    out->progress_percent = ewf_ui_muyu_progress_percent(in->round_cursor);
    out->arc_value = out->progress_percent;
    (void)snprintf(out->progress_value,
                   sizeof(out->progress_value),
                   "%lu / %u 字",
                   (unsigned long)shown,
                   (unsigned)EWF_UI_TONGJI_PROGRESS_DENOM);
    (void)snprintf(out->progress_percent_text,
                   sizeof(out->progress_percent_text),
                   "已完成 %lu%%",
                   (unsigned long)out->progress_percent);

    if (in->round_id > 0U) {
        (void)snprintf(out->round_index_text,
                       sizeof(out->round_index_text),
                       "第 %lu 次诵读",
                       (unsigned long)in->round_id);
    }

    if (in->pending_completion || in->round_state == 1U ||
        in->round_cursor >= EWF_UI_TONGJI_PROGRESS_DENOM) {
        (void)snprintf(out->round_status_text, sizeof(out->round_status_text), "%s",
                       "已完成");
    } else if (in->round_cursor > 0U) {
        (void)snprintf(out->round_status_text, sizeof(out->round_status_text), "%s",
                       "进行中");
    }

    out->pending_sync = (in->local_total > in->acked_total);
    if (out->pending_sync) {
        (void)snprintf(out->pending_phrase, sizeof(out->pending_phrase), "%s", "待同步");
    }
}
