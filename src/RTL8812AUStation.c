/* SPDX-License-Identifier: BSD-2-Clause */
#include "rtw/RTL8812AUStation.h"
#include "rtw/FCS.h"
#include <string.h>

/* All inputs are untrusted radio frames or caller-selected scan metadata.
 * The parser never allocates and all cursor operations are length checked. */
/* A result may not overwrite the borrowed RX descriptor or packet.
 * Compare offsets instead of adding pointer lengths (overflow-safe). */
static int output_overlaps_input(const void *out, size_t out_len,
                                 const void *input, size_t input_len) {
    uintptr_t a, b;
    if (!out || !input || !out_len || !input_len) return 0;
    a=(uintptr_t)out; b=(uintptr_t)input;
    return a<=b ? b-a<out_len : a-b<input_len;
}
static uint16_t read16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8U));
}
static void write16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8U);
}
static int unicast(const uint8_t *m) {
    unsigned nonzero = 0U, i;
    if (!m || (m[0] & 1U)) return 0;
    for (i = 0; i < 6U; ++i) nonzero |= m[i];
    return nonzero != 0U;
}
static int rate_allowed(uint8_t octet, int band_5ghz) {
    static const uint8_t rates[] =
        {2U, 4U, 11U, 22U, 12U, 18U, 24U, 36U, 48U, 72U, 96U, 108U};
    unsigned i;
    const uint8_t v = (uint8_t)(octet & 127U);
    if (band_5ghz && v < 12U) return 0;
    for (i = 0U; i < sizeof(rates); ++i)
        if (v == rates[i]) return 1;
    return 0;
}
static int copy_rates(rtw_r12au_ap *ap, const uint8_t *bytes, size_t length,
                      int band_5ghz) {
    size_t i, j;
    if (ap->rate_count > RTW_R12AU_LEGACY_RATES_MAX ||
        length > (size_t)RTW_R12AU_LEGACY_RATES_MAX - ap->rate_count) return 0;
    for (i = 0U; i < length; ++i) {
        if (!rate_allowed(bytes[i], band_5ghz)) return 0;
        for (j = 0U; j < ap->rate_count; ++j)
            if ((ap->rates[j] & 127U) == (bytes[i] & 127U)) return 0;
        ap->rates[ap->rate_count++] = bytes[i];
    }
    return 1;
}
static int suite_is(const uint8_t *p, uint8_t suite) {
    return p[0] == 0x00U && p[1] == 0x0fU &&
           p[2] == 0xacU && p[3] == suite;
}
/* Returns 1 when the AP offers a WPA2-PSK/CCMP choice, including
 * transition-mode beacons that also advertise SAE or other AKMs.
 * The station's association request selects ONLY PSK/CCMP: this is not
 * WPA3-SAE support. Group cipher must be CCMP; PMF must not be required.
 * Returns -1 for malformed lengths, 0 for well-formed unsupported suites. */
static int rsn_psk_ccmp(const uint8_t *p, size_t len, uint8_t *pmf_required) {
    size_t cursor, count, i;
    uint16_t caps = 0U;
    int pairwise_ccmp = 0, psk = 0;
    const int group_ccmp = len >= 6U && suite_is(p + 2U, 4U);

    if (len < 18U || read16(p) != 1U || !pmf_required) return -1;
    *pmf_required = 0U;
    count = read16(p + 6U);
    if (count == 0U || count > (len - 8U) / 4U) return -1;
    cursor = 8U;
    for (i = 0U; i < count; ++i) {
        if (suite_is(p + cursor + 4U * i, 4U)) pairwise_ccmp = 1;
    }
    cursor += 4U * count;
    if (len - cursor < 2U) return -1;
    count = read16(p + cursor);
    cursor += 2U;
    if (count == 0U || count > (len - cursor) / 4U) return -1;
    for (i = 0U; i < count; ++i) {
        if (suite_is(p + cursor + 4U * i, 2U)) psk = 1;
    }
    cursor += 4U * count;
    if (cursor < len) {
        if (len - cursor < 2U) return -1;
        caps = read16(p + cursor);
        cursor += 2U;
        /* PMF required is unsupported by the current station backend. */
        *pmf_required = (uint8_t)((caps & 0x0040U) != 0U);
    }
    if (cursor < len) {
        if (len - cursor < 2U) return -1;
        count = read16(p + cursor);
        cursor += 2U;
        if (count > (len - cursor) / 16U) return -1;
        cursor += 16U * count;
    }
    if (cursor < len) {
        if (len - cursor != 4U) return -1;
        if (!suite_is(p + cursor, 6U) &&
            !suite_is(p + cursor, 11U) &&
            !suite_is(p + cursor, 12U))
            return 0;
        cursor += 4U;
    }
    if (cursor != len) return -1;
    if ((caps & 0x0040U) && !(caps & 0x0080U)) return -1;
    return (group_ccmp && pairwise_ccmp && psk &&
            !*pmf_required) ? 1 : 0;
}

