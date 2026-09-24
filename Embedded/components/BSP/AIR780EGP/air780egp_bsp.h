/**
 * @file     air780egp_bsp.h
 * @brief    Air780EGP 调制解调器驱动公开 API。
 * @details  语义对齐经验文档 start / get_status / https_post_json / suspend。
 *           引脚：UART1 TX=IO43 RX=IO44；DTR=IO10 开漏；RST=IO15 输入高阻。
 *           数值宏标注 DEFERRED/hardware_pending；禁止应用层直读写 UART。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_AIR780EGP_BSP_H
#define EWF_AIR780EGP_BSP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** UART 端口：UART1。DEFERRED/hardware_pending。 */
#define EWF_AIR780_UART_PORT 1
/** TX GPIO = IO43。 */
#define EWF_AIR780_TX_GPIO 43
/** RX GPIO = IO44。 */
#define EWF_AIR780_RX_GPIO 44
/** DTR GPIO = IO10 开漏。 */
#define EWF_AIR780_DTR_GPIO 10
/** RST GPIO = IO15 输入高阻，不硬复位。 */
#define EWF_AIR780_RST_GPIO 15
/** 目标波特率。DEFERRED。 */
#define EWF_AIR780_BAUD_RATE 115200
/** SSL 上下文 ID（Air780EGP）。DEFERRED。 */
#define EWF_AIR780_SSL_CONTEXT_ID 153
/** 联调 seclevel=0；上线可调高。DEFERRED。 */
#define EWF_AIR780_SSL_SECLEVEL 0
/** 整笔 HTTPS deadline ≥65s。DEFERRED。 */
#define EWF_AIR780_HTTPS_DEADLINE_MS 65000U
/** HTTPS 响应正文上限。DEFERRED。 */
#define EWF_AIR780_HTTPS_RESPONSE_MAX_LEN 3356U
/** 连续 AT 超时恢复阈值。DEFERRED。 */
#define EWF_AIR780_TIMEOUT_RECOVERY_COUNT 3U

typedef struct {
    bool started;            /**< 驱动是否已 start。 */
    bool suspended;          /**< 是否处于休眠准备。 */
    bool sim_ready;          /**< SIM READY。 */
    bool attached;           /**< 附着。 */
    bool pdp_active;         /**< PDP 激活。 */
    bool ipv4_valid;         /**< IPv4 有效。 */
    bool network_ready;      /**< sim&&attached&&pdp&&ipv4。 */
    bool https_busy;         /**< 单事务占用中。 */
    int16_t csq_rssi;        /**< CSQ RSSI；未测为 -1。 */
    char last_error[48];     /**< 最近错误码（无敏感正文）。 */
} ewf_air780_status_t;

/**
 * @brief 启动模组驱动（波特率扫描不写 NVRAM；ATE0/CMEE/CSCLK）
 * @return ESP_OK 已启动；硬件未闭环前可能返回 NOT_SUPPORTED
 */
esp_err_t ewf_air780_start(void);

/**
 * @brief 读取状态快照
 * @param status 输出
 * @return ESP_OK 成功
 */
esp_err_t ewf_air780_get_status(ewf_air780_status_t *status);

/**
 * @brief 单事务 HTTPS JSON POST（队列深度 1）
 * @param url 目标 URL（调用方持有；日志只记长度）
 * @param token 设备 Token（日志脱敏）
 * @param request_json 请求正文
 * @param response_json 响应缓冲
 * @param response_capacity 响应容量
 * @param response_len 实际响应长度输出
 * @param timeout_ms 整笔 deadline（建议 ≥65000）
 * @return ESP_OK 成功；未核验硬件返回 NOT_SUPPORTED
 */
esp_err_t ewf_air780_https_post_json(const char *url,
                                     const char *token,
                                     const char *request_json,
                                     char *response_json,
                                     size_t response_capacity,
                                     size_t *response_len,
                                     uint32_t timeout_ms);

/**
 * @brief 活动窗口结束后的休眠准备（CSCLK+DTR）；失败也清理 HTTP 上下文
 * @return ESP_OK 或清理后的降级结果
 */
esp_err_t ewf_air780_suspend(void);

#ifdef __cplusplus
}
#endif

#endif /* EWF_AIR780EGP_BSP_H */
