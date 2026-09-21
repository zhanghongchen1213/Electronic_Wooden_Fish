/**
 * @file     ml307r_bsp.h
 * @brief    ML307R 4G 模块 BSP 资源与使能接口
 * @details  提供 ML307R 蜂窝模块资源查询和 IO10 高低电平门禁，统一管理 AT UART 与板级使能边界。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef LEGBOT_ML307R_BSP_H
#define LEGBOT_ML307R_BSP_H

#include <stdint.h>

#include "bsp_resources.h"

/** ML307R 从使能到 UART 可用的最小等待时间。 */
#define ML307R_BSP_UART_READY_DELAY_MS 10000U

/**
 * @brief 获取 ML307R 4G 模块 BSP 资源描述
 * @return ML307R 资源描述指针
 */
const legbot_bsp_resource_t *ml307r_bsp_resource(void);

/**
 * @brief 初始化并使能 ML307R 模块
 * @details 配置 IO10 为无上下拉的普通输出并驱动高电平；生产运行期不主动拉低。
 * @return ESP_OK IO10 已稳定保持主动高电平，其他值表示 GPIO 配置或写入失败
 */
esp_err_t ml307r_bsp_enable(void);

/**
 * @brief 在禁用 4G 的固件 profile 中保持 ML307R 关闭
 * @details 配置 IO10 为无上下拉的普通输出并驱动低电平；仅供启动阶段固定禁用，
 *          启用 4G 的运行期不得用本接口切换模块电源。
 * @return ESP_OK IO10 已稳定保持关闭电平，其他值表示 GPIO 配置或写入失败
 */
esp_err_t ml307r_bsp_hold_disabled(void);

/**
 * @brief 查询 ML307R UART 距离允许首条 AT 还需等待的时间
 * @param wait_ms 输出剩余等待毫秒数，已经到期时为 0
 * @return ESP_OK 已记录有效使能时刻，ESP_ERR_INVALID_ARG 表示参数为空，
 *         ESP_ERR_INVALID_STATE 表示 ML307R 尚未由 BSP 使能
 */
esp_err_t ml307r_bsp_uart_ready_wait_ms(uint32_t *wait_ms);

#endif /* LEGBOT_ML307R_BSP_H */
