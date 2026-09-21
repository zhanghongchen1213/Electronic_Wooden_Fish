/**
 * @file     audio_service.c
 * @brief    SPIFFS 音频资源、UI 反馈与语音采集执行实现
 * @details  由 audio_task 串行执行振动反馈、固定 WAV 播放与 ES8311 ADC 采集，启动时预留 I2S TX/RX DMA。
 * @author   ZHC
 * @date     2026-08-05
 */

#include "audio_service.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "es8311_bsp.h"
#include "freertos/queue.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "ns4150_bsp.h"
#include "state_service.h"
#include "vibration_bsp.h"

static const char *TAG = "SVC_AUDIO";

/** WAV RIFF 前导长度。 */
#define AUDIO_WAV_RIFF_PREFIX_SIZE 12U
/** WAV chunk 头长度。 */
#define AUDIO_WAV_CHUNK_HEADER_SIZE 8U
/** PCM fmt chunk 的最小字段长度。 */
#define AUDIO_WAV_FMT_MIN_SIZE 16U
/** 限制异常 WAV 的 chunk 数，防止损坏资源形成长时间解析循环。 */
#define AUDIO_WAV_MAX_CHUNKS 64U
/** 本 Story 唯一接受的 PCM 编码格式。 */
#define AUDIO_WAV_PCM_FORMAT 1U
/** 本 Story 唯一接受的声道数。 */
#define AUDIO_WAV_CHANNELS 1U
/** 本 Story 唯一接受的采样率。 */
#define AUDIO_WAV_SAMPLE_RATE_HZ 16000U
/** 本 Story 唯一接受的采样位数。 */
#define AUDIO_WAV_BITS_PER_SAMPLE 16U
/** 本 Story 唯一接受的采样帧字节数。 */
#define AUDIO_WAV_BLOCK_ALIGN 2U
/** 本 Story 唯一接受的 PCM 字节率。 */
#define AUDIO_WAV_BYTE_RATE 32000U
/** 有效音频后写入一整圈 DMA 的零采样，确保 PCM 已物理送出。 */
#define AUDIO_OUTPUT_DRAIN_SAMPLE_COUNT \
    (AUDIO_BSP_DMA_DESC_NUM * AUDIO_BSP_DMA_FRAME_NUM)
/** 语音 PCM Stream Buffer 的可用数据容量。 */
#define AUDIO_SERVICE_VOICE_PCM_BUFFER_BYTES \
    (AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES * AUDIO_SERVICE_VOICE_PCM_FRAME_COUNT)
/** 静态 Stream Buffer 额外保留一个内部哨兵字节。 */
#define AUDIO_SERVICE_VOICE_PCM_STORAGE_BYTES \
    (AUDIO_SERVICE_VOICE_PCM_BUFFER_BYTES + 1U)
/** Stream Buffer 之后追加一帧 audio owner 采集工作区，避免占用任务栈。 */
#define AUDIO_SERVICE_VOICE_PCM_WORKSPACE_BYTES \
    (AUDIO_SERVICE_VOICE_PCM_STORAGE_BYTES + \
     AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES)

_Static_assert(NS4150_BSP_STARTUP_SETTLE_MS >= 120U,
               "NS4150B startup settle must cover the 120 ms typical value.");
_Static_assert(AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES % sizeof(int16_t) == 0U,
               "Voice PCM frame must contain complete 16-bit samples.");

/** 单次 UI 按钮反馈请求。 */
typedef struct
{
    bool haptics_enabled;    /**< 点击发生时是否启用触觉反馈。 */
    bool click_audio_enabled; /**< 点击发生时是否启用点击音。 */
} audio_ui_feedback_request_t;

/** 编译期固定资源路径表，调用者不能注入任意文件路径。 */
static const char *const s_resource_paths[AUDIO_RESOURCE_COUNT] = {
    [AUDIO_RESOURCE_SELFTEST_OK] = "/spiffs/audio/selftest_16k_mono.wav",
    [AUDIO_RESOURCE_UI_CLICK] = "/spiffs/audio/ui_click_16k_mono.wav",
    [AUDIO_RESOURCE_VOICE_START] = "/spiffs/audio/voice_start_16k_mono.wav",
};

/** 本模块是否拥有当前 SPIFFS 注册，只有 owner 才能注销。 */
static bool s_spiffs_owned;
/** 全部必需固定资源是否已通过严格探测。 */
static atomic_bool s_assets_ready;
/** storage 挂载或注销是否正在改变 VFS 生命周期。 */
static atomic_bool s_storage_transitioning;
/** audio_task 私有 typed 命令队列。 */
static QueueHandle_t s_audio_queue;
/** audio_task 私有 UI 按钮反馈队列。 */
static QueueHandle_t s_ui_feedback_queue;
/** audio_task 向 voice_task 发布会话状态的事件队列。 */
static QueueHandle_t s_voice_event_queue;
/** PSRAM PCM Stream Buffer 的静态控制块。 */
static StaticStreamBuffer_t s_voice_pcm_stream_control;
/** PSRAM PCM Stream Buffer 的显式存储区。 */
static uint8_t *s_voice_pcm_storage;
/** audio_task 独占的 PSRAM 单帧采集工作区。 */
static uint8_t *s_voice_capture_frame;
/** 单生产者、单消费者语音 PCM Stream Buffer。 */
static StreamBufferHandle_t s_voice_pcm_stream;
/** 唯一 audio_task 句柄，用于请求入队后的事件唤醒。 */
static TaskHandle_t s_audio_task;
/** 保护跨核读取、通知和清空 audio_task 句柄的自旋锁。 */
static portMUX_TYPE s_audio_task_lock = portMUX_INITIALIZER_UNLOCKED;
/** audio_task 是否正在使用 typed queue。 */
static atomic_bool s_task_running;
/** audio_task 是否已接受 shutdown 且不再接受新命令。 */
static atomic_bool s_task_stopping;
/** audio_task 是否已离开命令循环并正在执行最终清理。 */
static atomic_bool s_task_exiting;
/** 硬件仍需由 audio_task 重试安全恢复。 */
static atomic_bool s_recovery_pending;
/** 恢复路径必须优先重试 ADC/RX 停止而不是 TX 停止。 */
static atomic_bool s_voice_recovery_pending;
/** 是否已有排队但尚未开始的固定资源播放。 */
static atomic_bool s_play_pending;
/** 是否已有排队但尚未消费的单次播放取消命令。 */
static atomic_bool s_cancel_pending;
/** 跨播放保持单调且跳过零值的播放代次。 */
static atomic_uint s_next_play_generation;
/** 当前排队播放的非零代次。 */
static atomic_uint s_queued_play_generation;
/** 当前正在输出资源播放的非零代次。 */
static atomic_uint s_active_play_generation;
/** audio_task 最近一次成功认领的播放代次，退出清理期间仍保留。 */
static atomic_uint s_last_claimed_play_generation;
/** 当前取消命令锁定的非零播放代次。 */
static atomic_uint s_cancel_target_generation;
/** audio_task 最近确认完成的取消代次。 */
static atomic_uint s_cancel_ack_generation;
/** audio_task 是否正在执行资源或 UI 反馈闭环。 */
static atomic_bool s_play_active;
/** audio_task 是否正在执行固定资源播放。 */
static atomic_bool s_resource_play_active;
/** 是否已有语音会话启动命令待 audio_task 认领。 */
static atomic_bool s_voice_pending;
/** 当前排队语音会话的非零代次。 */
static atomic_uint s_queued_voice_generation;
/** 当前由 audio_task 执行提示音或采集的非零代次。 */
static atomic_uint s_active_voice_generation;
/** 调用方请求立即停止的非零语音会话代次。 */
static atomic_uint s_voice_stop_generation;
/** PCM Stream Buffer 当前所属的非零语音会话代次。 */
static atomic_uint s_voice_pcm_generation;
/** ES8311 RX 当前是否由语音会话持有。 */
static atomic_bool s_voice_capture_active;
/** 最新触觉反馈开关，用于抑制关闭后的旧请求。 */
static atomic_bool s_ui_haptics_enabled;
/** 最新点击音开关，用于抑制关闭后的旧请求。 */
static atomic_bool s_ui_click_audio_enabled;
/** 最近一次成功读取的 power owner 电量事实。 */
static atomic_int s_last_power_level =
    ATOMIC_VAR_INIT(WATCH_POWER_LEVEL_NORMAL);
/** storage/assets 最近判定耗时。 */
static atomic_uint_fast32_t s_storage_verdict_ms;
/** 最近 play 请求到 terminal state 的耗时。 */
static atomic_uint_fast32_t s_playback_elapsed_ms;
/** 最近一次 stop 延迟。 */
static atomic_uint_fast32_t s_stop_latency_ms;
/** audio_task 最近一次栈高水位。 */
static atomic_uint_fast32_t s_stack_high_water_words;
/** 最近一次语音会话观察到的 I2S RX timeout 次数。 */
static atomic_uint_fast32_t s_voice_rx_timeout_count;
/** 分块路径观测到的最大连续处理墙钟上界。 */
static atomic_uint_fast32_t s_max_processing_us;
/** 公开资源探测正在持有 SPIFFS 文件的调用数。 */
static atomic_uint_fast32_t s_storage_readers;
/** 最近一次 storage/assets 阶段的 typed 结果，供队列就绪后补发。 */
static atomic_int s_storage_result = ATOMIC_VAR_INIT(AUDIO_RESULT_NOT_READY);
/** 未成功交付给 state_task 的最新 terminal 状态。 */
static watch_audio_update_t s_pending_terminal_update;
/** 未交付 terminal 状态对应的 play 接受时刻。 */
static int64_t s_pending_terminal_accepted_at_us;
/** 是否存在等待重试的 terminal 状态。 */
static atomic_bool s_terminal_pending;
/** audio_task 是否已完成本轮硬件资源初始化尝试。 */
static atomic_bool s_startup_complete;

static uint16_t read_le16(const uint8_t *bytes);
static uint32_t read_le32(const uint8_t *bytes);
static audio_service_result_t read_exact(int file_descriptor,
                                         void *buffer,
                                         size_t size);
static audio_service_result_t seek_absolute(int file_descriptor,
                                            uint64_t offset,
                                            uint64_t file_size);
static audio_service_result_t probe_resource_file(audio_resource_id_t resource_id,
                                                  audio_wav_info_t *info);
static audio_service_result_t parse_wav(int file_descriptor,
                                        audio_wav_info_t *info);
static esp_err_t result_to_esp_error(audio_service_result_t result);
static void complete_storage_transition(int64_t started_at_us,
                                        audio_service_result_t result);
static audio_service_result_t play_resource(audio_resource_id_t resource_id,
                                            bool *shutdown_requested,
                                            bool *playback_cancelled,
                                            uint32_t voice_generation,
                                            bool *voice_cancelled);
static void play_ui_feedback(const audio_ui_feedback_request_t *request,
                             bool *shutdown_requested);
static bool current_output_allowed(audio_service_priority_t priority);
static bool current_ui_feedback_allowed(void);
static watch_power_level_t current_power_level(void);
static esp_err_t play_ui_haptic(void);
static audio_service_result_t write_ui_pcm(const void *data,
                                           size_t size,
                                           bool *shutdown_requested,
                                           bool *playback_cancelled,
                                           uint32_t voice_generation,
                                           bool *voice_cancelled);
static bool take_output_end_request(bool *shutdown_requested,
                                    bool *playback_cancelled,
                                    uint32_t voice_generation,
                                    bool *voice_cancelled);
static void apply_stop_request(const audio_service_request_t *request,
                               bool *shutdown_requested);
static bool take_stop_request(bool *shutdown_requested);
static bool take_playback_end_request(bool *shutdown_requested,
                                      bool *playback_cancelled);
static void handle_pending_playback_cancel(
    const audio_service_request_t *cancel_request,
    bool *shutdown_requested);
static void acknowledge_playback_cancel(uint32_t generation);
static uint32_t next_play_generation(void);
static bool voice_stop_requested(uint32_t generation);
static void publish_voice_event(audio_service_voice_event_type_t type,
                                uint32_t generation,
                                audio_service_result_t result);
static void run_voice_capture_session(uint32_t generation,
                                      bool *shutdown_requested);
static bool wait_voice_settle(uint32_t generation,
                              uint32_t settle_ms,
                              bool *shutdown_requested);
static audio_service_result_t capture_voice_pcm(
    uint32_t generation,
    bool *shutdown_requested,
    bool *timed_out);
static audio_service_result_t finish_voice_capture(
    audio_service_result_t result);
