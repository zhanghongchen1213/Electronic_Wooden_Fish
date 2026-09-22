/**
 * @file     pvdf_confirm_policy.c
 * @brief    PVDF 唤醒与 ADC 二次确认的纯逻辑策略实现
 * @details  只处理唤醒事件时刻、采样值与 typed 采样状态，不访问 ADC/GPIO、不打印、不阻塞。
 *           同一文件内提供编译期固定的自检样本表与确定性重放函数，固件自检与主机测试共用。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "pvdf_confirm_policy.h"

#include <string.h>

/** 单个合成唤醒激励；采样状态为 VALID 时按三角波合成采样序列。 */
typedef struct
{
    ewf_pvdf_selfcheck_scenario_t scenario; /**< 所属样本类别。 */
    uint32_t at_ms;                         /**< 唤醒事件的单调毫秒。 */
    int32_t peak_millivolt;                 /**< 窗口内合成的正向峰值。 */
    ewf_pvdf_sample_status_t sample_status; /**< 窗口内每一次采样的 typed 状态。 */
} ewf_pvdf_selfcheck_wake_t;

/** 编译期固定的自检样本判定契约表。 */
static const ewf_pvdf_selfcheck_case_t s_selfcheck_cases[EWF_PVDF_SELFCHECK_SCENARIO_COUNT] = {
    {EWF_PVDF_SELFCHECK_SINGLE_TAP,
     "single_tap",
     true,
     1U,
     1U,
     0U,
     EWF_PVDF_EVENT_CANDIDATE_TAP},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND,
     "burst_20_per_second",
     true,
     20U,
     20U,
     0U,
     EWF_PVDF_EVENT_CANDIDATE_TAP},
    {EWF_PVDF_SELFCHECK_CARRY_VIBRATION,
     "carry_vibration",
     false,
     3U,
     0U,
     3U,
     EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD},
    {EWF_PVDF_SELFCHECK_OUT_OF_RANGE,
     "out_of_range",
     false,
     1U,
     0U,
     1U,
     EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE},
    {EWF_PVDF_SELFCHECK_BLIND_WINDOW,
     "blind_window",
     false,
     1U,
     0U,
     1U,
     EWF_PVDF_EVENT_REJECTED_BLIND_WINDOW},
    {EWF_PVDF_SELFCHECK_SAMPLE_FAILURE,
     "sample_failure",
     false,
     1U,
     0U,
     1U,
     EWF_PVDF_EVENT_REJECTED_SAMPLE_FAILED},
};

/**
 * 编译期固定的合成激励表：无随机数、无外部输入，重放结果字节级可复现。
 * 单次敲击与 1 秒 20 次连续敲击的间隔为 50 ms，明显大于确认窗口，用于证明不漏不重。
 */
static const ewf_pvdf_selfcheck_wake_t s_selfcheck_wakes[] = {
    {EWF_PVDF_SELFCHECK_SINGLE_TAP, 100U, 300, EWF_PVDF_SAMPLE_VALID},

    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 100U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 150U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 200U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 250U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 300U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 350U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 400U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 450U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 500U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 550U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 600U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 650U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 700U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 750U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 800U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 850U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 900U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 950U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 1000U, 320, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_BURST_20_PER_SECOND, 1050U, 320, EWF_PVDF_SAMPLE_VALID},

    {EWF_PVDF_SELFCHECK_CARRY_VIBRATION, 100U, 120, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_CARRY_VIBRATION, 130U, 120, EWF_PVDF_SAMPLE_VALID},
    {EWF_PVDF_SELFCHECK_CARRY_VIBRATION, 160U, 120, EWF_PVDF_SAMPLE_VALID},

    {EWF_PVDF_SELFCHECK_OUT_OF_RANGE, 100U, 0, EWF_PVDF_SAMPLE_OUT_OF_RANGE},

    /* 上电瞬间 TLV7042 高阻与阈值未建立造成的伪高电平落在盲窗内，必须被拒绝。 */
    {EWF_PVDF_SELFCHECK_BLIND_WINDOW, 5U, 400, EWF_PVDF_SAMPLE_VALID},

    {EWF_PVDF_SELFCHECK_SAMPLE_FAILURE, 100U, 0, EWF_PVDF_SAMPLE_FAILED},
};

