/**
 * @file     co5300_bsp.h
 * @brief    CO5300 AMOLED 项目 BSP 接口
 * @details  固化显示参数并封装 QSPI 初始化、绘制、熄亮屏和幂等复位能力。
 * @author   ZHC
 * @date     2026-07-14
 */

#ifndef LEGBOT_CO5300_BSP_H
#define LEGBOT_CO5300_BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** CO5300 可视区域宽度。 */
#define CO5300_BSP_WIDTH 410
/** CO5300 可视区域高度。 */
#define CO5300_BSP_HEIGHT 502
/** CO5300 X 方向固定偏移。 */
#define CO5300_BSP_X_GAP 22
/** CO5300 Y 方向固定偏移。 */
#define CO5300_BSP_Y_GAP 0
/** CO5300 RGB565 像素位数。 */
#define CO5300_BSP_BITS_PER_PIXEL 16
/** CO5300 QSPI 像素时钟，满足 410×502 RGB565 稳定 60 FPS 的物理带宽。 */
#define CO5300_BSP_QSPI_CLOCK_HZ (80 * 1000 * 1000)
/** LVGL 每个内部 DMA 局部绘制缓冲区的行数。 */
#define CO5300_BSP_DRAW_BUFFER_LINES 16
/** CO5300 原始亮度最小值。 */
#define CO5300_BSP_BRIGHTNESS_MIN 0U
/** CO5300 原始亮度最大值。 */
#define CO5300_BSP_BRIGHTNESS_MAX 255U
/** CO5300 驱动初始化表中的默认亮度。 */
#define CO5300_BSP_BRIGHTNESS_DEFAULT CO5300_BSP_BRIGHTNESS_MAX
/** CO5300 QSPI 参数写入格式编码后的 0x51 亮度命令。 */
#define CO5300_BSP_QSPI_BRIGHTNESS_COMMAND 0x02005100U
/** CO5300 QSPI 参数写入格式编码后的 SLPIN(0x10) 命令。 */
#define CO5300_BSP_QSPI_SLEEP_IN_COMMAND 0x02001000U
/** CO5300 QSPI 参数写入格式编码后的 SLPOUT(0x11) 命令。 */
#define CO5300_BSP_QSPI_SLEEP_OUT_COMMAND 0x02001100U
/** SLPIN 后等待面板 booster 完全关闭的最小时间。 */
#define CO5300_BSP_SLEEP_IN_SETTLE_MS 120U
/** SLPOUT 后等待面板 booster 完全开启的最小时间。 */
#define CO5300_BSP_SLEEP_OUT_SETTLE_MS 120U
/** AMOLED 偏压使能后到面板复位的最小稳定时间。 */
#define CO5300_BSP_POWER_RAIL_SETTLE_MS 10U

/**
 * @brief 显示颜色传输完成回调
 * @param user_ctx 调用方上下文
 * @return true 表示唤醒了更高优先级任务
 *         false 表示无需立即切换任务
 */
typedef bool (*co5300_bsp_flush_done_cb_t)(void *user_ctx);

typedef struct {
    co5300_bsp_flush_done_cb_t flush_done_cb; /**< 颜色传输完成回调，仅允许执行 ISR 安全操作。 */
    void *user_ctx;                           /**< 颜色传输完成回调上下文。 */
} co5300_bsp_config_t;

typedef struct {
    uint32_t command; /**< 适配 CO5300 QSPI panel IO 的编码命令。 */
    uint8_t value;    /**< 写入 0x51 寄存器的原始亮度值。 */
} co5300_bsp_brightness_command_t;

/**
 * @brief 获取 CO5300 屏幕 BSP 资源描述
 * @return CO5300 资源描述指针
 */
const legbot_bsp_resource_t *co5300_bsp_resource(void);

/**
 * @brief 幂等初始化 CO5300 QSPI 显示链路
 * @details 初始化后保持 DISPOFF，调用方应先写入确定首帧，再通过
 *          co5300_bsp_set_display(true) 显示画面。
 * @param config 项目显示配置
 * @return ESP_OK 成功或已初始化
 *         ESP_ERR_INVALID_ARG 配置无效
 *         其他错误表示 GPIO、SPI、panel IO 或 panel 初始化失败
 */
