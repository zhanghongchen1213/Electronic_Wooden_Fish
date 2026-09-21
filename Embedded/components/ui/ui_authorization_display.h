/**
 * @file     ui_authorization_display.h
 * @brief    定义景区模式授权阻断的单值显示状态
 * @details  供服务层授权投影与 UI 运行时共享，避免多个布尔值组合出矛盾页面。
 * @author   ZHC
 * @date     2026-08-18
 */

#ifndef LEGBOT_UI_AUTHORIZATION_DISPLAY_H
#define LEGBOT_UI_AUTHORIZATION_DISPLAY_H

/** 景区模式授权阻断的规范显示状态。 */
typedef enum
{
    UI_AUTHORIZATION_DISPLAY_NONE = 0, /**< 不显示授权阻断页面。 */
    UI_AUTHORIZATION_DISPLAY_CHECKING, /**< 正在联网并检查当前会话授权。 */
    UI_AUTHORIZATION_DISPLAY_UNPAID,   /**< 当前会话已确认未支付。 */
    UI_AUTHORIZATION_DISPLAY_FAILED,   /**< 当前会话验证失败。 */
} ui_authorization_display_t;

#endif /* LEGBOT_UI_AUTHORIZATION_DISPLAY_H */
