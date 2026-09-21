/**
 * @file     co5300_bounded_panel_io.h
 * @brief    CO5300 项目专用有界 QSPI panel IO 接口
 * @details  以单事务 8 位 opcode 加 24 位地址表达 CO5300 命令头，避免显示热路径进入无界 SPI 等待。
 * @author   ZHC
 * @date     2026-07-30
 */

#ifndef LEGBOT_CO5300_BOUNDED_PANEL_IO_H
#define LEGBOT_CO5300_BOUNDED_PANEL_IO_H

#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_io.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief 创建 CO5300 项目专用有界 QSPI panel IO
 * @details 仅支持本项目固定的 32 位 QSPI 命令头、8 位参数和四线颜色数据。
 * @param bus 已初始化的 SPI host
 * @param io_config CO5300 QSPI IO 配置
 * @param ret_io 输出 panel IO 句柄
 * @return ESP_OK 成功，其他值表示参数、内存或 SPI device 初始化失败
 */
esp_err_t co5300_bounded_panel_io_new(
    esp_lcd_spi_bus_handle_t bus,
    const esp_lcd_panel_io_spi_config_t *io_config,
    esp_lcd_panel_io_handle_t *ret_io);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CO5300_BOUNDED_PANEL_IO_H */
