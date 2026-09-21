/**
 * @file     voice_control_policy.h
 * @brief    离线语音控制纯门禁与命令映射接口
 * @details  复用控制页按值模型，把 MultiNet 命令 ID 转换为既有 typed 控制动作，不接触 GPIO、模型或 BLE owner。
 * @author   ZHC
 * @date     2026-08-05
 */

#ifndef LEGBOT_VOICE_CONTROL_POLICY_H
#define LEGBOT_VOICE_CONTROL_POLICY_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "control_ui_binding.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** BOOT0 进入语音会话的固定长按阈值。 */
#define VOICE_CONTROL_BOOT_LONG_PRESS_MS 500U
/** ADC/RX 启动后的固定语音采集窗口。 */
#define VOICE_CONTROL_CAPTURE_WINDOW_MS 3000U

    typedef enum
    {
        VOICE_COMMAND_AI_MODE = 1,       /**< AI、智能或标准模式。 */
        VOICE_COMMAND_EXTREME_MODE = 2,  /**< 极限模式。 */
        VOICE_COMMAND_FITNESS_MODE = 3,  /**< 健身模式。 */
        VOICE_COMMAND_DOWNHILL_MODE = 4, /**< 下山模式。 */
        VOICE_COMMAND_POWEROFF = 5,      /**< 外骨骼关机。 */
        VOICE_COMMAND_ASSIST_ON = 6,     /**< 开启助力。 */
        VOICE_COMMAND_ASSIST_OFF = 7,    /**< 关闭助力。 */
        VOICE_COMMAND_GEAR_UP = 8,       /**< 统一助力档位加一。 */
        VOICE_COMMAND_GEAR_DOWN = 9,     /**< 统一助力档位减一。 */
        VOICE_COMMAND_GEAR_1 = 10,       /**< 统一助力档位设为一档。 */
        VOICE_COMMAND_GEAR_2 = 11,       /**< 统一助力档位设为二档。 */
        VOICE_COMMAND_GEAR_3 = 12,       /**< 统一助力档位设为三档。 */
        VOICE_COMMAND_GEAR_4 = 13,       /**< 统一助力档位设为四档。 */
        VOICE_COMMAND_GEAR_5 = 14,       /**< 统一助力档位设为五档。 */
        VOICE_COMMAND_GEAR_6 = 15,       /**< 统一助力档位设为六档。 */
        VOICE_COMMAND_GEAR_7 = 16,       /**< 统一助力档位设为七档。 */
        VOICE_COMMAND_GEAR_8 = 17,       /**< 统一助力档位设为八档。 */
        VOICE_COMMAND_GEAR_9 = 18,       /**< 统一助力档位设为九档。 */
        VOICE_COMMAND_GEAR_10 = 19,      /**< 统一助力档位设为十档。 */
    } voice_command_id_t;

    /**
     * @brief 判定 BOOT0 长按是否具备请求亮屏并进入语音的业务条件
     * @details 接受亮屏或熄屏快照，但仍要求控制页完整业务门禁、自检与严重低电门禁通过。
     * @param snapshot 最新不可变产品快照
     * @param snapshot_valid 快照读取是否成功
     * @param now_ms 当前单调毫秒
     * @return true 可刷新本地活动并等待屏幕点亮，false 必须保持静默且不唤醒
     */
    bool voice_control_entry_gate_allows(
        const watch_state_snapshot_t *snapshot,
        bool snapshot_valid,
        uint64_t now_ms);

    /**
     * @brief 按当前完整产品快照判定是否允许开始或提交语音控制
     * @param snapshot 最新不可变产品快照
     * @param snapshot_valid 快照读取是否成功
     * @param now_ms 当前单调毫秒
     * @return true 屏幕点亮、控制页门禁开放、自检空闲且非严重低电
     */
    bool voice_control_gate_allows(
        const watch_state_snapshot_t *snapshot,
        bool snapshot_valid,
        uint64_t now_ms);

    /**
     * @brief 按统一概率阈值判定最高候选
     * @param command_id MultiNet 最高候选命令 ID
     * @param top_probability 最高候选概率
     * @param second_probability 第二候选概率；没有第二候选时传 0
     * @return true 候选合法且最高候选概率严格大于 0.40
     */
    bool voice_control_candidate_accepts(
        int command_id,
        float top_probability,
        float second_probability);

    /**
     * @brief 判定一次 BOOT0 按住是否刚好达到长按触发条件
     * @param pressed 当前去抖后的按键是否仍处于按下状态
     * @param already_evaluated 本次按下周期是否已经判定过长按
     * @param elapsed_ms 本次稳定按下已经持续的毫秒数
     * @return true 首次达到 500 ms，可执行一次门禁检查
     */
    bool voice_button_long_press_due(
        bool pressed,
        bool already_evaluated,
        uint64_t elapsed_ms);

    /**
     * @brief 判定已触发会话是否达到固定采集窗口终点
     * @param capture_started ADC/RX 是否已经发布采集开始事件
     * @param already_requested 本代次是否已经请求过自动停止
     * @param elapsed_ms 从 ADC/RX 采集开始起经过的毫秒数
     * @return true 首次达到 3000 ms，应请求一次正常停止
     */
    bool voice_capture_auto_stop_due(
        bool capture_started,
        bool already_requested,
        uint64_t elapsed_ms);

    /**
     * @brief 把语音命令映射为与 UI 相同的 typed 控制动作和目标
     * @details 开关已处于目标、档位位于边界时返回 ESP_OK 且 should_submit=false。
     * @param command_id 已通过阈值的语音命令 ID
     * @param snapshot 三秒采集结束后读取的最新产品快照
     * @param model 同一快照生成的控制页按值模型
     * @param action 成功时输出既有控制页动作
     * @param target 成功时输出既有控制页目标值
     * @param should_submit 成功时输出是否需要进入 BLE owner
     * @return ESP_OK 映射成功或幂等零写入
     *         ESP_ERR_NOT_SUPPORTED 当前型号不支持请求的场景模式
     *         ESP_ERR_INVALID_ARG 命令或输出参数非法
     *         ESP_ERR_INVALID_STATE 最新设备实际状态不可用
     */
    esp_err_t voice_control_resolve_command(
        int command_id,
        const watch_state_snapshot_t *snapshot,
        const control_ui_model_t *model,
        control_ui_action_t *action,
        uint8_t *target,
        bool *should_submit);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_VOICE_CONTROL_POLICY_H */
