/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_92C_DATA_PATH_H
#define RTW_92C_DATA_PATH_H
#include "RTL8192EUDataPath.h"
#include "RTL8192EUHT.h"
#include "USBPackets.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_92C_TX_DESCRIPTOR_BYTES 32U
/* Legacy-only wrapper: mixed 11b/g=4, 11g=5, 11b=6.
 * The extended encoder implements HT20 BGN=0/GN=1, QoS, SGI and per-TID
 * AMPDU with generation-specific aggregation bits. CU data unicast can set
 * firmware_rate after H2C masks; 88E uses explicit negotiated CCX/AMRR rates.
 * Explicit rates retain software sequence numbers and complete software
 * crypto IV/MIC; hardware crypto and hardware sequence replacement stay off.
 * Always call tx_report_request after encoding to select CU/88E AGGBK bits. */
int rtw_92c_make_tx_descriptor(size_t, const rtw_92e_tx_options *, uint8_t[32]);
int rtw_92c_make_tx_descriptor_ex(size_t, int is_88e, int qos,
    const rtw_92e_tx_options *, uint8_t[32]);
int rtw_92c_build_rate_config(uint16_t, const uint8_t[2], unsigned,
    int ht, int local_sgi, int peer_sgi, rtw_92e_rate_config *);
int rtw_92c_tx_protection(unsigned mode, uint8_t[32]);
/* Only 8188EU has this CCX feedback path. Normalizes the family-specific
 * aggregation-break bit even when requested=0. CU requests must be zero. */
int rtw_92c_tx_report_request(int is_88e, int requested, uint8_t[32]);
typedef struct {
    rtw_rx_packet packet;
    uint8_t security_code, sw_decrypt_required, report_type, ht;
} rtw_92c_rx_frame;
/* Both CU and 88E use a 24-byte RX descriptor and 128-byte USB units.
 * report_type: 0=802.11, 1=88E CCX TX1, 2=88E RA TX2, 3=88E interrupt.
 * TX2 and interrupt payloads are exposed for bounded dispatch, never decoded
 * as 802.11 or as gen2 C2H. Software-decrypt is DW0 bit27, not an ACK flag. */
rtw_packet_error rtw_92c_rx_next(rtw_rx_iterator *, int is_88e, rtw_92c_rx_frame *);
typedef struct {
    rtw_92e_tx_report report;
    uint8_t packet_ok;
} rtw_88e_tx_report;
/* Exact eight-byte 88E CCX payload for MACID0/unicast/EDCA, legacy/MCS0..7.
 * packet_ok is actual RF completion, independent of the failure flags.
 * No gen2 event header/sequence exists; report.event_sequence is zero.
 * report.queue_time preserves the raw 88E field; its unit is not asserted
 * to equal the 92E queue-time unit. */
int rtw_88e_parse_tx_report(const uint8_t *, size_t, rtw_88e_tx_report *);
#ifdef __cplusplus
}
#endif
#endif
