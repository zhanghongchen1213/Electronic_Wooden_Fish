/**
 * @file     ui_events.c
 * @brief    SquareLine 事件空桩：仅转发到 bindings intent。
 * @details  禁止在 generated/ 读取 NVS 或服务私有状态（Story 3.1 / AD-10）。
 * @author   ZHC
 * @date     2026-09-24
 */
#include "ui.h"
#include "ui_runtime_events.h"

void ewf_ui_event_forward(lv_event_t * e)
{
    ui_runtime_events_on_generated(e);
}
