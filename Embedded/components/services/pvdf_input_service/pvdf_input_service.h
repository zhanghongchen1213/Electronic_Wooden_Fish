/**
 * @file     pvdf_input_service.h
 * @brief    PVDF 候选有效输入边界服务接口
 * @details  服务在任务上下文完成 ADC 有界采样与二次确认判定，ISR 只取单调时间戳并投递
 *           ISR-safe 消息。本服务只产出**候选**事件，不建有效敲击队列、不写任何正式累计、
 *           不保存或上报原始波形，也不读取 QMI8658A 或充电状态。
 * @author   ZHC
 * @date     2026-09-22
 */

#ifndef EWF_PVDF_INPUT_SERVICE_H
#define EWF_PVDF_INPUT_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"
#include "pvdf_confirm_policy.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 服务在统一服务表中的固定 ID。 */
#define LEGBOT_PVDF_INPUT_SERVICE_ID LEGBOT_SERVICE_PVDF

/** pvdf_task 私有 typed 命令队列深度。 */
#define EWF_PVDF_INPUT_SERVICE_QUEUE_DEPTH 8U

/** 候选有效输入在事件总线上的稳定 code；来源固定 LEGBOT_EVENT_SOURCE_PVDF。 */
#define EWF_PVDF_EVENT_CANDIDATE 1U

/** 候选事件的产品名，架构约定事件来源固定 physical_pvdf/device_touch。 */
#define EWF_PVDF_EVENT_SOURCE_NAME "physical_pvdf"

/** 服务任务的稳定名，供自检与日志引用。 */
#define EWF_PVDF_INPUT_SERVICE_TASK_NAME "pvdf_task"

    /** pvdf_task 当前可观测事实的不可变快照，只暴露计数与 typed 原因。 */
    typedef struct
    {
        uint32_t wake_event_count;                      /**< 已交接的比较器唤醒事件数。 */
        uint32_t candidate_count;                       /**< 已产出的候选有效输入数。 */
        uint32_t rejected_counts[EWF_PVDF_EVENT_COUNT]; /**< 按 typed 结论分类的拒绝计数。 */
        ewf_pvdf_event_kind_t last_kind;                /**< 最近一次 typed 结论。 */
        int32_t last_margin_millivolt;                  /**< 最近一次候选的确认裕量（mV）。 */
        bool isr_registered;                            /**< 比较器上升沿 ISR 是否已登记。 */
        bool calibration_available;                     /**< ADC 校准是否可用。 */
        bool degraded;                                  /**< 链路是否处于降级路径。 */
        bool blind_window_open;                         /**< 上电盲窗是否仍未结束。 */
    } pvdf_input_service_snapshot_t;

    /** 自检项消费的确定性重放结果。 */
    typedef struct
    {
        ewf_pvdf_selfcheck_counts_t counts; /**< 各类样本的确定性重放计数。 */
        bool all_expectations_met;          /**< 全部类别是否与编译期期望一致。 */
    } pvdf_selfcheck_result_t;

    /**
     * @brief 创建 pvdf_task 私有 typed 命令队列
     * @return ESP_OK 成功或已创建，ESP_ERR_NO_MEM 队列创建失败
     */
    esp_err_t pvdf_input_service_init_contracts(void);

    /**
     * @brief 在框架创建 pvdf_task 前进入 prepared 状态
     * @return ESP_OK 已准备，ESP_ERR_INVALID_STATE 队列不可用或任务已活动
     */
    esp_err_t pvdf_input_service_prepare_run(void);

    /** @brief 回滚未成功创建的 pvdf_task prepared 状态。 */
    void pvdf_input_service_cancel_prepared_run(void);

    /**
     * @brief 删除未运行 pvdf_task 的 typed 命令队列
     * @return ESP_OK 成功或未创建，ESP_ERR_INVALID_STATE 任务正在活动
     */
    esp_err_t pvdf_input_service_deinit_contracts(void);

    /**
     * @brief 进入 pvdf_task 循环
     * @details 登记比较器上升沿 ISR，结束后进入有界采样与二次确认循环；
     *          只有 CANDIDATE_TAP 才发布候选事件。
     * @return ESP_OK 已安全退出，其他值表示框架级错误
     */
    esp_err_t pvdf_input_service_run(void);

    /**
     * @brief 请求停止 pvdf_task
     * @param timeout_ticks 等待命令队列空间的有界 tick 数
     * @return ESP_OK 已入队
     *         ESP_ERR_INVALID_STATE 任务未活动
     *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
     */
    esp_err_t pvdf_input_service_request_stop(TickType_t timeout_ticks);

    /**
     * @brief 复制当前不可变快照
     * @param snapshot 快照输出
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效，ESP_ERR_INVALID_STATE 尚未启动
     */
    esp_err_t pvdf_input_service_snapshot(pvdf_input_service_snapshot_t *snapshot);

    /**
     * @brief 获取 pvdf_task 私有 typed 命令队列
     * @return 队列句柄，尚未创建时为 NULL
     */
    QueueHandle_t pvdf_input_service_queue(void);

    /**
     * @brief ISR 采样完成后的比较器唤醒重武装入口
     * @details 遵循"电平保持期间先停中断、owner 采样后重武装"的既有语义，
     *          避免比较器输出保持高电平期间的重复中断风暴。
     * @return ESP_OK 重武装成功，ESP_ERR_INVALID_STATE 链路不可用，其它值表示 GPIO 配置失败
     */
    esp_err_t pvdf_input_service_rearm_isr(void);

    /**
     * @brief 用编译期固定的可复现样本表执行一次确定性重放
     * @details 固件自检与主机测试共用 pvdf_confirm_policy 的同一张样本表与同一个重放函数。
     * @param out 重放结果输出，非 NULL
     * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 输出指针无效
     */
    esp_err_t pvdf_input_service_selfcheck(pvdf_selfcheck_result_t *out);

    /**
     * @brief 获取 typed 结论的稳定 ASCII 名称
     * @param kind typed 结论
     * @return 稳定名称；未知结论返回 "NONE"
     */
    const char *pvdf_input_service_event_name(ewf_pvdf_event_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif /* EWF_PVDF_INPUT_SERVICE_H */
