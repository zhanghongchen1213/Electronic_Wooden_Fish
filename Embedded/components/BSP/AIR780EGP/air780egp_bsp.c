/**
 * @file     air780egp_bsp.c
 * @brief    Air780EGP 驱动边界实现（真机 AT 路径 hardware_pending）。
 * @details  应用层禁止直读写 UART；本文件独占 UART1 事务所有权。
 *           不发 AT+CGNS*；不启用 BLE/Wi-Fi；不引入 MQTT/模组独立云。
 *           UART API 依据 ESP-IDF v5.5.4 ESP32-S3 文档。
 * @author   ZHC
 * @date     2026-09-24
 */

#include "air780egp_bsp.h"

#include <stdio.h>
#include <string.h>

#include "bsp_resources.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "BSP_AIR780";

static bool s_started;
static bool s_suspended;
static bool s_https_busy;
static bool s_uart_installed;
static ewf_air780_status_t s_status;

static void clear_live_network_flags(void)
{
    s_status.sim_ready = false;
    s_status.attached = false;
    s_status.pdp_active = false;
    s_status.ipv4_valid = false;
    s_status.network_ready = false;
}

static void refresh_network_ready(void)
{
    s_status.network_ready = s_status.sim_ready && s_status.attached &&
                             s_status.pdp_active && s_status.ipv4_valid;
}

static esp_err_t configure_control_pins(void)
{
    /* RST=IO15：输入高阻，不硬复位。 */
    gpio_config_t rst_cfg = {
        .pin_bit_mask = 1ULL << EWF_BSP_AIR780EGP_RST_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&rst_cfg);
    if (err != ESP_OK) {
        return err;
    }

    /* DTR=IO10：开漏输出，休眠/唤醒电平由 suspend/start 管理。hardware_pending。 */
    gpio_config_t dtr_cfg = {
        .pin_bit_mask = 1ULL << EWF_BSP_AIR780EGP_DTR_GPIO,
        .mode = GPIO_MODE_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    err = gpio_config(&dtr_cfg);
    if (err != ESP_OK) {
        return err;
    }
    /* 唤醒电平：释放开漏（高）。极性样机定标前 hardware_pending。 */
    return gpio_set_level(EWF_BSP_AIR780EGP_DTR_GPIO, 1);
}

static esp_err_t install_uart(void)
{
    if (s_uart_installed) {
        return ESP_OK;
    }

    /* 依据 ESP-IDF v5.5.4：uart_driver_install / uart_param_config / uart_set_pin。 */
    const uart_config_t uart_config = {
        .baud_rate = EWF_BSP_AIR780EGP_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err = uart_driver_install(EWF_BSP_AIR780EGP_UART_PORT, 4096, 0, 0,
                                        NULL, 0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "UART 安装失败：%s（hardware_pending）",
                 esp_err_to_name(err));
        return err;
    }
    err = uart_param_config(EWF_BSP_AIR780EGP_UART_PORT, &uart_config);
    if (err != ESP_OK) {
        return err;
    }
    err = uart_set_pin(EWF_BSP_AIR780EGP_UART_PORT, EWF_BSP_AIR780EGP_TX_GPIO,
                       EWF_BSP_AIR780EGP_RX_GPIO, UART_PIN_NO_CHANGE,
                       UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        return err;
    }
    s_uart_installed = true;
    return ESP_OK;
}

esp_err_t ewf_air780_start(void)
{
    memset(&s_status, 0, sizeof(s_status));
    s_status.csq_rssi = -1;
    clear_live_network_flags();

    esp_err_t err = configure_control_pins();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "控制脚配置失败：%s", esp_err_to_name(err));
        return err;
    }
    err = install_uart();
    if (err != ESP_OK) {
        return err;
    }

    /*
     * 真机 AT 链（ATE0/CMEE/CSCLK/PDP）属 hardware_pending：
     * 本 Story 闭合 API 边界与引脚；不等待样机验收宣称通信可靠。
     */
    s_started = true;
    s_suspended = false;
    s_status.started = true;
    s_status.suspended = false;
    (void)snprintf(s_status.last_error, sizeof(s_status.last_error),
                   "HARDWARE_PENDING");
    ESP_LOGW(TAG,
             "Air780EGP 已 start：UART1 TX=%d RX=%d DTR=%d RST=%d；"
             "PDP/HTTPS 真机路径 hardware_pending，不得宣称通信可靠",
             (int)EWF_BSP_AIR780EGP_TX_GPIO, (int)EWF_BSP_AIR780EGP_RX_GPIO,
             (int)EWF_BSP_AIR780EGP_DTR_GPIO, (int)EWF_BSP_AIR780EGP_RST_GPIO);
    return ESP_OK;
}

esp_err_t ewf_air780_get_status(ewf_air780_status_t *status)
{
    if (status == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    refresh_network_ready();
    s_status.started = s_started;
    s_status.suspended = s_suspended;
    s_status.https_busy = s_https_busy;
    *status = s_status;
    return ESP_OK;
}

esp_err_t ewf_air780_https_post_json(const char *url,
                                     const char *token,
                                     const char *request_json,
                                     char *response_json,
                                     size_t response_capacity,
                                     size_t *response_len,
                                     uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (url == NULL || request_json == NULL || response_json == NULL ||
        response_capacity == 0U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_started || s_suspended) {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_https_busy) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 日志脱敏：只记长度，不回显 URL/token/正文。 */
    ESP_LOGI(TAG,
             "HTTPS 单事务请求：url(len=%lu) token(len=%lu) body(len=%lu) "
             "ssl_ctx=%d seclevel=%d deadline_ms=%u（hardware_pending）",
             (unsigned long)strlen(url),
             (unsigned long)(token != NULL ? strlen(token) : 0U),
             (unsigned long)strlen(request_json), EWF_AIR780_SSL_CONTEXT_ID,
             EWF_AIR780_SSL_SECLEVEL, (unsigned)EWF_AIR780_HTTPS_DEADLINE_MS);

    s_https_busy = true;
    /*
     * 真机 AT 序列（HTTPINIT/HTTPSSL/SSLCFG/HTTPDATA/HTTPACTION/HTTPREAD/HTTPTERM）
     * 尚未在 EWF 板闭环。失败必须释放上下文语义：此处清 busy，避免假 network_ready。
     */
    clear_live_network_flags();
    s_https_busy = false;
    if (response_len != NULL) {
        *response_len = 0U;
    }
    response_json[0] = '\0';
    (void)snprintf(s_status.last_error, sizeof(s_status.last_error),
                   "HTTPS_HARDWARE_PENDING");
    ESP_LOGW(TAG, "HTTPS 真机路径未核验，返回 NOT_SUPPORTED（不得伪装已同步）");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ewf_air780_suspend(void)
{
    if (!s_started) {
        return ESP_ERR_INVALID_STATE;
    }
    /* 失败也要释放 HTTP 上下文等价清理，摘掉假 network_ready。 */
    s_https_busy = false;
    clear_live_network_flags();
    s_suspended = true;
    s_status.suspended = true;
    /* CSCLK=1 + DTR 休眠电平：极性 hardware_pending。 */
    (void)gpio_set_level(EWF_BSP_AIR780EGP_DTR_GPIO, 0);
    ESP_LOGI(TAG, "Air780EGP 已 suspend：DTR 休眠准备，network_ready=false");
    return ESP_OK;
}
