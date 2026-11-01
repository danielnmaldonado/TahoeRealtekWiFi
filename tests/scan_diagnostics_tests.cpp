#include "rtw/ScanDiagnostics.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"Scan diagnostics line %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using State=rtw::ScanDiagnosticState;
using Transition=rtw::ScanDiagnosticTransition;
static void successfulHopsUseFourBoundedWords(){
    rtw::ScanDiagnostics d;
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,1,0);
    CHECK(d.generation==1&&d.state==State::Active&&d.hops==1&&d.hopAttempts==1);
    for(unsigned channel:{64U,128U,149U,196U})rtw::scanDiagnosticsTransition(d,true,Transition::Scan,channel,0);
    // Word index = channel / 64; bit index = channel % 64. Bit zero of word 0 is unused.
    CHECK(d.visited[0]==UINT64_C(2)&&d.visited[1]==UINT64_C(1));
    CHECK(d.visited[2]==UINT64_C(2097153)&&d.visited[3]==UINT64_C(16));
    CHECK(d.hops==5&&d.hopAttempts==5&&d.lastChannel==196&&d.lastVisitedChannel==196);
    for(unsigned channel:{0U,197U,256U,~0U})rtw::scanDiagnosticsTransition(d,true,Transition::Scan,channel,0);
    CHECK(d.invalidChannelHops==4&&d.hops==9&&d.lastVisitedChannel==196);
    CHECK(d.visited[0]==2&&d.visited[1]==1&&d.visited[2]==2097153&&d.visited[3]==16);
}
static void failureAndTerminalTransitionsPreserveEvidenceUntilRestart(){
    rtw::ScanDiagnostics d;
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,36,EIO);
    CHECK(d.generation==1&&d.state==State::Failed&&d.lastError==EIO);
    CHECK(d.hopAttempts==1&&d.hops==0&&d.visited[0]==0);
    rtw::scanDiagnosticsTransition(d,true,Transition::Scan,149,0);
    CHECK(d.generation==2&&d.state==State::Active&&d.lastError==0&&d.hops==1);
    CHECK(d.visited[2]==2097152&&d.visited[0]==0);
    rtw::scanDiagnosticsTransition(d,true,Transition::Scan,153,ETIMEDOUT);
    CHECK(d.state==State::Failed&&d.hops==1&&d.hopAttempts==2&&d.lastChannel==153&&d.lastVisitedChannel==149);
    CHECK(d.visited[2]==2097152&&d.lastError==ETIMEDOUT);
    rtw::scanDiagnosticsTransition(d,true,Transition::Init,153,0);
    CHECK(d.state==State::Failed&&d.lastError==ETIMEDOUT&&d.visited[2]==2097152);
    rtw::scanDiagnosticsTransition(d,true,Transition::Scan,64,0);
    CHECK(d.generation==3&&d.visited[1]==1&&d.visited[2]==0&&d.hopAttempts==1);
    rtw::scanDiagnosticsTransition(d,true,Transition::Auth,64,0);
    CHECK(d.state==State::Complete&&d.visited[1]==1&&d.hops==1);
    rtw::scanDiagnosticsTransition(d,false,Transition::Init,64,0);
    CHECK(d.state==State::Complete&&d.visited[1]==1);
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,128,0);
    rtw::scanDiagnosticsTransition(d,true,Transition::Init,128,0);
    CHECK(d.generation==4&&d.state==State::Cancelled&&d.visited[2]==1);
    rtw::scanDiagnosticsTransition(d,true,Transition::Scan,196,0);
    rtw::scanDiagnosticsTransition(d,true,Transition::Auth,196,EIO);
    CHECK(d.generation==5&&d.state==State::Failed&&d.lastError==EIO&&d.visited[3]==16);
}
static void frameBandsKeepSoftwareAndPhysicalEvidenceSeparate(){
    rtw::ScanDiagnostics d;uint8_t frame[40]{};frame[0]=0x80;
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,1,0);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),1,0);
    CHECK(d.beaconFrames==1&&d.software24Frames==1&&d.unknownPHYFrames==1&&d.phy24Frames==0);
    frame[0]=0x50;rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),149,149);
    CHECK(d.probeResponseFrames==1&&d.software5Frames==1&&d.phy5Frames==1&&d.phyChannelMismatches==0);
    frame[0]=0x80;rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),1,149);
    CHECK(d.software24Frames==2&&d.phy5Frames==2&&d.phyChannelMismatches==1&&d.lastPHYChannel==149);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),149,197);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),149,15);
    CHECK(d.unknownPHYFrames==3&&d.phy5Frames==2&&d.lastPHYChannel==0);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),0,1);
    CHECK(d.softwareUnknownFrames==1&&d.phy24Frames==1&&d.phyChannelMismatches==1);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),149,36);
    CHECK(d.phy5Frames==3&&d.phyChannelMismatches==2);
    rtw::scanDiagnosticsFrame(d,false,frame,sizeof(frame),1,1);
    CHECK(d.beaconFrames==6&&d.probeResponseFrames==1);
    rtw::scanDiagnosticsTransition(d,true,Transition::Auth,149,0);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),1,1);
    CHECK(d.beaconFrames==6&&d.probeResponseFrames==1);
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,36,0);
    CHECK(d.generation==2&&d.beaconFrames==0&&d.software24Frames==0&&d.unknownPHYFrames==0);
}
static void malformedAndNonScanFramesNeverCountAsScanReception(){
    rtw::ScanDiagnostics d;uint8_t frame[40]{};frame[0]=0x80;
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,36,0);
    rtw::scanDiagnosticsFrame(d,true,nullptr,sizeof(frame),36,36);
    rtw::scanDiagnosticsFrame(d,true,frame,35,36,36);
    for(uint8_t first:{0x81U,0x40U,0x08U,0xd0U}){frame[0]=first;rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),36,36);}
    frame[0]=0x80;
    for(uint8_t second:{0x40U,0x01U,0x02U,0x03U}){frame[1]=second;rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),36,36);}
    CHECK(d.beaconFrames==0&&d.probeResponseFrames==0&&d.software5Frames==0&&d.phy5Frames==0);
    frame[1]=0;rtw::scanDiagnosticsFrame(d,true,frame,36,36,36);
    CHECK(d.beaconFrames==1&&d.software5Frames==1&&d.phy5Frames==1);
}
static void publicationContainsOnlyScalarSessionEvidence(){
    rtw::ScanDiagnostics d;uint8_t frame[36]{};frame[0]=0x80;
    rtw::scanDiagnosticsTransition(d,false,Transition::Scan,149,0);
    rtw::scanDiagnosticsFrame(d,true,frame,sizeof(frame),149,0);
    std::map<std::string,uint64_t> scalars;
    rtw::publishScanDiagnostics(d,[&](const char*name,uint64_t value){scalars[name]=value;});
    CHECK(scalars["RTWDiagScanGeneration"]==1&&scalars["RTWDiagScanState"]==1);
    CHECK(scalars["RTWDiagScanHops"]==1&&scalars["RTWDiagScanHopAttempts"]==1);
    CHECK(scalars["RTWDiagScanVisited2"]==2097152&&scalars["RTWDiagScanVisited3"]==0);
    CHECK(scalars["RTWDiagScanSoftware5Frames"]==1&&scalars["RTWDiagScanPHY5Frames"]==0);
    CHECK(scalars["RTWDiagScanUnknownPHYFrames"]==1&&scalars["RTWDiagScanLastError"]==0);
}
int main(){
    successfulHopsUseFourBoundedWords();failureAndTerminalTransitionsPreserveEvidenceUntilRestart();
    frameBandsKeepSoftwareAndPhysicalEvidenceSeparate();malformedAndNonScanFramesNeverCountAsScanReception();
    publicationContainsOnlyScalarSessionEvidence();
    std::printf("Scan diagnostics: %u checks passed\n",checks);
}
