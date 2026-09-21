/**
 * @file     qmi8658c_bsp.c
 * @brief    QMI8658C IMU 模式与采样 BSP 实现
 * @details  在共享 I2C 短互斥外管理 WoM、CTRL9、INT1、锁定式加速度采样与兼容六轴诊断。
 * @author   ZHC
 * @date     2026-07-28
 */

#include "qmi8658c_bsp.h"

#include <limits.h>
#include <stdatomic.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

static const char *TAG = "BSP_QMI8658";

/** QMI8658C WHO_AM_I 寄存器。 */
#define QMI8658C_WHO_AM_I_REGISTER 0x00
/** QMI8658C 芯片修订版本寄存器。 */
#define QMI8658C_REVISION_ID_REGISTER 0x01
/** QMI8658C CTRL1 寄存器。 */
#define QMI8658C_CTRL1_REGISTER 0x02
/** QMI8658C CTRL2 加速度配置寄存器。 */
#define QMI8658C_CTRL2_REGISTER 0x03
/** QMI8658C CTRL3 陀螺仪配置寄存器。 */
#define QMI8658C_CTRL3_REGISTER 0x04
/** QMI8658C CTRL7 传感器使能寄存器。 */
#define QMI8658C_CTRL7_REGISTER 0x08
/** QMI8658C CTRL8 运动与 CTRL9 握手配置寄存器。 */
#define QMI8658C_CTRL8_REGISTER 0x09
/** QMI8658C CTRL9 命令寄存器。 */
#define QMI8658C_CTRL9_REGISTER 0x0A
/** QMI8658C CAL1_L 参数寄存器。 */
#define QMI8658C_CAL1_L_REGISTER 0x0B
/** QMI8658C CAL1_H 参数寄存器。 */
#define QMI8658C_CAL1_H_REGISTER 0x0C
/** QMI8658C STATUSINT 锁定与 CTRL9 状态寄存器。 */
#define QMI8658C_STATUSINT_REGISTER 0x2D
/** QMI8658C STATUS1 活动状态寄存器。 */
#define QMI8658C_STATUS1_REGISTER 0x2F
/** QMI8658C 软复位结果寄存器。 */
#define QMI8658C_RESET_RESULT_REGISTER 0x4D
/** QMI8658C 软复位命令寄存器。 */
#define QMI8658C_RESET_REGISTER 0x60
/** QMI8658C 加速度 X 轴低字节起始寄存器。 */
#define QMI8658C_ACCEL_X_LOW_REGISTER 0x35
/** CTRL1：地址自增、little-endian、内部高速时钟开启。 */
#define QMI8658C_CTRL1_LITTLE_ENDIAN_AUTO_INCREMENT 0x40
/** CTRL2：±2g、低功耗 21Hz，供 WoM 使用。 */
#define QMI8658C_ACCEL_CONFIG_2G_LOW_POWER_21HZ 0x0D
/** CTRL2：±4g、normal mode 62.5Hz，供分类采样使用。 */
#define QMI8658C_ACCEL_CONFIG_4G_62_5HZ 0x17
/** CTRL2：±4g、normal mode 125Hz，六轴组合时约112.1Hz。 */
#define QMI8658C_ACCEL_CONFIG_4G_125HZ 0x16
/** CTRL3：±512dps、normal mode 112.1Hz。 */
#define QMI8658C_GYRO_CONFIG_512DPS_112HZ 0x56
/** CTRL7：关闭全部传感器。 */
#define QMI8658C_CTRL7_DISABLE_SENSORS 0x00
/** CTRL7：只开启加速度计。 */
#define QMI8658C_CTRL7_ENABLE_ACCEL 0x01
/** CTRL7：开启加速度计和陀螺仪。 */
#define QMI8658C_CTRL7_ENABLE_ACCEL_GYRO 0x03
/** CTRL7：启用锁定采样并只开启加速度计。 */
#define QMI8658C_CTRL7_SYNC_ACCEL 0x81
/** CTRL8：通过 STATUSINT.bit7 完成 CTRL9 握手。 */
#define QMI8658C_CTRL8_STATUSINT_HANDSHAKE 0x80
/** STATUSINT：CTRL9 命令完成位。 */
#define QMI8658C_STATUSINT_CMD_DONE_BIT 0x80
/** STATUSINT：锁定式样本已经锁定位。 */
#define QMI8658C_STATUSINT_LOCKED_BIT 0x02
/** STATUSINT：锁定式样本可用位。 */
#define QMI8658C_STATUSINT_AVAILABLE_BIT 0x01
/** STATUS1：Wake on Motion 事件位。 */
#define QMI8658C_STATUS1_WOM_BIT 0x04
/** 软复位完成结果。 */
#define QMI8658C_RESET_RESULT_DONE_BIT 0x80
/** QMI8658C 软复位命令值。 */
#define QMI8658C_RESET_COMMAND 0xB0
/** CTRL9：命令确认。 */
#define QMI8658C_CTRL9_COMMAND_ACK 0x00
/** CTRL9：配置或关闭 Wake on Motion。 */
#define QMI8658C_CTRL9_COMMAND_WOM 0x08
/** CTRL9：配置内部 AHB clock gating。 */
#define QMI8658C_CTRL9_COMMAND_AHB_CLOCK_GATING 0x12
/** 锁定式采样关闭 AHB clock gating 的 CAL1_L 参数。 */
#define QMI8658C_AHB_CLOCK_GATING_DISABLE 0x01
/** 锁定式采样退出时开启 AHB clock gating 的 CAL1_L 参数。 */
#define QMI8658C_AHB_CLOCK_GATING_ENABLE 0x00
/** INT1 初始低电平并忽略四个 WoM 样本。 */
#define QMI8658C_WOM_INT1_LOW_CAL1_H ((uint8_t)QMI8658C_BSP_WOM_BLANKING_SAMPLES)
/** 模式互斥最大等待时间。 */
#define QMI8658C_MODE_LOCK_TIMEOUT_MS 100U
/** 诊断六轴相邻新样本的最小等待时间。 */
#define QMI8658C_DIAGNOSTIC_SAMPLE_WAIT_MS 10U
/** CTRL9 状态轮询间隔。 */
#define QMI8658C_CTRL9_POLL_INTERVAL_MS 1U
/** STATUS1 读取后等待 INT1 恢复低电平的最长时间。 */
#define QMI8658C_INT1_DEASSERT_TIMEOUT_MS 5U
/** QMI8658C 软复位完成的最长等待时间。 */
#define QMI8658C_RESET_TIMEOUT_MS 100U

_Static_assert(QMI8658C_BSP_WOM_THRESHOLD_MG <= UINT8_MAX,
               "QMI8658C WoM threshold must fit CAL1_L");
_Static_assert(QMI8658C_BSP_WOM_BLANKING_SAMPLES <= 0x3FU,
               "QMI8658C WoM blanking must fit CAL1_H bits 5:0");
