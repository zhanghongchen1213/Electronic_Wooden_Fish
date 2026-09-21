/**
 * @file     voice_service.c
 * @brief    BOOT0 按键离线语音控制服务实现
 * @details  在独立 PSRAM 栈任务中执行 GPIO0 去抖、500 ms 长按、最小 AFE 与 MultiNet 流式识别、既有 BLE typed intent 提交；model 分区 mmap 由内部 main task 启动栈提前完成并保持到本次开机结束。
 * @author   ZHC
 * @date     2026-08-05
 */

#include "voice_service.h"

#include <math.h>
#include <stddef.h>
#include <stdatomic.h>
#include <string.h>

#include "audio_service.h"
#include "ble_service.h"
#include "control_ui_binding.h"
#include "esp_afe_config.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include "key.h"
#include "model_path.h"
#include "power_service.h"
#include "sdkconfig.h"
#include "voice_control_policy.h"

static const char *TAG = "SVC_VOICE";

/** voice task 活动会话轮询周期。 */
#define VOICE_SERVICE_SESSION_POLL_MS 5U
/** voice owner 停止活动音频会话的最长清理等待。 */
#define VOICE_SERVICE_AUDIO_STOP_WAIT_MS 250U
/** 固定模型分区标签。 */
#define VOICE_SERVICE_MODEL_PARTITION_LABEL "model"
/** 首版唯一允许的中文 MultiNet 模型名。 */
#define VOICE_SERVICE_MODEL_NAME "mn5q8_cn"
/** MultiNet 单次会话内部识别超时。 */
#define VOICE_SERVICE_MULTINET_TIMEOUT_MS 5000
/** MultiNet 与业务共同使用的候选概率基线，业务层要求严格大于该值。 */
#define VOICE_SERVICE_MULTINET_INTERNAL_THRESHOLD 0.40F
/** MultiNet 创建窗口中，全部通用 malloc 优先使用 PSRAM，为 BLE 连接期保留内部 DMA。 */
#define VOICE_SERVICE_MODEL_MALLOC_INTERNAL_LIMIT_BYTES 0U
/** AFE 与 MultiNet 的固定采样率。 */
#define VOICE_SERVICE_SAMPLE_RATE_HZ 16000U
/** 单麦 AFE 输入通道格式。 */
#define VOICE_SERVICE_AFE_INPUT_FORMAT "M"
/** AFE fetch 的有界等待时间。 */
#define VOICE_SERVICE_AFE_FETCH_TIMEOUT_MS 40U
/** 进入会话前等待 power owner 点亮屏幕并确认本地活动的上限。 */
#define VOICE_SERVICE_ACTIVITY_CONFIRM_TIMEOUT_MS 1000U
/** 首版 AFE feed、fetch 与 MultiNet 共用的 32 ms 帧样本数。 */
#define VOICE_SERVICE_PIPELINE_FRAME_SAMPLES 512
/** PM lock 释放失败时的有界重试次数。 */
#define VOICE_SERVICE_PM_RELEASE_RETRY_COUNT 3U
/** 录音峰值内部 DMA 总空闲下限。 */
#define VOICE_SERVICE_DMA_FREE_FLOOR_BYTES (16U * 1024U)
/** 录音峰值内部 DMA 最大连续块下限。 */
#define VOICE_SERVICE_DMA_LARGEST_FLOOR_BYTES (8U * 1024U)
/** 录音峰值 PSRAM 总空闲与最大连续块下限。 */
#define VOICE_SERVICE_PSRAM_FLOOR_BYTES (1024U * 1024U)
/** voice task 最小剩余栈下限。 */
#define VOICE_SERVICE_STACK_FREE_FLOOR_BYTES (2U * 1024U)
/** 绝对值达到该值的采样计入削顶诊断。 */
#define VOICE_SERVICE_PCM_CLIP_THRESHOLD 32760U
/** 真机识别闭环采用的 WebRTC AGC 压缩增益。 */
#define VOICE_SERVICE_AFE_AGC_COMPRESSION_GAIN_DB 9
/** 真机识别闭环采用的 WebRTC AGC 目标电平。 */
#define VOICE_SERVICE_AFE_AGC_TARGET_LEVEL_DBFS 3

typedef struct
{
    int command_id;       /**< 同一命令可复用的稳定 ID。 */
    const char *phonemes; /**< MultiNet5 中文模型接受的空格分隔拼音。 */
} voice_command_phrase_t;

typedef struct
{
    uint32_t generation;    /**< 当前会话的非零代次。 */
    bool active;            /**< 提示音或解码流程尚未结束。 */
    bool audio_terminal;    /**< audio owner 已发布本代次终态。 */
    bool capture_started;   /**< 本代次曾进入 ADC/RX 采集。 */
    bool auto_stop_requested; /**< 三秒窗口已请求正常停止，且不得重复请求。 */
    bool decoder_finalized; /**< 队列残留与 320 ms 静音已处理。 */
    bool failed;            /**< 本代次必须零 BLE 写入。 */
    bool command_locked;    /**< 已锁存首个通过阈值的命令。 */
    int command_id;         /**< 锁存的 MultiNet 稳定命令 ID。 */
    uint64_t capture_started_at_ms; /**< ADC/RX 采集开始的单调毫秒。 */
    uint64_t pcm_square_sum; /**< 原始 ADC PCM 平方和，只用于会话内 RMS 诊断。 */
    uint32_t pcm_sample_count; /**< 已送入统计的原始 ADC PCM 样本数。 */
    uint32_t pcm_peak;       /**< 会话内原始 ADC PCM 最大绝对幅度。 */
    uint32_t pcm_clipped;    /**< 会话内达到削顶阈值的样本数。 */
    uint32_t multinet_frames; /**< 实际送入 MultiNet detect 的帧数。 */
    uint32_t multinet_detected_events; /**< MultiNet 返回 DETECTED 的次数。 */
    uint32_t multinet_empty_results; /**< DETECTED 但没有命令图候选的次数。 */
    size_t afe_feed_buffered_samples; /**< AFE feed 暂存区当前有效样本数。 */
    uint64_t afe_fed_samples; /**< 已成功送入 AFE 的单声道样本总数。 */
    uint64_t afe_fetched_samples; /**< 已从 AFE 取出的单声道样本总数。 */
    uint32_t afe_frames;      /**< AFE 成功 fetch 的帧数。 */
} voice_session_t;

typedef struct
{
    size_t dma_free;       /**< 片内 DMA 总空闲字节数。 */
    size_t dma_largest;    /**< 片内 DMA 最大连续块字节数。 */
    size_t psram_free;     /**< PSRAM 总空闲字节数。 */
    size_t psram_largest;  /**< PSRAM 最大连续块字节数。 */
    UBaseType_t stack_free; /**< voice task 栈高水位字节数。 */
} voice_runtime_resources_t;

/** 固定普通话命令表；不注册短口令“关机”。 */
static const voice_command_phrase_t s_command_phrases[] = {
    {VOICE_COMMAND_AI_MODE, "ei ai mo shi"},
    {VOICE_COMMAND_AI_MODE, "zhi neng mo shi"},
    {VOICE_COMMAND_AI_MODE, "biao zhun mo shi"},
    {VOICE_COMMAND_EXTREME_MODE, "ji xian mo shi"},
    {VOICE_COMMAND_FITNESS_MODE, "jian shen mo shi"},
    {VOICE_COMMAND_DOWNHILL_MODE, "xia shan mo shi"},
    {VOICE_COMMAND_POWEROFF, "wai gu ge guan ji"},
    {VOICE_COMMAND_ASSIST_ON, "kai qi zhu li"},
    {VOICE_COMMAND_ASSIST_OFF, "guan bi zhu li"},
    {VOICE_COMMAND_GEAR_UP, "zhu li jia"},
    {VOICE_COMMAND_GEAR_DOWN, "zhu li jian"},
    {VOICE_COMMAND_GEAR_1, "zhu li yi"},
    {VOICE_COMMAND_GEAR_2, "zhu li er"},
    {VOICE_COMMAND_GEAR_3, "zhu li san"},
    {VOICE_COMMAND_GEAR_4, "zhu li si"},
    {VOICE_COMMAND_GEAR_5, "zhu li wu"},
    {VOICE_COMMAND_GEAR_6, "zhu li liu"},
    {VOICE_COMMAND_GEAR_7, "zhu li qi"},
    {VOICE_COMMAND_GEAR_8, "zhu li ba"},
    {VOICE_COMMAND_GEAR_9, "zhu li jiu"},
    {VOICE_COMMAND_GEAR_10, "zhu li shi"},
};

