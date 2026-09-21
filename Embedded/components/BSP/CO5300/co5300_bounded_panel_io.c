/**
 * @file     co5300_bounded_panel_io.c
 * @brief    CO5300 项目专用有界 QSPI panel IO 实现
 * @details  将命令头、参数或颜色数据各自合并为单笔 SPI 事务，并对排队和结果回收设置固定上限。
 * @author   ZHC
 * @date     2026-07-30
 */

#include "co5300_bounded_panel_io.h"

#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#include "driver/spi_master.h"
#include "esp_lcd_panel_io_interface.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "BSP_CO5300_IO";

/** 单笔 QSPI 事务排队和结果回收的最大等待时长。 */
#define CO5300_BOUNDED_IO_TIMEOUT_MS 100U
/** CO5300 初始化、窗口和亮度参数的最大字节数。 */
#define CO5300_BOUNDED_PARAM_CAPACITY 16U

typedef struct
{
    esp_lcd_panel_io_t base; /**< 通用 panel IO 接口，必须位于首字段。 */
    spi_device_handle_t spi_device; /**< CO5300 独占的 SPI device。 */
    esp_lcd_panel_io_color_trans_done_cb_t color_done_cb; /**< 颜色完成回调。 */
    void *callback_context; /**< 颜色完成回调上下文。 */
    spi_transaction_t parameter_transaction; /**< 持久参数事务描述符。 */
    spi_transaction_t color_transaction; /**< 持久颜色事务描述符。 */
    uint8_t parameter_buffer[CO5300_BOUNDED_PARAM_CAPACITY]; /**< 参数按值缓冲。 */
    bool parameter_inflight; /**< 参数事务是否仍待结果回收。 */
    bool color_inflight; /**< 颜色事务是否仍待结果回收。 */
    atomic_bool color_pm_lock_held; /**< 颜色 DMA 期间是否持有禁止 light sleep 锁。 */
    esp_pm_lock_handle_t color_pm_lock; /**< 颜色 DMA 的最小作用域电源管理锁。 */
    TickType_t timeout_ticks; /**< 单次 SPI 等待上限。 */
} co5300_bounded_panel_io_t;

static esp_err_t bounded_rx_param(esp_lcd_panel_io_t *io,
                                  int lcd_cmd,
                                  void *param,
                                  size_t param_size);
static esp_err_t bounded_tx_param(esp_lcd_panel_io_t *io,
                                  int lcd_cmd,
                                  const void *param,
                                  size_t param_size);
static esp_err_t bounded_tx_color(esp_lcd_panel_io_t *io,
                                  int lcd_cmd,
                                  const void *color,
                                  size_t color_size);
static esp_err_t bounded_delete(esp_lcd_panel_io_t *io);
static esp_err_t bounded_register_callbacks(
    esp_lcd_panel_io_handle_t io,
    const esp_lcd_panel_io_callbacks_t *callbacks,
    void *user_context);
static void bounded_post_transaction(spi_transaction_t *transaction);
static esp_err_t recycle_transaction(
    co5300_bounded_panel_io_t *bounded_io,
    spi_transaction_t *expected,
    bool *inflight);
static void prepare_transaction_header(spi_transaction_t *transaction,
                                       int lcd_cmd);