static void update_processing_metric(int64_t started_at_us);
static audio_service_result_t map_es8311_failure(void);
static void log_dma_heap(const char *stage);
static esp_err_t probe_hardware_ready(void);
static esp_err_t safe_off_with_retry(void);
static audio_service_result_t finish_playback(int file_descriptor,
                                              audio_service_result_t result);
static audio_service_result_t finish_voice_cue_playback(
    int file_descriptor,
    audio_service_result_t result);
static audio_service_result_t shutdown_audio_hardware(
    audio_service_result_t result);
static watch_audio_error_t map_watch_error(audio_service_result_t result);
static void record_playback_terminal(int64_t accepted_at_us);
static esp_err_t flush_pending_terminal(void);
static esp_err_t publish_audio_state(watch_audio_state_t state,
                                     watch_audio_error_t error,
                                     bool terminal,
                                     int64_t accepted_at_us);
static void wake_audio_task(void);

bool audio_service_output_allowed(
    watch_power_level_t power_level,
    audio_service_priority_t priority)
{
    if (power_level < WATCH_POWER_LEVEL_NORMAL ||
        power_level >= WATCH_POWER_LEVEL_COUNT ||
        (priority != AUDIO_PRIORITY_DEFAULT &&
         priority != AUDIO_PRIORITY_SELFTEST))
    {
        return false;
    }
    return power_level !=
               WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY ||
           priority == AUDIO_PRIORITY_SELFTEST;
}

bool audio_service_ui_feedback_allowed(
    watch_power_level_t power_level)
{
    return power_level >= WATCH_POWER_LEVEL_NORMAL &&
           power_level < WATCH_POWER_LEVEL_COUNT &&
           power_level !=
               WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY;
}

audio_ui_feedback_output_t audio_service_resolve_ui_feedback(
    bool requested_haptics,
    bool requested_click_audio,
    bool current_haptics,
    bool current_click_audio)
{
    const audio_ui_feedback_output_t output = {
        .play_haptics = requested_haptics && current_haptics,
        .play_click_audio =
            requested_click_audio && current_click_audio,
    };
    return output;
}

esp_err_t audio_service_storage_init(audio_service_result_t *result)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_storage_transitioning, &expected, true))
    {
        *result = AUDIO_RESULT_BUSY;
        return ESP_ERR_INVALID_STATE;
    }
    const int64_t started_at_us = esp_timer_get_time();
    *result = AUDIO_RESULT_NOT_READY;
    atomic_store(&s_assets_ready, false);
    atomic_store(&s_storage_result, *result);

    if (!esp_spiffs_mounted(AUDIO_SERVICE_SPIFFS_PARTITION_LABEL))
    {
        const esp_vfs_spiffs_conf_t config = {
            .base_path = AUDIO_SERVICE_SPIFFS_BASE_PATH,
            .partition_label = AUDIO_SERVICE_SPIFFS_PARTITION_LABEL,
            .max_files = AUDIO_SERVICE_SPIFFS_MAX_FILES,
            .format_if_mount_failed = false,
        };
        esp_err_t err = esp_vfs_spiffs_register(&config);
        if (err != ESP_OK)
        {
            *result = err == ESP_ERR_NOT_FOUND ? AUDIO_RESULT_RESOURCE_MISSING
                                               : AUDIO_RESULT_IO_FAILED;
            complete_storage_transition(started_at_us, *result);
            ESP_LOGW(TAG, "SPIFFS 音频分区挂载失败，稳定错误码=%s，错误=0x%x",
                     audio_service_result_code(*result), (unsigned)err);
            return err;
        }
        s_spiffs_owned = true;
    }

    size_t total_bytes = 0;
    size_t used_bytes = 0;
    esp_err_t err = esp_spiffs_info(AUDIO_SERVICE_SPIFFS_PARTITION_LABEL,
                                    &total_bytes,
                                    &used_bytes);
    if (err != ESP_OK)
    {
        *result = AUDIO_RESULT_IO_FAILED;
        complete_storage_transition(started_at_us, *result);
        return err;
    }

    uint32_t total_pcm_bytes = 0U;
    for (audio_resource_id_t resource_id = AUDIO_RESOURCE_SELFTEST_OK;
         resource_id < AUDIO_RESOURCE_COUNT;
         resource_id = (audio_resource_id_t)(resource_id + 1))
    {
        audio_wav_info_t info = {0};
        *result = probe_resource_file(resource_id, &info);
        if (*result != AUDIO_RESULT_OK)
        {
            complete_storage_transition(started_at_us, *result);
            ESP_LOGW(TAG,
                     "必需音频资源探测失败，资源=%d，路径=%s，稳定错误码=%s",
                     (int)resource_id,
                     audio_service_resource_path(resource_id),
                     audio_service_result_code(*result));
            return result_to_esp_error(*result);
        }
        total_pcm_bytes += info.data_size;
    }

    atomic_store(&s_assets_ready, true);
    complete_storage_transition(started_at_us, *result);
    ESP_LOGI(TAG,
             "SPIFFS 必需音频资源已就绪，数量=%u，已用=%u 字节，总计=%u 字节，PCM=%u 字节，判定=%u ms",
             (unsigned)AUDIO_RESOURCE_COUNT,
             (unsigned)used_bytes,
             (unsigned)total_bytes,
             (unsigned)total_pcm_bytes,
             (unsigned)atomic_load(&s_storage_verdict_ms));
    return ESP_OK;
}

esp_err_t audio_service_storage_deinit(void)
{
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_storage_transitioning, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (atomic_load(&s_play_pending) || atomic_load(&s_play_active) ||
        atomic_load(&s_voice_pending) ||
        atomic_load(&s_active_voice_generation) != 0U ||
        atomic_load(&s_storage_readers) != 0U)
    {
        atomic_store(&s_storage_transitioning, false);
        return ESP_ERR_INVALID_STATE;
    }

    atomic_store(&s_assets_ready, false);
    atomic_store(&s_storage_result, AUDIO_RESULT_NOT_READY);
    if (!s_spiffs_owned)
    {
        atomic_store(&s_storage_transitioning, false);
        return ESP_OK;
    }

    esp_err_t err = esp_vfs_spiffs_unregister(AUDIO_SERVICE_SPIFFS_PARTITION_LABEL);
    if (err == ESP_OK || err == ESP_ERR_INVALID_STATE)
    {
        s_spiffs_owned = false;
        atomic_store(&s_storage_transitioning, false);
        return ESP_OK;
    }
    atomic_store(&s_storage_transitioning, false);
    return err;
}

bool audio_service_storage_ready(void)
{
    return atomic_load(&s_assets_ready) && !atomic_load(&s_storage_transitioning) &&
           esp_spiffs_mounted(AUDIO_SERVICE_SPIFFS_PARTITION_LABEL);
}

const char *audio_service_resource_path(audio_resource_id_t resource_id)
{
    if (resource_id < 0 || resource_id >= AUDIO_RESOURCE_COUNT)
    {
        return NULL;
    }
    return s_resource_paths[resource_id];
}

audio_service_result_t audio_service_probe_resource(audio_resource_id_t resource_id,
                                                    audio_wav_info_t *info)
{
    if (atomic_load(&s_storage_transitioning))
    {
        return AUDIO_RESULT_NOT_READY;
    }
    atomic_fetch_add(&s_storage_readers, 1U);
    if (atomic_load(&s_storage_transitioning))
    {
        atomic_fetch_sub(&s_storage_readers, 1U);
        return AUDIO_RESULT_NOT_READY;
    }
    audio_service_result_t result = probe_resource_file(resource_id, info);
    atomic_fetch_sub(&s_storage_readers, 1U);
    return result;
}

const char *audio_service_result_code(audio_service_result_t result)
{
    switch (result)
    {
    case AUDIO_RESULT_OK:
        return "AUDIO_OK";
    case AUDIO_RESULT_NOT_READY:
        return "AUDIO_NOT_READY";
    case AUDIO_RESULT_RESOURCE_MISSING:
        return "AUDIO_RESOURCE_MISSING";
    case AUDIO_RESULT_IO_FAILED:
        return "AUDIO_IO_FAILED";
    case AUDIO_RESULT_CORRUPT:
        return "AUDIO_CORRUPT";
    case AUDIO_RESULT_UNSUPPORTED:
        return "AUDIO_UNSUPPORTED";
    case AUDIO_RESULT_BUSY:
        return "AUDIO_BUSY";
    case AUDIO_RESULT_HW_FAILED:
        return "AUDIO_HW_FAILED";
    case AUDIO_RESULT_CODEC_FAILED:
        return "AUDIO_CODEC_FAILED";
    case AUDIO_RESULT_I2S_FAILED:
        return "AUDIO_I2S_FAILED";
    case AUDIO_RESULT_PA_FAILED:
        return "AUDIO_PA_FAILED";
    case AUDIO_RESULT_PCM_OVERFLOW:
        return "AUDIO_PCM_OVERFLOW";
    default:
        return "AUDIO_UNKNOWN";
    }
}