/** voice task 是否已进入运行函数。 */
static atomic_bool s_running;
/** 框架或运行态资源门槛请求停止任务。 */
static atomic_bool s_stop_requested;
/** 第二轮内存门槛或外部策略请求永久禁用本启动周期。 */
static atomic_bool s_disable_requested;
/** voice owner 已完成禁用清理。 */
static atomic_bool s_disable_acknowledged;
/** 模型初始化是否已经形成成功或失败结论。 */
static atomic_bool s_startup_complete;
/** 模型、命令表、PM lock 与 BOOT0 ISR 是否完整可用。 */
static atomic_bool s_ready;
/** 当前 voice task 句柄，仅供 BOOT0 ISR 和控制入口唤醒。 */
static _Atomic(TaskHandle_t) s_voice_task;
/** 启动周期内单调且跳过零值的语音会话代次。 */
static atomic_uint s_next_generation;
/** 当前活动语音会话代次，零表示空闲。 */
static atomic_uint s_active_generation;
/** 已在内部启动栈 mmap 的 model 分区模型列表，本次开机期间保持映射。 */
static srmodel_list_t *s_models;
/** MultiNet5 操作接口。 */
static esp_mn_iface_t *s_multinet;
/** MultiNet5 模型实例。 */
static model_iface_data_t *s_model_data;
/** MultiNet 单帧样本数，首版必须为 512。 */
static int s_model_chunk_samples;
/** 最小 AFE 操作接口。 */
static const esp_afe_sr_iface_t *s_afe;
/** 最小 AFE 常驻实例。 */
static esp_afe_sr_data_t *s_afe_data;
/** AFE 单次 feed 的单通道样本数。 */
static int s_afe_feed_chunk_samples;
/** AFE 单次 fetch 的单通道样本数。 */
static int s_afe_fetch_chunk_samples;
/** PSRAM 中常驻的 AFE feed 暂存区，避免压缩 voice task 栈余量。 */
static int16_t *s_afe_feed_buffer;
/** 禁止语音会话期间进入 light sleep 的 PM lock。 */
static esp_pm_lock_handle_t s_no_light_sleep_lock;
/** 保持语音会话期间最高 CPU 频率的 PM lock。 */
static esp_pm_lock_handle_t s_cpu_frequency_lock;
/** 当前会话是否持有禁止 light sleep 的 PM lock。 */
static bool s_no_light_sleep_acquired;
/** 当前会话是否持有最高 CPU 频率 PM lock。 */
static bool s_cpu_frequency_acquired;

static void voice_boot_isr(void *context);
static esp_err_t initialize_runtime(void);
static void deinitialize_runtime(void);
static esp_err_t initialize_model(void);
static void deinitialize_model(void);
static esp_err_t initialize_afe(void);
static void deinitialize_afe(void);
static esp_err_t register_commands(void);
static void run_button_loop(void);
static void stop_and_finish_session(voice_session_t *session);
static bool read_boot_pressed(void);
static void fail_session_for_rearm_error(voice_session_t *session,
                                         esp_err_t error);
static bool refresh_local_activity_before_session(void);
static void log_gate_rejection(const char *stage,
                               const watch_state_snapshot_t *snapshot,
                               esp_err_t snapshot_error,
                               uint64_t now_ms,
                               bool require_screen_on);
static const char *control_block_reason_text(
    control_ui_block_reason_t reason);
static bool begin_session(voice_session_t *session);
static void request_session_stop(voice_session_t *session);
static void pump_session(voice_session_t *session);
static void enforce_session_gate(voice_session_t *session);
static bool runtime_resources_meet_floors(
    voice_runtime_resources_t *resources);
static void enforce_session_resource_floors(voice_session_t *session);
static void enforce_capture_window(voice_session_t *session);
static void process_audio_events(voice_session_t *session);
static void process_pcm_frames(voice_session_t *session, bool drain_all);
static void update_pcm_metrics(voice_session_t *session,
                               const int16_t *samples,
                               size_t sample_count);
static void log_pcm_metrics(const voice_session_t *session);
static void process_afe_samples(voice_session_t *session,
                                const int16_t *samples,
                                size_t sample_count);
static bool feed_afe_chunk(voice_session_t *session);
static bool fetch_afe_frame(voice_session_t *session);
static void flush_afe_feed_buffer(voice_session_t *session);
static void process_model_frame(voice_session_t *session, int16_t *samples);
static void finalize_decoder(voice_session_t *session);
static void finish_session(voice_session_t *session);
static void submit_locked_command(const voice_session_t *session);
static esp_err_t acquire_session_pm_locks(void);
static esp_err_t release_session_pm_locks(void);
static esp_err_t release_pm_lock_with_retry(esp_pm_lock_handle_t lock,
                                            bool *acquired,
                                            const char *name);
static uint32_t next_generation(void);
static uint64_t monotonic_ms(void);
static void log_memory(const char *stage);

esp_err_t voice_service_prepare_run(void)
{
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_disable_requested, false);
    atomic_store(&s_disable_acknowledged, false);
    atomic_store(&s_startup_complete, false);
    atomic_store(&s_ready, false);
    atomic_store(&s_active_generation, 0U);
    return ESP_OK;
}

esp_err_t voice_service_prepare_model_mapping(void)
{
    if (atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_models != NULL)
    {
        return ESP_OK;
    }
    uint8_t stack_marker = 0U;
    if (!esp_ptr_in_dram(&stack_marker))
    {
        ESP_LOGE(TAG,
                 "拒绝从非 DRAM 栈映射 model 分区，稳定错误码=VOICE_MODEL_MAP_STACK_INVALID");
        return ESP_ERR_INVALID_STATE;
    }
    s_models = esp_srmodel_init(VOICE_SERVICE_MODEL_PARTITION_LABEL);
    if (s_models == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG,
             "model 分区已在内部启动栈完成映射，本次开机保持映射以避免 PSRAM 栈冻结 cache");
    return ESP_OK;
}

void voice_service_cancel_prepared_run(void)
{
    atomic_store(&s_stop_requested, true);
    atomic_store(&s_startup_complete, false);
    atomic_store(&s_ready, false);
}

esp_err_t voice_service_request_stop(void)
{
    if (!atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, true);
    const uint32_t generation = atomic_load(&s_active_generation);
    if (generation != 0U)
    {
        (void)audio_service_request_voice_capture_stop(generation);
    }
    TaskHandle_t task = atomic_load(&s_voice_task);
    if (task != NULL)
    {
        xTaskNotifyGive(task);
    }
    return ESP_OK;
}

