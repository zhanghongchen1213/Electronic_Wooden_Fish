/**
 * @file     ble_status_frame.h
 * @brief    67 字节 BLE 状态帧解码契约
 * @details  定义协议原始镜像，并提供无任务、无队列和无全局所有者的纯解析与新鲜度判断接口。
 * @author   ZHC
 * @date     2026-08-17
 */

#ifndef LEGBOT_BLE_STATUS_FRAME_H
#define LEGBOT_BLE_STATUS_FRAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** BLE 状态帧固定字节数。 */
#define BLE_STATUS_FRAME_LENGTH 67U
/** BLE 状态帧首个包头字节。 */
#define BLE_STATUS_HEADER_FIRST 0x55U
/** BLE 状态帧第二个包头字节。 */
#define BLE_STATUS_HEADER_SECOND 0xAAU
/** 版本协议区固定字节数。 */
#define BLE_STATUS_VERSION_FIELD_LENGTH 16U
/** 版本目标缓冲容量，包含结尾 NUL。 */
#define BLE_STATUS_VERSION_CAPACITY 17U
/** 状态包保留数据区固定字节数。 */
#define BLE_STATUS_RESERVED_DATA_LENGTH 9U
/** 包内时间戳原始区固定字节数。 */
#define BLE_STATUS_TIMESTAMP_LENGTH 4U
/** 状态帧所属规范 MAC 文本容量。 */
#define BLE_STATUS_MAC_CAPACITY 18U

typedef enum
{
    BLE_STATUS_DECODE_OK = 0,                    /**< 状态帧完整有效。 */
    BLE_STATUS_DECODE_HEADER_INVALID,            /**< 包头不是固定的 55 AA。 */
    BLE_STATUS_DECODE_CURRENT_MODE_NONFINITE,    /**< 当前运动模式不是有限浮点数。 */
    BLE_STATUS_DECODE_STEPS_NONFINITE,           /**< 步数不是有限浮点数。 */
    BLE_STATUS_DECODE_STEP_FREQUENCY_NONFINITE,  /**< 步频不是有限浮点数。 */
    BLE_STATUS_DECODE_LEFT_GEAR_NONFINITE,       /**< 左档位不是有限浮点数。 */
    BLE_STATUS_DECODE_RIGHT_GEAR_NONFINITE,      /**< 右档位不是有限浮点数。 */
    BLE_STATUS_DECODE_BALANCE_FACTOR_NONFINITE,  /**< 平衡因子不是有限浮点数。 */
    BLE_STATUS_DECODE_STEPS_OUT_OF_RANGE,        /**< 步数小于零。 */
    BLE_STATUS_DECODE_VERSION_UNSUPPORTED,       /**< 版本损坏、冲突或不属于 v1 至 v9。 */
    BLE_STATUS_DECODE_LEFT_GEAR_OUT_OF_RANGE,    /**< 左档位不在型号允许的 1 至 10/15。 */
    BLE_STATUS_DECODE_RIGHT_GEAR_OUT_OF_RANGE,   /**< 右档位不在型号允许的 1 至 10/15。 */
    BLE_STATUS_DECODE_BALANCE_FACTOR_OUT_OF_RANGE, /**< 平衡因子不在 -1 至 1。 */
    BLE_STATUS_DECODE_MOTOR_ENABLE_OUT_OF_RANGE, /**< 助力开关不是 0 或 1。 */
    BLE_STATUS_DECODE_BATTERY_OUT_OF_RANGE,      /**< 电池电量大于 100。 */
    BLE_STATUS_DECODE_SCENE_MODE_OUT_OF_RANGE,   /**< 场景模式不在 1 至 4。 */
    BLE_STATUS_DECODE_SCENE_MODE_MODEL_INCOMPATIBLE, /**< 场景模式不属于当前型号线协议。 */
    BLE_STATUS_DECODE_SCENE_CONFIG_INVALID,         /**< 场景能力配置不是 1 至 5。 */
} ble_status_decode_error_t;

