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
    iface.iface=&native;c.netif_=&iface;c.publishDiagnostics();
    CHECK(native.reads==1&&native.stats.packets_in==17&&native.stats.bytes_in==4000);
    CHECK(c.properties["RTWDiagNativeIfnetStatsValid"]==1);
    CHECK(c.properties["RTWDiagNativeIfnetStatsError"]==0);
    CHECK(c.properties["RTWDiagNativeIfnetIndex"]==7);
    CHECK(c.properties["RTWDiagNativeIfnetInputPackets"]==17);
    CHECK(c.properties["RTWDiagNativeIfnetInputBytes"]==4000);
    CHECK(c.properties["RTWDiagNativeIfnetInputErrors"]==3);
    native.error=5;c.publishDiagnostics();
    CHECK(c.properties["RTWDiagNativeIfnetStatsValid"]==0&&c.properties["RTWDiagNativeIfnetStatsError"]==5);
    CHECK(c.properties["RTWDiagNativeIfnetInputPackets"]==0&&c.properties["RTWDiagNativeIfnetInputBytes"]==0);
    c.closing_=true;c.publishDiagnostics();CHECK(native.reads==2);
    c.closing_=false;c.attached_=false;c.publishDiagnostics();CHECK(native.reads==2);
    c.attached_=true;iface.iface=nullptr;c.publishDiagnostics();CHECK(native.reads==2);
    CHECK(c.properties["RTWDiagNativeIfnetStatsValid"]==0&&c.properties["RTWDiagNativeIfnetStatsError"]==6);
    CHECK(c.properties["RTWDiagNativeIfnetIndex"]==0);
    c.netif_=nullptr;c.publishDiagnostics();CHECK(native.reads==2);
}

static void ethernetSubmissionBoundaryIsObservable() {
    RTWUSBController c;auto& d=c.core_.sc_ic.ic_if.if_input_diag;
    d={1,2,3,4,5,6,7,8};c.publishDiagnostics();
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

int main() {
    publicationDoesNotRunRatePolicy();everyFamilyPublishesFromStatus();
    publicationIsFreshAndPerDevice();existingReasonsAreExportedWithoutAttribution();
    unknownPhySpeedAndSelectedMedium();nativeCountersAreObservedAndInvalidSnapshotsCleared();
    ethernetSubmissionBoundaryIsObservable();
    if(failures)return 1;
    std::cout<<"Controller diagnostic publication: readonly HT/BA, freshness, reasons, unknown PHY speed passed\n";
}
