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
#include "fault_gate_policy.h"
#include "freertos/semphr.h"
#include "progress_store.h"
#include "progress_transaction.h"
#include "state_service.h"
#include "tap_input_service.h"
#include "today_bucket_policy.h"
#include "today_bucket_store.h"
#include "esp_timer.h"

static const char *TAG = "SVC_PROGRESS";
static const char *TAG_FAULT = "FAULT_GATE";

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
/** 事务组正在落盘（供 core_path_policy 只读）。 */
static atomic_bool s_persist_inflight;
/** 上次落盘失败、待重试（供 core_path_policy 只读）。 */
static atomic_bool s_persist_pending;
/** fault_locked 生产策略态（progress 介质失败 + 显示注入）。 */
static ewf_fault_gate_state_t s_fault_gate;
/** 非权威今日桶（裁决 B；与高水位事务分仓）。 */
static ewf_today_bucket_t s_today_bucket;

static esp_err_t persist_transaction_locked(ewf_progress_transaction_t *candidate);
static esp_err_t publish_progress_facts_locked(TickType_t timeout_ticks,
                                               bool persist_error);
static esp_err_t publish_gate_facts_locked(TickType_t timeout_ticks);
static void apply_fault_gate_locked(const ewf_fault_gate_decision_t *decision,
                                    TickType_t timeout_ticks);
static void handle_valid_event(uint32_t sequence);
static void probe_persist_recovery_idle(void);
static void refresh_today_bucket_locked(bool bump_on_tap);
static uint32_t current_shanghai_day_key(const watch_state_snapshot_t *snap);

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
    memset(&s_today_bucket, 0, sizeof(s_today_bucket));
    atomic_store(&s_prepared, false);
    s_degraded = false;
    s_last_completed = false;
    s_last_queue_full = false;
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_persist_inflight, false);
    atomic_store(&s_persist_pending, false);
    ewf_fault_gate_reset(&s_fault_gate);
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
    /* 不复位 persist_pending / fault_gate：任务停启不得静默清锁或丢待重试意图。 */

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
    /* 今日桶与高水位分仓加载；失败只告警，不阻断累计恢复。 */
    {
        ewf_today_bucket_t loaded = {0};
        const esp_err_t today_load = today_bucket_store_load(&loaded);
        if (today_load == ESP_OK) {
            s_today_bucket = loaded;
        } else {
            ESP_LOGW(TAG, "今日桶加载失败：%s；展示回待校时路径",
                     esp_err_to_name(today_load));
            memset(&s_today_bucket, 0, sizeof(s_today_bucket));
        }
    }
    atomic_store(&s_prepared, true);

    /* 恢复后立即发布初值快照与 gate 事实，消费者只读快照。 */
    const TickType_t publish_ticks = pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS);
    refresh_today_bucket_locked(false);
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
        /* 空闲轮询：介质恢复探测，打破 fault_locked + persist_pending 死锁。 */
        probe_persist_recovery_idle();
        /* 空闲对齐今日桶：跨日清零，或可信时间翻转后补发展示计数。 */
        if (xSemaphoreTake(s_mutex, 0) == pdTRUE) {
            const uint32_t before_key = s_today_bucket.day_key;
            const uint32_t before_count = s_today_bucket.count;
            watch_state_snapshot_t before_snap;
            memset(&before_snap, 0, sizeof(before_snap));
            (void)watch_state_snapshot(&before_snap, 0);
            const uint32_t before_published = before_snap.tap_today_count;
            const bool before_trusted = before_snap.time_synchronized;

            refresh_today_bucket_locked(false);

            watch_state_snapshot_t after_snap;
            memset(&after_snap, 0, sizeof(after_snap));
            (void)watch_state_snapshot(&after_snap, 0);
            const uint32_t day_key = current_shanghai_day_key(&after_snap);
            const uint32_t today_display = ewf_today_bucket_display_count(
                after_snap.time_synchronized, &s_today_bucket, day_key);
            if (before_key != s_today_bucket.day_key ||
                before_count != s_today_bucket.count ||
                before_published != today_display ||
                before_trusted != after_snap.time_synchronized) {
                (void)publish_progress_facts_locked(
                    pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS), false);
            }
            xSemaphoreGive(s_mutex);
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
    /* 高水位落盘成功后：可信时间下写入今日桶（未校时不写）。 */
    refresh_today_bucket_locked(true);
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
    atomic_store(&s_persist_inflight, true);
    const esp_err_t save_error = progress_store_save(candidate);
    atomic_store(&s_persist_inflight, false);
    if (save_error == ESP_OK)
    {
        s_transaction = *candidate;
        atomic_store(&s_persist_pending, false);
        const ewf_fault_gate_decision_t fault =
            ewf_fault_gate_on_persist_result(&s_fault_gate, true);
        apply_fault_gate_locked(&fault, pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS));
    }
    else
    {
        atomic_store(&s_persist_pending, true);
        const ewf_fault_gate_decision_t fault =
            ewf_fault_gate_on_persist_result(&s_fault_gate, false);
        apply_fault_gate_locked(&fault, pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS));
        if (fault.locked && fault.locked_changed)
        {
            ESP_LOGE(TAG_FAULT,
                     "进度介质连续失败达到阈值，输入进入 fault_locked：连续失败=%lu",
                     (unsigned long)s_fault_gate.consecutive_persist_errors);
        }
    }
    return save_error;
}

