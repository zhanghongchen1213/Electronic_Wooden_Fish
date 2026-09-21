/**
 * @file     es8311_bsp.h
 * @brief    ES8311 与 I2S0 音频输入输出 BSP 接口
 * @details  通过注入的共享 I2C0 access 配置 codec，并独占 BSP 权威 I2S0 TX/RX channel 生命周期。
 * @author   ZHC
 * @date     2026-07-22
 */

#ifndef LEGBOT_ES8311_BSP_H
#define LEGBOT_ES8311_BSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** ES8311 的 7-bit I2C 地址。 */
#define ES8311_BSP_I2C_ADDRESS 0x18U
/** 播放与上层语音 PCM 的固定采样率。 */
#define AUDIO_BSP_SAMPLE_RATE_HZ 16000U
/** ES8311 按 ESP-IDF v5.5.4 官方示例使用的物理采集采样率。 */
#define ES8311_BSP_CAPTURE_SAMPLE_RATE_HZ 16000U
/** 音频 BSP 固定位宽。 */
#define AUDIO_BSP_BITS_PER_SAMPLE 16U
/** ES8311 slave 模式下 MCLK/LRCK 比率，与 ESP-IDF v5.5.4 官方 ES8311 示例一致。 */
#define AUDIO_BSP_MCLK_MULTIPLE 384U
/** I2S DMA 单描述符帧数，对应 16 kHz 下 8 ms 回收周期。 */
#define AUDIO_BSP_DMA_FRAME_NUM 128U
/** I2S TX DMA 描述符数量，与 ESP-IDF v5.5.4 默认值一致。 */
#define AUDIO_BSP_DMA_DESC_NUM 6U
/** 自检与提示音 codec 输出音量，单位 dB；每次启动都必须恢复此值。 */
#define ES8311_BSP_OUTPUT_VOLUME_DB (-3.0F)
/** ADC 粗增益 ADC_SCALE，单位 dB；模拟 PGA 固定为 30 dB，写入后恢复标准音频时钟所需的 ADC_SYNC 位。 */
#define ES8311_BSP_INPUT_GAIN_DB 42.0F
/** ADC 启动后丢弃的 16 kHz 等效样本数，对应 120 ms codec 与 RX DMA 启动瞬态。 */
#define ES8311_BSP_CAPTURE_WARMUP_OUTPUT_SAMPLES 1920U
/** 提示音暖切换后丢弃的 16 kHz 等效样本数，对应 32 ms RX DMA 时钟重启瞬态。 */
#define ES8311_BSP_WARM_CAPTURE_DISCARD_OUTPUT_SAMPLES 512U

    typedef enum
    {
        ES8311_BSP_FAILURE_NONE = 0, /**< 最近操作没有驱动错误。 */
        ES8311_BSP_FAILURE_CONTROL,  /**< I2C control interface 操作失败。 */
        ES8311_BSP_FAILURE_CODEC,    /**< ES8311 codec 配置或启停失败。 */
        ES8311_BSP_FAILURE_I2S,      /**< I2S0 channel 配置、启停或写入失败。 */
        ES8311_BSP_FAILURE_RESAMPLER, /**< SILK 重采样器配置或处理失败。 */
    } es8311_bsp_failure_t;

    /**
     * @brief 获取 ES8311 音频编解码 BSP 资源描述
     * @return ES8311 资源描述指针
     */
    const legbot_bsp_resource_t *es8311_bsp_resource(void);

    /**
     * @brief 使用 manager-owned I2C access 初始化 ES8311 和 I2S0 TX/RX
     * @param access 共享 I2C0 bus 与短互斥能力
     * @return ESP_OK 成功或已初始化，其他值表示 codec/I2S 初始化失败
     */
    esp_err_t es8311_bsp_init(const legbot_bsp_i2c_access_t *access);

    /**
     * @brief 启动 I2S0 TX 与 ES8311 DAC
     * @return ESP_OK 成功，其他值表示 codec/I2S 启动失败
     */
    esp_err_t es8311_bsp_start(void);

    /**
     * @brief 先 mute codec 再停止 I2S0 TX
     * @return ESP_OK 成功，其他值表示清理失败且句柄保留以便重试
     */
    esp_err_t es8311_bsp_stop(void);

    /**
     * @brief 有界写入一块 PCM 数据
     * @details 输入保持 16-bit mono 合同，BSP 在 PSRAM 工作区内转换为左时隙数据和右时隙静音。
     * @param data 单声道 PCM 数据
     * @param size 待写单声道字节数，必须为完整 16-bit 样本
     * @param bytes_written 实际完成的单声道源字节数
     * @param timeout_ms I2S 等待上限，必须大于 0
     * @return ESP_OK 成功，ESP_ERR_TIMEOUT 等待超时，其他值表示 I2S 失败
     */
    esp_err_t es8311_bsp_write(const void *data,
                               size_t size,
                               size_t *bytes_written,
                               uint32_t timeout_ms);

    /**
     * @brief 将已完成的提示音播放链切换为待采集暖状态
     * @details 立即静音 DAC 并停止 TX，但保持 codec 模拟链启用；仅供功放已安全关闭的语音提示音成功路径使用。
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 表示当前不是独占播放状态，其他值表示 codec/I2S 切换失败
     */
    esp_err_t es8311_bsp_prepare_capture_after_playback(void);

    /**
     * @brief 启动 ES8311 ADC 与 I2S0 RX
     * @details I2S 在初始化阶段已固定为 stereo；采集与播放互斥，启动后 DAC 保持静音，ADC_SCALE 恢复为 42 dB，并保持标准时钟 ADC_SYNC。冷启动排空 120 ms，提示音暖切换只排空 32 ms。
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 表示尚未就绪或正在播放，其他值表示 codec/I2S 启动失败
     */
    esp_err_t es8311_bsp_capture_start(void);

    /**
     * @brief 有界读取一块 16 kHz 单声道麦克风 PCM 数据
     * @details ES8311/I2S 按官方配置使用 16 kHz/16-bit/stereo 传输，抽取左时隙 ADC 输出 MultiNet 所需的连续单声道 PCM。
     * @param data PCM 输出缓冲区
     * @param size 期望读取字节数
     * @param bytes_read 实际读取字节数
     * @param timeout_ms I2S 等待上限，必须大于 0
     * @return ESP_OK 成功，ESP_ERR_TIMEOUT 等待超时，其他值表示 I2S 失败
     */
    esp_err_t es8311_bsp_read(void *data,
                              size_t size,
                              size_t *bytes_read,
                              uint32_t timeout_ms);

    /**
     * @brief 停止 ES8311 ADC 与 I2S0 RX
     * @return ESP_OK 成功，其他值表示清理失败且句柄保留以便重试
     */
    esp_err_t es8311_bsp_capture_stop(void);

    /**
     * @brief 幂等释放 codec、I2S 和 ES8311 device handle
     * @return ESP_OK 全部释放，其他值为首个清理错误
     */
    esp_err_t es8311_bsp_deinit(void);

    /**
     * @brief 查询 codec 与 I2S0 是否已完成初始化
     * @return true 已就绪，false 尚未就绪或处于部分清理状态
     */
    bool es8311_bsp_is_ready(void);

    /**
     * @brief 获取最近一次 ES8311 BSP 失败域
     * @return 稳定的 control、codec 或 I2S 失败分类
     */
    es8311_bsp_failure_t es8311_bsp_last_failure(void);

    /**
     * @brief 获取最近一次 ES8311 BSP 稳定驱动错误码
     * @return DRV_ES8311_ 前缀的静态字符串
     */
    const char *es8311_bsp_error_code(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_ES8311_BSP_H */
