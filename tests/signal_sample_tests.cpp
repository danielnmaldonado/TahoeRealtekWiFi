// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/SignalSample.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    const uint8_t ap[6]={0x02,1,2,3,4,5};
    const uint8_t other[6]={0x02,1,2,3,4,6};
    std::array<uint8_t,24> frame{};
    frame[0]=0x80; // beacon
    std::memcpy(frame.data()+10,ap,6);std::memcpy(frame.data()+16,ap,6);
    rtw_signal_sample s{};
    auto observe=[&](bool phy,int dbm,uint64_t now) {
        rtw_signal_observe(&s,ap,6,frame.data(),frame.size(),phy,dbm,now);
    };
    assert(!rtw_signal_value(&s,ap,6,0));
    observe(true,-16,0);
    assert(rtw_signal_value(&s,ap,6,0)==-16); // uptime zero is valid
    assert(s.valid_samples==1);
    observe(false,0,2000);
    assert(s.missing_phy==1&&s.sampled_ms==0);
    assert(rtw_signal_value(&s,ap,6,10000)==-16);
    assert(rtw_signal_value(&s,ap,6,10001)==0); // poll/missing PHY cannot refresh
    observe(true,-41,11000);
    for(int invalid:{0,1,127,-128,-999})observe(true,invalid,12000);
    assert(s.invalid_phy==5&&s.dbm==-41&&s.sampled_ms==11000);
    frame[10]=0x04;observe(true,-1,12000);
    assert(s.foreign_frames==1&&s.dbm==-41);
    frame[10]=ap[0];
    frame[0]=0x08;frame[1]=0x01;observe(true,-1,12000); // uplink
    assert(s.foreign_frames==2);
    frame[1]=0x02;observe(true,-90,13000); // real weak downlink
    assert(rtw_signal_value(&s,ap,6,13000)==-90);
    frame[1]=0x03;observe(true,-1,13001); // WDS
    assert(s.foreign_frames==3&&s.dbm==-90);
    assert(!rtw_signal_value(&s,other,6,13000));
    assert(!rtw_signal_value(&s,ap,11,13000));
    assert(!rtw_signal_value(&s,ap,6,12000));
    frame[0]=0x80;frame[1]=0;
    rtw_signal_observe(&s,other,6,frame.data(),frame.size(),false,0,14000);
    assert(!s.valid); // new AP cannot borrow old RSSI
    observe(true,-110,15000);
    assert(rtw_signal_value(&s,ap,6,15000)==-110);
    rtw_signal_observe(&s,ap,11,frame.data(),frame.size(),false,0,15001);
    assert(!s.valid); // channel change also resets
    observe(true,-60,16000);
    observe(false,0,15999);assert(!s.valid); // backwards monotonic clock
    observe(true,-52,17000);
    const auto count=s.valid_samples;
    rtw_signal_reset(&s);assert(!s.valid&&!s.associated&&s.valid_samples==count);
    assert(!rtw_signal_from_ap(frame.data(),23,ap));
    frame[0]=0x81;assert(!rtw_signal_from_ap(frame.data(),24,ap));
    const uint8_t multicast[6]={0xff,1,2,3,4,5},zero[6]={};
    assert(!rtw_signal_valid_bssid(multicast)&&!rtw_signal_valid_bssid(zero));
    assert(!rtw_signal_valid_bssid(nullptr));
    for(int dbm=-127;dbm<0;++dbm)
        assert(rtw_signal_node_value(dbm)>=1&&rtw_signal_node_value(dbm)<=99);
    assert(rtw_signal_node_value(-110)==1);
    assert(rtw_signal_node_value(-16)==84);
    assert(!rtw_signal_node_value(0)&&!rtw_signal_node_value(5));
    std::puts("Signal sample: missing PHY, expiry, invalid/foreign frames, real weak signal, association/channel/clock resets passed");
}
