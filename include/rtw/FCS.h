#ifndef RTW_FCS_H
#define RTW_FCS_H
/* SPDX-License-Identifier: GPL-2.0-only
 * Portable, allocation-free IEEE 802.11 MAC FCS (CRC-32/ISO-HDLC).
 * This validates corruption only; CRC32 is NOT a cryptographic MIC.
 */
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Returns the bit-reflected 802.11 CRC32, serialized little-endian on air. */
uint32_t rtw_fcs32(const uint8_t* bytes, size_t length);
/* Frame must include four trailing FCS bytes. Returns 0 for invalid input. */
int rtw_fcs_valid(const uint8_t* frame_with_fcs, size_t length);
#ifdef __cplusplus
}
#endif
#endif
