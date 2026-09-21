/**
 * @file     ui_display_flush_guard.h
 * @brief    定义 UI 显示颜色事务代次与超时守卫
 * @details  为 LVGL 普通 flush 绑定提交代次，隔离快路遗留通知，并提供不依赖硬件的超时判定。
 * @author   ZHC
 * @date     2026-07-28
 */

#ifndef LEGBOT_UI_DISPLAY_FLUSH_GUARD_H
#define LEGBOT_UI_DISPLAY_FLUSH_GUARD_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint32_t submitted_generation; /**< 当前普通 flush 提交前的颜色完成代次。 */
        int64_t started_us;             /**< 当前普通 flush 的单调提交微秒。 */
        bool active;                    /**< 是否存在等待确认的普通 flush。 */
    } ui_display_flush_guard_t;

    /**
     * @brief 初始化显示 flush 守卫
     * @param guard 守卫实例
     */
    void ui_display_flush_guard_init(ui_display_flush_guard_t *guard);

    /**
     * @brief 绑定一笔新提交的普通颜色事务
     * @param guard 守卫实例
     * @param submitted_generation 提交前读取的颜色完成代次
     * @param started_us 提交时的单调微秒
     */
    void ui_display_flush_guard_begin(ui_display_flush_guard_t *guard,
                                      uint32_t submitted_generation,
                                      int64_t started_us);

    /**
     * @brief 判断当前颜色完成代次是否已越过本笔提交代次
     * @param guard 守卫实例
     * @param current_generation 当前颜色完成代次
     * @return true 当前事务已有匹配完成证据，false 仅有旧通知或尚未完成
     */
    bool ui_display_flush_guard_completed(
        const ui_display_flush_guard_t *guard,
        uint32_t current_generation);

    /**
     * @brief 判断当前普通 flush 是否已超过有界等待预算
     * @param guard 守卫实例
     * @param now_us 当前单调微秒
     * @param timeout_us 等待预算微秒
     * @return true 已超时，false 尚未超时、未激活或时钟倒退
     */
    bool ui_display_flush_guard_timed_out(
        const ui_display_flush_guard_t *guard,
        int64_t now_us,
        uint32_t timeout_us);

    /**
     * @brief 从当前时刻重新计算下一次恢复尝试的超时窗口
     * @param guard 守卫实例
     * @param now_us 当前单调微秒
     */
    void ui_display_flush_guard_rearm_timeout(
        ui_display_flush_guard_t *guard,
        int64_t now_us);

    /**
     * @brief 清除当前普通 flush 等待状态
     * @param guard 守卫实例
     */
    void ui_display_flush_guard_clear(ui_display_flush_guard_t *guard);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_DISPLAY_FLUSH_GUARD_H */