_Static_assert(configTICK_RATE_HZ >= 1000,
               "QMI8658C CTRL9 polling requires a 1 ms FreeRTOS tick");
/** manager 注入的共享 I2C 访问能力。 */
static legbot_bsp_i2c_access_t s_i2c_access;
/** QMI8658C device handle。 */
static i2c_master_dev_handle_t s_device;
/** 运行时确认的 QMI8658C 7-bit 地址。 */
static uint8_t s_address;
/** 最近一次 QMI8658C 状态。 */
static qmi8658c_status_t s_last_status = QMI8658C_STATUS_NOT_READY;
/** 当前 QMI8658C 模式，供无阻塞查询。 */
static atomic_int s_mode = QMI8658C_BSP_MODE_STANDBY;
/** INT1 单写者事件序号；ESP32-S3 对齐 32-bit 访问不会撕裂。 */
_Alignas(4) static volatile uint32_t s_wom_event_sequence;
/** 唯一任务消费者已经观察到的 INT1 事件序号。 */
static uint32_t s_wom_event_seen_sequence;
/** QMI8658C 初始化状态，供任务间无锁查询。 */
static atomic_bool s_initialized;
/** 上层唯一 INT1 ISR 唤醒回调。 */
static _Atomic(qmi8658c_bsp_wake_isr_callback_t) s_wake_callback;
/** 与 INT1 ISR 唤醒回调配对的上下文。 */
static _Atomic(void *) s_wake_callback_context;
/** GPIO41 handler 是否已经登记。 */
static bool s_interrupt_handler_installed;
/** GPIO41 中断能力是否可用于 WoM。 */
static bool s_interrupt_ready;
/** QMI 模式互斥锁静态存储。 */
static StaticSemaphore_t s_mode_mutex_storage;
/** QMI 模式互斥锁。 */
static SemaphoreHandle_t s_mode_mutex;

static esp_err_t find_device_locked(const legbot_bsp_i2c_access_t *access,
                                    i2c_master_dev_handle_t *device,
                                    uint8_t *address,
                                    qmi8658c_status_t *failure_status);
static esp_err_t configure_diagnostic_locked(i2c_master_dev_handle_t device);
static esp_err_t configure_interrupt(void);
static void remove_interrupt_handler(void);
static void qmi8658c_int1_isr(void *argument);
static void clear_wom_event(void);
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
static void log_wom_configuration_locked(void);
#endif
static esp_err_t set_mode_locked(qmi8658c_bsp_mode_t mode);
static esp_err_t stop_current_mode_locked(void);
static esp_err_t enter_wom_locked(void);
static esp_err_t enter_accel_classify_locked(bool reset_sampling_state);
static esp_err_t enter_diagnostic_locked(void);
static esp_err_t soft_reset_locked(void);
static esp_err_t ctrl9_command(uint8_t command);
static esp_err_t wait_statusint(bool cmd_done, int64_t deadline_us);
static esp_err_t transaction_write_before_deadline(
    uint8_t register_address,
    uint8_t value,
    int64_t deadline_us);
static esp_err_t transaction_read_before_deadline(
    uint8_t register_address,
    uint8_t *data,
    size_t data_size,
    int64_t deadline_us);
static uint32_t remaining_transaction_timeout_ms(int64_t deadline_us);
static esp_err_t transaction_write(uint8_t register_address, uint8_t value);
static esp_err_t transaction_read(uint8_t register_address, uint8_t *data, size_t data_size);
static esp_err_t write_register_locked(i2c_master_dev_handle_t device,
                                       uint8_t register_address,
                                       uint8_t value);
static esp_err_t read_register_locked(i2c_master_dev_handle_t device,
                                      uint8_t register_address,
                                      uint8_t *data,
                                      size_t data_size);
static esp_err_t take_mode_lock(void);
static void give_mode_lock(void);
static bool access_matches(const legbot_bsp_i2c_access_t *access);
static qmi8658c_status_t read_status_from_error(esp_err_t err);
static void set_failed_sample(qmi8658c_bsp_sample_t *sample,
                              qmi8658c_status_t status,
                              esp_err_t err);
static void set_failed_accel(qmi8658c_bsp_accel_sample_t *sample,
                             qmi8658c_status_t status,
                             esp_err_t err);
static esp_err_t read_diagnostic_sample_locked(qmi8658c_bsp_sample_t *sample);
static esp_err_t restore_mode_locked(qmi8658c_bsp_mode_t mode, esp_err_t operation_error);

const legbot_bsp_resource_t *qmi8658c_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_QMI8658C);
}

esp_err_t qmi8658c_bsp_init(const legbot_bsp_i2c_access_t *access)
{
    const legbot_bsp_resource_t *resource = qmi8658c_bsp_resource();
    if (access == NULL || access->bus_handle == NULL || access->acquire == NULL ||
        access->release == NULL)
    {
        s_last_status = QMI8658C_STATUS_INIT_FAILED;
        return ESP_ERR_INVALID_ARG;
    }
    if (resource == NULL || resource->i2c_port != LEGBOT_BSP_I2C_PORT ||
        resource->gpio_primary != LEGBOT_BSP_QMI8658C_INT1_GPIO ||
        resource->gpio_secondary != LEGBOT_BSP_QMI8658C_INT2_GPIO)
    {
        s_last_status = QMI8658C_STATUS_INIT_FAILED;
        return ESP_ERR_INVALID_STATE;
    }
    if (s_mode_mutex == NULL)
    {
        s_mode_mutex = xSemaphoreCreateMutexStatic(&s_mode_mutex_storage);
        if (s_mode_mutex == NULL)
        {
            s_last_status = QMI8658C_STATUS_INIT_FAILED;
            return ESP_ERR_NO_MEM;
        }
    }
    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        return err;
    }
    if (s_initialized)
    {
        bool matches = access_matches(access);
        give_mode_lock();
        return matches ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    err = access->acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        s_last_status = QMI8658C_STATUS_BUS_BUSY;
        give_mode_lock();
        return err;
    }
    if (s_device != NULL)
    {
        err = i2c_master_bus_rm_device(s_device);
        if (err != ESP_OK)
        {
            access->release();
            give_mode_lock();
            return err;
        }
        s_device = NULL;
        s_address = 0;
        memset(&s_i2c_access, 0, sizeof(s_i2c_access));
    }

    i2c_master_dev_handle_t device = NULL;
    uint8_t address = 0;
    qmi8658c_status_t failure_status = QMI8658C_STATUS_DEVICE_MISSING;
    err = find_device_locked(access, &device, &address, &failure_status);
    if (err == ESP_OK)
    {
        err = configure_diagnostic_locked(device);
        if (err != ESP_OK)
        {
            failure_status = err == ESP_ERR_TIMEOUT ? QMI8658C_STATUS_BUS_BUSY
                                                    : QMI8658C_STATUS_WRITE_FAILED;
        }
    }
    if (err != ESP_OK && device != NULL)
    {
        esp_err_t cleanup_err = i2c_master_bus_rm_device(device);
        if (cleanup_err == ESP_OK)
        {
            device = NULL;
        }
        else
        {
            s_i2c_access = *access;
            s_device = device;
            s_address = address;
            s_last_status = QMI8658C_STATUS_INIT_FAILED;
            access->release();
            give_mode_lock();
            ESP_LOGE(TAG,
                     "QMI8658C 初始化失败后清理 device handle 失败，错误=0x%x",
                     (unsigned)cleanup_err);
            return cleanup_err;
        }
    }
    access->release();

    if (err != ESP_OK)
    {
        s_last_status = failure_status;
        give_mode_lock();
        ESP_LOGW(TAG,
                 "QMI8658C 初始化失败，稳定错误码=%s，错误=0x%x",
                 qmi8658c_bsp_error_code(s_last_status),
                 (unsigned)err);
        return err;
    }

    s_i2c_access = *access;
    s_device = device;
    s_address = address;
    s_initialized = true;
    atomic_store(&s_mode, QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS);
    clear_wom_event();
    s_last_status = QMI8658C_STATUS_OK;

    esp_err_t interrupt_error = configure_interrupt();
    if (interrupt_error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "QMI8658C INT1 初始化失败，抬腕能力将保持关闭，错误=0x%x",
                 (unsigned)interrupt_error);
    }
    give_mode_lock();
    ESP_LOGI(TAG,
             "QMI8658C 初始化成功：地址=0x%02X，诊断六轴约112Hz，INT1=%s",
             s_address,
             s_interrupt_ready ? "可用" : "不可用");
    return ESP_OK;
}

