/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTL8812AU_MAC_H
#define RTW_RTL8812AU_MAC_H
#include "rtw/RTL8812AUBootstrap.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Pre-firmware phase: family-correct256-entry LLT chain/ring and DMA state.
 *8812:512-byte TX pages,boundary247;8821:256-byte pages,boundary248. */
int rtw_r12au_dma_prepare(rtw_r12au_boot *);
/* Post-firmware MAC/USB configuration, including actual OUT endpoint count,
 * packet size, queue pages and mappings, RX/EDCA/beacon state. TX stays paused.
 * Hardware crypto stays disabled: CCMP is owned by the host net80211 stack.
 * This prepares MAC state; it is not RF calibration or a station/link state. */
int rtw_r12au_mac_init(rtw_r12au_boot *);
/* Programs station port0 MAC/BSSID/AID and RX filtering. associate=0 requires
 * aid0 and permits NULL BSSID. Caller supplies actual host association state;
 * this routine never authenticates and leaves TX paused for caller commit. */
int rtw_r12au_mac_station(rtw_r12au_boot *,const uint8_t bssid[6],uint16_t aid,int associate);
/* Returns the descriptor-order bulk OUT endpoint for an actual QSEL. */
int rtw_r12au_qsel_endpoint(const rtw_r12au_boot *,uint8_t qsel,uint8_t *endpoint);
#ifdef __cplusplus
}
#endif
#endif
