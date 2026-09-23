/**
 * @file     progress_store_nvs.c
 * @brief    高水位/轮次事务组的 NVS 持久化实现。
 * @details  事务组全部字段落在单一 NVS namespace "progress"，以单次 nvs_commit()
 *           为事务边界：v5.5.4 官方文档明确 nvs_set_* 在 nvs_commit 之前不会更新
 *           实际存储，掉电时未提交写入整体丢失、已提交内容保持一致，满足契约 §11
 *           单事务分组约束。恢复介质错误 fail-closed，不静默回退零值。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "progress_store.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "SVC_PROGRESS";

/** 事务组唯一 NVS namespace；与既有 config_service 多 namespace 先例并列。 */
#define PROGRESS_NVS_NAMESPACE "progress"
/** 事务组 schema 版本 key（15 字符 key 上限内的稳定命名）。 */
#define PROGRESS_KEY_SCHEMA_VERSION "schema_ver"
/** 本地累计高水位 key。 */
#define PROGRESS_KEY_LOCAL_TOTAL "local_total"
/** 已确认累计高水位 key。 */
#define PROGRESS_KEY_ACKED_TOTAL "acked_total"
/** 轮次 ID key。 */
#define PROGRESS_KEY_ROUND_ID "round_id"
/** 轮次游标 key。 */
#define PROGRESS_KEY_ROUND_CURSOR "round_cursor"
/** 轮次状态 key。 */
#define PROGRESS_KEY_ROUND_STATE "round_state"
/** 本地完成锁定置位 key。 */
#define PROGRESS_KEY_PENDING_COMPLETION "pending_cmp"
/** 篇章动作族幂等去重键 key（本 Story 仅保留落盘位）。 */
#define PROGRESS_KEY_ACTION_ID "action_id"
/** canonical 经文版本 key。 */
#define PROGRESS_KEY_SCRIPTURE_VERSION "scripture"

static esp_err_t log_save_failure(const char *key, esp_err_t error);
static void apply_field_defaults(ewf_progress_transaction_t *tx);