esp_err_t qmi8658c_bsp_deinit(void)
{
    if (!s_initialized && s_device == NULL)
    {
        return ESP_OK;
    }
    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        return err;
    }

    esp_err_t first_error = ESP_OK;
    if (s_initialized)
    {
        first_error = set_mode_locked(QMI8658C_BSP_MODE_STANDBY);
    }
    remove_interrupt_handler();
    s_initialized = false;

    bool device_removed = s_device == NULL;
    if (s_device != NULL && s_i2c_access.acquire != NULL && s_i2c_access.release != NULL)
    {
        err = s_i2c_access.acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
        if (err == ESP_OK)
        {
            err = i2c_master_bus_rm_device(s_device);
            s_i2c_access.release();
            device_removed = err == ESP_OK;
        }
        if (first_error == ESP_OK)
        {
            first_error = err;
        }
    }
    if (device_removed)
    {
        s_device = NULL;
        s_address = 0;
        memset(&s_i2c_access, 0, sizeof(s_i2c_access));
        atomic_store(&s_mode, QMI8658C_BSP_MODE_STANDBY);
        clear_wom_event();
        s_last_status = QMI8658C_STATUS_NOT_READY;
    }
    give_mode_lock();
    return first_error;
}

bool qmi8658c_bsp_is_initialized(void)
{
    return atomic_load(&s_initialized);
}

uint8_t qmi8658c_bsp_address(void)
{
    return s_initialized ? s_address : 0;
}

qmi8658c_status_t qmi8658c_bsp_last_status(void)
{
    return s_last_status;
}

const char *qmi8658c_bsp_error_code(qmi8658c_status_t status)
{
    switch (status)
    {
    case QMI8658C_STATUS_OK:
        return QMI8658C_ERROR_OK;
    case QMI8658C_STATUS_NOT_READY:
        return QMI8658C_ERROR_NOT_READY;
    case QMI8658C_STATUS_DEVICE_MISSING:
        return QMI8658C_ERROR_DEVICE_MISSING;
    case QMI8658C_STATUS_ID_MISMATCH:
        return QMI8658C_ERROR_ID_MISMATCH;
    case QMI8658C_STATUS_BUS_BUSY:
        return QMI8658C_ERROR_BUS_BUSY;
    case QMI8658C_STATUS_INIT_FAILED:
        return QMI8658C_ERROR_INIT_FAILED;
    case QMI8658C_STATUS_READ_FAILED:
        return QMI8658C_ERROR_READ_FAILED;
    case QMI8658C_STATUS_WRITE_FAILED:
        return QMI8658C_ERROR_WRITE_FAILED;
    case QMI8658C_STATUS_MODE_FAILED:
        return QMI8658C_ERROR_MODE_FAILED;
    default:
        return QMI8658C_ERROR_INIT_FAILED;
    }
}

qmi8658c_bsp_mode_t qmi8658c_bsp_mode(void)
{
    return s_initialized
               ? (qmi8658c_bsp_mode_t)atomic_load(&s_mode)
               : QMI8658C_BSP_MODE_STANDBY;
}

esp_err_t qmi8658c_bsp_set_mode(qmi8658c_bsp_mode_t mode)
{
    if (mode < QMI8658C_BSP_MODE_STANDBY ||
        mode > QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        return err;
    }
    err = set_mode_locked(mode);
    give_mode_lock();
    return err;
}