esp_err_t audio_service_init_contracts(void)
{
    if (s_audio_queue != NULL && s_ui_feedback_queue != NULL &&
        s_voice_event_queue != NULL && s_voice_pcm_stream != NULL &&
        s_voice_capture_frame != NULL)
    {
        return ESP_OK;
    }
    if (s_audio_queue != NULL)
    {
        vQueueDelete(s_audio_queue);
        s_audio_queue = NULL;
    }
    if (s_ui_feedback_queue != NULL)
    {
        vQueueDelete(s_ui_feedback_queue);
        s_ui_feedback_queue = NULL;
    }
    if (s_voice_event_queue != NULL)
    {
        vQueueDelete(s_voice_event_queue);
        s_voice_event_queue = NULL;
    }
    if (s_voice_pcm_stream != NULL)
    {
        vStreamBufferDelete(s_voice_pcm_stream);
        s_voice_pcm_stream = NULL;
    }
    if (s_voice_pcm_storage != NULL)
    {
        s_voice_capture_frame = NULL;
        heap_caps_free(s_voice_pcm_storage);
        s_voice_pcm_storage = NULL;
    }
    s_audio_queue = xQueueCreate(AUDIO_SERVICE_QUEUE_DEPTH, sizeof(audio_service_request_t));
    if (s_audio_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    s_ui_feedback_queue = xQueueCreate(AUDIO_SERVICE_UI_FEEDBACK_QUEUE_DEPTH,
                                       sizeof(audio_ui_feedback_request_t));
    if (s_ui_feedback_queue == NULL)
    {
        vQueueDelete(s_audio_queue);
        s_audio_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    s_voice_event_queue = xQueueCreate(
        AUDIO_SERVICE_VOICE_EVENT_QUEUE_DEPTH,
        sizeof(audio_service_voice_event_t));
    if (s_voice_event_queue == NULL)
    {
        vQueueDelete(s_ui_feedback_queue);
        s_ui_feedback_queue = NULL;
        vQueueDelete(s_audio_queue);
        s_audio_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    s_voice_pcm_storage = heap_caps_malloc(
        AUDIO_SERVICE_VOICE_PCM_WORKSPACE_BYTES,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_voice_pcm_storage == NULL)
    {
        vQueueDelete(s_voice_event_queue);
        s_voice_event_queue = NULL;
        vQueueDelete(s_ui_feedback_queue);
        s_ui_feedback_queue = NULL;
        vQueueDelete(s_audio_queue);
        s_audio_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    s_voice_capture_frame =
        s_voice_pcm_storage + AUDIO_SERVICE_VOICE_PCM_STORAGE_BYTES;
    s_voice_pcm_stream = xStreamBufferCreateStatic(
        AUDIO_SERVICE_VOICE_PCM_BUFFER_BYTES,
        AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES,
        s_voice_pcm_storage,
        &s_voice_pcm_stream_control);
    if (s_voice_pcm_stream == NULL)
    {
        s_voice_capture_frame = NULL;
        heap_caps_free(s_voice_pcm_storage);
        s_voice_pcm_storage = NULL;
        vQueueDelete(s_voice_event_queue);
        s_voice_event_queue = NULL;
        vQueueDelete(s_ui_feedback_queue);
        s_ui_feedback_queue = NULL;
        vQueueDelete(s_audio_queue);
        s_audio_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    atomic_store(&s_play_pending, false);
    atomic_store(&s_cancel_pending, false);
    atomic_store(&s_next_play_generation, 0U);
    atomic_store(&s_queued_play_generation, 0U);
    atomic_store(&s_active_play_generation, 0U);
    atomic_store(&s_last_claimed_play_generation, 0U);
    atomic_store(&s_cancel_target_generation, 0U);
    atomic_store(&s_cancel_ack_generation, 0U);
    atomic_store(&s_play_active, false);
    atomic_store(&s_resource_play_active, false);
    atomic_store(&s_voice_pending, false);
    atomic_store(&s_queued_voice_generation, 0U);
    atomic_store(&s_active_voice_generation, 0U);
    atomic_store(&s_voice_stop_generation, 0U);
    atomic_store(&s_voice_pcm_generation, 0U);
    atomic_store(&s_voice_capture_active, false);
    atomic_store(&s_ui_haptics_enabled, false);
    atomic_store(&s_ui_click_audio_enabled, false);
    atomic_store(&s_last_power_level, WATCH_POWER_LEVEL_NORMAL);
    atomic_store(&s_task_running, false);
    atomic_store(&s_task_stopping, false);
    atomic_store(&s_task_exiting, false);
    atomic_store(&s_startup_complete, false);
    atomic_store(&s_recovery_pending, false);
    atomic_store(&s_voice_recovery_pending, false);
    atomic_store(&s_terminal_pending, false);
    atomic_store(&s_storage_readers, 0U);
    atomic_store(&s_playback_elapsed_ms, 0);
    atomic_store(&s_stop_latency_ms, 0);
    atomic_store(&s_stack_high_water_words, 0);
    atomic_store(&s_voice_rx_timeout_count, 0);
    atomic_store(&s_max_processing_us, 0);
    return ESP_OK;
}

esp_err_t audio_service_prepare_run(void)
{
    if (s_audio_queue == NULL || s_ui_feedback_queue == NULL ||
        s_voice_event_queue == NULL || s_voice_pcm_stream == NULL ||
        s_voice_capture_frame == NULL ||
        atomic_load(&s_task_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)xQueueReset(s_audio_queue);
    (void)xQueueReset(s_ui_feedback_queue);
    (void)xQueueReset(s_voice_event_queue);
    (void)xStreamBufferReset(s_voice_pcm_stream);
    atomic_store(&s_cancel_pending, false);
    atomic_store(&s_queued_play_generation, 0U);
    atomic_store(&s_active_play_generation, 0U);
    atomic_store(&s_last_claimed_play_generation, 0U);
    atomic_store(&s_cancel_target_generation, 0U);
    atomic_store(&s_cancel_ack_generation, 0U);
    atomic_store(&s_voice_pending, false);
    atomic_store(&s_queued_voice_generation, 0U);
    atomic_store(&s_active_voice_generation, 0U);
    atomic_store(&s_voice_stop_generation, 0U);
    atomic_store(&s_voice_pcm_generation, 0U);
    atomic_store(&s_voice_capture_active, false);
    atomic_store(&s_task_stopping, false);
    atomic_store(&s_task_exiting, false);
    atomic_store(&s_startup_complete, false);
    atomic_store(&s_voice_recovery_pending, false);
    atomic_store(&s_task_running, true);
    return ESP_OK;
}

void audio_service_cancel_prepared_run(void)
{
    atomic_store(&s_task_stopping, true);
    atomic_store(&s_task_running, false);
    atomic_store(&s_startup_complete, false);
}

bool audio_service_startup_complete(void)
{
    return atomic_load(&s_startup_complete);
}

esp_err_t audio_service_deinit_contracts(void)
{
    if (s_audio_queue == NULL && s_ui_feedback_queue == NULL &&
        s_voice_event_queue == NULL && s_voice_pcm_stream == NULL &&
        s_voice_pcm_storage == NULL && s_voice_capture_frame == NULL)
    {
        return ESP_OK;
    }
    if (atomic_load(&s_task_running) || atomic_load(&s_play_active) ||
        atomic_load(&s_voice_capture_active))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_audio_queue != NULL)
    {
        vQueueDelete(s_audio_queue);
        s_audio_queue = NULL;
    }
    if (s_ui_feedback_queue != NULL)
    {
        vQueueDelete(s_ui_feedback_queue);
        s_ui_feedback_queue = NULL;
    }
    if (s_voice_event_queue != NULL)
    {
        vQueueDelete(s_voice_event_queue);
        s_voice_event_queue = NULL;
    }
    if (s_voice_pcm_stream != NULL)
    {
        vStreamBufferDelete(s_voice_pcm_stream);
        s_voice_pcm_stream = NULL;
    }
    if (s_voice_pcm_storage != NULL)
    {
        s_voice_capture_frame = NULL;
        heap_caps_free(s_voice_pcm_storage);
        s_voice_pcm_storage = NULL;
    }
    atomic_store(&s_play_pending, false);
    atomic_store(&s_cancel_pending, false);
    atomic_store(&s_queued_play_generation, 0U);
    atomic_store(&s_active_play_generation, 0U);
    atomic_store(&s_last_claimed_play_generation, 0U);
    atomic_store(&s_cancel_target_generation, 0U);
    atomic_store(&s_cancel_ack_generation, 0U);
    atomic_store(&s_voice_pending, false);
    atomic_store(&s_queued_voice_generation, 0U);
    atomic_store(&s_active_voice_generation, 0U);
    atomic_store(&s_voice_stop_generation, 0U);
    atomic_store(&s_voice_pcm_generation, 0U);
    atomic_store(&s_voice_capture_active, false);
    return ESP_OK;
}

audio_service_result_t audio_service_request_play(audio_resource_id_t resource_id,
                                                  audio_service_priority_t priority,
                                                  TickType_t timeout_ticks)
{
    if (s_audio_queue == NULL || resource_id < 0 || resource_id >= AUDIO_RESOURCE_COUNT ||
        (priority != AUDIO_PRIORITY_DEFAULT && priority != AUDIO_PRIORITY_SELFTEST) ||
        !atomic_load(&s_task_running) || !audio_service_storage_ready() ||
        atomic_load(&s_task_stopping) ||
        atomic_load(&s_task_exiting) ||
        atomic_load(&s_recovery_pending))
    {
        return AUDIO_RESULT_NOT_READY;
    }
    if (!current_output_allowed(priority))
    {
        ESP_LOGI(TAG, "严重低电静默抑制非自检音频播放");
        return AUDIO_RESULT_OK;
    }
    bool expected = false;
    if (atomic_load(&s_resource_play_active) ||
        atomic_load(&s_voice_pending) ||
        atomic_load(&s_active_voice_generation) != 0U ||
        !atomic_compare_exchange_strong(&s_play_pending, &expected, true))
    {
        ESP_LOGW(TAG, "已有音频播放或排队请求，拒绝冲突播放，稳定错误码=AUDIO_BUSY");
        return AUDIO_RESULT_BUSY;
    }
    if (atomic_load(&s_storage_transitioning) || !audio_service_storage_ready())
    {
        atomic_store(&s_play_pending, false);
        return AUDIO_RESULT_NOT_READY;
    }

    const uint32_t generation = next_play_generation();
    atomic_store(&s_queued_play_generation, generation);
    const audio_service_request_t request = {
        .type = AUDIO_SERVICE_REQUEST_PLAY,
        .payload.play = {
            .resource_id = resource_id,
            .priority = priority,
            .accepted_at_us = esp_timer_get_time(),
            .generation = generation,
        },
    };
    if (xQueueSend(s_audio_queue, &request, timeout_ticks) != pdTRUE)
    {
        atomic_store(&s_queued_play_generation, 0U);
        atomic_store(&s_play_pending, false);
        ESP_LOGW(TAG, "音频命令队列已满，稳定错误码=AUDIO_BUSY");
        return AUDIO_RESULT_BUSY;
    }
    wake_audio_task();
    if (!atomic_load(&s_task_running) ||
        atomic_load(&s_task_stopping) ||
        atomic_load(&s_task_exiting))
    {
        unsigned int expected_generation = generation;
        const bool rejected_before_owner =
            atomic_compare_exchange_strong(
                &s_queued_play_generation,
                &expected_generation,
                0U);
        if (!rejected_before_owner &&
            (atomic_load(&s_active_play_generation) == generation ||
             atomic_load(&s_last_claimed_play_generation) == generation))
        {
            return AUDIO_RESULT_OK;
        }
        if (!rejected_before_owner &&
            atomic_load(&s_task_running) &&
            !atomic_load(&s_task_exiting))
        {
            return AUDIO_RESULT_OK;
        }
        if (rejected_before_owner)
        {
            atomic_store(&s_play_pending, false);
        }
        ESP_LOGW(TAG,
                 "音频播放入队时服务已停止，已回滚接受状态，稳定错误码=AUDIO_NOT_READY");
        return AUDIO_RESULT_NOT_READY;
    }
    return AUDIO_RESULT_OK;
}

audio_service_result_t audio_service_request_ui_feedback(
    bool haptics_enabled,
    bool click_audio_enabled)
{
    atomic_store(&s_ui_haptics_enabled, haptics_enabled);
    atomic_store(&s_ui_click_audio_enabled, click_audio_enabled);
    if (!haptics_enabled && !click_audio_enabled)
    {
        return AUDIO_RESULT_OK;
    }
    if (!current_ui_feedback_allowed())
    {
        ESP_LOGI(TAG, "严重低电静默抑制 UI 点击音与触觉反馈");
        return AUDIO_RESULT_OK;
    }
    if (s_ui_feedback_queue == NULL || !atomic_load(&s_task_running) ||
        atomic_load(&s_task_stopping) || atomic_load(&s_voice_pending) ||
        atomic_load(&s_active_voice_generation) != 0U)
    {
        return AUDIO_RESULT_NOT_READY;
    }

    const audio_ui_feedback_request_t request = {
        .haptics_enabled = haptics_enabled,
        .click_audio_enabled = click_audio_enabled,
    };
    if (xQueueSend(s_ui_feedback_queue, &request, 0) != pdTRUE)
    {
        ESP_LOGW(TAG, "UI 反馈队列已满，本次反馈安全丢弃，稳定错误码=AUDIO_BUSY");
        return AUDIO_RESULT_BUSY;
    }
    wake_audio_task();
    return AUDIO_RESULT_OK;
}

esp_err_t audio_service_request_cancel_playback(TickType_t timeout_ticks)
{
    if (s_audio_queue == NULL || !atomic_load(&s_task_running) ||
        atomic_load(&s_task_stopping))
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t target_generation =
        atomic_load(&s_active_play_generation);
    if (target_generation == 0U)
    {
        target_generation =
            atomic_load(&s_queued_play_generation);
    }
    if (target_generation == 0U)
    {
        return ESP_OK;
    }
    const TickType_t started_at = xTaskGetTickCount();
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_cancel_pending, &expected, true))
    {
        if (atomic_load(&s_cancel_target_generation) != target_generation)
        {
            return ESP_ERR_TIMEOUT;
        }
    }
    else
    {
        atomic_store(&s_cancel_ack_generation, 0U);
        atomic_store(&s_cancel_target_generation, target_generation);
        const audio_service_request_t request = {
            .type = AUDIO_SERVICE_REQUEST_CANCEL_PLAYBACK,
            .payload.cancel = {
                .requested_at_us = esp_timer_get_time(),
                .target_generation = target_generation,
            },
        };
        if (xQueueSendToFront(s_audio_queue, &request, timeout_ticks) != pdTRUE)
        {
            atomic_store(&s_cancel_target_generation, 0U);
            atomic_store(&s_cancel_pending, false);
            return ESP_ERR_TIMEOUT;
        }
        wake_audio_task();
    }
    while (atomic_load(&s_cancel_ack_generation) != target_generation)
    {
        if (timeout_ticks == 0U ||
            xTaskGetTickCount() - started_at >= timeout_ticks)
        {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(1U);
    }
    return ESP_OK;
}

esp_err_t audio_service_request_voice_capture_start(uint32_t generation)
{
    if (generation == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!audio_service_voice_capture_ready() || s_audio_queue == NULL ||
        s_ui_feedback_queue == NULL || s_voice_pcm_stream == NULL ||
        atomic_load(&s_task_stopping) || atomic_load(&s_task_exiting) ||
        !current_output_allowed(AUDIO_PRIORITY_DEFAULT))
    {
        return ESP_ERR_INVALID_STATE;
    }

    bool expected = false;
    if (atomic_load(&s_play_pending) || atomic_load(&s_play_active) ||
        atomic_load(&s_resource_play_active) ||
        atomic_load(&s_active_voice_generation) != 0U ||
        uxQueueMessagesWaiting(s_ui_feedback_queue) != 0U ||
        !atomic_compare_exchange_strong(&s_voice_pending, &expected, true))
    {
        return ESP_ERR_TIMEOUT;
    }

    atomic_store(&s_queued_voice_generation, generation);
    atomic_store(&s_voice_stop_generation, 0U);
    const audio_service_request_t request = {
        .type = AUDIO_SERVICE_REQUEST_VOICE_CAPTURE_START,
        .payload.voice.generation = generation,
    };
    if (xQueueSendToFront(s_audio_queue, &request, 0) != pdTRUE)
    {
        atomic_store(&s_queued_voice_generation, 0U);
        atomic_store(&s_voice_pending, false);
        return ESP_ERR_TIMEOUT;
    }
    wake_audio_task();
    return ESP_OK;
}

esp_err_t audio_service_request_voice_capture_stop(uint32_t generation)
{
    if (generation == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!atomic_load(&s_task_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t queued = atomic_load(&s_queued_voice_generation);
    const uint32_t active = atomic_load(&s_active_voice_generation);
    if (queued != generation && active != generation)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_voice_stop_generation, generation);
    wake_audio_task();
    return ESP_OK;
}

esp_err_t audio_service_receive_voice_event(
    audio_service_voice_event_t *event,
    TickType_t timeout_ticks)
{
    if (event == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_voice_event_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueReceive(s_voice_event_queue, event, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t audio_service_receive_voice_pcm(
    uint32_t generation,
    void *buffer,
    size_t buffer_size,
    size_t *bytes_received,
    TickType_t timeout_ticks)
{
    if (generation == 0U || buffer == NULL || buffer_size == 0U ||
        bytes_received == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *bytes_received = 0U;
    if (s_voice_pcm_stream == NULL ||
        atomic_load(&s_voice_pcm_generation) != generation)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *bytes_received = xStreamBufferReceive(s_voice_pcm_stream,
                                           buffer,
                                           buffer_size,
                                           timeout_ticks);
    if (*bytes_received > 0U)
    {
        return ESP_OK;
    }
    return atomic_load(&s_voice_pcm_generation) == generation
               ? ESP_ERR_TIMEOUT
               : ESP_ERR_INVALID_STATE;
}

bool audio_service_voice_capture_ready(void)
{
    return s_voice_event_queue != NULL && s_voice_pcm_stream != NULL &&
           atomic_load(&s_task_running) &&
           atomic_load(&s_startup_complete) &&
           !atomic_load(&s_task_stopping) &&
           !atomic_load(&s_task_exiting) &&
           !atomic_load(&s_recovery_pending) &&
           audio_service_storage_ready() && es8311_bsp_is_ready();
}

esp_err_t audio_service_request_stop(TickType_t timeout_ticks)
{
    if (s_audio_queue == NULL || !atomic_load(&s_task_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_task_stopping, &expected, true))
    {
        return ESP_OK;
    }
    const audio_service_request_t request = {
        .type = AUDIO_SERVICE_REQUEST_STOP,
        .payload.stop.requested_at_us = esp_timer_get_time(),
    };
    if (xQueueSendToFront(s_audio_queue, &request, timeout_ticks) == pdTRUE)
    {
        wake_audio_task();
        return ESP_OK;
    }
    atomic_store(&s_task_stopping, false);
    return ESP_ERR_TIMEOUT;
}

esp_err_t audio_service_run(void)
{
    if (s_audio_queue == NULL || s_ui_feedback_queue == NULL ||
        s_voice_event_queue == NULL || s_voice_pcm_stream == NULL ||
        s_voice_capture_frame == NULL)
    {
        ESP_LOGE(TAG, "audio_task 控制或 UI 反馈队列未初始化，无法启动");
        return ESP_ERR_INVALID_STATE;
    }
    if (!atomic_load(&s_task_running))
    {
        bool expected = false;
        if (!atomic_compare_exchange_strong(&s_task_running, &expected, true))
        {
            return ESP_ERR_INVALID_STATE;
        }
    }
    portENTER_CRITICAL(&s_audio_task_lock);
    s_audio_task = xTaskGetCurrentTaskHandle();
    portEXIT_CRITICAL(&s_audio_task_lock);

    esp_err_t hardware_error = probe_hardware_ready();
    atomic_store(&s_startup_complete, true);
    if (hardware_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "音频硬件无声就绪检查失败，稳定错误码=AUDIO_HW_FAILED，错误=0x%x",
                 (unsigned)hardware_error);
    }

    bool shutdown_requested = false;
    while (!shutdown_requested)
    {
        if (atomic_load(&s_recovery_pending))
        {
            const audio_service_result_t recovery_result =
                atomic_load(&s_voice_recovery_pending)
                    ? finish_voice_capture(AUDIO_RESULT_OK)
                    : finish_playback(-1, AUDIO_RESULT_OK);
            if (recovery_result != AUDIO_RESULT_OK)
            {
                (void)publish_audio_state(WATCH_AUDIO_STATE_FAILED,
                                          map_watch_error(recovery_result),
                                          true,
                                          0);
                if (take_stop_request(&shutdown_requested))
                {
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS));
                continue;
            }
        }
        (void)flush_pending_terminal();
        audio_service_request_t request = {0};
        if (xQueueReceive(s_audio_queue, &request, 0) != pdTRUE)
        {
            audio_ui_feedback_request_t feedback = {0};
            if (xQueueReceive(
                    s_ui_feedback_queue,
                    &feedback,
                    0U) != pdTRUE)
            {
                const TickType_t wait_ticks =
                    atomic_load(&s_terminal_pending) ||
                            atomic_load(&s_recovery_pending)
                        ? pdMS_TO_TICKS(AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS)
                        : portMAX_DELAY;
                (void)ulTaskNotifyTake(pdTRUE, wait_ticks);
                continue;
            }
            if (!current_ui_feedback_allowed())
            {
                ESP_LOGI(TAG, "消费前电量等级变化，静默丢弃 UI 音振反馈");
                continue;
            }
            atomic_store(&s_play_active, true);
            play_ui_feedback(&feedback, &shutdown_requested);
            atomic_store(&s_play_active, false);
            atomic_store(&s_stack_high_water_words,
                         uxTaskGetStackHighWaterMark(NULL));
            continue;
        }
        if (request.type == AUDIO_SERVICE_REQUEST_CANCEL_PLAYBACK)
        {
            handle_pending_playback_cancel(&request, &shutdown_requested);
            continue;
        }
        if (request.type == AUDIO_SERVICE_REQUEST_STOP)
        {
            apply_stop_request(&request, &shutdown_requested);
            break;
        }
        if (request.type == AUDIO_SERVICE_REQUEST_VOICE_CAPTURE_START)
        {
            const uint32_t generation = request.payload.voice.generation;
            unsigned int expected_generation = generation;
            if (!atomic_compare_exchange_strong(
                    &s_queued_voice_generation,
                    &expected_generation,
                    0U))
            {
                ESP_LOGI(TAG,
                         "丢弃已失效的语音采集代次=%lu",
                         (unsigned long)generation);
                continue;
            }
            atomic_store(&s_voice_pending, false);
            atomic_store(&s_active_voice_generation, generation);
            atomic_store(&s_voice_pcm_generation, generation);
            atomic_store(&s_voice_rx_timeout_count, 0U);
            (void)xStreamBufferReset(s_voice_pcm_stream);
            if (!current_output_allowed(AUDIO_PRIORITY_DEFAULT))
            {
                ESP_LOGI(TAG,
                         "语音启动前电量门禁失效，代次=%lu，本次不播放提示音且不采集",
                         (unsigned long)generation);
                publish_voice_event(AUDIO_VOICE_EVENT_FAILED,
                                    generation,
                                    AUDIO_RESULT_NOT_READY);
            }
            else
            {
                run_voice_capture_session(generation, &shutdown_requested);
            }
            atomic_store(&s_active_voice_generation, 0U);
            atomic_store(&s_voice_stop_generation, 0U);
            atomic_store(&s_voice_capture_active, false);
            atomic_store(&s_stack_high_water_words,
                         uxTaskGetStackHighWaterMark(NULL));
            continue;
        }
        if (request.type != AUDIO_SERVICE_REQUEST_PLAY)
        {
            continue;
        }

        atomic_store(&s_active_play_generation,
                     request.payload.play.generation);
        unsigned int expected_queued_generation =
            request.payload.play.generation;
        if (!atomic_compare_exchange_strong(
                &s_queued_play_generation,
                &expected_queued_generation,
                0U))
        {
            unsigned int expected_active_generation =
                request.payload.play.generation;
            (void)atomic_compare_exchange_strong(
                &s_active_play_generation,
                &expected_active_generation,
                0U);
            ESP_LOGI(TAG,
                     "丢弃已由调用方回滚的音频播放代次=%lu",
                     (unsigned long)request.payload.play.generation);
            continue;
        }
        atomic_store(&s_last_claimed_play_generation,
                     request.payload.play.generation);
        if (!current_output_allowed(request.payload.play.priority))
        {
            atomic_store(&s_play_pending, false);
            atomic_store(&s_active_play_generation, 0U);
            (void)publish_audio_state(WATCH_AUDIO_STATE_STOPPED,
                                      WATCH_AUDIO_ERROR_NONE,
                                      true,
                                      request.payload.play.accepted_at_us);
            ESP_LOGI(TAG,
                     "消费前电量等级变化，静默丢弃非自检音频播放");
            continue;
        }
        atomic_store(&s_play_active, true);
        atomic_store(&s_resource_play_active, true);
        atomic_store(&s_play_pending, false);
        (void)publish_audio_state(WATCH_AUDIO_STATE_PLAYING,
                                  WATCH_AUDIO_ERROR_NONE,
                                  false,
                                  0);
        bool playback_cancelled = false;
        audio_service_result_t result = play_resource(request.payload.play.resource_id,
                                                      &shutdown_requested,
                                                      &playback_cancelled,
                                                      0U,
                                                      NULL);
        int64_t terminal_started_at_us = esp_timer_get_time();
        atomic_store(&s_resource_play_active, false);
        atomic_store(&s_play_active, false);
        atomic_store(&s_active_play_generation, 0U);
        if ((shutdown_requested || playback_cancelled) &&
            result == AUDIO_RESULT_OK)
        {
            (void)publish_audio_state(WATCH_AUDIO_STATE_STOPPED,
                                      WATCH_AUDIO_ERROR_NONE,
                                      true,
                                      request.payload.play.accepted_at_us);
        }
        else if (result == AUDIO_RESULT_OK)
        {
            (void)publish_audio_state(WATCH_AUDIO_STATE_COMPLETED,
                                      WATCH_AUDIO_ERROR_NONE,
                                      true,
                                      request.payload.play.accepted_at_us);
        }
        else
        {
            (void)publish_audio_state(WATCH_AUDIO_STATE_FAILED,
                                      map_watch_error(result),
                                      true,
                                      request.payload.play.accepted_at_us);
        }
        if (playback_cancelled)
        {
            acknowledge_playback_cancel(request.payload.play.generation);
        }
        update_processing_metric(terminal_started_at_us);
        atomic_store(&s_stack_high_water_words, uxTaskGetStackHighWaterMark(NULL));
        ESP_LOGI(TAG,
                 "音频播放结束，稳定错误码=%s，play-to-terminal=%u ms，连续处理最大=%u us，stop=%u ms，栈高水位=%u word",
                 audio_service_result_code(result),
                 (unsigned)atomic_load(&s_playback_elapsed_ms),
                 (unsigned)atomic_load(&s_max_processing_us),
                 (unsigned)atomic_load(&s_stop_latency_ms),
                 (unsigned)atomic_load(&s_stack_high_water_words));
    }

    atomic_store(&s_task_exiting, true);
    int64_t cleanup_started_at_us = esp_timer_get_time();
    audio_service_result_t cleanup_result =
        shutdown_audio_hardware(AUDIO_RESULT_OK);
    update_processing_metric(cleanup_started_at_us);
    if (cleanup_result != AUDIO_RESULT_OK)
    {
        (void)publish_audio_state(WATCH_AUDIO_STATE_FAILED,
                                  map_watch_error(cleanup_result),
                                  true,
                                  0);
    }
    for (uint32_t attempt = 0;
         attempt < AUDIO_SERVICE_CRITICAL_RETRY_COUNT && atomic_load(&s_terminal_pending);
         ++attempt)
    {
        (void)flush_pending_terminal();
    }
    const uint32_t stopped_cancel_generation =
        atomic_load(&s_cancel_target_generation);
    if (stopped_cancel_generation != 0U)
    {
        acknowledge_playback_cancel(stopped_cancel_generation);
    }
    atomic_store(&s_play_pending, false);
    atomic_store(&s_cancel_pending, false);
    atomic_store(&s_queued_play_generation, 0U);
    atomic_store(&s_active_play_generation, 0U);
    atomic_store(&s_cancel_target_generation, 0U);
    atomic_store(&s_play_active, false);
    atomic_store(&s_resource_play_active, false);
    atomic_store(&s_voice_pending, false);
    atomic_store(&s_queued_voice_generation, 0U);
    atomic_store(&s_active_voice_generation, 0U);
    atomic_store(&s_voice_stop_generation, 0U);
    atomic_store(&s_voice_capture_active, false);
    (void)xQueueReset(s_audio_queue);
    (void)xQueueReset(s_ui_feedback_queue);
    portENTER_CRITICAL(&s_audio_task_lock);
    s_audio_task = NULL;
    portEXIT_CRITICAL(&s_audio_task_lock);
    atomic_store(&s_task_running, false);
    if (cleanup_result != AUDIO_RESULT_OK)
    {
        return result_to_esp_error(cleanup_result);
    }
    return atomic_load(&s_terminal_pending) ? ESP_ERR_TIMEOUT : ESP_OK;
}

static void wake_audio_task(void)
{
    portENTER_CRITICAL(&s_audio_task_lock);
    if (s_audio_task != NULL)
    {
        xTaskNotifyGive(s_audio_task);
    }
    portEXIT_CRITICAL(&s_audio_task_lock);
}

esp_err_t audio_service_metrics_snapshot(audio_service_metrics_t *metrics)
{
    if (metrics == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    metrics->storage_verdict_ms = (uint32_t)atomic_load(&s_storage_verdict_ms);
    metrics->playback_elapsed_ms = (uint32_t)atomic_load(&s_playback_elapsed_ms);
    metrics->max_processing_us = (uint32_t)atomic_load(&s_max_processing_us);
    metrics->stop_latency_ms = (uint32_t)atomic_load(&s_stop_latency_ms);
    metrics->stack_high_water_words =
        (UBaseType_t)atomic_load(&s_stack_high_water_words);
    metrics->voice_rx_timeout_count =
        (uint32_t)atomic_load(&s_voice_rx_timeout_count);
    return ESP_OK;
}

esp_err_t audio_service_publish_initial_state(void)
{
    const audio_service_result_t storage_result =
        (audio_service_result_t)atomic_load(&s_storage_result);
    const bool ready = audio_service_storage_ready();
    const watch_audio_update_t update = {
        .resource_available = ready,
        .state = ready ? WATCH_AUDIO_STATE_READY : WATCH_AUDIO_STATE_FAILED,
        .error = map_watch_error(storage_result),
        .updated_at_ticks = xTaskGetTickCount(),
    };
    return state_service_publish_audio(&update, pdMS_TO_TICKS(20));
}

static uint16_t read_le16(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8U);
}

static uint32_t read_le32(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8U) |
           ((uint32_t)bytes[2] << 16U) |
           ((uint32_t)bytes[3] << 24U);
}

static audio_service_result_t read_exact(int file_descriptor,
                                         void *buffer,
                                         size_t size)
{
    uint8_t *cursor = buffer;
    size_t remaining = size;
    while (remaining > 0U)
    {
        const ssize_t read_size = read(file_descriptor, cursor, remaining);
        if (read_size > 0)
        {
            cursor += (size_t)read_size;
            remaining -= (size_t)read_size;
            continue;
        }
        if (read_size < 0 && errno == EINTR)
        {
            continue;
        }
        return read_size == 0 ? AUDIO_RESULT_CORRUPT : AUDIO_RESULT_IO_FAILED;
    }
    return AUDIO_RESULT_OK;
}

static audio_service_result_t seek_absolute(int file_descriptor,
                                            uint64_t offset,
                                            uint64_t file_size)
{
    if (offset > file_size || offset > LONG_MAX)
    {
        return AUDIO_RESULT_CORRUPT;
    }
    return lseek(file_descriptor, (off_t)offset, SEEK_SET) == (off_t)offset
               ? AUDIO_RESULT_OK
               : AUDIO_RESULT_IO_FAILED;
}

static audio_service_result_t probe_resource_file(audio_resource_id_t resource_id,
                                                  audio_wav_info_t *info)
{
    if (info == NULL)
    {
        return AUDIO_RESULT_IO_FAILED;
    }
    const char *path = audio_service_resource_path(resource_id);
    if (path == NULL)
    {
        return AUDIO_RESULT_RESOURCE_MISSING;
    }

    errno = 0;
    const int file_descriptor = open(path, O_RDONLY);
    if (file_descriptor < 0)
    {
        return errno == ENOENT ? AUDIO_RESULT_RESOURCE_MISSING : AUDIO_RESULT_IO_FAILED;
    }

    int64_t processing_started_at_us = esp_timer_get_time();
    audio_service_result_t result = parse_wav(file_descriptor, info);
    update_processing_metric(processing_started_at_us);
    if (close(file_descriptor) != 0 && result == AUDIO_RESULT_OK)
    {
        result = AUDIO_RESULT_IO_FAILED;
    }
    return result;
}

static audio_service_result_t parse_wav(int file_descriptor,
                                        audio_wav_info_t *info)
{
    int64_t processing_started_at_us = esp_timer_get_time();
    const off_t file_end = lseek(file_descriptor, 0, SEEK_END);
    if (file_end < 0)
    {
        return AUDIO_RESULT_IO_FAILED;
    }
    if ((uint64_t)file_end < AUDIO_WAV_RIFF_PREFIX_SIZE)
    {
        return AUDIO_RESULT_CORRUPT;
    }
    if (lseek(file_descriptor, 0, SEEK_SET) != 0)
    {
        return AUDIO_RESULT_IO_FAILED;
    }
    const uint64_t file_size = (uint64_t)file_end;

    uint8_t riff_header[AUDIO_WAV_RIFF_PREFIX_SIZE];
    audio_service_result_t result =
        read_exact(file_descriptor, riff_header, sizeof(riff_header));
    if (result != AUDIO_RESULT_OK)
    {
        return result;
    }
    if (memcmp(riff_header, "RIFF", 4) != 0 || memcmp(&riff_header[8], "WAVE", 4) != 0)
    {
        return AUDIO_RESULT_CORRUPT;
    }

    const uint64_t riff_end = (uint64_t)read_le32(&riff_header[4]) + 8U;
    if (riff_end < AUDIO_WAV_RIFF_PREFIX_SIZE || riff_end > file_size)
    {
        return AUDIO_RESULT_CORRUPT;
    }

    bool fmt_found = false;
    bool data_found = false;
    audio_wav_info_t parsed = {0};
    uint64_t cursor = AUDIO_WAV_RIFF_PREFIX_SIZE;
    uint32_t chunk_count = 0;
    while (cursor < riff_end)
    {
        if (++chunk_count > AUDIO_WAV_MAX_CHUNKS)
        {
            return AUDIO_RESULT_CORRUPT;
        }
        if (riff_end - cursor < AUDIO_WAV_CHUNK_HEADER_SIZE)
        {
            return AUDIO_RESULT_CORRUPT;
        }

        uint8_t chunk_header[AUDIO_WAV_CHUNK_HEADER_SIZE];
        result = read_exact(file_descriptor, chunk_header, sizeof(chunk_header));
        if (result != AUDIO_RESULT_OK)
        {
            return result;
        }
        cursor += AUDIO_WAV_CHUNK_HEADER_SIZE;

        const uint32_t chunk_size = read_le32(&chunk_header[4]);
        const uint64_t padded_size = (uint64_t)chunk_size + (chunk_size & 1U);
        if (padded_size > riff_end - cursor)
        {
            return AUDIO_RESULT_CORRUPT;
        }

        if (memcmp(chunk_header, "fmt ", 4) == 0)
        {
            if (fmt_found || chunk_size < AUDIO_WAV_FMT_MIN_SIZE)
            {
                return AUDIO_RESULT_CORRUPT;
            }
            uint8_t fmt[AUDIO_WAV_FMT_MIN_SIZE];
            result = read_exact(file_descriptor, fmt, sizeof(fmt));
            if (result != AUDIO_RESULT_OK)
            {
                return result;
            }
            parsed.audio_format = read_le16(&fmt[0]);
            parsed.channels = read_le16(&fmt[2]);
            parsed.sample_rate_hz = read_le32(&fmt[4]);
            parsed.byte_rate = read_le32(&fmt[8]);
            parsed.block_align = read_le16(&fmt[12]);
            parsed.bits_per_sample = read_le16(&fmt[14]);
            fmt_found = true;
        }
        else if (memcmp(chunk_header, "data", 4) == 0)
        {
            if (data_found || chunk_size == 0U || cursor > UINT32_MAX)
            {
                return AUDIO_RESULT_CORRUPT;
            }
            parsed.data_offset = (uint32_t)cursor;
            parsed.data_size = chunk_size;
            data_found = true;
        }

        cursor += padded_size;
        result = seek_absolute(file_descriptor, cursor, file_size);
        if (result != AUDIO_RESULT_OK)
        {
            return result;
        }
        update_processing_metric(processing_started_at_us);
        processing_started_at_us = esp_timer_get_time();
        taskYIELD();
    }

    if (!fmt_found || !data_found)
    {
        return AUDIO_RESULT_CORRUPT;
    }
    if (parsed.audio_format != AUDIO_WAV_PCM_FORMAT ||
        parsed.channels != AUDIO_WAV_CHANNELS ||
        parsed.sample_rate_hz != AUDIO_WAV_SAMPLE_RATE_HZ ||
        parsed.bits_per_sample != AUDIO_WAV_BITS_PER_SAMPLE ||
        parsed.block_align != AUDIO_WAV_BLOCK_ALIGN ||
        parsed.byte_rate != AUDIO_WAV_BYTE_RATE)
    {
        return AUDIO_RESULT_UNSUPPORTED;
    }
    if (parsed.data_size % parsed.block_align != 0U)
    {
        return AUDIO_RESULT_CORRUPT;
    }

    update_processing_metric(processing_started_at_us);
    *info = parsed;
    return AUDIO_RESULT_OK;
}

static esp_err_t result_to_esp_error(audio_service_result_t result)
{
    switch (result)
    {
    case AUDIO_RESULT_OK:
        return ESP_OK;
    case AUDIO_RESULT_RESOURCE_MISSING:
        return ESP_ERR_NOT_FOUND;
    case AUDIO_RESULT_NOT_READY:
        return ESP_ERR_INVALID_STATE;
    default:
        return ESP_FAIL;
    }
}

static void complete_storage_transition(int64_t started_at_us,
                                        audio_service_result_t result)
{
    int64_t elapsed_us = esp_timer_get_time() - started_at_us;
    const uint32_t elapsed_ms = elapsed_us > 0 ? (uint32_t)(elapsed_us / 1000) : 0;
    atomic_store(&s_storage_verdict_ms, elapsed_ms);
    atomic_store(&s_storage_result, result);
    atomic_store(&s_storage_transitioning, false);
    if (elapsed_ms > AUDIO_SERVICE_STORAGE_ACCEPTANCE_MS)
    {
        ESP_LOGW(TAG,
                 "storage/assets 判定超过 3 秒代码口径，耗时=%u ms",
                 (unsigned)elapsed_ms);
    }
}

static audio_service_result_t play_resource(audio_resource_id_t resource_id,
                                            bool *shutdown_requested,
                                            bool *playback_cancelled,
                                            uint32_t voice_generation,
                                            bool *voice_cancelled)
{
    if (shutdown_requested == NULL || !audio_service_storage_ready())
    {
        return AUDIO_RESULT_NOT_READY;
    }
    if (playback_cancelled != NULL)
    {
        *playback_cancelled = false;
    }
    if (voice_cancelled != NULL)
    {
        *voice_cancelled = false;
    }
    esp_err_t pa_error = safe_off_with_retry();
    if (pa_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "播放前功放无法进入安全关断，稳定错误码=%s，错误=0x%x",
                 ns4150_bsp_error_code(pa_error),
                 (unsigned)pa_error);
        return AUDIO_RESULT_PA_FAILED;
    }

    audio_wav_info_t info = {0};
    audio_service_result_t result = audio_service_probe_resource(resource_id, &info);
    if (result != AUDIO_RESULT_OK)
    {
        return result;
    }

    const char *path = audio_service_resource_path(resource_id);
    const int file_descriptor = open(path, O_RDONLY);
    if (file_descriptor < 0)
    {
        return errno == ENOENT ? AUDIO_RESULT_RESOURCE_MISSING : AUDIO_RESULT_IO_FAILED;
    }
    if (lseek(file_descriptor, (off_t)info.data_offset, SEEK_SET) !=
        (off_t)info.data_offset)
    {
        return finish_playback(file_descriptor, AUDIO_RESULT_IO_FAILED);
    }

    legbot_bsp_i2c_access_t access = {0};
    if (i2c_manager_get_access(&access) != ESP_OK)
    {
        return finish_playback(file_descriptor, AUDIO_RESULT_CODEC_FAILED);
    }
    esp_err_t err = es8311_bsp_init(&access);
    if (err != ESP_OK)
    {
        audio_service_result_t hardware_result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "ES8311 初始化失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)err);
        return finish_playback(file_descriptor, hardware_result);
    }
    err = es8311_bsp_start();
    if (err != ESP_OK)
    {
        audio_service_result_t hardware_result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "ES8311/I2S 启动失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)err);
        return finish_playback(file_descriptor, hardware_result);
    }
    err = ns4150_bsp_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "NS4150 初始化失败，稳定错误码=%s，错误=0x%x",
                 ns4150_bsp_error_code(err),
                 (unsigned)err);
        return finish_playback(file_descriptor, AUDIO_RESULT_PA_FAILED);
    }

    uint8_t pcm[AUDIO_SERVICE_PCM_CHUNK_BYTES];
    uint32_t remaining = info.data_size;
    bool pa_enabled = false;
    while (remaining > 0)
    {
        if (take_output_end_request(shutdown_requested,
                                    playback_cancelled,
                                    voice_generation,
                                    voice_cancelled))
        {
            result = AUDIO_RESULT_OK;
            break;
        }

        const size_t wanted = remaining < sizeof(pcm) ? remaining : sizeof(pcm);
        result = read_exact(file_descriptor, pcm, wanted);
        int64_t processing_started_at_us = esp_timer_get_time();
        if (result != AUDIO_RESULT_OK)
        {
            update_processing_metric(processing_started_at_us);
            break;
        }
        const size_t read_size = wanted;

        if (!pa_enabled)
        {
            update_processing_metric(processing_started_at_us);
            err = ns4150_bsp_enable();
            processing_started_at_us = esp_timer_get_time();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG,
                         "NS4150 开启失败，稳定错误码=%s，错误=0x%x",
                         ns4150_bsp_error_code(err),
                         (unsigned)err);
                result = AUDIO_RESULT_PA_FAILED;
                update_processing_metric(processing_started_at_us);
                break;
            }
            pa_enabled = true;
            /*
             * NS4150B 从 shutdown 切到 enable 的典型启动时间为 120 ms。
             * 必须在写入短音频前完成等待，否则 70 ms 点击资源会在功放就绪前被吞掉。
             */
            if (voice_generation == 0U)
            {
                vTaskDelay(pdMS_TO_TICKS(NS4150_BSP_STARTUP_SETTLE_MS));
            }
            else if (!wait_voice_settle(voice_generation,
                                        NS4150_BSP_STARTUP_SETTLE_MS,
                                        shutdown_requested))
            {
                if (voice_cancelled != NULL &&
                    voice_stop_requested(voice_generation))
                {
                    *voice_cancelled = true;
                }
                update_processing_metric(processing_started_at_us);
                break;
            }
            update_processing_metric(processing_started_at_us);
        }

        update_processing_metric(processing_started_at_us);
        size_t offset = 0;
        while (offset < read_size)
        {
            size_t bytes_written = 0;
            esp_err_t err = es8311_bsp_write(&pcm[offset],
                                             read_size - offset,
                                             &bytes_written,
                                             AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS);
            int64_t post_write_started_at_us = esp_timer_get_time();
            if (bytes_written > read_size - offset)
            {
                result = AUDIO_RESULT_I2S_FAILED;
                update_processing_metric(post_write_started_at_us);
                break;
            }
            offset += bytes_written;
            if (err == ESP_ERR_TIMEOUT && bytes_written > 0)
            {
                /* v5.5.4 允许超时时报告部分进度；保留进度后交还调度权再续写。 */
                if (take_output_end_request(shutdown_requested,
                                            playback_cancelled,
                                            voice_generation,
                                            voice_cancelled))
                {
                    update_processing_metric(post_write_started_at_us);
                    break;
                }
                update_processing_metric(post_write_started_at_us);
                taskYIELD();
                continue;
            }
            if (err != ESP_OK || bytes_written == 0)
            {
                result = AUDIO_RESULT_I2S_FAILED;
                update_processing_metric(post_write_started_at_us);
                break;
            }
            if (take_output_end_request(shutdown_requested,
                                        playback_cancelled,
                                        voice_generation,
                                        voice_cancelled))
            {
                update_processing_metric(post_write_started_at_us);
                break;
            }
            update_processing_metric(post_write_started_at_us);
        }
        if (result != AUDIO_RESULT_OK || *shutdown_requested ||
            (playback_cancelled != NULL && *playback_cancelled) ||
            (voice_cancelled != NULL && *voice_cancelled))
        {
            break;
        }
        remaining -= (uint32_t)read_size;
        taskYIELD();
    }

    if (result == AUDIO_RESULT_OK && remaining == 0U &&
        !*shutdown_requested &&
        (playback_cancelled == NULL || !*playback_cancelled) &&
        (voice_cancelled == NULL || !*voice_cancelled))
    {
        memset(pcm, 0, sizeof(pcm));
        size_t drain_bytes =
            AUDIO_OUTPUT_DRAIN_SAMPLE_COUNT * sizeof(int16_t);
        while (drain_bytes > 0U)
        {
            const size_t write_size =
                drain_bytes < sizeof(pcm) ? drain_bytes : sizeof(pcm);
            result = write_ui_pcm(pcm,
                                  write_size,
                                  shutdown_requested,
                                  playback_cancelled,
                                  voice_generation,
                                  voice_cancelled);
            if (result != AUDIO_RESULT_OK || *shutdown_requested ||
                (playback_cancelled != NULL && *playback_cancelled) ||
                (voice_cancelled != NULL && *voice_cancelled))
            {
                break;
            }
            drain_bytes -= write_size;
        }
        if (result == AUDIO_RESULT_OK && !*shutdown_requested &&
            (playback_cancelled == NULL || !*playback_cancelled) &&
            (voice_cancelled == NULL || !*voice_cancelled))
        {
            ESP_LOGI(TAG,
                     "固定音频已追加一整圈 DMA 静音并完成物理排空");
        }
    }

    const bool prepare_capture =
        voice_generation != 0U && result == AUDIO_RESULT_OK &&
        !*shutdown_requested &&
        (voice_cancelled == NULL || !*voice_cancelled) &&
        !voice_stop_requested(voice_generation);
    return prepare_capture
               ? finish_voice_cue_playback(file_descriptor, result)
               : finish_playback(file_descriptor, result);
}

static void play_ui_feedback(const audio_ui_feedback_request_t *request,
                             bool *shutdown_requested)
{
    if (request == NULL || shutdown_requested == NULL)
    {
        return;
    }
    const audio_ui_feedback_output_t output =
        audio_service_resolve_ui_feedback(
            request->haptics_enabled,
            request->click_audio_enabled,
            atomic_load(&s_ui_haptics_enabled),
            atomic_load(&s_ui_click_audio_enabled));
    if ((!output.play_haptics && !output.play_click_audio) ||
        take_stop_request(shutdown_requested))
    {
        return;
    }
    ESP_LOGI(TAG,
             "开始执行 UI 反馈，振动=%u，点击音=%u",
             output.play_haptics ? 1U : 0U,
             output.play_click_audio ? 1U : 0U);

    if (output.play_haptics)
    {
        const esp_err_t haptic_error = play_ui_haptic();
        if (haptic_error != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "UI 触觉反馈失败，稳定错误码=%s，错误=0x%x",
                     haptic_error == ESP_ERR_TIMEOUT
                         ? "UI_FEEDBACK_HAPTIC_BUSY"
                         : "UI_FEEDBACK_HAPTIC_FAILED",
                     (unsigned)haptic_error);
        }
    }
    if (take_stop_request(shutdown_requested) ||
        !output.play_click_audio)
    {
        return;
    }

    const audio_service_result_t click_result =
        play_resource(AUDIO_RESOURCE_UI_CLICK,
                      shutdown_requested,
                      NULL,
                      0U,
                      NULL);
    if (click_result != AUDIO_RESULT_OK)
    {
        ESP_LOGW(TAG,
                 "UI 点击音播放失败并已安全清理，稳定错误码=%s",
                 audio_service_result_code(click_result));
    }
    else
    {
        ESP_LOGI(TAG, "UI 点击音播放完成");
    }
}

