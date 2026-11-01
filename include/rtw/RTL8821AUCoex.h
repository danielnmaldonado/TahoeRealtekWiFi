/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTL8821AU_COEX_H
#define RTW_RTL8821AU_COEX_H
#include "rtw/RTL8812AUBootstrap.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rtw_r21au_coex rtw_r21au_coex;
typedef enum {RTW_R21CX_SCAN_START,RTW_R21CX_SCAN_FINISH,RTW_R21CX_ASSOC_START,
 RTW_R21CX_ASSOC_FINISH,RTW_R21CX_MEDIA_CONNECT,RTW_R21CX_MEDIA_DISCONNECT,
 RTW_R21CX_SWITCH_BAND} rtw_r21au_coex_event;
/* Actual monotonic clock and station/hardware statistics. */
typedef struct {
 uint64_t now_ms,tx_bytes,rx_bytes;
 uint32_t tx_packets,rx_packets,cck_ok,cck_error;
 uint16_t beacon_interval,scan_ap_count;
 uint8_t primary,center,width_mhz,connected,scanning,rssi_percent,rx_rate,tx_rate;
} rtw_r21au_coex_input;
typedef struct {
 uint8_t initialized,active,bt_disabled,bt_profiles,table,tdma,under_5g;
 uint8_t frozen,antenna_phase,share_ant,bt_rssi,bt_ble_scan_type,bt_init_scan,owner_valid;
 uint32_t policy_runs,bt_info_events,mp_timeouts,pending_work;
 int error;
} rtw_r21au_coex_status;
size_t rtw_r21au_coex_size(void);
/* Only a positively identified8821AU with active BT and valid ROM metadata.
 * Caller serializes this context with its AU bootstrap/PHY. No I/O in init;
 * firmware commands share the canonical boot->h2c_box cursor. */
int rtw_r21au_coex_init(void *,size_t,rtw_r12au_boot *,rtw_r21au_coex **);
int rtw_r21au_coex_start(rtw_r21au_coex *,const rtw_r21au_coex_input *);
/* Owner-first live firmware admission only: no policy or mailbox actions. */
int rtw_r21au_coex_health(rtw_r21au_coex *);
int rtw_r21au_coex_notify(rtw_r21au_coex *,rtw_r21au_coex_event,const rtw_r21au_coex_input *);
int rtw_r21au_coex_watchdog(rtw_r21au_coex *,const rtw_r21au_coex_input *);
/* Complete C2H header(id,sequence) plus bounded payload. ENOMSG means other
 * commands; malformed input never reaches the imported policy. */
int rtw_r21au_coex_c2h(rtw_r21au_coex *,const uint8_t *,size_t,uint64_t now_ms);
int rtw_r21au_coex_stop(rtw_r21au_coex *);
int rtw_r21au_coex_bind_phy(rtw_r21au_coex *,rtw_r12au_phy *);
void rtw_r21au_coex_forget(rtw_r21au_coex *);
void rtw_r21au_coex_get_status(const rtw_r21au_coex *,rtw_r21au_coex_status *);
#ifdef __cplusplus
}
#endif
#endif
