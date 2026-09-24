/**
 * @file     tap_input_service.c
 * @brief    统一有效敲击 typed queue 服务实现。
 * @details  投递时执行统一 gate，任务逐个消费并发布 valid_tap 事件；不修改正式累计。
 * @author   ZHC
 * @date     2026-09-22
 */

#include "tap_input_service.h"

#include <string.h>

#include "esp_log.h"
#include "event_bus.h"
#include "cst9217_bsp.h"
#include "device_nav_service.h"
#include "device_nav_policy.h"
#include "legbot_services.h"
#include "freertos/semphr.h"
#include "state_service.h"
#include "app_state.h"
#include "ui_jingwen_gesture_policy.h"

static const char *TAG = "SVC_TAP";

#define EWF_TAP_INPUT_QUEUE_DEPTH EWF_TAP_INPUT_QUEUE_CAPACITY
#define EWF_TAP_VALID_EVENT_PUBLISH_TIMEOUT_MS 10U

typedef enum {
    TAP_INPUT_MESSAGE_EVENT = 0,
    TAP_INPUT_MESSAGE_TOUCH,
    TAP_INPUT_MESSAGE_STOP,
} tap_input_message_kind_t;

typedef struct {
    tap_input_message_kind_t kind;
    ewf_tap_event_t event;
} tap_input_message_t;

static QueueHandle_t s_queue;
static SemaphoreHandle_t s_mutex;
static ewf_tap_input_snapshot_t s_snapshot;
static uint32_t s_last_sequence;
static bool s_has_sequence;
static bool s_prepared;
static bool s_running;
static uint32_t s_next_producer_sequence;
/** 手势起点/最近按下点（任务上下文采样；ISR 只投递 TOUCH 消息）。 */
static bool s_gesture_armed;
static int16_t s_gesture_x0;
static int16_t s_gesture_y0;
static int16_t s_gesture_last_x;
static int16_t s_gesture_last_y;
/** 本接触始于熄屏：抬起只收尾，不得手势导航或计数。 */
static bool s_wake_only_contact;

static esp_err_t submit_from_source(ewf_tap_source_t source,
                                    const ewf_tap_event_t *event,
                                    TickType_t timeout_ticks,
                                    ewf_tap_decision_t *decision);
static void record_rejection(const ewf_tap_event_t *event,
                             ewf_tap_reason_t reason,
                             size_t queue_depth);
static void record_acceptance(ewf_tap_source_t source);
static bool load_gate_state(ewf_tap_gate_state_t *state);
static void tap_input_service_touch_isr(void *user_ctx);
static bool touch_is_wood_fish_region(const cst9217_bsp_point_t *point);

esp_err_t tap_input_service_init_contracts(void)
{
    if (s_queue != NULL && s_mutex != NULL) {
        return ESP_OK;
    }
    s_queue = xQueueCreate(EWF_TAP_INPUT_QUEUE_DEPTH, sizeof(tap_input_message_t));
    s_mutex = xSemaphoreCreateMutex();
    if (s_queue == NULL || s_mutex == NULL) {
        if (s_queue != NULL) { vQueueDelete(s_queue); s_queue = NULL; }
        if (s_mutex != NULL) { vSemaphoreDelete(s_mutex); s_mutex = NULL; }
        return ESP_ERR_NO_MEM;
    }
    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.last_reason = EWF_TAP_REASON_NONE;
    s_last_sequence = 0U;
    s_has_sequence = false;
    s_next_producer_sequence = 0U;
    return ESP_OK;
}

esp_err_t tap_input_service_prepare_run(void)
{
    if (s_queue == NULL || s_mutex == NULL || s_running) {
        return ESP_ERR_INVALID_STATE;
    }
    s_prepared = true;
    const esp_err_t touch_registration =
        cst9217_bsp_register_interrupt_callback(tap_input_service_touch_isr, NULL);
    if (touch_registration != ESP_OK)
    {
        ESP_LOGW(TAG, "CST9217 触摸生产路径未登记，错误=%s",
                 esp_err_to_name(touch_registration));
    }
    return ESP_OK;
}

void tap_input_service_cancel_prepared_run(void)
{
    if (!s_running)
    {
        (void)cst9217_bsp_register_interrupt_callback(NULL, NULL);
        s_prepared = false;
    }
}

