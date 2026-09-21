/**
 * @file     ui_performance_metrics.h
 * @brief    UI 滚动显示性能统计接口
 * @details  以有效运动时间轴汇总常驻横滑或设置纵滑的完整显示帧、滚动窗口帧数、帧间隔、渲染与传输耗时。
 * @author   ZHC
 * @date     2026-07-28
 */

#ifndef LEGBOT_UI_PERFORMANCE_METRICS_H
#define LEGBOT_UI_PERFORMANCE_METRICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 单次正式性能汇总所需的累计有效滚动时间。 */
#define UI_PERFORMANCE_REPORT_PERIOD_US 60000000ULL
/** 严格帧数验收的滚动窗口长度。 */
#define UI_PERFORMANCE_ROLLING_WINDOW_US 1000000ULL
/** 滚动窗口保存的最大完整帧时间戳数量。 */
#define UI_PERFORMANCE_ROLLING_FRAME_CAPACITY 256U
/** 帧间隔直方图单桶宽度。 */
#define UI_PERFORMANCE_INTERVAL_BUCKET_US 100U
/** 帧间隔直方图覆盖至 100 ms，最后一桶同时接收更大值。 */
#define UI_PERFORMANCE_INTERVAL_BUCKET_COUNT 1001U
/** 严格验收要求的每秒最低完整帧数。 */
#define UI_PERFORMANCE_MIN_WINDOW_FRAMES 60U
/** 严格验收允许的 P99 最大帧间隔。 */
#define UI_PERFORMANCE_P99_LIMIT_US 20000U
/** 严格验收允许的单次最大帧间隔。 */
#define UI_PERFORMANCE_MAX_INTERVAL_LIMIT_US 33333U

typedef struct
{
    uint64_t active_duration_us; /**< 本轮累计有效横滑时间。 */
    uint64_t transferred_pixels; /**< 本轮完成传输的像素总量。 */
    uint32_t complete_frames;    /**< 最后一块 DMA 已完成的完整帧数。 */
    uint32_t evaluated_windows;  /**< 已评估的一秒滚动窗口数量。 */
    uint32_t min_window_frames;  /**< 所有已评估窗口中的最低完整帧数。 */
    uint32_t p99_interval_us;    /**< 完整帧间隔 P99。 */
    uint32_t max_interval_us;    /**< 完整帧最大间隔。 */
    uint32_t average_render_us;  /**< LVGL timer handler 平均耗时。 */
    uint32_t max_render_us;      /**< LVGL timer handler 最大耗时。 */
    uint32_t average_flush_us;   /**< 完整帧首块提交至末块 DMA 完成平均耗时。 */
    uint32_t max_flush_us;       /**< 完整帧首块提交至末块 DMA 完成最大耗时。 */
    uint32_t fast_path_misses;   /**< 本周期运动期间快路不可用次数。 */
    bool passed;                 /**< 是否同时满足帧率与快路零缺失验收。 */
} ui_performance_report_t;

typedef struct
{
    bool motion_active;        /**< 被测滚动容器当前是否正在运动。 */
    bool previous_frame_valid; /**< 是否已有可计算间隔的完整帧。 */
    uint64_t completed_active_us; /**< 已结束运动片段的累计有效时间。 */
    uint64_t motion_started_us;   /**< 当前运动片段真实开始时间。 */
    uint64_t previous_frame_us;   /**< 上一完整帧的有效时间轴时间。 */
    uint64_t rolling_frames[UI_PERFORMANCE_ROLLING_FRAME_CAPACITY]; /**< 一秒窗口时间戳环。 */
    size_t rolling_head;  /**< 一秒窗口最旧时间戳索引。 */
    size_t rolling_count; /**< 一秒窗口当前时间戳数量。 */
    uint32_t interval_histogram[UI_PERFORMANCE_INTERVAL_BUCKET_COUNT]; /**< 帧间隔直方图。 */
    uint32_t interval_count;     /**< 已统计帧间隔数量。 */
    uint32_t max_interval_us;    /**< 已统计的最大帧间隔。 */
    uint32_t complete_frames;    /**< 本轮完整帧数量。 */
    uint32_t evaluated_windows;  /**< 本轮已评估窗口数量。 */
    uint32_t min_window_frames;  /**< 本轮最低窗口帧数。 */
    uint64_t render_total_us;    /**< 本轮 LVGL 渲染总耗时。 */
    uint32_t render_count;       /**< 本轮 LVGL 渲染次数。 */
    uint32_t max_render_us;      /**< 本轮 LVGL 最大渲染耗时。 */
    uint64_t flush_total_us;     /**< 本轮完整帧传输总耗时。 */
    uint32_t flush_count;        /**< 本轮完整帧传输次数。 */
    uint32_t max_flush_us;       /**< 本轮完整帧最大传输耗时。 */
    uint64_t transferred_pixels; /**< 本轮完整帧传输像素总量。 */
    uint32_t fast_path_misses;   /**< 本轮运动期间快路不可用次数。 */
} ui_performance_metrics_t;

/**
 * @brief 初始化滚动性能统计器
 * @param metrics 调用方持有的统计器
 */
void ui_performance_metrics_init(ui_performance_metrics_t *metrics);

/**
 * @brief 同步被测滚动容器运动状态
 * @param metrics 调用方持有的统计器
 * @param active true 表示运动开始或持续，false 表示运动结束
 * @param now_us 当前单调微秒
 */
void ui_performance_metrics_set_motion(ui_performance_metrics_t *metrics,
                                       bool active,
                                       uint64_t now_us);

/**
 * @brief 记录一次运动期间的 LVGL timer handler 耗时
 * @param metrics 调用方持有的统计器
 * @param duration_us 本轮调用耗时
 */
void ui_performance_metrics_record_render(ui_performance_metrics_t *metrics,
                                          uint32_t duration_us);

/**
 * @brief 记录一次运动期间硬件快路不可用
 * @param metrics 调用方持有的统计器
 */
void ui_performance_metrics_record_fast_path_miss(
    ui_performance_metrics_t *metrics);

/**
 * @brief 记录最后一块 DMA 已完成的完整显示帧
 * @param metrics 调用方持有的统计器
 * @param completed_us DMA 完成单调微秒
 * @param flush_us 本帧首块提交至末块 DMA 完成耗时
 * @param pixels 本帧实际传输像素数
 */
void ui_performance_metrics_record_frame(ui_performance_metrics_t *metrics,
                                         uint64_t completed_us,
                                         uint32_t flush_us,
                                         uint32_t pixels);

/**
 * @brief 在累计满六十秒有效滚动后取走一轮汇总
 * @param metrics 调用方持有的统计器
 * @param now_us 当前单调微秒
 * @param report 输出汇总
 * @return true 已输出并开启下一轮，false 尚未累计满诊断周期
 */
bool ui_performance_metrics_take_report(ui_performance_metrics_t *metrics,
                                        uint64_t now_us,
                                        ui_performance_report_t *report);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_PERFORMANCE_METRICS_H */
