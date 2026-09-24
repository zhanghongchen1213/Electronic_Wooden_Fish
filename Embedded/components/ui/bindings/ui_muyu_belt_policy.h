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
    /** 进度文案缓冲（产品词，无内部 ID；含百分比）。 */
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

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_MUYU_BELT_POLICY_H */
