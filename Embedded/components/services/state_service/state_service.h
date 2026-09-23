/**
 * @file     state_service.h
 * @brief    类型化状态聚合服务接口
 * @details  由固定 state_task 串行消费各 owner 的 typed 更新并归并授权与普通控制产品状态。
 * @author   ZHC
 * @date     2026-07-14
 */

#ifndef LEGBOT_STATE_SERVICE_H
#define LEGBOT_STATE_SERVICE_H

#include "app_state.h"
#include "freertos/task.h"
#include "legbot_services.h"

/** 状态服务在统一服务表中的固定 ID。 */
#define LEGBOT_STATE_SERVICE_ID LEGBOT_SERVICE_STATE
/** state_task 写入 watch_state 的有界互斥等待。 */
#define STATE_SERVICE_APPLY_TIMEOUT_MS 20
/** 单条状态更新遇到互斥繁忙时的最大重试次数。 */
#define STATE_SERVICE_APPLY_RETRY_COUNT 3U
/** 需要 apply ACK 的更新入队有界重试次数。 */
#define STATE_SERVICE_TERMINAL_PUBLISH_RETRY_COUNT 3U
/** 等待 state_task apply 确认的总预算。 */
#define STATE_SERVICE_APPLY_ACK_TIMEOUT_MS 750U
/** telemetry owned terminal 不可覆盖保留槽容量。 */
#define STATE_SERVICE_TELEMETRY_TERMINAL_CAPACITY 1U

typedef enum
{
    STATE_SERVICE_UPDATE_STOP = 0,        /**< 请求 state_task 停止。 */
    STATE_SERVICE_UPDATE_BATTERY,         /**< 类型化手环电量更新。 */
    STATE_SERVICE_UPDATE_POWER,           /**< 正交屏幕与本机电源更新。 */
    STATE_SERVICE_UPDATE_AUDIO,           /**< 类型化音频资源与播放状态更新。 */
    STATE_SERVICE_UPDATE_BLE,             /**< 类型化 BLE 链路与产品状态更新。 */
    STATE_SERVICE_UPDATE_CONFIG,          /**< 类型化身份与无凭据云配置更新。 */
    STATE_SERVICE_UPDATE_MODEM,           /**< 类型化 modem 生命周期与诊断最新真值。 */
    STATE_SERVICE_UPDATE_GPS,             /**< 类型化 GPS 状态、可空坐标与诊断最新真值。 */
    STATE_SERVICE_UPDATE_CONTROL,         /**< control_gate 授权、会话与普通控制更新。 */
    STATE_SERVICE_UPDATE_CLOUD,           /**< cloud_task 租赁查询诊断更新。 */
    STATE_SERVICE_UPDATE_TELEMETRY,       /**< cloud_task telemetry 独立诊断终态。 */
    STATE_SERVICE_UPDATE_CLOUD_OFFSET_PERSIST, /**< 内部栈持久化云时间偏移。 */
    STATE_SERVICE_UPDATE_SELFTEST_BEGIN,  /**< 开始一次自检摘要。 */
    STATE_SERVICE_UPDATE_SELFTEST_ITEM,   /**< 累积一个定长自检终态。 */
    STATE_SERVICE_UPDATE_SELFTEST_FINISH, /**< 固化自检完成或取消终态。 */
    STATE_SERVICE_UPDATE_SELFTEST_RETRY_BEGIN,  /**< 标记一个既有失败项进入重试。 */
    STATE_SERVICE_UPDATE_SELFTEST_RETRY_FINISH, /**< 覆盖被重试项的最新终态。 */
    STATE_SERVICE_UPDATE_TAP_GATE,         /**< 统一敲击完成/故障 gate owner 更新。 */
} state_service_update_type_t;