static void apply_fault_gate_locked(const ewf_fault_gate_decision_t *decision,
                                    TickType_t timeout_ticks)
{
    if (decision == NULL || !decision->locked_changed)
    {
        return;
    }
    const esp_err_t error =
        state_service_update_tap_fault_lock_owner(decision->locked, timeout_ticks);
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG_FAULT, "fault_locked 事实发布失败，错误=%s", esp_err_to_name(error));
    }
    else if (decision->locked)
    {
        ESP_LOGW(TAG_FAULT, "已发布 fault_locked=true，拒绝新输入直至落盘恢复或清除显示故障");
    }
    else
    {
        ESP_LOGI(TAG_FAULT, "已发布 fault_locked=false，输入闸门恢复");
    }
}

/**
 * @brief 空闲探测：对当前已一致事务做健康落盘，清除 pending 并解锁 persist 触发的 fault。
 * @details 落盘失败时内存未提交，但 pending/fault 仍闩住同步与新输入；无新敲击时
 *          只能通过对已一致事务重试落盘自愈（AD-13 / Task 4）。
 */
static void probe_persist_recovery_idle(void)
{
    if (xSemaphoreTake(s_mutex, 0) != pdTRUE)
    {
        return;
    }
    const bool pending = atomic_load(&s_persist_pending);
    const bool need_unlock =
        s_fault_gate.locked && !s_fault_gate.display_fault;
    if ((!pending && !need_unlock) || s_degraded)
    {
        xSemaphoreGive(s_mutex);
        return;
    }
    ewf_progress_transaction_t candidate = s_transaction;
    const esp_err_t save_error = persist_transaction_locked(&candidate);
    if (save_error == ESP_OK)
    {
        ESP_LOGI(TAG_FAULT, "落盘健康探测成功，persist_pending/fault 已按策略恢复");
    }
    xSemaphoreGive(s_mutex);
}

