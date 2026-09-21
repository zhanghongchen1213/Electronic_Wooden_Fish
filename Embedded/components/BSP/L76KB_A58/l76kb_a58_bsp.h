/**
 * @file     l76kb_a58_bsp.h
 * @brief    L76KB-A58 GPS UART 与 WAKE BSP 接口
 * @details  独占板级 UART1、IO8/IO9/IO3，并提供可停止的有界读取与幂等资源生命周期。
 * @author   ZHC
 * @date     2026-07-13
 */

#ifndef LEGBOT_L76KB_A58_BSP_H
#define LEGBOT_L76KB_A58_BSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_resources.h"
#include "esp_err.h"

/** GPS UART 固定波特率。 */
#define L76KB_A58_UART_BAUD_RATE 9600U
/** GPS UART 固定使用 8 data bits。 */
#define L76KB_A58_UART_DATA_BITS UART_DATA_8_BITS
/** GPS UART 固定关闭 parity。 */
#define L76KB_A58_UART_PARITY UART_PARITY_DISABLE
/** GPS UART 固定使用 1 stop bit。 */
#define L76KB_A58_UART_STOP_BITS UART_STOP_BITS_1
/** GPS UART 固定关闭硬件流控。 */
#define L76KB_A58_UART_FLOW_CONTROL UART_HW_FLOWCTRL_DISABLE
/** GPS UART RX ring buffer 容量，必须大于 ESP32-S3 UART 硬件 FIFO。 */
#define L76KB_A58_UART_RX_BUFFER_SIZE 512U
/** GPS UART 不创建 TX ring buffer。 */
#define L76KB_A58_UART_TX_BUFFER_SIZE 0U
/** 单次 UART 读取允许的最大等待毫秒数。 */
#define L76KB_A58_UART_READ_MAX_WAIT_MS 20U
/** BSP 驱动可用时的稳定状态码。 */
#define L76KB_A58_ERROR_OK "GPS_OK"
/** WAKE 电气策略不是高阻释放策略时的稳定失败码。 */
#define L76KB_A58_ERROR_WAKE_POLICY_UNVERIFIED "GPS_WAKE_POLICY_UNVERIFIED"
/** BSP 资源表与 GPS 固定合同不一致时的稳定失败码。 */
#define L76KB_A58_ERROR_RESOURCE_INVALID "DRV_GPS_RESOURCE_INVALID"
/** UART 参数配置失败时的稳定失败码。 */
#define L76KB_A58_ERROR_UART_CONFIG "DRV_GPS_UART_CONFIG"
/** UART 引脚路由失败时的稳定失败码。 */
#define L76KB_A58_ERROR_UART_PIN "DRV_GPS_UART_PIN"
/** UART ring buffer 驱动安装失败时的稳定失败码。 */
#define L76KB_A58_ERROR_UART_INSTALL "DRV_GPS_UART_INSTALL"
/** UART RX ring buffer 清空失败时的稳定失败码。 */
#define L76KB_A58_ERROR_UART_FLUSH "DRV_GPS_UART_FLUSH"
/** WAKE GPIO 配置失败时的稳定失败码。 */
#define L76KB_A58_ERROR_WAKE_CONFIG "DRV_GPS_WAKE_CONFIG"
/** UART 有界读取失败时的稳定失败码。 */
#define L76KB_A58_ERROR_UART_READ "DRV_GPS_UART_READ"
/** 驱动尚未启动时的稳定失败码。 */
#define L76KB_A58_ERROR_NOT_STARTED "GPS_DRIVER_NOT_STARTED"

typedef enum
{
    L76KB_A58_WAKE_POLICY_UNVERIFIED = 0, /**< 有效电平和时序尚无权威证据，必须 fail-closed。 */
    L76KB_A58_WAKE_POLICY_ACTIVE_HIGH,    /**< 经权威资料或实机波形确认高电平有效。 */
    L76KB_A58_WAKE_POLICY_ACTIVE_LOW,     /**< 经权威资料或实机波形确认低电平有效。 */
    L76KB_A58_WAKE_POLICY_RELEASED_INPUT, /**< 不主动拉高或拉低 WAKE，保持无内部上下拉的高阻释放态。 */
} l76kb_a58_wake_policy_t;

