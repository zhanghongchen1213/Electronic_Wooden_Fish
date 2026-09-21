/**
 * @file     cw2015_bsp.c
 * @brief    CW2015 电量计项目 BSP 实现
 * @details  在共享 I2C 短互斥内完成设备验证、SOC 寄存器读取和无泄漏 device handle 生命周期。
 *          CW2015 由 Cellwise 公司生产，I2C 地址 0x62，与 MAX17048/MAX17049 封装兼容。
 * @author   ZHC
 * @date     2026-07-14
 */

#include "cw2015_bsp.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "BSP_CW2015";

/** CW2015 电压寄存器地址。 */
#define CW2015_VCELL_REGISTER 0x02
/** CW2015 SOC 16-bit 寄存器地址（含整数 + 分数 1/256%）。 */
#define CW2015_SOC_REGISTER 0x04
/** CW2015 生产版本 16-bit 寄存器地址（芯片 ID），用于初始化通信验证。 */
#define CW2015_VERSION_REGISTER 0x00
/** CW2015 模式控制寄存器：bit[7:6]=SLEEP，bit[5:4]=QSTRT。 */
#define CW2015_MODE_REGISTER 0x0A
/** MODE 寄存器写入值：清除 SLEEP 位退出休眠模式。 */
#define CW2015_MODE_VALUE_WAKE 0x00
/** CW2015 独立设备时钟，100 kHz 保证兼容性。 */
#define CW2015_BSP_I2C_FREQ_HZ 100000U
/** CW2015 SOC 公共结果上限。 */
#define CW2015_BSP_SOC_MAX_PERCENT 100.0f
/** 初始化阶段直接读取版本寄存器的有界尝试次数。 */
#define CW2015_INIT_VERSION_READ_ATTEMPTS 2U
/** 写寄存器的有界重试次数。 */
#define CW2015_INIT_WRITE_ATTEMPTS 3U

/** manager 注入的共享 I2C 访问能力。 */
static legbot_bsp_i2c_access_t s_i2c_access;
/** CW2015 device handle。 */
static i2c_master_dev_handle_t s_device;
/** CW2015 初始化状态。 */
static bool s_initialized;

static esp_err_t read_register_locked(i2c_master_dev_handle_t device,
                                      uint8_t register_address,
                                      uint16_t *value);
static esp_err_t write_register_locked(i2c_master_dev_handle_t device,
                                       uint8_t register_address,
                                       uint8_t value);
static esp_err_t read_version_with_attempts_locked(i2c_master_dev_handle_t device,
                                                   uint8_t version_register,
                                                   uint16_t *version,
                                                   const char *phase);
static esp_err_t verify_with_one_time_recovery_locked(i2c_master_bus_handle_t bus_handle,
                                                      i2c_master_dev_handle_t device,
                                                      uint16_t *version,
                                                      esp_err_t *functional_error,
                                                      esp_err_t *probe_error);
static esp_err_t cw2015_wake_up_locked(i2c_master_dev_handle_t device,
                                       const legbot_bsp_i2c_access_t *access);
static bool access_matches(const legbot_bsp_i2c_access_t *access);
static cw2015_status_t status_from_error(esp_err_t err, bool during_init);
static const char *error_code_from_status(cw2015_status_t status);
static void set_failed_soc(cw2015_bsp_soc_t *soc, cw2015_status_t status, esp_err_t err);

const legbot_bsp_resource_t *cw2015_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_CW2015);
}

