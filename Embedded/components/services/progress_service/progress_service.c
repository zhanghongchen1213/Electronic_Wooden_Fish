/**
 * @file     progress_service.c
 * @brief    本地高水位/轮次事实 owner 服务实现。
 * @details  消费 event_bus 的 EWF_TAP_EVENT_VALID 事件，owner 是唯一累计入口；
 *           事务组以单次提交落盘（先持久化成功再提交内存），恢复 fail-closed，
 *           积压达到 1000 时拒绝新输入并发布队列已满 gate 事实。写集不含自动
 *           模式标志，重启后自动模式默认关闭（AC 2）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "progress_service.h"

#include <stdatomic.h>
#include <string.h>

#include "esp_log.h"
#include "event_bus.h"
#include "ewf_scripture_canonical.h"
#include "freertos/semphr.h"
#include "progress_store.h"
#include "progress_transaction.h"
#include "state_service.h"
#include "tap_input_service.h"

static const char *TAG = "SVC_PROGRESS";

/** 事件消费空队列时的有界轮询等待。 */
#define PROGRESS_CONSUME_TIMEOUT_MS 50U
/** 发布快照与 gate 事实时的有界队列等待。 */
#define PROGRESS_PUBLISH_TIMEOUT_MS 20U

/** 保护 owner 私有事务组的互斥锁；owner 是唯一累计入口。 */
static SemaphoreHandle_t s_mutex;
/** owner 私有单事务组；消费者只能经 state_service 快照读取。 */
static ewf_progress_transaction_t s_transaction;
/** prepared 状态：恢复已完成且任务允许进入消费循环；跨任务原子访问。 */
static atomic_bool s_prepared;
/** 停止请求标志；由 request_stop 置位，消费循环有界轮询。 */
static atomic_bool s_stop_requested;
/** fail-closed 状态：恢复校验失败后停止推进，保持可诊断。 */
static bool s_degraded;
/** 最近一次发布的 gate 完成/积压事实，用于仅在变化时发布。 */
static bool s_last_completed;
static bool s_last_queue_full;

static esp_err_t persist_transaction_locked(ewf_progress_transaction_t *candidate);
static esp_err_t publish_progress_facts_locked(TickType_t timeout_ticks,
                                               bool persist_error);
static esp_err_t publish_gate_facts_locked(TickType_t timeout_ticks);
static void handle_valid_event(uint32_t sequence);

esp_err_t progress_service_init_contracts(void)
{
    if (s_mutex != NULL)
    {
        return ESP_OK;
    }
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    memset(&s_transaction, 0, sizeof(s_transaction));
    atomic_store(&s_prepared, false);
    s_degraded = false;
    s_last_completed = false;
    s_last_queue_full = false;
    atomic_store(&s_stop_requested, false);
    return ESP_OK;
}

esp_err_t progress_service_prepare_run(void)
{
    if (s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    s_degraded = false;
    s_last_completed = false;
    s_last_queue_full = false;
    atomic_store(&s_stop_requested, false);

    ewf_progress_transaction_t recovered = {0};
    ewf_progress_store_status_t status = EWF_PROGRESS_STORE_ERROR;
    const esp_err_t load_error = progress_store_load(&recovered, &status);
    if (load_error != ESP_OK)
    {
        xSemaphoreGive(s_mutex);
        return load_error;
    }
    if (status == EWF_PROGRESS_STORE_EMPTY)
    {
        /* 缺字段首次启动：按零值初始化并写入初始事务组。 */
        ewf_progress_transaction_init(&recovered, EWF_SCRIPTURE_VERSION);
        const esp_err_t init_save_error = progress_store_save(&recovered);
        if (init_save_error != ESP_OK)
        {
            ESP_LOGE(TAG, "首启事务组初始落盘失败，错误=%s；稍后敲击将重试",
                     esp_err_to_name(init_save_error));
        }
    }
    else if (status == EWF_PROGRESS_STORE_ERROR)
    {
        /* 介质错误：不依赖兜底校验，显式 fail-closed，不静默回退零值。 */
        s_degraded = true;
        ESP_LOGE(TAG, "进度事务组介质读取错误，停止推进并保持可诊断");
    }
    else
    {
        const ewf_progress_validate_result_t valid =
            ewf_progress_transaction_validate(&recovered,
                                              EWF_SCRIPTURE_VERSION,
                                              EWF_SCRIPTURE_CONSUMABLE_COUNT);
        if (valid != EWF_PROGRESS_VALIDATE_OK)
        {
            /* 配置错误/字段不一致：fail-closed，不静默回退零值或伪造高水位。 */
            s_degraded = true;
            ESP_LOGE(TAG, "进度事务组恢复校验失败：结果=%d，停止推进并保持可诊断",
                     (int)valid);
        }
    }
    s_transaction = recovered;
    atomic_store(&s_prepared, true);

    /* 恢复后立即发布初值快照与 gate 事实，消费者只读快照。 */
    const TickType_t publish_ticks = pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS);
    esp_err_t publish_error = publish_progress_facts_locked(publish_ticks, false);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "恢复初值快照发布失败，错误=%s", esp_err_to_name(publish_error));
    }
    publish_error = publish_gate_facts_locked(publish_ticks);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "恢复 gate 事实发布失败，错误=%s", esp_err_to_name(publish_error));
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG,
             "进度 owner 已恢复：local=%lu，acked=%lu，round=%lu，cursor=%lu，degraded=%d",
             (unsigned long)s_transaction.local_total,
             (unsigned long)s_transaction.acked_total,
             (unsigned long)s_transaction.round_id,
             (unsigned long)s_transaction.round_cursor,
             (int)s_degraded);
    return ESP_OK;
}

