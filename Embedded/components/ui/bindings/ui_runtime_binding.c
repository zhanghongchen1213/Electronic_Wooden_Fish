/**
 * @file     ui_runtime_binding.c
 * @brief    SquareLine 对象树运行时 binding 实现
 * @details  在 ui_task 中按基线、canonical、整页变体、自检锁和实时字段顺序渲染，并管理设置双页、八个按需 Screen、09-01 modal 与 AMOLED 覆盖对象。
 * @author   ZHC
 * @date     2026-08-17
 */

#include "ui_runtime_binding.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "ui.h"
#include "ui_manifest_projection.h"
#include "ui_selftest_summary.h"
#include "ui_touch_session.h"

#if !defined(SCENIC_AREA_MANAGEMENT_DEBUG) || \
    (SCENIC_AREA_MANAGEMENT_DEBUG != 0 && SCENIC_AREA_MANAGEMENT_DEBUG != 1)
#error "SCENIC_AREA_MANAGEMENT_DEBUG must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_GPS_CONTINUOUS_LOCATION)
#error "LEGBOT_CAP_GPS_CONTINUOUS_LOCATION must be defined."
#endif
#if !defined(LEGBOT_CAP_GPS_TIME) || \
    (LEGBOT_CAP_GPS_TIME != 0 && LEGBOT_CAP_GPS_TIME != 1)
#error "LEGBOT_CAP_GPS_TIME must be defined as 0 or 1."
#endif
#if !defined(LEGBOT_CAP_MODEM) || \
    (LEGBOT_CAP_MODEM != 0 && LEGBOT_CAP_MODEM != 1)
#error "LEGBOT_CAP_MODEM must be defined as 0 or 1."
#endif

static const char *TAG = "UI_BIND";

/** AMOLED 单色显示时长。 */
#define UI_AMOLED_COLOR_DURATION_MS 2000U
/** AMOLED 五色数量。 */
#define UI_AMOLED_COLOR_COUNT 5U
/** 阶段三 manifest resolver 固定数量。 */
#define UI_PHASE3_RESOLVER_COUNT 37U
/** 日常主壳单个槽位宽度。 */
#define UI_SHELL_PAGE_WIDTH 410
/** 日常主壳触发相邻页切换的最小实际横向滚动行程。 */
#define UI_SHELL_SWIPE_TRIGGER_PX 8
/** 横滑快路同时缓存当前页和左右相邻页。 */
#define UI_SHELL_SNAPSHOT_BUFFER_COUNT 3U
/** 设置纵滑快路采用前台与后台两张内容快照。 */
#define UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT 2U
/** 设置内容快照尚无前台缓冲时的无效索引。 */
#define UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE \
    UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT
/** 设置列表视口高度，最多完整显示四栏。 */
#define UI_SETTINGS_LIST_VIEWPORT_HEIGHT 344
/** 设置列表视口左坐标。 */
#define UI_SETTINGS_LIST_VIEWPORT_X 16
/** 设置列表视口顶坐标。 */
#define UI_SETTINGS_LIST_VIEWPORT_Y 72
/** 设置列表与连续内容宽度。 */
#define UI_SETTINGS_LIST_WIDTH 378
/** 设置列表七栏连续内容高度。 */
#define UI_SETTINGS_LIST_CONTENT_HEIGHT 608
/** 设置页一张整屏背景与双内容快照的最大瞬态内存预算。 */
#define UI_SETTINGS_SNAPSHOT_TRANSIENT_BUDGET_BYTES 1330936U
/** 设置列表后三栏容器顶部坐标。 */
#define UI_SETTINGS_LIST_TRAILING_GROUP_Y 344
/** 设置列表后三栏容器高度。 */
#define UI_SETTINGS_LIST_TRAILING_GROUP_HEIGHT 264
/** 设置列表允许的最大连续纵向滚动量。 */
#define UI_SETTINGS_LIST_MAX_SCROLL_Y 264
/** 维护页 4G 状态文字与操作箭头之间的水平间距。 */
#define UI_MAINTENANCE_CELLULAR_VALUE_ARROW_GAP 8
/** 常驻主壳允许下拉进入设置的按下起点最大纵坐标。 */
#define UI_MAIN_SETTINGS_START_MAX_Y 96
/** 设置页允许上滑退出的按下起点最小纵坐标。 */
#define UI_SETTINGS_RETURN_START_MIN_Y 416
/** 顶部或底部边缘手势提交导航的最小累计纵向位移。 */
#define UI_EDGE_SWIPE_COMMIT_PX 48
/** 设置页全宽底部返回手势区横坐标。 */
#define UI_SETTINGS_RETURN_ZONE_X 0
/** 设置页全宽底部返回手势区纵坐标。 */
#define UI_SETTINGS_RETURN_ZONE_Y 416
/** 设置页全宽底部返回手势区宽度。 */
#define UI_SETTINGS_RETURN_ZONE_WIDTH 410
/** 设置页全宽底部返回手势区高度。 */
#define UI_SETTINGS_RETURN_ZONE_HEIGHT 86
/** 设置页滚动指示滑块顶部坐标。 */
#define UI_SETTINGS_SCROLL_THUMB_TOP_Y 88
/** 设置页滚动指示滑块底部坐标。 */
#define UI_SETTINGS_SCROLL_THUMB_BOTTOM_Y 248
/** 设置页滚动指示滑块左坐标。 */
#define UI_SETTINGS_SCROLL_THUMB_X 399
/** 设置页滚动指示滑块宽度。 */
#define UI_SETTINGS_SCROLL_THUMB_WIDTH 3
/** 设置页滚动指示滑块高度。 */
#define UI_SETTINGS_SCROLL_THUMB_HEIGHT 152
/** 设置页滚动指示滑块颜色。 */
#define UI_SETTINGS_SCROLL_THUMB_COLOR 0xD7FF00U
/** 自检结果自定义滚动条顶部坐标。 */
#define UI_SELFTEST_SCROLL_TRACK_Y 216
/** 自检结果自定义滚动条可用行程高度。 */
#define UI_SELFTEST_SCROLL_TRACK_HEIGHT 168
/** 自检结果自定义滚动条滑块最小高度。 */
#define UI_SELFTEST_SCROLL_THUMB_MIN_HEIGHT 32
/** 自检结果列表视口左坐标。 */
#define UI_SELFTEST_RESULT_VIEWPORT_X 16
/** 自检结果列表视口顶坐标。 */
#define UI_SELFTEST_RESULT_VIEWPORT_Y 204
/** 自检结果列表视口宽度。 */
#define UI_SELFTEST_RESULT_VIEWPORT_WIDTH 378
/** 自检结果列表视口高度。 */
#define UI_SELFTEST_RESULT_VIEWPORT_HEIGHT 196
/** 自检结果连续内容的安全高度上限。 */
#define UI_SELFTEST_RESULT_CONTENT_MAX_HEIGHT 366
/** 自检结果滚动滑块左坐标。 */
#define UI_SELFTEST_RESULT_THUMB_X 400
/** 自检结果滚动滑块宽度。 */
#define UI_SELFTEST_RESULT_THUMB_WIDTH 3
/** 自检结果滚动滑块颜色。 */
#define UI_SELFTEST_RESULT_THUMB_COLOR 0x8B919AU
/** 自检结果纵滑快路采用前台与后台两张内容快照。 */
#define UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT 2U
/** 自检结果尚无前台内容缓冲时的无效索引。 */
#define UI_SELFTEST_RESULT_CONTENT_INDEX_NONE \
    UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT
/** 触摸自检通过按钮的严格进度阈值。 */
#define UI_SELFTEST_TOUCH_PASS_THRESHOLD_PERCENT 50U
/** 顶部状态栏未连接图标颜色。 */
#define UI_STATUS_ICON_DISCONNECTED_COLOR 0x8E98A7U
/** 顶部状态栏 4G 已连接图标颜色。 */
#define UI_STATUS_ICON_CELLULAR_CONNECTED_COLOR 0x30D158U
/** 顶部状态栏 GPS 已定位图标颜色。 */
#define UI_STATUS_ICON_GPS_FIXED_COLOR 0x64D2FFU
/** 顶部状态栏 BLE 已连接图标颜色。 */
#define UI_STATUS_ICON_BLE_CONNECTED_COLOR 0x0A84FFU
/** 助力开启时的强调色。 */
#define UI_ASSIST_ENABLED_COLOR 0x30D158U
/** 助力关闭时的内容色。 */
#define UI_ASSIST_DISABLED_COLOR 0x8E98A7U
/** 助力关闭时的轨道色。 */
#define UI_ASSIST_DISABLED_TRACK_COLOR 0x484D56U
/** 首页助力开启图标与文字颜色。 */
#define UI_HOME_ASSIST_ENABLED_COLOR 0xD7FF00U
/** 首页助力关闭图标与文字颜色。 */
#define UI_HOME_ASSIST_DISABLED_COLOR 0x8E98A7U
/** 模式卡片选中背景与边框色。 */
#define UI_MODE_SELECTED_COLOR 0xD7FF00U
/** 模式卡片未选中背景色。 */
#define UI_MODE_IDLE_BACKGROUND_COLOR 0x16181CU
/** 模式卡片未选中边框色。 */
#define UI_MODE_IDLE_BORDER_COLOR 0x30343BU
/** 模式页断连态标题颜色。 */
#define UI_MODE_DISABLED_CONTENT_COLOR 0x8E98A7U
/** 通用白色内容色。 */
#define UI_CONTENT_WHITE_COLOR 0xFFFFFFU
/** 通用黑色内容色。 */
#define UI_CONTENT_BLACK_COLOR 0x000000U
/** 手环低电状态栏提醒色。 */
#define UI_WATCH_BATTERY_WARNING_COLOR 0xFFD60AU
/** 手环严重低电状态栏危险色。 */
#define UI_WATCH_BATTERY_DANGER_COLOR 0xFF453AU
/** 手环常态电池字形。 */
#define UI_WATCH_BATTERY_NORMAL_GLYPH ""
/** 手环低电和严重低电的空电池轮廓字形。 */
#define UI_WATCH_BATTERY_LOW_GLYPH ""
/** 顶部状态栏 4G 已连接图标字形。 */
#define UI_STATUS_ICON_CELLULAR_CONNECTED ""
/** 顶部状态栏 4G 未连接图标字形。 */
#define UI_STATUS_ICON_CELLULAR_DISCONNECTED ""
/** 顶部状态栏 GPS 已定位图标字形。 */
#define UI_STATUS_ICON_GPS_FIXED ""
/** 顶部状态栏 GPS 未定位图标字形。 */
#define UI_STATUS_ICON_GPS_NOT_FIXED ""
/** 顶部状态栏 BLE 已连接图标字形。 */
#define UI_STATUS_ICON_BLE_CONNECTED ""
/** 顶部状态栏 BLE 未连接图标字形。 */
#define UI_STATUS_ICON_BLE_DISCONNECTED ""
/** 首页助力开启闪电图标字形。 */
#define UI_HOME_ASSIST_ENABLED_GLYPH ""
/** 首页助力关闭断开闪电图标字形。 */
#define UI_HOME_ASSIST_DISABLED_GLYPH ""
/** 健身模式哑铃图标字形。 */
#define UI_MODE_FITNESS_GLYPH ""
/** 小碎步模式脚印图标字形 U+E3B9。 */
#define UI_MODE_SMALL_STEP_GLYPH ""

typedef enum
{
    UI_RUNTIME_SCREEN_MAIN = 0,        /**< 常驻主壳。 */
    UI_RUNTIME_SCREEN_SETTINGS,        /**< 设置。 */
    UI_RUNTIME_SCREEN_DEVICE_INFO,     /**< 设备信息。 */
    UI_RUNTIME_SCREEN_MAINTENANCE,     /**< 维护与 modal host。 */
    UI_RUNTIME_SCREEN_BLE,             /**< BLE 候选。 */
    UI_RUNTIME_SCREEN_SELFTEST,        /**< 自检进行态。 */
    UI_RUNTIME_SCREEN_SELFTEST_RESULT, /**< 自检结果。 */
    UI_RUNTIME_SCREEN_GPS_CONTEXT,     /**< GPS 环境。 */
    UI_RUNTIME_SCREEN_RETRY_DETAIL,    /**< 失败项详情。 */
    UI_RUNTIME_SCREEN_COUNT            /**< Screen 数量。 */
} ui_runtime_screen_t;

/** 真正影响设置七栏内容快照的规范化视觉字段。 */
typedef struct
{
    bool haptics_enabled;       /**< 触觉开关视觉值。 */
    bool click_audio_enabled;   /**< 点击音开关视觉值。 */
    bool raise_to_wake_enabled; /**< 抬腕亮屏开关视觉值。 */
    uint8_t brightness_index;   /**< 亮度分段按钮规范化索引。 */
    uint8_t timeout_index;      /**< 熄屏时间分段按钮规范化索引。 */
} ui_settings_visual_signature_t;

/** 真正影响自检结果列表内容的规范化视觉字段。 */
typedef struct
{
    selftest_outcome_t outcomes[UI_SELFTEST_CATEGORY_COUNT]; /**< 七类结果。 */
} ui_selftest_result_visual_signature_t;

/** 真正影响单个常驻主壳页面快照的规范化视觉字段。 */
typedef struct
{
    ui_manifest_state_t static_state; /**< 当前槽位已应用的静态状态。 */
    bool cellular_connected;          /**< 状态栏 4G 图标视觉值。 */
    bool gps_fixed;                   /**< 状态栏 GPS 图标视觉值。 */
    bool ble_connected;               /**< 状态栏 BLE 图标视觉值。 */
    watch_power_level_t power_level;  /**< 手环电池颜色与字形等级。 */
    uint8_t watch_battery;            /**< 手环电池百分比。 */
    uint8_t exo_battery;              /**< 首页外骨骼电量。 */
    uint8_t gear;                     /**< 首页与控制页档位。 */
    uint8_t drive_mode;               /**< 模式页规范化选中模式。 */
    watch_exoskeleton_scene_config_t scene_config; /**< 模式页完整场景配置。 */
    bool poweroff_committed;          /**< 关机已提交且控制页必须立即锁定。 */
    bool assist_enabled;              /**< 首页状态与控制页助力开关视觉值。 */
    bool authorization_pending;       /**< 首页是否隐藏助力状态。 */
    ui_authorization_display_t authorization_display; /**< 提醒页授权显示变体。 */
    bool payment_required;            /**< 提醒页是否显示支付阻断。 */
    bool payment_pending;             /**< 支付按钮等待视觉值。 */
    bool payment_action_enabled;      /**< 支付按钮可用视觉值。 */
    uint8_t visible_alert_count;      /**< 提醒页实际显示行数。 */
    uint32_t steps;                   /**< 首页步数。 */
    char time[8];                     /**< 首页 HH:MM 文本。 */
    char last_snapshot[UI_RUNTIME_TEXT_CAPACITY]; /**< 首页断连摘要。 */
    ui_runtime_alert_t
        alerts[UI_RUNTIME_VISIBLE_ALERT_COUNT]; /**< 提醒页可见内容。 */
} ui_shell_visual_signature_t;

typedef struct
{
    const char *root;      /**< manifest 根对象。 */
    const char *field;     /**< 待解析字段。 */
    const char *selector;  /**< 稳定 SquareLine selector。 */
    const char *operation; /**< binding 中实际 LVGL/路由操作。 */
    const char *consumer;  /**< 同时消费字段并执行操作的函数。 */
} ui_phase3_resolver_t;

/** 37 个 required_phase3_resolver 的稳定 selector 与操作。 */
static const ui_phase3_resolver_t s_phase3_resolvers[UI_PHASE3_RESOLVER_COUNT] = {
    {"ui_page_home_root", "binding_present", "ui_page_home_root", "ui_navigation_resolve_shell", "shell_page_for_model"},
    {"ui_page_home_root", "ble_connected", "ui_page_home_root", "ui_navigation_resolve_shell", "shell_page_for_model"},
    {"ui_page_home_root", "assist_enabled", "ui_home_normal_icon_assist_state", "apply_home_assist_status", "apply_model_projection"},
    {"ui_page_control_root", "ble_connected", "ui_page_control_root", "ui_navigation_resolve_shell", "shell_page_for_model"},
    {"ui_page_control_root", "selftest_running", "ui_control_connected_btn_gear_minus", "return", "control_intent_allowed"},
    {"ui_page_control_root", "assist_enabled", "ui_control_connected_switch_assist_thumb", "apply_assist_visual", "apply_model_projection"},
    {"ui_page_mode_power_root", "ble_connected", "ui_page_mode_power_root", "ui_navigation_resolve_shell", "shell_page_for_model"},
    {"ui_page_mode_power_root", "selftest_running", "ui_mode_default_btn_mode_standard", "return", "control_intent_allowed"},
    {"ui_page_mode_power_root", "power_action_enabled", "ui_mode_default_btn_exo_poweroff", "control_intent_allowed", "ui_runtime_binding_accept_intent"},
    {"ui_page_mode_power_root", "exoskeleton_model", "ui_mode_default_btn_mode_extreme", "set_flag", "apply_mode_layout"},
    {"ui_page_alerts_root", "payment_required", "ui_page_alerts_root", "ui_navigation_resolve_shell", "shell_page_for_model"},
    {"ui_page_alerts_root", "payment_attempted", "ui_page_alerts_root", "ui_navigation_resolve_shell", "shell_page_for_model"},
    {"ui_page_alerts_root", "alerts", "ui_alerts_readonly_alert_exo_battery_warning_instance", "set_label", "set_alerts"},
    {"ui_page_settings_root", "haptics_enabled", "ui_settings_row_haptics_instance", "set_setting_toggle", "apply_model_projection"},
    {"ui_page_settings_root", "click_audio_enabled", "ui_settings_row_click_audio_instance", "set_setting_toggle", "apply_model_projection"},
    {"ui_page_settings_root", "raise_to_wake_enabled", "ui_settings_row_raise_wake_instance", "set_setting_toggle", "apply_model_projection"},
    {"ui_page_settings_root", "brightness_level", "ui_settings_btn_brightness_high", "set_segmented_setting", "apply_model_projection"},
    {"ui_page_settings_root", "screen_timeout_seconds", "ui_settings_btn_screen_timeout_5s", "set_segmented_setting", "apply_model_projection"},
    {"ui_page_device_info_root", "device_model", "ui_device_info_row_firmware_label", "set_label", "apply_model_projection"},
    {"ui_page_device_info_root", "serial_number", "ui_device_info_row_watch_id_value", "set_label", "apply_model_projection"},
    {"ui_page_device_info_root", "binding_id", "ui_device_info_row_bound_mac_value", "set_label", "apply_model_projection"},
    {"ui_page_device_info_root", "leg_positions_available", "ui_device_info_row_left_leg_position_value", "set_label", "apply_leg_positions"},
    {"ui_page_device_info_root", "left_leg_position", "ui_device_info_row_left_leg_position_value", "set_label", "apply_leg_positions"},
    {"ui_page_device_info_root", "right_leg_position", "ui_device_info_row_right_leg_position_value", "set_label", "apply_leg_positions"},
    {"ui_page_maintenance_root", "binding_present", "ui_maintenance_row_clear_binding", "set_state", "apply_model_projection"},
    {"ui_page_ble_root", "binding_present", "ui_page_ble_root", "UI_MANIFEST_STATE_HOME_NORMAL", "resolve_static_state"},
    {"ui_page_ble_root", "ble_scan_state", "ui_ble_scanning_panel_scanning", "UI_MANIFEST_STATE_BLE_SCANNING", "resolve_static_state"},
    {"ui_page_ble_root", "ble_candidates", "ui_ble_candidates_device_candidate_1", "ui_comp_get_child", "set_ble_candidates"},
    {"ui_page_ble_root", "ble_connection_state", "ui_ble_candidates_device_candidate_1", "set_ble_candidate_state", "set_ble_candidates"},
    {"ui_page_selftest_progress_root", "selftest_running", "ui_page_selftest_progress_root", "ui_navigation_resolve_selftest", "selftest_page_for_model"},
    {"ui_page_selftest_result_root", "selftest_results", "ui_selftest_result_result_display", "set_selftest_results", "apply_model_projection"},
    {"ui_page_selftest_gps_context_root", "gps_context", "ui_page_selftest_gps_context_root", "UI_PAGE_SELFTEST_GPS_CONTEXT", "ui_runtime_binding_accept_intent"},
    {"ui_page_selftest_gps_context_root", "gps_submit_state", "ui_selftest_gps_submit_loading_btn_gps_outdoor", "UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_LOADING", "resolve_static_state"},
    {"ui_page_selftest_retry_detail_root", "failed_items", "ui_selftest_retry_detail_val_retry_item", "failed_category_at", "set_retry_detail"},
    {"ui_page_selftest_retry_detail_root", "retry_state", "ui_selftest_retry_detail_btn_retry_item", "set_state", "set_retry_detail"},
    {"ui_modal_clear_binding_root", "binding_present", "ui_clear_binding_confirm_btn_confirm_clear_binding", "set_state", "show_clear_binding_modal"},
    {"ui_modal_clear_binding_root", "clear_binding_pending", "ui_clear_binding_confirm_btn_cancel", "set_state", "show_clear_binding_modal"},
};

/** typed intent 消费回调。 */
static ui_runtime_intent_handler_t s_intent_handler;
/** typed intent 消费上下文。 */
static void *s_intent_context;
/** binding 是否完成初始化。 */
static bool s_initialized;
/** 当前逻辑页面。 */
static ui_page_id_t s_current_page = UI_PAGE_HOME_READY;
/** 返回当前按需页前的逻辑页面。 */
static ui_page_id_t s_return_page = UI_PAGE_HOME_READY;
/** 08-05 返回时恢复的自检来源页。 */
static ui_page_id_t s_gps_return_page = UI_PAGE_SELFTEST_IDLE;
/** 当前主壳槽位。 */
static ui_shell_slot_t s_shell_slot = UI_SHELL_SLOT_HOME;
/** 最近一次已主动对齐到分页容器的主壳槽位。 */
static ui_shell_slot_t s_committed_shell_slot = UI_SHELL_SLOT_COUNT;
/** 设置列表最近一次连续纵向滚动位置。 */
static lv_coord_t s_settings_scroll_y;
/** 当前手势相对逻辑槽位的最大有符号横向滚动行程。 */
static lv_coord_t s_shell_scroll_peak_delta;
/** 常驻主壳 pager 是否处于跟手滚动或回弹动画。 */
static bool s_shell_motion_active;
/** 设置七栏列表是否处于跟手滚动或惯性回弹。 */
static bool s_settings_motion_active;
/** 自检结果列表是否处于跟手滚动。 */
static bool s_selftest_result_motion_active;
/** 常驻主壳本次手势是否因硬件快路故障而禁止重新进入。 */
static bool s_shell_fast_path_fault_latched;
/** 设置列表本次手势是否因硬件快路故障而禁止重新进入。 */
static bool s_settings_fast_path_fault_latched;
/** 自检结果本次手势是否因硬件快路故障而禁止重新进入。 */
static bool s_selftest_result_fast_path_fault_latched;
/** 设置页静态背景 RGB565 快照缓冲。 */
static void *s_settings_background_snapshot_buffer;
/** 设置页七栏连续内容前后台 RGB565 快照缓冲。 */
static void *s_settings_content_snapshot_buffers
    [UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT];
/** 设置页静态背景快照描述。 */
static lv_img_dsc_t s_settings_background_snapshot_descriptor;
/** 设置页七栏连续内容前后台快照描述。 */
static lv_img_dsc_t s_settings_content_snapshot_descriptors
    [UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT];
/** 设置页静态背景快照缓冲字节数。 */
static size_t s_settings_background_snapshot_bytes;
/** 设置页连续内容快照缓冲字节数。 */
static size_t s_settings_content_snapshot_bytes;
/** 设置页一张背景与双内容快照缓冲是否完整分配。 */
static bool s_settings_snapshot_available;
/** 设置页本次生命周期的快照生成故障是否已熔断。 */
static bool s_settings_snapshot_warm_fault_latched;
/** 设置页静态背景快照是否已生成。 */
static bool s_settings_background_snapshot_valid;
/** 设置页双内容快照各自是否已完整生成。 */
static bool s_settings_content_snapshot_valid
    [UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT];
/** 当前服务纵滑的前台内容快照缓冲索引。 */
static uint8_t s_settings_content_front_index =
    UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE;
/** 设置视觉签名是否已由有效模型初始化。 */
static bool s_settings_visual_signature_valid;
/** 设置页最新目标视觉签名。 */
static ui_settings_visual_signature_t s_settings_target_signature;
/** 最新设置视觉目标代次。 */
static uint32_t s_settings_target_generation;
/** 当前前台内容快照视觉代次。 */
static uint32_t s_settings_front_generation;
/** 设置纵滑期间是否暂停 LVGL 绘制并启用硬件快路。 */
static bool s_settings_snapshot_active;
/** 本次启动设置纵滑快路成功启用次数。 */
static uint32_t s_settings_snapshot_prepare_count;
/** 本次启动设置纵滑快路未就绪次数。 */
static uint32_t s_settings_snapshot_prepare_failures;
/** 最近一次设置起滑切换快路耗时。 */
static uint32_t s_settings_snapshot_last_prepare_ms;
/** 本次启动设置起滑切换快路最大耗时。 */
static uint32_t s_settings_snapshot_max_prepare_ms;
/** 本次启动设置页闲时快照成功次数。 */
static uint32_t s_settings_snapshot_warm_count;
/** 本次启动设置页闲时快照失败次数。 */
static uint32_t s_settings_snapshot_warm_failures;
/** 本次启动设置页闲时单次快照最大耗时。 */
static uint32_t s_settings_snapshot_max_warm_ms;
/** 本次启动设置背景成功生成次数。 */
static uint32_t s_settings_background_build_count;
/** 本次启动设置内容成功重建次数。 */
static uint32_t s_settings_content_rebuild_count;
/** 本次启动设置内容前后台原子切换次数。 */
static uint32_t s_settings_content_swap_count;
/** 本次启动未改变设置视觉的模型刷新次数。 */
static uint32_t s_settings_ignored_refresh_count;
/** 自检结果页静态背景 RGB565 快照缓冲。 */
static void *s_selftest_result_background_snapshot_buffer;
/** 自检结果页连续内容前后台 RGB565 快照缓冲。 */
static void *s_selftest_result_content_snapshot_buffers
    [UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT];
/** 自检结果页静态背景快照描述。 */
static lv_img_dsc_t s_selftest_result_background_snapshot_descriptor;
/** 自检结果页连续内容前后台快照描述。 */
static lv_img_dsc_t s_selftest_result_content_snapshot_descriptors
    [UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT];
/** 自检结果页静态背景快照缓冲字节数。 */
static size_t s_selftest_result_background_snapshot_bytes;
/** 自检结果页单张内容快照缓冲字节数。 */
static size_t s_selftest_result_content_snapshot_bytes;
/** 自检结果页背景与双内容缓冲是否完整分配。 */
static bool s_selftest_result_snapshot_available;
/** 自检结果页静态背景是否已生成。 */
static bool s_selftest_result_background_snapshot_valid;
/** 自检结果页双内容缓冲是否已完整生成。 */
static bool s_selftest_result_content_snapshot_valid
    [UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT];
/** 当前服务纵滑的自检结果前台内容索引。 */
static uint8_t s_selftest_result_content_front_index =
    UI_SELFTEST_RESULT_CONTENT_INDEX_NONE;
/** 自检结果视觉签名是否已初始化。 */
static bool s_selftest_result_visual_signature_valid;
/** 自检结果最新目标视觉签名。 */
static ui_selftest_result_visual_signature_t
    s_selftest_result_target_signature;
/** 自检结果最新视觉目标代次。 */
static uint32_t s_selftest_result_target_generation;
/** 自检结果当前前台内容代次。 */
static uint32_t s_selftest_result_front_generation;
/** 自检结果纵滑期间是否启用硬件快路。 */
static bool s_selftest_result_snapshot_active;
/** 自检结果列表当前实际内容高度。 */
static uint16_t s_selftest_result_content_height;
/** 自检结果列表当前最大滚动量。 */
static uint16_t s_selftest_result_scroll_range;
/** 自检结果列表当前滑块高度。 */
static uint16_t s_selftest_result_thumb_height;
/** 自检结果快路成功启用次数。 */
static uint32_t s_selftest_result_snapshot_prepare_count;
/** 自检结果快路起滑未就绪次数。 */
static uint32_t s_selftest_result_snapshot_prepare_failures;
/** 自检结果最近一次起滑切换耗时。 */
static uint32_t s_selftest_result_snapshot_last_prepare_ms;
/** 自检结果起滑切换最大耗时。 */
static uint32_t s_selftest_result_snapshot_max_prepare_ms;
/** 自检结果闲时快照成功次数。 */
static uint32_t s_selftest_result_snapshot_warm_count;
/** 自检结果闲时快照失败次数。 */
static uint32_t s_selftest_result_snapshot_warm_failures;
/** 自检结果闲时单次快照最大耗时。 */
static uint32_t s_selftest_result_snapshot_max_warm_ms;
/** 自检结果背景成功生成次数。 */
static uint32_t s_selftest_result_background_build_count;
/** 自检结果内容成功重建次数。 */
static uint32_t s_selftest_result_content_rebuild_count;
/** 自检结果内容前后台切换次数。 */
static uint32_t s_selftest_result_content_swap_count;
/** 未改变自检结果视觉的模型刷新次数。 */
static uint32_t s_selftest_result_ignored_refresh_count;
/** 三个常驻页面 RGB565 快照缓冲。 */
static void *s_shell_snapshot_buffers[UI_SHELL_SNAPSHOT_BUFFER_COUNT];
/** 三个常驻页面快照图像描述。 */
static lv_img_dsc_t
    s_shell_snapshot_descriptors[UI_SHELL_SNAPSHOT_BUFFER_COUNT];
/** 四个逻辑槽位各自的快照覆盖图像对象。 */
static lv_obj_t *s_shell_snapshot_images[UI_SHELL_SLOT_COUNT];
/** 三个快照缓冲当前对应的逻辑槽位。 */
static ui_shell_slot_t
    s_shell_snapshot_slots[UI_SHELL_SNAPSHOT_BUFFER_COUNT];
/** 三个快照缓冲是否已成功生成过图像。 */
static bool
    s_shell_snapshot_slot_valid[UI_SHELL_SNAPSHOT_BUFFER_COUNT];
/** 四个逻辑槽位是否需要在闲时重新生成快照。 */
static bool s_shell_snapshot_dirty[UI_SHELL_SLOT_COUNT];
/** 四个逻辑槽位最新的规范化视觉签名。 */
static ui_shell_visual_signature_t
    s_shell_target_signatures[UI_SHELL_SLOT_COUNT];
/** 四个逻辑槽位是否已有有效视觉签名。 */
static bool s_shell_visual_signature_valid[UI_SHELL_SLOT_COUNT];
/** 四个逻辑槽位最新视觉目标代次。 */
static uint32_t s_shell_target_generations[UI_SHELL_SLOT_COUNT];
/** 三个快照缓冲各自对应的视觉代次。 */
static uint32_t
    s_shell_snapshot_buffer_generations[UI_SHELL_SNAPSHOT_BUFFER_COUNT];
/** 单个 RGB565 快照缓冲字节数。 */
static size_t s_shell_snapshot_buffer_bytes;
/** 三页快照缓冲是否完整分配。 */
static bool s_shell_snapshot_available;
/** 横滑期间是否已隐藏实时对象树并显示快照。 */
static bool s_shell_snapshot_active;
/** 本次启动成功生成三页快照的次数。 */
static uint32_t s_shell_snapshot_prepare_count;
/** 本次启动生成三页快照失败的次数。 */
static uint32_t s_shell_snapshot_prepare_failures;
/** 最近一次三页快照生成耗时。 */
static uint32_t s_shell_snapshot_last_prepare_ms;
/** 本次启动三页快照生成最大耗时。 */
static uint32_t s_shell_snapshot_max_prepare_ms;
/** 本次启动闲时成功预热单页快照的次数。 */
static uint32_t s_shell_snapshot_warm_page_count;
/** 本次启动闲时预热单页快照失败次数。 */
static uint32_t s_shell_snapshot_warm_failures;
/** 本次启动闲时预热单页快照最大耗时。 */
static uint32_t s_shell_snapshot_max_warm_page_ms;
/** 本次启动未改变主壳可见像素的模型刷新次数。 */
static uint32_t s_shell_ignored_refresh_count;
/** 本次启动因可见像素变化而失效的页面累计数。 */
static uint32_t s_shell_invalidated_page_count;
/** 四个常驻槽位最近一次已应用的静态状态。 */
static ui_manifest_state_t s_shell_projected_states[UI_SHELL_SLOT_COUNT];
/** 最近一次成功渲染的静态投影，用于识别同屏 variant 切换。 */
static ui_manifest_state_t s_last_projected_state = UI_MANIFEST_STATE_COUNT;
/** 九个 Screen 最近一次已应用的静态状态。 */
static ui_manifest_state_t
    s_screen_projected_states[UI_RUNTIME_SCREEN_COUNT];
