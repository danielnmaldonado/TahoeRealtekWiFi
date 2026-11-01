/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_COEX_H
#define RTW_RTW88_COEX_H
#include "rtw/RTW88PHYOperating.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW88_COEX_SCRATCH_BYTES 8192U
typedef struct rtw88_coex rtw88_coex;
typedef enum {
 RTW88_CX_SCAN_START,RTW88_CX_SCAN_FINISH,RTW88_CX_ASSOC_START,
 RTW88_CX_ASSOC_FINISH,RTW88_CX_MEDIA_CONNECT,RTW88_CX_MEDIA_DISCONNECT,
 RTW88_CX_SWITCH_BAND,RTW88_CX_LPS_ENTER,RTW88_CX_LPS_LEAVE
} rtw88_coex_event;
typedef struct {
 uint64_t now_ms,tx_bytes,rx_bytes;
 uint32_t tx_packets,rx_packets;
 uint16_t beacon_interval,scan_ap_count;
 uint8_t primary,center,width_mhz,connected,scanning,rssi_percent,rx_rate,tx_rate;
} rtw88_coex_input;
typedef struct {
 uint8_t initialized,active,bt_disabled,bt_profiles,table,tdma,under_5g;
 uint8_t frozen,antenna_phase,share_ant,bt_rssi,mimo_ps,rfk_active,bt_ble_scan_type,bt_init_scan;
 uint32_t policy_runs,bt_info_events,mp_timeouts,pending_work;
 uint32_t tx_mbps,rx_mbps;
 uint64_t next_deadline_ms;
 int error;
} rtw88_coex_status;
/* The serialized station bridge implements the real8822C TRX path change,
 * canonical RA update and AP SMPS action. Never aliases an AU HAL/context. */
typedef int (*rtw88_coex_mimo_callback)(void *,int enabled);
size_t rtw88_coex_size(void);
/* Pure retained-ownership/authority check for the fast station TX gate. */
int rtw88_coex_ready(const rtw88_coex *,const rtw88_phy_context *);
/* Pure metadata admission, including known B/C RFE and exact BT antenna ROM.
 * No-BT boards do not need a runtime coexistence context. */
int rtw88_coex_board_supported(const rtw88_boot_context *);
int rtw88_coex_init(void *,size_t,rtw88_phy_context *,rtw88_coex_mimo_callback,
 void *,rtw88_coex **);
int rtw88_coex_start(rtw88_coex *,const rtw88_coex_input *);
int rtw88_coex_notify(rtw88_coex *,rtw88_coex_event,const rtw88_coex_input *);
/* Actual two-second station unicast byte/packet deltas. The policy receives
 * the pinned Mbps conversion/EWMA and real per-chip CCK CRC counters. */
int rtw88_coex_watchdog(rtw88_coex *,const rtw88_coex_input *);
int rtw88_coex_c2h(rtw88_coex *,const uint8_t *,size_t,const rtw88_coex_input *);
/* Dedicated timer drains real delayed work and10ms MP deadlines during scan,
 * association and RUN. Zero means no pending deadline. Caller serializes. */
int rtw88_coex_poll(rtw88_coex *,uint64_t now_ms);
uint64_t rtw88_coex_next_deadline(const rtw88_coex *);
/* PHY retains the primary RFK grant/H2C protocol. This guard prevents policy
 * work from changing grants during that serialized calibration interval. */
int rtw88_coex_rfk(rtw88_coex *,int active);
int rtw88_coex_stop(rtw88_coex *);
void rtw88_coex_forget(rtw88_coex *);
void rtw88_coex_get_status(const rtw88_coex *,rtw88_coex_status *);
/* Pinned rtw88 CCX V0/V1 report.0=parsed,ENOMSG=other,EINVAL=malformed.
 * Token is the already-shifted firmware6-bit token, not802.11 sequence. */
int rtw88_coex_parse_tx_status(const uint8_t *,size_t,uint8_t *token,int *acked);
#ifdef __cplusplus
}
#endif
#endif
