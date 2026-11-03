#include "publication_host.hpp"
#include <iostream>

static unsigned failures=0;
#define CHECK(x) do { if(!(x)) { std::cerr<<__LINE__<<": " #x "\n"; ++failures; } } while(0)

void RTWUSBController::publishWpaDiagnostics() {}

static void publicationDoesNotRunRatePolicy() {
    RTWUSBController c; ieee80211_node n;
    c.core_.sc_ic.ic_bss=&n;
    c.family_=RTW_WIFI_RTL8814AU;
    c.acRateReady_=c.acMediaConnected_=true;
    c.acRateConfig_.ht=1;c.acRateConfig_.mask=0x0ffff010;c.acRateConfig_.tx_streams=2;
    n.ni_tx_ba[3].ba_state=IEEE80211_BA_AGREED;
    n.ni_rx_ba[6].ba_state=IEEE80211_BA_AGREED;
    c.txHTPackets_=7;c.txAMPDUPackets_=5;
    c.publishDiagnostics();
    CHECK(c.properties.count("RTWDiagHTNegotiated")&&c.properties.at("RTWDiagHTNegotiated")==1);
    CHECK(c.properties["RTWDiagHTTXBATIDs"]==8&&c.properties["RTWDiagHTRXBATIDs"]==64);
    CHECK(c.properties["RTWDiagHTFWRateSubmitted"]==7&&c.properties["RTWDiagHTTXAMPDURequested"]==5);
    CHECK(c.rateControl_.changes==0&&acPhyTrace.calls==0);
    c.core_.sc_ic.ic_state=0;
    c.publishDiagnostics();
    CHECK(c.properties["RTWDiagHTNegotiated"]==0&&c.properties["RTWDiagHTRateMask"]==0);
    CHECK(c.properties["RTWDiagHTTXBATIDs"]==0&&c.properties["RTWDiagHTRXBATIDs"]==0);
    CHECK(c.properties["RTWDiagHTTXStreams"]==0);
}

static void everyFamilyPublishesFromStatus() {
    const rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU,
        RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8192DU,
        RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU,
        RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
    for(auto family:families) {
        RTWUSBController c;ieee80211_node n;c.core_.sc_ic.ic_bss=&n;c.family_=family;
        c.publishDiagnostics();
        CHECK(c.properties.count("RTWDiagHTNegotiated"));
        CHECK(c.properties.count("RTWDiagHTTXBATIDs"));
        CHECK(c.properties.count("RTWDiagSiliconFamily")&&c.properties["RTWDiagSiliconFamily"]==family);
    }
}

static void publicationIsFreshAndPerDevice() {
    RTWUSBController a,b;
    publicationHostNow=400;a.publishDiagnostics();
    CHECK(a.properties["RTWDiagPublicationGeneration"]==2);
    CHECK(a.properties["RTWDiagPublicationMonotonicMS"]==400);
    CHECK(a.writes.front().first=="RTWDiagPublicationGeneration"&&a.writes.front().second==1);
    CHECK(a.writes.back().first=="RTWDiagPublicationGeneration"&&a.writes.back().second==2);
    publicationHostNow=450;a.publishDiagnostics();
    CHECK(a.properties["RTWDiagPublicationGeneration"]==4&&a.properties["RTWDiagPublicationMonotonicMS"]==450);
    publicationHostNow=440;a.publishDiagnostics();
    CHECK(a.properties["RTWDiagPublicationGeneration"]==6&&a.properties["RTWDiagPublicationMonotonicMS"]==450);
    publicationHostNow=500;b.publishDiagnostics();
    CHECK(b.properties["RTWDiagPublicationGeneration"]==2&&b.properties["RTWDiagPublicationMonotonicMS"]==500);
    CHECK(a.properties["RTWDiagPublicationGeneration"]==6);
}

