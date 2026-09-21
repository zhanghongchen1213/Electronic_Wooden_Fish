/**
 * @file     ui_manifest_projection.h
 * @brief    SquareLine manifest 静态状态投影接口
 * @details  由工具机械生成对象基线与状态覆盖；不包含业务事实读取或页面路由。
 * @author   ZHC
 * @date     2026-07-20
 */

#ifndef LEGBOT_UI_MANIFEST_PROJECTION_H
#define LEGBOT_UI_MANIFEST_PROJECTION_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** manifest 中 23 个 canonical 与 7 个整页变体。 */
    typedef enum
    {
        UI_MANIFEST_STATE_HOME_NORMAL, /**< home_normal 静态投影。 */
        UI_MANIFEST_STATE_HOME_UNBOUND, /**< home_unbound 静态投影。 */
        UI_MANIFEST_STATE_HOME_DISCONNECTED, /**< home_disconnected 静态投影。 */
        UI_MANIFEST_STATE_CONTROL_CONNECTED, /**< control_connected 静态投影。 */
        UI_MANIFEST_STATE_CONTROL_DISCONNECTED, /**< control_disconnected 静态投影。 */
        UI_MANIFEST_STATE_MODE_DEFAULT, /**< mode_default 静态投影。 */
        UI_MANIFEST_STATE_MODE_DISCONNECTED, /**< mode_disconnected 静态投影。 */
        UI_MANIFEST_STATE_ALERTS_EMPTY, /**< alerts_empty 静态投影。 */
        UI_MANIFEST_STATE_ALERTS_READONLY, /**< alerts_readonly 静态投影。 */
        UI_MANIFEST_STATE_PAYMENT_REQUIRED, /**< payment_required 静态投影。 */
        UI_MANIFEST_STATE_PAYMENT_VERIFY_FAILED, /**< payment_verify_failed 静态投影。 */
        UI_MANIFEST_STATE_SETTINGS, /**< settings 静态投影。 */
        UI_MANIFEST_STATE_DEVICE_INFO, /**< device_info 静态投影。 */
        UI_MANIFEST_STATE_MAINTENANCE, /**< maintenance 静态投影。 */
        UI_MANIFEST_STATE_BLE_EMPTY, /**< ble_empty 静态投影。 */
        UI_MANIFEST_STATE_BLE_CANDIDATES, /**< ble_candidates 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_IDLE, /**< selftest_idle 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_PROGRESS, /**< selftest_progress 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_MANUAL, /**< selftest_manual 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_RESULT, /**< selftest_result 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_GPS_CONTEXT, /**< selftest_gps_context 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_RETRY_DETAIL, /**< selftest_retry_detail 静态投影。 */
        UI_MANIFEST_STATE_CLEAR_BINDING_CONFIRM, /**< clear_binding_confirm 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_MANUAL_TOUCH, /**< selftest_manual_touch 静态投影。 */
        UI_MANIFEST_STATE_BLE_SCANNING, /**< ble_scanning 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO, /**< selftest_retry_auto 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_MANUAL_VIBRATION, /**< selftest_manual_vibration 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_RESULT_ALL_PASSED, /**< selftest_result_all_passed 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_LOADING, /**< selftest_gps_submit_loading 静态投影。 */
        UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_FAILED, /**< selftest_gps_submit_failed 静态投影。 */
        UI_MANIFEST_STATE_COUNT /**< manifest 静态投影数量。 */
    } ui_manifest_state_t;

    /**
     * @brief 按 baseline_reset、canonical、variant 顺序应用一个 manifest 静态状态
     * @param state 静态状态 ID
     * @return true 已应用，false 表示状态或对象生命周期无效
     */
    bool ui_manifest_projection_apply(ui_manifest_state_t state);

    /**
     * @brief 返回静态状态对应的 manifest state_key
     * @param state 静态状态 ID
     * @return 稳定 state_key；参数无效时返回 invalid
     */
    const char *ui_manifest_projection_key(ui_manifest_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_MANIFEST_PROJECTION_H */
