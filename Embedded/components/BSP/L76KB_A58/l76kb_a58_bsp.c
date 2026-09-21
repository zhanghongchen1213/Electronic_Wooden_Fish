/**
 * @file     l76kb_a58_bsp.c
 * @brief    L76KB-A58 GPS UART 与 WAKE BSP 实现
 * @details  从权威 BSP 资源表配置 UART1，并通过 WAKE 低电平待机和高阻释放切换接收机状态。
 * @author   ZHC
 * @date     2026-07-13
 */

#include "l76kb_a58_bsp.h"

#include <limits.h>
#include <stdatomic.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "soc/soc_caps.h"

static const char *TAG = "BSP_GPS";

_Static_assert(L76KB_A58_UART_RX_BUFFER_SIZE > SOC_UART_FIFO_LEN,
               "GPS UART RX ring buffer must exceed ESP32-S3 hardware FIFO");

/** prepare 已通过资源与 WAKE 策略校验。 */
static bool s_prepared;
/** WAKE GPIO 已由本 BSP 配置。 */
static bool s_wake_configured;
/** UART driver 已由本 BSP 安装。 */
static bool s_uart_installed;
/** WAKE 当前是否处于高阻释放工作态。 */
static bool s_active;
/** BSP 成功启动 UART/WAKE 的单调代次。 */
static uint32_t s_start_generation;
/** stop 标志使有界读取在下一检查点退出。 */
static atomic_bool s_stop_requested;
/** 最近一次稳定 GPS_/DRV_ 诊断码。 */
static const char *s_last_error_code = L76KB_A58_ERROR_NOT_STARTED;

static bool resource_is_valid(const legbot_bsp_resource_t *resource);
static esp_err_t configure_wake(bool active);
static esp_err_t cleanup_started_resources(void);
static esp_err_t remember_error(esp_err_t error, const char *error_code);

const legbot_bsp_resource_t *l76kb_a58_bsp_resource(void)
{
    return legbot_bsp_get_resource(LEGBOT_BSP_RESOURCE_GPS_L76KB_A58);
}

esp_err_t l76kb_a58_bsp_prepare(void)
{
    if (s_prepared)
    {
        return ESP_OK;
    }
    const legbot_bsp_resource_t *resource = l76kb_a58_bsp_resource();
    if (!resource_is_valid(resource))
    {
        return remember_error(ESP_ERR_INVALID_STATE,
                              L76KB_A58_ERROR_RESOURCE_INVALID);
    }
    if (L76KB_A58_WAKE_POLICY == L76KB_A58_WAKE_POLICY_UNVERIFIED)
    {
        ESP_LOGE(TAG, "GPS WAKE 策略未验证，拒绝驱动硬件");
        return remember_error(ESP_ERR_NOT_SUPPORTED,
                              L76KB_A58_ERROR_WAKE_POLICY_UNVERIFIED);
    }

    atomic_store(&s_stop_requested, false);
    s_prepared = true;
    s_last_error_code = L76KB_A58_ERROR_OK;
    return ESP_OK;
}

