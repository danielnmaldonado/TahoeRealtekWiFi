/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192DU_DATA_PATH_H
#define RTW_RTL8192DU_DATA_PATH_H
#include "USBPackets.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_92DU_TX_DESCRIPTOR_BYTES 32U
#define RTW_92DU_RX_DESCRIPTOR_BYTES 24U
typedef struct {
    uint8_t rate,raid,qsel,firmware_rate,multicast,qos;
    uint8_t ampdu,density,max_agg,short_preamble,short_gi;
    uint8_t is_5ghz,rf_paths,packet_offset;
    uint8_t protection,rts_rate,rts_short; /*0=off,1=RTS/CTS,2=CTS-to-self*/
    uint16_t sequence;
} rtw_92du_tx_options;
/* DU-specific descriptor, not the CU encoder. HT20 MCS0..15 and SW crypto /
 * sequence only. AMPDU needs an established BA session; firmware_rate needs
 * a configured DU station rate mask. DMDP MCS7 requires negotiated SGI because
 * the primary DU driver forces short GI for that exact hardware mode/rate.
 * Explicit-rate mode disables hardware fallback to unconfigured rate masks.
 * Management QSEL=0x12; QoS EDCA QSEL=0..7; non-QoS data QSEL=0.
 * packet_offset=0/1 adds 0/8 bytes between the 32-byte descriptor and MPDU.
 * No HW crypto/ACK-report claim. */
int rtw_92du_make_tx_descriptor(size_t length,const rtw_92du_tx_options *,
    uint8_t descriptor[RTW_92DU_TX_DESCRIPTOR_BYTES]);
typedef struct {size_t mpdu_offset,total;uint8_t packet_offset;} rtw_92du_tx_layout;
/* Vendor DU single-frame 8-byte packet-offset padding avoids relying on a
 * host-specific URB_ZERO_PACKET equivalent at exact USB packet boundaries. */
int rtw_92du_tx_transfer_layout(size_t mpdu_length,uint16_t max_packet,rtw_92du_tx_layout *);
typedef struct {
    rtw_rx_packet packet;
    uint8_t security_code,sw_decrypt_required,ht,short_preamble;
    uint8_t ampdu,first_ampdu,fcs_valid;
} rtw_92du_rx_frame;
/* A single USB RX frame: pinned DU disables USB aggregation and has no
 * segregate callback. No CU 128-byte aggregate stride is assumed. Frame data
 * contains trailing FCS; output MPDU excludes it. CRC/ICV flags and computed
 * FCS are exposed, not silently accepted as traffic authorization. Hardware
 * decryption changes FCS bytes' meaning; station integration uses SW crypto.
 * PHY pointer is desc+24+SHIFT, followed by driver-info then frame+FCS.
 * Rejects nonzero trailing data (unimplemented USB aggregation). */
rtw_packet_error rtw_92du_parse_rx(const uint8_t *transfer,size_t length,
    rtw_92du_rx_frame *out);
typedef struct {int16_t dbm;uint8_t pwdb,path_pwdb[2];} rtw_92du_rx_signal;
/* Primary DU PHY signal formulas, including OFDM -106 and the board's actual
 * CCK high-power mode. pwdb is the source percentage used by H2C RSSI_REPORT. */
int rtw_92du_parse_signal(const rtw_92du_rx_frame *,uint8_t rf_paths,
    uint8_t cck_high_power,rtw_92du_rx_signal *);
#ifdef __cplusplus
}
#endif
#endif
