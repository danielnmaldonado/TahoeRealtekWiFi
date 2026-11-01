#ifndef RTW_FRAME_WIRE_H
#define RTW_FRAME_WIRE_H

// Freestanding, allocation-free wire codec. All buffers are owned by callers.
// This translation does NOT perform authentication, association, encryption,
// replay protection, aggregation, firmware TX, or USB I/O.
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_FRAME_OK = 0,
    RTW_FRAME_NULL_INPUT,
    RTW_FRAME_TOO_SHORT,
    RTW_FRAME_TOO_LARGE,
    RTW_FRAME_INVALID_ETHERTYPE,
    RTW_FRAME_INVALID_ADDRESS,
    RTW_FRAME_WRONG_DIRECTION,
    RTW_FRAME_UNEXPECTED_BSSID,
    RTW_FRAME_UNSUPPORTED_FRAME,
    RTW_FRAME_UNSUPPORTED_ENCAP,
    RTW_FRAME_UNSUPPORTED_AGGREGATION,
    RTW_FRAME_FRAGMENTED,
    RTW_FRAME_ENCRYPTED_NOT_VERIFIED,
    RTW_FRAME_BUFFER_TOO_SMALL
} rtw_frame_error;

// Input: complete Ethernet-II MAC frame excluding FCS. Max 1518 bytes.
// Output: 802.11 ToDS=1 data frame, intentionally unencrypted.
rtw_frame_error rtw_ethernet_to_sta_mpdu(
    const uint8_t* ethernet, size_t length,
    const uint8_t station[6], const uint8_t bssid[6], uint16_t sequence,
    uint8_t* output, size_t capacity, size_t* written);

// Input: infrastructure AP -> station MPDU with FCS removed.
// protected_and_verified=true MUST be set only if external RX hardware/stack
// already verified MIC/ICV, decrypted, and enforced replay checks.
rtw_frame_error rtw_sta_mpdu_to_ethernet(
    const uint8_t* frame, size_t length,
    const uint8_t station[6], const uint8_t bssid[6],
    int protected_and_verified, uint8_t* output, size_t capacity, size_t* written);

#ifdef __cplusplus
}
#endif
#endif
