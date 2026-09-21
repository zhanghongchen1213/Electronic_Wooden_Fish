/**
 * @file     ble_service.c
 * @brief    NimBLE Central 服务生命周期与 typed 事件循环
 * @details  由固定 ble_task 串行拥有应用层 BLE 状态、普通控制写事务与结果队列，host callback 仅按值投递小事件。
 * @author   ZHC
 * @date     2026-08-17
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ble_service.h"
#include "ble_unlock.h"
#include "host/ble_att.h"
#include "host/ble_gatt.h"
#include "host/ble_hs.h"
#include "host/ble_hs_mbuf.h"

/** BLE 一次性临时升功率策略的 RAM 状态。 */
typedef struct
{
    bool boost_next_connection;             /**< 下一次物理连接是否临时使用 +9 dBm。 */
    bool has_recent_abnormal_disconnect;    /**< 是否已记录窗口内第一次异常链路丢失。 */
    uint64_t recent_abnormal_disconnect_ms; /**< 第一次异常链路丢失的单调毫秒。 */
} ble_tx_power_policy_t;

static bool control_callback_watchdog_must_survive_abort(
    bool callback_pending);
static bool unlock_callback_watchdog_must_survive_abort(
    const ble_unlock_context_t *context);
static bool disconnect_matches_pending_connect(
    bool connecting,
    bool link_connected,
    uint32_t current_link_generation,
    uint32_t event_link_generation);
static void ble_tx_power_policy_reset(ble_tx_power_policy_t *policy);
static void ble_tx_power_policy_request_boost(ble_tx_power_policy_t *policy);
static void ble_tx_power_policy_note_scan_rssi(ble_tx_power_policy_t *policy,
                                               int8_t rssi);
static bool ble_tx_power_disconnect_reason_is_abnormal(int32_t reason);
static void ble_tx_power_policy_note_disconnect(
    ble_tx_power_policy_t *policy,
    int32_t reason,
    uint64_t now_ms);
static void ble_tx_power_policy_note_control_attempt(
    ble_tx_power_policy_t *policy,
    uint8_t attempt);
static bool ble_tx_power_policy_take_boost(ble_tx_power_policy_t *policy);
static int start_gatt_control_write(
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    const uint8_t packet[BLE_CONTROL_PACKET_LENGTH],
    ble_gatt_attr_fn *callback,
    void *callback_argument);

#ifdef BLE_SERVICE_UNLOCK_OWNER_TEST

#if defined(_WIN32)
#define BLE_OWNER_TEST_EXPORT __declspec(dllexport)
#else
#define BLE_OWNER_TEST_EXPORT __attribute__((visibility("default")))
#endif

/** host harness 使用的真实 owner 写事务上下文。 */
static ble_unlock_context_t s_owner_test_unlock_context;
/** host harness 使用的 BLE 临时升功率策略。 */
static ble_tx_power_policy_t s_owner_test_tx_power_policy;

static int start_unlock_write_boundary(
    ble_unlock_context_t *unlock_context,
    const ble_unlock_request_t *request,
    const ble_status_frame_t *mirror,
    uint32_t status_sequence,
    uint64_t now_ms,
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    ble_gatt_attr_fn *callback,
    void *callback_argument);
static int start_control_write_boundary(
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override,
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    ble_gatt_attr_fn *callback,
    void *callback_argument);
static int owner_test_write_callback(uint16_t conn_handle,
                                     const struct ble_gatt_error *error,
                                     struct ble_gatt_attr *attr,
                                     void *arg);

/**
 * @brief 在 host harness 中执行 ble_service 的一次性 NimBLE 写入边界
 * @param current_link_ready 调用瞬间完整链路与状态新鲜度是否仍成立
 * @param request 按值绑定的解锁事务身份
 * @param mirror 最新完整有效状态镜像
 * @param status_sequence 写入前有效状态序号
 * @param now_ms 写入发起单调毫秒
 * @return NimBLE 初始返回值；前置条件失败固定返回 -1
 */
BLE_OWNER_TEST_EXPORT int ble_service_test_start_unlock_write(
    bool current_link_ready,
    const ble_unlock_request_t *request,
    const ble_status_frame_t *mirror,
    uint32_t status_sequence,
    uint64_t now_ms)
{
    if (!current_link_ready)
    {
        return -1;
    }
    return start_unlock_write_boundary(&s_owner_test_unlock_context,
                                       request,
                                       mirror,
                                       status_sequence,
                                       now_ms,
                                       1U,
                                       2U,
                                       owner_test_write_callback,
                                       NULL);
}

/**
 * @brief 查询 host harness 的 owner 写事务是否仍在等待 callback
 * @return true 已成功发起且等待 callback，false 未发起或初始调用失败
 */
BLE_OWNER_TEST_EXPORT bool ble_service_test_unlock_active(void)
{
    return s_owner_test_unlock_context.active;
}

/**
 * @brief 查询 host harness 当前 owner 写事务绑定的 intent 序号
 * @return 当前按值保存的 intent 序号
 */
BLE_OWNER_TEST_EXPORT uint32_t ble_service_test_unlock_intent_sequence(void)
{
    return s_owner_test_unlock_context.request.intent_sequence;
}

/**
 * @brief 清理 host harness 的 owner 写事务
 */
BLE_OWNER_TEST_EXPORT void ble_service_test_reset_unlock(void)
{
    ble_unlock_cancel(&s_owner_test_unlock_context);
}

/**
 * @brief 在 host harness 中执行普通控制完整包有响应写入边界
 * @param current_link_ready 写入瞬间所有 owner 前置事实是否仍成立
 * @param mirror 当前完整有效私有 mirror
 * @param override gate 锁定且 owner 复核的单字段覆盖
 * @return NimBLE 初始返回值；前置条件失败固定返回 -1
 */
BLE_OWNER_TEST_EXPORT int ble_service_test_start_control_write(
    bool current_link_ready,
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override)
{
    if (!current_link_ready)
    {
        return -1;
    }
    return start_control_write_boundary(mirror,
                                        override,
                                        1U,
                                        2U,
                                        owner_test_write_callback,
                                        NULL);
}

/**
 * @brief 在 host harness 中执行中止后的 callback watchdog 保留策略
 * @return 0 表示普通控制与首次解锁均只在 callback 未决时保留 watchdog
 */
BLE_OWNER_TEST_EXPORT int ble_service_test_abort_watchdog_policy(void)
{
    ble_unlock_context_t unlock = {.active = true};
    if (!control_callback_watchdog_must_survive_abort(true) ||
        control_callback_watchdog_must_survive_abort(false) ||
        !unlock_callback_watchdog_must_survive_abort(&unlock))
    {
        return 1;
    }
    unlock.callback_succeeded = true;
    return unlock_callback_watchdog_must_survive_abort(&unlock) ? 2 : 0;
}

/**
 * @brief 判断断链事件是否属于尚未发布成功结果的当前连接尝试
 * @param connecting 当前是否仍在等待连接结果
 * @param link_connected 当前是否已发布物理连接成功
 * @param current_link_generation 当前连接代次
 * @param event_link_generation 断链事件携带的连接代次
 * @return true 应按当前连接失败收口，false 不属于待完成连接
 */
BLE_OWNER_TEST_EXPORT bool
ble_service_test_disconnect_matches_pending_connect(
    bool connecting,
    bool link_connected,
    uint32_t current_link_generation,
    uint32_t event_link_generation)
{
    return disconnect_matches_pending_connect(connecting,
                                              link_connected,
                                              current_link_generation,
                                              event_link_generation);
}

/**
 * @brief 重置 host harness 的 BLE 临时升功率策略
 */
BLE_OWNER_TEST_EXPORT void ble_service_test_tx_power_policy_reset(void)
{
    ble_tx_power_policy_reset(&s_owner_test_tx_power_policy);
}

/**
 * @brief 模拟增强功率 API 或同步建链失败后保留一次性升功率意图
 */
BLE_OWNER_TEST_EXPORT void ble_service_test_tx_power_request_boost(void)
{
    ble_tx_power_policy_request_boost(&s_owner_test_tx_power_policy);
}

/**
 * @brief 向 host harness 注入最近扫描报告 RSSI
 * @param rssi 最近候选广播 RSSI
 */
BLE_OWNER_TEST_EXPORT void ble_service_test_tx_power_note_scan_rssi(int8_t rssi)
{
    ble_tx_power_policy_note_scan_rssi(&s_owner_test_tx_power_policy, rssi);
}

/**
 * @brief 向 host harness 注入一次断链原因
 * @param reason NimBLE GAP 断链原因
 * @param now_ms 当前单调毫秒
 */
BLE_OWNER_TEST_EXPORT void ble_service_test_tx_power_note_disconnect(
    int32_t reason,
    uint64_t now_ms)
{
    ble_tx_power_policy_note_disconnect(&s_owner_test_tx_power_policy,
                                        reason,
                                        now_ms);
}

/**
 * @brief 向 host harness 注入普通控制 attempt
 * @param attempt 同一普通控制请求当前 attempt
 */
BLE_OWNER_TEST_EXPORT void ble_service_test_tx_power_note_control_attempt(
    uint8_t attempt)
{
    ble_tx_power_policy_note_control_attempt(&s_owner_test_tx_power_policy,
                                             attempt);
}

/**
 * @brief 消费 host harness 的下一次物理连接临时升功率标志
 * @return true 下一次连接使用 +9 dBm，false 使用默认 0 dBm
 */
BLE_OWNER_TEST_EXPORT bool ble_service_test_tx_power_take_boost(void)
{
    return ble_tx_power_policy_take_boost(&s_owner_test_tx_power_policy);
}

static int owner_test_write_callback(uint16_t conn_handle,
                                     const struct ble_gatt_error *error,
                                     struct ble_gatt_attr *attr,
                                     void *arg)
{
    (void)conn_handle;
    (void)error;
    (void)attr;
    (void)arg;
    return 0;
}

#else

#include "ble_recovery_policy.h"
#include "ble_status_frame.h"
#include <ctype.h>
#include <stdatomic.h>
#include <stdio.h>

#include "config_service.h"
#include "cloud_service.h"
#include "control_gate.h"
#include "esp_bt.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_hs_adv.h"
#include "host/ble_hs_id.h"
#include "host/ble_l2cap.h"
#include "host/ble_uuid.h"
#include "maintenance_service.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "os/os_mbuf.h"
#include "state_service.h"
#include "ui_service.h"

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif

static const char *TAG = "SVC_BLE";

/** 活动 GATT、准入和控制事务检查 host 输入的短间隔。 */
#define BLE_EVENT_POLL_MS 20U
/** 完整服务启停边界预留给固定 v5.5.4 NimBLE controller 的连续 retention 块。 */
#define BLE_RADIO_RETENTION_RESERVE_BYTES 1100U
/** 等待新一代 NimBLE host 首次同步的最长时间。 */
#define BLE_HOST_SYNC_TIMEOUT_MS 5000U
/** 当前没有候选页 latest 请求。 */
#define BLE_BINDING_SCAN_REQUEST_NONE 0
/** 候选页 latest 请求为结束扫描会话。 */
#define BLE_BINDING_SCAN_REQUEST_END 1
/** 候选页 latest 请求为开始扫描会话。 */
#define BLE_BINDING_SCAN_REQUEST_BEGIN 2
/** 候选连接建立超时。 */
#define BLE_CONNECT_TIMEOUT_MS 30000
/** 等待 NimBLE host task 删除完成的有界时间。 */
#define BLE_HOST_STOP_WAIT_MS 1000U
/** 主动扫描期间关联可连接 ADV 与 Scan Response 的地址缓存上限。 */
#define BLE_CONNECTABLE_CACHE_CAPACITY 32U
/** 同时保留的不可变 GATT callback 上下文数量。 */
#define BLE_GATT_CALLBACK_CONTEXT_COUNT 16U
/** 停止前最终 fail-closed 状态的有界投递次数。 */
#define BLE_FINAL_STATE_PUBLISH_ATTEMPTS 5U
/** 纯噪声诊断合并发布的最小间隔。 */
#define BLE_DIAGNOSTIC_PUBLISH_INTERVAL_MS 200U
/** 非法完整状态帧日志和状态投影的最小间隔。 */
#define BLE_INVALID_FRAME_DIAGNOSTIC_INTERVAL_MS 1000U
/** 结果队列阻塞后可覆盖全部已接收 intent 的有界 FIFO 深度。 */
#define BLE_UNLOCK_PENDING_RESULT_DEPTH \
    (BLE_EVENT_QUEUE_DEPTH + CLOUD_RENTAL_REQUEST_QUEUE_DEPTH + 2U)
/** 单条物理链路自动授权入口最多执行次数。 */
#define BLE_AUTOMATIC_AUTHORIZATION_MAX_ATTEMPTS 3U
/** 自动授权入口暂态失败后的重试间隔。 */
#define BLE_AUTOMATIC_AUTHORIZATION_RETRY_DELAY_MS 1000U
/** 云租赁请求本地入队失败后的重试间隔。 */
#define BLE_CLOUD_RENTAL_SUBMISSION_RETRY_DELAY_MS 1000U
/** 云租赁请求本地入队故障退避上限。 */
#define BLE_CLOUD_RENTAL_SUBMISSION_RETRY_MAX_DELAY_MS 30000U
/** 连接参数更新含首次提交在内的最大尝试次数。 */
#define BLE_CONNECTION_PROFILE_MAX_ATTEMPTS 3U
/** 连接参数更新重试的基础退避。 */
#define BLE_CONNECTION_PROFILE_RETRY_BASE_MS 1000U
/** 自动授权入口重试耗尽稳定码。 */
#define BLE_AUTO_AUTH_RETRY_EXHAUSTED "BLE_AUTO_AUTH_RETRY_EXHAUSTED"
/** 人工立即检查已被当前支付会话同步接受。 */
#define BLE_PAYMENT_CHECK_ACCEPTED "CLOUD_ACCELERATE_ACCEPTED"

_Static_assert(BLE_BINDING_SCAN_WINDOW_MS == BLE_SCAN_WINDOW_DURATION_MS,
               "BLE 公共扫描窗口与低功耗策略必须一致");

typedef enum
{
    BLE_PRIVATE_EVENT_STOP = 0,             /**< 停止 ble_task 与 NimBLE host。 */
    BLE_PRIVATE_EVENT_WAKE,                 /**< 唤醒 owner 处理原子邮箱或策略事实。 */
    BLE_PRIVATE_EVENT_HOST_SYNC,            /**< NimBLE host 已同步，允许启动扫描。 */
    BLE_PRIVATE_EVENT_RECONNECT_NOW,        /**< 立即执行已绑定设备重连。 */
    BLE_PRIVATE_EVENT_RESCAN,               /**< 立即重新启动未绑定候选扫描。 */
    BLE_PRIVATE_EVENT_BEGIN_BINDING_SCAN,   /**< 候选页扫描会话开始。 */
    BLE_PRIVATE_EVENT_END_BINDING_SCAN,     /**< 候选页扫描会话结束。 */
    BLE_PRIVATE_EVENT_HOST_RESET,           /**< NimBLE host 发生 reset。 */
    BLE_PRIVATE_EVENT_DISC_REPORT,          /**< 符合目标 Service 的可连接广播。 */
    BLE_PRIVATE_EVENT_DISC_COMPLETE,        /**< 有界扫描窗口已结束。 */
    BLE_PRIVATE_EVENT_CONNECT_RESULT,       /**< 候选连接结果。 */
    BLE_PRIVATE_EVENT_DISCONNECT,           /**< 已连接 peer 断开。 */
    BLE_PRIVATE_EVENT_CONN_UPDATE,          /**< 连接参数更新终态。 */
    BLE_PRIVATE_EVENT_GATT_SERVICE,         /**< Service discovery 单项或终态。 */
    BLE_PRIVATE_EVENT_GATT_CHARACTERISTIC,  /**< Characteristic discovery 单项或终态。 */
    BLE_PRIVATE_EVENT_GATT_DESCRIPTOR,      /**< Descriptor discovery 单项或终态。 */
    BLE_PRIVATE_EVENT_MTU_RESULT,           /**< ATT MTU exchange completion 或 GAP MTU。 */
    BLE_PRIVATE_EVENT_NOTIFY_WRITE_RESULT,  /**< CCCD write completion。 */
    BLE_PRIVATE_EVENT_NOTIFY_RX,            /**< 已按值复制的原始 Notify bytes。 */
    BLE_PRIVATE_EVENT_MTU_EVIDENCE,         /**< 当前连接实际 ATT MTU 诊断证据。 */
    BLE_PRIVATE_EVENT_FRAME_EVIDENCE,       /**< owner 内部完整有效首帧证据。 */
    BLE_PRIVATE_EVENT_FIRST_UNLOCK_INTENT,  /**< 首次需要控制的 typed intent。 */
    BLE_PRIVATE_EVENT_UNLOCK_WRITE_RESULT,  /**< 一次性解锁写 callback evidence。 */
    BLE_PRIVATE_EVENT_CONTROL_INTENT,       /**< 已解锁会话普通控制 typed intent。 */
    BLE_PRIVATE_EVENT_CONTROL_WRITE_RESULT, /**< 普通控制写 callback evidence。 */
    BLE_PRIVATE_EVENT_CLOUD_RESULT_READY,   /**< cloud 租赁结果队列已有终态。 */
    BLE_PRIVATE_EVENT_SCREEN_STATE,         /**< power owner 已锁存最新物理屏幕事实。 */
} ble_private_event_type_t;

typedef enum
{
    BLE_RADIO_LIFECYCLE_DORMANT = 0, /**< 无线栈未初始化，完整启停所需 retention 块已保留。 */
    BLE_RADIO_LIFECYCLE_STARTING,    /**< retention 块已释放，正在等待 host sync。 */
    BLE_RADIO_LIFECYCLE_READY,       /**< host 已同步，无线 API 可安全使用。 */
    BLE_RADIO_LIFECYCLE_STOPPING,    /**< 正在反初始化或无法证明安全 DORMANT。 */
} ble_radio_lifecycle_t;

typedef struct
{
    ble_private_event_type_t type;                /**< typed 事件类型。 */
    int32_t reason;                               /**< host reset 或 GAP 结果，其他事件为 0。 */
    ble_addr_t address;                           /**< 广播报告按值复制的完整 peer address。 */
    int8_t rssi;                                  /**< 广播报告 RSSI。 */
    uint16_t conn_handle;                         /**< 连接成功时的 handle。 */
    uint16_t mtu;                                 /**< 实际协商 MTU。 */
    uint16_t start_handle;                        /**< Service 或 descriptor 起始 handle。 */
    uint16_t end_handle;                          /**< Service 或 descriptor 结束 handle。 */
    uint16_t def_handle;                          /**< Characteristic definition handle。 */
    uint16_t val_handle;                          /**< Characteristic value handle。 */
    uint8_t properties;                           /**< Characteristic properties。 */
    uint16_t payload_length;                      /**< 原始 Notify 按值字节数。 */
    uint8_t payload[BLE_NOTIFY_PAYLOAD_CAPACITY]; /**< 当前 callback 的有界字节副本。 */
    uint64_t received_at_ms;                      /**< Notify callback 收到字节时的本地单调毫秒。 */
    bool fully_valid;                             /**< 状态首帧是否已通过完整校验。 */
    char mac[BLE_MAC_CAPACITY];                   /**< 证据所属完整规范 MAC。 */
    uint32_t scan_generation;                     /**< 扫描报告/结束事件所属的启动代次。 */
    uint32_t link_generation;                     /**< GATT/MTU/Notify 结果所属物理连接代次。 */
    uint32_t request_id;                          /**< 解锁或普通控制 callback 的请求 ID。 */
    uint32_t unlock_intent_sequence;              /**< 首次授权 intent 的调用方关联序号。 */
    watch_control_kind_t control_kind;            /**< 普通控制 intent/callback 类型。 */
    uint8_t control_target;                       /**< 普通控制 intent/callback 目标。 */
    uint8_t control_attempt;                      /**< 普通控制 callback 绑定的不可变 attempt。 */
    uint32_t control_status_sequence_at_write;    /**< 普通控制写入发起前的有效状态序号。 */
    uint32_t control_intent_sequence;             /**< 公共控制 intent 的入口顺序。 */
    uint32_t radio_generation;                    /**< host 事件所属无线栈启停代次。 */
    bool poweroff_preempted_ordinary;             /**< 关机已在 owner 内取消旧普通控制状态。 */
    bool target_uuid;                             /**< discovery 单项是否为显式匹配的目标 UUID。 */
    bool target_service;                          /**< 当前报告字段是否包含目标 Service UUID。 */
    bool connectable_report;                      /**< 当前报告是否明确来自可连接广播。 */
    bool scan_response;                           /**< 当前报告是否为主动扫描响应。 */
    char advertised_name[MAINTENANCE_BLE_NAME_CAPACITY]; /**< 广播名称的小型按值副本。 */
} ble_private_event_t;

typedef enum
{
    BLE_ADMISSION_IDLE = 0,                   /**< 尚无活动物理连接准入。 */
    BLE_ADMISSION_GATT_DISCOVERING,           /**< 正在串行发现目标 Service。 */
    BLE_ADMISSION_CHARACTERISTIC_DISCOVERING, /**< 正在串行发现目标 Characteristic。 */
    BLE_ADMISSION_CCCD_DISCOVERING,           /**< 正在目标特征真实边界内发现 CCCD。 */
    BLE_ADMISSION_MTU_NEGOTIATING,            /**< 正在取得当前连接的实际 ATT MTU。 */
    BLE_ADMISSION_NOTIFY_ENABLING,            /**< 正在串行写入 CCCD，最多三次尝试。 */
    BLE_ADMISSION_NOTIFY_READY,               /**< GATT、MTU 与 Notify transport 已准入。 */
} ble_admission_state_t;

typedef struct
{
    atomic_bool in_use;       /**< 不可变上下文是否仍由 NimBLE procedure 持有。 */
    uint16_t conn_handle;     /**< procedure 启动时的连接 handle。 */
    uint32_t link_generation; /**< procedure 启动时的物理连接代次。 */
} ble_gatt_callback_context_t;

typedef struct
{
    atomic_bool in_use;           /**< callback 尚未归还该不可变值槽。 */
    ble_unlock_request_t request; /**< 写入发起时绑定的 intent、请求 ID、MAC 与代次。 */
} ble_unlock_callback_context_t;

typedef struct
{
    atomic_bool in_use;                   /**< callback 尚未归还该不可变值槽。 */
    control_gate_control_action_t action; /**< 写入前按值锁定的完整普通控制身份。 */
    uint32_t status_sequence_at_write;    /**< 写入发起前已观察到的有效状态序号。 */
} ble_control_callback_context_t;

typedef struct
{
    ble_addr_t address;                     /**< 连接使用的 NimBLE 地址真值。 */
    char mac[MAINTENANCE_BLE_MAC_CAPACITY]; /**< UI 与持久化使用的规范 MAC。 */
    char advertised_name[MAINTENANCE_BLE_NAME_CAPACITY]; /**< 最近广播名称。 */
    int8_t rssi;                                         /**< 最近一次候选广播 RSSI。 */
} ble_candidate_t;

typedef struct
{
    ble_addr_t address;                                  /**< 可连接广播的完整 peer address。 */
    char advertised_name[MAINTENANCE_BLE_NAME_CAPACITY]; /**< 广播或扫描响应中的名称。 */
    int8_t rssi;                                         /**< 最近一次可连接广播 RSSI。 */
} ble_connectable_observation_t;

typedef enum
{
    BLE_SELECTION_RESULT_OK = 0, /**< 已使用内部候选地址发起连接。 */
    BLE_SELECTION_RESULT_BUSY,   /**< 已在连接或连接已存在。 */
    BLE_SELECTION_RESULT_STALE,  /**< generation 过期、MAC 不在列表或与绑定不符。 */
    BLE_SELECTION_RESULT_ERROR,  /**< NimBLE 拒绝发起连接。 */
} ble_selection_result_t;

typedef struct
{
    bool active;               /**< 是否存在 RAM-only 待绑定事务。 */
    bool connected;            /**< 候选连接是否已成功。 */
    bool first_valid_frame;    /**< 同连接首帧状态包是否已完整校验。 */
    uint16_t conn_handle;      /**< 连接与首帧证据必须匹配的 handle。 */
    ble_candidate_t candidate; /**< 连接与首帧证据必须匹配的待绑定候选。 */
} ble_pending_binding_t;

/** 目标 16-bit Service UUID 0x00FF。 */
static const ble_uuid16_t s_target_uuid16 = BLE_UUID16_INIT(BLE_TARGET_SERVICE_UUID16);
/** 目标 Bluetooth Base 128-bit Service UUID 的 NimBLE 小端表达。 */
static const ble_uuid128_t s_target_uuid128 = BLE_UUID128_INIT(0xFB, 0x34, 0x9B, 0x5F,
                                                               0x80, 0x00, 0x00, 0x80,
                                                               0x00, 0x10, 0x00, 0x00,
                                                               0xFF, 0x00, 0x00, 0x00);
/** 目标 16-bit Characteristic UUID 0xFF01。 */
static const ble_uuid16_t s_target_characteristic_uuid16 =
    BLE_UUID16_INIT(BLE_TARGET_CHARACTERISTIC_UUID16);
/** 目标 Bluetooth Base 128-bit Characteristic UUID 的 NimBLE 小端表达。 */
static const ble_uuid128_t s_target_characteristic_uuid128 = BLE_UUID128_INIT(0xFB, 0x34, 0x9B, 0x5F,
                                                                              0x80, 0x00, 0x00, 0x80,
                                                                              0x00, 0x10, 0x00, 0x00,
                                                                              0x01, 0xFF, 0x00, 0x00);
/** CCCD 16-bit UUID 0x2902。 */
static const ble_uuid16_t s_target_cccd_uuid16 = BLE_UUID16_INIT(BLE_TARGET_CCCD_UUID16);

/** host callback 与 ble_task 之间的私有有界 typed 队列。 */
static QueueHandle_t s_event_queue;
/** ble_task 向调用方发布首次解锁业务终态的有界队列。 */
static QueueHandle_t s_unlock_result_queue;
/** ble_task 向调用方发布普通控制本地终态的有界队列。 */
static QueueHandle_t s_control_result_queue;
/** 是否接受 host callback 的新事件。 */
static atomic_bool s_accept_host_events;
/** callback 队列溢出时由 ble_task 统一消费的原子证据。 */
static atomic_bool s_host_queue_overflow;
/** NimBLE host task 删除回调已执行的原子证明。 */
static atomic_bool s_host_task_deleted;
/** 即使私有队列已满也不会丢失的全局停止意图。 */
static atomic_bool s_stop_requested;
/** ble_task 是否正在拥有 BLE 运行时。 */
static atomic_bool s_owner_running;
/** BLE 无线栈四态生命周期；正常运行保持 READY。 */
static atomic_int s_radio_lifecycle =
    ATOMIC_VAR_INIT(BLE_RADIO_LIFECYCLE_STOPPING);
/** 完整服务启停边界固定占用的连续 retention 保留块。 */
static void *s_radio_retention_reserve;
/** host callback 当前可见的无线栈代次。 */
static atomic_uint_fast32_t s_callback_radio_generation;
/** BLE owner 当前无线栈代次。 */
static uint32_t s_radio_generation;
/** STARTING 状态等待 host sync 的单调截止毫秒。 */
static uint64_t s_radio_start_deadline_ms;
/** power owner 最新物理屏幕事实序号 mailbox。 */
static atomic_uint_fast32_t s_pending_screen_sequence;
/** power owner 最新物理屏幕事实值 mailbox。 */
static atomic_bool s_pending_screen_on;
/** BLE owner 已采用的物理屏幕事实序号。 */
static uint32_t s_applied_screen_sequence;
/** 候选页开始/结束的 latest 请求，队列只负责唤醒。 */
static atomic_int s_binding_scan_request;
/** Notify mbuf 复制或 host ingress 超界的 fail-closed 证据。 */
static atomic_bool s_notify_transport_failure;
/** BLE 启动准备阶段的产品状态工作区，避免完整快照占用 main 任务栈。 */
static watch_state_snapshot_t s_prepare_product_state;
/** BLE owner 控制路径的产品状态工作区，避免完整快照占用 ble_task 栈。 */
static watch_state_snapshot_t s_runtime_product_state;
/** BLE 状态应用确认的独立快照工作区，允许控制路径嵌套确认。 */
static watch_state_snapshot_t s_verification_product_state;
/** GAP callback 可读取的当前连接代次。 */
static atomic_uint_fast32_t s_callback_link_generation;
/** GAP callback 可读取的当前连接 handle。 */
static atomic_uint_fast16_t s_callback_conn_handle;
/** GAP callback 可读取的目标 Characteristic value handle。 */
static atomic_uint_fast16_t s_callback_value_handle;
/** GAP callback 可读取的 Notify readiness。 */
static atomic_bool s_callback_notify_ready;
/** NimBLE port 是否已启动。 */
static bool s_nimble_started;
/** 禁止重复 stop/deinit 并锁存首个终端错误的纯策略守卫。 */
static ble_nimble_cleanup_guard_t s_nimble_cleanup;
/** 当前 own address type，仅在 host sync 后取得。 */
static uint8_t s_own_addr_type;
/** 当前候选扫描代次，启动周期内非零单调。 */
static uint32_t s_scan_generation;
/** 当前扫描窗口是否活动。 */
static bool s_scanning;
/** 未绑定候选页是否持有用户可见的扫描会话。 */
static bool s_binding_scan_session_active;
/** 候选页离开时扫描取消异常，等待原扫描窗口终态收口。 */
static bool s_binding_scan_cancel_pending;
/** 熄屏取消断连扫描异常时等待原扫描窗口终态收口。 */
static bool s_screen_scan_cancel_pending;
/** 自检暂停时扫描取消异常，等待原扫描窗口终态收口。 */
static bool s_selftest_scan_cancel_pending;
/** 当前是否已发起连接，防止重复进入。 */
static bool s_connecting;
/** 自检暂停时已请求取消的连接，等待 GAP 终态完成所有权交接。 */
static bool s_selftest_connection_cancel_pending;
/** 当前代次保留的有界内部候选。 */
static ble_candidate_t s_candidates[MAINTENANCE_BLE_CANDIDATE_MAX_COUNT];
/** 当前内部候选数量。 */
static size_t s_candidate_count;
/** BLE owner 发布候选快照的静态工作区，避免占用 3 KiB ble_task 栈。 */
static maintenance_ble_candidate_t s_candidate_snapshot_workspace[MAINTENANCE_BLE_CANDIDATE_MAX_COUNT];
/** 当前扫描中因地址类型冲突而拒绝的 MAC 字节值。 */
static uint8_t s_ambiguous_macs[MAINTENANCE_BLE_CANDIDATE_MAX_COUNT][6];
/** 当前扫描中歧义 MAC 的数量。 */
static size_t s_ambiguous_mac_count;
/** 歧义拒绝表耗尽后整轮扫描是否进入 fail-closed。 */
static bool s_identity_conflict_fail_closed;
/** 当前扫描已确认可连接的 advertiser 观测缓存。 */
static ble_connectable_observation_t s_connectable_observations[BLE_CONNECTABLE_CACHE_CAPACITY];
/** 当前可连接 advertiser 地址缓存数量。 */
static size_t s_connectable_address_count;
/** 地址缓存满后循环覆盖最旧项的位置。 */
static size_t s_connectable_address_next;
/** maintenance 候选快照是否等待重试发布。 */
static bool s_candidate_snapshot_dirty;
/** 候选会话清理是否等待 maintenance mutex 释放后重试。 */
static bool s_candidate_session_clear_dirty;
/** 待清理候选会话的新代次。 */
static uint32_t s_candidate_session_clear_generation;
/** 当前扫描轮次应保留到 UI 的稳定诊断。 */
static const char *s_scan_diagnostic = BLE_OK;
/** 最近一次非 BLE_OK 稳定诊断，正常状态迁移不会立即覆盖。 */
static const char *s_last_error_code = BLE_OK;
/** 主动终止链路后需跨 GAP disconnect 保留的准入失败诊断。 */
static const char *s_disconnect_diagnostic = BLE_OK;
/** 是否存在已提交绑定，Task 5 从 config owner 激活。 */
static bool s_has_binding;
/** 已提交绑定的规范 MAC，不包含地址类型。 */
static char s_bound_mac[MAINTENANCE_BLE_MAC_CAPACITY];
/** 当前 RAM-only 严格绑定提交事务。 */
static ble_pending_binding_t s_pending_binding;
/** 当前 BLE 物理链路事实。 */
static bool s_link_connected;
/** 未来控制链路就绪证明，清绑时必须清空。 */
static bool s_link_control_ready;
/** 当前外骨骼上电会话解锁证明，BLE 断开不自动清空。 */
/** 当前已建立链路 handle，UINT16_MAX 表示无链路。 */
static uint16_t s_connection_handle = UINT16_MAX;
/** 最近扫描、异常断链与控制重试共同驱动的一次性升功率策略。 */
static ble_tx_power_policy_t s_tx_power_policy;
/** 当前正在建立的物理连接是否已使用 +9 dBm。 */
static bool s_connecting_tx_power_boost;
/** 当前已建立物理连接是否使用 +9 dBm。 */
static bool s_active_connection_tx_power_boost;
/** 当前物理连接非零单调代次。 */
static uint32_t s_link_generation;
/** 是否等待统一状态投影就绪后自动发起当前模式授权。 */
static bool s_automatic_authorization_pending;
/** 当前物理链路已执行自动授权入口的代次。 */
static uint32_t s_automatic_authorization_attempt_generation;
/** 当前物理链路已消耗的自动授权入口次数。 */
static uint8_t s_automatic_authorization_attempt_count;
/** 下一次允许执行自动授权入口的单调毫秒。 */
static uint64_t s_automatic_authorization_retry_at_ms;
#if SCENIC_AREA_MANAGEMENT_DEBUG
/** 当前是否保留一条尚未进入 cloud_task 队列的租赁请求。 */
static bool s_cloud_rental_submission_pending;
/** 因本地队列拥塞等待重试的完整租赁请求身份。 */
static cloud_rental_request_t s_cloud_rental_submission;
/** 当前租赁请求已执行的本地入队次数。 */
static uint8_t s_cloud_rental_submission_attempt_count;
/** 下一次允许重试租赁请求入队的单调毫秒。 */
static uint64_t s_cloud_rental_submission_retry_at_ms;
#endif
/** 当前串行 GATT 准入阶段。 */
static ble_admission_state_t s_admission_state = BLE_ADMISSION_IDLE;
/** 当前整条 link admission 的绝对 tick deadline。 */
static TickType_t s_admission_deadline;
/** 目标 Service 唯一匹配数量。 */
static uint8_t s_target_service_count;
/** 目标 Service handle 范围。 */
static uint16_t s_service_start_handle;
static uint16_t s_service_end_handle;
/** 目标 Characteristic 唯一匹配数量与句柄。 */
static uint8_t s_target_characteristic_count;
static uint16_t s_characteristic_def_handle;
static uint16_t s_characteristic_val_handle;
static uint16_t s_next_characteristic_def_handle;
static uint8_t s_characteristic_properties;
/** 目标 Characteristic descriptor 范围内的 CCCD 唯一匹配。 */
static uint8_t s_target_cccd_count;
static uint16_t s_cccd_handle;
/** 当前连接 CCCD write 已消耗的尝试数。 */
static uint8_t s_notify_attempt_count;
/** 当前连接向 watch_state 投影的 transport readiness。 */
static bool s_gatt_ready;
static bool s_mtu_ready;
static uint16_t s_negotiated_mtu;
/** discovery 期间由当前 GAP ATT MTU 事件提前观察到的实际值。 */
static uint16_t s_observed_att_mtu;
static bool s_notify_ready;
/** ble_task 串行拥有的组帧、镜像、时间、证据与诊断状态。 */
static ble_status_owner_state_t s_status_owner;
/** 当前物理链路是否已收到一帧完整有效状态。 */
static bool s_data_ready;
/** 最近有效状态是否仍处于本地单调新鲜窗口。 */
static bool s_status_fresh;
/** 每帧合法状态递增并允许自然回绕的 evidence 序号。 */
static uint32_t s_valid_status_sequence;
/** 跨 host callback 保留的不可变 GATT procedure 上下文池。 */
static ble_gatt_callback_context_t s_gatt_callback_contexts[BLE_GATT_CALLBACK_CONTEXT_COUNT];
/** 一次性解锁 callback 的不可变按值上下文。 */
static ble_unlock_callback_context_t
    s_unlock_callback_contexts[BLE_UNLOCK_REQUEST_QUEUE_DEPTH];
/** 普通控制 callback 的不可变按值上下文池。 */
static ble_control_callback_context_t
    s_control_callback_contexts[BLE_CONTROL_RESULT_QUEUE_DEPTH];
/** 首次授权 reducer，由 ble_task 单线程推进。 */
static control_gate_t s_control_gate;
/** 当前首次授权事务绑定的调用方 intent 关联序号。 */
static uint32_t s_authorization_intent_sequence;
/** 当前解锁会话是否已实际发起过一次性关机 procedure。 */
static bool s_poweroff_issued_in_session;
/** 一次性写 callback/下一帧顺序证据。 */
static ble_unlock_context_t s_unlock_context;
/** state_task 队列暂满时保留的最新 control 真值。 */
static watch_control_update_t s_pending_control_update;
/** 是否存在待重投的 control 真值。 */
static bool s_pending_control_update_dirty;
/** 调用方结果队列暂满时按顺序保留的解锁终态 FIFO。 */
static ble_unlock_result_t
    s_pending_unlock_results[BLE_UNLOCK_PENDING_RESULT_DEPTH];
/** 解锁终态 FIFO 当前头索引。 */
static size_t s_pending_unlock_result_head;
/** 解锁终态 FIFO 当前元素数量。 */
static size_t s_pending_unlock_result_count;
/** 终态积压期间拒绝继续接收新的首次解锁 intent。 */
static atomic_bool s_unlock_result_backpressured;
/** 普通控制 callback 是否仍在有界等待。 */
static bool s_control_write_callback_pending;
/** 普通控制 callback 等待起始单调毫秒。 */
static uint64_t s_control_write_started_ms;
/** 当前普通控制写入绑定的完整身份。 */
static control_gate_control_action_t s_control_write_action;
/** 调用方结果队列暂满时按顺序保留的普通控制终态 FIFO。 */
static ble_control_result_t
    s_pending_control_results[BLE_CONTROL_PENDING_RESULT_DEPTH];
