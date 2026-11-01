/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/FamilyRX24.h"
#include <string.h>

#define RX24_DESCRIPTOR_BYTES 24U
#define RX24_MAX_BULK_BYTES 65536U
#define RX24_MAX_FRAMES 64U
#define RX24_MAX_MPDU_BYTES 11454U
#define RX24_MAX_C2H_BYTES 2048U

static uint32_t little32(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8U) |
           ((uint32_t)b[2] << 16U) | ((uint32_t)b[3] << 24U);
}

static int all_zero(const uint8_t *b, size_t count) {
    size_t n;
    for (n = 0; n < count; ++n)
        if (b[n]) return 0;
    return 1;
}

static int profile(rtw_wifi_family family, unsigned *chains, int *rtw88) {
    switch (family) {
    case RTW_WIFI_RTL8192FU: *chains = 2U; *rtw88 = 0; return 1;
    case RTW_WIFI_RTL8723BU: *chains = 1U; *rtw88 = 0; return 1;
    case RTW_WIFI_RTL8821CU: *chains = 1U; *rtw88 = 1; return 1;
    case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU: *chains = 2U; *rtw88 = 1; return 1;
    default: return 0;
    }
}

static int valid_rate(unsigned rate, unsigned chains, int rtw88) {
    if (rate <= 11U) return 1;
    if (rate >= 12U && rate < 12U + chains * 8U) return 1;
    if (rtw88 && rate >= 44U && rate < 44U + chains * 10U) return 1;
    return 0;
}

void rtw_rx24_iterator_init(rtw_rx24_iterator *iterator,
                            const uint8_t *bytes, size_t length) {
    if (!iterator) return;
    iterator->bytes = bytes;
    iterator->length = length;
    iterator->cursor = 0;
    iterator->frames = 0;
}

rtw_rx24_status rtw_rx24_next(rtw_rx24_iterator *iterator,
                              rtw_wifi_family family, rtw_rx24_frame *out) {
    unsigned chains = 0;
    int rtw88 = 0;
    const uint8_t *bytes;
    size_t remain, length, info, shift, used, padded, next;
    uint32_t w0, w2, w3, w4;
    unsigned rate, bandwidth, event;
    rtw_rx24_frame frame;

    if (out) memset(out, 0, sizeof(*out));
    if (!iterator || !out || (!iterator->bytes && iterator->length) ||
        iterator->length > RX24_MAX_BULK_BYTES ||
        iterator->cursor > iterator->length ||
        iterator->frames > RX24_MAX_FRAMES)
        return RTW_RX24_INVALID;
    if (!profile(family, &chains, &rtw88)) return RTW_RX24_UNSUPPORTED;
    if (iterator->cursor == iterator->length) return RTW_RX24_END;

    remain = iterator->length - iterator->cursor;
    bytes = iterator->bytes + iterator->cursor;
    if (remain < RX24_DESCRIPTOR_BYTES) {
        if (iterator->frames && remain <= 7U && all_zero(bytes, remain)) {
            iterator->cursor = iterator->length;
            return RTW_RX24_END;
        }
        return RTW_RX24_MALFORMED;
    }
    if (iterator->frames == RX24_MAX_FRAMES)
        return RTW_RX24_MALFORMED;

    w0 = little32(bytes);
    w2 = little32(bytes + 8U);
    w3 = little32(bytes + 12U);
    w4 = little32(bytes + 16U);
    length = (size_t)(w0 & 0x3fffU);
    if (length == 0U) {
        if (iterator->frames && all_zero(bytes, remain)) {
            iterator->cursor = iterator->length;
            return RTW_RX24_END;
        }
        return RTW_RX24_MALFORMED;
    }

    info = (size_t)((w0 >> 16U) & 15U) * 8U;
    shift = (size_t)((w0 >> 24U) & 3U);
    if (info > remain - RX24_DESCRIPTOR_BYTES ||
        shift > remain - RX24_DESCRIPTOR_BYTES - info ||
        length > remain - RX24_DESCRIPTOR_BYTES - info - shift)
        return RTW_RX24_MALFORMED;

    event = rtw88 ? ((w2 >> 28U) & 1U) : ((w3 >> 14U) & 3U);
    rate = w3 & 0x7fU;
    bandwidth = (w4 >> 4U) & 3U;
    if (event) {
        if (length < 2U || length > RX24_MAX_C2H_BYTES)
            return RTW_RX24_MALFORMED;
    } else {
        if (length < 4U || length > RX24_MAX_MPDU_BYTES ||
            !valid_rate(rate, chains, rtw88) ||
            bandwidth > (rtw88 ? 2U : 1U))
            return RTW_RX24_MALFORMED;
    }

    used = RX24_DESCRIPTOR_BYTES + info + shift + length;
    padded = (used + 7U) & ~(size_t)7U;
    if (padded > remain && used != remain)
        return RTW_RX24_MALFORMED;
    next = padded <= remain ? padded : used;
    if (next == 0U || next > remain)
        return RTW_RX24_MALFORMED;

    memset(&frame, 0, sizeof(frame));
    frame.payload = bytes + RX24_DESCRIPTOR_BYTES + info + shift;
    frame.payload_length = length;
    /* rtw88/rx.c puts SHIFT before the PHY report. rtl8xxxu/core.c
     * captures PHY before skipping its report and trailing SHIFT padding. */
    frame.driver_info = info ? bytes + RX24_DESCRIPTOR_BYTES + (rtw88 ? shift : 0U) : NULL;
    frame.driver_info_length = info;
    frame.timestamp = little32(bytes + 20U);
    frame.is_firmware_event = (uint8_t)(event != 0U);
    frame.crc_error = (uint8_t)((w0 >> 14U) & 1U);
    frame.icv_error = (uint8_t)((w0 >> 15U) & 1U);
    frame.rate = (uint8_t)rate;
    frame.bandwidth_code = (uint8_t)bandwidth;
    frame.rf_chains = (uint8_t)chains;
    *out = frame;
    iterator->cursor += next;
    ++iterator->frames;
    return RTW_RX24_FRAME;
}