/** 当前产品 WAKE 工作策略；释放为高阻输入后由模块内部上拉退出待机。 */
#define L76KB_A58_WAKE_POLICY L76KB_A58_WAKE_POLICY_RELEASED_INPUT

/**
 * @brief 获取 L76KB-A58 GPS BSP 资源描述
 * @return L76KB-A58 资源描述指针
 */
const legbot_bsp_resource_t *l76kb_a58_bsp_resource(void);

/**
 * @brief 将 GPS WAKE 无毛刺保持为低电平安全待机
 * @details 仅配置 IO3，不准备 GPS 生命周期、不初始化 UART1，也不创建任务或发送命令。
 * @return ESP_OK 已保持待机；其他值表示资源或 GPIO 配置失败
 */
esp_err_t l76kb_a58_bsp_hold_standby(void);

/**
 * @brief 校验 GPS 板级资源与 WAKE 策略并准备生命周期
 * @return ESP_OK 可启动；ESP_ERR_NOT_SUPPORTED 表示 WAKE 策略未验证；其他值表示资源非法
 */
esp_err_t l76kb_a58_bsp_prepare(void);

/**
 * @brief 保持 WAKE 低电平并按 Arduino 实测基线启动 UART1
 * @details 初始化过程不激活接收机、不主动发送 PCAS 命令；上层取得 GPS 射频 owner 后再显式切换 active。
 * @return ESP_OK 成功；其他值表示部分初始化已逆序清理
 */
esp_err_t l76kb_a58_bsp_start(void);

/**
 * @brief 释放 WAKE 为无上下拉高阻输入并激活接收机
 * @details UART 驱动保持安装，不发送任何接收机命令。
 * @return ESP_OK 成功或已经激活，其他值表示 GPIO 配置失败
 */
esp_err_t l76kb_a58_bsp_set_active(void);

/**
 * @brief 将 WAKE 输出低电平使接收机进入待机
 * @details UART 驱动保持安装，后续可通过 set_active 继续使用同一驱动。
 * @return ESP_OK 成功或已经待机，其他值表示 GPIO 配置失败
 */
esp_err_t l76kb_a58_bsp_set_standby(void);

/**
 * @brief 清空 GPS UART RX ring buffer
 * @return ESP_OK 已清空，ESP_ERR_INVALID_STATE 表示 UART 尚未安装
 */
esp_err_t l76kb_a58_bsp_flush_input(void);

/**
 * @brief 从 GPS UART 有界读取字节
 * @param buffer 接收缓冲区
 * @param capacity 接收缓冲区容量
 * @param timeout_ms 期望等待毫秒数，内部强制限制到最大预算
 * @return 大于等于 0 表示读取字节数，-1 表示参数、状态或驱动错误
 */
int l76kb_a58_bsp_read(uint8_t *buffer, size_t capacity, uint32_t timeout_ms);

/** @brief 请求当前或下一次 UART 读取尽快停止。 */
void l76kb_a58_bsp_request_stop(void);

/**
 * @brief 关闭 UART 读取入口并释放 WAKE/UART 资源
 * @return ESP_OK 成功或已释放，其他值表示清理过程中至少一个驱动调用失败
 */
esp_err_t l76kb_a58_bsp_stop(void);

/**
 * @brief 幂等结束 BSP 生命周期并恢复可再次 prepare 状态
 * @return ESP_OK 成功或已结束，其他值表示资源清理失败
 */
esp_err_t l76kb_a58_bsp_deinit(void);

/**
 * @brief 查询当前 BSP 驱动是否已完整启动
 * @return true UART 与 WAKE 已可用，false 未启动或已停止
 */
bool l76kb_a58_bsp_is_started(void);

/**
 * @brief 查询接收机是否处于 WAKE 高阻释放工作态
 * @return true 已激活，false 待机或驱动未启动
 */
bool l76kb_a58_bsp_is_active(void);

/**
 * @brief 获取最近一次稳定 GPS_/DRV_ 诊断码
 * @return 编译期稳定字符串指针
 */
const char *l76kb_a58_bsp_last_error_code(void);

#endif /* LEGBOT_L76KB_A58_BSP_H */