static void existingReasonsAreExportedWithoutAttribution() {
    RTWUSBController c;
    c.rxErrors_=14;
    c.rxDiag_.invalidFrameDrops=7;
    c.rxDiag_.rejectedReasons[RTW_RX_REJECT_BAR_RECEIVER]=7;
    c.rxDiag_.rejectLastReason=RTW_RX_REJECT_BAR_RECEIVER;
    c.rxDiag_.rejectLastType=1;c.rxDiag_.rejectLastSubtype=8;
    c.rxDiag_.rejectLastLength=20;c.rxDiag_.rejectLastTransmitterMatch=1;
    c.stats_->inputErrors=5;c.publishDiagnostics();c.stats_->inputErrors=12;
    c.core_.sc_ic.ic_stats.is_ht_rx_ba_no_buf=2;
    c.core_.sc_ic.ic_stats.is_ht_rx_frame_above_ba_winend=3;
    c.core_.sc_ic.ic_stats.is_ht_rx_ba_window_slide=4;
    c.core_.sc_ic.ic_stats.is_ht_rx_ba_window_jump=5;
    c.core_.sc_ic.ic_stats.is_ht_rx_ba_frame_lost=6;
    c.core_.sc_ic.ic_stats.is_ht_rx_ba_window_gap_timeout=7;
    c.core_.sc_ic.ic_stats.is_ht_rx_frame_below_ba_winstart=8;
    c.core_.sc_ic.ic_stats.is_ht_rx_ba_timeout=9;
    c.core_.sc_ic.ic_stats.is_ht_tx_ba_timeout=10;
    c.publishDiagnostics();
    CHECK(c.properties["RTWDiagNet80211InputErrors"]==12);
    CHECK(c.properties["RTWDiagRXErrors"]==14);
    CHECK(c.properties.count("RTWDiagRXBlockAckFiltered"));
    CHECK(c.properties["RTWDiagRXBlockAckFiltered"]==0);
    CHECK(c.properties.count("RTWDiagRXRejectControlSubtype9"));
    CHECK(c.properties["RTWDiagRXRejectLastBAControl"]==65535);
    CHECK(c.properties["RTWDiagRXRejectLastBAFragment"]==255);
    CHECK(c.properties["RTWDiagRXInvalidFrameDrops"]==7);
    CHECK(c.properties["RTWDiagRXRejectBARReceiver"]==7);
    CHECK(c.properties["RTWDiagRXRejectLastReason"]==RTW_RX_REJECT_BAR_RECEIVER);
    CHECK(c.properties["RTWDiagRXRejectLastType"]==1&&c.properties["RTWDiagRXRejectLastSubtype"]==8);
    CHECK(c.properties["RTWDiagRXRejectLastLength"]==20);
    CHECK(c.properties["RTWDiagRXRejectLastReceiverMatchesLocal"]==0);
    CHECK(c.properties["RTWDiagRXRejectLastTransmitterMatchesAP"]==1);
    CHECK(c.rxErrors_==14&&c.stats_->inputErrors==12&&c.rxDiag_.invalidFrameDrops==7);
    CHECK(c.properties["RTWDiagNet80211RXBANoBuffer"]==2);
    CHECK(c.properties["RTWDiagNet80211RXBAAboveWindow"]==3);
    CHECK(c.properties["RTWDiagNet80211RXBAWindowSlides"]==4);
    CHECK(c.properties["RTWDiagNet80211RXBAWindowJumps"]==5);
    CHECK(c.properties["RTWDiagNet80211RXBAFramesLost"]==6);
    CHECK(c.properties["RTWDiagNet80211RXBAWindowGapTimeouts"]==7);
    CHECK(c.properties["RTWDiagNet80211RXBABelowWindow"]==8);
    CHECK(c.properties["RTWDiagNet80211RXBATimeouts"]==9);
    CHECK(c.properties["RTWDiagNet80211TXBATimeouts"]==10);
    CHECK(c.properties["RTWDiagNet80211NoBuffer"]==0);
    CHECK(c.properties.count("RTWDiagNet80211DefragFailures")==0);
}

