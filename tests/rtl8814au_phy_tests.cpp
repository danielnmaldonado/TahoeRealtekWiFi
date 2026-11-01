/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTL8814AUPHY.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <array>
#include <cerrno>
#include <memory>
#include "RTL8814AUUpper5GGolden.inc"
#include "RTL8814AUPowerConstraintGolden.inc"
struct write_event {uint16_t addr;unsigned width;uint32_t value;};
struct fake_bus {
    std::array<uint8_t,0x10000> memory{};std::vector<write_event> writes;
    unsigned reads=0,calls=0,fail_at=0;uint64_t delayed=0;bool calibration_busy=false;
    static int read(void*p,uint16_t a,unsigned w,uint32_t*v){auto&b=*(fake_bus*)p;++b.calls;++b.reads;if(b.fail_at&&b.calls==b.fail_at)return EIO;
        *v=0;for(unsigned i=0;i<w;i++)*v|=uint32_t(b.memory[a+i])<<(8*i);
        if(a==0x1b00&&w==4&&!b.calibration_busy)*v&=~1U;
        if(a==0x1b08&&w==4)*v=0;
        if(a==0x1bfc&&w==4)*v=0x00100010U;
        return 0;}
    static int write(void*p,uint16_t a,unsigned w,uint32_t v){auto&b=*(fake_bus*)p;++b.calls;if(b.fail_at&&b.calls==b.fail_at)return EIO;
        b.writes.push_back({a,w,v});for(unsigned i=0;i<w;i++)b.memory[a+i]=uint8_t(v>>(8*i));
        static const uint16_t sipi[4]={0xc90,0xe90,0x1890,0x1a90},base[4]={0x2800,0x2c00,0x3800,0x3c00};
        for(unsigned path=0;path<4;path++)if(a==sipi[path]&&w==4){uint16_t rf=uint16_t(base[path]+((v>>20)&255U)*4U);for(unsigned i=0;i<4;i++)b.memory[rf+i]=uint8_t((v&0xfffffU)>>(8*i));}
        return 0;}
    static void delay(void*p,uint32_t us){((fake_bus*)p)->delayed+=us;}
};
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct fixture {
    fake_bus bus;rtw_8814au ctx{};std::vector<uint64_t> storage;
    fixture(){ctx.bus={&bus,fake_bus::read,fake_bus::write,nullptr,fake_bus::delay};ctx.board_valid=1;ctx.identified=1;ctx.powered=1;ctx.fw_running=1;ctx.mac_initialized=1;
        ctx.board.mp_chip=1;ctx.board.cut=1;ctx.board.rfe_option=1;ctx.board.regd=1;ctx.board.crystal_cap=31;ctx.board.thermal_meter=31;ctx.board.physical_rf_paths=4;ctx.board.tx_streams=ctx.board.rx_streams=2;ctx.board.antenna_tx=ctx.board.antenna_rx=3;
        for(auto&path:ctx.board.txpwr_idx_table){for(unsigned i=0;i<11;i++)path[i]=24;path[11]=0;}
        storage.resize((rtw_8814au_phy_size()+7)/8);}
    int init(){return rtw_8814au_phy_init(&ctx,storage.data(),storage.size()*8);}
};
static uint32_t mem32(const fake_bus&b,uint16_t a){return uint32_t(b.memory[a])|(uint32_t(b.memory[a+1])<<8)|(uint32_t(b.memory[a+2])<<16)|(uint32_t(b.memory[a+3])<<24);}
static void put32(fake_bus&b,uint16_t a,uint32_t value){for(unsigned i=0;i<4;i++)b.memory[a+i]=uint8_t(value>>(8*i));}
static void spur_nbi_channels(){
    /* Independent expectations from 2440MHz interference and the source
     * 128-tone thresholds: channel4..8 -> tone indices19,11,4,3,9. */
    const unsigned expected[11]={0,0,0,19,11,4,3,9,0,0,0};
    for(unsigned rfe:{0U,1U}){
        auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
        CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        for(unsigned channel=1;channel<=11;channel++){
            CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(channel),20)==0);
            const uint32_t nbi=mem32(f->bus,0x87c);
            CHECK(bool(nbi&(1U<<13))==(channel>=4&&channel<=8));
            if(channel>=4&&channel<=8)CHECK(((nbi&0xfc000U)>>14)==expected[channel-1]);
        }
    }
}
static void channel153_spur_and_reset(){
    /* Pinned Linux rtw8814a_set_bw_mode -> spur_calibration ->
     * spur_nbi_setting: HT20 ch153 selects NBI field0x0f, then disables
     * bit13 for5G, enables CSI and sets mask7 bit16. Other channels reset.
     * Catch the missing153 branch and stale masks after leaving153. */
    for(unsigned rfe:{0U,1U})for(bool passive:{false,true}){
        auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
        f->bus.memory[0x522]=0x12;
        CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        put32(f->bus,0x87c,0xa5000001);put32(f->bus,0x874,0x5a000000);
        for(uint16_t reg:{0x880,0x884,0x898})put32(f->bus,reg,0xffffffff);
        put32(f->bus,0x89c,0x100);
        bool first=true;
        for(unsigned channel:{153U,149U,153U,157U}){
            CHECK((passive?rtw_8814au_phy_set_passive_5g_channel(&f->ctx,uint8_t(channel),20):
                rtw_8814au_phy_set_channel(&f->ctx,uint8_t(channel),20))==0);
            const bool spur=channel==153;
            CHECK((mem32(f->bus,0x87c)&0x000fe000U)==(spur?0x0001c000U:0x000fc000U));
            CHECK((mem32(f->bus,0x87c)&~0x000fe000U)==0xa5000001U);
            CHECK(mem32(f->bus,0x874)==(0x5a000000U|(spur?1U:0U)));
            for(uint16_t reg:{0x880,0x884,0x898})CHECK(mem32(f->bus,reg)==0);
            CHECK(mem32(f->bus,0x89c)==(spur?(first?0x10100U:0x10000U):0U));
            CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
            CHECK(rtw_8814au_phy_calibrate(&f->ctx)==(passive?EINVAL:0));
            CHECK(f->ctx.phy_initialized==!passive);
            CHECK(f->bus.memory[0x522]==(passive?0x3f:0x12));
            first=false;
        }
    }
}
static void independent_ht20_spur_registers(){
    /* Goldens execute unchanged pinned Linux set_bw_mode and its complete
     * spur chain. Catch wrong153 filtering and masks surviving153->149/157. */
    for(const auto&g:upper5g_spur_goldens)for(bool passive:{false,true}){
        auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(g.rfe);
        CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        for(uint16_t reg:{0x87c,0x874,0x880,0x884,0x898,0x89c})put32(f->bus,reg,0);
        put32(f->bus,0x82c,0xa1b2c3d4);put32(f->bus,0x830,0xaabbccdd);
        auto tune=[&](unsigned channel){return passive&&channel>14?
            rtw_8814au_phy_set_passive_5g_channel(&f->ctx,uint8_t(channel),20):
            rtw_8814au_phy_set_channel(&f->ctx,uint8_t(channel),20);};
        if(g.previous_channel)CHECK(tune(g.previous_channel)==0);
        CHECK(tune(g.channel)==0);
        if((mem32(f->bus,0x87c)&0x000fe000U)!=(g.nbi&0x000fe000U)){
            std::fprintf(stderr,"Linux HT20 spur rfe%u previous%u ch%u passive%u NBI=%#x expected=%#x\n",
                g.rfe,g.previous_channel,g.channel,unsigned(passive),mem32(f->bus,0x87c)&0x000fe000U,g.nbi&0x000fe000U);
            CHECK(false);
        }
        CHECK((mem32(f->bus,0x87c)&0x000fe000U)==(g.nbi&0x000fe000U));
        CHECK((mem32(f->bus,0x874)&1U)==(g.csi_setting1&1U));
        CHECK(mem32(f->bus,0x880)==g.csi_mask0);CHECK(mem32(f->bus,0x884)==g.csi_mask1);
        CHECK(mem32(f->bus,0x898)==g.csi_mask6);CHECK(mem32(f->bus,0x89c)==g.csi_mask7);
        CHECK(mem32(f->bus,0x82c)==g.cca);CHECK(mem32(f->bus,0x830)==g.pdmfth);
        CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
    }
    std::printf("8814AU HT20 spur: Linux differential64 rows, RFE0/1, DFS/140 entry/exit, passive/operating\n");
}
static void channel153_io_failure_pauses(){
    unsigned positions=0;
    for(unsigned rfe:{0U,1U})for(bool passive:{false,true}){
        auto reference=std::make_unique<fixture>();reference->ctx.board.rfe_option=uint8_t(rfe);
        reference->bus.memory[0x522]=0x12;
        CHECK(reference->init()==0);CHECK(rtw_8814au_phy_load_tables(&reference->ctx)==0);
        const unsigned before=reference->bus.calls;
        CHECK((passive?rtw_8814au_phy_set_passive_5g_channel(&reference->ctx,153,20):
            rtw_8814au_phy_set_channel(&reference->ctx,153,20))==0);
        const unsigned tune_operations=reference->bus.calls-before;
        for(unsigned cut=1;cut<=tune_operations;cut++){
            auto failed=std::make_unique<fixture>();failed->ctx.board.rfe_option=uint8_t(rfe);
            failed->bus.memory[0x522]=0x12;
            CHECK(failed->init()==0);CHECK(rtw_8814au_phy_load_tables(&failed->ctx)==0);
            failed->bus.writes.clear();failed->bus.fail_at=failed->bus.calls+cut;
            CHECK((passive?rtw_8814au_phy_set_passive_5g_channel(&failed->ctx,153,20):
                rtw_8814au_phy_set_channel(&failed->ctx,153,20))==EIO);
            CHECK(failed->ctx.io_error==EIO&&!failed->ctx.phy_initialized);
            CHECK(failed->bus.memory[0x522]==0x3f);
            for(const auto&w:failed->bus.writes)if(w.addr==0x522)CHECK(w.value==0x3f);
            const unsigned failed_calls=failed->bus.calls;
            CHECK(rtw_8814au_phy_calibrate(&failed->ctx)!=0);
            CHECK(rtw_8814au_phy_set_passive_5g_channel(&failed->ctx,149,20)==EIO);
            CHECK(failed->bus.calls==failed_calls);
        }
        positions+=tune_operations;
    }
    std::printf("8814AU channel153: %u tuning I/O failure positions (RFE0/1, passive/operating)\n",positions);
}
static void scan_calibration_reuse(){
    auto f=std::make_unique<fixture>();
    CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
    auto before=f->bus.writes.size();
    CHECK(rtw_8814au_phy_set_scan_channel(&f->ctx,1,20)==EAGAIN);
    CHECK(f->bus.writes.size()==before);
    CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,20)==0);
    CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
    const auto initial_delay=f->bus.delayed;
    auto saved_writes=f->bus.writes.size();
    CHECK(rtw_8814au_phy_set_scan_channel(&f->ctx,36,20)==EINVAL);
    CHECK(f->bus.writes.size()==saved_writes&&f->ctx.phy_initialized);
    for(unsigned channel=1;channel<=11;channel++){
        f->bus.writes.clear();
        CHECK(rtw_8814au_phy_set_scan_channel(&f->ctx,uint8_t(channel),20)==0);
        CHECK(f->ctx.phy_initialized&&f->bus.memory[0x522]==0);
        for(auto&w:f->bus.writes)CHECK(w.addr!=0x1b00);
    }
    CHECK(f->bus.delayed-initial_delay<1000000);
    CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);
    CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
    before=f->bus.writes.size();
    CHECK(rtw_8814au_phy_set_scan_channel(&f->ctx,11,20)==EAGAIN);
    CHECK(f->bus.writes.size()==before);
    CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);
    f->bus.fail_at=f->bus.calls+3;
    CHECK(rtw_8814au_phy_set_scan_channel(&f->ctx,11,20)==EIO);
    CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
}
static void receive_only_5g_rf_tuning(){
    for(unsigned rfe:{0U,1U}) {
        auto f=std::make_unique<fixture>();
        f->ctx.board.rfe_option=uint8_t(rfe);
        CHECK(f->init()==0);
        CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);
        CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
        CHECK(f->ctx.phy_initialized!=0);
        for(unsigned ch:{36U,40U,44U,48U}) {
            f->bus.writes.clear();
            CHECK(rtw_8814au_phy_set_passive_5g_channel(&f->ctx,uint8_t(ch),20)==0);
            CHECK(f->ctx.phy_initialized==0);
            CHECK(f->bus.memory[0x522]==0x3f);
            CHECK(f->bus.memory[0x454]==0x80);
            CHECK(mem32(f->bus,0xcb0)==(rfe?0x33173317U:0x54775477U));
            CHECK(mem32(f->bus,0xeb0)==(rfe?0x33173317U:0x54775477U));
            CHECK(mem32(f->bus,0x18b4)==(rfe?0x33173317U:0x54775477U));
            CHECK(mem32(f->bus,0x1ab4)==(rfe?0x77177717U:0x54775477U));
            CHECK((mem32(f->bus,0x958)&0x1fU)==1U);
            for(unsigned path=0;path<4;path++) {
                const uint16_t base[]={0x2800,0x2c00,0x3800,0x3c00};
                const uint32_t rf=mem32(f->bus,uint16_t(base[path]+0x18*4));
                CHECK((rf&0x703ffU)==(((0x101U<<8)|ch)&0x703ffU));
            }
            for(const auto&w:f->bus.writes){
                CHECK(w.addr!=0x1998); /* No 2G TX power/index register writes. */
                if(w.addr==0x522) CHECK(w.value==0x3f);
            }
            CHECK(rtw_8814au_phy_calibrate(&f->ctx)==EINVAL);
            CHECK(!f->ctx.phy_initialized && f->bus.memory[0x522]==0x3f);
        }
        const auto before=f->bus.writes.size();
        for(unsigned ch:{0U,1U,34U,50U,68U,96U,144U,145U,255U})
            CHECK(rtw_8814au_phy_set_passive_5g_channel(&f->ctx,uint8_t(ch),20)==EINVAL);
        CHECK(rtw_8814au_phy_set_passive_5g_channel(&f->ctx,36,40)==EINVAL);
        CHECK(rtw_8814au_phy_set_passive_5g_channel(&f->ctx,36,80)==EINVAL);
        CHECK(f->bus.writes.size()==before);
        /* Existing 2G operation can be restored only through calibration. */
        CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);
        CHECK(!f->ctx.phy_initialized);
        CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
        CHECK(f->ctx.phy_initialized && f->bus.memory[0x522]==0);
    }
    {
        auto f=std::make_unique<fixture>();
        CHECK(f->init()==0);
        CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);
        CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
        f->bus.fail_at=f->bus.calls+11;
        CHECK(rtw_8814au_phy_set_passive_5g_channel(&f->ctx,36,20)==EIO);
        CHECK(!f->ctx.phy_initialized && f->bus.memory[0x522]==0x3f);
    }
}

