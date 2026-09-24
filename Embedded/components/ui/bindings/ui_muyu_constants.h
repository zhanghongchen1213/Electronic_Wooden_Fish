/**
 * @file     ui_muyu_constants.h
 * @brief    木鱼页字带/三环/触区/完成遮罩常量（Story 3.2 + 3.6 裁决入口）。
 * @details  3.2 裁决 A–H：7 槽=显示字符尾窗；只读 round_cursor；三环仅 PVDF/触摸；
 *           单 timer 重启；木鱼点击为唯一 device_touch UI 源；Serif 字带档；
 *           CHARGING_PAUSE 不实现；今日无可信时间显示「待校时」。
 *           3.6 裁决 A/G：完成遮罩为 MUYU 页内 modal-done；禁止顶层 OVERLAY Screen；
 *           无礼花；颜色对齐 DESIGN tokens。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_MUYU_CONSTANTS_H
#define EWF_UI_MUYU_CONSTANTS_H

#include <stdint.h>

/** 七字带固定槽位数（视口，不是经文上限）。 */
#define EWF_UI_MUYU_BELT_SLOTS 7U

/** 单槽 UTF-8 字节上限（含 NUL）。 */
#define EWF_UI_MUYU_SLOT_BYTES 8U

/** 三环数量。 */
#define EWF_UI_MUYU_TAP_RING_COUNT 3U

/** 三环 flash 时长（毫秒）。 */
#define EWF_UI_MUYU_TAP_FLASH_MS 160U

/** 木鱼触区最小边长（像素）。 */
#define EWF_UI_MUYU_TOUCH_MIN_PX 96U

/** glyph-current 焦点色 amber.400。 */
#define EWF_UI_MUYU_AMBER_400_HEX 0xD9A441U

/** tap-rings flash 描边色 amber.300；restart 按钮同色。 */
#define EWF_UI_MUYU_AMBER_300_HEX 0xE6BD69U

/** 已填槽正文色（高对比白）；遮罩标题同色。 */
#define EWF_UI_MUYU_CHAR_COLOR_HEX 0xFBFAF0U

/** 空位占位色（低对比）。 */
#define EWF_UI_MUYU_PLACEHOLDER_COLOR_HEX 0x6B6B6BU

/** 心经可消费分母（与 canonical 一致，禁止用 7 冒充）。 */
#define EWF_UI_MUYU_PROGRESS_DENOM 260U

/** idle 环描边（DESIGN：白半透明系，按不透明近似写入）。 */
#define EWF_UI_MUYU_RING_IDLE0_HEX 0xFBFAF0U
#define EWF_UI_MUYU_RING_IDLE1_HEX 0xFBFAF0U
#define EWF_UI_MUYU_RING_IDLE2_HEX 0xFBFAF0U

/** idle 环不透明度（LVGL opa）。 */
#define EWF_UI_MUYU_RING_IDLE0_OPA 0x66U
#define EWF_UI_MUYU_RING_IDLE1_OPA 0x44U
#define EWF_UI_MUYU_RING_IDLE2_OPA 0x33U

/** 空位占位字符（低对比点，非未来经文）。 */
#define EWF_UI_MUYU_PLACEHOLDER_GLYPH "·"

/* —— Story 3.6 modal-done（MUYU.DONE_OVERLAY 帧内浮层）—— */

/** 全屏遮罩不透明度（#00000099 ≈ 60% 黑）。 */
#define EWF_UI_MUYU_MODAL_OVERLAY_OPA 0x99U

/** 遮罩叠层宽高（与画布一致）。 */
#define EWF_UI_MUYU_MODAL_OVERLAY_W 410
#define EWF_UI_MUYU_MODAL_OVERLAY_H 502

/** 卡片几何：338×190 @ left=36,top=150。 */
#define EWF_UI_MUYU_MODAL_CARD_W 338
#define EWF_UI_MUYU_MODAL_CARD_H 190
#define EWF_UI_MUYU_MODAL_CARD_X 36
#define EWF_UI_MUYU_MODAL_CARD_Y 150
#define EWF_UI_MUYU_MODAL_CARD_RADIUS 16

/** 卡片底色 #121212。 */
#define EWF_UI_MUYU_MODAL_CARD_BG_HEX 0x121212U

/** 标题色 #fbfaf0。 */
#define EWF_UI_MUYU_MODAL_TITLE_HEX 0xFBFAF0U

/** 摘要/退出色 #cfc4b0。 */
#define EWF_UI_MUYU_MODAL_MUTED_HEX 0xCFC4B0U

/** restart 按钮色 amber.300。 */
#define EWF_UI_MUYU_MODAL_RESTART_HEX 0xE6BD69U

/** 文案容量（含 NUL）。 */
#define EWF_UI_MUYU_DONE_TITLE_CAP 16U
#define EWF_UI_MUYU_DONE_SUMMARY_CAP 48U

/** 产品文案闭包。 */
#define EWF_UI_MUYU_DONE_TITLE "本轮完成"
#define EWF_UI_MUYU_DONE_RESTART "从头开始"
#define EWF_UI_MUYU_DONE_EXIT "退出"

#endif /* EWF_UI_MUYU_CONSTANTS_H */
