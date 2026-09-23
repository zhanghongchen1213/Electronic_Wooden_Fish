/**
 * @file     feedback_service.c
 * @brief    统一反馈服务实现。
 * @details  消费 event_bus 扇出的 EWF_TAP_EVENT_VALID 事件（纯消费者），经
 *           feedback_policy 纯逻辑决策后驱动音频与 RGB 通道；音量事实由本服务
 *           owner 持有并经 audio_volume_store 单事务持久化；反馈通道故障按低
 *           打扰原则降级跳过并限频发布 typed 事实，不重试风暴、不置位输入锁定
 *           （tap_fault_locked 生产者属 Story 2.7）。反馈不推进累计/游标/持久化，
 *           也不阻塞 progress_service 的落盘路径（AD-12）。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "feedback_service.h"

#include <stdatomic.h>
#include <string.h>

#include "audio_volume_store.h"
#include "esp_log.h"
#include "event_bus.h"
#include "feedback_audio.h"
#include "feedback_policy.h"
#include "freertos/semphr.h"
#include "rgb_bsp.h"
#include "state_service.h"
#include "tap_input_service.h"

static const char *TAG = "SVC_FEEDBACK";

/** 反馈请求 typed 队列深度；可合并型队列，队满由 event_bus 扇出丢弃最新（AD-7）。 */
#define FEEDBACK_QUEUE_DEPTH 8U
/*
 * 队列元素必须与 event_bus 扇出投递的 legbot_event_t 完全一致：
 * 扇出按队列创建时的 item_size 按值复制，元素类型不一致会截断载荷。
 */
/** 空队列时的有界轮询等待。 */
#define FEEDBACK_CONSUME_TIMEOUT_MS 50U
/** 发布反馈事实时的有界队列等待。 */
#define FEEDBACK_PUBLISH_TIMEOUT_MS 20U
/** tick 到毫秒的换算基准。 */
#define FEEDBACK_MS_PER_TICK portTICK_PERIOD_MS

/** 保护 owner 私有状态（策略状态与音量事实）的互斥锁。 */
static SemaphoreHandle_t s_mutex;
/** 反馈请求 typed 队列（元素为 legbot_event_t）；event_bus 扇出写入，服务任务独占消费。 */
static QueueHandle_t s_queue;
/** 纯逻辑策略状态；owner 私有。 */
static ewf_feedback_policy_state_t s_policy;
/** 当前生效音量；owner 私有，0-100。 */
static uint8_t s_volume;
/** 最近一次音量落盘/恢复失败事实。 */
static bool s_volume_persist_error;
/** 本启动周期累计完成输出的反馈事件数。 */
static uint32_t s_feedback_count;
/** 本启动周期因限速合并而跳过输出的事件数。 */
static uint32_t s_merged_count;
/** prepared 状态：音量恢复已完成且任务允许进入消费循环。 */
static atomic_bool s_prepared;
/** 停止请求标志；由 request_stop 置位，消费循环有界轮询。 */
static atomic_bool s_stop_requested;
/** 音频链路初始化是否已尝试（失败后不重试风暴，下次事件按 NOT_READY 降级）。 */
static bool s_audio_init_done;
/** RGB 初始化是否已尝试。 */
static bool s_rgb_init_done;

static esp_err_t restore_volume_locked(void);
static esp_err_t publish_facts_locked(TickType_t timeout_ticks,
                                      uint32_t event_sequence);
static esp_err_t handle_tap_event_locked(uint32_t event_sequence);
static void ensure_channel_initialized_locked(void);
static uint32_t now_ms(void);

esp_err_t feedback_service_init_contracts(void)
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
    s_queue = xQueueCreate(FEEDBACK_QUEUE_DEPTH, sizeof(legbot_event_t));
    if (s_queue == NULL)
    {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
        return ESP_ERR_NO_MEM;
    }
    ewf_feedback_policy_init(&s_policy);
    s_volume = EWF_FEEDBACK_VOLUME_DEFAULT;
    s_volume_persist_error = false;
    s_feedback_count = 0U;
    s_merged_count = 0U;
    s_audio_init_done = false;
    s_rgb_init_done = false;
    atomic_store(&s_prepared, false);
    atomic_store(&s_stop_requested, false);
    return ESP_OK;
}

