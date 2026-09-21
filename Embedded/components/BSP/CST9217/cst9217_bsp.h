/**
 * @file     cst9217_bsp.h
 * @brief    CST9217 触摸项目 BSP 接口
 * @details  固化触摸尺寸、I2C0、复位和中断资源，并封装读点与幂等恢复能力。
 * @author   ZHC
 * @date     2026-07-10
 */

#ifndef LEGBOT_CST9217_BSP_H
#define LEGBOT_CST9217_BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** CST9217 映射区域宽度。 */
#define CST9217_BSP_WIDTH 410
/** CST9217 映射区域高度。 */
#define CST9217_BSP_HEIGHT 502

    /**
     * @brief CST9217 触摸中断回调
     * @details 回调在 GPIO ISR 上下文执行，只能调用 ISR 安全 API，不得访问 I2C。
     * @param user_ctx 注册时提供的调用方上下文
     */
    typedef void (*cst9217_bsp_interrupt_cb_t)(void *user_ctx);

    typedef struct
    {
        uint16_t x;        /**< 触摸 X 坐标。 */
        uint16_t y;        /**< 触摸 Y 坐标。 */
        uint16_t strength; /**< 触摸强度。 */
        bool pressed;      /**< 当前是否存在有效触点。 */
    } cst9217_bsp_point_t;

    /**
     * @brief 获取 CST9217 触摸 BSP 资源描述
     * @return CST9217 资源描述指针
     */
    const legbot_bsp_resource_t *cst9217_bsp_resource(void);

    /**
     * @brief 使用 manager-owned bus handle 幂等初始化 CST9217 触摸链路
     * @param bus_handle 共享 I2C0 master bus handle
     * @return ESP_OK 成功或已初始化
     *         ESP_ERR_INVALID_STATE 共享 I2C0 资源不匹配
     *         其他错误表示 panel IO 或触摸控制器初始化失败
     */
    esp_err_t cst9217_bsp_init(i2c_master_bus_handle_t bus_handle);

    /**
     * @brief 释放 CST9217 触摸链路资源
     * @return ESP_OK 成功或未初始化
     *         其他错误表示底层资源释放失败
     */
    esp_err_t cst9217_bsp_deinit(void);

    /**
     * @brief 读取单个触摸点
     * @param point 触摸点输出
     * @return ESP_OK 读取成功，无触摸时 pressed 为 false
     *         ESP_ERR_INVALID_STATE 触摸未初始化
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         其他错误表示 I2C 读点失败
     */
    esp_err_t cst9217_bsp_read_point(cst9217_bsp_point_t *point);

    /**
     * @brief 注册或解注册 CST9217 GPIO 中断回调
     * @details 不向 CST9217 写入任何休眠或私有寄存器，仅使用
     *          esp_lcd_touch 已有的 GPIO 中断入口。不允许直接替换已注册的不同回调。
     * @param callback ISR 安全回调；NULL 表示解注册
     * @param user_ctx 调用方上下文；callback 为 NULL 时必须为 NULL
     * @return ESP_OK 注册、幂等重复注册或解注册成功
     *         ESP_ERR_INVALID_ARG 解注册时仍传入用户上下文
     *         ESP_ERR_INVALID_STATE 触摸未初始化或尝试替换已注册回调
     *         其他错误表示 GPIO ISR 注册失败
     */
    esp_err_t cst9217_bsp_register_interrupt_callback(
        cst9217_bsp_interrupt_cb_t callback,
        void *user_ctx);

    /**
     * @brief 释放旧触摸句柄并重新初始化控制器
     * @return ESP_OK 恢复成功
     *         其他错误表示释放或重建失败
     */
    esp_err_t cst9217_bsp_reset(void);

    /**
     * @brief 查询 CST9217 触摸链路是否已经初始化
     * @return true 已初始化
     *         false 未初始化
     */
    bool cst9217_bsp_is_initialized(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CST9217_BSP_H */
