/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_NETWORK_SECURITY_H
#define RTW_NETWORK_SECURITY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Classification of an AP's negotiated RSN capabilities, not a statement that
 * firmware, radio association, key installation or IP traffic works. The
 * station currently offers WPA2-Personal with pairwise and group CCMP only.
 * Mixed WPA2/WPA3 transition networks are eligible only when the PSK AKM
 * remains available and management-frame protection is not mandatory. */
typedef struct rtw_network_security {
    uint8_t privacy;
    uint8_t rsn;
    uint8_t psk;
    uint8_t pairwise_ccmp;
    uint8_t group_ccmp;
    uint8_t pmf_required;
} rtw_network_security;

typedef enum rtw_network_security_kind {
    RTW_NETWORK_OPEN = 0,
    RTW_NETWORK_WPA2_PSK_CCMP = 1,
    RTW_NETWORK_UNSUPPORTED = 2
} rtw_network_security_kind;

static inline rtw_network_security_kind rtw_network_security_classify(
    const rtw_network_security *capabilities)
{
    if (!capabilities)
        return RTW_NETWORK_UNSUPPORTED;

    if (!capabilities->privacy) {
        return capabilities->rsn || capabilities->psk ||
               capabilities->pairwise_ccmp || capabilities->group_ccmp ||
               capabilities->pmf_required
               ? RTW_NETWORK_UNSUPPORTED : RTW_NETWORK_OPEN;
    }

    return capabilities->rsn && capabilities->psk &&
           capabilities->pairwise_ccmp && capabilities->group_ccmp &&
           !capabilities->pmf_required
           ? RTW_NETWORK_WPA2_PSK_CCMP : RTW_NETWORK_UNSUPPORTED;
}

#ifdef __cplusplus
}
#endif

#endif
