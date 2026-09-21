/**
 * @file     voice_service.h
 * @brief    BOOT0 按键离线语音控制服务接口
 * @details  声明 model 分区启动期映射、MultiNet 实例生命周期、BOOT0 长按会话、停止与运行态禁用入口。
 * @author   ZHC
 * @date     2026-08-04
 */

#ifndef LEGBOT_VOICE_SERVICE_H
#define LEGBOT_VOICE_SERVICE_H

#include <stdbool.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "legbot_services.h"
#include "voice_control_policy.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 语音服务在统一服务表中的固定 ID。 */
#define LEGBOT_VOICE_SERVICE_ID LEGBOT_SERVICE_VOICE
/** voice task 固定 PSRAM 栈大小；额外 1 KiB 为真机最深诊断路径保留安全余量。 */
#define VOICE_SERVICE_TASK_STACK_BYTES 9216U
/** BOOT0 输入去抖时间。 */
#define VOICE_SERVICE_BOOT_DEBOUNCE_MS 20U
/** BOOT0 进入语音会话的长按阈值。 */
#define VOICE_SERVICE_BOOT_LONG_PRESS_MS VOICE_CONTROL_BOOT_LONG_PRESS_MS
/** 松手后补入 MultiNet 的静音时长。 */
#define VOICE_SERVICE_DECODE_TAIL_SILENCE_MS 320U
/** MultiNet 实例创建与模型加载的验收上限。 */
#define VOICE_SERVICE_MODEL_CREATE_LIMIT_MS 5000U

    /**
     * @brief 在统一框架创建 voice task 前清理停止状态
     * @return ESP_OK 已准备，ESP_ERR_INVALID_STATE 任务仍在运行
     */
    esp_err_t voice_service_prepare_run(void);

    /**
     * @brief 在内部 main task 启动栈上完成 model 分区映射
     * @details 映射保持到本次开机结束，避免 PSRAM voice task 执行 cache 冻结型 mmap/munmap。
     * @return ESP_OK 已映射，ESP_ERR_INVALID_STATE voice task 已运行，ESP_ERR_NOT_FOUND 模型分区不可用
     */
    esp_err_t voice_service_prepare_model_mapping(void);

    /**
     * @brief 回滚未成功创建的 voice task 准备状态
     */
    void voice_service_cancel_prepared_run(void);

    /**
     * @brief 请求 voice task 停止并释放模型实例、PM lock 与 BOOT0 ISR
     * @return ESP_OK 已请求，ESP_ERR_INVALID_STATE 任务未运行
     */
    esp_err_t voice_service_request_stop(void);

    /**
     * @brief 请求 voice task 销毁模型实例并永久禁用本次启动周期的语音功能
     * @param timeout_ticks 等待 owner 完成禁用的有界 tick 数
     * @return ESP_OK 已禁用，ESP_ERR_TIMEOUT owner 未及时确认，ESP_ERR_INVALID_STATE 任务未运行
     */
    esp_err_t voice_service_disable(TickType_t timeout_ticks);

    /**
     * @brief 查询 voice task 是否已完成本轮模型初始化尝试
     * @return true 已成功启用或已形成稳定失败结论
     */
    bool voice_service_startup_complete(void);

    /**
     * @brief 查询离线语音模型与 BOOT0 输入是否均可用
     * @return true 可以接受新的按键语音会话
     */
    bool voice_service_is_ready(void);

    /**
     * @brief 在统一服务框架拥有的 voice task 中运行
     * @return ESP_OK 正常停止；模型失败会降级禁用但不终止既有 UI/BLE 服务
     */
    esp_err_t voice_service_run(void);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_VOICE_SERVICE_H */
