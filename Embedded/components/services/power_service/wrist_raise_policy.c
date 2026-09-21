/**
 * @file     wrist_raise_policy.c
 * @brief    抬腕亮屏姿态分类纯策略实现
 * @details  以整数加速度推进 WoM 候选、三点稳定重力和最终看表姿态判定。
 * @author   ZHC
 * @date     2026-07-28
 */

#include "wrist_raise_policy.h"

#include <limits.h>
#include <stddef.h>

/** 可信静态重力下限，单位 mg。 */
#define WRIST_RAISE_GRAVITY_MIN_MG 750
/** 可信静态重力上限，单位 mg。 */
#define WRIST_RAISE_GRAVITY_MAX_MG 1250
/** 任一样本超过该合加速度时按强冲击拒绝，单位 mg。 */
#define WRIST_RAISE_IMPACT_LIMIT_MG 2400
/** 单轴超过该值时视为输入饱和或坐标换算错误，单位 mg。 */
#define WRIST_RAISE_AXIS_LIMIT_MG 3900
/** 三点窗口的合向量均方波动上限，单位 mg²。 */
#define WRIST_RAISE_STABLE_VARIANCE_LIMIT_MG2 10000
/** 最终表盘法向的最小重力分量，单位 mg。 */
#define WRIST_RAISE_FOCUS_NORMAL_MIN_MG 450
/** 最终左右方向允许的绝对重力分量，单位 mg。 */
#define WRIST_RAISE_FOCUS_RIGHT_ABS_MAX_MG 850
/** 最终表盘上方向允许的最小重力分量，单位 mg。 */
#define WRIST_RAISE_FOCUS_TOP_MIN_MG (-900)
/** 最终表盘上方向允许的最大重力分量，单位 mg。 */
#define WRIST_RAISE_FOCUS_TOP_MAX_MG 850
/** 自然垂腕起点在表盘上方向的最小绝对重力分量，单位 mg。 */
#define WRIST_RAISE_START_TOP_ABS_MIN_MG 600
/** 最终姿态相对垂腕起点必须增加的最小表盘法向分量，单位 mg。 */
#define WRIST_RAISE_NORMAL_PROGRESS_MIN_MG 150
/** 无武装前基线时允许补建早期锚点的最长时间，单位毫秒。 */
#define WRIST_RAISE_ANCHOR_CAPTURE_MAX_MS 300U
/** 确认前至少接收的样本数，避免首个三点窗口立即误判。 */
#define WRIST_RAISE_MIN_CONFIRM_SAMPLES 6U
/** 确认前至少经历的动作时间，单位毫秒。 */
#define WRIST_RAISE_MIN_CONFIRM_MS 160U

static uint64_t monotonic_elapsed_ms(uint64_t now_ms, uint64_t started_ms);
static int32_t absolute_i32(int32_t value);
static int64_t magnitude_squared(const wrist_raise_accel_t *sample);
static bool sample_is_impact_or_saturated(
    const wrist_raise_accel_t *sample);
static bool stable_window_mean(const wrist_raise_policy_t *policy,
                               wrist_raise_accel_t *mean);
static bool angle_changed_at_least_45_degrees(
    const wrist_raise_accel_t *first,
    const wrist_raise_accel_t *last);
static bool start_pose_is_eligible(const wrist_raise_accel_t *sample);
static bool final_pose_is_readable(const wrist_raise_accel_t *sample);

void wrist_raise_policy_init(wrist_raise_policy_t *policy)
{
    wrist_raise_policy_disarm(policy);
}

void wrist_raise_policy_disarm(wrist_raise_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    *policy = (wrist_raise_policy_t){
        .state = WRIST_RAISE_STATE_DISARMED,
    };
}

void wrist_raise_policy_schedule_arm(wrist_raise_policy_t *policy,
                                     uint64_t now_ms,
                                     uint32_t delay_ms)
{
    if (policy == NULL)
    {
        return;
    }
    wrist_raise_policy_disarm(policy);
    policy->state = WRIST_RAISE_STATE_REARM_DELAY;
    policy->state_since_ms = now_ms;
    policy->rearm_delay_ms = delay_ms;
}

