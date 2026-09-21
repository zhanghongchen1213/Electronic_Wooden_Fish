/**
 * @file     es8311_pcm_transport_internal.h
 * @brief    ES8311 单声道逻辑 PCM 与双时隙物理帧转换
 * @details  为常驻 stereo I2S 提供可单测的左声道交错与完整物理帧计数，不暴露新的 BSP 公共接口。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_ES8311_PCM_TRANSPORT_INTERNAL_H
#define LEGBOT_ES8311_PCM_TRANSPORT_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

/** ES8311 常驻 I2S 的左右物理时隙数量。 */
#define ES8311_PCM_PHYSICAL_SLOT_COUNT 2U
/** 单个 16-bit stereo 物理帧的字节数。 */
#define ES8311_PCM_STEREO_FRAME_BYTES 4U

/**
 * @brief 把 16-bit 单声道 PCM 交错为左声道数据和右声道静音
 * @param mono_samples 单声道输入样本
 * @param mono_sample_count 输入样本数量
 * @param stereo_samples 双时隙输出缓冲区
 * @param stereo_sample_capacity 输出缓冲区可容纳的 16-bit 样本数量
 * @return 成功转换的单声道样本数；参数或容量无效时返回 0
 */
static inline size_t es8311_pcm_interleave_mono_left(
    const int16_t *mono_samples,
    size_t mono_sample_count,
    int16_t *stereo_samples,
    size_t stereo_sample_capacity)
{
    if (mono_samples == NULL || stereo_samples == NULL ||
        mono_sample_count == 0U ||
        mono_sample_count > SIZE_MAX / ES8311_PCM_PHYSICAL_SLOT_COUNT ||
        stereo_sample_capacity <
            mono_sample_count * ES8311_PCM_PHYSICAL_SLOT_COUNT)
    {
        return 0U;
    }

    for (size_t index = 0U; index < mono_sample_count; ++index)
    {
        const size_t output_index = index * ES8311_PCM_PHYSICAL_SLOT_COUNT;
        stereo_samples[output_index] = mono_samples[index];
        stereo_samples[output_index + 1U] = 0;
    }
    return mono_sample_count;
}

/**
 * @brief 把已完成的 stereo 物理字节数折算为完整 mono 源字节数
 * @param physical_bytes I2S 已接受的物理字节数
 * @return 只计完整左右时隙帧的单声道源字节数
 */
static inline size_t es8311_pcm_complete_mono_bytes(size_t physical_bytes)
{
    return physical_bytes / ES8311_PCM_STEREO_FRAME_BYTES * sizeof(int16_t);
}

#endif /* LEGBOT_ES8311_PCM_TRANSPORT_INTERNAL_H */
