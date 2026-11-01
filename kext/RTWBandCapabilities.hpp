#pragma once
// SPDX-License-Identifier: GPL-2.0-only
#include "RTWControl.h"
// Predicate reads the registered station channel map; it performs no USB I/O.
// A group flag permits every channel in that group in the ABI v1 client.
template<class Predicate>
inline uint32_t rtw_station_5ghz_flags(Predicate supported) {
    const uint8_t low[]={36,40,44,48},high[]={149,153,157,161,165};
    const uint8_t dfsLow[]={52,56,60,64},dfsMid[]={100,104,108,112,116,120,124,128,132,136,140};
    bool lowReady=true,highReady=true,dfsLowReady=true,dfsMidReady=true;
    for(uint8_t channel:low)if(!supported(channel)){lowReady=false;break;}
    for(uint8_t channel:high)if(!supported(channel)){highReady=false;break;}
    for(uint8_t channel:dfsLow)if(!supported(channel)){dfsLowReady=false;break;}
    for(uint8_t channel:dfsMid)if(!supported(channel)){dfsMidReady=false;break;}
    return (lowReady?RTW_CONTROL_BAND_5GHZ:0U)|
        (highReady?RTW_CONTROL_BAND_5GHZ_HIGH:0U)|
        (dfsLowReady?RTW_CONTROL_BAND_5GHZ_DFS_LOW:0U)|
        (dfsMidReady?RTW_CONTROL_BAND_5GHZ_DFS_MID:0U);
}

template<class Predicate>
inline uint32_t rtw_station_band_flags(Predicate supported) {
    bool twoReady=true;
    for(uint8_t channel=1;channel<=11;++channel)if(!supported(channel)){twoReady=false;break;}
    return RTW_CONTROL_BANDS_EXPLICIT|(twoReady?RTW_CONTROL_BAND_2GHZ:0U)|rtw_station_5ghz_flags(supported);
}