static void operating_5g_requires_own_power_and_calibration(){
    for(unsigned rfe:{0U,1U}) {
        auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
        // Linux packed rtw_txpwr_idx: 2G18 bytes,5G14 base indexes,
        // then signed OFDM delta in low nibble at offset32.
        for(auto&path:f->ctx.board.txpwr_idx_table){path[18]=22;path[19]=25;path[32]=0x0f;}
        CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        f->bus.writes.clear();
        for(unsigned ch:{36U,40U,44U,48U}) {
            CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(ch),20)==0);
            CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
            // RF18 must actually use the 5Gband field on all four paths.
            for(uint16_t base:{0x2800,0x2c00,0x3800,0x3c00})
                CHECK((mem32(f->bus,uint16_t(base+0x18*4))&0x703ffU)==((0x10100U|ch)&0x703ffU));
            CHECK((mem32(f->bus,0x958)&0x1f)==1);
            CHECK(rtw_8814au_phy_set_tx_rate(&f->ctx,0)==EINVAL);
            CHECK(rtw_8814au_phy_set_tx_rate(&f->ctx,4)==0);
            unsigned agc=0;
            for(const auto&w:f->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8){
                const unsigned rate=w.value&255;
                CHECK(rate>=4&&rate<28);CHECK(((w.value>>24)&63)<=63);
                if(((w.value>>8)&3U)==0&&rate==11)CHECK((w.value>>24)==(ch<=40?23U:26U));
                ++agc;
            }
            CHECK(agc>0);
            CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
            CHECK(f->ctx.phy_initialized&&f->bus.memory[0x522]==0);
            f->bus.writes.clear();
        }
        CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);
        CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
    }
}