esp_err_t l76kb_a58_bsp_start(void)
{
    if (s_uart_installed && s_wake_configured)
    {
        return l76kb_a58_bsp_set_standby();
    }
    esp_err_t err = l76kb_a58_bsp_prepare();
    if (err != ESP_OK)
    {
        return err;
    }

    const legbot_bsp_resource_t *resource = l76kb_a58_bsp_resource();
    err = configure_wake(false);
    if (err != ESP_OK)
    {
        return remember_error(err, L76KB_A58_ERROR_WAKE_CONFIG);
    }
    s_wake_configured = true;

    const uart_config_t uart_config = {
        .baud_rate = (int)L76KB_A58_UART_BAUD_RATE,
        .data_bits = L76KB_A58_UART_DATA_BITS,
        .parity = L76KB_A58_UART_PARITY,
        .stop_bits = L76KB_A58_UART_STOP_BITS,
        .flow_ctrl = L76KB_A58_UART_FLOW_CONTROL,
        .rx_flow_ctrl_thresh = 0U,
        .source_clk = UART_SCLK_DEFAULT,
        .flags = {0},
    };
    err = uart_param_config(resource->uart_port, &uart_config);
    if (err != ESP_OK)
    {
        (void)cleanup_started_resources();
        return remember_error(err, L76KB_A58_ERROR_UART_CONFIG);
    }
    err = uart_set_pin(resource->uart_port,
                       resource->gpio_secondary,
                       resource->gpio_primary,
                       UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        (void)cleanup_started_resources();
        return remember_error(err, L76KB_A58_ERROR_UART_PIN);
    }
    err = uart_driver_install(resource->uart_port,
                              (int)L76KB_A58_UART_RX_BUFFER_SIZE,
                              (int)L76KB_A58_UART_TX_BUFFER_SIZE,
                              0,
                              NULL,
                              0);
    if (err != ESP_OK)
    {
        (void)cleanup_started_resources();
        return remember_error(err, L76KB_A58_ERROR_UART_INSTALL);
    }
    s_uart_installed = true;
    err = l76kb_a58_bsp_flush_input();
    if (err != ESP_OK)
    {
        (void)cleanup_started_resources();
        return err;
    }
    s_start_generation =
        s_start_generation == UINT32_MAX ? 1U : s_start_generation + 1U;

    atomic_store(&s_stop_requested, false);
    s_last_error_code = L76KB_A58_ERROR_OK;
    ESP_LOGI(TAG,
             "GPS UART1 已按 Arduino 实测基线启动，代次=%lu，9600 8N1，WAKE=IO3 保持低电平待机，UART TX 命令=0",
             (unsigned long)s_start_generation);
    return ESP_OK;
}

esp_err_t l76kb_a58_bsp_set_active(void)
{
    if (!s_wake_configured)
    {
        return remember_error(ESP_ERR_INVALID_STATE,
                              L76KB_A58_ERROR_NOT_STARTED);
    }
    if (s_active)
    {
        return ESP_OK;
    }
    const esp_err_t error = configure_wake(true);
    if (error != ESP_OK)
    {
        return remember_error(error, L76KB_A58_ERROR_WAKE_CONFIG);
    }
    ESP_LOGI(TAG, "GPS WAKE 已高阻释放，接收机进入工作态");
    return ESP_OK;
}

esp_err_t l76kb_a58_bsp_set_standby(void)
{
    if (!s_wake_configured)
    {
        return remember_error(ESP_ERR_INVALID_STATE,
                              L76KB_A58_ERROR_NOT_STARTED);
    }
    if (!s_active)
    {
        return ESP_OK;
    }
    const esp_err_t error = configure_wake(false);
    if (error != ESP_OK)
    {
        return remember_error(error, L76KB_A58_ERROR_WAKE_CONFIG);
    }
    ESP_LOGI(TAG, "GPS WAKE 已拉低，接收机进入待机态");
    return ESP_OK;
}

esp_err_t l76kb_a58_bsp_flush_input(void)
{
    if (!s_uart_installed)
    {
        return remember_error(ESP_ERR_INVALID_STATE,
                              L76KB_A58_ERROR_NOT_STARTED);
    }
    const legbot_bsp_resource_t *resource = l76kb_a58_bsp_resource();
    const esp_err_t error = uart_flush_input(resource->uart_port);
    return error == ESP_OK
               ? ESP_OK
               : remember_error(error, L76KB_A58_ERROR_UART_FLUSH);
}

int l76kb_a58_bsp_read(uint8_t *buffer, size_t capacity, uint32_t timeout_ms)
{
    if (buffer == NULL || capacity == 0U || capacity > UINT32_MAX ||
        !s_uart_installed || !s_active || atomic_load(&s_stop_requested))
    {
        return -1;
    }
    const uint32_t bounded_timeout_ms = timeout_ms < L76KB_A58_UART_READ_MAX_WAIT_MS
                                            ? timeout_ms
                                            : L76KB_A58_UART_READ_MAX_WAIT_MS;
    const legbot_bsp_resource_t *resource = l76kb_a58_bsp_resource();
    const int read_count = uart_read_bytes(resource->uart_port,
                                           buffer,
                                           (uint32_t)capacity,
                                           pdMS_TO_TICKS(bounded_timeout_ms));
    if (read_count < 0)
    {
        (void)remember_error(ESP_FAIL, L76KB_A58_ERROR_UART_READ);
        return -1;
    }
    if (atomic_load(&s_stop_requested))
    {
        return 0;
    }
    return read_count;
}