esp_err_t qmi8658c_bsp_register_wake_isr_callback(
    qmi8658c_bsp_wake_isr_callback_t callback,
    void *context)
{
    if (callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const qmi8658c_bsp_wake_isr_callback_t registered =
        atomic_load_explicit(&s_wake_callback, memory_order_acquire);
    if (registered != NULL)
    {
        return registered == callback &&
                       atomic_load_explicit(&s_wake_callback_context,
                                            memory_order_relaxed) == context
                   ? ESP_OK
                   : ESP_ERR_INVALID_STATE;
    }
    atomic_store_explicit(&s_wake_callback_context,
                          context,
                          memory_order_relaxed);
    atomic_store_explicit(&s_wake_callback,
                          callback,
                          memory_order_release);
    return ESP_OK;
}

void qmi8658c_bsp_unregister_wake_isr_callback(void)
{
    atomic_store_explicit(&s_wake_callback, NULL, memory_order_release);
    atomic_store_explicit(&s_wake_callback_context,
                          NULL,
                          memory_order_relaxed);
}

bool qmi8658c_bsp_take_wom_event(void)
{
    uint32_t sequence = s_wom_event_sequence;
    if (sequence == s_wom_event_seen_sequence)
    {
        return false;
    }
    s_wom_event_seen_sequence = sequence;
    return true;
}

esp_err_t qmi8658c_bsp_poll_wom_status(bool *detected)
{
    if (detected == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *detected = false;
    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        return err;
    }
    if (!s_initialized ||
        atomic_load(&s_mode) != QMI8658C_BSP_MODE_WOM_MONITOR)
    {
        give_mode_lock();
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t status = 0;
    const int64_t started_us = esp_timer_get_time();
    const int64_t deadline_us =
        started_us +
        (int64_t)QMI8658C_INT1_DEASSERT_TIMEOUT_MS * 1000LL;
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    uint8_t first_status = 0;
    uint8_t statusint = 0;
    uint32_t status_read_count = 0U;
    const int int1_before_status =
        gpio_get_level(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    esp_err_t statusint_err = ESP_ERR_INVALID_STATE;
#endif
    err = gpio_intr_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    while (err == ESP_OK)
    {
        err = transaction_read(
            QMI8658C_STATUS1_REGISTER,
            &status,
            sizeof(status));
        if (err != ESP_OK)
        {
            break;
        }
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
        ++status_read_count;
        if (status_read_count == 1U)
        {
            first_status = status;
        }
#endif
        *detected = (status & QMI8658C_STATUS1_WOM_BIT) != 0U;
        if (*detected)
        {
            break;
        }
        if (gpio_get_level(LEGBOT_BSP_QMI8658C_INT1_GPIO) == 0)
        {
            err = gpio_intr_enable(
                LEGBOT_BSP_QMI8658C_INT1_GPIO);
            if (err != ESP_OK ||
                gpio_get_level(
                    LEGBOT_BSP_QMI8658C_INT1_GPIO) == 0)
            {
                break;
            }
            err = gpio_intr_disable(
                LEGBOT_BSP_QMI8658C_INT1_GPIO);
            if (err != ESP_OK)
            {
                break;
            }
        }
        if (esp_timer_get_time() >= deadline_us)
        {
            err = ESP_ERR_INVALID_STATE;
            break;
        }
        /*
         * STATUS1 首读与重武装之间可能到达新的 WoM。INT1 会保持高，
         * 必须再次读取 STATUS1 才能确认事件；单纯等待不会解除锁存。
         */
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
    statusint_err = transaction_read(
        QMI8658C_STATUSINT_REGISTER,
        &statusint,
        sizeof(statusint));
    if (int1_before_status != 0 ||
        status_read_count > 1U ||
        *detected ||
        err != ESP_OK)
    {
        ESP_LOGI(TAG,
                 "WoM 标定寄存器：INT1读前=%d，STATUS1首次/末次=0x%02x/0x%02x，读取次数=%lu，STATUSINT=0x%02x，STATUSINT错误=0x%x，INT1结束=%d，耗时=%lldus，事件=%u，结果=0x%x",
                 int1_before_status,
                 first_status,
                 status,
                 (unsigned long)status_read_count,
                 statusint,
                 (unsigned)statusint_err,
                 gpio_get_level(LEGBOT_BSP_QMI8658C_INT1_GPIO),
                 (long long)(esp_timer_get_time() - started_us),
                 (unsigned)*detected,
                 (unsigned)err);
        if (err != ESP_OK)
        {
            log_wom_configuration_locked();
        }
    }
#endif
    if (err == ESP_OK)
    {
        s_last_status = QMI8658C_STATUS_OK;
    }
    else
    {
        s_last_status = err == ESP_ERR_TIMEOUT
                            ? QMI8658C_STATUS_BUS_BUSY
                            : QMI8658C_STATUS_MODE_FAILED;
    }
    give_mode_lock();
    return err;
}

esp_err_t qmi8658c_bsp_read_locked_accel(qmi8658c_bsp_accel_sample_t *sample)
{
    if (sample == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    set_failed_accel(sample, QMI8658C_STATUS_NOT_READY, ESP_ERR_INVALID_STATE);
    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        set_failed_accel(sample, QMI8658C_STATUS_BUS_BUSY, err);
        return err;
    }
    if (!s_initialized ||
        atomic_load(&s_mode) != QMI8658C_BSP_MODE_ACCEL_CLASSIFY)
    {
        give_mode_lock();
        return ESP_ERR_INVALID_STATE;
    }

    uint8_t status = 0;
    err = transaction_read(QMI8658C_STATUSINT_REGISTER, &status, sizeof(status));
    if (err == ESP_OK &&
        (status & QMI8658C_STATUSINT_AVAILABLE_BIT) == 0U)
    {
        give_mode_lock();
        set_failed_accel(sample, QMI8658C_STATUS_OK, ESP_ERR_NOT_FINISHED);
        return ESP_ERR_NOT_FINISHED;
    }
    if (err == ESP_OK &&
        (status & QMI8658C_STATUSINT_LOCKED_BIT) == 0U)
    {
        /* 62.5Hz 的 Data_Lock_Delay 远小于 1ms，等待 1 tick 后直接 burst。 */
        vTaskDelay(pdMS_TO_TICKS(1U));
    }

    uint8_t data[6] = {0};
    if (err == ESP_OK)
    {
        err = transaction_read(QMI8658C_ACCEL_X_LOW_REGISTER, data, sizeof(data));
    }
    if (err != ESP_OK)
    {
        s_last_status = read_status_from_error(err);
        set_failed_accel(sample, s_last_status, err);
        give_mode_lock();
        return err;
    }

    *sample = (qmi8658c_bsp_accel_sample_t){
        .valid = true,
        .accel_x = qmi8658c_bsp_decode_le_i16(data[0], data[1]),
        .accel_y = qmi8658c_bsp_decode_le_i16(data[2], data[3]),
        .accel_z = qmi8658c_bsp_decode_le_i16(data[4], data[5]),
        .status = QMI8658C_STATUS_OK,
        .error_code = QMI8658C_ERROR_OK,
        .esp_error = ESP_OK,
    };
    s_last_status = QMI8658C_STATUS_OK;
    give_mode_lock();
    return ESP_OK;
}

esp_err_t qmi8658c_bsp_read_sample(qmi8658c_bsp_sample_t *sample)
{
    if (sample == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    set_failed_sample(sample, QMI8658C_STATUS_NOT_READY, ESP_ERR_INVALID_STATE);
    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        set_failed_sample(sample, QMI8658C_STATUS_BUS_BUSY, err);
        return err;
    }
    if (!s_initialized)
    {
        s_last_status = QMI8658C_STATUS_NOT_READY;
        give_mode_lock();
        return ESP_ERR_INVALID_STATE;
    }

    qmi8658c_bsp_mode_t previous_mode = (qmi8658c_bsp_mode_t)atomic_load(&s_mode);
    if (previous_mode != QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS)
    {
        err = set_mode_locked(QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS);
    }
    if (err == ESP_OK)
    {
        vTaskDelay(pdMS_TO_TICKS(QMI8658C_DIAGNOSTIC_SAMPLE_WAIT_MS));
        err = read_diagnostic_sample_locked(sample);
    }
    err = restore_mode_locked(previous_mode, err);
    give_mode_lock();
    return err;
}

esp_err_t qmi8658c_bsp_read_continuous(qmi8658c_bsp_continuous_report_t *report)
{
    if (report == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(report, 0, sizeof(*report));
    report->first_error = ESP_OK;

    esp_err_t err = take_mode_lock();
    if (err != ESP_OK)
    {
        return err;
    }
    if (!s_initialized)
    {
        give_mode_lock();
        return ESP_ERR_INVALID_STATE;
    }
    qmi8658c_bsp_mode_t previous_mode = (qmi8658c_bsp_mode_t)atomic_load(&s_mode);
    if (previous_mode != QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS)
    {
        err = set_mode_locked(QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS);
        if (err != ESP_OK)
        {
            give_mode_lock();
            return err;
        }
    }

    for (size_t index = 0; index < QMI8658C_BSP_DIAGNOSTIC_SAMPLE_COUNT; ++index)
    {
        /*
         * 诊断 ODR 约 112 Hz；每次读取前等待 10 ms，确保三次尝试来自
         * 不同输出周期，同时不占用共享 I2C 短事务锁。
         */
        vTaskDelay(pdMS_TO_TICKS(QMI8658C_DIAGNOSTIC_SAMPLE_WAIT_MS));
        err = read_diagnostic_sample_locked(&report->samples[index]);
        report->count = index + 1;
        if (err != ESP_OK && report->first_error == ESP_OK)
        {
            report->first_error = err;
        }
    }
    report->first_error = restore_mode_locked(previous_mode, report->first_error);
    give_mode_lock();
    return report->first_error;
}

int16_t qmi8658c_bsp_decode_le_i16(uint8_t low_byte, uint8_t high_byte)
{
    uint16_t raw = ((uint16_t)high_byte << 8) | low_byte;
    return raw <= INT16_MAX ? (int16_t)raw : (int16_t)((int32_t)raw - 65536);
}

static esp_err_t find_device_locked(const legbot_bsp_i2c_access_t *access,
                                    i2c_master_dev_handle_t *device,
                                    uint8_t *address,
                                    qmi8658c_status_t *failure_status)
{
    const uint8_t candidates[] = {
        QMI8658C_BSP_I2C_ADDRESS_HIGH,
        QMI8658C_BSP_I2C_ADDRESS_LOW,
    };
    esp_err_t selected_error = ESP_ERR_NOT_FOUND;

    for (size_t index = 0; index < sizeof(candidates); ++index)
    {
        uint8_t candidate = candidates[index];
        esp_err_t err = i2c_master_probe(access->bus_handle,
                                         candidate,
                                         LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS);
        if (err == ESP_ERR_NOT_FOUND)
        {
            continue;
        }
        if (err != ESP_OK)
        {
            selected_error = err;
            *failure_status = err == ESP_ERR_TIMEOUT ? QMI8658C_STATUS_BUS_BUSY
                                                     : QMI8658C_STATUS_READ_FAILED;
            continue;
        }

        const i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = candidate,
            .scl_speed_hz = LEGBOT_BSP_I2C_FREQ_HZ,
            .scl_wait_us = 0,
        };
        i2c_master_dev_handle_t candidate_device = NULL;
        err = i2c_master_bus_add_device(access->bus_handle, &device_config, &candidate_device);
        if (err != ESP_OK)
        {
            selected_error = err;
            *failure_status = QMI8658C_STATUS_INIT_FAILED;
            continue;
        }

        uint8_t chip_id = 0;
        err = read_register_locked(candidate_device,
                                   QMI8658C_WHO_AM_I_REGISTER,
                                   &chip_id,
                                   sizeof(chip_id));
        if (err == ESP_OK && chip_id == QMI8658C_BSP_WHO_AM_I_VALUE)
        {
            *device = candidate_device;
            *address = candidate;
            return ESP_OK;
        }

        esp_err_t remove_err = i2c_master_bus_rm_device(candidate_device);
        if (remove_err != ESP_OK)
        {
            *device = candidate_device;
            *address = candidate;
            *failure_status = QMI8658C_STATUS_INIT_FAILED;
            return remove_err;
        }
        if (err != ESP_OK)
        {
            selected_error = err;
            *failure_status = err == ESP_ERR_TIMEOUT ? QMI8658C_STATUS_BUS_BUSY
                                                     : QMI8658C_STATUS_READ_FAILED;
        }
        else
        {
            selected_error = ESP_ERR_INVALID_RESPONSE;
            *failure_status = QMI8658C_STATUS_ID_MISMATCH;
        }
    }
    return selected_error;
}

static esp_err_t configure_diagnostic_locked(i2c_master_dev_handle_t device)
{
    const struct
    {
        uint8_t register_address; /**< 配置寄存器地址。 */
        uint8_t value;            /**< 数据手册定义的配置值。 */
    } configurations[] = {
        {QMI8658C_CTRL1_REGISTER, QMI8658C_CTRL1_LITTLE_ENDIAN_AUTO_INCREMENT},
        {QMI8658C_CTRL2_REGISTER, QMI8658C_ACCEL_CONFIG_4G_125HZ},
        {QMI8658C_CTRL3_REGISTER, QMI8658C_GYRO_CONFIG_512DPS_112HZ},
        {QMI8658C_CTRL8_REGISTER, QMI8658C_CTRL8_STATUSINT_HANDSHAKE},
        {QMI8658C_CTRL7_REGISTER, QMI8658C_CTRL7_ENABLE_ACCEL_GYRO},
    };
    for (size_t index = 0; index < sizeof(configurations) / sizeof(configurations[0]); ++index)
    {
        esp_err_t err = write_register_locked(device,
                                              configurations[index].register_address,
                                              configurations[index].value);
        if (err != ESP_OK)
        {
            return err;
        }
    }
    return ESP_OK;
}

static esp_err_t configure_interrupt(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << LEGBOT_BSP_QMI8658C_INT1_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK)
    {
        return err;
    }
    err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        return err;
    }
    err = gpio_isr_handler_add(LEGBOT_BSP_QMI8658C_INT1_GPIO, qmi8658c_int1_isr, NULL);
    if (err != ESP_OK)
    {
        return err;
    }
    s_interrupt_handler_installed = true;
    err = gpio_intr_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    if (err != ESP_OK)
    {
        (void)gpio_isr_handler_remove(LEGBOT_BSP_QMI8658C_INT1_GPIO);
        s_interrupt_handler_installed = false;
        return err;
    }
    s_interrupt_ready = true;
    return ESP_OK;
}

static void remove_interrupt_handler(void)
{
    s_interrupt_ready = false;
    clear_wom_event();
    (void)gpio_intr_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    (void)gpio_wakeup_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    if (s_interrupt_handler_installed)
    {
        (void)gpio_isr_handler_remove(LEGBOT_BSP_QMI8658C_INT1_GPIO);
        s_interrupt_handler_installed = false;
    }
}

static void qmi8658c_int1_isr(void *argument)
{
    (void)argument;
    /* INT1 为保持高电平，先停中断，任务读取 STATUS1 后再重武装。 */
    (void)gpio_intr_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    /*
     * GPIO41 ISR 是唯一写者，任务只读取并保存 last-seen；即使交错，
     * 任务也只会本次或下次看到新序号，不会用清零覆盖 ISR 事件。
     */
    ++s_wom_event_sequence;
    const qmi8658c_bsp_wake_isr_callback_t callback =
        atomic_load_explicit(&s_wake_callback, memory_order_acquire);
    if (callback != NULL)
    {
        callback(atomic_load_explicit(&s_wake_callback_context,
                                      memory_order_relaxed));
    }
}

static void clear_wom_event(void)
{
    s_wom_event_seen_sequence = s_wom_event_sequence;
}

#if CONFIG_LEGBOT_WRIST_RAISE_CALIBRATION_TRACE
static void log_wom_configuration_locked(void)
{
    static const uint8_t registers[] = {
        QMI8658C_WHO_AM_I_REGISTER,
        QMI8658C_REVISION_ID_REGISTER,
        QMI8658C_CTRL1_REGISTER,
        QMI8658C_CTRL2_REGISTER,
        QMI8658C_CTRL7_REGISTER,
        QMI8658C_CTRL8_REGISTER,
        QMI8658C_CTRL9_REGISTER,
        QMI8658C_CAL1_L_REGISTER,
        QMI8658C_CAL1_H_REGISTER,
    };
    uint8_t values[sizeof(registers)] = {0};
    esp_err_t first_error = ESP_OK;
    for (size_t index = 0; index < sizeof(registers); ++index)
    {
        const esp_err_t read_error =
            transaction_read(registers[index],
                             &values[index],
                             sizeof(values[index]));
        if (first_error == ESP_OK && read_error != ESP_OK)
        {
            first_error = read_error;
        }
    }
    ESP_LOGI(TAG,
             "WoM 标定配置：WHO=0x%02x，REV=0x%02x，CTRL1/2/7/8/9=0x%02x/0x%02x/0x%02x/0x%02x/0x%02x，CAL1_L/H=0x%02x/0x%02x，读取错误=0x%x",
             values[0],
             values[1],
             values[2],
             values[3],
             values[4],
             values[5],
             values[6],
             values[7],
             values[8],
             (unsigned)first_error);
}
#endif

static esp_err_t set_mode_locked(qmi8658c_bsp_mode_t mode)
{
    if (!s_initialized || s_device == NULL)
    {
        s_last_status = QMI8658C_STATUS_NOT_READY;
        return ESP_ERR_INVALID_STATE;
    }
    const qmi8658c_bsp_mode_t previous_mode =
        (qmi8658c_bsp_mode_t)atomic_load(&s_mode);
    if (previous_mode == mode)
    {
        return ESP_OK;
    }

    esp_err_t err = stop_current_mode_locked();
    if (err == ESP_OK)
    {
        switch (mode)
        {
        case QMI8658C_BSP_MODE_STANDBY:
            break;
        case QMI8658C_BSP_MODE_WOM_MONITOR:
            err = enter_wom_locked();
            break;
        case QMI8658C_BSP_MODE_ACCEL_CLASSIFY:
            err = enter_accel_classify_locked(
                previous_mode != QMI8658C_BSP_MODE_WOM_MONITOR);
            break;
        case QMI8658C_BSP_MODE_DIAGNOSTIC_6AXIS:
            err = enter_diagnostic_locked();
            break;
        default:
            err = ESP_ERR_INVALID_ARG;
            break;
        }
    }
    if (err != ESP_OK)
    {
        (void)gpio_intr_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
        (void)gpio_wakeup_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
        clear_wom_event();
        const esp_err_t recovery_err =
            transaction_write(QMI8658C_CTRL7_REGISTER,
                              QMI8658C_CTRL7_DISABLE_SENSORS);
        if (recovery_err != ESP_OK)
        {
            /*
             * 关闭传感器也失败时硬件状态未知，强制下次访问先移除并
             * 重建 device handle，不能继续把软件 STANDBY 当成事实。
             */
            remove_interrupt_handler();
            s_initialized = false;
        }
        atomic_store(&s_mode, QMI8658C_BSP_MODE_STANDBY);
        s_last_status = QMI8658C_STATUS_MODE_FAILED;
        ESP_LOGW(TAG,
                 "QMI8658C 模式切换失败，目标=%d，稳定错误码=%s，错误=0x%x，恢复错误=0x%x",
                 (int)mode,
                 QMI8658C_ERROR_MODE_FAILED,
                 (unsigned)err,
                 (unsigned)recovery_err);
        return recovery_err != ESP_OK ? recovery_err : err;
    }

    atomic_store(&s_mode, mode);
    s_last_status = QMI8658C_STATUS_OK;
    return ESP_OK;
}

static esp_err_t stop_current_mode_locked(void)
{
    qmi8658c_bsp_mode_t current = (qmi8658c_bsp_mode_t)atomic_load(&s_mode);
    if (current == QMI8658C_BSP_MODE_STANDBY)
    {
        return ESP_OK;
    }
    if (current == QMI8658C_BSP_MODE_WOM_MONITOR)
    {
        (void)gpio_intr_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
        (void)gpio_wakeup_disable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
        clear_wom_event();
    }

    esp_err_t err = transaction_write(QMI8658C_CTRL7_REGISTER,
                                      QMI8658C_CTRL7_DISABLE_SENSORS);
    if (err == ESP_OK && current == QMI8658C_BSP_MODE_WOM_MONITOR)
    {
        err = transaction_write(QMI8658C_CAL1_L_REGISTER, 0U);
        if (err == ESP_OK)
        {
            err = transaction_write(QMI8658C_CAL1_H_REGISTER,
                                    QMI8658C_WOM_INT1_LOW_CAL1_H);
        }
        if (err == ESP_OK)
        {
            err = ctrl9_command(QMI8658C_CTRL9_COMMAND_WOM);
        }
    }
    else if (err == ESP_OK && current == QMI8658C_BSP_MODE_ACCEL_CLASSIFY)
    {
        err = transaction_write(QMI8658C_CAL1_L_REGISTER,
                                QMI8658C_AHB_CLOCK_GATING_ENABLE);
        if (err == ESP_OK)
        {
            err = ctrl9_command(QMI8658C_CTRL9_COMMAND_AHB_CLOCK_GATING);
        }
    }
    if (err == ESP_OK)
    {
        atomic_store(&s_mode, QMI8658C_BSP_MODE_STANDBY);
    }
    return err;
}

static esp_err_t enter_wom_locked(void)
{
    if (!s_interrupt_ready)
    {
        esp_err_t interrupt_err = configure_interrupt();
        if (interrupt_err != ESP_OK)
        {
            return interrupt_err;
        }
    }
    /*
     * 实测 REV=0x7B 从六轴模式直接切入 WoM 后会出现 INT1 拉高但
     * STATUS1.WoM 不置位；进入 WoM 前必须先复位内部运动状态机。
     */
    esp_err_t err = soft_reset_locked();
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL8_REGISTER,
                                      QMI8658C_CTRL8_STATUSINT_HANDSHAKE);
    }
    if (err == ESP_OK)
    {
        /*
         * 上一次分类若在关闭 AHB clock gating 后异常退出，进入低功耗
         * WoM 前必须显式恢复，不能依赖复位默认值。
         */
        err = transaction_write(QMI8658C_CAL1_L_REGISTER,
                                QMI8658C_AHB_CLOCK_GATING_ENABLE);
    }
    if (err == ESP_OK)
    {
        err = ctrl9_command(QMI8658C_CTRL9_COMMAND_AHB_CLOCK_GATING);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL2_REGISTER,
                                QMI8658C_ACCEL_CONFIG_2G_LOW_POWER_21HZ);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CAL1_L_REGISTER,
                                (uint8_t)QMI8658C_BSP_WOM_THRESHOLD_MG);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CAL1_H_REGISTER,
                                QMI8658C_WOM_INT1_LOW_CAL1_H);
    }
    if (err == ESP_OK)
    {
        err = ctrl9_command(QMI8658C_CTRL9_COMMAND_WOM);
    }
    uint8_t status = 0;
    if (err == ESP_OK)
    {
        err = transaction_read(QMI8658C_STATUS1_REGISTER, &status, sizeof(status));
    }
    if (err == ESP_OK)
    {
        clear_wom_event();
        err = gpio_wakeup_enable(LEGBOT_BSP_QMI8658C_INT1_GPIO,
                                 GPIO_INTR_HIGH_LEVEL);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL7_REGISTER, QMI8658C_CTRL7_ENABLE_ACCEL);
    }
    if (err == ESP_OK)
    {
        err = gpio_intr_enable(LEGBOT_BSP_QMI8658C_INT1_GPIO);
    }
    return err;
}

