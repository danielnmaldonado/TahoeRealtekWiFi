/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_8814AU_DATA_PATH_H
#define RTW_8814AU_DATA_PATH_H
#include "USBPackets.h"
#ifdef __cplusplus
extern "C" {
#endif

#define RTW_8814AU_TX_DESCRIPTOR_BYTES 40U
#define RTW_8814AU_RX_DESCRIPTOR_BYTES 24U
#define RTW_8814AU_H2C_COMMAND_BYTES 8U
#define RTW_8814AU_MAX_RX_MPDU 11454U
#define RTW_8814AU_C2H_MAX_BYTES 257U

/* Station MACID0, 2.4 GHz/20 MHz, legacy rates and optionally HT MCS0..15.
 * These types and firmware formats are independent of the 92E/88E codecs.
 * Bits0..11 are legacy rates, bits12..27 are HT; upper mask bits remain zero.
 * init_ra_level starts at1; the owner sets it to0 after the first RA pair. */
typedef struct {
    uint64_t mask;
    uint8_t rate_id, initial_rate, ht, sgi20, tx_streams, init_ra_level;
} rtw_8814au_rate_config;
int rtw_8814au_build_rate_config(uint16_t legacy_rates,
    const uint8_t peer_mcs[2], unsigned tx_chains, int ht_negotiated,
    int local_sgi20, int peer_sgi20, rtw_8814au_rate_config *);
/* Complete mailbox commands, including IDs0x40 and0x46. Always send BOTH,
 * including an all-zero upper mask, to clear a previous firmware table.
 * reset controls NO_UPDATE only; power training is disabled as in rtw88. */
int rtw_8814au_build_ra_commands(const rtw_8814au_rate_config *, int reset,
    uint8_t low[8], uint8_t high[8]);
int rtw_8814au_make_media_status(int connected, uint8_t command[8]);
/* RSSI is the firmware's0..100 scale, not a signed dBm value. */
int rtw_8814au_make_rssi_monitor(unsigned rssi, uint8_t command[8]);

typedef struct {
    uint8_t data, multicast, eapol, qsel, rate, firmware_rate;
    uint8_t short_preamble, short_gi, protection;
    uint8_t qos, ack_expected, fragment, ampdu, density, max_agg;
    uint16_t sequence;
} rtw_8814au_tx_options;
/* Internal station encoder. Firmware rates require a validated negotiated
 * mask and unicast data; management, multicast and EAPOL use explicit legacy
 * rates. QSEL is0..7 for unicast data and18 for management; group data may
 * use0..7 or18. Protection0/1 means
 * none/RTS-CTS. AMPDU requires firmware HT rate, unicast QoS with normal ACK,
 * no fragmentation, and a live BA agreement checked by the owner. Density
 * is0..7; max_agg is1..16 PAIRS of MPDUs, capped by the peer factor and half
 * the negotiated BA window. A one-frame window stays unaggregated. The owner
 * must program the peer HT factor at REG_AMPDU_MAX_LENGTH before accepting BA.
 * Unsupported CTS-to-self, VHT, HT40 and hardware crypto have no enabling
 * fields. All frames retain software sequence numbers and
 * complete software-encrypted IV/MIC; checksum covers the first32 bytes. */
int rtw_8814au_make_tx_descriptor(size_t,
    const rtw_8814au_tx_options *, const rtw_8814au_rate_config *, uint8_t[40]);

typedef struct {
    rtw_rx_packet packet;
    uint8_t is_c2h, security_code, sw_decrypt_required, cam_id, ppdu_count;
} rtw_8814au_rx_frame;
/* USB aggregate units are8 bytes. PHY starts at24+SHIFT; MPDU follows PHY.
 * Normal MPDUs include the FCS. Structurally valid foreign HT/VHT frames are
 * exposed so the owner can drop individual unsupported frames without losing
 * later beacons. CRC/ICV and software-decryption policy belong to the owner. */
rtw_packet_error rtw_8814au_rx_next(rtw_rx_iterator *, rtw_8814au_rx_frame *);

typedef struct {
    int8_t path_dbm[4], signal_dbm, snr_raw[4], evm_raw[4], cfo_tail_raw[2];
    uint16_t channel;
    uint8_t active_mask, measured_mask, bandwidth_code, cck, rssi, path_rssi[4];
} rtw_8814au_phy_status;
/* Four physical Jaguar paths; active_mask comes from board.antenna_rx.
 * OFDM signal follows upstream's second-lowest-of-four gain mitigation.
 * RSSI uses the literal rtw88 power/linear conversion, including all four
 * physical OFDM paths. SNR/EVM/CFO are raw signed fields.
 * Unknown CCK LNA codes have no valid dBm calibration and return failure. */
int rtw_8814au_parse_phy(const rtw_8814au_rx_frame *, uint8_t active_rx_mask,
    rtw_8814au_phy_status *);

typedef enum {
    RTW_8814AU_C2H_MALFORMED = 0,
    RTW_8814AU_C2H_OK,
    RTW_8814AU_C2H_IGNORED
} rtw_8814au_c2h_result;
typedef enum {
    RTW_8814AU_EVENT_NONE = 0,
    RTW_8814AU_EVENT_RA,
    RTW_8814AU_EVENT_RF_ON
} rtw_8814au_event_kind;
typedef struct {
    rtw_8814au_event_kind kind;
    uint8_t id, sequence, rate, short_gi, mac_id, status, ra_ratio;
} rtw_8814au_c2h_event;
/* ID0x0c has exactly six payload bytes on8814A, without the later BW byte.
 * RA reports are rate telemetry, NEVER ACK/retry reports. ID0x32 signals
 * firmware RF-on completion; it has no defined payload fields here.
 * Unknown IDs (including CCX0x03) are bounded and ignored. config is required
 * only for RA telemetry; reject other MACIDs/rates outside this station mask. */
rtw_8814au_c2h_result rtw_8814au_parse_c2h(const uint8_t *, size_t,
    const rtw_8814au_rate_config *, rtw_8814au_c2h_event *);

#ifdef __cplusplus
}
#endif
#endif