typedef struct
{
    uint8_t reserved_header[2];                           /**< 包头后的保留字节原始镜像。 */
    float current_mode;                                   /**< 当前运动模式原始 float32 值。 */
    float steps;                                          /**< 步数原始 float32 值。 */
    float step_frequency;                                 /**< 步频原始 float32 值。 */
    float left_gear;                                      /**< 左档位；成对未初始化值按解码契约回退。 */
    float right_gear;                                     /**< 右档位；成对未初始化值按解码契约回退。 */
    float balance_factor;                                 /**< 平衡因子原始 float32 值。 */
    uint8_t motor_enable;                                 /**< 助力开关，协议有效值为 0 或 1。 */
    uint8_t battery_level;                                /**< 外骨骼电量百分比。 */
    uint8_t scene_mode;                                   /**< 场景模式，协议有效值由 scene_config 决定。 */
    uint8_t scene_config;                                  /**< 场景能力配置，位于原始帧第 38 字节，取值 1 至 5。 */
    uint8_t firmware_update_flag;                         /**< 固件升级标志原始值。 */
    uint8_t reserved_data[BLE_STATUS_RESERVED_DATA_LENGTH]; /**< 保留数据区原始镜像。 */
    int8_t left_motor_position;                            /**< 左电机位置有符号角度。 */
    int8_t right_motor_position;                           /**< 右电机位置有符号角度。 */
    uint8_t standby_time;                                 /**< 待机时间原始值。 */
    uint8_t temp_alarm;                                   /**< 温控报警原始值。 */
    uint8_t padding[2];                                   /**< 版本字段前的填充原始镜像。 */
    char version[BLE_STATUS_VERSION_CAPACITY];            /**< 安全终止或被隔离为空的版本字符串。 */
    uint8_t timestamp[BLE_STATUS_TIMESTAMP_LENGTH];       /**< 不解释字节序的包内时间戳。 */
} ble_status_frame_t;

typedef struct
{
    uint8_t bytes[BLE_STATUS_FRAME_LENGTH]; /**< 当前同步候选的定长字节。 */
    size_t length;                          /**< 当前已保存的候选字节数。 */
} ble_status_sync_t;

typedef struct
{
    uint16_t conn_handle;              /**< 原始字节所属连接 handle。 */
    uint32_t link_generation;          /**< 原始字节所属不可变物理链路代次。 */
    char mac[BLE_STATUS_MAC_CAPACITY]; /**< 原始字节所属规范完整 MAC。 */
} ble_status_owner_token_t;

typedef struct
{
    uint32_t valid_frame_count;       /**< 完整有效帧饱和计数。 */
    uint32_t invalid_frame_count;     /**< 完整无效帧饱和计数。 */
    uint32_t discarded_byte_count;    /**< 包头前丢弃字节饱和计数。 */
    uint32_t version_sanitized_count; /**< 版本字符串隔离饱和计数。 */
} ble_status_diagnostics_t;

typedef struct
{
    ble_status_sync_t sync;                  /**< 当前链路的定长字节同步状态。 */
    ble_status_owner_token_t partial_token;  /**< 半包所属不可分割链路 token。 */
    bool partial_token_valid;                /**< 半包 token 是否已建立。 */
    ble_status_frame_t mirror;               /**< 最近一帧完整有效状态镜像，档位可能采用安全回退。 */
    ble_status_owner_token_t mirror_token;   /**< 最近状态镜像所属链路 token。 */
    uint64_t last_status_rx_ms;              /**< 最近有效镜像本地单调接收毫秒。 */
    bool mirror_valid;                       /**< 是否存在完整有效原始镜像。 */
    ble_status_owner_token_t evidence_token; /**< 已发布首帧证据所属 token。 */
    bool evidence_published;                 /**< 当前链路是否已发布首帧证据。 */
    ble_status_diagnostics_t diagnostics;    /**< 状态帧私有饱和诊断。 */
} ble_status_owner_state_t;

