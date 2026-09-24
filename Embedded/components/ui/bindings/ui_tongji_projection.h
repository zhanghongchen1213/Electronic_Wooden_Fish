/**
 * @file     ui_tongji_projection.h
 * @brief    统计页 LVGL 投影（仅 ui_task）。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_TONGJI_PROJECTION_H
#define EWF_UI_TONGJI_PROJECTION_H

#include "app_state.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化统计页投影脏标记（须在 ui_init 之后）
 */
esp_err_t ui_tongji_projection_init(void);

/**
 * @brief 从快照投影今日/累计/reading-progress；无变化时跳过全量刷
 */
void ui_tongji_projection_apply(const watch_state_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_TONGJI_PROJECTION_H */
