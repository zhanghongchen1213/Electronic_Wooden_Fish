#include "cst9217_bsp.h"

#include <stddef.h>

#define HOST_CST9217_POINT_QUEUE 8U

static cst9217_bsp_interrupt_cb_t s_callback;
static cst9217_bsp_point_t s_points[HOST_CST9217_POINT_QUEUE];
static size_t s_head;
static size_t s_tail;
static size_t s_count;
static cst9217_bsp_point_t s_last;

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
    if (s_count == 0U)
    {
        *point = s_last;
        point->pressed = false;
        return ESP_OK;
    }
    *point = s_points[s_head];
    s_last = *point;
    s_head = (s_head + 1U) % HOST_CST9217_POINT_QUEUE;
    --s_count;
    return ESP_OK;
}

void host_cst9217_set_point(uint16_t x, uint16_t y, bool pressed)
{
    if (s_count >= HOST_CST9217_POINT_QUEUE)
    {
        s_head = (s_head + 1U) % HOST_CST9217_POINT_QUEUE;
        --s_count;
    }
    s_points[s_tail] = (cst9217_bsp_point_t){
        .x = x,
        .y = y,
        .pressed = pressed,
    };
    s_tail = (s_tail + 1U) % HOST_CST9217_POINT_QUEUE;
    ++s_count;
}

void host_cst9217_reset_points(void)
{
    s_head = 0U;
    s_tail = 0U;
    s_count = 0U;
    s_last = (cst9217_bsp_point_t){0};
}

void host_cst9217_interrupt(void)
{
    if (s_callback != NULL)
    {
        s_callback(NULL);
    }
}
