/**
 * @file     feedback_audio_bsp.c
 * @brief    木鱼音播放链的固件实现。
 * @details  SPIFFS 挂载与木鱼音资源加载、ES8311 复用共享 I²C access 初始化、
 *           播放链 BSP 安全序列（init → start → NS4150 使能（含启动稳压等待）→
 *           有界写入 → 播毕安全关断）与 PCM 数字增益缩放。反馈服务不得绕过
 *           BSP 直接操作 I2S/GPIO/I²C（AD-8/AD-10）；I²S0 为 BSP 独占权威通道。
 * @author   ZHC
 * @date     2026-09-23
 */

#include "feedback_audio.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "es8311_bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_manager.h"
#include "ns4150_bsp.h"

static const char *TAG = "SVC_FEEDBACK";

/** SPIFFS 挂载点；与 main/CMakeLists.txt 的 spiffs 分区镜像一一对应。 */
#define FEEDBACK_AUDIO_MOUNT_POINT "/spiffs"
/** SPIFFS 分区标签。 */
#define FEEDBACK_AUDIO_PARTITION_LABEL "spiffs"
/** 木鱼敲击音资源路径（16 kHz/16-bit/mono，沿 *_16k_mono.wav 约定）。 */
#define FEEDBACK_AUDIO_WAV_PATH "/spiffs/audio/woodfish_tap_16k_mono.wav"
/** 同时打开文件上限；播放链任一时刻只打开一个资源文件。 */
#define FEEDBACK_AUDIO_MAX_FILES 2U
/** PCM 静态缓冲容量；覆盖 16 kHz/16-bit/mono 约 512 ms 敲击音。 */
#define FEEDBACK_AUDIO_PCM_CAPACITY 16384U
/** 单次 es8311_bsp_write 的有界超时。 */
#define FEEDBACK_AUDIO_WRITE_TIMEOUT_MS 200U
/** RIFF/WAVE 头部固定长度。 */
#define FEEDBACK_AUDIO_WAV_HEADER_MAX 128U

/** 最近一次播放链失败的稳定错误分类。 */
static atomic_int s_last_error;
/** 最近一次播放链失败的 typed 通道错误域。 */
static atomic_int s_typed_error;
/** 播放链是否已完成初始化（挂载、资源与 codec 全部就绪）。 */
static bool s_audio_ready;
/** 木鱼音 PCM 源缓冲；初始化时一次加载，保持未缩放原样。 */
static uint8_t s_pcm[FEEDBACK_AUDIO_PCM_CAPACITY];
/** 单次播放用 PCM 工作缓冲；每次播放由源缓冲复制后再做数字增益。 */
static uint8_t s_pcm_play[FEEDBACK_AUDIO_PCM_CAPACITY];
/** 已加载 PCM 字节数；未加载时为 0。 */
static size_t s_pcm_size;

static void record_error(esp_err_t error);
static void record_typed_error(watch_feedback_error_t typed);
static watch_feedback_error_t es8311_failure_to_typed(void);
static esp_err_t ensure_spiffs_mounted(void);
static esp_err_t load_wav_resource(void);

esp_err_t ewf_feedback_audio_backend_init(void)
{
    if (s_audio_ready)
    {
        return ESP_OK;
    }
    const esp_err_t mount_error = ensure_spiffs_mounted();
    if (mount_error != ESP_OK)
    {
        record_error(mount_error);
        record_typed_error(WATCH_FEEDBACK_ERROR_IO_FAILED);
        return mount_error;
    }
    const esp_err_t load_error = load_wav_resource();
    if (load_error != ESP_OK)
    {
        record_error(load_error);
        record_typed_error(load_error == ESP_ERR_NOT_FOUND
                               ? WATCH_FEEDBACK_ERROR_RESOURCE_MISSING
                               : WATCH_FEEDBACK_ERROR_RESOURCE_CORRUPT);
        return load_error;
    }

    /* ES8311 控制走共享 I²C access（AD-10）；BSP 内部独占 I2S0 生命周期。 */
    legbot_bsp_i2c_access_t i2c_access = {0};
    esp_err_t error = i2c_manager_get_access(&i2c_access);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "播放链获取共享 I2C access 失败，错误=%s", esp_err_to_name(error));
        record_error(error);
        return error;
    }
    error = es8311_bsp_init(&i2c_access);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "ES8311 播放链初始化失败，稳定错误码=%s，错误=%s",
                 es8311_bsp_error_code(), esp_err_to_name(error));
        record_error(error);
        record_typed_error(es8311_failure_to_typed());
        return error;
    }
    /* 资源加载成功即可宣布就绪；单次播放时再按需启动 TX 链。 */
    s_audio_ready = true;
    ESP_LOGI(TAG, "木鱼音播放链已就绪：资源=%s，音量策略=PCM 数字增益",
             FEEDBACK_AUDIO_WAV_PATH);
    return ESP_OK;
}