esp_err_t progress_service_get_persist_status(bool *persist_pending,
                                              bool *persist_inflight)
{
    if (persist_pending == NULL || persist_inflight == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *persist_pending = atomic_load(&s_persist_pending);
    *persist_inflight = atomic_load(&s_persist_inflight);
    return ESP_OK;
}

esp_err_t progress_service_set_display_fault(bool display_fault)
{
    if (s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    const ewf_fault_gate_decision_t decision =
        ewf_fault_gate_on_display_fault(&s_fault_gate, display_fault);
    esp_err_t publish_error = ESP_OK;
    if (decision.locked_changed)
    {
        publish_error = state_service_update_tap_fault_lock_owner(
            decision.locked, pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS));
        if (publish_error != ESP_OK)
        {
            ESP_LOGW(TAG_FAULT, "显示故障 fault_locked 发布失败，错误=%s",
                     esp_err_to_name(publish_error));
        }
        else if (decision.locked)
        {
            ESP_LOGW(TAG_FAULT, "已发布 fault_locked=true，拒绝新输入直至落盘恢复或清除显示故障");
        }
        else
        {
            ESP_LOGI(TAG_FAULT, "已发布 fault_locked=false，输入闸门恢复");
        }
    }
    if (display_fault)
    {
        ESP_LOGW(TAG_FAULT, "显示故障注入：fault_locked=%d（Epic 3 预留入口）",
                 (int)decision.locked);
    }
    xSemaphoreGive(s_mutex);
    return publish_error;
}

esp_err_t progress_service_request_round_action(
    ewf_progress_round_action_t action,
    const char *action_id,
    TickType_t timeout_ticks)
{
    if (s_mutex == NULL || !atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (action_id == NULL || action_id[0] == '\0')
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_degraded)
    {
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "进度 owner 处于 fail-closed，拒绝跨轮动作");
        return ESP_ERR_INVALID_STATE;
    }

    ewf_progress_transaction_t candidate = s_transaction;
    const ewf_progress_round_action_result_t result =
        ewf_progress_apply_round_action(&candidate,
                                        action,
                                        action_id,
                                        EWF_SCRIPTURE_CONSUMABLE_COUNT);
    if (result == EWF_PROGRESS_ROUND_ACTION_IDEMPOTENT)
    {
        xSemaphoreGive(s_mutex);
        ESP_LOGI(TAG, "跨轮动作幂等命中：action=%d，action_id 已应用", (int)action);
        return ESP_OK;
    }
    if (result == EWF_PROGRESS_ROUND_ACTION_REJECTED)
    {
        const bool locked = ewf_progress_is_completion_locked(&s_transaction);
        xSemaphoreGive(s_mutex);
        if (!locked)
        {
            ESP_LOGW(TAG, "跨轮动作拒绝：未处于完成锁定，action=%d", (int)action);
        }
        else if (action == EWF_PROGRESS_ROUND_ACTION_RESTART &&
                 s_transaction.round_id == UINT32_MAX)
        {
            ESP_LOGE(TAG, "跨轮重启拒绝：round_id 已达上限，禁止回绕");
        }
        else
        {
            ESP_LOGW(TAG, "跨轮动作拒绝：action=%d", (int)action);
        }
        return ESP_ERR_INVALID_ARG;
    }

    const TickType_t publish_ticks =
        (timeout_ticks == 0) ? pdMS_TO_TICKS(PROGRESS_PUBLISH_TIMEOUT_MS)
                             : timeout_ticks;
    const esp_err_t save_error = persist_transaction_locked(&candidate);
    if (save_error != ESP_OK)
    {
        (void)publish_progress_facts_locked(publish_ticks, true);
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "跨轮动作落盘失败，错误=%s；事务组保持不变",
                 esp_err_to_name(save_error));
        return save_error;
    }
    const esp_err_t publish_error =
        publish_progress_facts_locked(publish_ticks, false);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "跨轮动作快照发布失败，错误=%s",
                 esp_err_to_name(publish_error));
    }
    const esp_err_t gate_error = publish_gate_facts_locked(publish_ticks);
    if (gate_error != ESP_OK)
    {
        ESP_LOGW(TAG, "跨轮动作 gate 发布失败，错误=%s",
                 esp_err_to_name(gate_error));
    }
    ESP_LOGI(TAG,
             "跨轮动作已应用：action=%d，round=%lu，cursor=%lu，pending=%d，state=%d",
             (int)action,
             (unsigned long)s_transaction.round_id,
             (unsigned long)s_transaction.round_cursor,
             (int)s_transaction.pending_completion,
             (int)s_transaction.round_state);
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static uint32_t current_shanghai_day_key(const watch_state_snapshot_t *snap)
{
    if (snap == NULL || !snap->time_synchronized) {
        return 0U;
    }
    const int64_t mono_ms = esp_timer_get_time() / 1000LL;
    const int64_t utc_ms =
        ewf_today_utc_ms_from_offset(mono_ms, snap->time_offset_ms);
    return ewf_today_day_key_from_utc_ms(utc_ms);
}

static void refresh_today_bucket_locked(bool bump_on_tap)
{
    watch_state_snapshot_t snap;
    memset(&snap, 0, sizeof(snap));
    if (watch_state_snapshot(&snap, 0) != ESP_OK) {
        return;
    }
    if (!snap.time_synchronized) {
        /* 失信：不写入桶；展示层按 time_synchronized 显「待校时」。 */
        return;
    }
    const uint32_t day_key = current_shanghai_day_key(&snap);
    if (day_key == 0U) {
        return;
    }
    bool changed = false;
    if (bump_on_tap) {
        changed = ewf_today_bucket_on_trusted_tap(&s_today_bucket, day_key);
    } else {
        changed = ewf_today_bucket_align_day(&s_today_bucket, day_key);
    }
    if (changed) {
        const esp_err_t save_error = today_bucket_store_save(&s_today_bucket);
        if (save_error != ESP_OK) {
            ESP_LOGW(TAG, "今日桶落盘失败：%s（累计不受影响）",
                     esp_err_to_name(save_error));
        }
    }
}

static esp_err_t publish_progress_facts_locked(TickType_t timeout_ticks,
                                               bool persist_error)
{
    watch_state_snapshot_t snap;
    memset(&snap, 0, sizeof(snap));
    (void)watch_state_snapshot(&snap, 0);
    const uint32_t day_key = current_shanghai_day_key(&snap);
    const uint32_t today_display = ewf_today_bucket_display_count(
        snap.time_synchronized, &s_today_bucket, day_key);

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
        .today_count = today_display,
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
