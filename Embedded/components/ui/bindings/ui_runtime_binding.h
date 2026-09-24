/**
 * @file     ui_runtime_binding.h
 * @brief    EWF DEVICE-01 壳层运行时投影接口（Story 3.1）。
 * @details  裁决 C：仅由 ui_task 调用 LVGL。消费 watch_state 导航快照，
 *           驱动 pager 页索引与 SHEZHI 显隐；状态栏单槽默认 Wi-Fi/BLE/GPS=disabled。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_RUNTIME_BINDING_H
#define EWF_UI_RUNTIME_BINDING_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    EWF_UI_SIGNAL_CONNECTED = 0,
    EWF_UI_SIGNAL_NO_SIGNAL,
    EWF_UI_SIGNAL_DISABLED,
} ewf_ui_signal_state_t;

typedef enum {
    EWF_UI_SYNC_PHRASE_PENDING = 0, /**< 待同步（安全默认，禁止伪造成功）。 */
    EWF_UI_SYNC_PHRASE_BUSY,
    EWF_UI_SYNC_PHRASE_FAIL,
    EWF_UI_SYNC_PHRASE_OK, /**< 仅当 typed 事实明确成功时使用。 */
} ewf_ui_sync_phrase_t;

typedef struct {
    watch_nav_page_t active_page;
    bool settings_open;
    watch_screen_state_t screen_state;
    ewf_ui_signal_state_t signal_4g;
    ewf_ui_signal_state_t signal_wifi;
    ewf_ui_signal_state_t signal_ble;
    ewf_ui_signal_state_t signal_gps;
    ewf_ui_sync_phrase_t sync_phrase;
    int battery_percent; /**< <0 表示未知，显示 --%。 */
    bool display_on_request;
} ewf_ui_shell_model_t;

/**
 * @brief 初始化壳层对象引用（须在 ui_init 之后、仅 ui_task 调用）
 */
esp_err_t ui_runtime_binding_init(void);

/**
 * @brief 从 watch_state 快照构建壳层模型（纯逻辑，可主机测）
 */
void ui_runtime_binding_build_model(const watch_state_snapshot_t *snapshot,
                                    ewf_ui_shell_model_t *out);

/**
 * @brief 将模型投影到对象树（仅 ui_task）
 */
esp_err_t ui_runtime_binding_apply(const ewf_ui_shell_model_t *model);

/**
 * @brief 注入状态栏槽位（主机/调试；不伪造 OK）
 */
void ui_runtime_binding_inject_statusbar(ewf_ui_signal_state_t signal_4g,
                                         ewf_ui_sync_phrase_t sync_phrase,
                                         int battery_percent);

/**
 * @brief 读取最近一次注入/默认状态栏槽（可测）
 */
void ui_runtime_binding_get_statusbar_defaults(ewf_ui_signal_state_t *wifi,
                                               ewf_ui_signal_state_t *ble,
                                               ewf_ui_signal_state_t *gps);

/**
 * @brief 页枚举到 pager X 偏移
 */
int ui_runtime_binding_page_offset_x(watch_nav_page_t page);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_RUNTIME_BINDING_H */
