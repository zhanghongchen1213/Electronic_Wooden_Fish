/**
 * @file     key.h
 * @brief    板载按键 BSP 接口
 * @details  定义 LTC2954 PWR_INT 与 BOOT0 启动绑带的板级 GPIO 映射、原始读取和运行态任意边沿回调。
 * @author   ZHC
 * @date     2026-07-09
 */

#ifndef LEGBOT_KEY_H
#define LEGBOT_KEY_H

#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** LTC2954 PWR_INT GPIO，来自权威 BSP 资源表。 */
#define LEGBOT_KEY_PWR_GPIO EWF_BSP_PWR_INT_GPIO

/** BOOT0 启动绑带 GPIO，来自权威 BSP 资源表。 */
#define LEGBOT_KEY_BOOT_GPIO EWF_BSP_BOOT0_GPIO

    typedef enum
    {
        LEGBOT_KEY_PWR = 0, /**< 电源按键。 */
        LEGBOT_KEY_BOOT,    /**< BOOT 按键。 */
        LEGBOT_KEY_COUNT    /**< 按键数量。 */
    } legbot_key_id_t;

    typedef struct
    {
        uint8_t pwr_level;  /**< PWR 引脚原始逻辑电平，不解释按下极性。 */
        uint8_t boot_level; /**< BOOT 引脚原始逻辑电平，不解释按下极性。 */
    } legbot_key_state_t;

    /** PWR GPIO 中断上下文回调；实现只能调用 ISR-safe API。 */
    typedef void (*legbot_key_pwr_isr_callback_t)(void *context);

    /** BOOT0 GPIO 中断上下文回调；实现只能调用 ISR-safe API。 */
    typedef void (*legbot_key_boot_isr_callback_t)(void *context);

    /**
     * @brief 初始化板载按键 GPIO
     * @return ESP_OK 成功
     *         其他 ESP-IDF 错误码表示 GPIO 配置失败
     */
    esp_err_t key_init(void);

    /**
     * @brief 获取指定按键的 GPIO 编号
     * @param key 按键 ID
     * @return 有效按键返回对应 GPIO，非法按键返回 GPIO_NUM_NC
     */
    gpio_num_t key_gpio(legbot_key_id_t key);

    /**
     * @brief 读取 PWR/BOOT 原始电平的 typed 快照
     * @param state 双按键原始电平输出
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE 按键 BSP 尚未初始化
     */
    esp_err_t key_read_state(legbot_key_state_t *state);

    /**
     * @brief 为运行态 PWR_INT 登记唯一 ISR 回调
     * @details PWR_INT 保持开漏低有效只读输入；按下与释放各产生一次任意边沿中断，
     *          低电平时长由任务上下文测量，ISR 不做业务决策。
     * @param callback 中断上下文回调，不得访问 I2C 或执行阻塞操作
     * @param context 原样传回给 callback 的上下文
     * @return ESP_OK 登记成功或已登记同一回调
     *         ESP_ERR_INVALID_ARG 回调为空
     *         ESP_ERR_INVALID_STATE BSP 未初始化或已被其他所有者占用
     *         其他 ESP-IDF 错误码表示 GPIO ISR 登记失败
     */
    esp_err_t key_register_pwr_isr_callback(
        legbot_key_pwr_isr_callback_t callback,
        void *context);

    /**
     * @brief 使能 PWR_INT 的任意边沿中断与 light-sleep 唤醒
     * @details PWR_INT 允许在按住期间持续为低，因此必须捕获下降与上升两个边沿，
     *          不能按电平轮询，也不能只等单次脉冲。
     * @return ESP_OK 重武装成功
     *         ESP_ERR_INVALID_STATE BSP 未初始化或 ISR 尚未登记
     *         其他 ESP-IDF 错误码表示 GPIO 配置失败
     */
    esp_err_t key_rearm_pwr_isr_for_next_level(void);

    /**
     * @brief 解注册运行态 PWR_INT GPIO ISR 回调
     * @details 先禁用 PWR_INT 中断再移除 handler，不卸载共享 GPIO ISR service。
     * @return ESP_OK 解注册成功或原本未登记
     *         ESP_ERR_INVALID_STATE BSP 未初始化
     *         其他 ESP-IDF 错误码表示 GPIO 操作失败
     */
    esp_err_t key_unregister_pwr_isr_callback(void);

    /**
     * @brief 为运行态 BOOT0 GPIO 登记唯一 ISR 回调
     * @details BOOT0 始终保持上拉输入；应用只接收运行态电平变化，不改变关机态 PWR+BOOT0 的 Boot ROM 下载路径。
     * @param callback 中断上下文回调，不得执行阻塞操作
     * @param context 原样传回给 callback 的上下文
     * @return ESP_OK 登记成功或已登记同一回调
     *         ESP_ERR_INVALID_ARG 回调为空
     *         ESP_ERR_INVALID_STATE BSP 未初始化或已被其他所有者占用
     *         其他 ESP-IDF 错误码表示 GPIO ISR 登记失败
     */
    esp_err_t key_register_boot_isr_callback(
        legbot_key_boot_isr_callback_t callback,
        void *context);

    /**
     * @brief 使能 BOOT0 的任意边沿中断与 light-sleep 唤醒
     * @details 运行态短按的按下与释放各产生一次边沿；该操作绝不改写复位采样窗口。
     * @return ESP_OK 重武装成功
     *         ESP_ERR_INVALID_STATE BSP 未初始化或 ISR 尚未登记
     *         其他 ESP-IDF 错误码表示 GPIO 配置失败
     */
    esp_err_t key_rearm_boot_isr_for_next_level(void);

    /**
     * @brief 解注册运行态 BOOT0 GPIO ISR 回调
     * @details 先禁用 BOOT0 中断与唤醒再移除 handler，不卸载共享 GPIO ISR service。
     * @return ESP_OK 解注册成功或原本未登记
     *         ESP_ERR_INVALID_STATE BSP 未初始化
     *         其他 ESP-IDF 错误码表示 GPIO 操作失败
     */
    esp_err_t key_unregister_boot_isr_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_KEY_H */
