/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/FamilyRadioPolicy.h"
#include <string.h>

/* Compare address spans without adding lengths to pointers (overflow-safe).
 * Read-only caller inputs must never be cleared through a result alias. */
static int rtw_output_overlaps_input(const void *out, size_t out_len,
                                     const void *input, size_t input_len) {
    uintptr_t a, b;
    if (!out || !input || !out_len || !input_len) return 0;
    a=(uintptr_t)out; b=(uintptr_t)input;
    return a<=b ? b-a<out_len : a-b<input_len;
}

rtw_family_radio_result rtw_family_radio_prepare(
    rtw_wifi_family family, uint8_t primary_channel,
    uint8_t center_channel, uint8_t width_mhz,
    rtw_r12au_channel_plan *out) {
    rtw_r12au_channel_request request;
    rtw_r12au_channel_plan candidate;
    rtw_wifi_radio_capabilities caps;
    if (out) memset(out, 0, sizeof(*out));
    if (!out) return RTW_FAMILY_RADIO_INVALID_ARGUMENT;
    caps = rtw_wifi_radio_family_capabilities(family);
    /* Family geometry never authorizes RF writes, even for chips with an
     * integrated station. A candidate ID must still pass descriptor and
     * silicon checks before any live kernel path is selected. */
    if (primary_channel > 14U) {
        if (!(caps.family_bands & RTW_WIFI_BAND_5GHZ) ||
            width_mhz > caps.family_max_5g_width_mhz)
            return RTW_FAMILY_RADIO_UNSUPPORTED;
    } else if (!(caps.family_bands & RTW_WIFI_BAND_2GHZ) ||
               width_mhz > caps.family_max_2g_width_mhz)
        return RTW_FAMILY_RADIO_UNSUPPORTED;
    /* 2.4-GHz 40-MHz layouts are intentionally not modeled yet.
     * 20-MHz preflight is enough to reject an unsafe first bring-up. */
    request.chip = caps.family_max_nss != 1U ?
        RTW_R12AU_CHIP_8812AU : RTW_R12AU_CHIP_8821AU;
    request.primary_channel = primary_channel;
    request.center_channel = center_channel;
    request.width_mhz = width_mhz;
    if (rtw_r12au_channel_prepare(&request, &candidate) !=
        RTW_R12AU_CHANNEL_OK)
        return RTW_FAMILY_RADIO_UNSUPPORTED;
    candidate.rf_chains = caps.family_max_nss;
    candidate.passive_only = 1U;
    *out = candidate;
    return RTW_FAMILY_RADIO_OK;
}

rtw_family_radio_result rtw_family_radio_prepare_profile(
    const rtw_usb_wifi_profile *profile, uint8_t primary_channel,
    uint8_t center_channel, uint8_t width_mhz,
    rtw_r12au_channel_plan *out) {
    /* Reject overlap before clearing the result or reading a catalog
     * identity that could be overwritten through out. */
    if (out && profile && rtw_output_overlaps_input(
            out,sizeof(*out),profile,sizeof(*profile)))
        return RTW_FAMILY_RADIO_INVALID_ARGUMENT;
    const rtw_wifi_radio_capabilities caps =
        rtw_wifi_radio_profile_capabilities(profile);
    if (out) memset(out, 0, sizeof(*out));
    if (!out) return RTW_FAMILY_RADIO_INVALID_ARGUMENT;
    if (!caps.identity_verified) return RTW_FAMILY_RADIO_UNSUPPORTED;
    if (!(caps.family_bands & (primary_channel > 14U ? RTW_WIFI_BAND_5GHZ : RTW_WIFI_BAND_2GHZ)))
        return RTW_FAMILY_RADIO_UNSUPPORTED;
    /* profile can be dereferenced only after catalog pointer validation. */
    return rtw_family_radio_prepare(profile->family, primary_channel,
                                    center_channel, width_mhz, out);
}
