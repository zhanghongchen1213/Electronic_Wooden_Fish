/**
 * @file     ui_scripture_display_expand.c
 * @brief    心经步偏移展开实现（木鱼/经文页共享）。
 * @details  裁决 A：一步可展开多槽（如「萨，」）；不得预览未消费步。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_scripture_display_expand.h"

#include "ewf_scripture_canonical.h"

#include <string.h>

static size_t utf8_char_len(unsigned char lead)
{
    if (lead < 0x80U) {
        return 1U;
    }
    if ((lead & 0xE0U) == 0xC0U) {
        return 2U;
    }
    if ((lead & 0xF0U) == 0xE0U) {
        return 3U;
    }
    if ((lead & 0xF8U) == 0xF0U) {
        return 4U;
    }
    return 1U;
}

size_t ewf_ui_scripture_expand_consumed(uint32_t round_cursor,
                                        ewf_ui_scripture_char_t *out,
                                        size_t cap)
{
    if (out == NULL || cap == 0U) {
        return 0U;
    }

    size_t count = 0U;
    uint32_t cursor = round_cursor;
    if (cursor > (uint32_t)EWF_SCRIPTURE_CONSUMABLE_COUNT) {
        cursor = (uint32_t)EWF_SCRIPTURE_CONSUMABLE_COUNT;
    }

    for (uint32_t step = 0U; step < cursor; ++step) {
        const uint16_t begin = EWF_SCRIPTURE_STEP_OFFSETS[step];
        const uint16_t end =
            (step + 1U < (uint32_t)EWF_SCRIPTURE_STEP_COUNT)
                ? EWF_SCRIPTURE_STEP_OFFSETS[step + 1U]
                : (uint16_t)EWF_SCRIPTURE_DISPLAY_BYTES;
        uint16_t offset = begin;
        while (offset < end && count < cap) {
            const unsigned char lead =
                (unsigned char)EWF_SCRIPTURE_DISPLAY_UTF8[offset];
            const size_t clen = utf8_char_len(lead);
            if (offset + (uint16_t)clen > end) {
                break;
            }
            if (clen >= EWF_UI_MUYU_SLOT_BYTES) {
                offset = (uint16_t)(offset + clen);
                continue;
            }
            memcpy(out[count].utf8, &EWF_SCRIPTURE_DISPLAY_UTF8[offset], clen);
            out[count].utf8[clen] = '\0';
            ++count;
            offset = (uint16_t)(offset + clen);
        }
    }
    return count;
}
