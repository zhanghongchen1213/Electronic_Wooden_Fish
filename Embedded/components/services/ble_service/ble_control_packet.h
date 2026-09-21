/**
 * @file     ble_control_packet.h
 * @brief    外骨骼 32 字节完整控制包 builder 接口
 * @details  集中定义唯一二进制布局、安全默认值和 typed override，统一档位会原子覆盖左右两个协议字段。
 * @author   ZHC
 * @date     2026-08-17
 */

#ifndef LEGBOT_BLE_CONTROL_PACKET_H
#define LEGBOT_BLE_CONTROL_PACKET_H

#include <stdbool.h>
#include <stdint.h>

#include "ble_status_frame.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** APP 到外骨骼控制包固定字节数。 */
#define BLE_CONTROL_PACKET_LENGTH 32U
/** 云授权校验字段固定偏移。 */
#define BLE_CONTROL_DEVICE_CHECK_OFFSET 19U
/** 一次性关机字段固定偏移。 */
#define BLE_CONTROL_SHUTDOWN_OFFSET 20U
/** 控制包保留数据区固定字节数。 */
#define BLE_CONTROL_RESERVED_DATA_LENGTH 9U

    typedef enum
    {
        BLE_CONTROL_OVERRIDE_NONE = 0,     /**< 不覆盖开放状态字段。 */
        BLE_CONTROL_OVERRIDE_MOTOR_ENABLE, /**< 覆盖助力开关。 */
        BLE_CONTROL_OVERRIDE_GEAR,         /**< 同时覆盖左右档位为同一目标。 */
        BLE_CONTROL_OVERRIDE_SCENE_MODE,   /**< 覆盖场景模式。 */
        BLE_CONTROL_OVERRIDE_POWEROFF,     /**< 仅本包置位一次性关机命令。 */
        BLE_CONTROL_OVERRIDE_GEAR_SCENE_BOUNDARY, /**< 同时覆盖 10/11 档和标准/极限模式。 */
    } ble_control_override_kind_t;

    typedef struct
    {
        ble_control_override_kind_t kind; /**< 本包唯一 typed override 类型。 */
        uint8_t target;                   /**< 设备线协议目标；档位为 1 至 15，跨界为 10/11，场景值须先按型号转换。 */
    } ble_control_packet_override_t;

    /**
     * @brief 从最新完整有效 mirror 与 typed override 生成授权控制包
     * @details 调用者必须已通过云授权 gate；成功包始终携带 device_check=1。
     * @param mirror BLE owner 当前完整协议镜像
     * @param override 已由 gate 校验并由 owner 复核的覆盖；统一档位同时写两侧
     * @param packet 固定 32 字节输出；失败时保持调用前内容
     * @return true 已生成，false 表示 mirror 或 override 非有限、越界或非法
     */
    bool ble_control_packet_build(
        const ble_status_frame_t *mirror,
        const ble_control_packet_override_t *override,
        uint8_t packet[BLE_CONTROL_PACKET_LENGTH]);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BLE_CONTROL_PACKET_H */
