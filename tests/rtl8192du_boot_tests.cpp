/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUBoot.h"
#include "../kext/RTL8192DUFirmware.inc"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static rtw_backend_usb_plan usb_plan() {
    for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        const auto* p = rtw_usb_wifi_profile_at(i);
        if (p->family != RTW_WIFI_RTL8192DU || !p->primary_verified) continue;
        const uint8_t cfg[] = {9,2,32,0,1,p->configuration,0,0x80,50,
            9,4,p->interface_number,0,2,0xff,0xff,0xff,0,
            7,5,0x81,2,0,2,0, 7,5,0x02,2,0,2,0};
        rtw_backend_usb_plan plan{};
        const auto result=rtw_backend_usb_preflight(p->vendor,p->product,
            p->match_bcd ? p->bcd_device : 0,cfg,sizeof(cfg),p->interface_number,&plan);
        if(result==RTW_BACKEND_PREFLIGHT_DIAGNOSTICS || result==RTW_BACKEND_PREFLIGHT_STATION)return plan;
    }
    assert(false); return {};
}
static std::array<uint8_t,512> prom(bool dual) {
    std::array<uint8_t,512> p{};
    p[0]=0x29; p[1]=0x81; p[0x10]=dual ? 1 : 0;
    const uint8_t m0[]={0x02,0x23,0x45,0x67,0x89,0xab};
    const uint8_t m1[]={0x02,0x23,0x45,0x67,0x89,0xac};
    std::copy(m0,m0+6,p.begin()+0x19);
    std::copy(m1,m1+6,p.begin()+0x5b);
    return p;
}
static rtw_92du_board board(bool dual=false, uint8_t index=0) {
    const auto usb=usb_plan(); const auto p=prom(dual);
    rtw_92du_board b{};
    assert(rtw_92du_identify(&usb,0x00010000,p.data(),p.size(),index,&b)
        == RTW_92DU_OK);
    return b;
}
struct Op { bool write; uint16_t address, count; uint32_t value; };
struct Bus {
    std::array<uint8_t,65536> reg{};
    std::vector<uint8_t> payload;
    std::vector<Op> ops;
    size_t io=0, fail_at=0, short_at=0, waits=0, fail_wait_at=0;
    bool autoload=true, apfm=true, checksum=true, mac_ready=true;
    uint8_t selected_mac=0;
    size_t next_payload_offset=0;
    Bus() {
        reg[0xf2]=1; reg[4]=2; reg[0]=0xff; reg[1]=0xff;
        reg[3]=0x80; reg[0xf8]=0xd8; reg[0x82]=0xb8;
        reg[0x18]=0; reg[0x1a]=0;
    }
    static rtw_usb_error in(void* c, const rtw_usb_setup* s,
                           uint8_t* bytes, uint16_t* actual) {
        auto& b=*static_cast<Bus*>(c); ++b.io;
        assert(s->bm_request_type==0xc0 && s->b_request==5 && s->w_index==0);
        assert(s->timeout_ms==1000 && (s->w_length==1 ||
               s->w_length==2 || s->w_length==4));
        if (b.io==b.fail_at) return RTW_USB_IO_ERROR;
        if (s->w_value==4) {
            if (!b.autoload) b.reg[4]&=~2U;
            if (b.apfm && s->w_length==2) b.reg[5]&=~1U;
        }
        if (s->w_value==0x80 && b.checksum && b.payload.size()) b.reg[0x80]|=4;
        if (b.mac_ready && (b.reg[0x80]&2)) b.reg[b.selected_mac ? 0x1a:0x18]=1;
        uint32_t v=0;
        for (unsigned i=0;i<s->w_length;++i) {
            bytes[i]=b.reg[s->w_value+i]; v|=uint32_t(bytes[i])<<(i*8);
        }
        b.ops.push_back({false,s->w_value,s->w_length,v});
        *actual=b.io==b.short_at ? s->w_length-1 : s->w_length;
        return RTW_USB_OK;
    }
    static rtw_usb_error out(void* c, const rtw_usb_setup* s,
                            const uint8_t* bytes, uint16_t* actual) {
        auto& b=*static_cast<Bus*>(c); ++b.io;
        assert(s->bm_request_type==0x40 && s->b_request==5 && s->w_index==0);
        assert(s->timeout_ms==1000);
        if (b.io==b.fail_at) return RTW_USB_IO_ERROR;
        const bool fw=s->w_value>=0x1000 && s->w_value<0x2000;
        if (fw) {
            // The pinned rtlwifi USB helper uses 64 / 8 / 1, not EU's writer.
            assert(s->w_length==64 || s->w_length==8 || s->w_length==1);
            assert((b.reg[0x80]&1) && b.reg[0x522]==0xff);
            const size_t offset=(b.reg[0x82]&7)*4096+(s->w_value-0x1000);
            assert(offset==b.next_payload_offset);
            assert(unsigned(s->w_value)+s->w_length<=0x2000);
            if (b.io!=b.short_at) {
                b.payload.insert(b.payload.end(),bytes,bytes+s->w_length);
                b.next_payload_offset+=s->w_length;
            }
        } else {
            // No baseband/RF table or unpause may occur in the boot primitive.
            assert(s->w_value<0x800);
            if (s->w_value==0x522) assert(s->w_length==1 && bytes[0]==0xff);
            if (s->w_value<=0x81 && unsigned(s->w_value)+s->w_length>0x81)
                assert((bytes[0x81-s->w_value]&0x80)==(b.reg[0x81]&0x80) ||
                       s->w_value==0x81);
            for (unsigned i=0;i<s->w_length;++i) b.reg[s->w_value+i]=bytes[i];
            // A cold MAC power/reset may discard writes made while it was
            // unpowered. Require TX pause to be asserted after CR enables it.
            if (s->w_value==0x100 && (bytes[0]&0x40)) b.reg[0x522]=0;
        }
        uint32_t v=0;
        for (unsigned i=0;i<std::min<unsigned>(4,s->w_length);++i)
            v|=uint32_t(bytes[i])<<(i*8);
        b.ops.push_back({true,s->w_value,s->w_length,v});
        *actual=b.io==b.short_at ? s->w_length-1 : s->w_length;
        return RTW_USB_OK;
    }
    static int wait(void* c,uint32_t us) {
        auto& b=*static_cast<Bus*>(c); ++b.waits;
        assert(us>=1 && us<=1000); return b.waits!=b.fail_wait_at;
    }
};
static rtw_92du_boot initialized(Bus& bus, bool dual=false,uint8_t index=0) {
    rtw_92du_boot ctx{}; const auto b=board(dual,index);
    const rtw_usb_rw_transport t{&bus,Bus::in,Bus::out};
    const rtw_waiter w{&bus,Bus::wait};
    const rtw_92du_boot_options opt{1,1,5};
    assert(rtw_92du_boot_init(&ctx,&b,&t,&w,&opt)==RTW_92DU_OK);
    assert(bus.io==0); bus.selected_mac=index; return ctx;
}
static std::vector<uint8_t> image(size_t size) {
    std::vector<uint8_t> p(32+size);
    p[0]=0xd2; p[1]=0x92; p[4]=0x34; p[5]=0x12; p[6]=7;
    p[12]=uint8_t(size); p[13]=uint8_t(size>>8);
    for (size_t i=0;i<size;++i) p[32+i]=uint8_t(i*29+17);
    return p;
}
static size_t writes(const Bus& b,uint16_t a) {
    return static_cast<size_t>(std::count_if(b.ops.begin(),b.ops.end(),[a](const Op& o) {
        return o.write && o.address==a;
    }));
}
int main() {
    for(unsigned mode=0;mode<3;++mode) {
        Bus b;auto c=initialized(b,mode!=0,(uint8_t)(mode==2));
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
        assert(rtw_92du_firmware_start(&c,RTW8192DUFirmware,sizeof(RTW8192DUFirmware))==RTW_92DU_OK);
        assert(sizeof(RTW8192DUFirmware)==32302 && c.firmware_bytes==32270 && c.firmware_version==38 && !c.firmware_subversion);
        assert(b.payload.size()==32270 && std::equal(b.payload.begin(),b.payload.end(),RTW8192DUFirmware+32));
    }
    {
        const auto usb=usb_plan(); auto p=prom(false); rtw_92du_board b{};
        const bool integrated=rtw_usb_wifi_station_backend(usb.profile)==RTW_WIFI_RTL8192DU;
        assert(integrated==bool(usb.station_backend_integrated));
        const auto caps=rtw_wifi_radio_profile_capabilities(usb.profile);
        assert(caps.identity_verified);
        if(integrated)assert(caps.station_bands==(RTW_WIFI_BAND_2GHZ|RTW_WIFI_BAND_5GHZ) &&
            caps.station_max_nss==2 && caps.station_max_2g_width_mhz==20 && caps.station_max_5g_width_mhz==20);
        else assert(!caps.station_bands && !caps.station_max_nss);
        assert(rtw_92du_identify(&usb,0x00010000,p.data(),p.size(),0,&b)==RTW_92DU_OK);
        assert(b.mode==RTW_92DU_SINGLE_MAC && b.rf_paths==2 &&
               b.initial_band==RTW_WIFI_BAND_2GHZ && b.mac[5]==0xab);
        p=prom(true);
        assert(rtw_92du_identify(&usb,0x00010000,p.data(),p.size(),0,&b)==RTW_92DU_OK);
        assert(b.mode==RTW_92DU_DUAL_MAC && b.rf_paths==1 &&
               b.initial_band==RTW_WIFI_BAND_5GHZ);
        assert(rtw_92du_identify(&usb,0x00010000,p.data(),p.size(),1,&b)==RTW_92DU_OK);
        assert(b.initial_band==RTW_WIFI_BAND_2GHZ && b.mac[5]==0xac);
        assert(rtw_92du_identify(&usb,0,p.data(),p.size(),0,&b)==RTW_92DU_UNSUPPORTED_CHIP);
        assert(rtw_92du_identify(&usb,0xffffffff,p.data(),p.size(),0,&b)==RTW_92DU_UNSUPPORTED_CHIP);
        p[0]=0; assert(rtw_92du_identify(&usb,0x10000,p.data(),p.size(),0,&b)==RTW_92DU_BAD_PROM);
        p=prom(false); assert(rtw_92du_identify(&usb,0x10000,p.data(),p.size(),1,&b)==RTW_92DU_BAD_PROM);
        p[0x19]=1; assert(rtw_92du_identify(&usb,0x10000,p.data(),p.size(),0,&b)==RTW_92DU_BAD_PROM);
        p=prom(true); assert(rtw_92du_identify(&usb,0x10000,p.data(),0x60,1,&b)==RTW_92DU_BAD_PROM);
        auto forged=usb; auto profile=*usb.profile; forged.profile=&profile;
        assert(rtw_92du_identify(&forged,0x10000,p.data(),p.size(),0,&b)==RTW_92DU_IDENTITY_UNVERIFIED);
        forged=usb; forged.endpoints.num_bulk_in=0;
        assert(rtw_92du_identify(&forged,0x10000,p.data(),p.size(),0,&b)==RTW_92DU_IDENTITY_UNVERIFIED);
        forged=usb;forged.station_backend_integrated^=1U;forged.usb_preflight_only^=1U;
        assert(rtw_92du_validate_usb_plan(&forged)==RTW_92DU_IDENTITY_UNVERIFIED);
        const auto unchanged=b; p=prom(true); p[0x10]=0xff;
        assert(rtw_92du_identify(&usb,0x10000,p.data(),p.size(),0,&b)==RTW_92DU_BAD_PROM);
        assert(std::memcmp(&b,&unchanged,sizeof(b))==0);
    }
    {
        Bus b; auto c=initialized(b); auto opt=rtw_92du_boot_options{0,1,5};
        const auto id=board(); const rtw_usb_rw_transport t{&b,Bus::in,Bus::out};
        const rtw_waiter w{&b,Bus::wait};
        assert(rtw_92du_boot_init(&c,&id,&t,&w,&opt)==RTW_92DU_IO_NOT_CONFIRMED);
        assert(!b.io); opt={1,0,5};
        assert(rtw_92du_boot_init(&c,&id,&t,&w,&opt)==RTW_92DU_IO_NOT_CONFIRMED);
        assert(!b.io); opt={1,1,1001};
        assert(rtw_92du_boot_init(&c,&id,&t,&w,&opt)==RTW_92DU_BAD_ARGUMENT);
    }
    size_t cold_io=0, power_io=0, power_waits=0;
    {
        Bus b; auto c=initialized(b);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK); power_io=b.io; power_waits=b.waits;
        assert(c.power_ready && !c.firmware_ready && b.reg[0x522]==0xff);
        assert(b.reg[0xf8]==0xdc && (b.reg[0x81]&0x82)==0x82);
        assert(b.reg[0x11]==0x2b && (b.reg[0x21]&1) && !(b.reg[0]&1));
        assert(!(b.reg[1]&2) && b.reg[0x100]==0xff && b.reg[0x101]==2);
        assert(b.reg[0x5f0]==0 && b.reg[0x609]==0x10);
        assert(b.reg[0x116]==0xff && b.reg[0x117]==0x27);
        assert(!(b.reg[3]&0x80));
        const auto before=b.io;
        assert(rtw_92du_power_on(&c)==RTW_92DU_WRONG_STATE && b.io==before);
    }
    for (uint8_t index=0; index<2; ++index) {
        Bus b; auto c=initialized(b,true,index);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
        assert(b.reg[0xf8]==0xdb && b.reg[0x609]==8 && b.reg[0x117]==0x13);
        assert(index ? (b.reg[0x53]&3)==3 : (b.reg[0x81]&0x82)==0x80);
        const auto p=image(4096+73);
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_OK);
        assert(c.firmware_ready && !c.firmware_reused && c.firmware_bytes==4096+73);
        assert(c.firmware_version==0x1234 && c.firmware_subversion==7);
        assert(b.payload==std::vector<uint8_t>(p.begin()+32,p.end()));
        assert((b.reg[0x82]&0xf8)==0xb0 && (b.reg[0x82]&7)==1);
        assert(!(b.reg[0x80]&1) && (b.reg[0x80]&6)==6);
        assert(!(b.reg[0x80]&0x40)); // DU's per-MAC ready succeeds without EU bit6.
        assert(index ? b.reg[0x1a]==1 : b.reg[0x18]==1);
        assert(index || (b.reg[0x81]&0x80)); // Never zero reserved MCUFWDL+1.
        cold_io=b.io;
    }
    {
        Bus b; auto c=initialized(b); auto p=image(32768);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_OK);
        assert(b.payload.size()==32768 && (b.reg[0x82]&7)==7);
    }
    {
        Bus b; auto c=initialized(b); auto p=image(65);
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_WRONG_STATE && !b.io);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
        const auto before=b.io; p[0]=0xe0;
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_BAD_FIRMWARE);
        assert(b.io==before && b.payload.empty());
    }
    for (uint8_t flag : {uint8_t(1),uint8_t(4),uint8_t(0x80)}) {
        Bus b; auto c=initialized(b); const auto p=image(65);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK); b.reg[0x80]=flag;
        const auto before=b.io;
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_MCU_BUSY);
        assert(b.io==before+1 && b.payload.empty() && !writes(b,0x1000));
    }
    for (uint8_t index=0;index<2;++index) {
        Bus b; auto c=initialized(b,true,index); const auto p=image(65);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
        b.reg[index ? 0x1a:0x18]=1; // stale MAC ready without MCU RDY
        const auto before=b.io;
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_MCU_BUSY);
        assert(b.io==before+2 && b.payload.empty());
    }
    {
        Bus b; auto c=initialized(b); const auto p=image(65);
        assert(rtw_92du_power_on(&c)==RTW_92DU_OK); b.reg[0x80]=0x86;
        assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_OK);
        assert(c.firmware_reused && c.firmware_ready && !c.firmware_bytes && b.payload.empty());
    }
    for (int kind=0;kind<5;++kind) {
        Bus b; auto c=initialized(b,true,0); const auto p=image(65);
        if (kind==0) b.autoload=false;
        if (kind==1) b.apfm=false;
        if (kind==2) b.reg[0x17]=0x80;
        if (kind<3) assert(rtw_92du_power_on(&c)==RTW_92DU_POLL_TIMEOUT);
        else {
            assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
            if (kind==3) b.checksum=false;
            else { b.mac_ready=false; b.reg[0x80]=0x40; }
            assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_POLL_TIMEOUT);
            assert(!c.firmware_ready && !(b.reg[0x80]&1));
        }
        assert(b.reg[0x522]==0xff && b.io<200 && b.waits<30);
        const auto before=b.io;
        assert(rtw_92du_power_on(&c)!=RTW_92DU_OK && b.io==before);
    }
    for (uint16_t a : {uint16_t(0x81),uint16_t(0x53)}) {
        Bus b; auto c=initialized(b); b.reg[a]=a==0x81 ? 0x80:1;
        assert(rtw_92du_power_on(&c)==RTW_92DU_MCU_BUSY);
        assert(!std::any_of(b.ops.begin(),b.ops.end(),[](const Op& o){return o.write;}));
    }
    {
        Bus b; auto c=initialized(b); b.reg[0xf2]=2;
        assert(rtw_92du_power_on(&c)==RTW_92DU_UNSUPPORTED_CHIP);
        assert(!std::any_of(b.ops.begin(),b.ops.end(),[](const Op& o){return o.write;}));
    }
    // Every single USB failure and short completion is a hard stop. Firmware
    // writes are never retried; the sole subsequent writes are bounded cleanup.
    for (bool short_io : {false,true}) {
        for (size_t fail=1;fail<=cold_io;++fail) {
            Bus b; auto c=initialized(b,true,1); const auto p=image(4096+73);
            if (short_io) b.short_at=fail; else b.fail_at=fail;
            auto status=rtw_92du_power_on(&c);
            if (status==RTW_92DU_OK) status=rtw_92du_firmware_start(&c,p.data(),p.size());
            assert(status==RTW_92DU_IO_ERROR && !c.firmware_ready);
            assert(b.io<=fail+4);
            const auto before=b.io;
            assert(rtw_92du_firmware_start(&c,p.data(),p.size())!=RTW_92DU_OK && b.io==before);
        }
    }
    for (size_t fail=1;fail<=power_waits;++fail) {
        Bus b; auto c=initialized(b); b.fail_wait_at=fail;
        assert(rtw_92du_power_on(&c)==RTW_92DU_WAIT_ERROR && !c.power_ready);
    }
    // Exercise delay failure in every actual poll, not just fixed LDO waits.
    for (int kind=0;kind<5;++kind) {
        Bus b; auto c=initialized(b,true,0); const auto p=image(65);
        if (kind==0) { b.reg[0x17]=0x80; b.fail_wait_at=1; }
        if (kind==1) { b.autoload=false; b.fail_wait_at=1; }
        if (kind==2) { b.apfm=false; b.fail_wait_at=3; }
        if (kind<3) assert(rtw_92du_power_on(&c)==RTW_92DU_WAIT_ERROR);
        else {
            assert(rtw_92du_power_on(&c)==RTW_92DU_OK);
            b.fail_wait_at=b.waits+1;
            if (kind==3) b.checksum=false; else b.mac_ready=false;
            assert(rtw_92du_firmware_start(&c,p.data(),p.size())==RTW_92DU_WAIT_ERROR);
        }
        assert(c.last_error==RTW_92DU_WAIT_ERROR && !c.power_ready &&
               !c.firmware_ready && b.reg[0x522]==0xff && !(b.reg[0x80]&1));
    }
    std::printf("RTL8192DU boot: identity, power %zu I/O, DU pages/per-MAC FW, %zu injected failures passed\n",
        power_io,cold_io*2+power_waits+5);
}
