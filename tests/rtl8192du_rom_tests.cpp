/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUEfuse.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static rtw_backend_usb_plan plan() {
    for(size_t i=0;i<rtw_usb_wifi_profile_count();++i) {
        const auto* p=rtw_usb_wifi_profile_at(i);
        if(p->family!=RTW_WIFI_RTL8192DU || !p->primary_verified)continue;
        const uint8_t cfg[]={9,2,32,0,1,p->configuration,0,0x80,50,
            9,4,p->interface_number,0,2,0xff,0xff,0xff,0,
            7,5,0x81,2,0,2,0,7,5,2,2,0,2,0};
        rtw_backend_usb_plan out{};
        const auto status=rtw_backend_usb_preflight(p->vendor,p->product,
            p->match_bcd?p->bcd_device:0,cfg,sizeof(cfg),p->interface_number,&out);assert(status==RTW_BACKEND_PREFLIGHT_DIAGNOSTICS || status==RTW_BACKEND_PREFLIGHT_STATION);return out;
    }assert(false);return {};
}
static std::array<uint8_t,256> logical() {
    std::array<uint8_t,256> p{};p.fill(0xff);
    p[0]=0x29;p[1]=0x81;p[0x10]=0;
    const uint8_t mac[]={2,0x44,0x55,0x66,0x77,0x88};
    std::memcpy(p.data()+0x19,mac,6);
    p[0xbd]=0x34;p[0xc3]=0x12;p[0xc4]=2;p[0xbe]=0x40;p[0xbf]=0;
    return p;
}
static std::vector<uint8_t> pack(const std::array<uint8_t,256>& p) {
    std::vector<uint8_t> b;
    for(unsigned block=0;block<32;++block) {
        bool used=false;for(unsigned i=0;i<8;++i)used|=p[block*8+i]!=0xff;
        if(!used)continue;
        if(block<16)b.push_back(uint8_t(block<<4));
        else {b.push_back(uint8_t(0x0f|((block&7)<<5)));b.push_back(uint8_t((block&0x78)<<1));}
        b.insert(b.end(),p.begin()+block*8,p.begin()+block*8+8);
    }b.push_back(0xff);return b;
}
struct Bus {
    std::array<uint8_t,65536> regs{};
    std::array<uint8_t,1024> otp{};
    size_t io=0,fail=0,short_at=0,waits=0,wait_fail=0,triggers=0;
    bool ready=true;
    Bus() {
        regs[0xf2]=1;regs[0x0a]=0x20;regs[2]=0x80;regs[3]=0x20;
        otp.fill(0xff);const auto p=pack(logical());
        std::copy(p.begin(),p.end(),otp.begin());otp[0x3fe]=0x99;otp[0x3ff]=0x66;
    }
    static rtw_usb_error in(void* opaque,const rtw_usb_setup* s,uint8_t* data,uint16_t* n) {
        auto& b=*static_cast<Bus*>(opaque);++b.io;
        assert(s->bm_request_type==0xc0 && s->b_request==5 && s->w_index==0);
        if(b.io==b.fail)return RTW_USB_IO_ERROR;
        for(unsigned i=0;i<s->w_length;++i)data[i]=b.regs[s->w_value+i];
        *n=b.io==b.short_at?uint16_t(s->w_length-1):s->w_length;return RTW_USB_OK;
    }
    static rtw_usb_error out(void* opaque,const rtw_usb_setup* s,const uint8_t* data,uint16_t* n) {
        auto& b=*static_cast<Bus*>(opaque);++b.io;
        assert(s->bm_request_type==0x40 && s->b_request==5 && s->w_index==0);
        // No OTP programming voltage, no DATA-register write, and no RF.
        assert(s->w_value==0xcf || s->w_value==2 ||
               (s->w_value>=0x31 && s->w_value<=0x33));
        if(s->w_value==0x33) {
            assert(s->w_length==1 && !(data[0]&0x80));
            assert(b.regs[0xcf]==0x69 && (b.regs[3]&0x10));
        }
        if(b.io==b.fail)return RTW_USB_IO_ERROR;
        for(unsigned i=0;i<s->w_length;++i)b.regs[s->w_value+i]=data[i];
        if(s->w_value==0x33) {
            ++b.triggers;const unsigned addr=b.regs[0x31]|((b.regs[0x32]&3U)<<8);
            b.regs[0x30]=b.otp[addr];if(b.ready)b.regs[0x33]|=0x80;
        }
        *n=b.io==b.short_at?uint16_t(s->w_length-1):s->w_length;return RTW_USB_OK;
    }
    static int wait(void* opaque,uint32_t us) {
        auto& b=*static_cast<Bus*>(opaque);++b.waits;
        assert(us==1 || us==50);return b.waits!=b.wait_fail;
    }
};
static rtw_92du_result read(Bus& b,rtw_92du_rom& out,uint8_t index=0) {
    const auto p=plan();const rtw_usb_rw_transport bus{&b,Bus::in,Bus::out};
    const rtw_waiter w{&b,Bus::wait};const rtw_92du_boot_options opt{1,1,4};
    return rtw_92du_read_rom(&p,&bus,&w,&opt,index,&out);
}
int main() {
    const auto p=logical();const auto packed=pack(p);
    std::array<uint8_t,256> out{};
    assert(rtw_92du_unpack_rom(packed.data(),packed.size(),out.data(),out.size())==RTW_92DU_OK);
    assert(out==p); // includes legal FF payload bytes and extended sections.
    const uint8_t truncated[]={0x0e,0x29};
    assert(rtw_92du_unpack_rom(truncated,sizeof(truncated),out.data(),out.size())==RTW_92DU_BAD_PROM);
    const uint8_t outside[]={0x0f,0x8e,1,2,0xff}; // section64 outside DU's 32 sections.
    assert(rtw_92du_unpack_rom(outside,sizeof(outside),out.data(),out.size())==RTW_92DU_BAD_PROM);
    auto duplicate=packed;duplicate.pop_back();duplicate.insert(duplicate.end(),{0x0e,0x34,0x12,0xff});
    assert(rtw_92du_unpack_rom(duplicate.data(),duplicate.size(),out.data(),out.size())==RTW_92DU_OK);
    assert(out[0]==0x34 && out[1]==0x12);
    size_t count=0;
    {
        Bus b;rtw_92du_rom rom{};assert(read(b,rom)==RTW_92DU_OK);count=b.io;
        assert(!b.regs[0xcf] && b.triggers==515 && b.waits==515 && rom.pa_bias==0xff);
        assert(rom.logical[0]==0x29 && rom.logical[0x1e]==0x88);
        assert(rom.cut==RTW_92DU_CUT_D && rom.cut_word==0x9966);
        assert(rom.crystal==0x34 && rom.thermal==0x12 && rom.thermal_valid);
        assert(!rom.internal_pa_5g[0] && rom.internal_pa_5g[1] && rom.pa_valid);
        assert(rom.regulatory==2 && rom.board.mac[5]==0x88);
    }
    for(int kind=0;kind<5;++kind) {
        Bus b;rtw_92du_rom rom;std::memset(&rom,0x55,sizeof(rom));
        if(kind==0)b.regs[0x0a]=0;
        if(kind==1)b.regs[0x0a]=0x30;
        if(kind==2)b.otp[0x3fe]=0;
        if(kind==3)b.ready=false;
        if(kind==4)b.wait_fail=1;
        assert(read(b,rom)!=RTW_92DU_OK && !b.regs[0xcf]);
        const rtw_92du_rom zero{};assert(!std::memcmp(&rom,&zero,sizeof(rom)));
        if(kind<2)assert(!b.triggers);
    }
    for(bool short_io:{false,true})for(size_t i=1;i<=count;++i) {
        Bus b;rtw_92du_rom rom{};
        if(short_io)b.short_at=i;else b.fail=i;
        assert(read(b,rom)==RTW_92DU_IO_ERROR);
        assert(!b.regs[0xcf] && b.io<=i+1);
        const rtw_92du_rom zero{};assert(!std::memcmp(&rom,&zero,sizeof(rom)));
    }
    std::printf("RTL8192DU ROM: packed32sections, physical515 reads, cut/PA/thermal, %zu I/O failures passed\n",count*2);
}