esp_err_t cw2015_bsp_init(const legbot_bsp_i2c_access_t *access)
{
    const legbot_bsp_resource_t *resource = cw2015_bsp_resource();
    if (access == NULL || access->bus_handle == NULL || access->acquire == NULL ||
        access->release == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (resource == NULL || resource->i2c_port != LEGBOT_BSP_I2C_PORT)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_initialized)
    {
        return access_matches(access) ? ESP_OK : ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = access->acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "CW2015 初始化获取共享 I2C 失败，稳定错误码=%s，错误=0x%x",
                 error_code_from_status(status_from_error(err, true)),
                 (unsigned)err);
        return err;
    }
    if (s_initialized)
    {
        bool matches = access_matches(access);
        access->release();
        return matches ? ESP_OK : ESP_ERR_INVALID_STATE;
    }
    if (s_device != NULL)
    {
        err = i2c_master_bus_rm_device(s_device);
        if (err != ESP_OK)
        {
            access->release();
            return err;
        }
        s_device = NULL;
        memset(&s_i2c_access, 0, sizeof(s_i2c_access));
    }

    const uint8_t candidate_addresses[] = {
        CW2015_BSP_I2C_ADDRESS,
    };
    const size_t candidate_count = sizeof(candidate_addresses) / sizeof(candidate_addresses[0]);
    uint8_t active_address = 0;
    uint16_t version = 0;
    esp_err_t functional_error = ESP_OK;
    esp_err_t probe_error = ESP_OK;
    err = ESP_ERR_NOT_FOUND;

    for (size_t index = 0; index < candidate_count; ++index)
    {
        uint8_t addr = candidate_addresses[index];
        const i2c_device_config_t device_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addr,
            .scl_speed_hz = CW2015_BSP_I2C_FREQ_HZ,
            .scl_wait_us = 0,
        };
        err = i2c_master_bus_add_device(access->bus_handle, &device_config, &s_device);
        if (err != ESP_OK)
        {
            continue;
        }
        err = verify_with_one_time_recovery_locked(access->bus_handle,
                                                   s_device,
                                                   &version,
                                                   &functional_error,
                                                   &probe_error);
        if (err == ESP_OK)
        {
            /* 成功检测到设备后，在释放互斥锁前仅退出休眠 */
            err = cw2015_wake_up_locked(s_device, access);
        }
        if (err == ESP_OK)
        {
            active_address = addr;
            break;
        }
        esp_err_t cleanup_err = i2c_master_bus_rm_device(s_device);
        if (cleanup_err == ESP_OK)
        {
            s_device = NULL;
        }
        else
        {
            s_i2c_access = *access;
            access->release();
            ESP_LOGE(TAG,
                     "CW2015 地址 0x%02X 验证失败后清理 device handle 失败，错误=0x%x",
                     addr,
                     (unsigned)cleanup_err);
            return cleanup_err;
        }
    }
    access->release();

    if (err != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "CW2015 地址 0x%02X 无 ACK；请确认电芯电压已达到芯片 2.5 V 工作下限",
                 CW2015_BSP_I2C_ADDRESS);
        ESP_LOGW(TAG,
                 "CW2015 初始化失败，稳定错误码=%s，最终错误=0x%x，寄存器读取=0x%x，地址探测=0x%x",
                 error_code_from_status(status_from_error(err, true)),
                 (unsigned)err,
                 (unsigned)functional_error,
                 (unsigned)probe_error);
        return err;
    }

    s_i2c_access = *access;
    s_initialized = true;
    ESP_LOGI(TAG, "CW2015 初始化成功：地址=0x%02X，版本寄存器=0x%04X", active_address, version);
    return ESP_OK;
}