static void filteredBlockAckAndControlRejectionsRemainDistinct() {
    RTWUSBController c;
    c.rxErrors_=2;c.stats_->inputErrors=2;
    c.rxDiag_.blockAckFiltered=89;c.rxDiag_.invalidFrameDrops=2;
    c.rxDiag_.rejectedReasons[RTW_RX_REJECT_CONTROL]=2;
    c.rxDiag_.rejectedControlSubtypes[9]=2;
    c.rxDiag_.rejectLastBAControl=6;c.rxDiag_.rejectLastBAFragment=1;
    c.publishDiagnostics();
    CHECK(c.properties["RTWDiagRXRejectSchema"]==2);
    CHECK(c.properties["RTWDiagRXBlockAckFiltered"]==89);
    CHECK(c.properties["RTWDiagRXErrors"]==2);
    CHECK(c.properties["RTWDiagRXRejectControl"]==2);
    CHECK(c.properties["RTWDiagRXRejectControlSubtype9"]==2);
    CHECK(c.properties["RTWDiagRXRejectControlSubtype8"]==0);
    CHECK(c.properties["RTWDiagRXRejectLastBAControl"]==6);
    CHECK(c.properties["RTWDiagRXRejectLastBAFragment"]==1);
    CHECK(c.rxErrors_==2&&c.stats_->inputErrors==2&&c.rxDiag_.blockAckFiltered==89);
}

static void unknownPhySpeedAndSelectedMedium() {
    RTWUSBController c;
    CHECK(c.actualAutomaticMediumSpeed()==0);
    RTWUSBController::IONetworkMedium medium{0};
    CHECK(c.selectMedium(&medium)==RTWUSBController::kIOReturnSuccess);
    CHECK(c.selectedCalls==1&&c.currentCalls==0);
    CHECK(c.selectMedium(nullptr)==RTWUSBController::kIOReturnBadArgument);
    c.publishDiagnostics();
    CHECK(c.properties.count("RTWDiagLinkSpeedKnown")&&c.properties["RTWDiagLinkSpeedKnown"]==0);
    CHECK(c.properties.count("RTWDiagLinkSpeedBps")&&c.properties["RTWDiagLinkSpeedBps"]==0);
}

static void nativeCountersAreObservedAndInvalidSnapshotsCleared() {
    RTWUSBController c;publication_host_ifnet native;publication_host_interface iface;
    native.stats.packets_in=17;native.stats.bytes_in=4000;native.stats.errors_in=3;
    native.stats.packets_out=13;native.stats.bytes_out=2000;native.stats.errors_out=2;
    iface.iface=&native;c.netif_=&iface;c.publishDiagnostics();
    CHECK(native.reads==1&&native.stats.packets_in==17&&native.stats.bytes_in==4000);
    CHECK(c.properties["RTWDiagNativeIfnetStatsValid"]==1);
    CHECK(c.properties["RTWDiagNativeIfnetStatsError"]==0);
    CHECK(c.properties["RTWDiagNativeIfnetIndex"]==7);
    CHECK(c.properties["RTWDiagNativeIfnetInputPackets"]==17);
    CHECK(c.properties["RTWDiagNativeIfnetInputBytes"]==4000);
    CHECK(c.properties["RTWDiagNativeIfnetInputErrors"]==3);
    CHECK(c.properties["RTWDiagNativeIfnetOutputPackets"]==13);
    CHECK(c.properties["RTWDiagNativeIfnetOutputBytes"]==2000);
    CHECK(c.properties["RTWDiagNativeIfnetOutputErrors"]==2);
    CHECK(c.properties["RTWDiagNativeIfnetSupportedCapabilities"]==0x400);
    CHECK(c.properties["RTWDiagNativeIfnetEnabledCapabilities"]==0);
    CHECK(c.properties["RTWDiagNativeIfnetSkywalkEnabled"]==0);
    native.enabled=0x400;native.error=5;c.publishDiagnostics();
    CHECK(c.properties["RTWDiagNativeIfnetSkywalkEnabled"]==1);
    CHECK(c.properties["RTWDiagNativeIfnetStatsValid"]==0&&c.properties["RTWDiagNativeIfnetStatsError"]==5);
    CHECK(c.properties["RTWDiagNativeIfnetInputPackets"]==0&&c.properties["RTWDiagNativeIfnetInputBytes"]==0);
    CHECK(c.properties["RTWDiagNativeIfnetOutputPackets"]==0&&c.properties["RTWDiagNativeIfnetOutputBytes"]==0&&c.properties["RTWDiagNativeIfnetOutputErrors"]==0);
    c.closing_=true;c.publishDiagnostics();CHECK(native.reads==2);
    c.closing_=false;c.attached_=false;c.publishDiagnostics();CHECK(native.reads==2);
    c.attached_=true;iface.iface=nullptr;c.publishDiagnostics();CHECK(native.reads==2);
    CHECK(c.properties["RTWDiagNativeIfnetStatsValid"]==0&&c.properties["RTWDiagNativeIfnetStatsError"]==6);
    CHECK(c.properties["RTWDiagNativeIfnetIndex"]==0);
    CHECK(c.properties["RTWDiagNativeIfnetSupportedCapabilities"]==0&&c.properties["RTWDiagNativeIfnetEnabledCapabilities"]==0);
    c.netif_=nullptr;c.publishDiagnostics();CHECK(native.reads==2);
}

