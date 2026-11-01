/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_FAMILY_5GHZ_STATION_H
#define RTW_FAMILY_5GHZ_STATION_H

#include "rtw/FamilyRadioPolicy.h"
#include "rtw/RTL8812AUStation.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_FAMILY_5G_JOIN_OK = 0,
    RTW_FAMILY_5G_JOIN_BAD_ARGUMENT,
    RTW_FAMILY_5G_JOIN_UNVERIFIED_USB,
    RTW_FAMILY_5G_JOIN_UNSUPPORTED_FAMILY,
    RTW_FAMILY_5G_JOIN_BAD_CHANNEL,
    RTW_FAMILY_5G_JOIN_UNSUPPORTED_AP
} rtw_family_5g_join_result;

typedef struct {
    /* Catalog identity must be an immutable original pointer; no copied
     * or unverified USB profile may promote itself to a supported family. */
    const rtw_usb_wifi_profile *profile;
    rtw_r12au_channel_plan observed_channel;
    rtw_r12au_join_session management;
} rtw_family_5g_join_session;

/* Prepare real IEEE 802.11 authentication/association management state for
 * each verified dual-band family. AP must originate from validated RX on the
 * actual externally tuned primary channel (observed_primary_channel).
 *
 * This API neither initializes RF nor supplies TX permission. In particular,
 * a successful prepare or subsequent rtw_r12au_join_* transition NEVER
 * authorizes transmitting an assembled frame, setting carrier, marking keys
 * installed, operating on DFS, or reporting a functional 5 GHz connection.
 * Every family still requires its own firmware, PHY, power calibration,
 * regulatory integration, USB TX/RX and WPA2 key installation path.
 */
rtw_family_5g_join_result rtw_family_5g_join_prepare(
    const rtw_usb_wifi_profile *profile,
    const rtw_r12au_ap *ap,
    const uint8_t station_mac[6],
    uint8_t observed_primary_channel,
    rtw_family_5g_join_session *out);

/* Cancel all in-memory state, including AP and station MAC copies. */
void rtw_family_5g_join_reset(rtw_family_5g_join_session *session);

#ifdef __cplusplus
}
#endif
#endif