rtw_r12au_scan_error rtw_r12au_scan_parse(
    const rtw_r12au_rx_frame *rx, uint8_t tuned_channel, int fcs_present,
    rtw_r12au_ap *out) {
    rtw_r12au_channel_request request;
    rtw_r12au_channel_plan plan;
    rtw_r12au_ap ap;
    const uint8_t *p;
    size_t length, offset;
    unsigned ssid_seen = 0U, rates_seen = 0U, ext_seen = 0U,
             rsn_seen = 0U, ds_seen = 0U, ht_seen = 0U,
             vht_seen = 0U;
    uint8_t ht_secondary = 0U, vht_width = 0U,
            vht_center0 = 0U, vht_center1 = 0U;
    if (out && rx &&
        (output_overlaps_input(out,sizeof(*out),rx,sizeof(*rx)) ||
         output_overlaps_input(out,sizeof(*out),rx->payload,
                               rx->payload_length)))
        return RTW_R12AU_SCAN_BAD_ARGUMENT;
    if (out) memset(out, 0, sizeof(*out));
    if (!rx || !out || !rx->payload || (fcs_present != 0 && fcs_present != 1))
        return RTW_R12AU_SCAN_BAD_ARGUMENT;
    if (rx->is_c2h || rx->crc_error || rx->icv_error || rx->cipher ||
        rx->sw_decrypt)
        return RTW_R12AU_SCAN_MALFORMED;
    if (rx->payload_length < 36U || rx->payload_length > RTW_R12AU_MAX_MPDU)
        return RTW_R12AU_SCAN_MALFORMED;
    p = rx->payload;
    length = rx->payload_length;
    if (fcs_present) {
        if (!rtw_fcs_valid(p, length)) return RTW_R12AU_SCAN_BAD_FCS;
        length -= 4U;
    }
    if (length < 36U) return RTW_R12AU_SCAN_MALFORMED;
    if (p[0] != 0x80U && p[0] != 0x50U)
        return RTW_R12AU_SCAN_NOT_BEACON;
    if ((p[1] & (uint8_t)~0x08U) != 0U ||
        !unicast(p + 10U) ||
        memcmp(p + 10U, p + 16U, 6U) != 0)
        return RTW_R12AU_SCAN_MALFORMED;
    if (p[0] == 0x80U) {
        unsigned i;
        for (i = 4U; i < 10U; ++i)
            if (p[i] != 0xffU) return RTW_R12AU_SCAN_MALFORMED;
    }
    request.chip = RTW_R12AU_CHIP_8812AU;
    request.primary_channel = tuned_channel;
    request.center_channel = tuned_channel;
    request.width_mhz = 20U;
    if (rtw_r12au_channel_prepare(&request, &plan) != RTW_R12AU_CHANNEL_OK)
        return RTW_R12AU_SCAN_WRONG_CHANNEL;
    memset(&ap, 0, sizeof(ap));
    memcpy(ap.bssid, p + 16U, 6U);
    ap.beacon_interval = read16(p + 32U);
    ap.capability = read16(p + 34U);
    ap.primary_channel = tuned_channel;
    ap.operating_width_mhz = 20U;
    ap.center_channel = tuned_channel;
    if (!ap.beacon_interval || !(ap.capability & 1U) ||
        (ap.capability & 2U))
        return RTW_R12AU_SCAN_UNSUPPORTED;
    offset = 36U;
    while (offset < length) {
        uint8_t id, ie_len;
        const uint8_t *item;
        if (length - offset < 2U) return RTW_R12AU_SCAN_MALFORMED;
        id = p[offset];
        ie_len = p[offset + 1U];
        offset += 2U;
        if (ie_len > length - offset) return RTW_R12AU_SCAN_MALFORMED;
        item = p + offset;
        switch (id) {
        case 0U: /* SSID */
            if (++ssid_seen > 1U || ie_len > RTW_R12AU_SSID_MAX)
                return RTW_R12AU_SCAN_MALFORMED;
            ap.ssid_length = ie_len;
            memcpy(ap.ssid, item, ie_len);
            break;
        case 1U: /* Supported Rates */
            if (++rates_seen > 1U || ie_len == 0U || ie_len > 8U ||
                !copy_rates(&ap, item, ie_len, plan.is_5ghz))
                return RTW_R12AU_SCAN_MALFORMED;
            break;
        case 50U: /* Extended Supported Rates */
            if (++ext_seen > 1U || !ie_len ||
                !copy_rates(&ap, item, ie_len, plan.is_5ghz))
                return RTW_R12AU_SCAN_MALFORMED;
            break;
        case 3U: /* Direct Sequence: must match actual PHY. */
            if (++ds_seen > 1U || ie_len != 1U)
                return RTW_R12AU_SCAN_MALFORMED;
            if (item[0] != tuned_channel)
                return RTW_R12AU_SCAN_WRONG_CHANNEL;
            break;
        case 61U: /* HT operation: primary must match actual PHY. */
            if (++ht_seen > 1U || ie_len < 22U)
                return RTW_R12AU_SCAN_MALFORMED;
            if (item[0] != tuned_channel)
                return RTW_R12AU_SCAN_WRONG_CHANNEL;
            /* 802.11n HT Operation: secondary offset bits 0-1. Reserved
             * value 2 cannot define a reproducible channel layout. */
            ht_secondary = (uint8_t)(item[1] & 3U);
            if (ht_secondary == 2U)
                return RTW_R12AU_SCAN_MALFORMED;
            /* The HT Operation permits a secondary channel only when the
             * STA channel-width bit is set. Reject contradictory 5-GHz IEs. */
            if (plan.is_5ghz && ht_secondary && !(item[1] & 4U))
                return RTW_R12AU_SCAN_UNSUPPORTED;
            ap.ht_capable = 1U;
            break;
        case 192U: /* 802.11ac VHT operation */
            if (++vht_seen > 1U || ie_len != 5U)
                return RTW_R12AU_SCAN_MALFORMED;
            vht_width = item[0];
            vht_center0 = item[1];
            vht_center1 = item[2];
            ap.vht_capable = 1U;
            break;
        case 48U: {
            int result;
            if (++rsn_seen > 1U) return RTW_R12AU_SCAN_MALFORMED;
            result = rsn_psk_ccmp(item, ie_len, &ap.pmf_required);
            if (result < 0) return RTW_R12AU_SCAN_MALFORMED;
            ap.security = result ? RTW_R12AU_SECURITY_WPA2_PSK_CCMP
                                 : RTW_R12AU_SECURITY_UNSUPPORTED;
            break;
        }
        default:
            break;
        }
        offset += ie_len;
    }
    if (!ssid_seen || !ap.ssid_length || !rates_seen || !ap.rate_count)
        return RTW_R12AU_SCAN_UNSUPPORTED;
    if (rsn_seen) {
        if (!(ap.capability & 0x0010U))
            return RTW_R12AU_SCAN_MALFORMED;
    } else {
        ap.security = (ap.capability & 0x0010U) ?
            RTW_R12AU_SECURITY_UNSUPPORTED : RTW_R12AU_SECURITY_OPEN;
    }
    if (!plan.is_5ghz) {
        /* Preserve 2.4-GHz legacy behavior. VHT operation is not usable by
         * these devices on that band and must never imply 5-GHz readiness. */
        ap.vht_capable = 0U;
    } else {
        /* A tuned 20-MHz scan can observe APs advertising 40/80-MHz BSS.
         * Validate their advertised channel geometry independently of the
         * currently tuned receiver; no RF register writes occur here. */
        if (ht_seen && ht_secondary) {
            const unsigned center40 = ht_secondary == 1U ?
                (unsigned)tuned_channel + 2U : (unsigned)tuned_channel - 2U;
            rtw_r12au_channel_request layout = {
                RTW_R12AU_CHIP_8812AU, tuned_channel, 40U,
                (uint8_t)center40
            };
            if (rtw_r12au_channel_prepare(&layout, &plan) !=
                RTW_R12AU_CHANNEL_OK)
                return RTW_R12AU_SCAN_UNSUPPORTED;
            ap.operating_width_mhz = 40U;
            ap.center_channel = (uint8_t)center40;
        }
        if (vht_seen) {
            /* 802.11ac requires HT operation information. VHT 160 MHz
             * and 80+80 MHz are outside the supported 5-GHz geometry. */
            if (!ht_seen || vht_width > 1U || vht_center1 != 0U)
                return RTW_R12AU_SCAN_UNSUPPORTED;
            if (vht_width == 0U) {
                if (vht_center0 != 0U)
                    return RTW_R12AU_SCAN_UNSUPPORTED;
            } else {
                rtw_r12au_channel_request layout = {
                    RTW_R12AU_CHIP_8812AU, tuned_channel, 80U, vht_center0
                };
                if (!ht_secondary ||
                    rtw_r12au_channel_prepare(&layout, &plan) !=
                        RTW_R12AU_CHANNEL_OK)
                    return RTW_R12AU_SCAN_UNSUPPORTED;
                ap.operating_width_mhz = 80U;
                ap.center_channel = vht_center0;
            }
        }
    }
    *out = ap;
    return RTW_R12AU_SCAN_OK;
}