/** 普通控制终态 FIFO 当前头索引。 */
static size_t s_pending_control_result_head;
/** 普通控制终态 FIFO 当前元素数量。 */
static size_t s_pending_control_result_count;
/** 终态积压期间拒绝继续接收普通控制 intent。 */
static atomic_bool s_control_result_backpressured;
/** 公共首次授权 intent 的无锁入口顺序生成器。 */
static atomic_uint_fast32_t s_unlock_intent_sequence;
/** 公共控制 intent 的无锁入口顺序生成器。 */
static atomic_uint_fast32_t s_control_intent_sequence;
/** 不依赖共享事件队列容量的单槽关机 intent 序号邮箱。 */
static atomic_uint_fast32_t s_poweroff_mailbox_sequence;
/** 最近关机 intent 取消普通控制的入口顺序截点。 */
static uint32_t s_poweroff_cancel_through_sequence;
/** 是否已建立有效的关机取消截点。 */
static bool s_poweroff_cancel_through_valid;
/** 等待旧普通 GATT callback 释放 procedure 的关机 intent。 */
static ble_private_event_t s_deferred_poweroff_intent;
/** 是否存在已由 owner 受理但尚未发起写入的关机 intent。 */
static bool s_deferred_poweroff_valid;
/** 清绑命令发生于连接中时，延迟到 GAP 结果后重启扫描。 */
static bool s_restart_scan_after_clear;
/** 最近发布或等待发布的 BLE 事务状态。 */
static watch_ble_transaction_t s_transaction = WATCH_BLE_TRANSACTION_UNBOUND;
/** state_service 队列满时合并保留的最新 BLE 真值。 */
static watch_ble_update_t s_pending_state_update;
/** 是否存在等待重新投递的最新 BLE 真值。 */
static bool s_pending_state_update_dirty;
/** 是否有待按时间窗合并发布的 BLE 诊断。 */
static bool s_diagnostic_publish_dirty;
/** 最近一次诊断快照发布的单调毫秒。 */
static uint32_t s_last_diagnostic_publish_ms;
/** 当前链路最近一次非法完整状态帧诊断的单调毫秒。 */
static uint32_t s_last_invalid_frame_diagnostic_ms;
/** 当前链路是否已经输出过非法完整状态帧诊断。 */
static bool s_invalid_frame_diagnostic_logged;
/** maintenance mutex 忙时保留的最新选择命令。 */
static maintenance_ble_selection_command_t s_pending_selection_command;
/** maintenance mutex 忙时保留的最新选择结果。 */
static ble_selection_result_t s_pending_selection_result;
/** 是否存在等待重新发布的选择结果。 */
static bool s_pending_selection_feedback_dirty;
/** 当前手动选择连接，用于异步结果回写候选页。 */
static maintenance_ble_selection_command_t s_active_selection_command;
/** 是否存在等待异步连接结果的手动选择。 */
static bool s_active_selection_pending;
/** 产品级自动恢复固定退避状态。 */
static ble_recovery_policy_t s_recovery_policy;
/** 当前自动恢复 deadline 的非零单调代次。 */
static uint32_t s_recovery_generation;
/** 产品自动重连 procedure 启动饱和计数。 */
static uint32_t s_reconnect_attempt_count;
/** 同身份端到端恢复成功饱和计数。 */
static uint32_t s_reconnect_success_count;
/** 当前扫描/连接/准入是否属于自动恢复尝试。 */
static bool s_recovery_in_progress;
/** 当前恢复是否观察到目标身份。 */
static bool s_recovery_target_seen;
/** 自检期间是否已暂停 discovery 与断线重连。 */
static bool s_discovery_suspended_for_selftest;
/** 自检快照锁暂忙时是否等待一次有界重试。 */
static bool s_selftest_snapshot_retry_pending;
/** 最近一次成功读取的屏幕事实是否有效。 */
static bool s_screen_state_known;
/** 最近一次成功读取的屏幕是否点亮。 */
static bool s_screen_on;
/** 最近一次请求的连接功耗档位，用于状态转换去重。 */
static ble_connection_power_profile_t s_connection_target_profile;
/** 当前正在等待 GAP 终态的连接功耗档位。 */
static ble_connection_power_profile_t s_connection_pending_profile;
/** 最近一次由 GAP 成功确认的连接功耗档位。 */
static ble_connection_power_profile_t s_connection_applied_profile;
/** 连接参数更新下一次重试的绝对单调毫秒。 */
static uint64_t s_connection_profile_retry_at_ms;
/** 当前目标连接参数已经执行的失败次数。 */
static uint8_t s_connection_profile_failure_count;
/** 当前链路已耗尽重试的连接参数档位。 */
static ble_connection_power_profile_t s_connection_failed_profile;

static void ble_host_reset(int reason);
static void ble_host_sync(void);
static void ble_host_task(void *parameter);
static void ble_host_task_deleted(int index, void *value);
static int ble_gap_event_callback(struct ble_gap_event *event, void *argument);
static int gatt_service_callback(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *service,
                                 void *argument);
static int gatt_characteristic_callback(uint16_t conn_handle,
                                        const struct ble_gatt_error *error,
                                        const struct ble_gatt_chr *characteristic,
                                        void *argument);
static int gatt_descriptor_callback(uint16_t conn_handle,
                                    const struct ble_gatt_error *error,
                                    uint16_t characteristic_val_handle,
                                    const struct ble_gatt_dsc *descriptor,
                                    void *argument);
static int gatt_mtu_callback(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             uint16_t mtu,
                             void *argument);
static int gatt_notify_write_callback(uint16_t conn_handle,
                                      const struct ble_gatt_error *error,
                                      struct ble_gatt_attr *attribute,
                                      void *argument);
static int gatt_unlock_write_callback(uint16_t conn_handle,
                                      const struct ble_gatt_error *error,
                                      struct ble_gatt_attr *attribute,
                                      void *argument);
static int gatt_control_write_callback(uint16_t conn_handle,
                                       const struct ble_gatt_error *error,
                                       struct ble_gatt_attr *attribute,
                                       void *argument);
static int start_unlock_write_boundary(
    ble_unlock_context_t *unlock_context,
    const ble_unlock_request_t *request,
    const ble_status_frame_t *mirror,
    uint32_t status_sequence,
    uint64_t now_ms,
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    ble_gatt_attr_fn *callback,
    void *callback_argument);
static int start_control_write_boundary(
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override,
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    ble_gatt_attr_fn *callback,
    void *callback_argument);
static void post_event_from_host(ble_private_event_type_t type, int32_t reason);
static void post_private_event_from_host(const ble_private_event_t *event);
static ble_gatt_callback_context_t *allocate_gatt_callback_context(void);
static void release_gatt_callback_context(ble_gatt_callback_context_t *context);
static bool uuid_matches_explicit_base(const ble_uuid_t *uuid,
                                       const ble_uuid16_t *uuid16,
                                       const ble_uuid128_t *uuid128);
static bool parse_advertisement(const struct ble_gap_disc_desc *report, bool *target_service,
                                char advertised_name[MAINTENANCE_BLE_NAME_CAPACITY]);
static bool fields_contain_target_uuid(const struct ble_hs_adv_fields *fields);
static bool report_is_connectable(uint8_t event_type);
static bool report_is_scan_response(uint8_t event_type);
static bool snapshot_screen_on(bool *screen_on);
static esp_err_t start_scan_session(void);
static void suspend_disconnected_discovery_for_screen_off(void);
static void handle_begin_binding_scan_session(void);
static void handle_end_binding_scan_session(void);
static void handle_reconnect_now(void);
static void handle_rescan(void);
static void process_disc_report(const ble_private_event_t *event);
static void finish_scan_session(const ble_private_event_t *event);
static void publish_candidate_snapshot(void);
static void clear_candidate_session_model(void);
static void retry_pending_publications(void);
static ble_selection_result_t handle_selection_command(
    const maintenance_ble_selection_command_t *command);
static void publish_selection_feedback(
    const maintenance_ble_selection_command_t *command,
    ble_selection_result_t result);
static void fail_active_selection(void);
static void handle_clear_binding(void);
static void handle_host_reset_event(int32_t reason);
static void handle_connect_result(const ble_private_event_t *event);
static void handle_disconnect(const ble_private_event_t *event);
static void handle_connection_update(const ble_private_event_t *event);
static void begin_link_admission(void);
static void handle_gatt_service_event(const ble_private_event_t *event);
static void handle_gatt_characteristic_event(const ble_private_event_t *event);
static void handle_gatt_descriptor_event(const ble_private_event_t *event);
static void begin_mtu_negotiation(void);
static void handle_mtu_result(const ble_private_event_t *event);
static void begin_notify_attempt(void);
static void handle_notify_write_result(const ble_private_event_t *event);
static void handle_notify_rx_event(const ble_private_event_t *event);
static void consume_status_bytes(const uint8_t *data,
                                 size_t length,
                                 uint64_t received_at_ms,
                                 const ble_link_token_t *link_token);
static bool link_token_is_current(const ble_link_token_t *link_token);
static void reset_status_stream(void);
static void clear_status_mirror(void);
static void check_status_freshness(void);
static uint32_t monotonic_ms32(void);
static void update_selftest_discovery_suspension(void);
static bool screen_sequence_is_after(uint32_t candidate, uint32_t baseline);
static void apply_pending_screen_state(void);
static void apply_screen_power_policy(bool screen_on);
static void apply_pending_binding_scan_request(void);
static bool connection_control_transaction_active(void);
static void request_connection_power_profile(
    ble_connection_power_profile_t profile,
    bool force);
static void request_current_connection_power_profile(bool force);
static void schedule_connection_power_profile_retry(
    ble_connection_power_profile_t profile);
static void check_connection_power_profile_retry(void);
static TickType_t owner_event_wait_ticks(void);
static TickType_t min_wait_ticks(TickType_t current, TickType_t candidate);
static TickType_t wait_ticks_until_ms(uint64_t deadline_ms,
                                      uint64_t now_ms);
static void check_radio_start_deadline(void);
static bool radio_event_is_current(const ble_private_event_t *event);
static uint32_t advance_radio_generation(void);
static esp_err_t reserve_radio_retention(void);
static void release_radio_retention(void);
static esp_err_t try_enter_radio_dormant(void);
static void configure_default_tx_power(void);
static bool configure_initiating_tx_power(bool boost);
static bool configure_connection_tx_power(uint16_t conn_handle, bool boost);
static void note_control_retry_for_tx_power(
    const control_gate_control_action_t *action);
static void invalidate_scan_generation(void);
static void schedule_reconnect(const char *error_code);
static void check_reconnect_deadline(void);
static void mark_recovery_success(void);
static bool handle_first_unlock_intent(uint32_t intent_sequence);
static void try_start_automatic_authorization(void);
#if !SCENIC_AREA_MANAGEMENT_DEBUG
static void schedule_automatic_authorization_retry(const char *error_code);
#endif
static void handle_control_intent(const ble_private_event_t *event);
static void handle_priority_poweroff_mailbox(void);
static bool control_intent_precedes_latest_poweroff(
    const ble_private_event_t *event);
static bool preempt_ordinary_control_for_poweroff(void);
static void cancel_deferred_poweroff(const char *error_code);
#if SCENIC_AREA_MANAGEMENT_DEBUG
static void try_submit_pending_cloud_rental_request(void);
static void clear_pending_cloud_rental_submission(void);
static uint32_t cloud_submission_retry_delay_ms(uint8_t attempt_count);
static void cancel_cloud_rental_session(void);
static void handle_cloud_rental_results(void);
static void handle_cloud_rental_result(const cloud_rental_result_t *result);
#endif
static bool start_unlock_write(const control_gate_action_t *action);
static void handle_unlock_write_result(const ble_private_event_t *event);
static void handle_unlock_status_evidence(const ble_link_token_t *link_token);
static void handle_control_status_evidence(const ble_link_token_t *link_token);
static void check_unlock_timeout(void);
static void abort_unlock_transaction(const char *error_code,
                                     bool invalidate_session);
static void start_control_write(const control_gate_control_action_t *action);
static void handle_control_write_result(const ble_private_event_t *event);
static void check_control_write_timeout(void);
static void apply_control_attempt_result(
    const control_gate_control_action_t *completed_action,
    bool success,
    uint32_t status_sequence_at_write,
    uint64_t now_ms,
    const char *failure_error_code,
    bool start_retry);
static void try_release_control_lockout(void);
static void invalidate_current_authorization_proof(void);
static bool finalize_poweroff_offline(uint64_t now_ms);
static void publish_poweroff_terminal(
    const control_gate_control_action_t *action,
    watch_poweroff_result_t result);
static bool control_action_to_override(
    const control_gate_control_action_t *action,
    watch_exoskeleton_scene_config_t config,
    ble_control_packet_override_t *override);
static void publish_control_update(const watch_control_update_t *update);
static bool publish_control_update_confirmed(
    const watch_control_update_t *update);
static bool control_update_matches_snapshot(
    const watch_control_update_t *update);
static bool control_status_snapshot_is_current(
    const ble_link_token_t *link_token);
static bool control_update_sequence_is_at_or_before(uint32_t candidate,
                                                    uint32_t baseline);
static void publish_unlock_result(const ble_unlock_request_t *request,
                                  ble_unlock_result_code_t result,
                                  const char *error_code);
static void publish_control_result(
    const control_gate_control_action_t *action,
    const ble_control_intent_t *intent,
    ble_control_result_code_t result,
    const char *error_code);
static ble_unlock_callback_context_t *allocate_unlock_callback_context(void);
static void reset_unlock_callback_contexts(void);
static ble_control_callback_context_t *allocate_control_callback_context(void);
static void reset_control_callback_contexts(void);
static void flush_dirty_diagnostics(void);
static void copy_product_state(watch_exoskeleton_state_t *destination,
                               const ble_status_frame_t *source);
static bool event_is_current_link(const ble_private_event_t *event);
static void check_admission_deadline(void);
static void fail_current_link(const char *error_code);
static void reset_link_admission(void);
static void invalidate_link_generation(void);
static void reset_gatt_callback_contexts(void);
static void handle_mtu_evidence(const ble_private_event_t *evidence);
static void handle_frame_evidence(const ble_private_event_t *evidence);
static void try_commit_binding(void);
static void fail_pending_binding(const char *error_code);
static void clear_pending_binding(void);
static bool evidence_matches_bound_link(const ble_private_event_t *evidence);
static bool mac_bytes_are_ambiguous(const uint8_t value[6]);
static void mark_mac_bytes_ambiguous(const uint8_t value[6]);
static void remember_connectable_observation(const ble_private_event_t *event);
static const ble_connectable_observation_t *find_connectable_observation(const ble_addr_t *address);
static bool connectable_address_seen(const ble_addr_t *address);
static void reset_transport_runtime(void);
static void reset_service_runtime(void);
static esp_err_t recover_nimble_runtime(const char *error_code);
static void restart_scan_after_connection_failure(const char *error_code);
static bool normalize_evidence_mac(const char input[BLE_MAC_CAPACITY],
                                   char output[BLE_MAC_CAPACITY]);
static void publish_ble_state(watch_ble_transaction_t transaction,
                              const char *error_code);
static void flush_pending_ble_state(void);
static bool flush_pending_ble_state_with_retry(void);
static esp_err_t connect_candidate(const ble_candidate_t *candidate);
static void format_mac(const ble_addr_t *address,
                       char output[MAINTENANCE_BLE_MAC_CAPACITY]);
static esp_err_t start_nimble(void);
static esp_err_t stop_nimble(void);

esp_err_t ble_service_prepare_run(void)
{
    if (s_nimble_started)
    {
        return ESP_ERR_INVALID_STATE;
    }
    const esp_err_t retention_error = reserve_radio_retention();
    if (retention_error != ESP_OK)
    {
        atomic_store(&s_radio_lifecycle,
                     BLE_RADIO_LIFECYCLE_STOPPING);
        return retention_error;
    }
    if (s_event_queue != NULL)
    {
        if (s_unlock_result_queue == NULL || s_control_result_queue == NULL ||
            s_nimble_started)
        {
            return ESP_ERR_INVALID_STATE;
        }
        ble_private_event_t stale_event = {0};
        while (xQueueReceive(s_event_queue, &stale_event, 0) == pdTRUE)
        {
        }
        ble_unlock_result_t stale_unlock_result = {0};
        while (xQueueReceive(s_unlock_result_queue,
                             &stale_unlock_result,
                             0) == pdTRUE)
        {
        }
        ble_control_result_t stale_control_result = {0};
        while (xQueueReceive(s_control_result_queue,
                             &stale_control_result,
                             0) == pdTRUE)
        {
        }
        reset_service_runtime();
    }
    else
    {
        if (s_nimble_started)
        {
            return ESP_ERR_INVALID_STATE;
        }
        reset_service_runtime();
        s_event_queue = xQueueCreate(BLE_EVENT_QUEUE_DEPTH, sizeof(ble_private_event_t));
        if (s_event_queue == NULL)
        {
            release_radio_retention();
            return ESP_ERR_NO_MEM;
        }
        s_unlock_result_queue = xQueueCreate(BLE_UNLOCK_RESULT_QUEUE_DEPTH,
                                             sizeof(ble_unlock_result_t));
        if (s_unlock_result_queue == NULL)
        {
            vQueueDelete(s_event_queue);
            s_event_queue = NULL;
            release_radio_retention();
            return ESP_ERR_NO_MEM;
        }
        s_control_result_queue = xQueueCreate(BLE_CONTROL_RESULT_QUEUE_DEPTH,
                                              sizeof(ble_control_result_t));
        if (s_control_result_queue == NULL)
        {
            vQueueDelete(s_unlock_result_queue);
            s_unlock_result_queue = NULL;
            vQueueDelete(s_event_queue);
            s_event_queue = NULL;
            release_radio_retention();
            return ESP_ERR_NO_MEM;
        }
    }
    config_service_snapshot_t config = {0};
    esp_err_t config_error = config_service_snapshot(
        &config,
        pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (config_error != ESP_OK)
    {
        ble_service_cancel_prepared_run();
        return config_error;
    }
    s_has_binding = config.has_bound_exoskeleton;
    memset(s_bound_mac, 0, sizeof(s_bound_mac));
    if (s_has_binding)
    {
        memcpy(s_bound_mac, config.bound_exoskeleton_mac, sizeof(s_bound_mac));
    }
    if (!s_has_binding ||
        (s_status_owner.mirror_valid &&
         strcmp(s_status_owner.mirror_token.mac, s_bound_mac) != 0))
    {
        clear_status_mirror();
    }
    memset(&s_prepare_product_state, 0, sizeof(s_prepare_product_state));
    esp_err_t state_error = watch_state_snapshot(
        &s_prepare_product_state,
        pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (state_error != ESP_OK)
    {
        ble_service_cancel_prepared_run();
        memset(&s_prepare_product_state, 0, sizeof(s_prepare_product_state));
        return state_error;
    }
    s_control_gate.control_update_sequence =
        s_prepare_product_state.control_update_sequence;
    s_screen_on =
        s_prepare_product_state.screen_state == WATCH_SCREEN_STATE_ON;
    s_screen_state_known = true;
    s_applied_screen_sequence =
        s_prepare_product_state.screen_transition_sequence;
    atomic_store(&s_pending_screen_on, s_screen_on);
    atomic_store(&s_pending_screen_sequence,
                 s_applied_screen_sequence);
    atomic_store(&s_binding_scan_request,
                 BLE_BINDING_SCAN_REQUEST_NONE);
    memset(&s_prepare_product_state, 0, sizeof(s_prepare_product_state));
    if (!flush_pending_ble_state_with_retry())
    {
        ble_service_cancel_prepared_run();
        return ESP_ERR_TIMEOUT;
    }
    const char *initial_error = config.status == CONFIG_SERVICE_BINDING_CORRUPT
                                    ? BLE_BINDING_CONFIG_CORRUPT
                                    : BLE_OK;
    publish_ble_state(s_has_binding
                          ? WATCH_BLE_TRANSACTION_BOUND_READY
                          : WATCH_BLE_TRANSACTION_UNBOUND,
                      initial_error);
    atomic_store(&s_accept_host_events, false);
    atomic_store(&s_host_queue_overflow, false);
    atomic_store(&s_host_task_deleted, true);
    atomic_store(&s_stop_requested, false);
    atomic_store(&s_owner_running, false);
    atomic_store(&s_notify_transport_failure, false);
    atomic_store(&s_radio_lifecycle, BLE_RADIO_LIFECYCLE_DORMANT);
    return ESP_OK;
}

void ble_service_cancel_prepared_run(void)
{
    if (s_nimble_started)
    {
        return;
    }
    atomic_store(&s_accept_host_events, false);
    atomic_store(&s_owner_running, false);
    atomic_store(&s_radio_lifecycle, BLE_RADIO_LIFECYCLE_STOPPING);
    if (s_event_queue != NULL)
    {
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
    }
    if (s_unlock_result_queue != NULL)
    {
        vQueueDelete(s_unlock_result_queue);
        s_unlock_result_queue = NULL;
    }
    if (s_control_result_queue != NULL)
    {
        vQueueDelete(s_control_result_queue);
        s_control_result_queue = NULL;
    }
    release_radio_retention();
    reset_service_runtime();
}

bool ble_service_startup_ready(void)
{
    const ble_radio_lifecycle_t lifecycle =
        (ble_radio_lifecycle_t)atomic_load(&s_radio_lifecycle);
    return atomic_load(&s_owner_running) &&
           (lifecycle == BLE_RADIO_LIFECYCLE_DORMANT ||
            lifecycle == BLE_RADIO_LIFECYCLE_READY);
}

bool ble_service_runtime_ready(void)
{
    return atomic_load(&s_owner_running) &&
           atomic_load(&s_radio_lifecycle) ==
               BLE_RADIO_LIFECYCLE_READY;
}

esp_err_t ble_service_notify_screen_state(
    bool screen_on,
    uint32_t transition_sequence)
{
    if (transition_sequence == 0U)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const uint32_t pending_sequence =
        (uint32_t)atomic_load(&s_pending_screen_sequence);
    if (screen_sequence_is_after(transition_sequence,
                                 pending_sequence))
    {
        atomic_store(&s_pending_screen_on, screen_on);
        atomic_store(&s_pending_screen_sequence,
                     transition_sequence);
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_SCREEN_STATE,
    };
    /* 队列满表示 owner 已有工作可取，最新屏幕事实仍由 mailbox 保留。 */
    (void)xQueueSend(s_event_queue, &event, 0);
    return ESP_OK;
}

esp_err_t ble_service_request_stop(TickType_t timeout_ticks)
{
    if (s_event_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_stop_requested, true);
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_STOP,
    };
    (void)xQueueSend(s_event_queue, &event, timeout_ticks);
    return ESP_OK;
}

esp_err_t ble_service_notify_cloud_result_ready(void)
{
    if (s_event_queue == NULL || !atomic_load(&s_owner_running))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_CLOUD_RESULT_READY,
    };
    if (xQueueSend(s_event_queue, &event, 0U) == pdTRUE)
    {
        return ESP_OK;
    }
    /* 队列已满表示 owner 已有事件可处理，循环顶部仍会领取 cloud 结果。 */
    return ESP_OK;
}

esp_err_t ble_service_begin_binding_scan_session(
    TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    atomic_store(&s_binding_scan_request,
                 BLE_BINDING_SCAN_REQUEST_BEGIN);
    const ble_private_event_t wake_event = {
        .type = BLE_PRIVATE_EVENT_WAKE,
    };
    /* 队列满表示 owner 已有事件可取，latest 请求仍不会丢失。 */
    (void)xQueueSend(s_event_queue, &wake_event, 0);
    return ESP_OK;
}

esp_err_t ble_service_end_binding_scan_session(
    TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    (void)timeout_ticks;
    atomic_store(&s_binding_scan_request,
                 BLE_BINDING_SCAN_REQUEST_END);
    const ble_private_event_t wake_event = {
        .type = BLE_PRIVATE_EVENT_WAKE,
    };
    /* 队列满表示 owner 已有事件可取，latest 请求仍不会丢失。 */
    (void)xQueueSend(s_event_queue, &wake_event, 0);
    return ESP_OK;
}

esp_err_t ble_service_request_reconnect_now(TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_RECONNECT_NOW,
    };
    return xQueueSend(s_event_queue, &event, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t ble_service_request_rescan(TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_RESCAN,
    };
    return xQueueSend(s_event_queue, &event, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t ble_service_publish_mtu_evidence(
    const ble_link_token_t *link_token,
    uint16_t mtu,
    TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || link_token == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_MTU_EVIDENCE,
        .conn_handle = link_token->conn_handle,
        .link_generation = link_token->link_generation,
        .mtu = mtu,
    };
    if (!normalize_evidence_mac(link_token->mac, event.mac))
    {
        return ESP_ERR_INVALID_ARG;
    }
    return xQueueSend(s_event_queue, &event, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t ble_service_request_first_unlock(uint32_t *intent_sequence,
                                           TickType_t timeout_ticks)
{
    if (intent_sequence == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *intent_sequence = 0U;
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested) ||
        atomic_load(&s_unlock_result_backpressured))
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t sequence = 0U;
    do
    {
        sequence =
            (uint32_t)atomic_fetch_add(&s_unlock_intent_sequence, 1U) + 1U;
    } while (sequence == 0U);
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_FIRST_UNLOCK_INTENT,
        .unlock_intent_sequence = sequence,
    };
    if (xQueueSend(s_event_queue, &event, timeout_ticks) != pdTRUE)
    {
        return ESP_ERR_TIMEOUT;
    }
    *intent_sequence = sequence;
    return ESP_OK;
}

esp_err_t ble_service_receive_unlock_result(ble_unlock_result_t *result,
                                            TickType_t timeout_ticks)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_unlock_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueReceive(s_unlock_result_queue, result, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t ble_service_request_control(const ble_control_intent_t *intent,
                                      TickType_t timeout_ticks)
{
    if (s_event_queue == NULL || !atomic_load(&s_owner_running) ||
        atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (intent == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    const bool priority_poweroff =
        intent->kind == WATCH_CONTROL_KIND_POWEROFF && intent->target == 1U;
    if (!priority_poweroff && atomic_load(&s_control_result_backpressured))
    {
        return ESP_ERR_INVALID_STATE;
    }
    uint32_t intent_sequence = 0U;
    do
    {
        intent_sequence =
            (uint32_t)atomic_fetch_add(&s_control_intent_sequence, 1U) + 1U;
    } while (intent_sequence == 0U);
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_CONTROL_INTENT,
        .control_kind = intent->kind,
        .control_target = intent->target,
        .control_intent_sequence = intent_sequence,
    };
    if (priority_poweroff)
    {
        /* 单槽只接受一个未消费请求，避免连续点击覆盖已返回 ESP_OK 的关机。 */
        uint_fast32_t empty_sequence = 0U;
        if (!atomic_compare_exchange_strong(&s_poweroff_mailbox_sequence,
                                            &empty_sequence,
                                            (uint_fast32_t)intent_sequence))
        {
            return ESP_ERR_INVALID_STATE;
        }
        const ble_private_event_t wake_event = {
            .type = BLE_PRIVATE_EVENT_WAKE,
        };
        /* 队列满说明 owner 已有事件可消费，邮箱仍不会丢失。 */
        (void)xQueueSend(s_event_queue, &wake_event, 0);
        return ESP_OK;
    }
    return xQueueSend(s_event_queue, &event, timeout_ticks) == pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t ble_service_receive_control_result(ble_control_result_t *result,
                                             TickType_t timeout_ticks)
{
    if (result == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_control_result_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    return xQueueReceive(s_control_result_queue, result, timeout_ticks) ==
                   pdTRUE
               ? ESP_OK
               : ESP_ERR_TIMEOUT;
}

esp_err_t ble_service_run(void)
{
    if (s_event_queue == NULL)
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t err = start_nimble();
    if (err != ESP_OK)
    {
        atomic_store(&s_owner_running, false);
        ESP_LOGE(TAG,
                 "BLE 服务启动失败，稳定码=%s，错误=0x%x",
                 BLE_START_FAILED,
                 (unsigned)err);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                          BLE_START_FAILED);
        (void)flush_pending_ble_state_with_retry();
        return err;
    }
    atomic_store(&s_owner_running, true);

    bool running = true;
    while (running)
    {
        if (atomic_load(&s_stop_requested))
        {
            running = false;
            continue;
        }
        if (s_nimble_cleanup.terminal_error != ESP_OK)
        {
            err = (esp_err_t)s_nimble_cleanup.terminal_error;
            running = false;
            continue;
        }
        handle_priority_poweroff_mailbox();
        apply_pending_screen_state();
        update_selftest_discovery_suspension();
        apply_pending_binding_scan_request();
        check_radio_start_deadline();
        if (s_nimble_cleanup.terminal_error != ESP_OK)
        {
            continue;
        }
        if (atomic_exchange(&s_host_queue_overflow, false))
        {
            ESP_LOGE(TAG, "BLE host 事件队列溢出，稳定码=%s", BLE_EVENT_QUEUE_OVERFLOW);
            abort_unlock_transaction(BLE_EVENT_QUEUE_OVERFLOW, false);
            esp_err_t recovery_error = recover_nimble_runtime(BLE_EVENT_QUEUE_OVERFLOW);
            if (recovery_error != ESP_OK && atomic_load(&s_stop_requested))
            {
                running = false;
                continue;
            }
            if (recovery_error != ESP_OK)
            {
                err = ESP_FAIL;
                running = false;
                continue;
            }
        }
        if (atomic_exchange(&s_notify_transport_failure, false))
        {
            ESP_LOGE(TAG,
                     "BLE Notify 原始字节入口失败，稳定码=%s",
                     BLE_NOTIFY_INGRESS_FAILED);
            fail_current_link(BLE_NOTIFY_INGRESS_FAILED);
        }
        check_admission_deadline();
        check_status_freshness();
        check_connection_power_profile_retry();
#if SCENIC_AREA_MANAGEMENT_DEBUG
        try_submit_pending_cloud_rental_request();
        handle_cloud_rental_results();
#endif
        try_start_automatic_authorization();
        check_unlock_timeout();
        check_control_write_timeout();
        check_reconnect_deadline();
        flush_dirty_diagnostics();
        retry_pending_publications();
        try_release_control_lockout();
        if (ble_service_runtime_ready())
        {
            maintenance_ble_clear_command_t clear_command = {0};
            if (maintenance_service_receive_ble_clear(&clear_command, 0) ==
                    MAINTENANCE_RESULT_OK &&
                clear_command.confirmed)
            {
                handle_clear_binding();
            }
            maintenance_ble_selection_command_t selection = {0};
            if (maintenance_service_receive_ble_selection(&selection, 0) ==
                MAINTENANCE_RESULT_OK)
            {
                ble_selection_result_t selection_result =
                    handle_selection_command(&selection);
                if (selection_result == BLE_SELECTION_RESULT_BUSY)
                {
                    ESP_LOGW(TAG,
                             "BLE 候选选择被拒绝，稳定码=%s",
                             BLE_SELECTION_BUSY);
                    publish_ble_state(s_transaction, BLE_SELECTION_BUSY);
                }
                else if (selection_result == BLE_SELECTION_RESULT_STALE)
                {
                    ESP_LOGW(TAG,
                             "BLE 候选选择已过期，稳定码=%s",
                             BLE_SELECTION_STALE);
                    publish_ble_state(s_transaction, BLE_SELECTION_STALE);
                }
                else if (selection_result == BLE_SELECTION_RESULT_ERROR)
                {
                    ESP_LOGE(TAG,
                             "BLE 候选选择执行失败，稳定码=%s",
                             BLE_SELECTION_ERROR);
                    restart_scan_after_connection_failure(
                        BLE_SELECTION_ERROR);
                }
                publish_selection_feedback(&selection,
                                           selection_result);
            }
        }
        ble_private_event_t event = {0};
        if (xQueueReceive(s_event_queue,
                          &event,
                          owner_event_wait_ticks()) != pdTRUE)
        {
            continue;
        }
        if (!radio_event_is_current(&event))
        {
            ESP_LOGW(TAG,
                     "BLE 已忽略旧无线代次事件，事件代次=%lu，当前=%lu，类型=%d",
                     (unsigned long)event.radio_generation,
                     (unsigned long)s_radio_generation,
                     (int)event.type);
            continue;
        }
        switch (event.type)
        {
        case BLE_PRIVATE_EVENT_STOP:
            running = false;
            break;
        case BLE_PRIVATE_EVENT_WAKE:
            break;
        case BLE_PRIVATE_EVENT_HOST_SYNC:
            if (atomic_load(&s_radio_lifecycle) !=
                BLE_RADIO_LIFECYCLE_STARTING)
            {
                ESP_LOGW(TAG,
                         "BLE 已忽略非 STARTING 状态的 host sync，当前状态=%d",
                         atomic_load(&s_radio_lifecycle));
                break;
            }
            s_radio_start_deadline_ms = 0U;
            atomic_store(&s_radio_lifecycle,
                         BLE_RADIO_LIFECYCLE_READY);
            ESP_LOGI(TAG, "NimBLE host 已同步，允许按当前 discovery 模式运行");
            if (!s_discovery_suspended_for_selftest &&
                !s_scanning && !s_connecting && !s_link_connected)
            {
                if (s_has_binding)
                {
                    schedule_reconnect(BLE_DISCONNECTED);
                }
                else if (s_binding_scan_session_active &&
                         s_candidate_count == 0U &&
                         start_scan_session() != ESP_OK)
                {
                    ESP_LOGE(TAG, "BLE 扫描启动失败，稳定码=%s", BLE_SCAN_FAILED);
                    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_SCAN_FAILED);
                }
            }
            break;
        case BLE_PRIVATE_EVENT_BEGIN_BINDING_SCAN:
            handle_begin_binding_scan_session();
            break;
        case BLE_PRIVATE_EVENT_END_BINDING_SCAN:
            handle_end_binding_scan_session();
            break;
        case BLE_PRIVATE_EVENT_RECONNECT_NOW:
            handle_reconnect_now();
            break;
        case BLE_PRIVATE_EVENT_RESCAN:
            handle_rescan();
            break;
        case BLE_PRIVATE_EVENT_HOST_RESET:
            handle_host_reset_event(event.reason);
            break;
        case BLE_PRIVATE_EVENT_DISC_REPORT:
            process_disc_report(&event);
            break;
        case BLE_PRIVATE_EVENT_DISC_COMPLETE:
            finish_scan_session(&event);
            break;
        case BLE_PRIVATE_EVENT_CONNECT_RESULT:
            handle_connect_result(&event);
            break;
        case BLE_PRIVATE_EVENT_DISCONNECT:
            if (event_is_current_link(&event))
            {
                handle_disconnect(&event);
            }
            else if (disconnect_matches_pending_connect(
                         s_connecting,
                         s_link_connected,
                         s_link_generation,
                         event.link_generation))
            {
                ESP_LOGW(TAG,
                         "BLE 连接建立完成前已断开，按连接失败收口，handle=%u，代次=%lu，原因=%ld",
                         (unsigned)event.conn_handle,
                         (unsigned long)event.link_generation,
                         (long)event.reason);
                handle_connect_result(&event);
            }
            else
            {
                ESP_LOGW(TAG,
                         "BLE 已忽略迟到断链事件，handle=%u，代次=%lu",
                         (unsigned)event.conn_handle,
                         (unsigned long)event.link_generation);
            }
            break;
        case BLE_PRIVATE_EVENT_CONN_UPDATE:
            handle_connection_update(&event);
            break;
        case BLE_PRIVATE_EVENT_GATT_SERVICE:
            handle_gatt_service_event(&event);
            break;
        case BLE_PRIVATE_EVENT_GATT_CHARACTERISTIC:
            handle_gatt_characteristic_event(&event);
            break;
        case BLE_PRIVATE_EVENT_GATT_DESCRIPTOR:
            handle_gatt_descriptor_event(&event);
            break;
        case BLE_PRIVATE_EVENT_MTU_RESULT:
            handle_mtu_result(&event);
            break;
        case BLE_PRIVATE_EVENT_NOTIFY_WRITE_RESULT:
            handle_notify_write_result(&event);
            break;
        case BLE_PRIVATE_EVENT_NOTIFY_RX:
            handle_notify_rx_event(&event);
            break;
        case BLE_PRIVATE_EVENT_MTU_EVIDENCE:
            handle_mtu_evidence(&event);
            break;
        case BLE_PRIVATE_EVENT_FIRST_UNLOCK_INTENT:
            handle_first_unlock_intent(event.unlock_intent_sequence);
            break;
        case BLE_PRIVATE_EVENT_UNLOCK_WRITE_RESULT:
            handle_unlock_write_result(&event);
            break;
        case BLE_PRIVATE_EVENT_CONTROL_INTENT:
            handle_control_intent(&event);
            break;
        case BLE_PRIVATE_EVENT_CONTROL_WRITE_RESULT:
            handle_control_write_result(&event);
            break;
        case BLE_PRIVATE_EVENT_CLOUD_RESULT_READY:
#if SCENIC_AREA_MANAGEMENT_DEBUG
            handle_cloud_rental_results();
#endif
            break;
        case BLE_PRIVATE_EVENT_SCREEN_STATE:
            apply_pending_screen_state();
            break;
        default:
            ESP_LOGW(TAG, "BLE 收到未知 typed 事件，类型=%d", (int)event.type);
            break;
        }
    }

    /* 先关闭公共入口，再回收连接与控制状态。 */
    atomic_store(&s_owner_running, false);
    abort_unlock_transaction(BLE_UNLOCK_LINK_CHANGED, false);
    watch_control_update_t stopped_update = {0};
    control_gate_invalidate_session(&s_control_gate, &stopped_update);
    publish_control_update(&stopped_update);
    esp_err_t stop_error = stop_nimble();
    if (stop_error == ESP_OK)
    {
        reset_unlock_callback_contexts();
        reset_control_callback_contexts();
        reset_transport_runtime();
        publish_ble_state(s_has_binding
                              ? WATCH_BLE_TRANSACTION_BOUND_READY
                              : WATCH_BLE_TRANSACTION_UNBOUND,
                          BLE_OK);
    }
    if (!flush_pending_ble_state_with_retry() && err == ESP_OK)
    {
        err = ESP_FAIL;
    }
    return err != ESP_OK ? err : stop_error;
}

static void ble_host_reset(int reason)
{
    post_event_from_host(BLE_PRIVATE_EVENT_HOST_RESET, reason);
}

static void ble_host_sync(void)
{
    post_event_from_host(BLE_PRIVATE_EVENT_HOST_SYNC, 0);
}

static void ble_host_task(void *parameter)
{
    (void)parameter;
    vTaskSetThreadLocalStoragePointerAndDelCallback(NULL,
                                                    0,
                                                    &s_host_task_deleted,
                                                    ble_host_task_deleted);
    ESP_LOGI(TAG, "NimBLE host 内部任务已启动");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

static void ble_host_task_deleted(int index, void *value)
{
    (void)index;
    if (value != NULL)
    {
        atomic_store((atomic_bool *)value, true);
    }
}

static int ble_gap_event_callback(struct ble_gap_event *event, void *argument)
{
    if (event == NULL)
    {
        return 0;
    }
    ble_private_event_t private_event = {0};
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
        private_event.connectable_report =
            report_is_connectable(event->disc.event_type);
        private_event.scan_response =
            report_is_scan_response(event->disc.event_type);
        const bool advertisement_parsed =
            parse_advertisement(&event->disc, &private_event.target_service, private_event.advertised_name);
        if (!advertisement_parsed && !private_event.connectable_report)
        {
            return 0;
        }
        if (!private_event.connectable_report && !private_event.scan_response)
        {
            return 0;
        }
        if (private_event.scan_response && !private_event.target_service && private_event.advertised_name[0] == '\0')
        {
            return 0;
        }
        private_event.type = BLE_PRIVATE_EVENT_DISC_REPORT;
        private_event.address = event->disc.addr;
        private_event.rssi = event->disc.rssi;
        private_event.scan_generation = (uint32_t)(uintptr_t)argument;
        post_private_event_from_host(&private_event);
        break;
    case BLE_GAP_EVENT_DISC_COMPLETE:
        private_event.type = BLE_PRIVATE_EVENT_DISC_COMPLETE;
        private_event.reason = event->disc_complete.reason;
        private_event.scan_generation = (uint32_t)(uintptr_t)argument;
        post_private_event_from_host(&private_event);
        break;
    case BLE_GAP_EVENT_CONNECT:
        private_event.type = BLE_PRIVATE_EVENT_CONNECT_RESULT;
        private_event.reason = event->connect.status;
        private_event.conn_handle = event->connect.conn_handle;
        private_event.link_generation = (uint32_t)(uintptr_t)argument;
        post_private_event_from_host(&private_event);
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        private_event.type = BLE_PRIVATE_EVENT_DISCONNECT;
        private_event.reason = event->disconnect.reason;
        private_event.conn_handle = event->disconnect.conn.conn_handle;
        private_event.link_generation = (uint32_t)(uintptr_t)argument;
        post_private_event_from_host(&private_event);
        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        private_event.type = BLE_PRIVATE_EVENT_CONN_UPDATE;
        private_event.reason = event->conn_update.status;
        private_event.conn_handle = event->conn_update.conn_handle;
        private_event.link_generation = (uint32_t)(uintptr_t)argument;
        post_private_event_from_host(&private_event);
        break;
    case BLE_GAP_EVENT_MTU:
        if (event->mtu.channel_id != BLE_L2CAP_CID_ATT)
        {
            break;
        }
        private_event.type = BLE_PRIVATE_EVENT_MTU_RESULT;
        private_event.conn_handle = event->mtu.conn_handle;
        private_event.link_generation = (uint32_t)(uintptr_t)argument;
        private_event.mtu = event->mtu.value;
        post_private_event_from_host(&private_event);
        break;
    case BLE_GAP_EVENT_NOTIFY_RX:
    {
        const uint32_t callback_generation = (uint32_t)(uintptr_t)argument;
        const uint16_t callback_handle =
            (uint16_t)atomic_load(&s_callback_conn_handle);
        const uint16_t value_handle =
            (uint16_t)atomic_load(&s_callback_value_handle);
        if (!atomic_load(&s_callback_notify_ready) || event->notify_rx.indication != 0U ||
            event->notify_rx.conn_handle != callback_handle ||
            event->notify_rx.attr_handle != value_handle)
        {
            break;
        }
        private_event.received_at_ms =
            (uint64_t)(esp_timer_get_time() / 1000);
        const uint16_t payload_length = OS_MBUF_PKTLEN(event->notify_rx.om);
        if (payload_length > BLE_NOTIFY_PAYLOAD_CAPACITY ||
            os_mbuf_copydata(event->notify_rx.om,
                             0,
                             payload_length,
                             private_event.payload) != 0)
        {
            atomic_store(&s_notify_transport_failure, true);
            break;
        }
        private_event.type = BLE_PRIVATE_EVENT_NOTIFY_RX;
        private_event.conn_handle = event->notify_rx.conn_handle;
        private_event.link_generation = callback_generation;
        private_event.val_handle = event->notify_rx.attr_handle;
        private_event.payload_length = payload_length;
        post_private_event_from_host(&private_event);
        break;
    }
    default:
        break;
    }
    return 0;
}

static int gatt_unlock_write_callback(uint16_t conn_handle,
                                      const struct ble_gatt_error *error,
                                      struct ble_gatt_attr *attribute,
                                      void *argument)
{
    (void)attribute;
    ble_unlock_callback_context_t *context = argument;
    if (context == NULL || error == NULL ||
        !atomic_load(&context->in_use))
    {
        return 0;
    }
    ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_UNLOCK_WRITE_RESULT,
        .reason = error->status,
        .conn_handle = conn_handle,
        .request_id = context->request.request_id,
        .unlock_intent_sequence = context->request.intent_sequence,
        .link_generation = context->request.link_generation,
        .received_at_ms = (uint64_t)(esp_timer_get_time() / 1000),
    };
    memcpy(event.mac,
           context->request.exoskeleton_mac,
           sizeof(event.mac));
    atomic_store(&context->in_use, false);
    post_private_event_from_host(&event);
    return 0;
}

static int gatt_control_write_callback(uint16_t conn_handle,
                                       const struct ble_gatt_error *error,
                                       struct ble_gatt_attr *attribute,
                                       void *argument)
{
    (void)attribute;
    ble_control_callback_context_t *context = argument;
    if (context == NULL || error == NULL ||
        !atomic_load(&context->in_use))
    {
        return 0;
    }
    ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_CONTROL_WRITE_RESULT,
        .reason = error->status,
        .conn_handle = conn_handle,
        .request_id = context->action.request_id,
        .link_generation = context->action.link_generation,
        .received_at_ms = (uint64_t)(esp_timer_get_time() / 1000),
        .control_kind = context->action.kind,
        .control_target = context->action.target,
        .control_attempt = context->action.attempt,
        .control_status_sequence_at_write =
            context->status_sequence_at_write,
    };
    memcpy(event.mac,
           context->action.exoskeleton_mac,
           sizeof(event.mac));
    atomic_store(&context->in_use, false);
    post_private_event_from_host(&event);
    return 0;
}

static int gatt_service_callback(uint16_t conn_handle,
                                 const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *service,
                                 void *argument)
{
    ble_gatt_callback_context_t *context = argument;
    if (context == NULL || error == NULL)
    {
        return 0;
    }
    ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_GATT_SERVICE,
        .reason = error->status,
        .conn_handle = conn_handle,
        .link_generation = context->link_generation,
    };
    if (error->status == 0 && service != NULL)
    {
        event.start_handle = service->start_handle;
        event.end_handle = service->end_handle;
        event.target_uuid = uuid_matches_explicit_base(&service->uuid.u,
                                                       &s_target_uuid16,
                                                       &s_target_uuid128);
    }
    else
    {
        release_gatt_callback_context(context);
    }
    post_private_event_from_host(&event);
    return 0;
}

