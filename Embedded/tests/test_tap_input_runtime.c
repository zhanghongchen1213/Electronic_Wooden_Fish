/**
 * @file     test_tap_input_runtime.c
 * @brief    真实状态 owner、PVDF 转投与统一敲击服务的主机集成回归。
 * @details  编译实际生产 C 源码，仅替代 RTOS、硬件和下游总线。
 * @author   ZHC
 * @date     2026-09-22
 */
#include <assert.h>
#include <stdio.h>
#include "state_service.h"
#include "tap_input_service.h"
#include "pvdf_input_service.h"
#include "device_nav_policy.h"
#include "device_nav_service.h"

void host_platform_init(void);
void host_state_owner_start(void);
void host_state_owner_stop(void);
esp_err_t host_state_apply_one(void);
void host_pvdf_terminal(ewf_pvdf_event_kind_t kind, uint32_t at_ms);
void host_pvdf_reset_sequence(void);
void host_publish_error(esp_err_t error);
void host_fail_next_send(void);
uint32_t host_published_count(void);
uint32_t host_published_sequence(uint32_t index);
void host_cst9217_set_point(uint16_t x, uint16_t y, bool pressed);
void host_cst9217_reset_points(void);
void host_cst9217_interrupt(void);
void host_device_nav_reset(void);
void host_device_nav_set_page(ewf_nav_page_t page);
void host_device_nav_set_screen_on(bool on);
void host_device_nav_set_settings_open(bool open);
unsigned host_device_nav_input_calls(void);
ewf_nav_input_kind_t host_device_nav_last_kind(void);

static uint32_t s_state_sequence;
static void update_gate(bool completed, bool fault_locked)
{
    const watch_tap_gate_update_t update = {
        .update_sequence = ++s_state_sequence,
        .completed = completed,
        .fault_locked = fault_locked,
    };
    assert(state_service_publish_tap_gate(&update, 0) == ESP_OK);
    assert(host_state_apply_one() == ESP_OK);
}
static void reset_services(void)
{
    assert(tap_input_service_deinit_contracts() == ESP_OK);
    host_platform_init();
    host_device_nav_reset();
    host_cst9217_reset_points();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    update_gate(false, false);
    assert(tap_input_service_init_contracts() == ESP_OK);
    assert(tap_input_service_prepare_run() == ESP_OK);
    assert(pvdf_input_service_init_contracts() == ESP_OK);
    host_pvdf_reset_sequence();
}
static ewf_tap_event_t tap(ewf_tap_source_t source, uint32_t sequence)
{
    return (ewf_tap_event_t){
        .source = source, .sequence = sequence, .at_ms = sequence * 50U,
        .candidate_confirmed = true, .screen_on = true, .wood_fish_hit = true,
    };
}
static ewf_tap_input_snapshot_t snapshot(void)
{
    ewf_tap_input_snapshot_t result = {0};
    assert(tap_input_service_snapshot(&result) == ESP_OK);
    return result;
}
static void consume(void)
{
    assert(tap_input_service_request_stop(0) == ESP_OK);
    assert(tap_input_service_run() == ESP_OK);
    assert(tap_input_service_prepare_run() == ESP_OK);
}
static void test_state_owner_and_producer_gates(void)
{
    reset_services();
    bool ready = false, completed = false, fault = false, queue_full = false;
    assert(state_service_read_tap_gate(&ready, &completed, &fault, &queue_full) == ESP_OK);
    assert(ready && !completed && !fault && !queue_full);
    update_gate(true, false);
    assert(state_service_read_tap_gate(&ready, &completed, &fault, &queue_full) == ESP_OK);
    assert(ready && completed && !fault && !queue_full);
    for (unsigned source = 0U; source < EWF_TAP_SOURCE_COUNT; ++source)
    {
        ewf_tap_event_t event = tap((ewf_tap_source_t)source, source + 1U);
        ewf_tap_decision_t decision = {0};
        assert(tap_input_service_submit(&event, 0, &decision) != ESP_OK);
        assert(decision.reason == EWF_TAP_REASON_COMPLETED);
    }
    update_gate(false, true);
    assert(state_service_read_tap_gate(&ready, &completed, &fault, &queue_full) == ESP_OK);
    assert(ready && !completed && fault && !queue_full);
    for (unsigned source = 0U; source < EWF_TAP_SOURCE_COUNT; ++source)
    {
        ewf_tap_event_t event = tap((ewf_tap_source_t)source, source + 1U);
        ewf_tap_decision_t decision = {0};
        assert(tap_input_service_submit(&event, 0, &decision) != ESP_OK);
        assert(decision.reason == EWF_TAP_REASON_FAULT_LOCKED);
    }
    const watch_tap_gate_update_t stale = {.update_sequence = s_state_sequence};
    assert(state_service_publish_tap_gate(&stale, 0) == ESP_OK);
    assert(host_state_apply_one() == ESP_ERR_INVALID_STATE);
    assert(snapshot().queue_depth == 0U && host_published_count() == 0U);
    host_state_owner_stop();
    assert(state_service_read_tap_gate(&ready, &completed, &fault, &queue_full) == ESP_ERR_INVALID_STATE);
    assert(!ready);
}
static void test_consumer_second_gate(void)
{
    for (unsigned block = 0U; block < 3U; ++block)
    {
        reset_services();
        for (unsigned source = 0U; source < EWF_TAP_SOURCE_COUNT; ++source)
        {
            const ewf_tap_event_t event = tap((ewf_tap_source_t)source, source + 1U);
            assert(tap_input_service_submit(&event, 0, NULL) == ESP_OK);
        }
        if (block == 2U) { host_state_owner_stop(); }
        else { update_gate(block == 0U, block == 1U); }
        consume();
        const ewf_tap_reason_t reason = block == 0U ? EWF_TAP_REASON_COMPLETED :
            block == 1U ? EWF_TAP_REASON_FAULT_LOCKED : EWF_TAP_REASON_SERVICE_NOT_READY;
        assert(host_published_count() == 0U && snapshot().accepted_count == 0U);
        assert(snapshot().rejected_count == 3U);
        for (unsigned source = 0U; source < EWF_TAP_SOURCE_COUNT; ++source)
        {
            assert(snapshot().by_source[source].rejected_count == 1U);
            assert(snapshot().by_source[source].last_reason == reason);
        }
    }
}
static void test_pvdf_transfer_and_diagnostics(void)
{
    reset_services();
    host_pvdf_terminal(EWF_PVDF_EVENT_CANDIDATE_TAP, 100U);
    assert(snapshot().queue_depth == 1U);
    consume();
    assert(host_published_count() == 1U);
    assert(snapshot().by_source[EWF_TAP_SOURCE_PHYSICAL_PVDF].accepted_count == 1U);
    host_pvdf_terminal(EWF_PVDF_EVENT_REJECTED_BELOW_THRESHOLD, 200U);
    assert(snapshot().last_reason == EWF_TAP_REASON_CANDIDATE_REJECTED);
    assert(snapshot().queue_depth == 0U && host_published_count() == 1U);
}

