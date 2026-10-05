/**
 * @file     ui_muyu_constants.h
 * @brief    木鱼页字带/三环/触区/完成遮罩常量（Story 3.2 + 3.6 裁决入口）。
 * @details  3.2 裁决 A–H：7 槽=显示字符尾窗；只读 round_cursor；三环仅 PVDF/触摸；
 *           单 timer 重启；木鱼点击为唯一 device_touch UI 源；Serif 字带档；
 *           今日无可信时间显示「待校时」。
 *           3.6 裁决 A/G：完成遮罩为 MUYU 页内 modal-done；禁止顶层 OVERLAY Screen；
 *           无礼花；颜色对齐 DESIGN tokens。
 *           UI 对齐批次（2026-10）：几何与字号对齐 ewf-device-ui-export.html
 *           （字带面板 362×72、8 段进度条、tap-zone 348×216、椭圆环位图、
 *           charging banner、modal 内部排版）。
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

/** 三环数量（环形位图内仍为三道椭圆描边）。 */
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

/** 空位占位色（低对比，设计 charcell 空槽 #a6a29a）。 */
#define EWF_UI_MUYU_PLACEHOLDER_COLOR_HEX 0xA6A29AU

/** 空位占位字符（低对比点，非未来经文）。 */
#define EWF_UI_MUYU_PLACEHOLDER_GLYPH "·"

/** 心经可消费分母（与 canonical 一致，禁止用 7 冒充）。 */
#define EWF_UI_MUYU_PROGRESS_DENOM 260U

/* —— 字带面板（HTML cmp_charcell_recent_window 362×72 @ (23,99)）—— */

/** 字带面板宽/高。 */
#define EWF_UI_MUYU_BELT_PANEL_W 362
#define EWF_UI_MUYU_BELT_PANEL_H 72

/** 字带面板位置。 */
#define EWF_UI_MUYU_BELT_PANEL_X 23
#define EWF_UI_MUYU_BELT_PANEL_Y 99

/** 字带面板底色 #121212。 */
#define EWF_UI_MUYU_BELT_PANEL_BG_HEX 0x121212U

/** 字带面板圆角。 */
#define EWF_UI_MUYU_BELT_PANEL_RADIUS 18

/** 槽宽/槽高（HTML charcell 42×34）。 */
#define EWF_UI_MUYU_SLOT_W 42
#define EWF_UI_MUYU_SLOT_H 34

/** 槽首 x / 槽 y（面板内）。 */
#define EWF_UI_MUYU_SLOT_X0 15
#define EWF_UI_MUYU_SLOT_Y 20

/** 槽步进（42 + 6 间隙）。 */
#define EWF_UI_MUYU_SLOT_PITCH 48

/* —— 心经进度（HTML cmp_scripture_progress 346×68 @ (31,171)）—— */

/** 进度组件宽/高/位置。 */
#define EWF_UI_MUYU_PROGRESS_W 346
#define EWF_UI_MUYU_PROGRESS_H 68
#define EWF_UI_MUYU_PROGRESS_X 31
#define EWF_UI_MUYU_PROGRESS_Y 171

/** 进度 track：346×10 @ (0,2)，圆角 5。 */
#define EWF_UI_MUYU_PROGRESS_TRACK_H 10
#define EWF_UI_MUYU_PROGRESS_TRACK_Y 2
#define EWF_UI_MUYU_PROGRESS_TRACK_RADIUS 5

/** track/未填充段色 #252525。 */
#define EWF_UI_MUYU_PROGRESS_TRACK_HEX 0x252525U

/** 进度分段数与几何（段宽 41 + 2 间隙，x=round(i*43.25)，尾段右缘 344）。 */
#define EWF_UI_MUYU_PROGRESS_SEGMENTS 8U
#define EWF_UI_MUYU_PROGRESS_SEG_W 41
#define EWF_UI_MUYU_PROGRESS_SEG_H 10
#define EWF_UI_MUYU_PROGRESS_SEG_RADIUS 4

/** 进度 label @ (0,22) 346×28 居中。 */
#define EWF_UI_MUYU_PROGRESS_LABEL_Y 22
#define EWF_UI_MUYU_PROGRESS_LABEL_H 28