static bool current_output_allowed(audio_service_priority_t priority)
{
    if (priority == AUDIO_PRIORITY_SELFTEST)
    {
        return true;
    }
    return audio_service_output_allowed(current_power_level(), priority);
}

static bool current_ui_feedback_allowed(void)
{
    return audio_service_ui_feedback_allowed(current_power_level());
}

static watch_power_level_t current_power_level(void)
{
    watch_power_snapshot_t power = {0};
    const esp_err_t err = watch_state_power_snapshot(&power, 0);
    if (err == ESP_OK &&
        power.power_level >= WATCH_POWER_LEVEL_NORMAL &&
        power.power_level < WATCH_POWER_LEVEL_COUNT)
    {
        atomic_store(&s_last_power_level, power.power_level);
        return power.power_level;
    }
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "读取电量状态快照失败，沿用最近有效事实，错误=0x%x",
                 (unsigned)err);
    }
    else
    {
        ESP_LOGW(TAG,
                 "读取到非法电量等级，沿用最近有效事实，等级=%d",
                 (int)power.power_level);
    }
    return (watch_power_level_t)atomic_load(&s_last_power_level);
}

static esp_err_t play_ui_haptic(void)
{
    esp_err_t result = vibration_bsp_lease_acquire(0);
    if (result != ESP_OK)
    {
        return result;
    }

    result = vibration_bsp_start_pulse(
        VIBRATION_BSP_UI_FEEDBACK_PULSE_MS);
    if (result == ESP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(VIBRATION_BSP_UI_FEEDBACK_PULSE_MS));
    }

    const esp_err_t stop_error = vibration_bsp_stop();
    const esp_err_t release_error = vibration_bsp_lease_release();
    if (result != ESP_OK)
    {
        return result;
    }
    if (stop_error != ESP_OK)
    {
        return stop_error;
    }
    return release_error;
}

