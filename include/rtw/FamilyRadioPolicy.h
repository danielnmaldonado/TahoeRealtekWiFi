/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_FAMILY_RADIO_POLICY_H
#define RTW_FAMILY_RADIO_POLICY_H
#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTL8812AURadio.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Offline, conservative channel geometry for ALL KNOWN catalog families.
 * Valid output ALWAYS remains passive_only and never grants TX, DFS,
 * regulatory permission, or access to chipset registers. The generic 8812AU
 * geometry validator does NOT program the target family RF/PHY. 2.4-GHz 40MHz
 * geometry remains unsupported even when hardware advertises HT40. */
typedef enum {
    RTW_FAMILY_RADIO_OK = 0,
    RTW_FAMILY_RADIO_UNSUPPORTED,
    RTW_FAMILY_RADIO_INVALID_ARGUMENT
} rtw_family_radio_result;

rtw_family_radio_result rtw_family_radio_prepare(
    rtw_wifi_family family, uint8_t primary_channel,
    uint8_t center_channel, uint8_t width_mhz,
    rtw_r12au_channel_plan *out);

/* Catalog-scoped OFFLINE geometry, usable by any of the 270 known USB IDs.
 * Requires the exact immutable, VERIFIED profile pointer: unknown/unverified
 * identity cannot be promoted to a supported radio merely by claiming a
 * chipset family. The result is ALWAYS receive-only/non-operational. */
rtw_family_radio_result rtw_family_radio_prepare_profile(
    const rtw_usb_wifi_profile *profile, uint8_t primary_channel,
    uint8_t center_channel, uint8_t width_mhz,
    rtw_r12au_channel_plan *out);

#ifdef __cplusplus
}
#endif
#endif
