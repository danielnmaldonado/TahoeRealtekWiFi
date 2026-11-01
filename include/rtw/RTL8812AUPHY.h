/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright(c)2024 Realtek Corporation; portable adaptation of Linuxrtw88.
 * Source/license: src/RTL8812AUPHY-Upstream.json and RTL8812AUPHY-LICENSE.txt. */
#ifndef RTW_RTL8812AU_PHY_H
#define RTW_RTL8812AU_PHY_H
#include "rtw/RTL8812AURadio.h"
#include "rtw/USBRegisterIO.h"
#include "rtw/PowerSequence.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Every value must come from identified hardware/OTP. No guessed power index
 * fallback is used. This structure is separate from 8812-specific bootstrap
 * because RTL8821AU has distinct identity, one RF path and RF calibration. */
typedef struct {
    rtw_r12au_chip chip;
    uint8_t cut,rfe,crystal,thermal,rf_paths,package_type;
    uint8_t swing_2g,swing_5g,ext_pa_2g,ext_pa_5g,ext_lna_2g,ext_lna_5g;
    uint8_t gpa_type,apa_type,glna_type,alna_type,btcoex;
    uint8_t txpwr_idx[2][42];
} rtw_r12au_phy_board;
typedef struct rtw_r12au_phy rtw_r12au_phy;
typedef int (*rtw_r12au_coex_guard)(const void *,const rtw_r12au_phy_board *,const rtw_usb_rw_transport *);
/* A BT-active PHY can calibrate while paused, but TX requires a live,
 * matching typed coex context. The guard is checked again on every resume. */
int rtw_r12au_phy_bind_coex(rtw_r12au_phy *,const void *,rtw_r12au_coex_guard);
typedef struct {
    uint8_t loaded,tuned,calibrated,tx_enabled,channel,rf_paths,tx_iqk_mask,rx_iqk_mask;
    int io_error;
    uint8_t primary,center,width_mhz,mode,thermal;
    uint32_t thermal_samples,thermal_recalibrations;
} rtw_r12au_phy_status;
/* Strict shared PHY-ROM decode: actual family/adjusted cut and physicalRF
 * path count are supplied by identification. For8821,bt_function_enabled
 * must be the observedWL_BT_PWR_CTRL flag; it is not encoded inOTP. Missing
 * crystal/thermal/amplifier values are rejected instead of defaulted. The
 * primary USB RFE0xff encoding selects RFE0; each BB-swing byte carries
 * two2-bit selectors, so0xff is valid. BT-active8812 remains unsupported. BT-active8821 may prepare while paused,
 * but TX additionally requires its matching live typed coex lease.
 * Identity MAC offsets are0xD7 for8812 and0x107 for8821. No hardwareI/O. */
int rtw_r12au_phy_parse_board(rtw_r12au_chip,uint8_t cut,uint8_t rf_paths,
    unsigned bt_function_enabled,const uint8_t logical[512],rtw_r12au_phy_board *);
/* Pure board/power preflight for advertising channels before power-on.
 * No fabricated default power, hardware operations or TX authorization. */
int rtw_r12au_phy_channel_supported(const rtw_r12au_phy_board *,uint8_t channel,unsigned width_mhz);
typedef enum {RTW_R12AU_MODE_LEGACY=0,RTW_R12AU_MODE_HT=1,RTW_R12AU_MODE_VHT=2} rtw_r12au_mode;
typedef struct {uint8_t primary,center,width_mhz;rtw_r12au_mode mode;} rtw_r12au_operating;
typedef struct {uint8_t index[2][64];uint64_t valid_rates;} rtw_r12au_power_plan;
/* Pure full-mode channel/power plan. Every used OTP byte and PG/limit entry
 * must be present;20/40/80 geometry never infers a secondary channel. */
int rtw_r12au_phy_power_plan(const rtw_r12au_phy_board*,const rtw_r12au_operating*,rtw_r12au_power_plan*);
int rtw_r12au_phy_set_operating(rtw_r12au_phy*,const rtw_r12au_operating*);
size_t rtw_r12au_phy_size(void);
/* Caller supplies aligned storage and verified powered MAC/running firmware.
 * No hardware access occurs during init. Physical validation remains separate. */
int rtw_r12au_phy_init(void *storage,size_t bytes,const rtw_r12au_phy_board *,
    const rtw_usb_rw_transport *,const rtw_waiter *,rtw_r12au_phy **out);
int rtw_r12au_phy_load_tables(rtw_r12au_phy *);
/* Initial transmit-capable profile:20MHz,2.4GHz1..11 or qualified5GHz36..64,100..140,
 *149..165. TX stays paused until full RFK succeeds and caller resumes it.
 * Caller still owns regulatory authorization; successful geometry is not it. */
/* Pauses TX; the following tune/RFK applies association-owned2indices/dB
 * backoff at final TXAGC programming, including thermal reprogramming. */
/* Absolute SAR-style ceiling in mBm; INT32_MAX restores unbounded metadata. */
int rtw_r12au_phy_set_power_limit_mbm(rtw_r12au_phy*,int32_t mbm);
int rtw_r12au_phy_set_power_reduction(rtw_r12au_phy*,uint8_t db);
int rtw_r12au_phy_set_channel(rtw_r12au_phy *,uint8_t channel,unsigned width_mhz);
int rtw_r12au_phy_calibrate(rtw_r12au_phy *);
int rtw_r12au_phy_resume_tx(rtw_r12au_phy *);
/* Alternate actual meter trigger/read ticks; applies pinned chip/RFE thermal
 * swings and recalibrates at measured threshold. Invalid samples return
 * EAGAIN without creating a measurement or changing calibrated power. */
int rtw_r12au_phy_tick(rtw_r12au_phy*);
int rtw_r12au_phy_set_tx_rate(rtw_r12au_phy*,uint8_t actual_or_initial_rate);
int rtw_r12au_phy_stop(rtw_r12au_phy *);
void rtw_r12au_phy_forget(rtw_r12au_phy *);
void rtw_r12au_phy_get_status(const rtw_r12au_phy *,rtw_r12au_phy_status *);
#ifdef __cplusplus
}
#endif
#endif
