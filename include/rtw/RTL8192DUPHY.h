/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192DU_PHY_H
#define RTW_RTL8192DU_PHY_H
#include "RTL8192DUEfuse.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Actual pinned rtlwifi DU BB/AGC/RF, ROM power and IQ/LC algorithms, with
 * synchronous USB boundaries and permanent failure fencing. No station ID
 * is enabled by these APIs. Caller serializes the whole device and storage.
 * Single-MAC boards support both bands; dual-MAC boards are restricted to
 * their verified startup PHY assignment (MAC0=5G, MAC1=2G), avoiding cross-PHY
 * ownership changes. DUALMAC_SINGLEPHY is not accepted from USB PROM. */
size_t rtw_92du_phy_size(void);
typedef enum {RTW_92DU_USB_FULL=2,RTW_92DU_USB_HIGH=3} rtw_92du_usb_speed;
rtw_92du_result rtw_92du_phy_init(rtw_92du_boot *,const rtw_92du_rom *,
    void *storage,size_t storage_size,rtw_92du_usb_speed actual_usb_speed);
/* MAC table precedes DMA/EDCA setup in rtlwifi. The full PHY loader also
 * invokes this when a standalone caller did not load it earlier. */
rtw_92du_result rtw_92du_phy_load_mac_table(rtw_92du_boot *);
rtw_92du_result rtw_92du_phy_load_tables(rtw_92du_boot *);
/* 20 MHz only: 1..11 and qualified5GHz36..64,100..140,149..165; keeps TX paused. */
/* Pause before changing association-owned backoff. Subsequent tune/thermal
 * power writes apply2indices/dB; an insufficient hardware range fails closed. */
/* Absolute5GHz MODE_A ceiling in mBm; INT32_MAX disables it. */
rtw_92du_result rtw_92du_phy_set_power_limit_mbm(rtw_92du_boot*,int32_t mbm);
rtw_92du_result rtw_92du_phy_set_power_reduction(rtw_92du_boot *,uint8_t db);
rtw_92du_result rtw_92du_phy_set_channel(rtw_92du_boot *,uint8_t channel);
rtw_92du_result rtw_92du_phy_calibrate(rtw_92du_boot *);
/* Source scanning retune reuses previously captured LC curves; it skips new
 * IQK and keeps TX paused. 2G cached IQK may then be activated for probes;
 * 5G scanning remains receive-only even when an IQK cache exists. */
rtw_92du_result rtw_92du_phy_set_scan_channel(rtw_92du_boot *,uint8_t channel);
int rtw_92du_phy_receive_ready(const rtw_92du_boot *);
int rtw_92du_phy_tx_active(const rtw_92du_boot *);
uint8_t rtw_92du_phy_channel(const rtw_92du_boot *);
int rtw_92du_phy_cck_high_power(const rtw_92du_boot *);
/* Two-phase source thermal tracking: caller invokes at >=2s intervals.
 * Pauses TX while updating BB swing/RX gain or recalibrating and resumes only
 * the previously active, still-calibrated radio. Returns any USB/RFK fault. */
rtw_92du_result rtw_92du_phy_tick(rtw_92du_boot *);
/* Explicitly release TX pause only after MAC/station setup. Requires actual
 * successful channel, LC completion/curve capture and nonzero IQK matrices. */
rtw_92du_result rtw_92du_phy_activate(rtw_92du_boot *);
/* Pause MAC TX and invalidate only its active flag; preserve calibration. */
rtw_92du_result rtw_92du_phy_pause(rtw_92du_boot *);
int rtw_92du_phy_radio_ready(const rtw_92du_boot *);
/* A loaded, validated ROM/PHY implements this actual channel and mode. */
int rtw_92du_phy_supports_channel(const rtw_92du_boot *,uint8_t channel);
/* Source RF-sleep: pause, RF-A AC=0 with bounded readback, APSD/BB reset and
 * single-MAC SPS PFM. This is not the chip's full MAC/MCU/USB power-off flow. */
rtw_92du_result rtw_92du_phy_stop(rtw_92du_boot *);
#ifdef __cplusplus
}
#endif
#endif
