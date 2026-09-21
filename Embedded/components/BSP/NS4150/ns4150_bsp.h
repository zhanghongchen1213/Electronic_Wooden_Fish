/**
 * @file     ns4150_bsp.h
 * @brief    NS4150 功放安全控制接口
 * @details  封装板级 PA_EN 极性、初始化安全电平及幂等启停，避免上层直接操作 GPIO。
 * @author   ZHC
 * @date     2026-07-10
 */

#ifndef LEGBOT_NS4150_BSP_H
#define LEGBOT_NS4150_BSP_H

#include <stdbool.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** NS4150B 从关断切换到工作态的等待时间；在数据手册典型 120 ms 基础上保留 30 ms 裕量。 */
#define NS4150_BSP_STARTUP_SETTLE_MS 150U

    /**
     * @brief 获取 NS4150 功放 BSP 资源描述
     * @return NS4150 资源描述指针
     */
    const legbot_bsp_resource_t *ns4150_bsp_resource(void);

    /**
     * @brief 初始化 PA_EN 并立即保持数据手册规定的 shutdown 电平
     * @return ESP_OK 成功或已初始化，其他值表示 GPIO 配置失败
     */
    esp_err_t ns4150_bsp_init(void);

    /**
     * @brief 在 codec/I2S 已就绪后开启功放
     * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 表示尚未初始化
     */
    esp_err_t ns4150_bsp_enable(void);

    /**
     * @brief 将功放切回安全关断电平
     * @return ESP_OK 成功或尚未初始化，其他值表示 GPIO 操作失败
     */
    esp_err_t ns4150_bsp_safe_off(void);

    /**
     * @brief 以安全关断状态释放功放逻辑所有权
     * @return ESP_OK 成功，其他值表示关断失败且状态保留以便重试
     */
    esp_err_t ns4150_bsp_deinit(void);

    /**
     * @brief 查询功放是否由本 BSP 标记为开启
     * @return true 已开启，false 已关断或尚未初始化
     */
    bool ns4150_bsp_is_enabled(void);

    /**
     * @brief 将 NS4150 GPIO 操作结果映射为稳定驱动错误码
     * @param err ESP-IDF GPIO 操作结果
     * @return DRV_NS4150_ 前缀的静态字符串
     */
    const char *ns4150_bsp_error_code(esp_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_NS4150_BSP_H */
