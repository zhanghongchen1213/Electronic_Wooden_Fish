/**
 * @file     ui_jingwen_projection.h
 * @brief    经文页 scripture-history LVGL 投影（仅 ui_task）。
 * @details  裁决 C/E：只读 snapshot；新字 append + 强制锚尾；无 woodfish。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_JINGWEN_PROJECTION_H
#define EWF_UI_JINGWEN_PROJECTION_H

#include "app_state.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化经文页投影状态
 */
esp_err_t ui_jingwen_projection_init(void);

/**
 * @brief 按快照投影经文流与进度；仅 ui_task 调用
 * @param snapshot 只读 watch_state 快照；NULL 时按空流投影
 */
void ui_jingwen_projection_apply(const watch_state_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_JINGWEN_PROJECTION_H */
