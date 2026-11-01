/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtwn_ht_tx_host.hpp"
#include <memory>

extern "C" unsigned rtw_test_8814_thermal_rate(const rtw_8814au *);
extern "C" int rtw_test_8814_thermal_delta(const rtw_8814au *, unsigned);

static unsigned checks, failures;
#define REQUIRE(x) do { if(!(x)) { std::fprintf(stderr,"SETUP FAIL %d: %s\n",__LINE__,#x);std::exit(2); } } while(0)
#define EXPECT(x) do { ++checks;if(!(x)) { ++failures;std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); } } while(0)

/* Same register/RF boundary as rtl8814au_phy_tests.cpp. The controller and
 * portable PHY are compiled verbatim; nothing reaches USB, RF or credentials. */
struct thermal_bus {
    std::array<uint8_t,0x10000> memory{};
    unsigned writes=0;
    static int read(void *cookie,uint16_t address,unsigned width,uint32_t *value) {
        auto &bus=*static_cast<thermal_bus *>(cookie);*value=0;
        for(unsigned i=0;i<width;++i)*value|=uint32_t(bus.memory[address+i])<<(8*i);
        if(address==0x1b00&&width==4)*value&=~1U;
        if(address==0x1b08&&width==4)*value=0;
        if(address==0x1bfc&&width==4)*value=0x00100010U;
        return 0;
    }
    static int write(void *cookie,uint16_t address,unsigned width,uint32_t value) {
        auto &bus=*static_cast<thermal_bus *>(cookie);++bus.writes;
        for(unsigned i=0;i<width;++i)bus.memory[address+i]=uint8_t(value>>(8*i));
        const uint16_t sipi[4]={0xc90,0xe90,0x1890,0x1a90};
        const uint16_t base[4]={0x2800,0x2c00,0x3800,0x3c00};
        for(unsigned path=0;path<4;++path)if(address==sipi[path]&&width==4) {
            const auto rf=uint16_t(base[path]+((value>>20)&255U)*4U);
            for(unsigned i=0;i<4;++i)bus.memory[rf+i]=uint8_t((value&0xfffffU)>>(8*i));
        }
        return 0;
    }
    static void delay(void *,uint32_t) {}
};

struct rate_fixture {
    RTWUSBController controller;
    ieee80211_node node;
    thermal_bus bus;
    std::vector<uint64_t> phyStorage;
    rate_fixture() {
        effects={};auto &c=controller;auto &ctx=c.acCore_;
        c.family_=RTW_WIFI_RTL8814AU;c.core_.sc_ic.ic_bss=&node;
        const uint8_t rates[]={0x82,0x84,0x8b,0x96,12,18,24,36,48,72,96,108};
        node.ni_rates.rs_nrates=sizeof(rates);
        std::memcpy(node.ni_rates.rs_rates,rates,sizeof(rates));node.ni_txrate=11;
        ctx.bus={&bus,thermal_bus::read,thermal_bus::write,nullptr,thermal_bus::delay};
        ctx.board_valid=ctx.identified=ctx.powered=ctx.fw_running=ctx.mac_initialized=1;
        ctx.board.mp_chip=1;ctx.board.cut=1;ctx.board.rfe_option=1;ctx.board.regd=1;
        ctx.board.crystal_cap=31;ctx.board.thermal_meter=31;ctx.board.physical_rf_paths=4;
        ctx.board.tx_streams=ctx.board.rx_streams=2;ctx.board.antenna_tx=ctx.board.antenna_rx=3;
        for(auto &path:ctx.board.txpwr_idx_table)for(unsigned i=0;i<11;++i)path[i]=24;
        phyStorage.resize((rtw_8814au_phy_size()+7)/8);resetPHY();
        const uint8_t mcs[2]={0xff,0xff};
        REQUIRE(rtw_8814au_build_rate_config(0xfff,mcs,2,1,1,1,&c.acRateConfig_));
        REQUIRE(c.acRateConfig_.initial_rate==27);c.acRateReady_=c.acMediaConnected_=true;
        REQUIRE(c.uses8814FirmwareRate());
    }
    void resetPHY() {
        auto &ctx=controller.acCore_;
        REQUIRE(rtw_8814au_phy_init(&ctx,phyStorage.data(),phyStorage.size()*8)==0);
        REQUIRE(rtw_8814au_phy_load_tables(&ctx)==0);
        REQUIRE(rtw_8814au_phy_set_channel(&ctx,6,20)==0);
        REQUIRE(rtw_8814au_phy_calibrate(&ctx)==0);
    }
    void report(uint8_t rate) {
        const uint8_t event[8]={0x0c,7,rate,0,0,0,0,0};
        controller.handle8814C2H(event,sizeof(event));
    }
    unsigned thermalRate() const {return rtw_test_8814_thermal_rate(&controller.acCore_);}
    void thermalTick() {
        // Synthetic meter39 versus factory31 selects a measurable CCK/OFDM
        // difference on RFE1 pathB; the actual tick also performs its normal IQK.
        for(unsigned i=0;i<4;++i)bus.memory[0x2908+i]=uint8_t((39U<<10)>>(8*i));
        REQUIRE(rtw_8814au_phy_tick(&controller.acCore_)==0);
        REQUIRE(rtw_8814au_phy_tick(&controller.acCore_)==0);
    }
};

