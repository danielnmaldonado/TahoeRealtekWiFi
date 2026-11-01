#ifndef RTW_PHY_PROFILE_H
#define RTW_PHY_PROFILE_H
#include "rtw/PHYRegisters.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Complete register sequences for the pinned RTL8812AU B-cut / USB profile.
 * They come from the GPL-2.0 vendor-derived tables identified in
 * docs/ARCHITECTURE.md. These counts denote commands, not unique registers. */
typedef struct {
    size_t mac_entries, bb_entries, agc_entries, rf_a_entries, rf_b_entries;
    size_t executed; /* Successful table commands only (includes delays). */
} rtw_phy_profile_report;

/* Pure preflight of ALL arrays; no hardware callbacks and no I/O. */
rtw_phy_error rtw_phy_profile_preflight_8812au(rtw_phy_profile_report* report);
/* Writes MAC / enable RF / PHY / AGC / RF A+B / RFE3 and 2.4 GHz ch6.
 * PRECONDITIONS: verified exact device profile, firmware booted, LLT ready,
 * explicit user opt-in, and station/network interface STILL DOWN.
 * No TX calibration/802.11 STA or network link is claimed here. */
rtw_phy_error rtw_phy_profile_apply_8812au(
    const rtw_usb_rw_transport* bus, const rtw_waiter* waiter,
    rtw_phy_profile_report* report);
/* RFE3-only 2.4GHz 20MHz fixed channel tuning; refuses any other channel.
 * Future support for additional channels needs validated regulatory/calibration
 * programming per-board. Caller must already have applied RF tables. */
rtw_phy_error rtw_phy_channel6_8812au(const rtw_usb_rw_transport* bus);
#ifdef __cplusplus
}
#endif
#endif
