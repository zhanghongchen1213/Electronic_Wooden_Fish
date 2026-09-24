/**
 * @file     co5300_bsp.h
 * @brief    主机测试用 CO5300 BSP 声明。
 * @author   ZHC
 * @date     2026-09-24
 */

#ifndef CO5300_BSP_H
#define CO5300_BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t co5300_bsp_set_display(bool on);
esp_err_t co5300_bsp_set_brightness(uint16_t raw_level);

#ifdef __cplusplus
}
#endif

#endif