static void ht20_power_indexes(){
    for(unsigned rfe:{0U,1U})for(unsigned band:{0U,1U}){
        auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
        for(auto&rom:f->ctx.board.txpwr_idx_table){
            rom[11]=0x20;rom[12]=0x0f;
            rom[18]=rom[19]=22;rom[32]=0x2f;rom[33]=0x0f;
        }
        CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
        f->bus.writes.clear();
        CHECK(rtw_8814au_phy_set_channel(&f->ctx,band?36:1,20)==0);
        unsigned mcs7=0,mcs15=0;
        for(const auto&w:f->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8){
            if((w.value&255)==19){CHECK((w.value>>24)==(band?26U:28U));++mcs7;}
            if((w.value&255)==27){CHECK((w.value>>24)==(band?25U:27U));++mcs15;}
        }
        CHECK(mcs7==4&&mcs15==4);
        CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
        CHECK(rtw_8814au_phy_set_tx_rate(&f->ctx,27)==0);
        CHECK(rtw_8814au_phy_set_tx_rate(&f->ctx,28)==EINVAL);
        put32(f->bus,0x2908,32U<<10);
        CHECK(rtw_8814au_phy_tick(&f->ctx)==0);
        CHECK(rtw_8814au_phy_tick(&f->ctx)==0);
    }
}

