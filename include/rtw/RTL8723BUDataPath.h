/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8723BU_DATA_PATH_H
#define RTW_RTL8723BU_DATA_PATH_H
#include "RTL8723BU.h"
#include "RTL8192EU.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_8723BU_TX_DESCRIPTOR_BYTES 40U
typedef struct {
 uint8_t data,multicast,raid,rate,qsel,firmware_rate,short_preamble;
 uint16_t sequence;
} rtw_8723bu_tx_options;
/* Actual rtl8xxxu gen2 wire layout, fixed HT20/1SS, software CCMP/HWSEQ off.
 * Unicast data uses MACID2,matching this chip's station H2C/telemetry. */
int rtw_8723bu_make_tx_descriptor(size_t,const rtw_8723bu_tx_options*,uint8_t[40]);
/* Endpoint address from the actually programmed SIE/DMA queue plan. */
int rtw_8723bu_tx_endpoint(const rtw_8723bu*,uint8_t,uint8_t*);
/* Shared24-byte gen2 descriptor, then remove the explicit RCR APPEND_FCS4
 * bytes only on MPDUs. C2H payloads retain every byte. */
rtw_packet_error rtw_8723bu_rx_next(rtw_rx_iterator*,rtw_8192eu_rx_frame*);
#ifdef __cplusplus
}
#endif
#endif
