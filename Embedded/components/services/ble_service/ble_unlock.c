/**
 * @file     ble_unlock.c
 * @brief    BLE 一次性解锁兼容入口与状态证据实现
 * @details  委托共享完整包 builder，并以 request id、规范 MAC 和链路代次隔离迟到证据。
 * @author   ZHC
 * @date     2026-07-13
 */

#include "ble_unlock.h"

#include <string.h>

static bool request_is_valid(const ble_unlock_request_t *request);
static bool request_matches(const ble_unlock_context_t *context,
                            const ble_unlock_request_t *request);
static bool status_sequence_is_after(uint32_t candidate, uint32_t baseline);

bool ble_unlock_build_packet(const ble_status_frame_t *mirror,
                             uint8_t packet[BLE_CONTROL_PACKET_LENGTH])
{
    const ble_control_packet_override_t override = {
        .kind = BLE_CONTROL_OVERRIDE_NONE,
        .target = 0U,
    };
    return ble_control_packet_build(mirror, &override, packet);
}

bool ble_unlock_begin(ble_unlock_context_t *context,
                      const ble_unlock_request_t *request,
                      uint32_t current_status_sequence,
                      uint64_t now_ms)
{
    if (context == NULL || context->active || !request_is_valid(request))
    {
        return false;
    }
    *context = (ble_unlock_context_t){
        .request = *request,
        .status_sequence_at_callback = current_status_sequence,
        .write_started_ms = now_ms,
        .active = true,
    };
    return true;
}

bool ble_unlock_accept_write_callback(
    ble_unlock_context_t *context,
    const ble_unlock_request_t *request,
    bool success,
    uint32_t current_status_sequence,
    uint64_t now_ms)
{
    if (!request_matches(context, request) || context->callback_succeeded)
    {
        return false;
    }
    if (!success)
    {
        context->active = false;
        return true;
    }
    context->callback_succeeded = true;
    context->status_sequence_at_callback = current_status_sequence;
    context->callback_success_ms = now_ms;
    return true;
}

bool ble_unlock_accept_status(ble_unlock_context_t *context,
                              const ble_unlock_request_t *request,
                              uint32_t status_sequence,
                              uint64_t now_ms)
{
    if (!request_matches(context, request) || !context->callback_succeeded ||
        !status_sequence_is_after(status_sequence,
                                  context->status_sequence_at_callback) ||
        now_ms < context->callback_success_ms ||
        now_ms - context->callback_success_ms > BLE_UNLOCK_CONFIRM_TIMEOUT_MS)
    {
        return false;
    }
    context->active = false;
    return true;
}

bool ble_unlock_expire(ble_unlock_context_t *context, uint64_t now_ms)
{
    if (context == NULL || !context->active || !context->callback_succeeded ||
        now_ms < context->callback_success_ms ||
        now_ms - context->callback_success_ms <= BLE_UNLOCK_CONFIRM_TIMEOUT_MS)
    {
        return false;
    }
    context->active = false;
    return true;
}

bool ble_unlock_expire_write_callback(ble_unlock_context_t *context,
                                      uint64_t now_ms)
{
    if (context == NULL || !context->active || context->callback_succeeded ||
        now_ms < context->write_started_ms ||
        now_ms - context->write_started_ms <=
            BLE_UNLOCK_WRITE_CALLBACK_TIMEOUT_MS)
    {
        return false;
    }
    context->active = false;
    return true;
}

void ble_unlock_cancel(ble_unlock_context_t *context)
{
    if (context != NULL)
    {
        memset(context, 0, sizeof(*context));
    }
}

static bool status_sequence_is_after(uint32_t candidate, uint32_t baseline)
{
    const uint32_t distance = candidate - baseline;
    return distance != 0U && distance <= UINT32_MAX / 2U;
}

static bool request_is_valid(const ble_unlock_request_t *request)
{
    if (request == NULL || request->request_id == 0U ||
        request->link_generation == 0U ||
        strnlen(request->exoskeleton_mac, BLE_UNLOCK_MAC_CAPACITY) !=
            BLE_UNLOCK_MAC_CAPACITY - 1U)
    {
        return false;
    }
    for (size_t index = 0U; index < BLE_UNLOCK_MAC_CAPACITY - 1U; ++index)
    {
        const bool separator = index == 2U || index == 5U || index == 8U ||
                               index == 11U || index == 14U;
        if ((separator && request->exoskeleton_mac[index] != ':') ||
            (!separator &&
             !((request->exoskeleton_mac[index] >= '0' &&
                request->exoskeleton_mac[index] <= '9') ||
               (request->exoskeleton_mac[index] >= 'A' &&
                request->exoskeleton_mac[index] <= 'F'))))
        {
            return false;
        }
    }
    return true;
}

static bool request_matches(const ble_unlock_context_t *context,
                            const ble_unlock_request_t *request)
{
    return context != NULL && context->active && request_is_valid(request) &&
           context->request.intent_sequence == request->intent_sequence &&
           context->request.request_id == request->request_id &&
           context->request.link_generation == request->link_generation &&
           memcmp(context->request.exoskeleton_mac,
                  request->exoskeleton_mac,
                  BLE_UNLOCK_MAC_CAPACITY) == 0;
}
