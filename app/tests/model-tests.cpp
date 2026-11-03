// SPDX-License-Identifier: GPL-2.0-only
#include "RTWControl.h"
// Count the real validator at the model boundary. Revalidating an immutable
// map per BSS made a 64-record scan repeat the same full-map work hundreds of
// times. The wrapper preserves validation behavior and adds no production seam.
static unsigned mapValidations=0;
static int countedMapValid(const RTWControlChannelMap* map) {
    ++mapValidations;return rtw_control_channel_map_valid(map);
}
#define rtw_control_channel_map_valid countedMapValid
#include "Model.hpp"
#undef rtw_control_channel_map_valid
#include "Protocol.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

static void map_validation_is_bounded_per_model_operation() {
    RTWControlChannelMap map{};map.version=1;map.channels[11/8]|=1U<<(11%8);
    RTWControlScanResults scan{};scan.version=1;scan.count=64;
    for(unsigned i=0;i<scan.count;++i) {
        auto& row=scan.records[i];row.ssidLength=3;std::memcpy(row.ssid,"Lab",3);
        row.channel=6;row.securityFlags=7;row.rssi=-30-static_cast<int>(i);
    }
    mapValidations=0;
    const auto grouped=rtwui::networks(scan,UINT32_MAX,&map);
    assert(grouped.size()==1&&grouped[0].rssi==-30);
    assert(mapValidations<=1&&"network grouping must validate its immutable map at most once");
    const auto ssid=reinterpret_cast<const uint8_t*>("Lab");
    mapValidations=0;
    assert(!rtwui::observedSSIDSupported(scan,ssid,3,UINT32_MAX,&map));
    assert(mapValidations<=1&&"SSID lookup must validate its immutable map at most once");
    mapValidations=0;
    assert(!rtwui::savedProfileReconnectAllowed(scan,ssid,3,UINT32_MAX,&map));
    assert(mapValidations<=1&&"reconnect lookup must validate its immutable map at most once");
    mapValidations=0;
    assert(!rtwui::observedNetworkBands(scan,scan.records[0],UINT32_MAX,&map));
    assert(mapValidations<=1&&"band lookup must validate its immutable map at most once");
    // No validation cache may outlive one operation or hide later corruption.
    map.channels[6/8]|=1U<<(6%8);
    assert(rtwui::supported(scan.records[0],0,&map));
    assert(rtwui::observedSSIDSupported(scan,ssid,3,0,&map));
    assert(rtwui::savedProfileReconnectAllowed(scan,ssid,3,0,&map));
    assert(rtwui::observedNetworkBands(scan,scan.records[0],0,&map)==1);
    for(unsigned reserved=0;reserved<3;++reserved) {
        map.reserved[reserved]=UINT32_MAX;
        assert(!rtwui::supported(scan.records[0],UINT32_MAX,&map));
        assert(!rtwui::observedSSIDSupported(scan,ssid,3,UINT32_MAX,&map));
        assert(!rtwui::savedProfileReconnectAllowed(scan,ssid,3,UINT32_MAX,&map));
        assert(!rtwui::observedNetworkBands(scan,scan.records[0],UINT32_MAX,&map));
        map.reserved[reserved]=0;
    }
    map.channels[144/8]|=1U<<(144%8);
    assert(!rtwui::supported(scan.records[0],UINT32_MAX,&map));
    map.channels[144/8]=0;
    map.version=UINT32_MAX;assert(!rtwui::supported(scan.records[0],UINT32_MAX,&map));
    map.version=1;
    scan.records[0].securityFlags=UINT32_MAX;
    mapValidations=0;
    assert(!rtwui::supported(scan.records[0],UINT32_MAX,&map));
    assert(!mapValidations&&"unsupported security must reject before channel-map work");
    // An exact map is authoritative for every station-flag combination; no
    // flag bit may admit channel 144 or make a malformed map usable.
    scan.records[0].securityFlags=7;scan.records[0].channel=132;
    map.channels[132/8]|=1U<<(132%8);
    const uint32_t stationFlags[]={0,RTW_CONTROL_BANDS_EXPLICIT,
        RTW_CONTROL_BAND_2GHZ,RTW_CONTROL_BAND_5GHZ,
        RTW_CONTROL_BAND_5GHZ_HIGH,RTW_CONTROL_BAND_5GHZ_DFS_LOW,
        RTW_CONTROL_BAND_5GHZ_DFS_MID,0x80000000U,UINT32_MAX};
    for(uint32_t flags:stationFlags) {
        assert(rtwui::supported(scan.records[0],flags,&map));
        scan.records[0].channel=144;assert(!rtwui::supported(scan.records[0],flags,&map));
        scan.records[0].channel=132;
    }
}

