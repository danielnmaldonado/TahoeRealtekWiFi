#!/usr/bin/env python3
"""Exercise the actual receive RSSI block for every enabled station family."""
from pathlib import Path
import argparse
import runpy
import subprocess
from host_cpp import compiler, sanitizer_flags

project = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, default=project / 'out-rssi-feedback')
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
controller = (project / 'kext/RTWUSBController.cpp').read_text()
block = runpy.run_path(str(project / 'scripts/test-rtwn-performance.py'))['block']
receive = block(controller, 'void RTWUSBController::receive(')
signal = receive[receive.index('        int measuredRSSI=0;'):receive.index('        info.rxi_tstamp=')]
marker = 'void RTWUSBController::updateFirmwareRSSI('
method = block(controller, marker) if marker in controller else ''
closure = r'''#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "rtw/SignalSample.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTL8814AUDataPath.h"
#define IEEE80211_S_RUN 4
struct ieee80211_node { uint8_t ni_bssid[6]{2,1,2,3,4,5}; };
struct rtwn_softc {
    struct { int ic_state=IEEE80211_S_RUN; ieee80211_node* ic_bss=nullptr; } sc_ic;
    int avg_pwdb=-1;
    unsigned updates=0;
};
struct r92c_rx_phystat { uint8_t data[32]; };
struct ieee80211_rxinfo { uint8_t rxi_rssi=0; };
static int measured=-55;
static unsigned percent=51;
static void rtwn_update_avgrssi(rtwn_softc* core,int,int8_t dbm) {
    ++core->updates; core->avg_pwdb=rtw_signal_node_value(dbm);
}
static int rtwn_get_rssi(rtwn_softc*,int,uint8_t*) { return measured; }
extern "C" int rtw_8814au_parse_phy(const rtw_8814au_rx_frame*,uint8_t,rtw_8814au_phy_status* p) {
    *p={};p->signal_dbm=static_cast<int8_t>(measured);p->rssi=percent;p->channel=36;return 1;
}
class RTWUSBController {
public:
    rtwn_softc core_{};
    rtw_wifi_family family_=RTW_WIFI_RTL8192CU;
    struct {struct {uint8_t antenna_rx=3;} board;} acCore_;
    uint8_t acRSSI_=17;
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    int sample8723RSSI(const rtw_rx_packet&p){return p.phy_status_present?measured:0;}
    int sample8710BURSSI(const rtw_rx_packet&p){return p.phy_status_present?measured:0;}
    int sample8192FURSSI(const rtw_rx_packet&p){return p.phy_status_present?measured:0;}
    int sample8192DURSSI(const rtw_rx_packet&p){return p.phy_status_present?measured:0;}
    int sample88RSSI(const rtw_rx_packet&p){return p.phy_status_present?measured:0;}
    int sample8812RSSI(const rtw_rx_packet&p){return p.phy_status_present?measured:0;}
    void updateFirmwareRSSI(const rtw_rx_packet&,int,unsigned);
    uint8_t receiveSignal(const rtw_rx_packet&p) {
        ieee80211_rxinfo info{};
''' + signal + r'''
        (void)scanPhysicalChannel;
        return info.rxi_rssi;
    }
};
''' + method + '\n'
(args.out / 'production.hpp').write_text(closure)
binary = args.out / 'rssi-feedback-tests'
subprocess.run([*compiler(), '-std=c++17', '-g', '-O1', '-Wall', '-Wextra', '-Werror',
    *sanitizer_flags(), '-I'+str(project/'include'), '-I'+str(args.out),
    str(project/'tests/rssi_feedback_tests.cpp'), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)
