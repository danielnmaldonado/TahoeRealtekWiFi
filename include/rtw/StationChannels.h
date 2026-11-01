/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_STATION_CHANNELS_H
#define RTW_STATION_CHANNELS_H
#include <stdint.h>

/* Shared candidate geometry only. A live backend must independently check
 * ROM/RFE, calibrated power, passive-scan and managed-station permission.
 * No channel144: the AU/DU primary power tables do not cover it. */
#define RTW_STATION_5GHZ_CHANNELS {36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,149,153,157,161,165}
#define RTW_STATION_5GHZ_CHANNEL_COUNT 24U

static inline int rtw_station_is_dfs_channel(uint8_t channel) {
    return (channel>=52U && channel<=64U && channel%4U==0U) ||
           (channel>=100U && channel<=140U && channel%4U==0U);
}
static inline int rtw_station_5ghz_channel(uint8_t channel) {
    return (channel>=36U && channel<=48U && channel%4U==0U) ||
           rtw_station_is_dfs_channel(channel) ||
           channel==149U || channel==153U || channel==157U ||
           channel==161U || channel==165U;
}
#endif
