/* FU fields cross-checked against checksum-pinned Linux rtl8xxxu
 * rxdesc24,fill_txdesc_v2 and txdesc40; this is a wire-only specialization
 * of the existing BSD-2-Clause codec, never an AU hardware context. */
/* SPDX-License-Identifier: BSD-2-Clause */
#include "rtw/RTL8192FUDataPath.h"
#include <string.h>

static uint32_t read32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) |
           ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}
static void write32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8U);
    p[2] = (uint8_t)(value >> 16U); p[3] = (uint8_t)(value >> 24U);
}
static uint16_t xor_checksum(const uint8_t d[RTW_R92FU_TX_DESC_BYTES]) {
    uint16_t result = 0;
    unsigned i;
    for (i = 0; i < 32U; i += 2U)
        result ^= (uint16_t)((unsigned)d[i] | ((unsigned)d[i + 1U] << 8U));
    return result;
}
static int zero_bytes(const uint8_t *p, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i)
        if (p[i] != 0) return 0;
    return 1;
}
int rtw_r92fu_rate_valid(uint8_t rate, unsigned chains) {
    return chains>=1 && chains<=2 && rate<12+chains*8;
}
rtw_r92fu_result rtw_r92fu_rx_next(rtw_r92fu_rx_iterator *it, unsigned chains,
                                    rtw_r92fu_rx_frame *out) {
    size_t remaining, payload_length, phy_length, shift, used, aligned;
    uint32_t w0, w1, w2, w3, w4;
    rtw_r92fu_rx_frame frame;
    const uint8_t *p;
    if (out) memset(out, 0, sizeof(*out));
    if (!it || !out || !rtw_r92fu_rate_valid(0, chains) ||
        it->length > RTW_R92FU_MAX_BULK ||
        it->cursor > it->length || (!it->buffer && it->length != 0))
        return RTW_R92FU_BAD_ARGUMENT;
    if (it->cursor == it->length) return RTW_R92FU_END;
    p = it->buffer + it->cursor;
    remaining = it->length - it->cursor;
    if (remaining < RTW_R92FU_RX_DESC_BYTES) {
        if (remaining <= 7U && zero_bytes(p, remaining)) {
            it->cursor = it->length;
            return RTW_R92FU_END;
        }
        return RTW_R92FU_TRUNCATED;
    }
    if (it->frames >= RTW_R92FU_MAX_RX_FRAMES)
        return RTW_R92FU_TOO_MANY;
    w0 = read32(p); w1 = read32(p + 4U);
    w2 = read32(p + 8U); w3 = read32(p + 12U);
    w4 = read32(p + 16U);
    payload_length = w0 & 0x3fffU;
    if (payload_length == 0U) {
        if (zero_bytes(p, remaining)) {
            it->cursor = it->length;
            return RTW_R92FU_END;
        }
        return RTW_R92FU_MALFORMED;
    }
    phy_length = ((w0 >> 16U) & 15U) * 8U;
    shift = (w0 >> 24U) & 3U;
    if (phy_length && phy_length != 32U)
        return RTW_R92FU_MALFORMED;
    if (((w0 & 0x04000000U) != 0U) != (phy_length != 0U))
        return RTW_R92FU_MALFORMED;
    if (phy_length > remaining - RTW_R92FU_RX_DESC_BYTES)
        return RTW_R92FU_TRUNCATED;
    if (shift > remaining - RTW_R92FU_RX_DESC_BYTES - phy_length)
        return RTW_R92FU_TRUNCATED;
    if (payload_length > remaining - RTW_R92FU_RX_DESC_BYTES -
                         phy_length - shift)
        return RTW_R92FU_TRUNCATED;
    if (w2 & 0x10000000U) {
        if (payload_length < 2U || payload_length > RTW_R92FU_MAX_C2H ||
            phy_length || shift) return RTW_R92FU_MALFORMED;
    } else if (payload_length < 4U ||
               payload_length > RTW_R92FU_MAX_MPDU ||
               !rtw_r92fu_rate_valid((uint8_t)(w3 & 0x7fU), chains) ||
               ((w4 >> 4U) & 3U) == 3U)
        return RTW_R92FU_MALFORMED;
    used = RTW_R92FU_RX_DESC_BYTES + phy_length + shift + payload_length;
    aligned = (used + 7U) & ~(size_t)7U;
    if (aligned > remaining && used != remaining)
        return RTW_R92FU_TRUNCATED;
    memset(&frame, 0, sizeof(frame));
    frame.phy = phy_length ? p + RTW_R92FU_RX_DESC_BYTES : NULL;
    frame.phy_length = phy_length;
    frame.payload = p + RTW_R92FU_RX_DESC_BYTES + phy_length + shift;
    frame.payload_length = payload_length;
    frame.timestamp = read32(p + 20U);
    frame.cipher = (uint8_t)((w0 >> 20U) & 7U);
    frame.crc_error = (uint8_t)((w0 >> 14U) & 1U);
    frame.icv_error = (uint8_t)((w0 >> 15U) & 1U);
    frame.sw_decrypt = (uint8_t)((w0 >> 27U) & 1U);
    frame.is_c2h = (uint8_t)((w2 >> 28U) & 1U);
    frame.ampdu = (uint8_t)((w1 >> 15U) & 1U);
    frame.rate = (uint8_t)(w3 & 0x7fU);
    frame.bandwidth_mhz = (uint8_t)(20U << ((w4 >> 4U) & 3U));
    frame.short_gi = (uint8_t)(w4 & 1U);
    frame.ldpc = (uint8_t)((w4 >> 1U) & 1U);
    if (frame.is_c2h) {
        frame.c2h_id = frame.payload[0];
        frame.c2h_sequence = frame.payload[1];
    }
    *out = frame;
    it->cursor += aligned <= remaining ? aligned : used;
    ++it->frames;
    return RTW_R92FU_FRAME;
}
static int one_bit(uint8_t value) { return value <= 1U; }
static int valid_tx(const rtw_r92fu_tx_options *o) {
    if (!o || o->band_5ghz || o->bandwidth_mhz==80 || o->payload_bytes < 24U || o->payload_bytes > RTW_R92FU_MAX_MPDU ||
        o->sequence > 4095U || !rtw_r92fu_rate_valid(o->rate, o->rf_chains) ||
        !one_bit(o->data) || !one_bit(o->multicast) ||
        !one_bit(o->qos) || !one_bit(o->eapol) ||
        !one_bit(o->firmware_rate) || !one_bit(o->band_5ghz) ||
        !one_bit(o->short_gi) || !one_bit(o->short_preamble) ||
        !one_bit(o->ampdu) || o->density > 7U || o->max_agg > 31U ||
        !one_bit(o->ack_expected) || o->retry_limit > 63U ||
        o->protection > 2U || o->rts_rate > 11U ||
        (o->bandwidth_mhz != 20U && o->bandwidth_mhz != 40U &&
         o->bandwidth_mhz != 80U))
        return 0;
    if (o->ampdu && (!o->data || o->multicast || !o->qos || o->eapol ||
        !o->firmware_rate || !o->ack_expected || o->rate < 12U || !o->max_agg)) return 0;
    if (!o->ampdu && (o->density || o->max_agg)) return 0;
    if (o->multicast && (o->firmware_rate || o->ack_expected ||
                         o->protection || o->eapol))
        return 0;
    if (o->qos && (!o->data || o->multicast || o->tid > 7U))
        return 0;
    if (!o->qos && o->tid) return 0;
    if (o->eapol && (!o->data || o->multicast || o->firmware_rate ||
                      o->rate > 11U || o->bandwidth_mhz != 20U))
        return 0;
    if (o->firmware_rate && (!o->data || o->multicast || o->eapol))
        return 0;
    if (!o->data && (o->qos || o->firmware_rate || o->eapol ||
                      o->rate > 11U || o->bandwidth_mhz != 20U))
        return 0;
    if (o->band_5ghz && o->rate <= 3U) return 0;
    if (o->rate < 12U && o->bandwidth_mhz != 20U) return 0;
    if (o->bandwidth_mhz == 80U && (o->rate < 44U || !o->band_5ghz))
        return 0;
    if (o->rate >= 44U && !o->band_5ghz) return 0;
    if (o->bandwidth_mhz == 20U && (o->rate == 53U || o->rate == 63U)) return 0;
    if (o->short_gi && (o->rate < 12U || o->short_preamble))
        return 0;
    if (o->short_preamble && (o->rate < 1U || o->rate > 3U))
        return 0;
    if ((o->protection && (!o->data || o->multicast ||
                           o->rts_rate >= 12U)) ||
        (!o->protection && o->rts_rate))
        return 0;
    if (!o->ack_expected && o->retry_limit) return 0;
    if (o->multicast && o->rate >= 12U) return 0;
    return 1;
}
int rtw_r92fu_tx_descriptor(const rtw_r92fu_tx_options *o,
                            uint8_t out[RTW_R92FU_TX_DESC_BYTES]) {
    uint16_t check;
    uint32_t w0, w1, w2, w3, w4, w5;
    uint8_t raid;
    if (out) memset(out, 0, RTW_R92FU_TX_DESC_BYTES);
    if (!out || !valid_tx(o)) return 0;
    if (o->rate >= 44U)
        raid = (uint8_t)(o->rf_chains == 2U ? 9U : 10U);
    else if (o->rate >= 12U) {
        if (o->band_5ghz) raid = (uint8_t)(o->rf_chains == 2U ? 4U : 5U);
        else if (o->bandwidth_mhz == 40U) raid = (uint8_t)(o->rf_chains == 2U ? 0U : 1U);
        else raid = (uint8_t)(o->rf_chains == 2U ? 2U : 3U);
    } else if (o->rate <= 3U) raid = 8U;
    else raid = (uint8_t)(o->band_5ghz ? 7U : 6U);
    w0 = UINT32_C(0x0c280000) | (uint32_t)o->payload_bytes |
         (o->multicast ? UINT32_C(0x01000000) : 0U);
    w1 = ((uint32_t)raid << 16U) |
         ((uint32_t)(o->data && !o->multicast ?
                       (o->qos ? o->tid : 0U) : 18U) << 8U) |
         0U;
    w2 = o->ampdu ? 0x00001000U | ((uint32_t)o->density << 20U) :
         o->data ? 0x00010000U : 0U;
    w3 = (o->firmware_rate ? 0U : 0x00000100U) | ((uint32_t)o->max_agg << 17U);
    w4 = (uint32_t)o->rate | 0x00001f00U;
    w5 = 0U;
    if (o->ack_expected && !o->multicast)
        w4 |= 0x00020000U | ((uint32_t)o->retry_limit << 18U);
    if (o->bandwidth_mhz == 40U) w5 |= 0x00000020U;
    if (o->bandwidth_mhz == 80U) w5 |= 0x00000040U;
    if (o->short_gi || o->short_preamble) w5 |= 0x00000010U;
    if (o->protection == 1U) w3 |= 0x00000800U;
    else if (o->protection == 2U) w3 |= 0x00003000U;
    if (o->protection) w4 |= ((uint32_t)o->rts_rate << 24U) | 0x0001e000U;
    write32(out, w0); write32(out + 4U, w1); write32(out + 8U, w2);
    write32(out + 12U, w3); write32(out + 16U, w4);
    write32(out + 20U, w5);
    /* Pinned rtl8xxxu_fill_txdesc_v2 always copies the header sequence to
     * DW9. It enables HW_SEQ only for an explicit ASSIGN_SEQ request; this
     * port's net80211 caller already assigns every management/data/EAPOL
     * sequence and never delegates replacement to the hardware. */
    write32(out + 36U, (uint32_t)o->sequence << 12U);
    check = xor_checksum(out);
    out[28] = (uint8_t)check; out[29] = (uint8_t)(check >> 8U);
    return 1;
}
int rtw_r92fu_tx_descriptor_valid(
    const uint8_t out[RTW_R92FU_TX_DESC_BYTES], unsigned chains) {
    uint32_t w0, w1, w2, w3, w4, w5;
    uint8_t rate, raid, qsel;
    int multicast, data;
    if (!out || chains < 1U || chains > 2U || xor_checksum(out) != 0U)
        return 0;
    w0 = read32(out); w1 = read32(out + 4U);
    w2 = read32(out + 8U); w3 = read32(out + 12U);
    w4 = read32(out + 16U); w5 = read32(out + 20U);
    if ((w0 & 0xfeff0000U) != 0x0c280000U ||
        (w0 & 0xffffU) < 24U ||
        (w0 & 0xffffU) > RTW_R92FU_MAX_MPDU ||
        (w1 & 0xffe000c0U) ||
        (w2 & ~0x00711000U) ||
        (w3 & ~0x003e3900U) ||
        (w3 & 0x00001800U) == 0x00001800U ||
        (w4 & 0xe0000000U) ||
        (w4 & 0x00001f00U) != 0x00001f00U ||
        (w5 & ~0x00000070U) ||
        read32(out + 24U) != 0U ||
        read32(out + 32U) != 0U ||
        (read32(out + 36U) & ~0x00fff000U))
        return 0;
    rate = (uint8_t)(w4 & 127U);
    raid = (uint8_t)((w1 >> 16U) & 31U);
    qsel = (uint8_t)((w1 >> 8U) & 31U);
    multicast = (w0 & 0x01000000U) != 0U;
    data = w2 != 0U;
    if (!rtw_r92fu_rate_valid(rate, chains) ||
        ((w5 & 0x60U) == 0x60U) ||
        (w1 & 63U) != 0U)
        return 0;
    if (((w3 & 0x1000U)!=0U) != ((w3 & 0x2000U)!=0U)) return 0;
    if (!data && (rate > 11U || qsel != 18U || w5 ||
                  (w3 & ~0x00000100U) != 0U))
        return 0;
    if ((multicast && (qsel != 18U ||
                       (w3 & 0x00001800U) ||
                       (w4 & 0x00fe0000U))) ||
        (!multicast && data && qsel > 7U) ||
        (w3 & 0x00001800U && (!data || multicast ||
                           (w4 & 0x0001e000U) != 0x0001e000U)) ||
        (!(w3 & 0x00001800U) && (w4 & 0x1f01e000U)) ||
        (!(w4 & 0x00020000U) && (w4 & 0x00fc0000U)))
        return 0;
    if (w2 & 0x1000U) {
        if (!data || multicast || qsel > 7U || rate < 12U ||
            (w2 & 0x10000U) || (w3 & 0x100U) || !(w3 & 0x003e0000U) ||
            !(w4 & 0x20000U) || read32(out + 32U)) return 0;
    } else if ((w2 != 0U && w2 != 0x10000U) || (w3 & 0x003e0000U)) return 0;
    if (rate <= 3U && raid != 8U) return 0;
    if (rate >= 4U && rate <= 11U && raid != 6U && raid != 7U)
        return 0;
    if (rate >= 12U && rate <= 43U && raid > 5U) return 0;
    /* A two-path device may associate with a1SS peer. Both VHT RAIDs are
     * valid for1SS rates;2SS rates still require the2SS RAID and hardware. */
    if (rate >= 44U && ((raid != 9U && raid != 10U) ||
        (chains == 1U && raid != 10U) || (rate >= 54U && raid != 9U)))
        return 0;
    if ((w5 & 0x60U) == 0U && (rate == 53U || rate == 63U)) return 0;
    if (rate < 12U && (w5 & 0x60U)) return 0;
    if (rate >= 12U && (w5 & 0x10U) && rate > 3U &&
        !data) return 0;
    if (rate < 44U && (w5 & 0x40U)) return 0;
    return 1;
}
