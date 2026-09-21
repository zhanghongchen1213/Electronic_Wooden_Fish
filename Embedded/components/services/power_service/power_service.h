/**
 * @file     power_service.h
 * @brief    手环电量与日常屏幕交互服务接口
 * @details  复用固定 power_task 按绝对 deadline 采样 CW2015，并拥有 idle、PWR/QMI 事件和显示意图。
 * @author   ZHC
 * @date     2026-07-10
 */

#ifndef LEGBOT_POWER_SERVICE_H
#define LEGBOT_POWER_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "legbot_services.h"
#include "power_interaction_policy.h"

/** 电源服务在统一服务表中的固定 ID。 */
#define LEGBOT_POWER_SERVICE_ID LEGBOT_SERVICE_POWER
/** 手环电量轮询周期，兼顾变化速度和共享 I2C 占用。 */
#define POWER_SERVICE_POLL_PERIOD_MS 5000
/** 熄屏稳定态 CW2015 采样周期。 */
#define POWER_SERVICE_SCREEN_OFF_POLL_PERIOD_MS 60000U
/** 候选分类中硬件尚无新样本时的短重试间隔。 */
#define POWER_SERVICE_ACTIVE_RETRY_MS 10U
/** state/UI 队列短暂拥塞时的待提交意图重试间隔。 */
#define POWER_SERVICE_PENDING_RETRY_MS 20U
/** 目标板实测确认 PWR 按下时 GPIO46 为低电平。 */
#define POWER_SERVICE_PWR_ACTIVE_LEVEL 0U
/** 目标板实测确认的 PWR 原始输入稳定去抖窗口。 */
#define POWER_SERVICE_PWR_DEBOUNCE_MS 20U
/** 运行态 PWR 有效电平与去抖已由目标板实测确认。 */
#define POWER_SERVICE_PWR_INPUT_EVIDENCE_VERIFIED 1U
/** ui_task 唤醒 power_task 的本地活动消息类型。 */
#define POWER_SERVICE_MESSAGE_LOCAL_ACTIVITY 1U
/** ui_task 唤醒 power_task 采用最新显示执行结果的消息类型。 */
#define POWER_SERVICE_MESSAGE_RUNTIME_RESULT 2U
/** ui_task 唤醒 power_task 采用最新自动熄屏时间的消息类型。 */
#define POWER_SERVICE_MESSAGE_IDLE_TIMEOUT 3U
/** ui_task 唤醒 power_task 采用最新抬腕亮屏开关的消息类型。 */
#define POWER_SERVICE_MESSAGE_RAISE_TO_WAKE 4U
/** PWR IO46 ANYEDGE ISR 唤醒 power_task 的消息类型。 */
#define POWER_SERVICE_MESSAGE_PWR_EDGE 5U
/** QMI IO41 INT1 ISR 唤醒 power_task 的消息类型。 */
#define POWER_SERVICE_MESSAGE_QMI_WAKE 6U
/** state_task 应用自检状态后唤醒 power_task 的消息类型。 */
#define POWER_SERVICE_MESSAGE_STATE_CHANGE 7U
/** ui_task 唤醒 power_task 采用最新 BLE 页面状态的消息类型。 */
#define POWER_SERVICE_MESSAGE_BLE_PAGE_ACTIVE 8U
/** 电量发布到 state_task 的有界队列等待时间。 */
#define POWER_SERVICE_STATE_QUEUE_TIMEOUT_MS 20
/** 显示请求发布到 ui_task 的有界等待时间。 */
#define POWER_SERVICE_UI_QUEUE_TIMEOUT_MS 20
/** 有效手环电量的稳定状态码。 */
#define POWER_BATTERY_OK "POWER_BATTERY_OK"
/** 手环电量不可用的稳定错误码。 */
#define POWER_BATTERY_UNAVAILABLE "POWER_BATTERY_UNAVAILABLE"

/**
 * @brief 从本地输入 owner 合并通知一次真实用户活动
 * @details 原子保留最新活动，即使普通服务队列暂满也不会静默丢失唤醒事实。
 * @param timeout_ticks 唤醒 power_task 消息的有界队列等待
 * @return ESP_OK 已保留活动，其他值表示 power 服务尚未初始化
 */
esp_err_t power_service_notify_local_activity(TickType_t timeout_ticks);

/**
 * @brief 从 ui_task 合并回报显示或触摸运行结果
 * @details latest-only 原子槽保留实际显示状态与 typed error，普通队列暂满也不会丢失最新结果。
 * @param display_on ui_task 已确认的实际显示开关事实
 * @param error 显示、触摸或无错误状态
 * @param timeout_ticks 唤醒 power_task 消息的有界队列等待
 * @return ESP_OK 已保留结果，其他值表示参数非法或 power 服务尚未初始化
 */
esp_err_t power_service_notify_runtime_result(
    bool display_on,
    watch_power_error_t error,
    TickType_t timeout_ticks);

/**
 * @brief 向唯一 power_task 合并更新自动熄屏时间
 * @details latest-only 原子槽保留 5/15/30 秒中的最新值，普通服务队列暂满也不会重放旧值。
 * @param timeout_ms 自动熄屏毫秒数
 * @param timeout_ticks 唤醒 power_task 消息的有界队列等待
 * @return ESP_OK 已保留最新值，其他值表示档位非法或 power 服务尚未初始化
 */
esp_err_t power_service_set_idle_timeout_ms(
    uint32_t timeout_ms,
    TickType_t timeout_ticks);

/**
 * @brief 向唯一 power_task 合并更新 BLE 搜索与连接页状态
 * @details 页面进入时固定使用 30 秒熄屏，离开时恢复用户设置并重新计时。
 * @param active true BLE 页面可见，false 已离开 BLE 页面
 * @param timeout_ticks 唤醒 power_task 消息的有界队列等待
 * @return ESP_OK 已保留最新值，其他值表示 power 服务尚未初始化
 */
esp_err_t power_service_set_ble_page_active(
    bool active,
    TickType_t timeout_ticks);

/**
 * @brief 向唯一 power_task 合并更新抬腕亮屏开关
 * @details latest-only 原子槽保留最新布尔值，关闭时会立即取消候选并退出 WoM。
 * @param enabled true 开启抬腕亮屏，false 关闭
 * @param timeout_ticks 唤醒 power_task 消息的有界队列等待
 * @return ESP_OK 已保留最新值，其他值表示 power 服务尚未初始化
 */
esp_err_t power_service_set_raise_to_wake_enabled(
    bool enabled,
    TickType_t timeout_ticks);

/**
 * @brief 在 state_task 已应用自检状态后显式唤醒 power_task
 * @details 状态快照本身是单一事实源，队列暂满时可由其他 deadline 再次采用。
 * @param timeout_ticks 唤醒消息的有界队列等待
 * @return ESP_OK 消息已入队
 *         ESP_ERR_INVALID_STATE power 服务尚未初始化
 *         ESP_ERR_TIMEOUT 队列在给定时间内仍满
 */
esp_err_t power_service_notify_state_change(TickType_t timeout_ticks);

/**
 * @brief 进入固定 power_task 的电量与屏幕交互循环
 * @details 收到通用停止消息后返回，由统一服务框架回收当前任务。
 */
void power_service_run(void);

#endif /* LEGBOT_POWER_SERVICE_H */