esp_err_t voice_service_disable(TickType_t timeout_ticks)
{
    if (!atomic_load(&s_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_disable_requested, true);
    const uint32_t generation = atomic_load(&s_active_generation);
    if (generation != 0U)
    {
        (void)audio_service_request_voice_capture_stop(generation);
    }
    TaskHandle_t task = atomic_load(&s_voice_task);
    if (task != NULL)
    {
        xTaskNotifyGive(task);
    }
    const TickType_t started_at = xTaskGetTickCount();
    while (!atomic_load(&s_disable_acknowledged))
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

bool voice_service_startup_complete(void)
{
    return atomic_load(&s_startup_complete);
}

bool voice_service_is_ready(void)
{
    return atomic_load(&s_ready) && !atomic_load(&s_disable_requested) &&
           !atomic_load(&s_stop_requested);
}

esp_err_t voice_service_run(void)
{
    bool expected = false;
    if (!atomic_compare_exchange_strong(&s_running, &expected, true))
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_voice_task, xTaskGetCurrentTaskHandle());
    const esp_err_t init_error = initialize_runtime();
    atomic_store(&s_startup_complete, true);
    if (init_error == ESP_OK)
    {
        atomic_store(&s_ready, true);
        ESP_LOGI(TAG, "BOOT0 离线语音控制已就绪，模型=mn5q8_cn，唤醒词=关闭");
        run_button_loop();
    }
    else
    {
        ESP_LOGE(TAG,
                 "离线语音初始化失败，本启动周期已禁用，稳定错误码=VOICE_MODEL_UNAVAILABLE，错误=0x%x",
                 (unsigned)init_error);
        while (!atomic_load(&s_stop_requested) &&
               !atomic_load(&s_disable_requested))
        {
            (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }

    atomic_store(&s_ready, false);
    deinitialize_runtime();
    atomic_store(&s_disable_acknowledged, true);
    atomic_store(&s_voice_task, NULL);
    atomic_store(&s_running, false);
    return ESP_OK;
}

static void voice_boot_isr(void *context)
{
    (void)context;
    TaskHandle_t task = atomic_load_explicit(&s_voice_task,
                                             memory_order_acquire);
    if (task == NULL)
    {
        return;
    }
    BaseType_t higher_priority_task_woken = pdFALSE;
    vTaskNotifyGiveFromISR(task, &higher_priority_task_woken);
    if (higher_priority_task_woken == pdTRUE)
    {
        portYIELD_FROM_ISR();
    }
}

static esp_err_t initialize_runtime(void)
{
    esp_err_t err = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,
                                       0,
                                       "voice_no_ls",
                                       &s_no_light_sleep_lock);
    if (err != ESP_OK)
    {
        return err;
    }
    err = esp_pm_lock_create(ESP_PM_CPU_FREQ_MAX,
                             0,
                             "voice_cpu_max",
                             &s_cpu_frequency_lock);
    if (err != ESP_OK)
    {
        deinitialize_runtime();
        return err;
    }
    if (!audio_service_voice_capture_ready())
    {
        ESP_LOGE(TAG,
                 "音频 ADC/RX 未就绪，拒绝把语音模型标记为可用，稳定错误码=VOICE_AUDIO_NOT_READY");
        deinitialize_runtime();
        return ESP_ERR_INVALID_STATE;
    }
    err = initialize_model();
    if (err != ESP_OK)
    {
        deinitialize_runtime();
        return err;
    }
    err = key_register_boot_isr_callback(voice_boot_isr, NULL);
    if (err != ESP_OK)
    {
        deinitialize_runtime();
        return err;
    }
    return ESP_OK;
}

static void deinitialize_runtime(void)
{
    atomic_store(&s_ready, false);
    (void)key_unregister_boot_isr_callback();
    const esp_err_t release_error = release_session_pm_locks();
    if (release_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "语音运行态退出时 PM lock 仍未释放，稳定错误码=VOICE_PM_LOCK_RELEASE_FAILED，错误=0x%x",
                 (unsigned)release_error);
    }
    deinitialize_model();
    if (s_cpu_frequency_lock != NULL && !s_cpu_frequency_acquired)
    {
        const esp_err_t delete_error = esp_pm_lock_delete(s_cpu_frequency_lock);
        if (delete_error == ESP_OK)
        {
            s_cpu_frequency_lock = NULL;
        }
    }
    if (s_no_light_sleep_lock != NULL && !s_no_light_sleep_acquired)
    {
        const esp_err_t delete_error = esp_pm_lock_delete(s_no_light_sleep_lock);
        if (delete_error == ESP_OK)
        {
            s_no_light_sleep_lock = NULL;
        }
    }
}

static esp_err_t initialize_model(void)
{
    log_memory("模型加载前");
    if (s_models == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    char *model_name = esp_srmodel_filter(s_models,
                                          ESP_MN_PREFIX,
                                          ESP_MN_CHINESE);
    if (model_name == NULL || strcmp(model_name, VOICE_SERVICE_MODEL_NAME) != 0)
    {
        return ESP_ERR_NOT_FOUND;
    }
    s_multinet = esp_mn_handle_from_name(model_name);
    if (s_multinet == NULL)
    {
        return ESP_ERR_NOT_FOUND;
    }
    const int64_t create_started_at_us = esp_timer_get_time();
    heap_caps_malloc_extmem_enable(
        VOICE_SERVICE_MODEL_MALLOC_INTERNAL_LIMIT_BYTES);
    s_model_data = s_multinet->create(model_name,
                                      VOICE_SERVICE_MULTINET_TIMEOUT_MS);
    heap_caps_malloc_extmem_enable(
        CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL);
    const int64_t create_elapsed_ms =
        (esp_timer_get_time() - create_started_at_us) / 1000LL;
    if (s_model_data == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    if (create_elapsed_ms > VOICE_SERVICE_MODEL_CREATE_LIMIT_MS)
    {
        ESP_LOGE(TAG,
                 "MultiNet 实例创建超过 5000 ms 验收线，耗时=%lld ms",
                 (long long)create_elapsed_ms);
        return ESP_ERR_TIMEOUT;
    }
    if (s_multinet->get_samp_rate(s_model_data) !=
        VOICE_SERVICE_SAMPLE_RATE_HZ)
    {
        return ESP_ERR_INVALID_RESPONSE;
    }
    s_model_chunk_samples = s_multinet->get_samp_chunksize(s_model_data);
    if (s_model_chunk_samples <= 0 ||
        (size_t)s_model_chunk_samples * sizeof(int16_t) !=
            AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES)
    {
        ESP_LOGE(TAG,
                 "MultiNet 帧长与 1024 字节音频帧不一致，样本=%d",
                 s_model_chunk_samples);
        return ESP_ERR_INVALID_SIZE;
    }
    if (s_multinet->set_det_threshold == NULL ||
        s_multinet->set_det_threshold(
            s_model_data,
            VOICE_SERVICE_MULTINET_INTERNAL_THRESHOLD) != 0)
    {
        ESP_LOGE(TAG,
                 "MultiNet 内部候选阈值配置失败，稳定错误码=VOICE_MODEL_THRESHOLD_FAILED");
        return ESP_ERR_INVALID_RESPONSE;
    }
    const esp_err_t command_error = register_commands();
    if (command_error != ESP_OK)
    {
        return command_error;
    }
    const esp_err_t afe_error = initialize_afe();
    if (afe_error != ESP_OK)
    {
        return afe_error;
    }
    s_multinet->clean(s_model_data);
    log_memory("模型加载后");
    ESP_LOGI(TAG,
             "AFE/MultiNet 模型加载完成，耗时=%lld ms，采样率=%d，feed/fetch/MultiNet=%d/%d/%d，内部候选阈值=%.2f",
             (long long)create_elapsed_ms,
             s_multinet->get_samp_rate(s_model_data),
             s_afe_feed_chunk_samples,
             s_afe_fetch_chunk_samples,
             s_model_chunk_samples,
             (double)VOICE_SERVICE_MULTINET_INTERNAL_THRESHOLD);
    return ESP_OK;
}

static void deinitialize_model(void)
{
    deinitialize_afe();
    if (s_model_data != NULL && s_multinet != NULL)
    {
        (void)esp_mn_commands_free();
        s_multinet->destroy(s_model_data);
        s_model_data = NULL;
    }
    s_multinet = NULL;
    s_model_chunk_samples = 0;
}

static esp_err_t initialize_afe(void)
{
    afe_config_t *config = afe_config_init(VOICE_SERVICE_AFE_INPUT_FORMAT,
                                           s_models,
                                           AFE_TYPE_SR,
                                           AFE_MODE_LOW_COST);
    if (config == NULL)
    {
        ESP_LOGE(TAG,
                 "AFE 配置创建失败，稳定错误码=VOICE_AFE_CONFIG_FAILED");
        return ESP_ERR_NO_MEM;
    }

    config->aec_init = false;
    config->se_init = false;
    config->ns_init = false;
    config->ns_model_name = NULL;
    config->afe_ns_mode = AFE_NS_MODE_WEBRTC;
    config->vad_init = false;
    config->vad_model_name = NULL;
    config->vad_mode = VAD_MODE_0;
    config->wakenet_init = false;
    config->agc_init = true;
    config->agc_mode = AFE_AGC_MODE_WEBRTC;
    config->agc_compression_gain_db =
        VOICE_SERVICE_AFE_AGC_COMPRESSION_GAIN_DB;
    config->agc_target_level_dbfs =
        VOICE_SERVICE_AFE_AGC_TARGET_LEVEL_DBFS;
    config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    config->afe_linear_gain = 1.0F;
    config->fixed_first_channel = true;
    config->fixed_output_channel = true;
    config->output_playback_channel = false;

    afe_config_t *checked_config = afe_config_check(config);
    if (checked_config == NULL)
    {
        afe_config_free(config);
        ESP_LOGE(TAG,
                 "AFE 配置校验失败，稳定错误码=VOICE_AFE_CONFIG_FAILED");
        return ESP_ERR_INVALID_ARG;
    }
    config = checked_config;
    if (config->pcm_config.sample_rate != VOICE_SERVICE_SAMPLE_RATE_HZ ||
        config->pcm_config.total_ch_num != 1 ||
        config->pcm_config.mic_num != 1 || config->pcm_config.ref_num != 0 ||
        config->aec_init || config->se_init ||
        config->ns_init ||
        config->ns_model_name != NULL ||
        config->afe_ns_mode != AFE_NS_MODE_WEBRTC ||
        config->vad_init || config->wakenet_init || !config->agc_init ||
        config->agc_mode != AFE_AGC_MODE_WEBRTC ||
        config->agc_compression_gain_db !=
            VOICE_SERVICE_AFE_AGC_COMPRESSION_GAIN_DB ||
        config->agc_target_level_dbfs !=
            VOICE_SERVICE_AFE_AGC_TARGET_LEVEL_DBFS)
    {
        ESP_LOGE(TAG,
                 "AFE 校验后配置超出单麦最小管线，采样率=%d，通道=%d，麦克风=%d，参考=%d，稳定错误码=VOICE_AFE_CONFIG_MISMATCH",
                 config->pcm_config.sample_rate,
                 config->pcm_config.total_ch_num,
                 config->pcm_config.mic_num,
                 config->pcm_config.ref_num);
        afe_config_free(config);
        return ESP_ERR_INVALID_RESPONSE;
    }

    s_afe = esp_afe_handle_from_config(config);
    if (s_afe == NULL)
    {
        afe_config_free(config);
        ESP_LOGE(TAG,
                 "AFE 操作接口不可用，稳定错误码=VOICE_AFE_UNAVAILABLE");
        return ESP_ERR_NOT_FOUND;
    }
    heap_caps_malloc_extmem_enable(
        VOICE_SERVICE_MODEL_MALLOC_INTERNAL_LIMIT_BYTES);
    s_afe_data = s_afe->create_from_config(config);
    heap_caps_malloc_extmem_enable(CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL);
    afe_config_free(config);
    if (s_afe_data == NULL)
    {
        ESP_LOGE(TAG,
                 "AFE 实例创建失败，稳定错误码=VOICE_AFE_CREATE_FAILED");
        s_afe = NULL;
        return ESP_ERR_NO_MEM;
    }

    const int sample_rate = s_afe->get_samp_rate(s_afe_data);
    const int feed_channels = s_afe->get_feed_channel_num(s_afe_data);
    const int fetch_channels = s_afe->get_fetch_channel_num(s_afe_data);
    s_afe_feed_chunk_samples = s_afe->get_feed_chunksize(s_afe_data);
    s_afe_fetch_chunk_samples = s_afe->get_fetch_chunksize(s_afe_data);
    if (sample_rate != VOICE_SERVICE_SAMPLE_RATE_HZ || feed_channels != 1 ||
        fetch_channels != 1 || s_afe_feed_chunk_samples <= 0 ||
        s_afe_feed_chunk_samples > VOICE_SERVICE_PIPELINE_FRAME_SAMPLES ||
        s_afe_fetch_chunk_samples != VOICE_SERVICE_PIPELINE_FRAME_SAMPLES ||
        s_model_chunk_samples != VOICE_SERVICE_PIPELINE_FRAME_SAMPLES)
    {
        ESP_LOGE(TAG,
                 "AFE/MultiNet 帧合同不一致，采样率=%d，通道=%d/%d，帧=%d/%d/%d，稳定错误码=VOICE_AFE_FRAME_MISMATCH",
                 sample_rate,
                 feed_channels,
                 fetch_channels,
                 s_afe_feed_chunk_samples,
                 s_afe_fetch_chunk_samples,
                 s_model_chunk_samples);
        deinitialize_afe();
        return ESP_ERR_INVALID_SIZE;
    }
    s_afe_feed_buffer = heap_caps_malloc(
        (size_t)s_afe_feed_chunk_samples * sizeof(s_afe_feed_buffer[0]),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_afe_feed_buffer == NULL)
    {
        ESP_LOGE(TAG,
                 "AFE feed PSRAM 暂存区分配失败，样本=%d，稳定错误码=VOICE_AFE_BUFFER_NO_MEM",
                 s_afe_feed_chunk_samples);
        deinitialize_afe();
        return ESP_ERR_NO_MEM;
    }
    if (s_afe->reset_buffer(s_afe_data) != 1)
    {
        ESP_LOGE(TAG,
                 "AFE 初始缓冲区重置失败，稳定错误码=VOICE_AFE_RESET_FAILED");
        deinitialize_afe();
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

static void deinitialize_afe(void)
{
    if (s_afe_data != NULL && s_afe != NULL)
    {
        s_afe->destroy(s_afe_data);
    }
    if (s_afe_feed_buffer != NULL)
    {
        heap_caps_free(s_afe_feed_buffer);
        s_afe_feed_buffer = NULL;
    }
    s_afe_data = NULL;
    s_afe = NULL;
    s_afe_feed_chunk_samples = 0;
    s_afe_fetch_chunk_samples = 0;
}

static esp_err_t register_commands(void)
{
    esp_err_t err = esp_mn_commands_alloc(s_multinet, s_model_data);
    if (err != ESP_OK)
    {
        return err;
    }
    for (size_t index = 0;
         index < sizeof(s_command_phrases) / sizeof(s_command_phrases[0]);
         ++index)
    {
        err = esp_mn_commands_add(s_command_phrases[index].command_id,
                                  s_command_phrases[index].phonemes);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "MultiNet 命令词格式校验失败，ID=%d，词条=%s，稳定错误码=VOICE_COMMAND_TABLE_INVALID",
                     s_command_phrases[index].command_id,
                     s_command_phrases[index].phonemes);
            return err;
        }
    }
    esp_mn_error_t *command_errors = esp_mn_commands_update();
    if (command_errors != NULL && command_errors->num > 0)
    {
        ESP_LOGE(TAG,
                 "MultiNet 命令表更新存在不可解析词条，数量=%d，稳定错误码=VOICE_COMMAND_TABLE_INVALID",
                 command_errors->num);
        return ESP_ERR_INVALID_ARG;
    }
    /* 启动期打印模型实际采用的 FST，真机日志据此排除“注册成功但未生效”。 */
    esp_mn_active_commands_print();
    return ESP_OK;
}

static void run_button_loop(void)
{
    bool boot_pressed = read_boot_pressed();
    bool long_press_evaluated = false;
    bool poll_boot_level = false;
    int64_t pressed_at_us = boot_pressed ? esp_timer_get_time() : 0;
    voice_session_t session = {0};
    const esp_err_t initial_rearm_error =
        key_rearm_boot_isr_for_next_level();
    if (initial_rearm_error != ESP_OK)
    {
        poll_boot_level = true;
        fail_session_for_rearm_error(&session, initial_rearm_error);
    }

    while (!atomic_load(&s_stop_requested))
    {
        if (atomic_load(&s_disable_requested))
        {
            if (session.active)
            {
                stop_and_finish_session(&session);
            }
            atomic_store(&s_ready, false);
            (void)key_unregister_boot_isr_callback();
            deinitialize_model();
            atomic_store(&s_disable_acknowledged, true);
            while (!atomic_load(&s_stop_requested))
            {
                (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            }
            break;
        }

        TickType_t wait_ticks = portMAX_DELAY;
        if (session.active &&
            !(session.audio_terminal && session.decoder_finalized))
        {
            wait_ticks = pdMS_TO_TICKS(VOICE_SERVICE_SESSION_POLL_MS);
        }
        else if (boot_pressed && !long_press_evaluated)
        {
            const int64_t elapsed_us = esp_timer_get_time() - pressed_at_us;
            const int64_t remaining_ms =
                (int64_t)VOICE_SERVICE_BOOT_LONG_PRESS_MS - elapsed_us / 1000LL;
            wait_ticks = remaining_ms > 0
                             ? pdMS_TO_TICKS((uint32_t)remaining_ms)
                             : 0U;
        }
        if (poll_boot_level &&
            (wait_ticks == portMAX_DELAY ||
             wait_ticks > pdMS_TO_TICKS(VOICE_SERVICE_SESSION_POLL_MS)))
        {
            wait_ticks = pdMS_TO_TICKS(VOICE_SERVICE_SESSION_POLL_MS);
        }

        const uint32_t notifications = ulTaskNotifyTake(pdTRUE, wait_ticks);
        if (notifications > 0U || poll_boot_level)
        {
            const int64_t edge_observed_at_us = esp_timer_get_time();
            if (notifications > 0U)
            {
                vTaskDelay(pdMS_TO_TICKS(VOICE_SERVICE_BOOT_DEBOUNCE_MS));
            }
            const bool now_pressed = read_boot_pressed();
            const esp_err_t rearm_error =
                key_rearm_boot_isr_for_next_level();
            poll_boot_level = rearm_error != ESP_OK;
            if (rearm_error != ESP_OK)
            {
                fail_session_for_rearm_error(&session, rearm_error);
            }
            if (now_pressed && !boot_pressed)
            {
                boot_pressed = true;
                long_press_evaluated = false;
                pressed_at_us = edge_observed_at_us;
            }
            else if (!now_pressed && boot_pressed)
            {
                boot_pressed = false;
                long_press_evaluated = false;
                pressed_at_us = 0;
                if (session.active)
                {
                    (void)power_service_notify_local_activity(0);
                }
            }
        }

        if (boot_pressed && !long_press_evaluated &&
            esp_timer_get_time() - pressed_at_us >=
                (int64_t)VOICE_SERVICE_BOOT_LONG_PRESS_MS * 1000LL &&
            !read_boot_pressed())
        {
            /* 500 ms 边界重新采样实体电平，避免消费释放通知前误启动。 */
            boot_pressed = false;
            long_press_evaluated = false;
            pressed_at_us = 0;
        }
        if (!poll_boot_level && !session.active &&
            voice_button_long_press_due(
                boot_pressed,
                long_press_evaluated,
                (uint64_t)((esp_timer_get_time() - pressed_at_us) / 1000LL)))
        {
            long_press_evaluated = true;
            (void)begin_session(&session);
        }
        if (session.active)
        {
            pump_session(&session);
            if (session.audio_terminal)
            {
                finish_session(&session);
            }
        }
    }

    if (session.active)
    {
        stop_and_finish_session(&session);
    }
}

static void stop_and_finish_session(voice_session_t *session)
{
    if (session == NULL || !session->active)
    {
        return;
    }
    session->failed = true;
    request_session_stop(session);
    const TickType_t started_at = xTaskGetTickCount();
    while (!session->audio_terminal &&
           xTaskGetTickCount() - started_at <
               pdMS_TO_TICKS(VOICE_SERVICE_AUDIO_STOP_WAIT_MS))
    {
        pump_session(session);
        vTaskDelay(1U);
    }
    finish_session(session);
}

static bool read_boot_pressed(void)
{
    legbot_key_state_t state = {0};
    return key_read_state(&state) == ESP_OK && state.boot_level == 0U;
}

static void fail_session_for_rearm_error(voice_session_t *session,
                                         esp_err_t error)
{
    ESP_LOGE(TAG,
             "BOOT0 中断动态重武装失败，已转入有限轮询，稳定错误码=VOICE_BOOT_ISR_REARM_FAILED，错误=0x%x",
             (unsigned)error);
    if (session != NULL && session->active)
    {
        session->failed = true;
        request_session_stop(session);
    }
}

static bool refresh_local_activity_before_session(void)
{
    const uint64_t requested_at_ms = monotonic_ms();
    const esp_err_t activity_error = power_service_notify_local_activity(0);
    if (activity_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "语音进入前本地活动通知失败，本次静默忽略，错误=0x%x",
                 (unsigned)activity_error);
        return false;
    }

    const TickType_t started_at = xTaskGetTickCount();
    do
    {
        if (!voice_service_is_ready())
        {
            ESP_LOGI(TAG,
                     "语音进入等待期间服务已停止或禁用，本次静默忽略");
            return false;
        }
        watch_state_snapshot_t snapshot = {0};
        if (watch_state_snapshot(&snapshot, 0) == ESP_OK)
        {
            if (snapshot.screen_state >= WATCH_SCREEN_STATE_COUNT)
            {
                return false;
            }
            if (snapshot.screen_state == WATCH_SCREEN_STATE_ON &&
                snapshot.last_local_activity_ms >= requested_at_ms)
            {
                return true;
            }
        }
        vTaskDelay(1U);
    } while (xTaskGetTickCount() - started_at <
             pdMS_TO_TICKS(VOICE_SERVICE_ACTIVITY_CONFIRM_TIMEOUT_MS));

    ESP_LOGW(TAG,
             "语音进入前本地活动未在时限内确认，本次静默忽略");
    return false;
}

static bool begin_session(voice_session_t *session)
{
    if (session == NULL || session->active || !voice_service_is_ready() ||
        s_afe == NULL || s_afe_data == NULL || s_multinet == NULL ||
        s_model_data == NULL ||
        !audio_service_voice_capture_ready())
    {
        return false;
    }

    watch_state_snapshot_t snapshot = {0};
    const esp_err_t snapshot_error = watch_state_snapshot(&snapshot, 0);
    const uint64_t entry_now_ms = monotonic_ms();
    if (!voice_control_entry_gate_allows(&snapshot,
                                         snapshot_error == ESP_OK,
                                         entry_now_ms))
    {
        log_gate_rejection("BOOT0 长按入口",
                           &snapshot,
                           snapshot_error,
                           entry_now_ms,
                           false);
        ESP_LOGI(TAG, "本次不亮屏并静默忽略");
        return false;
    }
    if (!refresh_local_activity_before_session())
    {
        return false;
    }
    if (!voice_service_is_ready())
    {
        ESP_LOGI(TAG,
                 "BOOT0 活动确认后服务已停止或禁用，本次静默忽略");
        return false;
    }
    memset(&snapshot, 0, sizeof(snapshot));
    const esp_err_t refreshed_snapshot_error =
        watch_state_snapshot(&snapshot, 0);
    const uint64_t refreshed_now_ms = monotonic_ms();
    if (!voice_control_gate_allows(&snapshot,
                                   refreshed_snapshot_error == ESP_OK,
                                   refreshed_now_ms))
    {
        log_gate_rejection("BOOT0 活动确认后",
                           &snapshot,
                           refreshed_snapshot_error,
                           refreshed_now_ms,
                           true);
        ESP_LOGI(TAG, "本次静默忽略");
        return false;
    }
    const esp_err_t lock_error = acquire_session_pm_locks();
    if (lock_error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "语音会话 PM lock 获取失败，稳定错误码=VOICE_PM_LOCK_FAILED，错误=0x%x",
                 (unsigned)lock_error);
        return false;
    }
    if (!voice_service_is_ready())
    {
        (void)release_session_pm_locks();
        ESP_LOGI(TAG,
                 "语音资源获取期间服务已停止或禁用，本次静默忽略");
        return false;
    }

    const uint32_t generation = next_generation();
    voice_runtime_resources_t resources = {0};
    if (!runtime_resources_meet_floors(&resources))
    {
        (void)release_session_pm_locks();
        ESP_LOGE(TAG,
                 "语音提示音前资源低于硬门槛，代次=%lu，DMA=%u/%u，PSRAM=%u/%u，栈=%u 字节，稳定错误码=VOICE_RUNTIME_RESOURCE_LOW",
                 (unsigned long)generation,
                 (unsigned)resources.dma_free,
                 (unsigned)resources.dma_largest,
                 (unsigned)resources.psram_free,
                 (unsigned)resources.psram_largest,
                 (unsigned)resources.stack_free);
        return false;
    }
    if (s_afe->reset_buffer(s_afe_data) != 1)
    {
        (void)release_session_pm_locks();
        ESP_LOGE(TAG,
                 "AFE 会话缓冲区重置失败，代次=%lu，稳定错误码=VOICE_AFE_RESET_FAILED",
                 (unsigned long)generation);
        return false;
    }
    s_multinet->clean(s_model_data);
    if (!voice_service_is_ready())
    {
        (void)release_session_pm_locks();
        ESP_LOGI(TAG,
                 "语音音频启动前服务已停止或禁用，本次静默忽略");
        return false;
    }
    const esp_err_t audio_error =
        audio_service_request_voice_capture_start(generation);
    if (audio_error != ESP_OK)
    {
        (void)release_session_pm_locks();
        ESP_LOGW(TAG,
                 "语音音频资源忙或未就绪，本次零控制，稳定错误码=VOICE_AUDIO_BUSY，错误=0x%x",
                 (unsigned)audio_error);
        return false;
    }

    *session = (voice_session_t){
        .generation = generation,
        .active = true,
    };
    atomic_store(&s_active_generation, generation);
    if (!voice_service_is_ready())
    {
        session->failed = true;
        request_session_stop(session);
        ESP_LOGI(TAG,
                 "语音音频启动并发遇到停止或禁用，已立即取消本代次");
    }
    log_memory("录音会话开始");
    return true;
}

static void request_session_stop(voice_session_t *session)
{
    if (session == NULL || !session->active || session->audio_terminal)
    {
        return;
    }
    const esp_err_t err = audio_service_request_voice_capture_stop(
        session->generation);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        session->failed = true;
        ESP_LOGW(TAG,
                 "语音音频停止请求失败，代次=%lu，错误=0x%x",
                 (unsigned long)session->generation,
                 (unsigned)err);
    }
}

static void pump_session(voice_session_t *session)
{
    if (session == NULL || !session->active)
    {
        return;
    }
    enforce_session_gate(session);
    enforce_session_resource_floors(session);
    process_audio_events(session);
    enforce_capture_window(session);
    process_pcm_frames(session, session->audio_terminal);
    if (session->audio_terminal && !session->decoder_finalized)
    {
        finalize_decoder(session);
    }
}

static void enforce_session_gate(voice_session_t *session)
{
    if (session == NULL || session->failed || session->audio_terminal)
    {
        return;
    }
    watch_state_snapshot_t snapshot = {0};
    const esp_err_t snapshot_error = watch_state_snapshot(&snapshot, 0);
    const uint64_t now_ms = monotonic_ms();
    if (voice_control_gate_allows(&snapshot,
                                  snapshot_error == ESP_OK,
                                  now_ms))
    {
        return;
    }
    session->failed = true;
    log_gate_rejection("会话期间",
                       &snapshot,
                       snapshot_error,
                       now_ms,
                       true);
    ESP_LOGW(TAG,
             "语音会话期间控制门禁失效，代次=%lu，本次粘滞为零 BLE 写入",
             (unsigned long)session->generation);
    request_session_stop(session);
}

static bool runtime_resources_meet_floors(
    voice_runtime_resources_t *resources)
{
    if (resources == NULL)
    {
        return false;
    }
    const uint32_t dma_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    *resources = (voice_runtime_resources_t){
        .dma_free = heap_caps_get_free_size(dma_caps),
        .dma_largest = heap_caps_get_largest_free_block(dma_caps),
        .psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
        .psram_largest =
            heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
        .stack_free = uxTaskGetStackHighWaterMark(NULL),
    };
    return resources->dma_free >= VOICE_SERVICE_DMA_FREE_FLOOR_BYTES &&
           resources->dma_largest >= VOICE_SERVICE_DMA_LARGEST_FLOOR_BYTES &&
           resources->psram_free >= VOICE_SERVICE_PSRAM_FLOOR_BYTES &&
           resources->psram_largest >= VOICE_SERVICE_PSRAM_FLOOR_BYTES &&
           resources->stack_free >= VOICE_SERVICE_STACK_FREE_FLOOR_BYTES;
}

static void enforce_session_resource_floors(voice_session_t *session)
{
    if (session == NULL || session->failed || session->audio_terminal ||
        !session->capture_started)
    {
        return;
    }
    voice_runtime_resources_t resources = {0};
    if (runtime_resources_meet_floors(&resources))
    {
        return;
    }
    session->failed = true;
    ESP_LOGE(TAG,
             "录音峰值资源低于硬门槛，代次=%lu，DMA=%u/%u，PSRAM=%u/%u，栈=%u 字节，稳定错误码=VOICE_RUNTIME_RESOURCE_LOW",
             (unsigned long)session->generation,
             (unsigned)resources.dma_free,
             (unsigned)resources.dma_largest,
             (unsigned)resources.psram_free,
             (unsigned)resources.psram_largest,
             (unsigned)resources.stack_free);
    request_session_stop(session);
}

static void enforce_capture_window(voice_session_t *session)
{
    if (session == NULL || session->failed || session->audio_terminal ||
        !session->capture_started)
    {
        return;
    }
    const uint64_t now_ms = monotonic_ms();
    const uint64_t elapsed_ms = now_ms - session->capture_started_at_ms;
    if (!voice_capture_auto_stop_due(session->capture_started,
                                     session->auto_stop_requested,
                                     elapsed_ms))
    {
        return;
    }
    /* 先锁存逻辑终点，确保轮询和重复通知都不会再次发送停止命令。 */
    session->auto_stop_requested = true;
    (void)power_service_notify_local_activity(0);
    ESP_LOGI(TAG,
             "语音三秒采集窗口已结束，代次=%lu，开始正常收尾",
             (unsigned long)session->generation);
    request_session_stop(session);
}

static void process_audio_events(voice_session_t *session)
{
    audio_service_voice_event_t event = {0};
    while (audio_service_receive_voice_event(&event, 0) == ESP_OK)
    {
        if (event.generation != session->generation)
        {
            continue;
        }
        switch (event.type)
        {
        case AUDIO_VOICE_EVENT_CUE_STARTED:
            ESP_LOGI(TAG,
                     "语音进入提示音已开始，代次=%lu",
                     (unsigned long)session->generation);
            break;
        case AUDIO_VOICE_EVENT_CAPTURE_STARTED:
            if (!session->capture_started)
            {
                session->capture_started = true;
                session->capture_started_at_ms = monotonic_ms();
            }
            ESP_LOGI(TAG,
                     "语音 ADC/RX 已开始，代次=%lu",
                     (unsigned long)session->generation);
            break;
        case AUDIO_VOICE_EVENT_CAPTURE_STOPPED:
            session->audio_terminal = true;
            break;
        case AUDIO_VOICE_EVENT_TIMED_OUT:
            session->audio_terminal = true;
            session->failed = true;
            ESP_LOGW(TAG,
                     "语音采集达到五秒上限，代次=%lu，本次零 BLE 写入",
                     (unsigned long)session->generation);
            break;
        case AUDIO_VOICE_EVENT_CANCELLED:
            session->audio_terminal = true;
            session->failed = true;
            break;
        case AUDIO_VOICE_EVENT_FAILED:
        default:
            session->audio_terminal = true;
            session->failed = true;
            ESP_LOGW(TAG,
                     "语音音频会话失败，代次=%lu，稳定错误码=%s",
                     (unsigned long)session->generation,
                     audio_service_result_code(event.result));
            break;
        }
    }
}

static void process_pcm_frames(voice_session_t *session, bool drain_all)
{
    if (session == NULL || !session->capture_started)
    {
        return;
    }
    int16_t samples[AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES / sizeof(int16_t)];
    do
    {
        size_t bytes_received = 0U;
        const esp_err_t err = audio_service_receive_voice_pcm(
            session->generation,
            samples,
            sizeof(samples),
            &bytes_received,
            0U);
        if (err != ESP_OK)
        {
            break;
        }
        if (bytes_received != sizeof(samples))
        {
            session->failed = true;
            ESP_LOGE(TAG,
                     "语音 PCM 帧不完整，代次=%lu，字节=%u",
                     (unsigned long)session->generation,
                     (unsigned)bytes_received);
            break;
        }
        update_pcm_metrics(session,
                           samples,
                           sizeof(samples) / sizeof(samples[0]));
        process_afe_samples(session,
                            samples,
                            sizeof(samples) / sizeof(samples[0]));
    } while (drain_all);
}

static void update_pcm_metrics(voice_session_t *session,
                               const int16_t *samples,
                               size_t sample_count)
{
    if (session == NULL || samples == NULL)
    {
        return;
    }
    for (size_t index = 0U; index < sample_count; ++index)
    {
        const int32_t sample = samples[index];
        const uint32_t amplitude =
            sample < 0 ? (uint32_t)(-sample) : (uint32_t)sample;
        session->pcm_square_sum += (uint64_t)((int64_t)sample * sample);
        ++session->pcm_sample_count;
        if (amplitude > session->pcm_peak)
        {
            session->pcm_peak = amplitude;
        }
        if (amplitude >= VOICE_SERVICE_PCM_CLIP_THRESHOLD)
        {
            ++session->pcm_clipped;
        }
    }
}

static void log_pcm_metrics(const voice_session_t *session)
{
    if (session == NULL)
    {
        return;
    }
    const uint32_t rms =
        session->pcm_sample_count == 0U
            ? 0U
            : (uint32_t)sqrt((double)session->pcm_square_sum /
                             (double)session->pcm_sample_count);
    ESP_LOGI(TAG,
             "语音 PCM 统计：代次=%lu，样本=%lu，RMS=%lu，峰值=%lu，削顶=%lu，AFE帧=%lu，模型帧=%lu，检测事件=%lu，空结果=%lu",
             (unsigned long)session->generation,
             (unsigned long)session->pcm_sample_count,
             (unsigned long)rms,
             (unsigned long)session->pcm_peak,
             (unsigned long)session->pcm_clipped,
             (unsigned long)session->afe_frames,
             (unsigned long)session->multinet_frames,
             (unsigned long)session->multinet_detected_events,
             (unsigned long)session->multinet_empty_results);
}

static void process_afe_samples(voice_session_t *session,
                                const int16_t *samples,
                                size_t sample_count)
{
    if (session == NULL || samples == NULL || session->failed ||
        session->command_locked || s_afe == NULL || s_afe_data == NULL ||
        s_afe_feed_chunk_samples <= 0 || s_afe_feed_buffer == NULL)
    {
        return;
    }
    while (sample_count > 0U && !session->failed &&
           !session->command_locked)
    {
        const size_t feed_samples = (size_t)s_afe_feed_chunk_samples;
        const size_t available =
            feed_samples - session->afe_feed_buffered_samples;
        const size_t copy_samples =
            sample_count < available ? sample_count : available;
        memcpy(&s_afe_feed_buffer[session->afe_feed_buffered_samples],
               samples,
               copy_samples * sizeof(samples[0]));
        session->afe_feed_buffered_samples += copy_samples;
        samples += copy_samples;
        sample_count -= copy_samples;
        if (session->afe_feed_buffered_samples == feed_samples &&
            !feed_afe_chunk(session))
        {
            return;
        }
    }
}

static bool feed_afe_chunk(voice_session_t *session)
{
    if (session == NULL || s_afe == NULL || s_afe_data == NULL ||
        s_afe_feed_chunk_samples <= 0 || s_afe_fetch_chunk_samples <= 0 ||
        s_afe_feed_buffer == NULL ||
        session->afe_feed_buffered_samples !=
            (size_t)s_afe_feed_chunk_samples)
    {
        return false;
    }
    const int feed_result =
        s_afe->feed(s_afe_data, s_afe_feed_buffer);
    if (feed_result < 0)
    {
        session->failed = true;
        ESP_LOGE(TAG,
                 "AFE feed 失败，代次=%lu，返回=%d，稳定错误码=VOICE_AFE_FEED_FAILED",
                 (unsigned long)session->generation,
                 feed_result);
        request_session_stop(session);
        return false;
    }
    session->afe_feed_buffered_samples = 0U;
    session->afe_fed_samples += (uint64_t)s_afe_feed_chunk_samples;
    while (!session->failed && !session->command_locked &&
           session->afe_fed_samples - session->afe_fetched_samples >=
               (uint64_t)s_afe_fetch_chunk_samples)
    {
        if (!fetch_afe_frame(session))
        {
            return false;
        }
        session->afe_fetched_samples +=
            (uint64_t)s_afe_fetch_chunk_samples;
    }
    return true;
}

static bool fetch_afe_frame(voice_session_t *session)
{
    afe_fetch_result_t *fetch_result = s_afe->fetch_with_delay(
        s_afe_data,
        pdMS_TO_TICKS(VOICE_SERVICE_AFE_FETCH_TIMEOUT_MS));
    if (fetch_result == NULL)
    {
        session->failed = true;
        ESP_LOGE(TAG,
                 "AFE fetch 未返回结果，代次=%lu，稳定错误码=VOICE_AFE_FETCH_TIMEOUT",
                 (unsigned long)session->generation);
        request_session_stop(session);
        return false;
    }
    if (fetch_result->ret_value != ESP_OK || fetch_result->data == NULL)
    {
        session->failed = true;
        ESP_LOGE(TAG,
                 "AFE fetch 失败，代次=%lu，返回=0x%x，稳定错误码=VOICE_AFE_FETCH_FAILED",
                 (unsigned long)session->generation,
                 (unsigned)fetch_result->ret_value);
        request_session_stop(session);
        return false;
    }
    const size_t expected_bytes =
        (size_t)s_afe_fetch_chunk_samples * sizeof(int16_t);
    if ((size_t)fetch_result->data_size != expected_bytes)
    {
        session->failed = true;
        ESP_LOGE(TAG,
                 "AFE fetch 帧长异常，代次=%lu，实际=%d，期望=%u，稳定错误码=VOICE_AFE_FRAME_MISMATCH",
                 (unsigned long)session->generation,
                 fetch_result->data_size,
                 (unsigned)expected_bytes);
        request_session_stop(session);
        return false;
    }
    ++session->afe_frames;
    process_model_frame(session, fetch_result->data);
    return !session->failed;
}

static void flush_afe_feed_buffer(voice_session_t *session)
{
    if (session == NULL || session->failed || session->command_locked ||
        session->afe_feed_buffered_samples == 0U ||
        s_afe_feed_chunk_samples <= 0)
    {
        return;
    }
    const size_t feed_samples = (size_t)s_afe_feed_chunk_samples;
    memset(&s_afe_feed_buffer[session->afe_feed_buffered_samples],
           0,
           (feed_samples - session->afe_feed_buffered_samples) *
               sizeof(s_afe_feed_buffer[0]));
    session->afe_feed_buffered_samples = feed_samples;
    (void)feed_afe_chunk(session);
}

static void process_model_frame(voice_session_t *session, int16_t *samples)
{
    if (session == NULL || samples == NULL || session->failed ||
        session->command_locked || s_multinet == NULL || s_model_data == NULL)
    {
        return;
    }
    ++session->multinet_frames;
    const esp_mn_state_t state = s_multinet->detect(s_model_data, samples);
    if (state == ESP_MN_STATE_TIMEOUT)
    {
        esp_mn_results_t *results = s_multinet->get_results(s_model_data);
        session->failed = true;
        ESP_LOGW(TAG,
                 "MultiNet 会话内部超时，代次=%lu，候选=%d，命令序列=%s，原始序列=%s，稳定错误码=VOICE_MULTINET_TIMEOUT",
                 (unsigned long)session->generation,
                 results != NULL ? results->num : 0,
                 results != NULL ? results->string : "<null>",
                 results != NULL ? results->raw_string : "<null>");
        request_session_stop(session);
        return;
    }
    if (state != ESP_MN_STATE_DETECTED)
    {
        return;
    }
    ++session->multinet_detected_events;
    esp_mn_results_t *results = s_multinet->get_results(s_model_data);
    if (results == NULL || results->num <= 0)
    {
        ++session->multinet_empty_results;
        ESP_LOGI(TAG,
                 "MultiNet 已触发但命令图无候选，代次=%lu，原始序列=%s，稳定错误码=VOICE_MULTINET_EMPTY_RESULT",
                 (unsigned long)session->generation,
                 results != NULL ? results->raw_string : "<null>");
        s_multinet->clean(s_model_data);
        return;
    }
    const float second_probability =
        results->num > 1 ? results->prob[1] : 0.0F;
    if (voice_control_candidate_accepts(results->command_id[0],
                                        results->prob[0],
                                        second_probability))
    {
        session->command_locked = true;
        session->command_id = results->command_id[0];
        ESP_LOGI(TAG,
                 "语音命令候选已锁存，代次=%lu，命令=%d，概率=%.3f，差值=%.3f",
                 (unsigned long)session->generation,
                 session->command_id,
                 (double)results->prob[0],
                 (double)(results->prob[0] - second_probability));
        return;
    }
    ESP_LOGI(TAG,
             "语音候选未超过 0.40 阈值，命令=%d，概率=%.3f，第二候选=%.3f",
             results->command_id[0],
             (double)results->prob[0],
             (double)second_probability);
    s_multinet->clean(s_model_data);
}

static void finalize_decoder(voice_session_t *session)
{
    if (session == NULL || session->decoder_finalized)
    {
        return;
    }
    process_pcm_frames(session, true);
    if (!session->failed && !session->command_locked &&
        s_model_chunk_samples > 0)
    {
        int16_t silence[AUDIO_SERVICE_VOICE_PCM_FRAME_BYTES / sizeof(int16_t)] = {0};
        const uint32_t frame_ms =
            (uint32_t)s_model_chunk_samples * 1000U /
            VOICE_SERVICE_SAMPLE_RATE_HZ;
        const uint32_t silence_frames =
            VOICE_SERVICE_DECODE_TAIL_SILENCE_MS / frame_ms;
        for (uint32_t index = 0;
             index < silence_frames && !session->command_locked;
             ++index)
        {
            process_afe_samples(session,
                                silence,
                                sizeof(silence) / sizeof(silence[0]));
        }
        flush_afe_feed_buffer(session);
    }
    session->decoder_finalized = true;
}

static void finish_session(voice_session_t *session)
{
    if (session == NULL || !session->active)
    {
        return;
    }
    if (!session->decoder_finalized)
    {
        finalize_decoder(session);
    }
    log_pcm_metrics(session);
    if (!session->failed && session->command_locked &&
        session->auto_stop_requested)
    {
        submit_locked_command(session);
    }
    else
    {
        ESP_LOGI(TAG,
                 "语音会话零控制结束，代次=%lu，失败=%u，候选=%u，三秒到时=%u",
                 (unsigned long)session->generation,
                 session->failed ? 1U : 0U,
                 session->command_locked ? 1U : 0U,
                 session->auto_stop_requested ? 1U : 0U);
    }
    atomic_store(&s_active_generation, 0U);
    (void)release_session_pm_locks();
    log_memory("录音会话结束");
    memset(session, 0, sizeof(*session));
}

static void submit_locked_command(const voice_session_t *session)
{
    watch_state_snapshot_t snapshot = {0};
    const esp_err_t snapshot_error = watch_state_snapshot(&snapshot, 0);
    const uint64_t now_ms = monotonic_ms();
    if (!voice_control_gate_allows(&snapshot,
                                   snapshot_error == ESP_OK,
                                   now_ms))
    {
        log_gate_rejection("自动提交前",
                           &snapshot,
                           snapshot_error,
                           now_ms,
                           true);
        ESP_LOGW(TAG,
                 "语音自动提交前门禁已失效，代次=%lu，本次零 BLE 写入",
                 (unsigned long)session->generation);
        return;
    }
    control_ui_model_t model = {0};
    if (control_ui_model_from_snapshot(&snapshot,
                                       true,
                                       now_ms,
                                       &model) != ESP_OK ||
        !model.controls_enabled)
    {
        return;
    }
    control_ui_action_t action = CONTROL_UI_ACTION_MOTOR;
    uint8_t target = 0U;
    bool should_submit = false;
    const esp_err_t mapping_error = voice_control_resolve_command(
        session->command_id,
        &snapshot,
        &model,
        &action,
        &target,
        &should_submit);
    if (mapping_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "语音命令映射被型号或状态拒绝，命令=%d，错误=0x%x",
                 session->command_id,
                 (unsigned)mapping_error);
        return;
    }
    if (!should_submit)
    {
        ESP_LOGI(TAG,
                 "语音命令目标已满足或达到边界，命令=%d，本次幂等零 BLE 写入",
                 session->command_id);
        return;
    }

    control_ui_intent_t converted = {0};
    if (control_ui_intent_from_action(action, target, &converted) != ESP_OK)
    {
        return;
    }
    const ble_control_intent_t ble_intent = {
        .kind = converted.kind,
        .target = converted.target,
    };
    const esp_err_t ble_error = ble_service_request_control(&ble_intent, 0);
    if (ble_error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "语音控制已进入 BLE owner，命令=%d，类型=%d，目标=%u",
                 session->command_id,
                 (int)ble_intent.kind,
                 (unsigned)ble_intent.target);
    }
    else
    {
        ESP_LOGW(TAG,
                 "语音控制未被 BLE owner 接受，命令=%d，错误=0x%x",
                 session->command_id,
                 (unsigned)ble_error);
    }
}

