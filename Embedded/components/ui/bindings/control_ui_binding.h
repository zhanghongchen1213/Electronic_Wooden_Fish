/**
 * @file     control_ui_binding.h
 * @brief    控制页纯模型与 typed intent 接口
 * @details  把不可变产品快照投影为按值模型，并把页面动作转换为与 LVGL、SquareLine 和 BLE 实现解耦的 typed intent。
 * @author   ZHC
 * @date     2026-08-17
 */

#ifndef LEGBOT_CONTROL_UI_BINDING_H
#define LEGBOT_CONTROL_UI_BINDING_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 控制零提交的内部原因
     * @details 仅供门禁和路由判断，不是腕上可见状态枚举；技术原因不得直接映射为页面、组件或文案。
     */
    typedef enum
    {
        CONTROL_UI_BLOCK_NONE = 0,          /**< 当前允许提交控制。 */
        CONTROL_UI_BLOCK_SNAPSHOT_FAILED,   /**< 无法读取产品状态快照。 */
        CONTROL_UI_BLOCK_UNBOUND,           /**< 尚未绑定外骨骼。 */
        CONTROL_UI_BLOCK_BLE_DISCONNECTED,  /**< BLE 尚未连接。 */
        CONTROL_UI_BLOCK_GATT_NOT_READY,     /**< GATT 尚未就绪。 */
        CONTROL_UI_BLOCK_NOTIFY_NOT_READY,   /**< Notify 尚未就绪。 */
        CONTROL_UI_BLOCK_DATA_NOT_READY,     /**< 当前链路没有有效状态数据。 */
        CONTROL_UI_BLOCK_STATUS_STALE,       /**< 设备状态已过期。 */
        CONTROL_UI_BLOCK_LINK_NOT_READY,     /**< 链路控制准入尚未就绪。 */
        CONTROL_UI_BLOCK_PENDING,            /**< 授权、解锁或普通控制正在处理。 */
        CONTROL_UI_BLOCK_RETRY_EXHAUSTED,    /**< 普通控制三次失败，等待真实 BLE 重连。 */
        CONTROL_UI_BLOCK_AUTH_REQUIRED,      /**< 当前会话尚未解锁。 */
        CONTROL_UI_BLOCK_RENTAL_UNPAID,      /**< 当前租赁尚未支付。 */
        CONTROL_UI_BLOCK_DEVICE_DISABLED,    /**< 当前设备已被禁用。 */
        CONTROL_UI_BLOCK_UNKNOWN_DEVICE,     /**< 当前设备未登记。 */
        CONTROL_UI_BLOCK_CLOUD_UNAVAILABLE,  /**< 授权服务暂不可用。 */
    } control_ui_block_reason_t;

    typedef enum
    {
        CONTROL_UI_ACTION_MOTOR = 0,   /**< 提交助力目标。 */
        CONTROL_UI_ACTION_GEAR,        /**< 提交统一档位目标。 */
        CONTROL_UI_ACTION_SCENE_MODE,  /**< 提交场景模式目标。 */
        CONTROL_UI_ACTION_POWEROFF,    /**< 直接提交外骨骼关机。 */
    } control_ui_action_t;

    typedef struct
    {
        watch_control_kind_t kind; /**< typed 控制类型。 */
        uint8_t target;            /**< 未经 clamp 的 typed 目标。 */
    } control_ui_intent_t;

    typedef struct
    {
        bool actual_available;                  /**< 设备实际状态是否可展示。 */
        bool actual_current;                    /**< 设备实际状态是否属于当前连接且仍新鲜。 */
        bool poweroff_committed;                /**< 关机写已被设备接受或已形成产品终态。 */
        bool controls_enabled;                  /**< 普通控制是否允许交互。 */
        uint8_t actual_motor_enable;            /**< 快照中的实际助力状态。 */
        uint8_t actual_gear;                    /**< 按左腿协议字段显示的实际档位。 */
        uint8_t actual_scene_mode;              /**< 快照中的实际场景模式。 */
        control_ui_block_reason_t block_reason; /**< fail-closed 主阻塞原因。 */
    } control_ui_model_t;

    /**
     * @brief 从 ui_task 单次读取的不可变 watch_state 快照构建控制页模型
     * @param snapshot 产品状态快照；读取失败时可以为 NULL
     * @param snapshot_valid 本次快照读取是否成功
     * @param now_ms ui_task 当前单调毫秒
     * @param model 完整覆盖写入的按值页面模型
     * @return ESP_OK 已生成模型，ESP_ERR_INVALID_ARG 表示输出为空
     */
    esp_err_t control_ui_model_from_snapshot(const watch_state_snapshot_t *snapshot,
                                             bool snapshot_valid,
                                             uint64_t now_ms,
                                             control_ui_model_t *model);

    /**
     * @brief 在直接提交关机前用最新单份快照重新判断控制准入
     * @param snapshot 提交瞬间读取的产品状态快照
     * @param snapshot_valid 本次快照读取是否成功
     * @param now_ms 提交瞬间单调毫秒
     * @return true 当前仍允许发送一次关机 intent，false 必须零提交
     */
    bool control_ui_poweroff_submit_allowed(
        const watch_state_snapshot_t *snapshot,
        bool snapshot_valid,
        uint64_t now_ms);

    /**
     * @brief 把页面动作与原始目标转换为 typed 控制 intent
     * @param action 页面动作类型
     * @param target 页面产生的原始目标值，函数不会 clamp
     * @param intent 完整覆盖写入的 typed intent
     * @return ESP_OK 转换成功，ESP_ERR_INVALID_ARG 表示动作、目标或输出无效
     */
    esp_err_t control_ui_intent_from_action(control_ui_action_t action,
                                            uint8_t target,
                                            control_ui_intent_t *intent);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_CONTROL_UI_BINDING_H */