/* —— 今日/累计敲击（HTML today-taps @ (24,233) / total-taps @ (216,233)）—— */

/** 今日敲击 label 位置。 */
#define EWF_UI_MUYU_TODAY_X 24
#define EWF_UI_MUYU_TODAY_Y 233

/** 今日值相对容器 +70（绝对 94）。 */
#define EWF_UI_MUYU_TODAY_VALUE_DX 70

/** 累计敲击行：右对齐 170×28 @ (216,233)。 */
#define EWF_UI_MUYU_TOTAL_X 216
#define EWF_UI_MUYU_TOTAL_Y 233
#define EWF_UI_MUYU_TOTAL_W 170
#define EWF_UI_MUYU_TOTAL_H 28

/** 今日/累计行高（两行共用 28）。 */
#define EWF_UI_MUYU_TAPS_ROW_H 28

/** 未校时值色（muted）。 */
#define EWF_UI_MUYU_UNTRUSTED_HEX 0xA6A29AU

/* —— woodfish（HTML tap-zone 348×216 @ (32,265) + anatomy 300×180 @ (55,283)）—— */

/** tap 热区宽/高/位置。 */
#define EWF_UI_MUYU_TAPZONE_W 348
#define EWF_UI_MUYU_TAPZONE_H 216
#define EWF_UI_MUYU_TAPZONE_X 32
#define EWF_UI_MUYU_TAPZONE_Y 265

/** 木鱼位图尺寸与热区内偏移（(55,283)-(32,265)）。 */
#define EWF_UI_MUYU_WOODFISH_W 300
#define EWF_UI_MUYU_WOODFISH_H 180
#define EWF_UI_MUYU_WOODFISH_DX 23
#define EWF_UI_MUYU_WOODFISH_DY 18

/* —— charging banner（HTML state-banner 96×20 @ (157,64)）—— */

/** 横幅几何与色（amber.400）。 */
#define EWF_UI_MUYU_CHARGING_W 96
#define EWF_UI_MUYU_CHARGING_H 20
#define EWF_UI_MUYU_CHARGING_X 157
#define EWF_UI_MUYU_CHARGING_Y 64

/** 充电横幅文案（MUYU.CHARGING_PAUSE 轻量提示，不使用弹窗）。 */
#define EWF_UI_MUYU_CHARGING_TEXT "充电中 · 暂停敲击"

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

/** 标题 290×32 @ (24,24) 居中。 */
#define EWF_UI_MUYU_MODAL_TITLE_W 290
#define EWF_UI_MUYU_MODAL_TITLE_H 32
#define EWF_UI_MUYU_MODAL_TITLE_X 24
#define EWF_UI_MUYU_MODAL_TITLE_Y 24

/** 摘要 290×24 @ (24,66) 居中。 */
#define EWF_UI_MUYU_MODAL_SUMMARY_W 290
#define EWF_UI_MUYU_MODAL_SUMMARY_H 24
#define EWF_UI_MUYU_MODAL_SUMMARY_X 24
#define EWF_UI_MUYU_MODAL_SUMMARY_Y 66

/** 动作 126×34 @ y=122：restart x=30、exit x=182，文字居中。 */
#define EWF_UI_MUYU_MODAL_BTN_W 126
#define EWF_UI_MUYU_MODAL_BTN_H 34
#define EWF_UI_MUYU_MODAL_BTN_Y 122
#define EWF_UI_MUYU_MODAL_BTN_RESTART_X 30
#define EWF_UI_MUYU_MODAL_BTN_EXIT_X 182

/** 文案容量（含 NUL）。 */
#define EWF_UI_MUYU_DONE_TITLE_CAP 16U
#define EWF_UI_MUYU_DONE_SUMMARY_CAP 48U

/** 产品文案闭包。 */
#define EWF_UI_MUYU_DONE_TITLE "本轮完成"
#define EWF_UI_MUYU_DONE_RESTART "从头开始"
#define EWF_UI_MUYU_DONE_EXIT "退出"

#endif /* EWF_UI_MUYU_CONSTANTS_H */
