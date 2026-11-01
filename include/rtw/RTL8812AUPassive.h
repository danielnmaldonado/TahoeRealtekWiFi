/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef RTW_RTL8812AU_PASSIVE_H
#define RTW_RTL8812AU_PASSIVE_H

#include "rtw/RTL8812AUStation.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_R12AU_PASSIVE_OK = 0,
    RTW_R12AU_PASSIVE_BAD_ARGUMENT,
    RTW_R12AU_PASSIVE_BAD_AGGREGATE,
    RTW_R12AU_PASSIVE_TOO_MANY,
    RTW_R12AU_PASSIVE_CALLBACK_STOP
} rtw_r12au_passive_result;

typedef struct {
    size_t aggregates;
    size_t malformed_aggregates;
    size_t frames;
    size_t c2h;
    size_t crc_drops;
    size_t icv_drops;
    size_t non_beacons;
    size_t unsupported;
    size_t wrong_channel;
    size_t invalid_beacons;
    size_t fcs_drops;
    size_t accepted_beacons;
} rtw_r12au_passive_stats;

/*
 * Receive-only laboratory capture for RTL8812AU B-cut/RFE3/2.4GHz/channel 6.
 * No hardware, USB or transmission is performed inside this function.
 *
 * A caller must first separately obtain a successful authorized passive
 * monitor configuration and verify the registers on its actual device.
 * Physical channel comes from that trusted configuration, not from 802.11
 * advertisements. Every accepted beacon/probe must have a valid on-air FCS.
 *
 * The entire USB aggregate is checked BEFORE any callback is invoked, so a
 * truncated tail cannot cause a partially emitted list of APs. A callback
 * may retain the AP structure by copying it; its RX payload pointer expires
 * with the caller-owned USB transfer buffer.
 *
 * A failing callback returns 0 and stops processing. Statistics accumulate
 * in the caller's structure; no dynamic allocation is performed.
 */
typedef int (*rtw_r12au_passive_ap_callback)(
    void *context, const rtw_r12au_ap *ap, const rtw_r12au_rx_frame *frame);

/* Generalized OFFLINE 2.4/5-GHz receive inspection for the existing Jaguar
 * RTL8812AU/RTL8821AU bulk descriptor format. tuned_channel MUST come from
 * verified physical RX configuration, never a packet-provided channel IE.
 * This function never programs a radio and cannot authorize RF/TX. */
rtw_r12au_passive_result rtw_r12au_passive_on_channel(
    const uint8_t *bulk, size_t length, rtw_r12au_chip chip,
    uint8_t tuned_channel,
    rtw_r12au_passive_ap_callback on_ap, void *context,
    rtw_r12au_passive_stats *stats);

/* Source-compatible wrapper for the existing channel-6 profile. */
rtw_r12au_passive_result rtw_r12au_passive_channel6(
    const uint8_t *bulk, size_t length,
    rtw_r12au_passive_ap_callback on_ap, void *context,
    rtw_r12au_passive_stats *stats);
const char *rtw_r12au_passive_result_string(rtw_r12au_passive_result result);

#ifdef __cplusplus
}
#endif
#endif
