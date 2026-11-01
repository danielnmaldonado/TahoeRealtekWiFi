/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/Family5GHzStation.h"
#include "rtw/FamilyBackend.h"
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

void rtw_family_5g_join_reset(rtw_family_5g_join_session *session) {
    if (session) memset(session, 0, sizeof(*session));
}

rtw_family_5g_join_result rtw_family_5g_join_prepare(
    const rtw_usb_wifi_profile *profile, const rtw_r12au_ap *ap,
    const uint8_t station_mac[6], uint8_t observed_primary_channel,
    rtw_family_5g_join_session *out) {
    rtw_family_5g_join_session candidate;
    rtw_wifi_radio_capabilities caps;
    const rtw_family_backend *backend;

    if (!out) return RTW_FAMILY_5G_JOIN_BAD_ARGUMENT;
    if (rtw_output_overlaps_input(out,sizeof(*out),profile,
                                  profile ? sizeof(*profile) : 0U) ||
        rtw_output_overlaps_input(out,sizeof(*out),ap,
                                  ap ? sizeof(*ap) : 0U) ||
        rtw_output_overlaps_input(out,sizeof(*out),station_mac,
                                  station_mac ? 6U : 0U))
        return RTW_FAMILY_5G_JOIN_BAD_ARGUMENT;
    /* Work entirely in a local candidate. No writes through out can change
     * the caller's untrusted AP/MAC during validation, even if aliased. */
    memset(&candidate, 0, sizeof(candidate));
    if (!profile || !ap || !station_mac) {
        rtw_family_5g_join_reset(out);
        return RTW_FAMILY_5G_JOIN_BAD_ARGUMENT;
    }
    caps = rtw_wifi_radio_profile_capabilities(profile);
    if (!caps.identity_verified) {
        rtw_family_5g_join_reset(out);
        return RTW_FAMILY_5G_JOIN_UNVERIFIED_USB;
    }
    backend = rtw_backend_for_family(profile->family);
    if (!(caps.family_bands & RTW_WIFI_BAND_5GHZ) ||
        !backend || !(backend->capabilities & RTW_BACKEND_CAP_OFFLINE_MGMT)) {
        rtw_family_5g_join_reset(out);
        return RTW_FAMILY_5G_JOIN_UNSUPPORTED_FAMILY;
    }
    if (observed_primary_channel <= 14U ||
        ap->primary_channel != observed_primary_channel ||
        rtw_family_radio_prepare_profile(profile, ap->primary_channel,
            ap->center_channel, ap->operating_width_mhz,
            &candidate.observed_channel) != RTW_FAMILY_RADIO_OK ||
        !candidate.observed_channel.is_5ghz ||
        !candidate.observed_channel.passive_only) {
        rtw_family_5g_join_reset(out);
        return RTW_FAMILY_5G_JOIN_BAD_CHANNEL;
    }
    /* HT/VHT operation must agree with advertised channel width. For the
     * 802.11n-only RTL8192DU, VHT80 is already excluded by family geometry. */
    if ((ap->operating_width_mhz >= 40U && !ap->ht_capable) ||
        (ap->operating_width_mhz == 80U && !ap->vht_capable) ||
        rtw_r12au_join_select(&candidate.management, ap, station_mac,
            caps.family_max_nss == 1U ? RTW_R12AU_CHIP_8821AU :
                                        RTW_R12AU_CHIP_8812AU)
            != RTW_R12AU_JOIN_OK) {
        rtw_family_5g_join_reset(out);
        return RTW_FAMILY_5G_JOIN_UNSUPPORTED_AP;
    }
    candidate.profile = profile;
    /* A selected session can generate management frames only. No radio
     * capability is set, and station_bands remains unchanged (zero at 5G). */
    *out = candidate;
    return RTW_FAMILY_5G_JOIN_OK;
}
