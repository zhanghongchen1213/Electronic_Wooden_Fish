/**
 * @file     ui_scripture_display_expand.h
 * @brief    心经已消费步 → 显示字符流展开（零 ESP-IDF）。
 * @details  Story 3.3 裁决 A/C：与木鱼七字带共用同一展开语义；唯一经文源
 *           ewf_scripture_canonical.h；禁止第二份正文。UI 只读 round_cursor。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_SCRIPTURE_DISPLAY_EXPAND_H
#define EWF_UI_SCRIPTURE_DISPLAY_EXPAND_H

#include <stddef.h>
#include <stdint.h>

#include "ui_muyu_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 单显示字符（UTF-8，含标点占槽）。 */
typedef struct {
    char utf8[EWF_UI_MUYU_SLOT_BYTES];
} ewf_ui_scripture_char_t;

/**
 * @brief 将 [0, round_cursor) 可消费步展开为显示字符流
 * @param round_cursor 已消费可消费字数（钳制到 CONSUMABLE_COUNT）
 * @param out 输出缓冲；NULL 时返回 0
 * @param cap 输出容量（字符槽数）
 * @return 实际写入的显示字符数（含标点占槽）
 */
size_t ewf_ui_scripture_expand_consumed(uint32_t round_cursor,
                                        ewf_ui_scripture_char_t *out,
                                        size_t cap);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_SCRIPTURE_DISPLAY_EXPAND_H */
