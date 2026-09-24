/**
 * @file     today_bucket_store_host.c
 * @brief    今日桶主机测试持久化替身。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "today_bucket_store.h"

#include <string.h>

static ewf_today_bucket_t s_stored;
static bool s_has_stored;
static bool s_fail_save;

void host_today_bucket_store_reset(void)
{
    memset(&s_stored, 0, sizeof(s_stored));
    s_has_stored = false;
    s_fail_save = false;
}

void host_today_bucket_store_fail_save(bool fail)
{
    s_fail_save = fail;
}

esp_err_t today_bucket_store_load(ewf_today_bucket_t *bucket)
{
    if (bucket == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_has_stored) {
        bucket->day_key = 0U;
        bucket->count = 0U;
        return ESP_OK;
    }
    *bucket = s_stored;
    return ESP_OK;
}

esp_err_t today_bucket_store_save(const ewf_today_bucket_t *bucket)
{
    if (bucket == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_save) {
        return ESP_FAIL;
    }
    s_stored = *bucket;
    s_has_stored = true;
    return ESP_OK;
}