esp_err_t co5300_bsp_init(const co5300_bsp_config_t *config);

/**
 * @brief 释放 CO5300 显示链路资源
 * @return ESP_OK 成功或未初始化
 *         其他错误表示底层资源释放失败
 */
esp_err_t co5300_bsp_deinit(void);

/**
 * @brief 绘制一个 RGB565 区域
 * @param x_start 起始 X 坐标，包含
 * @param y_start 起始 Y 坐标，包含
 * @param x_end 结束 X 坐标，不包含
 * @param y_end 结束 Y 坐标，不包含
 * @param color_data RGB565 像素缓冲区
 * @return ESP_OK 已排队发送
 *         ESP_ERR_INVALID_STATE 显示未初始化
 *         ESP_ERR_INVALID_ARG 区域或缓冲区无效
 *         其他错误表示显示传输失败
 */
esp_err_t co5300_bsp_draw_bitmap(int x_start,
                                 int y_start,
                                 int x_end,
                                 int y_end,
                                 const void *color_data);

/**
 * @brief 设置屏幕显示开关
 * @param on true 亮屏，false 熄屏
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_STATE 显示未初始化
 *         其他错误表示显示命令失败
 */
esp_err_t co5300_bsp_set_display(bool on);

/**
 * @brief 将 CO5300 面板与 AMOLED 偏压电源转入极低功耗状态
 * @details 调用方必须先等待所有 LVGL 颜色传输完成。本函数依次执行
 *          DISPOFF、SLPIN、至少 120 ms 等待与 IO47 偏压断电。
 * @return ESP_OK 成功或已休眠
 *         ESP_ERR_INVALID_STATE 显示未初始化
 *         其他错误表示显示命令或 GPIO 操作失败
 */
esp_err_t co5300_bsp_suspend(void);

/**
 * @brief 从断电休眠状态恢复 CO5300 显示链路
 * @details 依次执行 IO47 上电、面板复位/初始化、SLPOUT 稳定、
 *          原亮度恢复并保持 DISPOFF。调用方写入确定首帧后再显式 DISPON；
 *          恢复失败时保持休眠事实，由调用方最多重试一次。
 * @return ESP_OK 成功或已处于运行态
 *         ESP_ERR_INVALID_STATE 显示未初始化
 *         其他错误表示 GPIO、复位、初始化或亮度恢复失败
 */
esp_err_t co5300_bsp_resume(void);

/**
 * @brief 构造 CO5300 QSPI 亮度命令
 * @param raw_level 0 到 255 的原始亮度值
 * @param command 输出的编码命令和值
 * @return ESP_OK 构造成功
 *         ESP_ERR_INVALID_ARG 参数为空或亮度越界
 */
esp_err_t co5300_bsp_build_brightness_command(
    uint16_t raw_level,
    co5300_bsp_brightness_command_t *command);

/**
 * @brief 设置 CO5300 原始亮度
 * @param raw_level 0 到 255 的原始亮度值
 * @return ESP_OK 成功或亮度未变化
 *         ESP_ERR_INVALID_ARG 亮度越界
 *         ESP_ERR_INVALID_STATE 显示未初始化
 *         其他错误表示显示命令失败
 */
esp_err_t co5300_bsp_set_brightness(uint16_t raw_level);

/**
 * @brief 在不重建设备句柄的前提下复位并恢复显示
 * @details 保留调用前的 DISPON/DISPOFF 状态，避免隐藏首帧准备期间的恢复
 *          意外提前显示面板 GRAM。
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_STATE 显示未初始化
 *         其他错误表示复位、初始化、偏移或亮屏失败
 */
esp_err_t co5300_bsp_reset(void);

/**
 * @brief 查询 CO5300 显示链路是否已经初始化
 * @return true 已初始化
 *         false 未初始化
 */
bool co5300_bsp_is_initialized(void);

/**
 * @brief 查询 CO5300 是否已完成 SLPIN 并关闭偏压电源
 * @return true 已休眠且 IO47 已拉低
 *         false 未初始化或显示链路处于运行态
 */
bool co5300_bsp_is_suspended(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CO5300_BSP_H */
