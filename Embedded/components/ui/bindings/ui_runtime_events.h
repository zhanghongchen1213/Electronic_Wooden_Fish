/**
 * @file     ui_runtime_events.h
 * @brief    generated 事件转发到 bindings intent。
 * @details  Story 3.5：设置控件 typed intent 经回调交给 ui_service → device_nav。
 *           Story 3.6：完成遮罩 restart/exit → ui_service → progress_service。
 *           UI 组件不直接依赖 services（避免循环 REQUIRES）。
 * @author   ZHC
 * @date     2026-09-24
 */
#ifndef EWF_UI_RUNTIME_EVENTS_H
#define EWF_UI_RUNTIME_EVENTS_H

#include <stdint.h>

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 设置页控件事件码（经 lv_event user_data 传递）。 */
typedef enum {
    EWF_UI_SHEZHI_EVT_NONE = 0,
    EWF_UI_SHEZHI_EVT_BRIGHTNESS_LOW = 1,
    EWF_UI_SHEZHI_EVT_BRIGHTNESS_MID = 2,
    EWF_UI_SHEZHI_EVT_BRIGHTNESS_HIGH = 3,
    EWF_UI_SHEZHI_EVT_TIMEOUT_5 = 4,
    EWF_UI_SHEZHI_EVT_TIMEOUT_15 = 5,
    EWF_UI_SHEZHI_EVT_TIMEOUT_30 = 6,
    EWF_UI_SHEZHI_EVT_VOLUME_CHANGED = 7,
    EWF_UI_SHEZHI_EVT_SYNC_CLICK = 8,
} ewf_ui_shezhi_event_t;

/** 木鱼完成遮罩事件码（与设置页码段分离，避免误路由）。 */
typedef enum {
    EWF_UI_MUYU_DONE_EVT_NONE = 0,
    EWF_UI_MUYU_DONE_EVT_RESTART = 100,
    EWF_UI_MUYU_DONE_EVT_EXIT = 101,
} ewf_ui_muyu_done_event_t;

/** 设置 intent 回调（由 ui_service 注册）。 */
typedef void (*ui_shezhi_intent_fn)(ewf_ui_shezhi_event_t code, int32_t value);

/** 完成遮罩 intent 回调（由 ui_service 注册）。 */
typedef void (*ui_muyu_done_intent_fn)(ewf_ui_muyu_done_event_t code);

/**
 * @brief 注册设置控件 intent 处理器（仅 ui_task / ui_service）
 */
void ui_runtime_events_set_shezhi_handler(ui_shezhi_intent_fn handler);

/**
 * @brief 注册完成遮罩 intent 处理器（仅 ui_task / ui_service）
 */
void ui_runtime_events_set_muyu_done_handler(ui_muyu_done_intent_fn handler);

/**
 * @brief generated 空桩/设置/遮罩控件统一入口（不提交 device_touch）
 */
void ui_runtime_events_on_generated(lv_event_t *e);

#ifdef __cplusplus
}
#endif
#endif
