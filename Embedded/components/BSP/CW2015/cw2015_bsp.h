/**
 * @file     cw2015_bsp.h
 * @brief    CW2015 电量计项目 BSP 接口
 * @details  通过 manager-owned I2C0 读取 SOC，并用有效标志与稳定错误码区分真实 0% 和通信失败。
 *          CW2015 是 Cellwise 公司生产的 LiPo 电量计芯片，兼容 MAX17048/MAX17049 封装。
 * @author   ZHC
 * @date     2026-07-14
 */

#ifndef LEGBOT_CW2015_BSP_H
#define LEGBOT_CW2015_BSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** CW2015 默认 7-bit I2C 地址。 */
#define CW2015_BSP_I2C_ADDRESS 0x62
/** 自检连续读取固定样本数量。 */
#define CW2015_BSP_DIAGNOSTIC_SAMPLE_COUNT 3
/** 三次 SOC 读取的共享锁与 I2C 事务累计超时预算。 */
#define CW2015_BSP_CONTINUOUS_TIMEOUT_BUDGET_MS \
    (CW2015_BSP_DIAGNOSTIC_SAMPLE_COUNT * 2 * LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS)
/** CW2015 成功稳定码。 */
#define CW2015_ERROR_OK "DRV_CW2015_OK"
/** CW2015 未初始化稳定错误码。 */
#define CW2015_ERROR_NOT_READY "DRV_CW2015_NOT_READY"
/** CW2015 设备缺失稳定错误码。 */
#define CW2015_ERROR_DEVICE_MISSING "DRV_CW2015_DEVICE_MISSING"
/** CW2015 总线忙稳定错误码。 */
#define CW2015_ERROR_BUS_BUSY "DRV_CW2015_BUS_BUSY"
/** CW2015 初始化失败稳定错误码。 */
#define CW2015_ERROR_INIT_FAILED "DRV_CW2015_INIT_FAILED"
/** CW2015 读取失败稳定错误码。 */
#define CW2015_ERROR_READ_FAILED "DRV_CW2015_READ_FAILED"

    typedef enum
    {
        CW2015_STATUS_OK = 0,         /**< SOC 读取成功。 */
        CW2015_STATUS_NOT_READY,      /**< 驱动尚未初始化。 */
        CW2015_STATUS_DEVICE_MISSING, /**< 设备没有应答。 */
        CW2015_STATUS_BUS_BUSY,       /**< 共享总线忙或等待超时。 */
        CW2015_STATUS_INIT_FAILED,    /**< 设备句柄创建或通信验证失败。 */
        CW2015_STATUS_READ_FAILED,    /**< SOC 寄存器读取失败。 */
    } cw2015_status_t;

    typedef struct
    {
        bool valid;                /**< true 表示本次 SOC 有效。 */
        float percent;             /**< 解码并限制到 0-100 的 SOC 百分比。 */
        uint16_t raw_soc;          /**< SOC 寄存器原始 16-bit 大端值。 */
        cw2015_status_t status;    /**< 项目级读取状态。 */
        const char *error_code;    /**< DRV_CW2015_ 前缀稳定错误码。 */
        esp_err_t esp_error;       /**< 原始 ESP-IDF 错误码。 */
    } cw2015_bsp_soc_t;

    typedef struct
    {
        cw2015_bsp_soc_t samples[CW2015_BSP_DIAGNOSTIC_SAMPLE_COUNT]; /**< 三次 SOC 尝试结果。 */
        size_t count;                                                  /**< 已完成的读取尝试数。 */
        esp_err_t first_error;                                         /**< 首个失败，全部成功为 ESP_OK。 */
    } cw2015_bsp_continuous_report_t;

    /**
     * @brief 获取 CW2015 电量计 BSP 资源描述
     * @return CW2015 资源描述指针
     */
    const legbot_bsp_resource_t *cw2015_bsp_resource(void);

    /**
     * @brief 使用 manager-owned I2C 访问能力幂等初始化 CW2015
     * @param access I2C0 bus handle 与短互斥入口
     * @return ESP_OK 成功或已初始化
     *         ESP_ERR_NOT_FOUND 设备没有应答
     *         其他 ESP-IDF 错误码表示初始化或通信验证失败
     */
    esp_err_t cw2015_bsp_init(const legbot_bsp_i2c_access_t *access);

    /**
     * @brief 幂等释放 CW2015 device handle
     * @return ESP_OK 成功或未初始化
     *         其他 ESP-IDF 错误码表示 device handle 移除失败
     */
    esp_err_t cw2015_bsp_deinit(void);

    /**
     * @brief 查询 CW2015 是否已经初始化
     * @return true 已初始化
     *         false 未初始化
     */
    bool cw2015_bsp_is_initialized(void);

    /**
     * @brief 读取 CW2015 SOC
     * @param soc SOC、有效性和错误上下文输出
     * @return ESP_OK 读取成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE 驱动尚未初始化
     *         其他 ESP-IDF 错误码表示总线或读事务失败
     */
    esp_err_t cw2015_bsp_read_soc(cw2015_bsp_soc_t *soc);

    /**
     * @brief 为硬件自检连续执行三次有界 SOC 读取
     * @param report 三次尝试结果与首个错误输出
     * @return ESP_OK 三次读取全部成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         其他 ESP-IDF 错误码为三次尝试中的首个失败
     */
    esp_err_t cw2015_bsp_read_continuous(cw2015_bsp_continuous_report_t *report);

    /**
     * @brief 按 1%/256 分辨率解码并限制 SOC 原始值
     * @param raw_soc SOC 寄存器原始 16-bit 值
     * @return 0-100 的 SOC 百分比
     */
    float cw2015_bsp_decode_soc(uint16_t raw_soc);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CW2015_BSP_H */