esp_err_t feedback_service_prepare_run(void)
{
    if (s_mutex == NULL || s_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    ewf_feedback_policy_init(&s_policy);
    s_volume = EWF_FEEDBACK_VOLUME_DEFAULT;
    s_volume_persist_error = false;
    s_feedback_count = 0U;
    s_merged_count = 0U;
    s_audio_init_done = false;
    s_rgb_init_done = false;
    atomic_store(&s_stop_requested, false);

    const esp_err_t restore_error = restore_volume_locked();
    if (restore_error != ESP_OK)
    {
        /* 恢复失败不阻断反馈服务：保持内存默认值并携带持久化失败事实。 */
        ESP_LOGE(TAG, "音量恢复失败，错误=%s；保持内存默认音量=%u",
                 esp_err_to_name(restore_error), (unsigned)s_volume);
        s_volume_persist_error = true;
    }
    atomic_store(&s_prepared, true);

    const esp_err_t publish_error =
        publish_facts_locked(pdMS_TO_TICKS(FEEDBACK_PUBLISH_TIMEOUT_MS), 0U);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "恢复初值反馈事实发布失败，错误=%s", esp_err_to_name(publish_error));
    }
    xSemaphoreGive(s_mutex);
    /* 反馈服务是 event_bus 的扇出订阅方：主队列消费者（progress owner）语义不变。
     * 订阅在 prepared 阶段完成，保证服务任务运行前的事件也能被扇出接收。 */
    if (event_bus_subscribe(s_queue) != ESP_OK)
    {
        ESP_LOGE(TAG, "event_bus 扇出订阅失败，反馈服务无法消费有效敲击");
        atomic_store(&s_prepared, false);
        return ESP_ERR_INVALID_STATE;
    }
    ESP_LOGI(TAG,
             "反馈服务已就绪：音量=%u，audio_config_version=%u，故障输入锁定生产者属 Story 2.7",
             (unsigned)s_volume, (unsigned)EWF_FEEDBACK_AUDIO_CONFIG_VERSION);
    return ESP_OK;
}

void feedback_service_cancel_prepared_run(void)
{
    if (s_mutex != NULL && xSemaphoreTake(s_mutex, 0) == pdTRUE)
    {
        atomic_store(&s_prepared, false);
        xSemaphoreGive(s_mutex);
    }
}

esp_err_t feedback_service_deinit_contracts(void)
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
    if (s_queue != NULL)
    {
        vQueueDelete(s_queue);
        s_queue = NULL;
    }
    vSemaphoreDelete(s_mutex);
    s_mutex = NULL;
    return ESP_OK;
}

