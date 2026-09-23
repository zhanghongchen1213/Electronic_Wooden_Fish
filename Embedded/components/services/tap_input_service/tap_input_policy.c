/**
 * @file     tap_input_policy.c
 * @brief    统一敲击输入的纯逻辑队列与闸门实现。
 * @details  队列固定容量且保持 FIFO；该模块不推进累计、游标或同步状态。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "tap_input_policy.h"

#include <string.h>

static ewf_tap_decision_t decision(bool accepted, bool valid_tap,
                                   ewf_tap_reason_t reason)
{
    const ewf_tap_decision_t result = {
        .accepted = accepted,
        .valid_tap = valid_tap,
        .reason = reason,
    };
    return result;
}

void ewf_tap_queue_init(ewf_tap_queue_t *queue)
{
    if (queue != NULL) {
        memset(queue, 0, sizeof(*queue));
    }
}

ewf_tap_decision_t ewf_tap_gate(const ewf_tap_event_t *event,
                                const ewf_tap_gate_state_t *state)
{
    if (event == NULL || state == NULL) {
        return decision(false, false, EWF_TAP_REASON_INVALID_EVENT);
    }
    if ((unsigned)event->source >= (unsigned)EWF_TAP_SOURCE_COUNT ||
        event->sequence == 0U) {
        return decision(false, false, EWF_TAP_REASON_INVALID_EVENT);
    }
    if (!state->service_ready) {
        return decision(false, false, EWF_TAP_REASON_SERVICE_NOT_READY);
    }
    if (state->completed) {
        return decision(false, false, EWF_TAP_REASON_COMPLETED);
    }
    if (state->fault_locked) {
        return decision(false, false, EWF_TAP_REASON_FAULT_LOCKED);
    }
    if (state->queue_full) {
        return decision(false, false, EWF_TAP_REASON_QUEUE_FULL);
    }
    if (event->control_event) {
        return decision(false, false, EWF_TAP_REASON_INVALID_EVENT);
    }
    if (event->source == EWF_TAP_SOURCE_PHYSICAL_PVDF &&
        !event->candidate_confirmed) {
        return decision(false, false, EWF_TAP_REASON_CANDIDATE_REJECTED);
    }
    if (event->source == EWF_TAP_SOURCE_DEVICE_TOUCH) {
        if (event->wake_only || !event->screen_on) {
            return decision(false, false, EWF_TAP_REASON_SCREEN_OFF_WAKE_ONLY);
        }
        if (!event->wood_fish_hit) {
            return decision(false, false, EWF_TAP_REASON_TOUCH_OUTSIDE_WOOD_FISH);
        }
    }
    return decision(true, true, EWF_TAP_REASON_ACCEPTED);
}

ewf_tap_decision_t ewf_tap_queue_push(ewf_tap_queue_t *queue,
                                      const ewf_tap_event_t *event,
                                      const ewf_tap_gate_state_t *state)
{
    if (queue == NULL || event == NULL || state == NULL) {
        return decision(false, false, EWF_TAP_REASON_INVALID_EVENT);
    }
    if (queue->has_sequence && event->sequence <= queue->last_sequence) {
        return decision(false, false, EWF_TAP_REASON_DUPLICATE_SEQUENCE);
    }
    if (queue->count >= EWF_TAP_INPUT_QUEUE_CAPACITY) {
        return decision(false, false, EWF_TAP_REASON_QUEUE_FULL);
    }
    const ewf_tap_decision_t gate = ewf_tap_gate(event, state);
    if (!gate.accepted) {
        return gate;
    }
    queue->items[queue->tail] = *event;
    queue->tail = (queue->tail + 1U) % EWF_TAP_INPUT_QUEUE_CAPACITY;
    ++queue->count;
    queue->last_sequence = event->sequence;
    queue->has_sequence = true;
    return gate;
}

bool ewf_tap_queue_pop(ewf_tap_queue_t *queue, ewf_tap_event_t *event)
{
    if (queue == NULL || event == NULL || queue->count == 0U) {
        return false;
    }
    *event = queue->items[queue->head];
    queue->head = (queue->head + 1U) % EWF_TAP_INPUT_QUEUE_CAPACITY;
    --queue->count;
    return true;
}

size_t ewf_tap_queue_size(const ewf_tap_queue_t *queue)
{
    return queue == NULL ? 0U : queue->count;
}


const char *ewf_tap_source_name(ewf_tap_source_t source)
{
    static const char *const names[] = {"physical_pvdf", "device_touch", "automatic_tap"};
    return (unsigned)source < (unsigned)EWF_TAP_SOURCE_COUNT ? names[source] : "unknown";
}

const char *ewf_tap_reason_name(ewf_tap_reason_t reason)
{
    static const char *const names[] = {
        "none", "accepted", "invalid_source", "invalid_event", "service_not_ready",
        "queue_full", "completed", "fault_locked", "screen_off_wake_only",
        "touch_outside_wood_fish", "duplicate_sequence", "candidate_rejected",
    };
    return (unsigned)reason <= (unsigned)EWF_TAP_REASON_CANDIDATE_REJECTED
               ? names[reason]
               : "unknown";
}
