// SPDX-License-Identifier: GPL-2.0-only
#include "SignalHistory.hpp"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
    RTWControlStatus s{};s.version=1;s.flags=15|RTW_CONTROL_SIGNAL_TRACKED;
    s.state=4;s.channel=6;s.ssidLength=3;
    std::memcpy(s.ssid,"Lab",3);s.bssid[0]=2;
    rtwui::SignalMemory m;
    auto sample=[&](int raw,double now){s.rssi=raw;m.observe(s,now);};
    sample(-16,0);assert(m.value(0)==-16&&m.level(0)==3);
    sample(-94,2);assert(m.rawValue(2)==-94&&m.value(2)==-16&&m.level(2)==3);
    sample(-16,4);assert(m.value(4)==-16&&m.level(4)==3);
    sample(-90,6);sample(-89,8);
    assert(m.value(8)==-89&&m.level(8)==1); // persistent actual weakness is visible
    sample(-16,10);sample(-16,12);assert(m.level(12)==3);
    sample(0,14);assert(!m.value(14)&&!m.level(14)); // authoritative driver expiry
    s.flags=15;sample(-45,16);sample(0,18);
    assert(m.value(26)==-45&&!m.value(26.001)); // legacy driver grace is bounded
    sample(-60,30);assert(!m.value(29));
    sample(-30,29);assert(m.value(29)==-30); // clock rollback reseeds
    s.channel=11;sample(-90,31);assert(m.value(31)==-90);
    s.bssid[5]=3;sample(-16,32);assert(m.value(32)==-16);
    s.ssid[0]='X';sample(-80,33);assert(m.value(33)==-80);
    s.flags=7;sample(-16,34);assert(!m.value(34)&&!m.hasAssociation);
    s.flags=15;sample(-50,35);s.usbError=1;sample(-50,36);assert(!m.level(36));
    s.usbError=0;sample(-45,std::numeric_limits<double>::quiet_NaN());assert(!m.hasAssociation);
    sample(-45,40);assert(!m.fresh(std::numeric_limits<double>::infinity()));
    s.ssidLength=33;sample(-16,41);assert(!m.hasAssociation);s.ssidLength=3;
    assert(rtwui::stableSignalLevel(-58,3)==3);
    assert(rtwui::stableSignalLevel(-59,3)==2);
    assert(rtwui::stableSignalLevel(-53,2)==2);
    assert(rtwui::stableSignalLevel(-52,2)==3);
    assert(rtwui::stableSignalLevel(-75,2)==2);
    assert(rtwui::stableSignalLevel(-76,2)==1);
    assert(rtwui::stableSignalLevel(-70,1)==1);
    assert(rtwui::stableSignalLevel(-69,1)==2);
    for(int raw:{0,1,127,-128})assert(!rtwui::signalLevel(raw));
    RTWControlScanResults scan{};scan.version=1;scan.count=2;
    for(auto& row:scan.records){row.ssidLength=1;row.ssid[0]='N';row.securityFlags=7;row.channel=6;}
    scan.records[0].rssi=-80;scan.records[1].rssi=120;
    assert(rtwui::networks(scan)[0].rssi==-80); // invalid scan RSSI cannot win
    rtwui::SignalHistory history;
    sample(-48,50);
    for(unsigned i=0;i<140;++i)history.record(s,m,50+i,0);
    assert(history.count==128&&history.next==12);
    const auto report=history.report();
    assert(report.find("\n50.000,")==std::string::npos);
    assert(report.find("\n62.000,")!=std::string::npos);
    assert(report.find("\n189.000,")!=std::string::npos);
    history.record(s,m,190,123);
    assert(history.entries[(history.next+127)%128].raw==0);
    history.clear();assert(history.count==0&&history.next==0);
    // Long-lived menus must not accumulate an unbounded sample log.
    const auto storage=sizeof(history);
    for(unsigned i=0;i<1000000;++i) {
        const double now=200+i;
        s.rssi=-40-static_cast<int>(i%60);m.observe(s,now);
        history.record(s,m,now,0);
        assert(history.count<=history.capacity && history.next<history.capacity);
    }
    assert(history.count==128 && sizeof(history)==storage);
    assert(history.report().size()<65536);
    std::puts("Signal UI: spike rejection, persistent change, hysteresis, unknown/expiry, identity/clock reset, scan sorting, bounded history passed");
}
