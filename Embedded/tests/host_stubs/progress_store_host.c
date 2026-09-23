/**
 * @file     progress_store_host.c
 * @brief    高水位/轮次 owner 主机测试的持久化替身。
 * @details  以内存单副本模拟单事务提交：save 全部字段一次性生效并计数，
 *           支持失败注入、预置介质内容与写集 key 观察，供恢复序列与
 *           自动模式标志不在写集的断言使用。
 * @author   ZHC
 * @date     2026-09-23
 */

#include <stdbool.h>
#include <string.h>

#include "esp_err.h"
#include "progress_store.h"
#include "progress_transaction.h"

/** 写集 key 名单：与 progress_store_nvs.c 的 namespace 字段一一对应。 */
static const char *const k_written_keys[] = {
    "schema_ver", "local_total", "acked_total", "round_id", "round_cursor",
    "round_state", "pending_cmp", "scripture", "action_id",
};
#define PROGRESS_HOST_KEY_COUNT (sizeof(k_written_keys) / sizeof(k_written_keys[0]))

static ewf_progress_transaction_t s_stored;
static bool s_has_stored;
static bool s_schema_present;
static uint32_t s_save_count;
static bool s_fail_save;
static bool s_fail_load_media;

void host_progress_store_reset(void)
{
    memset(&s_stored, 0, sizeof(s_stored));
    s_has_stored = false;
    s_schema_present = false;
    s_save_count = 0U;
    s_fail_save = false;
    s_fail_load_media = false;
}

void host_progress_store_fail_save(bool fail) { s_fail_save = fail; }
void host_progress_store_fail_load_media(bool fail) { s_fail_load_media = fail; }
uint32_t host_progress_store_save_count(void) { return s_save_count; }
bool host_progress_store_has_stored(void) { return s_has_stored; }
const ewf_progress_transaction_t *host_progress_store_stored(void)
{
    return &s_stored;
}

void host_progress_store_preload(const ewf_progress_transaction_t *tx)
{
    if (tx == NULL)
    {
        return;
    }
    s_stored = *tx;
    s_has_stored = true;
    s_schema_present = true;
}

bool host_progress_store_contains_key(const char *key)
{
    if (key == NULL)
    {
        return false;
    }
    for (size_t index = 0; index < PROGRESS_HOST_KEY_COUNT; ++index)
    {
        if (strcmp(k_written_keys[index], key) == 0)
        {
            return true;
        }
    }
    return false;
}

esp_err_t progress_store_load(ewf_progress_transaction_t *tx,
                              ewf_progress_store_status_t *status)
{
    if (tx == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_load_media)
    {
        *status = EWF_PROGRESS_STORE_ERROR;
        return ESP_OK;
    }
    if (!s_schema_present)
    {
        ewf_progress_transaction_init(tx, "");
        *status = EWF_PROGRESS_STORE_EMPTY;
        return ESP_OK;
    }
    *tx = s_stored;
    *status = EWF_PROGRESS_STORE_OK;
    return ESP_OK;
}

esp_err_t progress_store_save(const ewf_progress_transaction_t *tx)
{
    if (tx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_fail_save)
    {
        return ESP_FAIL;
    }
    /* 单次提交边界：全部字段一次性生效，模拟 commit 原子性。 */
    s_stored = *tx;
    s_has_stored = true;
    s_schema_present = true;
    ++s_save_count;
    return ESP_OK;
}