void progress_service_cancel_prepared_run(void)
{
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE)
    {
        atomic_store(&s_prepared, false);
        xSemaphoreGive(s_mutex);
    }
}

esp_err_t progress_service_deinit_contracts(void)
{
    if (s_mutex == NULL)
    {
        return ESP_OK;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (atomic_load(&s_prepared))
    {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    xSemaphoreGive(s_mutex);
    vSemaphoreDelete(s_mutex);
    s_mutex = NULL;
    return ESP_OK;
}

esp_err_t progress_service_run(void)
{
    if (s_mutex == NULL || !atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    QueueHandle_t queue = event_bus_queue();
    if (queue == NULL)
    {
        ESP_LOGE(TAG, "event_bus 队列未初始化，进度 owner 无法消费有效敲击");
        return ESP_ERR_INVALID_STATE;
    }
    legbot_event_t event = {0};
    for (;;)
    {
        if (xQueueReceive(queue, &event,
                          pdMS_TO_TICKS(PROGRESS_CONSUME_TIMEOUT_MS)) == pdTRUE)
        {
            if (event.code == EWF_TAP_EVENT_VALID)
            {
                handle_valid_event(event.value);
            }
            continue;
        }
        /* 空轮询超时后才检查停止请求：先消费完既有事件再退出，不丢事件。 */
        if (atomic_load(&s_stop_requested))
        {
            break;
        }
    }
    atomic_store(&s_prepared, false);
    return ESP_OK;
}

esp_err_t progress_service_request_stop(TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, true);
    return ESP_OK;
}

esp_err_t progress_service_advance_acked_total(uint32_t new_acked_total,
                                               TickType_t timeout_ticks)
{
    if (s_mutex == NULL || !atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_degraded)
    {
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "进度 owner 处于 fail-closed，拒绝确认收敛提交");
        return ESP_ERR_INVALID_STATE;
    }
    ewf_progress_transaction_t candidate = s_transaction;
    const ewf_progress_ack_result_t result =
        ewf_progress_apply_acked_total(&candidate, new_acked_total);
    if (result == EWF_PROGRESS_ACK_NOOP)
    {
        /* 重复同步的同值确认：no-op，不重复计数、不重复落盘。 */
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }
    if (result == EWF_PROGRESS_ACK_REJECTED)
    {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "确认值倒退提交被拒绝：当前=%lu，提交=%lu",
                 (unsigned long)s_transaction.acked_total,
                 (unsigned long)new_acked_total);
        return ESP_ERR_INVALID_ARG;
    }
    const TickType_t publish_ticks = timeout_ticks;
    const esp_err_t save_error = persist_transaction_locked(&candidate);
    if (save_error != ESP_OK)
    {
        (void)publish_progress_facts_locked(publish_ticks, true);
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "确认收敛落盘失败，错误=%s；本地确认值保持不变",
                 esp_err_to_name(save_error));
        return save_error;
    }
    const esp_err_t publish_error = publish_progress_facts_locked(publish_ticks, false);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "确认收敛快照发布失败，错误=%s", esp_err_to_name(publish_error));
    }
    /* 差值回落到阈值之下时 gate 立即恢复接受输入。 */
    const esp_err_t gate_error = publish_gate_facts_locked(publish_ticks);
    if (gate_error != ESP_OK)
    {
        ESP_LOGW(TAG, "确认收敛 gate 事实发布失败，错误=%s", esp_err_to_name(gate_error));
    }
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static void handle_valid_event(uint32_t sequence)
{
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return;
    }
    if (s_degraded)
    {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "进度 owner 处于 fail-closed，忽略有效敲击：序号=%lu",
                 (unsigned long)sequence);
        return;
    }
    if (ewf_progress_backlog_is_full(&s_transaction))
    {
        /* 统一拒绝 gate：不产生新字符、累计或落盘推进，也不静默丢弃已有事实。 */
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG,
                 "离线积压已达上限，拒绝敲击：local=%lu，acked=%lu，积压=%lu，序号=%lu",
                 (unsigned long)s_transaction.local_total,
                 (unsigned long)s_transaction.acked_total,
                 (unsigned long)ewf_progress_backlog_count(&s_transaction),
                 (unsigned long)sequence);
        return;
    }
    ewf_progress_transaction_t candidate = s_transaction;
    const bool completed_now = ewf_progress_apply_valid_tap(
        &candidate, EWF_SCRIPTURE_CONSUMABLE_COUNT);
    const TickType_t publish_ticks = pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS);
    const esp_err_t save_error = persist_transaction_locked(&candidate);
    if (save_error != ESP_OK)
    {
        /* 持久化失败：内存与介质保持一致，busy/error 日志与 typed 原因，不静默丢失。 */
        (void)publish_progress_facts_locked(publish_ticks, true);
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "敲击事务落盘失败，错误=%s；本地高水位保持不变，序号=%lu",
                 esp_err_to_name(save_error), (unsigned long)sequence);
        return;
    }
    const esp_err_t publish_error = publish_progress_facts_locked(publish_ticks, false);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "敲击快照发布失败，错误=%s", esp_err_to_name(publish_error));
    }
    if (completed_now)
    {
        /* 本地末字完成：发布本地完成事实；完成确认与新轮次属 Story 3.6/5.2 域。 */
        ESP_LOGW(TAG, "本地轮次到达末字，进入完成锁定：round=%lu，local=%lu",
                 (unsigned long)s_transaction.round_id,
                 (unsigned long)s_transaction.local_total);
    }
    const esp_err_t gate_error = publish_gate_facts_locked(publish_ticks);
    if (gate_error != ESP_OK)
    {
        ESP_LOGW(TAG, "敲击 gate 事实发布失败，错误=%s", esp_err_to_name(gate_error));
    }
    xSemaphoreGive(s_mutex);
}