static void log_gate_rejection(const char *stage,
                               const watch_state_snapshot_t *snapshot,
                               esp_err_t snapshot_error,
                               uint64_t now_ms,
                               bool require_screen_on)
{
    if (snapshot == NULL || snapshot_error != ESP_OK)
    {
        ESP_LOGI(TAG,
                 "语音%s门禁拒绝：原因=状态快照读取失败，错误=0x%x",
                 stage,
                 (unsigned)snapshot_error);
        return;
    }

    const char *reason = NULL;
    if (snapshot->screen_state < WATCH_SCREEN_STATE_ON ||
        snapshot->screen_state >= WATCH_SCREEN_STATE_COUNT)
    {
        reason = "屏幕状态无效";
    }
    else if (require_screen_on &&
             snapshot->screen_state != WATCH_SCREEN_STATE_ON)
    {
        reason = "屏幕未点亮";
    }
    else if (snapshot->power_level == WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY)
    {
        reason = "严重低电";
    }
    else if (snapshot->selftest.state == SELFTEST_RUN_RUNNING)
    {
        reason = "自检执行中";
    }
    else if (snapshot->selftest.retry_active)
    {
        reason = "自检重试中";
    }

    control_ui_model_t model = {0};
    if (reason == NULL &&
        control_ui_model_from_snapshot(snapshot,
                                       true,
                                       now_ms,
                                       &model) == ESP_OK)
    {
        reason = control_block_reason_text(model.block_reason);
    }
    if (reason == NULL)
    {
        reason = "控制模型生成失败";
    }

    ESP_LOGI(TAG,
             "语音%s门禁拒绝：原因=%s，屏幕=%d，BLE=%u，GATT=%u，Notify=%u，数据=%u，新鲜=%u，控制在途=%u，租赁阶段=%d，租赁结果=%d，解锁会话=%u",
             stage,
             reason,
             (int)snapshot->screen_state,
             snapshot->ble_connected ? 1U : 0U,
             snapshot->gatt_ready ? 1U : 0U,
             snapshot->notify_ready ? 1U : 0U,
             snapshot->device_state_available && snapshot->dataReady ? 1U : 0U,
             snapshot->status_fresh ? 1U : 0U,
             snapshot->pending_control ? 1U : 0U,
             (int)snapshot->rental_phase,
             (int)snapshot->last_rental_result,
             snapshot->unlock_session_valid ? 1U : 0U);
}

