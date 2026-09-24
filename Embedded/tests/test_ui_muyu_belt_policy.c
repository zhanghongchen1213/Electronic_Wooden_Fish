#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ui_muyu_belt_policy.h"
#include "ui_tap_rings_policy.h"

static void expect_slot(const ewf_ui_muyu_belt_view_t *view, int index, const char *text, bool filled)
{
    assert(strcmp(view->slots[index], text) == 0);
    assert(view->filled[index] == filled);
}

static void test_belt_empty(void)
{
    ewf_ui_muyu_belt_view_t view;
    ewf_ui_muyu_belt_project(0U, &view);
    assert(view.glyph_current_index == -1);
    assert(view.display_char_count == 0U);
    assert(view.progress_percent == 0U);
    assert(strstr(view.progress_text, "0 / 260") != NULL);
    for (int i = 0; i < 7; ++i) {
        expect_slot(&view, i, "·", false);
    }
}

static void test_belt_one(void)
{
    ewf_ui_muyu_belt_view_t view;
    ewf_ui_muyu_belt_project(1U, &view);
    assert(view.display_char_count == 1U);
    assert(view.glyph_current_index == 6);
    expect_slot(&view, 6, "观", true);
    for (int i = 0; i < 6; ++i) {
        expect_slot(&view, i, "·", false);
    }
}

static void test_belt_full_and_shift(void)
{
    ewf_ui_muyu_belt_view_t seven;
    ewf_ui_muyu_belt_project(7U, &seven);
    assert(seven.display_char_count >= 7U);
    assert(seven.glyph_current_index == 6);
    for (int i = 0; i < 7; ++i) {
        assert(seven.filled[i]);
    }

    ewf_ui_muyu_belt_view_t eight;
    ewf_ui_muyu_belt_project(8U, &eight);
    assert(eight.glyph_current_index == 6);
    assert(eight.display_char_count > seven.display_char_count);
    assert(eight.filled[0]);
    /* 第 8 字进入后：右侧最新槽变化，且左槽等于前一窗的次左槽（真正左移）。 */
    assert(strcmp(eight.slots[6], seven.slots[6]) != 0);
    assert(strcmp(eight.slots[0], seven.slots[1]) == 0);
}

static void test_belt_punctuation_step(void)
{
    /* 步「萨，」：第 5 个可消费字后带标点，展开为两槽。 */
    ewf_ui_muyu_belt_view_t view;
    ewf_ui_muyu_belt_project(5U, &view);
    assert(view.display_char_count == 6U);
    expect_slot(&view, 5, "萨", true);
    expect_slot(&view, 6, "，", true);
    assert(view.glyph_current_index == 6);
}

static void test_progress_full(void)
{
    assert(ewf_ui_muyu_progress_percent(0U) == 0U);
    assert(ewf_ui_muyu_progress_percent(130U) == 50U);
    assert(ewf_ui_muyu_progress_percent(259U) == 99U);
    assert(ewf_ui_muyu_progress_percent(260U) == 100U);
    assert(ewf_ui_muyu_progress_percent(999U) == 100U);

    ewf_ui_muyu_belt_view_t view;
    ewf_ui_muyu_belt_project(260U, &view);
    assert(view.progress_percent == 100U);
    assert(strstr(view.progress_text, "260 / 260") != NULL);
    assert(strstr(view.progress_text, "100%") != NULL);
}

static void test_rings_origin_and_coalesce(void)
{
    ewf_ui_tap_rings_gate_t gate = {
        .pending_completion = false,
        .fault_locked = false,
        .screen_on = true,
        .active_page_muyu = true,
    };
    assert(ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_PHYSICAL_PVDF, &gate));
    assert(ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_DEVICE_TOUCH, &gate));
    assert(!ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_AUTOMATIC_TAP, &gate));

    gate.fault_locked = true;
    assert(!ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_DEVICE_TOUCH, &gate));
    gate.fault_locked = false;
    gate.pending_completion = true;
    assert(!ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_DEVICE_TOUCH, &gate));
    gate.pending_completion = false;
    gate.screen_on = false;
    assert(!ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_DEVICE_TOUCH, &gate));
    gate.screen_on = true;
    gate.active_page_muyu = false;
    assert(!ewf_ui_muyu_touch_may_submit(&gate));
    assert(!ewf_ui_tap_rings_should_flash(EWF_UI_TAP_ORIGIN_DEVICE_TOUCH, &gate));
    gate.active_page_muyu = true;
    assert(ewf_ui_muyu_touch_may_submit(&gate));

    ewf_ui_tap_rings_state_t state = {0};
    assert(ewf_ui_tap_rings_on_event(&state, 1000U, EWF_UI_TAP_ORIGIN_DEVICE_TOUCH, &gate));
    assert(state.flashing);
    assert(state.flash_deadline_ms == 1160U);
    /* 新事件重启截止，不叠加。 */
    assert(ewf_ui_tap_rings_on_event(&state, 1100U, EWF_UI_TAP_ORIGIN_PHYSICAL_PVDF, &gate));
    assert(state.flash_deadline_ms == 1260U);
    assert(!ewf_ui_tap_rings_on_tick(&state, 1200U));
    assert(ewf_ui_tap_rings_on_tick(&state, 1260U));
    assert(!state.flashing);

    /* automatic 不 flash */
    assert(!ewf_ui_tap_rings_on_event(&state, 2000U, EWF_UI_TAP_ORIGIN_AUTOMATIC_TAP, &gate));
}

int main(void)
{
    test_belt_empty();
    test_belt_one();
    test_belt_full_and_shift();
    test_belt_punctuation_step();
    test_progress_full();
    test_rings_origin_and_coalesce();
    puts("PASS test_ui_muyu_belt_policy");
    return 0;
}