static audio_service_result_t write_ui_pcm(const void *data,
                                           size_t size,
                                           bool *shutdown_requested,
                                           bool *playback_cancelled,
                                           uint32_t voice_generation,
                                           bool *voice_cancelled)
{
    if (data == NULL || size == 0U || shutdown_requested == NULL)
    {
        return AUDIO_RESULT_I2S_FAILED;
    }
    size_t offset = 0U;
    while (offset < size)
    {
        size_t bytes_written = 0U;
        const esp_err_t err = es8311_bsp_write((const uint8_t *)data + offset,
                                               size - offset,
                                               &bytes_written,
                                               AUDIO_SERVICE_I2S_WRITE_TIMEOUT_MS);
        if (bytes_written > size - offset)
        {
            return AUDIO_RESULT_I2S_FAILED;
        }
        offset += bytes_written;
        if (err == ESP_ERR_TIMEOUT && bytes_written > 0U)
        {
            if (take_output_end_request(shutdown_requested,
                                        playback_cancelled,
                                        voice_generation,
                                        voice_cancelled))
            {
                return AUDIO_RESULT_OK;
            }
            taskYIELD();
            continue;
        }
        if (err != ESP_OK || bytes_written == 0U)
        {
            return AUDIO_RESULT_I2S_FAILED;
        }
        if (take_output_end_request(shutdown_requested,
                                    playback_cancelled,
                                    voice_generation,
                                    voice_cancelled))
        {
            return AUDIO_RESULT_OK;
        }
    }
    return AUDIO_RESULT_OK;
}