esp_err_t tap_input_service_deinit_contracts(void)
{
    if (s_running) { return ESP_ERR_INVALID_STATE; }
    (void)cst9217_bsp_register_interrupt_callback(NULL, NULL);
    if (s_queue != NULL) { vQueueDelete(s_queue); s_queue = NULL; }
    if (s_mutex != NULL) { vSemaphoreDelete(s_mutex); s_mutex = NULL; }
    s_last_sequence = 0U;
    s_has_sequence = false;
    s_next_producer_sequence = 0U;
    s_prepared = false;
    return ESP_OK;
}

QueueHandle_t tap_input_service_queue(void) { return s_queue; }

uint32_t tap_input_service_next_sequence(void)
{
    if (s_mutex == NULL || xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return 0U;
    }
    uint32_t sequence = ++s_next_producer_sequence;
    if (sequence == 0U)
    {
        sequence = ++s_next_producer_sequence;
    }
    xSemaphoreGive(s_mutex);
    return sequence;
}

esp_err_t tap_input_service_submit(const ewf_tap_event_t *event,
                                   TickType_t timeout_ticks,
                                   ewf_tap_decision_t *out_decision)
{
    if (event == NULL || s_queue == NULL || s_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    ewf_tap_gate_state_t state = {0};
    const bool state_available = load_gate_state(&state);
    if (!state_available) {
        state.service_ready = false;
    }
    state.queue_full = uxQueueMessagesWaiting(s_queue) >= EWF_TAP_INPUT_QUEUE_DEPTH;
    ewf_tap_decision_t decision = ewf_tap_gate(event, &state);
    if (!s_prepared) {
        decision.accepted = false;
        decision.valid_tap = false;
        decision.reason = EWF_TAP_REASON_SERVICE_NOT_READY;
    }
    if (s_has_sequence && event->sequence <= s_last_sequence) {
        decision.accepted = false;
        decision.valid_tap = false;
        decision.reason = EWF_TAP_REASON_DUPLICATE_SEQUENCE;
    }
    if (!decision.accepted) {
        const size_t queue_depth = uxQueueMessagesWaiting(s_queue);
        record_rejection(event, decision.reason, queue_depth);
        xSemaphoreGive(s_mutex);
        if (out_decision != NULL) { *out_decision = decision; }
        ESP_LOGW(TAG, "敲击输入拒绝：来源=%s，原因=%s，队列深度=%u",
                 ewf_tap_source_name(event->source), ewf_tap_reason_name(decision.reason),
                 (unsigned)queue_depth);
        return decision.reason == EWF_TAP_REASON_QUEUE_FULL ? ESP_ERR_TIMEOUT : ESP_ERR_INVALID_ARG;
    }
    const tap_input_message_t message = {
        .kind = TAP_INPUT_MESSAGE_EVENT,
        .event = *event,
    };
    if (xQueueSend(s_queue, &message, timeout_ticks) != pdTRUE) {
        record_rejection(event, EWF_TAP_REASON_QUEUE_FULL,
                         uxQueueMessagesWaiting(s_queue));
        if (out_decision != NULL) { out_decision->accepted = false; out_decision->valid_tap = false; out_decision->reason = EWF_TAP_REASON_QUEUE_FULL; }
        xSemaphoreGive(s_mutex);
        return ESP_ERR_TIMEOUT;
    }
    s_last_sequence = event->sequence;
    s_has_sequence = true;
    if (out_decision != NULL) { *out_decision = decision; }
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static esp_err_t submit_from_source(ewf_tap_source_t source,
                                    const ewf_tap_event_t *event,
                                    TickType_t timeout_ticks,
                                    ewf_tap_decision_t *decision)
{
    if (event == NULL || event->source != source) {
        if (decision != NULL) {
            decision->accepted = false;
            decision->valid_tap = false;
            decision->reason = EWF_TAP_REASON_INVALID_SOURCE;
        }
        return ESP_ERR_INVALID_ARG;
    }
    return tap_input_service_submit(event, timeout_ticks, decision);
}

esp_err_t tap_input_service_submit_physical_pvdf(const ewf_tap_event_t *event,
                                                 TickType_t timeout_ticks,
                                                 ewf_tap_decision_t *decision)
{
    return submit_from_source(EWF_TAP_SOURCE_PHYSICAL_PVDF, event,
                              timeout_ticks, decision);
}

esp_err_t tap_input_service_submit_device_touch(const ewf_tap_event_t *event,
                                                TickType_t timeout_ticks,
                                                ewf_tap_decision_t *decision)
{
    return submit_from_source(EWF_TAP_SOURCE_DEVICE_TOUCH, event,
                              timeout_ticks, decision);
}

esp_err_t tap_input_service_submit_automatic_tap(const ewf_tap_event_t *event,
                                                 TickType_t timeout_ticks,
                                                 ewf_tap_decision_t *decision)
{
    return submit_from_source(EWF_TAP_SOURCE_AUTOMATIC_TAP, event,
                              timeout_ticks, decision);
}

esp_err_t tap_input_service_run(void)
{
    if (s_queue == NULL || !s_prepared) { return ESP_ERR_INVALID_STATE; }
    s_running = true;
    tap_input_message_t message = {0};
    bool stop_requested = false;
    while (xQueueReceive(s_queue, &message, portMAX_DELAY) == pdTRUE) {
        if (message.kind == TAP_INPUT_MESSAGE_STOP)
        {
            stop_requested = true;
            if (uxQueueMessagesWaiting(s_queue) == 0U)
            {
                break;
            }
            message = (tap_input_message_t){0};
            continue;
        }
        if (message.kind == TAP_INPUT_MESSAGE_TOUCH)
        {
            cst9217_bsp_point_t point = {0};
            if (cst9217_bsp_read_point(&point) != ESP_OK)
            {
                ESP_LOGW(TAG, "CST9217 触摸点读取失败");
                message = (tap_input_message_t){0};
                continue;
            }
            const uint32_t touch_ms = (uint32_t)pdTICKS_TO_MS(xTaskGetTickCount());
            if (!point.pressed) {
                /*
                 * 抬起：使用 BSP 保留的抬起坐标（见 cst9217_bsp_read_point）；
                 * 若仍为全零则回退最近按下采样，避免误判为向原点滑动。
                 */
                if (s_gesture_armed) {
                    const bool wake_only = s_wake_only_contact;
                    const int16_t end_x =
                        (point.x == 0U && point.y == 0U)
                            ? s_gesture_last_x
                            : (int16_t)point.x;
                    const int16_t end_y =
                        (point.x == 0U && point.y == 0U)
                            ? s_gesture_last_y
                            : (int16_t)point.y;
                    const ewf_nav_input_kind_t gesture = ewf_nav_classify_gesture(
                        s_gesture_x0, s_gesture_y0, end_x, end_y);
                    s_gesture_armed = false;
                    s_wake_only_contact = false;
                    if (wake_only) {
                        /* 熄屏首触抬起：只收尾，不导航不计数。 */
                        message = (tap_input_message_t){0};
                        if (stop_requested &&
                            uxQueueMessagesWaiting(s_queue) == 0U) {
                            break;
                        }
                        continue;
                    }
                    if (gesture != EWF_NAV_INPUT_NONE) {
                        const bool vertical =
                            (gesture == EWF_NAV_INPUT_SWIPE_DOWN ||
                             gesture == EWF_NAV_INPUT_SWIPE_UP);
                        /* 裁决 H：JINGWEN + history 视口内垂直拖动交给 LVGL 滚动，不开设置。 */
                        const bool suppress = ewf_ui_jingwen_suppress_vertical_nav(
                            device_nav_service_active_page() == EWF_NAV_PAGE_JINGWEN,
                            device_nav_service_settings_open(),
                            s_gesture_x0,
                            s_gesture_y0,
                            vertical);
                        if (!suppress) {
                            const esp_err_t nav_error =
                                device_nav_service_on_nav_input(gesture, touch_ms);
                            if (nav_error != ESP_OK) {
                                ESP_LOGW(TAG, "手势导航投递失败：kind=%d，错误=%s",
                                         (int)gesture, esp_err_to_name(nav_error));
                            }
                        }
                        message = (tap_input_message_t){0};
                        if (stop_requested &&
                            uxQueueMessagesWaiting(s_queue) == 0U) {
                            break;
                        }
                        continue;
                    }
                    /* 无滑动 = 点按：仅木鱼页触区才正式计数。 */
                    cst9217_bsp_point_t tap_point = {
                        .x = (uint16_t)s_gesture_last_x,
                        .y = (uint16_t)s_gesture_last_y,
                        .pressed = false,
                    };
                    const bool on_muyu =
                        device_nav_service_active_page() == EWF_NAV_PAGE_MUYU;
                    const bool wood_hit =
                        device_nav_service_screen_on() && on_muyu &&
                        touch_is_wood_fish_region(&tap_point) &&
                        !device_nav_service_settings_open();
                    if (wood_hit) {
                        const ewf_tap_event_t touch = {
                            .source = EWF_TAP_SOURCE_DEVICE_TOUCH,
                            .sequence = tap_input_service_next_sequence(),
                            .at_ms = touch_ms,
                            .candidate_confirmed = true,
                            .screen_on = true,
                            .wood_fish_hit = true,
                            .wake_only = false,
                        };
                        ewf_tap_decision_t decision = {0};
                        (void)tap_input_service_submit_device_touch(
                            &touch,
                            pdMS_TO_TICKS(EWF_TAP_VALID_EVENT_PUBLISH_TIMEOUT_MS),
                            &decision);
                        if (decision.accepted) {
                            (void)device_nav_service_note_activity(touch_ms);
                        }
                    } else {
                        (void)device_nav_service_note_activity(touch_ms);
                    }
                }
                message = (tap_input_message_t){0};
                continue;
            }
            if (!s_gesture_armed) {
                s_gesture_armed = true;
                s_gesture_x0 = (int16_t)point.x;
                s_gesture_y0 = (int16_t)point.y;
                s_gesture_last_x = s_gesture_x0;
                s_gesture_last_y = s_gesture_y0;
            } else {
                s_gesture_last_x = (int16_t)point.x;
                s_gesture_last_y = (int16_t)point.y;
            }
            const bool screen_on = device_nav_service_screen_on();
            if (!screen_on) {
                /* 熄屏首触：只唤醒，不计数；抬起时禁止手势导航。 */
                s_wake_only_contact = true;
                const esp_err_t wake_error = device_nav_service_on_nav_input(
                    EWF_NAV_INPUT_TOUCH_WAKE, touch_ms);
                if (wake_error != ESP_OK) {
                    ESP_LOGW(TAG, "熄屏唤醒投递失败：错误=%s",
                             esp_err_to_name(wake_error));
                }
                const ewf_tap_event_t wake = {
                    .source = EWF_TAP_SOURCE_DEVICE_TOUCH,
                    .sequence = tap_input_service_next_sequence(),
                    .at_ms = touch_ms,
                    .candidate_confirmed = true,
                    .screen_on = false,
                    .wood_fish_hit = false,
                    .wake_only = true,
                };
                ewf_tap_decision_t decision = {0};
                (void)tap_input_service_submit_device_touch(
                    &wake, pdMS_TO_TICKS(EWF_TAP_VALID_EVENT_PUBLISH_TIMEOUT_MS),
                    &decision);
                message = (tap_input_message_t){0};
                if (stop_requested && uxQueueMessagesWaiting(s_queue) == 0U) {
                    break;
                }
                continue;
            }
            /* 亮屏按下：只跟踪轨迹并重置空闲；正式敲击延后到抬起确认非滑动。 */
            (void)device_nav_service_note_activity(touch_ms);
            message = (tap_input_message_t){0};
            if (stop_requested && uxQueueMessagesWaiting(s_queue) == 0U)
            {
                break;
            }
            continue;
        }
        ewf_tap_gate_state_t state = {0};
        const bool state_available = load_gate_state(&state);
        if (!state_available) {
            state.service_ready = false;
        }
        const ewf_tap_decision_t gate = ewf_tap_gate(&message.event, &state);
        if (!gate.accepted) {
            if (s_mutex != NULL && xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE) {
                record_rejection(&message.event, gate.reason,
                                 uxQueueMessagesWaiting(s_queue));
                xSemaphoreGive(s_mutex);
            }
            ESP_LOGW(TAG, "消费前 gate 拒绝：来源=%s，原因=%s",
                     ewf_tap_source_name(message.event.source),
                     ewf_tap_reason_name(gate.reason));
            continue;
        }
        const legbot_event_t valid = {
            .source = LEGBOT_EVENT_SOURCE_STATE,
            .type = LEGBOT_EVENT_TYPE_SIGNAL,
            .code = EWF_TAP_EVENT_VALID,
            .value = message.event.sequence,
        };
        const esp_err_t error = event_bus_publish(
            &valid, pdMS_TO_TICKS(EWF_TAP_VALID_EVENT_PUBLISH_TIMEOUT_MS));
        if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE) {
            if (error == ESP_OK) {
                ++s_snapshot.accepted_count;
                s_snapshot.last_reason = EWF_TAP_REASON_ACCEPTED;
                record_acceptance(message.event.source);
                /* 有效敲击（含 PVDF）重置自动熄屏空闲计时。 */
                (void)device_nav_service_note_activity(message.event.at_ms);
            } else {
                const ewf_tap_reason_t reason =
                    error == ESP_ERR_TIMEOUT ? EWF_TAP_REASON_QUEUE_FULL
                                             : EWF_TAP_REASON_SERVICE_NOT_READY;
                record_rejection(&message.event,
                                 reason,
                                 uxQueueMessagesWaiting(s_queue));
                ESP_LOGW(TAG, "有效敲击事件发布失败：序号=%lu，错误=%s",
                         (unsigned long)message.event.sequence,
                         esp_err_to_name(error));
            }
            s_snapshot.last_source = message.event.source;
            s_snapshot.queue_depth = uxQueueMessagesWaiting(s_queue);
            xSemaphoreGive(s_mutex);
        }
        if (stop_requested && uxQueueMessagesWaiting(s_queue) == 0U)
        {
            break;
        }
    }
    (void)cst9217_bsp_register_interrupt_callback(NULL, NULL);
    s_running = false;
    s_prepared = false;
    return ESP_OK;
}

esp_err_t tap_input_service_request_stop(TickType_t timeout_ticks)
{
    if (s_queue == NULL || !s_prepared) { return ESP_ERR_INVALID_STATE; }
    const tap_input_message_t stop = {.kind = TAP_INPUT_MESSAGE_STOP};
    return xQueueSend(s_queue, &stop, timeout_ticks) == pdTRUE ? ESP_OK : ESP_ERR_TIMEOUT;
}

esp_err_t tap_input_service_snapshot(ewf_tap_input_snapshot_t *snapshot)
{
    if (snapshot == NULL || s_mutex == NULL) { return ESP_ERR_INVALID_ARG; }
    if (xSemaphoreTake(s_mutex, 0) != pdTRUE) { return ESP_ERR_TIMEOUT; }
    *snapshot = s_snapshot;
    snapshot->queue_depth = uxQueueMessagesWaiting(s_queue);
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static bool load_gate_state(ewf_tap_gate_state_t *state)
{
    if (state == NULL) {
        return false;
    }
    bool service_ready = false;
    bool completed = false;
    bool fault_locked = false;
    bool queue_full = false;
    const esp_err_t error = state_service_read_tap_gate(&service_ready,
                                                        &completed,
                                                        &fault_locked,
                                                        &queue_full);
    state->service_ready = error == ESP_OK && service_ready;
    state->completed = completed;
    state->fault_locked = fault_locked;
    /* queue_full 是离线积压 owner 发布的持久化事实，输入侧只读不私判。 */
    state->queue_full = queue_full;
    return error == ESP_OK;
}

static void record_rejection(const ewf_tap_event_t *event,
                             ewf_tap_reason_t reason,
                             size_t queue_depth)
{
    ++s_snapshot.rejected_count;
    if (reason == EWF_TAP_REASON_QUEUE_FULL) {
        ++s_snapshot.queue_full_count;
    }
    s_snapshot.last_reason = reason;
    s_snapshot.last_source = event != NULL ? event->source : EWF_TAP_SOURCE_COUNT;
    s_snapshot.queue_depth = queue_depth;
    if (event != NULL && (unsigned)event->source < (unsigned)EWF_TAP_SOURCE_COUNT) {
        ewf_tap_source_snapshot_t *source = &s_snapshot.by_source[event->source];
        ++source->rejected_count;
        if (reason == EWF_TAP_REASON_QUEUE_FULL) {
            ++source->queue_full_count;
        }
        source->last_reason = reason;
    }
}

static void record_acceptance(ewf_tap_source_t source)
{
    if ((unsigned)source < (unsigned)EWF_TAP_SOURCE_COUNT) {
        ++s_snapshot.by_source[source].accepted_count;
        s_snapshot.by_source[source].last_reason = EWF_TAP_REASON_ACCEPTED;
    }
}

static void tap_input_service_touch_isr(void *user_ctx)
{
    (void)user_ctx;
    if (s_queue == NULL)
    {
        return;
    }
    const tap_input_message_t message = {.kind = TAP_INPUT_MESSAGE_TOUCH};
    BaseType_t higher_priority_task_woken = pdFALSE;
    (void)xQueueSendFromISR(s_queue, &message, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static bool touch_is_wood_fish_region(const cst9217_bsp_point_t *point)
{
    if (point == NULL)
    {
        return false;
    }
    /*
     * 木鱼触区初值矩形（≥96×96 下限）；仅与 active_page==MUYU 合取后可计数。
     * Epic 3 对拍后可收紧坐标，不得在非木鱼页放宽。
     */
    return point->x >= 80U && point->x <= 330U &&
           point->y >= 110U && point->y <= 390U;
}