esp_err_t progress_store_load(ewf_progress_transaction_t *tx,
                              ewf_progress_store_status_t *status)
{
    if (tx == NULL || status == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *status = EWF_PROGRESS_STORE_ERROR;
    ewf_progress_transaction_init(tx, "");

    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(PROGRESS_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (error == ESP_ERR_NVS_NOT_FOUND)
    {
        /* namespace 尚未建立：这是首次启动的合法零值状态。 */
        *status = EWF_PROGRESS_STORE_EMPTY;
        return ESP_OK;
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "进度事务组 namespace 打开失败，错误=%s", esp_err_to_name(error));
        return ESP_OK;
    }

    uint32_t schema_version = 0U;
    error = nvs_get_u32(handle, PROGRESS_KEY_SCHEMA_VERSION, &schema_version);
    if (error == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_close(handle);
        *status = EWF_PROGRESS_STORE_EMPTY;
        return ESP_OK;
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "进度事务组 schema 版本读取失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return ESP_OK;
    }
    tx->schema_version = schema_version;

    /* 新增字段可省略读取取默认值（契约 §11 迁移说明）；NOT_FOUND 落默认值。 */
    uint32_t local_total = 0U;
    uint32_t acked_total = 0U;
    uint32_t round_id = 0U;
    uint32_t round_cursor = 0U;
    uint32_t round_state = 0U;
    uint32_t pending_completion = 0U;
    size_t version_length = sizeof(tx->scripture_version);
    size_t action_length = sizeof(tx->action_id);
    (void)nvs_get_u32(handle, PROGRESS_KEY_LOCAL_TOTAL, &local_total);
    (void)nvs_get_u32(handle, PROGRESS_KEY_ACKED_TOTAL, &acked_total);
    (void)nvs_get_u32(handle, PROGRESS_KEY_ROUND_ID, &round_id);
    (void)nvs_get_u32(handle, PROGRESS_KEY_ROUND_CURSOR, &round_cursor);
    (void)nvs_get_u32(handle, PROGRESS_KEY_ROUND_STATE, &round_state);
    (void)nvs_get_u32(handle, PROGRESS_KEY_PENDING_COMPLETION, &pending_completion);
    if (nvs_get_str(handle, PROGRESS_KEY_SCRIPTURE_VERSION,
                    tx->scripture_version, &version_length) != ESP_OK)
    {
        tx->scripture_version[0] = '\0';
    }
    if (nvs_get_str(handle, PROGRESS_KEY_ACTION_ID,
                    tx->action_id, &action_length) != ESP_OK)
    {
        tx->action_id[0] = '\0';
    }
    tx->local_total = local_total;
    tx->acked_total = acked_total;
    tx->round_id = round_id;
    tx->round_cursor = round_cursor;
    tx->round_state = (ewf_progress_round_state_t)round_state;
    tx->pending_completion = pending_completion != 0U;
    apply_field_defaults(tx);
    nvs_close(handle);
    *status = EWF_PROGRESS_STORE_OK;
    return ESP_OK;
}

esp_err_t progress_store_save(const ewf_progress_transaction_t *tx)
{
    if (tx == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    nvs_handle_t handle = 0;
    esp_err_t error = nvs_open(PROGRESS_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "进度事务组 namespace 写入打开失败，错误=%s", esp_err_to_name(error));
        return error;
    }

    /* 同一 namespace 内全部 set 之后单次 commit；任一 set 失败即放弃本次提交。 */
    bool failed = false;
    error = nvs_set_u32(handle, PROGRESS_KEY_SCHEMA_VERSION, tx->schema_version);
    failed |= log_save_failure(PROGRESS_KEY_SCHEMA_VERSION, error) != ESP_OK;
    if (!failed)
    {
        error = nvs_set_u32(handle, PROGRESS_KEY_LOCAL_TOTAL, tx->local_total);
        failed |= log_save_failure(PROGRESS_KEY_LOCAL_TOTAL, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_u32(handle, PROGRESS_KEY_ACKED_TOTAL, tx->acked_total);
        failed |= log_save_failure(PROGRESS_KEY_ACKED_TOTAL, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_u32(handle, PROGRESS_KEY_ROUND_ID, tx->round_id);
        failed |= log_save_failure(PROGRESS_KEY_ROUND_ID, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_u32(handle, PROGRESS_KEY_ROUND_CURSOR, tx->round_cursor);
        failed |= log_save_failure(PROGRESS_KEY_ROUND_CURSOR, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_u32(handle, PROGRESS_KEY_ROUND_STATE,
                            (uint32_t)tx->round_state);
        failed |= log_save_failure(PROGRESS_KEY_ROUND_STATE, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_u32(handle, PROGRESS_KEY_PENDING_COMPLETION,
                            tx->pending_completion ? 1U : 0U);
        failed |= log_save_failure(PROGRESS_KEY_PENDING_COMPLETION, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_str(handle, PROGRESS_KEY_SCRIPTURE_VERSION,
                            tx->scripture_version);
        failed |= log_save_failure(PROGRESS_KEY_SCRIPTURE_VERSION, error) != ESP_OK;
    }
    if (!failed)
    {
        error = nvs_set_str(handle, PROGRESS_KEY_ACTION_ID, tx->action_id);
        failed |= log_save_failure(PROGRESS_KEY_ACTION_ID, error) != ESP_OK;
    }
    if (failed)
    {
        /* 未 commit 的修改不落盘；直接放弃并上抛 busy/error。 */
        nvs_close(handle);
        return ESP_FAIL;
    }
    error = nvs_commit(handle);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "进度事务组提交失败，错误=%s", esp_err_to_name(error));
        nvs_close(handle);
        return error;
    }
    nvs_close(handle);
    return ESP_OK;
}

static esp_err_t log_save_failure(const char *key, esp_err_t error)
{
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "进度事务组字段写入失败：key=%s，错误=%s",
                 key, esp_err_to_name(error));
    }
    return error;
}

static void apply_field_defaults(ewf_progress_transaction_t *tx)
{
    /* 首启零值基线：local_total=0、round_id=1、round_state=in_progress。 */
    if (tx->round_id == 0U)
    {
        tx->round_id = 1U;
    }
    if ((unsigned)tx->round_state > (unsigned)EWF_PROGRESS_ROUND_STATE_COMPLETED)
    {
        tx->round_state = EWF_PROGRESS_ROUND_STATE_IN_PROGRESS;
    }
}