static bool take_output_end_request(bool *shutdown_requested,
                                    bool *playback_cancelled,
                                    uint32_t voice_generation,
                                    bool *voice_cancelled)
{
    if (voice_generation != 0U &&
        voice_stop_requested(voice_generation))
    {
        if (voice_cancelled != NULL)
        {
            *voice_cancelled = true;
        }
        return true;
    }
    return playback_cancelled != NULL
               ? take_playback_end_request(shutdown_requested,
                                           playback_cancelled)
               : take_stop_request(shutdown_requested);
}

static void apply_stop_request(const audio_service_request_t *request,
                               bool *shutdown_requested)
{
    if (request == NULL || shutdown_requested == NULL ||
        request->type != AUDIO_SERVICE_REQUEST_STOP)
    {
        return;
    }
    const int64_t latency_us =
        esp_timer_get_time() - request->payload.stop.requested_at_us;
    atomic_store(&s_stop_latency_ms,
                 latency_us > 0 ? (uint32_t)(latency_us / 1000) : 0);
    atomic_store(&s_task_stopping, true);
    *shutdown_requested = true;
}

static bool take_stop_request(bool *shutdown_requested)
{
    audio_service_request_t request = {0};
    if (xQueuePeek(s_audio_queue, &request, 0) != pdTRUE ||
        request.type != AUDIO_SERVICE_REQUEST_STOP)
    {
        return false;
    }
    if (xQueueReceive(s_audio_queue, &request, 0) != pdTRUE)
    {
        return false;
    }
    apply_stop_request(&request, shutdown_requested);
    return true;
}

