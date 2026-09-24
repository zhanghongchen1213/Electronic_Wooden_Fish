/**
 * @file     esp_timer.h
 * @brief    主机测试用 esp_timer 替身声明。
 */
#ifndef EWF_HOST_ESP_TIMER_H
#define EWF_HOST_ESP_TIMER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int64_t esp_timer_get_time(void);

#ifdef __cplusplus
}
#endif

#endif
