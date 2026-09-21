/**
 * @file     qmi8658c_bsp.h
 * @brief    QMI8658C IMU 模式与采样 BSP 接口
 * @details  运行时识别板级地址，管理低功耗 WoM、锁定式加速度采样和兼容六轴诊断。
 * @author   ZHC
 * @date     2026-07-28
 */

#ifndef LEGBOT_QMI8658C_BSP_H
#define LEGBOT_QMI8658C_BSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** QMI8658C SA0 高或悬空时的 7-bit 地址。 */
#define QMI8658C_BSP_I2C_ADDRESS_HIGH 0x6A
/** QMI8658C SA0 低时的 7-bit 地址。 */
#define QMI8658C_BSP_I2C_ADDRESS_LOW 0x6B
/** QMI8658C WHO_AM_I 期望值。 */
#define QMI8658C_BSP_WHO_AM_I_VALUE 0x05
/** 自检连续读取固定样本数量。 */
#define QMI8658C_BSP_DIAGNOSTIC_SAMPLE_COUNT 3
/** WoM 默认阈值，单位为 mg。 */
#define QMI8658C_BSP_WOM_THRESHOLD_MG 32U
/** WoM 启用后忽略的加速度样本数量。 */
#define QMI8658C_BSP_WOM_BLANKING_SAMPLES 4U
/** CTRL9 CmdDone 与 ACK 各自的最大等待时间。 */
#define QMI8658C_BSP_CTRL9_TIMEOUT_MS 50U
/** 三次六轴读取、最坏双向模式切换与模式锁的累计超时预算。 */
#define QMI8658C_BSP_CONTINUOUS_TIMEOUT_BUDGET_MS \
    (100U + \
     9U * QMI8658C_BSP_CTRL9_TIMEOUT_MS + \
     QMI8658C_BSP_DIAGNOSTIC_SAMPLE_COUNT * \
         (10U + 2U * LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS))
