/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_PHY_OPERATING_H
#define RTW_RTW88_PHY_OPERATING_H
#include "rtw/RTW88PHY.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW88_PHY_RADIO_SCRATCH_BYTES 65536U
#define RTW88_PHY_REGD_WORLD 12U
typedef struct {
 uint8_t parameters_ready,channel_ready,calibrated_ready;
 uint8_t primary_channel,center_channel,width_mhz,primary_index;
 uint8_t iqk_complete,dac_complete,dpk_paths,txrx_1ss;
 int error;
} rtw88_phy_radio_status;
/* Pure channel geometry. Regulatory permission belongs to the station caller.
 * Index is rtw88 SC20:1=upper,2=lower,3=uppermost,4=lowest;20MHz uses0. */
int rtw88_phy_radio_validate_channel(uint8_t primary,uint8_t center,uint8_t width_mhz,uint8_t primary_index);
/* Scratch is caller-owned,8-byte aligned,nonoverlapping,and remains alive until
 * invalidation. Real TRX/RFE,power-by-rate/limit initialization and per-chip
 * RF prerequisites run here. World regulatory limits are the default.
 * Hardware reconfiguration and TXPAUSE restoration verify live3081 firmware;
 * cached ready/status and per-frame rate metadata remain pure. */
int rtw88_phy_radio_initialize(rtw88_phy_context*,void*scratch,size_t,uint8_t regulatory_domain);
/* Pauses TX and invalidates calibrated readiness. Subsequent power lookup
 * applies2indices/dB on21C/22B or4indices/dB on22C; underflow fails closed. */
/* Absolute SAR-style ceiling in mBm; INT32_MAX disables it. */
int rtw88_phy_radio_set_power_limit_mbm(rtw88_phy_context*,int32_t mbm);
int rtw88_phy_radio_set_power_reduction(rtw88_phy_context*,uint8_t db);
int rtw88_phy_radio_tune(rtw88_phy_context*,uint8_t primary,uint8_t center,uint8_t width_mhz,uint8_t primary_index);
int rtw88_phy_radio_calibrate(rtw88_phy_context*);
/* Inform actual MLME association solely for firmware segmented IQK. This does
 * not associate or authorize traffic. Tune clears it; caller must serialize. */
int rtw88_phy_radio_set_associated(rtw88_phy_context*,int associated);
/* Supply actual TX-report descriptor rate for rate-dependent power tracking.
 * Channel tune starts with that band's basic rate until feedback arrives. */
/* Share the long-packet sequence with startup info. Separate from the FW
 * mailbox cursor. Caller-owned pointer remains stable until invalidation. */
int rtw88_phy_radio_set_h2c_sequence(rtw88_phy_context*,uint16_t*sequence);
int rtw88_phy_radio_set_tx_rate(rtw88_phy_context*,uint8_t descriptor_rate);
/* Source rtw_set_txrx_1ss/config_trx_mode bridge for8822C coexistence.
 * Keeps the actual board antenna maps and RFK/channel tuple; the station
 * caller must also update firmware RA and notify an associated AP of SMPS. */
int rtw88_phy_radio_set_txrx_1ss(rtw88_phy_context*,int enabled);
int rtw88_phy_radio_thermal_track(rtw88_phy_context*);
rtw88_phy_radio_status rtw88_phy_radio_get_status(const rtw88_phy_context*);
int rtw88_phy_radio_ready(const rtw88_phy_context*);
void rtw88_phy_radio_invalidate(rtw88_phy_context*);
#ifdef __cplusplus
}
#endif
#endif
