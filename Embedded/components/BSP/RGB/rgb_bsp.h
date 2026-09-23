/**
 * @file     rgb_bsp.h
 * @brief    RGB 状态灯（WS2812 类单线器件）BSP 接口
 * @details  上游依赖 espressif/led_strip（RMT 后端）经组件管理器引入，本 BSP
 *           封装稳定 API（init/设色/熄灭/错误码映射，AD-8）：业务模块不得直接
 *           操作 RMT/GPIO。RGB_DATA=IO3 为 strap/JTAG 源选择相关脚，初始化须
 *           保证复位后确定电平（空闲输出低），不得改变 BOOT0/下载流程行为。
 * @author   ZHC
 * @date     2026-09-23
 */

#ifndef LEGBOT_RGB_BSP_H
#define LEGBOT_RGB_BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief RGB 数据引脚编号（权威事实源：电子木鱼-硬件网络清单.json RGB_DATA=IO3）
 * @details 以无驱动依赖的整型 ID 声明，保证反馈服务与主机测试可编译；
 *          contract_checks.c 静态断言其与 BSP 权威资源表常量一致。
 */
#define RGB_BSP_DATA_GPIO_ID 3
/** 状态灯颗数：单颗用户状态灯，不作调试灯。 */
#define RGB_BSP_LED_COUNT 1U
/**
 * @brief 有效敲击灯效默认琥珀色 R 分量
 * @details 对齐 DESIGN.md `{colors.brand.amber.300}` ≈ #e6bd69 的品牌语义；
 *          颜色/亮度/时长属表现类参数，待样机定标，不建立第二套 UX 令牌真源。
 */
#define RGB_BSP_AMBER_R 230U
/** 琥珀色 G 分量。 */
#define RGB_BSP_AMBER_G 189U
/** 琥珀色 B 分量。 */
#define RGB_BSP_AMBER_B 105U
/**
 * @brief 灯效默认亮度百分比（表现类参数，待样机定标）
 * @details 高速连击期间由反馈服务节流，不逐事件全亮度闪烁堆叠（AC 3）。
 */
#define RGB_BSP_TAP_BRIGHTNESS_PERCENT 40U
/**
 * @brief 单次短闪时长（毫秒，表现类参数，待样机定标）
 * @details 反馈服务任务在此时长内有界阻塞；反馈是可合并型通道，不阻塞核心链路。
 */
#define RGB_BSP_TAP_FLASH_MS 60U

    /**
     * @brief 初始化 RGB 状态灯（幂等）
     * @details 先把 RGB_DATA 配置为输出低（strap 确定电平约束，注释见
     *           bsp_resources.h），再创建 led_strip RMT 设备并熄灭显示。
     * @return ESP_OK 成功或已初始化，其他值表示 GPIO/RMT 初始化失败
     */
    esp_err_t rgb_bsp_init(void);

    /**
     * @brief 按默认琥珀色与默认亮度执行一次有效敲击短闪
     * @details 设色 → 有界阻塞短闪 → 熄灭；调用方为反馈服务任务。
     * @return ESP_OK 成功，其他值表示设色或刷新失败
     */
    esp_err_t rgb_bsp_tap_flash(void);

    /**
     * @brief 熄灭状态灯（幂等）
     * @return ESP_OK 成功，其他值表示刷新失败
     */
    esp_err_t rgb_bsp_off(void);

    /**
     * @brief 查询 RGB 状态灯是否已完成初始化
     * @return true 已就绪，false 尚未就绪
     */
    bool rgb_bsp_is_ready(void);

    /**
     * @brief 将 RGB 操作结果映射为稳定驱动错误码
     * @param err ESP-IDF 操作结果
     * @return DRV_RGB_ 前缀的静态字符串
     */
    const char *rgb_bsp_error_code(esp_err_t err);

#ifdef __cplusplus
}
#endif

#endif /* LEGBOT_RGB_BSP_H */
