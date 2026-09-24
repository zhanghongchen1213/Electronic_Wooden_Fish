/**
 * @file     test_device_nav_policy.c
 * @brief    设备导航纯逻辑主机测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_nav_policy.h"

#include <assert.h>
#include <stdio.h>

static void test_page_cycle_order(void)
{
    assert(ewf_nav_page_next(EWF_NAV_PAGE_MUYU) == EWF_NAV_PAGE_JINGWEN);
    assert(ewf_nav_page_next(EWF_NAV_PAGE_JINGWEN) == EWF_NAV_PAGE_TONGJI);
    assert(ewf_nav_page_next(EWF_NAV_PAGE_TONGJI) == EWF_NAV_PAGE_MUYU);
    assert(ewf_nav_page_prev(EWF_NAV_PAGE_MUYU) == EWF_NAV_PAGE_TONGJI);
}

static void test_pwr_short_cycle_and_settings(void)
{
    ewf_nav_state_t state;
    ewf_nav_state_init(&state, 15U, 0U);
    ewf_nav_input_t input = {.kind = EWF_NAV_INPUT_PWR_INTERVAL,
                             .duration_ms = 400U,
                             .now_ms = 100U};
    ewf_nav_decision_t d = ewf_nav_policy_apply(&state, &input);
    assert(d.changed);
    assert(state.active_page == EWF_NAV_PAGE_JINGWEN);
    assert(d.reason == EWF_NAV_REASON_PWR_CYCLE_PAGE);

    input.kind = EWF_NAV_INPUT_SWIPE_DOWN;
    input.now_ms = 200U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(state.settings_open);
    assert(d.reason == EWF_NAV_REASON_OPEN_SETTINGS);

    input.kind = EWF_NAV_INPUT_PWR_INTERVAL;
    input.duration_ms = 500U;
    input.now_ms = 300U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(!state.settings_open);
    assert(state.active_page == EWF_NAV_PAGE_JINGWEN);
    assert(d.reason == EWF_NAV_REASON_PWR_CLOSE_SETTINGS);
}

static void test_pwr_wake_and_ignore_long(void)
{
    ewf_nav_state_t state;
    ewf_nav_state_init(&state, 15U, 0U);
    state.screen_on = false;
    ewf_nav_input_t input = {.kind = EWF_NAV_INPUT_PWR_INTERVAL,
                             .duration_ms = 300U,
                             .now_ms = 10U};
    ewf_nav_decision_t d = ewf_nav_policy_apply(&state, &input);
    assert(state.screen_on);
    assert(d.wake_display);
    assert(d.reason == EWF_NAV_REASON_PWR_WAKE);

    input.duration_ms = 2000U;
    input.now_ms = 20U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(!d.changed);
    assert(d.reason == EWF_NAV_REASON_PWR_IGNORED_LONG);

    input.duration_ms = 10U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(d.reason == EWF_NAV_REASON_PWR_IGNORED_GLITCH);
}

static void test_touch_wake_only(void)
{
    ewf_nav_state_t state;
    ewf_nav_state_init(&state, 15U, 0U);
    state.screen_on = false;
    ewf_nav_input_t input = {.kind = EWF_NAV_INPUT_TOUCH_WAKE, .now_ms = 5U};
    ewf_nav_decision_t d = ewf_nav_policy_apply(&state, &input);
    assert(state.screen_on);
    assert(d.wake_display);
    assert(d.reason == EWF_NAV_REASON_TOUCH_WAKE);
}

static void test_idle_timeout(void)
{
    ewf_nav_state_t state;
    ewf_nav_state_init(&state, 5U, 0U);
    ewf_nav_input_t input = {.kind = EWF_NAV_INPUT_IDLE_TIMEOUT, .now_ms = 4999U};
    ewf_nav_decision_t d = ewf_nav_policy_apply(&state, &input);
    assert(!d.changed);
    assert(state.screen_on);

    input.now_ms = 5000U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(d.blank_display);
    assert(!state.screen_on);
    assert(d.reason == EWF_NAV_REASON_IDLE_OFF);

    /* 设置打开时超时熄屏须关闭设置浮层。 */
    ewf_nav_state_init(&state, 5U, 0U);
    input.kind = EWF_NAV_INPUT_SWIPE_DOWN;
    input.now_ms = 10U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(state.settings_open);
    input.kind = EWF_NAV_INPUT_IDLE_TIMEOUT;
    input.now_ms = 5010U;
    d = ewf_nav_policy_apply(&state, &input);
    assert(d.blank_display);
    assert(!state.screen_on);
    assert(!state.settings_open);
}

static void test_gesture_classify(void)
{
    assert(ewf_nav_classify_gesture(100, 100, 20, 105) ==
           EWF_NAV_INPUT_SWIPE_LEFT);
    assert(ewf_nav_classify_gesture(100, 100, 200, 105) ==
           EWF_NAV_INPUT_SWIPE_RIGHT);
    assert(ewf_nav_classify_gesture(100, 100, 105, 200) ==
           EWF_NAV_INPUT_SWIPE_DOWN);
    assert(ewf_nav_classify_gesture(100, 200, 105, 100) ==
           EWF_NAV_INPUT_SWIPE_UP);
    assert(ewf_nav_classify_gesture(100, 100, 110, 110) == EWF_NAV_INPUT_NONE);
}

static void test_pwr_short_helper(void)
{
    assert(!ewf_nav_pwr_is_short_press(10U));
    assert(ewf_nav_pwr_is_short_press(21U));
    assert(ewf_nav_pwr_is_short_press(800U));
    assert(!ewf_nav_pwr_is_short_press(801U));
}

int main(void)
{
    test_page_cycle_order();
    test_pwr_short_cycle_and_settings();
    test_pwr_wake_and_ignore_long();
    test_touch_wake_only();
    test_idle_timeout();
    test_gesture_classify();
    test_pwr_short_helper();
    printf("PASS test_device_nav_policy\n");
    return 0;
}
