/**
 * @file     i2c_manager.c
 * @brief    共享 I2C0 总线管理实现
 * @details  使用 ESP-IDF new-driver bus/device 模型独占 I2C0，并提供短互斥与有界诊断访问。
 * @author   ZHC
 * @date     2026-07-10
 */

#include "i2c_manager.h"

#include <string.h>

#include "bsp_resources.h"
#include "esp_log.h"
#include "freertos/semphr.h"

static const char *TAG = "PLAT_I2C";

/** I2C 输入去毛刺周期，ESP-IDF 官方示例推荐典型值 7。 */
#define I2C_MANAGER_GLITCH_IGNORE_COUNT 7
/** QMI8658C WHO_AM_I 寄存器。 */
#define I2C_MANAGER_QMI8658_WHO_AM_I_REGISTER 0x00
/** QMI8658C WHO_AM_I 期望值。 */
#define I2C_MANAGER_QMI8658_WHO_AM_I_VALUE 0x05

typedef struct
{
    const char *name;          /**< 稳定设备名称。 */
    uint8_t address;           /**< 主 7-bit 地址。 */
    uint8_t alternate_address; /**< 可选的备用 7-bit 地址。 */
    bool verify_qmi8658_id;    /**< 是否需要校验 QMI8658C 芯片 ID。 */
} i2c_expected_device_t;

/** 项目预期 I2C 逻辑设备表，QMI8658C 的两个 strap 地址聚合为一个结果。 */
static const i2c_expected_device_t s_expected_devices[I2C_MANAGER_EXPECTED_DEVICE_COUNT] = {
    {.name = "ES8311", .address = 0x18, .alternate_address = 0, .verify_qmi8658_id = false},
    {.name = "CW2015", .address = 0x62, .alternate_address = 0, .verify_qmi8658_id = false},
    {.name = "CST9217", .address = 0x5A, .alternate_address = 0, .verify_qmi8658_id = false},
    {.name = "QMI8658C", .address = 0x6A, .alternate_address = 0x6B, .verify_qmi8658_id = true},
};

/** 共享 I2C 总线互斥锁。 */
static SemaphoreHandle_t s_i2c_mutex;
/** manager-owned I2C master bus handle。 */
static i2c_master_bus_handle_t s_bus_handle;
/** 扫描清理失败时保留的临时 device handle，供下一次扫描或反初始化重试。 */
static i2c_master_dev_handle_t s_scan_cleanup_device;
/** I2C manager 初始化状态。 */
static bool s_initialized;

static i2c_manager_status_t map_probe_status(esp_err_t err);
static i2c_manager_status_t map_transaction_status(esp_err_t err, bool read_operation);
static void set_result(i2c_manager_result_t *result,
                       const char *device_name,
                       uint8_t address,
                       i2c_manager_status_t status,
                       esp_err_t err);
static esp_err_t add_device_locked(uint8_t address,
                                   uint32_t scl_speed_hz,
                                   i2c_master_dev_handle_t *device);
static esp_err_t remove_device_locked(i2c_master_dev_handle_t device);
static void verify_expected_qmi(const i2c_expected_device_t *expected,
                                i2c_manager_result_t *result);
static void log_scan_result(const i2c_manager_result_t *result);

esp_err_t i2c_manager_init(void)
{
    if (s_initialized)
    {
        return ESP_OK;
    }

    const i2c_master_bus_config_t bus_config = {
        .i2c_port = LEGBOT_BSP_I2C_PORT,
        .sda_io_num = LEGBOT_BSP_I2C_SDA_GPIO,
        .scl_io_num = LEGBOT_BSP_I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = I2C_MANAGER_GLITCH_IGNORE_COUNT,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
            .allow_pd = false,
        },
    };
    i2c_master_bus_handle_t bus_handle = NULL;
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
    if (err != ESP_OK)
    {
        return err;
    }

    SemaphoreHandle_t mutex = s_i2c_mutex;
    if (mutex == NULL)
    {
        mutex = xSemaphoreCreateMutex();
    }
    if (mutex == NULL)
    {
        /* 互斥锁创建失败时回收已创建总线，避免残留第二 owner。 */
        (void)i2c_del_master_bus(bus_handle);
        return ESP_ERR_NO_MEM;
    }

    s_i2c_mutex = mutex;
    s_bus_handle = bus_handle;
    s_initialized = true;
    ESP_LOGI(TAG,
             "共享 I2C0 manager 初始化成功：SDA=IO%d，SCL=IO%d，频率=%d Hz",
             LEGBOT_BSP_I2C_SDA_GPIO,
             LEGBOT_BSP_I2C_SCL_GPIO,
             LEGBOT_BSP_I2C_FREQ_HZ);
    return ESP_OK;
}