/** 当前加载的真实 Screen。 */
static ui_runtime_screen_t s_loaded_screen = UI_RUNTIME_SCREEN_MAIN;
/** 当前失败项列表索引。 */
static uint8_t s_failed_index;
/** 等待 GPS 环境或业务 owner 接受的单项重试项目。 */
static selftest_item_id_t s_pending_retry_item = SELFTEST_ITEM_COUNT;
/** 支付锁是否曾接管主壳路由。 */
static bool s_payment_lock_active;
/** 支付锁定期间是否由 PWR 明确进入四槽只读浏览。 */
static bool s_payment_power_key_navigation_active;
/** 支付并取得 BLE 证明后尚未按最新连接事实进入控制槽。 */
static bool s_pending_control_route;
/** ui_task 已确认的物理显示开关事实。 */
static bool s_display_on = true;
/** 是否已有一次成功渲染的路由事实。 */
static bool s_route_facts_valid;
/** 最近一次成功渲染时的绑定事实。 */
static bool s_last_binding_present;
/** 最近一次成功渲染时的物理连接事实。 */
static bool s_last_ble_connected;
/** 最近一次成功渲染时的 RAM-only 解锁证明事实。 */
static bool s_last_unlock_session_valid;
/** 当前触摸自检交互所属 run ID。 */
static uint32_t s_touch_run_id;
/** 当前触摸自检是否已命中目标。 */
static bool s_touch_target_hit;
/** 当前触摸自检人工回复是否已提交。 */
static bool s_touch_reply_pending;
/** 当前触摸自检滑动百分比。 */
static uint8_t s_touch_progress;
/** 最近一次触摸目标横坐标。 */
static int16_t s_touch_x;
/** 最近一次触摸目标纵坐标。 */
static int16_t s_touch_y;
/** AMOLED 覆盖对象。 */
static lv_obj_t *s_amoled_surface;
/** AMOLED 换色 timer。 */
static lv_timer_t *s_amoled_timer;
/** AMOLED 当前色号。 */
static uint8_t s_amoled_color_index;
/** 合并后的单次整屏重绘请求，仅由 ui_task 取走。 */
static bool s_full_redraw_requested;

static ui_runtime_screen_t screen_for_page(ui_page_id_t page);
static bool screen_needs_full_redraw(ui_runtime_screen_t screen);
static void request_full_redraw(void);
static lv_obj_t **screen_object(ui_runtime_screen_t screen);
static void screen_init(ui_runtime_screen_t screen);
static void screen_destroy(ui_runtime_screen_t screen);
static bool prepare_page_screen(ui_page_id_t page);
static void commit_page_screen(ui_page_id_t page);
static bool begin_ble_atomic_redraw(ui_runtime_screen_t resolved_screen,
                                    ui_manifest_state_t state,
                                    lv_disp_t **display_out);
static void finish_ble_atomic_redraw(bool active, lv_disp_t *display);
static ui_page_id_t shell_page_for_model(ui_shell_slot_t slot,
                                         const ui_runtime_model_t *model);
static void consume_pending_control_route(
    const ui_runtime_model_t *model);
static ui_manifest_state_t shell_state_for_page(ui_page_id_t page);
static bool apply_shell_static_patches(const ui_runtime_model_t *model);
static void invalidate_shell_projection_cache(void);
static ui_manifest_state_t resolve_static_state(const ui_runtime_model_t *model,
                                                ui_page_id_t *resolved_page);
static ui_manifest_state_t canonical_base_state(ui_manifest_state_t state);
static bool apply_static_patch(ui_manifest_state_t state);
static void apply_selftest_lock(const ui_runtime_model_t *model);
static void apply_model_projection(const ui_runtime_model_t *model);
static void apply_watch_battery_projection(
    const ui_runtime_model_t *model);
static void apply_connectivity_icons(const ui_runtime_model_t *model);
static void set_connectivity_icon(lv_obj_t *icon,
                                  bool connected,
                                  const char *connected_glyph,
                                  const char *disconnected_glyph,
                                  uint32_t connected_color);
static void apply_assist_visual(bool enabled);
static void apply_home_assist_status(bool enabled, bool visible);
static void apply_mode_layout(const ui_runtime_model_t *model);
static void apply_mode_selection(uint8_t drive_mode);
static uint8_t secondary_scene_target(
    watch_exoskeleton_scene_config_t config);
static void set_mode_card_selected(lv_obj_t *button,
                                   lv_obj_t *label,
                                   lv_obj_t *icon,
                                   bool selected);
static void apply_leg_positions(const ui_runtime_model_t *model);
static void set_object_geometry(lv_obj_t *object,
                                lv_coord_t x,
                                lv_coord_t y,
                                lv_coord_t width,
                                lv_coord_t height);
static void set_mode_card_geometry(lv_obj_t *button,
                                   lv_obj_t *label,
                                   lv_obj_t *icon,
                                   lv_coord_t x,
                                   lv_coord_t y,
                                   lv_coord_t width,
                                   bool compact);
static void set_power_button_geometry(lv_obj_t *button,
                                      lv_obj_t *label,
                                      lv_obj_t *icon,
                                      bool power_only,
                                      bool four_modes,
                                      bool disconnected,
                                      lv_coord_t three_mode_label_width);
static void set_label(lv_obj_t *label, const char *text);
static void set_text_color(lv_obj_t *object,
                           lv_color_t color,
                           lv_style_selector_t selector);
static void set_background_color(lv_obj_t *object,
                                 lv_color_t color,
                                 lv_style_selector_t selector);
static void set_border_color(lv_obj_t *object,
                             lv_color_t color,
                             lv_style_selector_t selector);
static void set_text_font(lv_obj_t *object,
                          const lv_font_t *font,
                          lv_style_selector_t selector);
static void set_x(lv_obj_t *object, lv_coord_t x);
static void set_y(lv_obj_t *object, lv_coord_t y);
static void set_width(lv_obj_t *object, lv_coord_t width);
static void set_height(lv_obj_t *object, lv_coord_t height);
static void set_background_opa(lv_obj_t *object,
                               lv_opa_t opacity,
                               lv_style_selector_t selector);
static void set_border_width(lv_obj_t *object,
                             lv_coord_t width,
                             lv_style_selector_t selector);
static void set_radius(lv_obj_t *object,
                       lv_coord_t radius,
                       lv_style_selector_t selector);
static void set_text_align(lv_obj_t *object,
                           lv_text_align_t align,
                           lv_style_selector_t selector);
static void set_flag(lv_obj_t *object, lv_obj_flag_t flag, bool enabled);
static void set_state(lv_obj_t *object, lv_state_t state, bool enabled);
static void remove_redundant_shell_clipping(void);
static void remove_redundant_temporary_clipping(
    ui_runtime_screen_t screen);
static void set_setting_toggle(lv_obj_t *instance, bool enabled);
static void set_segmented_setting(lv_obj_t *const buttons[3],
                                  lv_obj_t *const labels[3],
                                  uint8_t selected_index);
static void set_alerts(const ui_runtime_model_t *model);
static void set_payment_pending(const ui_runtime_model_t *model);
static void set_control_projection_locks(const ui_runtime_model_t *model);
static void set_ble_candidates(const ui_runtime_model_t *model);
static void set_ble_candidate_state(lv_obj_t *instance,
                                    ui_runtime_ble_connection_state_t state,
                                    int8_t rssi);
static void set_selftest_results(const watch_selftest_summary_t *source);
static void set_selftest_result_row(lv_obj_t *instance,
                                    selftest_outcome_t outcome);
static void set_retry_detail(const ui_runtime_model_t *model);
static void add_descendant_flags(lv_obj_t *root, lv_obj_flag_t flags);
static void configure_main_shell_interaction(void);
static void configure_shell_snapshot_cache(void);
static bool prepare_shell_snapshots(void);
static void set_shell_snapshot_active(bool active);
static void mark_shell_snapshots_dirty(void);
static ui_shell_visual_signature_t shell_visual_signature_for_model(
    ui_shell_slot_t slot,
    const ui_runtime_model_t *model);
static bool shell_visual_signatures_equal(
    const ui_shell_visual_signature_t *left,
    const ui_shell_visual_signature_t *right);
static void update_shell_visual_targets(
    const ui_runtime_model_t *model);
static bool shell_snapshot_cache_ready(void);
static int8_t shell_snapshot_buffer_for_slot(ui_shell_slot_t slot);
static int8_t shell_snapshot_replacement_buffer(ui_shell_slot_t slot);
static bool shell_touch_sequence_active(void);
static void release_shell_snapshot_cache(void);
static void shell_page_objects(lv_obj_t *pages[UI_SHELL_SLOT_COUNT],
                               lv_obj_t *roots[UI_SHELL_SLOT_COUNT]);
static void arrange_shell_pages(ui_shell_slot_t current_slot);
static void commit_shell_pager_slot(void);
static void configure_settings_scroll(void);
static void configure_settings_snapshot_cache(void);
static bool prepare_settings_snapshots(void);
static void set_settings_snapshot_active(bool active);
static ui_settings_visual_signature_t settings_visual_signature_for_model(
    const ui_runtime_model_t *model);
static bool settings_visual_signatures_equal(
    const ui_settings_visual_signature_t *left,
    const ui_settings_visual_signature_t *right);
static void update_settings_visual_target(
    const ui_runtime_model_t *model);
static bool settings_snapshot_cache_ready(void);
static void release_settings_snapshot_cache(void);
static void normalize_settings_interaction(void);
static void settings_scroll_event_cb(lv_event_t *event);
static void update_settings_scroll_thumb(void);
static void configure_selftest_result_scroll(void);
static void configure_selftest_result_snapshot_cache(void);
static bool prepare_selftest_result_snapshots(void);
static void set_selftest_result_snapshot_active(bool active);
static bool selftest_result_snapshot_cache_ready(void);
static void release_selftest_result_snapshot_cache(void);
static ui_selftest_result_visual_signature_t
    selftest_result_visual_signature_for_source(
        const watch_selftest_summary_t *source);
static bool selftest_result_visual_signatures_equal(
    const ui_selftest_result_visual_signature_t *left,
    const ui_selftest_result_visual_signature_t *right);
static void update_selftest_result_visual_target(
    const watch_selftest_summary_t *source);
static void update_selftest_result_scroll_geometry(void);
static void selftest_result_scroll_event_cb(lv_event_t *event);
static void update_selftest_result_scroll_thumb(void);
static void set_failed_navigation_state(lv_obj_t *button,
                                        lv_obj_t *icon,
                                        bool enabled);
static void reset_touch_selftest(void);
static void sync_touch_selftest_run(uint32_t run_id);
static void set_touch_selftest(const ui_runtime_model_t *model);
static void update_touch_selftest_visual(void);
static bool update_touch_progress_local(uint8_t progress);
static uint8_t failed_item_count(const ui_runtime_model_t *model);
static ui_selftest_category_t failed_category_at(
    const ui_runtime_model_t *model,
    uint8_t list_index);
static selftest_item_id_t failed_item_at(const ui_runtime_model_t *model,
                                         uint8_t list_index);
static const char *selftest_item_name(selftest_item_id_t item);
static ui_page_id_t selftest_page_for_model(const ui_runtime_model_t *model);
static void advance_selftest_route(const ui_runtime_model_t *model);
static void route_to(ui_page_id_t page);
static ui_page_id_t shell_page_for_slot(ui_shell_slot_t slot);
static void route_back(void);
static void hide_clear_binding_modal(void);
static void show_clear_binding_modal(const ui_runtime_model_t *model);
static bool is_settings_hierarchy_page(ui_page_id_t page);
static bool payment_lock_allows_intent(ui_runtime_intent_type_t type);
static bool control_intent_allowed(const ui_runtime_model_t *model);
static bool button_intent_blocked_by_gesture(void);
static void shell_scroll_end_event_cb(lv_event_t *event);
static void shell_release_event_cb(lv_event_t *event);
static void settings_release_event_cb(lv_event_t *event);
static void maintenance_gps_event_cb(lv_event_t *event);
static void touch_target_event_cb(lv_event_t *event);
static void touch_slider_event_cb(lv_event_t *event);
static void output_replay_event_cb(lv_event_t *event);
static void amoled_timer_cb(lv_timer_t *timer);
static void set_amoled_color(uint8_t index);

void ui_runtime_binding_init(ui_runtime_intent_handler_t handler, void *context)
{
    if (!s_initialized)
    {
        ui_init();
        ui_touch_session_reset();
        s_initialized = true;
        s_current_page = UI_PAGE_HOME_READY;
        s_return_page = UI_PAGE_HOME_READY;
        s_gps_return_page = UI_PAGE_SELFTEST_IDLE;
        s_shell_slot = UI_SHELL_SLOT_HOME;
        s_committed_shell_slot = UI_SHELL_SLOT_COUNT;
        s_settings_scroll_y = 0;
        s_shell_scroll_peak_delta = 0;
        s_shell_motion_active = false;
        s_settings_motion_active = false;
        s_selftest_result_motion_active = false;
        s_shell_fast_path_fault_latched = false;
        s_settings_fast_path_fault_latched = false;
        s_selftest_result_fast_path_fault_latched = false;
        s_settings_snapshot_warm_fault_latched = false;
        s_settings_snapshot_prepare_count = 0U;
        s_settings_snapshot_prepare_failures = 0U;
        s_settings_snapshot_last_prepare_ms = 0U;
        s_settings_snapshot_max_prepare_ms = 0U;
        s_settings_snapshot_warm_count = 0U;
        s_settings_snapshot_warm_failures = 0U;
        s_settings_snapshot_max_warm_ms = 0U;
        s_settings_background_build_count = 0U;
        s_settings_content_rebuild_count = 0U;
        s_settings_content_swap_count = 0U;
        s_settings_ignored_refresh_count = 0U;
        s_selftest_result_snapshot_prepare_count = 0U;
        s_selftest_result_snapshot_prepare_failures = 0U;
        s_selftest_result_snapshot_last_prepare_ms = 0U;
        s_selftest_result_snapshot_max_prepare_ms = 0U;
        s_selftest_result_snapshot_warm_count = 0U;
        s_selftest_result_snapshot_warm_failures = 0U;
        s_selftest_result_snapshot_max_warm_ms = 0U;
        s_selftest_result_background_build_count = 0U;
        s_selftest_result_content_rebuild_count = 0U;
        s_selftest_result_content_swap_count = 0U;
        s_selftest_result_ignored_refresh_count = 0U;
        s_shell_snapshot_prepare_count = 0U;
        s_shell_snapshot_prepare_failures = 0U;
        s_shell_snapshot_last_prepare_ms = 0U;
        s_shell_snapshot_max_prepare_ms = 0U;
        s_shell_snapshot_warm_page_count = 0U;
        s_shell_snapshot_warm_failures = 0U;
        s_shell_snapshot_max_warm_page_ms = 0U;
        s_shell_ignored_refresh_count = 0U;
        s_shell_invalidated_page_count = 0U;
        memset(s_shell_visual_signature_valid,
               0,
               sizeof(s_shell_visual_signature_valid));
        memset(s_shell_target_generations,
               0,
               sizeof(s_shell_target_generations));
        memset(s_shell_snapshot_buffer_generations,
               0,
               sizeof(s_shell_snapshot_buffer_generations));
        invalidate_shell_projection_cache();
        for (ui_runtime_screen_t screen = UI_RUNTIME_SCREEN_MAIN;
             screen < UI_RUNTIME_SCREEN_COUNT;
             screen = (ui_runtime_screen_t)(screen + 1))
        {
            s_screen_projected_states[screen] =
                UI_MANIFEST_STATE_COUNT;
        }
        s_last_projected_state = UI_MANIFEST_STATE_COUNT;
        s_loaded_screen = UI_RUNTIME_SCREEN_MAIN;
        s_failed_index = 0U;
        s_pending_retry_item = SELFTEST_ITEM_COUNT;
        s_payment_lock_active = false;
        s_payment_power_key_navigation_active = false;
        s_pending_control_route = false;
        s_display_on = true;
        s_route_facts_valid = false;
        s_last_binding_present = false;
        s_last_ble_connected = false;
        s_last_unlock_session_valid = false;
        s_touch_run_id = 0U;
        s_touch_target_hit = false;
        s_touch_reply_pending = false;
        s_touch_progress = 0U;
        s_touch_x = 0;
        s_touch_y = 0;
        s_full_redraw_requested = false;
        configure_main_shell_interaction();
        configure_shell_snapshot_cache();
        if (ui_ui_main_shell_pager != NULL)
        {
            lv_obj_add_event_cb(ui_ui_main_shell_pager,
                                shell_scroll_end_event_cb,
                                LV_EVENT_SCROLL_BEGIN,
                                NULL);
            lv_obj_add_event_cb(ui_ui_main_shell_pager,
                                shell_scroll_end_event_cb,
                                LV_EVENT_SCROLL,
                                NULL);
            lv_obj_add_event_cb(ui_ui_main_shell_pager,
                                shell_scroll_end_event_cb,
                                LV_EVENT_SCROLL_END,
                                NULL);
        }
    }
    s_intent_handler = handler;
    s_intent_context = context;
}

bool ui_runtime_binding_render(const ui_runtime_model_t *model)
{
    if (!s_initialized || model == NULL || !model->snapshot_valid)
    {
        return false;
    }

#if SCENIC_AREA_MANAGEMENT_DEBUG
    if (s_route_facts_valid && !s_last_unlock_session_valid &&
        model->unlock_session_valid)
    {
        s_pending_control_route = true;
    }
#endif

    /* 授权锁保持控制零提交；检查中直接使用支付验证页。 */
    if (model->payment_required)
    {
        const bool authorization_checking =
            model->authorization_display == UI_AUTHORIZATION_DISPLAY_CHECKING ||
            model->authorization_pending;
        s_payment_lock_active = true;
        if (authorization_checking)
        {
            s_payment_power_key_navigation_active = false;
        }
        if (!is_settings_hierarchy_page(s_current_page) &&
            !s_payment_power_key_navigation_active)
        {
            s_shell_slot = UI_SHELL_SLOT_ALERTS;
            s_current_page =
                model->authorization_display ==
                        UI_AUTHORIZATION_DISPLAY_FAILED ||
                        (model->payment_attempted && model->payment_failed)
                    ? UI_PAGE_ALERTS_VERIFY_FAILED
                    : UI_PAGE_ALERTS_PAYMENT_VERIFY;
        }
    }
    else if (s_payment_lock_active)
    {
        s_payment_lock_active = false;
        s_payment_power_key_navigation_active = false;
        if (!is_settings_hierarchy_page(s_current_page) &&
            !s_pending_control_route)
        {
            s_shell_slot = UI_SHELL_SLOT_HOME;
            s_current_page = UI_PAGE_HOME_READY;
        }
    }
    else if (s_route_facts_valid &&
             ((!s_last_binding_present && model->binding_present) ||
              (s_last_binding_present && !model->binding_present)))
    {
        hide_clear_binding_modal();
        s_shell_slot = UI_SHELL_SLOT_HOME;
        s_current_page = UI_PAGE_HOME_READY;
    }
    else if (s_route_facts_valid && s_last_ble_connected &&
             !model->ble_connected && model->binding_present)
    {
        if (ui_navigation_is_shell_page(s_current_page))
        {
            route_to(shell_page_for_slot(s_shell_slot));
        }
        else
        {
            hide_clear_binding_modal();
            s_shell_slot = UI_SHELL_SLOT_HOME;
            route_to(UI_PAGE_HOME_READY);
        }
    }

    consume_pending_control_route(model);

    advance_selftest_route(model);

    if (s_current_page == UI_PAGE_CLEAR_BINDING_CONFIRM &&
        !model->binding_present)
    {
        hide_clear_binding_modal();
        s_shell_slot = UI_SHELL_SLOT_HOME;
        s_current_page = UI_PAGE_HOME_READY;
    }

    ui_page_id_t resolved_page = s_current_page;
    const ui_manifest_state_t state = resolve_static_state(model, &resolved_page);
    const ui_runtime_screen_t resolved_screen = screen_for_page(resolved_page);
    const bool redraw_variant_transition =
        s_loaded_screen == resolved_screen &&
        screen_needs_full_redraw(resolved_screen) &&
        s_last_projected_state != state;
    lv_disp_t *ble_atomic_display = NULL;
    const bool ble_atomic_redraw = begin_ble_atomic_redraw(
        resolved_screen,
        state,
        &ble_atomic_display);
    if (!prepare_page_screen(resolved_page))
    {
        finish_ble_atomic_redraw(ble_atomic_redraw, ble_atomic_display);
        return false;
    }

    /* 常驻主壳先投影四个槽位；按需页保持 canonical/variant 固定顺序。 */
    bool static_patch_applied = false;
    bool static_patch_changed = false;
    if (ui_navigation_is_shell_page(resolved_page))
    {
        static_patch_applied = apply_shell_static_patches(model);
    }
    else if (s_screen_projected_states[resolved_screen] == state)
    {
        static_patch_applied = true;
    }
    else
    {
        const ui_manifest_state_t canonical = canonical_base_state(state);
        static_patch_applied = apply_static_patch(canonical) &&
                               (state == canonical || apply_static_patch(state));
        if (static_patch_applied)
        {
            static_patch_changed = true;
            s_screen_projected_states[resolved_screen] = state;
            if (resolved_screen == UI_RUNTIME_SCREEN_SELFTEST_RESULT)
            {
                /*
                 * 全通过与有失败两种静态按钮布局不同；同屏 variant
                 * 切换只废弃背景，双内容前台仍保持完整可读。
                 */
                s_selftest_result_background_snapshot_valid = false;
            }
        }
    }
    if (!static_patch_applied)
    {
        const ui_runtime_screen_t prepared = screen_for_page(resolved_page);
        if (prepared != s_loaded_screen)
        {
            screen_destroy(prepared);
        }
        finish_ble_atomic_redraw(ble_atomic_redraw, ble_atomic_display);
        return false;
    }
    s_current_page = resolved_page;
    apply_selftest_lock(model);
    apply_model_projection(model);
    if (resolved_screen == UI_RUNTIME_SCREEN_SETTINGS)
    {
        if (static_patch_changed)
        {
            normalize_settings_interaction();
        }
        update_settings_visual_target(model);
    }
    if (resolved_page != UI_PAGE_SELFTEST_MANUAL ||
        !model->selftest_running ||
        model->selftest_item != SELFTEST_ITEM_AMOLED)
    {
        ui_runtime_binding_stop_amoled_test();
    }
    commit_page_screen(resolved_page);
    finish_ble_atomic_redraw(ble_atomic_redraw, ble_atomic_display);
    if (resolved_screen == UI_RUNTIME_SCREEN_SETTINGS &&
        s_loaded_screen == UI_RUNTIME_SCREEN_SETTINGS)
    {
        /*
         * 首次进入必须先完成背景和首张内容；后续相关设置变化只在
         * 后台内容缓冲完成后切换，下一次触摸绝不退回 LVGL 慢路。
         */
        lv_obj_update_layout(ui_ui_scr_settings);
        for (uint8_t build = 0U;
             build < UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT;
             ++build)
        {
            if (!ui_runtime_binding_warm_settings_snapshot())
            {
                break;
            }
        }
    }
    if (resolved_screen == UI_RUNTIME_SCREEN_SELFTEST_RESULT &&
        s_loaded_screen == UI_RUNTIME_SCREEN_SELFTEST_RESULT)
    {
        lv_obj_update_layout(ui_ui_scr_selftest_result);
        for (uint8_t build = 0U;
             build < UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT;
             ++build)
        {
            if (!ui_runtime_binding_warm_selftest_result_snapshot())
            {
                break;
            }
        }
    }
    if (redraw_variant_transition)
    {
        request_full_redraw();
    }
    s_last_projected_state = state;
    s_route_facts_valid = true;
    s_last_binding_present = model->binding_present;
    s_last_ble_connected = model->ble_connected;
    s_last_unlock_session_valid = model->unlock_session_valid;
    if (resolved_screen == UI_RUNTIME_SCREEN_MAIN)
    {
        update_shell_visual_targets(model);
    }
    return true;
}

void ui_runtime_binding_set_display_on(bool display_on)
{
    s_display_on = display_on;
}

uint8_t ui_runtime_binding_scene_target_for_intent(
    ui_runtime_intent_type_t type,
    watch_exoskeleton_scene_config_t scene_config)
{
    switch (type)
    {
    case UI_INTENT_SET_MODE_STANDARD:
        return 1U;
    case UI_INTENT_SET_MODE_EXTREME:
        return 3U;
    case UI_INTENT_SET_MODE_SPORT:
        return secondary_scene_target(scene_config);
    case UI_INTENT_SET_MODE_ECO:
        return 4U;
    default:
        return 0U;
    }
}

