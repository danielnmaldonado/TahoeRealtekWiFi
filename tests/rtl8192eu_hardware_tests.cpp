// SPDX-License-Identifier: GPL-2.0-only
// RTL8192EU 8051 boot over a deterministic simulated USB control device.
#include "rtw/RTL8192EUHardware.h"
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

namespace {
static unsigned assertions=0;
#define CHECK(value) do{++assertions; assert((value));}while(0)
struct Sim {
    std::array<uint8_t,0x2000> regs{};
    std::array<uint8_t,32768> fw_ram{};
    unsigned writes=0;
    unsigned fw_chunks=0;
    unsigned fail_write_n=0;
    unsigned waits=0;
    bool ready_for_power=true;
    bool ready_for_mac=true;
    bool ready_for_checksum=true;
    bool ready_for_fw=true;
    bool wait_fails=false;
    bool wrong_reg=false;
    uint8_t active_page=0;
    bool started_firmware=false;
    Sim(){
        set32(0xf0,0x04011001U);
        set32(0xfc,0x0U);
        set32(0xec,0x0U);
        set32(0x80,0x0U);
        set32(0x04,(1U<<17U));
    }
    void set32(unsigned addr,uint32_t val){
        for(unsigned i=0;i<4;++i)regs[addr+i]=(uint8_t)(val>>(8U*i));
    }
    uint32_t get32(unsigned addr) const{
        uint32_t v=0;for(unsigned i=0;i<4;++i) v|=(uint32_t)regs[addr+i]<<(8U*i);
        return v;
    }
};
static rtw_usb_error read(void *ctx,const rtw_usb_setup *req,uint8_t *out,uint16_t *actual){
    auto& sim=*static_cast<Sim*>(ctx);
    if(actual)*actual=0;
    if(!out||!actual||!req||req->bm_request_type!=0xc0U||req->b_request!=5U||
       req->w_index!=0U||req->w_value+req->w_length>sim.regs.size())return RTW_USB_INVALID_ARGUMENT;
    if(req->w_value==0x0004U && req->w_length==4U){
        unsigned v=sim.get32(4U);
        if(!sim.ready_for_power) v&=~(1U<<17U);
        if(!sim.ready_for_mac) v|=(1U<<8U);
        sim.set32(4U,v);
    }
    if(req->w_value==0x80U && req->w_length==4U){
        uint32_t v=sim.get32(0x80U);
        if(sim.ready_for_checksum && sim.fw_chunks>0U) v|=4U;
        else v&=~4U;
        if(sim.started_firmware && sim.ready_for_fw) v|=0x40U;
        else v&=~0x40U;
        sim.set32(0x80U,v);
    }
    std::memcpy(out,sim.regs.data()+req->w_value,req->w_length);
    *actual=req->w_length;
    return RTW_USB_OK;
}
static rtw_usb_error write(void *ctx,const rtw_usb_setup *req,const uint8_t *buf,uint16_t *actual){
    auto& sim=*static_cast<Sim*>(ctx);
    if(actual)*actual=0;
    if(!req||!buf||!actual||req->bm_request_type!=0x40U||req->b_request!=5U||
       req->w_index!=0U||req->w_length==0 ||req->w_length>254U ||
       req->w_value+req->w_length>sim.regs.size())return RTW_USB_INVALID_ARGUMENT;
    ++sim.writes;
    if(sim.fail_write_n==sim.writes) return RTW_USB_IO_ERROR;
    if(req->w_value>=0x1000U){
        const size_t off=(size_t)sim.active_page*4096U+(size_t)req->w_value-0x1000U;
        if(off+req->w_length>sim.fw_ram.size())return RTW_USB_INVALID_ARGUMENT;
        std::memcpy(sim.fw_ram.data()+off,buf,req->w_length);
        ++sim.fw_chunks;
    }
    std::memcpy(sim.regs.data()+req->w_value,buf,req->w_length);
    if(req->w_value<=0x82U && req->w_value+req->w_length>0x82U) {
        sim.active_page=sim.regs[0x82U]&7U;
    }
    // HW clears the MAC_ENABLE bit when the command is handled.
    if(req->w_value==4U && req->w_length==4U && sim.ready_for_mac)
        sim.set32(4U,sim.get32(4U)&~(1U<<8U));
    // Firmware initialized when 8051 CPU is re-enabled after FW_RDY.
    if(req->w_value==2U && req->w_length==2U && (sim.get32(0x80U)&2U) &&
       (sim.get32(2U)&(1U<<10U))) sim.started_firmware=true;
    *actual=req->w_length;
    return RTW_USB_OK;
}
static int waiter(void *ctx,uint32_t us){
    auto& sim=*static_cast<Sim*>(ctx);++sim.waits;
    return us!=0U && !sim.wait_fails;
}
static std::vector<uint8_t> firmware(size_t length){
    std::vector<uint8_t> f(32+length,0U);
    f[0]=0xe1;f[1]=0x92;f[4]=7;f[6]=2;
    f[12]=(uint8_t)(length&0xff);f[13]=(uint8_t)((length>>8)&0xff);
    for(size_t i=0;i<length;++i) f[32+i]=(uint8_t)((i*17U+3U)&0xffU);
    return f;
}
static rtw_92e_boot_result run(Sim& sim,const std::vector<uint8_t>& blob,rtw_92e_boot_report& rep,
                               uint16_t vid=0x2357,uint16_t pid=0x0108){
    const rtw_usb_rw_transport usb{&sim,&read,&write};
    const rtw_waiter delay{&sim,&waiter};
    const rtw_92e_boot_config config{8,100};
    return rtw_8192eu_boot_8051(&usb,&delay,vid,pid,blob.data(),blob.size(),&config,&rep);
}
static void success(){
    Sim sim;
    auto blob=firmware(8192+135U);rtw_92e_boot_report rep{};
    CHECK(run(sim,blob,rep)==RTW_92E_BOOT_OK);
    CHECK(rep.last_stage==RTW_92E_STAGE_FW_RUNNING);
    CHECK(rep.firmware_ready==1U && rep.pages==3U);
    CHECK(rep.firmware_bytes==blob.size()-32U);
    CHECK(rep.usb_firmware_writes==2U*32U+2U);
    CHECK(rep.firmware_version==7U && rep.firmware_subversion==2U);
    CHECK(sim.fw_chunks==rep.usb_firmware_writes);
    CHECK((sim.get32(0x80U)&1U)==0U);
    CHECK((sim.get32(0x80U)&0x42U)==0x42U);
    CHECK(!std::memcmp(sim.fw_ram.data(),blob.data()+32U,rep.firmware_bytes));
}
static void refuse_wrong_profiles(){
    Sim sim;auto blob=firmware(256U);rtw_92e_boot_report rep{};
    CHECK(run(sim,blob,rep,0x0bda,0x8812)==RTW_92E_BOOT_WRONG_DEVICE);
    CHECK(sim.writes==0U && rep.firmware_bytes==0U);
    blob[1]=0x95;
    CHECK(run(sim,blob,rep)==RTW_92E_BOOT_BAD_FIRMWARE);
    CHECK(sim.writes==0U);
    blob[1]=0x92;
    sim.set32(0xf0,0x00811001U);
    CHECK(run(sim,blob,rep)==RTW_92E_BOOT_WRONG_DEVICE);
    CHECK(sim.writes==0U);
    const rtw_usb_rw_transport usb{&sim,&read,&write};
    const rtw_waiter delay{&sim,&waiter};
    const rtw_92e_boot_config invalid{0,1};
    CHECK(rtw_8192eu_boot_8051(&usb,&delay,0x2357,0x0108,
         blob.data(),blob.size(),&invalid,&rep)==RTW_92E_BOOT_INVALID_ARGUMENT);
    CHECK(sim.writes==0U);
}
static void timeouts(){
    auto blob=firmware(1024U);rtw_92e_boot_report rep{};
    {Sim sim;sim.ready_for_power=false;
     CHECK(run(sim,blob,rep)==RTW_92E_BOOT_POWER_TIMEOUT);
     CHECK(sim.fw_chunks==0U);}
    {Sim sim;sim.ready_for_mac=false;
     CHECK(run(sim,blob,rep)==RTW_92E_BOOT_MAC_TIMEOUT);
     CHECK(sim.fw_chunks==0U);}
    {Sim sim;sim.ready_for_checksum=false;
     CHECK(run(sim,blob,rep)==RTW_92E_BOOT_CHECKSUM_TIMEOUT);
     CHECK((sim.get32(0x80U)&1U)==0U);}
    {Sim sim;sim.ready_for_fw=false;
     CHECK(run(sim,blob,rep)==RTW_92E_BOOT_FIRMWARE_TIMEOUT);
     CHECK((sim.get32(0x80U)&1U)==0U);}
    {Sim sim;sim.ready_for_power=false;sim.wait_fails=true;
     CHECK(run(sim,blob,rep)==RTW_92E_BOOT_SLEEP_FAILED);}
}
static void failures_and_fuzz(){
    const auto blob=firmware(258U);
    for(unsigned n=1;n<=45;++n){
        Sim sim;sim.fail_write_n=n;rtw_92e_boot_report rep{};
        const auto result=run(sim,blob,rep);
        if(sim.writes>=n) CHECK(result!=RTW_92E_BOOT_OK);
        // A single failed USB transfer must never claim FW_RUNNING.
        CHECK(!rep.firmware_ready || result==RTW_92E_BOOT_OK);
    }
    for(size_t len : {size_t(1),size_t(127),size_t(128),size_t(4096),
                      size_t(4097),size_t(8192),size_t(32768)}) {
        Sim sim;auto img=firmware(len);rtw_92e_boot_report rep{};
        const auto result=run(sim,img,rep);
        CHECK(result==RTW_92E_BOOT_OK);
        CHECK(rep.firmware_bytes==len);
        CHECK(rep.usb_firmware_writes==(len+127U)/128U);
        CHECK(rep.pages==(len+4095U)/4096U);
        CHECK(!std::memcmp(sim.fw_ram.data(),img.data()+32U,len));
    }
}
}
int main(){success();refuse_wrong_profiles();timeouts();failures_and_fuzz();
    std::printf("rtl8192eu-hardware-tests: %u checks (simulated USB, no physical hardware)\n",assertions);
}
