/* SPDX-License-Identifier: BSD-2-Clause */
#ifndef RTW_RTL8812AU_RADIO_H
#define RTW_RTL8812AU_RADIO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_R12AU_CHIP_8812AU = 1,
    RTW_R12AU_CHIP_8821AU = 2
} rtw_r12au_chip;

typedef enum {
    RTW_R12AU_CHANNEL_OK = 0,
    RTW_R12AU_CHANNEL_INVALID_ARGUMENT,
    RTW_R12AU_CHANNEL_UNSUPPORTED
} rtw_r12au_channel_error;

typedef struct {
    rtw_r12au_chip chip;
    uint8_t primary_channel;
    uint8_t width_mhz;
    uint8_t center_channel;
} rtw_r12au_channel_request;

typedef struct {
    uint16_t primary_frequency_mhz;
    uint16_t center_frequency_mhz;
    uint8_t primary_channel;
    uint8_t center_channel;
    uint8_t width_mhz;
    uint8_t rf_chains;
    uint8_t is_5ghz;
    uint8_t dfs_required;
    uint8_t passive_only;
} rtw_r12au_channel_plan;

/*
 * Offline channel geometry for Realtek 8812AU/8821AU. No regulatory
 * permission, RF calibration, firmware readiness, or TX authorization is
 * implied by a successful result. The sole supported transmit-capable PHY
 * profile elsewhere in the repository remains the explicitly authorized
 * 8812AU RFE3/2.4GHz/channel-6/20MHz diagnostic profile.
 *
 * Supports 2.4GHz 1..11 at 20MHz and 5GHz groups at 20/40/80MHz.
 * DFS frequencies are identified, not enabled for active operation.
 * Regulatory permissions MUST be supplied by a future OS integration.
 */
rtw_r12au_channel_error rtw_r12au_channel_prepare(
    const rtw_r12au_channel_request *request,
    rtw_r12au_channel_plan *plan);

#ifdef __cplusplus
}
#endif
#endif