/** 合成激励表的条目数量。 */
#define EWF_PVDF_SELFCHECK_WAKE_COUNT \
    (sizeof(s_selfcheck_wakes) / sizeof(s_selfcheck_wakes[0]))

static bool publish_terminal(ewf_pvdf_confirm_policy_t *policy,
                             ewf_pvdf_event_kind_t kind,
                             int32_t peak_millivolt,
                             uint32_t sample_count,
                             uint32_t now_ms,
                             ewf_pvdf_event_t *event);
static bool resolve_confirmation(ewf_pvdf_confirm_policy_t *policy,
                                 uint32_t now_ms,
                                 ewf_pvdf_event_t *event);
static int32_t synthesize_millivolt(int32_t peak_millivolt, uint32_t sample_index);
static void record_terminal(ewf_pvdf_selfcheck_scenario_t scenario,
                           const ewf_pvdf_event_t *event,
                           ewf_pvdf_selfcheck_counts_t *counts);
static void replay_scenario(ewf_pvdf_confirm_policy_t *policy,
                            ewf_pvdf_selfcheck_scenario_t scenario,
                            ewf_pvdf_selfcheck_counts_t *counts);

void ewf_pvdf_confirm_policy_reset(ewf_pvdf_confirm_policy_t *policy)
{
    if (policy == NULL)
    {
        return;
    }
    policy->blind_window_open = false;
    policy->blind_deadline_ms = 0U;
    policy->confirm_in_flight = false;
    policy->confirm_started_ms = 0U;
    policy->confirm_deadline_ms = 0U;
    policy->peak_millivolt = 0;
    policy->sample_count = 0U;
    policy->out_of_range_seen = false;
    policy->sample_failed_seen = false;
    policy->wake_event_count = 0U;
    policy->candidate_count = 0U;
    memset(policy->rejected_counts, 0, sizeof(policy->rejected_counts));
    policy->last_kind = EWF_PVDF_EVENT_NONE;
    policy->last_margin_millivolt = 0;
}

void ewf_pvdf_confirm_policy_arm_runtime(ewf_pvdf_confirm_policy_t *policy,
                                         uint32_t now_ms)
{
    if (policy == NULL)
    {
        return;
    }
    policy->blind_window_open = true;
    policy->blind_deadline_ms = now_ms + EWF_PVDF_SETTLING_BLIND_MS;
}

bool ewf_pvdf_confirm_policy_on_wake_event(ewf_pvdf_confirm_policy_t *policy,
                                           uint32_t now_ms,
                                           ewf_pvdf_event_t *event)
{
    if (policy == NULL)
    {
        return false;
    }

    if (ewf_pvdf_confirm_policy_blind_window_open(policy, now_ms))
    {
        /* 阈值节点未建立且 TLV7042 可能仍为上电高阻，本事件不得产生任何候选。 */
        ++policy->wake_event_count;
        return publish_terminal(policy,
                                EWF_PVDF_EVENT_REJECTED_BLIND_WINDOW,
                                0,
                                0U,
                                now_ms,
                                event);
    }
    policy->blind_window_open = false;

    if (policy->confirm_in_flight)
    {
        /* AD-7：可合并事件可丢最新，既不排队堆积也不延长在飞确认的窗口。 */
        return false;
    }

    ++policy->wake_event_count;
    policy->confirm_in_flight = true;
    policy->confirm_started_ms = now_ms;
    policy->confirm_deadline_ms = now_ms + EWF_PVDF_CONFIRM_WINDOW_MS;
    policy->peak_millivolt = 0;
    policy->sample_count = 0U;
    policy->out_of_range_seen = false;
    policy->sample_failed_seen = false;
    return false;
}