void wrist_raise_policy_schedule_retry_arm(
    wrist_raise_policy_t *policy,
    uint64_t now_ms,
    uint32_t delay_ms)
{
    if (policy == NULL)
    {
        return;
    }
    const bool preserve_anchor =
        policy->anchor_valid &&
        policy->anchor_retry_available;
    const wrist_raise_accel_t anchor = policy->anchor;
    wrist_raise_policy_disarm(policy);
    policy->state = WRIST_RAISE_STATE_REARM_DELAY;
    policy->state_since_ms = now_ms;
    policy->rearm_delay_ms = delay_ms;
    policy->anchor = anchor;
    policy->anchor_valid = preserve_anchor;
    policy->anchor_retry_available = false;
}

bool wrist_raise_policy_arm_ready(const wrist_raise_policy_t *policy,
                                  uint64_t now_ms)
{
    return policy != NULL &&
           policy->state == WRIST_RAISE_STATE_REARM_DELAY &&
           monotonic_elapsed_ms(now_ms, policy->state_since_ms) >=
               policy->rearm_delay_ms;
}

bool wrist_raise_policy_add_arm_baseline_sample(
    wrist_raise_policy_t *policy,
    const wrist_raise_accel_t *sample)
{
    if (policy == NULL || sample == NULL ||
        policy->state != WRIST_RAISE_STATE_REARM_DELAY)
    {
        return false;
    }
    if (sample_is_impact_or_saturated(sample))
    {
        policy->window_count = 0U;
        policy->window_next = 0U;
        policy->anchor_valid = false;
        return false;
    }

    policy->window[policy->window_next] = *sample;
    policy->window_next =
        (policy->window_next + 1U) % WRIST_RAISE_STABLE_WINDOW_SAMPLES;
    if (policy->window_count < WRIST_RAISE_STABLE_WINDOW_SAMPLES)
    {
        ++policy->window_count;
    }

    wrist_raise_accel_t mean = {0};
    if (!stable_window_mean(policy, &mean))
    {
        return false;
    }
    if (!start_pose_is_eligible(&mean))
    {
        policy->anchor_valid = false;
        return false;
    }
    policy->anchor = mean;
    policy->anchor_valid = true;
    policy->anchor_retry_available = true;
    return true;
}

void wrist_raise_policy_mark_armed(wrist_raise_policy_t *policy,
                                   uint64_t now_ms)
{
    if (policy == NULL)
    {
        return;
    }
    const bool preserve_anchor =
        policy->state == WRIST_RAISE_STATE_REARM_DELAY &&
        policy->anchor_valid;
    const wrist_raise_accel_t anchor = policy->anchor;
    const bool anchor_retry_available =
        policy->anchor_retry_available;
    *policy = (wrist_raise_policy_t){
        .state = WRIST_RAISE_STATE_WOM_ARMED,
        .state_since_ms = now_ms,
        .anchor = anchor,
        .anchor_valid = preserve_anchor,
        .anchor_retry_available =
            preserve_anchor && anchor_retry_available,
    };
}

bool wrist_raise_policy_begin_candidate(wrist_raise_policy_t *policy,
                                        uint64_t now_ms)
{
    if (policy == NULL ||
        policy->state != WRIST_RAISE_STATE_WOM_ARMED)
    {
        return false;
    }
    const wrist_raise_accel_t anchor = policy->anchor;
    const bool anchor_valid = policy->anchor_valid;
    const bool anchor_retry_available =
        policy->anchor_retry_available;
    wrist_raise_policy_t candidate = {
        .state = WRIST_RAISE_STATE_COLLECTING,
        .state_since_ms = now_ms,
        .anchor = anchor,
        .anchor_valid = anchor_valid,
        .anchor_retry_available = anchor_retry_available,
    };
    *policy = candidate;
    return true;
}

