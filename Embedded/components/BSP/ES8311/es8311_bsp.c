/**
 * @file     es8311_bsp.c
 * @brief    ES8311 与 I2S0 音频输入输出 BSP 实现
 * @details  使用 esp_codec_dev 1.5.11 的 ES8311 寄存时序、自定义 manager 互斥 control interface 和 IDF v5.5.4 I2S channel API。
 * @author   ZHC
 * @date     2026-07-22
 */

#include "es8311_bsp.h"
#include "es8311_pcm_transport_internal.h"

#include <string.h>

#include "audio_codec_ctrl_if.h"
#include "audio_codec_if.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "esp_codec_dev_types.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "es8311_codec.h"
#include "sdkconfig.h"
#include "soc/soc_caps.h"

#if !CONFIG_IDF_TARGET_ESP32S3
#error "ES8311 BSP requires CONFIG_IDF_TARGET_ESP32S3."
#endif

#if SOC_I2S_NUM < 1
#error "ES8311 BSP requires at least one I2S peripheral."
#endif

static const char *TAG = "BSP_ES8311";

/** ES8311 control interface 的单次 I2C 等待上限。
 *  必须大于 CW2015 在最差路径（两次 VERSION 读取 + 地址探测）的互斥锁持有时间，
 *  避免与 power_task 并发初始化时因等待 I2C 互斥锁超时导致控制界面打开失败。 */
#define ES8311_BSP_I2C_TIMEOUT_MS 150
/** 采集链单批单声道输出样本数，与上层 32 ms PCM 帧一致。 */
#define ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES 512U
/** BSP 接受的最小单声道输出样本数。 */
#define ES8311_BSP_CAPTURE_MIN_OUTPUT_SAMPLES 16U
/** ADC 启动瞬态排空的最长等待时间。 */
#define ES8311_BSP_CAPTURE_WARMUP_TIMEOUT_MS 250U
/** ADC 启动瞬态单次读取的物理输入样本数。 */
#define ES8311_BSP_CAPTURE_WARMUP_BATCH_SAMPLES 96U
/** ES8311 ADC 控制寄存器地址。 */
#define ES8311_BSP_ADC_CONTROL_REGISTER 0x16
/** 标准音频时钟下需要保持的 ADC 滤波计数器同步位。 */
#define ES8311_BSP_ADC_STANDARD_CLOCK_SYNC_BIT 0x20
/** 42 dB ADC_SCALE 对应的寄存器字段值。 */
#define ES8311_BSP_ADC_SCALE_42_DB_VALUE 0x07

typedef struct
{
    audio_codec_ctrl_if_t base;     /**< esp_codec_dev control interface 基类。 */
    legbot_bsp_i2c_access_t access; /**< 注入的 manager-owned bus 与短互斥。 */
    i2c_master_dev_handle_t device; /**< 挂载在 manager-owned bus 上的 ES8311。 */
    bool is_open;                   /**< control interface 是否已经打开。 */
} es8311_bsp_ctrl_t;

/** 本项目自定义的 ES8311 control interface 实例。 */
static es8311_bsp_ctrl_t s_ctrl;
/** esp_codec_dev 创建的 ES8311 codec interface。 */
static const audio_codec_if_t *s_codec;
/** BSP 独占的 I2S0 TX channel。 */
static i2s_chan_handle_t s_tx_channel;
/** BSP 独占的 I2S0 RX channel。 */
static i2s_chan_handle_t s_rx_channel;
/** PSRAM 中由播放交错与双时隙采集互斥复用的工作区。 */
static int16_t *s_stereo_interleaved_samples;
/** I2S0 TX channel 是否处于 RUNNING 状态。 */
static bool s_tx_enabled;
/** I2S0 RX channel 是否处于 RUNNING 状态。 */
static bool s_rx_enabled;
/** 提示音已静音并停止 TX、codec 模拟链仍保持启用，等待切换到采集。 */
static bool s_capture_warm_pending;
/** codec 与 I2S 是否完整就绪。 */
static bool s_ready;
/** 最近一次操作的稳定驱动失败域。 */
static es8311_bsp_failure_t s_last_failure;
static int ctrl_open(const audio_codec_ctrl_if_t *ctrl, void *cfg, int cfg_size);
static bool ctrl_is_open(const audio_codec_ctrl_if_t *ctrl);
static int ctrl_read_reg(const audio_codec_ctrl_if_t *ctrl,
                         int reg,
                         int reg_len,
                         void *data,
                         int data_len);