typedef struct
{
    state_service_update_type_t type; /**< 类型化更新类型。 */
    union
    {
        watch_battery_update_t battery;                 /**< 手环电量更新载荷。 */
        watch_power_update_t power;                     /**< 屏幕与本机电源更新载荷。 */
        watch_audio_update_t audio;                     /**< 音频状态更新载荷。 */
        watch_ble_update_t ble;                         /**< BLE 身份、链路与产品状态更新载荷。 */
        watch_config_update_t config;                   /**< 身份与无凭据云配置更新载荷。 */
        watch_modem_update_t modem;                     /**< modem 生命周期、请求与脱敏诊断载荷。 */
        watch_gps_update_t gps;                         /**< GPS 状态、可空坐标、新鲜度与诊断载荷。 */
        watch_control_update_t control;                 /**< 授权、普通控制阶段、pending 与 RAM-only 会话。 */
        watch_cloud_update_t cloud;                     /**< 租赁请求、HTTP 与 CLOUD_ 诊断。 */
        watch_telemetry_update_t telemetry;             /**< telemetry 结果、HTTP 与云时间证据。 */
        int64_t cloud_offset_ms;                        /**< 待持久化的云时间偏移。 */
        watch_selftest_begin_update_t selftest_begin;   /**< 自检 begin 小载荷。 */
        watch_selftest_item_update_t selftest_item;     /**< 自检单项终态小载荷。 */
        watch_selftest_finish_update_t selftest_finish; /**< 自检 finish 小载荷。 */
        watch_selftest_retry_begin_update_t selftest_retry_begin;   /**< 自检重试 begin 小载荷。 */
        watch_selftest_retry_finish_update_t selftest_retry_finish; /**< 自检重试 finish 小载荷。 */
        watch_tap_gate_update_t tap_gate;             /**< 统一敲击 gate owner 小载荷。 */
    } payload;                                          /**< 按 type 解释的更新载荷。 */
    TaskHandle_t apply_ack_task;                        /**< 需要确认时的稳定任务目标，其他更新为 NULL。 */
    uint32_t apply_ack_id;                              /**< apply 确认序号，不需确认时为 0。 */
} state_service_update_t;

/**
 * @brief 判断类型化状态成功应用后是否需要刷新通用 UI 模型
 * @param type 状态更新类型
 * @return true 需要通知 ui_task 读取最新快照，false 使用其他刷新路径或无需刷新
 */
bool state_service_update_requires_model_refresh(
    state_service_update_type_t type);

/**
 * @brief 向 state_task 发布类型化手环电量更新
 * @param update 电量更新
 * @param timeout_ticks 等待状态队列空间的 tick 数
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_STATE 状态队列尚未初始化
 *         ESP_ERR_INVALID_ARG 参数无效
 *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
 */
esp_err_t state_service_publish_battery(const watch_battery_update_t *update,
                                        TickType_t timeout_ticks);

/**
 * @brief 向 state_task 发布正交屏幕与本机电源更新
 * @param update 显示开关、最近本地活动、转换序号与 typed 错误
 * @param timeout_ticks 等待状态队列空间的 tick 数
 * @return ESP_OK 已入队，其他值表示参数、状态或有界队列超时
 */
esp_err_t state_service_publish_power(const watch_power_update_t *update,
                                      TickType_t timeout_ticks);

/**
 * @brief 向 state_task 发布类型化音频状态更新
 * @param update 音频资源、播放状态与错误原因
 * @param timeout_ticks 等待状态队列空间的 tick 数
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_STATE 状态队列尚未初始化
 *         ESP_ERR_INVALID_ARG 参数无效
 *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
 */
esp_err_t state_service_publish_audio(const watch_audio_update_t *update,
                                      TickType_t timeout_ticks);

/**
 * @brief 向 state_task 发布类型化 BLE 链路与外骨骼产品状态更新
 * @param update 已提交身份、链路、产品快照、新鲜度和稳定 BLE_ 码
 * @param timeout_ticks 等待状态队列空间的 tick 数
 * @return ESP_OK 成功，其他值表示参数、状态或队列超时
 */
esp_err_t state_service_publish_ble(const watch_ble_update_t *update,
                                    TickType_t timeout_ticks);

/**
 * @brief 向 state_task 发布类型化身份与无凭据云配置最新真值
 * @details 队列未启动或暂满时保留 fail-closed 优先的 latest truth，供 state_task 重投。
 * @param update 配置修订号、watch_id、脱敏状态与 CLOUD_ 错误
 * @param timeout_ticks 保留接口一致性，latest-value mailbox 不做无界等待
 * @return ESP_OK 已保留最新真值，其他值表示参数非法或 mailbox 短临界区超时
 */
esp_err_t state_service_publish_config(const watch_config_update_t *update,
                                       TickType_t timeout_ticks);

