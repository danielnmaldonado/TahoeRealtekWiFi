/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_RX_H
#define RTW_RTW88_RX_H
#include "rtw/RTW88Station.h"
#include "rtw/USBPackets.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {rtw_wifi_family family;uint8_t rf_paths,rfe_option,cck_gain_low,cck_gain_high,current_width;} rtw88_rx_phy_config;
typedef struct {int8_t signal_dbm,path_dbm[2];uint8_t rssi,bandwidth,channel,measured_paths;} rtw88_rx_phy_sample;
/* Same pinned RX24 wire layout as8814A,distinct page0/page1 PHY reports.
 * Output MPDUs include FCS; caller strips exactly4 bytes for normal frames.
 * Foreign rate frames remain structurally decodable for aggregate continuity. */
rtw_packet_error rtw88_rx_next(const rtw88_rx_phy_config*,rtw_rx_iterator*,rtw_8814au_rx_frame*);
int rtw88_rx_parse_phy(const rtw88_rx_phy_config*,const rtw_rx_packet*,rtw88_rx_phy_sample*);
/* C2H RA0x0c:2-byte header +7-byte payload. Validated HT20 station telemetry,
 * never a per-packet ACK/retry outcome. Returns1=RA,0=ignored,-1=malformed. */
int rtw88_rx_parse_rate_report(const uint8_t*,size_t,const rtw_8814au_rate_config*,uint8_t*rate);
#ifdef __cplusplus
}
#endif
#endif