static int ctrl_write_reg(const audio_codec_ctrl_if_t *ctrl,
                          int reg,
                          int reg_len,
                          void *data,
                          int data_len);
static int ctrl_close(const audio_codec_ctrl_if_t *ctrl);
static bool has_partial_state(void);
static esp_err_t discard_capture_startup_samples(size_t output_samples,
                                                 bool warm_transition);
static esp_err_t init_i2s_channel(const legbot_bsp_resource_t *resource);
static int configure_capture_gain(void);
static esp_err_t codec_error_to_esp(int error);

const legbot_bsp_resource_t *es8311_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_ES8311);
}

esp_err_t es8311_bsp_init(const legbot_bsp_i2c_access_t *access)
{
    if (s_ready)
    {
        return ESP_OK;
    }
    if (access == NULL || access->bus_handle == NULL ||
        access->acquire == NULL || access->release == NULL)
    {
        s_last_failure = ES8311_BSP_FAILURE_CONTROL;
        return ESP_ERR_INVALID_ARG;
    }
    if (has_partial_state())
    {
        esp_err_t cleanup_error = es8311_bsp_deinit();
        if (cleanup_error != ESP_OK || has_partial_state())
        {
            return cleanup_error != ESP_OK ? cleanup_error : ESP_ERR_INVALID_STATE;
        }
    }
    const legbot_bsp_resource_t *resource = es8311_bsp_resource();
    if (resource == NULL || resource->i2s_port != LEGBOT_BSP_ES8311_I2S_PORT)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_STATE;
    }

    memset(&s_ctrl, 0, sizeof(s_ctrl));
    s_ctrl.base.open = ctrl_open;
    s_ctrl.base.is_open = ctrl_is_open;
    s_ctrl.base.read_reg = ctrl_read_reg;
    s_ctrl.base.write_reg = ctrl_write_reg;
    s_ctrl.base.close = ctrl_close;
    s_ctrl.access = *access;

    int codec_error = s_ctrl.base.open(&s_ctrl.base, NULL, 0);
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        s_last_failure = ES8311_BSP_FAILURE_CONTROL;
        return codec_error_to_esp(codec_error);
    }

    esp_err_t err = init_i2s_channel(resource);
    if (err != ESP_OK)
    {
        (void)es8311_bsp_deinit();
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return err;
    }

    es8311_codec_cfg_t codec_config = {
        .ctrl_if = &s_ctrl.base,
        .gpio_if = NULL,
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .pa_pin = -1,
        .pa_reverted = false,
        .master_mode = false,
        .use_mclk = true,
        .digital_mic = false,
        .invert_mclk = false,
        .invert_sclk = false,
        .mclk_div = AUDIO_BSP_MCLK_MULTIPLE,
    };
    s_codec = es8311_codec_new(&codec_config);
    if (s_codec == NULL)
    {
        (void)es8311_bsp_deinit();
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return ESP_ERR_NOT_FOUND;
    }

    esp_codec_dev_sample_info_t sample_info = {
        .bits_per_sample = AUDIO_BSP_BITS_PER_SAMPLE,
        .channel = ES8311_PCM_PHYSICAL_SLOT_COUNT,
        .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0) |
                        ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1),
        .sample_rate = AUDIO_BSP_SAMPLE_RATE_HZ,
        .mclk_multiple = AUDIO_BSP_MCLK_MULTIPLE,
    };
    codec_error = s_codec->set_fs(s_codec, &sample_info);
    codec_error |= s_codec->set_vol(s_codec, ES8311_BSP_OUTPUT_VOLUME_DB);
    codec_error |= configure_capture_gain();
    codec_error |= s_codec->mute(s_codec, true);
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        err = codec_error_to_esp(codec_error);
        (void)es8311_bsp_deinit();
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return err;
    }

    const size_t capture_input_samples =
        ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES *
        ES8311_PCM_PHYSICAL_SLOT_COUNT;
    s_stereo_interleaved_samples = heap_caps_malloc(
        capture_input_samples * sizeof(int16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_stereo_interleaved_samples == NULL)
    {
        (void)es8311_bsp_deinit();
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_NO_MEM;
    }

    s_ready = true;
    s_last_failure = ES8311_BSP_FAILURE_NONE;
    ESP_LOGI(TAG,
             "ES8311 已按官方 slave、MCLK=384fs、%ukHz、16-bit/stereo/Philips I2S 采集并抽取左时隙，输出=-3 dB，ADC_SCALE=%.0f dB，ADC_SYNC=标准时钟",
             (unsigned)(ES8311_BSP_CAPTURE_SAMPLE_RATE_HZ / 1000U),
             (double)ES8311_BSP_INPUT_GAIN_DB);
    return ESP_OK;
}