void l76kb_a58_bsp_request_stop(void)
{
    atomic_store(&s_stop_requested, true);
}

esp_err_t l76kb_a58_bsp_stop(void)
{
    atomic_store(&s_stop_requested, true);
    return cleanup_started_resources();
}

esp_err_t l76kb_a58_bsp_deinit(void)
{
    const esp_err_t err = l76kb_a58_bsp_stop();
    s_prepared = false;
    if (err == ESP_OK && strcmp(s_last_error_code, L76KB_A58_ERROR_OK) == 0)
    {
        s_last_error_code = L76KB_A58_ERROR_NOT_STARTED;
    }
    return err;
}

bool l76kb_a58_bsp_is_started(void)
{
    return s_uart_installed && s_wake_configured &&
           !atomic_load(&s_stop_requested);
}

bool l76kb_a58_bsp_is_active(void)
{
    return l76kb_a58_bsp_is_started() && s_active;
}

const char *l76kb_a58_bsp_last_error_code(void)
{
    return s_last_error_code;
}

static bool resource_is_valid(const legbot_bsp_resource_t *resource)
{
    return resource != NULL &&
           resource->id == LEGBOT_BSP_RESOURCE_GPS_L76KB_A58 &&
           resource->uart_port == LEGBOT_BSP_GPS_UART_PORT &&
           resource->gpio_primary == LEGBOT_BSP_GPS_RX_GPIO &&
           resource->gpio_secondary == LEGBOT_BSP_GPS_TX_GPIO &&
           resource->gpio_aux0 == LEGBOT_BSP_GPS_WAKE_GPIO &&
           LEGBOT_BSP_SDMMC_ENABLED == 0;
}

static esp_err_t configure_wake(bool active)
{
    if (!active)
    {
        const esp_err_t error = l76kb_a58_bsp_hold_standby();
        if (error == ESP_OK)
        {
            s_active = false;
        }
        return error;
    }

    const legbot_bsp_resource_t *resource = l76kb_a58_bsp_resource();
    if (resource == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const gpio_config_t wake_config = {
        .pin_bit_mask = 1ULL << (uint32_t)resource->gpio_aux0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t error = gpio_config(&wake_config);
    if (error != ESP_OK)
    {
        return error;
    }
    error = gpio_sleep_sel_dis(resource->gpio_aux0);
    if (error != ESP_OK)
    {
        return error;
    }
    s_active = true;
    return ESP_OK;
}

static esp_err_t cleanup_started_resources(void)
{
    esp_err_t first_error = ESP_OK;
    const legbot_bsp_resource_t *resource = l76kb_a58_bsp_resource();
    const bool resources_were_started = s_uart_installed || s_wake_configured;
    bool standby_confirmed = !s_wake_configured;
    if (s_wake_configured)
    {
        const esp_err_t standby_error = configure_wake(false);
        standby_confirmed = standby_error == ESP_OK;
        if (standby_error != ESP_OK && first_error == ESP_OK)
        {
            first_error = standby_error;
            ESP_LOGE(TAG,
                     "GPS WAKE 进入待机失败，错误=0x%x",
                     (unsigned)standby_error);
        }
    }
    if (s_uart_installed)
    {
        const esp_err_t uart_error = uart_driver_delete(resource->uart_port);
        if (uart_error != ESP_OK)
        {
            first_error = uart_error;
            ESP_LOGE(TAG, "GPS UART driver 删除失败，错误=0x%x", (unsigned)uart_error);
        }
        s_uart_installed = false;
    }
    if (s_wake_configured && standby_confirmed)
    {
        s_wake_configured = false;
        s_active = false;
    }
    if (resources_were_started && standby_confirmed)
    {
        ESP_LOGI(TAG,
                 "GPS BSP 资源已停止，代次=%lu，WAKE 保持低电平待机，UART TX 命令=0",
                 (unsigned long)s_start_generation);
    }
    else if (!standby_confirmed)
    {
        ESP_LOGE(TAG,
                 "GPS BSP 未确认 WAKE 低电平，保留活动状态供上层 fail-closed 重试");
    }
    return first_error;
}

static esp_err_t remember_error(esp_err_t error, const char *error_code)
{
    s_last_error_code = error_code;
    return error;
}
