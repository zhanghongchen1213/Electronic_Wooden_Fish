/**
 * @file     co5300_bsp_host.c
 * @brief    CO5300 显示/亮度主机替身。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

static bool s_display_on = true;
static uint16_t s_brightness;
static unsigned s_display_calls;
static unsigned s_brightness_calls;

void host_co5300_reset(void)
{
    s_display_on = true;
    s_brightness = 0U;
    s_display_calls = 0U;
    s_brightness_calls = 0U;
}

bool host_co5300_display_on(void) { return s_display_on; }
uint16_t host_co5300_brightness(void) { return s_brightness; }
unsigned host_co5300_display_calls(void) { return s_display_calls; }
unsigned host_co5300_brightness_calls(void) { return s_brightness_calls; }

esp_err_t co5300_bsp_set_display(bool on)
{
    s_display_on = on;
    ++s_display_calls;
    return ESP_OK;
}

esp_err_t co5300_bsp_set_brightness(uint16_t raw_level)
{
    s_brightness = raw_level;
    ++s_brightness_calls;
    return ESP_OK;
}