esp_err_t es8311_bsp_start(void)
{
    if (!s_ready || s_tx_channel == NULL || s_codec == NULL || s_rx_enabled ||
        s_capture_warm_pending)
    {
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return ESP_ERR_INVALID_STATE;
    }
    if (!s_tx_enabled)
    {
        esp_err_t err = i2s_channel_enable(s_tx_channel);
        if (err != ESP_OK)
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return err;
        }
        s_tx_enabled = true;
    }

    int codec_error = s_codec->enable(s_codec, true);
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        esp_err_t disable_error = i2s_channel_disable(s_tx_channel);
        if (disable_error == ESP_OK)
        {
            s_tx_enabled = false;
        }
        else
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return disable_error;
        }
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return codec_error_to_esp(codec_error);
    }
    /*
     * esp_codec_dev 1.5.11 的 ES8311 suspend 会把 DAC 音量寄存器清零，
     * 因此每次复用持久 codec 时都必须在外部功放开启前恢复目标音量。
     */
    codec_error = s_codec->set_vol(s_codec, ES8311_BSP_OUTPUT_VOLUME_DB);
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        const int stop_error = s_codec->enable(s_codec, false);
        const esp_err_t disable_error = i2s_channel_disable(s_tx_channel);
        if (disable_error == ESP_OK)
        {
            s_tx_enabled = false;
        }
        else
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return disable_error;
        }
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return codec_error_to_esp(
            stop_error != ESP_CODEC_DEV_OK ? stop_error : codec_error);
    }
    s_last_failure = ES8311_BSP_FAILURE_NONE;
    return ESP_OK;
}

esp_err_t es8311_bsp_stop(void)
{
    if (s_rx_enabled)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t first_error = ESP_OK;
    if (s_codec != NULL)
    {
        int codec_error = s_codec->enable(s_codec, false);
        if (codec_error != ESP_CODEC_DEV_OK)
        {
            first_error = codec_error_to_esp(codec_error);
            s_last_failure = ES8311_BSP_FAILURE_CODEC;
        }
        else
        {
            s_capture_warm_pending = false;
        }
    }
    if (s_tx_enabled && s_tx_channel != NULL)
    {
        esp_err_t err = i2s_channel_disable(s_tx_channel);
        if (err == ESP_OK)
        {
            s_tx_enabled = false;
        }
        else if (first_error == ESP_OK)
        {
            first_error = err;
            s_last_failure = ES8311_BSP_FAILURE_I2S;
        }
    }
    if (first_error == ESP_OK)
    {
        s_last_failure = ES8311_BSP_FAILURE_NONE;
    }
    return first_error;
}

esp_err_t es8311_bsp_write(const void *data,
                           size_t size,
                           size_t *bytes_written,
                           uint32_t timeout_ms)
{
    if (!s_ready || !s_tx_enabled || s_tx_channel == NULL ||
        s_stereo_interleaved_samples == NULL)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || size == 0U || bytes_written == NULL ||
        timeout_ms == 0U || size % sizeof(int16_t) != 0U)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_ARG;
    }

    *bytes_written = 0U;
    const int16_t *mono_samples = data;
    const size_t mono_sample_count = size / sizeof(int16_t);
    size_t mono_sample_offset = 0U;
    while (mono_sample_offset < mono_sample_count)
    {
        const size_t remaining_samples =
            mono_sample_count - mono_sample_offset;
        const size_t batch_samples =
            remaining_samples < ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES
                ? remaining_samples
                : ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES;
        const size_t converted = es8311_pcm_interleave_mono_left(
            &mono_samples[mono_sample_offset],
            batch_samples,
            s_stereo_interleaved_samples,
            ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES *
                ES8311_PCM_PHYSICAL_SLOT_COUNT);
        if (converted != batch_samples)
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return ESP_ERR_INVALID_SIZE;
        }

        const size_t physical_bytes =
            batch_samples * ES8311_PCM_STEREO_FRAME_BYTES;
        size_t physical_bytes_written = 0U;
        const esp_err_t err = i2s_channel_write(
            s_tx_channel,
            s_stereo_interleaved_samples,
            physical_bytes,
            &physical_bytes_written,
            timeout_ms);
        const size_t completed_mono_bytes =
            es8311_pcm_complete_mono_bytes(physical_bytes_written);
        *bytes_written =
            mono_sample_offset * sizeof(int16_t) + completed_mono_bytes;
        if (physical_bytes_written > physical_bytes ||
            physical_bytes_written % ES8311_PCM_STEREO_FRAME_BYTES != 0U)
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return ESP_ERR_INVALID_SIZE;
        }
        if (err != ESP_OK)
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return err;
        }
        if (physical_bytes_written != physical_bytes)
        {
            s_last_failure = ES8311_BSP_FAILURE_I2S;
            return ESP_ERR_INVALID_SIZE;
        }
        mono_sample_offset += batch_samples;
    }

    *bytes_written = size;
    s_last_failure = ES8311_BSP_FAILURE_NONE;
    return ESP_OK;
}

