/**
 * @file     device_nav_policy.c
 * @brief    设备导航与熄亮屏纯逻辑实现。
 * @details  设置打开时短按先关设置；熄屏短按/首触只唤醒；超时只熄屏不关机。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_nav_policy.h"

#include <stddef.h>

static ewf_nav_decision_t make_decision(const ewf_nav_state_t *state,
                                        ewf_nav_reason_t reason,
                                        bool changed,
                                        bool wake_display,
                                        bool blank_display,
                                        bool reset_idle)
{
    const ewf_nav_decision_t decision = {
        .changed = changed,
        .active_page = state != NULL ? state->active_page : EWF_NAV_PAGE_MUYU,
        .settings_open = state != NULL && state->settings_open,
        .screen_on = state != NULL && state->screen_on,
        .wake_display = wake_display,
        .blank_display = blank_display,
        .reset_idle = reset_idle,
        .reason = reason,
    };
    return decision;
}

static uint32_t sanitize_timeout_s(uint32_t idle_timeout_s)
{
    if (idle_timeout_s == 5U || idle_timeout_s == 15U || idle_timeout_s == 30U) {
        return idle_timeout_s;
    }
    return 15U;
}

void ewf_nav_state_init(ewf_nav_state_t *state, uint32_t idle_timeout_s,
                        uint64_t now_ms)
{
    if (state == NULL) {
        return;
    }
    state->active_page = EWF_NAV_PAGE_MUYU;
    state->settings_open = false;
    state->screen_on = true;
    state->idle_timeout_s = sanitize_timeout_s(idle_timeout_s);
    state->last_activity_ms = now_ms;
}

ewf_nav_page_t ewf_nav_page_next(ewf_nav_page_t page)
{
    switch (page) {
    case EWF_NAV_PAGE_MUYU:
        return EWF_NAV_PAGE_JINGWEN;
    case EWF_NAV_PAGE_JINGWEN:
        return EWF_NAV_PAGE_TONGJI;
    case EWF_NAV_PAGE_TONGJI:
        return EWF_NAV_PAGE_MUYU;
    default:
        return EWF_NAV_PAGE_MUYU;
    }
}

ewf_nav_page_t ewf_nav_page_prev(ewf_nav_page_t page)
{
    switch (page) {
    case EWF_NAV_PAGE_MUYU:
        return EWF_NAV_PAGE_TONGJI;
    case EWF_NAV_PAGE_JINGWEN:
        return EWF_NAV_PAGE_MUYU;
    case EWF_NAV_PAGE_TONGJI:
        return EWF_NAV_PAGE_JINGWEN;
    default:
        return EWF_NAV_PAGE_MUYU;
    }
}

bool ewf_nav_pwr_is_short_press(uint32_t duration_ms)
{
    return duration_ms > EWF_PWR_SHORT_PRESS_MIN_MS &&
           duration_ms <= EWF_PWR_SHORT_PRESS_MAX_MS;
}

ewf_nav_input_kind_t ewf_nav_classify_gesture(int16_t x0, int16_t y0,
                                               int16_t x1, int16_t y1)
{
    const int dx = (int)x1 - (int)x0;
    const int dy = (int)y1 - (int)y0;
    const int adx = dx < 0 ? -dx : dx;
    const int ady = dy < 0 ? -dy : dy;

    if (adx < EWF_NAV_SWIPE_MIN_DX && ady < EWF_NAV_SWIPE_MIN_DY) {
        return EWF_NAV_INPUT_NONE;
    }
    if (adx >= ady * EWF_NAV_SWIPE_AXIS_RATIO && adx >= EWF_NAV_SWIPE_MIN_DX) {
        return dx < 0 ? EWF_NAV_INPUT_SWIPE_LEFT : EWF_NAV_INPUT_SWIPE_RIGHT;
    }
    if (ady >= adx * EWF_NAV_SWIPE_AXIS_RATIO && ady >= EWF_NAV_SWIPE_MIN_DY) {
        return dy > 0 ? EWF_NAV_INPUT_SWIPE_DOWN : EWF_NAV_INPUT_SWIPE_UP;
    }
    return EWF_NAV_INPUT_NONE;
}

ewf_nav_decision_t ewf_nav_policy_apply(ewf_nav_state_t *state,
                                        const ewf_nav_input_t *input)
{
    if (state == NULL || input == NULL) {
        return make_decision(state, EWF_NAV_REASON_IGNORED, false, false, false,
                             false);
    }

    switch (input->kind) {
    case EWF_NAV_INPUT_ACTIVITY:
        state->last_activity_ms = input->now_ms;
        return make_decision(state, EWF_NAV_REASON_ACTIVITY_RESET, true, false,
                             false, true);

    case EWF_NAV_INPUT_IDLE_TIMEOUT: {
        const uint64_t limit_ms =
            (uint64_t)state->idle_timeout_s * 1000ULL;
        if (!state->screen_on) {
            return make_decision(state, EWF_NAV_REASON_IGNORED, false, false,
                                 false, false);
        }
        /* tick 回绕时重置活动锚点，避免永远达不到超时。 */
        if (input->now_ms < state->last_activity_ms) {
            state->last_activity_ms = input->now_ms;
        }
        if (input->now_ms < state->last_activity_ms + limit_ms) {
            return make_decision(state, EWF_NAV_REASON_IGNORED, false, false,
                                 false, false);
        }
        state->screen_on = false;
        /* 熄屏时关闭设置浮层，避免亮屏后仍悬停在设置态。 */
        state->settings_open = false;
        return make_decision(state, EWF_NAV_REASON_IDLE_OFF, true, false, true,
                             false);
    }

    case EWF_NAV_INPUT_TOUCH_WAKE:
        if (state->screen_on) {
            return make_decision(state, EWF_NAV_REASON_IGNORED, false, false,
                                 false, false);
        }
        state->screen_on = true;
        state->last_activity_ms = input->now_ms;
        return make_decision(state, EWF_NAV_REASON_TOUCH_WAKE, true, true,
                             false, true);

    case EWF_NAV_INPUT_PWR_INTERVAL:
        if (input->duration_ms <= EWF_PWR_SHORT_PRESS_MIN_MS) {
            return make_decision(state, EWF_NAV_REASON_PWR_IGNORED_GLITCH, false,
                                 false, false, false);
        }
        if (input->duration_ms > EWF_PWR_SHORT_PRESS_MAX_MS) {
            /* 超长区间交给板级关机；固件不导航、不关机。 */
            return make_decision(state, EWF_NAV_REASON_PWR_IGNORED_LONG, false,
                                 false, false, false);
        }
        if (!state->screen_on) {
            state->screen_on = true;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_PWR_WAKE, true, true,
                                 false, true);
        }
        if (state->settings_open) {
            state->settings_open = false;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_PWR_CLOSE_SETTINGS, true,
                                 false, false, true);
        }
        state->active_page = ewf_nav_page_next(state->active_page);
        state->last_activity_ms = input->now_ms;
        return make_decision(state, EWF_NAV_REASON_PWR_CYCLE_PAGE, true, false,
                             false, true);

    case EWF_NAV_INPUT_SWIPE_LEFT:
    case EWF_NAV_INPUT_SWIPE_RIGHT:
        if (!state->screen_on) {
            state->screen_on = true;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_TOUCH_WAKE, true, true,
                                 false, true);
        }
        if (state->settings_open) {
            /* 设置打开时左右滑先关闭设置，与上滑返回一致。 */
            state->settings_open = false;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_CLOSE_SETTINGS, true,
                                 false, false, true);
        }
        state->active_page = input->kind == EWF_NAV_INPUT_SWIPE_LEFT
                                 ? ewf_nav_page_next(state->active_page)
                                 : ewf_nav_page_prev(state->active_page);
        state->last_activity_ms = input->now_ms;
        return make_decision(state, EWF_NAV_REASON_SWIPE_PAGE, true, false,
                             false, true);

    case EWF_NAV_INPUT_SWIPE_DOWN:
        if (!state->screen_on) {
            state->screen_on = true;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_TOUCH_WAKE, true, true,
                                 false, true);
        }
        if (!state->settings_open) {
            state->settings_open = true;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_OPEN_SETTINGS, true,
                                 false, false, true);
        }
        state->last_activity_ms = input->now_ms;
        return make_decision(state, EWF_NAV_REASON_ACTIVITY_RESET, true, false,
                             false, true);

    case EWF_NAV_INPUT_SWIPE_UP:
        if (!state->screen_on) {
            state->screen_on = true;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_TOUCH_WAKE, true, true,
                                 false, true);
        }
        if (state->settings_open) {
            state->settings_open = false;
            state->last_activity_ms = input->now_ms;
            return make_decision(state, EWF_NAV_REASON_CLOSE_SETTINGS, true,
                                 false, false, true);
        }
        state->last_activity_ms = input->now_ms;
        return make_decision(state, EWF_NAV_REASON_ACTIVITY_RESET, true, false,
                             false, true);

    default:
        return make_decision(state, EWF_NAV_REASON_IGNORED, false, false, false,
                             false);
    }
}
