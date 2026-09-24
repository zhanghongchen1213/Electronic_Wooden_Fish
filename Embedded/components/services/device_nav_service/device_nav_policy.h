/**
 * @file     device_nav_policy.h
 * @brief    设备三主页导航、设置浮层与熄亮屏的纯逻辑策略。
 * @details  不依赖 ESP-IDF；主机测试钉死 PWR 短按、手势与超时决策。
 *           设置打开时 PWR 短按先关设置并保持当前主页（Story 2.4 裁决）。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_DEVICE_NAV_POLICY_H
#define EWF_DEVICE_NAV_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** PWR 短按上限毫秒；小于板级长按约 2.1s，大于去抖。样机定标前 hardware_pending。 */
#define EWF_PWR_SHORT_PRESS_MAX_MS 800U
/** PWR 去抖下限，与 power_boot_policy 的 20ms 对齐。 */
#define EWF_PWR_SHORT_PRESS_MIN_MS 20U

/** 左右滑最小水平位移（像素，CST9217 坐标空间）。hardware_pending。 */
#define EWF_NAV_SWIPE_MIN_DX 48
/** 上下滑最小垂直位移。hardware_pending。 */
#define EWF_NAV_SWIPE_MIN_DY 48
/** 水平主导比：|dx| 须大于 |dy| * 该比例（整数 1=等权）。 */
#define EWF_NAV_SWIPE_AXIS_RATIO 1

typedef enum {
    EWF_NAV_PAGE_MUYU = 0,   /**< 木鱼主页。 */
    EWF_NAV_PAGE_JINGWEN,    /**< 经文主页。 */
    EWF_NAV_PAGE_TONGJI,     /**< 统计主页。 */
    EWF_NAV_PAGE_COUNT
} ewf_nav_page_t;

typedef enum {
    EWF_NAV_INPUT_NONE = 0,
    EWF_NAV_INPUT_PWR_INTERVAL,   /**< 完整 PWR 按压区间（含 duration_ms）。 */
    EWF_NAV_INPUT_SWIPE_LEFT,    /**< 左滑 → 下一主页。 */
    EWF_NAV_INPUT_SWIPE_RIGHT,   /**< 右滑 → 上一主页。 */
    EWF_NAV_INPUT_SWIPE_DOWN,    /**< 下滑 → 打开设置。 */
    EWF_NAV_INPUT_SWIPE_UP,      /**< 上滑 → 关闭设置。 */
    EWF_NAV_INPUT_TOUCH_WAKE,    /**< 熄屏首触只唤醒。 */
    EWF_NAV_INPUT_IDLE_TIMEOUT,  /**< 无操作超时熄屏。 */
    EWF_NAV_INPUT_ACTIVITY,      /**< 重置空闲计时（敲击/手势/设置写入）。 */
} ewf_nav_input_kind_t;

typedef enum {
    EWF_NAV_REASON_NONE = 0,
    EWF_NAV_REASON_PWR_CYCLE_PAGE,
    EWF_NAV_REASON_PWR_CLOSE_SETTINGS,
    EWF_NAV_REASON_PWR_WAKE,
    EWF_NAV_REASON_PWR_IGNORED_LONG,
    EWF_NAV_REASON_PWR_IGNORED_GLITCH,
    EWF_NAV_REASON_SWIPE_PAGE,
    EWF_NAV_REASON_OPEN_SETTINGS,
    EWF_NAV_REASON_CLOSE_SETTINGS,
    EWF_NAV_REASON_TOUCH_WAKE,
    EWF_NAV_REASON_IDLE_OFF,
    EWF_NAV_REASON_ACTIVITY_RESET,
    EWF_NAV_REASON_IGNORED,
} ewf_nav_reason_t;

typedef struct {
    ewf_nav_page_t active_page; /**< 当前主页（设置不是循环页）。 */
    bool settings_open;         /**< 设置浮层是否打开。 */
    bool screen_on;             /**< 屏幕是否点亮。 */
    uint32_t idle_timeout_s;    /**< 自动熄屏秒数：5/15/30。 */
    uint64_t last_activity_ms;  /**< 最近活动单调毫秒。 */
} ewf_nav_state_t;

typedef struct {
    ewf_nav_input_kind_t kind; /**< 输入种类。 */
    uint32_t duration_ms;      /**< PWR 区间时长；其他输入为 0。 */
    uint64_t now_ms;           /**< 当前单调毫秒。 */
} ewf_nav_input_t;

typedef struct {
    bool changed;              /**< 是否产生状态变化。 */
    ewf_nav_page_t active_page;
    bool settings_open;
    bool screen_on;
    bool wake_display;         /**< 需要点亮显示。 */
    bool blank_display;        /**< 需要熄灭显示。 */
    bool reset_idle;           /**< 需要重置空闲计时。 */
    ewf_nav_reason_t reason;
} ewf_nav_decision_t;

/**
 * @brief 初始化导航状态为默认亮屏木鱼页
 * @param state 状态输出
 * @param idle_timeout_s 熄屏秒数（非法时用 15）
 * @param now_ms 当前单调毫秒
 */
void ewf_nav_state_init(ewf_nav_state_t *state, uint32_t idle_timeout_s,
                        uint64_t now_ms);

/**
 * @brief 对导航状态应用一次输入并给出决策
 * @param state 可变状态
 * @param input 输入事件
 * @return 决策（含下一页/设置/熄亮屏意图）
 */
ewf_nav_decision_t ewf_nav_policy_apply(ewf_nav_state_t *state,
                                        const ewf_nav_input_t *input);

/**
 * @brief 根据起点终点识别手势（任务上下文调用）
 * @param x0 起点 X
 * @param y0 起点 Y
 * @param x1 终点 X
 * @param y1 终点 Y
 * @return 手势对应的输入种类；点触返回 NONE
 */
ewf_nav_input_kind_t ewf_nav_classify_gesture(int16_t x0, int16_t y0,
                                               int16_t x1, int16_t y1);

/**
 * @brief 三主页循环的下一页
 */
ewf_nav_page_t ewf_nav_page_next(ewf_nav_page_t page);

/**
 * @brief 三主页循环的上一页
 */
ewf_nav_page_t ewf_nav_page_prev(ewf_nav_page_t page);

/**
 * @brief 分类 PWR 区间是否为短按导航
 * @param duration_ms 低电平时长
 * @return true 表示短按导航区间
 */
bool ewf_nav_pwr_is_short_press(uint32_t duration_ms);

#ifdef __cplusplus
}
#endif

#endif /* EWF_DEVICE_NAV_POLICY_H */
