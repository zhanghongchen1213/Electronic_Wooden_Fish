/**
 * @file     ui_performance_metrics.c
 * @brief    UI 滚动显示性能统计实现
 * @details  将多次横滑或纵滑片段拼接成连续有效时间轴，并执行严格一秒窗口和帧间隔验收。
 * @author   ZHC
 * @date     2026-07-28
 */

#include "ui_performance_metrics.h"

#include <limits.h>
#include <string.h>

static uint64_t active_time_us(const ui_performance_metrics_t *metrics,
                               uint64_t now_us);
static void evaluate_rolling_window(ui_performance_metrics_t *metrics,
                                    uint64_t active_now_us);
static void add_rolling_frame(ui_performance_metrics_t *metrics,
                              uint64_t active_now_us);
static void record_interval(ui_performance_metrics_t *metrics,
                            uint64_t interval_us);
static uint32_t interval_percentile_99(
    const ui_performance_metrics_t *metrics);
static void reset_period(ui_performance_metrics_t *metrics,
                         bool motion_active,
                         uint64_t now_us);
static void increment_saturating(uint32_t *value);
static void add_saturating_u64(uint64_t *value, uint64_t addition);

void ui_performance_metrics_init(ui_performance_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return;
    }
    memset(metrics, 0, sizeof(*metrics));
    metrics->min_window_frames = UINT32_MAX;
}

void ui_performance_metrics_set_motion(ui_performance_metrics_t *metrics,
                                       bool active,
                                       uint64_t now_us)
{
    if (metrics == NULL || metrics->motion_active == active)
    {
        return;
    }
    if (active)
    {
        metrics->motion_active = true;
        metrics->motion_started_us = now_us;
        return;
    }

    const uint64_t stopped_active_us = active_time_us(metrics, now_us);
    metrics->completed_active_us = stopped_active_us;
    metrics->motion_active = false;
    evaluate_rolling_window(metrics, stopped_active_us);
}

void ui_performance_metrics_record_render(ui_performance_metrics_t *metrics,
                                          uint32_t duration_us)
{
    if (metrics == NULL || !metrics->motion_active)
    {
        return;
    }
    add_saturating_u64(&metrics->render_total_us, duration_us);
    increment_saturating(&metrics->render_count);
    if (duration_us > metrics->max_render_us)
    {
        metrics->max_render_us = duration_us;
    }
}

void ui_performance_metrics_record_fast_path_miss(
    ui_performance_metrics_t *metrics)
{
    if (metrics == NULL || !metrics->motion_active)
    {
        return;
    }
    increment_saturating(&metrics->fast_path_misses);
}

void ui_performance_metrics_record_frame(ui_performance_metrics_t *metrics,
                                         uint64_t completed_us,
                                         uint32_t flush_us,
                                         uint32_t pixels)
{
    if (metrics == NULL || !metrics->motion_active)
    {
        return;
    }

    const uint64_t frame_active_us = active_time_us(metrics, completed_us);
    if (metrics->previous_frame_valid &&
        frame_active_us >= metrics->previous_frame_us)
    {
        record_interval(metrics,
                        frame_active_us - metrics->previous_frame_us);
    }
    metrics->previous_frame_valid = true;
    metrics->previous_frame_us = frame_active_us;
    increment_saturating(&metrics->complete_frames);
    add_saturating_u64(&metrics->flush_total_us, flush_us);
    increment_saturating(&metrics->flush_count);
    if (flush_us > metrics->max_flush_us)
    {
        metrics->max_flush_us = flush_us;
    }
    add_saturating_u64(&metrics->transferred_pixels, pixels);
    add_rolling_frame(metrics, frame_active_us);
    evaluate_rolling_window(metrics, frame_active_us);
}

bool ui_performance_metrics_take_report(ui_performance_metrics_t *metrics,
                                        uint64_t now_us,
                                        ui_performance_report_t *report)
{
    if (metrics == NULL || report == NULL)
    {
        return false;
    }
    const uint64_t duration_us = active_time_us(metrics, now_us);
    if (duration_us < UI_PERFORMANCE_REPORT_PERIOD_US)
    {
        return false;
    }

    evaluate_rolling_window(metrics, duration_us);
    const uint32_t min_window_frames =
        metrics->evaluated_windows == 0U
            ? 0U
            : metrics->min_window_frames;
    *report = (ui_performance_report_t){
        .active_duration_us = duration_us,
        .transferred_pixels = metrics->transferred_pixels,
        .complete_frames = metrics->complete_frames,
        .evaluated_windows = metrics->evaluated_windows,
        .min_window_frames = min_window_frames,
        .p99_interval_us = interval_percentile_99(metrics),
        .max_interval_us = metrics->max_interval_us,
        .average_render_us =
            metrics->render_count == 0U
                ? 0U
                : (uint32_t)(metrics->render_total_us /
                             metrics->render_count),
        .max_render_us = metrics->max_render_us,
        .average_flush_us =
            metrics->flush_count == 0U
                ? 0U
                : (uint32_t)(metrics->flush_total_us /
                             metrics->flush_count),
        .max_flush_us = metrics->max_flush_us,
        .fast_path_misses = metrics->fast_path_misses,
    };
    report->passed =
        report->evaluated_windows != 0U &&
        report->min_window_frames >= UI_PERFORMANCE_MIN_WINDOW_FRAMES &&
        report->p99_interval_us <= UI_PERFORMANCE_P99_LIMIT_US &&
        report->max_interval_us <=
            UI_PERFORMANCE_MAX_INTERVAL_LIMIT_US &&
        report->fast_path_misses == 0U;

    reset_period(metrics, metrics->motion_active, now_us);
    return true;
}