ui_runtime_intent_disposition_t ui_runtime_binding_accept_intent(
    const ui_runtime_intent_t *intent,
    const ui_runtime_model_t *model)
{
    if (!s_initialized || intent == NULL || model == NULL ||
        !model->snapshot_valid || intent->type < UI_INTENT_OPEN_SETTINGS ||
        intent->type >= UI_INTENT_COUNT ||
        intent->source < UI_RUNTIME_INTENT_SOURCE_BUTTON ||
        intent->source >= UI_RUNTIME_INTENT_SOURCE_COUNT)
    {
        return UI_RUNTIME_INTENT_REJECTED;
    }
    const bool authorization_checking =
        model->authorization_display == UI_AUTHORIZATION_DISPLAY_CHECKING ||
        model->authorization_pending;
    const bool payment_power_key_navigation =
        model->payment_required &&
        !authorization_checking &&
        intent->type == UI_INTENT_SHELL_NEXT &&
        intent->source == UI_RUNTIME_INTENT_SOURCE_POWER_KEY;
    if (model->payment_required &&
        !payment_power_key_navigation &&
        !payment_lock_allows_intent(intent->type))
    {
        return UI_RUNTIME_INTENT_REJECTED;
    }

    switch (intent->type)
    {
    case UI_INTENT_OPEN_SETTINGS:
        s_return_page = UI_PAGE_HOME_READY;
        s_settings_scroll_y = 0;
        route_to(UI_PAGE_SETTINGS);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_BACK_TO_SHELL:
        if (model->payment_required)
        {
            s_payment_power_key_navigation_active = false;
            s_shell_slot = UI_SHELL_SLOT_ALERTS;
            route_to(model->authorization_display ==
                                 UI_AUTHORIZATION_DISPLAY_FAILED ||
                             (model->payment_attempted &&
                              model->payment_failed)
                         ? UI_PAGE_ALERTS_VERIFY_FAILED
                         : UI_PAGE_ALERTS_PAYMENT_VERIFY);
        }
        else
        {
            s_shell_slot = UI_SHELL_SLOT_HOME;
            route_to(UI_PAGE_HOME_READY);
        }
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SHELL_HOME:
        s_shell_slot = UI_SHELL_SLOT_HOME;
        route_to(UI_PAGE_HOME_READY);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SHELL_CONTROL:
        s_shell_slot = UI_SHELL_SLOT_GEAR;
        route_to(UI_PAGE_GEAR_READY);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SHELL_ALERTS:
        s_shell_slot = UI_SHELL_SLOT_ALERTS;
        route_to(UI_PAGE_ALERTS_EMPTY);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SHELL_MODE_POWER:
        s_shell_slot = UI_SHELL_SLOT_MODE_POWER;
        route_to(UI_PAGE_MODE_POWER_READY);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SHELL_PREVIOUS:
        s_shell_slot = s_shell_slot > UI_SHELL_SLOT_HOME
                           ? (ui_shell_slot_t)(s_shell_slot - 1)
                           : (ui_shell_slot_t)(UI_SHELL_SLOT_COUNT - 1);
        route_to(shell_page_for_slot(s_shell_slot));
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SHELL_NEXT:
        if (payment_power_key_navigation)
        {
            s_payment_power_key_navigation_active = true;
        }
        s_shell_slot =
            (ui_shell_slot_t)((s_shell_slot + 1) % UI_SHELL_SLOT_COUNT);
        route_to(shell_page_for_slot(s_shell_slot));
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_OPEN_BLE_CANDIDATES:
        s_return_page = s_current_page;
        route_to(UI_PAGE_BLE_EMPTY);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_OPEN_DEVICE_INFO:
        route_to(UI_PAGE_DEVICE_INFO);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_OPEN_MAINTENANCE:
        route_to(UI_PAGE_MAINTENANCE);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_CONNECT_CELLULAR:
#if LEGBOT_CAP_MODEM
        return model->cellular_action_enabled
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
#else
        return UI_RUNTIME_INTENT_REJECTED;
#endif
    case UI_INTENT_REQUEST_GPS_ACQUISITION:
        return model->gps_action_enabled
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_BACK:
        route_back();
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_OPEN_SELFTEST:
        route_to(selftest_page_for_model(model));
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_OPEN_CLEAR_BINDING:
        if (model->binding_present && !model->clear_binding_pending)
        {
            route_to(UI_PAGE_CLEAR_BINDING_CONFIRM);
            show_clear_binding_modal(model);
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_CANCEL_CLEAR_BINDING:
        if (!model->clear_binding_pending)
        {
            hide_clear_binding_modal();
            route_to(UI_PAGE_MAINTENANCE);
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_FAILED_PREVIOUS:
        if (s_failed_index > 0U)
        {
            --s_failed_index;
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_FAILED_NEXT:
        if ((uint8_t)(s_failed_index + 1U) < failed_item_count(model))
        {
            ++s_failed_index;
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_OPEN_RETRY_FAILED:
        if (failed_item_count(model) > 0U)
        {
            s_failed_index = 0U;
            route_to(UI_PAGE_SELFTEST_RETRY_DETAIL);
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_RECONNECT_BLE:
        return model->binding_present && !model->ble_connected
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_GEAR_DECREMENT:
        return control_intent_allowed(model) &&
                       !model->gear_projection_locked && model->gear > 1U
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_GEAR_INCREMENT:
        return control_intent_allowed(model) &&
                       !model->gear_projection_locked &&
                       model->gear < watch_exoskeleton_scene_config_max_gear(
                                         model->scene_config)
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_TOGGLE_ASSIST:
        return control_intent_allowed(model) &&
                       !model->assist_projection_locked
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_SET_MODE_STANDARD:
    case UI_INTENT_SET_MODE_EXTREME:
    case UI_INTENT_SET_MODE_SPORT:
    case UI_INTENT_SET_MODE_ECO:
    {
        const uint8_t target = ui_runtime_binding_scene_target_for_intent(
            intent->type,
            model->scene_config);
        return control_intent_allowed(model) &&
                       watch_exoskeleton_scene_config_allows_control(
                           model->scene_config,
                           target) &&
                       !model->mode_projection_locked
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    }
    case UI_INTENT_EXO_POWEROFF:
        return control_intent_allowed(model) && model->power_action_enabled
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_VERIFY_PAYMENT:
        return model->payment_required && model->payment_action_enabled &&
                       !model->payment_pending
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_TOGGLE_HAPTICS:
    case UI_INTENT_TOGGLE_CLICK_AUDIO:
    case UI_INTENT_TOGGLE_RAISE_WAKE:
        return UI_RUNTIME_INTENT_SUBMIT_SERVICE;
    case UI_INTENT_SET_BRIGHTNESS:
        return intent->value < UI_RUNTIME_BRIGHTNESS_COUNT
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_SET_SCREEN_TIMEOUT:
        return intent->value == UI_RUNTIME_SCREEN_TIMEOUT_5S ||
                       intent->value == UI_RUNTIME_SCREEN_TIMEOUT_15S ||
                       intent->value == UI_RUNTIME_SCREEN_TIMEOUT_30S
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_CONFIRM_CLEAR_BINDING:
        return s_current_page == UI_PAGE_CLEAR_BINDING_CONFIRM &&
                       model->binding_present && !model->clear_binding_pending
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_RESCAN_BLE:
        return model->ble_scan_state != UI_RUNTIME_BLE_SCAN_ACTIVE
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_SELECT_BLE_CANDIDATE:
        return intent->value < model->ble_candidate_count &&
                       intent->value < UI_RUNTIME_BLE_CANDIDATE_COUNT &&
                       model->ble_connection_state != UI_RUNTIME_BLE_CONNECTION_CONNECTING
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_START_SELFTEST:
    case UI_INTENT_RERUN_SELFTEST:
        if (!model->selftest_running)
        {
            reset_touch_selftest();
#if LEGBOT_CAP_GPS_TIME
            s_gps_return_page = s_current_page == UI_PAGE_SELFTEST_RESULT
                                    ? UI_PAGE_SELFTEST_RESULT
                                    : UI_PAGE_SELFTEST_IDLE;
            route_to(UI_PAGE_SELFTEST_GPS_CONTEXT);
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
#else
            return UI_RUNTIME_INTENT_SUBMIT_SERVICE;
#endif
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_LEAVE_SELFTEST:
        s_pending_retry_item = SELFTEST_ITEM_COUNT;
        reset_touch_selftest();
        ui_runtime_binding_stop_amoled_test();
        route_to(UI_PAGE_MAINTENANCE);
        return UI_RUNTIME_INTENT_HANDLED_LOCAL;
    case UI_INTENT_SELFTEST_MANUAL_FAIL:
    case UI_INTENT_SELFTEST_MANUAL_PASS:
        if ((!model->selftest_running &&
             !(model->retry_running &&
               model->selftest_item == SELFTEST_ITEM_TOUCH)) ||
            model->manual_reply_pending ||
            (model->selftest_item != SELFTEST_ITEM_AMOLED &&
             model->selftest_item != SELFTEST_ITEM_TOUCH &&
             model->selftest_item != SELFTEST_ITEM_AUDIO &&
             model->selftest_item != SELFTEST_ITEM_VIBRATION))
        {
            return UI_RUNTIME_INTENT_REJECTED;
        }
        if (intent->type != UI_INTENT_SELFTEST_MANUAL_FAIL &&
            model->selftest_item == SELFTEST_ITEM_TOUCH &&
            (s_touch_run_id != model->selftest_run_id ||
             !s_touch_target_hit ||
             s_touch_progress <=
                 UI_SELFTEST_TOUCH_PASS_THRESHOLD_PERCENT))
        {
            return UI_RUNTIME_INTENT_REJECTED;
        }
        if (model->selftest_item == SELFTEST_ITEM_TOUCH)
        {
            reset_touch_selftest();
        }
        return UI_RUNTIME_INTENT_SUBMIT_SERVICE;
    case UI_INTENT_SELFTEST_TOUCH_TARGET:
        if ((model->selftest_running || model->retry_running) &&
            model->selftest_item == SELFTEST_ITEM_TOUCH &&
            !model->manual_reply_pending)
        {
            sync_touch_selftest_run(model->selftest_run_id);
            s_touch_target_hit = true;
            s_touch_x = (int16_t)((intent->value >> 16U) & 0xFFFFU);
            s_touch_y = (int16_t)(intent->value & 0xFFFFU);
            update_touch_selftest_visual();
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_SELFTEST_TOUCH_PROGRESS:
        if ((model->selftest_running || model->retry_running) &&
            model->selftest_item == SELFTEST_ITEM_TOUCH &&
            !model->manual_reply_pending)
        {
            sync_touch_selftest_run(model->selftest_run_id);
            const uint8_t progress = intent->value > 100U
                                         ? 100U
                                         : (uint8_t)intent->value;
            (void)update_touch_progress_local(progress);
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        return UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_SELFTEST_OUTPUT_REPLAY:
        return model->selftest_running &&
                       (model->selftest_item == SELFTEST_ITEM_AUDIO ||
                        model->selftest_item == SELFTEST_ITEM_VIBRATION) &&
                       !model->manual_reply_pending
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    case UI_INTENT_GPS_OUTDOOR:
    case UI_INTENT_GPS_INDOOR:
#if LEGBOT_CAP_GPS_TIME
        return model->gps_submit_state != UI_RUNTIME_GPS_SUBMIT_LOADING
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
#else
        return UI_RUNTIME_INTENT_REJECTED;
#endif
    case UI_INTENT_RETRY_FAILED_ITEM:
        if (model->retry_running || failed_item_count(model) == 0U)
        {
            return UI_RUNTIME_INTENT_REJECTED;
        }
        reset_touch_selftest();
        s_pending_retry_item = failed_item_at(model, s_failed_index);
        if (s_pending_retry_item == SELFTEST_ITEM_GPS_FIX &&
            LEGBOT_CAP_GPS_TIME)
        {
            s_gps_return_page = UI_PAGE_SELFTEST_RESULT;
            route_to(UI_PAGE_SELFTEST_GPS_CONTEXT);
            return UI_RUNTIME_INTENT_HANDLED_LOCAL;
        }
        /* 非 GPS 重试等待 owner 的 retry_active/current_item 事实再跳页。 */
        return s_pending_retry_item < SELFTEST_ITEM_COUNT
                   ? UI_RUNTIME_INTENT_SUBMIT_SERVICE
                   : UI_RUNTIME_INTENT_REJECTED;
    default:
        return UI_RUNTIME_INTENT_REJECTED;
    }
}

void ui_runtime_binding_deinit(void)
{
    if (!s_initialized)
    {
        return;
    }
    ui_runtime_binding_stop_amoled_test();
    hide_clear_binding_modal();
    if (s_loaded_screen != UI_RUNTIME_SCREEN_MAIN)
    {
        lv_scr_load(ui_ui_scr_main_shell);
        screen_destroy(s_loaded_screen);
    }
    set_shell_snapshot_active(false);
    set_settings_snapshot_active(false);
    set_selftest_result_snapshot_active(false);
    ui_destroy();
    release_shell_snapshot_cache();
    release_settings_snapshot_cache();
    release_selftest_result_snapshot_cache();
    s_intent_handler = NULL;
    s_intent_context = NULL;
    s_pending_retry_item = SELFTEST_ITEM_COUNT;
    s_gps_return_page = UI_PAGE_SELFTEST_IDLE;
    s_committed_shell_slot = UI_SHELL_SLOT_COUNT;
    s_settings_scroll_y = 0;
    s_shell_scroll_peak_delta = 0;
    s_shell_motion_active = false;
    s_settings_motion_active = false;
    s_selftest_result_motion_active = false;
    s_shell_fast_path_fault_latched = false;
    s_settings_fast_path_fault_latched = false;
    s_selftest_result_fast_path_fault_latched = false;
    invalidate_shell_projection_cache();
    s_last_projected_state = UI_MANIFEST_STATE_COUNT;
    s_payment_lock_active = false;
    s_payment_power_key_navigation_active = false;
    s_pending_control_route = false;
    s_display_on = true;
    s_route_facts_valid = false;
    s_last_unlock_session_valid = false;
    s_touch_run_id = 0U;
    s_touch_target_hit = false;
    s_touch_reply_pending = false;
    s_touch_progress = 0U;
    s_full_redraw_requested = false;
    ui_touch_session_reset();
    s_initialized = false;
}

bool ui_runtime_binding_start_amoled_test(void)
{
    if (!s_initialized)
    {
        return false;
    }
    if (s_amoled_surface != NULL)
    {
        return true;
    }
    s_amoled_surface = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(s_amoled_surface, 0, 0);
    lv_obj_set_size(s_amoled_surface, 410, 502);
    lv_obj_add_flag(s_amoled_surface, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_amoled_surface, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_border_width(s_amoled_surface, 0, 0);
    lv_obj_set_style_radius(s_amoled_surface, 0, 0);
    s_amoled_color_index = 0U;
    set_amoled_color(s_amoled_color_index);
    s_amoled_timer = lv_timer_create(amoled_timer_cb,
                                     UI_AMOLED_COLOR_DURATION_MS,
                                     NULL);
    if (s_amoled_timer == NULL)
    {
        lv_obj_del(s_amoled_surface);
        s_amoled_surface = NULL;
        return false;
    }
    return true;
}

void ui_runtime_binding_stop_amoled_test(void)
{
    const bool surface_removed = s_amoled_surface != NULL;
    if (s_amoled_timer != NULL)
    {
        lv_timer_del(s_amoled_timer);
        s_amoled_timer = NULL;
    }
    if (s_amoled_surface != NULL)
    {
        lv_obj_del(s_amoled_surface);
        s_amoled_surface = NULL;
    }
    s_amoled_color_index = 0U;
    if (surface_removed)
    {
        request_full_redraw();
    }
}

bool ui_runtime_binding_shell_motion_active(void)
{
    return s_initialized && s_loaded_screen == UI_RUNTIME_SCREEN_MAIN &&
           s_shell_motion_active;
}

bool ui_runtime_binding_settings_motion_active(void)
{
    return s_initialized &&
           s_loaded_screen == UI_RUNTIME_SCREEN_SETTINGS &&
           s_settings_motion_active;
}

bool ui_runtime_binding_selftest_result_motion_active(void)
{
    return s_initialized &&
           s_loaded_screen == UI_RUNTIME_SCREEN_SELFTEST_RESULT &&
           s_selftest_result_motion_active;
}

bool ui_runtime_binding_motion_active(void)
{
    return ui_runtime_binding_shell_motion_active() ||
           ui_runtime_binding_settings_motion_active() ||
           ui_runtime_binding_selftest_result_motion_active();
}

bool ui_runtime_binding_ble_candidate_page_active(void)
{
    return s_initialized &&
           s_loaded_screen == UI_RUNTIME_SCREEN_BLE;
}

void ui_runtime_binding_shell_snapshot_stats(
    ui_runtime_shell_snapshot_stats_t *stats)
{
    if (stats == NULL)
    {
        return;
    }
    static const int8_t relative_order
        [UI_SHELL_SNAPSHOT_BUFFER_COUNT] = {0, -1, 1};
    uint8_t dirty_page_count = 0U;
    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        const ui_shell_slot_t slot = (ui_shell_slot_t)(
            (s_shell_slot + UI_SHELL_SLOT_COUNT +
             relative_order[index]) %
            UI_SHELL_SLOT_COUNT);
        const int8_t buffer = shell_snapshot_buffer_for_slot(slot);
        if (buffer < 0 || s_shell_snapshot_dirty[slot] ||
            !s_shell_snapshot_slot_valid[(uint8_t)buffer] ||
            s_shell_snapshot_buffer_generations[(uint8_t)buffer] !=
                s_shell_target_generations[slot])
        {
            ++dirty_page_count;
        }
    }
    *stats = (ui_runtime_shell_snapshot_stats_t){
        .available = s_initialized && s_shell_snapshot_available,
        .active = s_initialized && s_shell_snapshot_active,
        .allocated_bytes =
            s_shell_snapshot_available
                ? s_shell_snapshot_buffer_bytes *
                      UI_SHELL_SNAPSHOT_BUFFER_COUNT
                : 0U,
        .prepare_count = s_shell_snapshot_prepare_count,
        .prepare_failures = s_shell_snapshot_prepare_failures,
        .last_prepare_ms = s_shell_snapshot_last_prepare_ms,
        .max_prepare_ms = s_shell_snapshot_max_prepare_ms,
        .warm_page_count = s_shell_snapshot_warm_page_count,
        .warm_failures = s_shell_snapshot_warm_failures,
        .max_warm_page_ms = s_shell_snapshot_max_warm_page_ms,
        .ignored_refresh_count = s_shell_ignored_refresh_count,
        .invalidated_page_count = s_shell_invalidated_page_count,
        .dirty_page_count = dirty_page_count,
    };
}

bool ui_runtime_binding_warm_shell_snapshot(void)
{
    if (!s_initialized || !s_shell_snapshot_available ||
        s_loaded_screen != UI_RUNTIME_SCREEN_MAIN ||
        s_shell_motion_active || s_shell_snapshot_active ||
        shell_touch_sequence_active())
    {
        return false;
    }

    static const int8_t relative_order[UI_SHELL_SNAPSHOT_BUFFER_COUNT] = {
        0,
        -1,
        1,
    };
    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        const ui_shell_slot_t slot = (ui_shell_slot_t)(
            (s_shell_slot + UI_SHELL_SLOT_COUNT +
             relative_order[index]) %
            UI_SHELL_SLOT_COUNT);
        const int8_t existing_buffer =
            shell_snapshot_buffer_for_slot(slot);
        if (existing_buffer >= 0 &&
            s_shell_snapshot_slot_valid[(uint8_t)existing_buffer] &&
            !s_shell_snapshot_dirty[slot] &&
            s_shell_snapshot_buffer_generations
                    [(uint8_t)existing_buffer] ==
                s_shell_target_generations[slot])
        {
            continue;
        }

        const int8_t selected_buffer =
            existing_buffer >= 0
                ? existing_buffer
                : shell_snapshot_replacement_buffer(slot);
        lv_obj_t *roots[UI_SHELL_SLOT_COUNT] = {0};
        shell_page_objects(NULL, roots);
        if (selected_buffer < 0 || roots[slot] == NULL ||
            s_shell_snapshot_images[slot] == NULL)
        {
            ++s_shell_snapshot_warm_failures;
            return false;
        }

        const int64_t started_us = esp_timer_get_time();
        const uint8_t buffer_index = (uint8_t)selected_buffer;
        const lv_res_t result = lv_snapshot_take_to_buf(
            roots[slot],
            LV_IMG_CF_TRUE_COLOR,
            &s_shell_snapshot_descriptors[buffer_index],
            s_shell_snapshot_buffers[buffer_index],
            (uint32_t)s_shell_snapshot_buffer_bytes);
        const uint32_t warm_duration_ms = (uint32_t)(
            (esp_timer_get_time() - started_us + 999) / 1000);
        if (warm_duration_ms > s_shell_snapshot_max_warm_page_ms)
        {
            s_shell_snapshot_max_warm_page_ms = warm_duration_ms;
        }
        if (result != LV_RES_OK)
        {
            ++s_shell_snapshot_warm_failures;
            s_shell_snapshot_slot_valid[buffer_index] = false;
            ESP_LOGE(TAG,
                     "常驻横滑闲时单页快照生成失败：槽位=%u，耗时=%lums",
                     (unsigned)slot,
                     (unsigned long)warm_duration_ms);
            return false;
        }

        s_shell_snapshot_slots[buffer_index] = slot;
        s_shell_snapshot_slot_valid[buffer_index] = true;
        s_shell_snapshot_buffer_generations[buffer_index] =
            s_shell_target_generations[slot];
        s_shell_snapshot_dirty[slot] = false;
        lv_img_set_src(s_shell_snapshot_images[slot],
                       &s_shell_snapshot_descriptors[buffer_index]);
        ++s_shell_snapshot_warm_page_count;
        return true;
    }
    return false;
}

bool ui_runtime_binding_shell_fast_frame(
    ui_runtime_shell_fast_frame_t *frame)
{
    if (frame == NULL || !s_initialized || !s_shell_motion_active ||
        !s_shell_snapshot_active || !shell_snapshot_cache_ready() ||
        ui_ui_main_shell_pager == NULL)
    {
        return false;
    }

    const ui_shell_slot_t left_slot = (ui_shell_slot_t)(
        (s_shell_slot + UI_SHELL_SLOT_COUNT - 1U) %
        UI_SHELL_SLOT_COUNT);
    const ui_shell_slot_t right_slot = (ui_shell_slot_t)(
        (s_shell_slot + 1U) % UI_SHELL_SLOT_COUNT);
    const int8_t current_buffer =
        shell_snapshot_buffer_for_slot(s_shell_slot);
    const int8_t left_buffer =
        shell_snapshot_buffer_for_slot(left_slot);
    const int8_t right_buffer =
        shell_snapshot_buffer_for_slot(right_slot);
    if (current_buffer < 0 || left_buffer < 0 || right_buffer < 0)
    {
        return false;
    }

    int32_t offset_px =
        (int32_t)lv_obj_get_scroll_x(ui_ui_main_shell_pager) -
        UI_SHELL_PAGE_WIDTH;
    if (offset_px < -UI_SHELL_PAGE_WIDTH)
    {
        offset_px = -UI_SHELL_PAGE_WIDTH;
    }
    else if (offset_px > UI_SHELL_PAGE_WIDTH)
    {
        offset_px = UI_SHELL_PAGE_WIDTH;
    }
    *frame = (ui_runtime_shell_fast_frame_t){
        .current_pixels =
            s_shell_snapshot_buffers[(uint8_t)current_buffer],
        .left_pixels =
            s_shell_snapshot_buffers[(uint8_t)left_buffer],
        .right_pixels =
            s_shell_snapshot_buffers[(uint8_t)right_buffer],
        .offset_px = (int16_t)offset_px,
    };
    return true;
}

void ui_runtime_binding_settings_snapshot_stats(
    ui_runtime_settings_snapshot_stats_t *stats)
{
    if (stats == NULL)
    {
        return;
    }
    *stats = (ui_runtime_settings_snapshot_stats_t){
        .available = s_initialized && s_settings_snapshot_available,
        .active = s_initialized && s_settings_snapshot_active,
        .allocated_bytes =
            s_settings_snapshot_available
                ? s_settings_background_snapshot_bytes +
                      UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT *
                          s_settings_content_snapshot_bytes
                : 0U,
        .prepare_count = s_settings_snapshot_prepare_count,
        .prepare_failures = s_settings_snapshot_prepare_failures,
        .last_prepare_ms = s_settings_snapshot_last_prepare_ms,
        .max_prepare_ms = s_settings_snapshot_max_prepare_ms,
        .warm_count = s_settings_snapshot_warm_count,
        .warm_failures = s_settings_snapshot_warm_failures,
        .max_warm_ms = s_settings_snapshot_max_warm_ms,
        .background_build_count = s_settings_background_build_count,
        .content_rebuild_count = s_settings_content_rebuild_count,
        .content_swap_count = s_settings_content_swap_count,
        .ignored_refresh_count = s_settings_ignored_refresh_count,
        .target_generation = s_settings_target_generation,
        .front_generation = s_settings_front_generation,
    };
}

bool ui_runtime_binding_warm_settings_snapshot(void)
{
    if (!s_initialized || !s_settings_snapshot_available ||
        s_settings_snapshot_warm_fault_latched ||
        s_loaded_screen != UI_RUNTIME_SCREEN_SETTINGS ||
        s_settings_motion_active || s_settings_snapshot_active)
    {
        return false;
    }

    lv_obj_t *snapshot_object = NULL;
    lv_img_dsc_t *descriptor = NULL;
    void *buffer = NULL;
    size_t buffer_bytes = 0U;
    bool background_snapshot = false;
    uint8_t content_buffer_index =
        UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE;
    if (!s_settings_background_snapshot_valid)
    {
        snapshot_object = ui_ui_scr_settings;
        descriptor = &s_settings_background_snapshot_descriptor;
        buffer = s_settings_background_snapshot_buffer;
        buffer_bytes = s_settings_background_snapshot_bytes;
        background_snapshot = true;
    }
    else if (s_settings_visual_signature_valid &&
             (s_settings_content_front_index ==
                  UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE ||
              s_settings_front_generation !=
                  s_settings_target_generation))
    {
        /*
         * manifest 静态状态保留 SquareLine 的 688 px 原始高度，
         * 内容快照合同固定为 378×608；每次后台重建前重申运行时几何，
         * 避免状态重放后缓冲尺寸与对象尺寸分叉。
         */
        normalize_settings_interaction();
        lv_obj_update_layout(ui_ui_settings_settings_list_content);
        content_buffer_index =
            s_settings_content_front_index ==
                    UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE
                ? 0U
                : (uint8_t)(1U - s_settings_content_front_index);
        snapshot_object = ui_ui_settings_settings_list_content;
        descriptor =
            &s_settings_content_snapshot_descriptors[content_buffer_index];
        buffer =
            s_settings_content_snapshot_buffers[content_buffer_index];
        buffer_bytes = s_settings_content_snapshot_bytes;
    }
    else
    {
        return false;
    }
    if (snapshot_object == NULL || descriptor == NULL ||
        buffer == NULL || buffer_bytes == 0U)
    {
        ++s_settings_snapshot_warm_failures;
        s_settings_snapshot_warm_fault_latched = true;
        return false;
    }

    const uint32_t required_bytes =
        lv_snapshot_buf_size_needed(snapshot_object,
                                    LV_IMG_CF_TRUE_COLOR);
    if (required_bytes == 0U || buffer_bytes < required_bytes)
    {
        ++s_settings_snapshot_warm_failures;
        s_settings_snapshot_warm_fault_latched = true;
        ESP_LOGE(TAG,
                 "设置纵滑闲时%s快照尺寸失配并已熔断：对象=%dx%d，需要=%lu，已分配=%lu",
                 background_snapshot ? "背景" : "七栏内容",
                 (int)lv_obj_get_width(snapshot_object),
                 (int)lv_obj_get_height(snapshot_object),
                 (unsigned long)required_bytes,
                 (unsigned long)buffer_bytes);
        return false;
    }

    const bool viewport_was_hidden =
        ui_ui_settings_settings_list_viewport != NULL &&
        lv_obj_has_flag(ui_ui_settings_settings_list_viewport,
                        LV_OBJ_FLAG_HIDDEN);
    const bool thumb_was_hidden =
        ui_ui_settings_settings_page_thumb != NULL &&
        lv_obj_has_flag(ui_ui_settings_settings_page_thumb,
                        LV_OBJ_FLAG_HIDDEN);
    if (background_snapshot)
    {
        lv_obj_add_flag(ui_ui_settings_settings_list_viewport,
                        LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(ui_ui_settings_settings_page_thumb,
                        LV_OBJ_FLAG_HIDDEN);
    }

    const int64_t started_us = esp_timer_get_time();
    const lv_res_t result = lv_snapshot_take_to_buf(
        snapshot_object,
        LV_IMG_CF_TRUE_COLOR,
        descriptor,
        buffer,
        (uint32_t)buffer_bytes);
    const uint32_t duration_ms = (uint32_t)(
        (esp_timer_get_time() - started_us + 999) / 1000);
    if (duration_ms > s_settings_snapshot_max_warm_ms)
    {
        s_settings_snapshot_max_warm_ms = duration_ms;
    }

    if (background_snapshot)
    {
        if (!viewport_was_hidden)
        {
            lv_obj_clear_flag(ui_ui_settings_settings_list_viewport,
                              LV_OBJ_FLAG_HIDDEN);
        }
        if (!thumb_was_hidden)
        {
            lv_obj_clear_flag(ui_ui_settings_settings_page_thumb,
                              LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (result != LV_RES_OK)
    {
        ++s_settings_snapshot_warm_failures;
        s_settings_snapshot_warm_fault_latched = true;
        ESP_LOGE(TAG,
                 "设置纵滑闲时%s快照生成失败并已熔断：耗时=%lums，需要=%lu，已分配=%lu",
                 background_snapshot ? "背景" : "七栏内容",
                 (unsigned long)duration_ms,
                 (unsigned long)required_bytes,
                 (unsigned long)buffer_bytes);
        return false;
    }

    if (background_snapshot)
    {
        s_settings_background_snapshot_valid = true;
        ++s_settings_background_build_count;
    }
    else
    {
        const bool replacing_front =
            s_settings_content_front_index !=
            UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE;
        s_settings_content_snapshot_valid[content_buffer_index] = true;
        /*
         * lv_snapshot_take_to_buf 成功返回后缓冲才成为前台；
         * 失败或生成中始终保留上一张完整快照。
         */
        s_settings_content_front_index = content_buffer_index;
        s_settings_front_generation = s_settings_target_generation;
        ++s_settings_content_rebuild_count;
        if (replacing_front)
        {
            ++s_settings_content_swap_count;
        }
    }
    ++s_settings_snapshot_warm_count;
    return true;
}

bool ui_runtime_binding_settings_fast_frame(
    ui_runtime_settings_fast_frame_t *frame)
{
    if (frame == NULL || !s_initialized ||
        !s_settings_motion_active ||
        !s_settings_snapshot_active ||
        !settings_snapshot_cache_ready() ||
        ui_ui_settings_settings_list_viewport == NULL)
    {
        return false;
    }

    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_settings_settings_list_viewport);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    else if (scroll_y > UI_SETTINGS_LIST_MAX_SCROLL_Y)
    {
        scroll_y = UI_SETTINGS_LIST_MAX_SCROLL_Y;
    }
    const int32_t thumb_range =
        UI_SETTINGS_SCROLL_THUMB_BOTTOM_Y -
        UI_SETTINGS_SCROLL_THUMB_TOP_Y;
    const uint16_t thumb_y = (uint16_t)(
        UI_SETTINGS_SCROLL_THUMB_TOP_Y +
        scroll_y * thumb_range /
            UI_SETTINGS_LIST_MAX_SCROLL_Y);
    *frame = (ui_runtime_settings_fast_frame_t){
        .background_pixels = s_settings_background_snapshot_buffer,
        .content_pixels =
            s_settings_content_snapshot_buffers
                [s_settings_content_front_index],
        .viewport_x = UI_SETTINGS_LIST_VIEWPORT_X,
        .viewport_y = UI_SETTINGS_LIST_VIEWPORT_Y,
        .viewport_width = UI_SETTINGS_LIST_WIDTH,
        .viewport_height = UI_SETTINGS_LIST_VIEWPORT_HEIGHT,
        .content_width = UI_SETTINGS_LIST_WIDTH,
        .content_height = UI_SETTINGS_LIST_CONTENT_HEIGHT,
        .scroll_y = (uint16_t)scroll_y,
        .thumb_x = UI_SETTINGS_SCROLL_THUMB_X,
        .thumb_y = thumb_y,
        .thumb_width = UI_SETTINGS_SCROLL_THUMB_WIDTH,
        .thumb_height = UI_SETTINGS_SCROLL_THUMB_HEIGHT,
        .thumb_color = lv_color_hex(
                           UI_SETTINGS_SCROLL_THUMB_COLOR)
                           .full,
    };
    return true;
}

void ui_runtime_binding_selftest_result_snapshot_stats(
    ui_runtime_selftest_result_snapshot_stats_t *stats)
{
    if (stats == NULL)
    {
        return;
    }
    *stats = (ui_runtime_selftest_result_snapshot_stats_t){
        .available =
            s_initialized && s_selftest_result_snapshot_available,
        .active = s_initialized && s_selftest_result_snapshot_active,
        .allocated_bytes =
            s_selftest_result_snapshot_available
                ? s_selftest_result_background_snapshot_bytes +
                      UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT *
                          s_selftest_result_content_snapshot_bytes
                : 0U,
        .prepare_count = s_selftest_result_snapshot_prepare_count,
        .prepare_failures =
            s_selftest_result_snapshot_prepare_failures,
        .last_prepare_ms =
            s_selftest_result_snapshot_last_prepare_ms,
        .max_prepare_ms = s_selftest_result_snapshot_max_prepare_ms,
        .warm_count = s_selftest_result_snapshot_warm_count,
        .warm_failures = s_selftest_result_snapshot_warm_failures,
        .max_warm_ms = s_selftest_result_snapshot_max_warm_ms,
        .background_build_count =
            s_selftest_result_background_build_count,
        .content_rebuild_count =
            s_selftest_result_content_rebuild_count,
        .content_swap_count = s_selftest_result_content_swap_count,
        .ignored_refresh_count =
            s_selftest_result_ignored_refresh_count,
        .target_generation = s_selftest_result_target_generation,
        .front_generation = s_selftest_result_front_generation,
    };
}

bool ui_runtime_binding_selftest_result_fast_frame(
    ui_runtime_selftest_result_fast_frame_t *frame)
{
    if (frame == NULL || !s_initialized ||
        !s_selftest_result_motion_active ||
        !s_selftest_result_snapshot_active ||
        !selftest_result_snapshot_cache_ready() ||
        ui_ui_selftest_result_result_list == NULL)
    {
        return false;
    }

    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_selftest_result_result_list);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    else if (scroll_y > s_selftest_result_scroll_range)
    {
        scroll_y = s_selftest_result_scroll_range;
    }
    const uint16_t thumb_range = (uint16_t)(
        UI_SELFTEST_SCROLL_TRACK_HEIGHT -
        s_selftest_result_thumb_height);
    const uint16_t thumb_y = (uint16_t)(
        UI_SELFTEST_SCROLL_TRACK_Y +
        (s_selftest_result_scroll_range > 0U
             ? (uint32_t)thumb_range * (uint32_t)scroll_y /
                   s_selftest_result_scroll_range
             : 0U));
    const lv_img_dsc_t *content_descriptor =
        &s_selftest_result_content_snapshot_descriptors
            [s_selftest_result_content_front_index];
    *frame = (ui_runtime_selftest_result_fast_frame_t){
        .background_pixels =
            s_selftest_result_background_snapshot_buffer,
        .content_pixels =
            s_selftest_result_content_snapshot_buffers
                [s_selftest_result_content_front_index],
        .viewport_x = UI_SELFTEST_RESULT_VIEWPORT_X,
        .viewport_y = UI_SELFTEST_RESULT_VIEWPORT_Y,
        .viewport_width = UI_SELFTEST_RESULT_VIEWPORT_WIDTH,
        .viewport_height = UI_SELFTEST_RESULT_VIEWPORT_HEIGHT,
        .content_width = UI_SELFTEST_RESULT_VIEWPORT_WIDTH,
        .content_height = content_descriptor->header.h,
        .scroll_y = (uint16_t)scroll_y,
        .thumb_x = UI_SELFTEST_RESULT_THUMB_X,
        .thumb_y = thumb_y,
        .thumb_width = UI_SELFTEST_RESULT_THUMB_WIDTH,
        .thumb_height = s_selftest_result_thumb_height,
        .thumb_color =
            lv_color_hex(UI_SELFTEST_RESULT_THUMB_COLOR).full,
    };
    return true;
}

void ui_runtime_binding_abort_fast_path(void)
{
    s_shell_fast_path_fault_latched =
        s_shell_motion_active || s_shell_snapshot_active;
    s_settings_fast_path_fault_latched =
        s_settings_motion_active || s_settings_snapshot_active;
    s_selftest_result_fast_path_fault_latched =
        s_selftest_result_motion_active ||
        s_selftest_result_snapshot_active;
    s_settings_motion_active = false;
    s_selftest_result_motion_active = false;
    set_shell_snapshot_active(false);
    set_settings_snapshot_active(false);
    set_selftest_result_snapshot_active(false);
    request_full_redraw();
}

bool ui_runtime_binding_take_full_redraw_request(void)
{
    const bool requested = s_full_redraw_requested;
    s_full_redraw_requested = false;
    return requested;
}

void ui_runtime_binding_request_full_redraw(void)
{
    request_full_redraw();
}

size_t ui_runtime_binding_resolver_count(void)
{
    return sizeof(s_phase3_resolvers) / sizeof(s_phase3_resolvers[0]);
}

selftest_item_id_t ui_runtime_binding_selected_failed_item(
    const ui_runtime_model_t *model)
{
    return model != NULL ? failed_item_at(model, s_failed_index)
                         : SELFTEST_ITEM_COUNT;
}

selftest_item_id_t ui_runtime_binding_pending_retry_item(void)
{
    return s_pending_retry_item;
}

void ui_runtime_binding_clear_pending_retry(void)
{
    s_pending_retry_item = SELFTEST_ITEM_COUNT;
}

void ui_runtime_binding_emit_intent(ui_runtime_intent_type_t type,
                                    uint32_t value,
                                    ui_runtime_intent_source_t source)
{
    if (s_intent_handler == NULL || type < UI_INTENT_OPEN_SETTINGS ||
        type >= UI_INTENT_COUNT ||
        source < UI_RUNTIME_INTENT_SOURCE_BUTTON ||
        source >= UI_RUNTIME_INTENT_SOURCE_COUNT)
    {
        return;
    }
    if (source == UI_RUNTIME_INTENT_SOURCE_BUTTON &&
        button_intent_blocked_by_gesture())
    {
        lv_indev_t *input = lv_indev_get_act();
        ESP_LOGI(TAG,
                 "按钮点击被滑动手势抑制，intent=%d，方向=0x%x",
                 (int)type,
                 input != NULL
                     ? (unsigned)lv_indev_get_gesture_dir(input)
                     : 0U);
        return;
    }
    const ui_runtime_intent_t intent = {
        .type = type,
        .source = source,
        .value = value,
    };
    s_intent_handler(&intent, s_intent_context);
}

static ui_runtime_screen_t screen_for_page(ui_page_id_t page)
{
    if (ui_navigation_is_shell_page(page))
    {
        return UI_RUNTIME_SCREEN_MAIN;
    }
    switch (page)
    {
    case UI_PAGE_SETTINGS:
        return UI_RUNTIME_SCREEN_SETTINGS;
    case UI_PAGE_DEVICE_INFO:
        return UI_RUNTIME_SCREEN_DEVICE_INFO;
    case UI_PAGE_MAINTENANCE:
    case UI_PAGE_CLEAR_BINDING_CONFIRM:
        return UI_RUNTIME_SCREEN_MAINTENANCE;
    case UI_PAGE_BLE_EMPTY:
    case UI_PAGE_BLE_CANDIDATES:
        return UI_RUNTIME_SCREEN_BLE;
    case UI_PAGE_SELFTEST_IDLE:
    case UI_PAGE_SELFTEST_RUNNING:
    case UI_PAGE_SELFTEST_MANUAL:
        return UI_RUNTIME_SCREEN_SELFTEST;
    case UI_PAGE_SELFTEST_RESULT:
        return UI_RUNTIME_SCREEN_SELFTEST_RESULT;
    case UI_PAGE_SELFTEST_GPS_CONTEXT:
        return UI_RUNTIME_SCREEN_GPS_CONTEXT;
    case UI_PAGE_SELFTEST_RETRY_DETAIL:
        return UI_RUNTIME_SCREEN_RETRY_DETAIL;
    default:
        return UI_RUNTIME_SCREEN_MAIN;
    }
}

static lv_obj_t **screen_object(ui_runtime_screen_t screen)
{
    switch (screen)
    {
    case UI_RUNTIME_SCREEN_MAIN:
        return &ui_ui_scr_main_shell;
    case UI_RUNTIME_SCREEN_SETTINGS:
        return &ui_ui_scr_settings;
    case UI_RUNTIME_SCREEN_DEVICE_INFO:
        return &ui_ui_scr_device_info;
    case UI_RUNTIME_SCREEN_MAINTENANCE:
        return &ui_ui_scr_maintenance;
    case UI_RUNTIME_SCREEN_BLE:
        return &ui_ui_scr_ble_candidates;
    case UI_RUNTIME_SCREEN_SELFTEST:
        return &ui_ui_scr_selftest_progress;
    case UI_RUNTIME_SCREEN_SELFTEST_RESULT:
        return &ui_ui_scr_selftest_result;
    case UI_RUNTIME_SCREEN_GPS_CONTEXT:
        return &ui_ui_scr_selftest_gps_context;
    case UI_RUNTIME_SCREEN_RETRY_DETAIL:
        return &ui_ui_scr_selftest_retry_detail;
    default:
        return NULL;
    }
}

static void screen_init(ui_runtime_screen_t screen)
{
    switch (screen)
    {
    case UI_RUNTIME_SCREEN_SETTINGS:
        ui_ui_scr_settings_screen_init();
        remove_redundant_temporary_clipping(screen);
        configure_settings_scroll();
        configure_settings_snapshot_cache();
        break;
    case UI_RUNTIME_SCREEN_DEVICE_INFO:
        ui_ui_scr_device_info_screen_init();
        remove_redundant_temporary_clipping(screen);
        break;
    case UI_RUNTIME_SCREEN_MAINTENANCE:
        ui_ui_scr_maintenance_screen_init();
        remove_redundant_temporary_clipping(screen);
        if (ui_ui_maintenance_row_gps != NULL)
        {
            lv_obj_add_flag(ui_ui_maintenance_row_gps,
                            LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(ui_ui_maintenance_row_gps,
                                maintenance_gps_event_cb,
                                LV_EVENT_CLICKED,
                                NULL);
        }
        break;
    case UI_RUNTIME_SCREEN_BLE:
        ui_ui_scr_ble_candidates_screen_init();
        remove_redundant_temporary_clipping(screen);
        break;
    case UI_RUNTIME_SCREEN_SELFTEST:
        ui_ui_scr_selftest_progress_screen_init();
        remove_redundant_temporary_clipping(screen);
        if (ui_ui_selftest_manual_touch_touch_target != NULL)
        {
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_target,
                            LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(ui_ui_selftest_manual_touch_touch_target,
                                touch_target_event_cb,
                                LV_EVENT_CLICKED,
                                NULL);
        }
        if (ui_ui_selftest_manual_touch_touch_slider != NULL)
        {
            lv_obj_add_flag(ui_ui_selftest_manual_touch_touch_slider,
                            LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(ui_ui_selftest_manual_touch_touch_slider,
                                touch_slider_event_cb,
                                LV_EVENT_PRESSING,
                                NULL);
        }
        if (ui_ui_selftest_manual_val_manual_countdown != NULL)
        {
            lv_obj_add_flag(ui_ui_selftest_manual_val_manual_countdown,
                            LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(ui_ui_selftest_manual_val_manual_countdown,
                                output_replay_event_cb,
                                LV_EVENT_CLICKED,
                                NULL);
        }
        if (ui_ui_selftest_manual_lbl_manual_seconds != NULL)
        {
            lv_obj_add_flag(ui_ui_selftest_manual_lbl_manual_seconds,
                            LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(ui_ui_selftest_manual_lbl_manual_seconds,
                                output_replay_event_cb,
                                LV_EVENT_CLICKED,
                                NULL);
        }
        if (ui_ui_selftest_manual_txt_manual_action_hint != NULL)
        {
            lv_obj_add_flag(ui_ui_selftest_manual_txt_manual_action_hint,
                            LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_event_cb(ui_ui_selftest_manual_txt_manual_action_hint,
                                output_replay_event_cb,
                                LV_EVENT_CLICKED,
                                NULL);
        }
        break;
    case UI_RUNTIME_SCREEN_SELFTEST_RESULT:
        ui_ui_scr_selftest_result_screen_init();
        remove_redundant_temporary_clipping(screen);
        configure_selftest_result_scroll();
        configure_selftest_result_snapshot_cache();
        break;
    case UI_RUNTIME_SCREEN_GPS_CONTEXT:
        ui_ui_scr_selftest_gps_context_screen_init();
        remove_redundant_temporary_clipping(screen);
        break;
    case UI_RUNTIME_SCREEN_RETRY_DETAIL:
        ui_ui_scr_selftest_retry_detail_screen_init();
        remove_redundant_temporary_clipping(screen);
        break;
    case UI_RUNTIME_SCREEN_MAIN:
    default:
        break;
    }
}

static void screen_destroy(ui_runtime_screen_t screen)
{
    if (screen < UI_RUNTIME_SCREEN_COUNT)
    {
        s_screen_projected_states[screen] =
            UI_MANIFEST_STATE_COUNT;
    }
    switch (screen)
    {
    case UI_RUNTIME_SCREEN_SETTINGS:
        s_settings_motion_active = false;
        set_settings_snapshot_active(false);
        release_settings_snapshot_cache();
        ui_ui_scr_settings_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_DEVICE_INFO:
        ui_ui_scr_device_info_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_MAINTENANCE:
        ui_ui_scr_maintenance_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_BLE:
        ui_ui_scr_ble_candidates_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_SELFTEST:
        ui_ui_scr_selftest_progress_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_SELFTEST_RESULT:
        s_selftest_result_motion_active = false;
        set_selftest_result_snapshot_active(false);
        release_selftest_result_snapshot_cache();
        ui_ui_scr_selftest_result_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_GPS_CONTEXT:
        ui_ui_scr_selftest_gps_context_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_RETRY_DETAIL:
        ui_ui_scr_selftest_retry_detail_screen_destroy();
        break;
    case UI_RUNTIME_SCREEN_MAIN:
    default:
        break;
    }
}

static bool prepare_page_screen(ui_page_id_t page)
{
    const ui_runtime_screen_t desired = screen_for_page(page);
    if (desired == s_loaded_screen)
    {
        return true;
    }

    lv_obj_t **target = screen_object(desired);
    if (target == NULL)
    {
        return false;
    }
    if (*target == NULL)
    {
        /*
         * 设置和自检结果复用同一 transient PSRAM 预算。目标 Screen
         * 构建前先释放上一 owner，避免页面切换瞬间同时持有两组缓存。
         */
        if (desired == UI_RUNTIME_SCREEN_SETTINGS)
        {
            set_selftest_result_snapshot_active(false);
            release_selftest_result_snapshot_cache();
        }
        else if (desired == UI_RUNTIME_SCREEN_SELFTEST_RESULT)
        {
            set_settings_snapshot_active(false);
            release_settings_snapshot_cache();
        }
        screen_init(desired);
    }
    if (*target == NULL)
    {
        return false;
    }

    return true;
}

static void commit_page_screen(ui_page_id_t page)
{
    const ui_runtime_screen_t desired = screen_for_page(page);
    if (desired == s_loaded_screen)
    {
        if (desired == UI_RUNTIME_SCREEN_MAIN &&
            ui_ui_main_shell_pager != NULL &&
            s_committed_shell_slot != s_shell_slot)
        {
            commit_shell_pager_slot();
        }
        return;
    }

    lv_obj_t **target = screen_object(desired);
    if (target == NULL || *target == NULL)
    {
        return;
    }

    const ui_runtime_screen_t previous = s_loaded_screen;
    if (desired != UI_RUNTIME_SCREEN_MAINTENANCE)
    {
        hide_clear_binding_modal();
    }
    lv_scr_load(*target);
    s_loaded_screen = desired;
    if (screen_needs_full_redraw(previous) ||
        screen_needs_full_redraw(desired))
    {
        request_full_redraw();
    }
    if (previous != UI_RUNTIME_SCREEN_MAIN)
    {
        screen_destroy(previous);
    }
    if (desired == UI_RUNTIME_SCREEN_MAIN && ui_ui_main_shell_pager != NULL)
    {
        commit_shell_pager_slot();
    }
}

static bool begin_ble_atomic_redraw(ui_runtime_screen_t resolved_screen,
                                    ui_manifest_state_t state,
                                    lv_disp_t **display_out)
{
    if (display_out == NULL)
    {
        return false;
    }
    *display_out = NULL;
    const bool ble_visual_transition =
        resolved_screen == UI_RUNTIME_SCREEN_BLE &&
        (s_loaded_screen != UI_RUNTIME_SCREEN_BLE ||
         s_screen_projected_states[UI_RUNTIME_SCREEN_BLE] != state);
    if (!ble_visual_transition)
    {
        return false;
    }

    lv_disp_t *display = lv_disp_get_default();
    if (display == NULL || !lv_disp_is_invalidation_enabled(display))
    {
        return false;
    }
    /* BLE 状态投影会批量隐藏、显示并重设图标，先阻止局部中间帧提交。 */
    lv_disp_enable_invalidation(display, false);
    *display_out = display;
    return true;
}

static void finish_ble_atomic_redraw(bool active, lv_disp_t *display)
{
    if (!active || display == NULL)
    {
        return;
    }
    lv_disp_enable_invalidation(display, true);
    lv_obj_t *screen = lv_scr_act();
    if (screen != NULL)
    {
        /* 同一 LVGL 帧只提交 BLE 搜索或候选页面的最终整屏。 */
        lv_obj_invalidate(screen);
    }
}

static bool screen_needs_full_redraw(ui_runtime_screen_t screen)
{
    return screen == UI_RUNTIME_SCREEN_SELFTEST ||
           screen == UI_RUNTIME_SCREEN_SELFTEST_RESULT ||
           screen == UI_RUNTIME_SCREEN_GPS_CONTEXT ||
           screen == UI_RUNTIME_SCREEN_RETRY_DETAIL;
}

static void request_full_redraw(void)
{
    s_full_redraw_requested = true;
}

static ui_page_id_t shell_page_for_model(ui_shell_slot_t slot,
                                         const ui_runtime_model_t *model)
{
    const bool verify_failed_layout =
        model->authorization_display == UI_AUTHORIZATION_DISPLAY_FAILED ||
        (model->payment_attempted && model->payment_failed);
    const ui_navigation_facts_t facts = {
        .has_binding = model->binding_present,
        .ble_connected = model->ble_connected,
        .link_control_ready = model->controls_enabled,
        .unlock_session_valid = model->unlock_session_valid,
        .payment_required = model->payment_required,
        .payment_verification_attempted =
            model->payment_attempted || verify_failed_layout,
        .payment_verification_failed =
            model->payment_failed || verify_failed_layout,
        .reminder_count = model->reminder_count,
    };
    return ui_navigation_resolve_shell(slot, &facts);
}

static void consume_pending_control_route(
    const ui_runtime_model_t *model)
{
    if (!s_pending_control_route || !s_display_on || model == NULL ||
        !ui_navigation_is_shell_page(s_current_page))
    {
        return;
    }

    if (model->binding_present)
    {
        s_shell_slot = UI_SHELL_SLOT_GEAR;
        route_to(UI_PAGE_GEAR_READY);
    }
    else
    {
        s_shell_slot = UI_SHELL_SLOT_HOME;
        route_to(UI_PAGE_HOME_READY);
    }
    s_pending_control_route = false;
}

static ui_manifest_state_t shell_state_for_page(ui_page_id_t page)
{
    switch (page)
    {
    case UI_PAGE_HOME_READY:
        return UI_MANIFEST_STATE_HOME_NORMAL;
    case UI_PAGE_HOME_UNBOUND:
        return UI_MANIFEST_STATE_HOME_UNBOUND;
    case UI_PAGE_HOME_DISCONNECTED:
        return UI_MANIFEST_STATE_HOME_DISCONNECTED;
    case UI_PAGE_GEAR_READY:
        return UI_MANIFEST_STATE_CONTROL_CONNECTED;
    case UI_PAGE_GEAR_UNAVAILABLE:
        return UI_MANIFEST_STATE_CONTROL_DISCONNECTED;
    case UI_PAGE_MODE_POWER_READY:
        return UI_MANIFEST_STATE_MODE_DEFAULT;
    case UI_PAGE_MODE_POWER_UNAVAILABLE:
        return UI_MANIFEST_STATE_MODE_DISCONNECTED;
    case UI_PAGE_ALERTS_EMPTY:
        return UI_MANIFEST_STATE_ALERTS_EMPTY;
    case UI_PAGE_ALERTS_READ_ONLY:
        return UI_MANIFEST_STATE_ALERTS_READONLY;
    case UI_PAGE_ALERTS_PAYMENT_VERIFY:
        return UI_MANIFEST_STATE_PAYMENT_REQUIRED;
    case UI_PAGE_ALERTS_VERIFY_FAILED:
        return UI_MANIFEST_STATE_PAYMENT_VERIFY_FAILED;
    default:
        return UI_MANIFEST_STATE_HOME_DISCONNECTED;
    }
}

static bool apply_shell_static_patches(const ui_runtime_model_t *model)
{
    for (ui_shell_slot_t slot = UI_SHELL_SLOT_HOME;
         slot < UI_SHELL_SLOT_COUNT;
         slot = (ui_shell_slot_t)(slot + 1))
    {
        const ui_page_id_t page = shell_page_for_model(slot, model);
        const ui_manifest_state_t state = shell_state_for_page(page);
        if (s_shell_projected_states[slot] == state)
        {
            continue;
        }
        if (!apply_static_patch(state))
        {
            return false;
        }
        s_shell_projected_states[slot] = state;
    }
    return true;
}

static void invalidate_shell_projection_cache(void)
{
    for (ui_shell_slot_t slot = UI_SHELL_SLOT_HOME;
         slot < UI_SHELL_SLOT_COUNT;
         slot = (ui_shell_slot_t)(slot + 1))
    {
        s_shell_projected_states[slot] = UI_MANIFEST_STATE_COUNT;
    }
}

static ui_manifest_state_t resolve_static_state(const ui_runtime_model_t *model,
                                                ui_page_id_t *resolved_page)
{
    if (ui_navigation_is_shell_page(s_current_page))
    {
        *resolved_page = shell_page_for_model(s_shell_slot, model);
        return shell_state_for_page(*resolved_page);
    }
    *resolved_page = s_current_page;

    switch (*resolved_page)
    {
    case UI_PAGE_SETTINGS:
        return UI_MANIFEST_STATE_SETTINGS;
    case UI_PAGE_DEVICE_INFO:
        return UI_MANIFEST_STATE_DEVICE_INFO;
    case UI_PAGE_MAINTENANCE:
        return UI_MANIFEST_STATE_MAINTENANCE;
    case UI_PAGE_BLE_EMPTY:
    case UI_PAGE_BLE_CANDIDATES:
        if (model->binding_present)
        {
            s_shell_slot = UI_SHELL_SLOT_HOME;
            *resolved_page = UI_PAGE_HOME_READY;
            return model->ble_connected ? UI_MANIFEST_STATE_HOME_NORMAL
                                        : UI_MANIFEST_STATE_HOME_DISCONNECTED;
        }
        if (model->ble_scan_state == UI_RUNTIME_BLE_SCAN_ACTIVE)
        {
            *resolved_page = UI_PAGE_BLE_EMPTY;
            return UI_MANIFEST_STATE_BLE_SCANNING;
        }
        *resolved_page = model->ble_candidate_count > 0U
                             ? UI_PAGE_BLE_CANDIDATES
                             : UI_PAGE_BLE_EMPTY;
        return model->ble_candidate_count > 0U
                   ? UI_MANIFEST_STATE_BLE_CANDIDATES
                   : UI_MANIFEST_STATE_BLE_EMPTY;
    case UI_PAGE_SELFTEST_IDLE:
    case UI_PAGE_SELFTEST_RUNNING:
    case UI_PAGE_SELFTEST_MANUAL:
        if (!model->selftest_running && !model->retry_running)
        {
            if (model->selftest_results.state == SELFTEST_RUN_FINISHED)
            {
                *resolved_page = UI_PAGE_SELFTEST_RESULT;
                return model->selftest_results.fail_count == 0U
                           ? UI_MANIFEST_STATE_SELFTEST_RESULT_ALL_PASSED
                           : UI_MANIFEST_STATE_SELFTEST_RESULT;
            }
            *resolved_page = UI_PAGE_SELFTEST_IDLE;
            return UI_MANIFEST_STATE_SELFTEST_IDLE;
        }
        if (model->selftest_item == SELFTEST_ITEM_TOUCH)
        {
            *resolved_page = UI_PAGE_SELFTEST_MANUAL;
            return UI_MANIFEST_STATE_SELFTEST_MANUAL_TOUCH;
        }
        if (model->selftest_item == SELFTEST_ITEM_VIBRATION)
        {
            *resolved_page = UI_PAGE_SELFTEST_MANUAL;
            return UI_MANIFEST_STATE_SELFTEST_MANUAL_VIBRATION;
        }
        if (model->selftest_item == SELFTEST_ITEM_AUDIO)
        {
            *resolved_page = UI_PAGE_SELFTEST_MANUAL;
            return UI_MANIFEST_STATE_SELFTEST_MANUAL;
        }
        if (model->selftest_item == SELFTEST_ITEM_AMOLED)
        {
            *resolved_page = UI_PAGE_SELFTEST_MANUAL;
            return UI_MANIFEST_STATE_SELFTEST_MANUAL;
        }
        *resolved_page = UI_PAGE_SELFTEST_RUNNING;
        return model->retry_running ? UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO
                                    : UI_MANIFEST_STATE_SELFTEST_PROGRESS;
    case UI_PAGE_SELFTEST_RESULT:
        return model->selftest_results.fail_count == 0U
                   ? UI_MANIFEST_STATE_SELFTEST_RESULT_ALL_PASSED
                   : UI_MANIFEST_STATE_SELFTEST_RESULT;
    case UI_PAGE_SELFTEST_GPS_CONTEXT:
        if (model->selftest_running || model->retry_running)
        {
            *resolved_page = UI_PAGE_SELFTEST_RUNNING;
            return model->retry_running ? UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO
                                        : UI_MANIFEST_STATE_SELFTEST_PROGRESS;
        }
        if (model->gps_submit_state == UI_RUNTIME_GPS_SUBMIT_LOADING)
        {
            return UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_LOADING;
        }
        if (model->gps_submit_state == UI_RUNTIME_GPS_SUBMIT_FAILED)
        {
            return UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_FAILED;
        }
        return UI_MANIFEST_STATE_SELFTEST_GPS_CONTEXT;
    case UI_PAGE_SELFTEST_RETRY_DETAIL:
        if (model->retry_running)
        {
            if (model->selftest_item == SELFTEST_ITEM_AMOLED ||
                model->selftest_item == SELFTEST_ITEM_TOUCH ||
                model->selftest_item == SELFTEST_ITEM_AUDIO ||
                model->selftest_item == SELFTEST_ITEM_VIBRATION)
            {
                *resolved_page = UI_PAGE_SELFTEST_MANUAL;
                return model->selftest_item == SELFTEST_ITEM_TOUCH
                           ? UI_MANIFEST_STATE_SELFTEST_MANUAL_TOUCH
                           : model->selftest_item == SELFTEST_ITEM_VIBRATION
                                 ? UI_MANIFEST_STATE_SELFTEST_MANUAL_VIBRATION
                                 : UI_MANIFEST_STATE_SELFTEST_MANUAL;
            }
            *resolved_page = UI_PAGE_SELFTEST_RUNNING;
            return UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO;
        }
        return UI_MANIFEST_STATE_SELFTEST_RETRY_DETAIL;
    case UI_PAGE_CLEAR_BINDING_CONFIRM:
        return UI_MANIFEST_STATE_CLEAR_BINDING_CONFIRM;
    default:
        *resolved_page = UI_PAGE_HOME_DISCONNECTED;
        return UI_MANIFEST_STATE_HOME_DISCONNECTED;
    }
}

static ui_manifest_state_t canonical_base_state(ui_manifest_state_t state)
{
    switch (state)
    {
    case UI_MANIFEST_STATE_SELFTEST_MANUAL_TOUCH:
    case UI_MANIFEST_STATE_SELFTEST_MANUAL_VIBRATION:
        return UI_MANIFEST_STATE_SELFTEST_MANUAL;
    case UI_MANIFEST_STATE_BLE_SCANNING:
        return UI_MANIFEST_STATE_BLE_EMPTY;
    case UI_MANIFEST_STATE_SELFTEST_RETRY_AUTO:
        return UI_MANIFEST_STATE_SELFTEST_PROGRESS;
    case UI_MANIFEST_STATE_SELFTEST_RESULT_ALL_PASSED:
        return UI_MANIFEST_STATE_SELFTEST_RESULT;
    case UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_LOADING:
    case UI_MANIFEST_STATE_SELFTEST_GPS_SUBMIT_FAILED:
        return UI_MANIFEST_STATE_SELFTEST_GPS_CONTEXT;
    default:
        return state;
    }
}

static bool apply_static_patch(ui_manifest_state_t state)
{
    return ui_manifest_projection_apply(state);
}

static void apply_selftest_lock(const ui_runtime_model_t *model)
{
    if (!model->selftest_running)
    {
        return;
    }
    lv_obj_t *controls[] = {
        ui_ui_control_connected_btn_gear_minus,
        ui_ui_control_connected_btn_gear_plus,
        ui_ui_control_connected_toggle_assist,
        ui_ui_mode_default_btn_mode_standard,
        ui_ui_mode_default_btn_mode_extreme,
        ui_ui_mode_default_btn_mode_sport,
        ui_ui_mode_default_btn_mode_eco,
        ui_ui_mode_default_btn_exo_poweroff,
    };
    for (size_t index = 0; index < sizeof(controls) / sizeof(controls[0]); ++index)
    {
        if (controls[index] != NULL)
        {
            lv_obj_clear_state(controls[index], LV_STATE_DISABLED);
        }
    }
}

static void apply_model_projection(const ui_runtime_model_t *model)
{
    const bool atomic_step_redraw =
        s_shell_slot == UI_SHELL_SLOT_HOME &&
        s_loaded_screen == UI_RUNTIME_SCREEN_MAIN &&
        !s_shell_snapshot_active &&
        s_shell_projected_states[UI_SHELL_SLOT_HOME] ==
            UI_MANIFEST_STATE_HOME_NORMAL &&
        s_shell_visual_signature_valid[UI_SHELL_SLOT_HOME] &&
        s_shell_target_signatures[UI_SHELL_SLOT_HOME].steps !=
            model->steps;
    const watch_exoskeleton_scene_config_t current_scene_config = model->scene_config;
    const uint8_t current_drive_mode =
        model->drive_mode >= WATCH_EXOSKELETON_SCENE_MODE_MIN &&
                model->drive_mode <= WATCH_EXOSKELETON_SCENE_MODE_MAX &&
                watch_exoskeleton_scene_config_supports_mode(
                    current_scene_config, model->drive_mode)
            ? model->drive_mode
            : 1U;
    const bool atomic_mode_power_redraw =
        s_shell_slot == UI_SHELL_SLOT_MODE_POWER &&
        s_loaded_screen == UI_RUNTIME_SCREEN_MAIN &&
        !s_shell_snapshot_active &&
        s_shell_visual_signature_valid[UI_SHELL_SLOT_MODE_POWER] &&
        (s_shell_target_signatures[UI_SHELL_SLOT_MODE_POWER]
                 .poweroff_committed != model->poweroff_committed ||
         s_shell_target_signatures[UI_SHELL_SLOT_MODE_POWER].scene_config !=
             current_scene_config ||
         s_shell_target_signatures[UI_SHELL_SLOT_MODE_POWER].drive_mode !=
             current_drive_mode);
    const bool atomic_shell_redraw =
        atomic_step_redraw || atomic_mode_power_redraw;
    lv_disp_t *display = atomic_shell_redraw ? lv_disp_get_default() : NULL;
    lv_obj_t *screen = atomic_shell_redraw ? lv_scr_act() : NULL;
    const bool invalidation_suspended =
        display != NULL && screen != NULL &&
        lv_disp_is_invalidation_enabled(display);
    if (invalidation_suspended)
    {
        /* 屏蔽步数或模式页最终态切换的局部脏区，禁止面板看到中间帧。 */
        lv_disp_enable_invalidation(display, false);
    }

    char value[UI_RUNTIME_TEXT_CAPACITY] = {0};
    switch (screen_for_page(s_current_page))
    {
    case UI_RUNTIME_SCREEN_MAIN:
    {
        uint8_t visible_drive_mode =
            model->drive_mode >= WATCH_EXOSKELETON_SCENE_MODE_MIN &&
                    model->drive_mode <= WATCH_EXOSKELETON_SCENE_MODE_MAX
                ? model->drive_mode
                : 1U;
        if (!watch_exoskeleton_scene_config_supports_mode(
                model->scene_config,
                visible_drive_mode))
        {
            visible_drive_mode = 1U;
        }
        apply_connectivity_icons(model);
        const bool home_assist_visible =
            s_shell_projected_states[UI_SHELL_SLOT_HOME] ==
                UI_MANIFEST_STATE_HOME_NORMAL &&
            !model->authorization_pending;
        apply_home_assist_status(model->assist_enabled,
                                 home_assist_visible);
        set_label(ui_ui_home_normal_txt_time, model->time);
        apply_watch_battery_projection(model);
        (void)snprintf(value, sizeof(value), "%u%%", model->exo_battery);
        set_label(ui_ui_home_normal_val_exo_battery, value);
        (void)snprintf(value, sizeof(value), "%u", model->gear);
        set_label(ui_ui_home_normal_val_gear, value);
        set_label(ui_ui_control_connected_val_actual_gear, value);
        (void)snprintf(value,
                       sizeof(value),
                       "%lu",
                       (unsigned long)model->steps);
        set_label(ui_ui_home_normal_val_steps, value);
        set_label(ui_ui_home_disconnected_txt_last_snapshot,
                  model->last_snapshot);
        set_label(ui_ui_mode_default_lbl_current_mode,
                  visible_drive_mode == 2U ? "健身"
                  : visible_drive_mode == 3U ? "极限"
                  : visible_drive_mode == 4U ? "下山"
                  : visible_drive_mode ==
                            WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP
                      ? "小碎步"
                                             : "标准");
        const bool small_step_profile =
            current_scene_config ==
            WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP;
        set_label(ui_ui_mode_default_btn_mode_sport_label,
                  small_step_profile ? "小碎步" : "健身");
        set_label(ui_ui_mode_default_icon_mode_sport,
                  small_step_profile ? UI_MODE_SMALL_STEP_GLYPH
                                     : UI_MODE_FITNESS_GLYPH);
        set_label(ui_ui_mode_default_btn_mode_extreme_label, "极限");
        set_label(ui_ui_mode_default_btn_mode_eco_label, "下山");
        set_label(ui_ui_mode_disconnected_btn_mode_sport_label_disabled,
                  small_step_profile ? "小碎步" : "健身");
        set_label(ui_ui_mode_disconnected_icon_mode_sport_disabled,
                  small_step_profile ? UI_MODE_SMALL_STEP_GLYPH
                                     : UI_MODE_FITNESS_GLYPH);
        set_label(ui_ui_mode_disconnected_btn_mode_extreme_label_disabled,
                  "极限");
        set_label(ui_ui_mode_disconnected_btn_mode_eco_label_disabled,
                  "下山");
        apply_mode_layout(model);
        apply_mode_selection(visible_drive_mode);
        apply_assist_visual(model->assist_enabled);
        set_alerts(model);
        set_payment_pending(model);
        set_control_projection_locks(model);
        break;
    }
    case UI_RUNTIME_SCREEN_SETTINGS:
    {
        set_setting_toggle(ui_ui_settings_row_haptics_instance,
                           model->haptics_enabled);
        set_setting_toggle(ui_ui_settings_row_click_audio_instance,
                           model->click_audio_enabled);
        set_setting_toggle(ui_ui_settings_row_raise_wake_instance,
                           model->raise_to_wake_enabled);
        lv_obj_t *brightness_buttons[3] = {
            ui_ui_settings_btn_brightness_low,
            ui_ui_settings_btn_brightness_medium,
            ui_ui_settings_btn_brightness_high,
        };
        lv_obj_t *brightness_labels[3] = {
            ui_ui_settings_txt_brightness_low,
            ui_ui_settings_txt_brightness_medium,
            ui_ui_settings_txt_brightness_high,
        };
        set_segmented_setting(
            brightness_buttons,
            brightness_labels,
            model->brightness_level < UI_RUNTIME_BRIGHTNESS_COUNT
                ? (uint8_t)model->brightness_level
                : (uint8_t)UI_RUNTIME_BRIGHTNESS_HIGH);
        lv_obj_t *timeout_buttons[3] = {
            ui_ui_settings_btn_screen_timeout_5s,
            ui_ui_settings_btn_screen_timeout_15s,
            ui_ui_settings_btn_screen_timeout_30s,
        };
        lv_obj_t *timeout_labels[3] = {
            ui_ui_settings_txt_screen_timeout_5s,
            ui_ui_settings_txt_screen_timeout_15s,
            ui_ui_settings_txt_screen_timeout_30s,
        };
        const uint8_t timeout_index =
            model->screen_timeout_seconds ==
                    UI_RUNTIME_SCREEN_TIMEOUT_15S
                ? 1U
                : model->screen_timeout_seconds ==
                          UI_RUNTIME_SCREEN_TIMEOUT_30S
                      ? 2U
                      : 0U;
        set_segmented_setting(timeout_buttons,
                              timeout_labels,
                              timeout_index);
        update_settings_scroll_thumb();
        break;
    }
    case UI_RUNTIME_SCREEN_DEVICE_INFO:
        set_label(ui_ui_device_info_row_firmware_label,
                  strcmp(model->device_model, "Legbot Watch") == 0
                      ? "手环版本"
                      : "设备版本");
        set_label(ui_ui_device_info_row_firmware_value,
                  model->firmware_version);
        set_label(ui_ui_device_info_row_watch_id_value,
                  model->serial_number);
        set_label(ui_ui_device_info_row_bound_mac_value,
                  model->binding_id);
        apply_leg_positions(model);
        break;
    case UI_RUNTIME_SCREEN_MAINTENANCE:
    {
#if LEGBOT_CAP_MODEM
        set_label(ui_ui_maintenance_row_cellular_value,
                  model->cellular_status);
#else
        set_label(ui_ui_maintenance_row_cellular_value, "不可用");
#endif
        const bool cellular_action_enabled =
            LEGBOT_CAP_MODEM && model->cellular_action_enabled;
        if (ui_ui_maintenance_row_cellular_value != NULL &&
            ((cellular_action_enabled &&
              ui_ui_maintenance_row_cellular_arrow != NULL) ||
             (!cellular_action_enabled &&
              ui_ui_maintenance_row_gps_value != NULL)))
        {
            const lv_coord_t value_x =
                lv_obj_get_x(ui_ui_maintenance_row_cellular_value);
            const lv_coord_t value_right =
                cellular_action_enabled
                    ? lv_obj_get_x(
                          ui_ui_maintenance_row_cellular_arrow) -
                          UI_MAINTENANCE_CELLULAR_VALUE_ARROW_GAP
                    : lv_obj_get_x(
                          ui_ui_maintenance_row_gps_value) +
                          lv_obj_get_width(
                              ui_ui_maintenance_row_gps_value);
            lv_obj_set_width(ui_ui_maintenance_row_cellular_value,
                             value_right - value_x);
        }
#if !LEGBOT_CAP_GPS_TIME
        set_label(ui_ui_maintenance_row_gps_label, "GPS");
        set_label(ui_ui_maintenance_row_gps_value, "不可用");
#elif LEGBOT_CAP_GPS_CONTINUOUS_LOCATION
        set_label(ui_ui_maintenance_row_gps_label, "GPS 定位");
        set_label(ui_ui_maintenance_row_gps_value,
                  model->gps_status);
#else
        set_label(ui_ui_maintenance_row_gps_label, "GPS 授时");
        set_label(ui_ui_maintenance_row_gps_value,
                  model->gps_status);
#endif
        set_state(ui_ui_maintenance_row_gps,
                  LV_STATE_DISABLED,
                  !model->gps_action_enabled);
        set_flag(ui_ui_maintenance_row_gps,
                 LV_OBJ_FLAG_CLICKABLE,
                 LEGBOT_CAP_GPS_TIME != 0);
        set_state(ui_ui_maintenance_row_cellular,
                  LV_STATE_DISABLED,
                  !cellular_action_enabled);
        set_flag(ui_ui_maintenance_row_cellular,
                 LV_OBJ_FLAG_CLICKABLE,
                 LEGBOT_CAP_MODEM != 0);
        set_flag(ui_ui_maintenance_row_cellular_arrow,
                 LV_OBJ_FLAG_HIDDEN,
                 !cellular_action_enabled);
#if !LEGBOT_CAP_MODEM
        set_text_color(ui_ui_maintenance_row_cellular_icon,
                       lv_color_hex(UI_STATUS_ICON_DISCONNECTED_COLOR),
                       0);
        set_text_color(ui_ui_maintenance_row_cellular_label,
                       lv_color_hex(UI_STATUS_ICON_DISCONNECTED_COLOR),
                       0);
        set_text_color(ui_ui_maintenance_row_cellular_value,
                       lv_color_hex(UI_STATUS_ICON_DISCONNECTED_COLOR),
                       0);
#endif
#if !LEGBOT_CAP_GPS_TIME
        set_text_color(ui_ui_maintenance_row_gps_icon,
                       lv_color_hex(UI_STATUS_ICON_DISCONNECTED_COLOR),
                       0);
        set_text_color(ui_ui_maintenance_row_gps_label,
                       lv_color_hex(UI_STATUS_ICON_DISCONNECTED_COLOR),
                       0);
        set_text_color(ui_ui_maintenance_row_gps_value,
                       lv_color_hex(UI_STATUS_ICON_DISCONNECTED_COLOR),
                       0);
#else
        set_text_color(ui_ui_maintenance_row_gps_icon,
                       lv_color_hex(UI_STATUS_ICON_GPS_FIXED_COLOR),
                       0);
        set_text_color(ui_ui_maintenance_row_gps_label,
                       lv_color_hex(UI_CONTENT_WHITE_COLOR),
                       0);
        set_text_color(ui_ui_maintenance_row_gps_value,
                       lv_color_hex(UI_STATUS_ICON_GPS_FIXED_COLOR),
                       0);
#endif
        set_label(ui_ui_maintenance_row_selftest_value,
                  model->selftest_status);
        set_state(ui_ui_maintenance_row_clear_binding,
                  LV_STATE_DISABLED,
                  !model->binding_present ||
                      model->clear_binding_pending);
        if (s_current_page == UI_PAGE_CLEAR_BINDING_CONFIRM)
        {
            show_clear_binding_modal(model);
        }
        break;
    }
    case UI_RUNTIME_SCREEN_BLE:
        set_ble_candidates(model);
        break;
    case UI_RUNTIME_SCREEN_SELFTEST:
    {
        set_label(ui_ui_selftest_progress_val_current_item,
                  model->selftest_item_text);
        set_label(ui_ui_selftest_idle_txt_selftest_count, "07");
        set_label(ui_ui_selftest_idle_txt_selftest_scope,
                  "7 类 · 11 项内部检查");
        set_label(ui_ui_selftest_progress_txt_progress_total, "/ 07");
        (void)snprintf(value,
                       sizeof(value),
                       "%02u",
                       model->selftest_completed_categories);
        set_label(ui_ui_selftest_progress_val_progress, value);
        (void)snprintf(value,
                       sizeof(value),
                       "%u%%",
                       model->selftest_progress);
        set_label(ui_ui_selftest_progress_txt_progress_percent, value);
        const lv_coord_t progress_height =
            (lv_coord_t)((91U * model->selftest_progress) / 100U);
        set_height(ui_ui_selftest_progress_progress_value,
                   progress_height);
        set_y(ui_ui_selftest_progress_progress_value,
              183 - progress_height);

        const bool vibration_static_layout =
            model->selftest_item == SELFTEST_ITEM_VIBRATION;
        if (!vibration_static_layout)
        {
            set_label(ui_ui_selftest_manual_val_manual_item,
                      selftest_item_name(model->selftest_item));
            const unsigned manual_index =
                model->selftest_item == SELFTEST_ITEM_TOUCH
                    ? 2U
                    : model->selftest_item == SELFTEST_ITEM_AUDIO
                          ? 3U
                          : 1U;
            (void)snprintf(value,
                           sizeof(value),
                           "人工确认 · %02u / 07",
                           manual_index);
            set_label(ui_ui_selftest_manual_lbl_manual_item, value);
            set_label(ui_ui_selftest_manual_txt_manual_question,
                      model->manual_prompt);
            set_label(ui_ui_selftest_manual_val_manual_countdown,
                      model->manual_countdown);
            set_label(
                ui_ui_selftest_manual_txt_manual_action_hint,
                model->selftest_item == SELFTEST_ITEM_AUDIO
                    ? "点击此处\n再次播放"
                    : "观察结果后\n选择结果");
        }
        const bool output_replay_enabled =
            model->selftest_running &&
            (model->selftest_item == SELFTEST_ITEM_AUDIO ||
             model->selftest_item == SELFTEST_ITEM_VIBRATION) &&
            !model->manual_reply_pending;
        lv_obj_t *output_replay_hotspots[] = {
            ui_ui_selftest_manual_val_manual_countdown,
            ui_ui_selftest_manual_lbl_manual_seconds,
            ui_ui_selftest_manual_txt_manual_action_hint,
        };
        for (size_t index = 0U;
             index < sizeof(output_replay_hotspots) /
                         sizeof(output_replay_hotspots[0]);
             ++index)
        {
            set_flag(output_replay_hotspots[index],
                     LV_OBJ_FLAG_CLICKABLE,
                     output_replay_enabled);
        }
        set_touch_selftest(model);
        set_state(ui_ui_selftest_manual_btn_manual_fail,
                  LV_STATE_DISABLED,
                  model->manual_reply_pending);
        const bool touch_incomplete =
            model->selftest_item == SELFTEST_ITEM_TOUCH &&
            (s_touch_run_id != model->selftest_run_id ||
             !s_touch_target_hit ||
             s_touch_progress <=
                 UI_SELFTEST_TOUCH_PASS_THRESHOLD_PERCENT);
        set_state(ui_ui_selftest_manual_btn_manual_pass,
                  LV_STATE_DISABLED,
                  model->manual_reply_pending || touch_incomplete);
        break;
    }
    case UI_RUNTIME_SCREEN_SELFTEST_RESULT:
        set_selftest_results(&model->selftest_results);
        break;
    case UI_RUNTIME_SCREEN_RETRY_DETAIL:
        set_retry_detail(model);
        break;
    case UI_RUNTIME_SCREEN_GPS_CONTEXT:
    case UI_RUNTIME_SCREEN_COUNT:
    default:
        break;
    }

    if (invalidation_suspended)
    {
        /* 同一 LVGL 帧只提交步数或关机态投影完成后的整屏最终态。 */
        lv_disp_enable_invalidation(display, true);
        lv_obj_invalidate(screen);
    }
}

static void apply_watch_battery_projection(
    const ui_runtime_model_t *model)
{
    const bool low =
        model->power_level == WATCH_POWER_LEVEL_LOW_BATTERY_WARN ||
        model->power_level == WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY;
    const uint32_t color =
        model->power_level == WATCH_POWER_LEVEL_CRITICAL_LOW_BATTERY
            ? UI_WATCH_BATTERY_DANGER_COLOR
            : model->power_level == WATCH_POWER_LEVEL_LOW_BATTERY_WARN
                  ? UI_WATCH_BATTERY_WARNING_COLOR
                  : UI_CONTENT_WHITE_COLOR;
    lv_obj_t *icons[] = {
        ui_ui_home_normal_rail_watch_battery_icon,
        ui_ui_control_connected_rail_watch_battery_icon,
        ui_ui_mode_default_rail_watch_battery_icon,
        ui_ui_alerts_empty_rail_watch_battery_icon,
    };
    lv_obj_t *values[] = {
        ui_ui_home_normal_rail_watch_battery_value,
        ui_ui_control_connected_rail_watch_battery_value,
        ui_ui_mode_default_rail_watch_battery_value,
        ui_ui_alerts_empty_rail_watch_battery_value,
    };
    char value[UI_RUNTIME_TEXT_CAPACITY] = {0};
    (void)snprintf(value, sizeof(value), "%u%%", model->watch_battery);
    for (size_t index = 0U;
         index < sizeof(icons) / sizeof(icons[0]);
         ++index)
    {
        set_label(icons[index],
                  low ? UI_WATCH_BATTERY_LOW_GLYPH
                      : UI_WATCH_BATTERY_NORMAL_GLYPH);
        set_label(values[index], value);
        if (icons[index] != NULL)
        {
            set_text_font(
                icons[index],
                low ? &ui_font_lucide_28
                    : &ui_font_lucide_22,
                0);
            set_text_color(
                icons[index],
                lv_color_hex(color),
                0);
        }
        if (values[index] != NULL)
        {
            set_text_color(
                values[index],
                lv_color_hex(color),
                0);
        }
    }
}

static void apply_connectivity_icons(const ui_runtime_model_t *model)
{
    lv_obj_t *cellular_icons[] = {
        ui_ui_home_normal_rail_4g_icon,
        ui_ui_control_connected_rail_4g_icon,
        ui_ui_mode_default_rail_4g_icon,
        ui_ui_alerts_empty_rail_4g_icon,
    };
    lv_obj_t *gps_icons[] = {
        ui_ui_home_normal_rail_gps_icon,
        ui_ui_control_connected_rail_gps_icon,
        ui_ui_mode_default_rail_gps_icon,
        ui_ui_alerts_empty_rail_gps_icon,
    };
    lv_obj_t *ble_icons[] = {
        ui_ui_home_normal_rail_ble_icon,
        ui_ui_control_connected_rail_ble_icon,
        ui_ui_mode_default_rail_ble_icon,
        ui_ui_alerts_empty_rail_ble_icon,
    };

    for (size_t index = 0U;
         index < sizeof(cellular_icons) / sizeof(cellular_icons[0]);
         ++index)
    {
#if SCENIC_AREA_MANAGEMENT_DEBUG
        set_flag(cellular_icons[index], LV_OBJ_FLAG_HIDDEN, false);
        set_flag(gps_icons[index], LV_OBJ_FLAG_HIDDEN, false);
        set_x(cellular_icons[index], 82);
        set_x(gps_icons[index], 116);
        set_x(ble_icons[index], 150);
        set_connectivity_icon(cellular_icons[index],
                              model->cellular_connected,
                              UI_STATUS_ICON_CELLULAR_CONNECTED,
                              UI_STATUS_ICON_CELLULAR_DISCONNECTED,
                              UI_STATUS_ICON_CELLULAR_CONNECTED_COLOR);
        set_connectivity_icon(gps_icons[index],
                              model->gps_fixed,
                              UI_STATUS_ICON_GPS_FIXED,
                              UI_STATUS_ICON_GPS_NOT_FIXED,
                              UI_STATUS_ICON_GPS_FIXED_COLOR);
#else
        set_flag(cellular_icons[index], LV_OBJ_FLAG_HIDDEN, true);
        set_flag(gps_icons[index], LV_OBJ_FLAG_HIDDEN, true);
        set_x(ble_icons[index], 82);
#endif
        set_flag(ble_icons[index], LV_OBJ_FLAG_HIDDEN, false);
        set_connectivity_icon(ble_icons[index],
                              model->ble_connected,
                              UI_STATUS_ICON_BLE_CONNECTED,
                              UI_STATUS_ICON_BLE_DISCONNECTED,
                              UI_STATUS_ICON_BLE_CONNECTED_COLOR);
    }
}

static void set_connectivity_icon(lv_obj_t *icon,
                                  bool connected,
                                  const char *connected_glyph,
                                  const char *disconnected_glyph,
                                  uint32_t connected_color)
{
    if (icon == NULL)
    {
        return;
    }
    set_label(icon,
              connected ? connected_glyph : disconnected_glyph);
    set_text_color(
        icon,
        lv_color_hex(connected ? connected_color
                               : UI_STATUS_ICON_DISCONNECTED_COLOR),
        LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void apply_assist_visual(bool enabled)
{
    const lv_color_t accent = lv_color_hex(
        enabled ? UI_ASSIST_ENABLED_COLOR : UI_ASSIST_DISABLED_COLOR);
    const lv_color_t content = lv_color_hex(
        enabled ? UI_CONTENT_WHITE_COLOR : UI_ASSIST_DISABLED_COLOR);

    if (ui_ui_control_connected_toggle_assist != NULL)
    {
        set_border_color(
            ui_ui_control_connected_toggle_assist,
            accent,
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_ui_control_connected_icon_assist != NULL)
    {
        set_text_color(
            ui_ui_control_connected_icon_assist,
            accent,
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_ui_control_connected_lbl_assist != NULL)
    {
        set_text_color(
            ui_ui_control_connected_lbl_assist,
            content,
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_ui_control_connected_switch_assist_track != NULL)
    {
        set_background_color(
            ui_ui_control_connected_switch_assist_track,
            lv_color_hex(enabled ? UI_ASSIST_ENABLED_COLOR
                                 : UI_ASSIST_DISABLED_TRACK_COLOR),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (ui_ui_control_connected_switch_assist_thumb != NULL)
    {
        set_x(ui_ui_control_connected_switch_assist_thumb,
              enabled ? 66 : 6);
        set_background_color(
            ui_ui_control_connected_switch_assist_thumb,
            content,
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void apply_home_assist_status(bool enabled, bool visible)
{
    const lv_color_t color = lv_color_hex(
        enabled ? UI_HOME_ASSIST_ENABLED_COLOR
                : UI_HOME_ASSIST_DISABLED_COLOR);

    set_label(ui_ui_home_normal_icon_assist_state,
              enabled ? UI_HOME_ASSIST_ENABLED_GLYPH
                      : UI_HOME_ASSIST_DISABLED_GLYPH);
    set_label(ui_ui_home_normal_txt_assist_state,
              enabled ? "助力开启" : "助力关闭");
    set_text_color(ui_ui_home_normal_icon_assist_state,
                   color,
                   LV_PART_MAIN | LV_STATE_DEFAULT);
    set_text_color(ui_ui_home_normal_txt_assist_state,
                   color,
                   LV_PART_MAIN | LV_STATE_DEFAULT);
    set_flag(ui_ui_home_normal_icon_assist_state,
             LV_OBJ_FLAG_HIDDEN,
             !visible);
    set_flag(ui_ui_home_normal_txt_assist_state,
             LV_OBJ_FLAG_HIDDEN,
             !visible);
}

static uint8_t secondary_scene_target(
    watch_exoskeleton_scene_config_t config)
{
    return config ==
                   WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP
               ? WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP
               : 2U;
}

static void apply_mode_layout(const ui_runtime_model_t *model)
{
    const watch_exoskeleton_scene_config_t config =
        model->scene_config;
    const bool power_only =
        config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_ONLY ||
        config == WATCH_EXOSKELETON_SCENE_CONFIG_INVALID;
    const bool four_modes =
        config == WATCH_EXOSKELETON_SCENE_CONFIG_FULL;
    const bool two_modes =
        config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_FITNESS ||
        config == WATCH_EXOSKELETON_SCENE_CONFIG_STANDARD_SMALL_STEP;
    const lv_coord_t high_card_width = two_modes ? 181 : 116;
    const bool controls_unavailable =
        !model->ble_connected || model->poweroff_committed ||
        config == WATCH_EXOSKELETON_SCENE_CONFIG_INVALID;

    set_object_geometry(ui_ui_mode_default_lbl_current_mode,
                        40,
                        51,
                        120,
                        30);
    set_flag(ui_ui_mode_default_lbl_current_mode,
             LV_OBJ_FLAG_HIDDEN,
             power_only || model->poweroff_committed ||
                 config == WATCH_EXOSKELETON_SCENE_CONFIG_INVALID);
    set_text_color(
        ui_ui_mode_default_lbl_current_mode,
        lv_color_hex(model->ble_connected
                         ? UI_CONTENT_WHITE_COLOR
                         : UI_MODE_DISABLED_CONTENT_COLOR),
        LV_PART_MAIN | LV_STATE_DEFAULT);

    set_mode_card_geometry(ui_ui_mode_default_btn_mode_standard,
                           ui_ui_mode_default_btn_mode_standard_label,
                           ui_ui_mode_default_icon_mode_standard,
                           16,
                           four_modes ? 92 : 96,
                           four_modes ? 181 : high_card_width,
                           four_modes);
    set_mode_card_geometry(ui_ui_mode_default_btn_mode_extreme,
                           ui_ui_mode_default_btn_mode_extreme_label,
                           ui_ui_mode_default_icon_mode_extreme,
                           213,
                           92,
                           181,
                           true);
    set_mode_card_geometry(ui_ui_mode_default_btn_mode_sport,
                           ui_ui_mode_default_btn_mode_sport_label,
                           ui_ui_mode_default_icon_mode_sport,
                           four_modes ? 16 : two_modes ? 213 : 147,
                           four_modes ? 196 : 96,
                           four_modes ? 181 : high_card_width,
                           four_modes);
    set_mode_card_geometry(ui_ui_mode_default_btn_mode_eco,
                           ui_ui_mode_default_btn_mode_eco_label,
                           ui_ui_mode_default_icon_mode_eco,
                           four_modes ? 213 : 278,
                           four_modes ? 196 : 96,
                           four_modes ? 181 : high_card_width,
                           four_modes);
    set_power_button_geometry(ui_ui_mode_default_btn_exo_poweroff,
                              ui_ui_mode_default_lbl_exo_poweroff,
                              ui_ui_mode_default_icon_exo_poweroff,
                              power_only,
                              four_modes,
                              false,
                              151);

    set_mode_card_geometry(
        ui_ui_mode_disconnected_btn_mode_standard_selected_disabled,
        ui_ui_mode_disconnected_btn_mode_standard_label_selected_disabled,
        ui_ui_mode_disconnected_icon_mode_standard_selected_disabled,
        16,
        four_modes ? 92 : 96,
        four_modes ? 181 : high_card_width,
        four_modes);
    set_mode_card_geometry(
        ui_ui_mode_disconnected_btn_mode_extreme_disabled,
        ui_ui_mode_disconnected_btn_mode_extreme_label_disabled,
        ui_ui_mode_disconnected_icon_mode_extreme_disabled,
        213,
        92,
        181,
        true);
    set_mode_card_geometry(
        ui_ui_mode_disconnected_btn_mode_sport_disabled,
        ui_ui_mode_disconnected_btn_mode_sport_label_disabled,
        ui_ui_mode_disconnected_icon_mode_sport_disabled,
        four_modes ? 16 : two_modes ? 213 : 147,
        four_modes ? 196 : 96,
        four_modes ? 181 : high_card_width,
        four_modes);
    set_mode_card_geometry(
        ui_ui_mode_disconnected_btn_mode_eco_disabled,
        ui_ui_mode_disconnected_btn_mode_eco_label_disabled,
        ui_ui_mode_disconnected_icon_mode_eco_disabled,
        four_modes ? 213 : 278,
        four_modes ? 196 : 96,
        four_modes ? 181 : high_card_width,
        four_modes);
    set_power_button_geometry(
        ui_ui_mode_disconnected_btn_exo_poweroff_disabled,
        ui_ui_mode_disconnected_lbl_exo_poweroff_disabled,
        ui_ui_mode_disconnected_icon_exo_poweroff_disabled,
        power_only,
        four_modes,
        true,
        152);
    set_flag(ui_ui_mode_default_btn_exo_poweroff,
             LV_OBJ_FLAG_HIDDEN,
             controls_unavailable);
    set_flag(ui_ui_mode_disconnected_btn_exo_poweroff_disabled,
             LV_OBJ_FLAG_HIDDEN,
             !controls_unavailable);

    lv_obj_t *normal_mode_buttons[] = {
        ui_ui_mode_default_btn_mode_standard,
        ui_ui_mode_default_btn_mode_extreme,
        ui_ui_mode_default_btn_mode_sport,
        ui_ui_mode_default_btn_mode_eco,
    };
    lv_obj_t *disconnected_mode_buttons[] = {
        ui_ui_mode_disconnected_btn_mode_standard_selected_disabled,
        ui_ui_mode_disconnected_btn_mode_extreme_disabled,
        ui_ui_mode_disconnected_btn_mode_sport_disabled,
        ui_ui_mode_disconnected_btn_mode_eco_disabled,
    };
    /** normal/disconnected 对象数组对应的归一化场景目标。 */
    const uint8_t mode_targets[] = {
        1U,
        3U,
        secondary_scene_target(model->scene_config),
        4U,
    };
    for (size_t index = 0U;
         index < sizeof(normal_mode_buttons) /
                     sizeof(normal_mode_buttons[0]);
         ++index)
    {
        const bool supported =
            watch_exoskeleton_scene_config_allows_control(
                model->scene_config,
                mode_targets[index]);
        set_flag(normal_mode_buttons[index],
                 LV_OBJ_FLAG_HIDDEN,
                 power_only || controls_unavailable ||
                     !supported);
        set_flag(disconnected_mode_buttons[index],
                 LV_OBJ_FLAG_HIDDEN,
                 power_only || !controls_unavailable ||
                     !supported);
        set_state(disconnected_mode_buttons[index],
                  LV_STATE_DISABLED,
                  true);
    }
}

static void apply_mode_selection(uint8_t drive_mode)
{
    const bool atomic_redraw =
        s_shell_slot == UI_SHELL_SLOT_MODE_POWER &&
        s_loaded_screen == UI_RUNTIME_SCREEN_MAIN &&
        !s_shell_snapshot_active &&
        s_shell_visual_signature_valid[UI_SHELL_SLOT_MODE_POWER] &&
        s_shell_target_signatures[UI_SHELL_SLOT_MODE_POWER].drive_mode !=
            drive_mode;
    lv_disp_t *display = atomic_redraw ? lv_disp_get_default() : NULL;
    lv_obj_t *screen = atomic_redraw ? lv_scr_act() : NULL;
    const bool invalidation_suspended =
        display != NULL && screen != NULL &&
        lv_disp_is_invalidation_enabled(display);
    if (invalidation_suspended)
    {
        /* 先屏蔽四张卡片各自的局部脏区，禁止面板看到换色中间帧。 */
        lv_disp_enable_invalidation(display, false);
    }
    set_mode_card_selected(ui_ui_mode_default_btn_mode_standard,
                           ui_ui_mode_default_btn_mode_standard_label,
                           ui_ui_mode_default_icon_mode_standard,
                           drive_mode == 1U);
    set_mode_card_selected(ui_ui_mode_default_btn_mode_sport,
                           ui_ui_mode_default_btn_mode_sport_label,
                           ui_ui_mode_default_icon_mode_sport,
                           drive_mode == 2U ||
                               drive_mode ==
                                   WATCH_EXOSKELETON_SCENE_MODE_SMALL_STEP);
    set_mode_card_selected(ui_ui_mode_default_btn_mode_extreme,
                           ui_ui_mode_default_btn_mode_extreme_label,
                           ui_ui_mode_default_icon_mode_extreme,
                           drive_mode == 3U);
    set_mode_card_selected(ui_ui_mode_default_btn_mode_eco,
                           ui_ui_mode_default_btn_mode_eco_label,
                           ui_ui_mode_default_icon_mode_eco,
                           drive_mode == 4U);
    if (invalidation_suspended)
    {
        /* 同一 LVGL 帧只提交换色完成后的整屏最终态。 */
        lv_disp_enable_invalidation(display, true);
        lv_obj_invalidate(screen);
    }
}

static void set_mode_card_selected(lv_obj_t *button,
                                   lv_obj_t *label,
                                   lv_obj_t *icon,
                                   bool selected)
{
    if (button != NULL)
    {
        set_background_color(
            button,
            lv_color_hex(selected ? UI_MODE_SELECTED_COLOR
                                  : UI_MODE_IDLE_BACKGROUND_COLOR),
            LV_PART_MAIN | LV_STATE_DEFAULT);
        set_border_color(
            button,
            lv_color_hex(selected ? UI_MODE_SELECTED_COLOR
                                  : UI_MODE_IDLE_BORDER_COLOR),
            LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    const lv_color_t content = lv_color_hex(
        selected ? UI_CONTENT_BLACK_COLOR : UI_CONTENT_WHITE_COLOR);
    if (label != NULL)
    {
        set_text_color(label,
                       content,
                       LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (icon != NULL)
    {
        set_text_color(icon,
                       content,
                       LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void apply_leg_positions(const ui_runtime_model_t *model)
{
    char left_value[12] = "--";
    char right_value[12] = "--";
    if (model->leg_positions_available)
    {
        (void)snprintf(left_value,
                       sizeof(left_value),
                       "%d°",
                       (int)model->left_leg_position);
        (void)snprintf(right_value,
                       sizeof(right_value),
                       "%d°",
                       (int)model->right_leg_position);
    }
    set_label(ui_ui_device_info_row_left_leg_position_value, left_value);
    set_label(ui_ui_device_info_row_right_leg_position_value, right_value);
}

static void set_object_geometry(lv_obj_t *object,
                                lv_coord_t x,
                                lv_coord_t y,
                                lv_coord_t width,
                                lv_coord_t height)
{
    if (object == NULL)
    {
        return;
    }
    if (lv_obj_get_x(object) != x || lv_obj_get_y(object) != y)
    {
        lv_obj_set_pos(object, x, y);
    }
    if (lv_obj_get_width(object) != width ||
        lv_obj_get_height(object) != height)
    {
        lv_obj_set_size(object, width, height);
    }
}

static void set_mode_card_geometry(lv_obj_t *button,
                                   lv_obj_t *label,
                                   lv_obj_t *icon,
                                   lv_coord_t x,
                                   lv_coord_t y,
                                   lv_coord_t width,
                                   bool compact)
{
    set_object_geometry(button,
                        x,
                        y,
                        width,
                        compact ? 96 : 128);
    set_object_geometry(label,
                        compact ? 78 : 8,
                        compact ? 33 : 85,
                        compact ? 85 : width - 16,
                        compact ? 30 : 27);
    set_object_geometry(icon,
                        compact ? 22 : (width - 44) / 2,
                        compact ? 29 : 23,
                        compact ? 40 : 44,
                        compact ? 38 : 42);
    if (label != NULL)
    {
        set_text_align(label,
                       compact ? LV_TEXT_ALIGN_LEFT
                               : LV_TEXT_ALIGN_CENTER,
                       LV_PART_MAIN | LV_STATE_DEFAULT);
        set_text_font(label,
                      compact ? &ui_font_ns700_24
                              : &ui_font_ns700_22,
                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (icon != NULL)
    {
        set_text_font(icon,
                      compact ? &ui_font_lucide_40
                              : &ui_font_lucide_44,
                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void set_power_button_geometry(lv_obj_t *button,
                                      lv_obj_t *label,
                                      lv_obj_t *icon,
                                      bool power_only,
                                      bool four_modes,
                                      bool disconnected,
                                      lv_coord_t three_mode_label_width)
{
    if (power_only)
    {
        const lv_coord_t border_width = 10;
        set_object_geometry(button, 89, 135, 232, 232);
        /* LVGL 子坐标从父内容区起算，需抵消边框才能对齐外圆中心。 */
        set_object_geometry(label, 16 - border_width, 126, 200, 36);
        set_object_geometry(icon, 88 - border_width, 42, 56, 52);
        if (button != NULL)
        {
            set_radius(button, 116, LV_PART_MAIN | LV_STATE_DEFAULT);
            set_border_width(button,
                             border_width,
                             LV_PART_MAIN | LV_STATE_DEFAULT);
            set_background_color(
                button,
                lv_color_hex(disconnected ? 0x111318U : 0xFF453AU),
                LV_PART_MAIN | LV_STATE_DEFAULT);
            set_border_color(
                button,
                lv_color_hex(disconnected ? 0x3C434DU : 0x6A1512U),
                LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (label != NULL)
        {
            set_text_align(label,
                           LV_TEXT_ALIGN_CENTER,
                           LV_PART_MAIN | LV_STATE_DEFAULT);
            set_text_font(label,
                          &ui_font_ns700_30,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
            set_text_color(
                label,
                lv_color_hex(disconnected ? 0x737B88U : 0x0A0A0AU),
                LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        if (icon != NULL)
        {
            set_text_font(icon,
                          &ui_font_lucide_56,
                          LV_PART_MAIN | LV_STATE_DEFAULT);
            set_text_color(
                icon,
                lv_color_hex(disconnected ? 0x626B78U : 0x0A0A0AU),
                LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        return;
    }

    set_object_geometry(button,
                        four_modes ? 55 : 16,
                        four_modes ? 316 : 282,
                        four_modes ? 300 : 378,
                        four_modes ? 72 : 112);
    set_object_geometry(label,
                        four_modes ? 92 : 145,
                        four_modes ? 21 : 38,
                        four_modes ? 162 : three_mode_label_width,
                        four_modes ? 30 : 36);
    set_object_geometry(icon,
                        four_modes ? 46 : 84,
                        four_modes ? 21 : 37,
                        four_modes ? 32 : 40,
                        four_modes ? 30 : 38);
    if (button != NULL)
    {
        set_radius(button,
                   four_modes ? 26 : 36,
                   LV_PART_MAIN | LV_STATE_DEFAULT);
        set_border_width(button,
                         disconnected ? 3 : 0,
                         LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (label != NULL)
    {
        set_text_align(label,
                       LV_TEXT_ALIGN_LEFT,
                       LV_PART_MAIN | LV_STATE_DEFAULT);
        set_text_font(label,
                      four_modes ? &ui_font_ns700_24
                                 : &ui_font_ns700_30,
                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    if (icon != NULL)
    {
        set_text_font(icon,
                      four_modes ? &ui_font_lucide_32
                                 : &ui_font_lucide_40,
                      LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void set_alerts(const ui_runtime_model_t *model)
{
    lv_obj_t *instances[] = {
        ui_ui_alerts_readonly_alert_temperature_danger_instance,
        ui_ui_alerts_readonly_alert_exo_battery_warning_instance,
        ui_ui_alerts_readonly_alert_watch_critical_battery_instance,
    };
    if (model->payment_required)
    {
        lv_obj_t *generic_content[] = {
            ui_ui_alerts_empty_icon_alerts_empty,
            ui_ui_alerts_empty_txt_alerts_empty,
            ui_ui_alerts_readonly_txt_alert_title,
            ui_ui_alerts_readonly_top_alert_rule,
        };
        for (size_t index = 0U;
             index < sizeof(generic_content) / sizeof(generic_content[0]);
             ++index)
        {
            set_flag(generic_content[index], LV_OBJ_FLAG_HIDDEN, true);
        }
        for (size_t index = 0U;
             index < sizeof(instances) / sizeof(instances[0]);
             ++index)
        {
            if (instances[index] != NULL)
            {
                set_flag(instances[index], LV_OBJ_FLAG_HIDDEN, true);
            }
        }
        return;
    }

    lv_coord_t y = 132;
    for (size_t index = 0U; index < sizeof(instances) / sizeof(instances[0]); ++index)
    {
        if (instances[index] == NULL)
        {
            continue;
        }
        if (index >= model->visible_alert_count)
        {
            set_flag(instances[index], LV_OBJ_FLAG_HIDDEN, true);
            continue;
        }
        const ui_runtime_alert_t *alert = &model->alerts[index];
        lv_obj_t *accent = ui_comp_get_child(
            instances[index],
            UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ACCENT);
        lv_obj_t *icon = ui_comp_get_child(
            instances[index],
            UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_ICON);
        lv_obj_t *label = ui_comp_get_child(
            instances[index],
            UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_LABEL);
        lv_obj_t *state = ui_comp_get_child(
            instances[index],
            UI_COMP_CMP_ALERT_ROW_UI_CMP_ALERT_ROW_ALERT_EXO_BATTERY_STATE);
        const lv_color_t color =
            lv_color_hex(alert->danger ? 0xFF453AU : 0xFFD60AU);
        static const char *const icons[UI_RUNTIME_ALERT_COUNT] = {
            [UI_RUNTIME_ALERT_TEMPERATURE] = "",
            [UI_RUNTIME_ALERT_EXO_BATTERY] = "",
            [UI_RUNTIME_ALERT_WATCH_SEVERE] = "",
            [UI_RUNTIME_ALERT_WATCH_BATTERY] = "",
            [UI_RUNTIME_ALERT_AUDIO] = "",
            [UI_RUNTIME_ALERT_CELLULAR] = "",
            [UI_RUNTIME_ALERT_GPS] = "",
        };
        set_label(icon,
                  alert->type < UI_RUNTIME_ALERT_COUNT
                      ? icons[alert->type]
                      : "");
        set_label(label, alert->label);
        set_label(state, alert->value);
        if (accent != NULL)
        {
            set_background_color(accent, color, 0);
        }
        if (icon != NULL)
        {
            set_text_color(icon, color, 0);
        }
        if (label != NULL)
        {
            set_text_color(label, color, 0);
        }
        if (state != NULL)
        {
            set_text_color(state, color, 0);
        }
        set_y(instances[index], y);
        set_flag(instances[index], LV_OBJ_FLAG_HIDDEN, false);
        y += 84;
    }
}

static void set_payment_pending(const ui_runtime_model_t *model)
{
    if (ui_ui_main_shell_pager != NULL)
    {
        if (model->payment_required)
        {
            set_flag(ui_ui_main_shell_pager,
                     LV_OBJ_FLAG_SCROLLABLE,
                     false);
        }
        else
        {
            set_flag(ui_ui_main_shell_pager,
                     LV_OBJ_FLAG_SCROLLABLE,
                     true);
        }
    }
    lv_obj_t *buttons[] = {
        ui_ui_payment_required_btn_payment_verify,
        ui_ui_payment_verify_failed_btn_payment_retry,
    };
    lv_obj_t *payment_required_content[] = {
        ui_ui_payment_required_icon_payment_phone,
        ui_ui_payment_required_txt_payment_required,
        ui_ui_payment_required_payment_focus_rule,
        ui_ui_payment_required_btn_payment_verify,
        ui_ui_payment_required_icon_payment_card,
    };
    lv_obj_t *verify_failed_content[] = {
        ui_ui_payment_verify_failed_icon_payment_verify_failed,
        ui_ui_payment_verify_failed_txt_payment_verify_failed,
        ui_ui_payment_verify_failed_verify_failed_rule,
        ui_ui_payment_verify_failed_btn_payment_retry,
        ui_ui_payment_verify_failed_txt_payment_failure_reason,
    };
    const bool payment_required_layout =
        model->payment_required &&
        model->authorization_display != UI_AUTHORIZATION_DISPLAY_FAILED &&
        !(model->payment_attempted && model->payment_failed);
    const bool verify_failed_layout =
        model->payment_required && !payment_required_layout;
    for (size_t index = 0U;
         index < sizeof(payment_required_content) /
                     sizeof(payment_required_content[0]);
         ++index)
    {
        set_flag(payment_required_content[index],
                 LV_OBJ_FLAG_HIDDEN,
                 !payment_required_layout);
    }
    for (size_t index = 0U;
         index < sizeof(verify_failed_content) /
                     sizeof(verify_failed_content[0]);
         ++index)
    {
        set_flag(verify_failed_content[index],
                 LV_OBJ_FLAG_HIDDEN,
                 !verify_failed_layout);
    }
    if (payment_required_layout)
    {
        set_label(ui_ui_payment_required_txt_payment_required, "请完成支付");
        set_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason,
                 LV_OBJ_FLAG_HIDDEN,
                 true);
        set_flag(ui_ui_payment_required_btn_payment_verify,
                 LV_OBJ_FLAG_HIDDEN,
                 false);
    }
    else if (verify_failed_layout)
    {
        set_label(ui_ui_payment_verify_failed_txt_payment_verify_failed,
                  "验证失败");
        set_label(ui_ui_payment_verify_failed_txt_payment_failure_reason,
                  "验证失败，请重新验证");
        set_flag(ui_ui_payment_verify_failed_txt_payment_failure_reason,
                 LV_OBJ_FLAG_HIDDEN,
                 false);
        set_flag(ui_ui_payment_verify_failed_btn_payment_retry,
                 LV_OBJ_FLAG_HIDDEN,
                 false);
    }
    for (size_t index = 0U; index < sizeof(buttons) / sizeof(buttons[0]); ++index)
    {
        if (buttons[index] == NULL)
        {
            continue;
        }
        if (model->payment_pending || !model->payment_action_enabled)
        {
            set_state(buttons[index], LV_STATE_DISABLED, true);
        }
        else
        {
            set_state(buttons[index], LV_STATE_DISABLED, false);
        }
    }
}

static void set_control_projection_locks(const ui_runtime_model_t *model)
{
    const bool poweroff_committed = model->poweroff_committed;
    if (ui_ui_control_connected_btn_gear_minus != NULL)
    {
        set_state(ui_ui_control_connected_btn_gear_minus,
                  LV_STATE_DISABLED,
                  poweroff_committed || model->gear <= 1U ||
                      model->gear_projection_locked);
    }
    if (ui_ui_control_connected_btn_gear_plus != NULL)
    {
        set_state(ui_ui_control_connected_btn_gear_plus,
                  LV_STATE_DISABLED,
                  poweroff_committed ||
                      model->gear >= watch_exoskeleton_scene_config_max_gear(
                                           model->scene_config) ||
                      model->gear_projection_locked);
    }
    if (ui_ui_control_connected_toggle_assist != NULL)
    {
        set_state(ui_ui_control_connected_toggle_assist,
                  LV_STATE_DISABLED,
                  poweroff_committed);
    }
    lv_obj_t *mode_controls[] = {
        ui_ui_mode_default_btn_mode_standard,
        ui_ui_mode_default_btn_mode_extreme,
        ui_ui_mode_default_btn_mode_sport,
        ui_ui_mode_default_btn_mode_eco,
    };
    /** 模式控件顺序对应的归一化场景目标。 */
    const uint8_t mode_targets[] = {
        1U,
        3U,
        secondary_scene_target(model->scene_config),
        4U,
    };
    for (size_t index = 0U;
         index < sizeof(mode_controls) / sizeof(mode_controls[0]);
         ++index)
    {
        if (mode_controls[index] != NULL)
        {
            set_state(mode_controls[index],
                      LV_STATE_DISABLED,
                      poweroff_committed ||
                          model->mode_projection_locked ||
                          !watch_exoskeleton_scene_config_allows_control(
                              model->scene_config,
                              mode_targets[index]));
        }
    }
    if (ui_ui_mode_default_btn_exo_poweroff != NULL)
    {
        set_state(ui_ui_mode_default_btn_exo_poweroff,
                  LV_STATE_DISABLED,
                  poweroff_committed);
    }
}

static void set_label(lv_obj_t *label, const char *text)
{
    if (label == NULL || text == NULL)
    {
        return;
    }
    const char *current = lv_label_get_text(label);
    if (current == NULL || strcmp(current, text) != 0)
    {
        lv_label_set_text(label, text);
    }
}

static void set_text_color(lv_obj_t *object,
                           lv_color_t color,
                           lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_color_to32(lv_obj_get_style_text_color(object, part)) !=
            lv_color_to32(color))
    {
        lv_obj_set_style_text_color(object, color, selector);
    }
}

static void set_background_color(lv_obj_t *object,
                                 lv_color_t color,
                                 lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_color_to32(lv_obj_get_style_bg_color(object, part)) !=
            lv_color_to32(color))
    {
        lv_obj_set_style_bg_color(object, color, selector);
    }
}

static void set_border_color(lv_obj_t *object,
                             lv_color_t color,
                             lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_color_to32(lv_obj_get_style_border_color(object, part)) !=
            lv_color_to32(color))
    {
        lv_obj_set_style_border_color(object, color, selector);
    }
}

static void set_text_font(lv_obj_t *object,
                          const lv_font_t *font,
                          lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL && font != NULL &&
        lv_obj_get_style_text_font(object, part) != font)
    {
        lv_obj_set_style_text_font(object, font, selector);
    }
}

static void set_x(lv_obj_t *object, lv_coord_t x)
{
    if (object != NULL && lv_obj_get_x(object) != x)
    {
        lv_obj_set_x(object, x);
    }
}

static void set_y(lv_obj_t *object, lv_coord_t y)
{
    if (object != NULL && lv_obj_get_y(object) != y)
    {
        lv_obj_set_y(object, y);
    }
}

static void set_width(lv_obj_t *object, lv_coord_t width)
{
    if (object != NULL && lv_obj_get_width(object) != width)
    {
        lv_obj_set_width(object, width);
    }
}

static void set_height(lv_obj_t *object, lv_coord_t height)
{
    if (object != NULL && lv_obj_get_height(object) != height)
    {
        lv_obj_set_height(object, height);
    }
}

static void set_background_opa(lv_obj_t *object,
                               lv_opa_t opacity,
                               lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_obj_get_style_bg_opa(object, part) != opacity)
    {
        lv_obj_set_style_bg_opa(object, opacity, selector);
    }
}

static void set_border_width(lv_obj_t *object,
                             lv_coord_t width,
                             lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_obj_get_style_border_width(object, part) != width)
    {
        lv_obj_set_style_border_width(object, width, selector);
    }
}

static void set_radius(lv_obj_t *object,
                       lv_coord_t radius,
                       lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_obj_get_style_radius(object, part) != radius)
    {
        lv_obj_set_style_radius(object, radius, selector);
    }
}

static void set_text_align(lv_obj_t *object,
                           lv_text_align_t align,
                           lv_style_selector_t selector)
{
    const lv_part_t part = lv_obj_style_get_selector_part(selector);
    if (object != NULL &&
        lv_obj_get_style_text_align(object, part) != align)
    {
        lv_obj_set_style_text_align(object, align, selector);
    }
}

static void set_flag(lv_obj_t *object, lv_obj_flag_t flag, bool enabled)
{
    if (object == NULL || lv_obj_has_flag(object, flag) == enabled)
    {
        return;
    }
    if (enabled)
    {
        lv_obj_add_flag(object, flag);
    }
    else
    {
        lv_obj_clear_flag(object, flag);
    }
}

static void set_state(lv_obj_t *object, lv_state_t state, bool enabled)
{
    if (object == NULL || lv_obj_has_state(object, state) == enabled)
    {
        return;
    }
    if (enabled)
    {
        lv_obj_add_state(object, state);
    }
    else
    {
        lv_obj_clear_state(object, state);
    }
}

static void set_setting_toggle(lv_obj_t *instance, bool enabled)
{
    if (instance == NULL)
    {
        return;
    }
    lv_obj_t *icon = ui_comp_get_child(instance,
        UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ICON);
    lv_obj_t *value = ui_comp_get_child(instance,
        UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_VALUE);
    lv_obj_t *arrow = ui_comp_get_child(instance,
        UI_COMP_CMP_SETTING_TOGGLE_UI_CMP_SETTING_TOGGLE_COMPONENT_SETTING_ARROW);
    const lv_color_t color = lv_color_hex(enabled ? 0xD7FF00U : 0x8E98A7U);
    set_label(value, enabled ? "已开启" : "已关闭");
    set_text_color(icon, color, 0);
    set_text_color(value, color, 0);
    set_text_color(arrow, color, 0);
}

static void set_segmented_setting(lv_obj_t *const buttons[3],
                                  lv_obj_t *const labels[3],
                                  uint8_t selected_index)
{
    for (uint8_t index = 0U; index < 3U; ++index)
    {
        const bool selected = index == selected_index;
        if (buttons[index] != NULL)
        {
            set_background_color(
                buttons[index],
                lv_color_hex(selected ? 0xD7FF00U : 0x16181CU),
                LV_PART_MAIN);
            set_background_opa(buttons[index],
                               LV_OPA_COVER,
                               LV_PART_MAIN);
        }
        if (labels[index] != NULL)
        {
            set_text_color(
                labels[index],
                lv_color_hex(selected ? UI_CONTENT_BLACK_COLOR
                                      : UI_STATUS_ICON_DISCONNECTED_COLOR),
                LV_PART_MAIN);
        }
    }
}

static void set_ble_candidates(const ui_runtime_model_t *model)
{
    lv_obj_t *instances[UI_RUNTIME_BLE_CANDIDATE_COUNT] = {
        ui_ui_ble_candidates_device_candidate_1,
        ui_ui_ble_candidates_device_candidate_2,
        ui_ui_ble_candidates_device_candidate_3,
        ui_ui_ble_candidates_device_candidate_4,
    };
    for (uint8_t index = 0U; index < UI_RUNTIME_BLE_CANDIDATE_COUNT; ++index)
    {
        lv_obj_t *instance = instances[index];
        if (instance == NULL)
        {
            continue;
        }
        if (index >= model->ble_candidate_count)
        {
            set_flag(instance, LV_OBJ_FLAG_HIDDEN, true);
            continue;
        }
        set_flag(instance, LV_OBJ_FLAG_HIDDEN, false);
        lv_obj_t *name = ui_comp_get_child(instance,
            UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_NAME);
        lv_obj_t *status = ui_comp_get_child(instance,
            UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI);
        set_label(name, model->ble_candidates[index].name);
        char rssi[24] = {0};
        (void)snprintf(rssi, sizeof(rssi), "%d dBm",
                       (int)model->ble_candidates[index].rssi);
        set_label(status, rssi);
        const ui_runtime_ble_connection_state_t state =
            index == model->selected_ble_candidate
                ? model->ble_connection_state
                : UI_RUNTIME_BLE_CONNECTION_IDLE;
        set_ble_candidate_state(instance, state,
                                model->ble_candidates[index].rssi);
    }
}

static void set_ble_candidate_state(lv_obj_t *instance,
                                    ui_runtime_ble_connection_state_t state,
                                    int8_t rssi)
{
    lv_obj_t *icon = ui_comp_get_child(instance,
        UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ICON);
    lv_obj_t *status = ui_comp_get_child(instance,
        UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_RSSI);
    lv_obj_t *arrow = ui_comp_get_child(instance,
        UI_COMP_CMP_BLE_CANDIDATE_ROW_UI_CMP_BLE_CANDIDATE_ROW_DEVICE_CANDIDATE_1_ARROW);
    lv_color_t color = lv_color_hex(0x64D2FFU);
    if (state == UI_RUNTIME_BLE_CONNECTION_CONNECTING)
    {
        color = lv_color_hex(0x64D2FFU);
        set_label(status, "连接中");
        set_state(instance, LV_STATE_DISABLED, true);
        set_border_width(instance, 1, 0);
    }
    else if (state == UI_RUNTIME_BLE_CONNECTION_FAILED)
    {
        color = lv_color_hex(0xFF453AU);
        set_label(status, "连接失败");
        set_state(instance, LV_STATE_DISABLED, false);
        set_border_width(instance, 1, 0);
    }
    else
    {
        color = lv_color_hex(0x64D2FFU);
        set_state(instance, LV_STATE_DISABLED, false);
        set_border_width(instance, 0, 0);
        char value[24] = {0};
        (void)snprintf(value, sizeof(value), "%d dBm", (int)rssi);
        set_label(status, value);
    }
    set_border_color(instance, color, 0);
    set_text_color(icon, color, 0);
    set_text_color(status, color, 0);
    set_text_color(arrow, color, 0);
}

static void set_selftest_results(const watch_selftest_summary_t *source)
{
    if (ui_ui_selftest_result_result_display == NULL || source == NULL)
    {
        return;
    }
    ui_selftest_summary_t summary = {0};
    if (!ui_selftest_summary_build(source, &summary))
    {
        return;
    }
    update_selftest_result_visual_target(source);
    char value[8] = {0};
    (void)snprintf(value, sizeof(value), "%u", summary.pass_count);
    set_label(ui_ui_selftest_result_txt_result_pass_count, value);
    (void)snprintf(value, sizeof(value), "%u", summary.fail_count);
    set_label(ui_ui_selftest_result_txt_result_failure_count, value);
    (void)snprintf(value, sizeof(value), "%u", summary.skip_count);
    set_label(ui_ui_selftest_result_txt_result_skip_count, value);
    lv_obj_t *instances[UI_SELFTEST_CATEGORY_COUNT] = {
        ui_ui_selftest_result_result_display,
        ui_ui_selftest_result_result_touch,
        ui_ui_selftest_result_result_power,
        ui_ui_selftest_result_result_sensor,
        ui_ui_selftest_result_result_audio,
        ui_ui_selftest_result_result_gps,
        ui_ui_selftest_result_result_cellular,
    };
    if (ui_ui_selftest_result_result_ble != NULL)
    {
        set_flag(ui_ui_selftest_result_result_ble,
                 LV_OBJ_FLAG_HIDDEN,
                 true);
    }
    if (ui_ui_selftest_result_rect_result_separator_07 != NULL)
    {
        set_flag(ui_ui_selftest_result_rect_result_separator_07,
                 LV_OBJ_FLAG_HIDDEN,
                 true);
    }
    for (size_t index = 0; index < UI_SELFTEST_CATEGORY_COUNT; ++index)
    {
        set_selftest_result_row(instances[index],
                                summary.categories[index].outcome);
    }
    update_selftest_result_scroll_geometry();
}

static void set_selftest_result_row(lv_obj_t *instance,
                                    selftest_outcome_t outcome)
{
    if (instance == NULL)
    {
        return;
    }
    lv_obj_t *icon = ui_comp_get_child(instance,
        UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_ICON);
    lv_obj_t *state = ui_comp_get_child(instance,
        UI_COMP_CMP_SELFTEST_RESULT_ROW_UI_CMP_SELFTEST_RESULT_ROW_RESULT_DISPLAY_STATE);
    const char *icon_text = "";
    const char *state_text = "通过";
    lv_color_t color = lv_color_hex(0x30D158U);
    if (outcome == SELFTEST_OUTCOME_FAIL)
    {
        icon_text = "";
        state_text = "失败";
        color = lv_color_hex(0xFF453AU);
    }
    else if (outcome == SELFTEST_OUTCOME_INCONCLUSIVE)
    {
        icon_text = "";
        state_text = "未完成";
        color = lv_color_hex(UI_WATCH_BATTERY_WARNING_COLOR);
    }
    else if (outcome == SELFTEST_OUTCOME_SKIP ||
             outcome == SELFTEST_OUTCOME_NOT_RUN)
    {
        icon_text = "";
        state_text = outcome == SELFTEST_OUTCOME_SKIP ? "跳过" : "未运行";
        color = lv_color_hex(0x8E98A7U);
    }
    set_label(icon, icon_text);
    set_label(state, state_text);
    set_text_color(icon, color, 0);
    set_text_color(state, color, 0);
}

static void set_touch_selftest(const ui_runtime_model_t *model)
{
    if (model->selftest_item != SELFTEST_ITEM_TOUCH ||
        (!model->selftest_running && !model->retry_running) ||
        s_current_page != UI_PAGE_SELFTEST_MANUAL)
    {
        reset_touch_selftest();
        return;
    }
    sync_touch_selftest_run(model->selftest_run_id);
    s_touch_reply_pending = model->manual_reply_pending;

    update_touch_selftest_visual();
}

static void update_touch_selftest_visual(void)
{
    set_label(ui_ui_selftest_manual_touch_touch_target_state,
              s_touch_target_hit ? "已点击" : "待点击");
    char coordinate[32] = {0};
    if (s_touch_target_hit)
    {
        (void)snprintf(coordinate,
                       sizeof(coordinate),
                       "点击坐标：%d , %d",
                       (int)s_touch_x,
                       (int)s_touch_y);
    }
    else
    {
        (void)snprintf(coordinate, sizeof(coordinate), "点击坐标：-- , --");
    }
    set_label(ui_ui_selftest_manual_touch_touch_coordinate, coordinate);
    set_width(ui_ui_selftest_manual_touch_touch_slider_fill,
              (lv_coord_t)(30U +
                           (300U * s_touch_progress) / 100U));
    set_x(ui_ui_selftest_manual_touch_touch_slider_thumb,
          (lv_coord_t)(5U +
                       (300U * s_touch_progress) / 100U));
    set_state(
        ui_ui_selftest_manual_btn_manual_pass,
        LV_STATE_DISABLED,
        s_touch_reply_pending || !s_touch_target_hit ||
            s_touch_progress <=
                UI_SELFTEST_TOUCH_PASS_THRESHOLD_PERCENT);
}

static bool update_touch_progress_local(uint8_t progress)
{
    if (!s_initialized ||
        s_loaded_screen != UI_RUNTIME_SCREEN_SELFTEST ||
        s_current_page != UI_PAGE_SELFTEST_MANUAL ||
        s_touch_run_id == 0U)
    {
        return false;
    }
    if (progress <= s_touch_progress)
    {
        return true;
    }
    s_touch_progress = progress > 100U ? 100U : progress;
    update_touch_selftest_visual();
    return true;
}

static void reset_touch_selftest(void)
{
    s_touch_run_id = 0U;
    s_touch_target_hit = false;
    s_touch_reply_pending = false;
    s_touch_progress = 0U;
    s_touch_x = 0;
    s_touch_y = 0;
}

static void sync_touch_selftest_run(uint32_t run_id)
{
    if (s_touch_run_id == run_id)
    {
        return;
    }
    reset_touch_selftest();
    s_touch_run_id = run_id;
}

static void set_retry_detail(const ui_runtime_model_t *model)
{
    if (ui_ui_selftest_retry_detail_val_retry_item == NULL)
    {
        return;
    }
    lv_obj_t *internal_fields[] = {
        ui_ui_selftest_retry_detail_lbl_retry_reason,
        ui_ui_selftest_retry_detail_val_retry_reason,
        ui_ui_selftest_retry_detail_rect_retry_detail_divider,
        ui_ui_selftest_retry_detail_lbl_retry_code,
        ui_ui_selftest_retry_detail_val_retry_code,
        ui_ui_selftest_retry_detail_lbl_retry_elapsed,
        ui_ui_selftest_retry_detail_val_retry_elapsed,
    };
    for (size_t index = 0U;
         index < sizeof(internal_fields) / sizeof(internal_fields[0]);
         ++index)
    {
        if (internal_fields[index] != NULL)
        {
            set_flag(internal_fields[index], LV_OBJ_FLAG_HIDDEN, true);
        }
    }
    const uint8_t count = failed_item_count(model);
    if (count > 0U && s_failed_index >= count)
    {
        s_failed_index = (uint8_t)(count - 1U);
    }
    else if (count == 0U)
    {
        s_failed_index = 0U;
    }
    char eyebrow[32] = {0};
    (void)snprintf(eyebrow,
                   sizeof(eyebrow),
                   "FAILED ITEM · %02u / %02u",
                   count > 0U ? (unsigned)(s_failed_index + 1U) : 0U,
                   (unsigned)count);
    set_label(ui_ui_selftest_retry_detail_txt_retry_eyebrow, eyebrow);
    set_failed_navigation_state(
        ui_ui_selftest_retry_detail_btn_failed_prev,
        ui_ui_selftest_retry_detail_icon_failed_prev,
        count > 0U && s_failed_index > 0U);
    set_failed_navigation_state(
        ui_ui_selftest_retry_detail_btn_failed_next,
        ui_ui_selftest_retry_detail_icon_failed_next,
        count > 0U && (uint8_t)(s_failed_index + 1U) < count);
    ui_selftest_summary_t summary = {0};
    const ui_selftest_category_t category =
        failed_category_at(model, s_failed_index);
    if (category >= UI_SELFTEST_CATEGORY_COUNT ||
        !ui_selftest_summary_build(&model->selftest_results, &summary))
    {
        set_label(ui_ui_selftest_retry_detail_val_retry_item, "无失败项");
        set_label(ui_ui_selftest_retry_detail_lbl_retry_item_action,
                  "重试本项");
        set_state(ui_ui_selftest_retry_detail_btn_retry_item,
                  LV_STATE_DISABLED,
                  true);
        return;
    }
    const selftest_item_id_t item =
        summary.categories[category].source_item_id;
    set_label(ui_ui_selftest_retry_detail_val_retry_item,
              ui_selftest_category_name(category));
    set_label(ui_ui_selftest_retry_detail_lbl_retry_item_action,
              item == SELFTEST_ITEM_GPS_FIX
                  ? "设置环境并重试"
                  : "重试本项");
    set_state(ui_ui_selftest_retry_detail_btn_retry_item,
              LV_STATE_DISABLED,
              model->retry_running);
}

static void add_descendant_flags(lv_obj_t *root, lv_obj_flag_t flags)
{
    if (root == NULL)
    {
        return;
    }
    const uint32_t child_count = lv_obj_get_child_cnt(root);
    for (uint32_t index = 0U; index < child_count; ++index)
    {
        lv_obj_t *child = lv_obj_get_child(root, (int32_t)index);
        if (child == NULL)
        {
            continue;
        }
        lv_obj_add_flag(child, flags);
        add_descendant_flags(child, flags);
    }
}

static void configure_main_shell_interaction(void)
{
    if (ui_ui_main_shell_pager == NULL)
    {
        return;
    }
    add_descendant_flags(
        ui_ui_main_shell_pager,
        LV_OBJ_FLAG_SCROLL_CHAIN_HOR | LV_OBJ_FLAG_EVENT_BUBBLE);
    remove_redundant_shell_clipping();
    lv_obj_add_event_cb(ui_ui_main_shell_pager,
                        shell_release_event_cb,
                        LV_EVENT_RELEASED,
                        NULL);
}

static void configure_shell_snapshot_cache(void)
{
    lv_obj_t *pages[UI_SHELL_SLOT_COUNT] = {0};
    lv_obj_t *roots[UI_SHELL_SLOT_COUNT] = {0};
    shell_page_objects(pages, roots);
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        if (pages[slot] == NULL || roots[slot] == NULL)
        {
            ESP_LOGE(TAG, "常驻横滑快照对象树不完整，禁用快路");
            return;
        }
    }

    size_t required_bytes = 0U;
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        const size_t slot_bytes =
            lv_snapshot_buf_size_needed(roots[slot],
                                        LV_IMG_CF_TRUE_COLOR);
        if (slot_bytes > required_bytes)
        {
            required_bytes = slot_bytes;
        }
    }
    if (required_bytes == 0U)
    {
        ESP_LOGE(TAG, "常驻横滑快照尺寸为零，禁用快路");
        return;
    }

    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        s_shell_snapshot_buffers[index] =
            heap_caps_malloc(required_bytes,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (s_shell_snapshot_buffers[index] == NULL)
        {
            ESP_LOGE(
                TAG,
                "常驻横滑 PSRAM 快照缓冲分配失败：单块=%lu 字节，序号=%u",
                (unsigned long)required_bytes,
                (unsigned)index);
            release_shell_snapshot_cache();
            return;
        }
    }

    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        lv_obj_t *image = lv_img_create(pages[slot]);
        if (image == NULL)
        {
            ESP_LOGE(TAG, "常驻横滑快照覆盖对象创建失败，禁用快路");
            for (uint8_t created_slot = 0U;
                 created_slot < UI_SHELL_SLOT_COUNT;
                 ++created_slot)
            {
                if (s_shell_snapshot_images[created_slot] != NULL)
                {
                    lv_obj_del(s_shell_snapshot_images[created_slot]);
                    s_shell_snapshot_images[created_slot] = NULL;
                }
            }
            release_shell_snapshot_cache();
            return;
        }
        lv_obj_set_pos(image, 0, 0);
        lv_obj_add_flag(image, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(image,
                          LV_OBJ_FLAG_CLICKABLE |
                              LV_OBJ_FLAG_CLICK_FOCUSABLE |
                              LV_OBJ_FLAG_SCROLLABLE);
        s_shell_snapshot_images[slot] = image;
    }

    s_shell_snapshot_buffer_bytes = required_bytes;
    s_shell_snapshot_available = true;
    mark_shell_snapshots_dirty();
    ESP_LOGI(TAG,
             "常驻横滑三页 PSRAM 快照快路已就绪：单块=%lu 字节，总计=%lu 字节",
             (unsigned long)required_bytes,
             (unsigned long)(required_bytes *
                             UI_SHELL_SNAPSHOT_BUFFER_COUNT));
}

static bool prepare_shell_snapshots(void)
{
    if (!s_shell_snapshot_available || s_shell_snapshot_active)
    {
        return s_shell_snapshot_active;
    }

    const int64_t started_us = esp_timer_get_time();
    if (!shell_snapshot_cache_ready())
    {
        ++s_shell_snapshot_prepare_failures;
        s_shell_snapshot_last_prepare_ms = (uint32_t)(
            (esp_timer_get_time() - started_us + 999) / 1000);
        return false;
    }

    s_shell_snapshot_last_prepare_ms = (uint32_t)(
        (esp_timer_get_time() - started_us + 999) / 1000);
    if (s_shell_snapshot_last_prepare_ms >
        s_shell_snapshot_max_prepare_ms)
    {
        s_shell_snapshot_max_prepare_ms =
            s_shell_snapshot_last_prepare_ms;
    }
    ++s_shell_snapshot_prepare_count;
    return true;
}

static void mark_shell_snapshots_dirty(void)
{
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        s_shell_snapshot_dirty[slot] = true;
    }
}

static ui_shell_visual_signature_t shell_visual_signature_for_model(
    ui_shell_slot_t slot,
    const ui_runtime_model_t *model)
{
    ui_shell_visual_signature_t signature = {
        .static_state = s_shell_projected_states[slot],
        .cellular_connected = model->cellular_connected,
        .gps_fixed = model->gps_fixed,
        .ble_connected = model->ble_connected,
        .poweroff_committed = model->poweroff_committed,
        .power_level = model->power_level,
        .watch_battery = model->watch_battery,
    };
    switch (slot)
    {
    case UI_SHELL_SLOT_HOME:
        signature.authorization_pending = model->authorization_pending;
        (void)snprintf(signature.time,
                       sizeof(signature.time),
                       "%s",
                       model->time);
        if (signature.static_state ==
            UI_MANIFEST_STATE_HOME_NORMAL)
        {
            signature.exo_battery = model->exo_battery;
            signature.gear = model->gear;
            signature.steps = model->steps;
            signature.assist_enabled = model->assist_enabled;
        }
        else if (signature.static_state ==
                 UI_MANIFEST_STATE_HOME_DISCONNECTED)
        {
            (void)snprintf(signature.last_snapshot,
                           sizeof(signature.last_snapshot),
                           "%s",
                           model->last_snapshot);
        }
        break;
    case UI_SHELL_SLOT_GEAR:
        signature.gear = model->gear;
        if (signature.static_state ==
            UI_MANIFEST_STATE_CONTROL_CONNECTED)
        {
            signature.assist_enabled = model->assist_enabled;
        }
        break;
    case UI_SHELL_SLOT_MODE_POWER:
        signature.scene_config = model->scene_config;
        signature.drive_mode =
            model->drive_mode >= WATCH_EXOSKELETON_SCENE_MODE_MIN &&
                    model->drive_mode <= WATCH_EXOSKELETON_SCENE_MODE_MAX
                ? model->drive_mode
                : 1U;
        if (!watch_exoskeleton_scene_config_supports_mode(
                model->scene_config,
                signature.drive_mode))
        {
            signature.drive_mode = 1U;
        }
        break;
    case UI_SHELL_SLOT_ALERTS:
        if (signature.static_state ==
                UI_MANIFEST_STATE_PAYMENT_REQUIRED ||
            signature.static_state ==
                UI_MANIFEST_STATE_PAYMENT_VERIFY_FAILED)
        {
            signature.authorization_display =
                model->authorization_display;
            signature.authorization_pending =
                model->authorization_pending;
            signature.payment_required = true;
            signature.payment_pending = model->payment_pending;
            signature.payment_action_enabled =
                model->payment_action_enabled;
        }
        else if (signature.static_state ==
                 UI_MANIFEST_STATE_ALERTS_READONLY)
        {
            signature.visible_alert_count =
                model->visible_alert_count <=
                        UI_RUNTIME_VISIBLE_ALERT_COUNT
                    ? model->visible_alert_count
                    : UI_RUNTIME_VISIBLE_ALERT_COUNT;
            memcpy(signature.alerts,
                   model->alerts,
                   signature.visible_alert_count *
                       sizeof(signature.alerts[0]));
        }
        break;
    case UI_SHELL_SLOT_COUNT:
    default:
        break;
    }
    return signature;
}

static bool shell_visual_signatures_equal(
    const ui_shell_visual_signature_t *left,
    const ui_shell_visual_signature_t *right)
{
    if (left->static_state != right->static_state ||
        left->cellular_connected != right->cellular_connected ||
        left->gps_fixed != right->gps_fixed ||
        left->ble_connected != right->ble_connected ||
        left->power_level != right->power_level ||
        left->watch_battery != right->watch_battery ||
        left->exo_battery != right->exo_battery ||
        left->gear != right->gear ||
        left->drive_mode != right->drive_mode ||
        left->scene_config != right->scene_config ||
        left->poweroff_committed != right->poweroff_committed ||
        left->assist_enabled != right->assist_enabled ||
        left->authorization_pending != right->authorization_pending ||
        left->authorization_display != right->authorization_display ||
        left->payment_required != right->payment_required ||
        left->payment_pending != right->payment_pending ||
        left->payment_action_enabled != right->payment_action_enabled ||
        left->visible_alert_count != right->visible_alert_count ||
        left->steps != right->steps ||
        strcmp(left->time, right->time) != 0 ||
        strcmp(left->last_snapshot, right->last_snapshot) != 0)
    {
        return false;
    }
    for (uint8_t index = 0U;
         index < UI_RUNTIME_VISIBLE_ALERT_COUNT;
         ++index)
    {
        if (left->alerts[index].type != right->alerts[index].type ||
            left->alerts[index].danger != right->alerts[index].danger ||
            strcmp(left->alerts[index].label,
                   right->alerts[index].label) != 0 ||
            strcmp(left->alerts[index].value,
                   right->alerts[index].value) != 0)
        {
            return false;
        }
    }
    return true;
}

static void update_shell_visual_targets(
    const ui_runtime_model_t *model)
{
    uint8_t changed_pages = 0U;
    for (ui_shell_slot_t slot = UI_SHELL_SLOT_HOME;
         slot < UI_SHELL_SLOT_COUNT;
         slot = (ui_shell_slot_t)(slot + 1))
    {
        const ui_shell_visual_signature_t next =
            shell_visual_signature_for_model(slot, model);
        if (s_shell_visual_signature_valid[slot] &&
            shell_visual_signatures_equal(
                &s_shell_target_signatures[slot],
                &next))
        {
            continue;
        }
        s_shell_target_signatures[slot] = next;
        s_shell_visual_signature_valid[slot] = true;
        ++s_shell_target_generations[slot];
        if (s_shell_target_generations[slot] == 0U)
        {
            s_shell_target_generations[slot] = 1U;
        }
        s_shell_snapshot_dirty[slot] = true;
        ++changed_pages;
    }
    if (changed_pages == 0U)
    {
        ++s_shell_ignored_refresh_count;
    }
    else
    {
        s_shell_invalidated_page_count += changed_pages;
    }
}

static bool shell_snapshot_cache_ready(void)
{
    static const int8_t relative_order[UI_SHELL_SNAPSHOT_BUFFER_COUNT] = {
        -1,
        0,
        1,
    };
    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        const ui_shell_slot_t slot = (ui_shell_slot_t)(
            (s_shell_slot + UI_SHELL_SLOT_COUNT +
             relative_order[index]) %
            UI_SHELL_SLOT_COUNT);
        const int8_t buffer = shell_snapshot_buffer_for_slot(slot);
        if (buffer < 0 ||
            !s_shell_snapshot_slot_valid[(uint8_t)buffer] ||
            s_shell_snapshot_dirty[slot] ||
            s_shell_snapshot_buffer_generations[(uint8_t)buffer] !=
                s_shell_target_generations[slot])
        {
            return false;
        }
    }
    return true;
}

static int8_t shell_snapshot_buffer_for_slot(ui_shell_slot_t slot)
{
    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        if (s_shell_snapshot_slot_valid[index] &&
            s_shell_snapshot_slots[index] == slot)
        {
            return (int8_t)index;
        }
    }
    return -1;
}

static int8_t shell_snapshot_replacement_buffer(ui_shell_slot_t slot)
{
    (void)slot;
    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        if (!s_shell_snapshot_slot_valid[index])
        {
            return (int8_t)index;
        }
    }

    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        const ui_shell_slot_t buffered_slot =
            s_shell_snapshot_slots[index];
        bool still_adjacent = false;
        for (int8_t relative = -1; relative <= 1; ++relative)
        {
            const ui_shell_slot_t desired_slot =
                (ui_shell_slot_t)(
                    (s_shell_slot + UI_SHELL_SLOT_COUNT +
                     relative) %
                    UI_SHELL_SLOT_COUNT);
            if (buffered_slot == desired_slot)
            {
                still_adjacent = true;
                break;
            }
        }
        if (!still_adjacent)
        {
            return (int8_t)index;
        }
    }
    return -1;
}

static bool shell_touch_sequence_active(void)
{
    ui_touch_snapshot_t snapshot = {0};
    return ui_touch_session_snapshot(&snapshot) &&
           snapshot.active && !snapshot.cancelled;
}

static void set_shell_snapshot_active(bool active)
{
    if (active && (!s_shell_snapshot_available ||
                   s_shell_snapshot_active))
    {
        return;
    }
    if (!active && !s_shell_snapshot_active)
    {
        return;
    }

    lv_disp_t *display = lv_disp_get_default();
    if (active && display != NULL)
    {
        /*
         * 横滑期间由 ui_task 直接合成三页快照并提交显示事务；
         * 清空旧脏区后暂停 LVGL 绘制，避免重复软件混合整屏快照。
         */
        _lv_inv_area(display, NULL);
        lv_disp_enable_invalidation(display, false);
    }
    else if (!active && display != NULL)
    {
        lv_disp_enable_invalidation(display, true);
    }

    lv_obj_t *roots[UI_SHELL_SLOT_COUNT] = {0};
    shell_page_objects(NULL, roots);
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        if (roots[slot] != NULL)
        {
            if (active)
            {
                lv_obj_add_flag(roots[slot], LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                lv_obj_clear_flag(roots[slot], LV_OBJ_FLAG_HIDDEN);
            }
        }
        if (s_shell_snapshot_images[slot] != NULL)
        {
            lv_obj_add_flag(s_shell_snapshot_images[slot],
                            LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (active)
    {
        for (uint8_t index = 0U;
             index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
             ++index)
        {
            const ui_shell_slot_t slot = s_shell_snapshot_slots[index];
            if (slot < UI_SHELL_SLOT_COUNT &&
                s_shell_snapshot_images[slot] != NULL)
            {
                lv_obj_clear_flag(s_shell_snapshot_images[slot],
                                  LV_OBJ_FLAG_HIDDEN);
                lv_obj_move_foreground(s_shell_snapshot_images[slot]);
            }
        }
    }
    s_shell_snapshot_active = active;
    if (!active)
    {
        request_full_redraw();
    }
}

static void release_shell_snapshot_cache(void)
{
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        s_shell_snapshot_images[slot] = NULL;
    }
    for (uint8_t index = 0U;
         index < UI_SHELL_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        if (s_shell_snapshot_buffers[index] != NULL)
        {
            heap_caps_free(s_shell_snapshot_buffers[index]);
            s_shell_snapshot_buffers[index] = NULL;
        }
        memset(&s_shell_snapshot_descriptors[index],
               0,
               sizeof(s_shell_snapshot_descriptors[index]));
        s_shell_snapshot_slots[index] = UI_SHELL_SLOT_COUNT;
        s_shell_snapshot_slot_valid[index] = false;
        s_shell_snapshot_buffer_generations[index] = 0U;
    }
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        s_shell_snapshot_dirty[slot] = false;
        s_shell_visual_signature_valid[slot] = false;
        s_shell_target_generations[slot] = 0U;
    }
    s_shell_snapshot_buffer_bytes = 0U;
    s_shell_snapshot_available = false;
    s_shell_snapshot_active = false;
}

static void shell_page_objects(lv_obj_t *pages[UI_SHELL_SLOT_COUNT],
                               lv_obj_t *roots[UI_SHELL_SLOT_COUNT])
{
    if (pages != NULL)
    {
        pages[UI_SHELL_SLOT_HOME] = ui_ui_main_shell_page_home;
        pages[UI_SHELL_SLOT_GEAR] = ui_ui_main_shell_page_control;
        pages[UI_SHELL_SLOT_MODE_POWER] = ui_ui_main_shell_page_mode;
        pages[UI_SHELL_SLOT_ALERTS] = ui_ui_main_shell_page_alerts;
    }
    if (roots != NULL)
    {
        roots[UI_SHELL_SLOT_HOME] = ui_ui_page_home_root;
        roots[UI_SHELL_SLOT_GEAR] = ui_ui_page_control_root;
        roots[UI_SHELL_SLOT_MODE_POWER] = ui_ui_page_mode_power_root;
        roots[UI_SHELL_SLOT_ALERTS] = ui_ui_page_alerts_root;
    }
}

static void remove_redundant_shell_clipping(void)
{
    lv_obj_t *objects[] = {
        ui_ui_main_shell_pager,
        ui_ui_main_shell_page_home,
        ui_ui_page_home_root,
        ui_ui_main_shell_page_control,
        ui_ui_page_control_root,
        ui_ui_main_shell_page_mode,
        ui_ui_page_mode_power_root,
        ui_ui_main_shell_page_alerts,
        ui_ui_page_alerts_root,
    };
    for (size_t index = 0U;
         index < sizeof(objects) / sizeof(objects[0]);
         ++index)
    {
        if (objects[index] == NULL)
        {
            continue;
        }
        /*
         * 主 Screen 已执行 110 px 圆角裁剪；内层仍保留矩形视口裁剪，
         * 只移除重复的圆角软件遮罩。
         */
        lv_obj_set_style_radius(objects[index],
                                0,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_clip_corner(objects[index],
                                     false,
                                     LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

static void remove_redundant_temporary_clipping(
    ui_runtime_screen_t screen)
{
    lv_obj_t *root = NULL;
    switch (screen)
    {
    case UI_RUNTIME_SCREEN_SETTINGS:
        root = ui_ui_page_settings_root;
        break;
    case UI_RUNTIME_SCREEN_DEVICE_INFO:
        root = ui_ui_page_device_info_root;
        break;
    case UI_RUNTIME_SCREEN_MAINTENANCE:
        root = ui_ui_page_maintenance_root;
        break;
    case UI_RUNTIME_SCREEN_BLE:
        root = ui_ui_page_ble_root;
        break;
    case UI_RUNTIME_SCREEN_SELFTEST:
        root = ui_ui_page_selftest_progress_root;
        break;
    case UI_RUNTIME_SCREEN_SELFTEST_RESULT:
        root = ui_ui_page_selftest_result_root;
        break;
    case UI_RUNTIME_SCREEN_GPS_CONTEXT:
        root = ui_ui_page_selftest_gps_context_root;
        break;
    case UI_RUNTIME_SCREEN_RETRY_DETAIL:
        root = ui_ui_page_selftest_retry_detail_root;
        break;
    case UI_RUNTIME_SCREEN_MAIN:
    case UI_RUNTIME_SCREEN_COUNT:
    default:
        break;
    }
    if (root == NULL)
    {
        return;
    }
    /*
     * 外层 Screen 已拥有最终 110 px 圆角；内层根只负责布局，
     * 同时关闭生成物遗留的无意义整页滚动与弹性动画。
     */
    set_radius(root, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_clip_corner(root,
                                 false,
                                 LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_clear_flag(root,
                      LV_OBJ_FLAG_SCROLLABLE |
                          LV_OBJ_FLAG_SCROLL_ELASTIC |
                          LV_OBJ_FLAG_SCROLL_MOMENTUM |
                          LV_OBJ_FLAG_SCROLL_CHAIN);
}

static void arrange_shell_pages(ui_shell_slot_t current_slot)
{
    if (current_slot >= UI_SHELL_SLOT_COUNT)
    {
        return;
    }
    lv_obj_t *pages[UI_SHELL_SLOT_COUNT] = {0};
    shell_page_objects(pages, NULL);
    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        if (pages[slot] == NULL)
        {
            return;
        }
    }

    for (uint8_t slot = 0U; slot < UI_SHELL_SLOT_COUNT; ++slot)
    {
        const uint8_t relative_slot =
            (uint8_t)((slot + UI_SHELL_SLOT_COUNT - current_slot) %
                      UI_SHELL_SLOT_COUNT);
        const uint8_t physical_slot =
            relative_slot == UI_SHELL_SLOT_COUNT - 1U
                ? 0U
                : (uint8_t)(relative_slot + 1U);
        lv_obj_set_x(pages[slot],
                     (lv_coord_t)physical_slot * UI_SHELL_PAGE_WIDTH);
    }
    lv_obj_update_layout(ui_ui_main_shell_pager);
}

static void commit_shell_pager_slot(void)
{
    if (ui_ui_main_shell_pager == NULL)
    {
        return;
    }
    arrange_shell_pages(s_shell_slot);
    s_shell_scroll_peak_delta = 0;
    lv_obj_scroll_to_x(ui_ui_main_shell_pager,
                       UI_SHELL_PAGE_WIDTH,
                       LV_ANIM_OFF);
    s_committed_shell_slot = s_shell_slot;
}

static void configure_settings_snapshot_cache(void)
{
    if (ui_ui_scr_settings == NULL ||
        ui_ui_settings_settings_list_content == NULL)
    {
        ESP_LOGE(TAG, "设置纵滑快照对象树不完整，禁用快路");
        return;
    }

    const size_t background_bytes =
        lv_snapshot_buf_size_needed(ui_ui_scr_settings,
                                    LV_IMG_CF_TRUE_COLOR);
    const size_t content_bytes =
        lv_snapshot_buf_size_needed(
            ui_ui_settings_settings_list_content,
            LV_IMG_CF_TRUE_COLOR);
    if (background_bytes == 0U || content_bytes == 0U)
    {
        ESP_LOGE(TAG,
                 "设置纵滑快照尺寸无效：背景=%lu，内容=%lu",
                 (unsigned long)background_bytes,
                 (unsigned long)content_bytes);
        return;
    }

    s_settings_background_snapshot_buffer =
        heap_caps_malloc(background_bytes,
                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    for (uint8_t index = 0U;
         index < UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        s_settings_content_snapshot_buffers[index] =
            heap_caps_malloc(content_bytes,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (s_settings_background_snapshot_buffer == NULL ||
        s_settings_content_snapshot_buffers[0] == NULL ||
        s_settings_content_snapshot_buffers[1] == NULL)
    {
        ESP_LOGE(TAG,
                 "设置纵滑 PSRAM 快照缓冲分配失败：背景=%lu，双内容各=%lu",
                 (unsigned long)background_bytes,
                 (unsigned long)content_bytes);
        release_settings_snapshot_cache();
        return;
    }

    s_settings_background_snapshot_bytes = background_bytes;
    s_settings_content_snapshot_bytes = content_bytes;
    s_settings_snapshot_available = true;
    s_settings_snapshot_warm_fault_latched = false;
    s_settings_background_snapshot_valid = false;
    memset(s_settings_content_snapshot_valid,
           0,
           sizeof(s_settings_content_snapshot_valid));
    s_settings_content_front_index =
        UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE;
    s_settings_front_generation = 0U;
    ESP_LOGI(TAG,
             "设置纵滑 PSRAM 双内容快照快路已就绪：背景=%lu 字节，双内容各=%lu 字节，总计=%lu 字节",
             (unsigned long)background_bytes,
             (unsigned long)content_bytes,
             (unsigned long)(
                 background_bytes +
                 UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT *
                     content_bytes));
}

static bool prepare_settings_snapshots(void)
{
    if (!s_settings_snapshot_available ||
        s_settings_snapshot_active)
    {
        return s_settings_snapshot_active;
    }

    const int64_t started_us = esp_timer_get_time();
    if (!settings_snapshot_cache_ready())
    {
        ++s_settings_snapshot_prepare_failures;
        s_settings_snapshot_last_prepare_ms = (uint32_t)(
            (esp_timer_get_time() - started_us + 999) / 1000);
        return false;
    }

    s_settings_snapshot_last_prepare_ms = (uint32_t)(
        (esp_timer_get_time() - started_us + 999) / 1000);
    if (s_settings_snapshot_last_prepare_ms >
        s_settings_snapshot_max_prepare_ms)
    {
        s_settings_snapshot_max_prepare_ms =
            s_settings_snapshot_last_prepare_ms;
    }
    ++s_settings_snapshot_prepare_count;
    return true;
}

static void set_settings_snapshot_active(bool active)
{
    if (active && (!s_settings_snapshot_available ||
                   s_settings_snapshot_active))
    {
        return;
    }
    if (!active && !s_settings_snapshot_active)
    {
        return;
    }

    lv_disp_t *display = lv_disp_get_default();
    if (active && display != NULL)
    {
        /*
         * 纵滑期间由 ui_task 直接拼接静态背景和连续七栏内容；
         * 清空旧脏区后暂停 LVGL 软件重绘，保留对象树继续接收触摸。
         */
        _lv_inv_area(display, NULL);
        lv_disp_enable_invalidation(display, false);
    }
    else if (!active && display != NULL)
    {
        lv_disp_enable_invalidation(display, true);
        request_full_redraw();
    }
    s_settings_snapshot_active = active;
}

static ui_settings_visual_signature_t settings_visual_signature_for_model(
    const ui_runtime_model_t *model)
{
    const uint8_t brightness_index =
        model->brightness_level < UI_RUNTIME_BRIGHTNESS_COUNT
            ? (uint8_t)model->brightness_level
            : (uint8_t)UI_RUNTIME_BRIGHTNESS_HIGH;
    const uint8_t timeout_index =
        model->screen_timeout_seconds == UI_RUNTIME_SCREEN_TIMEOUT_15S
            ? 1U
            : model->screen_timeout_seconds ==
                      UI_RUNTIME_SCREEN_TIMEOUT_30S
                  ? 2U
                  : 0U;
    return (ui_settings_visual_signature_t){
        .haptics_enabled = model->haptics_enabled,
        .click_audio_enabled = model->click_audio_enabled,
        .raise_to_wake_enabled = model->raise_to_wake_enabled,
        .brightness_index = brightness_index,
        .timeout_index = timeout_index,
    };
}

static bool settings_visual_signatures_equal(
    const ui_settings_visual_signature_t *left,
    const ui_settings_visual_signature_t *right)
{
    return left->haptics_enabled == right->haptics_enabled &&
           left->click_audio_enabled == right->click_audio_enabled &&
           left->raise_to_wake_enabled == right->raise_to_wake_enabled &&
           left->brightness_index == right->brightness_index &&
           left->timeout_index == right->timeout_index;
}

static void update_settings_visual_target(
    const ui_runtime_model_t *model)
{
    const ui_settings_visual_signature_t next =
        settings_visual_signature_for_model(model);
    if (s_settings_visual_signature_valid &&
        settings_visual_signatures_equal(&s_settings_target_signature,
                                         &next))
    {
        ++s_settings_ignored_refresh_count;
        return;
    }

    s_settings_target_signature = next;
    s_settings_visual_signature_valid = true;
    ++s_settings_target_generation;
    if (s_settings_target_generation == 0U)
    {
        s_settings_target_generation = 1U;
    }
}

static bool settings_snapshot_cache_ready(void)
{
    return s_settings_snapshot_available &&
           !s_settings_snapshot_warm_fault_latched &&
           s_settings_background_snapshot_valid &&
           s_settings_content_front_index <
               UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT &&
           s_settings_content_snapshot_valid
               [s_settings_content_front_index];
}

static void release_settings_snapshot_cache(void)
{
    if (s_settings_background_snapshot_buffer != NULL)
    {
        heap_caps_free(s_settings_background_snapshot_buffer);
        s_settings_background_snapshot_buffer = NULL;
    }
    for (uint8_t index = 0U;
         index < UI_SETTINGS_CONTENT_SNAPSHOT_BUFFER_COUNT;
         ++index)
    {
        if (s_settings_content_snapshot_buffers[index] != NULL)
        {
            heap_caps_free(
                s_settings_content_snapshot_buffers[index]);
            s_settings_content_snapshot_buffers[index] = NULL;
        }
    }
    memset(&s_settings_background_snapshot_descriptor,
           0,
           sizeof(s_settings_background_snapshot_descriptor));
    memset(s_settings_content_snapshot_descriptors,
           0,
           sizeof(s_settings_content_snapshot_descriptors));
    s_settings_background_snapshot_bytes = 0U;
    s_settings_content_snapshot_bytes = 0U;
    s_settings_snapshot_available = false;
    s_settings_snapshot_warm_fault_latched = false;
    s_settings_background_snapshot_valid = false;
    memset(s_settings_content_snapshot_valid,
           0,
           sizeof(s_settings_content_snapshot_valid));
    s_settings_content_front_index =
        UI_SETTINGS_CONTENT_SNAPSHOT_INDEX_NONE;
    s_settings_visual_signature_valid = false;
    memset(&s_settings_target_signature,
           0,
           sizeof(s_settings_target_signature));
    s_settings_target_generation = 0U;
    s_settings_front_generation = 0U;
    s_settings_snapshot_active = false;
}

static void configure_settings_scroll(void)
{
    if (ui_ui_settings_settings_list_viewport == NULL)
    {
        return;
    }
    lv_obj_add_event_cb(ui_ui_settings_settings_list_viewport,
                        settings_scroll_event_cb,
                        LV_EVENT_SCROLL_BEGIN,
                        NULL);
    lv_obj_add_event_cb(ui_ui_settings_settings_list_viewport,
                        settings_scroll_event_cb,
                        LV_EVENT_SCROLL,
                        NULL);
    lv_obj_add_event_cb(ui_ui_settings_settings_list_viewport,
                        settings_scroll_event_cb,
                        LV_EVENT_SCROLL_END,
                        NULL);
    if (ui_ui_page_settings_root != NULL)
    {
        lv_obj_add_event_cb(ui_ui_page_settings_root,
                            settings_release_event_cb,
                            LV_EVENT_RELEASED,
                            NULL);
    }
    normalize_settings_interaction();
    lv_obj_scroll_to_y(ui_ui_settings_settings_list_viewport,
                       s_settings_scroll_y,
                       LV_ANIM_OFF);
}

static void normalize_settings_interaction(void)
{
    if (ui_ui_settings_settings_list_viewport == NULL)
    {
        return;
    }
    lv_obj_add_flag(ui_ui_settings_settings_list_viewport,
                    LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_ui_settings_settings_list_viewport,
                      LV_OBJ_FLAG_SCROLL_CHAIN |
                          LV_OBJ_FLAG_SCROLL_ONE |
                          LV_OBJ_FLAG_SCROLL_ELASTIC);
    if (ui_ui_settings_settings_list_page_1 != NULL)
    {
        lv_obj_clear_flag(ui_ui_settings_settings_list_page_1,
                          LV_OBJ_FLAG_SNAPPABLE);
    }
    if (ui_ui_settings_settings_list_page_2 != NULL)
    {
        lv_obj_clear_flag(ui_ui_settings_settings_list_page_2,
                          LV_OBJ_FLAG_SNAPPABLE);
    }
    set_object_geometry(ui_ui_settings_settings_list_content,
                        0,
                        0,
                        378,
                        UI_SETTINGS_LIST_CONTENT_HEIGHT);
    set_object_geometry(ui_ui_settings_settings_list_page_1,
                        0,
                        0,
                        378,
                        UI_SETTINGS_LIST_VIEWPORT_HEIGHT);
    set_object_geometry(ui_ui_settings_settings_list_page_2,
                        0,
                        UI_SETTINGS_LIST_TRAILING_GROUP_Y,
                        378,
                        UI_SETTINGS_LIST_TRAILING_GROUP_HEIGHT);
    add_descendant_flags(ui_ui_settings_settings_list_viewport,
                         LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_set_scroll_dir(ui_ui_settings_settings_list_viewport, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_ui_settings_settings_list_viewport,
                              LV_SCROLLBAR_MODE_OFF);
    if (ui_ui_page_settings_root != NULL)
    {
        add_descendant_flags(ui_ui_page_settings_root,
                             LV_OBJ_FLAG_EVENT_BUBBLE);
    }
    set_object_geometry(ui_ui_settings_gesture_zone_settings_return,
                        UI_SETTINGS_RETURN_ZONE_X,
                        UI_SETTINGS_RETURN_ZONE_Y,
                        UI_SETTINGS_RETURN_ZONE_WIDTH,
                        UI_SETTINGS_RETURN_ZONE_HEIGHT);
    update_settings_scroll_thumb();
}

static void settings_scroll_event_cb(lv_event_t *event)
{
    if (event == NULL ||
        ui_ui_settings_settings_list_viewport == NULL ||
        lv_event_get_target(event) != ui_ui_settings_settings_list_viewport)
    {
        return;
    }
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCROLL_BEGIN)
    {
        s_settings_fast_path_fault_latched = false;
        if (prepare_settings_snapshots())
        {
            set_settings_snapshot_active(true);
        }
        s_settings_motion_active = true;
        return;
    }
    if (code != LV_EVENT_SCROLL && code != LV_EVENT_SCROLL_END)
    {
        return;
    }
    if (code == LV_EVENT_SCROLL && !s_settings_motion_active &&
        !s_settings_fast_path_fault_latched &&
        shell_touch_sequence_active())
    {
        if (prepare_settings_snapshots())
        {
            set_settings_snapshot_active(true);
        }
        s_settings_motion_active = true;
    }
    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_settings_settings_list_viewport);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    if (scroll_y > UI_SETTINGS_LIST_MAX_SCROLL_Y)
    {
        scroll_y = UI_SETTINGS_LIST_MAX_SCROLL_Y;
    }
    s_settings_scroll_y = (lv_coord_t)scroll_y;
    if (!s_settings_snapshot_active)
    {
        update_settings_scroll_thumb();
    }
    if (code == LV_EVENT_SCROLL_END)
    {
        s_settings_fast_path_fault_latched = false;
        s_settings_motion_active = false;
        update_settings_scroll_thumb();
        set_settings_snapshot_active(false);
    }
}

static void update_settings_scroll_thumb(void)
{
    if (ui_ui_settings_settings_list_viewport == NULL ||
        ui_ui_settings_settings_page_thumb == NULL)
    {
        return;
    }
    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_settings_settings_list_viewport);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    if (scroll_y > UI_SETTINGS_LIST_MAX_SCROLL_Y)
    {
        scroll_y = UI_SETTINGS_LIST_MAX_SCROLL_Y;
    }
    const int32_t thumb_range =
        UI_SETTINGS_SCROLL_THUMB_BOTTOM_Y -
        UI_SETTINGS_SCROLL_THUMB_TOP_Y;
    const lv_coord_t thumb_y =
        (lv_coord_t)(UI_SETTINGS_SCROLL_THUMB_TOP_Y +
                     scroll_y * thumb_range /
                         UI_SETTINGS_LIST_MAX_SCROLL_Y);
    lv_obj_set_y(ui_ui_settings_settings_page_thumb, thumb_y);
}

static void configure_selftest_result_scroll(void)
{
    if (ui_ui_selftest_result_result_list == NULL)
    {
        return;
    }
    lv_obj_add_flag(ui_ui_selftest_result_result_list,
                    LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(ui_ui_selftest_result_result_list,
                      LV_OBJ_FLAG_SCROLL_CHAIN |
                          LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_set_style_clip_corner(
        ui_ui_selftest_result_result_list,
        false,
        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_scroll_dir(ui_ui_selftest_result_result_list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_ui_selftest_result_result_list,
                              LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(ui_ui_selftest_result_result_list,
                        selftest_result_scroll_event_cb,
                        LV_EVENT_SCROLL_BEGIN,
                        NULL);
    lv_obj_add_event_cb(ui_ui_selftest_result_result_list,
                        selftest_result_scroll_event_cb,
                        LV_EVENT_SCROLL,
                        NULL);
    lv_obj_add_event_cb(ui_ui_selftest_result_result_list,
                        selftest_result_scroll_event_cb,
                        LV_EVENT_SCROLL_END,
                        NULL);
    lv_obj_scroll_to_y(ui_ui_selftest_result_result_list, 0, LV_ANIM_OFF);
    update_selftest_result_scroll_geometry();
}

static void configure_selftest_result_snapshot_cache(void)
{
    if (ui_ui_scr_selftest_result == NULL ||
        ui_ui_selftest_result_result_list == NULL)
    {
        ESP_LOGE(TAG, "自检结果纵滑快照对象树不完整，禁用快路");
        return;
    }
    const size_t background_bytes =
        lv_snapshot_buf_size_needed(ui_ui_scr_selftest_result,
                                    LV_IMG_CF_TRUE_COLOR);
    const lv_coord_t old_height =
        lv_obj_get_height(ui_ui_selftest_result_result_list);
    set_height(ui_ui_selftest_result_result_list,
               UI_SELFTEST_RESULT_CONTENT_MAX_HEIGHT);
    const size_t content_bytes =
        lv_snapshot_buf_size_needed(
            ui_ui_selftest_result_result_list,
            LV_IMG_CF_TRUE_COLOR);
    set_height(ui_ui_selftest_result_result_list, old_height);
    if (background_bytes == 0U || content_bytes == 0U)
    {
        ESP_LOGE(TAG,
                 "自检结果纵滑快照尺寸无效：背景=%lu，内容=%lu",
                 (unsigned long)background_bytes,
                 (unsigned long)content_bytes);
        return;
    }
    const size_t transient_bytes =
        background_bytes +
        UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT * content_bytes;
    if (transient_bytes > UI_SETTINGS_SNAPSHOT_TRANSIENT_BUDGET_BYTES)
    {
        ESP_LOGE(TAG,
                 "自检结果纵滑快照超过 transient 预算：总计=%lu 字节",
                 (unsigned long)transient_bytes);
        return;
    }

    s_selftest_result_background_snapshot_buffer =
        heap_caps_malloc(background_bytes,
                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    for (uint8_t index = 0U;
         index < UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT;
         ++index)
    {
        s_selftest_result_content_snapshot_buffers[index] =
            heap_caps_malloc(content_bytes,
                             MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
    if (s_selftest_result_background_snapshot_buffer == NULL ||
        s_selftest_result_content_snapshot_buffers[0] == NULL ||
        s_selftest_result_content_snapshot_buffers[1] == NULL)
    {
        ESP_LOGE(TAG,
                 "自检结果纵滑 PSRAM 缓冲分配失败：背景=%lu，双内容各=%lu",
                 (unsigned long)background_bytes,
                 (unsigned long)content_bytes);
        release_selftest_result_snapshot_cache();
        return;
    }
    s_selftest_result_background_snapshot_bytes = background_bytes;
    s_selftest_result_content_snapshot_bytes = content_bytes;
    s_selftest_result_snapshot_available = true;
    s_selftest_result_content_front_index =
        UI_SELFTEST_RESULT_CONTENT_INDEX_NONE;
    ESP_LOGI(TAG,
             "自检结果纵滑 PSRAM 双内容快照已就绪：背景=%lu 字节，双内容各=%lu 字节，总计=%lu 字节",
             (unsigned long)background_bytes,
             (unsigned long)content_bytes,
             (unsigned long)transient_bytes);
}

bool ui_runtime_binding_warm_selftest_result_snapshot(void)
{
    if (!s_initialized || !s_selftest_result_snapshot_available ||
        s_loaded_screen != UI_RUNTIME_SCREEN_SELFTEST_RESULT ||
        s_selftest_result_motion_active ||
        s_selftest_result_snapshot_active)
    {
        return false;
    }

    lv_obj_t *snapshot_object = NULL;
    lv_img_dsc_t *descriptor = NULL;
    void *buffer = NULL;
    size_t buffer_bytes = 0U;
    bool background_snapshot = false;
    uint8_t content_buffer_index =
        UI_SELFTEST_RESULT_CONTENT_INDEX_NONE;
    if (!s_selftest_result_background_snapshot_valid)
    {
        snapshot_object = ui_ui_scr_selftest_result;
        descriptor =
            &s_selftest_result_background_snapshot_descriptor;
        buffer = s_selftest_result_background_snapshot_buffer;
        buffer_bytes = s_selftest_result_background_snapshot_bytes;
        background_snapshot = true;
    }
    else if (s_selftest_result_visual_signature_valid &&
             (s_selftest_result_content_front_index ==
                  UI_SELFTEST_RESULT_CONTENT_INDEX_NONE ||
              s_selftest_result_front_generation !=
                  s_selftest_result_target_generation))
    {
        content_buffer_index =
            s_selftest_result_content_front_index ==
                    UI_SELFTEST_RESULT_CONTENT_INDEX_NONE
                ? 0U
                : (uint8_t)(
                      1U - s_selftest_result_content_front_index);
        snapshot_object = ui_ui_selftest_result_result_list;
        descriptor =
            &s_selftest_result_content_snapshot_descriptors
                [content_buffer_index];
        buffer =
            s_selftest_result_content_snapshot_buffers
                [content_buffer_index];
        buffer_bytes = s_selftest_result_content_snapshot_bytes;
    }
    else
    {
        return false;
    }
    if (snapshot_object == NULL || descriptor == NULL ||
        buffer == NULL || buffer_bytes == 0U)
    {
        ++s_selftest_result_snapshot_warm_failures;
        return false;
    }

    const bool list_was_hidden =
        lv_obj_has_flag(ui_ui_selftest_result_result_list,
                        LV_OBJ_FLAG_HIDDEN);
    const bool thumb_was_hidden =
        lv_obj_has_flag(ui_ui_selftest_result_result_scroll_thumb,
                        LV_OBJ_FLAG_HIDDEN);
    const lv_coord_t old_height =
        lv_obj_get_height(ui_ui_selftest_result_result_list);
    const lv_coord_t old_scroll =
        lv_obj_get_scroll_y(ui_ui_selftest_result_result_list);
    if (background_snapshot)
    {
        set_flag(ui_ui_selftest_result_result_list,
                 LV_OBJ_FLAG_HIDDEN,
                 true);
        set_flag(ui_ui_selftest_result_result_scroll_thumb,
                 LV_OBJ_FLAG_HIDDEN,
                 true);
    }
    else
    {
        lv_obj_scroll_to_y(ui_ui_selftest_result_result_list,
                           0,
                           LV_ANIM_OFF);
        set_height(ui_ui_selftest_result_result_list,
                   s_selftest_result_content_height);
        lv_obj_update_layout(ui_ui_selftest_result_result_list);
    }

    const int64_t started_us = esp_timer_get_time();
    const lv_res_t result = lv_snapshot_take_to_buf(
        snapshot_object,
        LV_IMG_CF_TRUE_COLOR,
        descriptor,
        buffer,
        (uint32_t)buffer_bytes);
    const uint32_t duration_ms = (uint32_t)(
        (esp_timer_get_time() - started_us + 999) / 1000);
    if (duration_ms > s_selftest_result_snapshot_max_warm_ms)
    {
        s_selftest_result_snapshot_max_warm_ms = duration_ms;
    }
    if (background_snapshot)
    {
        set_flag(ui_ui_selftest_result_result_list,
                 LV_OBJ_FLAG_HIDDEN,
                 list_was_hidden);
        set_flag(ui_ui_selftest_result_result_scroll_thumb,
                 LV_OBJ_FLAG_HIDDEN,
                 thumb_was_hidden);
    }
    else
    {
        set_height(ui_ui_selftest_result_result_list, old_height);
        lv_obj_scroll_to_y(ui_ui_selftest_result_result_list,
                           old_scroll,
                           LV_ANIM_OFF);
    }
    if (result != LV_RES_OK)
    {
        ++s_selftest_result_snapshot_warm_failures;
        ESP_LOGE(TAG,
                 "自检结果纵滑闲时%s快照生成失败：耗时=%lums",
                 background_snapshot ? "背景" : "内容",
                 (unsigned long)duration_ms);
        return false;
    }
    if (background_snapshot)
    {
        s_selftest_result_background_snapshot_valid = true;
        ++s_selftest_result_background_build_count;
    }
    else
    {
        const bool replacing_front =
            s_selftest_result_content_front_index !=
            UI_SELFTEST_RESULT_CONTENT_INDEX_NONE;
        s_selftest_result_content_snapshot_valid
            [content_buffer_index] = true;
        s_selftest_result_content_front_index =
            content_buffer_index;
        s_selftest_result_front_generation =
            s_selftest_result_target_generation;
        ++s_selftest_result_content_rebuild_count;
        if (replacing_front)
        {
            ++s_selftest_result_content_swap_count;
        }
    }
    ++s_selftest_result_snapshot_warm_count;
    return true;
}

static bool prepare_selftest_result_snapshots(void)
{
    if (!s_selftest_result_snapshot_available ||
        s_selftest_result_snapshot_active)
    {
        return s_selftest_result_snapshot_active;
    }
    const int64_t started_us = esp_timer_get_time();
    if (!selftest_result_snapshot_cache_ready())
    {
        ++s_selftest_result_snapshot_prepare_failures;
        s_selftest_result_snapshot_last_prepare_ms = (uint32_t)(
            (esp_timer_get_time() - started_us + 999) / 1000);
        return false;
    }
    s_selftest_result_snapshot_last_prepare_ms = (uint32_t)(
        (esp_timer_get_time() - started_us + 999) / 1000);
    if (s_selftest_result_snapshot_last_prepare_ms >
        s_selftest_result_snapshot_max_prepare_ms)
    {
        s_selftest_result_snapshot_max_prepare_ms =
            s_selftest_result_snapshot_last_prepare_ms;
    }
    ++s_selftest_result_snapshot_prepare_count;
    return true;
}

static void set_selftest_result_snapshot_active(bool active)
{
    if (active && (!s_selftest_result_snapshot_available ||
                   s_selftest_result_snapshot_active))
    {
        return;
    }
    if (!active && !s_selftest_result_snapshot_active)
    {
        return;
    }
    lv_disp_t *display = lv_disp_get_default();
    if (active && display != NULL)
    {
        _lv_inv_area(display, NULL);
        lv_disp_enable_invalidation(display, false);
    }
    else if (!active && display != NULL)
    {
        lv_disp_enable_invalidation(display, true);
        request_full_redraw();
    }
    s_selftest_result_snapshot_active = active;
}

static bool selftest_result_snapshot_cache_ready(void)
{
    return s_selftest_result_snapshot_available &&
           s_selftest_result_background_snapshot_valid &&
           s_selftest_result_content_front_index <
               UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT &&
           s_selftest_result_content_snapshot_valid
               [s_selftest_result_content_front_index] &&
           s_selftest_result_front_generation ==
               s_selftest_result_target_generation;
}

static void release_selftest_result_snapshot_cache(void)
{
    if (s_selftest_result_background_snapshot_buffer != NULL)
    {
        heap_caps_free(
            s_selftest_result_background_snapshot_buffer);
        s_selftest_result_background_snapshot_buffer = NULL;
    }
    for (uint8_t index = 0U;
         index < UI_SELFTEST_RESULT_CONTENT_BUFFER_COUNT;
         ++index)
    {
        if (s_selftest_result_content_snapshot_buffers[index] != NULL)
        {
            heap_caps_free(
                s_selftest_result_content_snapshot_buffers[index]);
            s_selftest_result_content_snapshot_buffers[index] = NULL;
        }
    }
    memset(&s_selftest_result_background_snapshot_descriptor,
           0,
           sizeof(s_selftest_result_background_snapshot_descriptor));
    memset(s_selftest_result_content_snapshot_descriptors,
           0,
           sizeof(s_selftest_result_content_snapshot_descriptors));
    memset(s_selftest_result_content_snapshot_valid,
           0,
           sizeof(s_selftest_result_content_snapshot_valid));
    s_selftest_result_background_snapshot_bytes = 0U;
    s_selftest_result_content_snapshot_bytes = 0U;
    s_selftest_result_snapshot_available = false;
    s_selftest_result_background_snapshot_valid = false;
    s_selftest_result_content_front_index =
        UI_SELFTEST_RESULT_CONTENT_INDEX_NONE;
    s_selftest_result_visual_signature_valid = false;
    memset(&s_selftest_result_target_signature,
           0,
           sizeof(s_selftest_result_target_signature));
    s_selftest_result_target_generation = 0U;
    s_selftest_result_front_generation = 0U;
    s_selftest_result_snapshot_active = false;
    s_selftest_result_content_height =
        UI_SELFTEST_RESULT_VIEWPORT_HEIGHT;
    s_selftest_result_scroll_range = 0U;
    s_selftest_result_thumb_height =
        UI_SELFTEST_SCROLL_TRACK_HEIGHT;
}

static ui_selftest_result_visual_signature_t
selftest_result_visual_signature_for_source(
    const watch_selftest_summary_t *source)
{
    ui_selftest_result_visual_signature_t signature = {0};
    ui_selftest_summary_t summary = {0};
    if (source == NULL ||
        !ui_selftest_summary_build(source, &summary))
    {
        return signature;
    }
    for (uint8_t index = 0U;
         index < UI_SELFTEST_CATEGORY_COUNT;
         ++index)
    {
        signature.outcomes[index] =
            summary.categories[index].outcome;
    }
    return signature;
}

static bool selftest_result_visual_signatures_equal(
    const ui_selftest_result_visual_signature_t *left,
    const ui_selftest_result_visual_signature_t *right)
{
    return memcmp(left, right, sizeof(*left)) == 0;
}

static void update_selftest_result_visual_target(
    const watch_selftest_summary_t *source)
{
    const ui_selftest_result_visual_signature_t next =
        selftest_result_visual_signature_for_source(source);
    if (s_selftest_result_visual_signature_valid &&
        selftest_result_visual_signatures_equal(
            &s_selftest_result_target_signature,
            &next))
    {
        ++s_selftest_result_ignored_refresh_count;
        return;
    }
    s_selftest_result_target_signature = next;
    s_selftest_result_visual_signature_valid = true;
    /*
     * 三项汇总计数位于列表外的背景区域，且可由七类 outcome 推导。
     * outcome 变化时必须与后台内容一起重建，禁止新计数搭配旧列表。
     */
    s_selftest_result_background_snapshot_valid = false;
    ++s_selftest_result_target_generation;
    if (s_selftest_result_target_generation == 0U)
    {
        s_selftest_result_target_generation = 1U;
    }
}

static void update_selftest_result_scroll_geometry(void)
{
    if (ui_ui_selftest_result_result_list == NULL ||
        ui_ui_selftest_result_result_scroll_thumb == NULL)
    {
        return;
    }
    lv_obj_update_layout(ui_ui_selftest_result_result_list);
    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_selftest_result_result_list);
    int32_t scroll_bottom =
        lv_obj_get_scroll_bottom(ui_ui_selftest_result_result_list);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    if (scroll_bottom < 0)
    {
        scroll_bottom = 0;
    }
    int32_t scroll_range = scroll_y + scroll_bottom;
    const int32_t max_range =
        UI_SELFTEST_RESULT_CONTENT_MAX_HEIGHT -
        UI_SELFTEST_RESULT_VIEWPORT_HEIGHT;
    if (scroll_range > max_range)
    {
        scroll_range = max_range;
    }
    s_selftest_result_scroll_range = (uint16_t)scroll_range;
    s_selftest_result_content_height = (uint16_t)(
        UI_SELFTEST_RESULT_VIEWPORT_HEIGHT + scroll_range);
    set_flag(ui_ui_selftest_result_result_scroll_track,
             LV_OBJ_FLAG_HIDDEN,
             scroll_range == 0);
    set_flag(ui_ui_selftest_result_result_scroll_thumb,
             LV_OBJ_FLAG_HIDDEN,
             scroll_range == 0);
    int32_t thumb_height =
        s_selftest_result_content_height > 0U
            ? (UI_SELFTEST_SCROLL_TRACK_HEIGHT *
               UI_SELFTEST_RESULT_VIEWPORT_HEIGHT) /
                  s_selftest_result_content_height
            : UI_SELFTEST_SCROLL_TRACK_HEIGHT;
    if (thumb_height < UI_SELFTEST_SCROLL_THUMB_MIN_HEIGHT)
    {
        thumb_height = UI_SELFTEST_SCROLL_THUMB_MIN_HEIGHT;
    }
    if (thumb_height > UI_SELFTEST_SCROLL_TRACK_HEIGHT)
    {
        thumb_height = UI_SELFTEST_SCROLL_TRACK_HEIGHT;
    }
    s_selftest_result_thumb_height = (uint16_t)thumb_height;
    set_height(ui_ui_selftest_result_result_scroll_thumb,
               (lv_coord_t)thumb_height);
    update_selftest_result_scroll_thumb();
}

static void selftest_result_scroll_event_cb(lv_event_t *event)
{
    if (event == NULL ||
        ui_ui_selftest_result_result_list == NULL ||
        lv_event_get_target(event) !=
            ui_ui_selftest_result_result_list)
    {
        return;
    }
    const lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_SCROLL_BEGIN)
    {
        s_selftest_result_fast_path_fault_latched = false;
        if (prepare_selftest_result_snapshots())
        {
            set_selftest_result_snapshot_active(true);
        }
        s_selftest_result_motion_active = true;
        return;
    }
    if (code != LV_EVENT_SCROLL && code != LV_EVENT_SCROLL_END)
    {
        return;
    }
    if (code == LV_EVENT_SCROLL &&
        !s_selftest_result_motion_active &&
        !s_selftest_result_fast_path_fault_latched &&
        shell_touch_sequence_active())
    {
        if (prepare_selftest_result_snapshots())
        {
            set_selftest_result_snapshot_active(true);
        }
        s_selftest_result_motion_active = true;
    }
    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_selftest_result_result_list);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    else if (scroll_y > s_selftest_result_scroll_range)
    {
        scroll_y = s_selftest_result_scroll_range;
    }
    if (lv_obj_get_scroll_y(ui_ui_selftest_result_result_list) !=
        scroll_y)
    {
        lv_obj_scroll_to_y(ui_ui_selftest_result_result_list,
                           (lv_coord_t)scroll_y,
                           LV_ANIM_OFF);
    }
    if (!s_selftest_result_snapshot_active)
    {
        update_selftest_result_scroll_thumb();
    }
    if (code == LV_EVENT_SCROLL_END)
    {
        s_selftest_result_fast_path_fault_latched = false;
        s_selftest_result_motion_active = false;
        update_selftest_result_scroll_thumb();
        set_selftest_result_snapshot_active(false);
    }
}

static void update_selftest_result_scroll_thumb(void)
{
    if (ui_ui_selftest_result_result_list == NULL ||
        ui_ui_selftest_result_result_scroll_thumb == NULL)
    {
        return;
    }
    int32_t scroll_y =
        lv_obj_get_scroll_y(ui_ui_selftest_result_result_list);
    if (scroll_y < 0)
    {
        scroll_y = 0;
    }
    else if (scroll_y > s_selftest_result_scroll_range)
    {
        scroll_y = s_selftest_result_scroll_range;
    }
    const int32_t thumb_range =
        UI_SELFTEST_SCROLL_TRACK_HEIGHT -
        s_selftest_result_thumb_height;
    const int32_t offset =
        s_selftest_result_scroll_range > 0U
            ? (thumb_range * scroll_y) /
                  s_selftest_result_scroll_range
            : 0;
    set_y(ui_ui_selftest_result_result_scroll_thumb,
          (lv_coord_t)(UI_SELFTEST_SCROLL_TRACK_Y + offset));
}

static void set_failed_navigation_state(lv_obj_t *button,
                                        lv_obj_t *icon,
                                        bool enabled)
{
    if (button == NULL)
    {
        return;
    }
    if (enabled)
    {
        set_state(button, LV_STATE_DISABLED, false);
    }
    else
    {
        set_state(button, LV_STATE_DISABLED, true);
    }
    const lv_style_selector_t button_selector =
        LV_PART_MAIN |
        (enabled ? LV_STATE_DEFAULT : LV_STATE_DISABLED);
    set_background_color(button,
                         lv_color_hex(enabled ? 0x16181CU : 0x000000U),
                         button_selector);
    set_background_opa(button,
                       enabled ? LV_OPA_COVER : LV_OPA_TRANSP,
                       button_selector);
    set_border_color(button,
                     lv_color_hex(enabled ? 0x30343BU : 0x24272DU),
                     button_selector);
    set_text_color(icon,
                   lv_color_hex(enabled ? 0xFFFFFFU : 0x484D56U),
                   LV_PART_MAIN | LV_STATE_DEFAULT);
}

static uint8_t failed_item_count(const ui_runtime_model_t *model)
{
    ui_selftest_summary_t summary = {0};
    if (model == NULL ||
        !ui_selftest_summary_build(&model->selftest_results, &summary))
    {
        return 0U;
    }
    return summary.fail_count;
}

static ui_selftest_category_t failed_category_at(
    const ui_runtime_model_t *model,
    uint8_t list_index)
{
    ui_selftest_summary_t summary = {0};
    if (model == NULL ||
        !ui_selftest_summary_build(&model->selftest_results, &summary))
    {
        return UI_SELFTEST_CATEGORY_COUNT;
    }

    uint8_t current = 0U;
    for (ui_selftest_category_t category = UI_SELFTEST_CATEGORY_DISPLAY;
         category < UI_SELFTEST_CATEGORY_COUNT;
         category = (ui_selftest_category_t)(category + 1))
    {
        if (summary.categories[category].outcome != SELFTEST_OUTCOME_FAIL)
        {
            continue;
        }
        if (current == list_index)
        {
            return category;
        }
        ++current;
    }
    return UI_SELFTEST_CATEGORY_COUNT;
}

static selftest_item_id_t failed_item_at(const ui_runtime_model_t *model,
                                         uint8_t list_index)
{
    ui_selftest_summary_t summary = {0};
    const ui_selftest_category_t category =
        failed_category_at(model, list_index);
    if (category >= UI_SELFTEST_CATEGORY_COUNT || model == NULL ||
        !ui_selftest_summary_build(&model->selftest_results, &summary))
    {
        return SELFTEST_ITEM_COUNT;
    }
    return summary.categories[category].source_item_id;
}

static const char *selftest_item_name(selftest_item_id_t item)
{
    static const char *const names[SELFTEST_ITEM_COUNT] = {
        [SELFTEST_ITEM_AMOLED] = "屏幕显示",
        [SELFTEST_ITEM_TOUCH] = "触摸",
        [SELFTEST_ITEM_I2C_SCAN] = "I2C 总线",
        [SELFTEST_ITEM_CW2015] = "电量计",
        [SELFTEST_ITEM_QMI8658C] = "传感器",
        [SELFTEST_ITEM_AUDIO] = "音频播放",
        [SELFTEST_ITEM_GPS_UART] = "GPS 串口",
        [SELFTEST_ITEM_GPS_FIX] = "GPS 定位",
        [SELFTEST_ITEM_ML307R] = "4G 网络",
        [SELFTEST_ITEM_VIBRATION] = "振动反馈",
        [SELFTEST_ITEM_SPIFFS] = "音频资源",
    };
    return item >= SELFTEST_ITEM_AMOLED && item < SELFTEST_ITEM_COUNT
               ? names[item]
               : "未知";
}

static ui_page_id_t selftest_page_for_model(const ui_runtime_model_t *model)
{
    ui_selftest_stage_t stage = UI_SELFTEST_STAGE_IDLE;
    if (model != NULL &&
        (model->selftest_running || model->retry_running))
    {
        stage = (model->selftest_item == SELFTEST_ITEM_AMOLED ||
                 model->selftest_item == SELFTEST_ITEM_TOUCH ||
                 model->selftest_item == SELFTEST_ITEM_AUDIO ||
                 model->selftest_item == SELFTEST_ITEM_VIBRATION)
                    ? UI_SELFTEST_STAGE_MANUAL
                    : UI_SELFTEST_STAGE_RUNNING;
    }
    else if (model != NULL &&
             model->selftest_results.state == SELFTEST_RUN_FINISHED)
    {
        stage = UI_SELFTEST_STAGE_FINISHED;
    }
    return ui_navigation_resolve_selftest(stage);
}

static void advance_selftest_route(const ui_runtime_model_t *model)
{
    const ui_page_id_t target = selftest_page_for_model(model);
    switch (s_current_page)
    {
    case UI_PAGE_SELFTEST_IDLE:
        if (target != UI_PAGE_SELFTEST_IDLE)
        {
            route_to(target);
        }
        break;
    case UI_PAGE_SELFTEST_RUNNING:
    case UI_PAGE_SELFTEST_MANUAL:
        route_to(target);
        break;
    case UI_PAGE_SELFTEST_RESULT:
    case UI_PAGE_SELFTEST_RETRY_DETAIL:
    case UI_PAGE_SELFTEST_GPS_CONTEXT:
        if (target == UI_PAGE_SELFTEST_RUNNING ||
            target == UI_PAGE_SELFTEST_MANUAL)
        {
            route_to(target);
        }
        break;
    default:
        /* 后台自检事实不抢占主壳、设置、维护或 BLE 页面。 */
        break;
    }
}

static void route_to(ui_page_id_t page)
{
    if (page >= UI_PAGE_HOME_READY && page < UI_PAGE_COUNT)
    {
        if (s_current_page == UI_PAGE_SELFTEST_MANUAL &&
            page != UI_PAGE_SELFTEST_MANUAL)
        {
            reset_touch_selftest();
        }
        s_current_page = page;
    }
}

static ui_page_id_t shell_page_for_slot(ui_shell_slot_t slot)
{
    switch (slot)
    {
    case UI_SHELL_SLOT_HOME:
        return UI_PAGE_HOME_READY;
    case UI_SHELL_SLOT_GEAR:
        return UI_PAGE_GEAR_READY;
    case UI_SHELL_SLOT_MODE_POWER:
        return UI_PAGE_MODE_POWER_READY;
    case UI_SHELL_SLOT_ALERTS:
        return UI_PAGE_ALERTS_EMPTY;
    default:
        return UI_PAGE_HOME_READY;
    }
}

static void shell_release_event_cb(lv_event_t *event)
{
    if (event == NULL || lv_event_get_code(event) != LV_EVENT_RELEASED)
    {
        return;
    }
    ui_touch_snapshot_t snapshot = {0};
    if (!ui_touch_session_snapshot(&snapshot) || snapshot.active ||
        snapshot.cancelled ||
        snapshot.start_y > UI_MAIN_SETTINGS_START_MAX_Y)
    {
        return;
    }
    const int32_t delta_x =
        (int32_t)snapshot.end_x - (int32_t)snapshot.start_x;
    const int32_t delta_y =
        (int32_t)snapshot.end_y - (int32_t)snapshot.start_y;
    if (delta_y < UI_EDGE_SWIPE_COMMIT_PX ||
        LV_ABS(delta_y) <= LV_ABS(delta_x) ||
        !ui_touch_session_claim_navigation())
    {
        return;
    }
    ESP_LOGI(TAG,
             "顶部下拉进入设置：序号=%lu，起点=(%d,%d)，位移=(%ld,%ld)",
             (unsigned long)snapshot.sequence_id,
             snapshot.start_x,
             snapshot.start_y,
             (long)delta_x,
             (long)delta_y);
    ui_runtime_binding_emit_intent(UI_INTENT_OPEN_SETTINGS,
                                   0U,
                                   UI_RUNTIME_INTENT_SOURCE_NON_BUTTON);
}

static void settings_release_event_cb(lv_event_t *event)
{
    if (event == NULL || lv_event_get_code(event) != LV_EVENT_RELEASED)
    {
        return;
    }
    ui_touch_snapshot_t snapshot = {0};
    if (!ui_touch_session_snapshot(&snapshot) || snapshot.active ||
        snapshot.cancelled ||
        snapshot.start_y < UI_SETTINGS_RETURN_START_MIN_Y)
    {
        return;
    }
    const int32_t delta_x =
        (int32_t)snapshot.end_x - (int32_t)snapshot.start_x;
    const int32_t delta_y =
        (int32_t)snapshot.end_y - (int32_t)snapshot.start_y;
    if (delta_y > -UI_EDGE_SWIPE_COMMIT_PX ||
        LV_ABS(delta_y) <= LV_ABS(delta_x) ||
        !ui_touch_session_claim_navigation())
    {
        return;
    }
    ESP_LOGI(TAG,
             "底部上滑返回主壳：序号=%lu，起点=(%d,%d)，位移=(%ld,%ld)",
             (unsigned long)snapshot.sequence_id,
             snapshot.start_x,
             snapshot.start_y,
             (long)delta_x,
             (long)delta_y);
    ui_runtime_binding_emit_intent(UI_INTENT_BACK_TO_SHELL,
                                   0U,
                                   UI_RUNTIME_INTENT_SOURCE_NON_BUTTON);
}

static void maintenance_gps_event_cb(lv_event_t *event)
{
    if (event == NULL || lv_event_get_code(event) != LV_EVENT_CLICKED)
    {
        return;
    }
    ui_runtime_binding_emit_intent(UI_INTENT_REQUEST_GPS_ACQUISITION,
                                   0U,
                                   UI_RUNTIME_INTENT_SOURCE_BUTTON);
}

static void shell_scroll_end_event_cb(lv_event_t *event)
{
    if (event == NULL || ui_ui_main_shell_pager == NULL ||
        lv_event_get_target(event) != ui_ui_main_shell_pager)
    {
        return;
    }
    const lv_event_code_t code = lv_event_get_code(event);
    const lv_coord_t scroll_x = lv_obj_get_scroll_x(ui_ui_main_shell_pager);
    if (code == LV_EVENT_SCROLL_BEGIN)
    {
        s_shell_fast_path_fault_latched = false;
        if (!shell_touch_sequence_active())
        {
            return;
        }
        if (prepare_shell_snapshots())
        {
            set_shell_snapshot_active(true);
        }
        s_shell_motion_active = true;
        return;
    }
    if (code == LV_EVENT_SCROLL)
    {
        if (!s_shell_motion_active)
        {
            if (s_shell_fast_path_fault_latched ||
                !shell_touch_sequence_active())
            {
                return;
            }
            if (prepare_shell_snapshots())
            {
                set_shell_snapshot_active(true);
            }
            s_shell_motion_active = true;
        }
        const lv_coord_t delta = scroll_x - UI_SHELL_PAGE_WIDTH;
        if (LV_ABS(delta) > LV_ABS(s_shell_scroll_peak_delta))
        {
            s_shell_scroll_peak_delta = delta;
        }
        return;
    }
    if (code != LV_EVENT_SCROLL_END)
    {
        return;
    }
    s_shell_fast_path_fault_latched = false;
    if (!s_shell_motion_active)
    {
        s_shell_scroll_peak_delta = 0;
        return;
    }
    s_shell_motion_active = false;
    set_shell_snapshot_active(false);

    ui_shell_slot_t target_slot = s_shell_slot;
    if (s_shell_scroll_peak_delta >= UI_SHELL_SWIPE_TRIGGER_PX)
    {
        target_slot =
            (ui_shell_slot_t)((s_shell_slot + 1U) %
                              UI_SHELL_SLOT_COUNT);
    }
    else if (s_shell_scroll_peak_delta <= -UI_SHELL_SWIPE_TRIGGER_PX)
    {
        target_slot = (ui_shell_slot_t)(
            (s_shell_slot + UI_SHELL_SLOT_COUNT - 1U) %
            UI_SHELL_SLOT_COUNT);
    }
    s_shell_scroll_peak_delta = 0;
    if (target_slot == s_shell_slot ||
        !ui_touch_session_claim_navigation())
    {
        return;
    }

    static const ui_runtime_intent_type_t intents[UI_SHELL_SLOT_COUNT] = {
        [UI_SHELL_SLOT_HOME] = UI_INTENT_SHELL_HOME,
        [UI_SHELL_SLOT_GEAR] = UI_INTENT_SHELL_CONTROL,
        [UI_SHELL_SLOT_MODE_POWER] = UI_INTENT_SHELL_MODE_POWER,
        [UI_SHELL_SLOT_ALERTS] = UI_INTENT_SHELL_ALERTS,
    };
    ui_runtime_binding_emit_intent(intents[target_slot],
                                   0U,
                                   UI_RUNTIME_INTENT_SOURCE_NON_BUTTON);
}

static void route_back(void)
{
    switch (s_current_page)
    {
    case UI_PAGE_SETTINGS:
        route_to(s_return_page);
        break;
    case UI_PAGE_DEVICE_INFO:
    case UI_PAGE_MAINTENANCE:
        route_to(UI_PAGE_SETTINGS);
        break;
    case UI_PAGE_BLE_EMPTY:
    case UI_PAGE_BLE_CANDIDATES:
        route_to(s_return_page);
        break;
    case UI_PAGE_SELFTEST_IDLE:
    case UI_PAGE_SELFTEST_RUNNING:
    case UI_PAGE_SELFTEST_MANUAL:
    case UI_PAGE_SELFTEST_RESULT:
        route_to(UI_PAGE_MAINTENANCE);
        break;
    case UI_PAGE_SELFTEST_GPS_CONTEXT:
        s_pending_retry_item = SELFTEST_ITEM_COUNT;
        route_to(s_gps_return_page);
        break;
    case UI_PAGE_SELFTEST_RETRY_DETAIL:
        s_pending_retry_item = SELFTEST_ITEM_COUNT;
        route_to(UI_PAGE_SELFTEST_RESULT);
        break;
    case UI_PAGE_CLEAR_BINDING_CONFIRM:
        hide_clear_binding_modal();
        route_to(UI_PAGE_MAINTENANCE);
        break;
    default:
        route_to(UI_PAGE_HOME_READY);
        break;
    }
}

static void hide_clear_binding_modal(void)
{
    ui_ui_clear_binding_modal_destroy();
}

static void show_clear_binding_modal(const ui_runtime_model_t *model)
{
    if (ui_ui_modal_clear_binding_root == NULL)
    {
        ui_ui_clear_binding_modal_create(lv_layer_top());
        if (ui_ui_modal_clear_binding_root == NULL)
        {
            return;
        }
    }
    set_flag(ui_ui_modal_clear_binding_root,
             LV_OBJ_FLAG_HIDDEN,
             false);
    if (model->clear_binding_failed)
    {
        set_label(ui_ui_clear_binding_confirm_txt_confirm_title,
                  "清除失败");
        set_label(ui_ui_clear_binding_confirm_txt_confirm_effect,
                  "未能清除绑定，请重试。");
        set_label(ui_ui_clear_binding_confirm_txt_confirm_rebind,
                  "当前绑定仍然保留。");
        set_label(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label,
                  "重试清除");
    }
    else if (model->clear_binding_pending)
    {
        set_label(ui_ui_clear_binding_confirm_txt_confirm_title,
                  "正在清除");
        set_label(ui_ui_clear_binding_confirm_txt_confirm_effect,
                  "正在清除绑定，请稍候。");
        set_label(ui_ui_clear_binding_confirm_txt_confirm_rebind,
                  "完成前不可重复提交。");
        set_label(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label,
                  "清除中");
    }
    else
    {
        set_label(ui_ui_clear_binding_confirm_txt_confirm_title,
                  "清除绑定？");
        set_label(ui_ui_clear_binding_confirm_txt_confirm_effect,
                  "将清除绑定设备和控制资格。");
        set_label(ui_ui_clear_binding_confirm_txt_confirm_rebind,
                  "下次使用必须重新授权。");
        set_label(ui_ui_clear_binding_confirm_btn_confirm_clear_binding_label,
                  "清除");
    }
    lv_obj_t *controls[] = {
        ui_ui_clear_binding_confirm_btn_confirm_clear_binding,
        ui_ui_clear_binding_confirm_btn_cancel,
    };
    for (size_t index = 0; index < sizeof(controls) / sizeof(controls[0]); ++index)
    {
        if (controls[index] == NULL)
        {
            continue;
        }
        set_state(controls[index],
                  LV_STATE_DISABLED,
                  !model->binding_present ||
                      model->clear_binding_pending);
    }
}

static bool is_settings_hierarchy_page(ui_page_id_t page)
{
    switch (page)
    {
    case UI_PAGE_SETTINGS:
    case UI_PAGE_DEVICE_INFO:
    case UI_PAGE_MAINTENANCE:
    case UI_PAGE_SELFTEST_IDLE:
    case UI_PAGE_SELFTEST_RUNNING:
    case UI_PAGE_SELFTEST_MANUAL:
    case UI_PAGE_SELFTEST_RESULT:
    case UI_PAGE_SELFTEST_GPS_CONTEXT:
    case UI_PAGE_SELFTEST_RETRY_DETAIL:
    case UI_PAGE_CLEAR_BINDING_CONFIRM:
        return true;
    default:
        return false;
    }
}

static bool payment_lock_allows_intent(ui_runtime_intent_type_t type)
{
    if (type == UI_INTENT_OPEN_SETTINGS)
    {
        return true;
    }
    if (!is_settings_hierarchy_page(s_current_page))
    {
        return type == UI_INTENT_VERIFY_PAYMENT;
    }

    switch (type)
    {
    case UI_INTENT_BACK_TO_SHELL:
    case UI_INTENT_TOGGLE_HAPTICS:
    case UI_INTENT_TOGGLE_CLICK_AUDIO:
    case UI_INTENT_TOGGLE_RAISE_WAKE:
    case UI_INTENT_SET_BRIGHTNESS:
    case UI_INTENT_SET_SCREEN_TIMEOUT:
    case UI_INTENT_OPEN_DEVICE_INFO:
    case UI_INTENT_OPEN_MAINTENANCE:
    case UI_INTENT_CONNECT_CELLULAR:
    case UI_INTENT_REQUEST_GPS_ACQUISITION:
    case UI_INTENT_BACK:
    case UI_INTENT_OPEN_SELFTEST:
    case UI_INTENT_OPEN_CLEAR_BINDING:
    case UI_INTENT_CONFIRM_CLEAR_BINDING:
    case UI_INTENT_CANCEL_CLEAR_BINDING:
    case UI_INTENT_START_SELFTEST:
    case UI_INTENT_LEAVE_SELFTEST:
    case UI_INTENT_SELFTEST_MANUAL_FAIL:
    case UI_INTENT_SELFTEST_MANUAL_PASS:
    case UI_INTENT_SELFTEST_TOUCH_TARGET:
    case UI_INTENT_SELFTEST_TOUCH_PROGRESS:
    case UI_INTENT_SELFTEST_OUTPUT_REPLAY:
    case UI_INTENT_OPEN_RETRY_FAILED:
    case UI_INTENT_RERUN_SELFTEST:
    case UI_INTENT_GPS_OUTDOOR:
    case UI_INTENT_GPS_INDOOR:
    case UI_INTENT_FAILED_PREVIOUS:
    case UI_INTENT_FAILED_NEXT:
    case UI_INTENT_RETRY_FAILED_ITEM:
        return true;
    default:
        return false;
    }
}

static bool control_intent_allowed(const ui_runtime_model_t *model)
{
    return model->binding_present && model->ble_connected &&
           model->controls_enabled && !model->selftest_running &&
           !model->payment_required && !model->poweroff_committed;
}

static bool button_intent_blocked_by_gesture(void)
{
    lv_indev_t *input = lv_indev_get_act();
    const bool lvgl_gesture_active =
        input != NULL && lv_indev_get_type(input) == LV_INDEV_TYPE_POINTER &&
        lv_indev_get_gesture_dir(input) != LV_DIR_NONE;
    return lvgl_gesture_active ||
           !ui_touch_session_button_tap_allowed();
}

static void touch_target_event_cb(lv_event_t *event)
{
    lv_indev_t *input = lv_event_get_indev(event);
    if (input == NULL)
    {
        return;
    }
    lv_point_t point = {0};
    lv_indev_get_point(input, &point);
    const uint32_t packed = ((uint32_t)(uint16_t)point.x << 16U) |
                            (uint32_t)(uint16_t)point.y;
    ui_runtime_binding_emit_intent(UI_INTENT_SELFTEST_TOUCH_TARGET,
                                   packed,
                                   UI_RUNTIME_INTENT_SOURCE_NON_BUTTON);
}

static void touch_slider_event_cb(lv_event_t *event)
{
    lv_indev_t *input = lv_event_get_indev(event);
    lv_obj_t *slider = lv_event_get_target(event);
    if (input == NULL || slider == NULL)
    {
        return;
    }
    lv_point_t point = {0};
    lv_area_t area = {0};
    lv_indev_get_point(input, &point);
    lv_obj_get_coords(slider, &area);
    const int32_t width = (int32_t)area.x2 - (int32_t)area.x1;
    uint32_t progress = 0U;
    if (width > 0 && point.x > area.x1)
    {
        const int32_t offset = point.x >= area.x2
                                   ? width
                                   : (int32_t)point.x - (int32_t)area.x1;
        progress = (uint32_t)((offset * 100) / width);
    }
    (void)update_touch_progress_local(
        progress > 100U ? 100U : (uint8_t)progress);
    ui_runtime_binding_emit_intent(UI_INTENT_SELFTEST_TOUCH_PROGRESS,
                                   progress,
                                   UI_RUNTIME_INTENT_SOURCE_NON_BUTTON);
}

static void output_replay_event_cb(lv_event_t *event)
{
    (void)event;
    ui_runtime_binding_emit_intent(UI_INTENT_SELFTEST_OUTPUT_REPLAY,
                                   0U,
                                   UI_RUNTIME_INTENT_SOURCE_BUTTON);
}

static void amoled_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    ++s_amoled_color_index;
    if (s_amoled_color_index >= UI_AMOLED_COLOR_COUNT)
    {
        ui_runtime_binding_stop_amoled_test();
        return;
    }
    set_amoled_color(s_amoled_color_index);
}

static void set_amoled_color(uint8_t index)
{
    static const uint32_t colors[UI_AMOLED_COLOR_COUNT] = {
        0x000000U,
        0xFFFFFFU,
        0xFF0000U,
        0x00FF00U,
        0x0000FFU,
    };
    if (s_amoled_surface != NULL && index < UI_AMOLED_COLOR_COUNT)
    {
        lv_obj_set_style_bg_color(s_amoled_surface,
                                  lv_color_hex(colors[index]),
                                  0);
        lv_obj_set_style_bg_opa(s_amoled_surface, LV_OPA_COVER, 0);
    }
}
