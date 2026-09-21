/**
 * @file     ui_navigation.h
 * @brief    最终腕表页面与跳转状态机接口
 * @details  把 watch-lvgl.pen 的 23 个 UI 页面、四页日常主壳和层级返回规则固化为不依赖 LVGL 的纯路由合同。
 * @author   ZHC
 * @date     2026-07-15
 */

#ifndef LEGBOT_UI_NAVIGATION_H
#define LEGBOT_UI_NAVIGATION_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Pencil 最终设计中的页面与状态页 ID。 */
    typedef enum
    {
        UI_PAGE_HOME_READY = 0,              /**< 01-01 主表盘正常态。 */
        UI_PAGE_HOME_UNBOUND,                /**< 01-02 主表盘未绑定态。 */
        UI_PAGE_HOME_DISCONNECTED,           /**< 01-03 主表盘 BLE 物理断连态。 */
        UI_PAGE_GEAR_READY,                  /**< 02-01 档位控制可用态。 */
        UI_PAGE_GEAR_UNAVAILABLE,            /**< 02-02 档位控制不可用态。 */
        UI_PAGE_MODE_POWER_READY,            /**< 03-01 模式与电源可用态。 */
        UI_PAGE_MODE_POWER_UNAVAILABLE,      /**< 03-02 模式与电源不可用态。 */
        UI_PAGE_ALERTS_EMPTY,                /**< 04-01 暂无提醒。 */
        UI_PAGE_ALERTS_READ_ONLY,            /**< 04-02 只读提醒。 */
        UI_PAGE_ALERTS_PAYMENT_VERIFY,       /**< 04-03 支付验证。 */
        UI_PAGE_ALERTS_VERIFY_FAILED,        /**< 04-04 验证失败。 */
        UI_PAGE_SETTINGS,                    /**< 05-01 设置。 */
        UI_PAGE_DEVICE_INFO,                 /**< 05-02 设备信息。 */
        UI_PAGE_MAINTENANCE,                 /**< 06-01 维护。 */
        UI_PAGE_BLE_EMPTY,                   /**< 07-01 BLE 未发现设备。 */
        UI_PAGE_BLE_CANDIDATES,              /**< 07-02 BLE 候选设备。 */
        UI_PAGE_SELFTEST_IDLE,               /**< 08-01 自检未开始。 */
        UI_PAGE_SELFTEST_RUNNING,            /**< 08-02 自检运行中。 */
        UI_PAGE_SELFTEST_MANUAL,             /**< 08-03 自检人工确认。 */
        UI_PAGE_SELFTEST_RESULT,             /**< 08-04 自检结果。 */
        UI_PAGE_SELFTEST_GPS_CONTEXT,        /**< 08-05 GPS 环境确认。 */
        UI_PAGE_SELFTEST_RETRY_DETAIL,       /**< 08-06 失败项详情与重试。 */
        UI_PAGE_CLEAR_BINDING_CONFIRM,       /**< 09-01 清除绑定确认。 */
        UI_PAGE_COUNT                        /**< 最终设计页面总数。 */
    } ui_page_id_t;

    /** 四页日常主壳固定槽位。 */
    typedef enum
    {
        UI_SHELL_SLOT_HOME = 0,       /**< 主表盘槽位。 */
        UI_SHELL_SLOT_GEAR,           /**< 档位控制槽位。 */
        UI_SHELL_SLOT_MODE_POWER,     /**< 模式与电源槽位。 */
        UI_SHELL_SLOT_ALERTS,         /**< 提醒槽位。 */
        UI_SHELL_SLOT_COUNT           /**< 日常主壳槽位数量。 */
    } ui_shell_slot_t;

    /** 自检维护入口需要区分的 UI 阶段。 */
    typedef enum
    {
        UI_SELFTEST_STAGE_IDLE = 0, /**< 尚无运行。 */
        UI_SELFTEST_STAGE_RUNNING,  /**< 后台运行中。 */
        UI_SELFTEST_STAGE_MANUAL,   /**< 等待人工确认。 */
        UI_SELFTEST_STAGE_FINISHED  /**< 已完成且存在结果。 */
    } ui_selftest_stage_t;

    /** 页面变体解析所需的最小产品事实。 */
    typedef struct
    {
        bool has_binding;                   /**< 是否存在绑定外骨骼。 */
        bool ble_connected;                 /**< BLE 物理链路是否已连接，用于 01-03、02-02、03-02 页面身份。 */
        bool link_control_ready;            /**< BLE 链路与状态新鲜度是否允许提交控制。 */
        bool unlock_session_valid;          /**< 当前外骨骼会话是否已解锁。 */
        bool payment_required;              /**< 是否因未支付而阻塞。 */
        bool payment_verification_attempted;/**< 用户是否已主动执行过本轮支付验证。 */
        bool payment_verification_failed;   /**< 最近一次支付验证是否失败。 */
        uint8_t reminder_count;             /**< 当前非阻断提醒数量。 */
    } ui_navigation_facts_t;

    /**
     * @brief 按当前产品事实解析日常主壳槽位的具体页面变体
     * @param slot 四页主壳槽位
     * @param facts 当前不可变产品事实
     * @return 对应设计页面；参数无效时返回 UI_PAGE_HOME_DISCONNECTED
     */
    ui_page_id_t ui_navigation_resolve_shell(
        ui_shell_slot_t slot,
        const ui_navigation_facts_t *facts);

    /**
     * @brief 解析 06-01 唯一“整机自检”行的目标页面
     * @param stage 当前自检 UI 阶段
     * @return 08-01、08-02、08-03 或 08-04
     */
    ui_page_id_t ui_navigation_resolve_selftest(ui_selftest_stage_t stage);

    /**
     * @brief 判断页面是否属于四页日常主壳
     * @param page 页面 ID
     * @return true 属于 01 至 04 槽位，false 为按需页面
     */
    bool ui_navigation_is_shell_page(ui_page_id_t page);

    /**
     * @brief 返回页面对应的 Pencil 稳定编号
     * @param page 页面 ID
     * @return 形如“01-01”的稳定编号；参数无效时返回“invalid”
     */
    const char *ui_navigation_design_id(ui_page_id_t page);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_NAVIGATION_H */