/** QMI8658C 成功稳定码。 */
#define QMI8658C_ERROR_OK "DRV_QMI8658_OK"
/** QMI8658C 未初始化稳定错误码。 */
#define QMI8658C_ERROR_NOT_READY "DRV_QMI8658_NOT_READY"
/** QMI8658C 设备缺失稳定错误码。 */
#define QMI8658C_ERROR_DEVICE_MISSING "DRV_QMI8658_DEVICE_MISSING"
/** QMI8658C 芯片 ID 不匹配稳定错误码。 */
#define QMI8658C_ERROR_ID_MISMATCH "DRV_QMI8658_ID_MISMATCH"
/** QMI8658C 总线忙稳定错误码。 */
#define QMI8658C_ERROR_BUS_BUSY "DRV_QMI8658_BUS_BUSY"
/** QMI8658C 初始化失败稳定错误码。 */
#define QMI8658C_ERROR_INIT_FAILED "DRV_QMI8658_INIT_FAILED"
/** QMI8658C 读取失败稳定错误码。 */
#define QMI8658C_ERROR_READ_FAILED "DRV_QMI8658_READ_FAILED"
/** QMI8658C 写入失败稳定错误码。 */
#define QMI8658C_ERROR_WRITE_FAILED "DRV_QMI8658_WRITE_FAILED"
/** QMI8658C 模式切换失败稳定错误码。 */
#define QMI8658C_ERROR_MODE_FAILED "DRV_QMI8658_MODE_FAILED"

    typedef enum
    {
        QMI8658C_STATUS_OK = 0,         /**< 驱动操作成功。 */
        QMI8658C_STATUS_NOT_READY,      /**< 驱动尚未初始化。 */
        QMI8658C_STATUS_DEVICE_MISSING, /**< 两个 strap 地址均无设备应答。 */
        QMI8658C_STATUS_ID_MISMATCH,    /**< 地址有应答但 WHO_AM_I 不是 0x05。 */
        QMI8658C_STATUS_BUS_BUSY,       /**< 共享总线忙或有界等待超时。 */
        QMI8658C_STATUS_INIT_FAILED,    /**< device handle 或基础配置失败。 */
        QMI8658C_STATUS_READ_FAILED,    /**< 寄存器读事务失败。 */
        QMI8658C_STATUS_WRITE_FAILED,   /**< 寄存器写事务失败。 */
        QMI8658C_STATUS_MODE_FAILED,    /**< WoM、锁定采样或诊断模式切换失败。 */
    } qmi8658c_status_t;

    typedef enum
    {
        QMI8658C_BSP_MODE_STANDBY = 0,       /**< 关闭加速度计和陀螺仪。 */
        QMI8658C_BSP_MODE_WOM_MONITOR,       /**< 21 Hz 低功耗 WoM 监测。 */
        QMI8658C_BSP_MODE_ACCEL_CLASSIFY,    /**< 62.5 Hz 锁定式加速度分类采样。 */
        QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS,  /**< 兼容既有自检的约 112 Hz 六轴采样。 */
    } qmi8658c_bsp_mode_t;

    typedef struct
    {
        bool valid;               /**< true 表示锁定式加速度样本有效。 */
        int16_t accel_x;          /**< X 轴原始加速度，±4g 时 8192 LSB/g。 */
        int16_t accel_y;          /**< Y 轴原始加速度，±4g 时 8192 LSB/g。 */
        int16_t accel_z;          /**< Z 轴原始加速度，±4g 时 8192 LSB/g。 */
        qmi8658c_status_t status; /**< 项目级驱动状态。 */
        const char *error_code;   /**< DRV_QMI8658_ 前缀稳定错误码。 */
        esp_err_t esp_error;      /**< 原始 ESP-IDF 错误码。 */
    } qmi8658c_bsp_accel_sample_t;

    typedef struct
    {
        bool valid;               /**< true 表示六轴原始样本有效。 */
        uint8_t address;          /**< 运行时确认的实际 7-bit 地址。 */
        int16_t accel_x;          /**< X 轴原始加速度。 */
        int16_t accel_y;          /**< Y 轴原始加速度。 */
        int16_t accel_z;          /**< Z 轴原始加速度。 */
        int16_t gyro_x;           /**< X 轴原始角速度。 */
        int16_t gyro_y;           /**< Y 轴原始角速度。 */
        int16_t gyro_z;           /**< Z 轴原始角速度。 */
        qmi8658c_status_t status; /**< 项目级驱动状态。 */
        const char *error_code;   /**< DRV_QMI8658_ 前缀稳定错误码。 */
        esp_err_t esp_error;      /**< 原始 ESP-IDF 错误码。 */
    } qmi8658c_bsp_sample_t;

    typedef struct
    {
        qmi8658c_bsp_sample_t samples[QMI8658C_BSP_DIAGNOSTIC_SAMPLE_COUNT]; /**< 三次六轴尝试结果。 */
        size_t count;                                                        /**< 已完成的读取尝试数。 */
        esp_err_t first_error;                                               /**< 首个失败，全部成功为 ESP_OK。 */
    } qmi8658c_bsp_continuous_report_t;

    /** QMI INT1 中断上下文回调；实现只能调用 ISR-safe API。 */
    typedef void (*qmi8658c_bsp_wake_isr_callback_t)(void *context);

    /**
     * @brief 获取 QMI8658C IMU BSP 资源描述
     * @return QMI8658C 资源描述指针
     */
    const legbot_bsp_resource_t *qmi8658c_bsp_resource(void);

    /**
     * @brief 使用 manager-owned I2C 访问能力幂等初始化 QMI8658C
     * @param access I2C0 bus handle 与短互斥入口
     * @return ESP_OK 成功或已初始化
     *         ESP_ERR_NOT_FOUND 两个地址均无设备
     *         ESP_ERR_INVALID_RESPONSE 地址有设备但 WHO_AM_I 不匹配
     *         其他 ESP-IDF 错误码表示总线、device handle 或基础配置失败
     */
    esp_err_t qmi8658c_bsp_init(const legbot_bsp_i2c_access_t *access);

    /**
     * @brief 幂等释放 QMI8658C device handle
     * @return ESP_OK 成功或未初始化
     *         其他 ESP-IDF 错误码表示 device handle 移除失败
     */
    esp_err_t qmi8658c_bsp_deinit(void);

    /**
     * @brief 查询 QMI8658C 是否已经初始化
     * @return true 已初始化
     *         false 未初始化
     */
    bool qmi8658c_bsp_is_initialized(void);

    /**
     * @brief 获取运行时确认的 QMI8658C 地址
     * @return 初始化成功时返回 0x6A 或 0x6B，否则返回 0
     */
    uint8_t qmi8658c_bsp_address(void);

    /**
     * @brief 获取最近一次初始化或读取状态
     * @return 项目级 QMI8658C 状态
     */
    qmi8658c_status_t qmi8658c_bsp_last_status(void);

    /**
     * @brief 获取指定状态的稳定错误码
     * @param status 项目级 QMI8658C 状态
     * @return DRV_QMI8658_ 前缀稳定错误码
     */
    const char *qmi8658c_bsp_error_code(qmi8658c_status_t status);

    /**
     * @brief 获取当前 QMI8658C 工作模式
     * @return 当前模式；未初始化时为 STANDBY
     */
    qmi8658c_bsp_mode_t qmi8658c_bsp_mode(void);

    /**
     * @brief 幂等切换 QMI8658C 工作模式
     * @param mode 目标模式
     * @return ESP_OK 切换成功或已经处于目标模式
     *         ESP_ERR_INVALID_STATE 驱动未初始化
     *         ESP_ERR_INVALID_ARG 模式无效
     *         其他 ESP-IDF 错误码表示 GPIO、总线或 CTRL9 失败
     */
    esp_err_t qmi8658c_bsp_set_mode(qmi8658c_bsp_mode_t mode);

    /**
     * @brief 登记唯一 QMI INT1 上层唤醒回调
     * @details 回调仅用于唤醒 owner task，真实 WoM 仍由任务上下文读 STATUS1 确认。
     * @param callback 中断上下文回调，不得访问 I2C 或阻塞
     * @param context 原样传回给 callback 的上下文
     * @return ESP_OK 登记成功或已登记同一回调
     *         ESP_ERR_INVALID_ARG 回调为空
     *         ESP_ERR_INVALID_STATE 已被其他所有者占用
     */
    esp_err_t qmi8658c_bsp_register_wake_isr_callback(
        qmi8658c_bsp_wake_isr_callback_t callback,
        void *context);

    /**
     * @brief 解注册 QMI INT1 上层唤醒回调
     * @details 不改变 QMI 工作模式或 GPIO handler；owner 应先退出 WoM 再调用。
     */
    void qmi8658c_bsp_unregister_wake_isr_callback(void);

    /**
     * @brief 取走 INT1 ISR 记录的最新 WoM 候选事件
     * @return true 自上次取走后至少发生过一次 INT1 上升沿
     *         false 没有待处理事件
     */
    bool qmi8658c_bsp_take_wom_event(void);

    /**
     * @brief 禁用 INT1 后读取 STATUS1，确认并清除 WoM 状态
     * @param detected 输出 STATUS1.bit2 是否为 1
     * @return ESP_OK 读取成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE 当前不在 WoM 模式
     *         其他 ESP-IDF 错误码表示总线或读事务失败
     * @note 检出事件时 INT1 保持禁用，供调用方立即切换分类模式；
     *       未检出时仅在线路已经恢复低电平后重新启用上升沿。
     */
    esp_err_t qmi8658c_bsp_poll_wom_status(bool *detected);

    /**
     * @brief 在 ACCEL_CLASSIFY 模式读取一组锁定式加速度样本
     * @param sample 加速度原始值和错误上下文输出
     * @return ESP_OK 读取成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE 当前不在分类模式
     *         ESP_ERR_NOT_FINISHED 尚无新样本
     *         其他 ESP-IDF 错误码表示总线或读事务失败
     */
    esp_err_t qmi8658c_bsp_read_locked_accel(qmi8658c_bsp_accel_sample_t *sample);

    /**
     * @brief 读取六轴原始样本
     * @param sample 六轴原始值和错误上下文输出
     * @return ESP_OK 读取成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         ESP_ERR_INVALID_STATE 驱动尚未初始化
     *         其他 ESP-IDF 错误码表示总线或读事务失败
     */
    esp_err_t qmi8658c_bsp_read_sample(qmi8658c_bsp_sample_t *sample);

    /**
     * @brief 为硬件自检连续执行三次有界六轴原始读取
     * @param report 三次尝试结果与首个错误输出
     * @return ESP_OK 三次读取全部成功
     *         ESP_ERR_INVALID_ARG 输出指针无效
     *         其他 ESP-IDF 错误码为三次尝试中的首个失败
     */
    esp_err_t qmi8658c_bsp_read_continuous(qmi8658c_bsp_continuous_report_t *report);

    /**
     * @brief 解码 QMI8658C little-endian two's-complement 原始值
     * @param low_byte 低字节
     * @param high_byte 高字节
     * @return 解码后的 int16_t 值
     */
    int16_t qmi8658c_bsp_decode_le_i16(uint8_t low_byte, uint8_t high_byte);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_QMI8658C_BSP_H */