static void thermal_failure_pauses(){
    auto f=std::make_unique<fixture>();CHECK(f->init()==0);
    CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
    CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,20)==0);
    CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
    f->bus.fail_at=f->bus.calls+2;
    CHECK(rtw_8814au_phy_tick(&f->ctx)==EIO);
    CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
}
static void power_and_failure_paths(){
    for(unsigned rfe:{0U,1U}){auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);f->bus.writes.clear();CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,20)==0);
        bool cck=false,ofdm=false,last=false;for(auto&w:f->bus.writes)if(w.addr==0x1998){if((w.value&0x300U)==0){unsigned rate=w.value&255U;if(rate==0){CHECK((w.value>>24)==(rfe?24U:26U));cck=true;}if(rate==4){CHECK((w.value>>24)==(rfe?28U:30U));ofdm=true;}if(rate==11){CHECK((w.value>>24)==26U);last=true;}}}CHECK(cck&&ofdm&&last);
    }
    const uint16_t backup[]={0xa14,0x808,0x838,0x90c,0x810,0xcb0,0xeb0,0x18b4,0x1ab4,0x1abc,0x9a4,0x764,0xcbc,0x910};
    for(unsigned failure:{1U,5U,16U,20U,30U,50U,90U,150U,220U}){auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
        uint32_t saved[14];for(unsigned i=0;i<14;i++)saved[i]=mem32(f->bus,backup[i]);f->bus.fail_at=f->bus.calls+failure;
        CHECK(rtw_8814au_phy_calibrate(&f->ctx)==EIO);CHECK(f->ctx.io_error==EIO&&!f->ctx.phy_initialized);CHECK(f->bus.memory[0x522]==0x3f);
        for(unsigned i=0;i<14;i++)CHECK(mem32(f->bus,backup[i])==saved[i]);
        CHECK(rtw_8814au_phy_stop(&f->ctx)==EIO);CHECK(f->bus.memory[0x1f]==0&&f->bus.memory[0xec]==0&&f->bus.memory[0xee]==0&&f->bus.memory[0xef]==0);
    }
    {auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
        put32(f->bus,0x2908,31U<<10);CHECK(rtw_8814au_phy_set_tx_rate(&f->ctx,11)==0);CHECK(rtw_8814au_phy_set_tx_rate(&f->ctx,28)==EINVAL);
        CHECK(rtw_8814au_phy_tick(&f->ctx)==0);CHECK(rtw_8814au_phy_tick(&f->ctx)==0);CHECK(f->ctx.phy_initialized);
    }
}
static void upper_band_operating(){
 for(unsigned rfe:{0U,1U}){auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
  for(auto&rom:f->ctx.board.txpwr_idx_table){rom[28]=20;rom[29]=24;rom[30]=28;rom[32]=0x2f;rom[33]=0x0f;}
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  for(unsigned ch:{149U,153U,157U,161U,165U}){unsigned before=f->bus.calls;CHECK(rtw_8814au_phy_channel_supported(&f->ctx,uint8_t(ch),20));CHECK(f->bus.calls==before);
   CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(ch),20)==0);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
   CHECK(((mem32(f->bus,0x860)&0x1ffe0000U)>>17)==0x412);CHECK((mem32(f->bus,0x958)&31)==3);
   for(uint16_t base:{0x2800,0x2c00,0x3800,0x3c00})CHECK((mem32(f->bus,uint16_t(base+0x18*4))&0x703ffU)==((0x50100U|ch)&0x703ffU));
   CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);
  }
 }
}
static void independent_upper5g_power_frequency(){
 for(const auto&g:upper5g_goldens){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(g.rfe);f->ctx.board.regd=uint8_t(g.domain);
  for(unsigned path=0;path<4;path++){auto&rom=f->ctx.board.txpwr_idx_table[path];for(unsigned group=0;group<14;group++)rom[18+group]=uint8_t(12+group+path);rom[32]=0x2f;rom[33]=0x0f;}
  // Query is valid during attach, before power/FW/MAC, and is purely local.
  f->ctx.powered=f->ctx.mac_initialized=f->ctx.fw_running=0;
  CHECK(rtw_8814au_phy_channel_supported(&f->ctx,uint8_t(g.channel),20));CHECK(f->bus.calls==0);
  f->ctx.powered=f->ctx.mac_initialized=f->ctx.fw_running=1;
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);f->bus.writes.clear();
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20)==0);
  CHECK(((mem32(f->bus,0x860)&0x1ffe0000U)>>17)==g.fc);
  CHECK((mem32(f->bus,0x958)&31)==g.agc);
  for(unsigned path=0;path<4;path++)CHECK((mem32(f->bus,uint16_t((uint16_t[]){0x2800,0x2c00,0x3800,0x3c00}[path]+0x18*4))&0x703ff)==(g.rf[path]&0x703ff));
  bool seen[96]{};for(const auto&w:f->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8){
   const unsigned rate=w.value&255,path=(w.value>>8)&3;CHECK(rate>=4&&rate<28);
   const unsigned index=path*24+rate-4;CHECK(!seen[index]);seen[index]=true;
   if((w.value>>24)!=g.power[index]){std::fprintf(stderr,"golden rfe%u domain%u ch%u path%u rate%u actual%u expected%u\n",g.rfe,g.domain,g.channel,path,rate,w.value>>24,g.power[index]);CHECK(false);}
  }
  for(bool entry:seen)CHECK(entry);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
  CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);
 }
}
static void upper5g_bad_factory_no_io(){
 for(unsigned rfe:{0U,1U}){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  const unsigned before=f->bus.calls;
  // Group numbers come from the unchanged compiled primary function.
  for(const auto&g:upper5g_goldens)if(g.rfe==rfe&&g.domain==0)
   for(unsigned path=0;path<4;path++)for(unsigned invalid:{64U,255U}){
    auto&value=f->ctx.board.txpwr_idx_table[path][18+g.group];uint8_t original=value;value=uint8_t(invalid);
    CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,uint8_t(g.channel),20));
    CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20)==ENOTSUP);
    CHECK(rtw_8814au_phy_channel_supported(&f->ctx,uint8_t(g.group==0?149:36),20));
    CHECK(f->bus.calls==before);value=original;
   }
  for(unsigned ch:{0U,12U,14U,50U,68U,96U,144U,145U,169U,255U}){
   CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,uint8_t(ch),20));CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(ch),20)==EINVAL);CHECK(f->bus.calls==before);
  }
  for(unsigned bw:{0U,10U,40U,80U,160U}){CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,bw));CHECK(rtw_8814au_phy_set_channel(&f->ctx,100,bw)==EINVAL);CHECK(f->bus.calls==before);}
  auto saved=f->ctx.board;f->ctx.board.txpwr_idx_table[3][0]=255;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));f->ctx.board=saved;
  f->ctx.board.rfe_option=2;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));f->ctx.board=saved;
  f->ctx.board.physical_rf_paths=2;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));f->ctx.board=saved;
  f->ctx.board.cut=7;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));f->ctx.board=saved;
  f->ctx.board.tx_streams=3;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));f->ctx.board=saved;
  f->ctx.board.rx_streams=0;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));f->ctx.board=saved;
  f->ctx.board_valid=0;CHECK(!rtw_8814au_phy_channel_supported(&f->ctx,100,20));CHECK(!rtw_8814au_phy_channel_supported(nullptr,100,20));CHECK(f->bus.calls==before);
 }
}
static void upper5g_actual_thermal_tables(){
 static const uint16_t txagc[]={0xc94,0xe94,0x1894,0x1a94},scale[]={0xc1c,0xe1c,0x181c,0x1a1c};
 for(const auto&g:upper5g_thermal_goldens){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(g.rfe);CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  for(unsigned sign=0;sign<2;sign++)for(unsigned delta=0;delta<30;delta++){
   CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
   CHECK(rtw_8814au_phy_tick(&f->ctx)==0);const unsigned thermal=sign?31+delta:31-delta;put32(f->bus,0x2908,thermal<<10);
   CHECK(rtw_8814au_phy_tick(&f->ctx)==0&&f->ctx.phy_initialized);
   for(unsigned path=0;path<4;path++){
    const unsigned adjustment=g.value[(sign*4+path)*30+delta];
    const unsigned agc=sign?(adjustment>15?15:adjustment):0;
    const unsigned index=sign?(24+(adjustment>15?adjustment-15:0)):(adjustment<24?24-adjustment:0);
    CHECK(((mem32(f->bus,txagc[path])>>25)&31)==agc);
    CHECK(((mem32(f->bus,scale[path])>>21)&0x7ff)==upper5g_swing_scale[index]);
   }
   f->bus.writes.clear();
  }
 }
}
static void upper5g_transition_and_failure(){
 auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
 for(unsigned ch:{1U,52U,64U,100U,140U,36U,153U,140U,1U,149U,11U,165U,44U,157U,6U}){
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(ch),20)==0);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
  CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);CHECK(f->bus.memory[0x522]==0);
  CHECK((mem32(f->bus,0x2860)&255)==ch);CHECK((mem32(f->bus,0x958)&31)==(ch>=149?3:ch>=100?2:ch>=36?1:0));
 }
 CHECK(rtw_8814au_phy_set_channel(&f->ctx,165,20)==0);f->bus.calibration_busy=true;CHECK(rtw_8814au_phy_calibrate(&f->ctx)==ETIMEDOUT);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
 CHECK(rtw_8814au_phy_tick(&f->ctx)==EINVAL);f->bus.calibration_busy=false;
 CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);CHECK(rtw_8814au_phy_stop(&f->ctx)==0);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
 auto reference=std::make_unique<fixture>();CHECK(reference->init()==0);CHECK(rtw_8814au_phy_load_tables(&reference->ctx)==0);unsigned before=reference->bus.calls;
 CHECK(rtw_8814au_phy_set_channel(&reference->ctx,149,20)==0);const unsigned tune_operations=reference->bus.calls-before;
 for(unsigned cut=1;cut<=tune_operations;cut++){
  auto failed=std::make_unique<fixture>();CHECK(failed->init()==0);CHECK(rtw_8814au_phy_load_tables(&failed->ctx)==0);failed->bus.fail_at=failed->bus.calls+cut;
  CHECK(rtw_8814au_phy_set_channel(&failed->ctx,149,20)==EIO);CHECK(!failed->ctx.phy_initialized&&failed->bus.memory[0x522]==0x3f);
  CHECK(rtw_8814au_phy_calibrate(&failed->ctx)!=0);CHECK(!failed->ctx.phy_initialized);
 }
 std::printf("8814AU upper5G: Linux differential192 power/channel rows,all thermal deltas, %u tuning I/O failure positions\n",tune_operations);
}
// Failed DFS entry or channel140 exit must never revive old calibration or TX.
static void dfs_tune_failure_and_calibration(){
 struct hop {unsigned previous,channel;};
 const hop hops[]={{0,52},{0,100},{0,140},{140,36},{140,100},{140,153},{140,1}};
 unsigned positions=0;
 for(unsigned rfe:{0U,1U})for(bool passive:{false,true})for(auto h:hops){
  auto tune=[&](fixture&f,unsigned channel){return passive&&channel>14?
   rtw_8814au_phy_set_passive_5g_channel(&f.ctx,uint8_t(channel),20):
   rtw_8814au_phy_set_channel(&f.ctx,uint8_t(channel),20);};
  auto prepare=[&](fixture&f){f.ctx.board.rfe_option=uint8_t(rfe);f.bus.memory[0x522]=0x12;
   CHECK(f.init()==0);CHECK(rtw_8814au_phy_load_tables(&f.ctx)==0);
   if(h.previous)CHECK(tune(f,h.previous)==0);};
  auto reference=std::make_unique<fixture>();prepare(*reference);
  const unsigned before=reference->bus.calls;CHECK(tune(*reference,h.channel)==0);
  const unsigned operations=reference->bus.calls-before;positions+=operations;
  CHECK(!reference->ctx.phy_initialized&&reference->bus.memory[0x522]==0x3f);
  if(!passive||h.channel<14){
   reference->bus.calibration_busy=true;CHECK(rtw_8814au_phy_calibrate(&reference->ctx)==ETIMEDOUT);
   CHECK(!reference->ctx.phy_initialized&&reference->bus.memory[0x522]==0x3f);
   CHECK(rtw_8814au_phy_tick(&reference->ctx)==EINVAL);
   reference->bus.calibration_busy=false;CHECK(rtw_8814au_phy_calibrate(&reference->ctx)==0);
   CHECK(reference->ctx.phy_initialized&&reference->bus.memory[0x522]==0x12);
  }else CHECK(rtw_8814au_phy_calibrate(&reference->ctx)==EINVAL);
  for(unsigned cut=1;cut<=operations;cut++){
   auto failed=std::make_unique<fixture>();prepare(*failed);failed->bus.writes.clear();failed->bus.fail_at=failed->bus.calls+cut;
   CHECK(tune(*failed,h.channel)==EIO);CHECK(failed->ctx.io_error==EIO&&!failed->ctx.phy_initialized);
   CHECK(failed->bus.memory[0x522]==0x3f);
   for(const auto&w:failed->bus.writes)if(w.addr==0x522)CHECK(w.value==0x3f);
   const unsigned failed_calls=failed->bus.calls;CHECK(rtw_8814au_phy_calibrate(&failed->ctx)!=0);
   CHECK(rtw_8814au_phy_set_passive_5g_channel(&failed->ctx,100,20)==EIO);
   CHECK(failed->bus.calls==failed_calls);
  }
 }
 std::printf("8814AU DFS/140: %u tuning I/O failure positions, calibration timeout/recovery, RFE0/1, passive/operating\n",positions);
}
static void ap_power_constraint_exact_drop(){
 // Every expected baseline is produced by unchanged pinned Linux functions.
 for(const auto&g:upper5g_goldens)for(unsigned db:{3U,6U}){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(g.rfe);f->ctx.board.regd=uint8_t(g.domain);
  for(unsigned path=0;path<4;path++){auto&rom=f->ctx.board.txpwr_idx_table[path];for(unsigned group=0;group<14;group++)rom[18+group]=uint8_t(32+group+path);rom[32]=0x2f;rom[33]=0x0f;}
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  unsigned before=f->bus.calls;CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,uint8_t(db))==0);CHECK(f->bus.calls==before);
  f->bus.writes.clear();CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20)==0);
  bool seen[96]{};for(const auto&w:f->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8){
   const unsigned rate=w.value&255,path=(w.value>>8)&3;CHECK(rate>=4&&rate<28);
   const unsigned index=path*24+rate-4;CHECK(!seen[index]);seen[index]=true;
   CHECK((w.value>>24)==g.power[index]+20U-power_constraint_txgi[db]);
  }
  for(bool entry:seen)CHECK(entry);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
  CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);
  before=f->bus.calls;CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,255)==EINVAL);CHECK(f->ctx.phy_initialized&&f->bus.calls==before);
  CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,0)==0);CHECK(!f->ctx.phy_initialized&&f->bus.calls==before);
  CHECK(rtw_8814au_phy_calibrate(&f->ctx)==EINVAL);
  f->bus.writes.clear();CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20)==0);
  for(const auto&w:f->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8){
   const unsigned rate=w.value&255,path=(w.value>>8)&3;CHECK((w.value>>24)==g.power[path*24+rate-4]+20U);
  }
 }
 std::printf("8814AU AP constraints: exact3/6dB on all96 indexes,192 primary RFE/domain/channel rows, zero-reset\n");
}
static void ap_power_constraint_underflow_and_saturation(){
 CHECK(rtw_8814au_phy_set_power_constraint(nullptr,0)==EINVAL);
 rtw_8814au detached{};CHECK(rtw_8814au_phy_set_power_constraint(&detached,0)==EINVAL);
 for(unsigned rfe:{0U,1U})for(unsigned db:{3U,6U})for(unsigned path=0;path<4;path++){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);
  for(auto&rom:f->ctx.board.txpwr_idx_table){for(unsigned group=0;group<14;group++)rom[18+group]=40;rom[32]=rom[33]=0;}
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,149,20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
  const auto hardware=f->bus.memory;const unsigned before=f->bus.calls;
  CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,uint8_t(db))==0);
  f->ctx.board.txpwr_idx_table[path][22]=0; // Primary group4, channel100.
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,100,20)==ERANGE);
  CHECK(f->bus.calls==before&&f->bus.memory==hardware&&!f->ctx.phy_initialized);
  CHECK(rtw_8814au_phy_calibrate(&f->ctx)==EINVAL);CHECK(rtw_8814au_phy_tick(&f->ctx)==EINVAL);
  CHECK(f->bus.calls==before);CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,32)==EINVAL);
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,100,20)==ERANGE);
  CHECK(f->bus.calls==before);CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,0)==0);
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,100,20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);
  CHECK(rtw_8814au_phy_stop(&f->ctx)==0);CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,0)==0);
 }
 for(unsigned rfe:{0U,1U})for(unsigned db:{3U,6U}){
  std::array<std::unique_ptr<fixture>,2> f;std::array<std::array<unsigned,96>,2> powers{};
  for(unsigned n=0;n<2;n++){
   f[n]=std::make_unique<fixture>();f[n]->ctx.board.rfe_option=uint8_t(rfe);
   for(auto&rom:f[n]->ctx.board.txpwr_idx_table){for(unsigned group=0;group<14;group++)rom[18+group]=63;rom[32]=rom[33]=0;}
   CHECK(f[n]->init()==0);CHECK(rtw_8814au_phy_load_tables(&f[n]->ctx)==0);
   CHECK(rtw_8814au_phy_set_power_constraint(&f[n]->ctx,uint8_t(n?db:0))==0);f[n]->bus.writes.clear();
   CHECK(rtw_8814au_phy_set_channel(&f[n]->ctx,100,20)==0);
   for(const auto&w:f[n]->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8)
    powers[n][((w.value>>8)&3)*24+(w.value&255)-4]=w.value>>24;
  }
  unsigned saturated=0;for(unsigned i=0;i<96;i++){saturated+=powers[0][i]==63;CHECK(powers[0][i]-powers[1][i]==power_constraint_txgi[db]);}
  CHECK(saturated>0);
 }
 std::printf("8814AU AP constraints: all-path underflow atomic/no-I/O; saturated+2 encoding preserves exact drop\n");
}
static void ap_power_constraint_thermal_bound(){
 static const uint16_t txagc[]={0xc94,0xe94,0x1894,0x1a94},scale[]={0xc1c,0xe1c,0x181c,0x1a1c};
 unsigned samples=0;
 for(bool absolute:{false,true})for(unsigned rfe:{0U,1U})for(unsigned channel:{52U,100U,140U,149U}){
  std::array<std::unique_ptr<fixture>,3> f;
  for(unsigned n=0;n<3;n++){
   f[n]=std::make_unique<fixture>();f[n]->ctx.board.rfe_option=uint8_t(rfe);
   for(auto&rom:f[n]->ctx.board.txpwr_idx_table){for(unsigned group=0;group<14;group++)rom[18+group]=63;rom[32]=rom[33]=0;}
   CHECK(f[n]->init()==0);CHECK(rtw_8814au_phy_load_tables(&f[n]->ctx)==0);
   if(absolute&&n)CHECK(rtw_8814au_phy_set_power_limit_dbm(&f[n]->ctx,int8_t(20-n*3))==0);
   else CHECK(rtw_8814au_phy_set_power_constraint(&f[n]->ctx,uint8_t(n*3))==0);
  }
  for(unsigned rate=4;rate<28;rate++)for(unsigned sign=0;sign<2;sign++)for(unsigned delta=0;delta<30;delta++){
   for(unsigned n=0;n<3;n++){
    CHECK(rtw_8814au_phy_set_channel(&f[n]->ctx,uint8_t(channel),20)==0);
    CHECK(rtw_8814au_phy_calibrate(&f[n]->ctx)==0);CHECK(rtw_8814au_phy_set_tx_rate(&f[n]->ctx,uint8_t(rate))==0);
    CHECK(rtw_8814au_phy_tick(&f[n]->ctx)==0);put32(f[n]->bus,0x2908,(sign?31+delta:31-delta)<<10);
    CHECK(rtw_8814au_phy_tick(&f[n]->ctx)==0&&f[n]->ctx.phy_initialized);
   }
   // The same primary correction keeps every per-rate index reduction intact;
   // lowering base must never buy back extra positive AGC or BB swing.
   for(unsigned n=1;n<3;n++)for(unsigned path=0;path<4;path++){
    CHECK((mem32(f[n]->bus,txagc[path])&0x3e000000)==(mem32(f[0]->bus,txagc[path])&0x3e000000));
    CHECK((mem32(f[n]->bus,scale[path])&0xffe00000)==(mem32(f[0]->bus,scale[path])&0xffe00000));
   }
   for(auto&entry:f)entry->bus.writes.clear();++samples;
  }
 }
 std::printf("8814AU AP constraints: %u thermal samples, all24 OFDM/HT rates, both RFEs, lower/mid/upper bands, relative and absolute caps\n",samples);
}
static void ap_constraint_preserves_per_path_factory_swing(){
 const uint16_t agc[]={0xc94,0xe94,0x1894,0x1a94},scale[]={0xc1c,0xe1c,0x181c,0x1a1c};
 // Primary get_bb_swing decodes two bits per physical path.
 const unsigned swing[]={0x200,0x16a,0x101,0x0b6};
 for(bool absolute:{false,true})for(unsigned rfe:{0U,1U})for(unsigned db:{3U,6U}){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(rfe);f->ctx.board.tx_bb_swing_setting_5g=0xe4;
  for(auto&rom:f->ctx.board.txpwr_idx_table){for(unsigned group=0;group<14;group++)rom[18+group]=40;rom[32]=rom[33]=0;}
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  for(auto address:agc)put32(f->bus,address,15U<<25); // Stale prior-channel positive compensation.
  if(absolute)CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,int8_t(20-db))==0);
  else CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,uint8_t(db))==0);
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,100,20)==0);
  for(unsigned path=0;path<4;path++){
   CHECK(((mem32(f->bus,scale[path])>>21)&0x7ff)==swing[path]);
   CHECK((mem32(f->bus,agc[path])&0x3e000000)==0);
  }
 }
}
static void ap_constraint_io_failures_remain_paused(){
 unsigned positions=0;
 for(bool absolute:{false,true})for(unsigned rfe:{0U,1U})for(unsigned db:{3U,6U}){
  const auto prepare=[&](fixture&f){
   f.ctx.board.rfe_option=uint8_t(rfe);
   for(auto&rom:f.ctx.board.txpwr_idx_table){for(unsigned group=0;group<14;group++)rom[18+group]=40;rom[32]=rom[33]=0;}
   CHECK(f.init()==0);CHECK(rtw_8814au_phy_load_tables(&f.ctx)==0);
   if(absolute)CHECK(rtw_8814au_phy_set_power_limit_dbm(&f.ctx,int8_t(20-db))==0);
   else CHECK(rtw_8814au_phy_set_power_constraint(&f.ctx,uint8_t(db))==0);
  };
  auto reference=std::make_unique<fixture>();prepare(*reference);
  const unsigned before=reference->bus.calls;CHECK(rtw_8814au_phy_set_channel(&reference->ctx,100,20)==0);
  const unsigned operations=reference->bus.calls-before;positions+=operations;
  for(unsigned cut=1;cut<=operations;cut++){
   auto failed=std::make_unique<fixture>();prepare(*failed);failed->bus.writes.clear();failed->bus.fail_at=failed->bus.calls+cut;
   CHECK(rtw_8814au_phy_set_channel(&failed->ctx,100,20)==EIO);
   CHECK(failed->ctx.io_error==EIO&&!failed->ctx.phy_initialized&&failed->bus.memory[0x522]==0x3f);
   for(const auto&w:failed->bus.writes)if(w.addr==0x522)CHECK(w.value==0x3f);
   const unsigned calls=failed->bus.calls;
   CHECK(rtw_8814au_phy_set_power_constraint(&failed->ctx,0)==EIO);
   CHECK(rtw_8814au_phy_set_power_limit_dbm(&failed->ctx,RTW_8814AU_POWER_LIMIT_DISABLED)==EIO);
   CHECK(rtw_8814au_phy_calibrate(&failed->ctx)!=0&&failed->bus.calls==calls);
  }
 }
 std::printf("8814AU AP constraints: %u constrained-tuning I/O failure positions remain paused\n",positions);
}
static void linux_sar_metadata_contract(){
 auto f=std::make_unique<fixture>();CHECK(f->init()==0);const unsigned before=f->bus.calls;
 CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,20)==0);CHECK(f->bus.calls==before);
 CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,-3)==0);CHECK(f->bus.calls==before);
 CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,-33)==EINVAL);CHECK(f->bus.calls==before);
 CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,RTW_8814AU_POWER_LIMIT_DISABLED)==0);
}
static void relative_constraint_respects_primary_encoded_floor(){
 bool found=false;
 for(const auto&g:upper5g_goldens){
  bool minimum=false;for(unsigned value:g.power)minimum|=value==11;
  if(!minimum)continue;found=true;
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(g.rfe);f->ctx.board.regd=uint8_t(g.domain);
  for(unsigned path=0;path<4;path++){auto&rom=f->ctx.board.txpwr_idx_table[path];for(unsigned group=0;group<14;group++)rom[18+group]=uint8_t(7+group+path);rom[32]=0x2f;rom[33]=0x0f;}
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,3)==0);const unsigned before=f->bus.calls;
  // Baseline minimum11-5=6; dropping6 would cross the primary index0+2 floor.
  CHECK(rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20)==ERANGE);CHECK(f->bus.calls==before);
 }
 CHECK(found);
}
static void linux_sar_primary_absolute_limits(){
 unsigned accepted=0,rejected=0;
 for(const auto&g:absolute_power_goldens){
  auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=uint8_t(g.rfe);f->ctx.board.regd=uint8_t(g.domain);
  for(auto&rom:f->ctx.board.txpwr_idx_table){for(unsigned group=0;group<14;group++)rom[18+group]=63;rom[32]=rom[33]=0;}
  CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
  unsigned before=f->bus.calls;CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,int8_t(g.ceiling))==0);
  CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,uint8_t(g.constraint))==0);CHECK(f->bus.calls==before);
  f->bus.writes.clear();int error=rtw_8814au_phy_set_channel(&f->ctx,uint8_t(g.channel),20);
  if(!g.supported){++rejected;CHECK(error==ERANGE&&f->bus.calls==before&&!f->ctx.phy_initialized);continue;}
  ++accepted;CHECK(error==0);bool seen[96]{};
  for(const auto&w:f->bus.writes)if(w.addr==0x1998&&w.width==4&&((w.value>>20)&15)==8){
   const unsigned rate=w.value&255,path=(w.value>>8)&3,index=path*24+rate-4;CHECK(rate>=4&&rate<28);CHECK(!seen[index]);seen[index]=true;
   if((w.value>>24)!=g.power[index]){std::fprintf(stderr,"SAR rfe%u domain%u ch%u ceiling%d AP%u path%u rate%u got%u expected%u\n",g.rfe,g.domain,g.channel,g.ceiling,g.constraint,path,rate,w.value>>24,g.power[index]);CHECK(false);}
  }
  for(bool entry:seen)CHECK(entry);CHECK(!f->ctx.phy_initialized&&f->bus.memory[0x522]==0x3f);
  CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0&&f->ctx.phy_initialized);
 }
 CHECK(accepted&&rejected);
 auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
 CHECK(rtw_8814au_phy_set_power_limit_dbm(&f->ctx,-32)==0);CHECK(rtw_8814au_phy_set_power_constraint(&f->ctx,3)==0);
 const unsigned before=f->bus.calls;CHECK(rtw_8814au_phy_set_channel(&f->ctx,100,20)==ERANGE);CHECK(f->bus.calls==before);
 std::printf("8814AU absolute SAR: %u primary rows accepted,%u underflow rows rejected atomically, all24 centers/RFE0/1/domains/negative caps\n",accepted,rejected);
}
int main(){
    relative_constraint_respects_primary_encoded_floor();
    linux_sar_primary_absolute_limits();
    linux_sar_metadata_contract();
    ap_constraint_io_failures_remain_paused();
    ap_constraint_preserves_per_path_factory_swing();
    ap_power_constraint_underflow_and_saturation();
    ap_power_constraint_thermal_bound();
    ap_power_constraint_exact_drop();
    dfs_tune_failure_and_calibration();
    independent_ht20_spur_registers();
    channel153_spur_and_reset();
    channel153_io_failure_pauses();
    independent_upper5g_power_frequency();
    upper5g_bad_factory_no_io();
    upper5g_actual_thermal_tables();
    upper5g_transition_and_failure();
    upper_band_operating();
    operating_5g_requires_own_power_and_calibration();
    receive_only_5g_rf_tuning();
    spur_nbi_channels();
    ht20_power_indexes();
    scan_calibration_reuse();
    thermal_failure_pauses();
    power_and_failure_paths();
    {auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(f->ctx.phy!=nullptr&&!f->ctx.phy_initialized);CHECK(f->bus.writes.empty());
     CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,20)!=0);CHECK(f->bus.writes.empty());}
    {auto f=std::make_unique<fixture>();f->ctx.board.rfe_option=7;CHECK(f->init()==ENOTSUP);CHECK(f->bus.writes.empty());}
    {auto f=std::make_unique<fixture>();CHECK(rtw_8814au_phy_init(&f->ctx,f->storage.data(),1)==EINVAL);CHECK(f->bus.writes.empty());}
    {auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);
     CHECK(f->bus.writes.size()>3000);bool paths[4]{};for(auto&w:f->bus.writes)for(unsigned p=0;p<4;p++)if(w.addr==(uint16_t[]){0xc90,0xe90,0x1890,0x1a90}[p]|| (w.addr>=((uint16_t[]){0x2800,0x2c00,0x3800,0x3c00}[p])&&w.addr<((uint16_t[]){0x2800,0x2c00,0x3800,0x3c00}[p])+0x400))paths[p]=true;
     for(bool p:paths)CHECK(p);CHECK(!f->ctx.phy_initialized);
     auto before=f->bus.writes.size();CHECK(rtw_8814au_phy_set_channel(&f->ctx,169,20)==EINVAL);CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,40)==EINVAL);CHECK(f->bus.writes.size()==before);
     CHECK(rtw_8814au_phy_set_channel(&f->ctx,1,20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);CHECK(f->ctx.phy_initialized);CHECK(f->bus.memory[0x522]==0);
     CHECK(rtw_8814au_phy_set_channel(&f->ctx,11,20)==0);CHECK(rtw_8814au_phy_calibrate(&f->ctx)==0);CHECK(rtw_8814au_phy_tick(&f->ctx)==0);
     CHECK(rtw_8814au_phy_stop(&f->ctx)==0);CHECK(!f->ctx.phy_initialized);}
    {auto f=std::make_unique<fixture>();CHECK(f->init()==0);CHECK(rtw_8814au_phy_load_tables(&f->ctx)==0);CHECK(rtw_8814au_phy_set_channel(&f->ctx,6,20)==0);
     f->bus.calibration_busy=true;CHECK(rtw_8814au_phy_calibrate(&f->ctx)==ETIMEDOUT);CHECK(!f->ctx.phy_initialized);CHECK(f->bus.delayed<5000000);}
    std::printf("8814AU PHY: %u checks passed (actual table/channel/RFK functions; register bus boundary only)\n",checks);
}
