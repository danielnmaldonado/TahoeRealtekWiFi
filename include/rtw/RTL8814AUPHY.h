/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTL8814AU_PHY_H
#define RTW_RTL8814AU_PHY_H
#include "RTL8814AU.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Caller-owned, suitably aligned storage. No allocation or USB access here. */
size_t rtw_8814au_phy_size(void);
/* Initialize after board validation, power-on, firmware and MAC initialization.
 * The size query and storage allocation may precede those hardware stages. */
int rtw_8814au_phy_init(struct rtw_8814au *, void *, size_t);
int rtw_8814au_phy_load_tables(struct rtw_8814au *);
/* Metadata only: apply an AP Power Constraint reduction on the next operating
 * channel program, using pinned chip TXGI units. A change invalidates readiness;
 * set_channel+calibrate are required before TX. Requires a bound PHY context.
 *0 resets the constraint, including after stop. Values>31 are rejected.
 * An unrepresentable per-rate reduction makes set_channel fail ERANGE before
 * bus I/O, preserving the previous hardware table; it never clips the reduction. */
int rtw_8814au_phy_set_power_constraint(struct rtw_8814au *, uint8_t reduction_db);
enum { RTW_8814AU_POWER_LIMIT_DISABLED = 127 };
/* Linux software-SAR ceiling in whole dBm, -32..31;127 disables it.
 * Metadata only, with the same bound-context/readiness rules as above.
 * Enabled: AP reduction lowers this ceiling before the SAR minimum.
 * Disabled: AP reduction retains its exact relative behavior. */
int rtw_8814au_phy_set_power_limit_dbm(struct rtw_8814au *, int8_t limit_dbm);
/* Operating20MHz paths:2.4GHz1..11 and5GHz36..64/100..140 (step4),
 *149/153/157/161/165. Channel144 has no pinned power limits. TX stays
 * paused until successful channel-specific RF calibration. */
/* Pure ROM/table support query; no bus operations,calibration or TX permission. */
int rtw_8814au_phy_channel_supported(const struct rtw_8814au*,uint8_t,unsigned);
int rtw_8814au_phy_supports_5g(const struct rtw_8814au *);
int rtw_8814au_phy_set_channel(struct rtw_8814au *, uint8_t channel, unsigned bandwidth_mhz);
/* Same-band scan hops reuse a successful 2G calibration. EAGAIN until initial
 * calibration completes. An operating-channel change must use set_channel
 * followed by calibrate before AUTH/management transmission. */
int rtw_8814au_phy_set_scan_channel(struct rtw_8814au *, uint8_t channel, unsigned bandwidth_mhz);
/* Receive-only RF tuning on the same24 backed5GHz centers at20MHz.
 * Leaves hardware TX paused and station calibration INVALID. This MUST NOT
 * be used to authorize active scanning, association or frame transmission.
 * Failure leaves the transmitter paused and the station unavailable. */
int rtw_8814au_phy_set_passive_5g_channel(struct rtw_8814au *,
    uint8_t primary_channel, unsigned bandwidth_mhz);
int rtw_8814au_phy_calibrate(struct rtw_8814au *);
/* Feed the actual descriptor legacy/HT20 rate0..27 for thermal tracking.
 * Rate0..3 (CCK) is rejected during 5GHz operation. */
int rtw_8814au_phy_set_tx_rate(struct rtw_8814au *, uint8_t hardware_rate);
int rtw_8814au_phy_tick(struct rtw_8814au *);
int rtw_8814au_phy_stop(struct rtw_8814au *);
#ifdef __cplusplus
}
#endif
#endif
