/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_92E_HT_H
#define RTW_92E_HT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_92E_H2C_MEDIA_STATUS 0x01U
#define RTW_92E_H2C_MACID_CONFIG 0x40U
#define RTW_92E_MACID_CONFIG_BYTES 7U
#define RTW_92E_MEDIA_STATUS_BYTES 3U
/* Generation-2 HT20/HT40 rate table. MCS occupy bits 12..27.
 * MCS32, VHT, DISRA and DISPT are not implemented. */
typedef struct {
    uint32_t rate_mask;
    uint8_t raid, sgi20, ht, tx_streams, initial_rate;
    uint8_t bw40, sgi40; /* 40 MHz is sent to gen2 H2C; SGI is width-specific. */
} rtw_92e_rate_config;
/* legacy_rates is the peer's negotiated hardware-index mask (CCK0..3,
 * OFDM4..11). peer_mcs describes peer receive support; tx_chains describes
 * local transmit chains, independently of the local receive chain count.
 * ht_negotiated must reflect the completed association, not a beacon IE. */
int rtw_92e_build_rate_config(uint16_t legacy_rates,
    const uint8_t peer_mcs[2], unsigned tx_chains, int ht_negotiated,
    int local_sgi20, int peer_sgi20, rtw_92e_rate_config *);
/* Width must come from the completed channel-width negotiation, never just
 * from the AP HT capability IE. SGI40 is separate from SGI20. */
int rtw_92e_build_rate_config_width(uint16_t legacy_rates,
    const uint8_t peer_mcs[2], unsigned tx_chains, int ht_negotiated,
    int local_sgi20, int peer_sgi20, int ht40,
    int local_sgi40, int peer_sgi40, rtw_92e_rate_config *);
/* Payloads exclude the H2C ID. MACID 0 is the AP peer for this STA port.
 * Media role is AP (2), as in rtl8xxxu_gen2_report_connect(). */
int rtw_92e_make_macid_config(const rtw_92e_rate_config *, uint8_t macid,
    uint8_t out[RTW_92E_MACID_CONFIG_BYTES]);
int rtw_92e_make_media_status(int connect, uint8_t macid,
    uint8_t out[RTW_92E_MEDIA_STATUS_BYTES]);
#ifdef __cplusplus
}
#endif
#endif
