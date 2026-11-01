#ifndef RTW_STATION_MGMT_H
#define RTW_STATION_MGMT_H
/* SPDX-License-Identifier: GPL-2.0-only
 * Offline 802.11 management-frame builder/parser for ONE open BSS on ch6.
 * No USB transmission, association with actual radio, RSN, CCMP or carrier.
 * Management responses in an OPEN network are unauthenticated and may be
 * spoofed; only a physical 802.11 MAC may establish link state later.
 */
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_STA_SSID_MAX 32U
#define RTW_STA_RATES_MAX 8U

typedef enum {
    RTW_STA_EMPTY=0, RTW_STA_CONFIGURED, RTW_STA_AUTH_PENDING,
    RTW_STA_AUTHENTICATED, RTW_STA_ASSOC_PENDING,
    RTW_STA_ASSOCIATED, RTW_STA_REJECTED, RTW_STA_DISCONNECTED
} rtw_sta_state;
typedef enum {
    RTW_STA_OK=0, RTW_STA_BAD_ARGUMENT, RTW_STA_BAD_STATE,
    RTW_STA_UNSUPPORTED_AP, RTW_STA_WRONG_PEER,
    RTW_STA_MALFORMED_RESPONSE, RTW_STA_AP_REJECTED,
    RTW_STA_BUFFER_TOO_SMALL
} rtw_sta_error;
typedef struct {
    uint8_t station[6], bssid[6];
    uint8_t ssid[RTW_STA_SSID_MAX];
    uint8_t ssid_length;
    uint16_t aid;       /* filled from a validated association response */
    uint16_t status;    /* last AP response status/reason; NOT crypto evidence */
    rtw_sta_state state;
} rtw_sta_session;

void rtw_sta_reset(rtw_sta_session* session);
/* Only an OPEN (privacy=0, RSN=0) ch6 infrastructure BSS with visible SSID.
 * A caller must separately validate scan metadata/FCS/PHY and rates.
 * Failed reconfiguration leaves the session EMPTY. */
rtw_sta_error rtw_sta_configure_open(rtw_sta_session* session,
    const uint8_t station[6], const uint8_t bssid[6],
    const uint8_t* ssid, size_t ssid_length, uint8_t channel,
    int privacy_capability, int has_rsn);
/* Builders output RAW management MPDUs with NO trailing FCS. No I/O. */
rtw_sta_error rtw_sta_build_auth_request(rtw_sta_session* session,
    uint16_t sequence_number, uint8_t* out, size_t capacity, size_t* written);
rtw_sta_error rtw_sta_accept_auth_response(rtw_sta_session* session,
    const uint8_t* frame, size_t length);
rtw_sta_error rtw_sta_build_assoc_request(rtw_sta_session* session,
    uint16_t sequence_number, uint16_t listen_interval,
    const uint8_t* rates, size_t rate_count,
    uint8_t* out, size_t capacity, size_t* written);
rtw_sta_error rtw_sta_accept_assoc_response(rtw_sta_session* session,
    const uint8_t* frame, size_t length);
rtw_sta_error rtw_sta_accept_disconnect(rtw_sta_session* session,
    const uint8_t* frame, size_t length);
/* This state never authorizes IP or the controlled-port; do NOT use to
 * publish carrier without radio/driver-level verification. */
const char* rtw_sta_error_string(rtw_sta_error error);
#ifdef __cplusplus
}
#endif
#endif
