/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_FAMILY_5GHZ_SCAN_H
#define RTW_FAMILY_5GHZ_SCAN_H

#include "rtw/RTL8812AUStation.h"
#include "rtw/USBWiFiProfiles.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Offline decoding only. This API DOES NOT tune RF, start a station, download
 * firmware, authorize transmission, or attest that a 5-GHz capture was made
 * by a particular USB adapter. The caller must obtain the tuned channel and
 * FCS-presence metadata from a trusted, independently configured receiver.
 * A verified catalog ID does not establish functional hardware support. */
typedef enum {
    RTW_FAMILY_5G_SCAN_OK = 0,
    RTW_FAMILY_5G_SCAN_BAD_ARGUMENT,
    RTW_FAMILY_5G_SCAN_UNAVAILABLE,
    RTW_FAMILY_5G_SCAN_BAD_AGGREGATE,
    RTW_FAMILY_5G_SCAN_CALLBACK_STOP
} rtw_family_5g_scan_result;

typedef struct {
    size_t aggregates;
    size_t malformed_aggregates;
    size_t frames;
    size_t firmware_events;
    size_t crc_drops;
    size_t icv_drops;
    size_t other_frames;
    size_t rejected_beacons;
    size_t wrong_channel;
    size_t bad_fcs;
    size_t accepted_beacons;
} rtw_family_5g_scan_stats;

/* On success the frame and AP are valid for the duration of the callback.
 * Its payload points into the caller-owned immutable bulk buffer. Copy any
 * data retained beyond the callback. Returning zero terminates delivery. */
typedef int (*rtw_family_5g_scan_callback)(
    void *context, const rtw_r12au_ap *ap, const rtw_r12au_rx_frame *frame);

/* Frame-atomic 5-GHz offline scan for existing RX descriptor implementations:
 * 8812AU / 8821AU: Jaguar; 8814AU: 8814; 8821CU / 8822BU / 8822CU: rtw88.
 * 8192DU is intentionally unavailable: no verified RX ABI is present.
 * Two bounded passes validate the complete USB aggregate before any callback;
 * no network is published if a later descriptor is malformed.
 * fcs_present MUST be exactly 0 or 1; NEVER infer it from beacon payload.
 * The profile must be an exact verified catalog pointer (not a forged copy).
 * Statistics are overwritten, including on failure. No allocation or I/O. */
rtw_family_5g_scan_result rtw_family_5g_scan_bulk(
    const rtw_usb_wifi_profile *profile,
    const uint8_t *bulk, size_t length, uint8_t tuned_channel,
    int fcs_present, rtw_family_5g_scan_callback on_ap,
    void *context, rtw_family_5g_scan_stats *stats);

#ifdef __cplusplus
}
#endif
#endif