static int gatt_characteristic_callback(uint16_t conn_handle,
                                        const struct ble_gatt_error *error,
                                        const struct ble_gatt_chr *characteristic,
                                        void *argument)
{
    ble_gatt_callback_context_t *context = argument;
    if (context == NULL || error == NULL)
    {
        return 0;
    }
    ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_GATT_CHARACTERISTIC,
        .reason = error->status,
        .conn_handle = conn_handle,
        .link_generation = context->link_generation,
    };
    if (error->status == 0 && characteristic != NULL)
    {
        event.def_handle = characteristic->def_handle;
        event.val_handle = characteristic->val_handle;
        event.properties = characteristic->properties;
        event.target_uuid = uuid_matches_explicit_base(&characteristic->uuid.u,
                                                       &s_target_characteristic_uuid16,
                                                       &s_target_characteristic_uuid128);
    }
    else
    {
        release_gatt_callback_context(context);
    }
    post_private_event_from_host(&event);
    return 0;
}

static int gatt_descriptor_callback(uint16_t conn_handle,
                                    const struct ble_gatt_error *error,
                                    uint16_t characteristic_val_handle,
                                    const struct ble_gatt_dsc *descriptor,
                                    void *argument)
{
    (void)characteristic_val_handle;
    ble_gatt_callback_context_t *context = argument;
    if (context == NULL || error == NULL)
    {
        return 0;
    }
    ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_GATT_DESCRIPTOR,
        .reason = error->status,
        .conn_handle = conn_handle,
        .link_generation = context->link_generation,
    };
    if (error->status == 0 && descriptor != NULL)
    {
        event.start_handle = descriptor->handle;
        event.target_uuid = descriptor->uuid.u.type == BLE_UUID_TYPE_16 &&
                            BLE_UUID16(&descriptor->uuid.u)->value ==
                                s_target_cccd_uuid16.value;
    }
    else
    {
        release_gatt_callback_context(context);
    }
    post_private_event_from_host(&event);
    return 0;
}

static int gatt_mtu_callback(uint16_t conn_handle,
                             const struct ble_gatt_error *error,
                             uint16_t mtu,
                             void *argument)
{
    ble_gatt_callback_context_t *context = argument;
    if (context == NULL || error == NULL)
    {
        return 0;
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_MTU_RESULT,
        .reason = error->status,
        .conn_handle = conn_handle,
        .mtu = mtu,
        .link_generation = context->link_generation,
    };
    release_gatt_callback_context(context);
    post_private_event_from_host(&event);
    return 0;
}

static int gatt_notify_write_callback(uint16_t conn_handle,
                                      const struct ble_gatt_error *error,
                                      struct ble_gatt_attr *attribute,
                                      void *argument)
{
    (void)attribute;
    ble_gatt_callback_context_t *context = argument;
    if (context == NULL || error == NULL)
    {
        return 0;
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_NOTIFY_WRITE_RESULT,
        .reason = error->status,
        .conn_handle = conn_handle,
        .link_generation = context->link_generation,
    };
    release_gatt_callback_context(context);
    post_private_event_from_host(&event);
    return 0;
}

static ble_gatt_callback_context_t *allocate_gatt_callback_context(void)
{
    for (size_t index = 0; index < BLE_GATT_CALLBACK_CONTEXT_COUNT; ++index)
    {
        bool expected = false;
        if (!atomic_compare_exchange_strong(&s_gatt_callback_contexts[index].in_use,
                                            &expected,
                                            true))
        {
            continue;
        }
        s_gatt_callback_contexts[index].conn_handle = s_connection_handle;
        s_gatt_callback_contexts[index].link_generation = s_link_generation;
        return &s_gatt_callback_contexts[index];
    }
    return NULL;
}

static ble_unlock_callback_context_t *allocate_unlock_callback_context(void)
{
    for (size_t index = 0U;
         index < BLE_UNLOCK_REQUEST_QUEUE_DEPTH;
         ++index)
    {
        bool expected = false;
        if (atomic_compare_exchange_strong(
                &s_unlock_callback_contexts[index].in_use,
                &expected,
                true))
        {
            memset(&s_unlock_callback_contexts[index].request,
                   0,
                   sizeof(s_unlock_callback_contexts[index].request));
            return &s_unlock_callback_contexts[index];
        }
    }
    return NULL;
}

static void reset_unlock_callback_contexts(void)
{
    for (size_t index = 0U;
         index < BLE_UNLOCK_REQUEST_QUEUE_DEPTH;
         ++index)
    {
        atomic_store(&s_unlock_callback_contexts[index].in_use, false);
        memset(&s_unlock_callback_contexts[index].request,
               0,
               sizeof(s_unlock_callback_contexts[index].request));
    }
}

static ble_control_callback_context_t *allocate_control_callback_context(void)
{
    for (size_t index = 0U;
         index < BLE_CONTROL_RESULT_QUEUE_DEPTH;
         ++index)
    {
        bool expected = false;
        if (atomic_compare_exchange_strong(
                &s_control_callback_contexts[index].in_use,
                &expected,
                true))
        {
            memset(&s_control_callback_contexts[index].action,
                   0,
                   sizeof(s_control_callback_contexts[index].action));
            return &s_control_callback_contexts[index];
        }
    }
    return NULL;
}

static void reset_control_callback_contexts(void)
{
    for (size_t index = 0U;
         index < BLE_CONTROL_RESULT_QUEUE_DEPTH;
         ++index)
    {
        atomic_store(&s_control_callback_contexts[index].in_use, false);
        memset(&s_control_callback_contexts[index].action,
               0,
               sizeof(s_control_callback_contexts[index].action));
    }
}

static void release_gatt_callback_context(ble_gatt_callback_context_t *context)
{
    if (context != NULL)
    {
        atomic_store(&context->in_use, false);
    }
}

static bool uuid_matches_explicit_base(const ble_uuid_t *uuid,
                                       const ble_uuid16_t *uuid16,
                                       const ble_uuid128_t *uuid128)
{
    if (uuid == NULL || uuid16 == NULL || uuid128 == NULL)
    {
        return false;
    }
    if (uuid->type == BLE_UUID_TYPE_16)
    {
        return BLE_UUID16(uuid)->value == uuid16->value;
    }
    if (uuid->type == BLE_UUID_TYPE_128 && uuid128->u.type == BLE_UUID_TYPE_128)
    {
        return ble_uuid_cmp(uuid, &uuid128->u) == 0;
    }
    return false;
}

static void post_event_from_host(ble_private_event_type_t type, int32_t reason)
{
    if (!atomic_load(&s_accept_host_events) || s_event_queue == NULL)
    {
        return;
    }
    const ble_private_event_t event = {
        .type = type,
        .reason = reason,
    };
    post_private_event_from_host(&event);
}

static void post_private_event_from_host(const ble_private_event_t *event)
{
    if (!atomic_load(&s_accept_host_events) || s_event_queue == NULL || event == NULL)
    {
        return;
    }
    ble_private_event_t generation_event = *event;
    generation_event.radio_generation =
        (uint32_t)atomic_load(&s_callback_radio_generation);
    if (xQueueSend(s_event_queue, &generation_event, 0) != pdTRUE)
    {
        atomic_store(&s_host_queue_overflow, true);
    }
}

static bool parse_advertisement(const struct ble_gap_disc_desc *report, bool *target_service,
                                char advertised_name[MAINTENANCE_BLE_NAME_CAPACITY])
{
    if (report == NULL || target_service == NULL || advertised_name == NULL || report->data == NULL ||
        report->length_data == 0U)
    {
        return false;
    }
    struct ble_hs_adv_fields fields = {0};
    if (ble_hs_adv_parse_fields(&fields, report->data, report->length_data) != 0)
    {
        return false;
    }
    *target_service = fields_contain_target_uuid(&fields);
    advertised_name[0] = '\0';
    if (fields.name != NULL && fields.name_len > 0U)
    {
        size_t copy_length = fields.name_len;
        if (copy_length >= MAINTENANCE_BLE_NAME_CAPACITY)
        {
            copy_length = MAINTENANCE_BLE_NAME_CAPACITY - 1U;
        }
        memcpy(advertised_name, fields.name, copy_length);
        advertised_name[copy_length] = '\0';
    }
    return true;
}

static bool fields_contain_target_uuid(const struct ble_hs_adv_fields *fields)
{
    for (uint8_t index = 0; index < fields->num_uuids16; ++index)
    {
        if (ble_uuid_cmp(&fields->uuids16[index].u, &s_target_uuid16.u) == 0)
        {
            return true;
        }
    }
    for (uint8_t index = 0; index < fields->num_uuids128; ++index)
    {
        if (ble_uuid_cmp(&fields->uuids128[index].u, &s_target_uuid128.u) == 0)
        {
            return true;
        }
    }
    return false;
}

static bool report_is_connectable(uint8_t event_type)
{
    return event_type == BLE_HCI_ADV_RPT_EVTYPE_ADV_IND ||
           event_type == BLE_HCI_ADV_RPT_EVTYPE_DIR_IND;
}

static bool report_is_scan_response(uint8_t event_type)
{
    return event_type == BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP;
}

static bool snapshot_screen_on(bool *screen_on)
{
    if (screen_on == NULL || !s_screen_state_known)
    {
        return false;
    }
    *screen_on = s_screen_on;
    return true;
}

static esp_err_t start_scan_session(void)
{
    if (atomic_load(&s_radio_lifecycle) !=
        BLE_RADIO_LIFECYCLE_READY)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_discovery_suspended_for_selftest)
    {
        return ESP_OK;
    }
    const ble_discovery_mode_t discovery_mode =
        ble_discovery_resolve_mode(s_has_binding,
                                   s_binding_scan_session_active);
    if (discovery_mode == BLE_DISCOVERY_MODE_IDLE)
    {
        return ESP_ERR_INVALID_STATE;
    }
    bool screen_on = false;
    if (!snapshot_screen_on(&screen_on))
    {
        ESP_LOGW(TAG, "BLE 扫描前无法读取屏幕事实，已保持停止");
        return ESP_ERR_TIMEOUT;
    }
    const ble_scan_power_policy_t scan_policy =
        ble_scan_power_policy_resolve(discovery_mode, screen_on);
    if (!scan_policy.allowed)
    {
        ESP_LOGI(TAG, "BLE 屏幕已熄灭，断连扫描保持停止");
        return ESP_OK;
    }
    if (s_scanning || s_connecting || s_link_connected)
    {
        return ESP_ERR_INVALID_STATE;
    }
    int result = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (result != 0)
    {
        return ESP_FAIL;
    }
    ++s_scan_generation;
    if (s_scan_generation == 0U)
    {
        ++s_scan_generation;
    }
    memset(s_candidates, 0, sizeof(s_candidates));
    s_candidate_count = 0U;
    memset(s_ambiguous_macs, 0, sizeof(s_ambiguous_macs));
    s_ambiguous_mac_count = 0U;
    s_identity_conflict_fail_closed = false;
    memset(s_connectable_observations, 0, sizeof(s_connectable_observations));
    s_connectable_address_count = 0U;
    s_connectable_address_next = 0U;
    s_candidate_snapshot_dirty = false;
    s_scan_diagnostic = BLE_OK;
    const struct ble_gap_disc_params parameters = {
        .itvl = scan_policy.interval_units,
        .window = scan_policy.window_units,
        .limited = 0,
        .passive = 0,
        .filter_duplicates = 1,
        .disable_observer_mode = 0,
    };
    result = ble_gap_disc(s_own_addr_type,
                          (int32_t)scan_policy.duration_ms,
                          &parameters,
                          ble_gap_event_callback,
                          (void *)(uintptr_t)s_scan_generation);
    if (result != 0)
    {
        s_scanning = false;
        return ESP_FAIL;
    }
    s_scanning = true;
    if (discovery_mode == BLE_DISCOVERY_MODE_BINDING_SESSION)
    {
        publish_ble_state(WATCH_BLE_TRANSACTION_SCANNING, BLE_OK);
        publish_candidate_snapshot();
        ESP_LOGI(TAG,
                 "BLE 候选页扫描已启动，代次=%lu，时长=%lu ms，间隔=%u，窗口=%u",
                 (unsigned long)s_scan_generation,
                 (unsigned long)scan_policy.duration_ms,
                 (unsigned)scan_policy.interval_units,
                 (unsigned)scan_policy.window_units);
    }
    else
    {
        publish_ble_state(WATCH_BLE_TRANSACTION_BOUND_READY, BLE_OK);
        ESP_LOGI(TAG,
                 "BLE 已绑定定向发现已启动，代次=%lu，绑定 MAC=%s，时长=%lu ms，间隔=%u，窗口=%u，屏幕=%s",
                 (unsigned long)s_scan_generation,
                 s_bound_mac,
                 (unsigned long)scan_policy.duration_ms,
                 (unsigned)scan_policy.interval_units,
                 (unsigned)scan_policy.window_units,
                 screen_on ? "亮" : "熄灭");
    }
    return ESP_OK;
}

static void suspend_disconnected_discovery_for_screen_off(void)
{
    ble_recovery_policy_cancel(&s_recovery_policy);
    s_recovery_in_progress = false;
    s_recovery_target_seen = false;
    if (s_link_connected)
    {
        return;
    }
    if (s_scanning && !s_selftest_scan_cancel_pending &&
        !s_binding_scan_cancel_pending && !s_screen_scan_cancel_pending)
    {
        const int cancel_result = ble_gap_disc_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            s_screen_scan_cancel_pending = true;
            ESP_LOGW(TAG,
                     "BLE 熄屏取消断连扫描返回异常，等待扫描终态，错误=%d",
                     cancel_result);
        }
        else
        {
            s_scanning = false;
        }
        invalidate_scan_generation();
        clear_candidate_session_model();
    }
    if (s_connecting)
    {
        const int cancel_result = ble_gap_conn_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            ESP_LOGW(TAG,
                     "BLE 熄屏取消断连连接返回异常，旧代次仍已作废，错误=%d",
                     cancel_result);
        }
        s_connecting = false;
        s_connecting_tx_power_boost = false;
        (void)configure_initiating_tx_power(false);
        clear_pending_binding();
        s_active_selection_pending = false;
        invalidate_link_generation();
    }
    publish_ble_state(s_has_binding
                          ? WATCH_BLE_TRANSACTION_BOUND_READY
                          : WATCH_BLE_TRANSACTION_UNBOUND,
                      s_has_binding ? BLE_DISCONNECTED : BLE_OK);
    ESP_LOGI(TAG,
             "BLE 熄屏已暂停断连扫描、连接与重连 deadline，绑定=%d",
             (int)s_has_binding);
}

static void handle_begin_binding_scan_session(void)
{
    if (s_has_binding)
    {
        s_binding_scan_session_active = false;
        ESP_LOGI(TAG, "BLE 已绑定设备，忽略候选页扫描会话开始请求");
        return;
    }
    s_binding_scan_session_active = true;
    if (s_discovery_suspended_for_selftest)
    {
        ESP_LOGI(TAG, "BLE 候选页扫描会话已记录，等待自检结束后执行");
        return;
    }
    const ble_radio_lifecycle_t lifecycle =
        (ble_radio_lifecycle_t)atomic_load(&s_radio_lifecycle);
    if (lifecycle == BLE_RADIO_LIFECYCLE_STARTING)
    {
        return;
    }
    if (lifecycle != BLE_RADIO_LIFECYCLE_READY)
    {
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                          BLE_START_FAILED);
        return;
    }
    if (s_scanning || s_binding_scan_cancel_pending)
    {
        ESP_LOGI(TAG, "BLE 候选页扫描会话已在执行，无需重复启动");
        return;
    }
    if (s_connecting || s_link_connected || s_pending_binding.active)
    {
        ESP_LOGI(TAG, "BLE 候选连接正在执行，暂不重复启动候选扫描");
        return;
    }
    if (s_candidate_count == 0U && start_scan_session() != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE 候选页扫描启动失败，稳定码=%s", BLE_SCAN_FAILED);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_SCAN_FAILED);
    }
}

static void handle_end_binding_scan_session(void)
{
    s_binding_scan_session_active = false;
    if (s_has_binding)
    {
        ESP_LOGI(TAG, "BLE 已绑定设备，候选页扫描会话结束请求无需处理");
        return;
    }

    if (s_scanning && !s_selftest_scan_cancel_pending)
    {
        const int cancel_result = ble_gap_disc_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            s_binding_scan_cancel_pending = true;
            ESP_LOGW(TAG,
                     "BLE 候选页离开时取消扫描返回异常，等待扫描终态，错误=%d",
                     cancel_result);
        }
        else
        {
            s_scanning = false;
            s_binding_scan_cancel_pending = false;
        }
    }

    invalidate_scan_generation();
    clear_candidate_session_model();
    if (!s_connecting && !s_link_connected && !s_pending_binding.active)
    {
        publish_ble_state(WATCH_BLE_TRANSACTION_UNBOUND, BLE_OK);
    }
    ESP_LOGI(TAG,
             "BLE 候选页扫描会话已结束，迟到代次已作废，当前代次=%lu",
             (unsigned long)s_scan_generation);
    (void)try_enter_radio_dormant();
}

static void handle_reconnect_now(void)
{
    if (s_discovery_suspended_for_selftest)
    {
        return;
    }
    if (!s_has_binding)
    {
        ESP_LOGW(TAG, "BLE 尚未绑定设备，忽略立即重连请求");
        return;
    }
    if (s_link_connected || s_connecting)
    {
        ESP_LOGI(TAG, "BLE 已连接或正在连接，无需重复执行立即重连");
        return;
    }
    bool screen_on = false;
    if (!snapshot_screen_on(&screen_on) || !screen_on)
    {
        ble_recovery_policy_cancel(&s_recovery_policy);
        ESP_LOGI(TAG, "BLE 熄屏期间忽略立即重连，保持断连空闲态");
        return;
    }
    if (s_scanning)
    {
        ESP_LOGI(TAG, "BLE 已在扫描绑定设备，立即重连正在执行");
        return;
    }

    ble_recovery_policy_cancel(&s_recovery_policy);
    ++s_recovery_generation;
    if (s_recovery_generation == 0U)
    {
        ++s_recovery_generation;
    }
    s_recovery_in_progress = true;
    s_recovery_target_seen = false;
    s_reconnect_attempt_count =
        ble_recovery_saturating_increment(s_reconnect_attempt_count);
    ESP_LOGI(TAG,
             "BLE 立即重连开始，累计=%lu，仅匹配绑定 MAC=%s",
             (unsigned long)s_reconnect_attempt_count,
             s_bound_mac);
    if (start_scan_session() != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE 立即重连扫描启动失败，稳定码=%s", BLE_SCAN_FAILED);
        schedule_reconnect(BLE_SCAN_FAILED);
    }
}

static void handle_rescan(void)
{
    if (s_discovery_suspended_for_selftest)
    {
        return;
    }
    if (s_has_binding)
    {
        ESP_LOGW(TAG, "BLE 已有绑定设备，忽略候选重扫请求");
        return;
    }
    if (!s_binding_scan_session_active)
    {
        ESP_LOGW(TAG, "BLE 候选页扫描会话未开启，忽略重扫请求");
        return;
    }
    if (s_connecting || s_link_connected)
    {
        ESP_LOGW(TAG, "BLE 正在连接或已有连接，忽略候选重扫请求");
        return;
    }
    if (atomic_load(&s_radio_lifecycle) ==
        BLE_RADIO_LIFECYCLE_DORMANT)
    {
        clear_candidate_session_model();
        if (start_nimble() != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "BLE 候选重扫冷启动无线栈失败，稳定码=%s",
                     BLE_START_FAILED);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_START_FAILED);
        }
        return;
    }
    if (atomic_load(&s_radio_lifecycle) !=
        BLE_RADIO_LIFECYCLE_READY)
    {
        return;
    }
    if (s_scanning)
    {
        int cancel_result = ble_gap_disc_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            ESP_LOGE(TAG,
                     "BLE 候选重扫取消旧扫描失败，稳定码=%s，错误=%d",
                     BLE_SCAN_FAILED,
                     cancel_result);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_SCAN_FAILED);
            return;
        }
        s_scanning = false;
    }
    if (start_scan_session() != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE 候选重扫启动失败，稳定码=%s", BLE_SCAN_FAILED);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_SCAN_FAILED);
    }
}

static void process_disc_report(const ble_private_event_t *event)
{
    if (!s_scanning || event == NULL ||
        event->scan_generation != s_scan_generation)
    {
        return;
    }
    if (event->connectable_report)
    {
        remember_connectable_observation(event);
    }
    if ((event->scan_response && !connectable_address_seen(&event->address)) || s_identity_conflict_fail_closed)
    {
        return;
    }
    char candidate_mac[MAINTENANCE_BLE_MAC_CAPACITY] = {0};
    format_mac(&event->address, candidate_mac);
    if (s_has_binding && strcmp(candidate_mac, s_bound_mac) != 0)
    {
        return;
    }
    if (mac_bytes_are_ambiguous(event->address.val))
    {
        return;
    }
    for (size_t index = 0; index < s_candidate_count; ++index)
    {
        if (memcmp(s_candidates[index].address.val,
                   event->address.val,
                   sizeof(event->address.val)) != 0)
        {
            continue;
        }
        if (s_candidates[index].address.type == event->address.type)
        {
            if (s_has_binding)
            {
                s_recovery_target_seen = true;
            }
            if (event->rssi != MAINTENANCE_BLE_RSSI_UNAVAILABLE && s_candidates[index].rssi != event->rssi)
            {
                s_candidates[index].rssi = event->rssi;
            }
            if (event->advertised_name[0] != '\0' &&
                strcmp(s_candidates[index].advertised_name, event->advertised_name) != 0)
            {
                memcpy(s_candidates[index].advertised_name, event->advertised_name,
                       sizeof(s_candidates[index].advertised_name));
            }
            return;
        }
        if (!event->target_service)
        {
            return;
        }
        mark_mac_bytes_ambiguous(event->address.val);
        if (s_identity_conflict_fail_closed)
        {
            s_scan_diagnostic = BLE_CANDIDATE_IDENTITY_CONFLICT;
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_CANDIDATE_IDENTITY_CONFLICT);
            return;
        }
        if (index + 1U < s_candidate_count)
        {
            memmove(&s_candidates[index],
                    &s_candidates[index + 1U],
                    (s_candidate_count - index - 1U) * sizeof(s_candidates[0]));
        }
        --s_candidate_count;
        memset(&s_candidates[s_candidate_count], 0, sizeof(s_candidates[0]));
        s_scan_diagnostic = BLE_CANDIDATE_IDENTITY_CONFLICT;
        ESP_LOGE(TAG,
                 "BLE 候选 MAC 地址类型冲突，已拒绝该身份，稳定码=%s",
                 BLE_CANDIDATE_IDENTITY_CONFLICT);
        publish_ble_state(WATCH_BLE_TRANSACTION_SCANNING,
                          BLE_CANDIDATE_IDENTITY_CONFLICT);
        return;
    }
    if (!event->target_service)
    {
        return;
    }
    if (s_has_binding)
    {
        s_recovery_target_seen = true;
    }
    if (s_candidate_count >= MAINTENANCE_BLE_CANDIDATE_MAX_COUNT)
    {
        ESP_LOGW(TAG, "BLE 候选超过 8 项上限，稳定码=%s", BLE_CANDIDATE_OVERFLOW);
        s_scan_diagnostic = BLE_CANDIDATE_OVERFLOW;
        publish_ble_state(WATCH_BLE_TRANSACTION_SCANNING, BLE_CANDIDATE_OVERFLOW);
        return;
    }
    const size_t insertion_index = s_candidate_count;
    const ble_connectable_observation_t *observation = find_connectable_observation(&event->address);
    s_candidates[insertion_index].address = event->address;
    s_candidates[insertion_index].rssi =
        event->rssi == MAINTENANCE_BLE_RSSI_UNAVAILABLE && observation != NULL ? observation->rssi : event->rssi;
    memcpy(s_candidates[insertion_index].mac,
           candidate_mac,
           sizeof(s_candidates[insertion_index].mac));
    if (event->advertised_name[0] != '\0')
    {
        memcpy(s_candidates[insertion_index].advertised_name, event->advertised_name,
               sizeof(s_candidates[insertion_index].advertised_name));
    }
    else if (observation != NULL)
    {
        memcpy(s_candidates[insertion_index].advertised_name, observation->advertised_name,
               sizeof(s_candidates[insertion_index].advertised_name));
    }
    ++s_candidate_count;
    if (s_has_binding)
    {
        ble_candidate_t bound_candidate = s_candidates[insertion_index];
        if (connect_candidate(&bound_candidate) != ESP_OK)
        {
            ESP_LOGE(TAG, "BLE 已绑定身份连接启动失败，稳定码=%s", BLE_CONNECT_FAILED);
            schedule_reconnect(BLE_CONNECT_FAILED);
        }
    }
}

static void finish_scan_session(const ble_private_event_t *event)
{
    if (s_screen_scan_cancel_pending)
    {
        s_screen_scan_cancel_pending = false;
        s_scanning = false;
        bool screen_on = false;
        if (s_binding_scan_session_active &&
            !s_discovery_suspended_for_selftest &&
            !atomic_load(&s_stop_requested) &&
            snapshot_screen_on(&screen_on) && screen_on &&
            start_scan_session() != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "BLE 亮屏后候选扫描恢复失败，稳定码=%s",
                     BLE_SCAN_FAILED);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_SCAN_FAILED);
        }
        (void)try_enter_radio_dormant();
        return;
    }
    if (s_binding_scan_cancel_pending)
    {
        s_binding_scan_cancel_pending = false;
        s_scanning = false;
        if (s_binding_scan_session_active &&
            !s_discovery_suspended_for_selftest &&
            !atomic_load(&s_stop_requested) &&
            start_scan_session() != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "BLE 候选页重新进入后扫描恢复失败，稳定码=%s",
                     BLE_SCAN_FAILED);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_SCAN_FAILED);
        }
        (void)try_enter_radio_dormant();
        return;
    }
    if (s_selftest_scan_cancel_pending)
    {
        s_selftest_scan_cancel_pending = false;
        s_scanning = false;
        if (!s_discovery_suspended_for_selftest &&
            !atomic_load(&s_stop_requested))
        {
            if (s_has_binding)
            {
                schedule_reconnect(BLE_DISCONNECTED);
            }
            else if (s_binding_scan_session_active &&
                     start_scan_session() != ESP_OK)
            {
                ESP_LOGE(TAG,
                         "自检结束后 BLE 候选扫描恢复失败，稳定码=%s",
                         BLE_SCAN_FAILED);
                publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                  BLE_SCAN_FAILED);
            }
        }
        (void)try_enter_radio_dormant();
        return;
    }
    if (s_discovery_suspended_for_selftest)
    {
        s_scanning = false;
        (void)try_enter_radio_dormant();
        return;
    }
    if (!s_scanning || event == NULL ||
        event->scan_generation != s_scan_generation)
    {
        return;
    }
    s_scanning = false;
    if (event->reason != 0)
    {
        memset(s_candidates, 0, sizeof(s_candidates));
        s_candidate_count = 0U;
        ESP_LOGE(TAG,
                 "BLE 扫描异常结束，稳定码=%s，原因=%ld",
                 BLE_SCAN_FAILED,
                 (long)event->reason);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_SCAN_FAILED);
        publish_candidate_snapshot();
        if (s_has_binding)
        {
            schedule_reconnect(BLE_SCAN_FAILED);
        }
        (void)try_enter_radio_dormant();
        return;
    }
    if (s_identity_conflict_fail_closed)
    {
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                          BLE_CANDIDATE_IDENTITY_CONFLICT);
        publish_candidate_snapshot();
        if (s_has_binding)
        {
            schedule_reconnect(BLE_CANDIDATE_IDENTITY_CONFLICT);
        }
        (void)try_enter_radio_dormant();
        return;
    }
    if (s_candidate_count == 0U)
    {
        publish_ble_state(s_has_binding
                              ? WATCH_BLE_TRANSACTION_BOUND_READY
                              : WATCH_BLE_TRANSACTION_UNBOUND,
                          s_scan_diagnostic);
        publish_candidate_snapshot();
        if (s_has_binding)
        {
            schedule_reconnect(BLE_SCAN_FAILED);
        }
        (void)try_enter_radio_dormant();
        return;
    }
    if (s_candidate_count == 1U && s_has_binding)
    {
        if (connect_candidate(&s_candidates[0]) != ESP_OK)
        {
            ESP_LOGE(TAG, "BLE 已绑定单候选自动连接失败，稳定码=%s", BLE_CONNECT_FAILED);
            restart_scan_after_connection_failure(BLE_CONNECT_FAILED);
        }
        return;
    }
    if (s_has_binding)
    {
        schedule_reconnect(BLE_CONNECT_FAILED);
        return;
    }
    ESP_LOGI(TAG,
             "BLE 候选页扫描完成，代次=%lu，候选数量=%u",
             (unsigned long)s_scan_generation,
             (unsigned)s_candidate_count);
    publish_ble_state(WATCH_BLE_TRANSACTION_SELECTING, s_scan_diagnostic);
    publish_candidate_snapshot();
    (void)try_enter_radio_dormant();
}

