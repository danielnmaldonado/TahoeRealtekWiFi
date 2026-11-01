/* SPDX-License-Identifier: GPL-2.0-only. Purely simulated verification. */
#include "rtw/RXMonitor.h"
#include "rtw/FCS.h"
#include "rtw/Beacon.hpp"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
unsigned checks=0;
#define CHECK(e) do {++checks; if(!(e)){std::fprintf(stderr,"CHECK failed %s:%d: %s\n",__FILE__,__LINE__,#e);std::abort();}} while(0)
struct Usb {
    std::array<uint8_t,65536> regs{};
    unsigned writes=0,reads=0,delays=0,fail_write=0;
    Usb(){regs[0xf0]=0x37;regs[0xf1]=0x11;regs[0xf2]=0x41;regs[0xf3]=0x04;}
};
rtw_usb_error read(void* context,const rtw_usb_setup* s,uint8_t* data,uint16_t* actual){
    auto& m=*static_cast<Usb*>(context);
    if(!s || !data || !actual || s->bm_request_type!=0xc0 || s->b_request!=5 ||
       (s->w_length!=1 && s->w_length!=2 && s->w_length!=4) ||
       static_cast<unsigned>(s->w_value)+s->w_length>65536U)return RTW_USB_INVALID_ARGUMENT;
    ++m.reads;std::memcpy(data,&m.regs[s->w_value],s->w_length);
    *actual=s->w_length;return RTW_USB_OK;
}
rtw_usb_error write(void* context,const rtw_usb_setup* s,const uint8_t* data,uint16_t* actual){
    auto& m=*static_cast<Usb*>(context);
    if(!s || !data || !actual || s->bm_request_type!=0x40 || s->b_request!=5 ||
       s->w_length==0 || s->w_length>254 ||
       static_cast<unsigned>(s->w_value)+s->w_length>65536U)return RTW_USB_INVALID_ARGUMENT;
    ++m.writes;
    if(m.fail_write && m.writes==m.fail_write)return RTW_USB_IO_ERROR;
    std::memcpy(&m.regs[s->w_value],data,s->w_length);
    *actual=s->w_length;return RTW_USB_OK;
}
int wait(void* context,uint32_t micros){
    auto& m=*static_cast<Usb*>(context);++m.delays;
    return micros>0 && micros<=10000;
}
rtw_hal_report valid_boot(){
    rtw_hal_report result{};
    result.last_success=RTW_HAL_MAC_PREPARED;
    result.sys_cfg=0x04411137;
    result.identity.eeprom_vendor=0x0bda;
    result.identity.eeprom_product=0x8812;
    result.identity.rfe_type=3;
    return result;
}
uint32_t le32(const std::array<uint8_t,65536>& a,size_t p){
    return uint32_t(a[p]) | (uint32_t(a[p+1])<<8) | (uint32_t(a[p+2])<<16) |
           (uint32_t(a[p+3])<<24);
}
void profile(){
    rtw_phy_profile_report p{};
    CHECK(rtw_phy_profile_preflight_8812au(&p)==RTW_PHY_OK);
    CHECK(p.mac_entries==107 && p.bb_entries==215 && p.agc_entries==132 &&
          p.rf_a_entries==210 && p.rf_b_entries==193 && p.executed==0);
    Usb dev;
    rtw_usb_rw_transport bus{&dev,&read,&write};
    rtw_waiter timer{&dev,&wait};
    auto boot=valid_boot();rtw_monitor_report state{};
    CHECK(rtw_monitor_prepare_channel6(&bus,&timer,&boot,0,&state)==RTW_MONITOR_NOT_AUTHORIZED);
    CHECK(dev.writes==0 && dev.reads==0);
    boot.identity.rfe_type=4;
    CHECK(rtw_monitor_prepare_channel6(&bus,&timer,&boot,1,&state)==RTW_MONITOR_UNSUPPORTED);
    CHECK(dev.writes==0 && dev.reads==0);
    boot.identity.rfe_type=3;
    CHECK(rtw_monitor_prepare_channel6(&bus,&timer,&boot,1,&state)==RTW_MONITOR_OK);
    CHECK(state.phy.executed==857 && state.phy.mac_entries==107);
    CHECK(state.mac_operations==13 && state.rx_register_operations==7);
    CHECK(state.monitor_registers_written && !state.ethernet_authorized);
    const unsigned writesBeforeVerification=dev.writes;
    const unsigned readsBeforeVerification=dev.reads;
    CHECK(rtw_monitor_verify_channel6(&bus,&state)==RTW_MONITOR_OK);
    CHECK(dev.writes==writesBeforeVerification &&
          dev.reads==readsBeforeVerification+6U);
    CHECK(rtw_monitor_verify_channel6(nullptr,&state)==RTW_MONITOR_NOT_AUTHORIZED);
    rtw_monitor_report unauthorised{};
    CHECK(rtw_monitor_verify_channel6(&bus,&unauthorised)==RTW_MONITOR_NOT_AUTHORIZED);
    unauthorised=state;unauthorised.ethernet_authorized=1U;
    CHECK(rtw_monitor_verify_channel6(&bus,&unauthorised)==RTW_MONITOR_NOT_AUTHORIZED);
    dev.regs[0x608]^=1U;
    CHECK(rtw_monitor_verify_channel6(&bus,&state)==RTW_MONITOR_REGISTER_MISMATCH);
    dev.regs[0x608]^=1U;
    dev.regs[0x800]&=static_cast<uint8_t>(~0x01U);
    dev.regs[0x803]&=static_cast<uint8_t>(~0x02U);
    CHECK(rtw_monitor_verify_channel6(&bus,&state)==RTW_MONITOR_REGISTER_MISMATCH);
    dev.regs[0x803]|=0x03U;
    CHECK(rtw_monitor_verify_channel6(&bus,&state)==RTW_MONITOR_OK);
    CHECK(dev.regs[0x10]==0x0c && dev.regs[0x11]==0x66 &&
          dev.regs[0x25]==0x0f);
    CHECK(dev.regs[0x6a0]==0xff && dev.regs[0x6a1]==0xff);
    CHECK(le32(dev.regs,0x620)==0xffffffffU);
    CHECK(dev.regs[0x100]==0xff && dev.regs[0x101]==0x06);
    CHECK((le32(dev.regs,0x808)&0x30000000U)==0x30000000U);
    CHECK((le32(dev.regs,0x800)&0x03000000U)==0x03000000U);
    CHECK(((le32(dev.regs,0xc90)>>20)&0xff)==0x18);
    CHECK((le32(dev.regs,0xc90)&0xff)==6);
    CHECK((le32(dev.regs,0xe90)&0xff)==6);
    CHECK(dev.delays>=4); // Radio A contains four 50ms delays.
    Usb broken;broken.fail_write=11;
    rtw_usb_rw_transport failBus{&broken,&read,&write};
    CHECK(rtw_monitor_prepare_channel6(&failBus,&timer,&boot,1,&state)==RTW_MONITOR_PHY_FAILED);
    CHECK(!state.monitor_registers_written && !state.ethernet_authorized);
    Usb switched;switched.regs[0xf0]=0;
    rtw_usb_rw_transport switchedBus{&switched,&read,&write};
    CHECK(rtw_monitor_prepare_channel6(&switchedBus,&timer,&boot,1,&state)==RTW_MONITOR_UNSUPPORTED);
    CHECK(switched.writes==0);
    const rtw_phy_entry restricted[]={{0x0020,RTW_PHY_MAC8,0x1}};
    CHECK(rtw_phy_validate_8812au(restricted,1)==RTW_PHY_BAD_TABLE);
    const rtw_phy_entry allowed[]={{0x0025,RTW_PHY_MAC8,0xf}};
    CHECK(rtw_phy_validate_8812au(allowed,1)==RTW_PHY_OK);
}
struct Received {size_t accepted=0;bool recognized=false;};
int on_beacon(void* ptr,const uint8_t* bytes,size_t n,const rtw_rx_packet*){
    auto& s=*static_cast<Received*>(ptr);
    ++s.accepted;
    const auto parsed=rtw::parseBeacon(bytes,n);
    s.recognized=static_cast<bool>(parsed) && parsed.beacon.ssid.size()==4 &&
                 parsed.beacon.ssid[0]=='T';
    return 1;
}
void rx(){
    std::array<uint8_t,64> mpdu{};
    mpdu[0]=0x80;
    mpdu[16]=0x02;mpdu[21]=0x01; // locally-administered BSSID
    mpdu[36]=0;mpdu[37]=4;
    mpdu[38]='T';mpdu[39]='e';mpdu[40]='s';mpdu[41]='t';
    const unsigned frameLen=46; // 802.11 42 + 4-byte FCS
    const uint32_t check=rtw_fcs32(mpdu.data(),42);
    for(unsigned n=0;n<4;++n)mpdu[42+n]=static_cast<uint8_t>(check>>(8U*n));
    std::vector<uint8_t> bulk(72+72,0);
    for(unsigned unit=0;unit<2;++unit){
        size_t off=unit*72;
        bulk[off]=static_cast<uint8_t>(frameLen);
        bulk[off+1]=unit==0?0:0x40; // second frame has CRC error
        std::memcpy(bulk.data()+off+24,mpdu.data(),frameLen);
    }
    Received received{};rtw_monitor_rx_stats stats{};
    CHECK(rtw_monitor_process_bulk(bulk.data(),bulk.size(),on_beacon,&received,&stats)==RTW_PACKETS_OK);
    CHECK(stats.packets_seen==2 && stats.crc_dropped==1 && stats.beacons==1);
    CHECK(received.accepted==1 && received.recognized);
    bulk[24+0]=0x08;
    CHECK(rtw_monitor_process_bulk(bulk.data(),bulk.size(),on_beacon,&received,&stats)==RTW_PACKETS_OK);
    CHECK(stats.beacons==1 && stats.non_beacons==1);
    bulk[24+0]=0x80; // valid beacon subtype again
    bulk[24+22]=0x01; // fragmented management frames are rejected
    CHECK(rtw_monitor_process_bulk(bulk.data(),bulk.size(),on_beacon,&received,&stats)==RTW_PACKETS_OK);
    CHECK(stats.non_beacons==2);
    bulk[24+22]=0x00;
    bulk[24+38]^=0x01U; // software FCS must detect corruption
    CHECK(rtw_monitor_process_bulk(bulk.data(),bulk.size(),on_beacon,&received,&stats)==RTW_PACKETS_OK);
    CHECK(stats.fcs_dropped==1 && stats.beacons==1);
    bulk[0]=0xff;bulk[1]=0x3f;
    CHECK(rtw_monitor_process_bulk(bulk.data(),bulk.size(),on_beacon,&received,&stats)==RTW_PACKETS_TRUNCATED);
    CHECK(stats.malformed==1);
    CHECK(rtw_monitor_process_bulk(nullptr,1,on_beacon,&received,&stats)==RTW_PACKETS_BAD_ARGUMENT);
    /* Deterministic hostile USB data for parser bounds under sanitizers. */
    std::array<uint8_t,257> noise{};
    uint32_t x=0x3ae156df;
    for(unsigned iteration=0;iteration<5000;++iteration){
        for(auto& v:noise){x^=x<<13;x^=x>>17;x^=x<<5;v=static_cast<uint8_t>(x);}
        rtw_monitor_rx_stats tmp{};
        auto outcome=rtw_monitor_process_bulk(noise.data(),noise.size(),on_beacon,&received,&tmp);
        CHECK(outcome==RTW_PACKETS_OK || outcome==RTW_PACKETS_TRUNCATED ||
              outcome==RTW_PACKETS_MALFORMED || outcome==RTW_PACKETS_TOO_MANY);
    }
}
}
int main(){profile();rx();std::printf("rtw-monitor-tests: %u checks passed; ONLY simulated USB, no macOS hardware\n",checks);}
