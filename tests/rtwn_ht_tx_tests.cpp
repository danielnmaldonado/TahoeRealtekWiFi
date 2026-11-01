#include "rtwn_ht_tx_host.hpp"
#include <memory>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static uint32_t word(const std::vector<uint8_t>&b,unsigned n){const auto*p=b.data()+n*4;return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
struct fixture {
    RTWUSBController c;ieee80211_node n;
    fixture(){
        effects={};c.core_.sc_ic.ic_bss=&n;
        const uint8_t rates[]={0x82,0x84,0x8b,0x96,12,18,24,36,48,72,96,108};
        n.ni_rates.rs_nrates=sizeof(rates);std::memcpy(n.ni_rates.rs_rates,rates,sizeof(rates));n.ni_txrate=11;
        const uint8_t mcs[2]={0xff,0xff};CHECK(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&c.core_.rate_config));
    }
};
static mbuf_t packet(unsigned tid=0,bool qos=true,bool eapol=false,bool protect=false,bool multicast=false,unsigned type=IEEE80211_FC0_TYPE_DATA,size_t payload=24){
    auto*m=new host_mbuf;++effects.live;size_t header=qos?26U:24U;m->bytes.assign(header+payload,0);
    m->bytes[0]=uint8_t(type|(qos?IEEE80211_FC0_SUBTYPE_QOS:0));m->bytes[1]=protect?IEEE80211_FC1_PROTECTED:0;
    m->bytes[4]=multicast?1:2;m->bytes[22]=0x30;m->bytes[23]=0x12;
    if(qos)m->bytes[24]=uint8_t(tid);
    const uint8_t snap[]={0xaa,0xaa,3,0,0,0,0x08,0};
    if(payload>=sizeof(snap)){std::memcpy(m->bytes.data()+header,snap,sizeof(snap));if(eapol){m->bytes[header+6]=0x88;m->bytes[header+7]=0x8e;}}
    return m;
}
static void safe_descriptor(const std::vector<uint8_t>&b){
    CHECK(b.size()>=40);CHECK(!(word(b,1)&0x00c00000U));CHECK(!(word(b,8)&0x8000U));
    CHECK((word(b,9)&0x00fff000U)==0x00123000U);
    unsigned sum=0;for(unsigned i=0;i<32;i+=2)sum^=unsigned(b[i])|(unsigned(b[i+1])<<8);CHECK(sum==0);
}
static void safe_gen1_descriptor(const std::vector<uint8_t>&b) {
    CHECK(b.size()>=32);CHECK(((word(b,0)>>16)&255U)==32U);
    CHECK((word(b,1)&0x00c00000U)==0); // No hardware cipher.
    CHECK(!(word(b,2)&0x1000U)); // No AMPDU on either legacy path.
    CHECK((word(b,3)&0x0fff0000U)==0x01230000U); // Software sequence.
    CHECK(!(word(b,3)&0x80000000U));CHECK(!(word(b,4)&0x80U));
    unsigned sum=0;for(unsigned i=0;i<32;i+=2)sum^=unsigned(b[i])|(unsigned(b[i+1])<<8);CHECK(sum==0);
}
static void legacy_profile(fixture&f,rtw_wifi_family family) {
    f.c.family_=family;f.n.ni_flags=0;f.c.core_.rate_config={};
    if(family==RTW_WIFI_RTL8814AU)f.c.acCore_.phy_initialized=1;
    f.c.core_.fw_station_connected=0; // 92E MEDIA_STATUS state is irrelevant on gen1.
}
static void gen1_station_tx_paths() {
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);
        const auto&b=f->c.last_transfer;
        CHECK(b.size()==80U);CHECK((word(b,0)&0xffffU)==48U);
        CHECK(((word(b,1)>>16)&15U)==4U);CHECK(((word(b,1)>>8)&31U)==0U);
        CHECK((word(b,5)&0x3fU)==11U);CHECK(b[32]==IEEE80211_FC0_TYPE_DATA&&b[54]==0x30&&b[55]==0x12);
        if(family==RTW_WIFI_RTL8192CU) {
            CHECK(!(word(b,4)&0x100U));CHECK(!(word(b,2)&0x80000U));
            CHECK((word(b,1)&0x40U)&&!(word(b,2)&0x10000U));
            CHECK(f->c.rateControl_.requests==0&&f->c.rateControl_.pendingReports==0);
        }else{
            CHECK(word(b,4)&0x100U);CHECK(word(b,2)&0x80000U);
            CHECK(!(word(b,1)&0x40U)&&(word(b,2)&0x10000U));
            CHECK(f->c.rateControl_.requests==1&&f->c.rateControl_.pendingReports==1);
        }
        CHECK(f->c.txHTPackets_==0&&f->c.txAMPDUPackets_==0);
        CHECK(effects.encryptions==0&&effects.live==0&&effects.frees==1&&effects.releases==1);
        safe_gen1_descriptor(b);
    }
    for(unsigned rates=0;rates<3;++rates){auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8192CU);
        if(rates==1){std::memmove(f->n.ni_rates.rs_rates,f->n.ni_rates.rs_rates+4,8);f->n.ni_rates.rs_nrates=8;f->n.ni_txrate=7;}
        if(rates==2){f->n.ni_rates.rs_nrates=4;f->n.ni_txrate=3;}
        CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);
        CHECK(((word(f->c.last_transfer,1)>>16)&15U)==4U+rates);
        CHECK(!(word(f->c.last_transfer,4)&0x100U));safe_gen1_descriptor(f->c.last_transfer);
    }
    // The USB padding decision must use the selected descriptor size.
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU})for(int delta:{-1,0,1}){
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        if(family==RTW_WIFI_RTL8192EU)f->c.core_.rate_config={};
        const size_t desc=family==RTW_WIFI_RTL8192EU?40U:32U;
        const size_t payload=(size_t)(512-int(desc)-24+delta);
        CHECK(f->c.sendFrame(packet(0,false,false,false,false,IEEE80211_FC0_TYPE_DATA,payload),&f->n)==0);
        const size_t raw=desc+24U+payload;
        CHECK(f->c.last_transfer.size()==raw+(raw%512U==0U?1U:0U));
        if(raw%512U==0U)CHECK(f->c.last_transfer.back()==0);
    }
    // A protected EAPOL SNAP is classified before the crypto boundary moves it.
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU})for(bool protect:{false,true})for(bool multicast:{false,true}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        CHECK(f->c.sendFrame(packet(0,false,true,protect,multicast),&f->n)==0);
        const auto&b=f->c.last_transfer;
        CHECK(word(b,4)&0x100U);CHECK((word(b,5)&0x3fU)==0U);CHECK(!(word(b,2)&0x80000U));
        CHECK(f->c.rateControl_.requests==0&&f->c.rateControl_.pendingReports==0);
        CHECK(bool(word(b,0)&0x01000000U)==multicast);
        CHECK(b.size()==80U+(protect?16U:0U));CHECK(effects.encryptions==unsigned(protect));
        if(protect)CHECK(b[32+24]==0xcc&&b[32+32]==uint8_t(0xaa^0x5a));
        else CHECK(b[32+24]==0xaa&&b[32+30]==0x88&&b[32+31]==0x8e);
        safe_gen1_descriptor(b);
    }
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU})for(unsigned mode=0;mode<3;++mode){
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        auto*m=mode==0?packet(0,false,false,false,false,0):packet(0,false,false,false,mode==1);
        if(mode==2)f->c.core_.sc_ic.ic_state=0;
        CHECK(f->c.sendFrame(m,&f->n)==0);const auto&b=f->c.last_transfer;
        CHECK(word(b,4)&0x100U);CHECK(!(word(b,2)&0x80000U));
        CHECK(f->c.rateControl_.requests==0&&f->c.rateControl_.pendingReports==0);safe_gen1_descriptor(b);
    }
    // Gen1 protection controls live in DW4; DW3 retains the software sequence.
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU})for(unsigned mode=0;mode<4;++mode)for(bool eapol:{false,true}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        if(mode==1||mode==2){f->c.core_.sc_ic.ic_flags|=IEEE80211_F_USEPROT;f->c.core_.sc_ic.ic_protmode=mode==1?IEEE80211_PROT_CTSONLY:IEEE80211_PROT_RTSCTS;}
        if(mode==3)f->c.core_.sc_ic.ic_rtsthreshold=32;
        CHECK(f->c.sendFrame(packet(0,false,eapol),&f->n)==0);
        const auto&b=f->c.last_transfer;
        CHECK((word(b,4)&0x3800U)==(mode==0?0U:mode==1?0x2800U:0x3000U));
        CHECK(!(word(b,3)&0x3800U));safe_gen1_descriptor(b);
        if(eapol){CHECK(word(b,4)&0x100U);CHECK((word(b,5)&0x3fU)==0);CHECK(!(word(b,2)&0x80000U));}
    }
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        f->c.core_.sc_ic.ic_rtsthreshold=1;f->c.core_.sc_ic.ic_flags|=IEEE80211_F_USEPROT;
        f->c.core_.sc_ic.ic_protmode=IEEE80211_PROT_CTSONLY;
        CHECK(f->c.sendFrame(packet(0,false,false,false,false,0),&f->n)==0);
        CHECK(!(word(f->c.last_transfer,4)&0x3800U));
    }
    {auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8192CU);
        CHECK(f->c.usesLegacyFirmwareRate()); // No 92E MEDIA_STATUS flag is needed.
        f->c.running_=false;CHECK(!f->c.usesLegacyFirmwareRate());f->c.running_=true;
        f->c.stopping_=true;CHECK(!f->c.usesLegacyFirmwareRate());f->c.stopping_=false;
        f->c.core_.sc_ic.ic_state=0;CHECK(!f->c.usesLegacyFirmwareRate());f->c.core_.sc_ic.ic_state=IEEE80211_S_RUN;
        f->c.core_.sc_ic.ic_bss=nullptr;CHECK(!f->c.usesLegacyFirmwareRate());f->c.core_.sc_ic.ic_bss=&f->n;
        for(auto family:{RTW_WIFI_UNKNOWN,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU}){f->c.family_=family;CHECK(!f->c.usesLegacyFirmwareRate());}
    }
    // CU submits use firmware RA without SPE, even across many USB submits.
    {auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8192CU);
        for(unsigned i=0;i<64;++i)CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);
        CHECK(f->c.rateControl_.requests==0&&f->c.rateControl_.pendingReports==0);
        CHECK(f->c.rateControl_.htSampleCounter==0&&f->c.txHTPackets_==0);
        CHECK(effects.frees==64&&effects.releases==64&&effects.live==0);
    }
    {auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8192CU);f->c.core_.sc_ic.ic_bss=nullptr;
        CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);CHECK(word(f->c.last_transfer,4)&0x100U);
        CHECK(!(word(f->c.last_transfer,2)&0x80000U)&&f->c.rateControl_.pendingReports==0);
    }
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,family);
        f->n.ni_flags=IEEE80211_NODE_HT;
        const uint8_t staleMCS[2]={0xff,0xff};CHECK(rtw_92e_build_rate_config(0xfff,staleMCS,2,1,1,1,&f->c.core_.rate_config));
        CHECK(!f->c.usesFirmwareRate());
        f->n.ni_rx_ba[0].ba_buf=&f->n;f->n.ni_rx_ba[0].ba_winsize=16;
        f->n.ni_tx_ba[0].ba_winsize=16;
        CHECK(RTWUSBController::ampduRXStart(&f->c.core_.sc_ic,&f->n,0)==EINVAL);
        CHECK(RTWUSBController::ampduTXStart(&f->c.core_.sc_ic,&f->n,0)==EINVAL);
        CHECK(f->c.core_.ampdu_calls==0);
        CHECK(f->c.sendFrame(packet(4,true),&f->n)==EINVAL);
        CHECK(f->c.submitted==0&&effects.live==0&&effects.frees==1);
    }
}
static void ac8814_station_tx_paths() {
    for(bool ready:{false,true}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        CHECK(rtw_8814au_build_rate_config(0xfff,nullptr,2,0,0,0,&f->c.acRateConfig_));
        f->c.acRateReady_=f->c.acMediaConnected_=ready;
        CHECK(f->c.uses8814FirmwareRate()==ready);
        CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);const auto&b=f->c.last_transfer;
        CHECK(b.size()==88 && word(b,0)==0x04280030U);
        CHECK((word(b,1)&0x00ff0000U)==(ready?0x60000U:0x70000U));
        CHECK((word(b,3)&0x500U)==(ready?0U:0x500U));CHECK(word(b,4)==11);
        CHECK(!word(b,2) && !word(b,8));safe_descriptor(b);
        CHECK(effects.phy_updates==0);
        CHECK(f->c.rateControl_.requests==0 && f->c.rateControl_.pendingReports==0);
        CHECK(f->c.txHTPackets_==0 && f->c.txAMPDUPackets_==0);
    }
    // A retained host legacy index must not select preamble/SGI bits for a
    // different firmware-owned initial OFDM rate in a mixed11b/g BSS.
    for(uint8_t hostRate:{uint8_t(1),uint8_t(2),uint8_t(3)}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        CHECK(rtw_8814au_build_rate_config(0xfff,nullptr,2,0,0,0,&f->c.acRateConfig_));
        f->c.acRateReady_=f->c.acMediaConnected_=true;
        f->n.ni_txrate=hostRate;f->c.core_.sc_ic.ic_flags|=IEEE80211_F_SHPREAMBLE;
        CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);
        CHECK(word(f->c.last_transfer,4)==11 && !(word(f->c.last_transfer,5)&0x10U));
        CHECK(!f->c.rateControl_.requests && !word(f->c.last_transfer,2));
    }
    // Group and protected group EAPOL remain explicit at the lowest basic
    // rate. The software crypto boundary moves SNAP and retains IV/MIC bytes.
    for(bool eapol:{false,true})for(bool protect:{false,true})for(bool multicast:{false,true}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        CHECK(rtw_8814au_build_rate_config(0xfff,nullptr,2,0,0,0,&f->c.acRateConfig_));
        f->c.acRateReady_=f->c.acMediaConnected_=true;
        CHECK(f->c.sendFrame(packet(0,false,eapol,protect,multicast),&f->n)==0);
        const auto&b=f->c.last_transfer;const bool fw=!eapol&&!multicast;
        CHECK((word(b,3)&0x500U)==(fw?0U:0x500U));CHECK(word(b,4)==(fw?11U:0U));
        CHECK(bool(word(b,0)&0x01000000U)==multicast);
        CHECK(((word(b,1)>>8)&31U)==(multicast?18U:0U));
        CHECK(f->c.last_pipe==(multicast?0U:2U)); /* QSEL18→HIGH, QSEL0→LOW. */
        CHECK(!word(b,2) && !f->c.rateControl_.requests && !f->c.rateControl_.pendingReports);
        CHECK(b.size()==88U+(protect?16U:0U));CHECK(effects.encryptions==unsigned(protect));
        if(protect)CHECK(b[40+24]==0xcc && b[40+32]==uint8_t(0xaa^0x5a));
        else CHECK(b[40+24]==0xaa);
        safe_descriptor(b);
    }
    {auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        CHECK(f->c.sendFrame(packet(0,false,false,false,false,0),&f->n)==0);
        CHECK(word(f->c.last_transfer,0)==0x84280030U && word(f->c.last_transfer,3)==0x500U);
        CHECK(word(f->c.last_transfer,4)==0 && f->c.last_pipe==0);safe_descriptor(f->c.last_transfer);
    }
    // Many submissions never request SPE, even if stale host AMRR state is active.
    {auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        CHECK(rtw_8814au_build_rate_config(0xfff,nullptr,2,0,0,0,&f->c.acRateConfig_));
        f->c.acRateReady_=f->c.acMediaConnected_=true;
        for(unsigned i=0;i<64;++i) {CHECK(f->c.sendFrame(packet(0,false),&f->n)==0);CHECK(!word(f->c.last_transfer,2));}
        CHECK(f->c.rateControl_.requests==0 && f->c.rateControl_.pendingReports==0 && f->c.rateControl_.htSampleCounter==0);
        CHECK(effects.frees==64 && effects.releases==64 && effects.phy_updates==0 && effects.live==0);
    }
    // Firmware readiness/media/RUN controls are distinct from the 92E path.
    for(unsigned gate=0;gate<5;++gate) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        f->c.acRateReady_=f->c.acMediaConnected_=true;
        if(gate==0)f->c.acRateReady_=false;
        if(gate==1)f->c.acMediaConnected_=false;
        if(gate==2)f->c.running_=false;
        if(gate==3)f->c.stopping_=true;
        if(gate==4)f->c.core_.sc_ic.ic_state=0;
        CHECK(!f->c.uses8814FirmwareRate() && !f->c.usesFirmwareRate());
    }
    for(int delta:{-1,0,1}) {
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        size_t payload=size_t(512-40-24+delta);
        CHECK(f->c.sendFrame(packet(0,false,false,false,false,IEEE80211_FC0_TYPE_DATA,payload),&f->n)==0);
        size_t raw=40+24+payload;CHECK(f->c.last_transfer.size()==raw+(raw%512==0?1:0));
    }
    {auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        CHECK(f->c.sendFrame(packet(4,true),&f->n)==0 && f->c.submitted==1 && effects.live==0);}
}
static void ac8814_radio_not_ready(){
    for(bool failed_io:{false,true}){
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        if(failed_io)f->c.acCore_.io_error=EIO;else f->c.acCore_.phy_initialized=0;
        CHECK(f->c.sendFrame(packet(0,false),&f->n)==ENETDOWN);
        CHECK(!f->c.submitted&&!effects.encryptions&&effects.live==0);
    }
}
static void ac8814_ht20_tx(){
    for(bool band5g:{false,true}){
        auto f=std::make_unique<fixture>();legacy_profile(*f,RTW_WIFI_RTL8814AU);
        const uint8_t mcs[2]={0xff,0xff};
        CHECK(rtw_8814au_build_rate_config(band5g?0xff0:0xfff,mcs,2,1,1,1,&f->c.acRateConfig_));
        f->c.acRateReady_=f->c.acMediaConnected_=true;f->n.ni_flags=IEEE80211_NODE_HT;
        ieee80211_channel channel{band5g?IEEE80211_CHAN_5GHZ:0};f->n.ni_chan=&channel;
        CHECK(f->c.sendFrame(packet(5),&f->n)==0);
        const auto&b=f->c.last_transfer;
        CHECK((word(b,4)&127)==27);CHECK(word(b,5)&0x10);
        CHECK(((word(b,1)>>16)&31)==(band5g?4:2));
        CHECK(!(word(b,3)&0x500));CHECK(((word(b,1)>>8)&31)==5);
        CHECK(f->c.txHTPackets_==1&&effects.phy_updates==0);
        safe_descriptor(b);
    }
}
static void ac8814_ht_fixture(fixture&f,bool five=false) {
    legacy_profile(f,RTW_WIFI_RTL8814AU);
    const uint8_t mcs[2]={255,255};
    CHECK(rtw_8814au_build_rate_config(five?0xff0:0xfff,mcs,2,1,1,1,&f.c.acRateConfig_));
    f.c.acRateReady_=f.c.acMediaConnected_=true;f.n.ni_flags=IEEE80211_NODE_HT;
    f.c.acCore_.powered=f.c.acCore_.fw_running=f.c.acCore_.mac_initialized=1;
    f.c.acCore_.bus.write=host_ac_write;
}
static void ac8814_ampdu_ba() {
    // Pinned Linux rtw8814a_set_ampdu_factor: peer HT factors cap byte length
    // at 8191/16383/32767/65535, with ONE checked 32-bit write to 0x458.
    const uint32_t limits[]={8191,16383,32767,65535};
    for(unsigned factor=0;factor<4;++factor)for(unsigned density=0;density<8;++density) {
        fixture f;ac8814_ht_fixture(f);auto&ic=f.c.core_.sc_ic;
        f.n.ni_ampdu_param=uint8_t(factor|(density<<2));f.n.ni_tx_ba[5].ba_winsize=16;
        CHECK(RTWUSBController::ampduTXStart(&ic,&f.n,5)==0);
        CHECK(effects.ac_writes.size()==1);
        CHECK((effects.ac_writes[0]==std::array<uint32_t,3>{0x458,4,limits[factor]}));
        CHECK(!f.c.core_.ampdu_calls); // Never use EU register programming/H2C.
    }
    for(unsigned gate=0;gate<16;++gate) {
        fixture f;ac8814_ht_fixture(f);auto&ic=f.c.core_.sc_ic;ieee80211_node other;
        f.n.ni_tx_ba[3].ba_winsize=16;auto*peer=&f.n;
        switch(gate) {
        case 0:f.c.acRateReady_=false;break;case 1:f.c.acMediaConnected_=false;break;
        case 2:f.c.running_=false;break;case 3:f.c.stopping_=true;break;
        case 4:ic.ic_state=0;break;case 5:f.n.ni_port_valid=false;break;
        case 6:peer=&other;break;case 7:ic.ic_tid_noack=1U<<3;break;
        case 8:f.n.ni_tx_ba[3].ba_winsize=0;break;case 9:f.n.ni_tx_ba[3].ba_winsize=65;break;
        case 10:f.c.acRateConfig_.ht=0;break;case 11:f.c.acCore_.phy_initialized=0;break;
        case 12:f.c.acCore_.io_error=EIO;break;
        case 13:f.c.present_=false;break;case 14:f.c.usbError_=1;break;case 15:f.n.ni_flags=0;break;
        }
        CHECK(RTWUSBController::ampduTXStart(&ic,peer,3)!=0);CHECK(effects.ac_writes.empty());
    }
    for(uint8_t tid:{uint8_t(8),uint8_t(15)}) {
        fixture f;ac8814_ht_fixture(f);f.n.ni_tx_ba[tid].ba_winsize=16;
        CHECK(RTWUSBController::ampduTXStart(&f.c.core_.sc_ic,&f.n,tid)==EINVAL);CHECK(effects.ac_writes.empty());
    }
    {fixture f;ac8814_ht_fixture(f);auto&ic=f.c.core_.sc_ic;f.n.ni_tx_ba[1].ba_winsize=16;
     effects.ac_write_error=EIO;
     CHECK(RTWUSBController::ampduTXStart(&ic,&f.n,1)==EIO);
     CHECK(f.c.acCore_.io_error==EIO&&effects.ac_writes.size()==1);
     CHECK(RTWUSBController::ampduTXStart(&ic,&f.n,1)!=0&&effects.ac_writes.size()==1);
     CHECK(f.c.sendFrame(packet(1),&f.n)==ENETDOWN&&!f.c.submitted&&effects.live==0);}
}
static void ac8814_ampdu_tx() {
    for(bool five:{false,true})for(unsigned tid=0;tid<8;++tid)for(unsigned winsize:{1U,7U,16U,64U}) {
        fixture f;ac8814_ht_fixture(f,five);ieee80211_channel channel{five?IEEE80211_CHAN_5GHZ:0};f.n.ni_chan=&channel;
        auto&ba=f.n.ni_tx_ba[tid];ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=winsize;ba.ba_timeout_val=5000;
        f.n.ni_ampdu_param=uint8_t(3U|((tid&7U)<<2));
        CHECK(f.c.sendFrame(packet(tid,true,false,true),&f.n)==0);const auto&b=f.c.last_transfer;
        // Primary MAX_AGG_NUM counts PAIRS of MPDUs. A one-frame window
        // cannot safely enable aggregation; never exceed the live BA window.
        const unsigned pairs=winsize==1?0:winsize==7?3:winsize==16?8:16;
        CHECK(word(b,2)==(pairs?(0x1000U|(tid<<20)):0));
        CHECK(word(b,3)==(pairs<<17));
        CHECK((word(b,4)&127U)==27U&&word(b,5)==0x10U);
        CHECK(f.c.txAMPDUPackets_==unsigned(pairs!=0)&&ba.ba_to.calls==unsigned(pairs!=0));
        if(pairs)CHECK(ba.ba_to.last_usec==5000);
        CHECK(effects.ac_writes.empty()&&effects.encryptions==1&&effects.live==0);safe_descriptor(b);
        ba.ba_state=0;CHECK(f.c.sendFrame(packet(tid),&f.n)==0);
        CHECK(!word(f.c.last_transfer,2)&&!(word(f.c.last_transfer,3)&0x3e0000U));
        CHECK(f.c.txAMPDUPackets_==unsigned(pairs!=0)&&ba.ba_to.calls==unsigned(pairs!=0)); // Live DELBA, no cached enable.
    }
    const unsigned peerPairs[]={3,7,15,16};
    for(unsigned factor=0;factor<4;++factor) {
        fixture f;ac8814_ht_fixture(f);f.n.ni_ampdu_param=uint8_t(factor);
        auto&ba=f.n.ni_tx_ba[3];ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=64;
        CHECK(f.c.sendFrame(packet(3),&f.n)==0);
        CHECK(word(f.c.last_transfer,3)==peerPairs[factor]<<17);
    }
    for(unsigned exclusion=0;exclusion<18;++exclusion) {
        fixture f;ac8814_ht_fixture(f);auto&ba=f.n.ni_tx_ba[3];ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=16;
        auto*m=exclusion==0?packet(3,false):exclusion==1?packet(3,true,true,true):exclusion==2?packet(3,true,false,false,true):
            exclusion==3?packet(3,false,false,false,false,0):packet(3);
        auto*peer=&f.n;ieee80211_node other=f.n;
        switch(exclusion) {
        case 4:m->bytes[1]|=4;break;case 5:m->bytes[22]|=1;break;
        case 6:m->bytes[24]|=0x20;break;case 7:f.c.core_.sc_ic.ic_tid_noack=1U<<3;break;
        case 8:ba.ba_state=1;break;case 9:ba.ba_winsize=0;break;case 10:ba.ba_winsize=65;break;
        case 11:f.c.acRateReady_=false;break;case 12:f.c.core_.sc_ic.ic_state=0;break;
        case 13:m->bytes[0]|=0x40;break; // QoS null is not aggregate data.
        case 14:f.n.ni_port_valid=false;break;case 15:peer=&other;break;
        case 16:f.n.ni_flags=0;break;case 17:m->bytes[24]|=0x40;break;
        }
        CHECK(f.c.sendFrame(m,peer)==0);CHECK(!word(f.c.last_transfer,2));
        CHECK(!(word(f.c.last_transfer,3)&0x3e0000U)&&!f.c.txAMPDUPackets_&&!ba.ba_to.calls);
        CHECK(effects.live==0);
    }
    {fixture f;ac8814_ht_fixture(f);auto&ba=f.n.ni_tx_ba[6];ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=64;
     f.c.core_.sc_ic.ic_edca_ac[EDCA_AC_VO].ac_acm=1;
     CHECK(f.c.sendFrame(packet(6),&f.n)==0);CHECK(!word(f.c.last_transfer,2));
     CHECK(((word(f.c.last_transfer,1)>>8)&31U)==5U&&f.c.last_transfer[64]==6U);}
    {fixture f;ac8814_ht_fixture(f);auto&ic=f.c.core_.sc_ic;
     auto&first=f.n.ni_tx_ba[3];auto&second=f.n.ni_tx_ba[5];
     first.ba_state=second.ba_state=IEEE80211_BA_AGREED;first.ba_winsize=second.ba_winsize=16;
     CHECK(f.c.sendFrame(packet(3),&f.n)==0&&(word(f.c.last_transfer,2)&0x1000U));
     RTWUSBController::ampduTXStop(&ic,&f.n,3);first.ba_state=0; // net80211 DELBA commits after callback.
     CHECK(f.c.sendFrame(packet(3),&f.n)==0&&!word(f.c.last_transfer,2));
     CHECK(f.c.sendFrame(packet(5),&f.n)==0&&(word(f.c.last_transfer,2)&0x1000U));
     CHECK(second.ba_state==IEEE80211_BA_AGREED&&f.c.txAMPDUPackets_==2);
     f.c.acRateReady_=f.c.acMediaConnected_=false;ic.ic_state=0;
     CHECK(f.c.sendFrame(packet(5),&f.n)==0&&!word(f.c.last_transfer,2));
     // Reassociation resets host BA; restoring firmware RUN alone cannot
     // reactivate aggregation. A stale node's agreement is never inherited.
     second.ba_state=0;ic.ic_state=IEEE80211_S_RUN;f.c.acRateReady_=f.c.acMediaConnected_=true;
     CHECK(f.c.sendFrame(packet(5),&f.n)==0&&!word(f.c.last_transfer,2));
     ieee80211_node next=f.n;ic.ic_bss=&next;second.ba_state=IEEE80211_BA_AGREED;
     CHECK(f.c.sendFrame(packet(5),&f.n)==0&&!word(f.c.last_transfer,2));
     CHECK(f.c.sendFrame(packet(5),&next)==0&&!word(f.c.last_transfer,2));
     next.ni_ampdu_param=0;next.ni_tx_ba[5].ba_winsize=64;
     CHECK(RTWUSBController::ampduTXStart(&ic,&next,5)==0);
     CHECK(effects.ac_writes.size()==1&&effects.ac_writes[0][2]==8191);
     next.ni_tx_ba[5].ba_state=IEEE80211_BA_AGREED;
     CHECK(f.c.sendFrame(packet(5),&next)==0&&word(f.c.last_transfer,3)==(3U<<17));
     CHECK(f.c.txAMPDUPackets_==3&&effects.ac_writes.size()==1&&effects.live==0);}
    {fixture f;ac8814_ht_fixture(f);auto&ba=f.n.ni_tx_ba[3];ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=16;ba.ba_timeout_val=5000;
     f.c.submit_error=EIO;CHECK(f.c.sendFrame(packet(3),&f.n)==EIO);
     CHECK(!f.c.txAMPDUPackets_&&!ba.ba_to.calls&&!effects.releases&&effects.live==0);}
}
int main(int argc,char**argv){
    // Removing common DFS admission before family dispatch leaks blocked
    // management/data to a backend (including its already queued USB path).
    for(auto family:{RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU,RTW_WIFI_RTL8192DU}){
        auto f=std::make_unique<fixture>();f->c.family_=family;f->c.dfsAllowed_=false;
        CHECK(f->c.sendFrame(packet(0,false,false,false,false,0),&f->n)==EACCES);
        CHECK(f->c.submitted==0&&effects.frees==1&&effects.live==0&&effects.releases==0);
    }

    if(argc>1&&!std::strcmp(argv[1],"au-ba")){ac8814_ampdu_ba();std::printf("AU native BA: %u checks passed\n",checks);return 0;}
    if(argc>1&&!std::strcmp(argv[1],"au-ampdu")){ac8814_ampdu_tx();std::printf("AU TX AMPDU: %u checks passed\n",checks);return 0;}
    ac8814_ampdu_ba();ac8814_ampdu_tx();
 {fixture f;f.c.family_=RTW_WIFI_RTL8723BU;f.n.ni_port_valid=true;f.c.r23FirmwareReady_=true;f.c.r23Rates_.mask=0xff010;auto&ic=f.c.core_.sc_ic;auto&ba=f.n.ni_rx_ba[3];ba.ba_buf=&f;ba.ba_winsize=32;CHECK(RTWUSBController::ampduRXStart(&ic,&f.n,3)==0);CHECK(RTWUSBController::ampduTXStart(&ic,&f.n,3)==EINVAL&&f.c.core_.ampdu_calls==0);CHECK(f.c.sendFrame(packet(),&f.n)==ENOTSUP&&f.c.r23TXCalls==1&&f.c.submitted==0);}

 {fixture f;f.c.family_=RTW_WIFI_RTL8710BU;f.n.ni_port_valid=true;f.c.buFirmwareReady_=f.c.buHT_=true;auto&ic=f.c.core_.sc_ic;auto&ba=f.n.ni_rx_ba[3];ba.ba_buf=&f;ba.ba_winsize=32;CHECK(RTWUSBController::ampduRXStart(&ic,&f.n,3)==0);f.c.buHT_=false;CHECK(RTWUSBController::ampduRXStart(&ic,&f.n,3)==EINVAL);f.c.buHT_=true;CHECK(RTWUSBController::ampduTXStart(&ic,&f.n,3)==ENOTSUP&&f.c.buBACalls==1&&f.c.core_.ampdu_calls==0);CHECK(f.c.sendFrame(packet(),&f.n)==ENOTSUP&&f.c.buTXCalls==1&&f.c.submitted==0);}
 {fixture f;f.c.family_=RTW_WIFI_RTL8192FU;f.n.ni_port_valid=true;f.c.fuFirmwareReady_=f.c.fuHT_=true;auto&ic=f.c.core_.sc_ic;auto&ba=f.n.ni_rx_ba[3];ba.ba_buf=&f;ba.ba_winsize=32;CHECK(RTWUSBController::ampduRXStart(&ic,&f.n,3)==0);f.c.fuHT_=false;CHECK(RTWUSBController::ampduRXStart(&ic,&f.n,3)==EINVAL);f.c.fuHT_=true;CHECK(RTWUSBController::ampduTXStart(&ic,&f.n,3)==ENOTSUP&&f.c.fuBACalls==1&&f.c.core_.ampdu_calls==0);CHECK(f.c.sendFrame(packet(),&f.n)==ENOTSUP&&f.c.fuTXCalls==1&&f.c.submitted==0);}

    gen1_station_tx_paths();ac8814_station_tx_paths();ac8814_radio_not_ready();ac8814_ht20_tx();
    const unsigned endpoint[8]={2,2,2,2,1,1,0,0};
    for(unsigned tid=0;tid<8;++tid){
        auto f=std::make_unique<fixture>();CHECK(f->c.sendFrame(packet(tid),&f->n)==0);
        const auto&b=f->c.last_transfer;CHECK(f->c.last_pipe==endpoint[tid]);CHECK(((word(b,1)>>8)&31U)==tid);
        CHECK(((word(b,1)>>16)&15U)==2);CHECK(!(word(b,3)&0x100U));CHECK((word(b,4)&0x7fU)==27);
        CHECK((word(b,2)&0x11000U)==0x10000U);CHECK(!(word(b,3)&0x3800U));
        CHECK(b[40+24]==tid&&b[40+22]==0x30&&b[40+23]==0x12);safe_descriptor(b);
        CHECK(effects.frees==1&&effects.live==0&&effects.releases==1&&f->c.txHTPackets_==1);
    }
    {auto f=std::make_unique<fixture>();auto&ic=f->c.core_.sc_ic;
     ic.ic_edca_ac[EDCA_AC_VO].ac_acm=1;ic.ic_edca_ac[EDCA_AC_VI].ac_acm=1;
     CHECK(f->c.sendFrame(packet(6),&f->n)==0);CHECK(f->c.last_pipe==2&&((word(f->c.last_transfer,1)>>8)&31U)==0);
     CHECK(f->c.last_transfer[64]==6);safe_descriptor(f->c.last_transfer);}
    for(unsigned mode=0;mode<3;++mode){auto f=std::make_unique<fixture>();
        auto*m=mode==0?packet(0,false,false,false,false,0):packet(0,false,false,false,mode==1);
        if(mode==2)f->n.ni_flags=0;
        CHECK(f->c.sendFrame(m,&f->n)==0);const auto&b=f->c.last_transfer;
        CHECK(word(b,3)&0x100U);CHECK(!(word(b,2)&0x1000U));CHECK(f->c.txHTPackets_==0);
        CHECK(((word(b,1)>>8)&31U)==(mode==2?0:0x12));CHECK((word(b,4)&0x7fU)==(mode==2?11:0));
        CHECK(bool(word(b,0)&0x01000000U)==(mode==1));safe_descriptor(b);
    }
    for(unsigned policy:{0U,0x20U,0x40U,0x60U}){auto f=std::make_unique<fixture>();auto*m=packet(2);
        m->bytes[24]|=uint8_t(policy);auto&ba=f->n.ni_tx_ba[2];ba.ba_state=policy?IEEE80211_BA_AGREED:1;ba.ba_winsize=16;
        CHECK(f->c.sendFrame(m,&f->n)==0);CHECK((word(f->c.last_transfer,2)&0x11000U)==0x10000U);
        CHECK(f->c.txAMPDUPackets_==0&&f->c.last_transfer[64]==uint8_t(2U|policy));
    }
    for(unsigned winsize:{1U,8U,16U,32U,64U}){
        auto f=std::make_unique<fixture>();auto&ba=f->n.ni_tx_ba[5];ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=winsize;ba.ba_timeout_val=5000;
        CHECK(f->c.sendFrame(packet(5),&f->n)==0);const auto&b=f->c.last_transfer;
        CHECK((word(b,2)&0x11000U)==0x1000U);CHECK(((word(b,2)>>20)&7U)==7);
        CHECK(((word(b,3)>>17)&31U)==std::min(16U,winsize));CHECK((word(b,3)&0x3800U)==0U);
        CHECK(f->c.txAMPDUPackets_==1&&ba.ba_to.calls==1&&ba.ba_to.last_usec==5000);safe_descriptor(b);
    }
    for(unsigned density=0;density<8;++density){auto f=std::make_unique<fixture>();
        f->n.ni_ampdu_param=uint8_t(3U|(density<<2));f->n.ni_tx_ba[4].ba_state=IEEE80211_BA_AGREED;f->n.ni_tx_ba[4].ba_winsize=7;
        CHECK(f->c.sendFrame(packet(4),&f->n)==0);CHECK(((word(f->c.last_transfer,2)>>20)&7U)==density);
        CHECK(((word(f->c.last_transfer,3)>>17)&31U)==7);
    }
    for(bool qos:{false,true})for(bool protect:{false,true}){
        auto f=std::make_unique<fixture>();f->n.ni_tx_ba[0].ba_state=IEEE80211_BA_AGREED;f->n.ni_tx_ba[0].ba_winsize=16;
        f->c.rateControl_.htSampleCounter=31;
        CHECK(f->c.sendFrame(packet(0,qos,true,protect),&f->n)==0);const auto&b=f->c.last_transfer;
        CHECK((word(b,3)&0x100U)&&!(word(b,2)&0x1000U)&&!(word(b,2)&0x80000U));
        CHECK((word(b,4)&0x7fU)==0);CHECK(f->c.txHTPackets_==0&&f->c.txAMPDUPackets_==0&&f->c.rateControl_.requests==0);
        CHECK(effects.encryptions==unsigned(protect)&&effects.live==0&&effects.frees==1&&effects.releases==1);
        CHECK((word(b,0)&0xffffU)==(qos?50U:48U)+(protect?16U:0U));safe_descriptor(b);
    }
    {auto f=std::make_unique<fixture>();CHECK(f->c.sendFrame(packet(4,true,false,true),&f->n)==0);const auto&b=f->c.last_transfer;
     CHECK(!(word(b,3)&0x100U)&&((word(b,1)>>8)&31U)==4);CHECK(effects.encryptions==1);safe_descriptor(b);}
    for(unsigned width:{20U,40U})for(unsigned direction:{1U,3U})for(bool sgi:{false,true})for(bool aggregate:{false,true}){auto f=std::make_unique<fixture>();
        f->c.core_.rate_config.bw40=f->c.core_.channel_bw40=uint8_t(width==40);f->n.ni_htop0=direction;
        f->c.core_.rate_config.sgi20=uint8_t(width==20&&sgi);f->c.core_.rate_config.sgi40=uint8_t(width==40&&sgi);
        if(aggregate){f->n.ni_tx_ba[0].ba_state=IEEE80211_BA_AGREED;f->n.ni_tx_ba[0].ba_winsize=8;}
        CHECK(f->c.sendFrame(packet(),&f->n)==0);
        // Independent primary 92E setters: DATA_BW=DW5[6:5], DATA_SHORT=DW5[4],
        // DATA_SC=DW5[3:0]; full HT40 is SC0 for either primary direction.
        CHECK(word(f->c.last_transfer,5)==((width==40?0x20U:0U)|(sgi?0x10U:0U)));
        CHECK(word(f->c.last_transfer,4)==0x08001f1bU);
        CHECK(bool(word(f->c.last_transfer,2)&0x1000U)==aggregate);
        safe_descriptor(f->c.last_transfer);
        const unsigned narrow=width==40?(direction==1U?2U:1U):0U;
        for(unsigned kind=0;kind<4;++kind){
            auto*m=kind==0?packet(0,false):kind==1?packet(0,true,true):kind==2?packet(0,false,false,false,false,0):packet(0,true,true,true);
            CHECK(f->c.sendFrame(m,&f->n)==0);CHECK(word(f->c.last_transfer,5)==narrow);
            CHECK(word(f->c.last_transfer,3)&0x100U);CHECK(!(word(f->c.last_transfer,2)&0x1000U));
            CHECK(word(f->c.last_transfer,4)==(kind==2?0x001a0000U:0x08001f00U|(kind==0?11U:0U)));
            safe_descriptor(f->c.last_transfer);
        }
    }
    for(unsigned mode:{1U,3U}){auto f=std::make_unique<fixture>();f->n.ni_htop1=mode;
        CHECK(f->c.sendFrame(packet(),&f->n)==0);CHECK((word(f->c.last_transfer,3)&0x3800U)==0x3000U);}
    {auto f=std::make_unique<fixture>();f->c.core_.rate_config.bw40=1;f->n.ni_htop0=1;f->n.ni_htop1=2;
     CHECK(f->c.sendFrame(packet(),&f->n)==0);CHECK((word(f->c.last_transfer,3)&0x3800U)==0x3000U);}
    {auto f=std::make_unique<fixture>();f->c.core_.sc_ic.ic_rtsthreshold=32;
     CHECK(f->c.sendFrame(packet(),&f->n)==0);CHECK((word(f->c.last_transfer,3)&0x3800U)==0x3000U);}
    for(unsigned mode:{unsigned(IEEE80211_PROT_CTSONLY),unsigned(IEEE80211_PROT_RTSCTS)}){
        auto f=std::make_unique<fixture>();f->c.core_.sc_ic.ic_flags|=IEEE80211_F_USEPROT;f->c.core_.sc_ic.ic_protmode=mode;
        CHECK(f->c.sendFrame(packet(),&f->n)==0);CHECK((word(f->c.last_transfer,3)&0x3800U)==(mode==IEEE80211_PROT_CTSONLY?0x800U:0x3000U));
    }
    {auto f=std::make_unique<fixture>();for(unsigned i=1;i<=32;++i){CHECK(f->c.sendFrame(packet(7),&f->n)==0);
        CHECK(bool(word(f->c.last_transfer,2)&0x80000U)==(i==32));}
     CHECK(f->c.rateControl_.requests==1&&f->c.rateControl_.pendingReports==1&&f->c.rateControl_.requestedSinceReset);
     CHECK(f->c.txHTPackets_==32&&effects.frees==32&&effects.releases==32&&effects.live==0);}
    {auto f=std::make_unique<fixture>();f->c.submit_error=EIO;f->c.rateControl_.htSampleCounter=31;auto&ba=f->n.ni_tx_ba[0];
     ba.ba_state=IEEE80211_BA_AGREED;ba.ba_winsize=16;ba.ba_timeout_val=5;
     CHECK(f->c.sendFrame(packet(),&f->n)==EIO);CHECK(effects.frees==1&&effects.live==0&&effects.releases==0);
     CHECK(f->c.txHTPackets_==0&&f->c.txAMPDUPackets_==0&&f->c.rateControl_.requests==0&&ba.ba_to.calls==0);}
    for(unsigned kind=0;kind<10;++kind){auto f=std::make_unique<fixture>();auto*m=packet();int expected=EINVAL;
        switch(kind){case 0:f->c.running_=false;expected=ENETDOWN;break;case 1:f->c.stopping_=true;expected=ENETDOWN;break;
          case 2:f->c.present_=false;expected=ENETDOWN;break;case 3:f->c.usbError_=1;expected=ENETDOWN;break;
          case 4:m->bytes.resize(23);break;case 5:m->bytes[24]=8;break;case 6:m->bytes[1]=3;break;
          case 7:f->n.ni_rates.rs_nrates=255;break;case 8:f->c.ledger_.pending=0xfff0U;expected=ENOBUFS;break;
          case 9:f->c.core_.ac2idx[0]=3;break;}
        CHECK(f->c.sendFrame(m,&f->n)==expected);CHECK(effects.frees==1&&effects.live==0&&effects.releases==0&&f->c.submitted==0);
    }
    for(unsigned kind=0;kind<3;++kind){auto f=std::make_unique<fixture>();
        if(kind==0)effects.key_missing=true;else if(kind==1)effects.cipher=1;else effects.encrypt_failure=true;
        CHECK(f->c.sendFrame(packet(0,true,false,true),&f->n)==(kind==2?ENOBUFS:EINVAL));
        CHECK(effects.frees==1&&effects.live==0&&effects.releases==0&&f->c.submitted==0);
    }
    {auto f=std::make_unique<fixture>();auto&ic=f->c.core_.sc_ic;auto&rx=f->n.ni_rx_ba[3];rx.ba_buf=&rx;rx.ba_winsize=64;
     CHECK(RTWUSBController::ampduRXStart(&ic,&f->n,3)==0);rx.ba_buf=nullptr;CHECK(RTWUSBController::ampduRXStart(&ic,&f->n,3)==EINVAL);
     auto&tx=f->n.ni_tx_ba[3];tx.ba_winsize=32;CHECK(RTWUSBController::ampduTXStart(&ic,&f->n,3)==0);
     CHECK(f->c.core_.ampdu_calls==1&&f->c.core_.last_factor==3&&f->c.core_.last_density==7);
     ic.ic_tid_noack=1U<<3;CHECK(RTWUSBController::ampduTXStart(&ic,&f->n,3)==EINVAL);CHECK(f->c.core_.ampdu_calls==1);ic.ic_tid_noack=0;
     f->c.core_.ampdu_error=EIO;CHECK(RTWUSBController::ampduTXStart(&ic,&f->n,3)==EIO);
     tx.ba_winsize=65;CHECK(RTWUSBController::ampduTXStart(&ic,&f->n,3)==EINVAL);
     tx.ba_winsize=32;f->n.ni_port_valid=false;CHECK(RTWUSBController::ampduTXStart(&ic,&f->n,3)==EINVAL);
     CHECK(RTWUSBController::ampduTXStart(&ic,&f->n,16)==EINVAL&&RTWUSBController::ampduRXStart(nullptr,&f->n,0)==EINVAL);
     f->n.ni_tx_ba[7].ba_state=IEEE80211_BA_AGREED;RTWUSBController::ampduTXStop(&ic,&f->n,3);RTWUSBController::ampduRXStop(&ic,&f->n,3);
     CHECK(f->n.ni_tx_ba[7].ba_state==IEEE80211_BA_AGREED);}
    CHECK(effects.live==0);std::printf("HT TX: %u checks passed (actual sendFrame, EDCA mapping and BA callbacks; offline transport/crypto boundaries)\n",checks);
}
