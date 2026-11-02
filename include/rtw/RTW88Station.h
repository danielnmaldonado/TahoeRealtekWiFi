/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_STATION_H
#define RTW_RTW88_STATION_H
#include "rtw/RTW88MAC.h"
#include "rtw/RTL8814AUDataPath.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
 RTW88_STATION_INIT=0,RTW88_STATION_SCAN,RTW88_STATION_AUTH,
 RTW88_STATION_ASSOC,RTW88_STATION_RUN
} rtw88_station_state;
typedef struct {
 uint8_t mac[6],bssid[6],channel,short_slot,passive_scan;
 uint16_t aid,basic_rates;
 uint8_t ht;
} rtw88_station_params;
typedef struct {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop;} rtw88_station_edca;
/* A pure snapshot of the exact board PHY and regulatory policy:
 * 0=not tuned,1=RX-only tuned,2=TX calibrated AND permitted on this channel.
 * The callback must never perform bus I/O or infer readiness from a family.
 * Boot/MAC/PHY and this context are serialized by the kernel work loop. */
typedef unsigned (*rtw88_station_radio_state)(void*,uint8_t channel);
typedef struct {
 rtw88_mac_context *mac;
 rtw88_station_radio_state radio_state;
 void *radio_cookie;
 uint8_t initialized,channel;
 rtw88_station_state state;
} rtw88_station_context;
/* B/C only. This never casts to an AU HAL or marks a PHY calibrated. It
 * requires actual identified/powered/FW/MAC readiness and clears CAM while
 * keeping hardware crypto disabled for net80211 software CCMP. */
int rtw88_station_initialize(rtw88_station_context*,rtw88_mac_context*,
 rtw88_station_radio_state,void*,const uint8_t mac[6]);
/* RUN programs port0 MAC filters/timing; it does not install keys, authorize
 * the net80211 controlled port, publish Ethernet carrier, or submit frames. */
int rtw88_station_configure(rtw88_station_context*,rtw88_station_state,
 const rtw88_station_params*);
int rtw88_station_set_edca(rtw88_station_context*,int short_slot,
 const rtw88_station_edca[4]);
/* Real four-box H2C register flow. No implicit retry after an I/O error. */
int rtw88_station_h2c(rtw88_station_context*,const uint8_t command[8]);
/* B/C firmware has one32-bit RA mask. VHT bits12..31 are MCS0..9 per
 * stream; descriptor rates44..63 map to mask bits12..31. The retained
 * rates.sgi20 member is the selected operating width's short-GI flag. */
typedef struct {
 rtw_8814au_rate_config rates;
 uint8_t command[8],vht,width_mhz;
} rtw88_station_rate;
typedef struct {uint8_t primary,center,width_mhz,primary_index;} rtw88_station_operating;
/* Conservative station geometry:2.4GHz/DFS/channel165 remain20MHz.
 * Wider operation requires every20MHz constituent in36..48 or149..161.
 * Regulatory/profile/ROM permission remains the controller's responsibility. */
int rtw88_station_operating_validate(const rtw88_station_operating*);
int rtw88_station_prepare_rate(rtw_wifi_family,uint8_t channel,unsigned tx_streams,uint16_t legacy,
 const uint8_t mcs[2],uint16_t vht_rx_map,int ht,int vht,unsigned width_mhz,
 int local_sgi,int peer_sgi,rtw88_station_rate*);
int rtw88_station_rate_selected(const rtw88_station_rate*,uint8_t descriptor_rate);
int rtw88_station_prepare_ht20(rtw_wifi_family,uint8_t channel,unsigned tx_streams,uint16_t legacy,
 const uint8_t mcs[2],int ht,int local_sgi,int peer_sgi,rtw88_station_rate*);
#ifdef __cplusplus
}
#endif
#endif
