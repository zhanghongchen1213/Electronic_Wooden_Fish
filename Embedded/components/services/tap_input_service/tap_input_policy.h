/**
 * @file     tap_input_policy.h
 * @brief    统一敲击来源、有限队列与输入闸门的纯逻辑策略。
 * @details  不依赖 ESP-IDF，可在主机测试中验证三类来源的归一、背压、去重和统一 gate。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_TAP_INPUT_POLICY_H
#define EWF_TAP_INPUT_POLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EWF_TAP_INPUT_QUEUE_CAPACITY 16U

typedef enum {
    EWF_TAP_SOURCE_PHYSICAL_PVDF = 0,
    EWF_TAP_SOURCE_DEVICE_TOUCH,
    EWF_TAP_SOURCE_AUTOMATIC_TAP,
    EWF_TAP_SOURCE_COUNT,
} ewf_tap_source_t;

typedef enum {
    EWF_TAP_REASON_NONE = 0,
    EWF_TAP_REASON_ACCEPTED,
    EWF_TAP_REASON_INVALID_SOURCE,
    EWF_TAP_REASON_INVALID_EVENT,
    EWF_TAP_REASON_SERVICE_NOT_READY,
    EWF_TAP_REASON_QUEUE_FULL,
    EWF_TAP_REASON_COMPLETED,
    EWF_TAP_REASON_FAULT_LOCKED,
    EWF_TAP_REASON_SCREEN_OFF_WAKE_ONLY,
    EWF_TAP_REASON_TOUCH_OUTSIDE_WOOD_FISH,
    EWF_TAP_REASON_DUPLICATE_SEQUENCE,
    EWF_TAP_REASON_CANDIDATE_REJECTED,
} ewf_tap_reason_t;

typedef struct {
    ewf_tap_source_t source;
    uint32_t sequence;
    uint32_t at_ms;
    bool candidate_confirmed;
    bool screen_on;
    bool wood_fish_hit;
    bool wake_only;
    bool control_event;
} ewf_tap_event_t;

typedef struct {
    bool service_ready;
    bool completed;
    bool fault_locked;
    bool queue_full;
} ewf_tap_gate_state_t;


typedef struct {
    bool accepted;
    bool valid_tap;
    ewf_tap_reason_t reason;
} ewf_tap_decision_t;

typedef struct {
    ewf_tap_event_t items[EWF_TAP_INPUT_QUEUE_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
    uint32_t last_sequence;
    bool has_sequence;
} ewf_tap_queue_t;


void ewf_tap_queue_init(ewf_tap_queue_t *queue);
ewf_tap_decision_t ewf_tap_gate(const ewf_tap_event_t *event,
                                const ewf_tap_gate_state_t *state);
ewf_tap_decision_t ewf_tap_queue_push(ewf_tap_queue_t *queue,
                                      const ewf_tap_event_t *event,
                                      const ewf_tap_gate_state_t *state);
bool ewf_tap_queue_pop(ewf_tap_queue_t *queue, ewf_tap_event_t *event);
size_t ewf_tap_queue_size(const ewf_tap_queue_t *queue);
const char *ewf_tap_source_name(ewf_tap_source_t source);
const char *ewf_tap_reason_name(ewf_tap_reason_t reason);

#ifdef __cplusplus
}
#endif

#endif
