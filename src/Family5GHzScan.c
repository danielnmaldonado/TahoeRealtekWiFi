/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/Family5GHzScan.h"
#include "rtw/FamilyRadioPolicy.h"
#include "rtw/FamilyRX24.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/RTL8812AUDataPath.h"
#include <string.h>

#define MAX_5G_BULK_BYTES 65536U
#define MAX_5G_FRAMES 64U

/* Avoid end-pointer arithmetic overflow while checking borrowed input/output.
 * Statistics may never alias a transfer or the immutable catalog identity. */
static int overlaps(const void *a, size_t an, const void *b, size_t bn) {
    uintptr_t ap, bp;
    if (!a || !b || !an || !bn) return 0;
    ap = (uintptr_t)a; bp = (uintptr_t)b;
    return ap <= bp ? bp - ap < an : ap - bp < bn;
}

typedef enum { CODEC_JAGUAR, CODEC_8814, CODEC_RTW88 } codec_kind;
typedef struct {
    codec_kind kind;
    rtw_wifi_family family;
    unsigned chains;
    rtw_r12au_rx_iterator jaguar;
    rtw_rx_iterator ac;
    rtw_rx24_iterator newer;
} decoder;

static int pick_codec(rtw_wifi_family family, codec_kind *kind,
                      unsigned *chains) {
    switch (family) {
    case RTW_WIFI_RTL8812AU:
        *kind = CODEC_JAGUAR; *chains = 2; return 1;
    case RTW_WIFI_RTL8821AU:
        *kind = CODEC_JAGUAR; *chains = 1; return 1;
    case RTW_WIFI_RTL8814AU:
        *kind = CODEC_8814; *chains = 2; return 1;
    case RTW_WIFI_RTL8821CU:
        *kind = CODEC_RTW88; *chains = 1; return 1;
    case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU:
        *kind = CODEC_RTW88; *chains = 2; return 1;
    default:
        return 0;
    }
}

static void initialize(decoder *d, codec_kind kind, unsigned chains,
                       rtw_wifi_family family, const uint8_t *bulk,
                       size_t length) {
    memset(d, 0, sizeof(*d));
    d->kind = kind; d->chains = chains; d->family = family;
    d->jaguar.buffer = bulk; d->jaguar.length = length;
    rtw_rx_iterator_init(&d->ac, bulk, length);
    rtw_rx24_iterator_init(&d->newer, bulk, length);
}

/* 1: frame, 0: end, -1: malformed descriptor. The returned record is a
 * borrowed view of the bytes, never a transformed copy of an MPDU. */
static int next_frame(decoder *d, rtw_r12au_rx_frame *out) {
    memset(out, 0, sizeof(*out));
    switch (d->kind) {
    case CODEC_JAGUAR: {
        const rtw_r12au_result e = rtw_r12au_rx_next(
            &d->jaguar, d->chains, out);
        if (e == RTW_R12AU_FRAME) return 1;
        return e == RTW_R12AU_END ? 0 : -1;
    }
    case CODEC_8814: {
        rtw_8814au_rx_frame f;
        const rtw_packet_error e = rtw_8814au_rx_next(&d->ac, &f);
        if (e == RTW_PACKETS_DONE) return 0;
        if (e != RTW_PACKETS_OK) return -1;
        out->payload = f.packet.mpdu;
        out->payload_length = f.packet.mpdu_length;
        out->phy = f.packet.phy_info;
        out->phy_length = f.packet.phy_info_length;
        out->timestamp = f.packet.timestamp;
        out->rate = f.packet.rate_index;
        out->bandwidth_mhz = (uint8_t)(20U << f.packet.bandwidth_code);
        out->crc_error = f.packet.crc_error;
        out->icv_error = f.packet.icv_error;
        out->cipher = f.security_code;
        out->sw_decrypt = f.sw_decrypt_required;
        out->is_c2h = f.is_c2h;
        return 1;
    }
    case CODEC_RTW88: {
        rtw_rx24_frame f;
        const rtw_rx24_status e = rtw_rx24_next(
            &d->newer, d->family, &f);
        if (e == RTW_RX24_END) return 0;
        if (e != RTW_RX24_FRAME) return -1;
        out->payload = f.payload;
        out->payload_length = f.payload_length;
        out->phy = f.driver_info;
        out->phy_length = f.driver_info_length;
        out->timestamp = f.timestamp;
        out->rate = f.rate;
        out->bandwidth_mhz = (uint8_t)(20U << f.bandwidth_code);
        out->crc_error = f.crc_error;
        out->icv_error = f.icv_error;
        out->is_c2h = f.is_firmware_event;
        /* The common rtw88 descriptor parser does not expose hardware
         * cipher flags. Protected management frames are independently
         * rejected by the 802.11 beacon parser. */
        return 1;
    }
    }
    return -1;
}