esp_err_t i2c_manager_deinit(void)
{
    if (!s_initialized)
    {
        return ESP_OK;
    }

    if (s_i2c_mutex == NULL ||
        xSemaphoreTake(s_i2c_mutex, pdMS_TO_TICKS(I2C_MANAGER_DEFAULT_TIMEOUT_MS)) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (s_scan_cleanup_device != NULL)
    {
        esp_err_t cleanup_err = remove_device_locked(s_scan_cleanup_device);
        if (cleanup_err != ESP_OK)
        {
            (void)xSemaphoreGive(s_i2c_mutex);
            return cleanup_err;
        }
        s_scan_cleanup_device = NULL;
    }
    esp_err_t err = i2c_del_master_bus(s_bus_handle);
    if (err != ESP_OK)
    {
        (void)xSemaphoreGive(s_i2c_mutex);
        return err;
    }

    s_bus_handle = NULL;
    s_initialized = false;
    /* 保留 manager 生命周期互斥锁，使已在等待的调用者可安全醒来并观察未初始化状态。 */
    (void)xSemaphoreGive(s_i2c_mutex);
    return ESP_OK;
}

bool i2c_manager_is_initialized(void)
{
    return s_initialized;
}

i2c_master_bus_handle_t i2c_manager_bus_handle(void)
{
    return s_initialized ? s_bus_handle : NULL;
}

esp_err_t i2c_manager_get_access(legbot_bsp_i2c_access_t *access)
{
    if (access == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized || s_bus_handle == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    *access = (legbot_bsp_i2c_access_t){
        .bus_handle = s_bus_handle,
        .acquire = i2c_manager_acquire,
        .release = i2c_manager_release,
    };
    return ESP_OK;
}

esp_err_t i2c_manager_acquire(TickType_t timeout_ticks)
{
    if (!s_initialized || s_i2c_mutex == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* 跨多次底层调用的事务只允许短时间占锁。 */
    if (xSemaphoreTake(s_i2c_mutex, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    if (!s_initialized || s_bus_handle == NULL)
    {
        (void)xSemaphoreGive(s_i2c_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}

void i2c_manager_release(void)
{
    if (s_i2c_mutex != NULL)
    {
        (void)xSemaphoreGive(s_i2c_mutex);
    }
}

esp_err_t i2c_manager_add_device(uint8_t address,
                                 uint32_t scl_speed_hz,
                                 i2c_master_dev_handle_t *device)
{
    if (address > 0x7F || scl_speed_hz == 0 || device == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = i2c_manager_acquire(pdMS_TO_TICKS(I2C_MANAGER_DEFAULT_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = add_device_locked(address, scl_speed_hz, device);
        i2c_manager_release();
    }
    return err;
}

esp_err_t i2c_manager_remove_device(i2c_master_dev_handle_t device)
{
    if (device == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t err = i2c_manager_acquire(pdMS_TO_TICKS(I2C_MANAGER_DEFAULT_TIMEOUT_MS));
    if (err == ESP_OK)
    {
        err = remove_device_locked(device);
        i2c_manager_release();
    }
    return err;
}

esp_err_t i2c_manager_probe(uint8_t address, int timeout_ms, i2c_manager_result_t *result)
{
    if (address > 0x7F || timeout_ms <= 0 || result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized || s_bus_handle == NULL)
    {
        set_result(result,
                   NULL,
                   address,
                   I2C_MANAGER_STATUS_MANAGER_NOT_READY,
                   ESP_ERR_INVALID_STATE);
        return ESP_OK;
    }

    esp_err_t err = i2c_manager_acquire(pdMS_TO_TICKS(timeout_ms));
    if (err == ESP_OK)
    {
        err = i2c_master_probe(s_bus_handle, address, timeout_ms);
        i2c_manager_release();
    }
    set_result(result, NULL, address, map_probe_status(err), err);
    return ESP_OK;
}

esp_err_t i2c_manager_transmit(i2c_master_dev_handle_t device,
                               uint8_t address,
                               const uint8_t *data,
                               size_t data_size,
                               int timeout_ms,
                               i2c_manager_result_t *result)
{
    if (device == NULL || address > 0x7F || data == NULL || data_size == 0 ||
        timeout_ms <= 0 || result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized)
    {
        set_result(result,
                   NULL,
                   address,
                   I2C_MANAGER_STATUS_MANAGER_NOT_READY,
                   ESP_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2c_manager_acquire(pdMS_TO_TICKS(timeout_ms));
    if (err == ESP_OK)
    {
        err = i2c_master_transmit(device, data, data_size, timeout_ms);
        i2c_manager_release();
    }
    set_result(result, NULL, address, map_transaction_status(err, false), err);
    return err;
}

esp_err_t i2c_manager_transmit_receive(i2c_master_dev_handle_t device,
                                       uint8_t address,
                                       const uint8_t *write_data,
                                       size_t write_size,
                                       uint8_t *read_data,
                                       size_t read_size,
                                       int timeout_ms,
                                       i2c_manager_result_t *result)
{
    if (device == NULL || address > 0x7F || write_data == NULL || write_size == 0 ||
        read_data == NULL || read_size == 0 || timeout_ms <= 0 || result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_initialized)
    {
        set_result(result,
                   NULL,
                   address,
                   I2C_MANAGER_STATUS_MANAGER_NOT_READY,
                   ESP_ERR_INVALID_STATE);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = i2c_manager_acquire(pdMS_TO_TICKS(timeout_ms));
    if (err == ESP_OK)
    {
        err = i2c_master_transmit_receive(device,
                                          write_data,
                                          write_size,
                                          read_data,
                                          read_size,
                                          timeout_ms);
        i2c_manager_release();
    }
    set_result(result, NULL, address, map_transaction_status(err, true), err);
    return err;
}

esp_err_t i2c_manager_scan_expected(i2c_manager_scan_report_t *report)
{
    if (report == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    memset(report, 0, sizeof(*report));
    report->count = I2C_MANAGER_EXPECTED_DEVICE_COUNT;

    for (size_t index = 0; index < I2C_MANAGER_EXPECTED_DEVICE_COUNT; ++index)
    {
        const i2c_expected_device_t *expected = &s_expected_devices[index];
        i2c_manager_result_t *result = &report->devices[index];
        if (expected->verify_qmi8658_id)
        {
            verify_expected_qmi(expected, result);
        }
        else
        {
            (void)i2c_manager_probe(expected->address, I2C_MANAGER_DEFAULT_TIMEOUT_MS, result);
            result->device_name = expected->name;
        }
        log_scan_result(result);
    }
    return ESP_OK;
}

const char *i2c_manager_status_name(i2c_manager_status_t status)
{
    switch (status)
    {
    case I2C_MANAGER_STATUS_PRESENT:
        return "present";
    case I2C_MANAGER_STATUS_DEVICE_MISSING:
        return "device_missing";
    case I2C_MANAGER_STATUS_BUS_BUSY_OR_TIMEOUT:
        return "bus_busy_or_timeout";
    case I2C_MANAGER_STATUS_READ_FAILED:
        return "read_failed";
    case I2C_MANAGER_STATUS_WRITE_FAILED:
        return "write_failed";
    case I2C_MANAGER_STATUS_MANAGER_NOT_READY:
        return "manager_not_ready";
    default:
        return "unknown";
    }
}

const char *i2c_manager_error_code(i2c_manager_status_t status)
{
    switch (status)
    {
    case I2C_MANAGER_STATUS_PRESENT:
        return "DRV_I2C_OK";
    case I2C_MANAGER_STATUS_DEVICE_MISSING:
        return "DRV_I2C_DEVICE_MISSING";
    case I2C_MANAGER_STATUS_BUS_BUSY_OR_TIMEOUT:
        return "DRV_I2C_BUS_BUSY";
    case I2C_MANAGER_STATUS_READ_FAILED:
        return "DRV_I2C_READ_FAILED";
    case I2C_MANAGER_STATUS_WRITE_FAILED:
        return "DRV_I2C_WRITE_FAILED";
    case I2C_MANAGER_STATUS_MANAGER_NOT_READY:
        return "DRV_I2C_MANAGER_NOT_READY";
    default:
        return "DRV_I2C_UNKNOWN";
    }
}

static i2c_manager_status_t map_probe_status(esp_err_t err)
{
    if (err == ESP_OK)
    {
        return I2C_MANAGER_STATUS_PRESENT;
    }
    if (err == ESP_ERR_NOT_FOUND)
    {
        return I2C_MANAGER_STATUS_DEVICE_MISSING;
    }
    if (err == ESP_ERR_TIMEOUT)
    {
        return I2C_MANAGER_STATUS_BUS_BUSY_OR_TIMEOUT;
    }
    if (err == ESP_ERR_INVALID_STATE)
    {
        return I2C_MANAGER_STATUS_MANAGER_NOT_READY;
    }
    return I2C_MANAGER_STATUS_READ_FAILED;
}

static i2c_manager_status_t map_transaction_status(esp_err_t err, bool read_operation)
{
    if (err == ESP_OK)
    {
        return I2C_MANAGER_STATUS_PRESENT;
    }
    if (err == ESP_ERR_INVALID_STATE)
    {
        /* ESP-IDF v5.5 新 I2C 驱动在同步事务失败（NACK 等）时
           统一返回 ESP_ERR_INVALID_STATE。映射为 DEVICE_MISSING。 */
        return I2C_MANAGER_STATUS_DEVICE_MISSING;
    }
    if (err == ESP_ERR_TIMEOUT)
    {
        return I2C_MANAGER_STATUS_BUS_BUSY_OR_TIMEOUT;
    }
    /* ESP_ERR_INVALID_STATE 在 acquire 成功后由 I2C 驱动报告 NACK 所致，
       不应再映射为 MANAGER_NOT_READY（该状态由 acquire 自身持有层处理）。 */
    return read_operation ? I2C_MANAGER_STATUS_READ_FAILED : I2C_MANAGER_STATUS_WRITE_FAILED;
}

static void set_result(i2c_manager_result_t *result,
                       const char *device_name,
                       uint8_t address,
                       i2c_manager_status_t status,
                       esp_err_t err)
{
    *result = (i2c_manager_result_t){
        .device_name = device_name,
        .address = address,
        .status = status,
        .esp_error = err,
    };
}

static esp_err_t add_device_locked(uint8_t address,
                                   uint32_t scl_speed_hz,
                                   i2c_master_dev_handle_t *device)
{
    const i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = scl_speed_hz,
        .scl_wait_us = 0,
    };
    return i2c_master_bus_add_device(s_bus_handle, &device_config, device);
}

static esp_err_t remove_device_locked(i2c_master_dev_handle_t device)
{
    return i2c_master_bus_rm_device(device);
}

static void verify_expected_qmi(const i2c_expected_device_t *expected,
                                i2c_manager_result_t *result)
{
    const uint8_t candidates[] = {expected->address, expected->alternate_address};
    esp_err_t err = i2c_manager_acquire(pdMS_TO_TICKS(I2C_MANAGER_DEFAULT_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        set_result(result,
                   expected->name,
                   expected->address,
                   map_probe_status(err),
                   err);
        result->alternate_address = expected->alternate_address;
        return;
    }

    if (s_scan_cleanup_device != NULL)
    {
        err = remove_device_locked(s_scan_cleanup_device);
        if (err != ESP_OK)
        {
            i2c_manager_release();
            set_result(result,
                       expected->name,
                       expected->address,
                       I2C_MANAGER_STATUS_READ_FAILED,
                       err);
            result->alternate_address = expected->alternate_address;
            return;
        }
        s_scan_cleanup_device = NULL;
    }

    i2c_manager_status_t final_status = I2C_MANAGER_STATUS_DEVICE_MISSING;
    esp_err_t final_error = ESP_ERR_NOT_FOUND;
    for (size_t index = 0; index < sizeof(candidates) / sizeof(candidates[0]); ++index)
    {
        uint8_t address = candidates[index];
        err = i2c_master_probe(s_bus_handle, address, I2C_MANAGER_DEFAULT_TIMEOUT_MS);
        if (err == ESP_ERR_NOT_FOUND)
        {
            continue;
        }
        if (err != ESP_OK)
        {
            final_status = map_probe_status(err);
            final_error = err;
            break;
        }

        i2c_master_dev_handle_t device = NULL;
        err = add_device_locked(address, LEGBOT_BSP_I2C_FREQ_HZ, &device);
        if (err != ESP_OK)
        {
            final_status = map_transaction_status(err, true);
            final_error = err;
            break;
        }

        const uint8_t register_address = I2C_MANAGER_QMI8658_WHO_AM_I_REGISTER;
        uint8_t chip_id = 0;
        esp_err_t read_err = i2c_master_transmit_receive(device,
                                                         &register_address,
                                                         sizeof(register_address),
                                                         &chip_id,
                                                         sizeof(chip_id),
                                                         I2C_MANAGER_DEFAULT_TIMEOUT_MS);
        esp_err_t remove_err = remove_device_locked(device);
        if (remove_err != ESP_OK)
        {
            s_scan_cleanup_device = device;
            final_status = I2C_MANAGER_STATUS_READ_FAILED;
            final_error = remove_err;
            break;
        }
        if (read_err != ESP_OK)
        {
            final_status = map_transaction_status(read_err, true);
            final_error = read_err;
            continue;
        }
        if (chip_id == I2C_MANAGER_QMI8658_WHO_AM_I_VALUE)
        {
            i2c_manager_release();
            set_result(result,
                       expected->name,
                       address,
                       I2C_MANAGER_STATUS_PRESENT,
                       ESP_OK);
            return;
        }
        final_status = I2C_MANAGER_STATUS_READ_FAILED;
        final_error = ESP_ERR_INVALID_RESPONSE;
    }
    i2c_manager_release();
    set_result(result,
               expected->name,
               expected->address,
               final_status,
               final_error);
    result->alternate_address = expected->alternate_address;
}

static void log_scan_result(const i2c_manager_result_t *result)
{
    if (result->status == I2C_MANAGER_STATUS_PRESENT)
    {
        ESP_LOGI(TAG,
                 "I2C 扫描：设备=%s，地址=0x%02X，状态=%s",
                 result->device_name,
                 result->address,
                 i2c_manager_status_name(result->status));
        return;
    }
    if (result->alternate_address != 0)
    {
        ESP_LOGW(TAG,
                 "I2C 扫描：设备=%s，候选地址=0x%02X/0x%02X，状态=%s，稳定错误码=%s，ESP 错误=0x%x",
                 result->device_name,
                 result->address,
                 result->alternate_address,
                 i2c_manager_status_name(result->status),
                 i2c_manager_error_code(result->status),
                 (unsigned)result->esp_error);
    }
    else
    {
        ESP_LOGW(TAG,
                 "I2C 扫描：设备=%s，地址=0x%02X，状态=%s，稳定错误码=%s，ESP 错误=0x%x",
                 result->device_name,
                 result->address,
                 i2c_manager_status_name(result->status),
                 i2c_manager_error_code(result->status),
                 (unsigned)result->esp_error);
    }
}