static void publish_candidate_snapshot(void)
{
    if (s_discovery_suspended_for_selftest ||
        !ble_discovery_mode_publishes_candidates(
            ble_discovery_resolve_mode(
                s_has_binding,
                s_binding_scan_session_active)))
    {
        s_candidate_snapshot_dirty = false;
        return;
    }
    s_candidate_snapshot_dirty = true;
    memset(s_candidate_snapshot_workspace, 0, sizeof(s_candidate_snapshot_workspace));
    for (size_t index = 0; index < s_candidate_count; ++index)
    {
        memcpy(s_candidate_snapshot_workspace[index].mac, s_candidates[index].mac,
               sizeof(s_candidate_snapshot_workspace[index].mac));
        memcpy(s_candidate_snapshot_workspace[index].advertised_name, s_candidates[index].advertised_name,
               sizeof(s_candidate_snapshot_workspace[index].advertised_name));
        s_candidate_snapshot_workspace[index].rssi = s_candidates[index].rssi;
    }
    const maintenance_ble_candidate_t *values = s_candidate_count == 0U ? NULL : s_candidate_snapshot_workspace;
    maintenance_result_t result = maintenance_service_publish_ble_candidates(
        s_scan_generation,
        values,
        s_candidate_count,
        pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (result == MAINTENANCE_RESULT_OK)
    {
        ESP_LOGI(TAG,
                 "BLE 候选已发布，代次=%lu，数量=%u",
                 (unsigned long)s_scan_generation,
                 (unsigned)s_candidate_count);
    }
    else if (result == MAINTENANCE_RESULT_STALE)
    {
        /* 旧代次或已接受选择的候选快照不再覆盖当前 UI 事实。 */
        s_candidate_snapshot_dirty = false;
        ESP_LOGD(TAG, "BLE 候选代次=%lu 已过期或已锁定选择，忽略更新", (unsigned long)s_scan_generation);
        return;
    }
    else
    {
        ESP_LOGE(TAG,
                 "BLE 候选快照发布失败，代次=%lu，结果=%d",
                 (unsigned long)s_scan_generation,
                 (int)result);
        return;
    }
    s_candidate_snapshot_dirty = false;
    /* 仅在用户主动触发扫描（非自动恢复背景扫描）时导航到 BLE 候选页，
       避免 auto-recovery 覆盖正在进行的 DRV-01 等测试页面。 */
    if (!s_recovery_in_progress)
    {
        const ui_service_request_t request = {
            .type = UI_SERVICE_REQUEST_MAINTENANCE_CANDIDATES,
            .item_id = SELFTEST_ITEM_COUNT,
        };
        if (ui_service_post_request(&request, 0) != ESP_OK)
        {
            /* 真实快照已在 maintenance model，测试人员仍可手动 REFRESH。 */
            ESP_LOGW(TAG, "BLE 候选已保存但 UI 刷新队列满，稳定码=%s", BLE_UI_REFRESH_QUEUE_FULL);
            publish_ble_state(s_transaction, BLE_UI_REFRESH_QUEUE_FULL);
        }
    }
}

static void clear_candidate_session_model(void)
{
    memset(s_candidates, 0, sizeof(s_candidates));
    s_candidate_count = 0U;
    memset(s_ambiguous_macs, 0, sizeof(s_ambiguous_macs));
    s_ambiguous_mac_count = 0U;
    s_identity_conflict_fail_closed = false;
    memset(s_connectable_observations, 0,
           sizeof(s_connectable_observations));
    s_connectable_address_count = 0U;
    s_connectable_address_next = 0U;
    s_candidate_snapshot_dirty = false;
    memset(&s_pending_selection_command, 0,
           sizeof(s_pending_selection_command));
    s_pending_selection_feedback_dirty = false;

    if (s_scan_generation == 0U)
    {
        s_candidate_session_clear_dirty = false;
        s_candidate_session_clear_generation = 0U;
        return;
    }
    s_candidate_session_clear_generation = s_scan_generation;
    s_candidate_session_clear_dirty = true;
    const maintenance_result_t result =
        maintenance_service_clear_ble_candidate_session(
            s_candidate_session_clear_generation,
            pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (result == MAINTENANCE_RESULT_OK ||
        result == MAINTENANCE_RESULT_STALE)
    {
        s_candidate_session_clear_dirty = false;
    }
    else
    {
        ESP_LOGW(TAG,
                 "BLE 候选会话瞬态清理待重试，代次=%lu，结果=%d",
                 (unsigned long)s_candidate_session_clear_generation,
                 (int)result);
    }
}

static void retry_pending_publications(void)
{
    flush_pending_ble_state();
    if (s_candidate_session_clear_dirty)
    {
        const maintenance_result_t clear_result =
            maintenance_service_clear_ble_candidate_session(
                s_candidate_session_clear_generation,
                0);
        if (clear_result == MAINTENANCE_RESULT_OK ||
            clear_result == MAINTENANCE_RESULT_STALE)
        {
            s_candidate_session_clear_dirty = false;
        }
    }
    if (s_pending_control_update_dirty &&
        state_service_publish_control(&s_pending_control_update, 0) == ESP_OK)
    {
        s_pending_control_update_dirty = false;
    }
    if (!s_pending_state_update_dirty && s_status_owner.mirror_valid &&
        s_control_gate.control_pending &&
        s_control_gate.control_phase ==
            WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED)
    {
        /* mailbox 已接收匹配帧后持续复核 watch_state，避免唯一证据因暂忙丢失。 */
        handle_control_status_evidence(&s_status_owner.mirror_token);
    }
    if (s_pending_unlock_result_count > 0U && s_unlock_result_queue != NULL &&
        xQueueSend(s_unlock_result_queue,
                   &s_pending_unlock_results[s_pending_unlock_result_head],
                   0) == pdTRUE)
    {
        memset(&s_pending_unlock_results[s_pending_unlock_result_head],
               0,
               sizeof(s_pending_unlock_results[s_pending_unlock_result_head]));
        s_pending_unlock_result_head =
            (s_pending_unlock_result_head + 1U) %
            BLE_UNLOCK_PENDING_RESULT_DEPTH;
        --s_pending_unlock_result_count;
        atomic_store(&s_unlock_result_backpressured,
                     s_pending_unlock_result_count != 0U);
    }
    if (s_pending_control_result_count > 0U &&
        s_control_result_queue != NULL &&
        xQueueSend(s_control_result_queue,
                   &s_pending_control_results[s_pending_control_result_head],
                   0) == pdTRUE)
    {
        memset(&s_pending_control_results[s_pending_control_result_head],
               0,
               sizeof(s_pending_control_results[s_pending_control_result_head]));
        s_pending_control_result_head =
            (s_pending_control_result_head + 1U) %
            BLE_CONTROL_PENDING_RESULT_DEPTH;
        --s_pending_control_result_count;
        atomic_store(&s_control_result_backpressured,
                     s_pending_control_result_count != 0U);
    }
    if (s_candidate_snapshot_dirty &&
        !s_discovery_suspended_for_selftest)
    {
        publish_candidate_snapshot();
    }
    if (s_pending_selection_feedback_dirty)
    {
        publish_selection_feedback(&s_pending_selection_command,
                                   s_pending_selection_result);
    }
}

static ble_selection_result_t handle_selection_command(
    const maintenance_ble_selection_command_t *command)
{
    if (command == NULL)
    {
        return BLE_SELECTION_RESULT_ERROR;
    }
    if (s_has_binding || !s_binding_scan_session_active)
    {
        return BLE_SELECTION_RESULT_STALE;
    }
    if (s_discovery_suspended_for_selftest)
    {
        return BLE_SELECTION_RESULT_BUSY;
    }
    if (s_connecting || s_link_connected || s_pending_binding.active)
    {
        return BLE_SELECTION_RESULT_BUSY;
    }
    if (command->generation != s_scan_generation)
    {
        return BLE_SELECTION_RESULT_STALE;
    }
    for (size_t index = 0; index < s_candidate_count; ++index)
    {
        if (strcmp(command->mac, s_candidates[index].mac) != 0)
        {
            continue;
        }
        if (s_has_binding && strcmp(command->mac, s_bound_mac) != 0)
        {
            return BLE_SELECTION_RESULT_STALE;
        }
        ESP_LOGI(TAG, "BLE 候选已选择，代次=%lu，MAC=%s，索引=%u",
                 (unsigned long)command->generation,
                 command->mac,
                 (unsigned)index);
        return connect_candidate(&s_candidates[index]) == ESP_OK
                   ? BLE_SELECTION_RESULT_OK
                   : BLE_SELECTION_RESULT_ERROR;
    }
    return BLE_SELECTION_RESULT_STALE;
}

static void publish_selection_feedback(
    const maintenance_ble_selection_command_t *command,
    ble_selection_result_t result)
{
    if (command == NULL)
    {
        return;
    }
    /* 将 BLE owner 内部结果稳定映射到维护瞬态模型。 */
    static const maintenance_ble_selection_result_t feedback_results[] = {
        [BLE_SELECTION_RESULT_OK] = MAINTENANCE_BLE_SELECTION_CONNECTING,
        [BLE_SELECTION_RESULT_BUSY] = MAINTENANCE_BLE_SELECTION_BUSY,
        [BLE_SELECTION_RESULT_STALE] = MAINTENANCE_BLE_SELECTION_STALE,
        [BLE_SELECTION_RESULT_ERROR] = MAINTENANCE_BLE_SELECTION_ERROR,
    };
    s_pending_selection_command = *command;
    s_pending_selection_result = result;
    s_pending_selection_feedback_dirty = true;
    if (result == BLE_SELECTION_RESULT_OK)
    {
        s_active_selection_command = *command;
        s_active_selection_pending = true;
    }
    maintenance_result_t publish_result =
        maintenance_service_publish_ble_selection_feedback(
            command->generation,
            command->mac,
            feedback_results[result],
            pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (publish_result == MAINTENANCE_RESULT_STALE)
    {
        s_pending_selection_feedback_dirty = false;
        return;
    }
    if (publish_result != MAINTENANCE_RESULT_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 选择最终结果未保存，代次=%lu，结果=%d",
                 (unsigned long)command->generation,
                 (int)publish_result);
        return;
    }
    s_pending_selection_feedback_dirty = false;
    const ui_service_request_t request = {
        .type = UI_SERVICE_REQUEST_MAINTENANCE_CANDIDATES,
        .item_id = SELFTEST_ITEM_COUNT,
    };
    if (ui_service_post_request(&request, 0) != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 选择最终结果已保存但 UI 刷新队列满，稳定码=%s",
                 BLE_UI_REFRESH_QUEUE_FULL);
        publish_ble_state(s_transaction, BLE_UI_REFRESH_QUEUE_FULL);
    }
}

static void fail_active_selection(void)
{
    if (!s_active_selection_pending)
    {
        return;
    }
    maintenance_ble_selection_command_t failed_selection =
        s_active_selection_command;
    s_active_selection_pending = false;
    memset(&s_active_selection_command, 0, sizeof(s_active_selection_command));
    publish_selection_feedback(&failed_selection, BLE_SELECTION_RESULT_ERROR);
}

static void handle_clear_binding(void)
{
    s_binding_scan_session_active = false;
    ble_recovery_policy_reset(&s_recovery_policy);
    s_recovery_in_progress = false;
    ++s_recovery_generation;
    if (s_recovery_generation == 0U)
    {
        ++s_recovery_generation;
    }
    /* 确认命令一到达就立即关闭控制与解锁证明。 */
    reset_link_admission();
    clear_status_mirror();
    s_link_control_ready = false;
    abort_unlock_transaction(BLE_UNLOCK_LINK_CHANGED, false);
    watch_control_update_t control_update = {0};
    control_gate_invalidate_session(&s_control_gate, &control_update);
    publish_control_update(&control_update);
    s_poweroff_issued_in_session = false;
    s_active_selection_pending = false;
    const bool connection_in_progress = s_connecting;
    clear_pending_binding();
    publish_ble_state(WATCH_BLE_TRANSACTION_CLEARING, BLE_OK);

    esp_err_t err = config_service_clear_bound_exoskeleton_mac();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "BLE 绑定清除提交失败，稳定码=%s，错误=0x%x",
                 BLE_BINDING_CLEAR_FAILED,
                 (unsigned)err);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_BINDING_CLEAR_FAILED);
        if (recover_nimble_runtime(BLE_BINDING_CLEAR_FAILED) != ESP_OK)
        {
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_DISCONNECT_FAILED);
        }
        return;
    }

    s_has_binding = false;
    ble_tx_power_policy_reset(&s_tx_power_policy);
    /* 保存清除前的绑定 MAC 用于日志和后续流程 */
    char cleared_mac[MAINTENANCE_BLE_MAC_CAPACITY] = {0};
    memcpy(cleared_mac, s_bound_mac, sizeof(cleared_mac));
    memset(s_bound_mac, 0, sizeof(s_bound_mac));
    if (s_scanning)
    {
        (void)ble_gap_disc_cancel();
        s_scanning = false;
    }
    invalidate_scan_generation();
    clear_candidate_session_model();
    if (s_link_connected && s_connection_handle != UINT16_MAX)
    {
        int terminate_result = ble_gap_terminate(s_connection_handle,
                                                 BLE_ERR_REM_USER_CONN_TERM);
        if (terminate_result == 0)
        {
            ESP_LOGI(TAG, "BLE 绑定清除成功，MAC=%s（已断开连接）", cleared_mac);
            s_restart_scan_after_clear = true;
            publish_ble_state(WATCH_BLE_TRANSACTION_CLEARING, BLE_OK);
            return;
        }
        if (terminate_result != BLE_HS_ENOTCONN)
        {
            ESP_LOGE(TAG,
                     "BLE 旧 peer 终止请求失败，稳定码=%s，错误=%d",
                     BLE_DISCONNECT_FAILED,
                     terminate_result);
            if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
            {
                publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                  BLE_DISCONNECT_FAILED);
            }
            return;
        }
        invalidate_link_generation();
        s_link_connected = false;
        s_connection_handle = UINT16_MAX;
    }

    if (connection_in_progress)
    {
        s_restart_scan_after_clear = true;
        int cancel_result = ble_gap_conn_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            ESP_LOGE(TAG,
                     "BLE 清绑时连接取消请求失败，稳定码=%s，错误=%d",
                     BLE_DISCONNECT_FAILED,
                     cancel_result);
            if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
            {
                publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                  BLE_DISCONNECT_FAILED);
            }
            return;
        }
        publish_ble_state(WATCH_BLE_TRANSACTION_CLEARING, BLE_OK);
        return;
    }
    ESP_LOGI(TAG, "BLE 绑定清除成功，MAC=%s，取消连接=%d",
             cleared_mac, (int)s_restart_scan_after_clear);
    publish_ble_state(WATCH_BLE_TRANSACTION_UNBOUND, BLE_OK);
    memset(cleared_mac, 0, sizeof(cleared_mac));
    (void)try_enter_radio_dormant();
}

static void handle_host_reset_event(int32_t reason)
{
    fail_active_selection();
    reset_transport_runtime();
    invalidate_current_authorization_proof();
    ESP_LOGE(TAG,
             "NimBLE host 已重置，稳定码=%s，原因=%ld",
             BLE_HOST_RESET,
             (long)reason);
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_HOST_RESET);
}

static void handle_connect_result(const ble_private_event_t *event)
{
    if (event == NULL)
    {
        return;
    }
    const bool current_attempt =
        event->link_generation != 0U &&
        event->link_generation == s_link_generation;
    const bool attempted_with_boost =
        current_attempt && s_connecting_tx_power_boost;
    if (current_attempt)
    {
        s_connecting_tx_power_boost = false;
        (void)configure_initiating_tx_power(false);
    }
    if (s_selftest_connection_cancel_pending &&
        event->link_generation == s_link_generation)
    {
        s_selftest_connection_cancel_pending = false;
        s_connecting = false;
        clear_pending_binding();
        s_active_selection_pending = false;
        if (event->reason == 0)
        {
            s_link_connected = true;
            s_connection_handle = event->conn_handle;
            const int terminate_result =
                ble_gap_terminate(event->conn_handle,
                                  BLE_ERR_REM_USER_CONN_TERM);
            if (terminate_result == BLE_HS_ENOTCONN)
            {
                s_link_connected = false;
                s_connection_handle = UINT16_MAX;
                restart_scan_after_connection_failure(BLE_DISCONNECTED);
            }
            else if (terminate_result != 0)
            {
                ESP_LOGE(TAG,
                         "自检暂停时终止迟到 BLE 连接失败，错误=%d",
                         terminate_result);
                if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
                {
                    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                      BLE_DISCONNECT_FAILED);
                }
            }
        }
        else
        {
            restart_scan_after_connection_failure(BLE_DISCONNECTED);
        }
        return;
    }
    if (s_restart_scan_after_clear)
    {
        s_connecting = false;
        if (event->reason != 0)
        {
            s_restart_scan_after_clear = false;
            s_link_connected = false;
            s_connection_handle = UINT16_MAX;
            clear_pending_binding();
            publish_ble_state(WATCH_BLE_TRANSACTION_UNBOUND, BLE_OK);
            (void)try_enter_radio_dormant();
            return;
        }
        s_link_connected = true;
        s_connection_handle = event->conn_handle;
        clear_pending_binding();
        int terminate_result = ble_gap_terminate(event->conn_handle,
                                                 BLE_ERR_REM_USER_CONN_TERM);
        if (terminate_result == 0)
        {
            publish_ble_state(WATCH_BLE_TRANSACTION_CLEARING, BLE_OK);
            return;
        }
        s_restart_scan_after_clear = false;
        if (terminate_result != BLE_HS_ENOTCONN)
        {
            ESP_LOGE(TAG,
                     "BLE 清绑后的迟到连接无法终止，稳定码=%s，错误=%d",
                     BLE_DISCONNECT_FAILED,
                     terminate_result);
            if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
            {
                publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                  BLE_DISCONNECT_FAILED);
            }
            return;
        }
        s_link_connected = false;
        s_connection_handle = UINT16_MAX;
        publish_ble_state(WATCH_BLE_TRANSACTION_UNBOUND, BLE_OK);
        (void)try_enter_radio_dormant();
        return;
    }
    if (event->link_generation == 0U ||
        event->link_generation != s_link_generation)
    {
        ESP_LOGW(TAG,
                 "BLE 已忽略迟到连接结果，代次=%lu，当前=%lu",
                 (unsigned long)event->link_generation,
                 (unsigned long)s_link_generation);
        if (event->reason == 0)
        {
            int terminate_result = ble_gap_terminate(event->conn_handle,
                                                     BLE_ERR_REM_USER_CONN_TERM);
            if (terminate_result != 0 && terminate_result != BLE_HS_ENOTCONN)
            {
                (void)recover_nimble_runtime(BLE_DISCONNECT_FAILED);
            }
        }
        return;
    }
    s_connecting = false;
    if (event->reason != 0)
    {
        fail_active_selection();
        s_link_connected = false;
        clear_pending_binding();
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_CONNECT_FAILED);
        ESP_LOGE(TAG,
                 "BLE 候选连接失败，稳定码=%s，原因=%ld",
                 BLE_CONNECT_FAILED,
                 (long)event->reason);
        restart_scan_after_connection_failure(BLE_CONNECT_FAILED);
        return;
    }
    if (!s_has_binding && !s_pending_binding.active)
    {
        fail_active_selection();
        s_link_connected = true;
        s_connection_handle = event->conn_handle;
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                          BLE_BINDING_EVIDENCE_REJECTED);
        int terminate_result = ble_gap_terminate(event->conn_handle,
                                                 BLE_ERR_REM_USER_CONN_TERM);
        if (terminate_result == BLE_HS_ENOTCONN)
        {
            s_link_connected = false;
            s_connection_handle = UINT16_MAX;
            restart_scan_after_connection_failure(
                BLE_BINDING_EVIDENCE_REJECTED);
        }
        else if (terminate_result != 0 &&
                 recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
        {
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_DISCONNECT_FAILED);
        }
        return;
    }
    s_link_connected = true;
    s_connection_handle = event->conn_handle;
    const bool connection_power_applied =
        configure_connection_tx_power(event->conn_handle,
                                      attempted_with_boost);
    s_active_connection_tx_power_boost =
        attempted_with_boost && connection_power_applied;
    if (attempted_with_boost && !connection_power_applied)
    {
        ble_tx_power_policy_request_boost(&s_tx_power_policy);
    }
    if (s_has_binding)
    {
        /* 每条新物理链路都必须重新取得当前云端支付事实。 */
        invalidate_current_authorization_proof();
        ESP_LOGI(TAG,
                 "BLE 新物理链路已撤销旧授权，将重新查询云端，handle=%u",
                 (unsigned)event->conn_handle);
    }
    if (s_pending_binding.active)
    {
        s_pending_binding.connected = true;
        s_pending_binding.conn_handle = event->conn_handle;
    }
    ESP_LOGI(TAG,
             "BLE 候选已连接，handle=%u，绑定仍等待链路诊断与有效首帧证据",
             (unsigned)event->conn_handle);
    publish_ble_state(s_has_binding
                          ? WATCH_BLE_TRANSACTION_BOUND_READY
                          : WATCH_BLE_TRANSACTION_PENDING_VALIDATION,
                      BLE_OK);
    begin_link_admission();
}

static void handle_disconnect(const ble_private_event_t *event)
{
    if (event == NULL || !s_link_connected ||
        s_connection_handle != event->conn_handle)
    {
        ESP_LOGW(TAG,
                 "BLE 已忽略迟到断链事件，handle=%u，当前=%u",
                 event != NULL ? (unsigned)event->conn_handle : UINT16_MAX,
                 (unsigned)s_connection_handle);
        return;
    }
    const char *disconnect_diagnostic = s_disconnect_diagnostic;
    s_disconnect_diagnostic = BLE_OK;
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    const int connection_power_dbm =
        s_active_connection_tx_power_boost
            ? BLE_TX_POWER_BOOST_DBM
            : BLE_TX_POWER_DEFAULT_DBM;
    const bool poweroff_finalized = finalize_poweroff_offline(now_ms);
    if (s_has_binding && !poweroff_finalized &&
        strcmp(disconnect_diagnostic, BLE_OK) == 0)
    {
        const bool boost_was_pending =
            s_tx_power_policy.boost_next_connection;
        ble_tx_power_policy_note_disconnect(&s_tx_power_policy,
                                            event->reason,
                                            now_ms);
        if (!boost_was_pending &&
            s_tx_power_policy.boost_next_connection)
        {
            ESP_LOGW(TAG,
                     "BLE 10 分钟内已观察到两次异常链路丢失，下一次物理重连将临时使用 +9 dBm");
        }
    }
    if (s_has_binding)
    {
        (void)control_gate_note_disconnect(&s_control_gate,
                                           s_bound_mac,
                                           s_link_generation);
    }
    /* 断链是授权硬边界，中止事务与撤销旧证明必须原子收口。 */
    abort_unlock_transaction(BLE_UNLOCK_LINK_CHANGED, true);
    fail_active_selection();
    clear_pending_binding();
    invalidate_link_generation();
    reset_link_admission();
    s_connecting = false;
    s_link_connected = false;
    s_connection_handle = UINT16_MAX;
    s_active_connection_tx_power_boost = false;
    s_restart_scan_after_clear = false;
    publish_ble_state(s_has_binding
                          ? WATCH_BLE_TRANSACTION_BOUND_READY
                          : WATCH_BLE_TRANSACTION_UNBOUND,
                      strcmp(disconnect_diagnostic, BLE_OK) == 0
                          ? BLE_DISCONNECTED
                          : disconnect_diagnostic);
    ESP_LOGW(TAG,
             "BLE 连接已断开，handle=%u，原因=%ld，本链路功率=%d dBm，未提交绑定已作废",
             (unsigned)event->conn_handle,
             (long)event->reason,
             connection_power_dbm);
    if (s_has_binding)
    {
        schedule_reconnect(strcmp(disconnect_diagnostic, BLE_OK) == 0
                               ? BLE_DISCONNECTED
                               : disconnect_diagnostic);
    }
    else
    {
        (void)try_enter_radio_dormant();
    }
}

static void handle_connection_update(const ble_private_event_t *event)
{
    if (event == NULL || !event_is_current_link(event))
    {
        return;
    }
    const ble_connection_power_profile_t completed_profile =
        s_connection_pending_profile;
    const bool owned_update =
        completed_profile != BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    s_connection_pending_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;

    struct ble_gap_conn_desc descriptor = {0};
    const int descriptor_result =
        ble_gap_conn_find(event->conn_handle, &descriptor);
    if (event->reason == 0 && owned_update)
    {
        s_connection_applied_profile = completed_profile;
        s_connection_profile_failure_count = 0U;
        s_connection_profile_retry_at_ms = 0U;
        s_connection_failed_profile = BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    }
    if (descriptor_result == 0)
    {
        ESP_LOGI(TAG,
                 "BLE 连接参数更新终态，状态=%ld，档位=%d，实际间隔=%u，延迟=%u，监督超时=%u",
                 (long)event->reason,
                 (int)completed_profile,
                 (unsigned)descriptor.conn_itvl,
                 (unsigned)descriptor.conn_latency,
                 (unsigned)descriptor.supervision_timeout);
    }
    else
    {
        ESP_LOGW(TAG,
                 "BLE 连接参数更新终态无法读取实际参数，状态=%ld，档位=%d，错误=%d",
                 (long)event->reason,
                 (int)completed_profile,
                 descriptor_result);
    }
    if (owned_update && event->reason != 0 &&
        s_connection_target_profile == completed_profile)
    {
        schedule_connection_power_profile_retry(completed_profile);
    }
    else if (owned_update &&
             s_connection_target_profile != completed_profile)
    {
        request_connection_power_profile(s_connection_target_profile, true);
    }
}

static void begin_link_admission(void)
{
    reset_link_admission();
    s_disconnect_diagnostic = BLE_OK;
    const uint32_t controller_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA;
    ESP_LOGI(TAG,
             "BLE 链路准入前控制器 DMA 内存：空闲=%lu 字节，最大连续块=%lu 字节",
             (unsigned long)heap_caps_get_free_size(controller_caps),
             (unsigned long)heap_caps_get_largest_free_block(controller_caps));
    if (s_link_generation == 0U)
    {
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
        return;
    }
    atomic_store(&s_callback_link_generation, s_link_generation);
    atomic_store(&s_callback_conn_handle, s_connection_handle);
    s_admission_deadline = xTaskGetTickCount() +
                           pdMS_TO_TICKS(BLE_LINK_ADMISSION_TIMEOUT_MS);
    s_admission_state = BLE_ADMISSION_GATT_DISCOVERING;
    ble_gatt_callback_context_t *context = allocate_gatt_callback_context();
    if (context == NULL ||
        ble_gattc_disc_all_svcs(s_connection_handle,
                                gatt_service_callback,
                                context) != 0)
    {
        release_gatt_callback_context(context);
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
        return;
    }
    ESP_LOGI(TAG,
             "BLE 已启动串行 GATT 准入，handle=%u，代次=%lu",
             (unsigned)s_connection_handle,
             (unsigned long)s_link_generation);
}

static void handle_gatt_service_event(const ble_private_event_t *event)
{
    if (!event_is_current_link(event) ||
        s_admission_state != BLE_ADMISSION_GATT_DISCOVERING)
    {
        return;
    }
    if (event->reason == 0)
    {
        if (event->target_uuid)
        {
            if (s_target_service_count < UINT8_MAX)
            {
                ++s_target_service_count;
            }
            s_service_start_handle = event->start_handle;
            s_service_end_handle = event->end_handle;
        }
        return;
    }
    if (event->reason != BLE_HS_EDONE)
    {
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
        return;
    }
    if (s_target_service_count != 1U)
    {
        fail_current_link(BLE_GATT_SERVICE_NOT_FOUND);
        return;
    }
    s_admission_state = BLE_ADMISSION_CHARACTERISTIC_DISCOVERING;
    ble_gatt_callback_context_t *context = allocate_gatt_callback_context();
    if (context == NULL ||
        ble_gattc_disc_all_chrs(s_connection_handle,
                                s_service_start_handle,
                                s_service_end_handle,
                                gatt_characteristic_callback,
                                context) != 0)
    {
        release_gatt_callback_context(context);
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
    }
}

static void handle_gatt_characteristic_event(const ble_private_event_t *event)
{
    if (!event_is_current_link(event) ||
        s_admission_state != BLE_ADMISSION_CHARACTERISTIC_DISCOVERING)
    {
        return;
    }
    if (event->reason == 0)
    {
        if (event->target_uuid)
        {
            if (s_target_characteristic_count < UINT8_MAX)
            {
                ++s_target_characteristic_count;
            }
            s_characteristic_def_handle = event->def_handle;
            s_characteristic_val_handle = event->val_handle;
            s_characteristic_properties = event->properties;
        }
        else if (s_target_characteristic_count > 0U &&
                 event->def_handle > s_characteristic_def_handle &&
                 (s_next_characteristic_def_handle == 0U ||
                  event->def_handle < s_next_characteristic_def_handle))
        {
            s_next_characteristic_def_handle = event->def_handle;
        }
        return;
    }
    if (event->reason != BLE_HS_EDONE)
    {
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
        return;
    }
    const uint8_t required_properties = BLE_GATT_CHR_PROP_NOTIFY |
                                        BLE_GATT_CHR_PROP_WRITE;
    if (s_target_characteristic_count != 1U ||
        (s_characteristic_properties & required_properties) != required_properties)
    {
        fail_current_link(BLE_GATT_CHARACTERISTIC_INCOMPATIBLE);
        return;
    }
    const uint16_t descriptor_end = s_next_characteristic_def_handle > 0U
                                        ? s_next_characteristic_def_handle - 1U
                                        : s_service_end_handle;
    if (s_characteristic_val_handle == UINT16_MAX ||
        s_characteristic_val_handle + 1U > descriptor_end)
    {
        fail_current_link(BLE_GATT_CCCD_NOT_FOUND);
        return;
    }
    s_admission_state = BLE_ADMISSION_CCCD_DISCOVERING;
    ble_gatt_callback_context_t *context = allocate_gatt_callback_context();
    if (context == NULL ||
        ble_gattc_disc_all_dscs(s_connection_handle,
                                s_characteristic_val_handle,
                                descriptor_end,
                                gatt_descriptor_callback,
                                context) != 0)
    {
        release_gatt_callback_context(context);
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
    }
}

static void handle_gatt_descriptor_event(const ble_private_event_t *event)
{
    if (!event_is_current_link(event) ||
        s_admission_state != BLE_ADMISSION_CCCD_DISCOVERING)
    {
        return;
    }
    if (event->reason == 0)
    {
        if (event->target_uuid)
        {
            if (s_target_cccd_count < UINT8_MAX)
            {
                ++s_target_cccd_count;
            }
            s_cccd_handle = event->start_handle;
        }
        return;
    }
    if (event->reason != BLE_HS_EDONE)
    {
        fail_current_link(BLE_GATT_DISCOVERY_FAILED);
        return;
    }
    if (s_target_cccd_count != 1U)
    {
        fail_current_link(BLE_GATT_CCCD_NOT_FOUND);
        return;
    }
    s_gatt_ready = true;
    begin_mtu_negotiation();
}

static void begin_mtu_negotiation(void)
{
    s_admission_state = BLE_ADMISSION_MTU_NEGOTIATING;
    const bool peer_exchange_observed = s_observed_att_mtu > 0U;
    uint16_t actual_mtu = s_observed_att_mtu;
    const uint16_t queried_mtu = ble_att_mtu(s_connection_handle);
    if (queried_mtu > actual_mtu)
    {
        actual_mtu = queried_mtu;
    }
    if (peer_exchange_observed || actual_mtu > BLE_ATT_MTU_DFLT)
    {
        const ble_private_event_t event = {
            .type = BLE_PRIVATE_EVENT_MTU_RESULT,
            .conn_handle = s_connection_handle,
            .mtu = actual_mtu,
            .link_generation = s_link_generation,
        };
        handle_mtu_result(&event);
        return;
    }
    ble_gatt_callback_context_t *context = allocate_gatt_callback_context();
    const int exchange_result =
        context != NULL
            ? ble_gattc_exchange_mtu(s_connection_handle,
                                     gatt_mtu_callback,
                                     context)
            : BLE_HS_ENOMEM;
    if (exchange_result != 0)
    {
        release_gatt_callback_context(context);
        actual_mtu = ble_att_mtu(s_connection_handle);
        if (actual_mtu < BLE_ATT_MTU_DFLT)
        {
            actual_mtu = BLE_ATT_MTU_DFLT;
        }
        const ble_private_event_t event = {
            .type = BLE_PRIVATE_EVENT_MTU_RESULT,
            .reason = exchange_result,
            .conn_handle = s_connection_handle,
            .link_generation = s_link_generation,
            .mtu = actual_mtu,
        };
        handle_mtu_result(&event);
    }
}

static void handle_mtu_result(const ble_private_event_t *event)
{
    if (!event_is_current_link(event))
    {
        return;
    }
    if (event->reason == 0 && event->mtu > 0U &&
        s_admission_state != BLE_ADMISSION_MTU_NEGOTIATING)
    {
        s_observed_att_mtu = event->mtu;
        return;
    }
    if (s_admission_state != BLE_ADMISSION_MTU_NEGOTIATING)
    {
        return;
    }
    uint16_t actual_mtu = event->mtu;
    if (event->reason != 0 || actual_mtu == 0U)
    {
        actual_mtu = ble_att_mtu(s_connection_handle);
    }
    if (actual_mtu < BLE_ATT_MTU_DFLT)
    {
        actual_mtu = BLE_ATT_MTU_DFLT;
    }
    if (event->reason != 0)
    {
        ESP_LOGW(TAG,
                 "BLE MTU 协商失败，回退当前 ATT MTU=%u，稳定码=%s，错误=%ld",
                 (unsigned)actual_mtu,
                 BLE_MTU_EXCHANGE_FAILED,
                 (long)event->reason);
    }
    s_mtu_ready = true;
    s_negotiated_mtu = actual_mtu;
    publish_ble_state(s_transaction,
                      event->reason == 0 ? BLE_OK
                                         : BLE_MTU_EXCHANGE_FAILED);
    const char *evidence_mac = s_pending_binding.active
                                   ? s_pending_binding.candidate.mac
                                   : s_bound_mac;
    ble_private_event_t evidence = {
        .type = BLE_PRIVATE_EVENT_MTU_EVIDENCE,
        .conn_handle = s_connection_handle,
        .link_generation = s_link_generation,
        .mtu = actual_mtu,
    };
    memcpy(evidence.mac, evidence_mac, sizeof(evidence.mac));
    handle_mtu_evidence(&evidence);
    if (!s_link_connected || s_connection_handle == UINT16_MAX ||
        s_admission_state != BLE_ADMISSION_MTU_NEGOTIATING)
    {
        return;
    }
    s_admission_state = BLE_ADMISSION_NOTIFY_ENABLING;
    begin_notify_attempt();
}

static void begin_notify_attempt(void)
{
    static const uint8_t cccd_payload[] = {0x01, 0x00};
    while (s_admission_state == BLE_ADMISSION_NOTIFY_ENABLING &&
           s_notify_attempt_count < BLE_NOTIFY_ENABLE_MAX_ATTEMPTS)
    {
        ++s_notify_attempt_count;
        ble_gatt_callback_context_t *context = allocate_gatt_callback_context();
        if (context != NULL &&
            ble_gattc_write_flat(s_connection_handle,
                                 s_cccd_handle,
                                 cccd_payload,
                                 sizeof(cccd_payload),
                                 gatt_notify_write_callback,
                                 context) == 0)
        {
            return;
        }
        release_gatt_callback_context(context);
    }
    fail_current_link(BLE_NOTIFY_ENABLE_FAILED);
}

static void handle_notify_write_result(const ble_private_event_t *event)
{
    if (!event_is_current_link(event) ||
        s_admission_state != BLE_ADMISSION_NOTIFY_ENABLING)
    {
        return;
    }
    if (event->reason != 0)
    {
        if (s_notify_attempt_count >= BLE_NOTIFY_ENABLE_MAX_ATTEMPTS)
        {
            fail_current_link(BLE_NOTIFY_ENABLE_FAILED);
        }
        else
        {
            begin_notify_attempt();
        }
        return;
    }
    s_notify_ready = true;
    s_admission_state = BLE_ADMISSION_NOTIFY_READY;
    s_admission_deadline = 0;
    atomic_store(&s_callback_value_handle, s_characteristic_val_handle);
    atomic_store(&s_callback_notify_ready, true);
    publish_ble_state(s_transaction, BLE_OK);
    ESP_LOGI(TAG,
             "BLE Notify 已开启，handle=%u，MTU=%u，控制仍保持关闭",
             (unsigned)s_connection_handle,
             (unsigned)s_negotiated_mtu);
}

static void handle_notify_rx_event(const ble_private_event_t *event)
{
    if (!event_is_current_link(event) || !s_notify_ready ||
        event->val_handle != s_characteristic_val_handle ||
        event->payload_length > BLE_NOTIFY_PAYLOAD_CAPACITY)
    {
        return;
    }
    ble_link_token_t link_token = {
        .conn_handle = event->conn_handle,
        .link_generation = event->link_generation,
    };
    const char *link_mac = s_pending_binding.active
                               ? s_pending_binding.candidate.mac
                               : s_bound_mac;
    memcpy(link_token.mac, link_mac, sizeof(link_token.mac));
    consume_status_bytes(event->payload,
                         event->payload_length,
                         event->received_at_ms,
                         &link_token);
}

