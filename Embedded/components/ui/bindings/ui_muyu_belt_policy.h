/**
 * @file     ui_muyu_belt_policy.h
 * @brief    木鱼页七字带与心经进度纯逻辑（零 ESP-IDF）。
 * @details  只读 round_cursor + canonical 步偏移；UI 绝不本地 ++cursor。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_MUYU_BELT_POLICY_H
#define EWF_UI_MUYU_BELT_POLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ui_muyu_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /** 7 槽 UTF-8 文本（空位为占位符或空串）。 */
    char slots[EWF_UI_MUYU_BELT_SLOTS][EWF_UI_MUYU_SLOT_BYTES];
    /** 槽是否含正式显示字符（非空位占位）。 */
    bool filled[EWF_UI_MUYU_BELT_SLOTS];
    /** glyph-current 槽索引；-1=全空。 */
    int glyph_current_index;
    /** 已展开显示字符总数（含标点占槽）。 */
    uint32_t display_char_count;
    /** 进度百分比 0–100（cursor==260 强制 100）。 */
    uint32_t progress_percent;
    /** 进度条已填充段数 0–8（四舍五入）。 */
    uint32_t progress_segments;
    /** 进度文案缓冲（产品词，无内部 ID；含一位小数百分比）。 */
    char progress_text[48];
} ewf_ui_muyu_belt_view_t;

/**
 * @brief 由 round_cursor 投影七字带与心经进度
 * @param round_cursor 已消费可消费字数（[0,260]）
 * @param out 输出视图；NULL 时无操作
 */
void ewf_ui_muyu_belt_project(uint32_t round_cursor, ewf_ui_muyu_belt_view_t *out);

/**
 * @brief 计算进度百分比（向下取整；满游标强制 100）
 */
uint32_t ewf_ui_muyu_progress_percent(uint32_t round_cursor);

/**
 * @brief 计算进度条已填充段数（0–8，四舍五入；满游标强制全满）
 */
uint32_t ewf_ui_muyu_progress_segments(uint32_t round_cursor);

/**
 * @brief 心经进度产品文案「心经进度 N / 260 字 · P.P%」（MUYU/JINGWEN 共用）
 */
void ewf_ui_muyu_progress_text(uint32_t round_cursor, char *out, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_MUYU_BELT_POLICY_H */
