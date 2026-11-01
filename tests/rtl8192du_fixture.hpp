#pragma once
/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUPHY.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
namespace du_test {
struct Op {uint16_t address,length;uint32_t value;};
struct RFWrite {unsigned path,address;uint32_t value;};
struct Bus {
    std::array<uint8_t,65536> regs{};
    std::array<std::array<uint32_t,256>,2> rf{};
    std::vector<Op> writes;
    std::vector<RFWrite> rf_writes;
    std::vector<unsigned> rf_reads;
    size_t io=0,fail_at=0,fail_from=0,short_at=0,waits=0,wait_fail_at=0;
    bool lc_timeout=false,zero_curve=false,zero_iq=false,rx_iq_fail=false,iq_timeout=false,rf_stuck=false;
    bool llt_timeout=false,cam_timeout=false,cpu_stuck=false,power_handshake=false;
    uint8_t h2c_busy=0;int zero_curve_path=-1;
    std::array<uint8_t,256> llt{};
    uint8_t thermal=18;
    static uint32_t word(const uint8_t* p,unsigned width) {
        uint32_t v=0;for(unsigned i=0;i<width;++i)v|=uint32_t(p[i])<<(8*i);return v;
    }
    void put(unsigned reg,uint32_t value,unsigned width=4) {for(unsigned i=0;i<width;++i)regs[reg+i]=uint8_t(value>>(8*i));}
    uint32_t rf_value(unsigned path,unsigned offset) {
        if(offset==0x42)return (rf[path][offset]&~0xf800U)|(uint32_t(thermal)<<11);
        if(offset==0x2a)return lc_timeout?rf[path][offset]&~0x800U:rf[path][offset]|0x800U;
        if(offset==0x4f || offset==0x50) {
            if(zero_curve || zero_curve_path==int(path))return 0;
            const unsigned index=rf[path][0x3f]&127U;
            const unsigned even=26001-index*100,odd=25951-index*100;
            return offset==0x4f?((odd<<5)|((even>>10)&31U)):((even&1023U)<<10);
        }return rf[path][offset];
    }
    static rtw_usb_error in(void* cookie,const rtw_usb_setup* s,uint8_t* bytes,uint16_t* actual) {
        auto& b=*static_cast<Bus*>(cookie);++b.io;
        assert(s->bm_request_type==0xc0 && s->b_request==5 && !s->w_index && s->timeout_ms==1000);
        if(b.io==b.fail_at || (b.fail_from && b.io>=b.fail_from))return RTW_USB_IO_ERROR;
        // Optional hardware autoload/APFM completion for clean power-cycle
        // tests; ordinary PHY/MAC fixtures do not synthesize this handshake.
        if(b.power_handshake && s->w_value==4){b.regs[4]|=2;if(s->w_length==2)b.regs[5]&=0xfe;}
        if(s->w_value==0x1cc)b.regs[0x1cc]=b.h2c_busy;
        if(s->w_value==0x670 && !b.cam_timeout)b.regs[0x673]&=0x7f;
        // IQK measurement registers are produced by radio one-shot hardware.
        if(s->w_length==4) {
            uint32_t measured=0;bool special=true;
            switch(s->w_value) {
            case 0xe94:case 0xeb4:measured=0x01000000;break;
            case 0xe9c:case 0xebc:case 0xecc:measured=0x00100000;break;
            case 0xea4:case 0xec4:measured=b.rx_iq_fail?0x01320000:0x01000000;break;
            case 0xeac:measured=(b.iq_timeout?0:0x24000000)|0x00010000;break;
            default:special=false;
            }
            if(special)b.put(s->w_value,b.zero_iq?0:measured);
        }
        for(unsigned i=0;i<s->w_length;++i)bytes[i]=b.regs[s->w_value+i];
        *actual=b.io==b.short_at?uint16_t(s->w_length-1):s->w_length;return RTW_USB_OK;
    }
    static rtw_usb_error out(void* cookie,const rtw_usb_setup* s,const uint8_t* bytes,uint16_t* actual) {
        auto& b=*static_cast<Bus*>(cookie);++b.io;
        assert(s->bm_request_type==0x40 && s->b_request==5 && !s->w_index && s->timeout_ms==1000);
        assert(s->w_length==1 || s->w_length==2 || s->w_length==4);
        if(b.io==b.fail_at || (b.fail_from && b.io>=b.fail_from))return RTW_USB_IO_ERROR;
        const uint32_t value=word(bytes,s->w_length);
        const uint32_t old=word(b.regs.data()+s->w_value,s->w_length);
        b.put(s->w_value,value,s->w_length);b.writes.push_back({s->w_value,s->w_length,value});
        if(s->w_value==0x1e0 && s->w_length==4) {
            b.llt[(value>>8)&255U]=uint8_t(value);
            if(!b.llt_timeout)b.regs[0x1e3]&=0x3f;
        }
        if(s->w_value==0x1cf && value==0x20 && !b.cpu_stuck)b.regs[3]&=0xfb;
        const unsigned normal=s->w_value&0x1fffU;
        if(s->w_length==4 && (normal==0x840 || normal==0x844)) {
            const unsigned path=normal==0x844?1:0,reg=(value>>20)&255U;
            if(!(b.rf_stuck && !reg && !(value&0xfffffU)))b.rf[path][reg]=value&0xfffffU;
            b.rf_writes.push_back({path,reg,value&0xfffffU});
        }
        if(s->w_length==4 && (normal==0x824 || normal==0x82c) && (value&0x80000000) &&
           (normal==0x82c || !(old&0x80000000))) {
            const unsigned path=normal==0x82c?1:0,offset=(value>>23)&255U;
            b.rf_reads.push_back(offset);
            const uint32_t result=b.rf_value(path,offset);
            b.put(path?0x8a4:0x8a0,result);b.put(path?0x8bc:0x8b8,result);
        }
        *actual=b.io==b.short_at?uint16_t(s->w_length-1):s->w_length;return RTW_USB_OK;
    }
    static int wait(void* cookie,uint32_t us) {
        auto& b=*static_cast<Bus*>(cookie);++b.waits;
        assert(us>0 && us<=50000);return b.waits!=b.wait_fail_at;
    }
};
/* Pinned DU RCR uses ACF bit12 and RXFLTMAP1 at0x6a2. The legacy
 * filter ABI maps numerical control subtype to its bit, and ACF bypasses
 * that bitmap. BAR=8; PS-POLL=10. Model admission, not just a register value. */
static inline bool control_admitted(const Bus& bus,unsigned subtype) {
    assert(subtype<16);
    return (Bus::word(bus.regs.data()+0x608,4)&(1U<<12)) ||
        (Bus::word(bus.regs.data()+0x6a2,2)&(1U<<subtype));
}
static rtw_backend_usb_plan plan() {
    for(size_t i=0;i<rtw_usb_wifi_profile_count();++i){const auto*p=rtw_usb_wifi_profile_at(i);
        if(p->family!=RTW_WIFI_RTL8192DU || !p->primary_verified)continue;
        const uint8_t cfg[]={9,2,32,0,1,p->configuration,0,0x80,50,
            9,4,p->interface_number,0,2,0xff,0xff,0xff,0,7,5,0x81,2,0,2,0,7,5,2,2,0,2,0};
        rtw_backend_usb_plan out{};const auto status=rtw_backend_usb_preflight(p->vendor,p->product,
            p->match_bcd?p->bcd_device:0,cfg,sizeof(cfg),p->interface_number,&out);assert(status==RTW_BACKEND_PREFLIGHT_DIAGNOSTICS || status==RTW_BACKEND_PREFLIGHT_STATION);return out;
    }assert(false);return {};
}
static rtw_92du_rom rom(bool dual=false,uint8_t index=0,bool internal=false,rtw_92du_cut cut=RTW_92DU_CUT_D) {
    rtw_92du_rom r{};r.logical[0]=0x29;r.logical[1]=0x81;r.logical[0x10]=dual?1:0;
    r.logical[0x19]=2;r.logical[0x1e]=0xa0;r.logical[0x5b]=2;r.logical[0x60]=0xa1;
    for(unsigned base:{0x61U,0x67U,0x7cU,0x91U,0xa6U})
        for(unsigned i=0;i<6;++i)r.logical[base+i]=24;
    r.crystal=r.logical[0xbd]=0x34;r.thermal=r.logical[0xc3]=0x12;
    r.regulatory=r.logical[0xc4]=2;r.thermal_valid=r.pa_valid=1;
    r.logical[0xbe]=r.logical[0xbf]=internal?0:0x40;
    r.internal_pa_5g[0]=r.internal_pa_5g[1]=uint8_t(internal);
    r.cut=cut;r.cut_word=cut==RTW_92DU_CUT_C?0xaa55:cut==RTW_92DU_CUT_D?0x9966:0xcc33;r.pa_bias=0xff;
    const auto p=plan();assert(rtw_92du_identify(&p,0x10000,r.logical,sizeof(r.logical),index,&r.board)==RTW_92DU_OK);
    return r;
}
struct Fixture {
    Bus bus;rtw_92du_boot boot{};rtw_92du_rom identity;
    std::vector<uint64_t> storage;
    Fixture(bool dual=false,uint8_t index=0,bool internal=false,rtw_92du_usb_speed speed=RTW_92DU_USB_HIGH,rtw_92du_cut cut=RTW_92DU_CUT_D):identity(rom(dual,index,internal,cut)),storage((rtw_92du_phy_size()+7)/8) {
        const rtw_usb_rw_transport t{&bus,Bus::in,Bus::out};const rtw_waiter w{&bus,Bus::wait};
        const rtw_92du_boot_options o{1,1,10};
        assert(rtw_92du_boot_init(&boot,&identity.board,&t,&w,&o)==RTW_92DU_OK);
        boot.power_ready=boot.firmware_ready=1;
        boot.firmware_version=0x21;
        assert(rtw_92du_phy_init(&boot,&identity,storage.data(),storage.size()*8,speed)==RTW_92DU_OK && !bus.io);
    }
    void setup(uint8_t channel) {
        assert(rtw_92du_phy_load_tables(&boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_set_channel(&boot,channel)==RTW_92DU_OK);
    }
};
} // namespace du_test
