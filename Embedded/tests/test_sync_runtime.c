/**
 * @file     test_sync_runtime.c
 * @brief    sync_service 与 progress/nav 接线主机测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "device_nav_service.h"
#include "device_settings_policy.h"
#include "state_service.h"
#include "sync_https_mock.h"
#include "sync_service.h"

void host_platform_init(void);
void host_state_owner_start(void);
esp_err_t host_state_apply_one(void);
void host_settings_store_reset(void);
void host_co5300_reset(void);
void host_sync_runtime_reset(void);
uint32_t host_sync_runtime_acked_total(void);
uint32_t host_sync_runtime_advance_calls(void);
uint8_t host_sync_runtime_last_volume(void);
void host_sync_set_persist_status(bool pending, bool inflight);

static void drain_state(void)
{
    for (;;) {
        const esp_err_t error = host_state_apply_one();
        if (error == ESP_ERR_NOT_FOUND) {
            break;
        }
    }
}

static void seed_progress(uint32_t local_total, uint32_t acked_total)
{
    watch_state_snapshot_t snap = {0};
    assert(watch_state_snapshot(&snap, 0) == ESP_OK);
    watch_tap_progress_update_t update = {
        .update_sequence = snap.tap_progress_update_sequence + 1U,
        .local_total = local_total,
        .acked_total = acked_total,
        .round_id = 1U,
        .round_cursor = local_total % 260U,
        .round_state = 0U,
        .pending_completion = false,
        .backlog_count = local_total > acked_total ? local_total - acked_total
                                                   : 0U,
        .persist_error = false,
    };
    if (update.update_sequence == 0U) {
        update.update_sequence = 1U;
    }
    assert(watch_state_apply_tap_progress_update(&update, 0) == ESP_OK);
}

static void setup(void)
{
    host_platform_init();
    host_sync_runtime_reset();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_settings_store_reset();
    host_co5300_reset();

    device_nav_service_cancel_prepared_run();
    (void)device_nav_service_deinit_contracts();
    assert(device_nav_service_init_contracts() == ESP_OK);
    assert(device_nav_service_prepare_run() == ESP_OK);

    sync_service_cancel_prepared_run();
    (void)sync_service_deinit_contracts();
    assert(sync_service_init_contracts() == ESP_OK);
    assert(sync_service_set_identity("ewf-host-1", "1.2.3", "HS-1.0.0", 1U) ==
           ESP_OK);
    assert(sync_service_set_battery_percent(77U) == ESP_OK);
    assert(sync_service_prepare_run() == ESP_OK);
    drain_state();
}

static void test_success_advances_acked_and_sync_ok(void)
{
    setup();
    seed_progress(20U, 10U);
    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_SUCCESS);

    assert(device_nav_service_request_sync(0) == ESP_OK);
    drain_state();
    assert(device_nav_service_sync_status() == EWF_SYNC_STATUS_PENDING);

    assert(sync_service_poll_once(1000U) == ESP_OK);
    drain_state();

    assert(host_sync_runtime_advance_calls() >= 1U);
    assert(host_sync_runtime_acked_total() == 20U);
    assert(device_nav_service_sync_status() == EWF_SYNC_STATUS_OK);

    ewf_sync_service_snapshot_t snap = {0};
    assert(sync_service_snapshot(&snap) == ESP_OK);
    assert(snap.sync_fact == EWF_SYNC_FACT_SYNCED);
    assert(snap.network_mode == EWF_SYNC_NETWORK_CONNECTED);
    assert(snap.window_count >= 1U);
    printf("PASS success_advances_acked_and_sync_ok\n");
}

static void test_conflict_does_not_fake_synced(void)
{
    setup();
    seed_progress(5U, 1U);
    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_CONFLICT_20003);
    assert(device_nav_service_request_sync(0) == ESP_OK);
    drain_state();
    (void)sync_service_poll_once(2000U);
    drain_state();
    assert(device_nav_service_sync_status() == EWF_SYNC_STATUS_FAIL);
    ewf_sync_service_snapshot_t snap = {0};
    assert(sync_service_snapshot(&snap) == ESP_OK);
    assert(snap.conflict);
    assert(snap.sync_fact != EWF_SYNC_FACT_SYNCED);
    printf("PASS conflict_does_not_fake_synced\n");
}

static void test_command_revision_applied(void)
{
    setup();
    seed_progress(3U, 3U);
    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_SUCCESS_WITH_COMMAND);
    assert(device_nav_service_request_sync(0) == ESP_OK);
    drain_state();
    (void)sync_service_poll_once(3000U);
    drain_state();

    ewf_device_settings_record_t settings = {0};
    assert(device_nav_service_get_settings(&settings) == ESP_OK);
    assert(settings.applied_revision >= 1U);
    assert(host_sync_runtime_last_volume() == 50U);
    printf("PASS command_revision_applied\n");
}

static void test_fail_closed_without_identity(void)
{
    host_platform_init();
    host_sync_runtime_reset();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    sync_service_cancel_prepared_run();
    (void)sync_service_deinit_contracts();
    assert(sync_service_init_contracts() == ESP_OK);
    /* 默认 device_id=ewf-unconfigured → fail-closed */
    assert(sync_service_prepare_run() == ESP_OK);
    ewf_sync_service_snapshot_t snap = {0};
    assert(sync_service_snapshot(&snap) == ESP_OK);
    assert(!snap.identity_ready);
    assert(sync_service_poll_once(1U) == ESP_ERR_INVALID_STATE);
    printf("PASS fail_closed_without_identity\n");
}

static void test_persist_busy_defers_sync_window(void)
{
    setup();
    seed_progress(20U, 10U);
    ewf_sync_https_mock_set_scenario(EWF_SYNC_MOCK_SUCCESS);
    host_sync_set_persist_status(true, false);

    assert(device_nav_service_request_sync(0) == ESP_OK);
    drain_state();
    assert(device_nav_service_sync_status() == EWF_SYNC_STATUS_PENDING);

    assert(sync_service_poll_once(1000U) == ESP_OK);
    drain_state();

    /* 落盘忙：不开窗、不推进 acked，立即同步保持 BUSY。 */
    assert(host_sync_runtime_advance_calls() == 0U);
    assert(host_sync_runtime_acked_total() == 0U);
    assert(device_nav_service_sync_status() == EWF_SYNC_STATUS_BUSY);

    /* 落盘空闲后可再次开窗（验证 force_close 回滚，非永久 MERGE）。 */
    host_sync_set_persist_status(false, false);
    assert(device_nav_service_request_sync(0) == ESP_OK);
    drain_state();
    assert(sync_service_poll_once(2000U) == ESP_OK);
    drain_state();
    assert(host_sync_runtime_advance_calls() >= 1U);
    assert(host_sync_runtime_acked_total() == 20U);
    printf("PASS persist_busy_defers_sync_window\n");
}

int main(void)
{
    test_success_advances_acked_and_sync_ok();
    test_conflict_does_not_fake_synced();
    test_command_revision_applied();
    test_fail_closed_without_identity();
    test_persist_busy_defers_sync_window();
    printf("PASS test_sync_runtime\n");
    return 0;
}