typedef struct
{
    bool frame_completed;      /**< 本字节是否完成一个 67 字节候选。 */
    bool frame_valid;          /**< 完成的候选是否通过关键字段校验。 */
    bool first_valid_evidence; /**< 是否应为当前 token 发布一次有效首帧证据。 */
    bool version_sanitized;    /**< 完成候选的版本字段是否被隔离。 */
    ble_status_decode_error_t decode_error; /**< 完整候选的精确解码结果。 */
} ble_status_owner_outcome_t;

typedef struct
{
    bool data_ready;         /**< 当前物理链路是否已收到有效帧。 */
    bool status_fresh;       /**< 最近有效帧是否仍在新鲜窗口。 */
    bool link_control_ready; /**< transport 与 freshness 是否同时满足链路准入。 */
} ble_status_freshness_state_t;

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief 解码并校验一帧固定 67 字节 BLE 状态
     * @details 先在临时候选中解码全部字段；成对 -1/-1 或 0/0 档位在无历史时回退为 5/5，关键字段全部通过后才覆盖输出。
     * @param frame 固定 67 字节协议帧
     * @param output 成功时接收完整原始镜像
     * @param version_sanitized 返回版本区是否因非法字符被置空，可为 NULL
     * @return true 表示关键字段有效且已写入 output，false 表示整帧拒绝
     */
    bool ble_status_frame_decode(const uint8_t frame[BLE_STATUS_FRAME_LENGTH],
                                 ble_status_frame_t *output,
                                 bool *version_sanitized);

    /**
     * @brief 解码状态帧并返回精确拒绝原因
     * @details 与 ble_status_frame_decode 保持相同的档位回退和原子写入语义，失败时不覆盖 output。
     * @param frame 固定 67 字节协议帧
     * @param output 成功时接收完整原始镜像
     * @param version_sanitized 返回版本区是否因非法字符被置空，可为 NULL
     * @param decode_error 返回精确解码结果，可为 NULL
     * @return true 表示关键字段有效且已写入 output，false 表示整帧拒绝
     */
    bool ble_status_frame_decode_detailed(
        const uint8_t frame[BLE_STATUS_FRAME_LENGTH],
        ble_status_frame_t *output,
        bool *version_sanitized,
        ble_status_decode_error_t *decode_error);

    /**
     * @brief 获取状态帧解码结果的稳定诊断名称
     * @param decode_error 解码结果
     * @return 永久有效的 ASCII 诊断名称
     */
    const char *ble_status_decode_error_name(
        ble_status_decode_error_t decode_error);

    /**
     * @brief 清空定长状态帧同步器
     * @param sync 待清空的同步器
     */
    void ble_status_sync_reset(ble_status_sync_t *sync);

    /**
     * @brief 向定长同步器追加一个字节
     * @details 只在累积到完整 67 字节候选时返回 true。
     * @param sync 同步器状态
     * @param value 新到达的字节
     * @param discarded_increment 本次应累加的包头前丢弃字节数，可为 NULL
     * @return true 表示 bytes 中已有完整候选，false 表示仍在等待
     */
    bool ble_status_sync_push(ble_status_sync_t *sync,
                              uint8_t value,
                              uint8_t *discarded_increment);

    /**
     * @brief 完成当前 67 字节候选并准备继续收帧
     * @details 有效或非法完整候选都按整帧消费；后续字节重新执行包头同步。
     * @param sync 已累积完整候选的同步器
     * @param frame_valid 当前候选是否通过完整字段校验
     */
    void ble_status_sync_finish(ble_status_sync_t *sync, bool frame_valid);

    /**
     * @brief 清空 owner 的半包与当前链路首帧证据
     * @details 保留最近有效镜像和累计诊断，供普通断链后只读投影。
     * @param state BLE owner 状态
     */
    void ble_status_owner_reset_link(ble_status_owner_state_t *state);

    /**
     * @brief 清空 owner 保留的最近有效原始镜像
     * @param state BLE owner 状态
     */
    void ble_status_owner_clear_mirror(ble_status_owner_state_t *state);

    /**
     * @brief 清空 owner 全部状态与诊断
     * @param state BLE owner 状态
     */
    void ble_status_owner_reset_all(ble_status_owner_state_t *state);

    /**
     * @brief 在单 owner 内消费一个带链路 token 的 Notify 字节
     * @details 成对 -1/-1 或 0/0 档位仅在当前链路已有有效首帧时沿用其镜像，否则回退为 5/5；完整候选校验后才原子替换镜像，无效帧不刷新接收时间或证据。
     * @param state BLE owner 状态
     * @param value 新到达的字节
     * @param link_token 字节所属不可分割链路 token
     * @param now_ms 当前本地单调接收毫秒
     * @param outcome 返回本字节产生的帧和首帧证据结果
     * @return true 表示参数有效且已消费，false 表示参数无效
     */
    bool ble_status_owner_consume_byte(ble_status_owner_state_t *state,
                                       uint8_t value,
                                       const ble_status_owner_token_t *link_token,
                                       uint64_t now_ms,
                                       ble_status_owner_outcome_t *outcome);

    /**
     * @brief 判断最近有效状态是否仍处于本地单调新鲜窗口
     * @param current_link_valid 当前物理链路 token 是否仍有效
     * @param device_state_available 是否保留最近有效产品状态
     * @param data_ready 当前物理链路是否已收到有效帧
     * @param now_ms 当前本地单调毫秒
     * @param last_status_rx_ms 最近有效帧本地单调接收毫秒
     * @param stale_ms 新鲜窗口毫秒数，边界值本身视为过期
     * @return true 表示状态新鲜，false 表示缺证据、过期或时基倒退
     */
    bool ble_status_is_fresh(bool current_link_valid,
                             bool device_state_available,
                             bool data_ready,
                             uint64_t now_ms,
                             uint64_t last_status_rx_ms,
                             uint64_t stale_ms);

    /**
     * @brief 对当前链路一帧完整有效状态执行 freshness 转移
     * @param current_link_valid 帧 token 是否仍属于当前物理链路
     * @param device_state_available owner 是否已保存有效镜像
     * @param transport_ready GATT、MTU 与 Notify transport 是否全部准入
     * @param received_at_ms 该帧最后字节在 callback 中的本地接收毫秒
     * @param now_ms 当前处理该帧的本地单调毫秒
     * @param stale_ms 新鲜窗口毫秒数
     * @param state 成功时接收新的链路新鲜度状态
     * @return true 表示帧证据可用且已应用，false 表示当前链路或时间证据无效
     */
    bool ble_status_freshness_accept_frame(
        bool current_link_valid,
        bool device_state_available,
        bool transport_ready,
        uint64_t received_at_ms,
        uint64_t now_ms,
        uint64_t stale_ms,
        ble_status_freshness_state_t *state);

    /**
     * @brief 计算 fresh 到 stale 的单次边沿转移
     * @param current_link_valid 最近镜像 token 是否仍属于当前链路
     * @param device_state_available 是否保留最近有效产品状态
     * @param now_ms 当前本地单调毫秒
     * @param last_status_rx_ms 最近有效帧 callback 接收毫秒
     * @param stale_ms 新鲜窗口毫秒数
     * @param state 当前链路新鲜度状态
     * @return true 仅表示本次产生 fresh 到 stale 边沿，false 表示不需要发布
     */
    bool ble_status_freshness_expire(
        bool current_link_valid,
        bool device_state_available,
        uint64_t now_ms,
        uint64_t last_status_rx_ms,
        uint64_t stale_ms,
        ble_status_freshness_state_t *state);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_BLE_STATUS_FRAME_H */
