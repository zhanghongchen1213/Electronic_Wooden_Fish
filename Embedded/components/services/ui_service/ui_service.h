/**
 * @file     ui_service.h
 * @brief    独占 ui_task 的设备 UI 服务接口（Story 3.1 / AD-10）。
 * @details  唯一允许调用 lv_* 的任务宿主；消费 watch_state 导航快照投影壳层。
 *           熄亮屏经 co5300_bsp_set_display；完整「先整帧再 DISPON」仍 hardware_pending。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef EWF_UI_SERVICE_H
#define EWF_UI_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"

#ifdef __cplusplus
extern "C" {
#endif

/** UI 服务在统一服务表中的固定 ID。 */
#define LEGBOT_UI_SERVICE_ID LEGBOT_SERVICE_UI

/** ui_task 固定钉在 Core 1（与显示文档一致）。 */
#define EWF_UI_TASK_CORE 1
/** ui_task 栈字节。 */
#define EWF_UI_TASK_STACK_BYTES 8192U
/** ui_task 优先级。 */
#define EWF_UI_TASK_PRIORITY 5U
/** 空闲轮询周期毫秒。 */
#define EWF_UI_POLL_PERIOD_MS 33U

/**
 * @brief 初始化 UI 服务契约资源
 */
esp_err_t ui_service_init_contracts(void);

/**
 * @brief 进入 prepared（不创建 LVGL 对象；对象仅在 ui_task 内创建）
 */
esp_err_t ui_service_prepare_run(void);

/** @brief 放弃 prepared。 */
void ui_service_cancel_prepared_run(void);

/**
 * @brief 释放契约资源
 */
esp_err_t ui_service_deinit_contracts(void);

/**
 * @brief 进入 ui_task 循环：独占 LVGL、投影导航快照、协调熄亮屏
 */
esp_err_t ui_service_run(void);

/**
 * @brief 请求停止
 */
esp_err_t ui_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 主机可测：从快照推导是否应点亮显示
 */
bool ui_service_should_display_on(const watch_state_snapshot_t *snapshot);

#ifdef __cplusplus
}
#endif

#endif /* EWF_UI_SERVICE_H */