static void consume_status_bytes(const uint8_t *data,
                                 size_t length,
                                 uint64_t received_at_ms,
                                 const ble_link_token_t *link_token)
{
    if (data == NULL || link_token == NULL)
    {
        return;
    }
    uint32_t published_discarded_count =
        s_status_owner.diagnostics.discarded_byte_count;
    for (size_t index = 0; index < length; ++index)
    {
        ble_status_owner_outcome_t outcome = {0};
        if (!ble_status_owner_consume_byte(&s_status_owner,
                                           data[index],
                                           link_token,
                                           received_at_ms,
                                           &outcome))
        {
            reset_status_stream();
            return;
        }
        if (outcome.frame_completed && !outcome.frame_valid)
        {
            const uint32_t now_ms = monotonic_ms32();
            if (!s_invalid_frame_diagnostic_logged ||
                (uint32_t)(now_ms - s_last_invalid_frame_diagnostic_ms) >=
                    BLE_INVALID_FRAME_DIAGNOSTIC_INTERVAL_MS)
            {
                ESP_LOGW(
                    TAG,
                    "BLE 完整状态帧校验失败，稳定码=%s，原因=%s，Notify片段长度=%u，累计=%lu",
                    BLE_PROTOCOL_FRAME_INVALID,
                    ble_status_decode_error_name(outcome.decode_error),
                    (unsigned)length,
                    (unsigned long)s_status_owner.diagnostics.invalid_frame_count);
                publish_ble_state(s_transaction, BLE_PROTOCOL_FRAME_INVALID);
                published_discarded_count =
                    s_status_owner.diagnostics.discarded_byte_count;
                s_last_diagnostic_publish_ms = now_ms;
                s_last_invalid_frame_diagnostic_ms = now_ms;
                s_invalid_frame_diagnostic_logged = true;
                s_diagnostic_publish_dirty = false;
            }
        }
        if (!outcome.frame_valid)
        {
            continue;
        }
        ++s_valid_status_sequence;
        if (outcome.first_valid_evidence)
        {
            const ble_private_event_t evidence = {
                .type = BLE_PRIVATE_EVENT_FRAME_EVIDENCE,
                .conn_handle = link_token->conn_handle,
                .link_generation = link_token->link_generation,
                .fully_valid = true,
            };
            ble_private_event_t bound_evidence = evidence;
            memcpy(bound_evidence.mac, link_token->mac, sizeof(bound_evidence.mac));
            handle_frame_evidence(&bound_evidence);
        }
        if (!link_token_is_current(link_token))
        {
            reset_status_stream();
            return;
        }
        ble_status_freshness_state_t freshness = {
            .data_ready = s_data_ready,
            .status_fresh = s_status_fresh,
            .link_control_ready = s_link_control_ready,
        };
        const bool transport_ready =
            s_link_connected && s_gatt_ready && s_notify_ready;
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
        if (!ble_status_freshness_accept_frame(
                true,
                s_status_owner.mirror_valid,
                transport_ready,
                s_status_owner.last_status_rx_ms,
                now_ms,
                STATUS_STALE_MS,
                &freshness))
        {
            freshness = (ble_status_freshness_state_t){0};
        }
        s_data_ready = freshness.data_ready;
        s_status_fresh = freshness.status_fresh;
        s_link_control_ready = freshness.link_control_ready;
        if (outcome.first_valid_evidence && s_link_control_ready)
        {
            request_current_connection_power_profile(true);
        }
        if (outcome.first_valid_evidence)
        {
            ESP_LOGI(TAG,
                     "BLE 首帧有效状态数据：模式=%.1f 步频=%.1f 左档=%.1f 右档=%.1f 助力=%u 电量=%u%% 场景=%u 配置=%u",
                     (double)s_status_owner.mirror.current_mode,
                     (double)s_status_owner.mirror.step_frequency,
                     (double)s_status_owner.mirror.left_gear,
                     (double)s_status_owner.mirror.right_gear,
                     (unsigned)s_status_owner.mirror.motor_enable,
                     (unsigned)s_status_owner.mirror.battery_level,
                     (unsigned)s_status_owner.mirror.scene_mode,
                     (unsigned)s_status_owner.mirror.scene_config);
        }
        if (outcome.first_valid_evidence && s_link_control_ready &&
            s_has_binding && strcmp(link_token->mac, s_bound_mac) == 0)
        {
            mark_recovery_success();
        }
        publish_ble_state(s_transaction, BLE_OK);
        if (ble_authorization_should_schedule(
                s_link_control_ready,
                s_has_binding,
                strcmp(link_token->mac, s_bound_mac) == 0,
                s_link_generation,
                s_automatic_authorization_attempt_generation))
        {
            s_automatic_authorization_pending = true;
        }
        handle_unlock_status_evidence(link_token);
        if (!s_pending_state_update_dirty)
        {
            handle_control_status_evidence(link_token);
        }
        published_discarded_count =
            s_status_owner.diagnostics.discarded_byte_count;
        s_last_diagnostic_publish_ms = monotonic_ms32();
        s_diagnostic_publish_dirty = false;
    }
    if (published_discarded_count !=
        s_status_owner.diagnostics.discarded_byte_count)
    {
        s_diagnostic_publish_dirty = true;
    }
}

static bool link_token_is_current(const ble_link_token_t *link_token)
{
    if (link_token == NULL || !s_link_connected || !s_notify_ready ||
        s_admission_state != BLE_ADMISSION_NOTIFY_READY ||
        link_token->conn_handle != s_connection_handle ||
        link_token->link_generation != s_link_generation)
    {
        return false;
    }
    const char *current_mac = s_pending_binding.active
                                  ? s_pending_binding.candidate.mac
                                  : s_bound_mac;
    return strcmp(link_token->mac, current_mac) == 0;
}

static void reset_status_stream(void)
{
    ble_status_owner_reset_link(&s_status_owner);
}

static void clear_status_mirror(void)
{
    ble_status_owner_clear_mirror(&s_status_owner);
    s_data_ready = false;
    s_status_fresh = false;
    s_link_control_ready = false;
    s_connection_target_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    s_connection_pending_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    s_connection_applied_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    s_connection_profile_retry_at_ms = 0U;
    s_connection_profile_failure_count = 0U;
    s_connection_failed_profile = BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
}

static void check_status_freshness(void)
{
    if (!s_status_fresh)
    {
        return;
    }
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    const bool current_link_valid = s_status_owner.mirror_valid &&
                                    link_token_is_current(&s_status_owner.mirror_token);
    ble_status_freshness_state_t freshness = {
        .data_ready = s_data_ready,
        .status_fresh = s_status_fresh,
        .link_control_ready = s_link_control_ready,
    };
    if (!ble_status_freshness_expire(current_link_valid,
                                     s_status_owner.mirror_valid,
                                     now_ms,
                                     s_status_owner.last_status_rx_ms,
                                     STATUS_STALE_MS,
                                     &freshness))
    {
        return;
    }
    s_data_ready = freshness.data_ready;
    s_status_fresh = freshness.status_fresh;
    s_link_control_ready = freshness.link_control_ready;
    if (!finalize_poweroff_offline(now_ms))
    {
        abort_unlock_transaction(BLE_STATUS_STALE, false);
    }
    ESP_LOGW(TAG, "BLE 状态已过期，稳定码=%s", BLE_STATUS_STALE);
    publish_ble_state(s_transaction, BLE_STATUS_STALE);
}

static uint32_t monotonic_ms32(void)
{
    return (uint32_t)((uint64_t)esp_timer_get_time() / 1000U);
}

static bool connection_control_transaction_active(void)
{
    return s_unlock_context.active || s_control_write_callback_pending ||
           s_control_gate.control_pending || s_deferred_poweroff_valid;
}

static void request_connection_power_profile(
    ble_connection_power_profile_t profile,
    bool force)
{
    if (!s_link_connected || !s_notify_ready || !s_link_control_ready ||
        s_connection_handle == UINT16_MAX ||
        profile == BLE_CONNECTION_POWER_PROFILE_UNKNOWN)
    {
        return;
    }
    if (profile == s_connection_failed_profile)
    {
        return;
    }
    if (profile != s_connection_target_profile)
    {
        s_connection_target_profile = profile;
        s_connection_profile_failure_count = 0U;
        s_connection_profile_retry_at_ms = 0U;
        s_connection_failed_profile = BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    }
    if (!force &&
        (profile == s_connection_pending_profile ||
         profile == s_connection_applied_profile ||
         s_connection_profile_retry_at_ms != 0U))
    {
        return;
    }
    if (s_connection_pending_profile !=
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN)
    {
        return;
    }

    const ble_connection_parameter_policy_t policy =
        ble_connection_parameter_policy(profile);
    const struct ble_gap_upd_params parameters = {
        .itvl_min = policy.interval_min_units,
        .itvl_max = policy.interval_max_units,
        .latency = policy.latency,
        .supervision_timeout = policy.timeout_units,
        .min_ce_len = 0U,
        .max_ce_len = 0U,
    };
    const int result = ble_gap_update_params(s_connection_handle,
                                             &parameters);
    if (result != 0)
    {
        ESP_LOGW(TAG,
                 "BLE 连接参数更新请求被拒绝，档位=%d，handle=%u，错误=%d，保持当前安全参数",
                 (int)profile,
                 (unsigned)s_connection_handle,
                 result);
        schedule_connection_power_profile_retry(profile);
        return;
    }
    s_connection_pending_profile = profile;
    ESP_LOGI(TAG,
             "BLE 连接参数更新已提交，档位=%d，间隔=%u-%u，延迟=%u，监督超时=%u",
             (int)profile,
             (unsigned)parameters.itvl_min,
             (unsigned)parameters.itvl_max,
             (unsigned)parameters.latency,
             (unsigned)parameters.supervision_timeout);
}

static void schedule_connection_power_profile_retry(
    ble_connection_power_profile_t profile)
{
    if (profile == BLE_CONNECTION_POWER_PROFILE_UNKNOWN ||
        profile != s_connection_target_profile)
    {
        return;
    }
    if (s_connection_profile_failure_count < UINT8_MAX)
    {
        ++s_connection_profile_failure_count;
    }
    if (s_connection_profile_failure_count >=
        BLE_CONNECTION_PROFILE_MAX_ATTEMPTS)
    {
        s_connection_profile_retry_at_ms = 0U;
        s_connection_failed_profile = profile;
        ESP_LOGW(TAG,
                 "BLE 连接参数更新已耗尽有限重试，档位=%d，保持当前安全参数直到档位变化或重连",
                 (int)profile);
        return;
    }
    const uint32_t delay_ms =
        BLE_CONNECTION_PROFILE_RETRY_BASE_MS <<
        (s_connection_profile_failure_count - 1U);
    s_connection_profile_retry_at_ms =
        (uint64_t)monotonic_ms32() + delay_ms;
}

static void check_connection_power_profile_retry(void)
{
    if (s_connection_profile_retry_at_ms == 0U ||
        (uint64_t)monotonic_ms32() < s_connection_profile_retry_at_ms)
    {
        return;
    }
    s_connection_profile_retry_at_ms = 0U;
    request_connection_power_profile(s_connection_target_profile, true);
}

static void request_current_connection_power_profile(bool force)
{
    const ble_connection_power_profile_t profile =
        ble_connection_power_profile_resolve(
            s_screen_state_known ? s_screen_on : false,
            connection_control_transaction_active());
    request_connection_power_profile(profile, force);
}

static bool screen_sequence_is_after(uint32_t candidate,
                                     uint32_t baseline)
{
    return candidate != 0U &&
           (baseline == 0U || (int32_t)(candidate - baseline) > 0);
}

static void apply_pending_screen_state(void)
{
    uint32_t sequence = 0U;
    uint32_t confirmed_sequence = 0U;
    bool screen_on = false;
    do
    {
        sequence =
            (uint32_t)atomic_load(&s_pending_screen_sequence);
        screen_on = atomic_load(&s_pending_screen_on);
        confirmed_sequence =
            (uint32_t)atomic_load(&s_pending_screen_sequence);
    } while (sequence != confirmed_sequence);
    if (!screen_sequence_is_after(sequence,
                                  s_applied_screen_sequence))
    {
        return;
    }
    s_applied_screen_sequence = sequence;
    apply_screen_power_policy(screen_on);
}

static void apply_screen_power_policy(bool screen_on)
{
    const bool changed = !s_screen_state_known || s_screen_on != screen_on;
    s_screen_state_known = true;
    s_screen_on = screen_on;

    if (!screen_on)
    {
        suspend_disconnected_discovery_for_screen_off();
        (void)try_enter_radio_dormant();
    }
    else if (changed && s_has_binding && !s_scanning && !s_connecting &&
             !s_link_connected && !s_discovery_suspended_for_selftest &&
             s_runtime_product_state.selftest.state != SELFTEST_RUN_RUNNING)
    {
        schedule_reconnect(BLE_DISCONNECTED);
    }
    else if (changed && !s_has_binding &&
             s_binding_scan_session_active && !s_scanning && !s_connecting &&
             !s_link_connected && !s_pending_binding.active &&
             !s_discovery_suspended_for_selftest &&
             s_runtime_product_state.selftest.state != SELFTEST_RUN_RUNNING)
    {
        const ble_radio_lifecycle_t lifecycle =
            (ble_radio_lifecycle_t)atomic_load(&s_radio_lifecycle);
        esp_err_t start_error = ESP_OK;
        if (lifecycle == BLE_RADIO_LIFECYCLE_DORMANT)
        {
            start_error = start_nimble();
        }
        else if (lifecycle == BLE_RADIO_LIFECYCLE_READY &&
                 s_candidate_count == 0U)
        {
            start_error = start_scan_session();
        }
        if (start_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "BLE 亮屏后候选扫描启动失败，稳定码=%s",
                     BLE_SCAN_FAILED);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_SCAN_FAILED);
        }
    }
    request_current_connection_power_profile(false);
}

static void apply_pending_binding_scan_request(void)
{
    const int request = atomic_exchange(
        &s_binding_scan_request,
        BLE_BINDING_SCAN_REQUEST_NONE);
    if (request == BLE_BINDING_SCAN_REQUEST_BEGIN)
    {
        handle_begin_binding_scan_session();
    }
    else if (request == BLE_BINDING_SCAN_REQUEST_END)
    {
        handle_end_binding_scan_session();
    }
}

static TickType_t owner_event_wait_ticks(void)
{
    TickType_t wait_ticks = portMAX_DELAY;
    const TickType_t now_ticks = xTaskGetTickCount();
    const uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000U;
    if (atomic_load(&s_radio_lifecycle) ==
            BLE_RADIO_LIFECYCLE_STARTING &&
        s_radio_start_deadline_ms != 0U)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_radio_start_deadline_ms, now_ms));
    }
    if (s_admission_deadline != 0U &&
        s_admission_state != BLE_ADMISSION_IDLE &&
        s_admission_state != BLE_ADMISSION_NOTIFY_READY)
    {
        const TickType_t admission_wait =
            (int32_t)(s_admission_deadline - now_ticks) <= 0
                ? 1U
                : s_admission_deadline - now_ticks;
        wait_ticks = min_wait_ticks(wait_ticks, admission_wait);
    }
    if (s_status_fresh && s_status_owner.mirror_valid)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(
                s_status_owner.last_status_rx_ms + STATUS_STALE_MS,
                now_ms));
    }
    if (s_automatic_authorization_pending &&
        s_automatic_authorization_retry_at_ms != 0U)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_automatic_authorization_retry_at_ms,
                                now_ms));
    }
    if (s_unlock_context.active)
    {
        const uint64_t unlock_deadline =
            (s_unlock_context.callback_succeeded
                 ? s_unlock_context.callback_success_ms +
                       BLE_UNLOCK_CONFIRM_TIMEOUT_MS
                 : s_unlock_context.write_started_ms +
                       BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT_MS);
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(unlock_deadline, now_ms));
    }
    if (s_control_write_callback_pending)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_control_write_started_ms +
                                    BLE_CONTROL_WRITE_CALLBACK_TIMEOUT_MS,
                                now_ms));
    }
    if (s_control_gate.control_ack_deadline_ms != 0U)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_control_gate.control_ack_deadline_ms,
                                now_ms));
    }
    if (s_control_gate.poweroff_deadline_ms != 0U)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_control_gate.poweroff_deadline_ms,
                                now_ms));
    }
    if (s_connection_profile_retry_at_ms != 0U)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_connection_profile_retry_at_ms,
                                now_ms));
    }
#if SCENIC_AREA_MANAGEMENT_DEBUG
    if (s_cloud_rental_submission_pending)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(s_cloud_rental_submission_retry_at_ms,
                                now_ms));
    }
#endif
    if (s_recovery_policy.pending)
    {
        const int32_t remaining =
            (int32_t)(s_recovery_policy.deadline_ms -
                      (uint32_t)now_ms);
        wait_ticks = min_wait_ticks(
            wait_ticks,
            pdMS_TO_TICKS(remaining <= 0 ? 1U
                                         : (uint32_t)remaining));
    }
    if (s_diagnostic_publish_dirty)
    {
        wait_ticks = min_wait_ticks(
            wait_ticks,
            wait_ticks_until_ms(
                (uint64_t)s_last_diagnostic_publish_ms +
                    BLE_DIAGNOSTIC_PUBLISH_INTERVAL_MS,
                now_ms));
    }
    if (s_pending_state_update_dirty || s_pending_control_update_dirty ||
        s_pending_unlock_result_count > 0U ||
        s_pending_control_result_count > 0U ||
        s_candidate_snapshot_dirty ||
        s_candidate_session_clear_dirty ||
        s_pending_selection_feedback_dirty ||
        s_deferred_poweroff_valid ||
        s_selftest_snapshot_retry_pending)
    {
        wait_ticks = min_wait_ticks(wait_ticks,
                                    pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    }
    return wait_ticks;
}

static TickType_t min_wait_ticks(TickType_t current,
                                 TickType_t candidate)
{
    const TickType_t normalized = candidate == 0U ? 1U : candidate;
    return current == portMAX_DELAY || normalized < current
               ? normalized
               : current;
}

static TickType_t wait_ticks_until_ms(uint64_t deadline_ms,
                                      uint64_t now_ms)
{
    if (deadline_ms <= now_ms)
    {
        return 1U;
    }
    const uint64_t remaining_ms = deadline_ms - now_ms;
    if (remaining_ms >= UINT32_MAX)
    {
        return pdMS_TO_TICKS(UINT32_MAX);
    }
    const TickType_t ticks = pdMS_TO_TICKS((uint32_t)remaining_ms);
    return ticks == 0U ? 1U : ticks;
}

static void check_radio_start_deadline(void)
{
    if (atomic_load(&s_radio_lifecycle) !=
            BLE_RADIO_LIFECYCLE_STARTING ||
        s_radio_start_deadline_ms == 0U)
    {
        return;
    }
    const uint64_t now_ms = (uint64_t)esp_timer_get_time() / 1000U;
    if (now_ms < s_radio_start_deadline_ms)
    {
        return;
    }
    s_radio_start_deadline_ms = 0U;
    ESP_LOGE(TAG,
             "NimBLE host 同步超时，稳定码=%s，代次=%lu",
             BLE_HOST_SYNC_TIMEOUT,
             (unsigned long)s_radio_generation);
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                      BLE_HOST_SYNC_TIMEOUT);
    const bool bound_start = s_has_binding;
    const esp_err_t stop_error = stop_nimble();
    if (bound_start && stop_error == ESP_OK)
    {
        /* 已绑定冷启动失败必须让服务门禁感知；未绑定保留显式重试入口。 */
        ble_nimble_cleanup_record_error(&s_nimble_cleanup,
                                        ESP_ERR_TIMEOUT);
    }
}

static void update_selftest_discovery_suspension(void)
{
    const bool state_known =
        watch_state_snapshot(&s_runtime_product_state,
                             pdMS_TO_TICKS(BLE_EVENT_POLL_MS)) == ESP_OK;
    s_selftest_snapshot_retry_pending = !state_known;
    /* 状态锁在整个有界窗口内仍不可读时先暂停 discovery，避免误启动。 */
    const bool should_suspend =
        !state_known ||
        s_runtime_product_state.selftest.state == SELFTEST_RUN_RUNNING;
    const ble_discovery_pause_transition_t transition =
        ble_discovery_pause_transition(
            s_discovery_suspended_for_selftest,
            should_suspend);
    if (transition == BLE_DISCOVERY_PAUSE_NO_CHANGE)
    {
        return;
    }

    s_discovery_suspended_for_selftest =
        transition == BLE_DISCOVERY_PAUSE_ENTER;
    if (transition == BLE_DISCOVERY_PAUSE_ENTER)
    {
        ble_recovery_policy_cancel(&s_recovery_policy);
        s_recovery_in_progress = false;
        s_recovery_target_seen = false;
        s_candidate_snapshot_dirty = false;
        s_pending_selection_feedback_dirty = false;
        s_active_selection_pending = false;
        /* 进入自检即作废旧候选，即使扫描窗口已经自然结束。 */
        invalidate_scan_generation();
        clear_candidate_session_model();
        if (s_scanning && !s_binding_scan_cancel_pending)
        {
            const int cancel_result = ble_gap_disc_cancel();
            if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
            {
                s_selftest_scan_cancel_pending = true;
                ESP_LOGW(TAG,
                         "自检暂停 BLE 扫描时取消返回异常，错误=%d",
                         cancel_result);
            }
            else
            {
                s_scanning = false;
            }
        }
        if (s_connecting)
        {
            const int cancel_result = ble_gap_conn_cancel();
            if (cancel_result == BLE_HS_EALREADY)
            {
                s_selftest_connection_cancel_pending = false;
                s_connecting = false;
                clear_pending_binding();
            }
            else
            {
                s_selftest_connection_cancel_pending = true;
                if (cancel_result != 0)
                {
                    ESP_LOGW(TAG,
                             "自检暂停 BLE 连接时取消返回异常，错误=%d",
                             cancel_result);
                }
            }
        }
        if (state_known)
        {
            ESP_LOGI(TAG,
                     "自检运行期间已暂停 BLE discovery 与断线重连，现有连接保持");
        }
        else
        {
            ESP_LOGW(TAG,
                     "自检状态暂不可读，已安全暂停 BLE discovery 与断线重连");
        }
        (void)try_enter_radio_dormant();
        return;
    }

    ESP_LOGI(TAG, "自检已结束，恢复 BLE discovery 与断线重连");
    if (s_scanning || s_connecting || s_link_connected ||
        atomic_load(&s_stop_requested))
    {
        return;
    }
    if (s_has_binding)
    {
        schedule_reconnect(BLE_DISCONNECTED);
    }
    else if (s_binding_scan_session_active)
    {
        const ble_radio_lifecycle_t lifecycle =
            (ble_radio_lifecycle_t)atomic_load(&s_radio_lifecycle);
        const esp_err_t resume_error =
            lifecycle == BLE_RADIO_LIFECYCLE_DORMANT
                ? start_nimble()
                : (lifecycle == BLE_RADIO_LIFECYCLE_READY
                       ? start_scan_session()
                       : ESP_ERR_INVALID_STATE);
        if (resume_error != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "自检结束后 BLE 候选扫描恢复失败，稳定码=%s",
                     BLE_SCAN_FAILED);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_SCAN_FAILED);
        }
    }
}

static void invalidate_scan_generation(void)
{
    ++s_scan_generation;
    if (s_scan_generation == 0U)
    {
        ++s_scan_generation;
    }
}

static void schedule_reconnect(const char *error_code)
{
    if (!s_has_binding || atomic_load(&s_stop_requested) ||
        s_recovery_policy.pending || s_discovery_suspended_for_selftest)
    {
        return;
    }
    bool screen_on = false;
    if (!snapshot_screen_on(&screen_on))
    {
        screen_on = false;
        s_screen_state_known = false;
    }
    publish_ble_state(WATCH_BLE_TRANSACTION_BOUND_READY,
                      error_code != NULL ? error_code : BLE_DISCONNECTED);
    if (!screen_on)
    {
        ble_recovery_policy_cancel(&s_recovery_policy);
        s_recovery_in_progress = false;
        s_recovery_target_seen = false;
        ESP_LOGI(TAG,
                 "BLE 已绑定断连但屏幕熄灭，不调度扫描或重连 deadline");
        return;
    }
    ++s_recovery_generation;
    if (s_recovery_generation == 0U)
    {
        ++s_recovery_generation;
    }
    s_recovery_in_progress = false;
    s_recovery_target_seen = false;
    ble_recovery_policy_schedule_for_screen(&s_recovery_policy,
                                            monotonic_ms32(),
                                            s_recovery_generation,
                                            screen_on);
    ESP_LOGW(TAG,
             "BLE 亮屏自动恢复已调度，代次=%lu，deadline=%lu ms",
             (unsigned long)s_recovery_generation,
             (unsigned long)s_recovery_policy.deadline_ms);
}

static void check_reconnect_deadline(void)
{
    if (s_discovery_suspended_for_selftest)
    {
        return;
    }
    bool screen_on = false;
    if (!snapshot_screen_on(&screen_on))
    {
        return;
    }
    if (!screen_on)
    {
        ble_recovery_policy_cancel(&s_recovery_policy);
        s_recovery_in_progress = false;
        s_recovery_target_seen = false;
        return;
    }
    (void)ble_recovery_policy_retarget_screen(&s_recovery_policy,
                                              monotonic_ms32(),
                                              screen_on);
    if (!ble_recovery_policy_take_due(&s_recovery_policy,
                                      monotonic_ms32(),
                                      s_recovery_generation))
    {
        return;
    }
    if (!s_has_binding || atomic_load(&s_stop_requested) || s_scanning ||
        s_connecting || s_link_connected)
    {
        return;
    }
    s_recovery_in_progress = true;
    s_recovery_target_seen = false;
    s_reconnect_attempt_count =
        ble_recovery_saturating_increment(s_reconnect_attempt_count);
    ESP_LOGI(TAG,
             "BLE 自动恢复尝试启动，累计=%lu，仅匹配绑定 MAC=%s",
             (unsigned long)s_reconnect_attempt_count,
             s_bound_mac);
    if (start_scan_session() != ESP_OK)
    {
        ESP_LOGE(TAG, "BLE 自动恢复扫描启动失败，稳定码=%s", BLE_SCAN_FAILED);
        schedule_reconnect(BLE_SCAN_FAILED);
    }
}

static void mark_recovery_success(void)
{
    if (!s_recovery_in_progress)
    {
        return;
    }
    s_recovery_in_progress = false;
    s_reconnect_success_count =
        ble_recovery_saturating_increment(s_reconnect_success_count);
    ble_recovery_policy_reset(&s_recovery_policy);
    ESP_LOGI(TAG,
             "BLE 同身份端到端恢复成功，退避已复位，累计成功=%lu",
             (unsigned long)s_reconnect_success_count);
}

static void try_release_control_lockout(void)
{
    if (!s_control_gate.control_locked_out || !s_link_control_ready ||
        !s_status_fresh || !s_has_binding || s_link_generation == 0U)
    {
        return;
    }
    memset(&s_runtime_product_state, 0, sizeof(s_runtime_product_state));
    if (watch_state_snapshot(&s_runtime_product_state, 0) != ESP_OK)
    {
        return;
    }
    control_gate_t candidate_gate = s_control_gate;
    watch_control_update_t update = {0};
    if (!control_gate_release_lockout_after_reconnect(&candidate_gate,
                                                      &s_runtime_product_state,
                                                      s_link_generation,
                                                      &update) ||
        !publish_control_update_confirmed(&update))
    {
        return;
    }
    s_control_gate = candidate_gate;
    ESP_LOGI(TAG,
             "BLE 同身份新链路已解除普通控制锁止，代次=%lu",
             (unsigned long)s_link_generation);
}

static bool handle_first_unlock_intent(uint32_t intent_sequence)
{
    ESP_LOGI(TAG, "BLE owner 收到首次授权请求");
    const ble_unlock_request_t entry_request = {
        .intent_sequence = intent_sequence,
        .link_generation = s_link_generation,
    };
    ble_unlock_request_t bound_entry_request = entry_request;
    memcpy(bound_entry_request.exoskeleton_mac,
           s_bound_mac,
           sizeof(bound_entry_request.exoskeleton_mac));
    if (s_unlock_context.active || s_control_write_callback_pending)
    {
        ESP_LOGW(TAG, "BLE 首次授权门禁拒绝：已有 GATT procedure 处理中");
        publish_unlock_result(&bound_entry_request,
                              BLE_UNLOCK_RESULT_BUSY,
                              "BLE_UNLOCK_BUSY");
        return false;
    }
    memset(&s_runtime_product_state, 0, sizeof(s_runtime_product_state));
    control_gate_action_t action = {0};
    watch_control_update_t update = {0};
    if (watch_state_snapshot(&s_runtime_product_state, 0) != ESP_OK)
    {
        publish_unlock_result(&bound_entry_request,
                              BLE_UNLOCK_RESULT_BLOCKED,
                              "BLE_STATE_SNAPSHOT_FAILED");
        return false;
    }
    const control_gate_request_status_t status =
        control_gate_request_first_unlock(&s_control_gate,
                                          &s_runtime_product_state,
                                          s_link_generation,
                                          &action,
                                          &update);
    ESP_LOGI(TAG,
             "BLE 首次授权门禁结果：状态=%d，请求=%lu，链路代次=%lu",
             (int)status,
             (unsigned long)action.request_id,
             (unsigned long)action.link_generation);
    if (status == CONTROL_GATE_ALREADY_UNLOCKED)
    {
        publish_control_update(&update);
        publish_unlock_result(&bound_entry_request,
                              BLE_UNLOCK_RESULT_ALREADY_UNLOCKED,
                              "BLE_UNLOCKED");
        return true;
    }
    if (status == CONTROL_GATE_BUSY)
    {
#if SCENIC_AREA_MANAGEMENT_DEBUG
        const bool payment_session_waiting =
            s_control_gate.pending && s_control_gate.request_id != 0U &&
            s_control_gate.link_generation == s_link_generation &&
            s_has_binding &&
            memcmp(s_control_gate.exoskeleton_mac,
                   s_bound_mac,
                   sizeof(s_bound_mac)) == 0 &&
            (s_control_gate.phase == WATCH_RENTAL_PHASE_WAITING_PAYMENT ||
             s_control_gate.phase == WATCH_RENTAL_PHASE_RETRY_WAIT);
        if (payment_session_waiting)
        {
            bound_entry_request.request_id = s_control_gate.request_id;
            bound_entry_request.link_generation =
                s_control_gate.link_generation;
            memcpy(bound_entry_request.exoskeleton_mac,
                   s_control_gate.exoskeleton_mac,
                   sizeof(bound_entry_request.exoskeleton_mac));
            bool joined_in_flight = false;
            const esp_err_t accelerate_error =
                cloud_service_rental_accelerate(s_control_gate.request_id,
                                                &joined_in_flight);
            if (accelerate_error == ESP_OK)
            {
                /* 仅更新 UI 结果关联，不改变支付会话的 request ID 与链路身份。 */
                s_authorization_intent_sequence = intent_sequence;
                publish_control_update(&update);
                if (intent_sequence != 0U)
                {
                    publish_unlock_result(
                        &bound_entry_request,
                        BLE_UNLOCK_RESULT_PAYMENT_CHECK_ACCEPTED,
                        BLE_PAYMENT_CHECK_ACCEPTED);
                }
                if (joined_in_flight)
                {
                    ESP_LOGI(TAG,
                             "人工支付验证已加入在途真实查询，请求=%lu，链路代次=%lu",
                             (unsigned long)s_control_gate.request_id,
                             (unsigned long)s_control_gate.link_generation);
                }
                else
                {
                    ESP_LOGI(TAG,
                             "当前支付会话已请求一次立即检查，请求=%lu，链路代次=%lu",
                             (unsigned long)s_control_gate.request_id,
                             (unsigned long)s_control_gate.link_generation);
                }
                return true;
            }

            ESP_LOGW(TAG,
                     "当前支付会话立即检查请求未被 cloud owner 接受，请求=%lu，错误=0x%x",
                     (unsigned long)s_control_gate.request_id,
                     (unsigned)accelerate_error);
            publish_control_update(&update);
            publish_unlock_result(&bound_entry_request,
                                  BLE_UNLOCK_RESULT_BUSY,
                                  "CLOUD_ACCELERATE_FAILED");
            return false;
        }
#endif
        publish_control_update(&update);
        publish_unlock_result(&bound_entry_request,
                              BLE_UNLOCK_RESULT_BUSY,
                              update.control_error_code);
        return false;
    }
    if (status != CONTROL_GATE_ACCEPTED ||
        action.type != CONTROL_GATE_ACTION_RENTAL_QUERY)
    {
        publish_control_update(&update);
        publish_unlock_result(&bound_entry_request,
                              BLE_UNLOCK_RESULT_BLOCKED,
                              update.control_error_code);
        return false;
    }
    s_authorization_intent_sequence = intent_sequence;

#if SCENIC_AREA_MANAGEMENT_DEBUG
    const cloud_rental_request_t request = {
        .request_id = action.request_id,
        .link_generation = action.link_generation,
    };
    cloud_rental_request_t bound_request = request;
    memcpy(bound_request.exoskeleton_mac,
           action.exoskeleton_mac,
           sizeof(bound_request.exoskeleton_mac));
    if (!publish_control_update_confirmed(&update))
    {
        control_gate_rental_result_t bound_failure = {
            .request_id = action.request_id,
            .link_generation = action.link_generation,
            .result = WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
        };
        memcpy(bound_failure.exoskeleton_mac,
               action.exoskeleton_mac,
               sizeof(bound_failure.exoskeleton_mac));
        memcpy(bound_failure.error_code,
               "CLOUD_STATE_SYNC_FAILED",
               sizeof("CLOUD_STATE_SYNC_FAILED"));
        control_gate_action_t ignored_action = {0};
        if (control_gate_apply_rental_result(&s_control_gate,
                                             &bound_failure,
                                             &ignored_action,
                                             &update) == ESP_OK)
        {
            publish_control_update(&update);
        }
        const ble_unlock_request_t unlock_request = {
            .intent_sequence = s_authorization_intent_sequence,
            .request_id = action.request_id,
            .link_generation = action.link_generation,
        };
        ble_unlock_request_t bound_unlock_request = unlock_request;
        memcpy(bound_unlock_request.exoskeleton_mac,
               action.exoskeleton_mac,
               sizeof(bound_unlock_request.exoskeleton_mac));
        publish_unlock_result(&bound_unlock_request,
                              BLE_UNLOCK_RESULT_CLOUD_DENIED,
                              "CLOUD_STATE_SYNC_FAILED");
        return true;
    }
    s_cloud_rental_submission = bound_request;
    s_cloud_rental_submission_pending = true;
    s_cloud_rental_submission_attempt_count = 0U;
    s_cloud_rental_submission_retry_at_ms = 0U;
    try_submit_pending_cloud_rental_request();
#else
    ble_unlock_request_t unlock_request = {
        .intent_sequence = s_authorization_intent_sequence,
        .request_id = action.request_id,
        .link_generation = action.link_generation,
    };
    memcpy(unlock_request.exoskeleton_mac,
           action.exoskeleton_mac,
           sizeof(unlock_request.exoskeleton_mac));
    if (!publish_control_update_confirmed(&update))
    {
        watch_control_update_t blocked_update = {0};
        if (control_gate_abort_pending(&s_control_gate, &blocked_update))
        {
            publish_control_update(&blocked_update);
        }
        publish_unlock_result(&unlock_request,
                              BLE_UNLOCK_RESULT_BLOCKED,
                              BLE_CONTROL_STATE_SYNC_FAILED);
        return false;
    }

    const control_gate_rental_result_t local_authorization = {
        .request_id = action.request_id,
        .link_generation = action.link_generation,
        .result = WATCH_RENTAL_RESULT_PAID,
    };
    control_gate_rental_result_t bound_authorization = local_authorization;
    memcpy(bound_authorization.exoskeleton_mac,
           action.exoskeleton_mac,
           sizeof(bound_authorization.exoskeleton_mac));
    control_gate_action_t unlock_action = {0};
    if (control_gate_apply_rental_result(&s_control_gate,
                                         &bound_authorization,
                                         &unlock_action,
                                         &update) != ESP_OK ||
        unlock_action.type != CONTROL_GATE_ACTION_BLE_UNLOCK)
    {
        watch_control_update_t blocked_update = {0};
        if (control_gate_abort_pending(&s_control_gate, &blocked_update))
        {
            publish_control_update(&blocked_update);
        }
        publish_unlock_result(&unlock_request,
                              BLE_UNLOCK_RESULT_BLOCKED,
                              "BLE_LOCAL_AUTH_REJECTED");
        return false;
    }
    if (!publish_control_update_confirmed(&update))
    {
        watch_control_update_t blocked_update = {0};
        if (control_gate_abort_pending(&s_control_gate, &blocked_update))
        {
            publish_control_update(&blocked_update);
        }
        publish_unlock_result(&unlock_request,
                              BLE_UNLOCK_RESULT_BLOCKED,
                              BLE_CONTROL_STATE_SYNC_FAILED);
        return false;
    }
    ESP_LOGI(TAG,
             "非景区模式本地授权完成，开始 BLE 解锁确认，请求=%lu，链路代次=%lu",
             (unsigned long)unlock_action.request_id,
             (unsigned long)unlock_action.link_generation);
    return start_unlock_write(&unlock_action);
#endif
    return true;
}

static void try_start_automatic_authorization(void)
{
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    if (!s_automatic_authorization_pending ||
        s_automatic_authorization_attempt_generation == s_link_generation ||
        s_automatic_authorization_attempt_count >=
            BLE_AUTOMATIC_AUTHORIZATION_MAX_ATTEMPTS ||
        now_ms < s_automatic_authorization_retry_at_ms ||
        s_unlock_context.active || s_control_write_callback_pending ||
        !s_has_binding || !s_link_connected || !s_gatt_ready ||
        !s_notify_ready || !s_data_ready || !s_status_fresh ||
        !s_link_control_ready || !s_status_owner.mirror_valid ||
        !link_token_is_current(&s_status_owner.mirror_token))
    {
        return;
    }

    memset(&s_runtime_product_state, 0, sizeof(s_runtime_product_state));
    if (watch_state_snapshot(&s_runtime_product_state, 0) != ESP_OK ||
        !s_runtime_product_state.has_bound_exoskeleton ||
        !s_runtime_product_state.ble_connected ||
        s_runtime_product_state.ble_link_generation != s_link_generation ||
        !s_runtime_product_state.gatt_ready ||
        !s_runtime_product_state.notify_ready ||
        !s_runtime_product_state.device_state_available ||
        !s_runtime_product_state.dataReady ||
        !s_runtime_product_state.status_fresh ||
        !s_runtime_product_state.link_control_ready ||
        memcmp(s_runtime_product_state.bound_exoskeleton_mac,
               s_bound_mac,
               sizeof(s_bound_mac)) != 0)
    {
        return;
    }

    const uint32_t attempt_generation = s_link_generation;
    ++s_automatic_authorization_attempt_count;
    if (handle_first_unlock_intent(0U) &&
        attempt_generation == s_link_generation)
    {
        s_automatic_authorization_pending = false;
        s_automatic_authorization_attempt_generation = s_link_generation;
        s_automatic_authorization_retry_at_ms = 0U;
        return;
    }

    if (attempt_generation != s_link_generation)
    {
        return;
    }
    if (s_automatic_authorization_attempt_count >=
        BLE_AUTOMATIC_AUTHORIZATION_MAX_ATTEMPTS)
    {
        s_automatic_authorization_pending = false;
        s_automatic_authorization_attempt_generation = s_link_generation;
        s_automatic_authorization_retry_at_ms = 0U;
#if SCENIC_AREA_MANAGEMENT_DEBUG
        watch_control_update_t failed_update = {0};
        if (control_gate_fail_authorization_entry(
                &s_control_gate,
                s_bound_mac,
                s_link_generation,
                WATCH_RENTAL_RESULT_TRANSPORT_ERROR,
                BLE_AUTO_AUTH_RETRY_EXHAUSTED,
                &failed_update) == ESP_OK)
        {
            publish_control_update(&failed_update);
            const ble_unlock_request_t failed_request = {
                .intent_sequence = 0U,
                .request_id = failed_update.request_id,
                .link_generation = failed_update.link_generation,
            };
            ble_unlock_request_t bound_failed_request = failed_request;
            memcpy(bound_failed_request.exoskeleton_mac,
                   failed_update.exoskeleton_mac,
                   sizeof(bound_failed_request.exoskeleton_mac));
            publish_unlock_result(&bound_failed_request,
                                  BLE_UNLOCK_RESULT_BLOCKED,
                                  BLE_AUTO_AUTH_RETRY_EXHAUSTED);
        }
#endif
#if SCENIC_AREA_MANAGEMENT_DEBUG
        ESP_LOGE(TAG,
                 "自动授权入口重试已耗尽，链路代次=%lu，次数=%u，稳定码=%s",
                 (unsigned long)s_link_generation,
                 (unsigned)s_automatic_authorization_attempt_count,
                 BLE_AUTO_AUTH_RETRY_EXHAUSTED);
#else
        ESP_LOGE(TAG,
                 "自动授权入口重试已耗尽，链路代次=%lu，次数=%u",
                 (unsigned long)s_link_generation,
                 (unsigned)s_automatic_authorization_attempt_count);
#endif
        return;
    }
    s_automatic_authorization_retry_at_ms =
        now_ms + BLE_AUTOMATIC_AUTHORIZATION_RETRY_DELAY_MS;
    ESP_LOGW(TAG,
             "自动授权入口将在当前链路重试，链路代次=%lu，下一次=%u",
             (unsigned long)s_link_generation,
             (unsigned)(s_automatic_authorization_attempt_count + 1U));
}