esp_err_t ewf_feedback_audio_backend_play(uint8_t volume_percent)
{
    if (!s_audio_ready || !es8311_bsp_is_ready())
    {
        record_error(ESP_ERR_INVALID_STATE);
        record_typed_error(WATCH_FEEDBACK_ERROR_NOT_READY);
        return ESP_ERR_INVALID_STATE;
    }
    if (volume_percent == 0U)
    {
        /* 音量 0 等价静音：跳过播放链并确保功放安全关断，不产生可听输出。 */
        const esp_err_t off_error = ewf_feedback_audio_backend_ensure_pa_off();
        if (off_error != ESP_OK)
        {
            record_error(off_error);
            record_typed_error(WATCH_FEEDBACK_ERROR_PA_FAILED);
            return off_error;
        }
        return ESP_OK;
    }

    esp_err_t error = es8311_bsp_start();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "ES8311/I2S 启动失败，稳定错误码=%s，错误=%s",
                 es8311_bsp_error_code(), esp_err_to_name(error));
        (void)ns4150_bsp_safe_off();
        record_error(error);
        record_typed_error(es8311_failure_to_typed());
        return error;
    }
    /* PA 使能必须在 codec/I2S 就绪之后（BSP 安全合同），并预留数据手册稳压窗口。 */
    error = ns4150_bsp_enable();
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "功放使能失败，稳定错误码=%s，错误=%s",
                 ns4150_bsp_error_code(error), esp_err_to_name(error));
        (void)ns4150_bsp_safe_off();
        /* PA 失败时停掉已启动的 codec/I2S，避免 TX 链悬挂。 */
        (void)es8311_bsp_stop();
        record_error(error);
        record_typed_error(WATCH_FEEDBACK_ERROR_PA_FAILED);
        return error;
    }
    vTaskDelay(pdMS_TO_TICKS(NS4150_BSP_STARTUP_SETTLE_MS));

    /* PCM 数字增益缩放（选择依据：host 可测、纯软件，不动自检固定 dB 合同）。
     * 必须在工作缓冲上缩放，禁止原地改写源缓冲，否则非 100 音量会指数衰减。 */
    if ((s_pcm_size % sizeof(int16_t)) != 0U)
    {
        ESP_LOGE(TAG, "木鱼音 PCM 字节数非偶数：size=%u", (unsigned)s_pcm_size);
        (void)ns4150_bsp_safe_off();
        (void)es8311_bsp_stop();
        record_error(ESP_ERR_INVALID_STATE);
        record_typed_error(WATCH_FEEDBACK_ERROR_RESOURCE_MISSING);
        return ESP_ERR_INVALID_STATE;
    }
    memcpy(s_pcm_play, s_pcm, s_pcm_size);
    const size_t sample_count = s_pcm_size / sizeof(int16_t);
    int16_t *samples = (int16_t *)s_pcm_play;
    for (size_t index = 0; index < sample_count; ++index)
    {
        int32_t scaled = ((int32_t)samples[index] * (int32_t)volume_percent) / 100;
        samples[index] = (int16_t)scaled;
    }

    size_t bytes_written = 0U;
    error = es8311_bsp_write(s_pcm_play,
                             s_pcm_size,
                             &bytes_written,
                             FEEDBACK_AUDIO_WRITE_TIMEOUT_MS);
    /* 播毕（无论成败）立即安全关断功放：静音与故障时关断，不保持常开。 */
    const esp_err_t off_error = ns4150_bsp_safe_off();
    if (off_error != ESP_OK)
    {
        ESP_LOGE(TAG, "功放播毕安全关断失败，稳定错误码=%s，错误=%s",
                 ns4150_bsp_error_code(off_error), esp_err_to_name(off_error));
    }
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "木鱼音写入失败，稳定错误码=%s，错误=%s",
                 es8311_bsp_error_code(), esp_err_to_name(error));
        record_error(error);
        record_typed_error(es8311_failure_to_typed());
        return error;
    }
    if (bytes_written != s_pcm_size)
    {
        ESP_LOGE(TAG, "木鱼音写入不完整：期望=%u，实际=%u",
                 (unsigned)s_pcm_size, (unsigned)bytes_written);
        record_error(ESP_FAIL);
        record_typed_error(WATCH_FEEDBACK_ERROR_I2S_FAILED);
        return ESP_FAIL;
    }
    record_error(ESP_OK);
    record_typed_error(WATCH_FEEDBACK_ERROR_NONE);
    return ESP_OK;
}

esp_err_t ewf_feedback_audio_backend_ensure_pa_off(void)
{
    return ns4150_bsp_safe_off();
}

esp_err_t ewf_feedback_audio_backend_last_error(void)
{
    return (esp_err_t)atomic_load(&s_last_error);
}

watch_feedback_error_t ewf_feedback_audio_backend_typed_error(void)
{
    return (watch_feedback_error_t)atomic_load(&s_typed_error);
}

static void record_error(esp_err_t error)
{
    atomic_store(&s_last_error, (int)error);
}

static void record_typed_error(watch_feedback_error_t typed)
{
    atomic_store(&s_typed_error, (int)typed);
}

