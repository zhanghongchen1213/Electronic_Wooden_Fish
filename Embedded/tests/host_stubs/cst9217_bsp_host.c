#include "cst9217_bsp.h"

#include <stddef.h>

static cst9217_bsp_interrupt_cb_t s_callback;
static cst9217_bsp_point_t s_point;

esp_err_t cst9217_bsp_register_interrupt_callback(
    cst9217_bsp_interrupt_cb_t callback,
    void *user_ctx)
{
    (void)user_ctx;
    s_callback = callback;
    return ESP_OK;
}

esp_err_t cst9217_bsp_read_point(cst9217_bsp_point_t *point)
{
    if (point == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    *point = s_point;
    s_point.pressed = false;
    return ESP_OK;
}

void host_cst9217_set_point(uint16_t x, uint16_t y, bool pressed)
{
    s_point = (cst9217_bsp_point_t){
        .x = x,
        .y = y,
        .pressed = pressed,
    };
}

void host_cst9217_interrupt(void)
{
    if (s_callback != NULL)
    {
        s_callback(NULL);
    }
}
