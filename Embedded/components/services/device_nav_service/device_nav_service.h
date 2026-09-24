/**
 * @file     device_nav_service.h
 * @brief    设备导航、熄亮屏、设置持久化与立即同步请求服务接口。
 * @details  单一新增服务：消费 PWR 短按与触摸手势意图，经 typed update 发布
 *           导航/设置事实；设置存储是音量唯一可写持久化真源。不实现 AT/TLS。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_DEVICE_NAV_SERVICE_H
#define EWF_DEVICE_NAV_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "device_nav_policy.h"
#include "device_settings_policy.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"

#ifdef __cplusplus
extern "C" {
#endif

/** 导航服务在统一服务表中的固定 ID。 */
#define LEGBOT_DEVICE_NAV_SERVICE_ID LEGBOT_SERVICE_DEVICE_NAV

/**
 * @brief 初始化导航服务契约资源
 */
esp_err_t device_nav_service_init_contracts(void);

/**
 * @brief 恢复设置、应用亮度并进入 prepared
 */
esp_err_t device_nav_service_prepare_run(void);

/** @brief 放弃 prepared 状态。 */
void device_nav_service_cancel_prepared_run(void);

/**
 * @brief 释放契约资源
 */
esp_err_t device_nav_service_deinit_contracts(void);

/**
 * @brief 进入导航任务循环（空闲超时轮询）
 */
esp_err_t device_nav_service_run(void);

/**
 * @brief 请求停止
 */
esp_err_t device_nav_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 消费 power_service 交接的 PWR 按压区间
 * @param duration_ms 低电平时长
 * @param now_ms 单调毫秒
 */
esp_err_t device_nav_service_on_pwr_interval(uint32_t duration_ms,
                                             uint64_t now_ms);

/**
 * @brief 消费触摸手势或熄屏首触唤醒意图
 * @param kind 手势/唤醒输入种类
 * @param now_ms 单调毫秒
 */
esp_err_t device_nav_service_on_nav_input(ewf_nav_input_kind_t kind,
                                          uint64_t now_ms);

/**
 * @brief 有效敲击或设置写入后重置空闲计时
 */
esp_err_t device_nav_service_note_activity(uint64_t now_ms);

/**
 * @brief 读取当前导航页（供 tap_input 与木鱼触区合取）
 */
ewf_nav_page_t device_nav_service_active_page(void);

/**
 * @brief 当前屏幕是否点亮
 */
bool device_nav_service_screen_on(void);

/**
 * @brief 设置是否打开
 */
bool device_nav_service_settings_open(void);

/**
 * @brief 设置音量并落盘（音量唯一写入入口）
 */
esp_err_t device_nav_service_set_volume(uint8_t volume,
                                        TickType_t timeout_ticks);

/**
 * @brief 设置亮度并落盘 + 应用到 CO5300
 */
esp_err_t device_nav_service_set_brightness(ewf_brightness_t brightness,
                                            TickType_t timeout_ticks);

/**
 * @brief 设置熄屏时长并落盘
 */
esp_err_t device_nav_service_set_timeout_s(uint32_t timeout_s,
                                           TickType_t timeout_ticks);

/**
 * @brief 读取当前设置记录（内存副本）
 */
esp_err_t device_nav_service_get_settings(ewf_device_settings_record_t *record);

/**
 * @brief 触发立即同步：进入 pending（无通信客户端时保持 pending 并中文日志）
 */
esp_err_t device_nav_service_request_sync(TickType_t timeout_ticks);

/**
 * @brief Story 2.5 客户端回报同步终态
 */
esp_err_t device_nav_service_complete_sync(ewf_sync_status_t status,
                                           TickType_t timeout_ticks);

/**
 * @brief 应用待应用命令载荷并单调抬升 applied_revision
 * @details 仅当 command_revision > 当前 applied_revision 时写入；旧修订拒绝。
 *           volume/brightness/timeout 经既有校验；音量同时通知反馈 owner。
 * @param command_revision 响应侧最新修订
 * @param volume 0–100
 * @param brightness 亮度枚举
 * @param timeout_s 5|15|30
 * @param timeout_ticks 发布有界等待
 * @return ESP_OK 已应用或同修订 no-op；ESP_ERR_INVALID_ARG 倒退修订或字段非法
 */
esp_err_t device_nav_service_apply_command_revision(
    uint32_t command_revision,
    uint8_t volume,
    ewf_brightness_t brightness,
    uint32_t timeout_s,
    TickType_t timeout_ticks);

/**
 * @brief 读取立即同步状态（供 sync_service 轮询）
 */
ewf_sync_status_t device_nav_service_sync_status(void);

/**
 * @brief 读取立即同步请求号
 */
uint32_t device_nav_service_sync_request_id(void);

#ifdef __cplusplus
}
#endif

#endif /* EWF_DEVICE_NAV_SERVICE_H */