static esp_err_t soft_reset_locked(void)
{
    esp_err_t err = transaction_write(QMI8658C_RESET_REGISTER,
                                      QMI8658C_RESET_COMMAND);
    const int64_t deadline_us =
        esp_timer_get_time() +
        (int64_t)QMI8658C_RESET_TIMEOUT_MS * 1000LL;
    while (err == ESP_OK)
    {
        uint8_t result = 0;
        err = transaction_read(QMI8658C_RESET_RESULT_REGISTER,
                               &result,
                               sizeof(result));
        if (err != ESP_OK)
        {
            break;
        }
        if ((result & QMI8658C_RESET_RESULT_DONE_BIT) != 0U)
        {
            return transaction_write(
                QMI8658C_CTRL1_REGISTER,
                QMI8658C_CTRL1_LITTLE_ENDIAN_AUTO_INCREMENT);
        }
        if (esp_timer_get_time() >= deadline_us)
        {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    return err;
}

static esp_err_t enter_accel_classify_locked(bool reset_sampling_state)
{
    /*
     * REV=0x7B 从诊断模式退出后直接启用 SyncSample 会保持
     * STATUSINT.Avail=0；待机路径先复位内部采样状态机。WoM 退出路径
     * 已由运动状态机提供有效时钟，重复复位反而会破坏后续采样。
     */
    esp_err_t err =
        reset_sampling_state ? soft_reset_locked() : ESP_OK;
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL8_REGISTER,
                                QMI8658C_CTRL8_STATUSINT_HANDSHAKE);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL2_REGISTER,
                                QMI8658C_ACCEL_CONFIG_4G_62_5HZ);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CAL1_L_REGISTER,
                                QMI8658C_AHB_CLOCK_GATING_DISABLE);
    }
    if (err == ESP_OK)
    {
        err = ctrl9_command(QMI8658C_CTRL9_COMMAND_AHB_CLOCK_GATING);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL7_REGISTER,
                                QMI8658C_CTRL7_SYNC_ACCEL);
    }
    return err;
}

