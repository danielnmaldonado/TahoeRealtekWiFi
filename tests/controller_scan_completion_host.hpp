// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <net80211_scan_host.hpp>
#include "RTWControl.h"
#include "rtw/ScanDiagnostics.hpp"
#include "rtw/SignalSample.h"
#include "rtw/USBWiFiProfiles.h"
#include <cerrno>
constexpr unsigned kIOReturnSuccess=0;

struct rtwn_softc {
    ieee80211com&sc_ic;
    struct {void*cookie=nullptr;} sc_ops;
    int core_error=0;
};
struct ScanCompletionTimer {
    bool*pending=nullptr;
    unsigned cancelled=0;
    void cancelTimeout() {++cancelled;if(pending)*pending=false;}
};
class RTWUSBController {
public:
    rtwn_softc core_;
    rtw_wifi_family family_=RTW_WIFI_RTL8192EU;
    bool attached_=true,running_=true,radioReady_=true,joinRequested_=false,present_=true;
    bool sleeping_=false,stopping_=false,closing_=false,browseScanComplete_=false;
    bool userRadioDisabled_=false;
    unsigned publications=0;
    rtw::ScanDiagnostics scanDiag_{};
    ScanCompletionTimer timer;
    ScanCompletionTimer*scanTimer_=&timer;
    rtw_signal_sample signal_{};
    unsigned usbError_=0;
    struct {uint8_t bytes[6]{};} mac_;
    uint64_t txPackets_=0,rxPackets_=0,txErrors_=0,rxErrors_=0;
    explicit RTWUSBController(ieee80211com&ic,bool*pending):core_{ic,{this}} {
        ic.ic_softc=&core_;timer.pending=pending;registerScanCallback();
    }
    static RTWUSBController*owner(void*p) {return static_cast<RTWUSBController*>(p);}
    static uint64_t nowMS() {return 1000;}
    static int scanComplete(ieee80211com*);
    void registerScanCallback();
    void observeScanTransition(ieee80211_state,ieee80211_state,unsigned,int);
    void publishScanDiagnostics() {++publications;}
    void publishDiagnostics() {++publications;}
    uint32_t supported5GHzFlags()const {return 0;}
    bool authorized()const {return joinRequested_&&core_.sc_ic.ic_state==IEEE80211_S_RUN;}
    void status(RTWControlStatus*);
};
