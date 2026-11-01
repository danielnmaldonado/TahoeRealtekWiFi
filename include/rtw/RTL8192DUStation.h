/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192DU_STATION_H
#define RTW_RTL8192DU_STATION_H
#include "RTL8192DUPHY.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Typed DU station MAC/LLT/firmware commands. All host 802.11 sequence numbers,
 * WPA2/CCMP and the controlled port remain net80211 responsibilities. No key,
 * password, protected Ethernet packet or connection authorization is accepted
 * here. Callers own/serialize the whole USB device and both internal MACs. */
typedef enum {RTW_92DU_ST_INIT=0,RTW_92DU_ST_SCAN,RTW_92DU_ST_AUTH,
    RTW_92DU_ST_ASSOC,RTW_92DU_ST_RUN} rtw_92du_station_state;
typedef struct {
    uint16_t legacy,mcs;
    uint8_t ht,sgi20,smps; /* SMPS 0=static,1=dynamic,3=disabled; 2 reserved */
} rtw_92du_peer_rates;
typedef struct {
    uint32_t mask;
    uint8_t channel,raid,initial_rate,ht,sgi20;
} rtw_92du_rate_config;
typedef struct {
    uint8_t bssid[6],channel,short_slot;
    uint16_t aid,beacon_interval,basic_rates;
} rtw_92du_bss;
typedef struct {
    rtw_92du_boot *boot;
    rtw_92du_rate_config rates;
    uint8_t out_pipes,prepared,started,next_h2c,rate_ready,joined,cleanup_done;
    uint8_t bssid[6],station_address[6];
    rtw_92du_station_state state;
} rtw_92du_mac;
/* No I/O. The immutable USB identity/endpoint plan must match the boot board. */
rtw_92du_result rtw_92du_mac_init(rtw_92du_mac *,rtw_92du_boot *,const rtw_backend_usb_plan *);
/* DMDP uses the selected physical interface's actual descriptor endpoints,
 * while preserving the immutable interface0 catalog identity. */
rtw_92du_result rtw_92du_mac_init_for_interface(rtw_92du_mac *,rtw_92du_boot *,
    const rtw_backend_usb_plan *,const uint8_t *configuration,size_t length);
/* Program the logical Ethernet station address while TX is paused and the
 * MAC is disconnected. This keeps one net80211 identity across DMDP bands;
 * the immutable PROM board/factory addresses are never modified. */
rtw_92du_result rtw_92du_mac_set_address(rtw_92du_mac *,const uint8_t address[6]);
/* Cold powered MAC: LLT only, before firmware download. TX stays paused. */
rtw_92du_result rtw_92du_mac_prepare(rtw_92du_mac *);
/* After firmware + PHY init (not BB/RF load): MAC table, queues, EDCA, 20MHz,
 * software crypto/CAM reset, host sequence and disconnected RX filters.
 * Call PHY load/channel/calibrate next. No TX unpause occurs here. */
rtw_92du_result rtw_92du_mac_start(rtw_92du_mac *);
/* DU H2C: mailbox 0x1d0+4*n; 2-byte extension 0x88+2*n; command length1..5.
 * Caller serialization replaces the source's Linux spinlock. Busy/partial
 * command failures poison the context; never retry an uncertain command. */
rtw_92du_result rtw_92du_h2c(rtw_92du_mac *,uint8_t id,const uint8_t *,uint8_t length);
rtw_92du_result rtw_92du_build_rate_config(const rtw_92du_boot *,uint8_t channel,
    const rtw_92du_peer_rates *,rtw_92du_rate_config *);
rtw_92du_result rtw_92du_mac_set_rates(rtw_92du_mac *,const rtw_92du_rate_config *);
rtw_92du_result rtw_92du_mac_set_state(rtw_92du_mac *,rtw_92du_station_state,const rtw_92du_bss *);
rtw_92du_result rtw_92du_mac_report_rssi(rtw_92du_mac *,uint8_t source_pwdb);
/* Live DU ABI: MCUFWDL.RDY set, MCUFWDL.EN clear, and the selected
 * FW_MAC0/1_READY bit set. Uses the same typed USB bank mapping as boot.
 * Failure invalidates firmware/rates/join authority and pauses TX. */
rtw_92du_result rtw_92du_mac_health(rtw_92du_mac *);
typedef struct {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop;} rtw_92du_edca_ac;
/* AC order BE/BK/VI/VO; SIFS10+slot9/20 on2G, SIFS16+slot9 on5G.
 * Invalid/overflowing parameters are rejected before any register write.
 * Host net80211 performs ACM queue admission/downgrade. */
rtw_92du_result rtw_92du_mac_edca(rtw_92du_mac *,uint8_t channel,uint8_t short_slot,
    const rtw_92du_edca_ac ac[4]);
/* Source RF-sleep + MAC/MCU/analog/USB power-off, bounded FW self-reset with
 * the source's forced reset fallback. Refuses global power-off if another
 * internal MAC is on or unreadable. Cleanup still makes bounded best-effort
 * register attempts after an earlier error, which remains the return value;
 * it never retries an uncertain H2C or restores TX. Caller retains USB
 * ownership until this synchronous cleanup and all transfers are drained. */
rtw_92du_result rtw_92du_mac_stop(rtw_92du_mac *);
#ifdef __cplusplus
}
#endif
#endif