bool ewf_pvdf_confirm_policy_on_sample(ewf_pvdf_confirm_policy_t *policy,
                                       int32_t millivolt,
                                       ewf_pvdf_sample_status_t status,
                                       uint32_t now_ms,
                                       ewf_pvdf_event_t *event)
{
    if (policy == NULL || !policy->confirm_in_flight)
    {
        return false;
    }
    if (policy->sample_count >= EWF_PVDF_CONFIRM_MAX_SAMPLES)
    {
        return false;
    }

    ++policy->sample_count;
    switch (status)
    {
    case EWF_PVDF_SAMPLE_VALID:
        /* 单电源缓冲把负半周钳在 0 V，判定只取正向峰值。 */
        if (millivolt > policy->peak_millivolt)
        {
            policy->peak_millivolt = millivolt;
        }
        break;
    case EWF_PVDF_SAMPLE_OUT_OF_RANGE:
        policy->out_of_range_seen = true;
        break;
    case EWF_PVDF_SAMPLE_FAILED:
    default:
        policy->sample_failed_seen = true;
        break;
    }

    if (status == EWF_PVDF_SAMPLE_OUT_OF_RANGE)
    {
        /* 越界读数不重试、不放大、不硬扛：立即判本次唤醒无效。 */
        return publish_terminal(policy,
                                EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE,
                                policy->peak_millivolt,
                                policy->sample_count,
                                now_ms,
                                event);
    }
    if (policy->sample_count >= EWF_PVDF_CONFIRM_MAX_SAMPLES ||
        now_ms >= policy->confirm_deadline_ms)
    {
        return resolve_confirmation(policy, now_ms, event);
    }
    return false;
}

bool ewf_pvdf_confirm_policy_on_deadline(ewf_pvdf_confirm_policy_t *policy,
                                         uint32_t now_ms,
                                         ewf_pvdf_event_t *event)
{
    if (policy == NULL || !policy->confirm_in_flight)
    {
        return false;
    }
    if (now_ms < policy->confirm_deadline_ms)
    {
        return false;
    }
    return resolve_confirmation(policy, now_ms, event);
}

uint32_t ewf_pvdf_confirm_policy_deadline_ms(
    const ewf_pvdf_confirm_policy_t *policy)
{
    if (policy == NULL || !policy->confirm_in_flight)
    {
        return 0U;
    }
    return policy->confirm_deadline_ms;
}

bool ewf_pvdf_confirm_policy_blind_window_open(
    const ewf_pvdf_confirm_policy_t *policy,
    uint32_t now_ms)
{
    if (policy == NULL || !policy->blind_window_open)
    {
        return false;
    }
    return now_ms < policy->blind_deadline_ms;
}

const char *ewf_pvdf_event_name(ewf_pvdf_event_kind_t kind)
{
    switch (kind)
    {
    case EWF_PVDF_EVENT_CANDIDATE_TAP:
        return "CANDIDATE_TAP";
    case EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD:
        return "REJECTED_BELOW_THRESHOLD";
    case EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE:
        return "REJECTED_OUT_OF_RANGE";
    case EWF_PVDF_EVENT_REJECTED_BLIND_WINDOW:
        return "REJECTED_BLIND_WINDOW";
    case EWF_PVDF_EVENT_REJECTED_SAMPLE_FAILED:
        return "REJECTED_SAMPLE_FAILED";
    default:
        return "NONE";
    }
}

size_t ewf_pvdf_selfcheck_case_count(void)
{
    return (size_t)EWF_PVDF_SELFCHECK_SCENARIO_COUNT;
}

const ewf_pvdf_selfcheck_case_t *ewf_pvdf_selfcheck_cases(size_t *count)
{
    if (count != NULL)
    {
        *count = (size_t)EWF_PVDF_SELFCHECK_SCENARIO_COUNT;
    }
    return s_selfcheck_cases;
}