static bool take_playback_end_request(bool *shutdown_requested,
                                      bool *playback_cancelled)
{
    if (shutdown_requested == NULL || playback_cancelled == NULL)
    {
        return false;
    }

    audio_service_request_t request = {0};
    if (xQueueReceive(s_audio_queue, &request, 0) != pdTRUE)
    {
        return false;
    }
    if (request.type == AUDIO_SERVICE_REQUEST_STOP)
    {
        apply_stop_request(&request, shutdown_requested);
        return true;
    }
    if (request.type != AUDIO_SERVICE_REQUEST_CANCEL_PLAYBACK)
    {
        (void)xQueueSendToBack(s_audio_queue, &request, 0);
        return false;
    }

    const uint32_t target_generation =
        request.payload.cancel.target_generation;
    const bool targets_active_playback =
        target_generation != 0U &&
        target_generation == atomic_load(&s_active_play_generation);
    if (!targets_active_playback)
    {
        acknowledge_playback_cancel(target_generation);
        return false;
    }
    *playback_cancelled = true;
    return true;
}

static void handle_pending_playback_cancel(
    const audio_service_request_t *cancel_request,
    bool *shutdown_requested)
{
    if (cancel_request == NULL || shutdown_requested == NULL)
    {
        return;
    }
    const uint32_t target_generation =
        cancel_request->payload.cancel.target_generation;

    audio_service_request_t request = {0};
    const bool matching_queued_play =
        atomic_load(&s_play_pending) &&
        xQueuePeek(s_audio_queue, &request, 0) == pdTRUE &&
        request.type == AUDIO_SERVICE_REQUEST_PLAY &&
        request.payload.play.generation == target_generation;
    if (!matching_queued_play &&
        atomic_load(&s_play_pending) &&
        atomic_load(&s_queued_play_generation) == target_generation)
    {
        if (request.type == AUDIO_SERVICE_REQUEST_STOP)
        {
            /*
             * STOP 已在目标 PLAY 之前时，任务下一轮会直接退出并清空 PLAY；
             * 不把取消重新压到队首，避免服务级停止命令饥饿。
             */
            acknowledge_playback_cancel(target_generation);
            return;
        }
        /*
         * request_play 已发布代次但尚未完成队列写入时，不能提前确认取消。
         * 让出一个 tick 后把同一取消放回队首，确保目标 PLAY 不会先启动硬件。
         */
        vTaskDelay(1U);
        if (xQueueSendToFront(s_audio_queue,
                              cancel_request,
                              0) != pdTRUE)
        {
            atomic_store(&s_cancel_target_generation, 0U);
            atomic_store(&s_cancel_pending, false);
        }
        return;
    }
    if (matching_queued_play &&
        xQueueReceive(s_audio_queue, &request, 0) == pdTRUE)
    {
        if (request.type == AUDIO_SERVICE_REQUEST_STOP)
        {
            apply_stop_request(&request, shutdown_requested);
            acknowledge_playback_cancel(target_generation);
            return;
        }
        if (request.type != AUDIO_SERVICE_REQUEST_PLAY ||
            request.payload.play.generation != target_generation)
        {
            (void)xQueueSendToFront(s_audio_queue, &request, 0);
            if (xQueueSendToBack(s_audio_queue,
                                 cancel_request,
                                 0) != pdTRUE)
            {
                atomic_store(&s_cancel_target_generation, 0U);
                atomic_store(&s_cancel_pending, false);
            }
            return;
        }
        unsigned int expected_queued_generation = target_generation;
        (void)atomic_compare_exchange_strong(
            &s_queued_play_generation,
            &expected_queued_generation,
            0U);
        atomic_store(&s_play_pending, false);
        (void)publish_audio_state(WATCH_AUDIO_STATE_STOPPED,
                                  WATCH_AUDIO_ERROR_NONE,
                                  true,
                                  request.payload.play.accepted_at_us);
    }
    acknowledge_playback_cancel(target_generation);
}

static void acknowledge_playback_cancel(uint32_t generation)
{
    atomic_store(&s_cancel_ack_generation, generation);
    atomic_store(&s_cancel_target_generation, 0U);
    atomic_store(&s_cancel_pending, false);
}

static uint32_t next_play_generation(void)
{
    uint32_t generation = 0U;
    do
    {
        generation =
            atomic_fetch_add(&s_next_play_generation, 1U) + 1U;
    } while (generation == 0U);
    return generation;
}

static bool voice_stop_requested(uint32_t generation)
{
    return generation != 0U &&
           atomic_load(&s_voice_stop_generation) == generation;
}

static void publish_voice_event(audio_service_voice_event_type_t type,
                                uint32_t generation,
                                audio_service_result_t result)
{
    if (s_voice_event_queue == NULL || generation == 0U)
    {
        return;
    }
    const audio_service_voice_event_t event = {
        .type = type,
        .generation = generation,
        .result = result,
    };
    if (xQueueSend(s_voice_event_queue, &event, pdMS_TO_TICKS(20)) != pdTRUE)
    {
        ESP_LOGE(TAG,
                 "语音会话事件队列已满，代次=%lu，事件=%d，稳定错误码=AUDIO_BUSY",
                 (unsigned long)generation,
                 (int)type);
        audio_service_voice_event_t discarded = {0};
        const audio_service_voice_event_t failed = {
            .type = AUDIO_VOICE_EVENT_FAILED,
            .generation = generation,
            .result = AUDIO_RESULT_BUSY,
        };
        (void)xQueueReceive(s_voice_event_queue, &discarded, 0U);
        if (xQueueSend(s_voice_event_queue, &failed, 0U) != pdTRUE)
        {
            ESP_LOGE(TAG,
                     "语音失败终态仍无法入队，代次=%lu，稳定错误码=AUDIO_EVENT_TERMINAL_LOST",
                     (unsigned long)generation);
        }
    }
}

static void run_voice_capture_session(uint32_t generation,
                                      bool *shutdown_requested)
{
    if (generation == 0U || shutdown_requested == NULL)
    {
        return;
    }
    if (voice_stop_requested(generation))
    {
        publish_voice_event(AUDIO_VOICE_EVENT_CANCELLED,
                            generation,
                            AUDIO_RESULT_OK);
        return;
    }

    publish_voice_event(AUDIO_VOICE_EVENT_CUE_STARTED,
                        generation,
                        AUDIO_RESULT_OK);
    bool cue_cancelled = false;
    audio_service_result_t result = play_resource(
        AUDIO_RESOURCE_VOICE_START,
        shutdown_requested,
        NULL,
        generation,
        &cue_cancelled);
    if (result != AUDIO_RESULT_OK)
    {
        publish_voice_event(AUDIO_VOICE_EVENT_FAILED,
                            generation,
                            result);
        return;
    }
    if (*shutdown_requested || cue_cancelled ||
        voice_stop_requested(generation))
    {
        result = finish_playback(-1, AUDIO_RESULT_OK);
        if (result != AUDIO_RESULT_OK)
        {
            publish_voice_event(AUDIO_VOICE_EVENT_FAILED,
                                generation,
                                result);
            return;
        }
        publish_voice_event(AUDIO_VOICE_EVENT_CANCELLED,
                            generation,
                            AUDIO_RESULT_OK);
        return;
    }
    if (!wait_voice_settle(generation,
                           AUDIO_SERVICE_VOICE_CAPTURE_SETTLE_MS,
                           shutdown_requested))
    {
        result = finish_playback(-1, AUDIO_RESULT_OK);
        if (result != AUDIO_RESULT_OK)
        {
            publish_voice_event(AUDIO_VOICE_EVENT_FAILED,
                                generation,
                                result);
            return;
        }
        publish_voice_event(AUDIO_VOICE_EVENT_CANCELLED,
                            generation,
                            AUDIO_RESULT_OK);
        return;
    }

    const esp_err_t start_error = es8311_bsp_capture_start();
    if (start_error != ESP_OK)
    {
        result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "语音 ADC/RX 启动失败，代次=%lu，稳定错误码=%s，错误=0x%x",
                 (unsigned long)generation,
                 audio_service_result_code(result),
                 (unsigned)start_error);
        publish_voice_event(AUDIO_VOICE_EVENT_FAILED,
                            generation,
                            result);
        return;
    }

    atomic_store(&s_voice_capture_active, true);
    publish_voice_event(AUDIO_VOICE_EVENT_CAPTURE_STARTED,
                        generation,
                        AUDIO_RESULT_OK);
    bool timed_out = false;
    result = capture_voice_pcm(generation,
                               shutdown_requested,
                               &timed_out);
    result = finish_voice_capture(result);
    atomic_store(&s_voice_capture_active, false);
    if (result != AUDIO_RESULT_OK)
    {
        publish_voice_event(AUDIO_VOICE_EVENT_FAILED,
                            generation,
                            result);
        return;
    }
    if (timed_out)
    {
        publish_voice_event(AUDIO_VOICE_EVENT_TIMED_OUT,
                            generation,
                            AUDIO_RESULT_OK);
        return;
    }
    if (*shutdown_requested)
    {
        publish_voice_event(AUDIO_VOICE_EVENT_CANCELLED,
                            generation,
                            AUDIO_RESULT_OK);
        return;
    }
    publish_voice_event(AUDIO_VOICE_EVENT_CAPTURE_STOPPED,
                        generation,
                        AUDIO_RESULT_OK);
}

static bool wait_voice_settle(uint32_t generation,
                              uint32_t settle_ms,
                              bool *shutdown_requested)
{
    if (generation == 0U || shutdown_requested == NULL)
    {
        return false;
    }
    const TickType_t settle_ticks = pdMS_TO_TICKS(settle_ms);
    const TickType_t started_at = xTaskGetTickCount();
    while (xTaskGetTickCount() - started_at < settle_ticks)
    {
        if (voice_stop_requested(generation) ||
            take_stop_request(shutdown_requested))
        {
            return false;
        }
        const TickType_t elapsed = xTaskGetTickCount() - started_at;
        const TickType_t remaining = settle_ticks - elapsed;
        const TickType_t slice = remaining < pdMS_TO_TICKS(5U)
                                     ? remaining
                                     : pdMS_TO_TICKS(5U);
        vTaskDelay(slice > 0U ? slice : 1U);
    }
    return !voice_stop_requested(generation) && !*shutdown_requested;
}

static audio_service_result_t capture_voice_pcm(
    uint32_t generation,
    bool *shutdown_requested,
    bool *timed_out)
{
    if (generation == 0U || shutdown_requested == NULL || timed_out == NULL ||
        s_voice_pcm_stream == NULL || s_voice_capture_frame == NULL)
    {
        return AUDIO_RESULT_NOT_READY;
    }
    *timed_out = false;
    const int64_t started_at_us = esp_timer_get_time();
    uint8_t *const frame = s_voice_capture_frame;
    size_t frame_offset = 0U;
    while (true)
    {
        if (voice_stop_requested(generation) ||
            take_stop_request(shutdown_requested))
        {
            return AUDIO_RESULT_OK;
        }
        const int64_t elapsed_us = esp_timer_get_time() - started_at_us;
        const int64_t remaining_us =
            (int64_t)AUDIO_SERVICE_VOICE_CAPTURE_MAX_MS * 1000LL -
            elapsed_us;
        if (remaining_us < 1000LL)
        {
            *timed_out = true;
            return AUDIO_RESULT_OK;
        }
        uint32_t read_timeout_ms = (uint32_t)(remaining_us / 1000LL);
        if (read_timeout_ms > AUDIO_SERVICE_I2S_READ_TIMEOUT_MS)
        {
            read_timeout_ms = AUDIO_SERVICE_I2S_READ_TIMEOUT_MS;
        }

        size_t bytes_read = 0U;
        const esp_err_t err = es8311_bsp_read(
            frame + frame_offset,
            AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES - frame_offset,
            &bytes_read,
            read_timeout_ms);
        if (bytes_read > AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES - frame_offset)
        {
            return AUDIO_RESULT_I2S_FAILED;
        }
        frame_offset += bytes_read;
        if (err == ESP_ERR_TIMEOUT)
        {
            atomic_fetch_add(&s_voice_rx_timeout_count, 1U);
            ESP_LOGE(TAG,
                     "I2S RX 读取超时，代次=%lu，次数=%lu，稳定错误码=AUDIO_I2S_TIMEOUT",
                     (unsigned long)generation,
                     (unsigned long)atomic_load(&s_voice_rx_timeout_count));
            return AUDIO_RESULT_I2S_FAILED;
        }
        if (err != ESP_OK)
        {
            return map_es8311_failure();
        }
        if (frame_offset < AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES)
        {
            taskYIELD();
            continue;
        }

        if (xStreamBufferSpacesAvailable(s_voice_pcm_stream) <
            AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES)
        {
            ESP_LOGE(TAG,
                     "语音 PCM Stream Buffer 溢出，代次=%lu，稳定错误码=AUDIO_PCM_OVERFLOW",
                     (unsigned long)generation);
            return AUDIO_RESULT_PCM_OVERFLOW;
        }
        const size_t sent = xStreamBufferSend(s_voice_pcm_stream,
                                              frame,
                                              AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES,
                                              0U);
        if (sent != AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES)
        {
            ESP_LOGE(TAG,
                     "语音 PCM Stream Buffer 溢出，代次=%lu，稳定错误码=AUDIO_PCM_OVERFLOW",
                     (unsigned long)generation);
            return AUDIO_RESULT_PCM_OVERFLOW;
        }
        frame_offset = 0U;
        taskYIELD();
    }
}

