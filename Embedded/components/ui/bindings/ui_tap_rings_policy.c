/**
 * @file     ui_tap_rings_policy.c
 * @brief    三环 flash 与木鱼触区 gate 纯逻辑实现。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "ui_tap_rings_policy.h"

#include <stddef.h>

bool ewf_ui_tap_rings_should_flash(ewf_ui_tap_origin_t origin,
                                   const ewf_ui_tap_rings_gate_t *gate)
{
    if (gate == NULL) {
        return false;
    }
    if (!gate->screen_on || !gate->active_page_muyu) {
        return false;
    }
    if (gate->pending_completion || gate->fault_locked) {
        return false;
    }
    return origin == EWF_UI_TAP_ORIGIN_PHYSICAL_PVDF ||
           origin == EWF_UI_TAP_ORIGIN_DEVICE_TOUCH;
}

bool ewf_ui_muyu_touch_may_submit(const ewf_ui_tap_rings_gate_t *gate)
{
    if (gate == NULL) {
        return false;
    }
    return gate->screen_on && gate->active_page_muyu &&
           !gate->pending_completion && !gate->fault_locked;
}

bool ewf_ui_tap_rings_on_event(ewf_ui_tap_rings_state_t *state,
                               uint32_t now_ms,
                               ewf_ui_tap_origin_t origin,
                               const ewf_ui_tap_rings_gate_t *gate)
{
    if (state == NULL || !ewf_ui_tap_rings_should_flash(origin, gate)) {
        return false;
    }
    state->flashing = true;
    state->flash_deadline_ms = now_ms + EWF_UI_MUYU_TAP_FLASH_MS;
    return true;
}

bool ewf_ui_tap_rings_on_tick(ewf_ui_tap_rings_state_t *state, uint32_t now_ms)
{
    if (state == NULL || !state->flashing) {
        return false;
    }
    if (now_ms < state->flash_deadline_ms) {
        return false;
    }
    state->flashing = false;
    state->flash_deadline_ms = 0U;
    return true;
}