static void test_device_touch_production_bridge(void)
{
    reset_services();
    /* 按下 + 抬起（同坐标）：点按计数；stub 第二次读点为抬起。 */
    host_cst9217_set_point(205U, 250U, true);
    host_cst9217_interrupt();
    host_cst9217_interrupt();
    consume();
    assert(host_published_count() == 1U);
    assert(snapshot().by_source[EWF_TAP_SOURCE_DEVICE_TOUCH].accepted_count == 1U);
}

static void test_device_touch_page_and_wake_gates(void)
{
    reset_services();
    host_device_nav_set_page(EWF_NAV_PAGE_JINGWEN);
    host_cst9217_set_point(205U, 250U, true);
    host_cst9217_interrupt();
    host_cst9217_interrupt();
    consume();
    assert(host_published_count() == 0U);

    reset_services();
    host_device_nav_set_settings_open(true);
    host_cst9217_set_point(205U, 250U, true);
    host_cst9217_interrupt();
    host_cst9217_interrupt();
    consume();
    assert(host_published_count() == 0U);

    reset_services();
    host_device_nav_set_screen_on(false);
    const unsigned before = host_device_nav_input_calls();
    host_cst9217_set_point(205U, 250U, true);
    host_cst9217_interrupt();
    host_cst9217_interrupt();
    consume();
    assert(host_device_nav_last_kind() == EWF_NAV_INPUT_TOUCH_WAKE);
    assert(host_device_nav_input_calls() == before + 1U);
    /* 熄屏首触只唤醒，不产生正式敲击 accepted。 */
    assert(snapshot().by_source[EWF_TAP_SOURCE_DEVICE_TOUCH].accepted_count ==
           0U);
}