int main() {
    map_validation_is_bounded_per_model_operation();
    const uint32_t dfsLow=1U<<11,dfsMid=1U<<12,explicitFlag=1U<<10;
    RTWControlScanRecord dfs{};dfs.channel=100;dfs.securityFlags=7;
    assert(rtwui::supported(dfs,explicitFlag|dfsMid));
    assert(!rtwui::supported(dfs,explicitFlag|dfsLow));
    dfs.channel=52;
    assert(rtwui::supported(dfs,explicitFlag|dfsLow));
    assert(!rtwui::supported(dfs,0));
    assert(std::strstr(rtwui::stationBandDescription(explicitFlag|dfsLow),"5 GHz"));
    assert(!std::strstr(rtwui::stationBandDescription(explicitFlag|dfsLow),"2.4"));
    assert(std::strstr(rtwui::stationChannelDescription(explicitFlag|dfsLow),"52/56/60/64"));
    assert(!std::strstr(rtwui::stationChannelDescription(explicitFlag|dfsLow),"100/104"));
    assert(std::strstr(rtwui::stationChannelDescription(explicitFlag|dfsMid),"100/104"));
    assert(!std::strstr(rtwui::stationChannelDescription(explicitFlag|dfsMid),"144"));
    // A stronger2.4GHz representative must still expose the observed DFS band.
    RTWControlScanResults dfsScan{};dfsScan.version=1;dfsScan.count=2;
    for(auto&r:dfsScan.records){r.ssidLength=3;std::memcpy(r.ssid,"Lab",3);r.securityFlags=7;}
    dfsScan.records[0].channel=6;dfsScan.records[0].rssi=-35;
    dfsScan.records[1].channel=100;dfsScan.records[1].rssi=-60;
    const uint32_t dfsBoth=explicitFlag|RTW_CONTROL_BAND_2GHZ|dfsMid;
    const auto dfsNetworks=rtwui::networks(dfsScan,dfsBoth);
    assert(dfsNetworks.size()==1&&dfsNetworks[0].channel==6);
    assert(rtwui::observedNetworkBands(dfsScan,dfsNetworks[0],dfsBoth)==3);
    assert(rtwui::observedNetworkBands(dfsScan,dfsNetworks[0],explicitFlag|RTW_CONTROL_BAND_2GHZ)==1);
    const uint32_t explicitFive=(1U<<10)|RTW_CONTROL_BAND_5GHZ;
    assert(!rtw_control_channel_supported(6,explicitFive));
    assert(!std::strstr(rtwui::bandDescription(explicitFive),"2.4"));
    assert(!std::strstr(rtwui::stationChannelDescription(explicitFive),"2.4"));
    assert(sizeof(rtwui::Frame)==32 && "helper IPC must carry explicit registry ID/version");
    // A RUN station still awaiting WPA2 must never be labelled connected.
    RTWControlStatus s{}; s.version=1; s.flags=7; s.state=4;
    assert(rtwui::phase(s)==rtwui::Phase::Authenticating);
    s.flags|=8; assert(rtwui::phase(s)==rtwui::Phase::Connected);
    s.flags=1; assert(rtwui::phase(s)==rtwui::Phase::RadioOff);
    s.flags=23;s.state=1;assert(rtwui::phase(s)==rtwui::Phase::Scanning);
    s.usbError=1;assert(rtwui::phase(s)==rtwui::Phase::Error);
    s.flags=0;s.usbError=0;assert(rtwui::phase(s)==rtwui::Phase::Absent);
    assert(rtwui::signalLevel(0)==0);
    assert(rtwui::signalLevel(-48)==3);
    assert(rtwui::signalLevel(-65)==2);
    assert(rtwui::signalLevel(-85)==1);
    RTWControlScanResults scan{};scan.version=1;scan.count=4;
    for (unsigned i=0;i<4;++i) { auto &r=scan.records[i];r.ssidLength=3;
        std::memcpy(r.ssid,"Lab",3);r.securityFlags=7;r.channel=6;r.rssi=-80; }
    scan.records[1].rssi=-40;
    scan.records[2].securityFlags=9; // same SSID, different unsupported security
    scan.records[3].ssidLength=0; // hidden beacon is not a named network
    auto networks=rtwui::networks(scan);
    assert(networks.size()==2 && networks[0].rssi==-40);
    assert(rtwui::supported(networks[0]));
    assert(!rtwui::supported(networks[1]));
    // A live driver's optional band capability is required. Silicon family,
    // a scan record or a familiar SSID must never enable an unsupported band.
    const uint32_t fiveGHz=1U<<7,highGHz=1U<<8;
    RTWControlScanRecord five=scan.records[0];five.channel=36;
    assert(!rtwui::supported(five));
    assert(rtwui::supported(five,fiveGHz));
    for(unsigned channel=0;channel<256;++channel) {
        five.channel=static_cast<uint8_t>(channel);
        const bool g2=channel>=1&&channel<=11;
        const bool g5=channel==36||channel==40||channel==44||channel==48;
        const bool high=channel==149||channel==153||channel==157||channel==161||channel==165;
        assert(rtwui::supported(five,0)==g2);
        assert(rtwui::supported(five,fiveGHz)==(g2||g5));
        assert(rtwui::supported(five,highGHz)==(g2||high));
        assert(rtwui::supported(five,fiveGHz|highGHz)==(g2||g5||high));
    }
    assert(!std::strstr(rtwui::stationBandDescription(0),"5 GHz"));
    assert(std::strstr(rtwui::stationBandDescription(highGHz),"149")&&!std::strstr(rtwui::stationBandDescription(highGHz),"36"));
    assert(std::strstr(rtwui::stationChannelDescription(fiveGHz|highGHz),"36/40/44/48 and 149/153/157/161/165"));
    assert(!std::strstr(rtwui::stationChannelDescription(fiveGHz),"149"));
    five.channel=36;five.securityFlags=RTW_CONTROL_SECURITY_WPA2_PSK;
    assert(!rtwui::supported(five,fiveGHz));
    five.securityFlags=7|RTW_CONTROL_SECURITY_UNSUPPORTED;
    assert(!rtwui::supported(five,fiveGHz));
    assert(std::strcmp(rtwui::bandDescription(1),"2.4 GHz")==0);
    assert(std::strcmp(rtwui::bandDescription(36),"5 GHz")==0);
    assert(std::strcmp(rtwui::bandDescription(149),"5 GHz")==0);
    assert(std::strcmp(rtwui::bandDescription(35),"unknown band")==0);
    assert(rtwui::channelFrequencyMHz(1)==2412&&rtwui::channelFrequencyMHz(11)==2462);
    assert(rtwui::channelFrequencyMHz(14)==2484&&rtwui::channelFrequencyMHz(36)==5180);
    assert(rtwui::channelFrequencyMHz(149)==5745&&rtwui::channelFrequencyMHz(177)==5885);
    assert(!rtwui::channelFrequencyMHz(0)&&!rtwui::channelFrequencyMHz(35)&&
        !rtwui::channelFrequencyMHz(50)&&!rtwui::channelFrequencyMHz(255));
    // A usable 2.4 GHz AP wins over a stronger, unsupported 5 GHz AP.
    scan.count=2;
    scan.records[0].channel=6;scan.records[0].rssi=-80;
    scan.records[1].channel=36;scan.records[1].rssi=-30;
    const auto dualNetworks=rtwui::networks(scan,fiveGHz);
    assert(dualNetworks.size()==1&&dualNetworks[0].channel==36);
    networks=rtwui::networks(scan);
    assert(networks.size()==1 && networks[0].channel==6);
    std::swap(scan.records[0],scan.records[1]);
    networks=rtwui::networks(scan);
    assert(networks.size()==1 && networks[0].channel==6);
    scan.records[1].rssi=0; // usable, unknown RSSI remains preferable
    assert(rtwui::networks(scan)[0].channel==6);
    scan.records[0].channel=11;scan.records[0].rssi=-90;
    assert(rtwui::networks(scan)[0].channel==11); // both usable: known RSSI wins
    scan.count=3;
    scan.records[0].channel=6;scan.records[0].rssi=-90;
    scan.records[1]=scan.records[0];scan.records[1].channel=36;scan.records[1].rssi=-70;
    assert(rtwui::observedNetworkBands(scan,scan.records[0],fiveGHz)==3);
    assert(rtwui::observedNetworkBands(scan,scan.records[0],0)==1);
    assert(std::string(rtwui::observedNetworkBandsText(3))=="2.4 + 5 GHz");
    auto other=scan.records[0];other.ssid[0]='X';assert(rtwui::observedNetworkBands(scan,other,fiveGHz)==0);

    scan.records[2]=scan.records[0];scan.records[2].channel=149;scan.records[2].rssi=-30;
    assert(rtwui::networks(scan,0)[0].channel==6);
    assert(rtwui::networks(scan,fiveGHz)[0].channel==36);
    assert(rtwui::networks(scan,highGHz)[0].channel==149);
    assert(rtwui::networks(scan,fiveGHz|highGHz)[0].channel==149);
    scan.count=65;assert(rtwui::networks(scan).empty());
    scan.count=1;scan.records[0].ssidLength=33;
    assert(rtwui::networks(scan).empty());
    assert(rtwui::validPassword("12345678",8));
    assert(!rtwui::validPassword("1234567",7));
    assert(!rtwui::validPassword("1234567\n",8));
    char longpass[64];std::memset(longpass,'a',64);
    assert(!rtwui::validPassword(longpass,64));
    const uint8_t binary[]={0xff,0x00,0x1b};
    assert(rtwui::displaySSID(binary,3)=="[ff001b]");
    assert(rtwui::displaySSID(reinterpret_cast<const uint8_t*>("Lab"),3)=="Lab");
    auto f=rtwui::makeFrame(0,1234,16);
    assert(rtwui::validFrame(f,false));
    f.registryID=0;assert(!rtwui::validFrame(f,false));f.registryID=1234;
    f.length=80;assert(!rtwui::validFrame(f,false));
    f.selector=2;assert(rtwui::validFrame(f,false));
    f.selector=99;assert(!rtwui::validFrame(f,false));
    f=rtwui::makeFrame(0,1234,120);assert(rtwui::validFrame(f,true));
    f.result=1;f.length=0;assert(rtwui::validFrame(f,true));
    f.length=120;assert(!rtwui::validFrame(f,true));
    f=rtwui::makeFrame(0,1234,16);f.version=1;assert(!rtwui::validFrame(f,false));
    f=rtwui::makeFrame(rtwui::listSelector,0,0);assert(rtwui::validFrame(f,false));
    f.registryID=1234;assert(!rtwui::validFrame(f,false));
    rtwui::DeviceList devices{};devices.version=2;devices.count=2;
    devices.devices[0].registryID=1234;devices.devices[1].registryID=5678;
    assert(rtwui::validDevices(devices));devices.devices[1].registryID=1234;assert(!rtwui::validDevices(devices));
    devices.devices[1].registryID=5678;devices.count=17;assert(!rtwui::validDevices(devices));
    rtwui::DeviceSelection selection;assert(selection.select(1234));const auto token=selection.generation;
    assert(selection.matches(1234,token));selection.invalidate();assert(!selection.matches(1234,token));
    // An unplug invalidates replies while retaining the exact selected ID.
    assert(selection.registryID==1234&&selection.everSelected);assert(!selection.select(0));
    // A temporarily absent adapter must not validate new reads using a stale
    // selection token. Selecting the same registry ID after reappearance must
    // restore it instead of leaving the UI in a disconnected state.
    assert(!selection.matches(1234,selection.generation));
    assert(selection.select(1234));
    assert(selection.matches(1234,selection.generation));
    assert(!selection.select(1234)); // unchanged active selection is a no-op
    selection.invalidate();
    assert(selection.select(5678));assert(!selection.matches(1234,selection.generation));
    // A status captured before the latest JOIN/ACK cannot confirm that JOIN.
    s={};s.version=1;s.flags=15;s.ssidLength=3;memcpy(s.ssid,"Lab",3);
    const uint8_t wanted[]={'L','a','b'};
    assert(!rtwui::confirmsJoin(s,wanted,3,1,2,false));
    assert(!rtwui::confirmsJoin(s,wanted,3,2,2,false));
    assert(!rtwui::confirmsJoin(s,wanted,3,1,2,true));
    assert(rtwui::confirmsJoin(s,wanted,3,2,2,true));
    s.flags=7;assert(!rtwui::confirmsJoin(s,wanted,3,2,2,true));
    s.flags=15;s.ssid[2]='x';assert(!rtwui::confirmsJoin(s,wanted,3,2,2,true));
    // No single failed IPC read may turn a verified WPA link into a false unplug.
    s={};s.version=1;s.flags=15;s.ssidLength=3;std::memcpy(s.ssid,"Lab",3);
    using I=rtwui::LinkIndicator;
    assert(rtwui::linkIndicator(true,true,true,true,false,false,s,100,100)==I::Connected);
    assert(rtwui::linkIndicator(true,true,true,true,true,false,s,105,100)==I::Connected);
    assert(rtwui::linkIndicator(true,true,true,true,true,false,s,107,100)==I::Unknown);
    assert(rtwui::linkIndicator(true,true,true,true,true,false,s,100, -1)==I::Unknown);
    assert(rtwui::linkIndicator(false,true,true,true,false,false,s,100,100)==I::Unknown);
    assert(rtwui::linkIndicator(true,true,false,true,false,false,s,100,100)==I::Disconnected);
    assert(rtwui::linkIndicator(true,false,false,false,false,false,s,100,100)==I::Unknown);
    assert(rtwui::linkIndicator(true,true,true,true,false,true,s,100,100)==I::Connecting);
    s.flags=7; // Successful status: WPA no longer authorized.
    assert(rtwui::linkIndicator(true,true,true,true,false,false,s,101,100)==I::Disconnected);
    s.flags=15;s.rssi=-43;s.bssid[0]=0xA4;
    rtwui::SignalMemory signal;
    signal.observe(s,100);assert(signal.value(101)==-43);
    s.rssi=0;signal.observe(s,102);assert(signal.value(107)==-43);
    assert(signal.value(111)==0); // stale RSSI must expire after 10 seconds
    s.rssi=-60;signal.observe(s,113);assert(signal.value(114)==-60);
    s.bssid[0]=0xA5;s.rssi=0;signal.observe(s,115);assert(signal.value(115)==0);
    s.rssi=-65;signal.observe(s,116);assert(signal.value(116)==-65);
    s.ssid[0]='X';s.rssi=0;signal.observe(s,117);assert(signal.value(117)==0);
    s.flags=7;signal.observe(s,118);assert(signal.value(118)==0);
    s.flags=15;s.rssi=-50;signal.observe(s,119);assert(signal.value(120)==-50);
    signal.clear();assert(signal.value(121)==0);
    std::puts("model/protocol/join generation: passed");
}
