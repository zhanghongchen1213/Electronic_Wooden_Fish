/**
 * @file     wrist_raise_policy.h
 * @brief    抬腕亮屏姿态分类纯策略接口
 * @details  定义不依赖任务、总线和 UI 的低功耗门控状态机与加速度分类合同。
 * @author   ZHC
 * @date     2026-07-28
 */

#ifndef LEGBOT_WRIST_RAISE_POLICY_H
#define LEGBOT_WRIST_RAISE_POLICY_H

#include <stdbool.h>
#include <stdint.h>

/** 候选加速度采样周期，对应 QMI8658C 62.5 Hz。 */
#define WRIST_RAISE_SAMPLE_PERIOD_MS 16U
/** 单次候选分类的最长时间窗口。 */
#define WRIST_RAISE_COLLECTION_TIMEOUT_MS 1000U
/** 单次候选允许写入的最大样本数。 */
#define WRIST_RAISE_MAX_SAMPLES 64U
/** 屏幕确认关闭后的防反弹等待时间。 */
#define WRIST_RAISE_SCREEN_OFF_REARM_MS 800U
/** 非抬腕候选拒绝后的重新武装等待时间。 */
#define WRIST_RAISE_REJECTED_REARM_MS 300U
/** 稳定重力估计使用的连续样本数量。 */
#define WRIST_RAISE_STABLE_WINDOW_SAMPLES 3U

typedef enum
{
    WRIST_RAISE_STATE_DISARMED = 0, /**< 功能关闭或当前不允许监测。 */
    WRIST_RAISE_STATE_WOM_ARMED,    /**< QMI8658C WoM 已武装。 */
    WRIST_RAISE_STATE_COLLECTING,   /**< 正在采集候选加速度。 */
    WRIST_RAISE_STATE_CONFIRMED,    /**< 候选已确认是抬腕。 */
    WRIST_RAISE_STATE_REJECTED,     /**< 候选已确认不是抬腕。 */
    WRIST_RAISE_STATE_REARM_DELAY,  /**< 等待防反弹或拒绝冷却结束。 */
} wrist_raise_state_t;

typedef enum
{
    WRIST_RAISE_RESULT_NONE = 0, /**< 本次输入尚未形成结论。 */
    WRIST_RAISE_RESULT_CONFIRMED, /**< 本次输入确认抬腕。 */
    WRIST_RAISE_RESULT_REJECTED,  /**< 本次输入拒绝候选。 */
} wrist_raise_result_t;

typedef struct
{
    int32_t right_mg;  /**< 屏幕向右轴的加速度，单位 mg。 */
    int32_t top_mg;    /**< 屏幕向上轴的加速度，单位 mg。 */
    int32_t normal_mg; /**< 屏幕正面法向轴的加速度，单位 mg。 */
} wrist_raise_accel_t;

typedef struct
{
    wrist_raise_state_t state; /**< 当前分类状态。 */
    uint64_t state_since_ms;   /**< 当前状态开始时的单调毫秒。 */
    uint32_t rearm_delay_ms;   /**< 当前重新武装等待时间。 */
    uint32_t sample_count;     /**< 本轮候选已接收的样本数。 */
    wrist_raise_accel_t window[WRIST_RAISE_STABLE_WINDOW_SAMPLES]; /**< 三点滑动窗口。 */
    uint32_t window_count;     /**< 滑动窗口中的有效样本数。 */
    uint32_t window_next;      /**< 下一次覆盖的滑动窗口下标。 */
    wrist_raise_accel_t anchor; /**< 候选早期可信重力姿态。 */
    bool anchor_valid;         /**< 是否已经取得早期可信姿态。 */
    bool anchor_retry_available; /**< 已验证基线是否还可供一次拒绝重试使用。 */
} wrist_raise_policy_t;

/**
 * @brief 初始化抬腕策略为未武装状态
 * @param policy 调用方拥有的策略状态
 */
void wrist_raise_policy_init(wrist_raise_policy_t *policy);

/**
 * @brief 立即取消候选并进入未武装状态
 * @param policy 调用方拥有的策略状态
 */
void wrist_raise_policy_disarm(wrist_raise_policy_t *policy);

/**
 * @brief 安排一次有界延迟后的 WoM 重新武装
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 * @param delay_ms 重新武装等待时间
 */
void wrist_raise_policy_schedule_arm(wrist_raise_policy_t *policy,
                                     uint64_t now_ms,
                                     uint32_t delay_ms);

/**
 * @brief 候选拒绝后安排一次保留已验证基线的重新武装
 * @details 仅武装前取得的可信基线允许续用一次；候选内补建的锚点、
 *          已经续用过的基线和普通重新调度均不会被保留。
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 * @param delay_ms 重新武装等待时间
 */
void wrist_raise_policy_schedule_retry_arm(
    wrist_raise_policy_t *policy,
    uint64_t now_ms,
    uint32_t delay_ms);

/**
 * @brief 判断重新武装等待时间是否已经结束
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 * @return true 可以尝试进入硬件 WoM，false 尚不可
 */
bool wrist_raise_policy_arm_ready(const wrist_raise_policy_t *policy,
                                  uint64_t now_ms);

/**
 * @brief 在普通加速度模式下写入一个 WoM 武装前姿态样本
 * @details 仅重新武装等待状态接收样本；连续三点稳定后保存当前姿态，
 *          供同一轮 WoM 候选使用。冲击或饱和样本会清空未完成窗口。
 * @param policy 调用方拥有的策略状态
 * @param sample 屏幕坐标系加速度，单位 mg
 * @return true 已取得稳定基线，false 尚未取得或当前状态不允许
 */
bool wrist_raise_policy_add_arm_baseline_sample(
    wrist_raise_policy_t *policy,
    const wrist_raise_accel_t *sample);

/**
 * @brief 在硬件 WoM 成功后标记策略已武装
 * @details 从重新武装等待状态进入时，保留本轮取得的稳定姿态基线。
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 */
void wrist_raise_policy_mark_armed(wrist_raise_policy_t *policy,
                                   uint64_t now_ms);

/**
 * @brief 在确认 WoM 事件并切入候选模式后开始分类
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 * @return true 已开始候选，false 当前状态不允许
 */
bool wrist_raise_policy_begin_candidate(wrist_raise_policy_t *policy,
                                        uint64_t now_ms);

/**
 * @brief 判断当前候选是否已经越过一秒分类窗口
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 * @return true 候选应按超时拒绝，false 尚未超时或不在采集
 */
bool wrist_raise_policy_candidate_expired(
    const wrist_raise_policy_t *policy,
    uint64_t now_ms);

/**
 * @brief 写入一个已映射到屏幕坐标系的加速度样本
 * @details 使用三点稳定重力、至少 45 度姿态变化和最终可读包络确认抬腕。
 * @param policy 调用方拥有的策略状态
 * @param sample 屏幕坐标系加速度，单位 mg
 * @param now_ms 当前 boot 单调毫秒
 * @return 尚无结论、确认抬腕或拒绝候选
 */
wrist_raise_result_t wrist_raise_policy_add_sample(
    wrist_raise_policy_t *policy,
    const wrist_raise_accel_t *sample,
    uint64_t now_ms);

/**
 * @brief 在采样或硬件失败时拒绝当前候选
 * @param policy 调用方拥有的策略状态
 * @param now_ms 当前 boot 单调毫秒
 */
void wrist_raise_policy_reject(wrist_raise_policy_t *policy,
                               uint64_t now_ms);

#endif /* LEGBOT_WRIST_RAISE_POLICY_H */
