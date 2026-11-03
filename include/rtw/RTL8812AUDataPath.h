/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * RTL8812AU / RTL8821AU USB wire-format layer.
 *
 * Portable, allocation-free RX descriptor decoder and legacy/HT/VHT TX40
 * serializer. No firmware boot, channel, RF calibration, USB or station attach
 * occurs here; callers must not treat success as authority to transmit.
 *
 * Wire fields cross-checked with FreeBSD rtwn r12a_{tx,rx}_desc.h,
 * r12au_rx.c and rtl8192c/r92c_rx_desc.h, by Andriy Voskoboinyk and
 * Damien Bergamini (BSD-2-Clause / ISC license). See docs/SOURCES.md.
 */
#ifndef RTW_RTL8812AU_DATAPATH_H
#define RTW_RTL8812AU_DATAPATH_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum {
    RTW_R12AU_TX_DESC_BYTES = 40,
    RTW_R12AU_RX_DESC_BYTES = 24,
    RTW_R12AU_MAX_MPDU = 4096,
    RTW_R12AU_MAX_C2H = 256,
    RTW_R12AU_MAX_BULK = 65536,
    /* Shortest rounded RX24 unit is32 bytes, including a C2H payload.
     * Pinned rtw88/usb.c iterates to the buffer end without a64-unit cap. */
    RTW_R12AU_MAX_RX_FRAMES = RTW_R12AU_MAX_BULK / 32
};

typedef enum {
    RTW_R12AU_FRAME = 0,
    RTW_R12AU_END,
    RTW_R12AU_BAD_ARGUMENT,
    RTW_R12AU_TRUNCATED,
    RTW_R12AU_MALFORMED,
    RTW_R12AU_TOO_MANY
} rtw_r12au_result;

typedef struct {
    const uint8_t *buffer;
    size_t length;
    size_t cursor;
    unsigned frames;
} rtw_r12au_rx_iterator;

typedef struct {
    const uint8_t *payload;
    size_t payload_length;
    const uint8_t *phy;
    size_t phy_length;
    uint32_t timestamp;
    uint8_t rate;
    uint8_t bandwidth_mhz;
    uint8_t crc_error, icv_error, sw_decrypt, is_c2h;
    uint8_t cipher, ampdu, short_gi, ldpc;
    uint8_t c2h_id, c2h_sequence;
} rtw_r12au_rx_frame;

/* The chipset has at most two RF chains. Unlike other members of the
 * 88xx family, MCS24-31 and VHT NSS3-4 are never legal. */
int rtw_r12au_rate_valid(uint8_t hardware_rate, unsigned rf_chains);

/* The iterator is valid only while the supplied USB transfer is alive.
 * FRAME advances the cursor; a failure never advances it. */
rtw_r12au_result rtw_r12au_rx_next(
    rtw_r12au_rx_iterator *iterator, unsigned rf_chains,
    rtw_r12au_rx_frame *frame);

typedef struct {
    size_t payload_bytes;
    uint16_t sequence;
    uint8_t rate, rf_chains;
    uint8_t data, multicast, qos, tid, eapol;
    uint8_t firmware_rate, band_5ghz;
    uint8_t bandwidth_mhz, short_gi, short_preamble;
    uint8_t retry_limit, ack_expected;
    /* 0=none, 1=CTS-to-self, 2=RTS/CTS. */
    uint8_t protection, rts_rate;
    /* Caller must hold a live per-TID BA agreement for aggregation. */
    uint8_t ampdu, density, max_agg;
} rtw_r12au_tx_options;

/* Emits exactly 40 bytes of descriptors, including XOR checksum; never
 * touches a USB device. Refuses inconsistent QoS, EAPOL and RF options.
 * On failure output is cleared, not partially valid. */
int rtw_r12au_tx_descriptor(const rtw_r12au_tx_options *options,
                            uint8_t descriptor[RTW_R12AU_TX_DESC_BYTES]);

/* Returns 1 only for internally consistent, unencrypted TX40 descriptors
 * produced by this restricted serializer. */
int rtw_r12au_tx_descriptor_valid(
    const uint8_t descriptor[RTW_R12AU_TX_DESC_BYTES], unsigned rf_chains);
#ifdef __cplusplus
}
#endif
#endif