static uint64_t active_time_us(const ui_performance_metrics_t *metrics,
                               uint64_t now_us)
{
    if (metrics == NULL || !metrics->motion_active ||
        now_us < metrics->motion_started_us)
    {
        return metrics != NULL ? metrics->completed_active_us : 0U;
    }
    const uint64_t segment_us = now_us - metrics->motion_started_us;
    if (UINT64_MAX - metrics->completed_active_us < segment_us)
    {
        return UINT64_MAX;
    }
    return metrics->completed_active_us + segment_us;
}

static void evaluate_rolling_window(ui_performance_metrics_t *metrics,
                                    uint64_t active_now_us)
{
    if (metrics == NULL ||
        active_now_us < UI_PERFORMANCE_ROLLING_WINDOW_US)
    {
        return;
    }
    const uint64_t window_start =
        active_now_us - UI_PERFORMANCE_ROLLING_WINDOW_US;
    while (metrics->rolling_count > 0U)
    {
        const uint64_t oldest =
            metrics->rolling_frames[metrics->rolling_head];
        if (oldest > window_start)
        {
            break;
        }
        metrics->rolling_head =
            (metrics->rolling_head + 1U) %
            UI_PERFORMANCE_ROLLING_FRAME_CAPACITY;
        --metrics->rolling_count;
    }
    increment_saturating(&metrics->evaluated_windows);
    const uint32_t frames = (uint32_t)metrics->rolling_count;
    if (frames < metrics->min_window_frames)
    {
        metrics->min_window_frames = frames;
    }
}

static void add_rolling_frame(ui_performance_metrics_t *metrics,
                              uint64_t active_now_us)
{
    if (metrics->rolling_count ==
        UI_PERFORMANCE_ROLLING_FRAME_CAPACITY)
    {
        metrics->rolling_head =
            (metrics->rolling_head + 1U) %
            UI_PERFORMANCE_ROLLING_FRAME_CAPACITY;
        --metrics->rolling_count;
    }
    const size_t tail =
        (metrics->rolling_head + metrics->rolling_count) %
        UI_PERFORMANCE_ROLLING_FRAME_CAPACITY;
    metrics->rolling_frames[tail] = active_now_us;
    ++metrics->rolling_count;
}

static void record_interval(ui_performance_metrics_t *metrics,
                            uint64_t interval_us)
{
    size_t bucket =
        (size_t)((interval_us +
                  UI_PERFORMANCE_INTERVAL_BUCKET_US - 1U) /
                 UI_PERFORMANCE_INTERVAL_BUCKET_US);
    if (bucket >= UI_PERFORMANCE_INTERVAL_BUCKET_COUNT)
    {
        bucket = UI_PERFORMANCE_INTERVAL_BUCKET_COUNT - 1U;
    }
    increment_saturating(&metrics->interval_histogram[bucket]);
    increment_saturating(&metrics->interval_count);
    const uint32_t bounded_interval =
        interval_us > UINT32_MAX ? UINT32_MAX : (uint32_t)interval_us;
    if (bounded_interval > metrics->max_interval_us)
    {
        metrics->max_interval_us = bounded_interval;
    }
}

static uint32_t interval_percentile_99(
    const ui_performance_metrics_t *metrics)
{
    if (metrics == NULL || metrics->interval_count == 0U)
    {
        return 0U;
    }
    const uint64_t rank =
        ((uint64_t)metrics->interval_count * 99U + 99U) / 100U;
    uint64_t cumulative = 0U;
    for (size_t bucket = 0U;
         bucket < UI_PERFORMANCE_INTERVAL_BUCKET_COUNT;
         ++bucket)
    {
        cumulative += metrics->interval_histogram[bucket];
        if (cumulative >= rank)
        {
            return (uint32_t)(bucket *
                              UI_PERFORMANCE_INTERVAL_BUCKET_US);
        }
    }
    return (UI_PERFORMANCE_INTERVAL_BUCKET_COUNT - 1U) *
           UI_PERFORMANCE_INTERVAL_BUCKET_US;
}

static void reset_period(ui_performance_metrics_t *metrics,
                         bool motion_active,
                         uint64_t now_us)
{
    memset(metrics, 0, sizeof(*metrics));
    metrics->min_window_frames = UINT32_MAX;
    metrics->motion_active = motion_active;
    metrics->motion_started_us = motion_active ? now_us : 0U;
}

static void increment_saturating(uint32_t *value)
{
    if (value != NULL && *value != UINT32_MAX)
    {
        ++(*value);
    }
}

static void add_saturating_u64(uint64_t *value, uint64_t addition)
{
    if (value == NULL)
    {
        return;
    }
    *value = UINT64_MAX - *value < addition
                 ? UINT64_MAX
                 : *value + addition;
}
