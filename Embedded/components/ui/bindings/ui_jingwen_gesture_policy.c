/**
 * @file     ui_jingwen_gesture_policy.c
 * @brief    经文页 history 视口手势命中与垂直导航抑制。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_jingwen_gesture_policy.h"

bool ewf_ui_jingwen_point_in_history(int16_t x, int16_t y)
{
    const int16_t x0 = (int16_t)EWF_UI_JINGWEN_HISTORY_X;
    const int16_t y0 = (int16_t)EWF_UI_JINGWEN_HISTORY_Y;
    const int16_t x1 = (int16_t)(EWF_UI_JINGWEN_HISTORY_X + EWF_UI_JINGWEN_HISTORY_W);
    const int16_t y1 = (int16_t)(EWF_UI_JINGWEN_HISTORY_Y + EWF_UI_JINGWEN_HISTORY_H);
    return x >= x0 && x < x1 && y >= y0 && y < y1;
}

bool ewf_ui_jingwen_suppress_vertical_nav(bool page_is_jingwen,
                                          bool settings_open,
                                          int16_t gesture_start_x,
                                          int16_t gesture_start_y,
                                          bool is_vertical_swipe)
{
    if (!page_is_jingwen || settings_open || !is_vertical_swipe) {
        return false;
    }
    return ewf_ui_jingwen_point_in_history(gesture_start_x, gesture_start_y);
}