/**
 * @brief 保留并发布 modem typed 最新真值
 * @param update modem 生命周期、链路和脱敏诊断
 * @param timeout_ticks 保留接口一致性，mailbox 使用有界短临界区
 * @return ESP_OK 已保留，其他值表示参数、状态或临界区繁忙
 */
esp_err_t state_service_publish_modem(const watch_modem_update_t *update,
                                      TickType_t timeout_ticks);

/**
 * @brief 保留并发布 GPS owner 的完整 typed 最新真值
 * @param update GPS 状态、可空坐标、新鲜度、序号和稳定诊断码
 * @param timeout_ticks 保留接口一致性，mailbox 使用有界短临界区
 * @return ESP_OK 已保留，其他值表示参数、状态、旧序号或临界区繁忙
 */
esp_err_t state_service_publish_gps(const watch_gps_update_t *update,
                                    TickType_t timeout_ticks);

/**
 * @brief 向 state_task 发布 control_gate 拥有的授权与普通控制状态
 * @param update 单调序号、事务身份、租赁/普通阶段、pending 与 RAM-only 证明
 * @param timeout_ticks 等待有界 typed 队列空间的 tick 数
 * @return ESP_OK 已入队，其他值表示参数、状态或队列满
 */
esp_err_t state_service_publish_control(const watch_control_update_t *update,
                                        TickType_t timeout_ticks);

/**
 * @brief 有界发布 control 更新并等待 state_task 成功应用
 * @details 首次云查询与普通控制写入均用该接口建立产品状态先于外部副作用的 happens-before。
 * @param update 单调序号、事务身份、租赁/普通阶段与 pending 状态
 * @param timeout_ticks 每次等待状态队列空间的 tick 数
 * @return ESP_OK 已成功应用，其他值表示参数、状态、入队或 apply 确认失败
 */
esp_err_t state_service_publish_control_confirmed(
    const watch_control_update_t *update,
    TickType_t timeout_ticks);

/**
 * @brief 向 state_task 发布 cloud_task 拥有的租赁查询诊断
 * @param update 单调序号、请求 ID、HTTP 状态、本地终态与 CLOUD_ 稳定码
 * @param timeout_ticks 等待有界 typed 队列空间的 tick 数
 * @return ESP_OK 已入队，其他值表示参数、状态或队列满
 */
esp_err_t state_service_publish_cloud(const watch_cloud_update_t *update,
                                      TickType_t timeout_ticks);

/**
 * @brief 通过独立保留槽非阻塞发布 telemetry owned terminal
 * @details 已有终态尚未由 state_task 取走时返回超时，不覆盖、不吞并旧终态。
 * @param update telemetry 请求、结果、时间证据与 CLOUD_ 稳定码
 * @param timeout_ticks 保留接口一致性，短临界区不做无界等待
 * @return ESP_OK 已保留，其他值表示参数、状态或保留槽繁忙
 */
esp_err_t state_service_publish_telemetry(
    const watch_telemetry_update_t *update,
    TickType_t timeout_ticks);

/**
 * @brief 由内部栈 state_task 持久化云时间偏移
 * @details cloud_task 只入队并等待 apply ACK，不得在 PSRAM 栈上直接进入 NVS/Flash。
 * @param offset_ms server_time_ms 减当前 boot 单调毫秒的派生偏移
 * @param timeout_ticks 每次等待状态队列空间的有界 tick 数
 * @return ESP_OK 已在内部栈完成 NVS 提交，其他值表示状态、入队、ACK 或 NVS 错误
 */
esp_err_t state_service_persist_cloud_offset(
    int64_t offset_ms,
    TickType_t timeout_ticks);

/**
 * @brief 有界重试发布并等待 state_task 应用自检 begin 小载荷
 * @param update 运行 ID、首项和开始 tick
 * @param timeout_ticks 每次入队尝试的有界 tick 数
 * @return ESP_OK 已成功应用，其他值表示参数、状态、入队或 apply 确认失败
 */
esp_err_t state_service_publish_selftest_begin(const watch_selftest_begin_update_t *update,
                                               TickType_t timeout_ticks);

/**
 * @brief 有界重试发布并等待 state_task 应用一个自检 terminal 单项
 * @param update 运行 ID、定长结果和下一项
 * @param timeout_ticks 每次入队尝试的有界 tick 数
 * @return ESP_OK 已成功应用，其他值表示参数、状态、入队或 apply 确认失败
 */