static void test_device_touch_swipe_navigates_without_count(void)
{
    reset_services();
    host_cst9217_set_point(205U, 250U, true);
    host_cst9217_interrupt();
    /* 滑动过程中的按下采样（真实驱动会在抬起前上报轨迹）。 */
    host_cst9217_set_point(40U, 250U, true);
    host_cst9217_interrupt();
    host_cst9217_set_point(40U, 250U, false);
    host_cst9217_interrupt();
    consume();
    assert(host_device_nav_last_kind() == EWF_NAV_INPUT_SWIPE_LEFT);
    assert(host_published_count() == 0U);
    assert(device_nav_service_active_page() == EWF_NAV_PAGE_JINGWEN);
}

static void test_jingwen_history_vertical_swipe_suppresses_settings(void)
{
    reset_services();
    host_device_nav_set_page(EWF_NAV_PAGE_JINGWEN);
    host_device_nav_set_settings_open(false);

    /* history 内起点 (100,200)：垂直下滑不得开设置。 */
    const unsigned before_in = host_device_nav_input_calls();
    host_cst9217_set_point(100U, 200U, true);
    host_cst9217_interrupt();
    host_cst9217_set_point(100U, 280U, true);
    host_cst9217_interrupt();
    host_cst9217_set_point(100U, 280U, false);
    host_cst9217_interrupt();
    consume();
    assert(host_device_nav_input_calls() == before_in);
    assert(!device_nav_service_settings_open());

    /* history 外起点 (100,70) 标题带：垂直下滑应投递 SWIPE_DOWN 并开设置。 */
    const unsigned before_out = host_device_nav_input_calls();
    host_cst9217_set_point(100U, 70U, true);
    host_cst9217_interrupt();
    host_cst9217_set_point(100U, 150U, true);
    host_cst9217_interrupt();
    host_cst9217_set_point(100U, 150U, false);
    host_cst9217_interrupt();
    consume();
    assert(host_device_nav_input_calls() == before_out + 1U);
    assert(host_device_nav_last_kind() == EWF_NAV_INPUT_SWIPE_DOWN);
    assert(device_nav_service_settings_open());
}

static void test_three_sources_backpressure_and_downstream(void)
{
    reset_services();
    for (uint32_t index = 0U; index < 20U; ++index)
    {
        const ewf_tap_event_t event = tap((ewf_tap_source_t)(index % 3U), index + 1U);
        assert(tap_input_service_submit(&event, 0, NULL) == ESP_OK);
        consume();
        assert(host_published_sequence(index) == index + 1U);
    }
    assert(snapshot().accepted_count == 20U && host_published_count() == 20U);
    for (unsigned source = 0U; source < EWF_TAP_SOURCE_COUNT; ++source)
    {
        const ewf_tap_event_t event = tap((ewf_tap_source_t)source, 21U + source);
        assert(tap_input_service_submit(&event, 0, NULL) == ESP_OK);
    }
    host_publish_error(ESP_ERR_TIMEOUT);
    consume();
    assert(snapshot().queue_full_count == 3U && snapshot().rejected_count == 3U);
    host_publish_error(ESP_ERR_INVALID_STATE);
    const ewf_tap_event_t failed = tap(EWF_TAP_SOURCE_DEVICE_TOUCH, 24U);
    assert(tap_input_service_submit_device_touch(&failed, 0, NULL) == ESP_OK);
    consume();
    assert(snapshot().by_source[1].last_reason == EWF_TAP_REASON_SERVICE_NOT_READY);
    reset_services();
    const ewf_tap_event_t retry = tap(EWF_TAP_SOURCE_AUTOMATIC_TAP, 1U);
    host_fail_next_send();
    assert(tap_input_service_submit_automatic_tap(&retry, 0, NULL) == ESP_ERR_TIMEOUT);
    assert(tap_input_service_submit_automatic_tap(&retry, 0, NULL) == ESP_OK);
}
int main(void)
{
    test_state_owner_and_producer_gates();
    test_consumer_second_gate();
    test_pvdf_transfer_and_diagnostics();
    test_device_touch_production_bridge();
    test_device_touch_page_and_wake_gates();
    test_device_touch_swipe_navigates_without_count();
    test_jingwen_history_vertical_swipe_suppresses_settings();
    test_three_sources_backpressure_and_downstream();
    assert(tap_input_service_deinit_contracts() == ESP_OK);
    assert(pvdf_input_service_deinit_contracts() == ESP_OK);
    puts("tap-runtime: 真实 state_service/watch_state/PVDF/tap_input_service 集成回归全部通过");
    return 0;
}
