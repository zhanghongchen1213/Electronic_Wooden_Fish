/**
 * @file     feedback_audio.h
 * @brief    木鱼音播放链后端接口。
 * @details  反馈服务通过本接口驱动 ES8311/NS4150B 播放链，不直接操作
 *           I2S/GPIO/I²C（AD-8/AD-10）；固件侧由 feedback_audio_bsp.c 以
 *           SPIFFS + BSP 安全序列实现（init → start → PA 使能 → 有界写入 →
 *           播毕安全关断），主机测试由替身实现并记录调用序列。两侧符号同名，
 *           由链接选择。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_FEEDBACK_AUDIO_H
#define EWF_FEEDBACK_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化木鱼音播放链（幂等）
 * @details 完成 SPIFFS 挂载检查、木鱼音资源加载与 ES8311 复用共享 I²C access
 *           的初始化；资源缺失/损坏按稳定错误域返回，由服务层降级为跳过播放。
 * @return ESP_OK 成功或已初始化，其他值表示资源、codec 或 I2S 初始化失败
 */
esp_err_t ewf_feedback_audio_backend_init(void);

/**
 * @brief 按当前音量播放一次木鱼音
 * @details 音量 0 等价静音：跳过播放链并只确保功放处于安全关断，不产生任何
 *           可听输出（AC 2）。非 0 音量按 PCM 数字增益缩放输出（host 可测、
 *           纯软件，选择依据见 Story 2.3 Task 3）。
 * @param volume_percent 当前生效音量 0-100
 * @return ESP_OK 已完整播放或已按静音跳过，其他值表示播放链失败（稳定错误域
 *           由 ewf_feedback_audio_backend_last_error() 给出）
 */
esp_err_t ewf_feedback_audio_backend_play(uint8_t volume_percent);

/**
 * @brief 确保功放处于安全关断状态（幂等）
 * @details 静音与故障时关断功放（硬件基线 §5.3.1 的被消解旧语义"充电"不实现）。
 * @return ESP_OK 成功，其他值表示功放控制失败
 */
esp_err_t ewf_feedback_audio_backend_ensure_pa_off(void);

/**
 * @brief 获取最近一次播放链失败的稳定错误分类
 * @return 调用方存储的最近错误码；成功后复位为 ESP_OK
 */
esp_err_t ewf_feedback_audio_backend_last_error(void);

/**
 * @brief 获取最近一次播放链失败的 typed 通道错误域
 * @details 固件侧按 ES8311 失败域与 NS4150 功放域映射（codec/I2S/PA/资源）；
 *           成功后复位为 WATCH_FEEDBACK_ERROR_NONE。
 * @return app_state.h 反馈通道稳定错误域
 */
watch_feedback_error_t ewf_feedback_audio_backend_typed_error(void);

#ifdef __cplusplus
}
#endif

#endif /* EWF_FEEDBACK_AUDIO_H */
