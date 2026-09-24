/**
 * @file     device_nav_host.c
 * @brief    tap_input 主机测试用的导航服务最小替身。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_nav_policy.h"
#include "device_nav_service.h"

#include <stdbool.h>

static ewf_nav_page_t s_page = EWF_NAV_PAGE_MUYU;
static bool s_screen_on = true;
static bool s_settings_open;
static unsigned s_nav_input_calls;
static ewf_nav_input_kind_t s_last_nav_kind = EWF_NAV_INPUT_NONE;
static unsigned s_note_activity_calls;

void host_device_nav_reset(void)
{
    s_page = EWF_NAV_PAGE_MUYU;
    s_screen_on = true;
    s_settings_open = false;
    s_nav_input_calls = 0U;
    s_last_nav_kind = EWF_NAV_INPUT_NONE;
    s_note_activity_calls = 0U;
}

void host_device_nav_set_page(ewf_nav_page_t page) { s_page = page; }
void host_device_nav_set_screen_on(bool on) { s_screen_on = on; }
void host_device_nav_set_settings_open(bool open) { s_settings_open = open; }

unsigned host_device_nav_input_calls(void) { return s_nav_input_calls; }
ewf_nav_input_kind_t host_device_nav_last_kind(void) { return s_last_nav_kind; }
unsigned host_device_nav_note_activity_calls(void)
{
    return s_note_activity_calls;
}

ewf_nav_page_t device_nav_service_active_page(void) { return s_page; }
bool device_nav_service_screen_on(void) { return s_screen_on; }
bool device_nav_service_settings_open(void) { return s_settings_open; }

esp_err_t device_nav_service_on_nav_input(ewf_nav_input_kind_t kind,
                                          uint64_t now_ms)
{
    (void)now_ms;
    ++s_nav_input_calls;
    s_last_nav_kind = kind;
    if (kind == EWF_NAV_INPUT_TOUCH_WAKE) {
        s_screen_on = true;
        return ESP_OK;
    }
    if (kind == EWF_NAV_INPUT_SWIPE_DOWN) {
        s_settings_open = true;
        return ESP_OK;
    }
    if (kind == EWF_NAV_INPUT_SWIPE_UP) {
        s_settings_open = false;
        return ESP_OK;
    }
    if (kind == EWF_NAV_INPUT_SWIPE_LEFT) {
        s_page = ewf_nav_page_next(s_page);
        return ESP_OK;
    }
    if (kind == EWF_NAV_INPUT_SWIPE_RIGHT) {
        s_page = ewf_nav_page_prev(s_page);
        return ESP_OK;
    }
    return ESP_OK;
}

esp_err_t device_nav_service_note_activity(uint64_t now_ms)
{
    (void)now_ms;
    ++s_note_activity_calls;
    return ESP_OK;
}