#if !SCENIC_AREA_MANAGEMENT_DEBUG
static void schedule_automatic_authorization_retry(const char *error_code)
{
    if (s_link_generation == 0U ||
        s_automatic_authorization_attempt_generation != s_link_generation)
    {
        return;
    }
    if (s_automatic_authorization_attempt_count >=
        BLE_AUTOMATIC_AUTHORIZATION_MAX_ATTEMPTS)
    {
        s_automatic_authorization_pending = false;
        ESP_LOGE(TAG,
                 "非景区模式本地授权失败且重试已耗尽，链路代次=%lu，稳定码=%s",
                 (unsigned long)s_link_generation,
                 error_code != NULL ? error_code : "BLE_LOCAL_AUTH_FAILED");
        return;
    }
    s_automatic_authorization_attempt_generation = 0U;
    s_automatic_authorization_pending = true;
    s_automatic_authorization_retry_at_ms =
        (uint64_t)(esp_timer_get_time() / 1000) +
        BLE_AUTOMATIC_AUTHORIZATION_RETRY_DELAY_MS;
    ESP_LOGW(TAG,
             "非景区模式本地授权终态失败，将在当前链路重试，链路代次=%lu，稳定码=%s",
             (unsigned long)s_link_generation,
             error_code != NULL ? error_code : "BLE_LOCAL_AUTH_FAILED");
}
#endif

static void handle_control_intent(const ble_private_event_t *event)
{
    if (event == NULL)
    {
        return;
    }
    const ble_control_intent_t public_intent = {
        .kind = event->control_kind,
        .target = event->control_target,
    };
    ESP_LOGI(TAG,
             "BLE owner 收到控制请求：类型=%d，目标=%u，入口序号=%lu",
             (int)event->control_kind,
             (unsigned)event->control_target,
             (unsigned long)event->control_intent_sequence);
    if (control_intent_precedes_latest_poweroff(event))
    {
        ESP_LOGI(TAG,
                 "BLE 控制 intent 已被更新的关机优先级取消，类型=%d，序号=%lu",
                 (int)event->control_kind,
                 (unsigned long)event->control_intent_sequence);
        publish_control_result(NULL,
                               &public_intent,
                               BLE_CONTROL_RESULT_BLOCKED,
                               BLE_CONTROL_PREEMPTED_BY_POWEROFF);
        return;
    }
    bool ordinary_control_preempted = event->poweroff_preempted_ordinary;
    if (event->control_kind == WATCH_CONTROL_KIND_POWEROFF &&
        event->control_target == 1U)
    {
        s_poweroff_cancel_through_sequence = event->control_intent_sequence;
        s_poweroff_cancel_through_valid = true;
        ordinary_control_preempted = ordinary_control_preempted ||
                                     (s_control_gate.control_pending &&
                                      s_control_gate.control_kind !=
                                          WATCH_CONTROL_KIND_POWEROFF);
        if (!preempt_ordinary_control_for_poweroff())
        {
            publish_control_result(NULL,
                                   &public_intent,
                                   BLE_CONTROL_RESULT_BLOCKED,
                                   BLE_CONTROL_STATE_SYNC_FAILED);
            return;
        }
        if (s_control_write_callback_pending &&
            s_control_write_action.kind != WATCH_CONTROL_KIND_POWEROFF)
        {
            if (s_deferred_poweroff_valid)
            {
                const ble_control_intent_t superseded_intent = {
                    .kind = s_deferred_poweroff_intent.control_kind,
                    .target = s_deferred_poweroff_intent.control_target,
                };
                publish_control_result(NULL,
                                       &superseded_intent,
                                       BLE_CONTROL_RESULT_BUSY,
                                       "BLE_CONTROL_BUSY");
            }
            s_deferred_poweroff_intent = *event;
            s_deferred_poweroff_intent.poweroff_preempted_ordinary =
                ordinary_control_preempted;
            s_deferred_poweroff_valid = true;
            ESP_LOGI(TAG,
                     "BLE 关机已优先受理，等待旧普通 callback 释放 GATT procedure");
            return;
        }
    }
    if (s_control_write_callback_pending || s_unlock_context.active)
    {
        publish_control_result(NULL,
                               &public_intent,
                               BLE_CONTROL_RESULT_BUSY,
                               "BLE_CONTROL_BUSY");
        return;
    }
    const control_gate_control_intent_t gate_intent = {
        .kind = event->control_kind,
        .target = event->control_target,
    };
    memset(&s_runtime_product_state, 0, sizeof(s_runtime_product_state));
    control_gate_control_action_t action = {0};
    watch_control_update_t update = {0};
    if (watch_state_snapshot(&s_runtime_product_state, 0) != ESP_OK)
    {
        publish_control_result(NULL,
                               &public_intent,
                               BLE_CONTROL_RESULT_BLOCKED,
                               BLE_CONTROL_STATE_SNAPSHOT_FAILED);
        return;
    }
    if (!s_status_owner.mirror_valid ||
        !link_token_is_current(&s_status_owner.mirror_token))
    {
        publish_control_result(NULL,
                               &public_intent,
                               BLE_CONTROL_RESULT_BLOCKED,
                               BLE_CONTROL_REVALIDATION_FAILED);
        return;
    }
    /* 控制分类与最终构包必须读取同一 BLE owner 最新实际镜像。 */
    copy_product_state(&s_runtime_product_state.exoskeleton_state,
                       &s_status_owner.mirror);
    s_runtime_product_state.exoskeleton_model =
        watch_exoskeleton_model_from_version(
            s_status_owner.mirror.version);
    if (event->control_kind == WATCH_CONTROL_KIND_POWEROFF &&
        ordinary_control_preempted)
    {
        /* 取消状态为非阻塞发布；关机更新可直接覆盖仍未消费的旧 pending 快照。 */
        s_runtime_product_state.pending_control = false;
    }

    control_gate_t candidate_gate = s_control_gate;
    const control_gate_control_request_status_t status =
        control_gate_request_control(&candidate_gate,
                                     &s_runtime_product_state,
                                     s_link_generation,
                                     &gate_intent,
                                     &action,
                                     &update);
    ESP_LOGI(TAG,
             "BLE owner 控制门禁结果：状态=%d，请求=%lu，类型=%d，目标=%u，attempt=%u",
             (int)status,
             (unsigned long)action.request_id,
             (int)event->control_kind,
             (unsigned)event->control_target,
             (unsigned)action.attempt);
    if (status == CONTROL_GATE_CONTROL_NO_OP)
    {
        if (!publish_control_update_confirmed(&update))
        {
            publish_control_result(&action,
                                   &public_intent,
                                   BLE_CONTROL_RESULT_BLOCKED,
                                   BLE_CONTROL_STATE_SYNC_FAILED);
            return;
        }
        s_control_gate = candidate_gate;
        publish_control_result(&action,
                               &public_intent,
                               BLE_CONTROL_RESULT_NO_OP,
                               "BLE_CONTROL_NO_OP");
        return;
    }
    if (status != CONTROL_GATE_CONTROL_ACCEPTED ||
        action.type != CONTROL_GATE_CONTROL_ACTION_BLE_WRITE)
    {
        const ble_control_result_code_t result =
            status == CONTROL_GATE_CONTROL_BUSY
                ? BLE_CONTROL_RESULT_BUSY
                : (status == CONTROL_GATE_CONTROL_LOCKED_OUT
                       ? BLE_CONTROL_RESULT_RETRY_EXHAUSTED
                       : (status == CONTROL_GATE_CONTROL_AUTH_REQUIRED
                              ? BLE_CONTROL_RESULT_AUTH_REQUIRED
                              : (status == CONTROL_GATE_CONTROL_INVALID
                                     ? BLE_CONTROL_RESULT_INVALID
                                     : BLE_CONTROL_RESULT_BLOCKED)));
        const char *error_code =
            result == BLE_CONTROL_RESULT_BUSY
                ? "BLE_CONTROL_BUSY"
                : (result == BLE_CONTROL_RESULT_RETRY_EXHAUSTED
                       ? BLE_CONTROL_RETRY_EXHAUSTED
                       : (result == BLE_CONTROL_RESULT_AUTH_REQUIRED
                              ? "BLE_CONTROL_AUTH_REQUIRED"
                              : (result == BLE_CONTROL_RESULT_INVALID
                                     ? "BLE_CONTROL_INVALID"
                                     : "BLE_CONTROL_BLOCKED")));
        publish_control_result(NULL, &public_intent, result, error_code);
        return;
    }
    if (!publish_control_update_confirmed(&update))
    {
        publish_control_result(&action,
                               &public_intent,
                               BLE_CONTROL_RESULT_BLOCKED,
                               BLE_CONTROL_STATE_SYNC_FAILED);
        return;
    }
    s_control_gate = candidate_gate;
    start_control_write(&action);
}

static void handle_priority_poweroff_mailbox(void)
{
    const uint32_t sequence =
        (uint32_t)atomic_exchange(&s_poweroff_mailbox_sequence, 0U);
    if (sequence == 0U)
    {
        return;
    }
    const ble_private_event_t event = {
        .type = BLE_PRIVATE_EVENT_CONTROL_INTENT,
        .control_kind = WATCH_CONTROL_KIND_POWEROFF,
        .control_target = 1U,
        .control_intent_sequence = sequence,
    };
    handle_control_intent(&event);
}

static bool control_intent_precedes_latest_poweroff(
    const ble_private_event_t *event)
{
    return event != NULL && s_poweroff_cancel_through_valid &&
           (int32_t)(event->control_intent_sequence -
                     s_poweroff_cancel_through_sequence) < 0;
}

static bool preempt_ordinary_control_for_poweroff(void)
{
    if (!s_control_gate.control_pending ||
        s_control_gate.control_kind == WATCH_CONTROL_KIND_POWEROFF)
    {
        return true;
    }
    const control_gate_control_action_t active_action = {
        .type = CONTROL_GATE_CONTROL_ACTION_NONE,
        .request_id = s_control_gate.request_id,
        .link_generation = s_control_gate.link_generation,
        .kind = s_control_gate.control_kind,
        .target = s_control_gate.control_target,
        .attempt = s_control_gate.control_attempt,
    };
    control_gate_control_action_t bound_action = active_action;
    memcpy(bound_action.exoskeleton_mac,
           s_control_gate.exoskeleton_mac,
           sizeof(bound_action.exoskeleton_mac));
    control_gate_t candidate_gate = s_control_gate;
    watch_control_update_t update = {0};
    if (!control_gate_abort_pending(&candidate_gate, &update))
    {
        ESP_LOGE(TAG, "BLE 关机抢占普通控制失败，稳定码=%s",
                 BLE_CONTROL_STATE_SYNC_FAILED);
        return false;
    }
    s_control_gate = candidate_gate;
    publish_control_update(&update);
    publish_control_result(&bound_action,
                           NULL,
                           BLE_CONTROL_RESULT_BLOCKED,
                           BLE_CONTROL_PREEMPTED_BY_POWEROFF);
    ESP_LOGI(TAG,
             "BLE 关机已终结普通控制，请求=%lu，attempt=%u",
             (unsigned long)bound_action.request_id,
             (unsigned)bound_action.attempt);
    return true;
}

static void cancel_deferred_poweroff(const char *error_code)
{
    if (!s_deferred_poweroff_valid)
    {
        return;
    }
    const ble_control_intent_t intent = {
        .kind = s_deferred_poweroff_intent.control_kind,
        .target = s_deferred_poweroff_intent.control_target,
    };
    memset(&s_deferred_poweroff_intent, 0,
           sizeof(s_deferred_poweroff_intent));
    s_deferred_poweroff_valid = false;
    publish_control_result(NULL,
                           &intent,
                           BLE_CONTROL_RESULT_BLOCKED,
                           error_code != NULL
                               ? error_code
                               : BLE_CONTROL_REVALIDATION_FAILED);
}

#if SCENIC_AREA_MANAGEMENT_DEBUG
static void try_submit_pending_cloud_rental_request(void)
{
    if (!s_cloud_rental_submission_pending)
    {
        return;
    }
    if (!s_control_gate.pending ||
        s_control_gate.phase != WATCH_RENTAL_PHASE_QUERYING ||
        s_control_gate.request_id != s_cloud_rental_submission.request_id ||
        s_control_gate.link_generation !=
            s_cloud_rental_submission.link_generation ||
        memcmp(s_control_gate.exoskeleton_mac,
               s_cloud_rental_submission.exoskeleton_mac,
               sizeof(s_control_gate.exoskeleton_mac)) != 0)
    {
        clear_pending_cloud_rental_submission();
        return;
    }

    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    if (now_ms < s_cloud_rental_submission_retry_at_ms)
    {
        return;
    }
    if (s_cloud_rental_submission_attempt_count < UINT8_MAX)
    {
        ++s_cloud_rental_submission_attempt_count;
    }
    const esp_err_t submit_error =
        cloud_service_rental_submit(&s_cloud_rental_submission, 0);
    if (submit_error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "云租赁请求已进入 cloud owner 队列，请求=%lu，链路代次=%lu，入队次数=%u",
                 (unsigned long)s_cloud_rental_submission.request_id,
                 (unsigned long)s_cloud_rental_submission.link_generation,
                 (unsigned)s_cloud_rental_submission_attempt_count);
        clear_pending_cloud_rental_submission();
        return;
    }
    const uint32_t retry_delay_ms = cloud_submission_retry_delay_ms(
        s_cloud_rental_submission_attempt_count);
    s_cloud_rental_submission_retry_at_ms =
        now_ms + retry_delay_ms;
    ESP_LOGW(TAG,
             "云租赁请求暂未入队，将保留同一请求退避重试，请求=%lu，链路代次=%lu，次数=%u，延迟=%lu ms，错误=0x%x",
             (unsigned long)s_cloud_rental_submission.request_id,
             (unsigned long)s_cloud_rental_submission.link_generation,
             (unsigned)s_cloud_rental_submission_attempt_count,
             (unsigned long)retry_delay_ms,
             (unsigned)submit_error);
}

static uint32_t cloud_submission_retry_delay_ms(uint8_t attempt_count)
{
    uint32_t delay_ms = BLE_CLOUD_RENTAL_SUBMISSION_RETRY_DELAY_MS;
    uint8_t remaining_steps = attempt_count > 0U
                                  ? (uint8_t)(attempt_count - 1U)
                                  : 0U;
    while (remaining_steps > 0U &&
           delay_ms < BLE_CLOUD_RENTAL_SUBMISSION_RETRY_MAX_DELAY_MS)
    {
        const uint32_t doubled =
            delay_ms > BLE_CLOUD_RENTAL_SUBMISSION_RETRY_MAX_DELAY_MS / 2U
                ? BLE_CLOUD_RENTAL_SUBMISSION_RETRY_MAX_DELAY_MS
                : delay_ms * 2U;
        delay_ms = doubled;
        --remaining_steps;
    }
    return delay_ms;
}

static void clear_pending_cloud_rental_submission(void)
{
    s_cloud_rental_submission_pending = false;
    memset(&s_cloud_rental_submission,
           0,
           sizeof(s_cloud_rental_submission));
    s_cloud_rental_submission_attempt_count = 0U;
    s_cloud_rental_submission_retry_at_ms = 0U;
}

static void cancel_cloud_rental_session(void)
{
    if (!s_control_gate.pending || s_control_gate.request_id == 0U)
    {
        return;
    }
    const esp_err_t error =
        cloud_service_rental_cancel(s_control_gate.request_id);
    if (error == ESP_OK)
    {
        ESP_LOGI(TAG,
                 "BLE 链路身份变化已取消支付会话，请求=%lu，链路代次=%lu",
                 (unsigned long)s_control_gate.request_id,
                 (unsigned long)s_control_gate.link_generation);
        return;
    }
    ESP_LOGW(TAG,
             "BLE 链路身份变化锁存支付会话取消失败，请求=%lu，错误=0x%x",
             (unsigned long)s_control_gate.request_id,
             (unsigned)error);
}

static void handle_cloud_rental_results(void)
{
    cloud_rental_result_t result = {0};
    while (cloud_service_rental_receive(&result, 0) == ESP_OK)
    {
        handle_cloud_rental_result(&result);
        memset(&result, 0, sizeof(result));
    }
}

static void handle_cloud_rental_result(const cloud_rental_result_t *result)
{
    if (result == NULL)
    {
        return;
    }
    control_gate_rental_result_t rental = {
        .request_id = result->request_id,
        .link_generation = result->link_generation,
        .result = result->result,
        .http_status = result->http_status,
    };
    memcpy(rental.exoskeleton_mac,
           result->exoskeleton_mac,
           sizeof(rental.exoskeleton_mac));
    memcpy(rental.error_code,
           result->error_code,
           sizeof(rental.error_code));
    control_gate_action_t action = {0};
    watch_control_update_t update = {0};
    if (control_gate_apply_rental_result(&s_control_gate,
                                         &rental,
                                         &action,
                                         &update) != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 已忽略迟到租赁终态，请求=%lu，代次=%lu",
                 (unsigned long)result->request_id,
                 (unsigned long)result->link_generation);
        return;
    }
    const ble_unlock_request_t request = {
        .intent_sequence = s_authorization_intent_sequence,
        .request_id = result->request_id,
        .link_generation = result->link_generation,
    };
    ble_unlock_request_t bound_request = request;
    memcpy(bound_request.exoskeleton_mac,
           result->exoskeleton_mac,
           sizeof(bound_request.exoskeleton_mac));
    if (action.type == CONTROL_GATE_ACTION_BLE_UNLOCK)
    {
        if (!publish_control_update_confirmed(&update))
        {
            watch_control_update_t blocked_update = {0};
            if (control_gate_abort_pending(&s_control_gate,
                                           &blocked_update))
            {
                publish_control_update(&blocked_update);
            }
            publish_unlock_result(&bound_request,
                                  BLE_UNLOCK_RESULT_BLOCKED,
                                  BLE_CONTROL_STATE_SYNC_FAILED);
            return;
        }
        start_unlock_write(&action);
        return;
    }
    publish_control_update(&update);
    if (update.rental_phase == WATCH_RENTAL_PHASE_WAITING_PAYMENT ||
        update.rental_phase == WATCH_RENTAL_PHASE_RETRY_WAIT)
    {
        ESP_LOGI(TAG,
                 "BLE 已保留当前支付会话中间态，请求=%lu，链路代次=%lu，阶段=%d",
                 (unsigned long)result->request_id,
                 (unsigned long)result->link_generation,
                 (int)update.rental_phase);
        return;
    }
    publish_unlock_result(&bound_request,
                          BLE_UNLOCK_RESULT_CLOUD_DENIED,
                          result->error_code);
}
#endif

static bool start_unlock_write(const control_gate_action_t *action)
{
    if (action == NULL || action->type != CONTROL_GATE_ACTION_BLE_UNLOCK)
    {
        return false;
    }
    ble_unlock_request_t request = {
        .intent_sequence = s_authorization_intent_sequence,
        .request_id = action->request_id,
        .link_generation = action->link_generation,
    };
    memcpy(request.exoskeleton_mac,
           action->exoskeleton_mac,
           sizeof(request.exoskeleton_mac));
    const bool current_link = s_link_connected && s_gatt_ready &&
                              s_notify_ready && s_link_control_ready &&
                              s_characteristic_val_handle != 0U &&
                              action->link_generation == s_link_generation &&
                              s_has_binding &&
                              memcmp(action->exoskeleton_mac,
                                     s_bound_mac,
                                     sizeof(s_bound_mac)) == 0 &&
                              s_status_owner.mirror_valid &&
                              link_token_is_current(&s_status_owner.mirror_token);
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    ble_unlock_callback_context_t *callback_context = NULL;
    int write_result = BLE_HS_EBUSY;
    if (current_link)
    {
        request_connection_power_profile(
            BLE_CONNECTION_POWER_PROFILE_ACTIVE,
            false);
        callback_context = allocate_unlock_callback_context();
        if (callback_context != NULL)
        {
            callback_context->request = request;
            write_result = start_unlock_write_boundary(
                &s_unlock_context,
                &request,
                &s_status_owner.mirror,
                s_status_owner.diagnostics.valid_frame_count,
                now_ms,
                s_connection_handle,
                s_characteristic_val_handle,
                gatt_unlock_write_callback,
                callback_context);
            if (write_result != 0)
            {
                atomic_store(&callback_context->in_use, false);
            }
        }
    }
    if (write_result == 0)
    {
        ESP_LOGI(TAG,
                 "BLE 一次性解锁包已发起，请求=%lu，代次=%lu",
                 (unsigned long)request.request_id,
                 (unsigned long)request.link_generation);
        return true;
    }
    ble_unlock_cancel(&s_unlock_context);
    watch_control_update_t update = {0};
    if (control_gate_apply_write_result(&s_control_gate,
                                        request.request_id,
                                        request.exoskeleton_mac,
                                        request.link_generation,
                                        false,
                                        s_status_owner.diagnostics.valid_frame_count,
                                        now_ms,
                                        &update) == ESP_OK)
    {
        publish_control_update(&update);
    }
    ESP_LOGE(TAG,
             "BLE 一次性解锁写入未启动，稳定码=%s，错误=%d",
             BLE_UNLOCK_WRITE_START_FAILED,
             write_result);
    publish_unlock_result(&request,
                          BLE_UNLOCK_RESULT_WRITE_FAILED,
                          BLE_UNLOCK_WRITE_START_FAILED);
    return false;
}

static void start_control_write(const control_gate_control_action_t *action)
{
    if (action == NULL ||
        action->type != CONTROL_GATE_CONTROL_ACTION_BLE_WRITE)
    {
        return;
    }
    memset(&s_runtime_product_state, 0, sizeof(s_runtime_product_state));
    ble_control_packet_override_t override = {0};
    const bool product_snapshot_ready =
        watch_state_snapshot(&s_runtime_product_state, 0) == ESP_OK;
    const bool current_link =
        product_snapshot_ready && !s_control_write_callback_pending &&
        s_link_connected && s_gatt_ready && s_notify_ready && s_data_ready &&
        s_status_fresh && s_link_control_ready &&
        s_characteristic_val_handle != 0U &&
        (s_characteristic_properties & BLE_GATT_CHR_PROP_WRITE) != 0U &&
        action->link_generation == s_link_generation && s_has_binding &&
        memcmp(action->exoskeleton_mac,
               s_bound_mac,
               sizeof(s_bound_mac)) == 0 &&
        s_status_owner.mirror_valid &&
        link_token_is_current(&s_status_owner.mirror_token) &&
        s_runtime_product_state.has_bound_exoskeleton &&
        s_runtime_product_state.ble_connected &&
        s_runtime_product_state.gatt_ready &&
        s_runtime_product_state.notify_ready &&
        s_runtime_product_state.dataReady &&
        s_runtime_product_state.status_fresh &&
        s_runtime_product_state.link_control_ready &&
        s_runtime_product_state.unlock_session_valid &&
        s_runtime_product_state.pending_control &&
        s_runtime_product_state.rental_phase == WATCH_RENTAL_PHASE_UNLOCKED &&
        s_runtime_product_state.control_phase == WATCH_CONTROL_PHASE_WRITING &&
        s_runtime_product_state.control_attempt == action->attempt &&
        !s_runtime_product_state.control_locked_out &&
        s_runtime_product_state.control_request_id == action->request_id &&
        s_runtime_product_state.control_link_generation ==
            action->link_generation &&
        s_runtime_product_state.control_kind == action->kind &&
        s_runtime_product_state.control_target == action->target &&
        memcmp(s_runtime_product_state.control_exoskeleton_mac,
               action->exoskeleton_mac,
               sizeof(s_runtime_product_state.control_exoskeleton_mac)) == 0 &&
        control_action_to_override(action,
                                   s_runtime_product_state.exoskeleton_state.scene_config,
                                   &override);
    if (!current_link)
    {
        control_gate_t candidate_gate = s_control_gate;
        watch_control_update_t rejected = {0};
        if (control_gate_abort_pending(&candidate_gate, &rejected) &&
            publish_control_update_confirmed(&rejected))
        {
            s_control_gate = candidate_gate;
            publish_control_result(action,
                                   NULL,
                                   BLE_CONTROL_RESULT_BLOCKED,
                                   product_snapshot_ready
                                       ? BLE_CONTROL_REVALIDATION_FAILED
                                       : BLE_CONTROL_STATE_SNAPSHOT_FAILED);
        }
        return;
    }

    ble_control_callback_context_t *callback_context =
        allocate_control_callback_context();
    int write_result = BLE_HS_EBUSY;
    if (callback_context != NULL)
    {
        request_connection_power_profile(
            BLE_CONNECTION_POWER_PROFILE_ACTIVE,
            false);
        callback_context->action = *action;
        callback_context->status_sequence_at_write =
            s_valid_status_sequence;
        s_control_write_action = *action;
        s_control_write_started_ms =
            (uint64_t)(esp_timer_get_time() / 1000);
        s_control_write_callback_pending = true;
        write_result = start_control_write_boundary(
            &s_status_owner.mirror,
            &override,
            s_connection_handle,
            s_characteristic_val_handle,
            gatt_control_write_callback,
            callback_context);
    }
    if (write_result == 0)
    {
        if (action->kind == WATCH_CONTROL_KIND_POWEROFF)
        {
            s_poweroff_issued_in_session = true;
        }
        ESP_LOGI(TAG,
                 "BLE 普通控制完整包已发起，请求=%lu，类型=%d，目标=%u",
                 (unsigned long)action->request_id,
                 (int)action->kind,
                 (unsigned)action->target);
        return;
    }
    if (callback_context != NULL)
    {
        atomic_store(&callback_context->in_use, false);
        memset(&callback_context->action, 0, sizeof(callback_context->action));
    }
    s_control_write_callback_pending = false;
    s_control_write_started_ms = 0U;
    memset(&s_control_write_action, 0, sizeof(s_control_write_action));
    ESP_LOGE(TAG,
             "BLE 普通控制 attempt=%u 写入未启动，稳定码=%s，错误=%d",
             (unsigned)action->attempt,
             BLE_CONTROL_WRITE_START_FAILED,
             write_result);
    apply_control_attempt_result(
        action,
        false,
        s_valid_status_sequence,
        (uint64_t)(esp_timer_get_time() / 1000),
        BLE_CONTROL_WRITE_START_FAILED,
        true);
}

static void handle_control_write_result(const ble_private_event_t *event)
{
    if (event == NULL || !s_control_write_callback_pending ||
        event->request_id != s_control_write_action.request_id ||
        event->link_generation != s_control_write_action.link_generation ||
        event->control_kind != s_control_write_action.kind ||
        event->control_target != s_control_write_action.target ||
        event->control_attempt != s_control_write_action.attempt ||
        memcmp(event->mac,
               s_control_write_action.exoskeleton_mac,
               sizeof(event->mac)) != 0)
    {
        return;
    }
    const control_gate_control_action_t action = s_control_write_action;
    s_control_write_callback_pending = false;
    s_control_write_started_ms = 0U;
    memset(&s_control_write_action, 0, sizeof(s_control_write_action));
    if (s_deferred_poweroff_valid &&
        action.kind != WATCH_CONTROL_KIND_POWEROFF)
    {
        const ble_private_event_t poweroff_intent =
            s_deferred_poweroff_intent;
        memset(&s_deferred_poweroff_intent, 0,
               sizeof(s_deferred_poweroff_intent));
        s_deferred_poweroff_valid = false;
        ESP_LOGI(TAG,
                 "BLE 旧普通 callback 已隔离，立即发起已受理关机请求");
        handle_control_intent(&poweroff_intent);
        return;
    }
    const bool success = event->reason == 0 && event_is_current_link(event) &&
                         s_has_binding && s_status_fresh &&
                         memcmp(event->mac,
                                s_bound_mac,
                                sizeof(s_bound_mac)) == 0;
    ESP_LOGI(TAG,
             "BLE 控制写回调：请求=%lu，类型=%d，目标=%u，attempt=%u，状态=%ld，链路有效=%u",
             (unsigned long)action.request_id,
             (int)action.kind,
             (unsigned)action.target,
             (unsigned)action.attempt,
             (long)event->reason,
             success ? 1U : 0U);
    apply_control_attempt_result(
        &action,
        success,
        event->control_status_sequence_at_write,
        (uint64_t)(esp_timer_get_time() / 1000),
        BLE_CONTROL_WRITE_CALLBACK_FAILED,
        true);
    if (success && s_control_gate.control_pending &&
        s_control_gate.control_phase ==
            WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED &&
        s_valid_status_sequence !=
            event->control_status_sequence_at_write)
    {
        /* 回调前到达的后续状态帧仍是有效 ACK 证据，立即复核缓存镜像。 */
        handle_control_status_evidence(&s_status_owner.mirror_token);
    }
}

static void apply_control_attempt_result(
    const control_gate_control_action_t *completed_action,
    bool success,
    uint32_t status_sequence_at_write,
    uint64_t now_ms,
    const char *failure_error_code,
    bool start_retry)
{
    if (completed_action == NULL)
    {
        return;
    }
    control_gate_t candidate_gate = s_control_gate;
    control_gate_control_action_t next_action = {0};
    watch_control_update_t update = {0};
    if (control_gate_apply_control_write_result_at(
            &candidate_gate,
            completed_action->request_id,
            completed_action->exoskeleton_mac,
            completed_action->link_generation,
            completed_action->kind,
            completed_action->target,
            success,
            status_sequence_at_write,
            now_ms,
            &next_action,
            &update) != ESP_OK)
    {
        return;
    }
    if (!publish_control_update_confirmed(&update))
    {
        control_gate_t blocked_gate = s_control_gate;
        watch_control_update_t blocked = {0};
        if (control_gate_abort_pending(&blocked_gate, &blocked) &&
            publish_control_update_confirmed(&blocked))
        {
            s_control_gate = blocked_gate;
            publish_control_result(completed_action,
                                   NULL,
                                   BLE_CONTROL_RESULT_BLOCKED,
                                   BLE_CONTROL_STATE_SYNC_FAILED);
        }
        return;
    }
    s_control_gate = candidate_gate;

    if (next_action.type == CONTROL_GATE_CONTROL_ACTION_BLE_WRITE)
    {
        note_control_retry_for_tx_power(&next_action);
        publish_control_result(&next_action,
                               NULL,
                               BLE_CONTROL_RESULT_RETRYING,
                               BLE_CONTROL_RETRYING);
        ESP_LOGW(TAG,
                 "BLE 普通控制准备重试，请求=%lu，attempt=%u",
                 (unsigned long)next_action.request_id,
                 (unsigned)next_action.attempt);
        if (start_retry)
        {
            start_control_write(&next_action);
        }
        return;
    }
    if (update.pending_control)
    {
        const bool poweroff_pending =
            update.control_phase == WATCH_CONTROL_PHASE_POWEROFF_PENDING;
        publish_control_result(
            completed_action,
            NULL,
            poweroff_pending ? BLE_CONTROL_RESULT_POWEROFF_PENDING
                             : BLE_CONTROL_RESULT_SUBMITTED_UNCONFIRMED,
            poweroff_pending ? BLE_POWEROFF_PENDING
                             : "BLE_CONTROL_SUBMITTED_UNCONFIRMED");
        return;
    }
    if (update.control_last_result ==
        WATCH_CONTROL_RESULT_RETRY_EXHAUSTED)
    {
        publish_control_result(completed_action,
                               NULL,
                               BLE_CONTROL_RESULT_RETRY_EXHAUSTED,
                               BLE_CONTROL_RETRY_EXHAUSTED);
        return;
    }
    publish_control_result(completed_action,
                           NULL,
                           BLE_CONTROL_RESULT_WRITE_REJECTED,
                           failure_error_code != NULL
                               ? failure_error_code
                               : BLE_CONTROL_WRITE_CALLBACK_FAILED);
}

static void check_control_write_timeout(void)
{
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    if (s_control_write_callback_pending)
    {
        if (now_ms < s_control_write_started_ms ||
            now_ms - s_control_write_started_ms <=
                BLE_CONTROL_WRITE_CALLBACK_TIMEOUT_MS)
        {
            return;
        }
        const control_gate_control_action_t action = s_control_write_action;
        s_control_write_callback_pending = false;
        s_control_write_started_ms = 0U;
        memset(&s_control_write_action, 0, sizeof(s_control_write_action));
        apply_control_attempt_result(&action,
                                     false,
                                     s_valid_status_sequence,
                                     now_ms,
                                     BLE_CONTROL_WRITE_CALLBACK_TIMEOUT,
                                     false);
        ESP_LOGE(TAG,
                 "BLE 普通控制 callback 超时，请求=%lu，attempt=%u，稳定码=%s",
                 (unsigned long)action.request_id,
                 (unsigned)action.attempt,
                 BLE_CONTROL_WRITE_CALLBACK_TIMEOUT);
        abort_unlock_transaction(BLE_CONTROL_WRITE_CALLBACK_TIMEOUT, false);
        if (recover_nimble_runtime(BLE_CONTROL_WRITE_CALLBACK_TIMEOUT) !=
            ESP_OK)
        {
            ESP_LOGE(TAG,
                     "BLE 普通控制 callback 超时后 host 隔离失败，稳定码=%s",
                     BLE_HOST_STOP_FAILED);
        }
        return;
    }

    control_gate_t candidate_gate = s_control_gate;
    control_gate_control_action_t next_action = {0};
    watch_control_update_t update = {0};
    const esp_err_t control_timeout =
        control_gate_check_control_timeout(&candidate_gate,
                                           now_ms,
                                           &next_action,
                                           &update);
    if (control_timeout == ESP_ERR_TIMEOUT)
    {
        if (!publish_control_update_confirmed(&update))
        {
            abort_unlock_transaction(BLE_CONTROL_STATE_SYNC_FAILED, false);
            return;
        }
        const control_gate_control_action_t terminal_action = {
            .type = CONTROL_GATE_CONTROL_ACTION_NONE,
            .request_id = candidate_gate.request_id,
            .link_generation = candidate_gate.link_generation,
            .kind = candidate_gate.control_kind,
            .target = candidate_gate.control_target,
            .attempt = candidate_gate.control_attempt,
        };
        control_gate_control_action_t bound_terminal = terminal_action;
        memcpy(bound_terminal.exoskeleton_mac,
               candidate_gate.exoskeleton_mac,
               sizeof(bound_terminal.exoskeleton_mac));
        s_control_gate = candidate_gate;
        if (next_action.type == CONTROL_GATE_CONTROL_ACTION_BLE_WRITE)
        {
            note_control_retry_for_tx_power(&next_action);
            publish_control_result(&next_action,
                                   NULL,
                                   BLE_CONTROL_RESULT_RETRYING,
                                   BLE_CONTROL_RETRYING);
            start_control_write(&next_action);
            return;
        }
        publish_control_result(&bound_terminal,
                               NULL,
                               BLE_CONTROL_RESULT_RETRY_EXHAUSTED,
                               BLE_CONTROL_RETRY_EXHAUSTED);
        return;
    }

    candidate_gate = s_control_gate;
    memset(&update, 0, sizeof(update));
    if (control_gate_check_poweroff_timeout(&candidate_gate,
                                            now_ms,
                                            &update) != ESP_ERR_TIMEOUT)
    {
        return;
    }
    const control_gate_control_action_t poweroff_action = {
        .type = CONTROL_GATE_CONTROL_ACTION_NONE,
        .request_id = s_control_gate.request_id,
        .link_generation = s_control_gate.link_generation,
        .kind = s_control_gate.control_kind,
        .target = s_control_gate.control_target,
        .attempt = s_control_gate.control_attempt,
    };
    control_gate_control_action_t bound_poweroff = poweroff_action;
    memcpy(bound_poweroff.exoskeleton_mac,
           s_control_gate.exoskeleton_mac,
           sizeof(bound_poweroff.exoskeleton_mac));
    if (!publish_control_update_confirmed(&update))
    {
        abort_unlock_transaction(BLE_CONTROL_STATE_SYNC_FAILED, false);
        return;
    }
    s_control_gate = candidate_gate;
    publish_poweroff_terminal(&bound_poweroff,
                              WATCH_POWEROFF_RESULT_TIMEOUT_UNKNOWN);
}