static size_t cursor(const decoder *d) {
    switch (d->kind) {
    case CODEC_JAGUAR: return d->jaguar.cursor;
    case CODEC_8814: return d->ac.cursor;
    case CODEC_RTW88: return d->newer.cursor;
    }
    return SIZE_MAX;
}

static int complete_aggregate(decoder *d, size_t length) {
    unsigned frames = 0;
    for (;;) {
        rtw_r12au_rx_frame frame;
        const size_t before = cursor(d);
        const int status = next_frame(d, &frame);
        if (status == 0) return cursor(d) == length;
        if (status != 1 || cursor(d) <= before || cursor(d) > length ||
            ++frames > MAX_5G_FRAMES) return 0;
    }
}

rtw_family_5g_scan_result rtw_family_5g_scan_bulk(
    const rtw_usb_wifi_profile *profile,
    const uint8_t *bulk, size_t length, uint8_t tuned_channel,
    int fcs_present, rtw_family_5g_scan_callback on_ap,
    void *context, rtw_family_5g_scan_stats *stats) {
    rtw_wifi_radio_capabilities cap;
    rtw_r12au_channel_plan geometry;
    codec_kind kind;
    unsigned chains;
    decoder d;
    /* Validate aliases before clearing stats: otherwise a malformed call
     * can overwrite the supposedly immutable USB transfer or catalog data. */
    if (stats && (overlaps(stats, sizeof(*stats), bulk, length) ||
        overlaps(stats, sizeof(*stats), profile,
                 profile ? sizeof(*profile) : 0U)))
        return RTW_FAMILY_5G_SCAN_BAD_ARGUMENT;
    if (stats) memset(stats, 0, sizeof(*stats));
    if (!stats || !on_ap || (!bulk && length) ||
        length > MAX_5G_BULK_BYTES ||
        (fcs_present != 0 && fcs_present != 1))
        return RTW_FAMILY_5G_SCAN_BAD_ARGUMENT;
    cap = rtw_wifi_radio_profile_capabilities(profile);
    if (!cap.identity_verified || !(cap.family_bands & RTW_WIFI_BAND_5GHZ))
        return RTW_FAMILY_5G_SCAN_UNAVAILABLE;
    /* Only dereference profile after verifying immutable catalog identity. */
    if (!pick_codec(profile->family, &kind, &chains))
        return RTW_FAMILY_5G_SCAN_UNAVAILABLE;
    if (tuned_channel <= 14U ||
        rtw_family_radio_prepare_profile(profile, tuned_channel,
          tuned_channel, 20U, &geometry) != RTW_FAMILY_RADIO_OK ||
        !geometry.passive_only || !geometry.is_5ghz)
        return RTW_FAMILY_5G_SCAN_BAD_ARGUMENT;
    if (!length) return RTW_FAMILY_5G_SCAN_OK;

    initialize(&d, kind, chains, profile->family, bulk, length);
    if (!complete_aggregate(&d, length)) {
        stats->malformed_aggregates = 1U;
        return RTW_FAMILY_5G_SCAN_BAD_AGGREGATE;
    }
    initialize(&d, kind, chains, profile->family, bulk, length);
    stats->aggregates = 1U;
    for (;;) {
        rtw_r12au_rx_frame frame;
        rtw_r12au_ap ap;
        rtw_r12au_scan_error scan;
        int status = next_frame(&d, &frame);
        if (!status) return RTW_FAMILY_5G_SCAN_OK;
        /* Buffer mutation between passes breaks the immutable-input contract.
         * Already emitted results cannot be retracted; caller must not mutate. */
        if (status < 0) {
            ++stats->malformed_aggregates;
            return RTW_FAMILY_5G_SCAN_BAD_AGGREGATE;
        }
        ++stats->frames;
        if (frame.is_c2h) { ++stats->firmware_events; continue; }
        if (frame.crc_error) { ++stats->crc_drops; continue; }
        if (frame.icv_error) { ++stats->icv_drops; continue; }
        scan = rtw_r12au_scan_parse(&frame, tuned_channel, fcs_present, &ap);
        switch (scan) {
        case RTW_R12AU_SCAN_OK:
            if (!on_ap(context, &ap, &frame))
                return RTW_FAMILY_5G_SCAN_CALLBACK_STOP;
            ++stats->accepted_beacons;
            break;
        case RTW_R12AU_SCAN_NOT_BEACON: ++stats->other_frames; break;
        case RTW_R12AU_SCAN_WRONG_CHANNEL: ++stats->wrong_channel; break;
        case RTW_R12AU_SCAN_BAD_FCS: ++stats->bad_fcs; break;
        default: ++stats->rejected_beacons; break;
        }
    }
}
