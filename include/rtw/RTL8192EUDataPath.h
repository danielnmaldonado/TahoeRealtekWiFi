/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_92E_DATA_PATH_H
#define RTW_92E_DATA_PATH_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_92E_TX_DESCRIPTOR_BYTES 40U
#define RTW_TRANSFER_SLOTS 16U
/* External serialization required. Completion is recorded before a slot can
 * be reclaimed; stop blocks new submissions but still accepts completions. */
typedef struct {
    uint32_t pending, completed;
    uint8_t stopping, order_count, order[RTW_TRANSFER_SLOTS];
} rtw_transfer_ledger;
void rtw_transfer_reset(rtw_transfer_ledger*);
int rtw_transfer_begin(rtw_transfer_ledger*, unsigned slot);
int rtw_transfer_complete(rtw_transfer_ledger*, unsigned slot);
int rtw_transfer_take(rtw_transfer_ledger*, unsigned slot);
/* Selected slots retain submission order. An incomplete oldest selected slot
 * blocks later ones; slots outside the mask do not block independent queues. */
int rtw_transfer_oldest_ready(const rtw_transfer_ledger*, uint32_t mask);
int rtw_transfer_take_oldest(rtw_transfer_ledger*, uint32_t mask, unsigned *slot);
void rtw_transfer_stop(rtw_transfer_ledger*);
int rtw_transfer_restart(rtw_transfer_ledger*);
int rtw_transfer_drained(const rtw_transfer_ledger*);
/* RTL8192EU generation-2 descriptor. Software-encrypted MPDUs retain their
 * complete IV/MIC. This encoder NEVER requests hardware encryption, sequence
 * replacement or hardware encryption. */
int rtw_92e_make_tx_descriptor(size_t length, int data_unicast, int multicast,
    int b_only, uint8_t rate, uint16_t sequence,
    uint8_t descriptor[RTW_92E_TX_DESCRIPTOR_BYTES]);
/* Generation-2 HT20/HT40 options. Firmware rate selection requires a configured
 * unicast station; aggregation additionally requires an agreed QoS BA session. */
#define RTW_92E_HT_TX_OPTIONS 1
typedef struct {
    uint8_t data, multicast, raid, rate, qsel, firmware_rate;
    uint8_t ampdu, density, max_agg, short_preamble;
    /* Gen2 DW5: packet width, current radio width/direction and DATA_SHORT.
     * A 20MHz packet on a 40MHz radio needs its primary DATA_SC subchannel;
     * a full-width 40MHz packet uses DATA_SC=0 in either direction. */
    uint8_t bw40, primary_above, channel_bw40, short_gi;
    uint16_t sequence;
} rtw_92e_tx_options;
int rtw_92e_make_tx_descriptor_ex(size_t, const rtw_92e_tx_options *,
    uint8_t descriptor[RTW_92E_TX_DESCRIPTOR_BYTES]);
/* Header gate before normal24-byte frame casts. Short BARs need a live HT
 * association and exact receiver/transmitter addresses; other control rejected. */
#define RTW_92E_RX_BAR_FILTER 1
#define RTW_92E_RX_ADMISSION_DIAGNOSTICS 1
/* Reasons describe the existing admission policy. They do not change whether
 * a frame is accepted, and are not a claim of RF corruption. No packet data. */
typedef enum {
    RTW_RX_ADMIT = 0,
    RTW_RX_REJECT_LENGTH,
    RTW_RX_REJECT_VERSION,
    RTW_RX_REJECT_HEADER,
    RTW_RX_REJECT_CONTROL,
    RTW_RX_REJECT_BAR_AUTH,
    RTW_RX_REJECT_BAR_CONTEXT,
    RTW_RX_REJECT_BAR_FLAGS,
    RTW_RX_REJECT_BAR_RECEIVER,
    RTW_RX_REJECT_BAR_TRANSMITTER,
    RTW_RX_REJECT_RATE,
    RTW_RX_REJECT_REASON_COUNT
} rtw_rx_admission_reason;
rtw_rx_admission_reason rtw_92e_rx_frame_reason(const uint8_t *, size_t,
    int allow_bar, const uint8_t local[6], const uint8_t peer[6]);
int rtw_92e_rx_frame_allowed(const uint8_t *, size_t, int allow_bar,
    const uint8_t local[6], const uint8_t peer[6]);
/* Pure consume-only match for an ordinary single-TID Block Ack. The caller
 * supplies a complete normalized MPDU without FCS, after its existing wire,
 * CRC/ICV and security checks, with a live authorized HT context. This does
 * not accept a frame through the old admission policy or interpret bitmap
 * bytes as TX ACKs. Only the conservative FC1==0 form is recognized. */
#define RTW_92E_RX_BA_CONSUME 1
int rtw_92e_rx_ba_consumable(const uint8_t *, size_t, int authorized_ht,
    const uint8_t local[6], const uint8_t peer[6]);
/* Protection mode: 0=none, 1=RTS/CTS, 2=CTS-to-self. DW3 fields on92E. */
int rtw_92e_tx_protection(unsigned mode, uint8_t descriptor[RTW_92E_TX_DESCRIPTOR_BYTES]);
/* USB gen2 C2H ID 3: 2-byte event header and 8-byte CCX report. This
 * station port accepts only BSS MACID 0, EDCA queues and unicast. */
typedef struct {
    uint8_t event_sequence, retries, final_rate, lifetime_expired, retry_over;
    uint16_t queue_time; /* units of 256 microseconds */
} rtw_92e_tx_report;
int rtw_92e_parse_tx_report(const uint8_t *, size_t, rtw_92e_tx_report *);
/* Request genuine RF completion feedback; never marks a USB completion ACKed. */
int rtw_92e_tx_report_request(int, uint8_t descriptor[RTW_92E_TX_DESCRIPTOR_BYTES]);
/* Beacon-loss policy for monotonic milliseconds: ten beacon intervals,
 * bounded to 2..10 seconds. A clock discontinuity fails closed. */
uint64_t rtw_92e_beacon_timeout_ms(uint16_t interval_tu);
int rtw_92e_beacon_expired(uint64_t now_ms, uint64_t last_ms, uint16_t interval_tu);
#ifdef __cplusplus
}
#endif
#endif