static const char *control_block_reason_text(
    control_ui_block_reason_t reason)
{
    switch (reason)
    {
    case CONTROL_UI_BLOCK_NONE:
        return "控制门禁已开放";
    case CONTROL_UI_BLOCK_SNAPSHOT_FAILED:
        return "状态快照读取失败";
    case CONTROL_UI_BLOCK_UNBOUND:
        return "外骨骼未绑定";
    case CONTROL_UI_BLOCK_BLE_DISCONNECTED:
        return "BLE 未连接";
    case CONTROL_UI_BLOCK_GATT_NOT_READY:
        return "GATT 未就绪";
    case CONTROL_UI_BLOCK_NOTIFY_NOT_READY:
        return "Notify 未就绪";
    case CONTROL_UI_BLOCK_DATA_NOT_READY:
        return "外骨骼状态未就绪";
    case CONTROL_UI_BLOCK_STATUS_STALE:
        return "外骨骼状态已陈旧";
    case CONTROL_UI_BLOCK_LINK_NOT_READY:
        return "BLE 控制链路未就绪";
    case CONTROL_UI_BLOCK_PENDING:
        return "支付、解锁或控制在途";
    case CONTROL_UI_BLOCK_RETRY_EXHAUSTED:
        return "控制重试已耗尽";
    case CONTROL_UI_BLOCK_AUTH_REQUIRED:
        return "尚无有效解锁会话";
    case CONTROL_UI_BLOCK_RENTAL_UNPAID:
        return "租赁未支付";
    case CONTROL_UI_BLOCK_DEVICE_DISABLED:
        return "设备已禁用";
    case CONTROL_UI_BLOCK_UNKNOWN_DEVICE:
        return "设备未登记";
    case CONTROL_UI_BLOCK_CLOUD_UNAVAILABLE:
        return "云授权不可用";
    default:
        return "未知控制门禁";
    }
}

