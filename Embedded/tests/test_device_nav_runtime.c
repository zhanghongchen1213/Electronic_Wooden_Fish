/**
 * @file     test_device_nav_runtime.c
 * @brief    导航/设置服务主机接线测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "app_state.h"
#include "device_nav_policy.h"
#include "device_nav_service.h"
#include "device_settings_policy.h"
#include "state_service.h"

void host_platform_init(void);
void host_state_owner_start(void);
esp_err_t host_state_apply_one(void);
void host_settings_store_reset(void);
void host_settings_store_preload_legacy_audio(uint8_t volume);
bool host_settings_store_has(void);
const ewf_device_settings_record_t *host_settings_store_record(void);
void host_co5300_reset(void);
bool host_co5300_display_on(void);
uint16_t host_co5300_brightness(void);

static void drain_state(void)
{
    for (;;) {
        const esp_err_t error = host_state_apply_one();
        if (error == ESP_ERR_NOT_FOUND) {
            break;
        }
    }
}

static watch_nav_update_t nav_snapshot(void)
{
    watch_nav_update_t snapshot = {0};
    assert(watch_state_nav_snapshot(&snapshot, 0) == ESP_OK);
    return snapshot;
}

static void setup(void)
{
    host_platform_init();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_settings_store_reset();
    host_co5300_reset();
    device_nav_service_cancel_prepared_run();
    (void)device_nav_service_deinit_contracts();
    assert(device_nav_service_init_contracts() == ESP_OK);
    assert(device_nav_service_prepare_run() == ESP_OK);
    drain_state();
}

static void test_defaults_and_brightness(void)
{
    setup();
    const watch_nav_update_t nav = nav_snapshot();
    assert(nav.active_page == WATCH_NAV_PAGE_MUYU);
    assert(nav.volume == 50U);
    assert(nav.timeout_s == 15U);
    assert(nav.brightness == WATCH_BRIGHTNESS_MID);
    assert(host_co5300_brightness() == EWF_BRIGHTNESS_RAW_MID);
    assert(host_settings_store_has());
    printf("PASS defaults_and_brightness\n");
}

static void test_pwr_cycle_and_settings(void)
{
    setup();
    assert(device_nav_service_on_pwr_interval(400U, 100U) == ESP_OK);
    drain_state();
    assert(device_nav_service_active_page() == EWF_NAV_PAGE_JINGWEN);

    assert(device_nav_service_on_nav_input(EWF_NAV_INPUT_SWIPE_DOWN, 200U) ==
           ESP_OK);
    drain_state();
    assert(device_nav_service_settings_open());

    assert(device_nav_service_on_pwr_interval(500U, 300U) == ESP_OK);
    drain_state();
    assert(!device_nav_service_settings_open());
    assert(device_nav_service_active_page() == EWF_NAV_PAGE_JINGWEN);
    printf("PASS pwr_cycle_and_settings\n");
}

static void test_settings_persist_and_sync(void)
{
    setup();
    assert(device_nav_service_set_volume(80U, 0) == ESP_OK);
    assert(device_nav_service_set_timeout_s(30U, 0) == ESP_OK);
    assert(device_nav_service_set_brightness(EWF_BRIGHTNESS_HIGH, 0) == ESP_OK);
    drain_state();
    assert(host_settings_store_record()->volume == 80U);
    assert(host_settings_store_record()->timeout_s == 30U);
    assert(host_co5300_brightness() == EWF_BRIGHTNESS_RAW_HIGH);

    assert(device_nav_service_request_sync(0) == ESP_OK);
    drain_state();
    watch_nav_update_t nav = nav_snapshot();
    assert(nav.sync_status == WATCH_SYNC_STATUS_PENDING);
    assert(nav.sync_request_id != 0U);

    /* 非法值拒绝并保持原值。 */
    assert(device_nav_service_set_volume(101U, 0) == ESP_ERR_INVALID_ARG);
    assert(device_nav_service_set_timeout_s(10U, 0) == ESP_ERR_INVALID_ARG);
    ewf_device_settings_record_t settings = {0};
    assert(device_nav_service_get_settings(&settings) == ESP_OK);
    assert(settings.volume == 80U);
    assert(settings.timeout_s == 30U);
    printf("PASS settings_persist_and_sync\n");
}

static void test_legacy_audio_migration(void)
{
    host_platform_init();
    assert(watch_state_init() == ESP_OK);
    host_state_owner_start();
    host_settings_store_reset();
    host_settings_store_preload_legacy_audio(66U);
    host_co5300_reset();
    device_nav_service_cancel_prepared_run();
    (void)device_nav_service_deinit_contracts();
    assert(device_nav_service_init_contracts() == ESP_OK);
    assert(device_nav_service_prepare_run() == ESP_OK);
    drain_state();
    ewf_device_settings_record_t settings = {0};
    assert(device_nav_service_get_settings(&settings) == ESP_OK);
    assert(settings.volume == 66U);
    printf("PASS legacy_audio_migration\n");
}

static void test_wake_and_idle(void)
{
    setup();
    assert(device_nav_service_on_nav_input(EWF_NAV_INPUT_TOUCH_WAKE, 1U) ==
           ESP_OK);
    /* 已亮屏时 TOUCH_WAKE 忽略。 */
    assert(device_nav_service_screen_on());

    /* 强制熄屏：用超长空闲。 */
    assert(device_nav_service_set_timeout_s(5U, 0) == ESP_OK);
    assert(device_nav_service_note_activity(0U) == ESP_OK);
    assert(device_nav_service_on_nav_input(EWF_NAV_INPUT_IDLE_TIMEOUT, 5000U) ==
           ESP_OK);
    drain_state();
    assert(!device_nav_service_screen_on());
    assert(!host_co5300_display_on());

    assert(device_nav_service_on_pwr_interval(300U, 6000U) == ESP_OK);
    drain_state();
    assert(device_nav_service_screen_on());
    assert(host_co5300_display_on());
    printf("PASS wake_and_idle\n");
}

int main(void)
{
    test_defaults_and_brightness();
    test_pwr_cycle_and_settings();
    test_settings_persist_and_sync();
    test_legacy_audio_migration();
    test_wake_and_idle();
    printf("host device-nav-runtime: 全部通过\n");
    return 0;
}
