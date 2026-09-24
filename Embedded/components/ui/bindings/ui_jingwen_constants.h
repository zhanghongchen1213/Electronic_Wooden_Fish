/**
 * @file     ui_jingwen_constants.h
 * @brief    经文页 13 槽 append-only 流常量（Story 3.3 裁决入口）。
 * @details  裁决 A–I：填充单位=显示字符槽；布局=全文 13 槽换行非 7 槽尾窗；
 *           只读 round_cursor；无 woodfish/tap-rings；新字强制锚尾；
 *           进度复用 260 分母；手势 history 内滚 / 外下滑设置；
 *           CHARGING_PAUSE/统计卡/设置五态/完成遮罩不实现。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_JINGWEN_CONSTANTS_H
#define EWF_UI_JINGWEN_CONSTANTS_H

#include "ui_muyu_constants.h"

/** 经文流每行固定槽位数（视口行宽，不是经文上限）。 */
#define EWF_UI_JINGWEN_ROW_SLOTS 13U

/** 显示流最大字符数（canonical totalChars=303）。 */
#define EWF_UI_JINGWEN_DISPLAY_CAP 303U

/** 最大行数：ceil(303/13)=24。 */
#define EWF_UI_JINGWEN_MAX_ROWS 24U

/** scripture-history 几何（对齐 HTML：362×286 @ left=24,top=102）。 */
#define EWF_UI_JINGWEN_HISTORY_X 24
#define EWF_UI_JINGWEN_HISTORY_Y 102
#define EWF_UI_JINGWEN_HISTORY_W 362
#define EWF_UI_JINGWEN_HISTORY_H 286

/** history 面底色 #121212。 */
#define EWF_UI_JINGWEN_HISTORY_BG_HEX 0x121212U

/** history 圆角约 18。 */
#define EWF_UI_JINGWEN_HISTORY_RADIUS 18

/** 已确认行正文色（HTML #a6a29a）。 */
#define EWF_UI_JINGWEN_CONFIRMED_COLOR_HEX 0xA6A29AU

/** scripture-progress 约 top=410。 */
#define EWF_UI_JINGWEN_PROGRESS_Y 410

/** 行高：须容纳流尾 glyph-current（Serif 52/700），正文 26 亦落在此行盒内。 */
#define EWF_UI_JINGWEN_ROW_HEIGHT 56

/** 槽宽（13 槽塞入 history 内容宽，净空 ≥8）。 */
#define EWF_UI_JINGWEN_SLOT_W 24
#define EWF_UI_JINGWEN_SLOT_GAP 2

#endif /* EWF_UI_JINGWEN_CONSTANTS_H */