static esp_err_t acquire_session_pm_locks(void)
{
    if (s_no_light_sleep_lock == NULL || s_cpu_frequency_lock == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = esp_pm_lock_acquire(s_no_light_sleep_lock);
    if (err != ESP_OK)
    {
        return err;
    }
    s_no_light_sleep_acquired = true;
    err = esp_pm_lock_acquire(s_cpu_frequency_lock);
    if (err != ESP_OK)
    {
        (void)release_session_pm_locks();
    }
    else
    {
        s_cpu_frequency_acquired = true;
    }
    return err;
}

static esp_err_t release_session_pm_locks(void)
{
    esp_err_t first_error = release_pm_lock_with_retry(
        s_cpu_frequency_lock,
        &s_cpu_frequency_acquired,
        "ESP_PM_CPU_FREQ_MAX");
    const esp_err_t no_sleep_error = release_pm_lock_with_retry(
        s_no_light_sleep_lock,
        &s_no_light_sleep_acquired,
        "ESP_PM_NO_LIGHT_SLEEP");
    if (first_error == ESP_OK)
    {
        first_error = no_sleep_error;
    }
    return first_error;
}

static esp_err_t release_pm_lock_with_retry(esp_pm_lock_handle_t lock,
                                            bool *acquired,
                                            const char *name)
{
    if (acquired == NULL || !*acquired)
    {
        return ESP_OK;
    }
    if (lock == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = ESP_FAIL;
    for (uint32_t attempt = 0U;
         attempt < VOICE_SERVICE_PM_RELEASE_RETRY_COUNT;
         ++attempt)
    {
        err = esp_pm_lock_release(lock);
        if (err == ESP_OK)
        {
            *acquired = false;
            return ESP_OK;
        }
        vTaskDelay(1U);
    }
    ESP_LOGE(TAG,
             "%s 释放失败且仍保留持有标记，稳定错误码=VOICE_PM_LOCK_RELEASE_FAILED，错误=0x%x",
             name != NULL ? name : "PM lock",
             (unsigned)err);
    return err;
}

static uint32_t next_generation(void)
{
    uint32_t generation = 0U;
    do
    {
        generation = atomic_fetch_add(&s_next_generation, 1U) + 1U;
    } while (generation == 0U);
    return generation;
}

static uint64_t monotonic_ms(void)
{
    return (uint64_t)esp_timer_get_time() / 1000U;
}

static void log_memory(const char *stage)
{
    const uint32_t dma_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    ESP_LOGI(TAG,
             "语音资源%s：内部=%lu，DMA=%lu/%lu，PSRAM=%lu/%lu，栈高水位=%u 字节",
             stage,
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
             (unsigned long)heap_caps_get_free_size(dma_caps),
             (unsigned long)heap_caps_get_largest_free_block(dma_caps),
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
             (unsigned)uxTaskGetStackHighWaterMark(NULL));
}