void ewf_pvdf_selfcheck_replay(ewf_pvdf_confirm_policy_t *scratch,
                               ewf_pvdf_selfcheck_counts_t *out_counts)
{
    ewf_pvdf_confirm_policy_t local_policy;
    ewf_pvdf_confirm_policy_t *policy = scratch != NULL ? scratch : &local_policy;

    if (out_counts != NULL)
    {
        memset(out_counts, 0, sizeof(*out_counts));
    }
    for (size_t index = 0U; index < (size_t)EWF_PVDF_SELFCHECK_SCENARIO_COUNT; ++index)
    {
        replay_scenario(policy, (ewf_pvdf_selfcheck_scenario_t)index, out_counts);
    }
}

bool ewf_pvdf_selfcheck_expectations_met(
    const ewf_pvdf_selfcheck_counts_t *counts)
{
    if (counts == NULL || counts->total_false_triggers != 0U)
    {
        return false;
    }

    for (size_t index = 0U; index < (size_t)EWF_PVDF_SELFCHECK_SCENARIO_COUNT; ++index)
    {
        const ewf_pvdf_selfcheck_case_t *expected = &s_selfcheck_cases[index];
        if (counts->wake_event_count[index] != expected->expected_wake_events ||
            counts->candidate_count[index] != expected->expected_candidates ||
            counts->rejection_count[index] != expected->expected_rejections ||
            counts->false_trigger_count[index] != 0U)
        {
            return false;
        }

        /* 每个类别必须至少出现一次自己的主导结论，否则明细计数可被误读为通过。 */
        switch (expected->expected_kind)
        {
        case EWF_PVDF_EVENT_CANDIDATE_TAP:
            if (counts->candidate_count[index] == 0U)
            {
                return false;
            }
            break;
        case EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD:
            if (counts->below_threshold_count[index] == 0U)
            {
                return false;
            }
            break;
        case EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE:
            if (counts->out_of_range_count[index] == 0U)
            {
                return false;
            }
            break;
        case EWF_PVDF_EVENT_REJECTED_BLIND_WINDOW:
            if (counts->blind_window_count[index] == 0U)
            {
                return false;
            }
            break;
        case EWF_PVDF_EVENT_REJECTED_SAMPLE_FAILED:
            if (counts->sample_failed_count[index] == 0U)
            {
                return false;
            }
            break;
        default:
            return false;
        }
    }
    return true;
}

static bool publish_terminal(ewf_pvdf_confirm_policy_t *policy,
                             ewf_pvdf_event_kind_t kind,
                             int32_t peak_millivolt,
                             uint32_t sample_count,
                             uint32_t now_ms,
                             ewf_pvdf_event_t *event)
{
    policy->confirm_in_flight = false;
    policy->confirm_deadline_ms = 0U;
    policy->last_kind = kind;

    if (kind == EWF_PVDF_EVENT_CANDIDATE_TAP)
    {
        ++policy->candidate_count;
        policy->last_margin_millivolt = peak_millivolt - EWF_PVDF_CONFIRM_THRESHOLD_MV;
    }
    else
    {
        policy->last_margin_millivolt = 0;
        if ((unsigned)kind < (unsigned)EWF_PVDF_EVENT_COUNT)
        {
            ++policy->rejected_counts[kind];
        }
    }

    if (event != NULL)
    {
        event->kind = kind;
        event->peak_millivolt = peak_millivolt;
        event->margin_millivolt = policy->last_margin_millivolt;
        event->at_ms = now_ms;
        event->sample_count = sample_count;
    }
    return true;
}

static bool resolve_confirmation(ewf_pvdf_confirm_policy_t *policy,
                                 uint32_t now_ms,
                                 ewf_pvdf_event_t *event)
{
    ewf_pvdf_event_kind_t kind = EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD;
    if (policy->out_of_range_seen)
    {
        /* 同一窗口内既超阈又越界时按 fail-closed 取越界拒绝，绝不放大信号。 */
        kind = EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE;
    }
    else if (policy->peak_millivolt > EWF_PVDF_CONFIRM_THRESHOLD_MV)
    {
        kind = EWF_PVDF_EVENT_CANDIDATE_TAP;
    }
    else if (policy->sample_failed_seen)
    {
        kind = EWF_PVDF_EVENT_REJECTED_SAMPLE_FAILED;
    }
    return publish_terminal(policy,
                            kind,
                            policy->peak_millivolt,
                            policy->sample_count,
                            now_ms,
                            event);
}