bool wrist_raise_policy_candidate_expired(
    const wrist_raise_policy_t *policy,
    uint64_t now_ms)
{
    return policy != NULL &&
           policy->state == WRIST_RAISE_STATE_COLLECTING &&
           monotonic_elapsed_ms(now_ms, policy->state_since_ms) >=
               WRIST_RAISE_COLLECTION_TIMEOUT_MS;
}

wrist_raise_result_t wrist_raise_policy_add_sample(
    wrist_raise_policy_t *policy,
    const wrist_raise_accel_t *sample,
    uint64_t now_ms)
{
    if (policy == NULL || sample == NULL ||
        policy->state != WRIST_RAISE_STATE_COLLECTING)
    {
        return WRIST_RAISE_RESULT_NONE;
    }
    if (monotonic_elapsed_ms(now_ms, policy->state_since_ms) >=
            WRIST_RAISE_COLLECTION_TIMEOUT_MS ||
        policy->sample_count >= WRIST_RAISE_MAX_SAMPLES ||
        sample_is_impact_or_saturated(sample))
    {
        wrist_raise_policy_reject(policy, now_ms);
        return WRIST_RAISE_RESULT_REJECTED;
    }

    policy->window[policy->window_next] = *sample;
    policy->window_next =
        (policy->window_next + 1U) % WRIST_RAISE_STABLE_WINDOW_SAMPLES;
    if (policy->window_count < WRIST_RAISE_STABLE_WINDOW_SAMPLES)
    {
        ++policy->window_count;
    }
    ++policy->sample_count;

    wrist_raise_accel_t mean = {0};
    if (!stable_window_mean(policy, &mean))
    {
        return WRIST_RAISE_RESULT_NONE;
    }
    if (!policy->anchor_valid)
    {
        if (monotonic_elapsed_ms(now_ms, policy->state_since_ms) <=
                WRIST_RAISE_ANCHOR_CAPTURE_MAX_MS &&
            start_pose_is_eligible(&mean))
        {
            policy->anchor = mean;
            policy->anchor_valid = true;
            policy->anchor_retry_available = false;
        }
        return WRIST_RAISE_RESULT_NONE;
    }
    if (policy->sample_count < WRIST_RAISE_MIN_CONFIRM_SAMPLES ||
        monotonic_elapsed_ms(now_ms, policy->state_since_ms) <
            WRIST_RAISE_MIN_CONFIRM_MS)
    {
        return WRIST_RAISE_RESULT_NONE;
    }
    if (mean.normal_mg - policy->anchor.normal_mg >=
            WRIST_RAISE_NORMAL_PROGRESS_MIN_MG &&
        angle_changed_at_least_45_degrees(&policy->anchor, &mean) &&
        final_pose_is_readable(&mean))
    {
        policy->state = WRIST_RAISE_STATE_CONFIRMED;
        policy->state_since_ms = now_ms;
        return WRIST_RAISE_RESULT_CONFIRMED;
    }
    return WRIST_RAISE_RESULT_NONE;
}

void wrist_raise_policy_reject(wrist_raise_policy_t *policy,
                               uint64_t now_ms)
{
    if (policy == NULL)
    {
        return;
    }
    policy->state = WRIST_RAISE_STATE_REJECTED;
    policy->state_since_ms = now_ms;
}

static uint64_t monotonic_elapsed_ms(uint64_t now_ms, uint64_t started_ms)
{
    const uint64_t elapsed = now_ms - started_ms;
    return elapsed <= (uint64_t)INT64_MAX ? elapsed : 0U;
}

static int32_t absolute_i32(int32_t value)
{
    if (value == INT32_MIN)
    {
        return INT32_MAX;
    }
    return value < 0 ? -value : value;
}

static int64_t magnitude_squared(const wrist_raise_accel_t *sample)
{
    return (int64_t)sample->right_mg * sample->right_mg +
           (int64_t)sample->top_mg * sample->top_mg +
           (int64_t)sample->normal_mg * sample->normal_mg;
}

