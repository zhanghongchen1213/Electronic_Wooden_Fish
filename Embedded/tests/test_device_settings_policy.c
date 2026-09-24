/**
 * @file     test_device_settings_policy.c
 * @brief    设备设置校验与默认真值主机测试。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "device_settings_policy.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_defaults(void)
{
    ewf_device_settings_record_t record;
    ewf_device_settings_defaults(&record);
    assert(record.volume == 50U);
    assert(record.brightness == EWF_BRIGHTNESS_MID);
    assert(record.timeout_s == 15U);
    assert(record.applied_revision == 0U);
    assert(ewf_device_settings_validate(&record) == EWF_SETTINGS_OK);
}

static void test_volume_and_timeout(void)
{
    assert(ewf_device_settings_volume_valid(0));
    assert(ewf_device_settings_volume_valid(100));
    assert(!ewf_device_settings_volume_valid(101));
    assert(ewf_device_settings_timeout_valid(5));
    assert(ewf_device_settings_timeout_valid(15));
    assert(ewf_device_settings_timeout_valid(30));
    assert(!ewf_device_settings_timeout_valid(10));
}

static void test_brightness_wire(void)
{
    ewf_brightness_t value = EWF_BRIGHTNESS_LOW;
    assert(ewf_device_settings_brightness_parse("mid", &value));
    assert(value == EWF_BRIGHTNESS_MID);
    assert(!ewf_device_settings_brightness_parse("medium", &value));
    assert(strcmp(ewf_device_settings_brightness_name(EWF_BRIGHTNESS_MID),
                  "mid") == 0);
    assert(ewf_device_settings_brightness_raw(EWF_BRIGHTNESS_LOW) ==
           EWF_BRIGHTNESS_RAW_LOW);
}

static void test_revision_monotonic(void)
{
    assert(ewf_device_settings_revision_ok(3U, 3U));
    assert(ewf_device_settings_revision_ok(3U, 4U));
    assert(!ewf_device_settings_revision_ok(4U, 3U));
}

int main(void)
{
    test_defaults();
    test_volume_and_timeout();
    test_brightness_wire();
    test_revision_monotonic();
    printf("PASS test_device_settings_policy\n");
    return 0;
}
