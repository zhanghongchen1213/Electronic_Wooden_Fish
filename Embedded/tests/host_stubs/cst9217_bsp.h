#ifndef EWF_HOST_CST9217_BSP_H
#define EWF_HOST_CST9217_BSP_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t strength;
    bool pressed;
} cst9217_bsp_point_t;

typedef void (*cst9217_bsp_interrupt_cb_t)(void *user_ctx);

esp_err_t cst9217_bsp_register_interrupt_callback(
    cst9217_bsp_interrupt_cb_t callback,
    void *user_ctx);
esp_err_t cst9217_bsp_read_point(cst9217_bsp_point_t *point);

#endif