static void ethernetSubmissionBoundaryIsObservable() {
    RTWUSBController c;auto& d=c.core_.sc_ic.ic_if.if_input_diag;
    d.calls=1;d.offeredPackets=2;d.offeredBytes=3;d.immediateReturns=4;
    d.flushCalls=5;d.flushPackets=6;d.submissionMismatches=7;d.noIfnetCalls=8;c.publishDiagnostics();
    CHECK(c.properties["RTWDiagRXInputCalls"]==1);
    CHECK(c.properties["RTWDiagRXInputOfferedPackets"]==2);
    CHECK(c.properties["RTWDiagRXInputOfferedBytes"]==3);
    CHECK(c.properties["RTWDiagRXInputImmediateReturns"]==4);
    CHECK(c.properties["RTWDiagRXInputFlushCalls"]==5);
    CHECK(c.properties["RTWDiagRXInputFlushPackets"]==6);
    CHECK(c.properties["RTWDiagRXInputSubmissionMismatches"]==7);
    CHECK(c.properties["RTWDiagRXInputNoIfnetCalls"]==8);
    CHECK(c.stats_->inputPackets==0&&c.stats_->inputErrors==0);
}

static void nativeBoundarySnapshotsRemainSeparateAndReadOnly() {
    RTWUSBController c;publication_host_interface iface,other;c.netif_=&iface;
    c.core_.sc_ic.ic_if.iface=&other;
    auto& d=c.core_.sc_ic.ic_if.if_input_diag;
    d.nativeBefore={11,1200,1,12,1300,2,7,0x400,0,0};
    d.nativeAfter={14,1500,3,15,1600,4,7,0x400,0,0};
    c.publishDiagnostics();
    CHECK(c.properties.count("RTWDiagRXNativeSampleCount"));
    CHECK(c.properties["RTWDiagRXNativeBeforeStatsValid"]==0);
    CHECK(c.properties["RTWDiagRXNativeBeforeInputPackets"]==0);
    CHECK(c.properties["RTWDiagRXInputInterfaceMatchesChild"]==0);
    d.nativeSampleCount=1;d.nativeSampleBatch=1025;d.nativeSampleOfferedPackets=3;
    d.nativeSampleOfferedBytes=300;d.nativeSampleImmediateReturns=0;
    d.nativeSampleFlushPackets=3;d.nativeSampleIfnetStable=1;
    c.core_.sc_ic.ic_if.iface=&iface;c.publishDiagnostics();
    CHECK(c.properties["RTWDiagRXInputInterfaceMatchesChild"]==1);
    CHECK(c.properties["RTWDiagRXNativeSampleCount"]==1&&c.properties["RTWDiagRXNativeSampleBatch"]==1025);
    CHECK(c.properties["RTWDiagRXNativeSampleOfferedPackets"]==3&&c.properties["RTWDiagRXNativeSampleOfferedBytes"]==300);
    CHECK(c.properties["RTWDiagRXNativeSampleImmediateReturns"]==0&&c.properties["RTWDiagRXNativeSampleFlushPackets"]==3);
    CHECK(c.properties["RTWDiagRXNativeSampleIfnetStable"]==1);
    CHECK(c.properties["RTWDiagRXNativeBeforeStatsValid"]==1&&c.properties["RTWDiagRXNativeAfterStatsValid"]==1);
    CHECK(c.properties["RTWDiagRXNativeBeforeInputPackets"]==11&&c.properties["RTWDiagRXNativeAfterInputPackets"]==14);
    CHECK(c.properties["RTWDiagRXNativeBeforeInputBytes"]==1200&&c.properties["RTWDiagRXNativeAfterInputBytes"]==1500);
    CHECK(c.properties["RTWDiagRXNativeBeforeInputErrors"]==1&&c.properties["RTWDiagRXNativeAfterInputErrors"]==3);
    CHECK(c.properties["RTWDiagRXNativeBeforeOutputPackets"]==12&&c.properties["RTWDiagRXNativeAfterOutputPackets"]==15);
    CHECK(c.properties["RTWDiagRXNativeBeforeOutputBytes"]==1300&&c.properties["RTWDiagRXNativeAfterOutputBytes"]==1600);
    CHECK(c.properties["RTWDiagRXNativeBeforeOutputErrors"]==2&&c.properties["RTWDiagRXNativeAfterOutputErrors"]==4);
    CHECK(c.properties["RTWDiagRXNativeBeforeIndex"]==7&&c.properties["RTWDiagRXNativeAfterSupportedCapabilities"]==0x400);
    d.nativeAfter.statsError=5;c.publishDiagnostics();
    CHECK(c.properties["RTWDiagRXNativeBeforeStatsValid"]==1&&c.properties["RTWDiagRXNativeAfterStatsValid"]==0);
    CHECK(c.properties["RTWDiagRXNativeAfterStatsError"]==5&&c.properties["RTWDiagRXNativeAfterInputPackets"]==0);
    CHECK(c.properties["RTWDiagRXNativeAfterOutputPackets"]==0);
    CHECK(d.nativeAfter.packetsIn==14&&d.nativeAfter.packetsOut==15&&d.nativeBefore.packetsIn==11);
    c.closing_=true;c.publishDiagnostics();CHECK(c.properties["RTWDiagRXInputInterfaceMatchesChild"]==0);
    c.closing_=false;c.attached_=false;c.publishDiagnostics();CHECK(c.properties["RTWDiagRXInputInterfaceMatchesChild"]==0);
    c.netif_=nullptr;c.publishDiagnostics();CHECK(c.properties["RTWDiagRXInputInterfaceMatchesChild"]==0);
}