void rtw_r12au_join_reset(rtw_r12au_join_session *s) {
    if (s) memset(s, 0, sizeof(*s));
}
rtw_r12au_join_error rtw_r12au_join_select(
    rtw_r12au_join_session *s, const rtw_r12au_ap *ap,
    const uint8_t mac[6], rtw_r12au_chip chip) {
    rtw_r12au_join_session next;
    rtw_r12au_channel_request request;
    rtw_r12au_channel_plan plan;
    size_t i, j;
    if (!s) return RTW_R12AU_JOIN_BAD_ARGUMENT;
    memset(&next, 0, sizeof(next));
    if (!ap || !mac || !unicast(mac) || !unicast(ap->bssid) ||
        !memcmp(mac, ap->bssid, 6U) ||
        !ap->ssid_length || ap->ssid_length > RTW_R12AU_SSID_MAX ||
        !ap->rate_count || ap->rate_count > RTW_R12AU_LEGACY_RATES_MAX ||
        !(ap->capability & 1U) || (ap->capability & 2U)) {
        rtw_r12au_join_reset(s);
        return RTW_R12AU_JOIN_BAD_ARGUMENT;
    }
    request.chip = chip;
    request.primary_channel = ap->primary_channel;
    request.center_channel = ap->primary_channel;
    request.width_mhz = 20U;
    if (rtw_r12au_channel_prepare(&request, &plan) != RTW_R12AU_CHANNEL_OK ||
        ap->pmf_required ||
        ap->security == RTW_R12AU_SECURITY_UNSUPPORTED ||
        (ap->security != RTW_R12AU_SECURITY_OPEN &&
         ap->security != RTW_R12AU_SECURITY_WPA2_PSK_CCMP) ||
        ((ap->capability & 0x0010U) != 0U) !=
            (ap->security == RTW_R12AU_SECURITY_WPA2_PSK_CCMP)) {
        rtw_r12au_join_reset(s);
        return RTW_R12AU_JOIN_UNSUPPORTED_AP;
    }
    for (i = 0U; i < ap->rate_count; ++i) {
        if (!rate_allowed(ap->rates[i], plan.is_5ghz)) {
            rtw_r12au_join_reset(s);
            return RTW_R12AU_JOIN_UNSUPPORTED_AP;
        }
        for (j = 0U; j < i; ++j) {
            if ((ap->rates[j] & 127U) == (ap->rates[i] & 127U)) {
                rtw_r12au_join_reset(s);
                return RTW_R12AU_JOIN_UNSUPPORTED_AP;
            }
        }
    }
    /* No external pointers retained; aliasing s->ap is safe. */
    next.ap = *ap;
    memcpy(next.station, mac, 6U);
    next.state = RTW_R12AU_JOIN_SELECTED;
    *s = next;
    return RTW_R12AU_JOIN_OK;
}
static void management_header(const rtw_r12au_join_session *s,
                              uint8_t subtype, uint16_t seq, uint8_t *p) {
    memset(p, 0, 24U);
    p[0] = subtype;
    memcpy(p + 4U, s->ap.bssid, 6U);
    memcpy(p + 10U, s->station, 6U);
    memcpy(p + 16U, s->ap.bssid, 6U);
    write16(p + 22U, (uint16_t)(seq << 4U));
}
static rtw_r12au_join_error verify_management(
    const rtw_r12au_join_session *s, const uint8_t *p, size_t n,
    uint8_t subtype, size_t minimum) {
    if (!p || n < minimum || n > RTW_R12AU_MAX_MPDU ||
        p[0] != subtype || (p[1] & (uint8_t)~0x08U) ||
        (p[22] & 15U))
        return RTW_R12AU_JOIN_MALFORMED;
    if (memcmp(p + 4U, s->station, 6U) ||
        memcmp(p + 10U, s->ap.bssid, 6U) ||
        memcmp(p + 16U, s->ap.bssid, 6U))
        return RTW_R12AU_JOIN_WRONG_PEER;
    return RTW_R12AU_JOIN_OK;
}
rtw_r12au_join_error rtw_r12au_join_auth_request(
    rtw_r12au_join_session *s, uint16_t seq,
    uint8_t *out, size_t capacity, size_t *written) {
    if (written) *written = 0U;
    if (!s || !out || !written || seq > 4095U)
        return RTW_R12AU_JOIN_BAD_ARGUMENT;
    if (s->state != RTW_R12AU_JOIN_SELECTED)
        return RTW_R12AU_JOIN_BAD_STATE;
    if (capacity < 30U) return RTW_R12AU_JOIN_BUFFER_TOO_SMALL;
    management_header(s, 0xb0U, seq, out);
    write16(out + 24U, 0U); /* Open System authentication, step 1. */
    write16(out + 26U, 1U);
    write16(out + 28U, 0U);
    *written = 30U;
    s->state = RTW_R12AU_JOIN_AUTH_SENT;
    return RTW_R12AU_JOIN_OK;
}
rtw_r12au_join_error rtw_r12au_join_auth_response(
    rtw_r12au_join_session *s, const uint8_t *p, size_t n) {
    rtw_r12au_join_error e;
    if (!s) return RTW_R12AU_JOIN_BAD_ARGUMENT;
    if (s->state != RTW_R12AU_JOIN_AUTH_SENT)
        return RTW_R12AU_JOIN_BAD_STATE;
    e = verify_management(s, p, n, 0xb0U, 30U);
    if (e != RTW_R12AU_JOIN_OK) return e;
    if (read16(p + 24U) != 0U || read16(p + 26U) != 2U)
        return RTW_R12AU_JOIN_MALFORMED;
    s->status = read16(p + 28U);
    if (s->status != 0U) {
        s->state = RTW_R12AU_JOIN_REJECTED;
        return RTW_R12AU_JOIN_REJECTED_BY_AP;
    }
    s->state = RTW_R12AU_JOIN_AUTHENTICATED;
    return RTW_R12AU_JOIN_OK;
}
rtw_r12au_join_error rtw_r12au_join_assoc_request(
    rtw_r12au_join_session *s, uint16_t seq, uint16_t listen,
    uint8_t *out, size_t cap, size_t *written) {
    size_t needed, cursor, first, remaining;
    if (written) *written = 0U;
    if (!s || !out || !written || seq > 4095U ||
        listen == 0U || listen > 1000U)
        return RTW_R12AU_JOIN_BAD_ARGUMENT;
    if (s->state != RTW_R12AU_JOIN_AUTHENTICATED)
        return RTW_R12AU_JOIN_BAD_STATE;
    first = s->ap.rate_count > 8U ? 8U : s->ap.rate_count;
    remaining = s->ap.rate_count - first;
    needed = 28U + 2U + s->ap.ssid_length + 2U + first +
             (remaining ? 2U + remaining : 0U) +
             (s->ap.security == RTW_R12AU_SECURITY_WPA2_PSK_CCMP ? 22U : 0U);
    if (cap < needed) return RTW_R12AU_JOIN_BUFFER_TOO_SMALL;
    management_header(s, 0x00U, seq, out);
    write16(out + 24U, (uint16_t)(1U |
            (s->ap.security == RTW_R12AU_SECURITY_WPA2_PSK_CCMP ?
             0x0010U : 0U) |
            (s->ap.capability & 0x0420U))); /* short preamble / short slot */
    write16(out + 26U, listen);
    cursor = 28U;
    out[cursor++] = 0U;
    out[cursor++] = s->ap.ssid_length;
    memcpy(out + cursor, s->ap.ssid, s->ap.ssid_length);
    cursor += s->ap.ssid_length;
    out[cursor++] = 1U;
    out[cursor++] = (uint8_t)first;
    memcpy(out + cursor, s->ap.rates, first);
    cursor += first;
    if (remaining) {
        out[cursor++] = 50U;
        out[cursor++] = (uint8_t)remaining;
        memcpy(out + cursor, s->ap.rates + first, remaining);
        cursor += remaining;
    }
    if (s->ap.security == RTW_R12AU_SECURITY_WPA2_PSK_CCMP) {
        static const uint8_t rsn_ie[22] = {
            48U, 20U, 1U, 0U,
            0x00U, 0x0fU, 0xacU, 4U,
            1U, 0U, 0x00U, 0x0fU, 0xacU, 4U,
            1U, 0U, 0x00U, 0x0fU, 0xacU, 2U,
            0U, 0U
        };
        memcpy(out + cursor, rsn_ie, sizeof(rsn_ie));
        cursor += sizeof(rsn_ie);
    }
    if (cursor != needed) return RTW_R12AU_JOIN_BAD_ARGUMENT;
    *written = cursor;
    s->state = RTW_R12AU_JOIN_ASSOC_SENT;
    return RTW_R12AU_JOIN_OK;
}
rtw_r12au_join_error rtw_r12au_join_assoc_response(
    rtw_r12au_join_session *s, const uint8_t *p, size_t n) {
    rtw_r12au_join_error e;
    uint16_t aid, capability, returned_rates = 0U;
    size_t cursor;
    unsigned rates_seen = 0U, ext_rates_seen = 0U;
    int band_5ghz;
    if (!s) return RTW_R12AU_JOIN_BAD_ARGUMENT;
    if (s->state != RTW_R12AU_JOIN_ASSOC_SENT)
        return RTW_R12AU_JOIN_BAD_STATE;
    e = verify_management(s, p, n, 0x10U, 30U);
    if (e != RTW_R12AU_JOIN_OK) return e;
    s->status = read16(p + 26U);
    if (s->status) {
        s->state = RTW_R12AU_JOIN_REJECTED;
        return RTW_R12AU_JOIN_REJECTED_BY_AP;
    }
    capability = read16(p + 24U);
    /* The AP must still represent the ESS and security policy we selected.
     * Association without this check could appear to succeed after a BSS
     * security change, before a handshake verifies any cryptographic key. */
    if (!(capability & 0x0001U) || (capability & 0x0002U) ||
        ((capability ^ s->ap.capability) & 0x0010U))
        return RTW_R12AU_JOIN_MALFORMED;
    aid = read16(p + 28U);
    if ((aid & 0xc000U) != 0xc000U ||
        !(aid & 0x3fffU) || (aid & 0x3fffU) > 2007U)
        return RTW_R12AU_JOIN_MALFORMED;
    band_5ghz = s->ap.primary_channel > 14U;
    cursor = 30U;
    while (cursor < n) {
        uint8_t id, len;
        size_t j;
        if (n - cursor < 2U) return RTW_R12AU_JOIN_MALFORMED;
        id = p[cursor];
        len = p[cursor + 1U];
        cursor += 2U;
        if (len > n - cursor) return RTW_R12AU_JOIN_MALFORMED;
        if (id == 1U || id == 50U) {
            if ((id == 1U && (++rates_seen != 1U || !len || len > 8U)) ||
                (id == 50U && (++ext_rates_seen != 1U || !len)))
                return RTW_R12AU_JOIN_MALFORMED;
            for (j = 0U; j < len; ++j) {
                size_t k;
                const uint8_t rate = (uint8_t)(p[cursor + j] & 127U);
                int negotiated = 0;
                if (!rate_allowed(p[cursor + j], band_5ghz))
                    return RTW_R12AU_JOIN_MALFORMED;
                for (k = 0U; k < s->ap.rate_count; ++k) {
                    if ((s->ap.rates[k] & 127U) == rate) {
                        if (returned_rates & (uint16_t)(1U << k))
                            return RTW_R12AU_JOIN_MALFORMED;
                        returned_rates |= (uint16_t)(1U << k);
                        negotiated = 1;
                        break;
                    }
                }
                if (!negotiated) return RTW_R12AU_JOIN_MALFORMED;
            }
        }
        cursor += len;
    }
    if (!rates_seen || !returned_rates) return RTW_R12AU_JOIN_MALFORMED;
    s->aid = (uint16_t)(aid & 0x3fffU);
    s->state = (s->ap.security == RTW_R12AU_SECURITY_WPA2_PSK_CCMP)
        ? RTW_R12AU_JOIN_WPA2_KEYS_REQUIRED
        : RTW_R12AU_JOIN_OPEN_ASSOCIATED_UNVERIFIED;
    return RTW_R12AU_JOIN_OK;
}
rtw_r12au_join_error rtw_r12au_join_disconnect(
    rtw_r12au_join_session *s, const uint8_t *p, size_t n) {
    rtw_r12au_join_error e;
    if (!s) return RTW_R12AU_JOIN_BAD_ARGUMENT;
    if (s->state != RTW_R12AU_JOIN_AUTHENTICATED &&
        s->state != RTW_R12AU_JOIN_ASSOC_SENT &&
        s->state != RTW_R12AU_JOIN_OPEN_ASSOCIATED_UNVERIFIED &&
        s->state != RTW_R12AU_JOIN_WPA2_KEYS_REQUIRED)
        return RTW_R12AU_JOIN_BAD_STATE;
    if (!p || n < 26U || (p[0] != 0xa0U && p[0] != 0xc0U))
        return RTW_R12AU_JOIN_MALFORMED;
    e = verify_management(s, p, n, p[0], 26U);
    if (e != RTW_R12AU_JOIN_OK) return e;
    if (!read16(p + 24U)) return RTW_R12AU_JOIN_MALFORMED;
    s->status = read16(p + 24U);
    s->aid = 0U;
    s->state = RTW_R12AU_JOIN_DISCONNECTED;
    return RTW_R12AU_JOIN_OK;
}
void rtw_r12au_join_expire(rtw_r12au_join_session *s) {
    if (!s) return;
    s->aid = 0U;
    s->status = 0U;
    s->state = RTW_R12AU_JOIN_DISCONNECTED;
}
