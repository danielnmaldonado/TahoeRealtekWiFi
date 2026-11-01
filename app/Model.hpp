// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "RTWControl.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <iterator>
#include <cstdint>
#include <string>
#include <vector>

namespace rtwui {
enum class Phase { Absent, RadioOff, Scanning, Associating, Authenticating, Connected, Idle, Error };
inline Phase phase(const RTWControlStatus& s) {
    if (!(s.flags & RTW_CONTROL_DEVICE)) return Phase::Absent;
    if (s.usbError) return Phase::Error;
    if (!(s.flags & RTW_CONTROL_RADIO)) return Phase::RadioOff;
    if (s.flags & RTW_CONTROL_AUTHORIZED) return Phase::Connected;
    if (s.flags & RTW_CONTROL_SCANNING) return Phase::Scanning;
    if (s.state==4) return Phase::Authenticating;
    if (s.state==2 || s.state==3) return Phase::Associating;
    return Phase::Idle;
}
// A status read error does not prove that the USB adapter or WPA link vanished.
// Retain a known-good connected icon briefly, then show "indeterminate" rather
// than a false disconnected icon. Successful, explicit state changes are immediate.
enum class LinkIndicator { Disconnected, Connected, Connecting, Unknown };
inline LinkIndicator linkIndicator(bool session, bool selected, bool present,
                                    bool hasStatus, bool statusDelayed, bool joining,
                                    const RTWControlStatus& status, double now, double lastSuccess) {
    if(selected && !present) return LinkIndicator::Disconnected;
    if(joining && present) return LinkIndicator::Connecting;
    if(!session || !selected || !hasStatus) return LinkIndicator::Unknown;
    if(statusDelayed) {
        if(phase(status)==Phase::Connected && lastSuccess>=0 && now>=lastSuccess && now-lastSuccess<=6.0)
            return LinkIndicator::Connected;
        return LinkIndicator::Unknown;
    }
    return phase(status)==Phase::Connected ? LinkIndicator::Connected : LinkIndicator::Disconnected;
}

inline bool validSignal(int rssi) { return rssi >= -127 && rssi < 0; }
inline unsigned signalLevel(int rssi) {
    if (!validSignal(rssi)) return 0;
    return rssi>=-55?3:rssi>=-72?2:1;
}
inline unsigned stableSignalLevel(int rssi,unsigned previous) {
    if (!validSignal(rssi)) return 0;
    if (previous==3) return rssi>=-58?3:rssi>=-75?2:1;
    if (previous==2) return rssi>=-52?3:rssi>=-75?2:1;
    if (previous==1) return rssi>=-52?3:rssi>=-69?2:1;
    return signalLevel(rssi);
}
// The raw reading is retained for diagnosis. A median of three polls rejects
// one isolated spike; two persistent low readings still lower the indicator.
// Hysteresis prevents chatter at bar boundaries. No filter affects link state.
struct SignalMemory {
    uint8_t ssid[32]{};
    uint8_t bssid[6]{};
    uint32_t ssidLength=0,channel=0;
    double lastSample=-1;
    int lastRSSI=0;
    bool hasAssociation=false;
    int samples[3]{},filteredRSSI=0;
    unsigned nextSample=0,displayLevel=0;
    void clearSamples() {
        lastSample=-1;lastRSSI=0;filteredRSSI=0;nextSample=0;displayLevel=0;
        std::fill(std::begin(samples),std::end(samples),0);
    }
    void clear() {
        std::memset(ssid,0,sizeof(ssid));std::memset(bssid,0,sizeof(bssid));
        ssidLength=0;channel=0;hasAssociation=false;clearSamples();
    }
    void observe(const RTWControlStatus& s,double now) {
        if(phase(s)!=Phase::Connected || s.ssidLength>sizeof(ssid) || !std::isfinite(now) || now<0) {
            clear();return;
        }
        if(!hasAssociation || ssidLength!=s.ssidLength || channel!=s.channel ||
           std::memcmp(ssid,s.ssid,s.ssidLength) || std::memcmp(bssid,s.bssid,sizeof(bssid))) {
            clear();ssidLength=s.ssidLength;channel=s.channel;
            std::memcpy(ssid,s.ssid,s.ssidLength);std::memcpy(bssid,s.bssid,sizeof(bssid));
            hasAssociation=true;
        }
        if(!validSignal(s.rssi)) {
            // New drivers expire hardware samples themselves. Do not add a
            // second app grace period after an authoritative expiry.
            if(s.flags&RTW_CONTROL_SIGNAL_TRACKED)clearSamples();
            return;
        }
        if(lastSample<0 || now<lastSample || now-lastSample>10.0) {
            std::fill(std::begin(samples),std::end(samples),s.rssi);
            displayLevel=0;nextSample=0;
        } else {
            samples[nextSample]=s.rssi;nextSample=(nextSample+1)%3;
        }
        int ordered[3]={samples[0],samples[1],samples[2]};
        std::sort(std::begin(ordered),std::end(ordered));
        filteredRSSI=ordered[1];
        displayLevel=stableSignalLevel(filteredRSSI,displayLevel);
        lastRSSI=s.rssi;lastSample=now;
    }
    bool fresh(double now) const {
        return std::isfinite(now) && lastSample>=0 && now>=lastSample && now-lastSample<=10.0;
    }
    int value(double now) const { return fresh(now)?filteredRSSI:0; }
    int rawValue(double now) const { return fresh(now)?lastRSSI:0; }
    unsigned level(double now) const { return fresh(now)?displayLevel:0; }
};
inline bool confirmsJoin(const RTWControlStatus& s,const uint8_t* ssid,size_t n,
                         uint64_t pollGeneration,uint64_t joinGeneration,bool acknowledged) {
    return acknowledged && pollGeneration==joinGeneration && ssid && n && n<=32 &&
        phase(s)==Phase::Connected && s.ssidLength==n && !std::memcmp(s.ssid,ssid,n);
}
inline unsigned channelFrequencyMHz(unsigned channel) {
    if(channel>=1&&channel<=13)return 2407+5*channel;
    if(channel==14)return 2484;
    if((channel>=36&&channel<=64&&channel%4==0)||
       (channel>=100&&channel<=144&&channel%4==0)||
       (channel>=149&&channel<=177&&(channel-149)%4==0))return 5000+5*channel;
    return 0;
}
inline const char* bandDescription(unsigned channel) {
    const unsigned frequency=channelFrequencyMHz(channel);
    return !frequency?"unknown band":frequency<3000?"2.4 GHz":"5 GHz";
}
inline unsigned stationDescriptionIndex(uint32_t flags) {
    const bool two=!(flags&RTW_CONTROL_BANDS_EXPLICIT)||(flags&RTW_CONTROL_BAND_2GHZ);
    return (two?1U:0U)|((flags&RTW_CONTROL_BAND_5GHZ)?2U:0U)|
        ((flags&RTW_CONTROL_BAND_5GHZ_DFS_LOW)?4U:0U)|
        ((flags&RTW_CONTROL_BAND_5GHZ_DFS_MID)?8U:0U)|
        ((flags&RTW_CONTROL_BAND_5GHZ_HIGH)?16U:0U);
}
inline std::array<std::string,32> makeStationDescriptions(bool detailed) {
    std::array<std::string,32> result;
    for(unsigned index=0;index<result.size();++index) {
        const bool two=(index&1U)!=0;
        std::vector<std::string> groups;
        if(index&2U)groups.emplace_back(detailed?"36/40/44/48":"36–48");
        if(index&4U)groups.emplace_back(detailed?"52/56/60/64 (DFS)":"52–64 DFS");
        if(index&8U)groups.emplace_back(detailed?"100/104/108/112/116/120/124/128/132/136/140 (DFS)":"100–140 DFS");
        if(index&16U)groups.emplace_back(detailed?"149/153/157/161/165":"149–165");
        if(groups.empty()) {
            result[index]=two?(detailed?"2.4 GHz (channels 1–11)":"2.4 GHz"):
                (detailed?"No authorized station channels":"No authorized band");
            continue;
        }
        std::string channels;
        for(unsigned i=0;i<groups.size();++i) {
            if(i)channels+=detailed&&i+1==groups.size()?" and ":", ";
            channels+=groups[i];
        }
        result[index]=two?(detailed?"2.4 GHz (channels 1–11) or 5 GHz (":"2.4 + 5 GHz ("):"5 GHz (";
        result[index]+=channels+")";
    }
    return result;
}
inline const char* stationBandDescription(uint32_t flags) {
    static const auto descriptions=makeStationDescriptions(false);
    return descriptions[stationDescriptionIndex(flags)].c_str();
}
inline const char* stationChannelDescription(uint32_t flags) {
    static const auto descriptions=makeStationDescriptions(true);
    return descriptions[stationDescriptionIndex(flags)].c_str();
}
inline std::string mappedStationDescription(const RTWControlChannelMap& map,bool detailed) {
    if(!rtw_control_channel_map_valid(&map))return detailed?"No authorized station channels":"No authorized band";
    const uint8_t groups[][11]={{1,2,3,4,5,6,7,8,9,10,11},{36,40,44,48},{52,56,60,64},
        {100,104,108,112,116,120,124,128,132,136,140},{149,153,157,161,165}};
    const unsigned counts[]={11,4,4,11,5};
    const char* ranges[]={"1–11","36–48","52–64","100–140","149–165"};
    std::string two,five;
    for(unsigned group=0;group<5;++group) {
        std::string list;unsigned count=0;
        for(unsigned i=0;i<counts[group];++i)if(rtw_control_channel_map_contains(&map,groups[group][i])) {
            if(count++)list+='/';list+=std::to_string(groups[group][i]);
        }
        if(!count)continue;
        if(count==counts[group]&&(!detailed||group==0))list=ranges[group];
        if(group==2||group==3)list+=detailed?" (DFS)":" DFS";
        if(group==0)two="2.4 GHz ("+list+")";
        else {if(!five.empty())five+=", ";five+=list;}
    }
    if(!five.empty())five="5 GHz ("+five+")";
    if(two.empty())return five.empty()?(detailed?"No authorized station channels":"No authorized band"):five;
    return five.empty()?two:two+(detailed?" or ":" + ")+five;
}
inline std::string stationBandDescription(uint32_t flags,const RTWControlChannelMap* map) {
    return map?mappedStationDescription(*map,false):stationBandDescription(flags);
}
inline std::string stationChannelDescription(uint32_t flags,const RTWControlChannelMap* map) {
    return map?mappedStationDescription(*map,true):stationChannelDescription(flags);
}
inline bool supported(const RTWControlScanRecord& r,uint32_t stationFlags=0,const RTWControlChannelMap* map=nullptr) {
    return (r.securityFlags&7)==7 && !(r.securityFlags&RTW_CONTROL_SECURITY_UNSUPPORTED)
        && (map?(rtw_control_channel_map_valid(map)&&rtw_control_channel_map_contains(map,r.channel)):
            rtw_control_channel_supported(r.channel,stationFlags));
}
inline bool observedSSIDSupported(const RTWControlScanResults& scan,const uint8_t* ssid,size_t length,
    uint32_t flags,const RTWControlChannelMap* map=nullptr) {
    if(!ssid||!length||length>32||scan.version!=RTW_CONTROL_VERSION||scan.count>RTW_CONTROL_MAX_NETWORKS||scan.truncated>1)return false;
    for(unsigned i=0;i<scan.count;++i) {
        const auto& record=scan.records[i];
        if(record.ssidLength==length&&!std::memcmp(record.ssid,ssid,length)&&supported(record,flags,map))return true;
    }
    return false;
}
// A saved hidden2.4GHz profile cannot appear by name in a wildcard scan.
// Preserve directed discovery only where the current driver permits active
//2.4GHz. A visible same-SSID refusal cannot be bypassed by this fallback;
//5GHz/DFS still requires a supported observed BSS and kernel admission.
inline bool savedProfileReconnectAllowed(const RTWControlScanResults& scan,const uint8_t* ssid,size_t length,
    uint32_t flags,const RTWControlChannelMap* map) {
    if(!ssid||!length||length>32||scan.version!=RTW_CONTROL_VERSION||scan.count>RTW_CONTROL_MAX_NETWORKS||scan.truncated>1)return false;
    bool visibleMatch=false;
    for(unsigned i=0;i<scan.count;++i) {
        const auto& record=scan.records[i];
        if(record.ssidLength!=length||std::memcmp(record.ssid,ssid,length))continue;
        visibleMatch=true;
        if(supported(record,flags,map))return true;
    }
    if(visibleMatch)return false;
    if(map) {
        if(!rtw_control_channel_map_valid(map))return false;
        for(unsigned channel=1;channel<=11;++channel)
            if(rtw_control_channel_map_contains(map,channel))return true;
        return false;
    }
    // NULL is supplied only after explicit old-driver Unsupported fallback.
    return rtw_control_channel_supported(1,flags)!=0;
}
// Grouping an SSID must not hide another usable band behind its strongest BSS.
inline unsigned observedNetworkBands(const RTWControlScanResults& scan,const RTWControlScanRecord& group,uint32_t flags,const RTWControlChannelMap* map=nullptr) {
    if(scan.version!=1||scan.count>64||scan.truncated>1||!group.ssidLength||group.ssidLength>32)return 0;
    unsigned bands=0;
    for(unsigned i=0;i<scan.count;++i){const auto&r=scan.records[i];
        if(r.ssidLength!=group.ssidLength||r.securityFlags!=group.securityFlags||
           std::memcmp(r.ssid,group.ssid,group.ssidLength)||!supported(r,flags,map))continue;
        bands|=r.channel<=14?1U:2U;
    }
    return bands;
}
inline const char* observedNetworkBandsText(unsigned bands) {
    return bands==3?"2.4 + 5 GHz":bands==2?"5 GHz":bands==1?"2.4 GHz":"No supported observed band";
}
inline std::vector<RTWControlScanRecord> networks(const RTWControlScanResults& scan,uint32_t stationFlags=0,const RTWControlChannelMap* map=nullptr) {
    std::vector<RTWControlScanRecord> out;
    if (scan.version!=1 || scan.count>64 || scan.truncated>1) return out;
    out.reserve(scan.count);
    for (unsigned i=0;i<scan.count;++i) {
        auto r=scan.records[i];
        if(!validSignal(r.rssi))r.rssi=0;
        if (!r.ssidLength || r.ssidLength>32) continue;
        auto it=std::find_if(out.begin(),out.end(),[&](const auto& existing){
            return existing.ssidLength==r.ssidLength && existing.securityFlags==r.securityFlags &&
                !std::memcmp(existing.ssid,r.ssid,r.ssidLength);
        });
        if (it==out.end()) out.push_back(r);
        // Prefer the strongest BSS the selected live driver can use. A strong
        // unsupported band must never hide a usable BSS with the same SSID.
        else if ((supported(r,stationFlags,map) && !supported(*it,stationFlags,map)) ||
                 (supported(r,stationFlags,map)==supported(*it,stationFlags,map) &&
                  r.rssi && (!it->rssi || r.rssi>it->rssi))) *it=r;
    }
    std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){
        if (a.rssi!=b.rssi) return (a.rssi?a.rssi:-999)>(b.rssi?b.rssi:-999);
        const int order=std::memcmp(a.ssid,b.ssid,std::min(a.ssidLength,b.ssidLength));
        if (order) return order<0;
        if (a.ssidLength!=b.ssidLength) return a.ssidLength<b.ssidLength;
        return a.securityFlags<b.securityFlags;
    });
    return out;
}
inline bool validPassword(const char* p,size_t n) {
    if (!p || n<8 || n>63) return false;
    for (size_t i=0;i<n;++i) if (static_cast<unsigned char>(p[i])<32 || static_cast<unsigned char>(p[i])>126) return false;
    return true;
}
inline std::string displaySSID(const uint8_t* bytes,size_t n) {
    if (!bytes || n>32) return "[Invalid SSID]";
    bool safe=true;
    for (size_t i=0;i<n && safe;) {
        uint32_t cp=bytes[i++];unsigned extra=0;uint32_t minimum=0;
        if (cp>=0xc2 && cp<=0xdf) { cp&=31;extra=1;minimum=128; }
        else if (cp>=0xe0 && cp<=0xef) { cp&=15;extra=2;minimum=2048; }
        else if (cp>=0xf0 && cp<=0xf4) { cp&=7;extra=3;minimum=65536; }
        else if (cp>=128) {safe=false;break;}
        if (i+extra>n) {safe=false;break;}
        for (unsigned k=0;k<extra;++k) {
            const auto b=bytes[i++];if ((b&0xc0)!=0x80) {safe=false;break;}cp=(cp<<6)|(b&63);
        }
        if (cp<minimum || cp>0x10ffff || (cp>=0xd800 && cp<=0xdfff) || cp<32 ||
            (cp>=127 && cp<=159) || (cp>=0x202a && cp<=0x202e) ||
            (cp>=0x2066 && cp<=0x2069) || cp==0x200e || cp==0x200f) safe=false;
    }
    if (safe) return std::string(reinterpret_cast<const char*>(bytes),n);
    std::string out="[";const char* hex="0123456789abcdef";
    for (size_t i=0;i<n;++i) {out+=hex[bytes[i]>>4];out+=hex[bytes[i]&15];}
    return out+"]";
}
inline void wipe(void* p,size_t n) { volatile uint8_t* b=static_cast<volatile uint8_t*>(p);while(n--) *b++=0; }
}