static void operatingFailureIsNotHiddenByAttachSuccess() {
    RTWUSBController c;c.core_.core_error=60;c.attached_=true;c.present_=true;
    c.enabled_=c.radioReady_=c.running_=false;
    c.publishDiagnostics();
    CHECK(c.properties.count("RTWDiagCoreError")&&c.properties["RTWDiagCoreError"]==60);
    CHECK(c.properties["RTWDiagAttached"]==1&&c.properties["RTWDiagPresent"]==1);
    CHECK(c.properties["RTWDiagEnabled"]==0&&c.properties["RTWDiagRadioReady"]==0&&c.properties["RTWDiagRunning"]==0);
    CHECK(c.core_.core_error==60); // read-only diagnostic must not clear latch
}

int main() {
    operatingFailureIsNotHiddenByAttachSuccess();
    publicationDoesNotRunRatePolicy();everyFamilyPublishesFromStatus();
    publicationIsFreshAndPerDevice();existingReasonsAreExportedWithoutAttribution();
    filteredBlockAckAndControlRejectionsRemainDistinct();
    unknownPhySpeedAndSelectedMedium();nativeCountersAreObservedAndInvalidSnapshotsCleared();
    ethernetSubmissionBoundaryIsObservable();
    nativeBoundarySnapshotsRemainSeparateAndReadOnly();
    if(failures)return 1;
    std::cout<<"Controller diagnostic publication: readonly HT/BA, freshness, reasons, unknown PHY speed passed\n";
}