static int32_t synthesize_millivolt(int32_t peak_millivolt, uint32_t sample_index)
{
    const uint32_t half = EWF_PVDF_CONFIRM_MAX_SAMPLES / 2U;
    if (half == 0U)
    {
        return peak_millivolt;
    }
    if (sample_index <= half)
    {
        return (int32_t)(((int32_t)sample_index * peak_millivolt) / (int32_t)half);
    }
    return (int32_t)(((int32_t)(EWF_PVDF_CONFIRM_MAX_SAMPLES - sample_index) *
                      peak_millivolt) /
                     (int32_t)half);
}

static void record_terminal(ewf_pvdf_selfcheck_scenario_t scenario,
                           const ewf_pvdf_event_t *event,
                           ewf_pvdf_selfcheck_counts_t *counts)
{
    if (event == NULL || counts == NULL)
    {
        return;
    }
    counts->last_kind[scenario] = event->kind;

    switch (event->kind)
    {
    case EWF_PVDF_EVENT_CANDIDATE_TAP:
        ++counts->candidate_count[scenario];
        ++counts->total_candidates;
        if (!s_selfcheck_cases[scenario].is_tap_scenario)
        {
            /* 非敲击样本产出候选即为误触发，必须为零。 */
            ++counts->false_trigger_count[scenario];
            ++counts->total_false_triggers;
        }
        break;
    case EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD:
        ++counts->below_threshold_count[scenario];
        ++counts->rejection_count[scenario];
        break;
    case EWF_PVDF_EVENT_REJECTED_OUT_OF_RANGE:
        ++counts->out_of_range_count[scenario];
        ++counts->rejection_count[scenario];
        break;
    case EWF_PVDF_EVENT_REJECTED_BLIND_WINDOW:
        ++counts->blind_window_count[scenario];
        ++counts->rejection_count[scenario];
        break;
    case EWF_PVDF_EVENT_REJECTED_SAMPLE_FAILED:
        ++counts->sample_failed_count[scenario];
        ++counts->rejection_count[scenario];
        break;
    default:
        break;
    }
}

static void replay_scenario(ewf_pvdf_confirm_policy_t *policy,
                            ewf_pvdf_selfcheck_scenario_t scenario,
                            ewf_pvdf_selfcheck_counts_t *counts)
{
    ewf_pvdf_confirm_policy_reset(policy);
    ewf_pvdf_confirm_policy_arm_runtime(policy, 0U);

    for (size_t index = 0U; index < EWF_PVDF_SELFCHECK_WAKE_COUNT; ++index)
    {
        const ewf_pvdf_selfcheck_wake_t *wake = &s_selfcheck_wakes[index];
        if (wake->scenario != scenario)
        {
            continue;
        }
        if (counts != NULL)
        {
            ++counts->wake_event_count[scenario];
        }

        ewf_pvdf_event_t event = {0};
        if (ewf_pvdf_confirm_policy_on_wake_event(policy, wake->at_ms, &event))
        {
            record_terminal(scenario, &event, counts);
            continue;
        }

        for (uint32_t sample = 0U; sample < EWF_PVDF_CONFIRM_MAX_SAMPLES; ++sample)
        {
            const int32_t millivolt =
                wake->sample_status == EWF_PVDF_SAMPLE_VALID
                    ? synthesize_millivolt(wake->peak_millivolt, sample)
                    : 0;
            const uint32_t at_ms =
                wake->at_ms + sample * EWF_PVDF_CONFIRM_SAMPLE_INTERVAL_MS;
            if (ewf_pvdf_confirm_policy_on_sample(policy,
                                                  millivolt,
                                                  wake->sample_status,
                                                  at_ms,
                                                  &event))
            {
                record_terminal(scenario, &event, counts);
                break;
            }
        }
    }
}
