/**
 * @file     ui_service.h
 * @brief    单一 UI 所有权服务接口
 * @details  定义跨任务 UI 请求队列，并由 ui_task 独占 LVGL display、input、SquareLine binding 与 timer handler。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef LEGBOT_UI_SERVICE_H
#define LEGBOT_UI_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 亮屏且无 LVGL deadline 时的最长时钟刷新等待；请求与触摸中断会提前唤醒。 */
#define UI_SERVICE_MAX_LOOP_DELAY_MS 1000
/** UI typed 请求队列固定容量。 */
#define UI_SERVICE_QUEUE_DEPTH 8U
/** UI 首帧完整模型、维护快照与 LVGL 事件调用链的 IDF 字节口径栈预算。 */
#define UI_SERVICE_TASK_STACK_BYTES 16384U

    typedef struct
    {
        bool display_on;              /**< ui_task 已确认的显示开关事实。 */
        bool consume_until_release;   /**< 唤醒首触序列尚未完全释放。 */
    } ui_wake_touch_gate_t;

    typedef enum
    {
        UI_SERVICE_REQUEST_MODEL_UPDATE = 0,       /**< UI 模型更新请求。 */
        UI_SERVICE_REQUEST_WAKE,                   /**< 亮屏请求。 */
        UI_SERVICE_REQUEST_SLEEP,                  /**< 熄屏请求。 */
        UI_SERVICE_REQUEST_NEXT_RESIDENT_PAGE,     /**< 切换到下一个常驻页面。 */
        UI_SERVICE_REQUEST_DISPLAY_RESET,          /**< 显示与触摸恢复请求。 */
        UI_SERVICE_REQUEST_MAINTENANCE_CANDIDATES, /**< BLE 候选快照已更新。 */
        UI_SERVICE_REQUEST_SELFTEST_ITEM,          /**< 请求指定自检人工判定状态。 */
        UI_SERVICE_REQUEST_SELFTEST_REFRESH,       /**< 自检快照已更新。 */
        UI_SERVICE_REQUEST_STOP,                   /**< 停止 UI 任务请求。 */
    } ui_service_request_type_t;

    typedef struct
    {
        ui_service_request_type_t type; /**< UI 请求类型。 */
        uint32_t value;                 /**< 请求附加值，由请求类型解释。 */
        uint32_t run_id;                /**< 自检运行 ID，非自检请求为 0。 */
        selftest_item_id_t item_id;     /**< 自检项目 ID，非自检请求为 SELFTEST_ITEM_COUNT。 */
    } ui_service_request_t;

    /**
     * @brief 初始化首触唤醒消费门
     * @param gate 调用方拥有的触摸门状态
     * @param display_on 当前已确认的显示事实
     */
    void ui_wake_touch_gate_init(ui_wake_touch_gate_t *gate,
                                 bool display_on);

    /**
     * @brief 同步 ui_task 已确认的显示事实
     * @param gate 调用方拥有的触摸门状态
     * @param display_on 当前已确认的显示事实
     */
    void ui_wake_touch_gate_set_display(ui_wake_touch_gate_t *gate,
                                        bool display_on);

    /**
     * @brief 过滤原始触摸按压并消费熄屏后的首个完整触摸序列
     * @param gate 调用方拥有的触摸门状态
     * @param raw_pressed 本轮 CST9217 是否报告有效按压
     * @param wake_requested 首次熄屏按下时输出一次唤醒请求
     * @return true 可向 LVGL 报告 PRESSED，false 必须报告 RELEASED
     */
    bool ui_wake_touch_gate_filter(ui_wake_touch_gate_t *gate,
                                   bool raw_pressed,
                                   bool *wake_requested);

    /**
     * @brief 判断当前 UI 事件是否允许产生按钮反馈
     * @param button_source true 表示事件来自按钮而不是背景手势或只读区域
     * @param intent_accepted true 表示当前事实门禁已接受该 intent
     * @return true 可按最终偏好请求反馈，false 必须静默
     */
    bool ui_service_should_request_feedback(bool button_source,
                                            bool intent_accepted);

    /**
     * @brief 判断维护页是否允许提交 4G 显式联网请求
     * @param network_connected 当前是否已经具备有效蜂窝网络
     * @param selftest_running 整机自检是否正在运行
     * @param lifecycle 当前 modem 生命周期
     * @return true 显示“点击连接”并允许提交；4G 能力关闭或其他门禁状态返回 false
     */
    bool ui_service_cellular_action_available(
        bool network_connected,
        bool selftest_running,
        watch_modem_lifecycle_t lifecycle);

    /**
     * @brief 初始化 UI 请求队列、显示触摸 BSP 和 PSRAM 双缓冲
     * @return ESP_OK 成功或已初始化
     *         ESP_ERR_NO_MEM 队列或双缓冲分配失败
     *         其他错误表示显示、触摸或共享 I2C 边界初始化失败
     */
    esp_err_t ui_service_init(void);

    /**
     * @brief 获取 UI 服务请求队列
     * @return UI 队列句柄，未初始化时可能为 NULL
     */
    QueueHandle_t ui_service_queue(void);

    /**
     * @brief 投递 UI 服务请求
     * @param request UI 请求内容
     * @param timeout_ticks 等待队列写入的超时时间
     * @return ESP_OK 成功
     *         ESP_ERR_INVALID_STATE UI 服务尚未初始化
     *         ESP_ERR_INVALID_ARG 参数无效
     *         ESP_ERR_TIMEOUT 队列写入超时
     */
    esp_err_t ui_service_post_request(const ui_service_request_t *request, TickType_t timeout_ticks);

    /**
     * @brief 请求 ui_task 进入指定自检人工判定状态
     * @param run_id 当前运行 ID
     * @param item_id AMOLED、触摸或振动项
     * @param timeout_ticks 等待 UI 队列空间的有界 tick 数
     * @return ESP_OK 已入队，其他值表示参数、状态或有界等待失败
     */
    esp_err_t ui_service_request_selftest_item(uint32_t run_id,
                                               selftest_item_id_t item_id,
                                               TickType_t timeout_ticks);

    /**
     * @brief 通知 ui_task 在 state apply 完成后刷新自检结果
     * @param run_id 已更新的运行 ID
     * @param timeout_ticks 保留的兼容参数；刷新使用原子合并信号且不等待队列
     * @return ESP_OK 已记录最新刷新信号，其他值表示参数或状态无效
     */
    esp_err_t ui_service_refresh_selftest_results(uint32_t run_id, TickType_t timeout_ticks);

    /**
     * @brief 在当前 ui_task 中运行 LVGL 所有权循环
     * @details 仅由服务框架创建的 ui_task 调用，函数持续运行直到收到停止请求。
     */
    void ui_service_run(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_SERVICE_H */
