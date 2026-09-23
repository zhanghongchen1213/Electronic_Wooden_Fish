/**
 * @file     feedback_service.h
 * @brief    统一反馈服务接口。
 * @details  反馈服务是 EWF_TAP_EVENT_VALID 的第二个业务消费者（纯消费者，
 *           AD-9/AD-12）：只读取事件事实，不回写累计/游标/gate。经自有有界
 *           typed 队列合并限速后驱动木鱼音与 RGB 反馈，并以 typed update 发布
 *           音频/RGB 通道事实与音量事实；反馈失败不阻塞 progress_service 的
 *           持久化路径。三类来源（physical_pvdf/device_touch/automatic_tap）
 *           获得的反馈完全一致（NFR8）。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_FEEDBACK_SERVICE_H
#define EWF_FEEDBACK_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"

/** 反馈服务在统一服务表中的固定 ID。 */
#define LEGBOT_FEEDBACK_SERVICE_ID LEGBOT_SERVICE_FEEDBACK

/**
 * @brief 初始化反馈服务契约资源
 * @return ESP_OK 成功，ESP_ERR_NO_MEM 队列创建失败
 */
esp_err_t feedback_service_init_contracts(void);

/**
 * @brief 执行音量恢复并进入 prepared 状态
 * @details 音量经 audio_volume_store 恢复：EMPTY 首启按默认 50 初始化并落盘；
 *          损坏或介质错误 fail-closed（报错 + 保持内存默认值 + 中文日志），
 *          不静默回退伪造。恢复完成后发布初值反馈事实。
 * @return ESP_OK 成功，其他值表示状态或发布失败
 */
esp_err_t feedback_service_prepare_run(void);

/**
 * @brief 放弃 prepared 状态
 */
void feedback_service_cancel_prepared_run(void);

/**
 * @brief 释放反馈服务契约资源
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 任务仍在运行
 */
esp_err_t feedback_service_deinit_contracts(void);

/**
 * @brief 进入事件消费与反馈输出循环
 * @details 订阅 event_bus 扇出消费 EWF_TAP_EVENT_VALID；每个事件经纯逻辑
 *           策略决策后驱动音频/RGB 通道，播放失败与灯效失败按低打扰降级并
 *           发布故障事实，绝不置位输入锁定（tap_fault_locked 生产者属 Story 2.7）。
 *           收到停止请求后有界退出。
 * @return ESP_OK 正常退出，其他值表示状态错误
 */
esp_err_t feedback_service_run(void);

/**
 * @brief 请求反馈服务停止
 * @param timeout_ticks 保留接口一致性；停止为原子标志置位
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 未初始化
 */
esp_err_t feedback_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 设置音量（带校验的唯一写入入口）
 * @details 取值域恰为整数 0-100：越界拒绝并保持原值（中文日志）；同值 no-op；
 *           新值先落盘成功再提交内存（NVS 单次提交事务边界），介质错误时保持
 *           原内存值并发布持久化失败事实。本 Story 不实现设置页 UI（Story 2.4），
 *           云端命令应用属 Epic 5；该入口保证音量 owner 唯一、可校验、可持久化。
 * @param volume 目标音量 0-100
 * @param timeout_ticks 发布反馈事实时的有界等待
 * @return ESP_OK 已接受或同值 no-op，ESP_ERR_INVALID_ARG 越界拒绝，
 *         其他值表示落盘或发布失败
 */
esp_err_t feedback_service_set_volume(uint8_t volume, TickType_t timeout_ticks);

#endif /* EWF_FEEDBACK_SERVICE_H */