static bool finalize_poweroff_offline(uint64_t now_ms)
{
    if (!s_control_gate.control_pending ||
        s_control_gate.control_phase != WATCH_CONTROL_PHASE_POWEROFF_PENDING ||
        s_control_gate.control_kind != WATCH_CONTROL_KIND_POWEROFF)
    {
        return false;
    }
    const control_gate_control_action_t poweroff_action = {
        .type = CONTROL_GATE_CONTROL_ACTION_NONE,
        .request_id = s_control_gate.request_id,
        .link_generation = s_control_gate.link_generation,
        .kind = s_control_gate.control_kind,
        .target = s_control_gate.control_target,
        .attempt = s_control_gate.control_attempt,
    };
    control_gate_control_action_t bound_action = poweroff_action;
    memcpy(bound_action.exoskeleton_mac,
           s_control_gate.exoskeleton_mac,
           sizeof(bound_action.exoskeleton_mac));
    control_gate_t terminal_gate = s_control_gate;
    watch_control_update_t update = {0};
    if (control_gate_apply_poweroff_offline_evidence(
            &terminal_gate,
            bound_action.request_id,
            bound_action.exoskeleton_mac,
            bound_action.link_generation,
            now_ms,
            &update) != ESP_OK)
    {
        return false;
    }
    if (!publish_control_update_confirmed(&update))
    {
        ESP_LOGE(TAG,
                 "关机离线终态应用确认失败，请求=%lu",
                 (unsigned long)bound_action.request_id);
        return false;
    }
    s_control_gate = terminal_gate;
    publish_poweroff_terminal(&bound_action,
                              WATCH_POWEROFF_RESULT_ASSUMED_OFF);
    return true;
}

static void invalidate_current_authorization_proof(void)
{
    if (s_control_gate.phase != WATCH_RENTAL_PHASE_UNLOCKED ||
        s_control_gate.last_result != WATCH_RENTAL_RESULT_PAID)
    {
        return;
    }
    watch_control_update_t update = {0};
    control_gate_invalidate_session(&s_control_gate, &update);
    publish_control_update(&update);
}

static void publish_poweroff_terminal(
    const control_gate_control_action_t *action,
    watch_poweroff_result_t result)
{
    ble_control_result_code_t public_result;
    const char *error_code;
    switch (result)
    {
    case WATCH_POWEROFF_RESULT_ASSUMED_OFF:
        public_result = BLE_CONTROL_RESULT_POWEROFF_ASSUMED_OFF;
        error_code = BLE_POWEROFF_ASSUMED_OFF;
        break;
    case WATCH_POWEROFF_RESULT_TIMEOUT_UNKNOWN:
        public_result = BLE_CONTROL_RESULT_POWEROFF_TIMEOUT_UNKNOWN;
        error_code = BLE_POWEROFF_TIMEOUT_UNKNOWN;
        break;
    case WATCH_POWEROFF_RESULT_FAILED_RECONNECTED:
        public_result = BLE_CONTROL_RESULT_POWEROFF_FAILED_RECONNECTED;
        error_code = BLE_POWEROFF_FAILED_RECONNECTED;
        break;
    case WATCH_POWEROFF_RESULT_NONE:
    default:
        return;
    }
    publish_control_result(action, NULL, public_result, error_code);
}

static bool control_action_to_override(
    const control_gate_control_action_t *action,
    watch_exoskeleton_scene_config_t config,
    ble_control_packet_override_t *override)
{
    if (action == NULL || override == NULL)
    {
        return false;
    }
    ble_control_override_kind_t kind = BLE_CONTROL_OVERRIDE_NONE;
    switch (action->kind)
    {
    case WATCH_CONTROL_KIND_MOTOR_ENABLE:
        kind = BLE_CONTROL_OVERRIDE_MOTOR_ENABLE;
        break;
    case WATCH_CONTROL_KIND_GEAR:
        kind = BLE_CONTROL_OVERRIDE_GEAR;
        break;
    case WATCH_CONTROL_KIND_SCENE_MODE:
        kind = BLE_CONTROL_OVERRIDE_SCENE_MODE;
        break;
    case WATCH_CONTROL_KIND_POWEROFF:
        kind = BLE_CONTROL_OVERRIDE_POWEROFF;
        break;
    case WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY:
        kind = BLE_CONTROL_OVERRIDE_GEAR_SCENE_BOUNDARY;
        break;
    default:
        return false;
    }
    uint8_t protocol_target = action->target;
    if (action->kind == WATCH_CONTROL_KIND_GEAR &&
        action->target > watch_exoskeleton_scene_config_max_gear(s_runtime_product_state.exoskeleton_state.scene_config))
    {
        return false;
    }
    if (action->kind == WATCH_CONTROL_KIND_GEAR_SCENE_BOUNDARY &&
        s_runtime_product_state.exoskeleton_state.scene_config != WATCH_EXOSKELETON_SCENE_CONFIG_FULL)
    {
        return false;
    }
    if (action->kind == WATCH_CONTROL_KIND_SCENE_MODE &&
        !watch_exoskeleton_scene_config_supports_mode(config, action->target))
    {
        return false;
    }
    if (config == WATCH_EXOSKELETON_SCENE_CONFIG_NO_EXTREME && action->target == 4U)
        protocol_target = 3U;
    else if (config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP && action->target == WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP)
        protocol_target = 2U;
    *override = (ble_control_packet_override_t){
        .kind = kind,
        .target = protocol_target,
    };
    return true;
}

static void handle_unlock_write_result(const ble_private_event_t *event)
{
    if (event == NULL)
    {
        return;
    }
    ble_unlock_request_t request = {
        .intent_sequence = event->unlock_intent_sequence,
        .request_id = event->request_id,
        .link_generation = event->link_generation,
    };
    memcpy(request.exoskeleton_mac, event->mac, sizeof(request.exoskeleton_mac));
    const bool success = event->reason == 0 && event_is_current_link(event) &&
                         s_has_binding &&
                         memcmp(event->mac,
                                s_bound_mac,
                                sizeof(s_bound_mac)) == 0;
    ESP_LOGI(TAG,
             "BLE 解锁写回调：请求=%lu，状态=%ld，链路有效=%u",
             (unsigned long)request.request_id,
             (long)event->reason,
             success ? 1U : 0U);
    if (!ble_unlock_accept_write_callback(
            &s_unlock_context,
            &request,
            success,
            s_status_owner.diagnostics.valid_frame_count,
            event->received_at_ms))
    {
        return;
    }
    watch_control_update_t update = {0};
    if (control_gate_apply_write_result(
            &s_control_gate,
            request.request_id,
            request.exoskeleton_mac,
            request.link_generation,
            success,
            s_status_owner.diagnostics.valid_frame_count,
            event->received_at_ms,
            &update) != ESP_OK)
    {
        return;
    }
    publish_control_update(&update);
    if (!success)
    {
        publish_unlock_result(&request,
                              BLE_UNLOCK_RESULT_WRITE_FAILED,
                              BLE_UNLOCK_WRITE_CALLBACK_FAILED);
#if !SCENIC_AREA_MANAGEMENT_DEBUG
        schedule_automatic_authorization_retry(
            BLE_UNLOCK_WRITE_CALLBACK_FAILED);
#endif
    }
}

static void handle_unlock_status_evidence(const ble_link_token_t *link_token)
{
    if (link_token == NULL || !s_unlock_context.active ||
        !s_unlock_context.callback_succeeded)
    {
        return;
    }
    const ble_unlock_request_t request = s_unlock_context.request;
    if (link_token->link_generation != request.link_generation ||
        memcmp(link_token->mac,
               request.exoskeleton_mac,
               sizeof(link_token->mac)) != 0)
    {
        return;
    }
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    if (!ble_unlock_accept_status(&s_unlock_context,
                                  &request,
                                  s_status_owner.diagnostics.valid_frame_count,
                                  now_ms))
    {
        return;
    }
    watch_control_update_t update = {0};
    if (control_gate_apply_status_evidence(
            &s_control_gate,
            request.request_id,
            link_token->mac,
            link_token->link_generation,
            s_status_owner.diagnostics.valid_frame_count,
            now_ms,
            &update) == ESP_OK)
    {
        s_poweroff_issued_in_session = false;
        ESP_LOGI(TAG,
                 "BLE 解锁状态证据已确认：请求=%lu，授权会话已建立",
                 (unsigned long)request.request_id);
        publish_control_update(&update);
        publish_unlock_result(&request,
                              BLE_UNLOCK_RESULT_SUCCESS,
                              "BLE_UNLOCKED");
    }
}

static void handle_control_status_evidence(const ble_link_token_t *link_token)
{
    if (link_token == NULL || !s_control_gate.control_pending ||
        !link_token_is_current(link_token) || !s_status_owner.mirror_valid ||
        !s_status_fresh || !s_has_binding ||
        memcmp(link_token->mac, s_bound_mac, sizeof(s_bound_mac)) != 0)
    {
        return;
    }
    if (!control_status_snapshot_is_current(link_token))
    {
        return;
    }
    watch_exoskeleton_state_t actual_state = {0};
    copy_product_state(&actual_state, &s_status_owner.mirror);
    watch_control_update_t update = {0};
    const control_gate_control_action_t applied_action = {
        .type = CONTROL_GATE_CONTROL_ACTION_NONE,
        .request_id = s_control_gate.request_id,
        .link_generation = s_control_gate.link_generation,
        .kind = s_control_gate.control_kind,
        .target = s_control_gate.control_target,
        .attempt = s_control_gate.control_attempt,
    };
    control_gate_control_action_t bound_action = applied_action;
    memcpy(bound_action.exoskeleton_mac, s_control_gate.exoskeleton_mac,
           sizeof(bound_action.exoskeleton_mac));
    if (s_control_gate.control_phase ==
            WATCH_CONTROL_PHASE_POWEROFF_PENDING &&
        s_control_gate.control_kind == WATCH_CONTROL_KIND_POWEROFF)
    {
        control_gate_t terminal_gate = s_control_gate;
        const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
        if (control_gate_apply_poweroff_online_evidence(
                &terminal_gate,
                bound_action.request_id,
                link_token->mac,
                link_token->link_generation,
                s_valid_status_sequence,
                true,
                link_token->link_generation == s_control_gate.link_generation,
                now_ms,
                &update) != ESP_OK)
        {
            return;
        }
        if (!publish_control_update_confirmed(&update))
        {
            ESP_LOGE(TAG,
                     "关机在线终态应用确认失败，请求=%lu",
                     (unsigned long)bound_action.request_id);
            return;
        }
        s_control_gate = terminal_gate;
        publish_poweroff_terminal(
            &bound_action,
            WATCH_POWEROFF_RESULT_FAILED_RECONNECTED);
        return;
    }
    if (s_control_gate.control_phase !=
        WATCH_CONTROL_PHASE_SUBMITTED_UNCONFIRMED)
    {
        return;
    }
    control_gate_t applied_gate = {0};
    if (control_gate_prepare_control_status_evidence(
            &s_control_gate,
            bound_action.request_id,
            link_token->mac,
            link_token->link_generation,
            bound_action.kind,
            bound_action.target,
            s_valid_status_sequence,
            &actual_state,
            true,
            &applied_gate,
            &update) != ESP_OK)
    {
        return;
    }
    if (!publish_control_update_confirmed(&update))
    {
        ESP_LOGE(TAG,
                 "普通控制 APPLIED 状态应用确认失败，请求=%lu",
                 (unsigned long)bound_action.request_id);
        return;
    }
    s_control_gate = applied_gate;
    ESP_LOGI(TAG,
             "BLE 控制状态证据已确认：请求=%lu，类型=%d，目标=%u，结果=APPLIED",
             (unsigned long)bound_action.request_id,
             (int)bound_action.kind,
             (unsigned)bound_action.target);
    publish_control_result(&bound_action, NULL,
                           BLE_CONTROL_RESULT_APPLIED,
                           "BLE_CONTROL_APPLIED");
}

static void check_unlock_timeout(void)
{
    if (!s_unlock_context.active)
    {
        return;
    }
    const ble_unlock_request_t request = s_unlock_context.request;
    const uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
    if (!s_unlock_context.callback_succeeded)
    {
        if (!ble_unlock_expire_write_callback(&s_unlock_context, now_ms))
        {
            return;
        }
        watch_control_update_t update = {0};
        if (control_gate_apply_write_result(
                &s_control_gate,
                request.request_id,
                request.exoskeleton_mac,
                request.link_generation,
                false,
                s_status_owner.diagnostics.valid_frame_count,
                now_ms,
                &update) == ESP_OK)
        {
            publish_control_update(&update);
            publish_unlock_result(&request,
                                  BLE_UNLOCK_RESULT_WRITE_FAILED,
                                  BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT);
        }
        ESP_LOGE(TAG,
                 "BLE 一次性解锁写 callback 超时，请求=%lu，稳定码=%s",
                 (unsigned long)request.request_id,
                 BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT);
        if (recover_nimble_runtime(BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT) != ESP_OK)
        {
            ESP_LOGE(TAG,
                     "BLE 写 callback 超时后 host 隔离失败，稳定码=%s",
                     BLE_HOST_STOP_FAILED);
        }
        return;
    }
    if (!ble_unlock_expire(&s_unlock_context, now_ms))
    {
        return;
    }
    watch_control_update_t update = {0};
    if (control_gate_check_timeout(&s_control_gate, now_ms, &update) ==
        ESP_ERR_TIMEOUT)
    {
        publish_control_update(&update);
        publish_unlock_result(&request,
                              BLE_UNLOCK_RESULT_CONFIRM_TIMEOUT,
                              BLE_UNLOCK_CONFIRM_TIMEOUT);
#if !SCENIC_AREA_MANAGEMENT_DEBUG
        schedule_automatic_authorization_retry(BLE_UNLOCK_CONFIRM_TIMEOUT);
#endif
    }
}

static void abort_unlock_transaction(const char *error_code,
                                     bool invalidate_session)
{
#if SCENIC_AREA_MANAGEMENT_DEBUG
    cancel_cloud_rental_session();
    clear_pending_cloud_rental_submission();
#endif
    const bool ordinary_control = s_control_gate.control_pending;
    const bool unlock_callback_outstanding =
        unlock_callback_watchdog_must_survive_abort(&s_unlock_context);
    const control_gate_control_action_t control_action = {
        .type = CONTROL_GATE_CONTROL_ACTION_BLE_WRITE,
        .request_id = s_control_gate.request_id,
        .link_generation = s_control_gate.link_generation,
        .kind = s_control_gate.control_kind,
        .target = s_control_gate.control_target,
        .attempt = s_control_gate.control_attempt,
    };
    control_gate_control_action_t bound_control_action = control_action;
    memcpy(bound_control_action.exoskeleton_mac,
           s_control_gate.exoskeleton_mac,
           sizeof(bound_control_action.exoskeleton_mac));
    const ble_unlock_request_t request = {
        .intent_sequence = s_authorization_intent_sequence,
        .request_id = s_control_gate.request_id,
        .link_generation = s_control_gate.link_generation,
    };
    ble_unlock_request_t bound_request = request;
    memcpy(bound_request.exoskeleton_mac,
           s_control_gate.exoskeleton_mac,
           sizeof(bound_request.exoskeleton_mac));
    control_gate_t candidate_gate = s_control_gate;
    watch_control_update_t update = {0};
    const bool aborted = invalidate_session
                             ? control_gate_abort_and_invalidate_session(
                                   &candidate_gate,
                                   &update)
                             : control_gate_abort_pending(&candidate_gate,
                                                          &update);
    if (!unlock_callback_outstanding)
    {
        ble_unlock_cancel(&s_unlock_context);
    }
    if (!aborted && !invalidate_session)
    {
        return;
    }
    if (ordinary_control)
    {
        if (!publish_control_update_confirmed(&update))
        {
            return;
        }
        s_control_gate = candidate_gate;
        /* 产品 pending 可立即结束，但 procedure context 必须等 callback 或 host 隔离。 */
        if (!control_callback_watchdog_must_survive_abort(
                s_control_write_callback_pending))
        {
            s_control_write_started_ms = 0U;
            memset(&s_control_write_action, 0, sizeof(s_control_write_action));
        }
        if (update.poweroff_result != WATCH_POWEROFF_RESULT_NONE)
        {
            publish_poweroff_terminal(&bound_control_action,
                                      update.poweroff_result);
        }
        else
        {
            publish_control_result(&bound_control_action,
                                   NULL,
                                   BLE_CONTROL_RESULT_BLOCKED,
                                   error_code != NULL
                                       ? error_code
                                       : BLE_CONTROL_REVALIDATION_FAILED);
        }
        return;
    }
    s_control_gate = candidate_gate;
    publish_control_update(&update);
    if (aborted)
    {
        publish_unlock_result(&bound_request,
                              BLE_UNLOCK_RESULT_LINK_CHANGED,
                              error_code != NULL ? error_code
                                                 : BLE_UNLOCK_LINK_CHANGED);
    }
#if !SCENIC_AREA_MANAGEMENT_DEBUG
    if (aborted && !invalidate_session)
    {
        schedule_automatic_authorization_retry(error_code);
    }
#endif
}

static void publish_control_update(const watch_control_update_t *update)
{
    if (update == NULL)
    {
        return;
    }
    s_pending_control_update = *update;
    s_pending_control_update_dirty = true;
    if (state_service_publish_control(update, 0) == ESP_OK)
    {
        s_pending_control_update_dirty = false;
        return;
    }
    ESP_LOGW(TAG,
             "control 状态队列暂满，已保留最新真值，序号=%lu",
             (unsigned long)update->control_update_sequence);
}

static bool publish_control_update_confirmed(
    const watch_control_update_t *update)
{
    if (update == NULL)
    {
        return false;
    }
    const esp_err_t error = state_service_publish_control_confirmed(
        update,
        pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (error != ESP_OK && !control_update_matches_snapshot(update))
    {
        ESP_LOGE(TAG,
                 "control 状态应用确认失败，序号=%lu，错误=0x%x",
                 (unsigned long)update->control_update_sequence,
                 (unsigned)error);
        return false;
    }
    if (error != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "control apply ACK 丢失但快照已确认，序号=%lu",
                 (unsigned long)update->control_update_sequence);
    }
    if (s_pending_control_update_dirty &&
        control_update_sequence_is_at_or_before(
            s_pending_control_update.control_update_sequence,
            update->control_update_sequence))
    {
        s_pending_control_update_dirty = false;
    }
    return true;
}

static bool control_update_matches_snapshot(
    const watch_control_update_t *update)
{
    if (update == NULL)
    {
        return false;
    }
    memset(&s_verification_product_state,
           0,
           sizeof(s_verification_product_state));
    if (watch_state_snapshot(&s_verification_product_state, 0) != ESP_OK)
    {
        return false;
    }
    return s_verification_product_state.control_update_sequence ==
               update->control_update_sequence &&
           s_verification_product_state.control_request_id ==
               update->request_id &&
           s_verification_product_state.control_link_generation ==
               update->link_generation &&
           memcmp(s_verification_product_state.control_exoskeleton_mac,
                  update->exoskeleton_mac,
                  sizeof(s_verification_product_state.control_exoskeleton_mac)) == 0 &&
           s_verification_product_state.rental_phase == update->rental_phase &&
           s_verification_product_state.control_kind == update->control_kind &&
           s_verification_product_state.control_target == update->control_target &&
           s_verification_product_state.control_phase == update->control_phase &&
           s_verification_product_state.control_last_result ==
               update->control_last_result &&
           s_verification_product_state.control_attempt ==
               update->control_attempt &&
           s_verification_product_state.control_ack_deadline_ms ==
               update->control_ack_deadline_ms &&
           s_verification_product_state.control_locked_out ==
               update->control_locked_out &&
           s_verification_product_state.poweroff_result == update->poweroff_result &&
           s_verification_product_state.poweroff_deadline_ms ==
               update->poweroff_deadline_ms &&
           s_verification_product_state.control_block_reason ==
               update->block_reason &&
           s_verification_product_state.last_rental_result ==
               update->last_rental_result &&
           s_verification_product_state.pending_control ==
               update->pending_control &&
           s_verification_product_state.unlock_session_valid ==
               update->unlock_session_valid &&
           memcmp(s_verification_product_state.control_error_code,
                  update->control_error_code,
                  sizeof(s_verification_product_state.control_error_code)) == 0;
}

static bool control_status_snapshot_is_current(
    const ble_link_token_t *link_token)
{
    if (link_token == NULL)
    {
        return false;
    }
    memset(&s_verification_product_state,
           0,
           sizeof(s_verification_product_state));
    if (watch_state_snapshot(&s_verification_product_state, 0) != ESP_OK)
    {
        return false;
    }
    return s_verification_product_state.has_bound_exoskeleton &&
           s_verification_product_state.ble_connected &&
           s_verification_product_state.device_state_available &&
           s_verification_product_state.dataReady &&
           s_verification_product_state.status_fresh &&
           s_verification_product_state.link_control_ready &&
           s_verification_product_state.ble_link_generation ==
               link_token->link_generation &&
           s_verification_product_state.last_status_rx_ms ==
               s_status_owner.last_status_rx_ms &&
           memcmp(s_verification_product_state.bound_exoskeleton_mac,
                  link_token->mac,
                  sizeof(s_verification_product_state.bound_exoskeleton_mac)) == 0;
}

static bool control_update_sequence_is_at_or_before(uint32_t candidate,
                                                    uint32_t baseline)
{
    return candidate == baseline || (int32_t)(candidate - baseline) < 0;
}

static void publish_unlock_result(const ble_unlock_request_t *request,
                                  ble_unlock_result_code_t result,
                                  const char *error_code)
{
    ble_unlock_result_t output = {.result = result};
    if (request != NULL)
    {
        output.intent_sequence = request->intent_sequence;
        output.request_id = request->request_id;
        output.link_generation = request->link_generation;
        memcpy(output.exoskeleton_mac,
               request->exoskeleton_mac,
               sizeof(output.exoskeleton_mac));
    }
    if (error_code == NULL ||
        strnlen(error_code, sizeof(output.error_code)) >=
            sizeof(output.error_code))
    {
        error_code = "BLE_UNLOCK_RESULT_INVALID";
    }
    memcpy(output.error_code, error_code, strlen(error_code) + 1U);
    if (s_pending_unlock_result_count == 0U && s_unlock_result_queue != NULL &&
        xQueueSend(s_unlock_result_queue, &output, 0) == pdTRUE)
    {
        return;
    }
    if (s_pending_unlock_result_count >= BLE_UNLOCK_PENDING_RESULT_DEPTH)
    {
        atomic_store(&s_unlock_result_backpressured, true);
        ESP_LOGE(TAG,
                 "BLE 解锁终态 FIFO 已满，请求=%lu，稳定码=%s",
                 (unsigned long)output.request_id,
                 BLE_EVENT_QUEUE_OVERFLOW);
        return;
    }
    const size_t tail =
        (s_pending_unlock_result_head + s_pending_unlock_result_count) %
        BLE_UNLOCK_PENDING_RESULT_DEPTH;
    s_pending_unlock_results[tail] = output;
    ++s_pending_unlock_result_count;
    atomic_store(&s_unlock_result_backpressured, true);
    ESP_LOGW(TAG,
             "BLE 解锁终态队列暂满，已保留请求=%lu",
             (unsigned long)output.request_id);
}

static void publish_control_result(
    const control_gate_control_action_t *action,
    const ble_control_intent_t *intent,
    ble_control_result_code_t result,
    const char *error_code)
{
    ble_control_result_t output = {.result = result};
    if (action != NULL)
    {
        output.request_id = action->request_id;
        output.link_generation = action->link_generation;
        output.kind = action->kind;
        output.target = action->target;
        output.attempt = action->attempt;
        memcpy(output.exoskeleton_mac,
               action->exoskeleton_mac,
               sizeof(output.exoskeleton_mac));
    }
    else if (intent != NULL)
    {
        output.kind = intent->kind;
        output.target = intent->target;
    }
    if (error_code == NULL ||
        strnlen(error_code, sizeof(output.error_code)) >=
            sizeof(output.error_code) ||
        strncmp(error_code, "BLE_", 4U) != 0)
    {
        error_code = "BLE_CONTROL_RESULT_INVALID";
    }
    memcpy(output.error_code, error_code, strlen(error_code) + 1U);
    if (s_pending_control_result_count == 0U &&
        s_control_result_queue != NULL &&
        xQueueSend(s_control_result_queue, &output, 0) == pdTRUE)
    {
        return;
    }
    if (s_pending_control_result_count >= BLE_CONTROL_PENDING_RESULT_DEPTH)
    {
        atomic_store(&s_control_result_backpressured, true);
        ESP_LOGE(TAG,
                 "BLE 普通控制终态 FIFO 已满，请求=%lu，稳定码=%s",
                 (unsigned long)output.request_id,
                 BLE_EVENT_QUEUE_OVERFLOW);
        return;
    }
    const size_t tail =
        (s_pending_control_result_head + s_pending_control_result_count) %
        BLE_CONTROL_PENDING_RESULT_DEPTH;
    s_pending_control_results[tail] = output;
    ++s_pending_control_result_count;
    atomic_store(&s_control_result_backpressured, true);
    ESP_LOGW(TAG,
             "BLE 普通控制终态队列暂满，已保留请求=%lu",
             (unsigned long)output.request_id);
}

static void flush_dirty_diagnostics(void)
{
    if (!s_diagnostic_publish_dirty)
    {
        return;
    }
    const uint32_t now_ms = monotonic_ms32();
    if ((uint32_t)(now_ms - s_last_diagnostic_publish_ms) <
        BLE_DIAGNOSTIC_PUBLISH_INTERVAL_MS)
    {
        return;
    }
    publish_ble_state(s_transaction, BLE_OK);
    s_last_diagnostic_publish_ms = now_ms;
    s_diagnostic_publish_dirty = false;
}

static bool event_is_current_link(const ble_private_event_t *event)
{
    return event != NULL && s_link_connected &&
           event->conn_handle == s_connection_handle &&
           event->link_generation == s_link_generation;
}

static void check_admission_deadline(void)
{
    if (s_admission_deadline == 0U ||
        s_admission_state == BLE_ADMISSION_IDLE ||
        s_admission_state == BLE_ADMISSION_NOTIFY_READY)
    {
        return;
    }
    if ((int32_t)(xTaskGetTickCount() - s_admission_deadline) >= 0)
    {
        ESP_LOGE(TAG,
                 "BLE 链路准入超时，隔离 host 以回收未返回的 GATT callback 上下文");
        if (recover_nimble_runtime(BLE_LINK_ADMISSION_TIMEOUT) != ESP_OK)
        {
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_LINK_ADMISSION_TIMEOUT);
        }
    }
}

static void fail_current_link(const char *error_code)
{
    if (!s_link_connected || s_connection_handle == UINT16_MAX)
    {
        return;
    }
    fail_active_selection();
    s_disconnect_diagnostic = error_code;
    reset_link_admission();
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, error_code);
    ESP_LOGE(TAG, "BLE 当前链路准入失败，稳定码=%s", error_code);
    int terminate_result = ble_gap_terminate(s_connection_handle,
                                             BLE_ERR_REM_USER_CONN_TERM);
    if (terminate_result == 0)
    {
        return;
    }
    if (terminate_result != BLE_HS_ENOTCONN)
    {
        if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
        {
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_DISCONNECT_FAILED);
        }
        return;
    }
    invalidate_link_generation();
    clear_pending_binding();
    s_link_connected = false;
    s_connection_handle = UINT16_MAX;
    restart_scan_after_connection_failure(error_code);
}

static void reset_link_admission(void)
{
    reset_status_stream();
    s_admission_state = BLE_ADMISSION_IDLE;
    s_admission_deadline = 0;
    s_last_invalid_frame_diagnostic_ms = 0U;
    s_invalid_frame_diagnostic_logged = false;
    s_target_service_count = 0U;
    s_service_start_handle = 0U;
    s_service_end_handle = 0U;
    s_target_characteristic_count = 0U;
    s_characteristic_def_handle = 0U;
    s_characteristic_val_handle = 0U;
    s_next_characteristic_def_handle = 0U;
    s_characteristic_properties = 0U;
    s_target_cccd_count = 0U;
    s_cccd_handle = 0U;
    s_notify_attempt_count = 0U;
    s_gatt_ready = false;
    s_mtu_ready = false;
    s_negotiated_mtu = 0U;
    s_observed_att_mtu = 0U;
    s_notify_ready = false;
    s_data_ready = false;
    s_status_fresh = false;
    s_link_control_ready = false;
    s_connection_target_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    s_connection_pending_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    s_connection_applied_profile =
        BLE_CONNECTION_POWER_PROFILE_UNKNOWN;
    atomic_store(&s_callback_value_handle, UINT16_MAX);
    atomic_store(&s_callback_notify_ready, false);
}

static void invalidate_link_generation(void)
{
#if SCENIC_AREA_MANAGEMENT_DEBUG
    cancel_cloud_rental_session();
    if (s_link_generation != 0U)
    {
        (void)cloud_service_authorized_link_end(s_link_generation);
    }
    clear_pending_cloud_rental_submission();
#endif
    reset_status_stream();
    s_data_ready = false;
    s_status_fresh = false;
    s_link_control_ready = false;
    s_automatic_authorization_pending = false;
    s_automatic_authorization_attempt_generation = 0U;
    s_automatic_authorization_attempt_count = 0U;
    s_automatic_authorization_retry_at_ms = 0U;
    ++s_link_generation;
    if (s_link_generation == 0U)
    {
        ++s_link_generation;
    }
    atomic_store(&s_callback_link_generation, s_link_generation);
    atomic_store(&s_callback_conn_handle, UINT16_MAX);
}

static void reset_gatt_callback_contexts(void)
{
    for (size_t index = 0; index < BLE_GATT_CALLBACK_CONTEXT_COUNT; ++index)
    {
        atomic_store(&s_gatt_callback_contexts[index].in_use, false);
        s_gatt_callback_contexts[index].conn_handle = UINT16_MAX;
        s_gatt_callback_contexts[index].link_generation = 0U;
    }
}

static void handle_mtu_evidence(const ble_private_event_t *evidence)
{
    /* MTU 证据仅供诊断；不参与绑定提交或控制准入。 */
    if (!event_is_current_link(evidence) ||
        s_admission_state == BLE_ADMISSION_IDLE)
    {
        return;
    }
    if (!s_gatt_ready || !s_mtu_ready ||
        evidence->mtu != s_negotiated_mtu)
    {
        publish_ble_state(s_transaction, BLE_BINDING_EVIDENCE_REJECTED);
        return;
    }
    if (!s_pending_binding.active)
    {
        if (!evidence_matches_bound_link(evidence))
        {
            publish_ble_state(s_transaction, BLE_BINDING_EVIDENCE_REJECTED);
        }
        return;
    }
    if (!s_pending_binding.connected ||
        evidence->conn_handle != s_pending_binding.conn_handle ||
        strcmp(evidence->mac, s_pending_binding.candidate.mac) != 0)
    {
        publish_ble_state(s_transaction, BLE_BINDING_EVIDENCE_REJECTED);
        return;
    }
}

static void handle_frame_evidence(const ble_private_event_t *evidence)
{
    if (!event_is_current_link(evidence) || !s_notify_ready ||
        s_admission_state != BLE_ADMISSION_NOTIFY_READY)
    {
        if (s_pending_binding.active)
        {
            fail_pending_binding(BLE_BINDING_EVIDENCE_REJECTED);
        }
        else
        {
            publish_ble_state(s_transaction, BLE_BINDING_EVIDENCE_REJECTED);
        }
        return;
    }
    if (!s_pending_binding.active)
    {
        if (!evidence_matches_bound_link(evidence))
        {
            publish_ble_state(s_transaction, BLE_BINDING_EVIDENCE_REJECTED);
        }
        return;
    }
    if (!s_pending_binding.connected ||
        evidence->conn_handle != s_pending_binding.conn_handle ||
        strcmp(evidence->mac, s_pending_binding.candidate.mac) != 0 ||
        !evidence->fully_valid)
    {
        fail_pending_binding(BLE_BINDING_EVIDENCE_REJECTED);
        return;
    }
    s_pending_binding.first_valid_frame = true;
    try_commit_binding();
}

static void try_commit_binding(void)
{
    if (!s_pending_binding.active || !s_pending_binding.connected ||
        !s_pending_binding.first_valid_frame || !s_gatt_ready ||
        !s_notify_ready ||
        s_admission_state != BLE_ADMISSION_NOTIFY_READY)
    {
        return;
    }
    esp_err_t err = config_service_set_bound_exoskeleton_mac(
        s_pending_binding.candidate.mac);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "BLE 绑定提交失败，稳定码=%s，错误=0x%x",
                 BLE_BINDING_COMMIT_FAILED,
                 (unsigned)err);
        fail_pending_binding(BLE_BINDING_COMMIT_FAILED);
        return;
    }
    s_has_binding = true;
    s_binding_scan_session_active = false;
    memcpy(s_bound_mac,
           s_pending_binding.candidate.mac,
           sizeof(s_bound_mac));
    ble_recovery_policy_reset(&s_recovery_policy);
    s_recovery_in_progress = false;
    s_active_selection_pending = false;
    memset(&s_active_selection_command, 0, sizeof(s_active_selection_command));
    invalidate_scan_generation();
    clear_candidate_session_model();
    ESP_LOGI(TAG, "BLE 绑定已提交，MAC=%s", s_bound_mac);
    publish_ble_state(WATCH_BLE_TRANSACTION_BOUND_READY, BLE_OK);
    clear_pending_binding();
}

static void fail_pending_binding(const char *error_code)
{
    fail_active_selection();
    clear_pending_binding();
    reset_link_admission();
    clear_status_mirror();
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, error_code);
    if (s_connecting)
    {
        int cancel_result = ble_gap_conn_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            ESP_LOGE(TAG, "BLE 失败连接取消请求被拒绝，错误=%d", cancel_result);
            if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
            {
                publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                  BLE_DISCONNECT_FAILED);
            }
        }
        return;
    }
    if (s_link_connected && s_connection_handle != UINT16_MAX)
    {
        int terminate_result = ble_gap_terminate(s_connection_handle,
                                                 BLE_ERR_REM_USER_CONN_TERM);
        if (terminate_result == 0)
        {
            return;
        }
        if (terminate_result != BLE_HS_ENOTCONN)
        {
            ESP_LOGE(TAG,
                     "BLE 失败候选终止请求被拒绝，稳定码=%s，错误=%d",
                     BLE_DISCONNECT_FAILED,
                     terminate_result);
            if (recover_nimble_runtime(BLE_DISCONNECT_FAILED) != ESP_OK)
            {
                publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                                  BLE_DISCONNECT_FAILED);
            }
            return;
        }
        invalidate_link_generation();
        s_link_connected = false;
        s_connection_handle = UINT16_MAX;
    }
    restart_scan_after_connection_failure(error_code);
}

static void clear_pending_binding(void)
{
    memset(&s_pending_binding, 0, sizeof(s_pending_binding));
}

static bool evidence_matches_bound_link(const ble_private_event_t *evidence)
{
    return evidence != NULL && !s_pending_binding.active && s_has_binding &&
           event_is_current_link(evidence) &&
           strcmp(evidence->mac, s_bound_mac) == 0;
}

static bool mac_bytes_are_ambiguous(const uint8_t value[6])
{
    for (size_t index = 0; index < s_ambiguous_mac_count; ++index)
    {
        if (memcmp(s_ambiguous_macs[index], value, sizeof(s_ambiguous_macs[index])) == 0)
        {
            return true;
        }
    }
    return false;
}

static void mark_mac_bytes_ambiguous(const uint8_t value[6])
{
    if (mac_bytes_are_ambiguous(value))
    {
        return;
    }
    invalidate_current_authorization_proof();
    if (s_ambiguous_mac_count >= MAINTENANCE_BLE_CANDIDATE_MAX_COUNT)
    {
        s_identity_conflict_fail_closed = true;
        memset(s_candidates, 0, sizeof(s_candidates));
        s_candidate_count = 0U;
        return;
    }
    memcpy(s_ambiguous_macs[s_ambiguous_mac_count],
           value,
           sizeof(s_ambiguous_macs[s_ambiguous_mac_count]));
    ++s_ambiguous_mac_count;
}

static void remember_connectable_observation(const ble_private_event_t *event)
{
    if (event == NULL)
    {
        return;
    }
    for (size_t index = 0; index < s_connectable_address_count; ++index)
    {
        ble_connectable_observation_t *observation = &s_connectable_observations[index];
        if (observation->address.type != event->address.type ||
            memcmp(observation->address.val, event->address.val, sizeof(event->address.val)) != 0)
        {
            continue;
        }
        if (event->rssi != MAINTENANCE_BLE_RSSI_UNAVAILABLE)
        {
            observation->rssi = event->rssi;
        }
        if (event->advertised_name[0] != '\0')
        {
            memcpy(observation->advertised_name, event->advertised_name, sizeof(observation->advertised_name));
        }
        return;
    }
    size_t insertion_index = s_connectable_address_count;
    if (s_connectable_address_count < BLE_CONNECTABLE_CACHE_CAPACITY)
    {
        ++s_connectable_address_count;
    }
    else
    {
        insertion_index = s_connectable_address_next;
        s_connectable_address_next = (s_connectable_address_next + 1U) % BLE_CONNECTABLE_CACHE_CAPACITY;
    }
    s_connectable_observations[insertion_index].address = event->address;
    s_connectable_observations[insertion_index].rssi = event->rssi;
    memcpy(s_connectable_observations[insertion_index].advertised_name, event->advertised_name,
           sizeof(s_connectable_observations[insertion_index].advertised_name));
}

static const ble_connectable_observation_t *find_connectable_observation(const ble_addr_t *address)
{
    if (address == NULL)
    {
        return NULL;
    }
    for (size_t index = 0; index < s_connectable_address_count; ++index)
    {
        if (s_connectable_observations[index].address.type == address->type &&
            memcmp(s_connectable_observations[index].address.val, address->val, sizeof(address->val)) == 0)
        {
            return &s_connectable_observations[index];
        }
    }
    return NULL;
}

static bool connectable_address_seen(const ble_addr_t *address)
{
    return find_connectable_observation(address) != NULL;
}

