/* All thirteen enabled station families exercise their actual RCR and
 * local-MAC initialization against the existing hardware-boundary fixtures.
 * RXFLTMAP1 is checked separately: newer AU/B/C bit12 is not legacy ACF. */
#ifdef RTW_PRIVATE_FILTER
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include RTW_FILTER_FIXTURE
static void filter_bits(uint32_t rcr,bool run) {
    assert((rcr&15U)==14U); // AAP clear; physical/multicast/broadcast present.
    assert((rcr&0xc0U)==(run?0xc0U:0U));
}
#if RTW_PRIVATE_FILTER == 1
#include "rtl8192fu_model.hpp"
int main() {
    for(bool ht:{false,true}) {
        FUFixture f;f.identify();assert(!f.boot());
        const auto rom=fu_rom();assert(!memcmp(f.m.reg.data()+0x610,rom.data()+0x107,6));
        filter_bits(f.m.read(0x608),false);
        const uint8_t ap[6]={2,1,2,3,4,5};
        assert(!rtw_r92fu_station(f.c,1,ap,42,100,0x15,1,ht));
        filter_bits(f.m.read(0x608),true);
        assert(f.m.read(0x6a2,2)==(ht?0x100U:0U)&&f.m.read(0x6a4,2)==0xffff);
        assert(!rtw_r92fu_station(f.c,0,nullptr,0,0,0x15,1,0));
        filter_bits(f.m.read(0x608),false);assert(!f.m.read(0x6a2,2));
    }
    puts("filter RTL8192FU: actual boot/MAC, legacy+HT RUN and scan passed");
}
#elif RTW_PRIVATE_FILTER == 2
#include "rtl8710bu_model.hpp"
int main() {
    for(auto vendor:{RTW_R10BU_SMIC,RTW_R10BU_UMC})for(bool ht:{false,true}) {
        BUFixture f(vendor);f.identify();assert(!f.boot());rtw_r10bu_board board{};
        assert(!rtw_r10bu_board_info(f.c,&board));assert(!memcmp(f.m.reg.data()+0x610,board.mac,6));
        filter_bits(f.m.read(0x608),false);const uint8_t ap[6]={2,1,2,3,4,5};
        assert(!rtw_r10bu_station(f.c,1,ap,42,100,0x15,1,ht));
        filter_bits(f.m.read(0x608),true);
        assert(f.m.read(0x6a2,2)==(ht?0x100U:0U)&&f.m.read(0x6a4,2)==0xffff);
        assert(!rtw_r10bu_station(f.c,0,nullptr,0,0,0x15,1,0));
        filter_bits(f.m.read(0x608),false);assert(!f.m.read(0x6a2,2));
    }
    puts("filter RTL8710BU: actual SMIC/UMC boot/MAC, legacy+HT RUN and scan passed");
}
#elif RTW_PRIVATE_FILTER == 3
static uint32_t word(const uint8_t*p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
int main() {
    for(bool ht:{false,true}) {
        fake_8723 f;f.simulate=true;rtw_8723bu c{};auto bus=f.bus();
        rtw_8723bu_usb usb{0x81,{2,3,0},2,1,512};
        std::vector<uint64_t> storage((rtw_8723bu_private_size()+7)/8);
        CHECK(!rtw_8723bu_init(&c,0xbda,0xb720,&bus,&usb,storage.data(),storage.size()*8));
        fake_8723::write(&f,0xf0,4,2U<<12);fake_8723::write(&f,0x68,4,4U|(1U<<18));
        CHECK(!rtw_8723bu_identify(&c));std::array<uint8_t,512> rom;valid_board_image(rom);
        CHECK(!rtw_8723bu_parse_board(&c,rom.data(),rom.size()));
        CHECK(!rtw_8723bu_boot(&c,RTW8723BUFirmware,sizeof(RTW8723BUFirmware)));
        assert(!memcmp(f.memory.data()+0x610,c.board.mac,6));filter_bits(word(f.memory.data()+0x608),false);
        CHECK(!rtw_8723bu_set_channel(&c,6,20));CHECK(!rtw_8723bu_calibrate(&c));
        rtw_8723bu_station p{{2,3,4,5,6,7},6,1,1,uint8_t(ht),0,uint8_t(ht?255:0),1,100,0x15f,0xfff,0};
        CHECK(!rtw_8723bu_station_state(&c,2,&p));CHECK(!rtw_8723bu_station_state(&c,3,&p));
        filter_bits(word(f.memory.data()+0x608),true);
        assert((unsigned(f.memory[0x6a2])|(unsigned(f.memory[0x6a3])<<8))==(ht?0x100U:0U));
        CHECK(!rtw_8723bu_station_state(&c,1,&p));filter_bits(word(f.memory.data()+0x608),false);
    }
    puts("filter RTL8723BU: actual boot/MAC, legacy+HT RUN and scan passed");
}
#elif RTW_PRIVATE_FILTER == 4
int main() {
    for(unsigned mode:{0U,1U,2U})for(bool ht:{false,true}) {
        const uint8_t index=uint8_t(mode==2);
        Fixture f(mode!=0,index);rtw_92du_mac c{};
        if(!mode)start(f,c);
        else {
            const auto primary=plan();
            const uint8_t cfg[]={9,2,62,0,2,primary.profile->configuration,0,0x80,50,
                9,4,0,0,2,0xff,0xff,0xff,0,7,5,0x81,2,0,2,0,7,5,2,2,0,2,0,
                9,4,1,0,3,0xff,0xff,0xff,0,7,5,0x84,2,0,2,0,7,5,5,2,0,2,0,7,5,6,2,0,2,0};
            f.boot.firmware_ready=0;
            assert(rtw_92du_mac_init_for_interface(&c,&f.boot,&primary,cfg,sizeof(cfg))==RTW_92DU_OK);
            assert(rtw_92du_mac_prepare(&c)==RTW_92DU_OK);f.boot.firmware_ready=1;
            assert(rtw_92du_mac_start(&c)==RTW_92DU_OK);
        }
        assert(!memcmp(f.bus.regs.data()+0x610,f.boot.board.mac,6));
        filter_bits(Bus::word(f.bus.regs.data()+0x608,4),false);
        uint16_t physical=0;assert(rtw_92du_usb_address(index,0x608,&physical)==RTW_USB_OK);
        assert(physical==(index?0x4608:0x608));
        const uint8_t channel=uint8_t(mode==1?36:6);
        assert(rtw_92du_phy_load_tables(&f.boot)==RTW_92DU_OK);
        assert(rtw_92du_phy_set_channel(&f.boot,channel)==RTW_92DU_OK);
        assert(rtw_92du_phy_calibrate(&f.boot)==RTW_92DU_OK);
        rtw_92du_bss p{};p.channel=channel;p.bssid[0]=2;p.bssid[5]=4;p.aid=42;p.beacon_interval=100;p.basic_rates=mode==1?0x150:0xf;p.short_slot=1;
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_AUTH,&p)==RTW_92DU_OK);
        assert(rtw_92du_phy_activate(&f.boot)==RTW_92DU_OK);
        rtw_92du_peer_rates peer{uint16_t(mode==1?0xff0:0xfff),uint16_t(ht?0xffff:0),uint8_t(ht),0,3};rtw_92du_rate_config rates{};
        assert(rtw_92du_build_rate_config(&f.boot,channel,&peer,&rates)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_rates(&c,&rates)==RTW_92DU_OK);
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_RUN,&p)==RTW_92DU_OK);
        filter_bits(Bus::word(f.bus.regs.data()+0x608,4),true);
        assert(Bus::word(f.bus.regs.data()+0x6a2,2)==(ht?0x100U:0U));
        assert(rtw_92du_mac_set_state(&c,RTW_92DU_ST_SCAN,nullptr)==RTW_92DU_OK);
        filter_bits(Bus::word(f.bus.regs.data()+0x608,4),false);
    }
    puts("filter RTL8192DU: SMSP/DMDP MAC0+MAC1, actual MAC start, legacy+HT RUN and scan passed");
}
#elif RTW_PRIVATE_FILTER == 5
int main() {
    for(bool one:{false,true})for(bool ht:{false,true}) {
        Fixture f(one);f.identify();assert(!rtw_r12au_power_on(&f.c));assert(!rtw_r12au_dma_prepare(&f.c));
        auto image=firmware(one);f.b.expected_fw=image.size()-32;rtw_boot_report report{};
        assert(!rtw_r12au_download_fw(&f.c,image.data(),image.size(),&report));assert(!rtw_r12au_mac_init(&f.c));
        assert(!memcmp(f.b.regs.data()+0x610,f.c.board.mac,6));filter_bits(f.b.read(0x608,4),false);
        const uint8_t ap[6]={2,1,2,3,4,5};assert(!rtw_r12au_mac_station(&f.c,ap,42,1));
        assert(!actual_au_receive_maps(&f.c,true,ht));filter_bits(f.b.read(0x608,4),true);
        assert(f.b.read(0x6a2,2)==(ht?0x100U:0U)&&f.b.read(0x6a4,2)==0xffff);
        assert(!rtw_r12au_mac_station(&f.c,nullptr,0,0));assert(!actual_au_receive_maps(&f.c,false,false));
        filter_bits(f.b.read(0x608,4),false);assert(!f.b.read(0x6a2,2));
    }
    puts("filter RTL8812AU/RTL8821AU: actual bootstrap/MAC and controller subtype writes, legacy+HT RUN and scan passed");
}
#elif RTW_PRIVATE_FILTER == 6
int main() {
    const auto gated=rtw_wifi_radio_family_capabilities(RTW_WIFI_RTL8814BU);
    assert(!gated.station_bands&&!gated.station_max_nss);
    for(size_t i=0;i<rtw_usb_wifi_profile_count();++i) {
        const auto *p=rtw_usb_wifi_profile_at(i);
        if(p->family==RTW_WIFI_RTL8814BU)
            assert(rtw_usb_wifi_station_backend(p)!=RTW_WIFI_RTL8814BU);
    }
    for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool ht:{false,true}) {
        Fixture f(family);assert(!f.init());auto p=f.params();
        assert(!memcmp(f.memory.data()+0x610,p.mac,6));filter_bits(f.value(0x608),false);p.ht=uint8_t(ht);
        assert(!rtw88_station_configure(&f.station,RTW88_STATION_RUN,&p));filter_bits(f.value(0x608),true);
        assert(f.value(0x6a2,2)==(ht?0x100U:0U)&&f.value(0x6a4,2)==0xffff);
        assert(!rtw88_station_configure(&f.station,RTW88_STATION_SCAN,&p));filter_bits(f.value(0x608),false);
    }
    puts("filter RTL8821CU/RTL8822BU/RTL8822CU: actual station MAC, legacy+HT RUN and scan passed");
}
#elif RTW_PRIVATE_FILTER == 7
int main() {
    for(bool ht:{false,true}) {
        Bus b;auto c=context(b);auto p=params();p.ht=uint8_t(ht);
        assert(!rtw_8814au_station_initialize(&c,&p));assert(!memcmp(b.bytes+0x610,p.mac,6));filter_bits(value(b,0x608,4),false);
        assert(!rtw_8814au_station_configure(&c,RTW_8814AU_STATION_RUN,&p));filter_bits(value(b,0x608,4),true);
        assert(value(b,0x6a2,2)==(ht?0x100U:0U)&&value(b,0x6a4,2)==0xffff);
        assert(!rtw_8814au_station_configure(&c,RTW_8814AU_STATION_SCAN,&p));filter_bits(value(b,0x608,4),false);
    }
    puts("filter RTL8814AU: actual station MAC, legacy+HT RUN and scan passed");
}
#endif
#else
#include "rtwn_ht_host.hpp"
static void station_receive_filter() {
    /* Catch inherited accept-all-unicast surviving association, including
     * reassociation after monitor mode, without narrowing scan/monitor RX. */
    for (uint32_t chip : {RTWN_CHIP_USB|RTWN_CHIP_92C|RTWN_CHIP_88C,
            RTWN_CHIP_USB|RTWN_CHIP_88C,
            RTWN_CHIP_USB|RTWN_CHIP_92C|RTWN_CHIP_88C|RTWN_CHIP_92C_1T2R,
            RTWN_CHIP_USB|RTWN_CHIP_88E,RTWN_CHIP_USB|RTWN_CHIP_92E}) {
    for (bool ht : {false, true}) {
        rtwn_softc s;
        s.chip=chip;
        s.ntxchains=(chip&(RTWN_CHIP_92C|RTWN_CHIP_92E))&&!(chip&RTWN_CHIP_92C_1T2R)?2:1;
        s.nrxchains=chip&(RTWN_CHIP_92C|RTWN_CHIP_92E)?2:1;
        ieee80211_node peer;
        ieee80211_channel channel;
        s.sc_ic.ic_softc=&s;s.sc_ic.ic_bss=&peer;
        s.sc_ic.ic_ibss_chan=&channel;peer.ni_chan=&channel;
        peer.ni_rates={4,{0x82,0x84,0x8b,0x96}};
        if (ht) {peer.ni_flags=IEEE80211_NODE_HT;peer.ni_rxmcs[0]=peer.ni_rxmcs[1]=0xff;}
        s.sc_ic.ic_opmode=IEEE80211_M_MONITOR;
        actual_station_defaults(&s);
        require(s.sc_ic.ic_opmode==IEEE80211_M_STA&&
            !memcmp(s.registers+R92C_MACID,s.sc_ic.ic_myaddr,6),
            "actual attach/init defaults program station mode and the local MAC");
        rtwn_rxfilter_init(&s);
        const uint32_t initial=rtwn_read_4(&s,R92C_RCR);
        const uint32_t preserved=R92C_RCR_APM|R92C_RCR_AM|R92C_RCR_AB|
            R92C_RCR_APP_ICV|R92C_RCR_AMF|R92C_RCR_HTC_LOC_CTRL|
            R92C_RCR_APP_MIC|R92C_RCR_APP_PHYSTS;
        require((initial&(preserved|R92C_RCR_AAP))==(preserved|R92C_RCR_AAP),
            "filter initialization retains scan receive policy");
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_SCAN,0)==0,
            "initial station scan succeeds");
        require(rtwn_read_4(&s,R92C_RCR)==initial,
            "initial scan preserves AAP and physical/multicast/broadcast policy");
        s.sc_ic.ic_state=IEEE80211_S_ASSOC;
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,
            "station filter test associates");
        require(!(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_AAP),
            "every shared legacy station RUN clears accept-all-unicast");
        require((rtwn_read_4(&s,R92C_RCR)&preserved)==preserved,
            "station RUN retains physical-match multicast broadcast and status delivery");
        require((rtwn_read_4(&s,R92C_RCR)&(R92C_RCR_CBSSID_DATA|R92C_RCR_CBSSID_BCN))==
            (R92C_RCR_CBSSID_DATA|R92C_RCR_CBSSID_BCN),
            "station RUN enables associated BSSID checks");
        require(!(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF)&&
            rtwn_read_2(&s,R92C_RXFLTMAP1)==(ht?0x0100:0)&&
            rtwn_read_2(&s,R92C_RXFLTMAP2)==0xffff,
            "station RUN retains only negotiated BAR control subtype and data reception");
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_SCAN,0)==0,
            "station RUN returns to scan");
        require(!(rtwn_read_4(&s,R92C_RCR)&(R92C_RCR_CBSSID_DATA|R92C_RCR_CBSSID_BCN))&&
            (rtwn_read_4(&s,R92C_RCR)&preserved)==preserved,
            "scan still accepts management from any BSSID after station filtering");
        s.sc_ic.ic_opmode=IEEE80211_M_MONITOR;
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,
            "monitor RUN succeeds after station scan");
        require((rtwn_read_4(&s,R92C_RCR)&
            (R92C_RCR_AAP|R92C_RCR_ADF|R92C_RCR_ACF|R92C_RCR_AMF))==
            (R92C_RCR_AAP|R92C_RCR_ADF|R92C_RCR_ACF|R92C_RCR_AMF)&&
            rtwn_read_2(&s,R92C_RXFLTMAP1)==0xffff&&
            rtwn_read_2(&s,R92C_RXFLTMAP2)==0xffff,
            "monitor RUN restores all-unicast and all control/data subtype reception");
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,
            "monitor teardown succeeds");
        s.sc_ic.ic_opmode=IEEE80211_M_STA;s.sc_ic.ic_state=IEEE80211_S_ASSOC;
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0&&
            !(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_AAP),
            "station reassociation removes monitor all-unicast admission");
        require(!(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF)&&
            rtwn_read_2(&s,R92C_RXFLTMAP1)==(ht?0x0100:0),
            "legacy and HT reassociation restore station control subtype filtering");
    }
    }
    std::puts("filter RTL8192CU aliases/RTL8188EU/RTL8192EU: actual attach/init MAC, legacy+HT RUN, scan/monitor and reassociation passed");
    {
        rtwn_softc s;ieee80211_node peer;
        s.sc_ic.ic_softc=&s;s.sc_ic.ic_bss=&peer;
        peer.ni_rates={4,{0x82,0x84,0x8b,0x96}};
        /* An unsupported nonstation opmode must not inherit the station-only
         * policy. Existing RUN behavior for other opmodes is unchanged. */
        s.sc_ic.ic_opmode=3;
        rtwn_rxfilter_init(&s);
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0&&
            (rtwn_read_4(&s,R92C_RCR)&R92C_RCR_AAP),
            "nonstation opmodes preserve their inherited AAP policy");
    }
}
int main() {
    station_receive_filter();
    for(unsigned smps=0;smps<4;++smps) {
        rtwn_softc cu;ieee80211_node peer;cu.chip=RTWN_CHIP_92C;cu.ntxchains=cu.nrxchains=2;cu.sc_ic.ic_bss=&peer;
        peer.ni_flags=IEEE80211_NODE_HT;peer.ni_htcaps=uint16_t(smps<<IEEE80211_HTCAP_SMPS_SHIFT);
        peer.ni_rates={4,{0x82,0x84,0x8b,0x96}};peer.ni_rxmcs[0]=peer.ni_rxmcs[1]=0xff;
        rtwn_ht_capabilities(&cu);
        require(rtwn_ra_init(&cu)==0,"CU SMPS peer installs actual firmware rate mask");
        require(cu.rate_config.tx_streams==(smps==IEEE80211_HTCAP_SMPS_DIS?2:1),"CU caps static/dynamic/reserved SMPS peers to one TX stream");
        require(cu.rate_config.rate_mask==(smps==IEEE80211_HTCAP_SMPS_DIS?0xffff00fU:0xff00fU),"CU firmware mask never requests unprotected SMPS MIMO");
        require(cu.sc_ic.ic_sup_mcs[1]==0xff&&cu.nrxchains==2,"peer SMPS limit preserves local two-stream receive capability");
    }
    for(auto chip:{RTWN_CHIP_92C,RTWN_CHIP_88E}) {
        rtwn_softc legacy;ieee80211_node peer;legacy.chip=chip;legacy.sc_ic.ic_bss=&peer;
        legacy.ntxchains=legacy.nrxchains=chip==RTWN_CHIP_88E?1:2;
        peer.ni_rates={4,{0x82,0x84,0x8b,0x96}};peer.ni_rxmcs[0]=0x81;peer.ni_rxmcs[1]=0xff;
        peer.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20;
        peer.ni_htcaps=IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT;
        rtwn_ht_capabilities(&legacy);
        require(legacy.sc_ic.ic_htcaps&IEEE80211_HTCAP_SGI20,"legacy advertises implemented HT20 SGI");
        require(!(legacy.sc_ic.ic_htcaps&(IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI40)),"legacy does not claim unvalidated HT40");
        require(legacy.sc_ic.ic_sup_mcs[0]==0xff&&legacy.sc_ic.ic_sup_mcs[1]==(chip==RTWN_CHIP_88E?0:0xff),"receive MCS follows actual chains");
        require(rtwn_ra_init(&legacy)==0,"legacy HT20 station configuration succeeds");
        require(legacy.rate_config.ht&&legacy.rate_config.sgi20&&!legacy.rate_config.bw40&&
            legacy.rate_config.rate_mask==(chip==RTWN_CHIP_88E?0x8100fU:0xff8100fU),"gen1 applies actual peer MCS and stream count");
        require((legacy.rrsr&0xfffU)==15&&!(legacy.rrsr&0xfffff000U),"HT MCS never leaks into legacy response rates");
        require(!legacy.fw_station_connected,"gen1 does not fabricate gen2 firmware media state");
        if(chip==RTWN_CHIP_92C)require(legacy.commands.size()==2&&legacy.commands[1].id==6&&legacy.commands[1].payload==std::vector<uint8_t>({0xf8,0x0f,0xa0,0x0f,0x10}),"CU H2C carries negotiated MCS and SGI in gen1 wire layout");
        else require(legacy.commands.empty(),"88E uses host rate control with no fake firmware command");
        legacy.io_error=EIO;require(rtwn_ra_init(&legacy)==EIO&&!legacy.rate_config.rate_mask,"failed HT setup clears all published rates");
    }
    rtwn_softc s; ieee80211_node ni; s.sc_ic.ic_bss=&ni;
    static const uint8_t rates[]={0x82,0x84,0x8b,0x96,12,18,24,36,48,72,96,108};
    ni.ni_rates.rs_nrates=sizeof(rates); std::memcpy(ni.ni_rates.rs_rates,rates,sizeof(rates));
    require(rtwn_ra_init(&s)==0,"RTL8192EU initializes negotiated station rates");
    require(s.commands.size()==1 && s.commands[0].id==0x40,"RTL8192EU configures generation-2 firmware rate adaptation");
    require(s.commands[0].payload==std::vector<uint8_t>({0,6,0,0xff,0x0f,0,0}),"legacy fallback uses generation-2 BG RAID and only peer legacy rates");
    require((s.rrsr&0xfffU)==0xfffU && s.inirts==3,"legacy response rates and basic RTS rate are preserved");
    s.commands.clear();s.sc_ic.ic_softc=&s;
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"successful RUN enters associated state");
    require(s.commands.size()==2 && s.commands[0].id==0x40 && s.commands[1].id==0x01,"RUN installs negotiated rates before enabling firmware connection");
    require(s.commands[1].payload==std::vector<uint8_t>({0x21,0,0}) && s.fw_station_connected,"RUN identifies MACID0 AP peer and enables firmware rate control");
    require(rtwn_read_2(&s,R92C_RXFLTMAP1)==0 && !(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF),"legacy RUN does not open control reception");
    s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"leaving RUN succeeds");
    require(s.commands.size()==1 && s.commands[0].id==0x01 && s.commands[0].payload==std::vector<uint8_t>({0x20,0,0}),"leaving RUN disconnects firmware AP peer");
    require(!s.fw_station_connected && !s.rate_config.rate_mask,"leaving RUN clears firmware rate eligibility");
    ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20;ni.ni_rxmcs[0]=0xff;ni.ni_rxmcs[1]=0xff;
    s.commands.clear();
    rtwn_write_4(&s,R92C_RCR,rtwn_read_4(&s,R92C_RCR)|R92C_RCR_ACF);
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT20 RUN succeeds");
    require(s.commands[0].payload==std::vector<uint8_t>({0,0x82,0,0xff,0xff,0xff,0x0f}),"HT20 two-stream MCS0-15 and negotiated SGI use exact generation-2 wire bytes");
    require(s.rate_config.ht && s.rate_config.tx_streams==2 && s.rate_config.initial_rate==27,"HT state enables two transmit streams and MCS15");
    require(s.registers[R92C_RXFLTMAP1]==0 && s.registers[R92C_RXFLTMAP1+1]==1,"HT RX admits only BAR control subtype");
    require(!(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF),"HT subtype filter is not bypassed by accepting every control frame");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT shutdown succeeds");
    require(rtwn_read_2(&s,R92C_RXFLTMAP1)==0 && !(rtwn_read_4(&s,R92C_RCR)&R92C_RCR_ACF),"HT shutdown closes the control subtype filter");
    s.commands.clear();s.ntxchains=1;
    require(rtwn_ra_init(&s)==0 && s.commands[0].payload==std::vector<uint8_t>({0,0x83,0,0xff,0xff,0x0f,0}),"1T2R bond limits transmit rates to MCS0-7 and 1SS RAID");
    s.commands.clear();s.ntxchains=2;ni.ni_rxmcs[0]=0x11;ni.ni_rxmcs[1]=0;
    require(rtwn_ra_init(&s)==0 && s.rate_config.rate_mask==0x00011fffU && s.rate_config.raid==3,"peer receive MCS intersection preserves disabled MCS bits");
    ni.ni_flags=0;s.commands.clear();
    require(rtwn_ra_init(&s)==0 && !s.rate_config.ht && !s.rate_config.sgi20 && s.rate_config.rate_mask==0xfffU,"beacon capabilities without negotiated HT retain legacy rates");
    ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20;ni.ni_rxmcs[0]=0xff;ni.ni_rxmcs[1]=0xff;
    for(int failed_id : {0x40,0x01}) {
        s.sc_ic.ic_state=IEEE80211_S_ASSOC;s.ledlink=0;s.fw_station_connected=0;s.fw_error=ETIMEDOUT;s.fw_fail_id=failed_id;s.commands.clear();
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==ETIMEDOUT,"mailbox failures propagate from RUN initialization");
        require(s.sc_ic.ic_state==IEEE80211_S_ASSOC && !s.ledlink && !s.fw_station_connected && !s.rate_config.rate_mask,"failed firmware configuration cannot expose a connected HT station");
        require(s.registers[R92C_RXFLTMAP2]==0 && s.registers[R92C_RXFLTMAP2+1]==0,"failed RUN disables data reception");
    }
    s.fw_error=0;s.fw_fail_id=-1;s.ntxchains=0;s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==EINVAL && s.commands.empty(),"invalid hardware stream configuration fails before sending firmware commands");
    s.ntxchains=2;s.fw_station_connected=1;s.sc_ic.ic_state=IEEE80211_S_RUN;s.fw_error=ETIMEDOUT;s.fw_fail_id=0x01;
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==ETIMEDOUT && s.sc_ic.ic_state==IEEE80211_S_INIT,"INIT still tears down host state when firmware disconnect fails");
    require(!s.fw_station_connected && !s.rate_config.rate_mask,"failed disconnect clears host eligibility");
    rtw_92e_rate_config c{};uint8_t mcs[2]={0xff,0xff},wire[7]{};
    require(rtw_92e_build_rate_config(0xff0,mcs,2,1,1,1,&c) && c.raid==4,"OFDM-only HT20 two-stream peer uses GN RAID4");
    require(rtw_92e_build_rate_config(0xff0,mcs,1,1,1,1,&c) && c.raid==5,"OFDM-only HT20 one-stream peer uses GN RAID5");
    require(rtw_92e_build_rate_config(0xff0,nullptr,1,0,1,1,&c) && c.raid==7,"OFDM-only legacy peer uses G RAID7");
    require(rtw_92e_build_rate_config(0x0f,nullptr,2,0,1,1,&c) && c.raid==8,"CCK-only legacy peer uses B RAID8");
    for(unsigned local=0;local<2;local++) for(unsigned peer=0;peer<2;peer++) {
        require(rtw_92e_build_rate_config(0xfff,mcs,2,1,local,peer,&c),"SGI combination has a valid rate mask");
        require(c.sgi20==(local&&peer),"SGI20 requires both hardware and negotiated peer support");
    }
    mcs[0]=0;mcs[1]=0xff;
    require(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&c) && !c.ht,"invalid peer without mandatory first-stream MCS falls back to legacy");
    require(!rtw_92e_build_rate_config(0,mcs,2,1,1,1,&c) && !c.rate_mask,"empty legacy mask fails closed");
    require(!rtw_92e_build_rate_config(0x1000,mcs,2,1,1,1,&c),"unknown legacy rate bits are rejected");
    require(!rtw_92e_build_rate_config(0xfff,nullptr,2,1,1,1,&c),"HT requires peer rate evidence");
    require(!rtw_92e_make_macid_config(nullptr,0,wire) && !rtw_92e_make_macid_config(&c,0,wire),"invalid firmware configuration is rejected");
    require(!rtw_92e_make_media_status(2,0,wire) && !rtw_92e_make_media_status(1,128,wire),"media report rejects invalid state and MACID");
    mcs[0]=0xff;mcs[1]=0xff;
    require(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&c),"valid config exists for serializer validation");
    c.raid=0;
    require(!rtw_92e_make_macid_config(&c,0,wire),"HT40 firmware rate tables cannot enter the HT20 command");
    c.raid=8;
    require(!rtw_92e_make_macid_config(&c,0,wire),"CCK-only firmware table cannot carry negotiated MCS mask");
    s.fw_error=0;s.io_error=0;s.byte_writes.clear();s.registers[R92C_AMPDU_MIN_SPACE]=0xa8;
    const uint8_t expected[4][4]={{0x41,0x44,0x42,0x44},{0x41,0x88,0x72,0x88},{0x41,0xa8,0x72,0xb9},{0x41,0xa8,0x72,0xb9}};
    for(unsigned factor=0;factor<4;factor++) for(unsigned density=0;density<8;density++) {
        require(rtwn_r92e_ampdu_params(&s,factor,density)==0,"valid peer AMPDU parameters configure hardware");
        require(std::memcmp(s.registers+R92C_AGGLEN_LMT,expected[factor],4)==0,"aggregation limit nibbles match pinned Linux factor algorithm");
        require(s.registers[R92C_AMPDU_MIN_SPACE]==(0xa8U|density),"peer minimum spacing preserves unrelated hardware bits");
    }
    auto writes=s.byte_writes.size();
    require(rtwn_r92e_ampdu_params(&s,4,0)==EINVAL && rtwn_r92e_ampdu_params(&s,0,8)==EINVAL && s.byte_writes.size()==writes,"invalid aggregation factors and densities do not touch hardware");
    s.io_error=EIO;
    require(rtwn_r92e_ampdu_params(&s,3,7)==EIO,"AMPDU register failures propagate to BA setup");
    ieee80211_channel primary,extension;extension.ic_freq=2432;
    rtwn_set_chan(&s,&primary,&extension);
    require(!s.extension_power,"unflagged channel prevents extension-channel transmit power programming");
    require(!(rtwn_read_2(&s,R92C_WMAC_TRXPTCL_CTL)&R92C_WMAC_TRXPTCL_CTL_BW_MASK) && !(rtwn_read_4(&s,R92C_FPGA0_RFMOD)&R92C_RFMOD_40MHZ),"invalid channel direction cannot enable HT40");
    require((s.rf_output[0]&R88E_RF_CHNLBW_BW20)==R88E_RF_CHNLBW_BW20 && (s.rf_output[1]&R88E_RF_CHNLBW_BW20)==R88E_RF_CHNLBW_BW20,"both RF paths retain20MHz bandwidth");
    s.io_error=0;
    /* Real RUN negotiation, including channel direction and regulatory guard. */
    for(int ch=1;ch<=11;ch++){
        auto&c=s.sc_ic.ic_channels[ch];c.ic_freq=2407U+5U*ch;
        c.ic_flags=IEEE80211_CHAN_2GHZ|(ch<=7?IEEE80211_CHAN_HT40U:0U)|(ch>=5?IEEE80211_CHAN_HT40D:0U);
    }
    ni.ni_chan=&s.sc_ic.ic_channels[6];ni.ni_htcaps=IEEE80211_HTCAP_CBW20_40;
    ni.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_HT_SGI20|IEEE80211_NODE_HT_SGI40;
    ni.ni_chw=IEEE80211_CHAN_WIDTH_40;ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;
    s.sc_ic.ic_state=IEEE80211_S_ASSOC;s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT40 channel6 extension above associates");
    require(s.channel_bw40&&s.extension_power&&s.rate_config.bw40&&s.rate_config.sgi40&&!s.rate_config.sgi20,
        "negotiated 40 MHz and SGI40 update radio, power and firmware together");
    require((s.rf_output[0]&0xffU)==8U&&(s.rf_output[1]&0xffU)==8U,"40 MHz radio uses RF center channel8 for primary6+secondary10");
    require((s.rf_output[0]&R88E_RF_CHNLBW_BW20)==R92E_RF_CHNLBW_BW40&&
        (s.rf_output[1]&R88E_RF_CHNLBW_BW20)==R92E_RF_CHNLBW_BW40,"RTL8192EU HT40 RF width code=0x400, not HT20 or 80 MHz");
    require(s.commands.size()==2&&s.commands[0].payload==std::vector<uint8_t>({0,0x82,1,0xff,0xff,0xff,0x0f}),
        "Linux gen2 HT40 firmware MACID payload uses BW=1 and SGI40 bit");
    require((rtwn_read_2(&s,R92C_WMAC_TRXPTCL_CTL)&R92C_WMAC_TRXPTCL_CTL_BW_MASK)==R92C_WMAC_TRXPTCL_CTL_BW_40,
        "40MHz MAC width follows the associated AP");
    require(rtwn_read_1(&s,R92E_DATA_SC)==2,
        "HT40+ DATA_SUBCHANNEL matches Linux gen2 secondary above code 2");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT40 teardown succeeds");
    ni.ni_htop0=IEEE80211_HTOP0_SCO_SCB|IEEE80211_HTOP0_CHW;s.commands.clear();
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT40 channel6 extension below associates");
    require(s.channel_bw40&&s.extension_power&&(s.rf_output[0]&0xffU)==4U,
        "below-primary extension tunes radio to center channel4");
    require(rtwn_read_1(&s,R92E_DATA_SC)==1,
        "HT40- DATA_SUBCHANNEL matches Linux gen2 secondary below code 1");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT40 below teardown succeeds");
    /* Width fallback cases: AP HT20, 40-intolerant, user override, or
     * mismatch between secondary offset and permitted local channel. */
    for(unsigned reason=0;reason<7;reason++){
        ni.ni_chw=IEEE80211_CHAN_WIDTH_40;ni.ni_htcaps=IEEE80211_HTCAP_CBW20_40;
        ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;s.sc_ic.ic_userflags=0;
        s.sc_ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;
        ni.ni_chan=&s.sc_ic.ic_channels[6];
        if(reason==0)ni.ni_chw=IEEE80211_CHAN_WIDTH_20;
        if(reason==1)ni.ni_htcaps|=IEEE80211_HTCAP_40INTOLERANT;
        if(reason==2)s.sc_ic.ic_userflags=IEEE80211_F_NOHT40;
        if(reason==3)ni.ni_htcaps=0;
        if(reason==4){ni.ni_chan=&s.sc_ic.ic_channels[11];ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;}
        if(reason==5)ni.ni_htop0=0;
        if(reason==6)ni.ni_htop0&=~IEEE80211_HTOP0_CHW;
        s.sc_ic.ic_state=IEEE80211_S_ASSOC;s.commands.clear();
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0,"HT40 unsafe association gracefully falls back to HT20");
        require(!s.channel_bw40&&!s.extension_power&&!s.rate_config.bw40&&!s.rate_config.sgi40&&
                !s.commands[0].payload[2],"HT20 fallback is consistent across firmware and RF");
        require(rtwn_read_1(&s,R92E_DATA_SC)==0,
            "HT20 fallback resets the secondary subchannel register");
        require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT20 fallback teardown succeeds");
    }
    ni.ni_htcaps=IEEE80211_HTCAP_CBW20_40;ni.ni_chan=&s.sc_ic.ic_channels[6];
    ni.ni_chw=IEEE80211_CHAN_WIDTH_40;ni.ni_htop0=IEEE80211_HTOP0_SCO_SCA|IEEE80211_HTOP0_CHW;s.sc_ic.ic_userflags=0;
    s.commands.clear();s.sc_ic.ic_state=IEEE80211_S_ASSOC;
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_RUN,0)==0&&s.rate_config.bw40,"retry after fallback enables 40MHz");
    require(rtwn_newstate(&s.sc_ic,IEEE80211_S_INIT,0)==0,"HT40 last teardown succeeds");
    rtw_92e_rate_config width{};uint8_t widest[2]={0xff,0xff},width_payload[7]{};
    for(int l=0;l<=1;l++)for(int peer=0;peer<=1;peer++){
        require(rtw_92e_build_rate_config_width(0xfff,widest,2,1,1,1,1,l,peer,&width),"all SGI40 combinations are valid");
        require(width.bw40 && width.sgi40==(l&&peer) && !width.sgi20,"SGI40 never leaks SGI20");
        require(rtw_92e_make_macid_config(&width,0,width_payload) && width_payload[2]==1 &&
                bool(width_payload[1]&0x80)==bool(l&&peer),"SGI40 and width encode independently");
    }
    require(rtw_92e_build_rate_config_width(0xfff,widest,2,0,1,1,1,1,1,&width) && !width.bw40,
        "non-HT peer never negotiates HT40");
    require(!rtw_92e_build_rate_config_width(0xfff,widest,2,1,1,1,2,1,1,&width),
        "invalid HT40 selector rejected");
    require(rtw_92e_build_rate_config_width(0xfff,widest,2,1,1,1,1,1,1,&width),
        "valid HT40 config exists");
    width.sgi20=1;
    require(!rtw_92e_make_macid_config(&width,0,width_payload),"firmware rejects conflicting SGI modes");

    /* Power table regression: the first HT40 implementation accidentally
     * left the second spatial stream at zero, despite advertising MCS8-15.
     * Keep HT20 calibration byte-for-byte compatible with the old path. */
    ieee80211_channel pwr_primary{},pwr_secondary{};
    pwr_primary.ic_freq=2412;pwr_secondary.ic_freq=2432;
    s.ntxchains=2;
    s.sc_r92e_rom.txpwr_a.ht40_tx_pwr[0]=36;
    s.sc_r92e_rom.txpwr_a.ht20_ofdm_tx_pwr_diff=0x10;
    s.sc_r92e_rom.txpwr_a.pwr_diff[0].ht40_ht20_tx_pwr_diff=0xe3;
    uint16_t pwr[RTWN_POWER_COUNT]{};
    rtwn_r92e_get_txpower(&s,0,&pwr_primary,&pwr_secondary,pwr);
    require(pwr[RTWN_RIDX_MCS0]==36&&pwr[RTWN_RIDX_MCS8]==34&&
            pwr[RTWN_RIDX_MCS15]==34,
        "HT40 calibrated two-stream power uses signed ROM high-nibble differential, not zero");
    rtwn_r92e_get_txpower(&s,0,&pwr_primary,nullptr,pwr);
    require(pwr[RTWN_RIDX_MCS0]==37&&pwr[RTWN_RIDX_MCS8]==40&&
            pwr[RTWN_RIDX_MCS15]==40,
        "HT20 calibrated one-stream/two-stream power retains the legacy ROM offsets");
    s.sc_r92e_rom.txpwr_a.ht40_tx_pwr[0]=63;
    s.sc_r92e_rom.txpwr_a.pwr_diff[0].ht40_ht20_tx_pwr_diff=0x71;
    rtwn_r92e_get_txpower(&s,0,&pwr_primary,&pwr_secondary,pwr);
    require(pwr[RTWN_RIDX_MCS8]==63,
        "HT40 two-stream positive differential is clamped to radio maximum");

    s.io_error=0;rtwn_write_4(&s,R92C_HPON_FSM,1U<<22);
    require(rtwn_read_chipid(&s)==0 && (s.chip&RTWN_CHIP_92C_1T2R),"8191EU1T2R bonding is detected before chain count and MCS advertisement");
    rtwn_write_4(&s,R92C_HPON_FSM,0);
    require(rtwn_read_chipid(&s)==0 && !(s.chip&RTWN_CHIP_92C_1T2R),"8192EU2T2R bonding clears any stale one-stream flag");
    rtwn_write_4(&s,R92C_SYS_CFG,R92C_SYS_CFG_TRP_VAUX_EN);
    require(rtwn_read_chipid(&s)==EIO,"unsupported8192EU test chips fail before HT advertisement");
    std::puts("RTL8192EU HT core tests passed");
}

#endif /* RTW_PRIVATE_FILTER */