esp_err_t co5300_bounded_panel_io_new(
    esp_lcd_spi_bus_handle_t bus,
    const esp_lcd_panel_io_spi_config_t *io_config,
    esp_lcd_panel_io_handle_t *ret_io)
{
    if (io_config == NULL || ret_io == NULL ||
        io_config->cs_gpio_num < 0 ||
        io_config->dc_gpio_num >= 0 ||
        io_config->lcd_cmd_bits != 32 ||
        io_config->lcd_param_bits != 8 ||
        !io_config->flags.quad_mode ||
        io_config->flags.octal_mode ||
        io_config->trans_queue_depth < 2U)
    {
        return ESP_ERR_INVALID_ARG;
    }

    co5300_bounded_panel_io_t *bounded_io =
        calloc(1, sizeof(*bounded_io));
    if (bounded_io == NULL)
    {
        return ESP_ERR_NO_MEM;
    }

    const spi_device_interface_config_t device_config = {
        .command_bits = 8,
        .address_bits = 24,
        .mode = io_config->spi_mode,
        .clock_speed_hz = (int)io_config->pclk_hz,
        .spics_io_num = io_config->cs_gpio_num,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = (int)io_config->trans_queue_depth,
        .post_cb = bounded_post_transaction,
        .cs_ena_pretrans = io_config->cs_ena_pretrans,
        .cs_ena_posttrans = io_config->cs_ena_posttrans,
    };
    esp_err_t error = spi_bus_add_device(
        (spi_host_device_t)bus,
        &device_config,
        &bounded_io->spi_device);
    if (error != ESP_OK)
    {
        free(bounded_io);
        return error;
    }
    error = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP,
                               0,
                               "co5300_color",
                               &bounded_io->color_pm_lock);
    if (error != ESP_OK)
    {
        (void)spi_bus_remove_device(bounded_io->spi_device);
        free(bounded_io);
        return error;
    }
    atomic_store(&bounded_io->color_pm_lock_held, false);

    bounded_io->base.rx_param = bounded_rx_param;
    bounded_io->base.tx_param = bounded_tx_param;
    bounded_io->base.tx_color = bounded_tx_color;
    bounded_io->base.del = bounded_delete;
    bounded_io->base.register_event_callbacks =
        bounded_register_callbacks;
    bounded_io->color_done_cb = io_config->on_color_trans_done;
    bounded_io->callback_context = io_config->user_ctx;
    bounded_io->timeout_ticks =
        pdMS_TO_TICKS(CO5300_BOUNDED_IO_TIMEOUT_MS);
    if (bounded_io->timeout_ticks == 0U)
    {
        bounded_io->timeout_ticks = 1U;
    }
    *ret_io = &bounded_io->base;
    ESP_LOGI(TAG,
             "CO5300 有界 QSPI panel IO 已创建：事务超时=%u ms，队列=%u",
             CO5300_BOUNDED_IO_TIMEOUT_MS,
             (unsigned)io_config->trans_queue_depth);
    return ESP_OK;
}

static esp_err_t bounded_rx_param(esp_lcd_panel_io_t *io,
                                  int lcd_cmd,
                                  void *param,
                                  size_t param_size)
{
    (void)io;
    (void)lcd_cmd;
    (void)param;
    (void)param_size;
    return ESP_ERR_NOT_SUPPORTED;
}

static esp_err_t bounded_tx_param(esp_lcd_panel_io_t *io,
                                  int lcd_cmd,
                                  const void *param,
                                  size_t param_size)
{
    if (io == NULL ||
        (param_size > 0U && param == NULL) ||
        param_size > CO5300_BOUNDED_PARAM_CAPACITY)
    {
        return ESP_ERR_INVALID_ARG;
    }
    co5300_bounded_panel_io_t *bounded_io =
        __containerof(io, co5300_bounded_panel_io_t, base);
    esp_err_t error = recycle_transaction(
        bounded_io,
        &bounded_io->color_transaction,
        &bounded_io->color_inflight);
    if (error != ESP_OK)
    {
        return error;
    }
    error = recycle_transaction(
        bounded_io,
        &bounded_io->parameter_transaction,
        &bounded_io->parameter_inflight);
    if (error != ESP_OK)
    {
        return error;
    }

    memset(&bounded_io->parameter_transaction,
           0,
           sizeof(bounded_io->parameter_transaction));
    prepare_transaction_header(
        &bounded_io->parameter_transaction, lcd_cmd);
    if (param_size > 0U)
    {
        memcpy(bounded_io->parameter_buffer, param, param_size);
        bounded_io->parameter_transaction.length =
            param_size * 8U;
        bounded_io->parameter_transaction.tx_buffer =
            bounded_io->parameter_buffer;
    }
    bounded_io->parameter_transaction.user = bounded_io;
    error = spi_device_queue_trans(
        bounded_io->spi_device,
        &bounded_io->parameter_transaction,
        bounded_io->timeout_ticks);
    if (error != ESP_OK)
    {
        return error;
    }
    bounded_io->parameter_inflight = true;
    return recycle_transaction(
        bounded_io,
        &bounded_io->parameter_transaction,
        &bounded_io->parameter_inflight);
}

