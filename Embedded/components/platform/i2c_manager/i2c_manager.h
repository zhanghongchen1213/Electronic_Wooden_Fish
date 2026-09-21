/**
 * @file     i2c_manager.h
 * @brief    共享 I2C0 总线管理接口
 * @details  独占 I2C0 生命周期，并提供受控设备挂载、短互斥、有界事务和预期设备扫描入口。
 * @author   ZHC
 * @date     2026-07-10
 */

#ifndef LEGBOT_I2C_MANAGER_H
#define LEGBOT_I2C_MANAGER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 共享 I2C 单次事务的默认超时。 */
#define I2C_MANAGER_DEFAULT_TIMEOUT_MS LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS
/** 预期逻辑设备扫描表条目数量。 */
#define I2C_MANAGER_EXPECTED_DEVICE_COUNT 4
/** 扫描五个物理地址及 QMI8658C WHO_AM_I 验证的累计超时预算。 */
#define I2C_MANAGER_SCAN_TIMEOUT_BUDGET_MS \
    (11 * I2C_MANAGER_DEFAULT_TIMEOUT_MS)

    typedef enum
    {
        I2C_MANAGER_STATUS_PRESENT = 0,         /**< 设备存在且指定访问成功。 */
        I2C_MANAGER_STATUS_DEVICE_MISSING,      /**< 地址没有设备应答。 */
        I2C_MANAGER_STATUS_BUS_BUSY_OR_TIMEOUT, /**< 总线忙或有界等待超时。 */
        I2C_MANAGER_STATUS_READ_FAILED,         /**< 设备读事务失败。 */
        I2C_MANAGER_STATUS_WRITE_FAILED,        /**< 设备写事务失败。 */
        I2C_MANAGER_STATUS_MANAGER_NOT_READY,   /**< I2C manager 尚未初始化。 */
    } i2c_manager_status_t;

    typedef struct
    {
        const char *device_name;     /**< 稳定设备名称。 */
        uint8_t address;             /**< 设备 7-bit 地址。 */
        uint8_t alternate_address;   /**< 逻辑设备的备用 7-bit 地址，无备用地址时为 0。 */
        i2c_manager_status_t status; /**< 分类后的访问状态。 */
        esp_err_t esp_error;         /**< 原始 ESP-IDF 错误码。 */
    } i2c_manager_result_t;

    typedef struct
    {
        i2c_manager_result_t devices[I2C_MANAGER_EXPECTED_DEVICE_COUNT]; /**< 预期设备扫描结果。 */
        size_t count;                                                    /**< 有效扫描结果数量。 */
    } i2c_manager_scan_report_t;

    /**
     * @brief 初始化共享 I2C0 总线和互斥锁
     * @return ESP_OK 成功或已初始化
     *         ESP_ERR_NO_MEM 互斥锁或 driver 资源创建失败
     *         其他 ESP-IDF 错误码表示 I2C master bus 初始化失败
     */
    esp_err_t i2c_manager_init(void);

    /**
     * @brief 释放共享 I2C0 总线
     * @return ESP_OK 成功或未初始化
     *         ESP_ERR_INVALID_STATE 仍有 device handle 挂载在总线上
     *         其他 ESP-IDF 错误码表示 driver 释放失败
     */
    esp_err_t i2c_manager_deinit(void);

    /**
     * @brief 查询 I2C manager 是否已经初始化
     * @return true 已初始化
     *         false 未初始化
     */
    bool i2c_manager_is_initialized(void);

    /**
     * @brief 获取 manager-owned I2C master bus handle
     * @return 已初始化时返回总线句柄，否则返回 NULL
     */
    i2c_master_bus_handle_t i2c_manager_bus_handle(void);

    /**
     * @brief 获取供 BSP 芯片封装使用的 manager-owned bus 与短互斥入口
     * @param access I2C 访问能力输出
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE manager 尚未初始化
     */
    esp_err_t i2c_manager_get_access(legbot_bsp_i2c_access_t *access);

    /**
     * @brief 获取共享 I2C 总线互斥访问权
     * @param timeout_ticks 等待互斥锁的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE I2C manager 尚未初始化
     *         ESP_ERR_TIMEOUT 等待互斥锁超时
     */
    esp_err_t i2c_manager_acquire(TickType_t timeout_ticks);

    /**
     * @brief 释放共享 I2C 总线互斥访问权
     */
    void i2c_manager_release(void);

    /**
     * @brief 在 manager-owned bus 上挂载 7-bit I2C 设备
     * @param address 设备 7-bit 地址
     * @param scl_speed_hz 设备 SCL 频率
     * @param device 输出 device handle
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE manager 尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         其他 ESP-IDF 错误码表示设备挂载失败
     */
    esp_err_t i2c_manager_add_device(uint8_t address,
                                     uint32_t scl_speed_hz,
                                     i2c_master_dev_handle_t *device);

    /**
     * @brief 从 manager-owned bus 移除设备
     * @param device 待移除的 device handle
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_ARG 参数无效
     *         其他 ESP-IDF 错误码表示设备移除失败
     */
    esp_err_t i2c_manager_remove_device(i2c_master_dev_handle_t device);

    /**
     * @brief 在有界等待内探测一个 7-bit 地址
     * @param address 设备 7-bit 地址
     * @param timeout_ms 总线操作超时，必须大于 0
     * @param result 分类结果输出
     * @return ESP_OK 表示完成分类，具体状态见 result
     *         ESP_ERR_INVALID_ARG 参数无效
     */
    esp_err_t i2c_manager_probe(uint8_t address, int timeout_ms, i2c_manager_result_t *result);

    /**
     * @brief 执行有界写事务
     * @param device I2C device handle
     * @param address 设备 7-bit 地址，用于诊断
     * @param data 待写数据
     * @param data_size 待写数据长度
     * @param timeout_ms 总线操作超时，必须大于 0
     * @param result 分类结果输出
     * @return ESP_OK 写成功，其他值为原始 ESP-IDF 错误码
     */
    esp_err_t i2c_manager_transmit(i2c_master_dev_handle_t device,
                                   uint8_t address,
                                   const uint8_t *data,
                                   size_t data_size,
                                   int timeout_ms,
                                   i2c_manager_result_t *result);

    /**
     * @brief 执行无 STOP 间隔的有界写后读事务
     * @param device I2C device handle
     * @param address 设备 7-bit 地址，用于诊断
     * @param write_data 待读前写入的数据
     * @param write_size 待读前写入的数据长度
     * @param read_data 读取缓冲区
     * @param read_size 读取长度
     * @param timeout_ms 总线操作超时，必须大于 0
     * @param result 分类结果输出
     * @return ESP_OK 读取成功，其他值为原始 ESP-IDF 错误码
     */
    esp_err_t i2c_manager_transmit_receive(i2c_master_dev_handle_t device,
                                           uint8_t address,
                                           const uint8_t *write_data,
                                           size_t write_size,
                                           uint8_t *read_data,
                                           size_t read_size,
                                           int timeout_ms,
                                           i2c_manager_result_t *result);

    /**
     * @brief 有界扫描项目预期 I2C 设备并输出中文诊断
     * @param report 扫描结果输出
     * @return ESP_OK 扫描流程完成，单设备结果见 report
     *         ESP_ERR_INVALID_ARG 参数无效
     */
    esp_err_t i2c_manager_scan_expected(i2c_manager_scan_report_t *report);

    /**
     * @brief 获取 I2C 分类状态稳定名称
     * @param status I2C 分类状态
     * @return ASCII 稳定名称
     */
    const char *i2c_manager_status_name(i2c_manager_status_t status);

    /**
     * @brief 获取 I2C 分类状态稳定错误码
     * @param status I2C 分类状态
     * @return DRV_I2C_ 前缀稳定错误码
     */
    const char *i2c_manager_error_code(i2c_manager_status_t status);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_I2C_MANAGER_H */
