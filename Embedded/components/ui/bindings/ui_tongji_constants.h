/**
 * @file     ui_tongji_constants.h
 * @brief    统计页几何与色值常量（Story 3.4 裁决入口）。
 * @details  裁决 A–J：两张同构 stat-card + reading-progress；不含七日/三十日/连续天数字段；
 *           今日桶只投影；累计=local_total；进度复用 260 分母；触摸不计数；
 *           CHARGING_PAUSE/设置五态/完成遮罩不实现。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_TONGJI_CONSTANTS_H
#define EWF_UI_TONGJI_CONSTANTS_H

#include "ui_muyu_constants.h"

/** 顶部分割线。 */
#define EWF_UI_TONGJI_DIVIDER_X 16
#define EWF_UI_TONGJI_DIVIDER_Y 99
#define EWF_UI_TONGJI_DIVIDER_W 378
#define EWF_UI_TONGJI_DIVIDER_H 2

/** 今日/累计卡几何。 */
#define EWF_UI_TONGJI_CARD_X 16
#define EWF_UI_TONGJI_CARD_W 378
#define EWF_UI_TONGJI_CARD_H 64
#define EWF_UI_TONGJI_TODAY_Y 112
#define EWF_UI_TONGJI_TOTAL_Y 184

/** reading-progress 卡。 */
#define EWF_UI_TONGJI_PROGRESS_Y 264
#define EWF_UI_TONGJI_PROGRESS_H 176

/** 环几何（相对 progress 卡内）。 */
#define EWF_UI_TONGJI_RING_X 28
#define EWF_UI_TONGJI_RING_Y 28
#define EWF_UI_TONGJI_RING_SIZE 120

/** 卡圆角 / 底色 / 琥珀刻线。 */
#define EWF_UI_TONGJI_CARD_RADIUS 18
#define EWF_UI_TONGJI_CARD_BG_HEX 0x121212U
#define EWF_UI_TONGJI_ACCENT_HEX 0xD9A441U
#define EWF_UI_TONGJI_MUTED_HEX 0xA6A29AU
#define EWF_UI_TONGJI_UNIT_HEX 0xCFC4B0U
#define EWF_UI_TONGJI_VALUE_HEX 0xFBFAF0U
#define EWF_UI_TONGJI_DIVIDER_HEX 0x30343BU
#define EWF_UI_TONGJI_RING_TRACK_HEX 0x252525U

/** 进度分母（复用 consumable）。 */
#define EWF_UI_TONGJI_PROGRESS_DENOM EWF_UI_MUYU_PROGRESS_DENOM

#endif /* EWF_UI_TONGJI_CONSTANTS_H */
