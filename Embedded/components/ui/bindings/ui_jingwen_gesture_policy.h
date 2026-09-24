/**
 * @file     ui_jingwen_gesture_policy.h
 * @brief    经文页手势与导航共存策略（零 ESP-IDF）。
 * @details  裁决 H：history 视口内垂直拖动=内容滚动；视口外下滑=开设置；
 *           左右滑仍换页。经文页点击不产生 device_touch。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_JINGWEN_GESTURE_POLICY_H
#define EWF_UI_JINGWEN_GESTURE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "ui_jingwen_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 点是否落在 scripture-history 视口内（画布坐标）
 */
bool ewf_ui_jingwen_point_in_history(int16_t x, int16_t y);

/**
 * @brief 是否应抑制垂直导航手势（下滑开设置 / 上滑关设置）
 * @param page_is_jingwen 当前主页是否为经文页
 * @param settings_open 设置是否已打开（打开时不抑制，以便上滑关闭）
 * @param gesture_start_x 手势起点 X
 * @param gesture_start_y 手势起点 Y
 * @param is_vertical_swipe 是否为垂直滑动（SWIPE_UP/DOWN）
 * @return true=吞掉垂直导航，交给 LVGL 滚动
 */
bool ewf_ui_jingwen_suppress_vertical_nav(bool page_is_jingwen,
                                          bool settings_open,
                                          int16_t gesture_start_x,
                                          int16_t gesture_start_y,
                                          bool is_vertical_swipe);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_JINGWEN_GESTURE_POLICY_H */
