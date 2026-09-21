/**
 * @file     ui_touch_session.h
 * @brief    连续触摸采样与单序列手势仲裁接口
 * @details  将按压、零触点和暂时未知采样归并为稳定 LVGL 输入，并保存点击与导航判定所需的序列事实。
 * @author   ZHC
 * @date     2026-07-28
 */

#ifndef LEGBOT_UI_TOUCH_SESSION_H
#define LEGBOT_UI_TOUCH_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 达到该任一轴位移后，本触摸序列永久禁止派生按钮点击。 */
#define UI_TOUCH_TAP_SLOP_PX 10U
/** 暂时未知采样允许保持上一稳定按压状态的最长时间。 */
#define UI_TOUCH_UNKNOWN_HOLD_MS 50U
/** 按压期间确认真实释放所需的连续有效零触点数量。 */
#define UI_TOUCH_RELEASE_CONFIRM_SAMPLES 2U

    /** 触摸控制器单次原始采样分类。 */
    typedef enum
    {
        UI_TOUCH_SAMPLE_PRESSED = 0, /**< 成功读取到一个有效触点。 */
        UI_TOUCH_SAMPLE_RELEASED,    /**< 成功读取且明确没有触点。 */
        UI_TOUCH_SAMPLE_UNKNOWN,     /**< 总线竞争或读取失败，当前触摸事实未知。 */
    } ui_touch_sample_t;

    /** 输入过滤后本周期应转发给 LVGL 的稳定状态。 */
    typedef struct
    {
        uint32_t sequence_id;    /**< 当前或刚结束的触摸序号。 */
        int16_t x;               /**< 稳定触点横坐标。 */
        int16_t y;               /**< 稳定触点纵坐标。 */
        bool pressed;            /**< true 向 LVGL 转发按压，false 转发释放。 */
        bool sequence_started;   /**< 本周期是否开始了新触摸序列。 */
        bool sequence_finished;  /**< 本周期是否结束了触摸序列。 */
        bool cancelled;          /**< 本周期结束是否由读取空洞超时取消。 */
    } ui_touch_output_t;

    /** 最近一个触摸序列的完整仲裁事实。 */
    typedef struct
    {
        uint32_t sequence_id;          /**< 单调递增触摸序号。 */
        int16_t start_x;               /**< 按下起点横坐标。 */
        int16_t start_y;               /**< 按下起点纵坐标。 */
        int16_t end_x;                 /**< 最近有效触点横坐标。 */
        int16_t end_y;                 /**< 最近有效触点纵坐标。 */
        uint16_t max_delta_x;          /**< 相对起点最大横向绝对位移。 */
        uint16_t max_delta_y;          /**< 相对起点最大纵向绝对位移。 */
        uint32_t duration_ms;          /**< 从按下到结束的持续毫秒数。 */
        uint32_t valid_sample_count;   /**< 有效按压或零触点采样数量。 */
        uint32_t unknown_sample_count; /**< 暂时未知采样数量。 */
        bool active;                   /**< 当前序列是否仍保持按压。 */
        bool dragged;                  /**< 是否已越过点击取消位移。 */
        bool cancelled;                /**< 是否因输入事实超时而取消。 */
        bool navigation_committed;     /**< 是否已由本序列提交过导航。 */
    } ui_touch_snapshot_t;

    /** @brief 清空触摸过滤器与最近序列事实。 */
    void ui_touch_session_reset(void);

    /**
     * @brief 输入一个原始采样并生成稳定 LVGL 状态
     * @param sample 原始采样分类
     * @param x 有效按压横坐标，其他分类忽略
     * @param y 有效按压纵坐标，其他分类忽略
     * @param now_ms 单调毫秒时间
     * @return 本周期稳定输出与序列边界
     */
    ui_touch_output_t ui_touch_session_feed(ui_touch_sample_t sample,
                                            int16_t x,
                                            int16_t y,
                                            uint64_t now_ms);

    /**
     * @brief 读取当前或最近完成的触摸序列事实
     * @param snapshot 输出快照
     * @return true 存在有效序列，false 尚未收到过按压
     */
    bool ui_touch_session_snapshot(ui_touch_snapshot_t *snapshot);

    /**
     * @brief 判断最近一次 LVGL 按钮事件是否仍可作为短点处理
     * @return true 允许按钮 intent，false 本序列已拖动、取消或仍未释放
     */
    bool ui_touch_session_button_tap_allowed(void);

    /**
     * @brief 为最近完成的拖动序列唯一认领一次导航
     * @return true 本次成功认领，false 无可用拖动或已经认领
     */
    bool ui_touch_session_claim_navigation(void);

#ifdef __cplusplus
}
#endif

#endif
