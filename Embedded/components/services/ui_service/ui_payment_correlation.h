/**
 * @file     ui_payment_correlation.h
 * @brief    定义支付验证结果的本地关联与页面投影策略
 * @details  隔离迟到终态，并把支付会话中间态规范投影到既有 04-03/04-04 页面模型。
 * @author   ZHC
 * @date     2026-08-03
 */

#ifndef UI_PAYMENT_CORRELATION_H
#define UI_PAYMENT_CORRELATION_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"
#include "ui_authorization_display.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        ui_authorization_display_t display; /**< 当前互斥的授权显示状态。 */
        bool authorization_pending; /**< 是否正在联网并检查授权。 */
        bool payment_required;      /**< 是否由既有支付页面接管主壳。 */
        bool payment_attempted;     /**< 是否选择验证失败页面变体。 */
        bool payment_pending;       /**< 手动加速或业务 owner 是否仍在处理。 */
        bool payment_failed;        /**< 是否投影既有 04-04 页面。 */
        bool payment_action_enabled; /**< 既有验证按钮是否允许提交。 */
    } ui_payment_projection_t;

    /**
     * @brief 把当前租赁阶段投影为既有支付页面字段
     * @param authorization_needed 当前绑定会话是否仍缺少 BLE 解锁证明
     * @param current_generation 控制事务是否属于当前 BLE 链路代次
     * @param authorization_transport_ready 当前链路是否允许提交立即检查
     * @param manual_attempted 用户是否已点击过既有验证按钮
     * @param manual_pending 该按钮提交是否仍等待当前会话终态
     * @param phase 当前租赁阶段
     * @param last_result 最近租赁结果
     * @param projection 规范化页面字段输出
     */
    void ui_payment_projection_build(
        bool authorization_needed,
        bool current_generation,
        bool authorization_transport_ready,
        bool manual_attempted,
        bool manual_pending,
        watch_rental_phase_t phase,
        watch_rental_result_t last_result,
        ui_payment_projection_t *projection);

    /**
     * @brief 判断 BLE 解锁终态是否属于当前手动支付验证
     * @param pending 当前是否存在等待终态的手动验证
     * @param pending_intent_sequence 当前手动验证的非零 intent 序号
     * @param pending_link_generation 当前手动验证所属非零链路代次
     * @param result_intent_sequence 解锁终态携带的 intent 序号
     * @param result_link_generation 解锁终态携带的链路代次
     * @return true 表示终态可收口当前手动验证，false 表示必须忽略
     */
    bool ui_payment_result_matches(bool pending,
                                   uint32_t pending_intent_sequence,
                                   uint32_t pending_link_generation,
                                   uint32_t result_intent_sequence,
                                   uint32_t result_link_generation);

#ifdef __cplusplus
}
#endif

#endif
