/**
 * @file     ui_shezhi_projection.h
 * @brief    设置页 LVGL 投影（仅 ui_task）。
 * @details  裁决 B/D/G：只读 nav/settings/identity；五态只改 action；
 *           无 woodfish/tap-rings；设置触摸不提交 device_touch。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_SHEZHI_PROJECTION_H
#define EWF_UI_SHEZHI_PROJECTION_H

#include "app_state.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化设置页投影脏标记（须在 ui_init 之后）
 */
esp_err_t ui_shezhi_projection_init(void);

/**
 * @brief 设置屏销毁后失效脏标记（下次打开强制重投影）
 */
void ui_shezhi_projection_invalidate(void);

/**
 * @brief 投影是否正在写 LVGL（抑制滑块 VALUE_CHANGED 回写）
 */
bool ui_shezhi_projection_is_applying(void);

/**
 * @brief 设置打开时投影控件态；无变化跳过全量刷
 * @param snapshot 导航/设置快照
 * @param firmware_version 只读固件版本；可为 NULL
 * @param device_id 只读设备 ID；可为 NULL
 * @param identity_configured 身份是否已配置（未配置用安全占位）
 */
void ui_shezhi_projection_apply(const watch_state_snapshot_t *snapshot,
                                const char *firmware_version,
                                const char *device_id,
                                bool identity_configured);

/**
 * @brief 可测滚动偏移注入（真机 lv_indev 合流仍 hardware_pending）
 * @param scroll_y 内容向下滚动的像素（负值钳制为 0）
 */
void ui_shezhi_projection_set_scroll_y(int16_t scroll_y);

/**
 * @brief 读取当前内容 scroll Y（可测）
 */
int16_t ui_shezhi_projection_get_scroll_y(void);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_SHEZHI_PROJECTION_H */