static watch_feedback_error_t es8311_failure_to_typed(void)
{
    switch (es8311_bsp_last_failure())
    {
    case ES8311_BSP_FAILURE_CONTROL:
    case ES8311_BSP_FAILURE_CODEC:
        return WATCH_FEEDBACK_ERROR_CODEC_FAILED;
    case ES8311_BSP_FAILURE_I2S:
    case ES8311_BSP_FAILURE_RESAMPLER:
    default:
        return WATCH_FEEDBACK_ERROR_I2S_FAILED;
    }
}

static esp_err_t ensure_spiffs_mounted(void)
{
    if (esp_spiffs_mounted(FEEDBACK_AUDIO_PARTITION_LABEL))
    {
        return ESP_OK;
    }
    const esp_vfs_spiffs_conf_t conf = {
        .base_path = FEEDBACK_AUDIO_MOUNT_POINT,
        .partition_label = FEEDBACK_AUDIO_PARTITION_LABEL,
        .max_files = FEEDBACK_AUDIO_MAX_FILES,
        .format_if_mount_failed = false,
    };
    /* 挂载失败不做隐式格式化：格式化会掩盖介质故障，交由故障事实呈现。 */
    const esp_err_t error = esp_vfs_spiffs_register(&conf);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS 挂载失败，错误=%s", esp_err_to_name(error));
        return error;
    }
    return ESP_OK;
}

static esp_err_t load_wav_resource(void)
{
    FILE *file = fopen(FEEDBACK_AUDIO_WAV_PATH, "rb");
    if (file == NULL)
    {
        ESP_LOGE(TAG, "木鱼音资源缺失：%s", FEEDBACK_AUDIO_WAV_PATH);
        return ESP_ERR_NOT_FOUND;
    }

    uint8_t header[FEEDBACK_AUDIO_WAV_HEADER_MAX];
    const size_t header_read = fread(header, 1U, sizeof(header), file);
    /* 逐字段解析 RIFF/WAVE：只接受 16 kHz/16-bit/mono PCM（AUDIO_BSP 合同）。 */
    if (header_read < 44U || memcmp(header, "RIFF", 4U) != 0 ||
        memcmp(header + 8U, "WAVE", 4U) != 0 || memcmp(header + 12U, "fmt ", 4U) != 0)
    {
        ESP_LOGE(TAG, "木鱼音资源头部损坏：%s", FEEDBACK_AUDIO_WAV_PATH);
        (void)fclose(file);
        return ESP_ERR_INVALID_STATE;
    }
    const uint16_t audio_format = (uint16_t)header[20U] | ((uint16_t)header[21U] << 8);
    const uint16_t channels = (uint16_t)header[22U] | ((uint16_t)header[23U] << 8);
    const uint32_t sample_rate = (uint32_t)header[24U] |
                                 ((uint32_t)header[25U] << 8) |
                                 ((uint32_t)header[26U] << 16) |
                                 ((uint32_t)header[27U] << 24);
    const uint16_t bits = (uint16_t)header[34U] | ((uint16_t)header[35U] << 8);
    if (audio_format != 1U || channels != 1U ||
        sample_rate != AUDIO_BSP_SAMPLE_RATE_HZ || bits != AUDIO_BSP_BITS_PER_SAMPLE)
    {
        ESP_LOGE(TAG,
                 "木鱼音资源参数不符合同：format=%u，channels=%u，rate=%lu，bits=%u",
                 (unsigned)audio_format, (unsigned)channels,
                 (unsigned long)sample_rate, (unsigned)bits);
        (void)fclose(file);
        return ESP_ERR_INVALID_STATE;
    }
    size_t data_offset = 0U;
    for (size_t offset = 36U; offset + 8U <= header_read; offset += 4U)
    {
        if (memcmp(header + offset, "data", 4U) == 0)
        {
            data_offset = offset + 8U;
            break;
        }
    }
    if (data_offset == 0U)
    {
        ESP_LOGE(TAG, "木鱼音资源缺少 data 块：%s", FEEDBACK_AUDIO_WAV_PATH);
        (void)fclose(file);
        return ESP_ERR_INVALID_STATE;
    }
    if (fseek(file, (long)data_offset, SEEK_SET) != 0)
    {
        ESP_LOGE(TAG, "木鱼音资源 data 块定位失败：%s", FEEDBACK_AUDIO_WAV_PATH);
        (void)fclose(file);
        return ESP_ERR_INVALID_STATE;
    }
    s_pcm_size = fread(s_pcm, 1U, FEEDBACK_AUDIO_PCM_CAPACITY, file);
    const bool read_error = ferror(file) != 0;
    (void)fclose(file);
    if (read_error || s_pcm_size == 0U || (s_pcm_size % sizeof(int16_t)) != 0U)
    {
        ESP_LOGE(TAG, "木鱼音资源读取失败：%s", FEEDBACK_AUDIO_WAV_PATH);
        s_pcm_size = 0U;
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}