static esp_err_t bounded_tx_color(esp_lcd_panel_io_t *io,
                                  int lcd_cmd,
                                  const void *color,
                                  size_t color_size)
{
    if (io == NULL || color == NULL || color_size == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    co5300_bounded_panel_io_t *bounded_io =
        __containerof(io, co5300_bounded_panel_io_t, base);
    esp_err_t error = recycle_transaction(
        bounded_io,
        &bounded_io->parameter_transaction,
        &bounded_io->parameter_inflight);
    if (error != ESP_OK)
    {
        return error;
    }
    error = recycle_transaction(
        bounded_io,
        &bounded_io->color_transaction,
        &bounded_io->color_inflight);
    if (error != ESP_OK)
    {
        return error;
    }

    memset(&bounded_io->color_transaction,
           0,
           sizeof(bounded_io->color_transaction));
    prepare_transaction_header(&bounded_io->color_transaction, lcd_cmd);
    bounded_io->color_transaction.flags = SPI_TRANS_MODE_QIO;
    bounded_io->color_transaction.length = color_size * 8U;
    bounded_io->color_transaction.tx_buffer = color;
    bounded_io->color_transaction.user = bounded_io;
    error = esp_pm_lock_acquire(bounded_io->color_pm_lock);
    if (error != ESP_OK)
    {
        return error;
    }
    atomic_store(&bounded_io->color_pm_lock_held, true);
    error = spi_device_queue_trans(
        bounded_io->spi_device,
        &bounded_io->color_transaction,
        bounded_io->timeout_ticks);
    if (error == ESP_OK)
    {
        bounded_io->color_inflight = true;
    }
    else if (atomic_exchange(&bounded_io->color_pm_lock_held, false))
    {
        (void)esp_pm_lock_release(bounded_io->color_pm_lock);
    }
    return error;
}

static esp_err_t bounded_delete(esp_lcd_panel_io_t *io)
{
    if (io == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    co5300_bounded_panel_io_t *bounded_io =
        __containerof(io, co5300_bounded_panel_io_t, base);
    esp_err_t error = recycle_transaction(
        bounded_io,
        &bounded_io->parameter_transaction,
        &bounded_io->parameter_inflight);
    if (error == ESP_OK)
    {
        error = recycle_transaction(
            bounded_io,
            &bounded_io->color_transaction,
            &bounded_io->color_inflight);
    }
    if (error != ESP_OK)
    {
        return error;
    }
    if (atomic_exchange(&bounded_io->color_pm_lock_held, false))
    {
        (void)esp_pm_lock_release(bounded_io->color_pm_lock);
    }
    error = spi_bus_remove_device(bounded_io->spi_device);
    if (error != ESP_OK)
    {
        return error;
    }
    error = esp_pm_lock_delete(bounded_io->color_pm_lock);
    if (error != ESP_OK)
    {
        return error;
    }
    free(bounded_io);
    return ESP_OK;
}

static esp_err_t bounded_register_callbacks(
    esp_lcd_panel_io_handle_t io,
    const esp_lcd_panel_io_callbacks_t *callbacks,
    void *user_context)
{
    if (io == NULL || callbacks == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    co5300_bounded_panel_io_t *bounded_io =
        __containerof(io, co5300_bounded_panel_io_t, base);
    bounded_io->color_done_cb = callbacks->on_color_trans_done;
    bounded_io->callback_context = user_context;
    return ESP_OK;
}

static void bounded_post_transaction(spi_transaction_t *transaction)
{
    if (transaction == NULL || transaction->user == NULL)
    {
        return;
    }
    co5300_bounded_panel_io_t *bounded_io = transaction->user;
    if (transaction == &bounded_io->color_transaction)
    {
        if (atomic_exchange(&bounded_io->color_pm_lock_held, false))
        {
            (void)esp_pm_lock_release(bounded_io->color_pm_lock);
        }
        if (bounded_io->color_done_cb != NULL)
        {
            (void)bounded_io->color_done_cb(
                &bounded_io->base,
                NULL,
                bounded_io->callback_context);
        }
    }
}

static esp_err_t recycle_transaction(
    co5300_bounded_panel_io_t *bounded_io,
    spi_transaction_t *expected,
    bool *inflight)
{
    if (bounded_io == NULL || expected == NULL || inflight == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (!*inflight)
    {
        return ESP_OK;
    }
    spi_transaction_t *completed = NULL;
    const esp_err_t error = spi_device_get_trans_result(
        bounded_io->spi_device,
        &completed,
        bounded_io->timeout_ticks);
    if (error != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "CO5300 QSPI 事务结果在 %u ms 内未返回，错误=0x%x",
                 CO5300_BOUNDED_IO_TIMEOUT_MS,
                 (unsigned)error);
        return error;
    }
    if (completed != expected)
    {
        ESP_LOGE(TAG, "CO5300 QSPI 事务结果顺序异常");
        return ESP_ERR_INVALID_STATE;
    }
    *inflight = false;
    return ESP_OK;
}

static void prepare_transaction_header(spi_transaction_t *transaction,
                                       int lcd_cmd)
{
    const uint32_t encoded = (uint32_t)lcd_cmd;
    transaction->cmd = (uint16_t)((encoded >> 24U) & 0xFFU);
    transaction->addr = encoded & 0x00FFFFFFU;
}
