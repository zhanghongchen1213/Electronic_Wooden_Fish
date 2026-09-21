/**
 * @file     ui_selftest_summary.h
 * @brief    十一项内部自检到七类 UI 结果的聚合接口
 * @details  保留完整硬件自检覆盖，只把结果页需要的显示、触摸、供电、传感、音频、定位和 4G 七类投影给 UI。
 * @author   ZHC
 * @date     2026-07-24
 */

#ifndef LEGBOT_UI_SELFTEST_SUMMARY_H
#define LEGBOT_UI_SELFTEST_SUMMARY_H

#include <stdbool.h>
#include <stdint.h>

#include "app_state.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** 最终设计固定展示的七个自检类别。 */
    typedef enum
    {
        UI_SELFTEST_CATEGORY_DISPLAY = 0, /**< AMOLED 显示。 */
        UI_SELFTEST_CATEGORY_TOUCH,       /**< 触摸输入。 */
        UI_SELFTEST_CATEGORY_POWER,       /**< I2C 基础与电量计。 */
        UI_SELFTEST_CATEGORY_SENSOR,      /**< IMU 与振动反馈。 */
        UI_SELFTEST_CATEGORY_AUDIO,       /**< SPIFFS 资源与音频播放。 */
        UI_SELFTEST_CATEGORY_GPS,         /**< GPS UART 与定位。 */
        UI_SELFTEST_CATEGORY_CELLULAR,    /**< ML307R 4G。 */
        UI_SELFTEST_CATEGORY_COUNT        /**< UI 自检类别数量。 */
    } ui_selftest_category_t;

    /** 单个 UI 类别的聚合结果。 */
    typedef struct
    {
        selftest_outcome_t outcome;                      /**< 类别聚合终态。 */
        selftest_reason_t reason;                        /**< 最高优先级原因。 */
        selftest_item_id_t source_item_id;               /**< 提供当前聚合终态的内部项目。 */
        uint32_t elapsed_ms;                             /**< 类别内部项累计耗时。 */
        char detail_code[SELFTEST_DETAIL_CODE_CAPACITY]; /**< 最高优先级稳定码。 */
    } ui_selftest_category_result_t;

    /** 七类 UI 自检摘要。 */
    typedef struct
    {
        ui_selftest_category_result_t categories[UI_SELFTEST_CATEGORY_COUNT]; /**< 固定类别结果。 */
        uint8_t pass_count;      /**< 通过类别数。 */
        uint8_t fail_count;      /**< 失败类别数。 */
        uint8_t skip_count;      /**< 跳过类别数。 */
        uint8_t inconclusive_count; /**< 未完成、需复测类别数。 */
        uint8_t completed_count; /**< 已完成类别数。 */
    } ui_selftest_summary_t;

    /**
     * @brief 把十一项产品自检摘要聚合为七类 UI 摘要
     * @param source watch_state 中的完整内部自检结果
     * @param summary 完整覆盖写入的 UI 摘要
     * @return true 聚合完成，false 表示参数或内部 item_id 无效
     */
    bool ui_selftest_summary_build(const watch_selftest_summary_t *source,
                                   ui_selftest_summary_t *summary);

    /**
     * @brief 返回自检类别中文名称
     * @param category 自检类别
     * @return 稳定中文名称；参数无效时返回“未知”
     */
    const char *ui_selftest_category_name(ui_selftest_category_t category);

    /**
     * @brief 返回内部自检项目所属的七类 UI 类别
     * @param item 内部自检项目
     * @return 对应 UI 类别；参数无效时返回 UI_SELFTEST_CATEGORY_COUNT
     */
    ui_selftest_category_t ui_selftest_category_for_item(
        selftest_item_id_t item);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_UI_SELFTEST_SUMMARY_H */