static audio_service_result_t finish_voice_capture(
    audio_service_result_t result)
{
    const esp_err_t pa_error = safe_off_with_retry();
    if (pa_error != ESP_OK)
    {
        atomic_store(&s_recovery_pending, true);
        atomic_store(&s_voice_recovery_pending, true);
        ESP_LOGE(TAG,
                 "语音采集结束时功放无法安全关断，稳定错误码=%s，错误=0x%x",
                 ns4150_bsp_error_code(pa_error),
                 (unsigned)pa_error);
        return AUDIO_RESULT_PA_FAILED;
    }
    const esp_err_t stop_error = es8311_bsp_capture_stop();
    if (stop_error != ESP_OK)
    {
        atomic_store(&s_recovery_pending, true);
        atomic_store(&s_voice_recovery_pending, true);
        const audio_service_result_t stop_result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "语音 ADC/RX 停止失败，稳定错误码=%s，错误=0x%x",
                 audio_service_result_code(stop_result),
                 (unsigned)stop_error);
        return stop_result;
    }
    atomic_store(&s_recovery_pending, false);
    atomic_store(&s_voice_recovery_pending, false);
    return result;
}

static void update_processing_metric(int64_t started_at_us)
{
    int64_t elapsed = esp_timer_get_time() - started_at_us;
    if (elapsed <= 0)
    {
        return;
    }
    uint_fast32_t observed = atomic_load(&s_max_processing_us);
    const uint_fast32_t candidate = (uint_fast32_t)elapsed;
    while (candidate > observed &&
           !atomic_compare_exchange_weak(&s_max_processing_us, &observed, candidate))
    {
        /* observed 由失败的 CAS 刷新，直到保留更大的并发样本。 */
    }
}

static audio_service_result_t map_es8311_failure(void)
{
    return es8311_bsp_last_failure() == ES8311_BSP_FAILURE_I2S
               ? AUDIO_RESULT_I2S_FAILED
               : AUDIO_RESULT_CODEC_FAILED;
}

static void log_dma_heap(const char *stage)
{
    const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA |
                          MALLOC_CAP_8BIT;
    ESP_LOGI(TAG,
             "音频 DMA 内存%s：空闲=%lu 字节，最大连续块=%lu 字节",
             stage,
             (unsigned long)heap_caps_get_free_size(caps),
             (unsigned long)heap_caps_get_largest_free_block(caps));
}

static esp_err_t probe_hardware_ready(void)
{
    esp_err_t err = safe_off_with_retry();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "音频就绪检查无法保持 NS4150 关断，稳定错误码=%s，错误=0x%x",
                 ns4150_bsp_error_code(err),
                 (unsigned)err);
        return err;
    }

    legbot_bsp_i2c_access_t access = {0};
    err = i2c_manager_get_access(&access);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "音频就绪检查获取共享 I2C 失败，稳定错误码=AUDIO_CODEC_FAILED，错误=0x%x",
                 (unsigned)err);
        return err;
    }
    log_dma_heap("初始化前");
    err = es8311_bsp_init(&access);
    if (err != ESP_OK)
    {
        log_dma_heap("初始化失败后");
        ESP_LOGE(TAG,
                 "ES8311 无声初始化失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)err);
        return err;
    }

    err = es8311_bsp_stop();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "ES8311 无声检查停止失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)err);
        return err;
    }
    log_dma_heap("初始化后");
    ESP_LOGI(TAG,
             "ES8311/I2S 无声就绪检查通过，DMA 句柄保留，NS4150 保持安全关断");
    return ESP_OK;
}

static esp_err_t safe_off_with_retry(void)
{
    esp_err_t err = ESP_OK;
    for (uint32_t attempt = 0; attempt < AUDIO_SERVICE_CRITICAL_RETRY_COUNT; ++attempt)
    {
        err = ns4150_bsp_safe_off();
        if (err == ESP_OK)
        {
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return err;
}

static audio_service_result_t finish_playback(int file_descriptor,
                                              audio_service_result_t result)
{
    esp_err_t pa_error = safe_off_with_retry();
    if (pa_error != ESP_OK)
    {
        esp_err_t stop_error = es8311_bsp_stop();
        atomic_store(&s_recovery_pending, true);
        atomic_store(&s_voice_recovery_pending, false);
        ESP_LOGE(TAG,
                 "功放安全关断失败，已执行 codec/I2S 二级静音并保留句柄重试，稳定错误码=%s，错误=0x%x，二级静音=0x%x",
                 ns4150_bsp_error_code(pa_error),
                 (unsigned)pa_error,
                 (unsigned)stop_error);
        if (file_descriptor >= 0 && close(file_descriptor) != 0)
        {
            ESP_LOGW(TAG, "功放失败路径关闭 WAV 文件失败，稳定错误码=AUDIO_IO_FAILED");
        }
        return AUDIO_RESULT_PA_FAILED;
    }

    esp_err_t codec_error = es8311_bsp_stop();
    audio_service_result_t cleanup_result = AUDIO_RESULT_OK;
    if (codec_error != ESP_OK)
    {
        atomic_store(&s_recovery_pending, true);
        atomic_store(&s_voice_recovery_pending, false);
        cleanup_result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "ES8311/I2S 停止失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)codec_error);
    }
    if (file_descriptor >= 0 && close(file_descriptor) != 0 &&
        cleanup_result == AUDIO_RESULT_OK)
    {
        cleanup_result = AUDIO_RESULT_IO_FAILED;
    }
    if (cleanup_result == AUDIO_RESULT_OK)
    {
        atomic_store(&s_recovery_pending, false);
        atomic_store(&s_voice_recovery_pending, false);
    }
    return cleanup_result == AUDIO_RESULT_OK ? result : cleanup_result;
}

static audio_service_result_t finish_voice_cue_playback(
    int file_descriptor,
    audio_service_result_t result)
{
    const esp_err_t pa_error = safe_off_with_retry();
    if (pa_error != ESP_OK)
    {
        return finish_playback(file_descriptor, AUDIO_RESULT_PA_FAILED);
    }

    const esp_err_t prepare_error =
        es8311_bsp_prepare_capture_after_playback();
    if (prepare_error != ESP_OK)
    {
        const audio_service_result_t prepare_result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "语音提示音暖切换失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)prepare_error);
        return finish_playback(file_descriptor, prepare_result);
    }
    if (file_descriptor >= 0 && close(file_descriptor) != 0)
    {
        return finish_playback(-1, AUDIO_RESULT_IO_FAILED);
    }
    atomic_store(&s_recovery_pending, false);
    atomic_store(&s_voice_recovery_pending, false);
    return result;
}

static audio_service_result_t shutdown_audio_hardware(
    audio_service_result_t result)
{
    audio_service_result_t cleanup_result = finish_playback(-1, result);
    const esp_err_t codec_error = es8311_bsp_deinit();
    if (codec_error != ESP_OK)
    {
        atomic_store(&s_recovery_pending, true);
        cleanup_result = map_es8311_failure();
        ESP_LOGE(TAG,
                 "audio_task 关闭时释放 ES8311/I2S 失败，稳定错误码=%s，错误=0x%x",
                 es8311_bsp_error_code(),
                 (unsigned)codec_error);
    }
    else
    {
        atomic_store(&s_recovery_pending, false);
    }
    return cleanup_result;
}

static watch_audio_error_t map_watch_error(audio_service_result_t result)
{
    switch (result)
    {
    case AUDIO_RESULT_OK:
        return WATCH_AUDIO_ERROR_NONE;
    case AUDIO_RESULT_RESOURCE_MISSING:
        return WATCH_AUDIO_ERROR_RESOURCE_MISSING;
    case AUDIO_RESULT_CORRUPT:
        return WATCH_AUDIO_ERROR_CORRUPT;
    case AUDIO_RESULT_UNSUPPORTED:
        return WATCH_AUDIO_ERROR_UNSUPPORTED;
    case AUDIO_RESULT_IO_FAILED:
        return WATCH_AUDIO_ERROR_IO_FAILED;
    case AUDIO_RESULT_BUSY:
        return WATCH_AUDIO_ERROR_BUSY;
    case AUDIO_RESULT_I2S_FAILED:
    case AUDIO_RESULT_PCM_OVERFLOW:
        return WATCH_AUDIO_ERROR_I2S_FAILED;
    case AUDIO_RESULT_PA_FAILED:
        return WATCH_AUDIO_ERROR_PA_FAILED;
    case AUDIO_RESULT_CODEC_FAILED:
    case AUDIO_RESULT_HW_FAILED:
        return WATCH_AUDIO_ERROR_CODEC_FAILED;
    case AUDIO_RESULT_NOT_READY:
    default:
        return WATCH_AUDIO_ERROR_NOT_READY;
    }
}

static void record_playback_terminal(int64_t accepted_at_us)
{
    if (accepted_at_us <= 0)
    {
        return;
    }
    int64_t elapsed_us = esp_timer_get_time() - accepted_at_us;
    const uint32_t elapsed_ms = elapsed_us > 0 ? (uint32_t)(elapsed_us / 1000) : 0;
    atomic_store(&s_playback_elapsed_ms, elapsed_ms);
    if (elapsed_ms > AUDIO_SERVICE_PLAYBACK_ACCEPTANCE_MS)
    {
        ESP_LOGW(TAG,
                 "play 请求到 terminal state 超过 10 秒代码口径，耗时=%u ms",
                 (unsigned)elapsed_ms);
    }
    if (atomic_load(&s_max_processing_us) > AUDIO_SERVICE_MAX_PROCESSING_US)
    {
        ESP_LOGW(TAG,
                 "连续处理区段超过 20 ms 代码口径，最大=%u us",
                 (unsigned)atomic_load(&s_max_processing_us));
    }
}

static esp_err_t flush_pending_terminal(void)
{
    if (!atomic_load(&s_terminal_pending))
    {
        return ESP_OK;
    }
    esp_err_t err = state_service_publish_audio(
        &s_pending_terminal_update,
        pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        record_playback_terminal(s_pending_terminal_accepted_at_us);
        atomic_store(&s_terminal_pending, false);
    }
    return err;
}

static esp_err_t publish_audio_state(watch_audio_state_t state,
                                     watch_audio_error_t error,
                                     bool terminal,
                                     int64_t accepted_at_us)
{
    const watch_audio_update_t update = {
        .resource_available = audio_service_storage_ready(),
        .state = state,
        .error = error,
        .updated_at_ticks = xTaskGetTickCount(),
    };
    const uint32_t attempts = terminal ? AUDIO_SERVICE_CRITICAL_RETRY_COUNT : 1U;
    esp_err_t err = ESP_ERR_TIMEOUT;
    for (uint32_t attempt = 0; attempt < attempts; ++attempt)
    {
        err = state_service_publish_audio(
            &update,
            terminal ? pdMS_TO_TICKS(STATE_SERVICE_APPLY_TIMEOUT_MS) : 0);
        if (err == ESP_OK)
        {
            if (terminal)
            {
                record_playback_terminal(accepted_at_us);
                atomic_store(&s_terminal_pending, false);
            }
            return ESP_OK;
        }
        taskYIELD();
    }
    ESP_LOGW(TAG,
             "音频 typed 状态发布失败，状态=%d，原因=%d，错误=0x%x",
             (int)state,
             (int)error,
             (unsigned)err);
    if (terminal)
    {
        s_pending_terminal_update = update;
        s_pending_terminal_accepted_at_us = accepted_at_us;
        atomic_store(&s_terminal_pending, true);
    }
    return err;
}
