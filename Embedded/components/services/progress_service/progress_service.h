/**
 * @file     progress_service.h
 * @brief    本地高水位/轮次事实 owner 服务接口。
 * @details  owner 是唯一累计入口：消费 event_bus 的 EWF_TAP_EVENT_VALID 事件，
 *           维护单事务持久化组、启动恢复、离线积压判定与 acked_total 单调收敛。
 *           消费者只经 state_service 不可变快照读取事实（AD-9），不读 owner 私有变量。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef EWF_PROGRESS_SERVICE_H
#define EWF_PROGRESS_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"

/** 高水位/轮次 owner 服务在统一服务表中的固定 ID。 */
#define LEGBOT_PROGRESS_SERVICE_ID LEGBOT_SERVICE_PROGRESS

/**
 * @brief 初始化 owner 服务契约资源
 * @return ESP_OK 成功，ESP_ERR_NO_MEM 互斥锁创建失败
 */
esp_err_t progress_service_init_contracts(void);

/**
 * @brief 执行启动恢复并进入 prepared 状态
 * @details 读取持久化事务组并重建 owner 状态：EMPTY 首启按零值初始化并落盘；
 *          版本不一致或字段校验失败 fail-closed——停止推进并报中文日志错误，
 *          不静默回退零值。恢复完成后发布初值快照。
 * @return ESP_OK 成功，其他值表示状态或发布失败
 */
esp_err_t progress_service_prepare_run(void);

/**
 * @brief 放弃 prepared 状态
 */
void progress_service_cancel_prepared_run(void);

/**
 * @brief 释放 owner 服务契约资源
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 任务仍在运行
 */
esp_err_t progress_service_deinit_contracts(void);

/**
 * @brief 进入 event_bus 事件消费循环
 * @details 每个有效敲击事件经纯逻辑推进事务组并以单次提交落盘后发布快照；
 *          积压达到 1000 时拒绝并发布队列已满 gate 事实。收到停止请求后有界退出。
 * @return ESP_OK 正常退出，其他值表示状态错误
 */
esp_err_t progress_service_run(void);

/**
 * @brief 请求 owner 服务停止
 * @param timeout_ticks 保留接口一致性；停止为原子标志置位
 * @return ESP_OK 成功，ESP_ERR_INVALID_STATE 未初始化
 */
esp_err_t progress_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 带单调校验地推进 acked_total（确认结果更新入口）
 * @details 真实确认响应由 Story 2.5 活动窗口客户端接入；本接口只建立 owner 入口。
 *          同值 no-op，倒退拒绝并报中文日志；差值回落后 gate 立即恢复接受。
 * @param new_acked_total 确认结果携带的新确认值
 * @param timeout_ticks 发布快照时的有界等待
 * @return ESP_OK 已接受或同值 no-op，其他值表示参数、倒退提交或落盘失败
 */
esp_err_t progress_service_advance_acked_total(uint32_t new_acked_total,
                                               TickType_t timeout_ticks);

#endif /* EWF_PROGRESS_SERVICE_H */
