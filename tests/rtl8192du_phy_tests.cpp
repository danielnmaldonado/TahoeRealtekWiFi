/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtl8192du_fixture.hpp"
using namespace du_test;
static void dfs_channels() {
    // Pinned rtl8192d channel_all/ROM groups. 144 has no power/IQK slot.
    const uint8_t channels[]={52,56,60,64,100,104,108,112,116,120,124,128,132,136,140};
    for(bool dual:{false,true})for(bool internal:{false,true})for(auto cut:{RTW_92DU_CUT_C,RTW_92DU_CUT_D,RTW_92DU_CUT_E}) {
        Fixture f(dual,0,internal&&cut!=RTW_92DU_CUT_C,RTW_92DU_USB_HIGH,cut);
        for(unsigned g=0;g<12;++g)for(unsigned path=0;path<2;++path)
            f.identity.logical[0x67+(g/3)*21+g%3+path*3]=uint8_t(10+g+path);
        f.boot.phy=nullptr;assert(rtw_92du_phy_init(&f.boot,&f.identity,f.storage.data(),f.storage.size()*8,RTW_92DU_USB_HIGH)==RTW_92DU_OK);
        assert(rtw_92du_phy_load_tables(&f.boot)==RTW_92DU_OK);
        for(uint8_t ch:channels){
            assert(rtw_92du_phy_supports_channel(&f.boot,ch));
            assert(rtw_92du_phy_set_channel(&f.boot,ch)==RTW_92DU_OK);
            assert((f.bus.rf[0][0x18]&255U)==ch && f.bus.regs[0x522]==255);
            unsigned group=ch<=54?4:ch<=64?5:ch<=112?6:ch<=126?7:8;
            assert(Bus::word(f.bus.regs.data()+0xe00,4)==(10U+group)*0x01010101U);
            if(!dual)assert(Bus::word(f.bus.regs.data()+0x830,4)==(11U+group)*0x01010101U);
            assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
            assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
            assert(rtw_92du_phy_set_scan_channel(&f.boot,ch)==RTW_92DU_OK);
            assert(rtw_92du_phy_receive_ready(&f.boot)&&!rtw_92du_phy_tx_active(&f.boot)&&f.bus.regs[0x522]==255);
        }
        auto calls=f.bus.io;assert(!rtw_92du_phy_supports_channel(&f.boot,144));
        assert(rtw_92du_phy_set_channel(&f.boot,144)==RTW_92DU_BAD_ARGUMENT&&f.bus.io==calls);
    }
    Fixture second(true,1);second.setup(1);auto calls=second.bus.io;
    assert(!rtw_92du_phy_supports_channel(&second.boot,52));
    assert(rtw_92du_phy_set_channel(&second.boot,52)==RTW_92DU_UNSUPPORTED_CHIP&&second.bus.io==calls);
}
static void power_reduction(){
 for(bool dual:{false,true})for(uint8_t ch:{uint8_t(52),uint8_t(100),uint8_t(140)}){
  Fixture baseline(dual,0),limited(dual,0);baseline.setup(ch);limited.setup(ch);
  assert(rtw_92du_phy_set_power_reduction(&limited.boot,2)==RTW_92DU_OK);
  assert(rtw_92du_phy_set_channel(&limited.boot,ch)==RTW_92DU_OK);
  const unsigned regs[]={0xe00,0xe04,0xe10,0xe14,0xe18,0xe1c,0x830,0x834,0x83c,0x848,0x84c,0x868};
  for(unsigned phase=0;phase<2;++phase){
   if(phase)for(auto*f:{&baseline,&limited}){assert(rtw_92du_phy_calibrate(&f->boot)==RTW_92DU_OK);assert(rtw_92du_phy_activate(&f->boot)==RTW_92DU_OK);f->bus.thermal=31;assert(rtw_92du_phy_tick(&f->boot)==RTW_92DU_OK);assert(rtw_92du_phy_tick(&f->boot)==RTW_92DU_OK);}
   for(auto reg:regs)for(unsigned j=0;j<4;++j)assert(unsigned(baseline.bus.regs[reg+j])==unsigned(limited.bus.regs[reg+j])+4U);
   assert((baseline.bus.rf[0][0x18]&255U)==(limited.bus.rf[0][0x18]&255U));
  }
  assert(rtw_92du_phy_set_power_limit_mbm(&limited.boot,949)==RTW_92DU_OK);
  assert(rtw_92du_phy_calibrate(&limited.boot)==RTW_92DU_WRONG_STATE);
  assert(rtw_92du_phy_set_channel(&limited.boot,ch)==RTW_92DU_OK);
  for(auto reg:regs)assert(Bus::word(limited.bus.regs.data()+reg,4)==0x0e0e0e0eU); // floor949/50=18, then2dB=4indices
  assert(rtw_92du_phy_calibrate(&limited.boot)==RTW_92DU_OK&&rtw_92du_phy_activate(&limited.boot)==RTW_92DU_OK);
  assert(rtw_92du_phy_tick(&limited.boot)==RTW_92DU_OK&&rtw_92du_phy_tick(&limited.boot)==RTW_92DU_OK);
  for(auto reg:regs)for(unsigned j=0;j<4;++j)assert(limited.bus.regs[reg+j]<=14U);
  assert(rtw_92du_phy_set_power_limit_mbm(&limited.boot,INT32_MAX)==RTW_92DU_OK);
  assert(rtw_92du_phy_set_power_reduction(&limited.boot,0)==RTW_92DU_OK&&rtw_92du_phy_set_channel(&limited.boot,ch)==RTW_92DU_OK);
 }
 Fixture floor;floor.setup(52);assert(rtw_92du_phy_set_power_reduction(&floor.boot,255)==RTW_92DU_OK);
 assert(rtw_92du_phy_set_channel(&floor.boot,52)!=RTW_92DU_OK&&!rtw_92du_phy_tx_active(&floor.boot)&&floor.bus.regs[0x522]==255);
}
static void signed_caps_and_rejected_tune(){
 for(bool dual:{false,true}){
  for(int32_t cap:{INT32_MIN,int32_t(-10000)}){Fixture f(dual,0);f.setup(52);assert(rtw_92du_phy_set_power_limit_mbm(&f.boot,cap)==RTW_92DU_OK);
   assert(rtw_92du_phy_set_channel(&f.boot,52)==RTW_92DU_BAD_ARGUMENT&&!rtw_92du_phy_tx_active(&f.boot));
   assert(rtw_92du_phy_calibrate(&f.boot)!=RTW_92DU_OK&&rtw_92du_phy_activate(&f.boot)!=RTW_92DU_OK&&f.bus.regs[0x522]==255);
  }
  Fixture f(dual,0);f.setup(52);assert(rtw_92du_phy_set_power_limit_mbm(&f.boot,949)==RTW_92DU_OK&&rtw_92du_phy_set_channel(&f.boot,52)==RTW_92DU_OK);
  assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK&&rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);auto calls=f.bus.io;
  assert(rtw_92du_phy_set_channel(&f.boot,144)==RTW_92DU_BAD_ARGUMENT&&f.bus.io==calls&&rtw_92du_phy_tx_active(&f.boot));
  f.bus.thermal=31;assert(rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK&&rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK);
  for(unsigned reg:{0xe00,0xe04,0xe10,0xe14,0xe18,0xe1c,0x830,0x834,0x83c,0x848,0x84c,0x868})for(unsigned j=0;j<4;++j)assert(f.bus.regs[reg+j]<=18U);
 }
 puts("DU signed cap bounds fail closed; rejected channel preserves live thermal cap");
}
int main() {
    signed_caps_and_rejected_tune();power_reduction();dfs_channels();
    rtw_92du_boot boot{};rtw_92du_rom rom{};
    std::vector<uint64_t> storage((rtw_92du_phy_size()+7)/8);
    assert(rtw_92du_phy_init(&boot,&rom,storage.data(),storage.size()*8,RTW_92DU_USB_HIGH)==RTW_92DU_WRONG_STATE);
    assert(!rtw_92du_phy_radio_ready(&boot));
    assert(rtw_92du_phy_activate(&boot)==RTW_92DU_WRONG_STATE);
    {
        Fixture f;auto candidate=f.identity;auto operating=f.boot;operating.phy=nullptr;
        for(unsigned i=0;i<6;++i)candidate.logical[0xa6+i]=uint8_t(17+i%3);
        std::vector<uint64_t> radio((rtw_92du_phy_size()+7)/8);
        assert(rtw_92du_phy_init(&operating,&candidate,radio.data(),radio.size()*8,RTW_92DU_USB_HIGH)==RTW_92DU_OK);
        assert(rtw_92du_phy_load_tables(&operating)==RTW_92DU_OK);
        for(uint8_t channel:{uint8_t(149),uint8_t(157),uint8_t(165)}) {
            const uint32_t index=channel==149?17U:channel==157?18U:19U;
            assert(rtw_92du_phy_supports_channel(&operating,channel));
            assert(rtw_92du_phy_set_channel(&operating,channel)==RTW_92DU_OK);
            assert(Bus::word(f.bus.regs.data()+0xe00,4)==index*0x01010101U);
            assert(rtw_92du_phy_calibrate(&operating)==RTW_92DU_OK);
            assert(rtw_92du_phy_activate(&operating)==RTW_92DU_OK);
        }
    }
    for(int path=0;path<2;++path) {
        Fixture f;f.setup(36);f.bus.zero_curve_path=path;
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_POLL_TIMEOUT);
        assert(!rtw_92du_phy_radio_ready(&f.boot) && f.bus.regs[0x522]==0xff);
    }
    for(int fault=0;fault<6;++fault) {
        Fixture f;auto candidate=f.identity;auto pending=f.boot;pending.phy=nullptr;
        if(fault==0)candidate.logical[0x61]=0xff;
        if(fault==1)candidate.cut_word=0;
        if(fault==2)candidate.board.rf_paths=1;
        if(fault==3)candidate.internal_pa_5g[0]=1;
        if(fault==4)candidate.logical[0x10]=1;
        if(fault==5)candidate.logical[0xbe]=0xff;
        std::vector<uint64_t> next((rtw_92du_phy_size()+7)/8,0x55667788);
        assert(rtw_92du_phy_init(&pending,&candidate,next.data(),next.size()*8,RTW_92DU_USB_HIGH)==RTW_92DU_BAD_PROM);
        assert(!f.bus.io && next[0]==0x55667788);
    }
    for(uint8_t channel:{uint8_t(1),uint8_t(36),uint8_t(165)}) {
        Fixture full(false,0,false,RTW_92DU_USB_FULL);full.setup(channel);
        assert(rtw_92du_phy_calibrate(&full.boot)==RTW_92DU_OK);
    }
    for(auto cut:{RTW_92DU_CUT_C,RTW_92DU_CUT_E})for(uint8_t channel:{uint8_t(1),uint8_t(36),uint8_t(165)}) {
        Fixture f(false,0,false,RTW_92DU_USB_HIGH,cut);f.setup(channel);
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
    }
    size_t total_io=0,total_waits=0;
    for(bool internal:{false,true})for(uint8_t channel:{uint8_t(1),uint8_t(11),uint8_t(36),uint8_t(48),uint8_t(149),uint8_t(165)}) {
        Fixture f(false,0,internal);f.setup(channel);
        assert(!rtw_92du_phy_radio_ready(&f.boot) && f.bus.regs[0x522]==0xff);
        assert(Bus::word(f.bus.regs.data()+0xe00,4)==0x18181818);
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_radio_ready(&f.boot) && f.bus.regs[0x522]==0xff);
        bool found=false;for(const auto& w:f.bus.rf_writes)
            if(!w.path && w.address==0x3c && w.value==(internal?0xff455U:0xff454U))found=true;
        assert(found);
        const auto before=f.bus.io;
        assert(rtw_92du_phy_set_channel(&f.boot,144)==RTW_92DU_BAD_ARGUMENT && f.bus.io==before);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK && !f.bus.regs[0x522]);
        assert(rtw_92du_phy_stop(&f.boot)==RTW_92DU_OK && !rtw_92du_phy_radio_ready(&f.boot));
        assert(f.bus.rf[0][0]==0 && f.bus.regs[2]==0xe2 && f.bus.regs[0x11]==0x22);
        total_io=f.bus.io;total_waits=f.bus.waits;
    }
    {
        Fixture f;f.setup(1);assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK && rtw_92du_phy_tx_active(&f.boot));
        const auto before=f.bus.io;
        assert(rtw_92du_phy_set_scan_channel(&f.boot,11)==RTW_92DU_OK);
        assert(f.bus.io-before<600 && rtw_92du_phy_receive_ready(&f.boot));
        assert(!rtw_92du_phy_tx_active(&f.boot) && rtw_92du_phy_channel(&f.boot)==11);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
        const auto scan=f.bus.io;
        assert(rtw_92du_phy_set_scan_channel(&f.boot,36)==RTW_92DU_OK);
        assert(f.bus.io-scan<1100 && rtw_92du_phy_receive_ready(&f.boot));
        assert(!rtw_92du_phy_tx_active(&f.boot) && f.bus.regs[0x522]==0xff);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_WRONG_STATE);
        // AUTH leaves passive scan; only a freshly calibrated radio may TX.
        assert(rtw_92du_phy_set_channel(&f.boot,36)==RTW_92DU_OK);
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK && rtw_92du_phy_tx_active(&f.boot));
    }
    for(uint8_t index=0;index<2;++index) {
        Fixture f(true,index);f.setup(index?1:36);
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        const auto before=f.bus.io;
        assert(rtw_92du_phy_set_channel(&f.boot,index?36:1)==RTW_92DU_UNSUPPORTED_CHIP && f.bus.io==before);
    }
    for(uint8_t channel:{uint8_t(1),uint8_t(36),uint8_t(165)})for(uint8_t temperature:{uint8_t(1),uint8_t(18),uint8_t(31)}) {
        Fixture f;f.setup(channel);assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);f.bus.thermal=temperature;
        assert(rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_radio_ready(&f.boot) && !f.bus.regs[0x522]);
        // RF0x24 appears only in an upstream disabled debug expression.
        for(unsigned address:f.bus.rf_reads)assert(address!=0x24);
    }
    size_t thermal_io=0;
    {
        Fixture f;f.setup(36);assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK && rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK);
        const auto before=f.bus.io;f.bus.thermal=31;
        assert(rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK && rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK);
        thermal_io=f.bus.io-before;
        assert(thermal_io>2000 && rtw_92du_phy_radio_ready(&f.boot) && !f.bus.regs[0x522]);
    }
    for(size_t fail=1;fail<=thermal_io;++fail) {
        Fixture f;f.setup(36);assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK && rtw_92du_phy_tick(&f.boot)==RTW_92DU_OK);
        f.bus.thermal=31;f.bus.fail_at=f.bus.io+fail;
        auto result=rtw_92du_phy_tick(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_tick(&f.boot);
        assert(result==RTW_92DU_IO_ERROR && !rtw_92du_phy_radio_ready(&f.boot));
        assert(f.bus.io<=f.bus.fail_at+1 && f.bus.regs[0x522]==0xff);
    }
    for(int failure=0;failure<5;++failure) {
        Fixture f;f.setup(failure==4?36:1);
        if(failure==0)f.bus.lc_timeout=true;
        if(failure==1)f.bus.zero_curve=true;
        if(failure==2)f.bus.zero_iq=true;
        if(failure==3)f.bus.rx_iq_fail=true;
        if(failure==4)f.bus.iq_timeout=true;
        assert(rtw_92du_phy_calibrate(&f.boot)!=RTW_92DU_OK);
        assert(!rtw_92du_phy_radio_ready(&f.boot) && f.bus.regs[0x522]==0xff);
    }
    {
        Fixture f;f.setup(1);assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
        f.bus.rf[0][0]=0x30000;f.bus.rf_stuck=true;const auto before=f.bus.io;
        assert(rtw_92du_phy_stop(&f.boot)==RTW_92DU_POLL_TIMEOUT);
        assert(!rtw_92du_phy_radio_ready(&f.boot) && f.bus.regs[0x522]==0xff && f.bus.io<before+100);
    }
    for(bool short_io:{false,true})for(size_t fail=1;fail<=total_io;++fail) {
        Fixture f(false,0,true);
        if(short_io)f.bus.short_at=fail;else f.bus.fail_at=fail;
        auto result=rtw_92du_phy_load_tables(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_set_channel(&f.boot,165);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_calibrate(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_activate(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_stop(&f.boot);
        assert(result==RTW_92DU_IO_ERROR && !rtw_92du_phy_radio_ready(&f.boot));
        assert(f.bus.io<=fail+1 && f.bus.regs[0x522]==0xff);
        const auto before=f.bus.io;
        assert(rtw_92du_phy_set_channel(&f.boot,1)!=RTW_92DU_OK && f.bus.io==before);
    }
    for(size_t fail=1;fail<=total_waits;++fail) {
        Fixture f(false,0,true);f.bus.wait_fail_at=fail;
        auto result=rtw_92du_phy_load_tables(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_set_channel(&f.boot,165);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_calibrate(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_activate(&f.boot);
        if(result==RTW_92DU_OK)result=rtw_92du_phy_stop(&f.boot);
        assert(result==RTW_92DU_WAIT_ERROR && !rtw_92du_phy_radio_ready(&f.boot));
        assert(f.bus.waits==fail && f.bus.regs[0x522]==0xff);
    }
    std::printf("RTL8192DU PHY source tables/radio/LC/IQ/thermal: %zu full-path USB operations, %zu injected faults verified\n",total_io,total_io*2+total_waits+thermal_io);
}
