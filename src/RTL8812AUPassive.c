/* SPDX-License-Identifier: BSD-2-Clause */
#include "rtw/RTL8812AUPassive.h"
#include <stdint.h>
#include <string.h>

/* The borrowed USB transfer cannot also be the mutable statistics buffer. */
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    uintptr_t ap, bp;
    if (!a || !b || !an || !bn) return 0;
    ap = (uintptr_t)a; bp = (uintptr_t)b;
    return ap <= bp ? bp - ap < an : ap - bp < bn;
}

static void count_one(size_t *counter) {
    if (*counter != SIZE_MAX) ++*counter;
}

static rtw_r12au_passive_result invalid_aggregate(
    rtw_r12au_result reason, rtw_r12au_passive_stats *stats) {
    count_one(&stats->malformed_aggregates);
    if (reason == RTW_R12AU_TOO_MANY) return RTW_R12AU_PASSIVE_TOO_MANY;
    return RTW_R12AU_PASSIVE_BAD_AGGREGATE;
}

rtw_r12au_passive_result rtw_r12au_passive_on_channel(
    const uint8_t *bulk, size_t length, rtw_r12au_chip chip,
    uint8_t tuned_channel,
    rtw_r12au_passive_ap_callback on_ap, void *context,
    rtw_r12au_passive_stats *stats) {
    rtw_r12au_rx_iterator iterator;
    rtw_r12au_rx_frame frame;
    rtw_r12au_result status;
    rtw_r12au_channel_request channel = {chip, tuned_channel, 20U,
                                        tuned_channel};
    rtw_r12au_channel_plan plan;
    if (!on_ap || !stats || (!bulk && length) ||
        overlaps(bulk, length, stats, stats ? sizeof(*stats) : 0U) ||
        length > RTW_R12AU_MAX_BULK ||
        rtw_r12au_channel_prepare(&channel, &plan) != RTW_R12AU_CHANNEL_OK ||
        !plan.passive_only)
        return RTW_R12AU_PASSIVE_BAD_ARGUMENT;
    if (length == 0U) return RTW_R12AU_PASSIVE_OK;

    /* First pass: no callbacks or frame counters before full validation. */
    memset(&iterator, 0, sizeof(iterator));
    iterator.buffer = bulk;
    iterator.length = length;
    while ((status = rtw_r12au_rx_next(&iterator,
           chip == RTW_R12AU_CHIP_8812AU ? 2U : 1U, &frame)) ==
           RTW_R12AU_FRAME) {}
    if (status != RTW_R12AU_END)
        return invalid_aggregate(status, stats);

    memset(&iterator, 0, sizeof(iterator));
    iterator.buffer = bulk;
    iterator.length = length;
    count_one(&stats->aggregates);
    while ((status = rtw_r12au_rx_next(&iterator,
           chip == RTW_R12AU_CHIP_8812AU ? 2U : 1U, &frame)) ==
           RTW_R12AU_FRAME) {
        rtw_r12au_scan_error scan;
        rtw_r12au_ap ap;
        count_one(&stats->frames);
        if (frame.is_c2h) {
            count_one(&stats->c2h);
            continue;
        }
        if (frame.crc_error) {
            count_one(&stats->crc_drops);
            continue;
        }
        if (frame.icv_error) {
            count_one(&stats->icv_drops);
            continue;
        }
        scan = rtw_r12au_scan_parse(&frame, tuned_channel, 1, &ap);
        switch (scan) {
        case RTW_R12AU_SCAN_OK:
            /* An AP can be well-formed but unsupported (WPA3/WEP/PMF).
             * Keep it visible to diagnostics, never imply association. */
            if (ap.security == RTW_R12AU_SECURITY_UNSUPPORTED ||
                ap.pmf_required)
                count_one(&stats->unsupported);
            if (!on_ap(context, &ap, &frame))
                return RTW_R12AU_PASSIVE_CALLBACK_STOP;
            count_one(&stats->accepted_beacons);
            break;
        case RTW_R12AU_SCAN_NOT_BEACON:
            count_one(&stats->non_beacons);
            break;
        case RTW_R12AU_SCAN_BAD_FCS:
            count_one(&stats->fcs_drops);
            break;
        case RTW_R12AU_SCAN_WRONG_CHANNEL:
            count_one(&stats->wrong_channel);
            break;
        case RTW_R12AU_SCAN_UNSUPPORTED:
            count_one(&stats->unsupported);
            break;
        case RTW_R12AU_SCAN_MALFORMED:
        case RTW_R12AU_SCAN_BAD_ARGUMENT:
        default:
            count_one(&stats->invalid_beacons);
            break;
        }
    }
    /* The first pass ensures this never happens for stable immutable input.
     * A concurrent mutation of a caller-owned buffer is treated as corrupt. */
    if (status != RTW_R12AU_END)
        return invalid_aggregate(status, stats);
    return RTW_R12AU_PASSIVE_OK;
}

rtw_r12au_passive_result rtw_r12au_passive_channel6(
    const uint8_t *bulk, size_t length,
    rtw_r12au_passive_ap_callback on_ap, void *context,
    rtw_r12au_passive_stats *stats) {
    return rtw_r12au_passive_on_channel(bulk, length,
        RTW_R12AU_CHIP_8812AU, 6U, on_ap, context, stats);
}

const char *rtw_r12au_passive_result_string(rtw_r12au_passive_result result) {
    switch (result) {
    case RTW_R12AU_PASSIVE_OK: return "ok";
    case RTW_R12AU_PASSIVE_BAD_ARGUMENT: return "invalid passive capture arguments";
    case RTW_R12AU_PASSIVE_BAD_AGGREGATE: return "malformed RTL8812AU USB aggregate";
    case RTW_R12AU_PASSIVE_TOO_MANY: return "too many frames in RTL8812AU USB aggregate";
    case RTW_R12AU_PASSIVE_CALLBACK_STOP: return "capture callback requested stop";
    default: return "unknown passive capture error";
    }
}