static esp_err_t enter_diagnostic_locked(void)
{
    esp_err_t err = transaction_write(QMI8658C_CTRL2_REGISTER,
                                      QMI8658C_ACCEL_CONFIG_4G_125HZ);
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL3_REGISTER,
                                QMI8658C_GYRO_CONFIG_512DPS_112HZ);
    }
    if (err == ESP_OK)
    {
        err = transaction_write(QMI8658C_CTRL7_REGISTER,
                                QMI8658C_CTRL7_ENABLE_ACCEL_GYRO);
    }
    return err;
}

static esp_err_t ctrl9_command(uint8_t command)
{
    const int64_t deadline_us =
        esp_timer_get_time() +
        (int64_t)QMI8658C_BSP_CTRL9_TIMEOUT_MS * 1000;
    uint8_t status = 0;
    esp_err_t err = transaction_read_before_deadline(
        QMI8658C_STATUSINT_REGISTER,
        &status,
        sizeof(status),
        deadline_us);
    if (err == ESP_OK && (status & QMI8658C_STATUSINT_CMD_DONE_BIT) != 0U)
    {
        err = transaction_write_before_deadline(
            QMI8658C_CTRL9_REGISTER,
            QMI8658C_CTRL9_COMMAND_ACK,
            deadline_us);
        if (err == ESP_OK)
        {
            err = wait_statusint(false, deadline_us);
        }
    }
    if (err == ESP_OK)
    {
        err = transaction_write_before_deadline(
            QMI8658C_CTRL9_REGISTER,
            command,
            deadline_us);
    }
    if (err == ESP_OK)
    {
        err = wait_statusint(true, deadline_us);
    }
    if (err == ESP_OK)
    {
        err = transaction_write_before_deadline(
            QMI8658C_CTRL9_REGISTER,
            QMI8658C_CTRL9_COMMAND_ACK,
            deadline_us);
    }
    if (err == ESP_OK)
    {
        err = wait_statusint(false, deadline_us);
    }
    return err;
}