esp_err_t es8311_bsp_prepare_capture_after_playback(void)
{
    if (!s_ready || s_codec == NULL || s_tx_channel == NULL ||
        !s_tx_enabled || s_rx_enabled || s_capture_warm_pending)
    {
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return ESP_ERR_INVALID_STATE;
    }

    const int codec_error = s_codec->mute(s_codec, true);
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return codec_error_to_esp(codec_error);
    }
    const esp_err_t err = i2s_channel_disable(s_tx_channel);
    if (err != ESP_OK)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return err;
    }
    s_tx_enabled = false;
    s_capture_warm_pending = true;
    s_last_failure = ES8311_BSP_FAILURE_NONE;
    return ESP_OK;
}

esp_err_t es8311_bsp_capture_start(void)
{
    if (!s_ready || s_rx_channel == NULL || s_codec == NULL ||
        s_tx_enabled || s_rx_enabled || s_stereo_interleaved_samples == NULL)
    {
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return ESP_ERR_INVALID_STATE;
    }
    const bool warm_transition = s_capture_warm_pending;

    /* ESP32-S3 全双工对由先初始化的 TX 提供共享 BCLK/WS，录音也必须先启动 TX 时钟。 */
    esp_err_t err = i2s_channel_enable(s_tx_channel);
    if (err != ESP_OK)
    {
        if (warm_transition)
        {
            (void)es8311_bsp_stop();
        }
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return err;
    }
    s_tx_enabled = true;

    err = i2s_channel_enable(s_rx_channel);
    if (err != ESP_OK)
    {
        const esp_err_t disable_error = i2s_channel_disable(s_tx_channel);
        if (disable_error == ESP_OK)
        {
            s_tx_enabled = false;
        }
        if (warm_transition)
        {
            (void)es8311_bsp_stop();
        }
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        if (disable_error != ESP_OK)
        {
            return disable_error;
        }
        return err;
    }
    s_rx_enabled = true;

    int codec_error = warm_transition
                          ? ESP_CODEC_DEV_OK
                          : s_codec->enable(s_codec, true);
    if (codec_error == ESP_CODEC_DEV_OK)
    {
        /* BOTH 模式 enable 会自动解除 DAC 静音，采集阶段立即恢复静音。 */
        codec_error = s_codec->mute(s_codec, true);
    }
    if (codec_error == ESP_CODEC_DEV_OK)
    {
        codec_error = configure_capture_gain();
    }
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        (void)s_codec->enable(s_codec, false);
        const esp_err_t cleanup_error = es8311_bsp_capture_stop();
        s_last_failure = ES8311_BSP_FAILURE_CODEC;
        return cleanup_error != ESP_OK
                   ? cleanup_error
                   : codec_error_to_esp(codec_error);
    }
    s_capture_warm_pending = false;

    err = discard_capture_startup_samples(
        warm_transition
            ? ES8311_BSP_WARM_CAPTURE_DISCARD_OUTPUT_SAMPLES
            : ES8311_BSP_CAPTURE_WARMUP_OUTPUT_SAMPLES,
        warm_transition);
    if (err != ESP_OK)
    {
        const esp_err_t cleanup_error = es8311_bsp_capture_stop();
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return cleanup_error != ESP_OK ? cleanup_error : err;
    }
    s_last_failure = ES8311_BSP_FAILURE_NONE;
    return ESP_OK;
}

