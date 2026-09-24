/**
 * @file     ewf_ui_geometry.h
 * @brief    DEVICE-01 画布与壳层几何常量（Story 3.1/3.2 裁决入口）。
 * @details  裁决 A：UX 路径以 Embedded/lvgl-design/ 为准。
 *           裁决 B：watch-lvgl 外骨骼产物为遗留，由 ewf-device 替换。
 *           裁决 C：ui_task 为唯一 LVGL 调用方。
 *           裁决 D：字体子集 = canonical 可消费汉字 ∪ UI 短文案 ∪ ASCII/数字/标点。
 *           裁决 E：Wi-Fi/BLE/GPS 槽位默认 disabled。
 *           裁决 F：CHARGING_PAUSE 不接线为暂停 gate。
 *           裁决 G：本 Story 验收=骨架+字体合同脚本+门禁，不等于 15 态终验。
 *           Story 3.2 木鱼页裁决见 ui_muyu_constants.h（A–H）。
 *           Story 3.3 经文页裁决见 ui_jingwen_constants.h（A–I）：只消费
 *           progress/nav 快照与 API，不重写累计/游标 owner；不改 3.2 木鱼触区。
 *           Story 3.4 统计页裁决见 ui_tongji_constants.h（A–J）：双卡+reading-progress；
 *           今日桶 owner 在 progress 邻接；UI 只投影；触摸不计数。
 *           本模块只消费 device_nav_service 的 active_page/settings_open/screen_state，
 *           不重写导航状态机；不重写 progress 游标 owner。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_GEOMETRY_H
#define EWF_UI_GEOMETRY_H

/** 画布宽度（像素）。 */
#define EWF_UI_CANVAS_W 410
/** 画布高度（像素）。 */
#define EWF_UI_CANVAS_H 502
/** 画布圆角半径。 */
#define EWF_UI_CORNER_RADIUS 110
/** 屏幕边距。 */
#define EWF_UI_MARGIN_PX 16
/** 关键内容净空。 */
#define EWF_UI_CRITICAL_CLEARANCE_PX 8
/** 状态栏高度。 */
#define EWF_UI_STATUSBAR_H 24
/** 横向 pager 内容总宽（三主页）。 */
#define EWF_UI_PAGER_CONTENT_W 1230
/** MUYU 根帧 X。 */
#define EWF_UI_PAGE_MUYU_X 0
/** JINGWEN 根帧 X。 */
#define EWF_UI_PAGE_JINGWEN_X 410
/** TONGJI 根帧 X。 */
#define EWF_UI_PAGE_TONGJI_X 820

#endif /* EWF_UI_GEOMETRY_H */
