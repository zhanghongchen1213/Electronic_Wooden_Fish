/**
 * @file     vibration_bsp.h
 * @brief    振动马达 LEDC/PWM BSP 接口
 * @details  通过静态租约独占 IO11 PWM 所有权，运行期停止保留配置，完整反初始化时释放 GPIO。
 * @author   ZHC
 * @date     2026-07-23
 */

#ifndef LEGBOT_VIBRATION_BSP_H
#define LEGBOT_VIBRATION_BSP_H

#include "bsp_resources.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

/** 产品已授权采用通用低边驱动 ERM 软件 profile。 */
#define VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED 1
/** ESP32-S3 振动 PWM 仅允许 LEDC low-speed 模式。 */
#define VIBRATION_BSP_LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
/** PWM 采用 20 kHz，避开可听频段并满足 ESP32-S3 10-bit LEDC 分频范围。 */
#define VIBRATION_BSP_PWM_FREQUENCY_HZ 20000U
/** 非反相 PWM 的有效电平为高电平。 */
#define VIBRATION_BSP_ACTIVE_LEVEL 1U
/** 10-bit PWM 周期包含 1024 个计数步。 */
#define VIBRATION_BSP_PWM_PERIOD_STEPS 1024U
/** ESP32-S3 10-bit LEDC 可安全写入的最大 duty，禁止使用会溢出的 1024。 */
#define VIBRATION_BSP_DUTY_MAX 1023U
/** 振动脉冲采用 10-bit LEDC 最大有效 duty，提供最大可用启动转矩。 */
#define VIBRATION_BSP_ACTIVE_DUTY VIBRATION_BSP_DUTY_MAX
/** 空闲时 PWM duty 固定为 0。 */
#define VIBRATION_BSP_IDLE_DUTY 0U
/** 停止 LEDC 后保持低电平，避免马达误动作。 */
#define VIBRATION_BSP_IDLE_LEVEL 0U
/** 任何一次振动脉冲的强制时长上限。 */
#define VIBRATION_BSP_PULSE_MAX_MS 1000U
/** 自检使用 1 秒有界脉冲，提高贴腕状态下的可感知性。 */
#define VIBRATION_BSP_SELFTEST_PULSE_MS 1000U
/** UI 按钮触觉反馈使用 60 ms 短脉冲。 */
#define VIBRATION_BSP_UI_FEEDBACK_PULSE_MS 60U

#if VIBRATION_BSP_ELECTRICAL_PROFILE_VERIFIED
#ifndef VIBRATION_BSP_PWM_FREQUENCY_HZ
#error "Verified vibration profile must define VIBRATION_BSP_PWM_FREQUENCY_HZ."
#endif
#ifndef VIBRATION_BSP_ACTIVE_DUTY
#error "Verified vibration profile must define VIBRATION_BSP_ACTIVE_DUTY."
#endif
#ifndef VIBRATION_BSP_IDLE_DUTY
#error "Verified vibration profile must define VIBRATION_BSP_IDLE_DUTY."
#endif
#ifndef VIBRATION_BSP_IDLE_LEVEL
#error "Verified vibration profile must define VIBRATION_BSP_IDLE_LEVEL."
#endif
#endif

/**
 * @brief 获取振动马达 BSP 资源描述
 * @return 振动马达资源描述指针
 */
const legbot_bsp_resource_t *vibration_bsp_resource(void);

/**
 * @brief 查询振动软件 profile 是否已经启用
 * @return true 可以驱动 PWM，false 必须失效保护
 */
bool vibration_bsp_profile_verified(void);

/**
 * @brief 初始化振动硬件静态互斥租约
 * @return ESP_OK 成功或已初始化，ESP_ERR_NO_MEM 静态互斥量创建失败
 */
esp_err_t vibration_bsp_lease_init(void);

/**
 * @brief 获取振动硬件独占租约
 * @param timeout_ticks 等待租约的有限 tick 数
 * @return ESP_OK 已获取，ESP_ERR_INVALID_STATE 租约未初始化，ESP_ERR_TIMEOUT 等待超时
 */
esp_err_t vibration_bsp_lease_acquire(TickType_t timeout_ticks);

/**
 * @brief 释放当前任务持有的振动硬件租约
 * @return ESP_OK 已释放，ESP_ERR_INVALID_STATE 租约未初始化或释放失败
 */
esp_err_t vibration_bsp_lease_release(void);

/**
 * @brief 初始化 IO11 唯一 low-speed LEDC owner
 * @return ESP_OK 成功或已初始化
 *         ESP_ERR_NOT_SUPPORTED 电气 profile 未验证
 *         其他 ESP-IDF 错误码表示 LEDC 初始化失败
 */
esp_err_t vibration_bsp_init(void);

/**
 * @brief 启动一次须由调用者在 duration_ms 内停止的有界 PWM 脉冲
 * @param duration_ms 调用者承诺的脉冲时长，不得超过 VIBRATION_BSP_PULSE_MAX_MS
 * @return ESP_OK 已启动，其他值表示未就绪、profile 未验证或 LEDC 失败
 */
esp_err_t vibration_bsp_start_pulse(uint32_t duration_ms);

/**
 * @brief 停止 PWM 并恢复已验证的安全 idle 电平，同时保留 LEDC/GPIO 配置供下次脉冲复用
 * @return ESP_OK 已安全停止，其他值表示 LEDC 停止失败
 */
esp_err_t vibration_bsp_stop(void);

/**
 * @brief 幂等停止并完整释放 LEDC 定时器与 GPIO 所有权
 * @return ESP_OK 已释放，其他值表示安全停止、定时器反初始化或 GPIO 释放失败
 */
esp_err_t vibration_bsp_deinit(void);

#endif /* LEGBOT_VIBRATION_BSP_H */
