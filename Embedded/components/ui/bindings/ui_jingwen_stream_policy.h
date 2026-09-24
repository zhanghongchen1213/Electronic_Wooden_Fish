/**
 * @file     ui_jingwen_stream_policy.h
 * @brief    经文页 13 槽 append-only 流纯逻辑（零 ESP-IDF）。
 * @details  裁决 B：全文按 13 槽换行；流尾唯一 glyph-current；前缀只增长。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_JINGWEN_STREAM_POLICY_H
#define EWF_UI_JINGWEN_STREAM_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_jingwen_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 经文流投影视图。 */
typedef struct {
    /** 行数（0=空流）。 */
    uint32_t row_count;
    /** 每行槽文本；未用行/槽为空串。 */
    char rows[EWF_UI_JINGWEN_MAX_ROWS][EWF_UI_JINGWEN_ROW_SLOTS][EWF_UI_MUYU_SLOT_BYTES];
    /** 每行已填槽数（≤13）。 */
    uint8_t row_filled[EWF_UI_JINGWEN_MAX_ROWS];
    /** 已展开显示字符总数。 */
    uint32_t display_char_count;
    /** glyph-current 行索引；空流=-1。 */
    int glyph_row;
    /** glyph-current 列索引；空流=-1。 */
    int glyph_col;
    /** 进度百分比 0–100。 */
    uint32_t progress_percent;
    /** 进度文案。 */
    char progress_text[48];
    /** 可选 round-index 文案；无 round_id 时为空。 */
    char round_index_text[32];
} ewf_ui_jingwen_stream_view_t;

/**
 * @brief 由 round_cursor 投影 13 槽全文流与进度
 * @param round_cursor 已消费可消费字数
 * @param tap_round_id 本地诵读轮次（0=不投影 round-index）
 * @param out 输出；NULL 无操作
 */
void ewf_ui_jingwen_stream_project(uint32_t round_cursor,
                                   uint32_t tap_round_id,
                                   ewf_ui_jingwen_stream_view_t *out);

/**
 * @brief 锚点策略：回看态被新字打断后强制回尾
 * @param reviewing 当前是否处于回看（scroll 离开尾部）
 * @param cursor_grew 本轮显示流是否因新字增长
 * @return true=应强制锚回流尾
 */
bool ewf_ui_jingwen_should_anchor_tail(bool reviewing, bool cursor_grew);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_JINGWEN_STREAM_POLICY_H */
