/**
 * @file     power_boot_policy.h
 * @brief    PWR_INT/BOOT0 边沿与低电平时长判定的纯逻辑
 * @details  不含任何 ESP-IDF 依赖，可由主机测试重复运行。策略只做原始边沿交接、
 *           去抖和低电平时长分类，不做业务决策，也不改写复位采样条件。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_POWER_BOOT_POLICY_H
#define EWF_POWER_BOOT_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 短于该单调毫秒数的电平区间视为抖动，不生成按压区间。 */
#define EWF_POWER_BOOT_DEBOUNCE_MS 20U
/** 运行态 BOOT0 仍算作短按的最长单调毫秒数。 */
#define EWF_POWER_BOOT_RUNTIME_TAP_MAX_MS 3000U
/** PWR_INT 有效电平：按下期间持续为低。 */
#define EWF_POWER_BOOT_ACTIVE_LEVEL 0U

    /** 一次边沿交接的 typed 结论。 */
    typedef enum
    {
        EWF_POWER_BOOT_EVENT_NONE = 0,             /**< 本次边沿没有形成可交接结论。 */
        EWF_POWER_BOOT_EVENT_PWR_INTERVAL,         /**< 一个完整的 PWR 按压区间及其低电平时长。 */
        EWF_POWER_BOOT_EVENT_PWR_GLITCH_IGNORED,   /**< PWR 电平区间短于去抖门限。 */
        EWF_POWER_BOOT_EVENT_BOOT0_RUNTIME_TAP,    /**< 运行态 BOOT0 完整短按区间。 */
        EWF_POWER_BOOT_EVENT_BOOT0_LONG_SUPPRESSED,/**< 运行态 BOOT0 长按，不产生自动模式事件。 */
        EWF_POWER_BOOT_EVENT_BOOT0_GLITCH_IGNORED, /**< BOOT0 电平区间短于去抖门限。 */
        EWF_POWER_BOOT_EVENT_BOOT0_STARTUP_SUPPRESSED, /**< 启动/下载采样窗口内的 BOOT0 电平变化。 */
        EWF_POWER_BOOT_EVENT_COUNT                 /**< 结论数量，不是有效结论。 */
    } ewf_power_boot_event_kind_t;

    typedef struct
    {
        bool pwr_interval_open;      /**< 是否已记录一个未闭合的 PWR 低电平区间。 */
        uint32_t pwr_started_at_ms;  /**< 该区间下降沿的单调毫秒。 */
        bool boot_interval_open;     /**< 是否已记录一个未闭合的 BOOT0 低电平区间。 */
        uint32_t boot_started_at_ms; /**< 该区间下降沿的单调毫秒。 */
        bool boot_runtime_armed;     /**< 启动采样窗口是否已经结束。 */
        bool boot_held_from_startup; /**< BOOT0 在进入运行态时仍保持低电平。 */
        uint32_t pwr_interval_count; /**< 已交接的合法 PWR 按压区间数。 */
        uint32_t boot_tap_count;     /**< 已交接的运行态 BOOT0 短按数。 */
    } ewf_power_boot_policy_t;

    typedef struct
    {
        ewf_power_boot_event_kind_t kind; /**< typed 结论。 */
        uint32_t duration_ms;             /**< 区间单调毫秒时长；无区间结论时为 0。 */
        uint32_t at_ms;                   /**< 结论形成时刻的单调毫秒。 */
    } ewf_power_boot_event_t;

    /**
     * @brief 复位策略状态
     * @param policy 策略实例
     */
    void ewf_power_boot_policy_reset(ewf_power_boot_policy_t *policy);

    /**
     * @brief 结束启动采样窗口并进入运行态
     * @details BOOT0 在进入运行态时仍为低电平，说明本次启动处于下载/启动绑带窗口，
     *          该区间的释放不得产生运行态自动事件。
     * @param policy 策略实例
     * @param boot_level 进入运行态时 BOOT0 的原始电平
     * @param now_ms 当前单调毫秒
     */
    void ewf_power_boot_policy_arm_runtime(ewf_power_boot_policy_t *policy,
                                           uint8_t boot_level,
                                           uint32_t now_ms);

    /**
     * @brief 交接一次 PWR_INT 原始边沿
     * @param policy 策略实例
     * @param level 采样到的原始电平
     * @param now_ms 边沿时刻的单调毫秒
     * @param event 结论输出，可为 NULL
     * @return true 表示已形成可交接结论
     */
    bool ewf_power_boot_policy_on_pwr_level(ewf_power_boot_policy_t *policy,
                                            uint8_t level,
                                            uint32_t now_ms,
                                            ewf_power_boot_event_t *event);

    /**
     * @brief 交接一次 BOOT0 原始边沿
     * @param policy 策略实例
     * @param level 采样到的原始电平
     * @param now_ms 边沿时刻的单调毫秒
     * @param event 结论输出，可为 NULL
     * @return true 表示已形成可交接结论
     */
    bool ewf_power_boot_policy_on_boot_level(ewf_power_boot_policy_t *policy,
                                             uint8_t level,
                                             uint32_t now_ms,
                                             ewf_power_boot_event_t *event);

    /**
     * @brief 观察当前仍持续的 PWR 低电平时长
     * @param policy 策略实例
     * @param now_ms 当前单调毫秒
     * @return 已持续的单调毫秒数，未处于低电平区间时为 0
     */
    uint32_t ewf_power_boot_policy_pwr_low_duration_ms(
        const ewf_power_boot_policy_t *policy,
        uint32_t now_ms);

    /**
     * @brief 判断当前是否处于已确认的 PWR 低电平区间
     * @param policy 策略实例
     * @return true 表示区间已打开
     */
    bool ewf_power_boot_policy_pwr_is_low(const ewf_power_boot_policy_t *policy);

#ifdef __cplusplus
}
#endif

#endif /* EWF_POWER_BOOT_POLICY_H */
