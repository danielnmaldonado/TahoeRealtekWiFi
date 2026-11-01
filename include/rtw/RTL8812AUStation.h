/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef RTW_RTL8812AU_STATION_H
#define RTW_RTL8812AU_STATION_H

#include "rtw/RTL8812AUDataPath.h"
#include "rtw/RTL8812AURadio.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    RTW_R12AU_SSID_MAX = 32,
    RTW_R12AU_LEGACY_RATES_MAX = 12,
    RTW_R12AU_ASSOC_FRAME_MAX = 128
};

typedef enum {
    RTW_R12AU_SCAN_OK = 0,
    RTW_R12AU_SCAN_NOT_BEACON,
    RTW_R12AU_SCAN_BAD_ARGUMENT,
    RTW_R12AU_SCAN_MALFORMED,
    RTW_R12AU_SCAN_WRONG_CHANNEL,
    RTW_R12AU_SCAN_BAD_FCS,
    RTW_R12AU_SCAN_UNSUPPORTED
} rtw_r12au_scan_error;

typedef enum {
    RTW_R12AU_SECURITY_OPEN = 0,
    RTW_R12AU_SECURITY_WPA2_PSK_CCMP,
    RTW_R12AU_SECURITY_UNSUPPORTED
} rtw_r12au_security;

typedef struct {
    uint8_t bssid[6];
    uint8_t ssid[RTW_R12AU_SSID_MAX];
    uint8_t ssid_length;
    uint8_t rates[RTW_R12AU_LEGACY_RATES_MAX];
    uint8_t rate_count;
    uint8_t primary_channel;
    /* Beacon-advertised channel geometry, not permission to tune or transmit.
     * Remains 20 MHz / primary when HT/VHT operating IEs are absent. */
    uint8_t operating_width_mhz;
    uint8_t center_channel;
    uint8_t ht_capable;
    uint8_t vht_capable;
    uint8_t pmf_required;
    uint16_t capability;
    uint16_t beacon_interval;
    rtw_r12au_security security;
} rtw_r12au_ap;

/* Strict parse of 802.11 beacon/probe-response received on a KNOWN physical
 * channel. The supplied channel is derived from trusted RF tuning metadata,
 * never from the untrusted frame. No USB work and no wireless transmission.
 * fcs_present=1 checks the frame's trailing 4-byte CRC-32; frame CRC is not
 * authentication of an AP. Plain RX descriptors must not have CRC/ICV errors.
 * Hidden SSIDs, WEP, WPA1, WPA3-only SAE, mandatory PMF and malformed RSN
 * are rejected. WPA2/WPA3 transition IEs are eligible only when the AP
 * also offers WPA2-PSK/CCMP with group CCMP and optional PMF.
 */
rtw_r12au_scan_error rtw_r12au_scan_parse(
    const rtw_r12au_rx_frame *rx, uint8_t tuned_channel, int fcs_present,
    rtw_r12au_ap *ap);

typedef enum {
    RTW_R12AU_JOIN_IDLE = 0,
    RTW_R12AU_JOIN_SELECTED,
    RTW_R12AU_JOIN_AUTH_SENT,
    RTW_R12AU_JOIN_AUTHENTICATED,
    RTW_R12AU_JOIN_ASSOC_SENT,
    RTW_R12AU_JOIN_OPEN_ASSOCIATED_UNVERIFIED,
    RTW_R12AU_JOIN_WPA2_KEYS_REQUIRED,
    RTW_R12AU_JOIN_REJECTED,
    RTW_R12AU_JOIN_DISCONNECTED
} rtw_r12au_join_state;

typedef enum {
    RTW_R12AU_JOIN_OK = 0,
    RTW_R12AU_JOIN_BAD_ARGUMENT,
    RTW_R12AU_JOIN_BAD_STATE,
    RTW_R12AU_JOIN_UNSUPPORTED_AP,
    RTW_R12AU_JOIN_WRONG_PEER,
    RTW_R12AU_JOIN_MALFORMED,
    RTW_R12AU_JOIN_REJECTED_BY_AP,
    RTW_R12AU_JOIN_BUFFER_TOO_SMALL
} rtw_r12au_join_error;

typedef struct {
    rtw_r12au_ap ap;
    uint8_t station[6];
    uint16_t status;
    uint16_t aid;
    rtw_r12au_join_state state;
} rtw_r12au_join_session;

/* Offline frame construction and response parsing for open-system auth
 * and WPA2-PSK/CCMP association-IE negotiation. WPA2 is never authorized:
 * 4-way handshake, replay protection and CCMP encryption are not implemented
 * here. Even successful OPEN association is UNVERIFIED: neither station state
 * authorizes network traffic, marks RF ready nor publishes carrier.
 */
void rtw_r12au_join_reset(rtw_r12au_join_session *session);
rtw_r12au_join_error rtw_r12au_join_select(
    rtw_r12au_join_session *session, const rtw_r12au_ap *ap,
    const uint8_t station_mac[6], rtw_r12au_chip chip);

rtw_r12au_join_error rtw_r12au_join_auth_request(
    rtw_r12au_join_session *session, uint16_t sequence,
    uint8_t *out, size_t capacity, size_t *written);
rtw_r12au_join_error rtw_r12au_join_auth_response(
    rtw_r12au_join_session *session, const uint8_t *frame, size_t length);
rtw_r12au_join_error rtw_r12au_join_assoc_request(
    rtw_r12au_join_session *session, uint16_t sequence,
    uint16_t listen_interval, uint8_t *out, size_t capacity, size_t *written);
rtw_r12au_join_error rtw_r12au_join_assoc_response(
    rtw_r12au_join_session *session, const uint8_t *frame, size_t length);
rtw_r12au_join_error rtw_r12au_join_disconnect(
    rtw_r12au_join_session *session, const uint8_t *frame, size_t length);
/* Caller owns timeouts; an expired attempt is revoked and cannot restart
 * without explicit selection. No stale frames change a terminated session. */
void rtw_r12au_join_expire(rtw_r12au_join_session *session);

#ifdef __cplusplus
}
#endif
#endif