static esp_err_t wait_statusint(bool cmd_done, int64_t deadline_us)
{
    for (;;)
    {
        uint8_t status = 0;
        esp_err_t err = transaction_read_before_deadline(
            QMI8658C_STATUSINT_REGISTER,
            &status,
            sizeof(status),
            deadline_us);
        if (err != ESP_OK)
        {
            return err;
        }
        if (((status & QMI8658C_STATUSINT_CMD_DONE_BIT) != 0U) == cmd_done)
        {
            return ESP_OK;
        }
        if (remaining_transaction_timeout_ms(deadline_us) <=
            QMI8658C_CTRL9_POLL_INTERVAL_MS)
        {
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(QMI8658C_CTRL9_POLL_INTERVAL_MS));
    }
}

static esp_err_t transaction_write_before_deadline(
    uint8_t register_address,
    uint8_t value,
    int64_t deadline_us)
{
    if (!s_initialized || s_device == NULL ||
        s_i2c_access.acquire == NULL || s_i2c_access.release == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t timeout_ms = remaining_transaction_timeout_ms(deadline_us);
    if (timeout_ms == 0U)
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err =
        s_i2c_access.acquire(pdMS_TO_TICKS(timeout_ms));
    if (err != ESP_OK)
    {
        return err;
    }
    timeout_ms = remaining_transaction_timeout_ms(deadline_us);
    if (timeout_ms == 0U)
    {
        s_i2c_access.release();
        return ESP_ERR_TIMEOUT;
    }
    const uint8_t data[] = {register_address, value};
    err = i2c_master_transmit(
        s_device,
        data,
        sizeof(data),
        timeout_ms);
    s_i2c_access.release();
    if (err == ESP_OK &&
        remaining_transaction_timeout_ms(deadline_us) == 0U)
    {
        return ESP_ERR_TIMEOUT;
    }
    return err;
}

static esp_err_t transaction_read_before_deadline(
    uint8_t register_address,
    uint8_t *data,
    size_t data_size,
    int64_t deadline_us)
{
    if (!s_initialized || s_device == NULL || data == NULL ||
        data_size == 0U || s_i2c_access.acquire == NULL ||
        s_i2c_access.release == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t timeout_ms = remaining_transaction_timeout_ms(deadline_us);
    if (timeout_ms == 0U)
    {
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err =
        s_i2c_access.acquire(pdMS_TO_TICKS(timeout_ms));
    if (err != ESP_OK)
    {
        return err;
    }
    timeout_ms = remaining_transaction_timeout_ms(deadline_us);
    if (timeout_ms == 0U)
    {
        s_i2c_access.release();
        return ESP_ERR_TIMEOUT;
    }
    err = i2c_master_transmit_receive(
        s_device,
        &register_address,
        1,
        data,
        data_size,
        timeout_ms);
    s_i2c_access.release();
    if (err == ESP_OK &&
        remaining_transaction_timeout_ms(deadline_us) == 0U)
    {
        return ESP_ERR_TIMEOUT;
    }
    return err;
}

static uint32_t remaining_transaction_timeout_ms(int64_t deadline_us)
{
    const int64_t remaining_us = deadline_us - esp_timer_get_time();
    if (remaining_us <= 0)
    {
        return 0U;
    }
    uint64_t remaining_ms =
        (uint64_t)remaining_us / 1000U;
    if (remaining_ms > LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS)
    {
        remaining_ms = LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS;
    }
    return (uint32_t)remaining_ms;
}

static esp_err_t transaction_write(uint8_t register_address, uint8_t value)
{
    if (!s_initialized || s_device == NULL ||
        s_i2c_access.acquire == NULL || s_i2c_access.release == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = s_i2c_access.acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = write_register_locked(s_device, register_address, value);
        s_i2c_access.release();
    }
    return err;
}

static esp_err_t transaction_read(uint8_t register_address, uint8_t *data, size_t data_size)
{
    if (!s_initialized || s_device == NULL ||
        s_i2c_access.acquire == NULL || s_i2c_access.release == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = s_i2c_access.acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = read_register_locked(s_device, register_address, data, data_size);
        s_i2c_access.release();
    }
    return err;
}

static esp_err_t write_register_locked(i2c_master_dev_handle_t device,
                                       uint8_t register_address,
                                       uint8_t value)
{
    const uint8_t write_data[] = {register_address, value};
    return i2c_master_transmit(device,
                               write_data,
                               sizeof(write_data),
                               LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS);
}

static esp_err_t read_register_locked(i2c_master_dev_handle_t device,
                                      uint8_t register_address,
                                      uint8_t *data,
                                      size_t data_size)
{
    return i2c_master_transmit_receive(device,
                                       &register_address,
                                       sizeof(register_address),
                                       data,
                                       data_size,
                                       LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS);
}

static esp_err_t take_mode_lock(void)
{
    if (s_mode_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xSemaphoreTake(s_mode_mutex, pdMS_TO_TICKS(QMI8658C_MODE_LOCK_TIMEOUT_MS)) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

static void give_mode_lock(void)
{
    if (s_mode_mutex != NULL)
    {
        (void)xSemaphoreGive(s_mode_mutex);
    }
}

static bool access_matches(const legbot_bsp_i2c_access_t *access)
{
    return s_i2c_access.bus_handle == access->bus_handle &&
           s_i2c_access.acquire == access->acquire &&
           s_i2c_access.release == access->release;
}

static qmi8658c_status_t read_status_from_error(esp_err_t err)
{
    return err == ESP_ERR_TIMEOUT ? QMI8658C_STATUS_BUS_BUSY : QMI8658C_STATUS_READ_FAILED;
}

static void set_failed_sample(qmi8658c_bsp_sample_t *sample,
                              qmi8658c_status_t status,
                              esp_err_t err)
{
    *sample = (qmi8658c_bsp_sample_t){
        .valid = false,
        .address = s_address,
        .status = status,
        .error_code = qmi8658c_bsp_error_code(status),
        .esp_error = err,
    };
}

static void set_failed_accel(qmi8658c_bsp_accel_sample_t *sample,
                             qmi8658c_status_t status,
                             esp_err_t err)
{
    *sample = (qmi8658c_bsp_accel_sample_t){
        .valid = false,
        .status = status,
        .error_code = qmi8658c_bsp_error_code(status),
        .esp_error = err,
    };
}

static esp_err_t read_diagnostic_sample_locked(qmi8658c_bsp_sample_t *sample)
{
    uint8_t data[12] = {0};
    esp_err_t err = transaction_read(QMI8658C_ACCEL_X_LOW_REGISTER, data, sizeof(data));
    if (err != ESP_OK)
    {
        s_last_status = read_status_from_error(err);
        set_failed_sample(sample, s_last_status, err);
        ESP_LOGW(TAG,
                 "QMI8658C 六轴读取失败，稳定错误码=%s，错误=0x%x",
                 sample->error_code,
                 (unsigned)err);
        return err;
    }
    *sample = (qmi8658c_bsp_sample_t){
        .valid = true,
        .address = s_address,
        .accel_x = qmi8658c_bsp_decode_le_i16(data[0], data[1]),
        .accel_y = qmi8658c_bsp_decode_le_i16(data[2], data[3]),
        .accel_z = qmi8658c_bsp_decode_le_i16(data[4], data[5]),
        .gyro_x = qmi8658c_bsp_decode_le_i16(data[6], data[7]),
        .gyro_y = qmi8658c_bsp_decode_le_i16(data[8], data[9]),
        .gyro_z = qmi8658c_bsp_decode_le_i16(data[10], data[11]),
        .status = QMI8658C_STATUS_OK,
        .error_code = QMI8658C_ERROR_OK,
        .esp_error = ESP_OK,
    };
    s_last_status = QMI8658C_STATUS_OK;
    return ESP_OK;
}

static esp_err_t restore_mode_locked(qmi8658c_bsp_mode_t mode, esp_err_t operation_error)
{
    if ((qmi8658c_bsp_mode_t)atomic_load(&s_mode) == mode)
    {
        return operation_error;
    }
    esp_err_t restore_error = set_mode_locked(mode);
    if (restore_error != ESP_OK)
    {
        if (operation_error != ESP_OK)
        {
            ESP_LOGW(TAG,
                     "QMI8658C 诊断读取与模式恢复均失败，读取错误=0x%x，恢复错误=0x%x",
                     (unsigned)operation_error,
                     (unsigned)restore_error);
        }
        return restore_error;
    }
    return operation_error;
}
