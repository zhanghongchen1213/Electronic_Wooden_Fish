/**
 * @file     ui_runtime_events.c
 * @brief    generated 事件转发：设置/完成遮罩 → 已注册 intent 回调。
 * @details  裁决 G：设置页触摸不提交 device_touch；遮罩按钮只转发跨轮 intent。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_runtime_events.h"

#include "ui_shezhi_projection.h"

static ui_shezhi_intent_fn s_shezhi_handler;
static ui_muyu_done_intent_fn s_muyu_done_handler;

void ui_runtime_events_set_shezhi_handler(ui_shezhi_intent_fn handler)
{
    s_shezhi_handler = handler;
}

void ui_runtime_events_set_muyu_done_handler(ui_muyu_done_intent_fn handler)
{
    s_muyu_done_handler = handler;
}

void ui_runtime_events_on_generated(lv_event_t *e)
{
    if (e == NULL) {
        return;
    }

    const uintptr_t code = (uintptr_t)lv_event_get_user_data(e);
    const lv_event_code_t ev = lv_event_get_code(e);

    if (code == (uintptr_t)EWF_UI_MUYU_DONE_EVT_RESTART ||
        code == (uintptr_t)EWF_UI_MUYU_DONE_EVT_EXIT) {
        if (ev != LV_EVENT_CLICKED || s_muyu_done_handler == NULL) {
            return;
        }
        s_muyu_done_handler((ewf_ui_muyu_done_event_t)code);
        return;
    }

    if (s_shezhi_handler == NULL) {
        return;
    }

    if (code == (uintptr_t)EWF_UI_SHEZHI_EVT_VOLUME_CHANGED &&
        ev == LV_EVENT_VALUE_CHANGED) {
        /* 投影写回滑块时 LVGL 仍会发 VALUE_CHANGED，不得二次落盘。 */
        if (ui_shezhi_projection_is_applying()) {
            return;
        }
        lv_obj_t *target = lv_event_get_target(e);
        if (target == NULL) {
            return;
        }
        s_shezhi_handler(EWF_UI_SHEZHI_EVT_VOLUME_CHANGED,
                         lv_slider_get_value(target));
        return;
    }

    if (ev != LV_EVENT_CLICKED) {
        return;
    }
    if (code == (uintptr_t)EWF_UI_SHEZHI_EVT_NONE) {
        return;
    }
    s_shezhi_handler((ewf_ui_shezhi_event_t)code, 0);
}
