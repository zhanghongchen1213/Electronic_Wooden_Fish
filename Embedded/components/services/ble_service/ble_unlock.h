/**
 * @file     ble_unlock.h
 * @brief    BLE 一次性解锁兼容入口与状态证据接口
 * @details  委托唯一完整包 builder，并提供无 NimBLE handle 泄露的 callback/下一帧顺序校验。
 * @author   ZHC
 * @date     2026-07-13
 */

#ifndef LEGBOT_BLE_UNLOCK_H
#define LEGBOT_BLE_UNLOCK_H

#include <stdbool.h>
#include <stdint.h>

#include "ble_control_packet.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 解锁写 callback 成功后的状态确认窗口。 */
#define BLE_UNLOCK_CONFIRM_TIMEOUT_MS 5000U
/** 解锁写入发起后等待 GATT callback 的最长时间。 */
#define BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT_MS 5000U
/** BLE 解锁事务 MAC 容量。 */
#define BLE_UNLOCK_MAC_CAPACITY BLE_STATUS_MAC_CAPACITY

    typedef struct
    {
        uint32_t intent_sequence;                      /**< 调用方 intent 关联序号；自动入口为 0。 */
        uint32_t request_id;                           /**< control_gate 生成的非零请求 ID。 */
        char exoskeleton_mac[BLE_UNLOCK_MAC_CAPACITY]; /**< 事务绑定规范 MAC。 */
        uint32_t link_generation;                      /**< 事务绑定物理链路代次。 */
    } ble_unlock_request_t;

    typedef struct
    {
        ble_unlock_request_t request;         /**< 当前唯一一次性写事务。 */
        uint32_t status_sequence_at_callback; /**< callback 成功时有效状态序号。 */
        uint64_t write_started_ms;            /**< GATT 写入发起前的本地单调毫秒。 */
        uint64_t callback_success_ms;         /**< callback 成功的本地单调毫秒。 */
        bool active;                          /**< 是否仍等待 callback 或状态证据。 */
        bool callback_succeeded;              /**< 有响应写 callback 是否已成功一次。 */
    } ble_unlock_context_t;

    /**
     * @brief 从最新有效 lastDeviceState mirror 生成云授权控制包
     * @param mirror BLE owner 最近完整有效协议镜像
     * @param packet 固定 32 字节输出
     * @return true 已生成，false 表示 mirror 字段不满足协议范围
     */
    bool ble_unlock_build_packet(const ble_status_frame_t *mirror,
                                 uint8_t packet[BLE_CONTROL_PACKET_LENGTH]);

    /**
     * @brief 开始唯一一次解锁写事务
     * @param context BLE owner 私有事务
     * @param request 请求 ID、MAC 与链路代次
     * @param current_status_sequence 写入前有效状态序号
     * @param now_ms 写入发起单调毫秒
     * @return true 已开始，false 表示参数非法或已有活动事务
     */
    bool ble_unlock_begin(ble_unlock_context_t *context,
                          const ble_unlock_request_t *request,
                          uint32_t current_status_sequence,
                          uint64_t now_ms);

    /**
     * @brief 应用有响应写 callback evidence
     * @param context BLE owner 私有事务
     * @param request callback 按值绑定 token
     * @param success callback status 是否为 0
     * @param current_status_sequence callback 时有效状态序号
     * @param now_ms callback 单调毫秒
     * @return true 首次匹配 callback 已应用，false 表示迟到、重复或不匹配
     */
    bool ble_unlock_accept_write_callback(
        ble_unlock_context_t *context,
        const ble_unlock_request_t *request,
        bool success,
        uint32_t current_status_sequence,
        uint64_t now_ms);

    /**
     * @brief 判断 callback 后下一帧完整有效状态是否完成确认
     * @param context BLE owner 私有事务
     * @param request 当前状态帧绑定 token
     * @param status_sequence 当前完整有效状态序号
     * @param now_ms 当前单调毫秒
     * @return true 唯一成功闭环，false 表示顺序、身份、代次或时间不匹配
     */
    bool ble_unlock_accept_status(ble_unlock_context_t *context,
                                  const ble_unlock_request_t *request,
                                  uint32_t status_sequence,
                                  uint64_t now_ms);

    /**
     * @brief 判断当前解锁确认是否已超过 5 秒截止
     * @param context BLE owner 私有事务
     * @param now_ms 当前单调毫秒
     * @return true 已超时并清除活动事务，false 尚未超时或未等待确认
     */
    bool ble_unlock_expire(ble_unlock_context_t *context, uint64_t now_ms);

    /**
     * @brief 判断等待 GATT 写 callback 是否已超过有界截止
     * @param context BLE owner 私有事务
     * @param now_ms 当前单调毫秒
     * @return true 写 callback 已超时并清除活动事务，false 尚未超时或已进入确认阶段
     */
    bool ble_unlock_expire_write_callback(ble_unlock_context_t *context,
                                          uint64_t now_ms);

    /**
     * @brief 断连、身份变化或停止时清除易失解锁事务
     * @param context BLE owner 私有事务
     */
    void ble_unlock_cancel(ble_unlock_context_t *context);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BLE_UNLOCK_H */