esp_err_t cw2015_bsp_deinit(void)
{
    if (!s_initialized && s_device == NULL)
    {
        return ESP_OK;
    }
    if (s_i2c_access.acquire == NULL || s_i2c_access.release == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = s_i2c_access.acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
    if (err != ESP_OK)
    {
        return err;
    }
    s_initialized = false;
    if (s_device != NULL)
    {
        err = i2c_master_bus_rm_device(s_device);
    }
    if (err != ESP_OK)
    {
        s_i2c_access.release();
        ESP_LOGE(TAG, "CW2015 释放 device handle 失败，错误=0x%x", (unsigned)err);
        return err;
    }

    legbot_bsp_i2c_release_fn_t release = s_i2c_access.release;
    s_device = NULL;
    memset(&s_i2c_access, 0, sizeof(s_i2c_access));
    release();
    return ESP_OK;
}

bool cw2015_bsp_is_initialized(void)
{
    return s_initialized;
}

esp_err_t cw2015_bsp_read_soc(cw2015_bsp_soc_t *soc)
{
    if (soc == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    set_failed_soc(soc, CW2015_STATUS_NOT_READY, ESP_ERR_INVALID_STATE);
    if (!s_initialized || s_device == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t err = s_i2c_access.acquire(pdMS_TO_TICKS(LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS));
    uint16_t raw_soc = 0;
    if (err == ESP_OK)
    {
        if (!s_initialized || s_device == NULL)
        {
            err = ESP_ERR_INVALID_STATE;
        }
        else
        {
            err = read_register_locked(s_device, CW2015_SOC_REGISTER, &raw_soc);
        }
        s_i2c_access.release();
    }
    if (err != ESP_OK)
    {
        cw2015_status_t status = status_from_error(err, false);
        set_failed_soc(soc, status, err);
        ESP_LOGW(TAG,
                 "CW2015 SOC 读取失败，稳定错误码=%s，错误=0x%x",
                 soc->error_code,
                 (unsigned)err);
        return err;
    }

    *soc = (cw2015_bsp_soc_t){
        .valid = true,
        .percent = cw2015_bsp_decode_soc(raw_soc),
        .raw_soc = raw_soc,
        .status = CW2015_STATUS_OK,
        .error_code = CW2015_ERROR_OK,
        .esp_error = ESP_OK,
    };
    return ESP_OK;
}

esp_err_t cw2015_bsp_read_continuous(cw2015_bsp_continuous_report_t *report)
{
    if (report == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    memset(report, 0, sizeof(*report));
    report->first_error = ESP_OK;
    for (size_t index = 0; index < CW2015_BSP_DIAGNOSTIC_SAMPLE_COUNT; ++index)
    {
        esp_err_t err = cw2015_bsp_read_soc(&report->samples[index]);
        report->count = index + 1;
        if (err != ESP_OK && report->first_error == ESP_OK)
        {
            report->first_error = err;
        }
    }
    return report->first_error;
}

float cw2015_bsp_decode_soc(uint16_t raw_soc)
{
    float percent = raw_soc / 256.0f;
    return percent > CW2015_BSP_SOC_MAX_PERCENT ? CW2015_BSP_SOC_MAX_PERCENT : percent;
}

static esp_err_t read_register_locked(i2c_master_dev_handle_t device,
                                      uint8_t register_address,
                                      uint16_t *value)
{
    uint8_t read_data[2] = {0};
    esp_err_t err = i2c_master_transmit_receive(device,
                                                &register_address,
                                                sizeof(register_address),
                                                read_data,
                                                sizeof(read_data),
                                                LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS);
    if (err == ESP_OK)
    {
        *value = ((uint16_t)read_data[0] << 8) | read_data[1];
    }
    return err;
}

static esp_err_t write_register_locked(i2c_master_dev_handle_t device,
                                       uint8_t register_address,
                                       uint8_t value)
{
    const uint8_t frame[2] = {register_address, value};
    return i2c_master_transmit(device,
                               frame,
                               sizeof(frame),
                               LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS);
}

/**
 * @brief 在互斥锁保护下退出 CW2015 休眠模式。
 * @details 仅退出 SLEEP 模式，不触发 Quick Start。
 *          Quick Start 会丢弃芯片保留的 SOC 估算并重新估算，
 *          每次手环重启都触发会造成电量百分比跳变。
 *          调用前必须已持有共享 I2C 互斥锁。
 * @param device  CW2015 I2C device handle
 * @param access  I2C manager access（用于临时释放/重获互斥锁期间的等待）
 */
static esp_err_t cw2015_wake_up_locked(i2c_master_dev_handle_t device,
                                       const legbot_bsp_i2c_access_t *access)
{
    esp_err_t err;

    /* Step 1: 清除 SLEEP 位，退出休眠模式 */
    for (uint32_t attempt = 0; attempt < CW2015_INIT_WRITE_ATTEMPTS; ++attempt)
    {
        err = write_register_locked(device, CW2015_MODE_REGISTER, CW2015_MODE_VALUE_WAKE);
        if (err == ESP_OK)
        {
            break;
        }
        ESP_LOGW(TAG, "CW2015 唤醒写入失败，尝试=%lu/%u，错误=0x%x",
                 (unsigned long)(attempt + 1U), CW2015_INIT_WRITE_ATTEMPTS, (unsigned)err);
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "CW2015 无法退出休眠模式，错误=0x%x", (unsigned)err);
        return err;
    }

    /* 保留芯片内部的 SOC 学习结果，首次有效读取直接作为当前值。 */
    ESP_LOGI(TAG, "CW2015 已退出休眠，保留芯片 SOC 学习结果");
    return ESP_OK;
}

static esp_err_t read_version_with_attempts_locked(i2c_master_dev_handle_t device,
                                                   uint8_t version_register,
                                                   uint16_t *version,
                                                   const char *phase)
{
    esp_err_t err = ESP_FAIL;
    for (uint32_t attempt = 0U;
         attempt < CW2015_INIT_VERSION_READ_ATTEMPTS;
         ++attempt)
    {
        err = read_register_locked(device, version_register, version);
        if (err == ESP_OK)
        {
            return ESP_OK;
        }
        ESP_LOGW(TAG,
                 "CW2015 %s VERSION 读取失败，尝试=%lu/%u，错误=0x%x",
                 phase,
                 (unsigned long)(attempt + 1U),
                 CW2015_INIT_VERSION_READ_ATTEMPTS,
                 (unsigned)err);
    }
    return err;
}

static esp_err_t verify_with_one_time_recovery_locked(i2c_master_bus_handle_t bus_handle,
                                                      i2c_master_dev_handle_t device,
                                                      uint16_t *version,
                                                      esp_err_t *functional_error,
                                                      esp_err_t *probe_error)
{
    /* 先尝试 CW2015 的版本寄存器 0x00（芯片 ID） */
    *functional_error = read_version_with_attempts_locked(device,
                                                          CW2015_VERSION_REGISTER,
                                                          version, "首次");
    if (*functional_error == ESP_OK)
    {
        *probe_error = ESP_OK;
        return ESP_OK;
    }

    *probe_error = i2c_master_probe(bus_handle,
                                   CW2015_BSP_I2C_ADDRESS,
                                   LEGBOT_BSP_I2C_TRANSACTION_TIMEOUT_MS);

    return *probe_error == ESP_ERR_NOT_FOUND ? *probe_error : *functional_error;
}

static bool access_matches(const legbot_bsp_i2c_access_t *access)
{
    return s_i2c_access.bus_handle == access->bus_handle &&
           s_i2c_access.acquire == access->acquire &&
           s_i2c_access.release == access->release;
}

static cw2015_status_t status_from_error(esp_err_t err, bool during_init)
{
    if (err == ESP_OK)
    {
        return CW2015_STATUS_OK;
    }
    if (err == ESP_ERR_NOT_FOUND || err == ESP_ERR_INVALID_STATE)
    {
        /* ESP-IDF v5.5 新 I2C 驱动在 i2c_master_transmit_receive()
           同步事务失败（含 NACK）时统一返回 ESP_ERR_INVALID_STATE (0x103)，
           而非区分 NACK 与总线错误。将其等价于 DEVICE_MISSING。 */
        return CW2015_STATUS_DEVICE_MISSING;
    }
    if (err == ESP_ERR_TIMEOUT)
    {
        return CW2015_STATUS_BUS_BUSY;
    }
    return during_init ? CW2015_STATUS_INIT_FAILED : CW2015_STATUS_READ_FAILED;
}

static const char *error_code_from_status(cw2015_status_t status)
{
    switch (status)
    {
    case CW2015_STATUS_OK:
        return CW2015_ERROR_OK;
    case CW2015_STATUS_NOT_READY:
        return CW2015_ERROR_NOT_READY;
    case CW2015_STATUS_DEVICE_MISSING:
        return CW2015_ERROR_DEVICE_MISSING;
    case CW2015_STATUS_BUS_BUSY:
        return CW2015_ERROR_BUS_BUSY;
    case CW2015_STATUS_INIT_FAILED:
        return CW2015_ERROR_INIT_FAILED;
    case CW2015_STATUS_READ_FAILED:
    default:
        return CW2015_ERROR_READ_FAILED;
    }
}

static void set_failed_soc(cw2015_bsp_soc_t *soc, cw2015_status_t status, esp_err_t err)
{
    *soc = (cw2015_bsp_soc_t){
        .valid = false,
        .percent = 0.0f,
        .raw_soc = 0,
        .status = status,
        .error_code = error_code_from_status(status),
        .esp_error = err,
    };
}