esp_err_t feedback_service_run(void)
{
    if (s_mutex == NULL || s_queue == NULL || !atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    legbot_event_t event = {0};
    for (;;)
    {
        if (xQueueReceive(s_queue, &event,
                          pdMS_TO_TICKS(FEEDBACK_CONSUME_TIMEOUT_MS)) == pdTRUE)
        {
            /* 扇出投递全部事件类型：反馈服务只消费有效敲击事实，其余忽略。 */
            if (event.code == EWF_TAP_EVENT_VALID)
            {
                if (xSemaphoreTake(s_mutex, portMAX_DELAY) == pdTRUE)
                {
                    (void)handle_tap_event_locked(event.value);
                    xSemaphoreGive(s_mutex);
                }
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

esp_err_t feedback_service_request_stop(TickType_t timeout_ticks)
{
    (void)timeout_ticks;
    if (s_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, true);
    return ESP_OK;
}

esp_err_t feedback_service_set_volume(uint8_t volume, TickType_t timeout_ticks)
{
    if (s_mutex == NULL || !atomic_load(&s_prepared))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (!ewf_feedback_volume_valid(volume))
    {
        ESP_LOGW(TAG, "音量写入被拒绝：取值域恰为 0-100，提交值=%u", (unsigned)volume);
        return ESP_ERR_INVALID_ARG;
    }
    if (xSemaphoreTake(s_mutex, portMAX_DELAY) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (volume == s_volume)
    {
        /* 同值 no-op：不重复落盘、不重复发布。 */
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }
    ewf_feedback_volume_record_t record = {0};
    const ewf_feedback_volume_result_t packed =
        ewf_feedback_volume_record_pack(volume, &record);
    if (packed != EWF_FEEDBACK_VOLUME_OK)
    {
        xSemaphoreGive(s_mutex);
        ESP_LOGW(TAG, "音量记录打包被拒绝：结果=%d", (int)packed);
        return ESP_ERR_INVALID_ARG;
    }
    /* 先持久化成功再提交内存（与 progress 事务同序）：介质错误时保持原内存值。 */
    const esp_err_t save_error = ewf_audio_volume_store_save(&record);
    if (save_error != ESP_OK)
    {
        s_volume_persist_error = true;
        (void)publish_facts_locked(timeout_ticks, 0U);
        xSemaphoreGive(s_mutex);
        ESP_LOGE(TAG, "音量落盘失败，错误=%s；内存音量保持原值=%u",
                 esp_err_to_name(save_error), (unsigned)s_volume);
        return save_error;
    }
    s_volume = volume;
    s_volume_persist_error = false;
    const esp_err_t publish_error = publish_facts_locked(timeout_ticks, 0U);
    if (publish_error != ESP_OK)
    {
        ESP_LOGW(TAG, "音量事实发布失败，错误=%s", esp_err_to_name(publish_error));
    }
    xSemaphoreGive(s_mutex);
    ESP_LOGI(TAG, "音量已更新并落盘：volume=%u", (unsigned)volume);
    return ESP_OK;
}

static esp_err_t restore_volume_locked(void)
{
    ewf_feedback_volume_record_t record = {0};
    ewf_audio_store_status_t status = EWF_AUDIO_STORE_ERROR;
    const esp_err_t load_error = ewf_audio_volume_store_load(&record, &status);
    if (load_error != ESP_OK)
    {
        return load_error;
    }
    if (status == EWF_AUDIO_STORE_EMPTY)
    {
        /* 首启缺字段：按默认 50 初始化并落盘；落盘失败只告警，反馈照常可用。 */
        ewf_feedback_volume_record_t initial = {0};
        (void)ewf_feedback_volume_record_pack(EWF_FEEDBACK_VOLUME_DEFAULT, &initial);
        const esp_err_t save_error = ewf_audio_volume_store_save(&initial);
        if (save_error != ESP_OK)
        {
            ESP_LOGE(TAG, "首启音量初始落盘失败，错误=%s", esp_err_to_name(save_error));
            s_volume_persist_error = true;
            return ESP_OK;
        }
        s_volume = EWF_FEEDBACK_VOLUME_DEFAULT;
        s_volume_persist_error = false;
        return ESP_OK;
    }
    if (status == EWF_AUDIO_STORE_ERROR)
    {
        /* 介质错误 fail-closed：报错 + 保持内存默认值，不静默回退伪造。 */
        return ESP_FAIL;
    }
    const ewf_feedback_record_result_t valid = ewf_feedback_volume_record_validate(&record);
    if (valid != EWF_FEEDBACK_RECORD_OK)
    {
        ESP_LOGE(TAG, "音量恢复校验失败：结果=%d；保持内存默认值", (int)valid);
        return ESP_ERR_INVALID_STATE;
    }
    s_volume = record.volume;
    s_volume_persist_error = false;
    return ESP_OK;
}

static esp_err_t handle_tap_event_locked(uint32_t event_sequence)
{
    ensure_channel_initialized_locked();
    const uint32_t now = now_ms();

    const ewf_feedback_audio_decision_t audio_decision =
        ewf_feedback_audio_decide(&s_policy, now);
    const ewf_feedback_rgb_decision_t rgb_decision =
        ewf_feedback_rgb_decide(&s_policy, now);

    bool audio_ok = true;
    bool audio_played = false;
    watch_feedback_error_t audio_error = WATCH_FEEDBACK_ERROR_NONE;
    if (audio_decision == EWF_FEEDBACK_AUDIO_PLAY)
    {
        const esp_err_t play_error = ewf_feedback_audio_backend_play(s_volume);
        if (play_error == ESP_OK)
        {
            audio_ok = true;
            /* 音量 0 静音路径同样算完成反馈处理（等价静音是 AC 2 的正常输出）。 */
            audio_played = true;
        }
        else
        {
            /* 播放失败降级：安全关断功放并跳过本次音频（确定性降级，不重试风暴）。 */
            (void)ewf_feedback_audio_backend_ensure_pa_off();
            audio_ok = false;
            audio_error = ewf_feedback_audio_backend_typed_error();
        }
        const bool should_log = ewf_feedback_note_audio_result(
            &s_policy, now, now, audio_ok);
        if (!audio_ok && should_log)
        {
            ESP_LOGE(TAG, "音频反馈失败，降级跳过本次播放：事件序号=%lu，typed=%d",
                     (unsigned long)event_sequence, (int)audio_error);
        }
    }
    else if (s_merged_count < 0xFFFFFFFFU)
    {
        /* 合并跳过：只累计合并计数，输出保持既有节奏（不叠加成噪声，AD-12）。 */
        ++s_merged_count;
    }

    bool rgb_ok = true;
    if (rgb_decision == EWF_FEEDBACK_RGB_UPDATE)
    {
        const esp_err_t flash_error = rgb_bsp_tap_flash();
        if (flash_error != ESP_OK)
        {
            /* RGB 失败降级：熄灭并跳过本次灯效（不重试风暴）。 */
            (void)rgb_bsp_off();
            rgb_ok = false;
        }
        const bool should_log = ewf_feedback_note_rgb_result(&s_policy, now, now, rgb_ok);
        if (!rgb_ok && should_log)
        {
            ESP_LOGE(TAG, "RGB 反馈失败，降级跳过本次灯效：事件序号=%lu，稳定错误码=%s",
                     (unsigned long)event_sequence, rgb_bsp_error_code(flash_error));
        }
    }

    /* 完成输出的事件：音频实际播放或 RGB 实际闪任一产出（含静音正常路径）。 */
    if ((audio_played || rgb_ok) &&
        (rgb_decision == EWF_FEEDBACK_RGB_UPDATE || audio_played))
    {
        if (s_feedback_count < 0xFFFFFFFFU)
        {
            ++s_feedback_count;
        }
    }
    return publish_facts_locked(pdMS_TO_TICKS(FEEDBACK_PUBLISH_TIMEOUT_MS),
                                event_sequence);
}

static void ensure_channel_initialized_locked(void)
{
    if (!s_audio_init_done)
    {
        const esp_err_t init_error = ewf_feedback_audio_backend_init();
        if (init_error != ESP_OK)
        {
            ESP_LOGW(TAG, "音频链路初始化失败，音频反馈降级：错误=%s",
                     esp_err_to_name(init_error));
        }
        s_audio_init_done = true;
    }
    if (!s_rgb_init_done)
    {
        const esp_err_t init_error = rgb_bsp_init();
        if (init_error != ESP_OK)
        {
            ESP_LOGW(TAG, "RGB 初始化失败，RGB 反馈降级：稳定错误码=%s，错误=%s",
                     rgb_bsp_error_code(init_error), esp_err_to_name(init_error));
        }
        s_rgb_init_done = true;
    }
}

static esp_err_t publish_facts_locked(TickType_t timeout_ticks,
                                      uint32_t event_sequence)
{
    watch_feedback_update_t facts = {0};
    facts.last_event_sequence = event_sequence;
    facts.feedback_count = s_feedback_count;
    facts.merged_count = s_merged_count;
    facts.volume = s_volume;
    facts.volume_persist_error = s_volume_persist_error;
    facts.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].channel = WATCH_FEEDBACK_CHANNEL_AUDIO;
    facts.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].state =
        s_policy.audio_fault_active ? WATCH_FEEDBACK_CHANNEL_FAILED
                                    : WATCH_FEEDBACK_CHANNEL_OK;
    facts.channels[WATCH_FEEDBACK_CHANNEL_AUDIO].error =
        s_policy.audio_fault_active ? ewf_feedback_audio_backend_typed_error()
                                    : WATCH_FEEDBACK_ERROR_NONE;
    facts.channels[WATCH_FEEDBACK_CHANNEL_RGB].channel = WATCH_FEEDBACK_CHANNEL_RGB;
    facts.channels[WATCH_FEEDBACK_CHANNEL_RGB].state =
        s_policy.rgb_fault_active ? WATCH_FEEDBACK_CHANNEL_FAILED
                                  : WATCH_FEEDBACK_CHANNEL_OK;
    facts.channels[WATCH_FEEDBACK_CHANNEL_RGB].error =
        s_policy.rgb_fault_active ? WATCH_FEEDBACK_ERROR_RGB_FAILED
                                  : WATCH_FEEDBACK_ERROR_NONE;
    /* 屏幕通道只预留承载位：生产者属 Epic 3 显示链路，本 Story 保持 OK 空事实。 */
    facts.channels[WATCH_FEEDBACK_CHANNEL_SCREEN].channel = WATCH_FEEDBACK_CHANNEL_SCREEN;
    facts.channels[WATCH_FEEDBACK_CHANNEL_SCREEN].state = WATCH_FEEDBACK_CHANNEL_OK;
    facts.channels[WATCH_FEEDBACK_CHANNEL_SCREEN].error = WATCH_FEEDBACK_ERROR_NONE;
    return state_service_update_feedback_owner(&facts, timeout_ticks);
}

static uint32_t now_ms(void)
{
    return (uint32_t)(xTaskGetTickCount() * FEEDBACK_MS_PER_TICK);
}
