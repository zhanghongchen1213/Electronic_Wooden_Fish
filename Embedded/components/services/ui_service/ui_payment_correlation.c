/**
 * @file     ui_payment_correlation.c
 * @brief    实现支付验证结果的本地关联与页面投影策略
 * @details  仅允许同一会话收口手动验证，并保持 WAITING_PAYMENT/RETRY_WAIT 为非终态页面。
 * @author   ZHC
 * @date     2026-08-03
 */

#include "ui_payment_correlation.h"

#include <string.h>

void ui_payment_projection_build(
    bool authorization_needed,
    bool current_generation,
    bool authorization_transport_ready,
    bool manual_attempted,
    bool manual_pending,
    watch_rental_phase_t phase,
    watch_rental_result_t last_result,
    ui_payment_projection_t *projection)
{
    if (projection == NULL)
    {
        return;
    }
    memset(projection, 0, sizeof(*projection));
    if (!authorization_needed)
    {
        return;
    }
    const bool waiting_payment =
        current_generation && phase == WATCH_RENTAL_PHASE_WAITING_PAYMENT;
    const bool retry_wait =
        current_generation && phase == WATCH_RENTAL_PHASE_RETRY_WAIT;
    const bool blocked =
        current_generation && phase == WATCH_RENTAL_PHASE_BLOCKED;
    const bool legacy_unpaid =
        blocked && last_result == WATCH_RENTAL_RESULT_UNPAID;
    const bool owner_pending =
        current_generation &&
        (phase == WATCH_RENTAL_PHASE_QUERYING ||
         phase == WATCH_RENTAL_PHASE_UNLOCK_WRITING ||
         phase == WATCH_RENTAL_PHASE_CONFIRMING);

    projection->display =
        waiting_payment || legacy_unpaid
            ? UI_AUTHORIZATION_DISPLAY_UNPAID
            : (retry_wait || blocked
                   ? UI_AUTHORIZATION_DISPLAY_FAILED
                   : UI_AUTHORIZATION_DISPLAY_CHECKING);
    projection->authorization_pending =
        projection->display == UI_AUTHORIZATION_DISPLAY_CHECKING;
    projection->payment_required =
        projection->display != UI_AUTHORIZATION_DISPLAY_NONE;
    projection->payment_attempted =
        projection->display == UI_AUTHORIZATION_DISPLAY_FAILED ||
        (manual_attempted &&
         projection->display != UI_AUTHORIZATION_DISPLAY_CHECKING);
    projection->payment_pending =
        projection->authorization_pending ||
        (projection->payment_required && (manual_pending || owner_pending));
    projection->payment_failed =
        projection->display == UI_AUTHORIZATION_DISPLAY_FAILED;
    projection->payment_action_enabled =
        (projection->display == UI_AUTHORIZATION_DISPLAY_UNPAID || retry_wait) &&
        authorization_transport_ready && !projection->payment_pending &&
        (!blocked || legacy_unpaid);
}

bool ui_payment_result_matches(bool pending,
                               uint32_t pending_intent_sequence,
                               uint32_t pending_link_generation,
                               uint32_t result_intent_sequence,
                               uint32_t result_link_generation)
{
    return pending && pending_intent_sequence != 0U &&
           pending_link_generation != 0U &&
           result_intent_sequence == pending_intent_sequence &&
           result_link_generation == pending_link_generation;
}
