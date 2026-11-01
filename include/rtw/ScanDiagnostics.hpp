#pragma once
// SPDX-License-Identifier: GPL-2.0-only
#include <stddef.h>
#include <stdint.h>

namespace rtw {
enum class ScanDiagnosticState : uint32_t { Idle=0, Active=1, Complete=2, Cancelled=3, Failed=4 };
enum class ScanDiagnosticTransition { Scan, Auth, Init, Other };
struct ScanDiagnostics {
    uint64_t generation=0, hopAttempts=0, hops=0, invalidChannelHops=0;
    uint64_t visited[4]{};
    uint64_t beaconFrames=0, probeResponseFrames=0;
    uint64_t software24Frames=0, software5Frames=0, softwareUnknownFrames=0;
    uint64_t phy24Frames=0, phy5Frames=0, unknownPHYFrames=0, phyChannelMismatches=0;
    uint32_t lastChannel=0, lastVisitedChannel=0, lastPHYChannel=0, lastError=0;
    ScanDiagnosticState state=ScanDiagnosticState::Idle;
};
inline bool scanDiagnosticChannelInRange(unsigned channel) { return channel>=1&&channel<=196; }
// Diagnostic classification only. It grants no channel or transmit permission.
// Zero, reserved channel numbers and undecoded PHY values stay unknown.
inline unsigned scanDiagnosticBand(unsigned channel) {
    if(channel>=1&&channel<=14)return 24;
    if((channel>=36&&channel<=144&&channel%4==0)||
       (channel>=149&&channel<=177&&(channel-149)%4==0)||
       (channel>=184&&channel<=196&&channel%4==0))return 5;
    return 0;
}
inline void scanDiagnosticsTransition(ScanDiagnostics&d,bool previousScanning,
    ScanDiagnosticTransition requested,unsigned channel,int result) {
    if(requested==ScanDiagnosticTransition::Scan){
        if(!previousScanning||d.state!=ScanDiagnosticState::Active){
            uint64_t generation=d.generation+1;if(!generation)generation=1;
            d=ScanDiagnostics{};d.generation=generation;
        }
        ++d.hopAttempts;d.lastChannel=scanDiagnosticChannelInRange(channel)?channel:0;
        d.lastError=(uint32_t)result;
        if(result){d.state=ScanDiagnosticState::Failed;return;}
        d.state=ScanDiagnosticState::Active;++d.hops;
        if(scanDiagnosticChannelInRange(channel)){
            // The shift is always 0..63, including channels 64, 128 and 196.
            d.visited[channel/64]|=UINT64_C(1)<<(channel%64);
            d.lastVisitedChannel=channel;
        }else ++d.invalidChannelHops;
        return;
    }
    if(!previousScanning||d.state!=ScanDiagnosticState::Active)return;
    d.lastError=(uint32_t)result;
    if(result)d.state=ScanDiagnosticState::Failed;
    else d.state=requested==ScanDiagnosticTransition::Auth?ScanDiagnosticState::Complete:ScanDiagnosticState::Cancelled;
}
inline void scanDiagnosticsFrame(ScanDiagnostics&d,bool scanning,const uint8_t*frame,
    size_t length,unsigned softwareChannel,unsigned physicalChannel) {
    if(!scanning||d.state!=ScanDiagnosticState::Active||!frame||length<36||
       (frame[0]!=0x80&&frame[0]!=0x50)||(frame[1]&(0x40U|0x03U)))return;
    if(frame[0]==0x80)++d.beaconFrames;else ++d.probeResponseFrames;
    unsigned softwareBand=scanDiagnosticBand(softwareChannel),physicalBand=scanDiagnosticBand(physicalChannel);
    if(softwareBand==24)++d.software24Frames;
    else if(softwareBand==5)++d.software5Frames;
    else ++d.softwareUnknownFrames;
    d.lastPHYChannel=physicalBand?physicalChannel:0;
    if(physicalBand==24)++d.phy24Frames;
    else if(physicalBand==5)++d.phy5Frames;
    else ++d.unknownPHYFrames;
    if(softwareBand&&physicalBand&&softwareChannel!=physicalChannel)++d.phyChannelMismatches;
}
template<class Publish> inline void publishScanDiagnostics(const ScanDiagnostics&d,Publish publish) {
    publish("RTWDiagScanGeneration",d.generation);publish("RTWDiagScanState",(uint32_t)d.state);
    publish("RTWDiagScanHopAttempts",d.hopAttempts);publish("RTWDiagScanHops",d.hops);
    publish("RTWDiagScanVisited0",d.visited[0]);publish("RTWDiagScanVisited1",d.visited[1]);
    publish("RTWDiagScanVisited2",d.visited[2]);publish("RTWDiagScanVisited3",d.visited[3]);
    publish("RTWDiagScanLastChannel",d.lastChannel);publish("RTWDiagScanLastVisitedChannel",d.lastVisitedChannel);
    publish("RTWDiagScanInvalidChannelHops",d.invalidChannelHops);publish("RTWDiagScanLastError",d.lastError);
    // Reception counters count frames, including repeat observations of one BSS.
    publish("RTWDiagScanBeaconFrames",d.beaconFrames);publish("RTWDiagScanProbeResponseFrames",d.probeResponseFrames);
    publish("RTWDiagScanSoftware24Frames",d.software24Frames);publish("RTWDiagScanSoftware5Frames",d.software5Frames);
    publish("RTWDiagScanSoftwareUnknownFrames",d.softwareUnknownFrames);
    publish("RTWDiagScanPHY24Frames",d.phy24Frames);publish("RTWDiagScanPHY5Frames",d.phy5Frames);
    publish("RTWDiagScanUnknownPHYFrames",d.unknownPHYFrames);
    publish("RTWDiagScanPHYChannelMismatches",d.phyChannelMismatches);publish("RTWDiagScanLastPHYChannel",d.lastPHYChannel);
}
}