static void reset_transport_runtime(void)
{
    abort_unlock_transaction(BLE_UNLOCK_LINK_CHANGED, false);
    cancel_deferred_poweroff(BLE_CONTROL_REVALIDATION_FAILED);
    ble_recovery_policy_cancel(&s_recovery_policy);
    s_recovery_in_progress = false;
    ++s_recovery_generation;
    if (s_recovery_generation == 0U)
    {
        ++s_recovery_generation;
    }
    invalidate_link_generation();
    s_automatic_authorization_pending = false;
    s_automatic_authorization_attempt_generation = 0U;
    s_automatic_authorization_attempt_count = 0U;
    s_automatic_authorization_retry_at_ms = 0U;
    reset_link_admission();
    s_scanning = false;
    s_binding_scan_cancel_pending = false;
    s_screen_scan_cancel_pending = false;
    s_selftest_scan_cancel_pending = false;
    s_connecting = false;
    s_connecting_tx_power_boost = false;
    s_active_connection_tx_power_boost = false;
    s_selftest_connection_cancel_pending = false;
    memset(s_candidates, 0, sizeof(s_candidates));
    s_candidate_count = 0U;
    memset(s_ambiguous_macs, 0, sizeof(s_ambiguous_macs));
    s_ambiguous_mac_count = 0U;
    s_identity_conflict_fail_closed = false;
    memset(s_connectable_observations, 0, sizeof(s_connectable_observations));
    s_connectable_address_count = 0U;
    s_connectable_address_next = 0U;
    s_candidate_snapshot_dirty = false;
    s_scan_diagnostic = BLE_OK;
    s_disconnect_diagnostic = BLE_OK;
    clear_pending_binding();
    s_link_connected = false;
    s_connection_handle = UINT16_MAX;
    s_restart_scan_after_clear = false;
    memset(&s_pending_selection_command, 0, sizeof(s_pending_selection_command));
    s_pending_selection_feedback_dirty = false;
    memset(&s_active_selection_command, 0, sizeof(s_active_selection_command));
    s_active_selection_pending = false;
}

static void reset_service_runtime(void)
{
    reset_transport_runtime();
    ble_tx_power_policy_reset(&s_tx_power_policy);
    s_binding_scan_session_active = false;
    s_candidate_session_clear_dirty = false;
    s_candidate_session_clear_generation = 0U;
    ble_recovery_policy_reset(&s_recovery_policy);
    /* 服务停止/重启仅作废物理链路，同 MAC 最近有效值保留只读。 */
    ble_status_owner_reset_link(&s_status_owner);
    s_scan_generation = 0U;
    s_link_generation = 0U;
    s_automatic_authorization_pending = false;
    s_automatic_authorization_attempt_generation = 0U;
    s_automatic_authorization_attempt_count = 0U;
    s_automatic_authorization_retry_at_ms = 0U;
    atomic_store(&s_callback_link_generation, 0U);
    atomic_store(&s_callback_conn_handle, UINT16_MAX);
    reset_gatt_callback_contexts();
    reset_unlock_callback_contexts();
    reset_control_callback_contexts();
    control_gate_init(&s_control_gate);
    s_authorization_intent_sequence = 0U;
    s_poweroff_issued_in_session = false;
    s_valid_status_sequence = 0U;
    ble_unlock_cancel(&s_unlock_context);
    memset(&s_pending_control_update, 0, sizeof(s_pending_control_update));
    s_pending_control_update_dirty = false;
    memset(&s_runtime_product_state, 0, sizeof(s_runtime_product_state));
    memset(&s_verification_product_state,
           0,
           sizeof(s_verification_product_state));
    memset(s_pending_unlock_results, 0, sizeof(s_pending_unlock_results));
    s_pending_unlock_result_head = 0U;
    s_pending_unlock_result_count = 0U;
    atomic_store(&s_unlock_result_backpressured, false);
    s_control_write_callback_pending = false;
    s_control_write_started_ms = 0U;
    memset(&s_control_write_action, 0, sizeof(s_control_write_action));
    memset(s_pending_control_results, 0, sizeof(s_pending_control_results));
    s_pending_control_result_head = 0U;
    s_pending_control_result_count = 0U;
    atomic_store(&s_control_result_backpressured, false);
    atomic_store(&s_unlock_intent_sequence, 0U);
    atomic_store(&s_control_intent_sequence, 0U);
    atomic_store(&s_poweroff_mailbox_sequence, 0U);
    s_poweroff_cancel_through_sequence = 0U;
    s_poweroff_cancel_through_valid = false;
    memset(&s_deferred_poweroff_intent, 0,
           sizeof(s_deferred_poweroff_intent));
    s_deferred_poweroff_valid = false;
    s_has_binding = false;
    memset(s_bound_mac, 0, sizeof(s_bound_mac));
    s_transaction = WATCH_BLE_TRANSACTION_UNBOUND;
    s_discovery_suspended_for_selftest = false;
    s_selftest_snapshot_retry_pending = false;
    s_screen_state_known = false;
    s_screen_on = true;
    s_applied_screen_sequence = 0U;
    s_radio_start_deadline_ms = 0U;
    atomic_store(&s_pending_screen_sequence, 0U);
    atomic_store(&s_pending_screen_on, true);
    atomic_store(&s_binding_scan_request,
                 BLE_BINDING_SCAN_REQUEST_NONE);
    s_diagnostic_publish_dirty = false;
    s_last_diagnostic_publish_ms = 0U;
}

static esp_err_t recover_nimble_runtime(const char *error_code)
{
    fail_active_selection();
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, error_code);
    esp_err_t stop_error = stop_nimble();
    if (stop_error != ESP_OK)
    {
        return stop_error;
    }
    /* host 已完全停止后才可安全复用旧 procedure 的 callback context。 */
    reset_gatt_callback_contexts();
    reset_unlock_callback_contexts();
    reset_control_callback_contexts();
    ble_unlock_cancel(&s_unlock_context);
    s_control_write_callback_pending = false;
    s_control_write_started_ms = 0U;
    memset(&s_control_write_action, 0, sizeof(s_control_write_action));
    reset_transport_runtime();
    invalidate_current_authorization_proof();
    if (s_event_queue != NULL)
    {
        ble_private_event_t queued_event = {0};
        bool stop_requested = false;
        while (xQueueReceive(s_event_queue, &queued_event, 0) == pdTRUE)
        {
            stop_requested = stop_requested || queued_event.type == BLE_PRIVATE_EVENT_STOP;
            if (queued_event.type == BLE_PRIVATE_EVENT_FIRST_UNLOCK_INTENT)
            {
                handle_first_unlock_intent(
                    queued_event.unlock_intent_sequence);
            }
            else if (queued_event.type == BLE_PRIVATE_EVENT_CONTROL_INTENT)
            {
                handle_control_intent(&queued_event);
            }
        }
        if (stop_requested)
        {
            const ble_private_event_t stop_event = {
                .type = BLE_PRIVATE_EVENT_STOP,
            };
            (void)xQueueSend(s_event_queue, &stop_event, 0);
        }
    }
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, error_code);
    if (atomic_load(&s_stop_requested))
    {
        return ESP_ERR_INVALID_STATE;
    }
    esp_err_t start_error = start_nimble();
    if (start_error != ESP_OK)
    {
        ble_nimble_cleanup_record_error(&s_nimble_cleanup, start_error);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_START_FAILED);
    }
    return start_error;
}

static void restart_scan_after_connection_failure(const char *error_code)
{
    publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, error_code);
    if (s_discovery_suspended_for_selftest || s_scanning || s_connecting ||
        s_link_connected)
    {
        return;
    }
    if (s_has_binding)
    {
        schedule_reconnect(error_code);
    }
    else
    {
        (void)try_enter_radio_dormant();
    }
}

static bool normalize_evidence_mac(const char input[BLE_MAC_CAPACITY],
                                   char output[BLE_MAC_CAPACITY])
{
    if (input == NULL || output == NULL ||
        strnlen(input, BLE_MAC_CAPACITY) != BLE_MAC_CAPACITY - 1U)
    {
        return false;
    }
    for (size_t index = 0; index < BLE_MAC_CAPACITY - 1U; ++index)
    {
        const bool separator = index == 2U || index == 5U || index == 8U ||
                               index == 11U || index == 14U;
        if (separator)
        {
            if (input[index] != ':')
            {
                return false;
            }
            output[index] = ':';
        }
        else
        {
            const unsigned char value = (unsigned char)input[index];
            if (!isxdigit(value))
            {
                return false;
            }
            output[index] = (char)toupper(value);
        }
    }
    output[BLE_MAC_CAPACITY - 1U] = '\0';
    return true;
}

static void publish_ble_state(watch_ble_transaction_t transaction,
                              const char *error_code)
{
    s_transaction = transaction;
    watch_ble_update_t update = {
        .has_bound_exoskeleton = s_has_binding,
        .ble_connected = s_link_connected,
        .link_generation = s_link_connected ? s_link_generation : 0U,
        .transaction = transaction,
        .gatt_ready = s_gatt_ready,
        .mtu_ready = s_mtu_ready,
        .negotiated_mtu = s_negotiated_mtu,
        .notify_ready = s_notify_ready,
        .link_control_ready = s_link_control_ready,
        .clear_pending_control = !s_link_control_ready,
        .diagnostics = {
            .valid_frame_count = s_status_owner.diagnostics.valid_frame_count,
            .invalid_frame_count = s_status_owner.diagnostics.invalid_frame_count,
            .discarded_byte_count = s_status_owner.diagnostics.discarded_byte_count,
            .version_sanitized_count =
                s_status_owner.diagnostics.version_sanitized_count,
            .reconnect_attempt_count = s_reconnect_attempt_count,
            .reconnect_success_count = s_reconnect_success_count,
        },
    };
    if (s_has_binding)
    {
        memcpy(update.bound_exoskeleton_mac,
               s_bound_mac,
               sizeof(update.bound_exoskeleton_mac));
    }
    if (s_status_owner.mirror_valid &&
        (!s_has_binding ||
         strcmp(s_status_owner.mirror_token.mac, s_bound_mac) == 0))
    {
        update.device_state_available = true;
        update.dataReady = s_data_ready;
        update.status_fresh = s_status_fresh;
        update.last_status_rx_ms = s_status_owner.last_status_rx_ms;
        copy_product_state(&update.exoskeleton_state, &s_status_owner.mirror);
    }
    if (error_code == NULL ||
        strlen(error_code) >= sizeof(update.ble_error_code))
    {
        error_code = BLE_STATE_QUEUE_FULL;
    }
    if (strcmp(error_code, BLE_OK) != 0)
    {
        s_last_error_code = error_code;
    }
    const char *effective_error = strcmp(error_code, BLE_OK) == 0
                                      ? s_last_error_code
                                      : error_code;
    memcpy(update.ble_error_code,
           effective_error,
           strlen(effective_error) + 1U);
    memcpy(update.diagnostics.last_error_code,
           effective_error,
           strlen(effective_error) + 1U);
    (void)watch_ble_update_store_latest(&s_pending_state_update,
                                        &s_pending_state_update_dirty,
                                        &update);
    esp_err_t err = state_service_publish_ble(&update,
                                              pdMS_TO_TICKS(BLE_EVENT_POLL_MS));
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG,
                 "BLE 状态快照未入队，稳定码=%s，错误=0x%x",
                 BLE_STATE_QUEUE_FULL,
                 (unsigned)err);
        return;
    }
    s_pending_state_update_dirty = false;
}

static void copy_product_state(watch_exoskeleton_state_t *destination,
                               const ble_status_frame_t *source)
{
    if (destination == NULL || source == NULL)
    {
        return;
    }
    const watch_exoskeleton_scene_config_t config =
        (watch_exoskeleton_scene_config_t)source->scene_config;
    uint8_t scene_mode = source->scene_mode;
    if (!watch_exoskeleton_scene_mode_from_protocol_config(config,
                                                    source->scene_mode,
                                                    &scene_mode))
    {
        scene_mode = 0U;
    }
    *destination = (watch_exoskeleton_state_t){
        .current_mode = source->current_mode,
        .steps = source->steps,
        .step_frequency = source->step_frequency,
        .left_gear = source->left_gear,
        .right_gear = source->right_gear,
        .balance_factor = source->balance_factor,
        .motor_enable = source->motor_enable,
        .battery_level = source->battery_level,
        .scene_mode = scene_mode,
        .scene_config = config,
        .firmware_update_flag = source->firmware_update_flag,
        .left_motor_position = source->left_motor_position,
        .right_motor_position = source->right_motor_position,
        .standby_time = source->standby_time,
        .temp_alarm = source->temp_alarm,
    };
    memcpy(destination->version, source->version, sizeof(destination->version));
}

static void flush_pending_ble_state(void)
{
    if (!s_pending_state_update_dirty)
    {
        return;
    }
    if (state_service_publish_ble(&s_pending_state_update, 0) == ESP_OK)
    {
        s_pending_state_update_dirty = false;
    }
}

static bool flush_pending_ble_state_with_retry(void)
{
    for (uint32_t attempt = 0U;
         attempt < BLE_FINAL_STATE_PUBLISH_ATTEMPTS;
         ++attempt)
    {
        if (!s_pending_state_update_dirty)
        {
            return true;
        }
        if (state_service_publish_ble(&s_pending_state_update,
                                      pdMS_TO_TICKS(BLE_EVENT_POLL_MS)) == ESP_OK)
        {
            s_pending_state_update_dirty = false;
            return true;
        }
        taskYIELD();
    }
    ESP_LOGE(TAG,
             "BLE 最终 fail-closed 状态有界重投失败，稳定码=%s",
             BLE_STATE_QUEUE_FULL);
    return false;
}

static esp_err_t connect_candidate(const ble_candidate_t *candidate)
{
    if (candidate == NULL ||
        atomic_load(&s_radio_lifecycle) !=
            BLE_RADIO_LIFECYCLE_READY ||
        s_discovery_suspended_for_selftest ||
        s_connecting || s_link_connected ||
        s_pending_binding.active)
    {
        return ESP_ERR_INVALID_STATE;
    }
    if (s_scanning)
    {
        int cancel_result = ble_gap_disc_cancel();
        if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
        {
            return ESP_FAIL;
        }
        s_scanning = false;
    }
    if (ble_hs_id_infer_auto(0, &s_own_addr_type) != 0)
    {
        return ESP_FAIL;
    }
    if (s_status_owner.mirror_valid &&
        strcmp(s_status_owner.mirror_token.mac, candidate->mac) != 0)
    {
        clear_status_mirror();
    }
    clear_pending_binding();
    if (!s_has_binding)
    {
        s_pending_binding.active = true;
        s_pending_binding.candidate = *candidate;
    }
    if (candidate->rssi != MAINTENANCE_BLE_RSSI_UNAVAILABLE)
    {
        ble_tx_power_policy_note_scan_rssi(&s_tx_power_policy,
                                           candidate->rssi);
    }
    const bool boost_requested =
        ble_tx_power_policy_take_boost(&s_tx_power_policy);
    const bool initiating_power_applied =
        configure_initiating_tx_power(boost_requested);
    const bool boost_applied =
        boost_requested && initiating_power_applied;
    if (boost_requested && !boost_applied)
    {
        ble_tx_power_policy_request_boost(&s_tx_power_policy);
    }
    s_connecting_tx_power_boost = boost_applied;
    /* ble_gap_connect 的 cb_arg 会由成功连接继承，作为 GAP 事件不可变代次。 */
    invalidate_link_generation();
    int result = ble_gap_connect(s_own_addr_type,
                                 &candidate->address,
                                 BLE_CONNECT_TIMEOUT_MS,
                                 NULL,
                                 ble_gap_event_callback,
                                 (void *)(uintptr_t)s_link_generation);
    if (result != 0)
    {
        s_connecting_tx_power_boost = false;
        (void)configure_initiating_tx_power(false);
        if (boost_applied)
        {
            /* NimBLE 同步拒绝时尚未开始物理建链，保留一次性升功率意图。 */
            ble_tx_power_policy_request_boost(&s_tx_power_policy);
        }
        clear_pending_binding();
        return ESP_FAIL;
    }
    s_connecting = true;
    publish_ble_state(WATCH_BLE_TRANSACTION_CONNECTING, BLE_OK);
    if (candidate->rssi == MAINTENANCE_BLE_RSSI_UNAVAILABLE)
    {
        ESP_LOGI(TAG,
                 "BLE 已发起候选连接，MAC=%s，RSSI=不可用，建链功率=%d dBm",
                 candidate->mac,
                 boost_applied ? BLE_TX_POWER_BOOST_DBM
                               : BLE_TX_POWER_DEFAULT_DBM);
    }
    else
    {
        ESP_LOGI(TAG,
                 "BLE 已发起候选连接，MAC=%s，RSSI=%d dBm，建链功率=%d dBm",
                 candidate->mac,
                 (int)candidate->rssi,
                 boost_applied ? BLE_TX_POWER_BOOST_DBM
                               : BLE_TX_POWER_DEFAULT_DBM);
    }
    return ESP_OK;
}

static void format_mac(const ble_addr_t *address,
                       char output[MAINTENANCE_BLE_MAC_CAPACITY])
{
    (void)snprintf(output,
                   MAINTENANCE_BLE_MAC_CAPACITY,
                   "%02X:%02X:%02X:%02X:%02X:%02X",
                   address->val[5],
                   address->val[4],
                   address->val[3],
                   address->val[2],
                   address->val[1],
                   address->val[0]);
}

static void configure_default_tx_power(void)
{
    const esp_err_t default_result = esp_ble_tx_power_set_enhanced(
        ESP_BLE_ENHANCED_PWR_TYPE_DEFAULT,
        0U,
        ESP_PWR_LVL_N0);
    const esp_err_t scan_result = esp_ble_tx_power_set_enhanced(
        ESP_BLE_ENHANCED_PWR_TYPE_SCAN,
        0U,
        ESP_PWR_LVL_N0);
    const esp_err_t initiating_result = esp_ble_tx_power_set_enhanced(
        ESP_BLE_ENHANCED_PWR_TYPE_INIT,
        0U,
        ESP_PWR_LVL_N0);
    if (default_result != ESP_OK || scan_result != ESP_OK ||
        initiating_result != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 默认 0 dBm 发射功率配置未完全生效，默认=0x%x，扫描=0x%x，建链=0x%x",
                 (unsigned)default_result,
                 (unsigned)scan_result,
                 (unsigned)initiating_result);
        return;
    }
    ESP_LOGI(TAG, "BLE 默认、扫描与建链发射功率已配置为 0 dBm");
}

static bool configure_initiating_tx_power(bool boost)
{
    const esp_power_level_t level =
        boost ? ESP_PWR_LVL_P9 : ESP_PWR_LVL_N0;
    const esp_err_t result = esp_ble_tx_power_set_enhanced(
        ESP_BLE_ENHANCED_PWR_TYPE_INIT,
        0U,
        level);
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 建链发射功率配置失败，目标=%d dBm，错误=0x%x",
                 boost ? BLE_TX_POWER_BOOST_DBM
                       : BLE_TX_POWER_DEFAULT_DBM,
                 (unsigned)result);
        return false;
    }
    if (boost)
    {
        ESP_LOGI(TAG, "BLE 下一次物理建链临时使用 +9 dBm");
    }
    return true;
}

static bool configure_connection_tx_power(uint16_t conn_handle, bool boost)
{
    const esp_power_level_t level =
        boost ? ESP_PWR_LVL_P9 : ESP_PWR_LVL_N0;
    const esp_err_t result = esp_ble_tx_power_set_enhanced(
        ESP_BLE_ENHANCED_PWR_TYPE_CONN,
        conn_handle,
        level);
    if (result != ESP_OK)
    {
        ESP_LOGW(TAG,
                 "BLE 连接发射功率配置失败，handle=%u，目标=%d dBm，错误=0x%x",
                 (unsigned)conn_handle,
                 boost ? BLE_TX_POWER_BOOST_DBM
                       : BLE_TX_POWER_DEFAULT_DBM,
                 (unsigned)result);
        return false;
    }
    ESP_LOGI(TAG,
             "BLE 连接发射功率已配置，handle=%u，功率=%d dBm",
             (unsigned)conn_handle,
             boost ? BLE_TX_POWER_BOOST_DBM
                   : BLE_TX_POWER_DEFAULT_DBM);
    return true;
}

static void note_control_retry_for_tx_power(
    const control_gate_control_action_t *action)
{
    if (action == NULL)
    {
        return;
    }
    const bool boost_was_pending =
        s_tx_power_policy.boost_next_connection;
    ble_tx_power_policy_note_control_attempt(&s_tx_power_policy,
                                             action->attempt);
    if (!boost_was_pending && s_tx_power_policy.boost_next_connection)
    {
        ESP_LOGW(TAG,
                 "BLE 同一普通控制已重试两次，下一次物理重连将临时使用 +9 dBm，请求=%lu",
                 (unsigned long)action->request_id);
    }
}

static esp_err_t start_nimble(void)
{
    if (s_nimble_started || s_nimble_cleanup.terminal_error != ESP_OK)
    {
        return s_nimble_cleanup.terminal_error != ESP_OK
                   ? (esp_err_t)s_nimble_cleanup.terminal_error
                   : ESP_ERR_INVALID_STATE;
    }
    if (atomic_load(&s_radio_lifecycle) !=
            BLE_RADIO_LIFECYCLE_DORMANT ||
        s_radio_retention_reserve == NULL)
    {
        atomic_store(&s_radio_lifecycle,
                     BLE_RADIO_LIFECYCLE_STOPPING);
        return ESP_ERR_INVALID_STATE;
    }
    atomic_store(&s_radio_lifecycle,
                 BLE_RADIO_LIFECYCLE_STARTING);
    (void)advance_radio_generation();
    s_radio_start_deadline_ms = 0U;
    ESP_LOGI(TAG,
             "NimBLE Host 初始化前内存：内部空闲=%lu，内部最大块=%lu，RETENTION 空闲=%lu，RETENTION 最大块=%lu，PSRAM 空闲=%lu，PSRAM 最大块=%lu",
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_RETENTION),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_RETENTION),
             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT),
             (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    /* 保留块必须紧邻 controller 初始化前释放，避免其他任务抢占连续区。 */
    release_radio_retention();
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK)
    {
        s_radio_start_deadline_ms = 0U;
        if (reserve_radio_retention() == ESP_OK)
        {
            atomic_store(&s_radio_lifecycle,
                         BLE_RADIO_LIFECYCLE_DORMANT);
        }
        else
        {
            atomic_store(&s_radio_lifecycle,
                         BLE_RADIO_LIFECYCLE_STOPPING);
        }
        return err;
    }
    configure_default_tx_power();
    ble_hs_cfg.reset_cb = ble_host_reset;
    ble_hs_cfg.sync_cb = ble_host_sync;
    atomic_store(&s_host_task_deleted, false);
    atomic_store(&s_accept_host_events, true);
    nimble_port_freertos_init(ble_host_task);
    s_nimble_started = true;
    s_radio_start_deadline_ms =
        (uint64_t)esp_timer_get_time() / 1000U +
        BLE_HOST_SYNC_TIMEOUT_MS;
    ble_nimble_cleanup_guard_reset(&s_nimble_cleanup);
    return ESP_OK;
}

static esp_err_t stop_nimble(void)
{
    s_radio_start_deadline_ms = 0U;
    if (!s_nimble_started)
    {
        if (s_nimble_cleanup.terminal_error != ESP_OK)
        {
            atomic_store(&s_radio_lifecycle,
                         BLE_RADIO_LIFECYCLE_STOPPING);
            return (esp_err_t)s_nimble_cleanup.terminal_error;
        }
        const esp_err_t reserve_error = reserve_radio_retention();
        atomic_store(&s_radio_lifecycle,
                     reserve_error == ESP_OK
                         ? BLE_RADIO_LIFECYCLE_DORMANT
                         : BLE_RADIO_LIFECYCLE_STOPPING);
        return reserve_error;
    }
    if (s_nimble_cleanup.terminal_error != ESP_OK)
    {
        return (esp_err_t)s_nimble_cleanup.terminal_error;
    }
    atomic_store(&s_radio_lifecycle,
                 BLE_RADIO_LIFECYCLE_STOPPING);
    if (ble_nimble_cleanup_begin_stop(&s_nimble_cleanup))
    {
        /* 先停止 GAP 活动并禁止新事件，再等待 host task 删除回调。 */
        atomic_store(&s_accept_host_events, false);
        if (s_scanning)
        {
            (void)ble_gap_disc_cancel();
            s_scanning = false;
        }
        if (s_connecting)
        {
            int cancel_result = ble_gap_conn_cancel();
            if (cancel_result != 0 && cancel_result != BLE_HS_EALREADY)
            {
                ESP_LOGW(TAG, "BLE 停止时连接取消请求失败，错误=%d", cancel_result);
            }
            s_connecting = false;
        }
        if (s_link_connected && s_connection_handle != UINT16_MAX)
        {
            invalidate_link_generation();
            reset_link_admission();
            int terminate_result = ble_gap_terminate(s_connection_handle,
                                                     BLE_ERR_REM_USER_CONN_TERM);
            if (terminate_result != 0 && terminate_result != BLE_HS_ENOTCONN)
            {
                ESP_LOGW(TAG, "BLE 停止时旧 peer 终止请求失败，错误=%d", terminate_result);
            }
        }
        int stop_result = nimble_port_stop();
        if (stop_result != 0)
        {
            ble_nimble_cleanup_record_error(&s_nimble_cleanup, ESP_FAIL);
            ESP_LOGE(TAG,
                     "NimBLE host 停止失败，稳定码=%s，错误=%d",
                     BLE_HOST_STOP_FAILED,
                     stop_result);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR, BLE_HOST_STOP_FAILED);
            return (esp_err_t)s_nimble_cleanup.terminal_error;
        }
    }
    const TickType_t wait_started = xTaskGetTickCount();
    const TickType_t wait_budget = pdMS_TO_TICKS(BLE_HOST_STOP_WAIT_MS);
    while (!atomic_load(&s_host_task_deleted))
    {
        if (xTaskGetTickCount() - wait_started >= wait_budget)
        {
            ESP_LOGE(TAG,
                     "NimBLE host task 删除等待超时，稳定码=%s",
                     BLE_HOST_STOP_TIMEOUT);
            publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                              BLE_HOST_STOP_TIMEOUT);
            ble_nimble_cleanup_record_error(&s_nimble_cleanup,
                                            ESP_ERR_TIMEOUT);
            return (esp_err_t)s_nimble_cleanup.terminal_error;
        }
        vTaskDelay(1);
    }
    if (!ble_nimble_cleanup_begin_deinit(&s_nimble_cleanup))
    {
        return s_nimble_cleanup.terminal_error != ESP_OK
                   ? (esp_err_t)s_nimble_cleanup.terminal_error
                   : ESP_ERR_INVALID_STATE;
    }
    esp_err_t deinit_error = nimble_port_deinit();
    if (deinit_error == ESP_OK)
    {
        s_nimble_started = false;
        (void)advance_radio_generation();
        atomic_store(&s_host_queue_overflow, false);
        atomic_store(&s_notify_transport_failure, false);
    }
    if (deinit_error != ESP_OK)
    {
        ble_nimble_cleanup_record_error(&s_nimble_cleanup, deinit_error);
        ESP_LOGE(TAG,
                 "NimBLE port 回收失败，稳定码=%s，错误=0x%x",
                 BLE_HOST_DEINIT_FAILED,
                 (unsigned)deinit_error);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                          BLE_HOST_DEINIT_FAILED);
        return deinit_error;
    }
    ble_nimble_cleanup_guard_reset(&s_nimble_cleanup);
    const esp_err_t reserve_error = reserve_radio_retention();
    if (reserve_error != ESP_OK)
    {
        ble_nimble_cleanup_record_error(&s_nimble_cleanup,
                                        reserve_error);
        atomic_store(&s_radio_lifecycle,
                     BLE_RADIO_LIFECYCLE_STOPPING);
        ESP_LOGE(TAG,
                 "BLE Radio Dormant retention 保留失败，稳定码=%s，字节=%u",
                 BLE_DORMANT_RETENTION_FAILED,
                 (unsigned)BLE_RADIO_RETENTION_RESERVE_BYTES);
        publish_ble_state(WATCH_BLE_TRANSACTION_ERROR,
                          BLE_DORMANT_RETENTION_FAILED);
        return reserve_error;
    }
    atomic_store(&s_radio_lifecycle,
                 BLE_RADIO_LIFECYCLE_DORMANT);
    return ESP_OK;
}

static bool radio_event_is_current(const ble_private_event_t *event)
{
    return event != NULL &&
           (event->radio_generation == 0U ||
            event->radio_generation == s_radio_generation);
}

static uint32_t advance_radio_generation(void)
{
    ++s_radio_generation;
    if (s_radio_generation == 0U)
    {
        ++s_radio_generation;
    }
    atomic_store(&s_callback_radio_generation,
                 s_radio_generation);
    return s_radio_generation;
}

static esp_err_t reserve_radio_retention(void)
{
    if (s_radio_retention_reserve != NULL)
    {
        return ESP_OK;
    }
    s_radio_retention_reserve =
        heap_caps_malloc(BLE_RADIO_RETENTION_RESERVE_BYTES,
                         MALLOC_CAP_RETENTION);
    if (s_radio_retention_reserve == NULL)
    {
        ESP_LOGE(TAG,
                 "BLE Radio Dormant retention 保留块申请失败，稳定码=%s，字节=%u",
                 BLE_DORMANT_RETENTION_FAILED,
                 (unsigned)BLE_RADIO_RETENTION_RESERVE_BYTES);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

static void release_radio_retention(void)
{
    if (s_radio_retention_reserve == NULL)
    {
        return;
    }
    heap_caps_free(s_radio_retention_reserve);
    s_radio_retention_reserve = NULL;
}

static esp_err_t try_enter_radio_dormant(void)
{
    /*
     * ESP32-S3 真机已证明：其他服务落位后反初始化再冷启动会因
     * RETENTION 碎片导致 controller 初始化失败。未完成 100 次可靠
     * 启停验证前保持 host READY，只停止 GAP 业务，绑定功能优先。
     */
    return ESP_OK;
}

#endif /* BLE_SERVICE_UNLOCK_OWNER_TEST */

static bool control_callback_watchdog_must_survive_abort(
    bool callback_pending)
{
    return callback_pending;
}

static bool unlock_callback_watchdog_must_survive_abort(
    const ble_unlock_context_t *context)
{
    return context != NULL && context->active && !context->callback_succeeded;
}

static bool disconnect_matches_pending_connect(
    bool connecting,
    bool link_connected,
    uint32_t current_link_generation,
    uint32_t event_link_generation)
{
    /*
     * ESP-IDF v5.5.4 NimBLE Central 可能在 CONNECT 事件发布前先报告断链。
     * 同代次断链必须结束连接尝试，否则 s_connecting 与 UI 会永久停留。
     */
    return connecting && !link_connected &&
           current_link_generation != 0U &&
           event_link_generation == current_link_generation;
}

static void ble_tx_power_policy_reset(ble_tx_power_policy_t *policy)
{
    if (policy != NULL)
    {
        memset(policy, 0, sizeof(*policy));
    }
}

static void ble_tx_power_policy_request_boost(ble_tx_power_policy_t *policy)
{
    if (policy != NULL)
    {
        policy->boost_next_connection = true;
    }
}

static void ble_tx_power_policy_note_scan_rssi(ble_tx_power_policy_t *policy,
                                               int8_t rssi)
{
    if (policy != NULL && rssi <= BLE_TX_POWER_WEAK_RSSI_DBM)
    {
        policy->boost_next_connection = true;
    }
}

static bool ble_tx_power_disconnect_reason_is_abnormal(int32_t reason)
{
    /*
     * ESP-IDF v5.5.4 NimBLE 将 HCI reason 包装为 BLE_HS_HCI_ERR()。
     * 只将监督超时、LL 响应超时和 MAC 建链失败视为无线链路异常，
     * 明确排除本地终止、远端用户终止和设备关机。
     */
    return reason == BLE_HS_HCI_ERR(BLE_ERR_CONN_SPVN_TMO) ||
           reason == BLE_HS_HCI_ERR(BLE_ERR_LMP_LL_RSP_TMO) ||
           reason == BLE_HS_HCI_ERR(BLE_ERR_MAC_CONN_FAIL);
}

static void ble_tx_power_policy_note_disconnect(
    ble_tx_power_policy_t *policy,
    int32_t reason,
    uint64_t now_ms)
{
    if (policy == NULL ||
        !ble_tx_power_disconnect_reason_is_abnormal(reason))
    {
        return;
    }
    if (policy->has_recent_abnormal_disconnect &&
        now_ms - policy->recent_abnormal_disconnect_ms <=
            BLE_TX_POWER_ABNORMAL_DISCONNECT_WINDOW_MS)
    {
        policy->boost_next_connection = true;
        policy->has_recent_abnormal_disconnect = false;
        policy->recent_abnormal_disconnect_ms = 0U;
        return;
    }
    policy->has_recent_abnormal_disconnect = true;
    policy->recent_abnormal_disconnect_ms = now_ms;
}

static void ble_tx_power_policy_note_control_attempt(
    ble_tx_power_policy_t *policy,
    uint8_t attempt)
{
    if (policy != NULL &&
        attempt >= BLE_TX_POWER_CONTROL_RETRY_TRIGGER_ATTEMPT)
    {
        policy->boost_next_connection = true;
    }
}

static bool ble_tx_power_policy_take_boost(ble_tx_power_policy_t *policy)
{
    if (policy == NULL || !policy->boost_next_connection)
    {
        return false;
    }
    policy->boost_next_connection = false;
    return true;
}

static int start_unlock_write_boundary(
    ble_unlock_context_t *unlock_context,
    const ble_unlock_request_t *request,
    const ble_status_frame_t *mirror,
    uint32_t status_sequence,
    uint64_t now_ms,
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    ble_gatt_attr_fn *callback,
    void *callback_argument)
{
    uint8_t packet[BLE_CONTROL_PACKET_LENGTH] = {0};
    if (unlock_context == NULL || request == NULL || mirror == NULL ||
        callback == NULL || characteristic_val_handle == 0U ||
        !ble_unlock_build_packet(mirror, packet) ||
        !ble_unlock_begin(unlock_context,
                          request,
                          status_sequence,
                          now_ms))
    {
        memset(packet, 0, sizeof(packet));
        return -1;
    }
#ifndef BLE_SERVICE_UNLOCK_OWNER_TEST
    ESP_LOGI(TAG,
             "BLE 解锁包摘要：长度=%u，助力=%u，左档=%u，右档=%u，场景=%u，device_check=%u，shutdown=%u，待机=%u",
             (unsigned)BLE_CONTROL_PACKET_LENGTH,
             (unsigned)packet[2],
             (unsigned)packet[3],
             (unsigned)packet[4],
             (unsigned)packet[5],
             (unsigned)packet[BLE_CONTROL_DEVICE_CHECK_OFFSET],
             (unsigned)packet[BLE_CONTROL_SHUTDOWN_OFFSET],
             (unsigned)packet[21]);
#endif
    const int result = start_gatt_control_write(conn_handle,
                                                characteristic_val_handle,
                                                packet,
                                                callback,
                                                callback_argument);
    memset(packet, 0, sizeof(packet));
    if (result != 0)
    {
        ble_unlock_cancel(unlock_context);
    }
    return result;
}

static int start_control_write_boundary(
    const ble_status_frame_t *mirror,
    const ble_control_packet_override_t *override,
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    ble_gatt_attr_fn *callback,
    void *callback_argument)
{
    uint8_t packet[BLE_CONTROL_PACKET_LENGTH] = {0};
    if (mirror == NULL || override == NULL || callback == NULL ||
        characteristic_val_handle == 0U ||
        !ble_control_packet_build(mirror, override, packet))
    {
        return -1;
    }
#ifndef BLE_SERVICE_UNLOCK_OWNER_TEST
    ESP_LOGI(TAG,
             "BLE 控制包摘要：长度=%u，覆盖类型=%d，覆盖目标=%u，助力=%u，左档=%u，右档=%u，场景=%u，device_check=%u，shutdown=%u，待机=%u",
             (unsigned)BLE_CONTROL_PACKET_LENGTH,
             (int)override->kind,
             (unsigned)override->target,
             (unsigned)packet[2],
             (unsigned)packet[3],
             (unsigned)packet[4],
             (unsigned)packet[5],
             (unsigned)packet[BLE_CONTROL_DEVICE_CHECK_OFFSET],
             (unsigned)packet[BLE_CONTROL_SHUTDOWN_OFFSET],
             (unsigned)packet[21]);
#endif
    const int result = start_gatt_control_write(conn_handle,
                                                characteristic_val_handle,
                                                packet,
                                                callback,
                                                callback_argument);
    memset(packet, 0, sizeof(packet));
    return result;
}

static int start_gatt_control_write(
    uint16_t conn_handle,
    uint16_t characteristic_val_handle,
    const uint8_t packet[BLE_CONTROL_PACKET_LENGTH],
    ble_gatt_attr_fn *callback,
    void *callback_argument)
{
    const uint16_t actual_mtu = ble_att_mtu(conn_handle);
    if (actual_mtu >= BLE_CONTROL_PACKET_LENGTH + 3U)
    {
#ifndef BLE_SERVICE_UNLOCK_OWNER_TEST
        ESP_LOGI(TAG,
                 "BLE 控制写路径：ATT MTU=%u，负载=%u，方式=普通 Write Request",
                 (unsigned)actual_mtu,
                 (unsigned)BLE_CONTROL_PACKET_LENGTH);
#endif
        return ble_gattc_write_flat(conn_handle,
                                    characteristic_val_handle,
                                    packet,
                                    BLE_CONTROL_PACKET_LENGTH,
                                    callback,
                                    callback_argument);
    }

    struct os_mbuf *packet_mbuf =
        ble_hs_mbuf_from_flat(packet, BLE_CONTROL_PACKET_LENGTH);
    if (packet_mbuf == NULL)
    {
#ifndef BLE_SERVICE_UNLOCK_OWNER_TEST
        ESP_LOGE(TAG,
                 "BLE Long Write 分配 mbuf 失败，ATT MTU=%u，负载=%u",
                 (unsigned)actual_mtu,
                 (unsigned)BLE_CONTROL_PACKET_LENGTH);
#endif
        return BLE_HS_ENOMEM;
    }
#ifndef BLE_SERVICE_UNLOCK_OWNER_TEST
    ESP_LOGI(TAG,
             "BLE 控制写路径：ATT MTU=%u，负载=%u，方式=NimBLE Long Write",
             (unsigned)actual_mtu,
             (unsigned)BLE_CONTROL_PACKET_LENGTH);
#endif
    /* ble_gattc_write_long 无论启动成败都接管 mbuf 所有权。 */
    return ble_gattc_write_long(conn_handle,
                                characteristic_val_handle,
                                0U,
                                packet_mbuf,
                                callback,
                                callback_argument);
}