static esp_err_t persist_transaction_locked(ewf_progress_transaction_t *candidate)
{
    /* 先持久化成功再提交内存：掉电后介质与内存高水位、轮次与游标保持一致。 */
    const esp_err_t save_error = progress_store_save(candidate);
    if (save_error == ESP_OK)
    {
        s_transaction = *candidate;
    }
    return save_error;
}

static esp_err_t publish_progress_facts_locked(TickType_t timeout_ticks,
                                               bool persist_error)
{
    const watch_tap_progress_update_t facts = {
        .update_sequence = 0U,
        .local_total = s_transaction.local_total,
        .acked_total = s_transaction.acked_total,
        .round_id = s_transaction.round_id,
        .round_cursor = s_transaction.round_cursor,
        .round_state = (uint32_t)s_transaction.round_state,
        .pending_completion = s_transaction.pending_completion,
        .backlog_count = ewf_progress_backlog_count(&s_transaction),
        .persist_error = persist_error,
    };
    return state_service_update_tap_progress_owner(&facts, timeout_ticks);
}

static esp_err_t publish_gate_facts_locked(TickType_t timeout_ticks)
{
    ewf_tap_gate_state_t gate = {0};
    ewf_progress_fill_gate(&s_transaction, &gate);
    if (s_last_completed == gate.completed &&
        s_last_queue_full == gate.queue_full)
    {
        return ESP_OK;
    }
    const esp_err_t error = state_service_update_tap_gate_owner(
        gate.completed, gate.queue_full, timeout_ticks);
    if (error == ESP_OK)
    {
        s_last_completed = gate.completed;
        s_last_queue_full = gate.queue_full;
    }
    return error;
}
