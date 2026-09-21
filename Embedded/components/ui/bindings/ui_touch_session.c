/**
 * @file     ui_touch_session.c
 * @brief    连续触摸采样与单序列手势仲裁实现
 * @details  对有效零触点做两帧释放确认，对短暂读取空洞保持上一稳定按压，并为点击和导航提供统一事实。
 * @author   ZHC
 * @date     2026-07-28
 */

#include "ui_touch_session.h"

#include <limits.h>
#include <string.h>

/** 最近一个触摸序列快照。 */
static ui_touch_snapshot_t s_snapshot;
/** 是否已收到过至少一个有效按压序列。 */
static bool s_snapshot_valid;
/** 本轮启动后的触摸序号。 */
static uint32_t s_next_sequence_id;
/** 最近一次有效控制器采样的单调毫秒时间。 */
static uint64_t s_last_valid_ms;
/** 当前触摸序列的按下单调毫秒时间。 */
static uint64_t s_started_ms;
/** 当前按压期间连续收到的有效零触点数量。 */
static uint8_t s_release_sample_count;

static uint16_t absolute_delta(int16_t value, int16_t origin);
static uint32_t bounded_duration_ms(uint64_t started_ms, uint64_t ended_ms);
static ui_touch_output_t current_output(void);
static void begin_session(int16_t x, int16_t y, uint64_t now_ms);
static void update_pressed(int16_t x, int16_t y, uint64_t now_ms);
static void finish_session(uint64_t now_ms, bool cancelled);

void ui_touch_session_reset(void)
{
    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot_valid = false;
    s_next_sequence_id = 0U;
    s_last_valid_ms = 0U;
    s_started_ms = 0U;
    s_release_sample_count = 0U;
}

ui_touch_output_t ui_touch_session_feed(ui_touch_sample_t sample,
                                        int16_t x,
                                        int16_t y,
                                        uint64_t now_ms)
{
    ui_touch_output_t output = current_output();
    switch (sample)
    {
    case UI_TOUCH_SAMPLE_PRESSED:
        if (!s_snapshot.active)
        {
            begin_session(x, y, now_ms);
            output = current_output();
            output.sequence_started = true;
            return output;
        }
        update_pressed(x, y, now_ms);
        return current_output();

    case UI_TOUCH_SAMPLE_RELEASED:
        if (!s_snapshot.active)
        {
            return output;
        }
        s_last_valid_ms = now_ms;
        ++s_snapshot.valid_sample_count;
        if (s_release_sample_count < UINT8_MAX)
        {
            ++s_release_sample_count;
        }
        if (s_release_sample_count < UI_TOUCH_RELEASE_CONFIRM_SAMPLES)
        {
            return current_output();
        }
        finish_session(now_ms, false);
        output = current_output();
        output.sequence_finished = true;
        return output;

    case UI_TOUCH_SAMPLE_UNKNOWN:
        if (!s_snapshot.active)
        {
            return output;
        }
        ++s_snapshot.unknown_sample_count;
        if (now_ms < s_last_valid_ms ||
            now_ms - s_last_valid_ms < UI_TOUCH_UNKNOWN_HOLD_MS)
        {
            return current_output();
        }
        finish_session(now_ms, true);
        output = current_output();
        output.sequence_finished = true;
        output.cancelled = true;
        return output;

    default:
        return output;
    }
}

bool ui_touch_session_snapshot(ui_touch_snapshot_t *snapshot)
{
    if (snapshot == NULL || !s_snapshot_valid)
    {
        return false;
    }
    *snapshot = s_snapshot;
    return true;
}

bool ui_touch_session_button_tap_allowed(void)
{
    if (!s_snapshot_valid)
    {
        return true;
    }
    return !s_snapshot.active && !s_snapshot.dragged &&
           !s_snapshot.cancelled && !s_snapshot.navigation_committed;
}

bool ui_touch_session_claim_navigation(void)
{
    if (!s_snapshot_valid || s_snapshot.active || !s_snapshot.dragged ||
        s_snapshot.cancelled || s_snapshot.navigation_committed)
    {
        return false;
    }
    s_snapshot.navigation_committed = true;
    return true;
}

static uint16_t absolute_delta(int16_t value, int16_t origin)
{
    int32_t delta = (int32_t)value - (int32_t)origin;
    if (delta < 0)
    {
        delta = -delta;
    }
    return delta > UINT16_MAX ? UINT16_MAX : (uint16_t)delta;
}

static uint32_t bounded_duration_ms(uint64_t started_ms, uint64_t ended_ms)
{
    if (ended_ms < started_ms)
    {
        return 0U;
    }
    const uint64_t duration = ended_ms - started_ms;
    return duration > UINT32_MAX ? UINT32_MAX : (uint32_t)duration;
}

static ui_touch_output_t current_output(void)
{
    return (ui_touch_output_t){
        .sequence_id = s_snapshot.sequence_id,
        .x = s_snapshot.end_x,
        .y = s_snapshot.end_y,
        .pressed = s_snapshot_valid && s_snapshot.active,
    };
}

static void begin_session(int16_t x, int16_t y, uint64_t now_ms)
{
    ++s_next_sequence_id;
    if (s_next_sequence_id == 0U)
    {
        s_next_sequence_id = 1U;
    }
    s_snapshot = (ui_touch_snapshot_t){
        .sequence_id = s_next_sequence_id,
        .start_x = x,
        .start_y = y,
        .end_x = x,
        .end_y = y,
        .valid_sample_count = 1U,
        .active = true,
    };
    s_snapshot_valid = true;
    s_started_ms = now_ms;
    s_last_valid_ms = now_ms;
    s_release_sample_count = 0U;
    s_snapshot.duration_ms = 0U;
}

static void update_pressed(int16_t x, int16_t y, uint64_t now_ms)
{
    s_snapshot.end_x = x;
    s_snapshot.end_y = y;
    ++s_snapshot.valid_sample_count;
    s_last_valid_ms = now_ms;
    s_release_sample_count = 0U;

    const uint16_t delta_x = absolute_delta(x, s_snapshot.start_x);
    const uint16_t delta_y = absolute_delta(y, s_snapshot.start_y);
    if (delta_x > s_snapshot.max_delta_x)
    {
        s_snapshot.max_delta_x = delta_x;
    }
    if (delta_y > s_snapshot.max_delta_y)
    {
        s_snapshot.max_delta_y = delta_y;
    }
    if (s_snapshot.max_delta_x >= UI_TOUCH_TAP_SLOP_PX ||
        s_snapshot.max_delta_y >= UI_TOUCH_TAP_SLOP_PX)
    {
        s_snapshot.dragged = true;
    }
}

static void finish_session(uint64_t now_ms, bool cancelled)
{
    s_snapshot.active = false;
    s_snapshot.cancelled = cancelled;
    s_snapshot.duration_ms = bounded_duration_ms(s_started_ms, now_ms);
    s_release_sample_count = 0U;
}
