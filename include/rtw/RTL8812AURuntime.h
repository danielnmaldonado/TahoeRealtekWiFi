/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTL8812AU_RUNTIME_H
#define RTW_RTL8812AU_RUNTIME_H
#include "rtw/RTL8812AUMAC.h"
#include "rtw/RTL8812AUDataPath.h"
#include "rtw/RTL8821AUCoex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {uint32_t mask;uint8_t rate_id,initial_rate,ht,vht,bandwidth_mhz,short_gi,streams;} rtw_r12au_rate_config;
typedef rtw_r12au_rate_config rtw_r12au_legacy_rate; /* Source-compatible legacy API. */
int rtw_r12au_build_rate_config(uint16_t legacy,const uint8_t ht_mcs[2],uint16_t vht_rx_map,
 unsigned physical_streams,rtw_r12au_mode,unsigned bandwidth_mhz,int band_5g,int local_sgi,int peer_sgi,rtw_r12au_rate_config*);
int rtw_r12au_make_tx(const rtw_r12au_tx_options*,const rtw_r12au_rate_config*,uint8_t descriptor[40]);
int rtw_r12au_rate_selected(const rtw_r12au_rate_config*,uint8_t hardware_rate);

typedef struct {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop;} rtw_r12au_edca_ac;
/* Saturated, valid reports retain100% RSSI and clip dBm to-1. Zero is
 * reserved for unavailable measurements in the common station cache. */
typedef struct {int signal_dbm;uint8_t rssi_percent;} rtw_r12au_phy_sample;
/* The operating PHY has only verified legacy20 power. Mask builders cannot
 * request HT/VHT, SGI, wider channels, a foreign MACID or a8814 high-mask H2C. */
int rtw_r12au_build_legacy_rate(uint16_t legacy_mask,int band_5g,rtw_r12au_legacy_rate *);
int rtw_r12au_make_rate_command(const rtw_r12au_legacy_rate *,int reset,uint8_t command[8]);
/* Legacy-only TX40 builder keeps descriptor RAID consistent with the exact
 * firmware mask, including OFDM-only peers on2.4GHz. */
int rtw_r12au_make_legacy_tx(const rtw_r12au_tx_options *,const rtw_r12au_legacy_rate *,uint8_t descriptor[40]);
int rtw_r12au_make_media_command(int connected,uint8_t command[8]);
int rtw_r12au_make_rssi_command(unsigned rssi,uint8_t command[8]);
/* Returns1 for a matching MACID0 legacy RA report,0 for unrelated C2H and-1
 * for malformed/unsupported matching reports. Firmware RA is telemetry only;
 * no packet ACK/retry result is invented. The payload includes C2H ID/sequence. */
int rtw_r12au_parse_rate_report(const uint8_t *,size_t,const rtw_r12au_legacy_rate *,uint8_t *rate);
/* No channel/RFK or TX resume here. All params are checked before bus writes. */
int rtw_r12au_station_rates(rtw_r12au_boot *,uint16_t basic_rates,int short_slot,int band_5g);
int rtw_r12au_station_edca(rtw_r12au_boot *,int short_slot,int band_5g,const rtw_r12au_edca_ac[4]);
/* Pinned Jaguar PHY format; actual chip decides the CCK gain mapping. */
int rtw_r12au_decode_phy_sample(rtw_r12au_chip,unsigned rf_paths,int cck_high_power,
    uint8_t rate,const uint8_t *,size_t,rtw_r12au_phy_sample *);
#ifdef __cplusplus
}
#endif
#endif