static bool sample_is_impact_or_saturated(
    const wrist_raise_accel_t *sample)
{
    return absolute_i32(sample->right_mg) >
               WRIST_RAISE_AXIS_LIMIT_MG ||
           absolute_i32(sample->top_mg) >
               WRIST_RAISE_AXIS_LIMIT_MG ||
           absolute_i32(sample->normal_mg) >
               WRIST_RAISE_AXIS_LIMIT_MG ||
           magnitude_squared(sample) >
               (int64_t)WRIST_RAISE_IMPACT_LIMIT_MG *
                   WRIST_RAISE_IMPACT_LIMIT_MG;
}

static bool stable_window_mean(const wrist_raise_policy_t *policy,
                               wrist_raise_accel_t *mean)
{
    if (policy->window_count < WRIST_RAISE_STABLE_WINDOW_SAMPLES)
    {
        return false;
    }

    int64_t right_sum = 0;
    int64_t top_sum = 0;
    int64_t normal_sum = 0;
    for (size_t index = 0U;
         index < WRIST_RAISE_STABLE_WINDOW_SAMPLES;
         ++index)
    {
        right_sum += policy->window[index].right_mg;
        top_sum += policy->window[index].top_mg;
        normal_sum += policy->window[index].normal_mg;
    }
    *mean = (wrist_raise_accel_t){
        .right_mg =
            (int32_t)(right_sum / WRIST_RAISE_STABLE_WINDOW_SAMPLES),
        .top_mg =
            (int32_t)(top_sum / WRIST_RAISE_STABLE_WINDOW_SAMPLES),
        .normal_mg =
            (int32_t)(normal_sum / WRIST_RAISE_STABLE_WINDOW_SAMPLES),
    };

    const int64_t mean_magnitude = magnitude_squared(mean);
    if (mean_magnitude <
            (int64_t)WRIST_RAISE_GRAVITY_MIN_MG *
                WRIST_RAISE_GRAVITY_MIN_MG ||
        mean_magnitude >
            (int64_t)WRIST_RAISE_GRAVITY_MAX_MG *
                WRIST_RAISE_GRAVITY_MAX_MG)
    {
        return false;
    }

    int64_t squared_deviation_sum = 0;
    for (size_t index = 0U;
         index < WRIST_RAISE_STABLE_WINDOW_SAMPLES;
         ++index)
    {
        const int64_t right =
            policy->window[index].right_mg - mean->right_mg;
        const int64_t top =
            policy->window[index].top_mg - mean->top_mg;
        const int64_t normal =
            policy->window[index].normal_mg - mean->normal_mg;
        squared_deviation_sum +=
            right * right + top * top + normal * normal;
    }
    return squared_deviation_sum <=
           (int64_t)WRIST_RAISE_STABLE_VARIANCE_LIMIT_MG2 *
               WRIST_RAISE_STABLE_WINDOW_SAMPLES;
}

static bool angle_changed_at_least_45_degrees(
    const wrist_raise_accel_t *first,
    const wrist_raise_accel_t *last)
{
    const int64_t dot =
        (int64_t)first->right_mg * last->right_mg +
        (int64_t)first->top_mg * last->top_mg +
        (int64_t)first->normal_mg * last->normal_mg;
    if (dot <= 0)
    {
        return true;
    }
    const int64_t first_magnitude = magnitude_squared(first);
    const int64_t last_magnitude = magnitude_squared(last);
    return 2 * dot * dot <= first_magnitude * last_magnitude;
}

static bool start_pose_is_eligible(const wrist_raise_accel_t *sample)
{
    return sample->normal_mg <
               WRIST_RAISE_FOCUS_NORMAL_MIN_MG &&
           absolute_i32(sample->top_mg) >=
               WRIST_RAISE_START_TOP_ABS_MIN_MG;
}

static bool final_pose_is_readable(const wrist_raise_accel_t *sample)
{
    return sample->normal_mg >= WRIST_RAISE_FOCUS_NORMAL_MIN_MG &&
           absolute_i32(sample->right_mg) <=
               WRIST_RAISE_FOCUS_RIGHT_ABS_MAX_MG &&
           sample->top_mg >= WRIST_RAISE_FOCUS_TOP_MIN_MG &&
           sample->top_mg <= WRIST_RAISE_FOCUS_TOP_MAX_MG;
}
