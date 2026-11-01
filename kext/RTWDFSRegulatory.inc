/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/DFSRegulatory.h"
#include "rtw/StationChannels.h"
int16_t RTWUSBController::factoryDFSChannelLimitMBM(uint8_t channel) const {
    const uint8_t* country=nullptr;
    if(family_==RTW_WIFI_RTL8814AU)country=acCore_.board.country_code;
    else if(is8812Family())country=auCore_.board.country;
    else if(is88Family())country=r88Core_.board.country_code;
    /* DU has no verified AU-style country-byte layout. Keep the explicit
     * world station baseline instead of reading unrelated OTP bytes. */
    return rtw_dfs_regulatory_limit_mbm(country,channel);
}
bool RTWUSBController::permitsDFSChannelByCountry(uint8_t channel) const {
    return !rtw_station_is_dfs_channel(channel)||
        factoryDFSChannelLimitMBM(channel)!=RTW_DFS_REGULATORY_UNAVAILABLE;
}