esp_err_t state_service_publish_selftest_item(const watch_selftest_item_update_t *update,
                                              TickType_t timeout_ticks);

/**
 * @brief 有界重试发布并等待 state_task 应用自检 finish 小载荷
 * @param update 运行 ID、终态和结束 tick
 * @param timeout_ticks 每次入队尝试的有界 tick 数
 * @return ESP_OK 已成功应用，其他值表示参数、状态、入队或 apply 确认失败
 */
esp_err_t state_service_publish_selftest_finish(const watch_selftest_finish_update_t *update,
                                                TickType_t timeout_ticks);

/**
 * @brief 有界发布并等待 state_task 应用自检单项重试开始
 * @param update 原运行 ID、失败项 ID 和开始 tick
 * @param timeout_ticks 每次入队尝试的有界 tick 数
 * @return ESP_OK 已成功应用，其他值表示参数、状态、入队或 apply 确认失败
 */
esp_err_t state_service_publish_selftest_retry_begin(
    const watch_selftest_retry_begin_update_t *update,
    TickType_t timeout_ticks);

/**
 * @brief 有界发布并等待 state_task 应用自检单项重试终态
 * @param update 原运行 ID、最新结果和结束 tick
 * @param timeout_ticks 每次入队尝试的有界 tick 数
 * @return ESP_OK 已成功应用，其他值表示参数、状态、入队或 apply 确认失败
 */
esp_err_t state_service_publish_selftest_retry_finish(
    const watch_selftest_retry_finish_update_t *update,
    TickType_t timeout_ticks);

/**
 * @brief 请求 state_task 停止
 * @param timeout_ticks 等待状态队列空间的 tick 数
 * @return ESP_OK 成功
 *         ESP_ERR_INVALID_STATE 状态队列尚未初始化
 *         ESP_ERR_TIMEOUT 队列在有界等待内仍满
 */
esp_err_t state_service_request_stop(TickType_t timeout_ticks);

/**
 * @brief 发布统一敲击 gate owner 的完成/故障状态
 * @details 由 state_task 串行应用到不可变状态快照；敲击服务不得自行伪造 gate 状态。
 * @param update 单调序号、完成遮罩与故障锁定事实
 * @param timeout_ticks 等待状态队列空间的有界 tick 数
 * @return ESP_OK 已入队，其他值表示参数、状态或队列超时
 */
esp_err_t state_service_publish_tap_gate(
    const watch_tap_gate_update_t *update,
    TickType_t timeout_ticks);

/**
 * @brief 读取统一敲击 gate 所需的状态服务只读快照
 * @details 调用方只能取得按值复制的事实快照，不能把自造状态传入敲击服务。
 * @param service_ready 输出 state_task 与状态快照是否可用
 * @param completed 输出完成遮罩事实
 * @param fault_locked 输出故障锁定事实
 * @return ESP_OK 成功，ESP_ERR_INVALID_ARG 参数非法，ESP_ERR_INVALID_STATE 状态服务未运行，
 *         ESP_ERR_TIMEOUT 快照互斥锁繁忙
 */
esp_err_t state_service_read_tap_gate(bool *service_ready,
                                      bool *completed,
                                      bool *fault_locked);

/**
 * @brief 进入固定 state_task 的类型化更新循环
 * @details 发布者通过任务通知唤醒 owner；稳定态永久阻塞，仅配置重投锁超时使用有限 deadline。收到停止更新后返回，由统一服务框架回收当前任务。
 */
void state_service_run(void);

#ifdef STATE_SERVICE_CONTROL_TEST
/** @brief host 回归中清空 GPS latest-value mailbox。 */
void state_service_test_reset_gps_mailbox(void);

/**
 * @brief host 回归中取走 GPS latest-value mailbox
 * @param update 最新完整 GPS 真值输出
 * @return true 存在并已取走待应用真值
 */
bool state_service_test_take_gps_mailbox(watch_gps_update_t *update);

/** @brief 验证 telemetry terminal 保留槽不会覆盖未消费终态。 */
int state_service_test_telemetry_mailbox_contract(void);
#endif

#endif /* LEGBOT_STATE_SERVICE_H */
