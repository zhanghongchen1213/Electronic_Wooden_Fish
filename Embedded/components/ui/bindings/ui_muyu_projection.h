/**
 * @file     ui_muyu_projection.h
 * @brief    木鱼页 LVGL 投影：字带、进度、三环（仅 ui_task）。
 * @details  device_touch 提交由 ui_service 注入回调，避免 ui↔services 循环依赖。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_MUYU_PROJECTION_H
#define EWF_UI_MUYU_PROJECTION_H

#include "app_state.h"
#include "esp_err.h"
#include "ui_tap_rings_policy.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ui_muyu_woodfish_click_fn)(void);

/**
 * @brief 初始化木鱼页对象引用与三环 timer（须在 ui_init 之后）
 * @param on_woodfish_click 木鱼热区点击回调（可为 NULL）
 */
esp_err_t ui_muyu_projection_init(ui_muyu_woodfish_click_fn on_woodfish_click);

/**
 * @brief 从 watch_state 快照投影字带/进度，并按游标变化触发三环
 */
void ui_muyu_projection_apply(const watch_state_snapshot_t *snapshot,
                              ewf_ui_tap_origin_t last_tap_origin,
                              uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_MUYU_PROJECTION_H */