esp_err_t es8311_bsp_read(void *data,
                          size_t size,
                          size_t *bytes_read,
                          uint32_t timeout_ms)
{
    if (!s_ready || !s_rx_enabled || s_rx_channel == NULL)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_STATE;
    }
    if (data == NULL || size == 0 || bytes_read == NULL || timeout_ms == 0 ||
        s_stereo_interleaved_samples == NULL ||
        size % sizeof(int16_t) != 0U ||
        size / sizeof(int16_t) < ES8311_BSP_CAPTURE_MIN_OUTPUT_SAMPLES)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_ARG;
    }
    *bytes_read = 0U;
    int16_t *output_samples = data;
    const int64_t deadline_us =
        esp_timer_get_time() + (int64_t)timeout_ms * 1000LL;

    while (*bytes_read < size)
    {
        const size_t remaining_output_samples =
            (size - *bytes_read) / sizeof(int16_t);
        const size_t batch_output_samples =
            remaining_output_samples <
                    ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES
                ? remaining_output_samples
                : ES8311_BSP_CAPTURE_OUTPUT_BATCH_SAMPLES;
        const size_t batch_input_samples =
            batch_output_samples * ES8311_PCM_PHYSICAL_SLOT_COUNT;
        const size_t batch_input_bytes =
            batch_input_samples * sizeof(int16_t);
        size_t raw_bytes_read = 0U;
        while (raw_bytes_read < batch_input_bytes)
        {
            const int64_t remaining_us = deadline_us - esp_timer_get_time();
            if (remaining_us <= 0LL)
            {
                s_last_failure = ES8311_BSP_FAILURE_I2S;
                return ESP_ERR_TIMEOUT;
            }
            const uint32_t remaining_ms =
                (uint32_t)((remaining_us + 999LL) / 1000LL);
            size_t chunk_bytes = 0U;
            const esp_err_t err = i2s_channel_read(
                s_rx_channel,
                (uint8_t *)s_stereo_interleaved_samples + raw_bytes_read,
                batch_input_bytes - raw_bytes_read,
                &chunk_bytes,
                remaining_ms);
            if (err != ESP_OK || chunk_bytes == 0U ||
                chunk_bytes > batch_input_bytes - raw_bytes_read)
            {
                s_last_failure = ES8311_BSP_FAILURE_I2S;
                return err != ESP_OK ? err : ESP_ERR_INVALID_SIZE;
            }
            raw_bytes_read += chunk_bytes;
        }

        const size_t output_offset = *bytes_read / sizeof(int16_t);
        for (size_t index = 0U; index < batch_output_samples; ++index)
        {
            output_samples[output_offset + index] =
                s_stereo_interleaved_samples[
                    index * ES8311_PCM_PHYSICAL_SLOT_COUNT];
        }
        *bytes_read += batch_output_samples * sizeof(int16_t);
    }
    s_last_failure = ES8311_BSP_FAILURE_NONE;
    return ESP_OK;
}