enum class traffic { adaptive,management,eapol,fixed,multicast,rejected };
static mbuf_t packet(traffic kind) {
    const bool qos=kind==traffic::adaptive||kind==traffic::rejected;
    const size_t header=qos?26:24;auto *m=new host_mbuf;++effects.live;
    m->bytes.assign(header+24,0);
    m->bytes[0]=kind==traffic::management?0:uint8_t(IEEE80211_FC0_TYPE_DATA|(qos?IEEE80211_FC0_SUBTYPE_QOS:0));
    m->bytes[4]=kind==traffic::multicast?1:2;m->bytes[22]=0x30;m->bytes[23]=0x12;
    const uint8_t snap[]={0xaa,0xaa,3,0,0,0,0x08,0};
    std::memcpy(m->bytes.data()+header,snap,sizeof(snap));
    if(kind==traffic::eapol){m->bytes[header+6]=0x88;m->bytes[header+7]=0x8e;}
    return m;
}
static uint32_t word(const std::vector<uint8_t> &bytes,unsigned index) {
    const auto *p=bytes.data()+4*index;
    return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);
}
static void txKeepsReportedRate(traffic kind) {
    auto f=std::make_unique<rate_fixture>();auto &c=f->controller;
    f->report(19);REQUIRE(f->thermalRate()==19&&c.rateControl_.finalRate==19);
    if(kind==traffic::rejected)c.submit_error=EIO;
    EXPECT(c.sendFrame(packet(kind),&f->node)==(kind==traffic::rejected?EIO:0));
    REQUIRE(c.last_transfer.size()>=40);
    const bool adaptive=kind==traffic::adaptive||kind==traffic::rejected;
    EXPECT((word(c.last_transfer,4)&127U)==(adaptive?27U:kind==traffic::fixed?11U:0U));
    EXPECT(bool(word(c.last_transfer,3)&0x100U)==!adaptive);
    unsigned checksum=0;for(unsigned i=0;i<32;i+=2)
        checksum^=unsigned(c.last_transfer[i])|(unsigned(c.last_transfer[i+1])<<8);
    EXPECT(checksum==0);EXPECT(effects.live==0&&effects.frees==1);
    f->thermalTick();
    std::printf("native thermal input after TX: report19 actual%u\n",f->thermalRate());
    EXPECT(c.rateControl_.finalRate==19);EXPECT(f->thermalRate()==19);
}
static void actualThermalProcessingKeepsC2HAuthority() {
    auto reference=std::make_unique<rate_fixture>();auto sent=std::make_unique<rate_fixture>();
    reference->report(3);sent->report(3);
    REQUIRE(sent->controller.sendFrame(packet(traffic::adaptive),&sent->node)==0);
    reference->thermalTick();sent->thermalTick();
    const int expected=rtw_test_8814_thermal_delta(&reference->controller.acCore_,1);
    const int actual=rtw_test_8814_thermal_delta(&sent->controller.acCore_,1);
    REQUIRE(expected==3); // Pinned RFE1 CCK pathB positive table at delta8.
    std::printf("native thermal delta pathB: C2H-only%d C2H-plus-TX%d\n",expected,actual);
    EXPECT(actual==expected);EXPECT(sent->thermalRate()==3);
}
static void invalidAndInactiveReportsKeepRate() {
    auto f=std::make_unique<rate_fixture>();auto &c=f->controller;f->report(19);
    const uint8_t invalid[][8]={{0xc,1,12,1,0,0,0,0},{0xc,1,28,0,0,0,0,0},
        {0xc,1,0x83,0,0,0,0,0},{0xff,1,12,0,0,0,0,0}};
    for(const auto &event:invalid)c.handle8814C2H(event,sizeof(event));
    const uint8_t shortEvent[]={0xc,1,12};c.handle8814C2H(shortEvent,sizeof(shortEvent));
    EXPECT(c.rateControl_.reports==1&&f->thermalRate()==19);
    const auto mask=c.acRateConfig_.mask;c.acRateConfig_.mask&=~(UINT64_C(1)<<12);
    f->report(12);EXPECT(c.rateControl_.reports==1&&f->thermalRate()==19);
    c.acRateConfig_.mask=mask;
    for(unsigned state=0;state<5;++state) {
        c.acRateReady_=state!=0;c.acMediaConnected_=state!=1;
        c.running_=state!=2;c.stopping_=state==3;c.core_.sc_ic.ic_state=state==4?0:IEEE80211_S_RUN;
        f->report(12);EXPECT(c.rateControl_.reports==1&&f->thermalRate()==19);
    }
    c.acRateReady_=c.acMediaConnected_=c.running_=true;c.stopping_=false;
    c.core_.sc_ic.ic_state=IEEE80211_S_RUN;f->report(12);
    EXPECT(c.rateControl_.reports==2&&f->thermalRate()==12);
}
static void newPHYStartsWithoutOldRate() {
    auto f=std::make_unique<rate_fixture>();auto &c=f->controller;f->report(19);
    c.acRateReady_=c.acMediaConnected_=false;
    REQUIRE(rtw_8814au_phy_stop(&c.acCore_)==0);f->resetPHY();
    EXPECT(f->thermalRate()==0);f->report(19);EXPECT(f->thermalRate()==0);
    c.acRateReady_=c.acMediaConnected_=true;f->report(12);EXPECT(f->thermalRate()==12);
}
static void apConstraintSurvivesActualC2HAndTX() {
    const uint16_t txagc[]={0xc94,0xe94,0x1894,0x1a94},scale[]={0xc1c,0xe1c,0x181c,0x1a1c};
    const auto readWord=[](const thermal_bus&bus,unsigned address){
        return uint32_t(bus.memory[address])|(uint32_t(bus.memory[address+1])<<8)|
            (uint32_t(bus.memory[address+2])<<16)|(uint32_t(bus.memory[address+3])<<24);
    };
    for(bool absolute:{false,true})for(unsigned rfe:{0U,1U}) {
        std::array<std::unique_ptr<rate_fixture>,3> f;
        for(unsigned n=0;n<3;n++) {
            f[n]=std::make_unique<rate_fixture>();auto&ctx=f[n]->controller.acCore_;
            ctx.board.rfe_option=uint8_t(rfe);
            for(auto&rom:ctx.board.txpwr_idx_table) {
                for(unsigned group=0;group<14;group++)rom[18+group]=63;
                rom[32]=rom[33]=0;
            }
            REQUIRE(rtw_8814au_phy_init(&ctx,f[n]->phyStorage.data(),f[n]->phyStorage.size()*8)==0);
            REQUIRE(rtw_8814au_phy_load_tables(&ctx)==0);
            if(absolute&&n)REQUIRE(rtw_8814au_phy_set_power_limit_dbm(&ctx,int8_t(20-n*3))==0);
            else REQUIRE(rtw_8814au_phy_set_power_constraint(&ctx,uint8_t(n*3))==0);
        }
        for(unsigned rate=4;rate<28;rate++) {
            for(auto&entry:f) {
                auto&ctx=entry->controller.acCore_;
                REQUIRE(rtw_8814au_phy_set_channel(&ctx,100,20)==0);
                REQUIRE(rtw_8814au_phy_calibrate(&ctx)==0);
                entry->report(uint8_t(rate));EXPECT(entry->thermalRate()==rate);
                REQUIRE(entry->controller.sendFrame(packet(traffic::adaptive),&entry->node)==0);
                entry->thermalTick();EXPECT(entry->thermalRate()==rate);
            }
            for(unsigned n=1;n<3;n++)for(unsigned path=0;path<4;path++) {
                EXPECT((readWord(f[n]->bus,txagc[path])&0x3e000000)==(readWord(f[0]->bus,txagc[path])&0x3e000000));
                EXPECT((readWord(f[n]->bus,scale[path])&0xffe00000)==(readWord(f[0]->bus,scale[path])&0xffe00000));
            }
        }
    }
}
int main(int argc,char **argv) {
    const char *selected=argc>1?argv[1]:"all";bool found=false;
    const auto run=[&](const char *name,auto test) {
        if(std::strcmp(selected,"all")&&std::strcmp(selected,name))return;
        found=true;const unsigned before=failures;test();
        std::printf("%s: %s\n",name,failures==before?"PASS":"FAIL");
    };
    run("adaptive",[]{txKeepsReportedRate(traffic::adaptive);});
    run("management",[]{txKeepsReportedRate(traffic::management);});
    run("eapol",[]{txKeepsReportedRate(traffic::eapol);});
    run("fixed",[]{txKeepsReportedRate(traffic::fixed);});
    run("multicast",[]{txKeepsReportedRate(traffic::multicast);});
    run("rejected",[]{txKeepsReportedRate(traffic::rejected);});
    run("thermal",actualThermalProcessingKeepsC2HAuthority);
    run("invalid-inactive",invalidAndInactiveReportsKeepRate);
    run("new-phy",newPHYStartsWithoutOldRate);
    run("ap-constraint",apConstraintSurvivesActualC2HAndTX);
    if(!found)return 2;
    std::printf("8814 rate/thermal: %u checks, %u failures (actual controller and native PHY)\n",checks,failures);
    return failures?1:0;
}
