/**
 * @file     test_tap_input_policy.c
 * @brief    统一敲击输入策略的主机测试。
 * @details  覆盖三来源归一、顺序、统一 gate、重复序号与固定容量背压。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "tap_input_policy.h"

#include <assert.h>
#include <stdio.h>

static ewf_tap_event_t event(ewf_tap_source_t source, uint32_t sequence)
{
    return (ewf_tap_event_t){
        .source = source,
        .sequence = sequence,
        .at_ms = sequence * 50U,
        .candidate_confirmed = true,
        .screen_on = true,
        .wood_fish_hit = true,
    };
}

static ewf_tap_gate_state_t ready(void)
{
    return (ewf_tap_gate_state_t){.service_ready = true};
}

static void test_three_sources_and_fifo(void)
{
    ewf_tap_queue_t queue;
    ewf_tap_event_t received;
    const ewf_tap_gate_state_t state = ready();
    ewf_tap_queue_init(&queue);

    for (uint32_t index = 0; index < 3U; ++index) {
        const ewf_tap_event_t input = event((ewf_tap_source_t)index, index + 1U);
        const ewf_tap_decision_t decision = ewf_tap_queue_push(&queue, &input, &state);
        assert(decision.accepted && decision.valid_tap);
    }
    for (uint32_t index = 0; index < 3U; ++index) {
        assert(ewf_tap_queue_pop(&queue, &received));
        assert(received.sequence == index + 1U);
        assert(received.source == (ewf_tap_source_t)index);
    }
    assert(ewf_tap_queue_size(&queue) == 0U);
}

static void test_gate_matrix_does_not_mutate_queue(void)
{
    ewf_tap_queue_t queue;
    const ewf_tap_event_t input = event(EWF_TAP_SOURCE_AUTOMATIC_TAP, 1U);
    ewf_tap_gate_state_t state = ready();
    ewf_tap_queue_init(&queue);

    state.completed = true;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_COMPLETED);
    state.completed = false;
    state.fault_locked = true;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_FAULT_LOCKED);
    state.fault_locked = false;
    state.service_ready = false;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_SERVICE_NOT_READY);
    assert(ewf_tap_queue_size(&queue) == 0U);
}

static void test_touch_and_candidate_boundaries(void)
{
    ewf_tap_queue_t queue;
    ewf_tap_gate_state_t state = ready();
    ewf_tap_event_t input = event(EWF_TAP_SOURCE_DEVICE_TOUCH, 1U);
    ewf_tap_queue_init(&queue);

    input.wake_only = true;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_SCREEN_OFF_WAKE_ONLY);
    input.wake_only = false;
    input.wood_fish_hit = false;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_TOUCH_OUTSIDE_WOOD_FISH);
    input = event(EWF_TAP_SOURCE_PHYSICAL_PVDF, 1U);
    input.candidate_confirmed = false;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_CANDIDATE_REJECTED);
    input.control_event = true;
    input.candidate_confirmed = true;
    assert(ewf_tap_queue_push(&queue, &input, &state).reason == EWF_TAP_REASON_INVALID_EVENT);
    assert(ewf_tap_queue_size(&queue) == 0U);
}

static void test_duplicate_and_backpressure(void)
{
    ewf_tap_queue_t queue;
    const ewf_tap_gate_state_t state = ready();
    ewf_tap_queue_init(&queue);
    for (uint32_t sequence = 1U; sequence <= EWF_TAP_INPUT_QUEUE_CAPACITY; ++sequence) {
        const ewf_tap_event_t input = event(EWF_TAP_SOURCE_AUTOMATIC_TAP, sequence);
        assert(ewf_tap_queue_push(&queue, &input, &state).accepted);
    }
    const ewf_tap_event_t full_input = event(EWF_TAP_SOURCE_AUTOMATIC_TAP,
                                             EWF_TAP_INPUT_QUEUE_CAPACITY + 1U);
    assert(ewf_tap_queue_push(&queue, &full_input, &state).reason == EWF_TAP_REASON_QUEUE_FULL);
    const ewf_tap_event_t duplicate_input = event(EWF_TAP_SOURCE_AUTOMATIC_TAP,
                                                  EWF_TAP_INPUT_QUEUE_CAPACITY);
    assert(ewf_tap_queue_push(&queue, &duplicate_input,
                              &state).reason == EWF_TAP_REASON_DUPLICATE_SEQUENCE);
}

int main(void)
{
    test_three_sources_and_fifo();
    test_gate_matrix_does_not_mutate_queue();
    test_touch_and_candidate_boundaries();
    test_duplicate_and_backpressure();
    puts("tap-policy: 全部通过");
    return 0;
}