esp_err_t es8311_bsp_capture_stop(void)
{
    if (s_tx_enabled && !s_rx_enabled)
    {
        s_last_failure = ES8311_BSP_FAILURE_I2S;
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t first_error = ESP_OK;
    if (s_codec != NULL)
    {
        const int codec_error = s_codec->enable(s_codec, false);
        if (codec_error != ESP_CODEC_DEV_OK)
        {
            first_error = codec_error_to_esp(codec_error);
            s_last_failure = ES8311_BSP_FAILURE_CODEC;
        }
        else
        {
            s_capture_warm_pending = false;
        }
    }
    if (s_rx_enabled && s_rx_channel != NULL)
    {
        const esp_err_t err = i2s_channel_disable(s_rx_channel);
        if (err == ESP_OK)
        {
            s_rx_enabled = false;
        }
        else if (first_error == ESP_OK)
        {
            first_error = err;
            s_last_failure = ES8311_BSP_FAILURE_I2S;
        }
    }
    if (s_tx_enabled && s_tx_channel != NULL)
    {
        const esp_err_t err = i2s_channel_disable(s_tx_channel);
        if (err == ESP_OK)
        {
            s_tx_enabled = false;
        }
        else if (first_error == ESP_OK)
        {
            first_error = err;
            s_last_failure = ES8311_BSP_FAILURE_I2S;
        }
    }
    if (first_error == ESP_OK)
    {
        s_last_failure = ES8311_BSP_FAILURE_NONE;
    }
    return first_error;
}

esp_err_t es8311_bsp_deinit(void)
{
    esp_err_t first_error = ESP_OK;
    if (s_rx_enabled)
    {
        first_error = es8311_bsp_capture_stop();
    }
    if (!s_rx_enabled)
    {
        const esp_err_t stop_error = es8311_bsp_stop();
        if (first_error == ESP_OK)
        {
            first_error = stop_error;
        }
    }
    s_ready = false;
    s_capture_warm_pending = false;

    if (s_codec != NULL)
    {
        int codec_error = audio_codec_delete_codec_if(s_codec);
        /* esp_codec_dev 1.5.11 的 delete 契约无论 close 结果如何都会释放对象。 */
        s_codec = NULL;
        if (codec_error != ESP_CODEC_DEV_OK && first_error == ESP_OK)
        {
            first_error = codec_error_to_esp(codec_error);
            s_last_failure = ES8311_BSP_FAILURE_CODEC;
        }
    }

    if (s_ctrl.is_open)
    {
        int ctrl_error = s_ctrl.base.close(&s_ctrl.base);
        if (ctrl_error != ESP_CODEC_DEV_OK && first_error == ESP_OK)
        {
            first_error = codec_error_to_esp(ctrl_error);
            s_last_failure = ES8311_BSP_FAILURE_CONTROL;
        }
    }

    if (s_tx_channel != NULL && !s_tx_enabled)
    {
        esp_err_t err = i2s_del_channel(s_tx_channel);
        if (err == ESP_OK)
        {
            s_tx_channel = NULL;
        }
        else if (first_error == ESP_OK)
        {
            first_error = err;
            s_last_failure = ES8311_BSP_FAILURE_I2S;
        }
    }
    if (s_rx_channel != NULL && !s_rx_enabled)
    {
        esp_err_t err = i2s_del_channel(s_rx_channel);
        if (err == ESP_OK)
        {
            s_rx_channel = NULL;
        }
        else if (first_error == ESP_OK)
        {
            first_error = err;
            s_last_failure = ES8311_BSP_FAILURE_I2S;
        }
    }
    if (s_stereo_interleaved_samples != NULL)
    {
        heap_caps_free(s_stereo_interleaved_samples);
        s_stereo_interleaved_samples = NULL;
    }
    if (first_error == ESP_OK)
    {
        s_last_failure = ES8311_BSP_FAILURE_NONE;
    }
    return first_error;
}

bool es8311_bsp_is_ready(void)
{
    return s_ready && s_codec != NULL && s_tx_channel != NULL &&
           s_rx_channel != NULL && s_ctrl.is_open &&
           s_stereo_interleaved_samples != NULL;
}

es8311_bsp_failure_t es8311_bsp_last_failure(void)
{
    return s_last_failure;
}

const char *es8311_bsp_error_code(void)
{
    switch (s_last_failure)
    {
    case ES8311_BSP_FAILURE_NONE:
        return "DRV_ES8311_OK";
    case ES8311_BSP_FAILURE_CONTROL:
        return "DRV_ES8311_CONTROL_FAILED";
    case ES8311_BSP_FAILURE_I2S:
        return "DRV_ES8311_I2S_FAILED";
    case ES8311_BSP_FAILURE_RESAMPLER:
        return "DRV_ES8311_RESAMPLER_FAILED";
    case ES8311_BSP_FAILURE_CODEC:
    default:
        return "DRV_ES8311_CODEC_FAILED";
    }
}

static int ctrl_open(const audio_codec_ctrl_if_t *ctrl, void *cfg, int cfg_size)
{
    (void)cfg;
    (void)cfg_size;
    es8311_bsp_ctrl_t *instance = (es8311_bsp_ctrl_t *)ctrl;
    if (instance == NULL || instance->access.bus_handle == NULL)
    {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    if (instance->is_open)
    {
        return ESP_CODEC_DEV_OK;
    }

    esp_err_t err = instance->access.acquire(pdMS_TO_TICKS(ES8311_BSP_I2C_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        return ESP_CODEC_DEV_DRV_ERR;
    }
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8311_BSP_I2C_ADDRESS,
        .scl_speed_hz = LEGBOT_BSP_I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(instance->access.bus_handle,
                                    &device_config,
                                    &instance->device);
    instance->access.release();
    if (err == ESP_OK)
    {
        instance->is_open = true;
        return ESP_CODEC_DEV_OK;
    }
    return ESP_CODEC_DEV_DRV_ERR;
}

static bool ctrl_is_open(const audio_codec_ctrl_if_t *ctrl)
{
    const es8311_bsp_ctrl_t *instance = (const es8311_bsp_ctrl_t *)ctrl;
    return instance != NULL && instance->is_open && instance->device != NULL;
}

static int ctrl_read_reg(const audio_codec_ctrl_if_t *ctrl,
                         int reg,
                         int reg_len,
                         void *data,
                         int data_len)
{
    es8311_bsp_ctrl_t *instance = (es8311_bsp_ctrl_t *)ctrl;
    if (!ctrl_is_open(ctrl) || reg < 0 || reg > UINT8_MAX ||
        reg_len != 1 || data == NULL || data_len != 1)
    {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    const uint8_t register_address = (uint8_t)reg;
    esp_err_t err = instance->access.acquire(pdMS_TO_TICKS(ES8311_BSP_I2C_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = i2c_master_transmit_receive(instance->device,
                                          &register_address,
                                          sizeof(register_address),
                                          data,
                                          1,
                                          ES8311_BSP_I2C_TIMEOUT_MS);
        instance->access.release();
    }
    return err == ESP_OK ? ESP_CODEC_DEV_OK : ESP_CODEC_DEV_READ_FAIL;
}

static int ctrl_write_reg(const audio_codec_ctrl_if_t *ctrl,
                          int reg,
                          int reg_len,
                          void *data,
                          int data_len)
{
    es8311_bsp_ctrl_t *instance = (es8311_bsp_ctrl_t *)ctrl;
    if (!ctrl_is_open(ctrl) || reg < 0 || reg > UINT8_MAX ||
        reg_len != 1 || data == NULL || data_len != 1)
    {
        return ESP_CODEC_DEV_INVALID_ARG;
    }
    const uint8_t frame[2] = {(uint8_t)reg, *(const uint8_t *)data};
    esp_err_t err = instance->access.acquire(pdMS_TO_TICKS(ES8311_BSP_I2C_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = i2c_master_transmit(instance->device,
                                  frame,
                                  sizeof(frame),
                                  ES8311_BSP_I2C_TIMEOUT_MS);
        instance->access.release();
    }
    return err == ESP_OK ? ESP_CODEC_DEV_OK : ESP_CODEC_DEV_WRITE_FAIL;
}

static int ctrl_close(const audio_codec_ctrl_if_t *ctrl)
{
    es8311_bsp_ctrl_t *instance = (es8311_bsp_ctrl_t *)ctrl;
    if (instance == NULL || !instance->is_open)
    {
        return ESP_CODEC_DEV_OK;
    }
    esp_err_t err = instance->access.acquire(pdMS_TO_TICKS(ES8311_BSP_I2C_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = i2c_master_bus_rm_device(instance->device);
        instance->access.release();
    }
    if (err == ESP_OK)
    {
        instance->device = NULL;
        instance->is_open = false;
        return ESP_CODEC_DEV_OK;
    }
    return ESP_CODEC_DEV_DRV_ERR;
}

static bool has_partial_state(void)
{
    return s_codec != NULL || s_tx_channel != NULL || s_rx_channel != NULL ||
           s_tx_enabled || s_rx_enabled || s_capture_warm_pending || s_ctrl.is_open ||
           s_ctrl.device != NULL || s_stereo_interleaved_samples != NULL;
}

static esp_err_t discard_capture_startup_samples(size_t output_samples,
                                                 bool warm_transition)
{
    int16_t samples[ES8311_BSP_CAPTURE_WARMUP_BATCH_SAMPLES];
    const size_t target_samples =
        output_samples * ES8311_PCM_PHYSICAL_SLOT_COUNT;
    const size_t target_bytes = target_samples * sizeof(int16_t);
    size_t discarded_bytes = 0U;
    const int64_t deadline_us =
        esp_timer_get_time() +
        (int64_t)ES8311_BSP_CAPTURE_WARMUP_TIMEOUT_MS * 1000LL;

    while (discarded_bytes < target_bytes)
    {
        const int64_t remaining_us = deadline_us - esp_timer_get_time();
        if (remaining_us <= 0LL)
        {
            return ESP_ERR_TIMEOUT;
        }
        const size_t remaining_bytes = target_bytes - discarded_bytes;
        const size_t request_bytes =
            remaining_bytes < sizeof(samples) ? remaining_bytes : sizeof(samples);
        const uint32_t remaining_ms =
            (uint32_t)((remaining_us + 999LL) / 1000LL);
        size_t bytes_read = 0U;
        const esp_err_t err = i2s_channel_read(s_rx_channel,
                                               samples,
                                               request_bytes,
                                               &bytes_read,
                                               remaining_ms);
        if (err != ESP_OK || bytes_read == 0U || bytes_read > request_bytes)
        {
            return err != ESP_OK ? err : ESP_ERR_INVALID_SIZE;
        }
        discarded_bytes += bytes_read;
    }

    ESP_LOGI(TAG,
             "ES8311 ADC %s瞬态已排空：输入样本率=%u Hz，输入样本=%u，16kHz 等效=%u",
             warm_transition ? "暖切换" : "冷启动",
             (unsigned)ES8311_BSP_CAPTURE_SAMPLE_RATE_HZ,
             (unsigned)target_samples,
             (unsigned)output_samples);
    return ESP_OK;
}

static esp_err_t init_i2s_channel(const legbot_bsp_resource_t *resource)
{
    if (s_tx_channel != NULL || s_rx_channel != NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    i2s_chan_config_t channel_config = I2S_CHANNEL_DEFAULT_CONFIG(
        (i2s_port_t)resource->i2s_port,
        I2S_ROLE_MASTER);
    channel_config.dma_desc_num = AUDIO_BSP_DMA_DESC_NUM;
    channel_config.dma_frame_num = AUDIO_BSP_DMA_FRAME_NUM;
    channel_config.auto_clear_after_cb = true;
    esp_err_t err = i2s_new_channel(&channel_config,
                                    &s_tx_channel,
                                    &s_rx_channel);
    if (err != ESP_OK)
    {
        return err;
    }

    i2s_std_config_t standard_config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_BSP_SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            I2S_DATA_BIT_WIDTH_16BIT,
            I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = resource->gpio_aux2,
            .bclk = resource->gpio_aux1,
            .ws = resource->gpio_secondary,
            .dout = resource->gpio_primary,
            .din = resource->gpio_aux0,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    standard_config.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_384;
    /* 最坏双时隙 DMA 在启动期一次落位，播放由 BSP 内部补右时隙静音。 */
    standard_config.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;
    err = i2s_channel_init_std_mode(s_tx_channel, &standard_config);
    if (err != ESP_OK)
    {
        goto cleanup;
    }
    err = i2s_channel_init_std_mode(s_rx_channel, &standard_config);
    if (err == ESP_OK)
    {
        return ESP_OK;
    }

cleanup:
    {
        const esp_err_t tx_error = i2s_del_channel(s_tx_channel);
        const esp_err_t rx_error = i2s_del_channel(s_rx_channel);
        if (tx_error == ESP_OK)
        {
            s_tx_channel = NULL;
        }
        if (rx_error == ESP_OK)
        {
            s_rx_channel = NULL;
        }
        if (tx_error != ESP_OK)
        {
            return tx_error;
        }
        if (rx_error != ESP_OK)
        {
            return rx_error;
        }
        return err;
    }
}

static int configure_capture_gain(void)
{
    if (s_codec == NULL || s_codec->set_mic_gain == NULL ||
        s_codec->set_reg == NULL)
    {
        return ESP_CODEC_DEV_INVALID_ARG;
    }

    int codec_error = s_codec->set_mic_gain(s_codec,
                                            ES8311_BSP_INPUT_GAIN_DB);
    if (codec_error != ESP_CODEC_DEV_OK)
    {
        return codec_error;
    }

    /*
     * esp_codec_dev 1.5.11 的 set_mic_gain() 会整字节覆盖 Reg0x16，
     * 从而把初始化阶段正确设置的 ADC_SYNC(bit5) 清零。当前 16 kHz、
     * 384fs 是标准音频时钟，必须在写入 ADC_SCALE 后恢复同步位。
     */
    return s_codec->set_reg(
        s_codec,
        ES8311_BSP_ADC_CONTROL_REGISTER,
        ES8311_BSP_ADC_STANDARD_CLOCK_SYNC_BIT |
            ES8311_BSP_ADC_SCALE_42_DB_VALUE);
}

static esp_err_t codec_error_to_esp(int error)
{
    return error == ESP_CODEC_DEV_OK ? ESP_OK : (esp_err_t)error;
}
