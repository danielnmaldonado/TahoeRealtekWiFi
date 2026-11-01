/* SPDX-License-Identifier: GPL-2.0-only */
#include "Model.hpp"
#include "Protocol.hpp"
#include "rtw/DFSRegulatory.h"
#include <cassert>
#include <cstdio>
#include <cstring>
static void partial_country(const char country[3],unsigned expected) {
    const uint32_t flags=RTW_CONTROL_BANDS_EXPLICIT|RTW_CONTROL_BAND_5GHZ_DFS_LOW;
    unsigned admitted=0;
#ifdef RTW_CONTROL_CHANNEL_MAP_BYTES
    RTWControlChannelMap channels{};channels.version=1;
    for(unsigned ch=0;ch<256;++ch)if(rtw_dfs_regulatory_channel(reinterpret_cast<const uint8_t*>(country),uint8_t(ch))) {
        channels.channels[ch/8U]|=uint8_t(1U<<(ch%8U));
        if(ch>=100)++admitted;
    }
    assert(rtw_control_channel_map_valid(&channels));
#else
    for(unsigned ch=100;ch<=140;ch+=4)admitted+=rtw_dfs_regulatory_channel(reinterpret_cast<const uint8_t*>(country),uint8_t(ch));
#endif
    assert(admitted==expected);
    for(unsigned ch=100;ch<=140;ch+=4) {
        RTWControlScanRecord row{};row.channel=uint8_t(ch);row.securityFlags=7;
        const bool legal=rtw_dfs_regulatory_channel(reinterpret_cast<const uint8_t*>(country),uint8_t(ch));
#ifdef RTW_CONTROL_CHANNEL_MAP_BYTES
        const bool offered=rtwui::supported(row,flags,&channels);
#else
        const bool offered=rtwui::supported(row,flags);
#endif
        assert(offered==legal && "a partial regional map must preserve every registered legal DFS channel");
    }
#ifdef RTW_CONTROL_CHANNEL_MAP_BYTES
    RTWControlScanResults scan{};scan.version=1;scan.count=2;
    for(auto&row:scan.records){row.ssidLength=3;std::memcpy(row.ssid,"Lab",3);row.securityFlags=7;}
    const uint8_t allowed=country[0]=='A'?100:132,denied=country[0]=='A'?120:100;
    scan.records[0].channel=denied;scan.records[0].rssi=-30;
    scan.records[1].channel=allowed;scan.records[1].rssi=-65;
    auto grouped=rtwui::networks(scan,flags,&channels);
    assert(grouped.size()==1&&grouped[0].channel==allowed);
    assert(rtwui::observedNetworkBands(scan,grouped[0],flags,&channels)==2);
    assert(rtwui::observedSSIDSupported(scan,reinterpret_cast<const uint8_t*>("Lab"),3,flags,&channels));
    scan.count=1;
    assert(!rtwui::observedSSIDSupported(scan,reinterpret_cast<const uint8_t*>("Lab"),3,flags,&channels));
    const auto description=rtwui::stationChannelDescription(flags,&channels);
    assert(description.find(country[0]=='A'?"100/104/108":"132/136/140")!=std::string::npos);
    assert(description.find(std::to_string(denied))==std::string::npos&&description.find("144")==std::string::npos);
#endif
}
int main() {
    partial_country("AU",8);partial_country("RU",3);
#ifdef RTW_CONTROL_CHANNEL_MAP_BYTES
    RTWControlRequest request{};request.version=1;
    assert(RTW_CONTROL_CHANNEL_MAP==5&&RTW_CONTROL_SELECTOR_COUNT==6);
    assert(sizeof(RTWControlChannelMap)==48&&sizeof(RTWControlStatus)==120&&sizeof(RTWControlScanResults)==3096);
    assert(rtw_control_validate_request(5,&request,sizeof(request))==RTW_CONTROL_VALID);
    assert(rtw_control_input_size(5)==16&&rtw_control_output_size(5)==48);
    request.reserved[1]=1;assert(rtw_control_validate_request(5,&request,sizeof(request))==RTW_CONTROL_BAD_RESERVED);
    RTWControlChannelMap map{};map.version=1;map.channels[100/8]|=1U<<(100%8);
    assert(rtw_control_channel_map_valid(&map));
    rtwui::DeviceSelection selection;selection.select(42);const auto first=selection.generation;
    rtwui::ChannelMapState state;
    assert(!rtw_control_channel_map_contains(state.current(selection),100));
    assert(state.setExact(selection,42,first,map));
    assert(rtw_control_channel_map_contains(state.current(selection),100));
    selection.select(77);
    assert(!rtw_control_channel_map_contains(state.current(selection),100));
    assert(!state.setExact(selection,42,first,map));
    assert(state.setLegacy(selection,77,selection.generation));
    assert(state.current(selection)==nullptr);
    RTWControlScanRecord row{};row.channel=6;row.securityFlags=7;
    assert(rtwui::supported(row,0,state.current(selection)));
    row.channel=100;assert(!rtwui::supported(row,0,state.current(selection)));
    selection.invalidate();assert(state.current(selection)!=nullptr);
    assert(!rtwui::supported(row,RTW_CONTROL_BAND_5GHZ_DFS_MID,state.current(selection)));
    selection.select(77);const auto current=selection.generation;
    map.reserved[0]=1;assert(!state.setExact(selection,77,current,map));
    assert(!rtwui::supported(row,RTW_CONTROL_BAND_5GHZ_DFS_MID,state.current(selection)));
    map.reserved[0]=0;map.channels[0]=1;assert(!rtw_control_channel_map_valid(&map));
    map.channels[0]=0;map.channels[144/8]|=1U<<(144%8);assert(!rtw_control_channel_map_valid(&map));
    map.channels[144/8]=0;map.version=2;assert(!rtw_control_channel_map_valid(&map));
    RTWControlScanResults hidden{};hidden.version=1;hidden.count=1;
    hidden.records[0].channel=6; // wildcard hidden beacon has no SSID
    const auto profile=reinterpret_cast<const uint8_t*>("HiddenLab");
    assert(!rtwui::savedProfileReconnectAllowed(hidden,profile,9,RTW_CONTROL_BAND_2GHZ,&map));
    map={};map.version=1;map.channels[6/8]|=1U<<(6%8);
    assert(rtwui::savedProfileReconnectAllowed(hidden,profile,9,0,&map));
    hidden.records[0].ssidLength=9;std::memcpy(hidden.records[0].ssid,profile,9);
    hidden.records[0].channel=100;hidden.records[0].securityFlags=7;
    assert(!rtwui::savedProfileReconnectAllowed(hidden,profile,9,0,&map));
#endif
    std::puts("Exact channel map: AU8/RU3 partial MID, grouping, legacy fallback, malformed and stale generation passed");
}
