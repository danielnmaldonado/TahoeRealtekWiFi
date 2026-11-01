/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/FirmwareBoot.h"
#include "rtw/PowerSequence.h"
#include "rtw/LLT.h"
#include "rtw/MacDMA.h"
#include "rtw/HAL8812.h"
#include "rtw/PHYRegisters.h"
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>

static unsigned checks=0;
#define CHECK(x) do { ++checks; if(!(x)) {std::fprintf(stderr,"check failed %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();} } while(0)

struct Mock {
    std::array<uint8_t,65536> regs{};
    std::array<uint8_t,512> physical{};
    std::vector<uint8_t> writtenFw;
    unsigned writes=0,reads=0, delays=0, fwchunks=0, lltwrites=0;
    unsigned fail_write=0,fail_read=0,fail_delay=0;
    unsigned efuseIdx=0;
    bool efuseOpen=false,readyChecksum=true,readyFirmware=true,checksumStuck=false;
    bool lltStuck=false, powerStuck=false;
    uint8_t active_page=0;
    Mock():writtenFw(32768,0) {
        physical.fill(0xff);
        const unsigned char fixture[]={
          0x0f,0x3e,0xff,0x23,
          0x2f,0x3c,0x08,0x00,0x03,0xff,
          0x4f,0x30,0xda,0x0b,0x12,0x88,0xff,0xff,0xff,0x02,
          0x6f,0x38,0x11,0x22,0x33,0x44,0x55,0xff,0xff
        };
        std::memcpy(physical.data(),fixture,sizeof(fixture));
        regs[0xF0]=0x37;regs[0xF1]=0x11;regs[0xF2]=0x41;regs[0xF3]=0x04;
        regs[0x06]=0x02;
    }
};
static rtw_usb_error in(void* ctx,const rtw_usb_setup* req,uint8_t* out,uint16_t* actual) {
    auto& m=*static_cast<Mock*>(ctx);
    ++m.reads;
    if(m.fail_read && m.fail_read==m.reads) return RTW_USB_IO_ERROR;
    if(!req||!out||!actual||req->bm_request_type!=0xC0||req->b_request!=5||
       (req->w_length!=1 && req->w_length!=2 && req->w_length!=4) ||
       static_cast<unsigned>(req->w_value)+req->w_length>65536)
        return RTW_USB_INVALID_ARGUMENT;
    std::memcpy(out,m.regs.data()+req->w_value,req->w_length);
    if(req->w_value==0x0030 && req->w_length==4 && m.efuseOpen) {
        out[0]=m.physical[m.efuseIdx];out[3]=0x80;
    }
    if(req->w_value==0x01E0 && req->w_length==4 && !m.lltStuck)
        out[3]&=0x3f;
    if(m.powerStuck && req->w_value==0x06 && req->w_length==1) out[0]=0;
    *actual=req->w_length;
    return RTW_USB_OK;
}
static rtw_usb_error out(void* ctx,const rtw_usb_setup* req,const uint8_t* data,uint16_t* actual) {
    auto& m=*static_cast<Mock*>(ctx);
    ++m.writes;
    if(m.fail_write && m.fail_write==m.writes) return RTW_USB_IO_ERROR;
    if(!req||!data||!actual||req->bm_request_type!=0x40||req->b_request!=5||
       !req->w_length||req->w_length>254 ||
       static_cast<unsigned>(req->w_value)+req->w_length>65536)
        return RTW_USB_INVALID_ARGUMENT;
    std::memcpy(m.regs.data()+req->w_value,data,req->w_length);
    /* MCUFWDL[2] is W1C, unlike an ordinary RAM-backed register. */
    if(req->w_value==0x80 && req->w_length==1 && (data[0]&0x04)) {
        if(m.checksumStuck) m.regs[0x80]|=0x04;
        else m.regs[0x80]&=(uint8_t)~0x04;
    }
    if(req->w_value==0xcf && req->w_length==1) m.efuseOpen=data[0]==0x69;
    if(req->w_value==0x33 && req->w_length==1 && !(data[0]&0x80))
        m.efuseIdx=m.regs[0x31] | ((static_cast<unsigned>(m.regs[0x32])&3)<<8);
    if(req->w_value==0x05 && req->w_length==1 && !m.powerStuck)
        m.regs[0x05] &= 0xFE;
    if(req->w_value==0x1e0 && req->w_length==4) ++m.lltwrites;
    if(req->w_value==0x82 && req->w_length==1) m.active_page=data[0]&7;
    if(req->w_value>=0x1000 && req->w_value<0x2000) {
        unsigned off=static_cast<unsigned>(m.active_page)*4096+(req->w_value-0x1000);
        if(off+req->w_length>m.writtenFw.size())return RTW_USB_INVALID_ARGUMENT;
        std::memcpy(m.writtenFw.data()+off,data,req->w_length);
        ++m.fwchunks;
    }
    if(req->w_value==0x80 && req->w_length==1 && !(data[0]&1) && m.fwchunks && m.readyChecksum)
        m.regs[0x80]|=0x04;
    if(req->w_value==0x80 && req->w_length==4 && (data[0]&2) && m.readyFirmware)
        m.regs[0x80]|=0x40;
    *actual=req->w_length;
    return RTW_USB_OK;
}
static int sleep_us(void* ctx,uint32_t micros){
    auto& m=*static_cast<Mock*>(ctx);
    ++m.delays;
    if(m.fail_delay && m.delays==m.fail_delay) return 0;
    return micros>0 && micros<=10000;
}
static std::vector<uint8_t> firmware(size_t payload=4500) {
    std::vector<uint8_t> b(payload+32,0);
    b[0]=1;b[1]=0x95;b[4]=52;b[6]=14;
    for(size_t i=0;i<payload;i++) b[i+32]=static_cast<uint8_t>(i*7+3);
    return b;
}
static rtw_usb_rw_transport bus(Mock& m){return {&m,in,out};}
static rtw_waiter waiter(Mock& m){return {&m,sleep_us};}
static void test_power(){
    Mock m;auto b=bus(m);auto w=waiter(m);size_t n=13;
    CHECK(rtw_power_on_8812au_bcut_tsmc_usb(&b,&w,0x04411137,&n)==RTW_POWER_OK && n==7);
    CHECK(rtw_power_on_8812au_bcut_tsmc_usb(&b,&w,0x04411137 & ~0x1000U,&n)==RTW_POWER_UNSUPPORTED && n==0);
    CHECK(rtw_power_on_8812au_bcut_tsmc_usb(&b,&w,0x04411137 | (1U<<19),&n)==RTW_POWER_UNSUPPORTED);
    const rtw_power_step invalid[]={{0x0005,RTW_PWR_WRITE8,0x01,0},{0,200,0,0},{0,RTW_PWR_END,0,0}};
    auto prev=m.writes;
    CHECK(rtw_power_execute(&b,invalid,3,2,&w,&n)==RTW_POWER_BAD_ARGUMENT && m.writes==prev);
    Mock stuck;stuck.powerStuck=true;auto sb=bus(stuck);auto sw=waiter(stuck);
    CHECK(rtw_power_on_8812au_bcut_tsmc_usb(&sb,&sw,0x04411137,&n)==RTW_POWER_POLL_TIMEOUT);
    CHECK(stuck.delays==999);
    Mock failed;failed.fail_write=1;auto fb=bus(failed);auto fw=waiter(failed);
    CHECK(rtw_power_on_8812au_bcut_tsmc_usb(&fb,&fw,0x04411137,&n)==RTW_POWER_USB_ERROR);
}
static void test_llt(){
    Mock m;auto b=bus(m);auto w=waiter(m);size_t n=0;
    CHECK(rtw_llt_initialize_8812au(&b,&w,24,&n)==RTW_LLT_OK && n==256);
    CHECK(m.lltwrites==256);
    CHECK(m.regs[0x1e0]==0xf7 && m.regs[0x1e1]==0xff);
    Mock bad;bad.lltStuck=true;auto bb=bus(bad);auto ww=waiter(bad);
    CHECK(rtw_llt_initialize_8812au(&bb,&ww,3,&n)==RTW_LLT_TIMEOUT && n==0);
    CHECK(rtw_llt_initialize_8812au(&b,&w,0,&n)==RTW_LLT_BAD_ARGUMENT);
}
static void test_firmware(){
    auto blob=firmware();Mock m;auto b=bus(m);auto w=waiter(m);
    rtw_boot_report report{};rtw_boot_options opts{7,1000};
    CHECK(rtw_fw_boot_8812au(&b,&w,blob.data(),blob.size(),&opts,&report)==RTW_BOOT_OK);
    CHECK(report.firmware_ready && report.checksum_seen && report.pages_written==2);
    CHECK(report.bytes_written==4500 && report.vendor_writes==m.fwchunks);
    CHECK(std::memcmp(m.writtenFw.data(),blob.data()+32,4500)==0);
    CHECK(!(m.regs[0x80]&1));
    Mock noCS;noCS.readyChecksum=false;auto ncb=bus(noCS);auto ncw=waiter(noCS);
    CHECK(rtw_fw_boot_8812au(&ncb,&ncw,blob.data(),blob.size(),&opts,&report)==RTW_BOOT_CHECKSUM_TIMEOUT);
    CHECK(!(noCS.regs[0x80]&1));
    /* A checksum bit from an earlier download must not satisfy this boot. */
    Mock stale;stale.regs[0x80]=0x04;stale.readyChecksum=false;
    auto staleBus=bus(stale);auto staleWait=waiter(stale);
    CHECK(rtw_fw_boot_8812au(&staleBus,&staleWait,blob.data(),
          blob.size(),&opts,&report)==RTW_BOOT_CHECKSUM_TIMEOUT);
    CHECK(!report.checksum_seen && !report.firmware_ready);
    /* Failure to reset checksum prevents any firmware payload writes. */
    Mock stuckCS;stuckCS.regs[0x80]=0x04;stuckCS.checksumStuck=true;
    auto stuckBus=bus(stuckCS);auto stuckWait=waiter(stuckCS);
    CHECK(rtw_fw_boot_8812au(&stuckBus,&stuckWait,blob.data(),
          blob.size(),&opts,&report)==RTW_BOOT_USB_ERROR);
    CHECK(stuckCS.fwchunks==0 && !(stuckCS.regs[0x80]&1));
    Mock noReady;noReady.readyFirmware=false;auto nrb=bus(noReady);auto nrw=waiter(noReady);
    CHECK(rtw_fw_boot_8812au(&nrb,&nrw,blob.data(),blob.size(),&opts,&report)==RTW_BOOT_INIT_TIMEOUT);
    Mock failed;failed.fail_write=12;auto fb=bus(failed);auto fw=waiter(failed);
    CHECK(rtw_fw_boot_8812au(&fb,&fw,blob.data(),blob.size(),&opts,&report)==RTW_BOOT_USB_ERROR);
    CHECK(!(failed.regs[0x80]&1));
    blob[1]=0;
    CHECK(rtw_fw_boot_8812au(&b,&w,blob.data(),blob.size(),&opts,&report)==RTW_BOOT_BAD_FIRMWARE);
    CHECK(report.bytes_written==0);
}
static void test_mac(){
    Mock m;auto b=bus(m);size_t n=0;
    const rtw_mac_options o{3,4,0};
    CHECK(rtw_mac_dma_prepare_8812au(&b,&o,&n)==RTW_MAC_OK && n==13);
    CHECK(m.regs[0x424]==0xf7 && m.regs[0x425]==0xf7);
    CHECK(m.regs[0x116]==0x7f && m.regs[0x117]==0x3e);
    CHECK(m.regs[0x10c]==0xb4 && m.regs[0x10d]==0xf5);
    CHECK(m.regs[0x522]==0 && m.regs[0x100]==0); // no TX start, no RX monitor
    const rtw_mac_options bad{0,4,0};
    CHECK(rtw_mac_dma_prepare_8812au(&b,&bad,&n)==RTW_MAC_BAD_ARGUMENT && n==0);
}
static void test_hal(){
    auto image=firmware();Mock m;auto b=bus(m);auto w=waiter(m);
    rtw_hal_environment env{&b,&w,image.data(),image.size(),3,0};
    rtw_hal_report report{};
    CHECK(rtw_hal_boot_pre_rf_8812au(&env,&report)==RTW_HAL_NOT_AUTHORIZED);
    CHECK(!m.writes && !m.reads);
    env.allow_hardware_writes=1;
    CHECK(rtw_hal_boot_pre_rf_8812au(&env,&report)==RTW_HAL_OK);
    CHECK(report.last_success==RTW_HAL_MAC_PREPARED && report.failed_at==RTW_HAL_NOT_STARTED);
    CHECK(report.sys_cfg==0x04411137 && report.identity.rfe_type==3);
    CHECK(report.power_steps==7 && report.llt_entries==256);
    CHECK(report.firmware.bytes_written==image.size()-32);
    CHECK(report.mac_registers==13);
    CHECK(!report.radio_ready && !report.ethernet_authorized);
    Mock badId;badId.physical.fill(0xff);auto bb=bus(badId);auto bw=waiter(badId);
    env.bus=&bb;env.waiter=&bw;
    CHECK(rtw_hal_boot_pre_rf_8812au(&env,&report)==RTW_HAL_EFUSE_FAILURE);
    CHECK(!report.radio_ready);
    Mock badPower;badPower.powerStuck=true;auto pb=bus(badPower);auto pw=waiter(badPower);
    env.bus=&pb;env.waiter=&pw;
    CHECK(rtw_hal_boot_pre_rf_8812au(&env,&report)==RTW_HAL_POWER_FAILURE);
    CHECK(report.failed_at==RTW_HAL_POWERED);
    image[1]=0;
    CHECK(rtw_hal_boot_pre_rf_8812au(&env,&report)==RTW_HAL_BAD_ARGUMENT);
    CHECK(!report.radio_ready);
}
static void test_phy(){
    Mock m;auto b=bus(m);auto w=waiter(m);size_t n=0;
    rtw_phy_entry entries[]={
      {0x0620,RTW_PHY_MAC8,0x33},
      {0x0808,RTW_PHY_BB32,0xAABBCCDD},
      {0x0018,RTW_PHY_RF_A,0x7036},
      {0x0018,RTW_PHY_RF_B,0x7056},
      {0,RTW_PHY_DELAY_US,25000}
    };
    CHECK(rtw_phy_apply_8812au(&b,&w,entries,5,&n)==RTW_PHY_OK && n==5);
    CHECK(m.regs[0x620]==0x33);
    CHECK(m.regs[0x808]==0xdd && m.regs[0x80b]==0xaa);
    CHECK(m.regs[0xc90]==0x36 && m.regs[0xc91]==0x70 && m.regs[0xc92]==0x80 && m.regs[0xc93]==0x01);
    CHECK(m.regs[0xe90]==0x56 && m.regs[0xe91]==0x70 && m.regs[0xe92]==0x80 && m.regs[0xe93]==0x01);
    CHECK(m.delays==3);
    const auto original=m.writes;
    entries[4].operation=255;
    CHECK(rtw_phy_apply_8812au(&b,&w,entries,5,&n)==RTW_PHY_BAD_TABLE && n==0 && m.writes==original);
    entries[4].operation=RTW_PHY_DELAY_US;
    entries[2].value=0x100000;
    CHECK(rtw_phy_apply_8812au(&b,&w,entries,5,&n)==RTW_PHY_BAD_TABLE && m.writes==original);
    entries[2].value=0x7036;
    Mock rf;auto rb=bus(rf);
    CHECK(rtw_phy_enable_rf_8812au(&rb)==RTW_PHY_OK);
    CHECK(rf.regs[0x02]==0x1f && rf.regs[0x1f]==0x07 && rf.regs[0x76]==0x07);
    CHECK(rtw_phy_config_rfe3_24ghz(&rb)==RTW_PHY_OK);
    CHECK(rf.regs[0xcb0]==0x70 && rf.regs[0xcb3]==0x54);
    CHECK(rf.regs[0xeb0]==0x70 && rf.regs[0xeb3]==0x54);
    CHECK(rf.regs[0xcb4+3]==0x01 && rf.regs[0xeb4+3]==0x01);
    CHECK(rf.regs[0x900]==0x01);
    CHECK(rtw_phy_apply_8812au(nullptr,&w,entries,5,&n)==RTW_PHY_BAD_ARGUMENT);
}
int main(){test_power();test_llt();test_firmware();test_mac();test_hal();test_phy();
    std::printf("rtw-hw-tests: %u checks passed (simulated USB, never real hardware)\n",checks);return 0;}
