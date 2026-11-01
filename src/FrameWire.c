#include "rtw/FrameWire.h"

static int equal_bytes(const uint8_t* a, const uint8_t* b, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) if (a[i] != b[i]) return 0;
    return 1;
}
static void copy_bytes(uint8_t* to, const uint8_t* from, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) to[i] = from[i];
}
static void zero_bytes(uint8_t* to, size_t n) {
    size_t i;
    for (i = 0; i < n; ++i) to[i] = 0;
}
static int is_unicast(const uint8_t addr[6]) {
    size_t i;
    if (addr[0] & 1U) return 0;
    for (i = 0; i < 6; ++i) if (addr[i]) return 1;
    return 0;
}

rtw_frame_error rtw_ethernet_to_sta_mpdu(
    const uint8_t* ethernet, size_t length,
    const uint8_t station[6], const uint8_t bssid[6], uint16_t sequence,
    uint8_t* output, size_t capacity, size_t* written) {
    static const uint8_t snap[6] = {0xAA,0xAA,0x03,0x00,0x00,0x00};
    size_t need;
    uint16_t seq;
    if (written) *written = 0;
    if (!ethernet || !station || !bssid || !output || !written) return RTW_FRAME_NULL_INPUT;
    if (length < 14) return RTW_FRAME_TOO_SHORT;
    if (length > 1518) return RTW_FRAME_TOO_LARGE;
    if (!is_unicast(station) || !is_unicast(bssid) || !equal_bytes(ethernet+6, station, 6))
        return RTW_FRAME_INVALID_ADDRESS;
    if ((((uint16_t)ethernet[12] << 8) | ethernet[13]) < 0x0600)
        return RTW_FRAME_INVALID_ETHERTYPE;
    need = length + 18; // 24-byte 802.11 + 8-byte SNAP - 14-byte Ethernet
    if (capacity < need) return RTW_FRAME_BUFFER_TOO_SMALL;
    // Overlapping buffers are forbidden: no in-place conversion.
    zero_bytes(output, 24);
    output[0] = 0x08;
    output[1] = 0x01;
    copy_bytes(output+4, bssid, 6);
    copy_bytes(output+10, station, 6);
    copy_bytes(output+16, ethernet, 6);
    seq = (uint16_t)((sequence & 0x0FFFU) << 4);
    output[22] = (uint8_t)(seq & 0xFFU);
    output[23] = (uint8_t)(seq >> 8);
    copy_bytes(output+24, snap, 6);
    output[30] = ethernet[12];
    output[31] = ethernet[13];
    copy_bytes(output+32, ethernet+14, length-14);
    *written = need;
    return RTW_FRAME_OK;
}

rtw_frame_error rtw_sta_mpdu_to_ethernet(
    const uint8_t* frame, size_t length,
    const uint8_t station[6], const uint8_t bssid[6],
    int protected_and_verified, uint8_t* output, size_t capacity, size_t* written) {
    static const uint8_t snap[6] = {0xAA,0xAA,0x03,0x00,0x00,0x00};
    uint16_t fc, ethertype;
    unsigned type, subtype;
    size_t header, need;
    if (written) *written = 0;
    if (!frame || !station || !bssid || !output || !written) return RTW_FRAME_NULL_INPUT;
    if (length < 24 + 8) return RTW_FRAME_TOO_SHORT;
    if (length > 4096) return RTW_FRAME_TOO_LARGE;
    if (!is_unicast(station) || !is_unicast(bssid)) return RTW_FRAME_INVALID_ADDRESS;
    fc = (uint16_t)(frame[0] | ((uint16_t)frame[1] << 8));
    type = (fc >> 2) & 3U;
    subtype = (fc >> 4) & 15U;
    if ((fc & 0x0003U) || type != 2 || (subtype != 0 && subtype != 8) || (fc & 0x8000U))
        return RTW_FRAME_UNSUPPORTED_FRAME; // no HT Control field parsing
    if ((fc & 0x0300U) != 0x0200U) return RTW_FRAME_WRONG_DIRECTION;
    if ((fc & 0x0400U) || (frame[22] & 0x0FU)) return RTW_FRAME_FRAGMENTED;
    if ((fc & 0x4000U) && !protected_and_verified) return RTW_FRAME_ENCRYPTED_NOT_VERIFIED;
    if (!equal_bytes(frame+10, bssid, 6) ||
        (!equal_bytes(frame+4, station, 6) && !(frame[4] & 1U)))
        return RTW_FRAME_UNEXPECTED_BSSID;
    header = 24 + (subtype == 8 ? 2 : 0);
    if (length < header+8) return RTW_FRAME_TOO_SHORT;
    if (subtype == 8 && (frame[24] & 0x80U)) return RTW_FRAME_UNSUPPORTED_AGGREGATION;
    if (!equal_bytes(frame+header, snap, 6)) return RTW_FRAME_UNSUPPORTED_ENCAP;
    ethertype = (uint16_t)(((uint16_t)frame[header+6]<<8) | frame[header+7]);
    if (ethertype < 0x0600) return RTW_FRAME_INVALID_ETHERTYPE;
    need = 14 + length - header - 8;
    if (need > 1518) return RTW_FRAME_TOO_LARGE;
    if (capacity < need) return RTW_FRAME_BUFFER_TOO_SMALL;
    copy_bytes(output, frame+4, 6);
    copy_bytes(output+6, frame+16, 6);
    output[12] = frame[header+6];
    output[13] = frame[header+7];
    copy_bytes(output+14, frame+header+8, length-header-8);
    *written = need;
    return RTW_FRAME_OK;
}
