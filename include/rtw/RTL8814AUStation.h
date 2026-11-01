/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTL8814AU_STATION_H
#define RTW_RTL8814AU_STATION_H
#include "RTL8814AU.h"
#ifdef __cplusplus
extern "C" {
#endif
enum rtw_8814au_station_state {RTW_8814AU_STATION_INIT=0,
    RTW_8814AU_STATION_SCAN,RTW_8814AU_STATION_AUTH,
    RTW_8814AU_STATION_ASSOC,RTW_8814AU_STATION_RUN};
struct rtw_8814au_station_params {
    uint8_t mac[6],bssid[6],short_slot,band_5g;
    uint16_t aid,basic_rates;
    uint8_t passive_scan,ht;
};
/* EDCA order BE,BK,VI,VO; exponent limits are validated before any I/O. */
struct rtw_8814au_edca_ac {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop;};
/* Software crypto only. Clears CAM and configures own/broadcast/multicast
 * reception with full encrypted MPDUs/FCS and no RX data until RUN. */
int rtw_8814au_station_initialize(struct rtw_8814au *,const struct rtw_8814au_station_params *);
/* Does not set PHY channel or send RA/media-status H2C. Caller must perform
 * channel/RFK and RA setup first; RUN commits port0 network type last.
 * INIT accepts null params and needs no live PHY. No AP/monitor/injection. */
int rtw_8814au_station_configure(struct rtw_8814au *,enum rtw_8814au_station_state,
    const struct rtw_8814au_station_params *);
int rtw_8814au_station_edca_band(struct rtw_8814au *,int short_slot,int band_5g,
    const struct rtw_8814au_edca_ac[4]);
int rtw_8814au_station_edca(struct rtw_8814au *,int short_slot,
    const struct rtw_8814au_edca_ac[4]);
#ifdef __cplusplus
}
#endif
#endif
