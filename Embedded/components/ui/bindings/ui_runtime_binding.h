/**
 * @file     ui_runtime_binding.h
 * @brief    SquareLine 对象树运行时 binding 接口
 * @details  定义 ui_task 消费的不可变视图模型、typed intent、页面生命周期和 35 项阶段三 resolver 边界。
 * @author   ZHC
 * @date     2026-07-22
 */

#ifndef LEGBOT_UI_RUNTIME_BINDING_H
#define LEGBOT_UI_RUNTIME_BINDING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_state.h"
#include "control_ui_binding.h"
#include "ui_authorization_display.h"
#include "ui_navigation.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** 固件最多同时投影的 BLE 候选行数。 */
#define UI_RUNTIME_BLE_CANDIDATE_COUNT 4U
/** UI 文本模型的通用短文本容量。 */
#define UI_RUNTIME_TEXT_CAPACITY 48U
/** 提醒页最多同时显示的只读行数。 */
#define UI_RUNTIME_VISIBLE_ALERT_COUNT 3U

    /** 提醒页固定优先级类型。 */
    typedef enum
    {
        UI_RUNTIME_ALERT_TEMPERATURE = 0, /**< 外骨骼高温危险提醒。 */
        UI_RUNTIME_ALERT_EXO_BATTERY,     /**< 外骨骼低电提醒。 */
        UI_RUNTIME_ALERT_WATCH_SEVERE,    /**< 手环严重低电危险提醒。 */
        UI_RUNTIME_ALERT_WATCH_BATTERY,   /**< 手环普通低电提醒。 */
        UI_RUNTIME_ALERT_AUDIO,           /**< 音频资源异常提醒。 */
        UI_RUNTIME_ALERT_CELLULAR,        /**< 4G 未就绪提醒。 */
        UI_RUNTIME_ALERT_GPS,             /**< GPS 未定位提醒。 */
        UI_RUNTIME_ALERT_COUNT            /**< 固定提醒类型数量。 */
    } ui_runtime_alert_type_t;

    /** 单条只读提醒投影值。 */
    typedef struct
    {
        ui_runtime_alert_type_t type; /**< 固定提醒类型。 */
        bool danger;                 /**< true 使用统一危险红色视觉。 */
        char label[UI_RUNTIME_TEXT_CAPACITY]; /**< 用户可理解的提醒名称。 */
        char value[UI_RUNTIME_TEXT_CAPACITY]; /**< 用户可理解的当前值。 */
    } ui_runtime_alert_t;

    /** BLE 候选页扫描状态。 */
    typedef enum
    {
        UI_RUNTIME_BLE_SCAN_IDLE = 0, /**< 当前未扫描。 */
        UI_RUNTIME_BLE_SCAN_ACTIVE,   /**< 正在扫描。 */
    } ui_runtime_ble_scan_state_t;

    /** BLE 候选连接反馈。 */
    typedef enum
    {
        UI_RUNTIME_BLE_CONNECTION_IDLE = 0, /**< 尚未选择候选。 */
        UI_RUNTIME_BLE_CONNECTION_CONNECTING, /**< 已提交并正在连接。 */
        UI_RUNTIME_BLE_CONNECTION_FAILED,     /**< 最近选择连接失败。 */
    } ui_runtime_ble_connection_state_t;

    /** GPS 环境提交状态。 */
    typedef enum
    {
        UI_RUNTIME_GPS_SUBMIT_IDLE = 0, /**< 等待选择环境。 */
        UI_RUNTIME_GPS_SUBMIT_LOADING,  /**< 提交中。 */
        UI_RUNTIME_GPS_SUBMIT_FAILED,   /**< 提交失败。 */
    } ui_runtime_gps_submit_state_t;

    /** 设置页屏幕亮度档位。 */
    typedef enum
    {
        UI_RUNTIME_BRIGHTNESS_LOW = 0, /**< 低亮度。 */
        UI_RUNTIME_BRIGHTNESS_MEDIUM,  /**< 中亮度。 */
        UI_RUNTIME_BRIGHTNESS_HIGH,    /**< 高亮度。 */
        UI_RUNTIME_BRIGHTNESS_COUNT,   /**< 亮度档位数量边界。 */
    } ui_runtime_brightness_t;

    /** 设置页自动熄屏秒数。 */
    typedef enum
    {
        UI_RUNTIME_SCREEN_TIMEOUT_5S = 5,   /**< 5 秒自动熄屏。 */
        UI_RUNTIME_SCREEN_TIMEOUT_15S = 15, /**< 15 秒自动熄屏。 */
        UI_RUNTIME_SCREEN_TIMEOUT_30S = 30, /**< 30 秒自动熄屏。 */
    } ui_runtime_screen_timeout_t;

    /** 单条 BLE 候选值。 */
    typedef struct
    {
        char name[UI_RUNTIME_TEXT_CAPACITY]; /**< 广播名称或稳定回退名称。 */
        char mac[18];                        /**< 规范完整 MAC。 */
        int8_t rssi;                         /**< 最近 RSSI。 */
    } ui_runtime_ble_candidate_t;

    /** ui_task 单次快照构建的完整视图模型。 */
    typedef struct
    {
        bool snapshot_valid;                    /**< 本轮事实读取是否成功。 */
        bool binding_present;                   /**< 是否存在绑定外骨骼。 */
        bool ble_connected;                     /**< BLE 物理链路是否连接。 */
        bool cellular_connected;                /**< 4G 是否已建立有效 PDP/IP 网络连接。 */
        bool cellular_connecting;               /**< 4G 是否正在启动或执行联网会话。 */
        bool cellular_action_enabled;           /**< 维护页是否允许点击显式联网。 */
        bool gps_action_enabled;                /**< 维护页是否允许点击显式搜星。 */
        bool gps_fixed;                         /**< GPS 是否已有当前有效定位。 */
        bool controls_enabled;                  /**< 普通控制是否允许提交。 */
        bool unlock_session_valid;              /**< 当前 BLE 上电会话是否已有新鲜解锁证明。 */
        bool selftest_running;                  /**< 自检是否占用控制。 */
        bool assist_enabled;                    /**< 实际助力开关。 */
        bool poweroff_committed;                /**< 外骨骼关机写是否已被设备接受。 */
        bool power_action_enabled;              /**< 外骨骼关机是否允许提交。 */
        ui_authorization_display_t authorization_display; /**< 授权阻断的单值显示状态。 */
        bool authorization_pending;             /**< 是否正在联网并检查授权。 */
        bool payment_required;                  /**< 支付验证是否阻断。 */
        bool payment_attempted;                 /**< 本轮是否已尝试验证。 */
        bool payment_pending;                   /**< 支付验证命令是否仍在处理。 */
        bool payment_failed;                    /**< 最近验证是否失败。 */
        bool payment_action_enabled;             /**< 当前事实是否允许提交支付验证。 */
        control_ui_block_reason_t control_block_reason; /**< 普通控制零提交的内部原因。 */
        bool haptics_enabled;                   /**< 触觉反馈显示值。 */
        bool click_audio_enabled;               /**< 点击音效显示值。 */
        bool raise_to_wake_enabled;             /**< 抬腕亮屏显示值。 */
        ui_runtime_brightness_t brightness_level; /**< 屏幕亮度显示档位。 */
        ui_runtime_screen_timeout_t screen_timeout_seconds; /**< 自动熄屏显示秒数。 */
        bool leg_positions_available;           /**< 左右腿位置是否来自当前新鲜状态。 */
        uint8_t reminder_count;                 /**< 非阻断提醒总数。 */
        uint8_t visible_alert_count;            /**< 当前投影的提醒行数。 */
        ui_runtime_alert_t alerts[UI_RUNTIME_VISIBLE_ALERT_COUNT]; /**< 按固定优先级投影的提醒行。 */
        watch_power_level_t power_level;        /**< power owner 已采用的手环电量等级。 */
        uint8_t watch_battery;                  /**< 手表电量百分比。 */
        uint8_t exo_battery;                    /**< 外骨骼电量百分比。 */
        uint8_t gear;                           /**< 外骨骼统一档位。 */
        uint8_t drive_mode;                     /**< 外骨骼场景模式。 */
        int8_t left_leg_position;                /**< 当前左腿有符号角度。 */
        int8_t right_leg_position;               /**< 当前右腿有符号角度。 */
        watch_exoskeleton_model_t exoskeleton_model; /**< 当前绑定外骨骼缓存型号。 */
        watch_exoskeleton_scene_config_t scene_config; /**< 当前设备上报的场景能力配置。 */
        uint32_t control_update_sequence;        /**< 控制事实更新序号。 */
        bool gear_projection_locked;            /**< 档位乐观投影五秒锁是否有效。 */
        bool assist_projection_locked;          /**< 助力乐观投影五秒锁是否有效。 */
        bool mode_projection_locked;            /**< 模式乐观投影五秒锁是否有效。 */
        uint32_t steps;                         /**< 当日步数。 */
        char time[8];                           /**< HH:MM 时间。 */
        char last_snapshot[UI_RUNTIME_TEXT_CAPACITY]; /**< 断连页最近状态摘要。 */
        char device_model[UI_RUNTIME_TEXT_CAPACITY]; /**< 固定产品型号。 */
        char firmware_version[UI_RUNTIME_TEXT_CAPACITY]; /**< 固件版本。 */
        char serial_number[UI_RUNTIME_TEXT_CAPACITY];    /**< Watch 序列号。 */
        char binding_id[18];                             /**< 已绑定 MAC 标识。 */
        char cellular_status[UI_RUNTIME_TEXT_CAPACITY];  /**< 4G 状态。 */
        char gps_status[UI_RUNTIME_TEXT_CAPACITY];       /**< GPS 状态。 */
        char selftest_status[UI_RUNTIME_TEXT_CAPACITY];  /**< 自检摘要状态。 */
        ui_runtime_ble_scan_state_t ble_scan_state;      /**< BLE 扫描状态。 */
        ui_runtime_ble_connection_state_t ble_connection_state; /**< BLE 连接反馈。 */
        uint32_t ble_candidate_generation;               /**< 候选代次。 */
        uint8_t ble_candidate_count;                     /**< 有效候选数量。 */
        uint8_t selected_ble_candidate;                  /**< 当前反馈对应候选索引。 */
        ui_runtime_ble_candidate_t ble_candidates[UI_RUNTIME_BLE_CANDIDATE_COUNT]; /**< 候选值。 */
        uint32_t selftest_run_id;                        /**< 当前自检 run ID。 */
        selftest_item_id_t selftest_item;                /**< 当前自检项目。 */
        uint8_t selftest_completed_categories;           /**< 已完成的用户可见类别数量。 */
        uint8_t selftest_progress;                       /**< 0 至 100 进度。 */
        char selftest_item_text[UI_RUNTIME_TEXT_CAPACITY]; /**< 当前项目文案。 */
        char manual_prompt[UI_RUNTIME_TEXT_CAPACITY];      /**< 人工判定问题。 */
        char manual_countdown[UI_RUNTIME_TEXT_CAPACITY];   /**< 人工判定倒计时。 */
        bool manual_reply_pending;                         /**< 人工判定是否已提交等待推进。 */
        watch_selftest_summary_t selftest_results;         /**< 十一项原始结果。 */
        uint8_t selected_failed_item;                       /**< 当前失败项索引。 */
        bool retry_running;                                 /**< 单项重试进行中。 */
        ui_runtime_gps_submit_state_t gps_submit_state;     /**< GPS 环境提交状态。 */
        bool clear_binding_pending;                         /**< 清除绑定命令已提交。 */
        bool clear_binding_failed;                          /**< 最近清除绑定请求失败。 */
    } ui_runtime_model_t;

    /** SquareLine 事件唯一允许产生的 typed intent。 */
    typedef enum
    {
        UI_INTENT_OPEN_SETTINGS = 0,      /**< 打开设置。 */
        UI_INTENT_BACK_TO_SHELL,          /**< 返回常驻主壳。 */
        UI_INTENT_SHELL_HOME,              /**< 切到主表盘槽位。 */
        UI_INTENT_SHELL_CONTROL,           /**< 切到控制槽位。 */
        UI_INTENT_SHELL_ALERTS,            /**< 切到提醒槽位。 */
        UI_INTENT_SHELL_MODE_POWER,        /**< 切到模式电源槽位。 */
        UI_INTENT_SHELL_PREVIOUS,          /**< 横滑到前一个主壳槽位。 */
        UI_INTENT_SHELL_NEXT,              /**< 横滑到后一个主壳槽位。 */
        UI_INTENT_OPEN_BLE_CANDIDATES,     /**< 打开 BLE 候选页。 */
        UI_INTENT_RECONNECT_BLE,           /**< 请求重新连接。 */
        UI_INTENT_GEAR_DECREMENT,          /**< 档位减一。 */
        UI_INTENT_GEAR_INCREMENT,          /**< 档位加一。 */
        UI_INTENT_TOGGLE_ASSIST,           /**< 切换助力。 */
        UI_INTENT_SET_MODE_STANDARD,       /**< 设为标准模式。 */
        UI_INTENT_SET_MODE_EXTREME,        /**< 设为极限模式。 */
        UI_INTENT_SET_MODE_SPORT,          /**< 设为健身模式，标准小碎步配置下复用为小碎步。 */
        UI_INTENT_SET_MODE_ECO,            /**< 设为下山模式。 */
        UI_INTENT_EXO_POWEROFF,            /**< 外骨骼关机。 */
        UI_INTENT_VERIFY_PAYMENT,          /**< 验证支付。 */
        UI_INTENT_TOGGLE_HAPTICS,          /**< 切换触觉反馈。 */
        UI_INTENT_TOGGLE_CLICK_AUDIO,      /**< 切换点击音效。 */
        UI_INTENT_TOGGLE_RAISE_WAKE,       /**< 切换抬腕亮屏。 */
        UI_INTENT_SET_BRIGHTNESS,           /**< 设置目标亮度档位。 */
        UI_INTENT_SET_SCREEN_TIMEOUT,       /**< 设置目标自动熄屏秒数。 */
        UI_INTENT_OPEN_DEVICE_INFO,        /**< 打开设备信息。 */
        UI_INTENT_OPEN_MAINTENANCE,        /**< 打开维护。 */
        UI_INTENT_BACK,                    /**< 按当前层级返回。 */
        UI_INTENT_OPEN_SELFTEST,           /**< 打开自检入口。 */
        UI_INTENT_OPEN_CLEAR_BINDING,      /**< 打开清除绑定 modal。 */
        UI_INTENT_CONFIRM_CLEAR_BINDING,   /**< 确认清除绑定。 */
        UI_INTENT_CANCEL_CLEAR_BINDING,    /**< 取消清除绑定。 */
        UI_INTENT_RESCAN_BLE,              /**< 重新扫描 BLE。 */
        UI_INTENT_SELECT_BLE_CANDIDATE,    /**< 选择 BLE 候选。 */
        UI_INTENT_START_SELFTEST,          /**< 开始整机自检。 */
        UI_INTENT_LEAVE_SELFTEST,          /**< 请求离开自检。 */
        UI_INTENT_SELFTEST_MANUAL_FAIL,    /**< 人工判定失败。 */
        UI_INTENT_SELFTEST_MANUAL_PASS,    /**< 人工判定通过。 */
        UI_INTENT_SELFTEST_TOUCH_TARGET,   /**< 触摸测试命中目标。 */
        UI_INTENT_SELFTEST_TOUCH_PROGRESS, /**< 触摸测试滑动进度。 */
        UI_INTENT_SELFTEST_OUTPUT_REPLAY, /**< 再次播放音频或振动。 */
        UI_INTENT_OPEN_RETRY_FAILED,       /**< 打开失败项详情。 */
        UI_INTENT_RERUN_SELFTEST,          /**< 重新整机自检。 */
        UI_INTENT_GPS_OUTDOOR,             /**< 选择室外或靠窗。 */
        UI_INTENT_GPS_INDOOR,              /**< 选择室内。 */
        UI_INTENT_FAILED_PREVIOUS,         /**< 上一个失败项。 */
        UI_INTENT_FAILED_NEXT,             /**< 下一个失败项。 */
        UI_INTENT_RETRY_FAILED_ITEM,       /**< 重试当前失败项。 */
        UI_INTENT_CONNECT_CELLULAR,        /**< 维护页显式请求 4G 联网。 */
        UI_INTENT_REQUEST_GPS_ACQUISITION, /**< 维护页显式请求 GPS 搜星。 */
        UI_INTENT_COUNT                    /**< typed intent 数量。 */
    } ui_runtime_intent_type_t;

    /** typed intent 的物理交互来源。 */
    typedef enum
    {
        UI_RUNTIME_INTENT_SOURCE_BUTTON = 0, /**< 已启用可操作控件的点击事件。 */
        UI_RUNTIME_INTENT_SOURCE_NON_BUTTON, /**< 手势、拖动、滚动或其他非按钮事件。 */
        UI_RUNTIME_INTENT_SOURCE_POWER_KEY,  /**< 运行态 PWR 释放后的物理导航事件。 */
        UI_RUNTIME_INTENT_SOURCE_COUNT,      /**< intent 物理来源闭集边界。 */
    } ui_runtime_intent_source_t;

    /** binding 对 typed intent 的处置结果。 */
    typedef enum
    {
        UI_RUNTIME_INTENT_REJECTED = 0,   /**< 当前事实不允许该操作。 */
        UI_RUNTIME_INTENT_HANDLED_LOCAL,  /**< binding 已完成本地状态或页面处理。 */
        UI_RUNTIME_INTENT_SUBMIT_SERVICE, /**< intent 合法且须继续提交业务 owner。 */
    } ui_runtime_intent_disposition_t;

    /** typed intent 按值载荷。 */
    typedef struct
    {
        ui_runtime_intent_type_t type;     /**< intent 类型。 */
        ui_runtime_intent_source_t source; /**< intent 的物理交互来源。 */
        uint32_t value;                    /**< 候选索引或其他有界数值。 */
    } ui_runtime_intent_t;

    /** ui_task 中的 typed intent 消费回调。 */
    typedef void (*ui_runtime_intent_handler_t)(const ui_runtime_intent_t *intent,
                                                void *context);

    /** 常驻主壳快照直传的内部性能与资源状态。 */
    typedef struct
    {
        bool available;          /**< 三页 PSRAM 快照缓冲是否完整可用。 */
        bool active;             /**< 当前横滑是否正显示快照并隐藏实时对象树。 */
        size_t allocated_bytes;  /**< 三个 RGB565 快照缓冲的总分配字节数。 */
        uint32_t prepare_count;  /**< 本次启动成功启用快路的手势次数。 */
        uint32_t prepare_failures; /**< 起滑时三页快照未就绪的次数。 */
        uint32_t last_prepare_ms; /**< 最近一次起滑快照切换耗时。 */
        uint32_t max_prepare_ms;  /**< 本次启动起滑快照切换最大耗时。 */
        uint32_t warm_page_count; /**< 闲时成功预热单页快照的次数。 */
        uint32_t warm_failures;   /**< 闲时预热单页快照失败次数。 */
        uint32_t max_warm_page_ms; /**< 闲时预热单页快照最大耗时。 */
        uint32_t ignored_refresh_count; /**< 未改变主壳可见像素的模型刷新次数。 */
        uint32_t invalidated_page_count; /**< 因可见像素变化而失效的页面累计数。 */
        uint8_t dirty_page_count; /**< 当前横滑所需三页中仍等待预热的页面数。 */
    } ui_runtime_shell_snapshot_stats_t;

    /** 常驻主壳横滑快路单帧的三个 RGB565 页面源。 */
    typedef struct
    {
        const uint16_t *current_pixels; /**< 当前页 410×502 RGB565 像素。 */
        const uint16_t *left_pixels;    /**< 左相邻页 410×502 RGB565 像素。 */
        const uint16_t *right_pixels;   /**< 右相邻页 410×502 RGB565 像素。 */
        int16_t offset_px; /**< pager 相对当前页中心的有符号横向位移。 */
    } ui_runtime_shell_fast_frame_t;

    /** 设置七栏纵滑快照快路的内部性能与资源状态。 */
    typedef struct
    {
        bool available;          /**< 一张背景与双内容 PSRAM 缓冲是否完整可用。 */
        bool active;             /**< 当前纵滑是否由硬件快路直接合成显示。 */
        size_t allocated_bytes;  /**< 三个 RGB565 快照缓冲的总分配字节数。 */
        uint32_t prepare_count;  /**< 本次启动成功启用快路的手势次数。 */
        uint32_t prepare_failures; /**< 起滑时快照未就绪的次数。 */
        uint32_t last_prepare_ms; /**< 最近一次起滑切换快路耗时。 */
        uint32_t max_prepare_ms;  /**< 本次启动起滑切换最大耗时。 */
        uint32_t warm_count;      /**< 闲时成功生成背景或内容快照的次数。 */
        uint32_t warm_failures;   /**< 闲时快照生成失败次数。 */
        uint32_t max_warm_ms;     /**< 闲时单次快照生成最大耗时。 */
        uint32_t background_build_count; /**< 页面生命周期内背景生成次数。 */
        uint32_t content_rebuild_count;  /**< 内容后台缓冲成功重建次数。 */
        uint32_t content_swap_count;     /**< 新内容完成后原子切换次数。 */
        uint32_t ignored_refresh_count;  /**< 未改变设置视觉的模型刷新次数。 */
        uint32_t target_generation;      /**< 最新设置视觉目标代次。 */
        uint32_t front_generation;       /**< 当前前台内容快照视觉代次。 */
    } ui_runtime_settings_snapshot_stats_t;

    /** 设置七栏纵滑快路单帧的 RGB565 背景与连续内容源。 */
    typedef struct
    {
        const uint16_t *background_pixels; /**< 410×502 静态背景像素。 */
        const uint16_t *content_pixels;    /**< 378×608 七栏连续内容像素。 */
        uint16_t viewport_x;               /**< 内容视口左坐标。 */
        uint16_t viewport_y;               /**< 内容视口顶坐标。 */
        uint16_t viewport_width;           /**< 内容视口宽度。 */
        uint16_t viewport_height;          /**< 内容视口高度。 */
        uint16_t content_width;            /**< 连续内容快照宽度。 */
        uint16_t content_height;           /**< 连续内容快照高度。 */
        uint16_t scroll_y;                 /**< 当前有界纵向滚动位置。 */
        uint16_t thumb_x;                  /**< 自定义滚动滑块左坐标。 */
        uint16_t thumb_y;                  /**< 自定义滚动滑块顶坐标。 */
        uint16_t thumb_width;              /**< 自定义滚动滑块宽度。 */
        uint16_t thumb_height;             /**< 自定义滚动滑块高度。 */
        uint16_t thumb_color;              /**< LVGL RGB565 滑块颜色。 */
    } ui_runtime_settings_fast_frame_t;

    /** 自检结果纵滑快照快路的内部性能与资源状态。 */
    typedef struct
    {
        bool available;          /**< 一张背景与双内容 PSRAM 缓冲是否完整可用。 */
        bool active;             /**< 当前纵滑是否由硬件快路直接合成显示。 */
        size_t allocated_bytes;  /**< 三个 RGB565 快照缓冲的总分配字节数。 */
        uint32_t prepare_count;  /**< 本次启动成功启用快路的手势次数。 */
        uint32_t prepare_failures; /**< 起滑时快照未就绪的次数。 */
        uint32_t last_prepare_ms; /**< 最近一次起滑切换快路耗时。 */
        uint32_t max_prepare_ms;  /**< 本次启动起滑切换最大耗时。 */
        uint32_t warm_count;      /**< 闲时成功生成背景或内容快照的次数。 */
        uint32_t warm_failures;   /**< 闲时快照生成失败次数。 */
        uint32_t max_warm_ms;     /**< 闲时单次快照生成最大耗时。 */
        uint32_t background_build_count; /**< 页面生命周期内背景生成次数。 */
        uint32_t content_rebuild_count;  /**< 内容后台缓冲成功重建次数。 */
        uint32_t content_swap_count;     /**< 新内容完成后原子切换次数。 */
        uint32_t ignored_refresh_count;  /**< 未改变结果视觉的模型刷新次数。 */
        uint32_t target_generation;      /**< 最新结果视觉目标代次。 */
        uint32_t front_generation;       /**< 当前前台内容快照视觉代次。 */
    } ui_runtime_selftest_result_snapshot_stats_t;

    /** 自检结果纵滑快路单帧的 RGB565 背景与连续内容源。 */
    typedef struct
    {
        const uint16_t *background_pixels; /**< 410×502 静态背景像素。 */
        const uint16_t *content_pixels;    /**< 378×内容高度的结果像素。 */
        uint16_t viewport_x;               /**< 内容视口左坐标。 */
        uint16_t viewport_y;               /**< 内容视口顶坐标。 */
        uint16_t viewport_width;           /**< 内容视口宽度。 */
        uint16_t viewport_height;          /**< 内容视口高度。 */
        uint16_t content_width;            /**< 连续内容快照宽度。 */
        uint16_t content_height;           /**< 连续内容快照实际高度。 */
        uint16_t scroll_y;                 /**< 当前有界纵向滚动位置。 */
        uint16_t thumb_x;                  /**< 自定义滚动滑块左坐标。 */
        uint16_t thumb_y;                  /**< 自定义滚动滑块顶坐标。 */
        uint16_t thumb_width;              /**< 自定义滚动滑块宽度。 */
        uint16_t thumb_height;             /**< 自定义滚动滑块高度。 */
        uint16_t thumb_color;              /**< LVGL RGB565 滑块颜色。 */
    } ui_runtime_selftest_result_fast_frame_t;

    /**
     * @brief 初始化 SquareLine UI 与 binding
     * @param handler typed intent 消费回调，可为 NULL
     * @param context 回调上下文
     */
    void ui_runtime_binding_init(ui_runtime_intent_handler_t handler, void *context);

    /**
     * @brief 按固定 patch 顺序渲染最新不可变模型
     * @param model ui_task 本轮完整模型
     * @return true 已渲染，false 表示快照或对象生命周期无效并保留当前页面
     */
    bool ui_runtime_binding_render(const ui_runtime_model_t *model);

    /**
     * @brief 同步 UI owner 已确认的物理显示开关事实
     * @details 支付成功只在亮屏时消费控制页跳转；熄屏时仅锁存目标，绝不产生唤醒动作。
     * @param display_on true 表示显示已点亮，false 表示显示已熄灭
     */
    void ui_runtime_binding_set_display_on(bool display_on);

    /**
     * @brief 将场景 typed intent 解析为当前型号的归一化业务目标
     * @param type 待解析的 typed intent 类型
     * @param model 当前外骨骼型号
     * @return 场景业务目标 1 至 5；非场景 intent 返回 0
     */
    uint8_t ui_runtime_binding_scene_target_for_intent(
        ui_runtime_intent_type_t type,
        watch_exoskeleton_scene_config_t scene_config);

    /**
     * @brief 用最新事实接受 typed intent 并执行页面路由
     * @param intent SquareLine 事件产生的 intent
     * @param model 事件发生后重新读取的最新模型
     * @return 拒绝、本地已处理或继续提交业务 owner
     */
    ui_runtime_intent_disposition_t ui_runtime_binding_accept_intent(
        const ui_runtime_intent_t *intent,
        const ui_runtime_model_t *model);

    /** @brief 释放按需页面、modal、AMOLED 对象和回调上下文。 */
    void ui_runtime_binding_deinit(void);

    /**
     * @brief 开始单一对象 AMOLED 五色测试
     * @return true 已开始或已在运行，false 表示 UI 尚未初始化
     */
    bool ui_runtime_binding_start_amoled_test(void);

    /** @brief 停止并删除 AMOLED 五色覆盖对象。 */
    void ui_runtime_binding_stop_amoled_test(void);

    /**
     * @brief 查询常驻主壳 pager 是否正在跟手滚动或回弹
     * @return true 正在运动，false 未初始化、非主壳或已停止
     */
    bool ui_runtime_binding_shell_motion_active(void);

    /**
     * @brief 查询设置七栏列表是否正在跟手滚动或惯性回弹
     * @return true 正在运动，false 未初始化、非设置页或已停止
     */
    bool ui_runtime_binding_settings_motion_active(void);

    /**
     * @brief 查询自检结果列表是否正在跟手滚动
     * @return true 正在运动，false 未初始化、非结果页或已停止
     */
    bool ui_runtime_binding_selftest_result_motion_active(void);

    /**
     * @brief 查询任一连续滚动 owner 是否正在运动
     * @return true 主壳、设置或自检结果任一运动中
     */
    bool ui_runtime_binding_motion_active(void);

    /**
     * @brief 查询当前已加载页面是否为 BLE 候选页面
     * @return true 表示 BLE 候选空页、扫描页或候选列表页正在显示
     */
    bool ui_runtime_binding_ble_candidate_page_active(void);

    /**
     * @brief 读取常驻主壳三页快照直传的资源与起滑耗时
     * @param stats 调用方提供的状态输出
     */
    void ui_runtime_binding_shell_snapshot_stats(
        ui_runtime_shell_snapshot_stats_t *stats);

    /**
     * @brief 由 ui_task 在常驻页面空闲时最多预热一个页面快照
     * @return true 本轮完成一个页面快照，false 表示无需预热或生成失败
     */
    bool ui_runtime_binding_warm_shell_snapshot(void);

    /**
     * @brief 读取当前横滑位置与三页 RGB565 快照源
     * @param frame 调用方提供的只读快帧描述
     * @return true 快照横滑正在运行且三页源有效，false 不应走硬件快路
     */
    bool ui_runtime_binding_shell_fast_frame(
        ui_runtime_shell_fast_frame_t *frame);

    /**
     * @brief 读取设置纵滑快照快路的资源与起滑耗时
     * @param stats 调用方提供的状态输出
     */
    void ui_runtime_binding_settings_snapshot_stats(
        ui_runtime_settings_snapshot_stats_t *stats);

    /**
     * @brief 由 ui_task 在设置页空闲时最多预热一个快照
     * @return true 本轮完成背景或内容快照，false 表示无需预热或生成失败
     */
    bool ui_runtime_binding_warm_settings_snapshot(void);

    /**
     * @brief 读取当前纵滑位置、静态背景与七栏连续内容快照源
     * @param frame 调用方提供的只读快帧描述
     * @return true 设置纵滑快路正在运行且两类快照有效
     */
    bool ui_runtime_binding_settings_fast_frame(
        ui_runtime_settings_fast_frame_t *frame);

    /**
     * @brief 读取自检结果纵滑快路的资源与起滑耗时
     * @param stats 调用方提供的状态输出
     */
    void ui_runtime_binding_selftest_result_snapshot_stats(
        ui_runtime_selftest_result_snapshot_stats_t *stats);

    /**
     * @brief 由 ui_task 在结果页空闲时最多预热一个快照
     * @return true 本轮完成背景或内容快照，false 表示无需预热或生成失败
     */
    bool ui_runtime_binding_warm_selftest_result_snapshot(void);

    /**
     * @brief 读取自检结果纵滑位置、背景与连续内容快照源
     * @param frame 调用方提供的只读快帧描述
     * @return true 自检结果纵滑快路正在运行且两类快照有效
     */
    bool ui_runtime_binding_selftest_result_fast_frame(
        ui_runtime_selftest_result_fast_frame_t *frame);

    /**
     * @brief 在显示快路故障后恢复 LVGL invalidation 与实时对象树
     * @details 常驻横滑保留当前滚动生命周期以完成槽位结算；其他滚动
     *          中止快路，并统一请求一次整屏重绘。
     */
    void ui_runtime_binding_abort_fast_path(void);

    /**
     * @brief 由 ui_task 取走并清除一次性整屏重绘请求
     * @return true 表示存在待执行请求，false 表示没有请求
     */
    bool ui_runtime_binding_take_full_redraw_request(void);

    /**
     * @brief 重新合并一次整屏重绘请求
     * @details 仅供 ui_task 在同步刷新或显示恢复失败后保留未完成意图。
     */
    void ui_runtime_binding_request_full_redraw(void);

    /** @brief 返回 manifest 中阶段三 resolver 数量，固定为 35。 */
    size_t ui_runtime_binding_resolver_count(void);

    /**
     * @brief 解析当前失败项详情选中的真实自检项目
     * @param model 最新完整模型
     * @return 失败项目 ID；无有效失败项时返回 SELFTEST_ITEM_COUNT
     */
    selftest_item_id_t ui_runtime_binding_selected_failed_item(
        const ui_runtime_model_t *model);

    /**
     * @brief 返回等待 GPS 环境或业务提交的单项重试项目
     * @return 项目 ID；没有待提交重试时返回 SELFTEST_ITEM_COUNT
     */
    selftest_item_id_t ui_runtime_binding_pending_retry_item(void);

    /** @brief 清除已经被业务 owner 接受的单项重试上下文。 */
    void ui_runtime_binding_clear_pending_retry(void);

    /**
     * @brief 由纯事件包装器上报 typed intent
     * @param type intent 类型
     * @param value 有界载荷
     * @param source 按钮或非按钮交互来源
     */
    void ui_runtime_binding_emit_intent(ui_runtime_intent_type_t type,
                                        uint32_t value,
                                        ui_runtime_intent_source_t source);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_RUNTIME_BINDING_H */
