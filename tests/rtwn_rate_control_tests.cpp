#include "rtwn_rate_control_host.hpp"
#include <array>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
static uint32_t le32(const uint8_t *p) { return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24); }
static void put32(uint8_t *p,uint32_t n) { for(unsigned i=0;i<4;++i)p[i]=uint8_t(n>>(8*i)); }
static std::array<uint8_t,10> fixture(uint8_t seq,uint8_t flags=0,uint8_t retries=0,uint8_t rate=0) {
    /* Public 92E CCX layout, synthetic event: ID, seq, flags/QSEL, MACID,
     * retries, queue LE16, final HW rate and two reserved bytes. */
    return {{3,seq,flags,0,retries,0x34,0x12,rate,0,0}};
}
static void fresh(RTWUSBController &c,ieee80211_node &n) {
    n.ni_rates={4,{2,4,11,22}}; n.ni_txrate=0; n.ni_port_valid=true;
    c.core_.sc_ic.ic_bss=&n; c.rateTransportFresh_=true; c.resetRateControl();
}
static void sent(RTWUSBController &c) {
    ++c.rateControl_.requests; ++c.rateControl_.pendingReports;
    c.rateControl_.requestedSinceReset=true;
}
static void reports(RTWUSBController &c,unsigned count,uint8_t &seq,uint8_t retries=0,uint8_t flags=0) {
    for(unsigned i=0;i<count;++i) { sent(c); auto b=fixture(seq++,flags,retries); c.handleTxReport(b.data(),b.size()); }
}
static std::array<uint8_t,8> fixture88e(uint8_t ok=1,uint8_t retries=0,uint8_t flags=0,uint8_t rate=0) {
    return {{0x80,uint8_t(ok?0x40:0),uint8_t(retries|flags),0x34,0x12,rate,0x00,0x00}};
}
static void legacy_ownership() {
    RTWUSBController cu;ieee80211_node station;cu.family_=RTW_WIFI_RTL8192CU;fresh(cu,station);
    station.ni_txrate=3;cu.resetRateControl();
    CHECK(station.ni_txrate==3);CHECK(!cu.rateControl_.active);
    CHECK(cu.usesLegacyFirmwareRate()&&!cu.usesFirmwareRate());
    cu.rateControl_.node.amn_txcnt=11;cu.rateControl_.node.amn_retrycnt=11;
    cu.chooseRateControl();CHECK(station.ni_txrate==3&&cu.rateControl_.changes==0);
    // A second calibration tick with unchanged counters must not publish
    // duplicate OSNumber properties into IORegistry.
    const auto publishedOnce=cu.propertyWrites;
    CHECK(publishedOnce>=20);
    cu.chooseRateControl();
    CHECK(cu.propertyWrites==publishedOnce);
    ++cu.rateControl_.reports;
    cu.chooseRateControl();
    CHECK(cu.propertyWrites==publishedOnce+1);
    --cu.rateControl_.reports; // Restore fixture state after the synthetic counter change.
    CHECK(cu.properties["RTWDiagFirmwareRateControl"]==1);
    CHECK(cu.properties["RTWDiagSiliconFamily"]==RTW_WIFI_RTL8192CU);
    CHECK(cu.properties["RTWDiagHTNegotiated"]==0&&cu.properties["RTWDiagTXRateFeedbackActive"]==0);
    CHECK(cu.properties["RTWDiagTXRate500Kbps"]==0);
    // A CU USB completion or another family's report cannot fabricate AMRR.
    sent(cu);auto gen2=fixture(1);auto gen1=fixture88e();
    cu.handleTxReport(gen2.data(),gen2.size());cu.handleTxReport(gen1.data(),gen1.size());
    CHECK(cu.rateControl_.reports==0&&cu.rateControl_.node.amn_txcnt==11&&cu.rateControl_.node.amn_retrycnt==11);
    station.ni_port_valid=false;CHECK(cu.usesLegacyFirmwareRate()); // EAPOL chooses explicit rate at send boundary.
    cu.stopping_=true;CHECK(!cu.usesLegacyFirmwareRate());cu.stopping_=false;
    cu.running_=false;CHECK(!cu.usesLegacyFirmwareRate());cu.running_=true;
    cu.core_.sc_ic.ic_state=0;CHECK(!cu.usesLegacyFirmwareRate());cu.core_.sc_ic.ic_state=IEEE80211_S_RUN;
    cu.core_.sc_ic.ic_bss=nullptr;CHECK(!cu.usesLegacyFirmwareRate());cu.core_.sc_ic.ic_bss=&station;
    for(auto family:{RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU,RTW_WIFI_UNKNOWN}) {
        cu.family_=family;CHECK(!cu.usesLegacyFirmwareRate());
    }
    const uint8_t mcs[2]={0xff,0xff};CHECK(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&cu.core_.rate_config));
    station.ni_flags=IEEE80211_NODE_HT;cu.core_.fw_station_connected=1;
    cu.family_=RTW_WIFI_RTL8192CU;CHECK(!cu.usesFirmwareRate());
    cu.family_=RTW_WIFI_RTL8188EU;CHECK(!cu.usesFirmwareRate());
    cu.family_=RTW_WIFI_RTL8192EU;CHECK(cu.usesFirmwareRate());
}
static void feedback88e() {
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8188EU;fresh(c,n);
    CHECK(c.rateControl_.active&&!c.usesFirmwareRate()&&!c.usesLegacyFirmwareRate());
    auto event=fixture88e();
    // The same8-byte report is legal for successive requests: no C2H sequence exists.
    for(unsigned i=0;i<11;i++){sent(c);c.handleTxReport(event.data(),event.size());}
    CHECK(c.rateControl_.reports==11&&c.rateControl_.pendingReports==0);
    CHECK(c.rateControl_.repeatedSequences==0);
    CHECK(c.rateControl_.node.amn_txcnt==11&&c.rateControl_.node.amn_retrycnt==0);
    c.chooseRateControl();CHECK(n.ni_txrate==1);
    CHECK(c.properties["RTWDiagFirmwareRateControl"]==0&&c.properties["RTWDiagHTNegotiated"]==0);
    CHECK(c.properties["RTWDiagSiliconFamily"]==RTW_WIFI_RTL8188EU);
    // PKTOK=0 is an RF failure even without LIFE_EXPIRE or RETRY_OVER.
    event=fixture88e(0);for(unsigned i=0;i<11;i++){sent(c);c.handleTxReport(event.data(),event.size());}
    CHECK(c.rateControl_.failures==11&&c.rateControl_.retried==11);
    c.chooseRateControl();CHECK(n.ni_txrate==0);
    event=fixture88e(1,2);sent(c);c.handleTxReport(event.data(),event.size());
    CHECK(c.rateControl_.failures==11&&c.rateControl_.retried==12&&c.rateControl_.node.amn_retrycnt==1);
    event=fixture88e(0,3,0x80);sent(c);c.handleTxReport(event.data(),event.size());
    CHECK(c.rateControl_.failures==12&&c.rateControl_.retried==13);
    auto before=c.rateControl_.reports;auto pending=c.rateControl_.pendingReports;
    c.handleTxReport(event.data(),event.size());CHECK(c.rateControl_.reports==before&&c.rateControl_.pendingReports==pending);
    // A report from another MACID, multicast or unnegotiated rate never consumes a request.
    sent(c);pending=c.rateControl_.pendingReports;
    for(unsigned bad=0;bad<5;bad++) {auto b=fixture88e();
        if(bad==0)b[1]|=1;else if(bad==1)b[1]|=0x80;else if(bad==2)b[5]=11;else if(bad==3)b[6]=0x80;else b[2]=0x80;
        c.handleTxReport(b.data(),b.size());CHECK(c.rateControl_.reports==before&&c.rateControl_.pendingReports==pending);
    }
    auto gen2=fixture(7);c.handleTxReport(gen2.data(),gen2.size());CHECK(c.rateControl_.reports==before);
    auto good=fixture88e();n.ni_port_valid=false;c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before);n.ni_port_valid=true;
    n.ni_bssid[5]^=1;c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before);n.ni_bssid[5]^=1;
    c.resetRateControl();CHECK(!c.rateTransportFresh_&&!c.rateControl_.active);
    sent(c);c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before);
    c.rateControl_.requestedSinceReset=false;c.rateTransportFresh_=true;c.resetRateControl();CHECK(c.rateControl_.active);
    c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before); // no fresh request.
    sent(c);c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before+1);
}
static void ht_feedback88e() {
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8188EU;fresh(c,n);
    const uint8_t mcs[2]={0x81,0};CHECK(rtw_92c_build_rate_config(15,mcs,1,1,1,1,&c.core_.rate_config));
    n.ni_flags=IEEE80211_NODE_HT;c.resetRateControl();
    CHECK(c.usesLegacyHighThroughput()&&!c.usesFirmwareRate()&&!c.usesLegacyFirmwareRate());
    CHECK(c.legacyHostHTRate()==12);
    for(unsigned round=0;round<2;++round) {
        for(unsigned i=0;i<11;++i){sent(c);auto event=fixture88e(1,0,0,c.legacyHostHTRate());c.handleTxReport(event.data(),event.size());}
        c.chooseRateControl();
    }
    CHECK(c.legacyHostHTRate()==19&&c.rateControl_.changes==2);
    CHECK(c.properties["RTWDiagHTNegotiated"]==1&&c.properties["RTWDiagHTChannelWidthMHz"]==20);
    CHECK(c.properties["RTWDiagTXRate500Kbps"]==144&&c.properties["RTWDiagFirmwareRateControl"]==0);
    sent(c);auto unsupported=fixture88e(1,0,0,13);auto before=c.rateControl_.reports;
    c.handleTxReport(unsupported.data(),unsupported.size());CHECK(c.rateControl_.reports==before&&c.rateControl_.pendingReports==1);
    for(unsigned i=0;i<11;++i){if(i)sent(c);auto event=fixture88e(0,0,0,c.legacyHostHTRate());c.handleTxReport(event.data(),event.size());}
    c.chooseRateControl();CHECK(c.legacyHostHTRate()==3); // CCK11 fallback, not an unnegotiated MCS.
    CHECK(n.ni_txrate==0); // The HT ladder does not overwrite the net80211 legacy rate index.
    c.resetRateControl();CHECK(!c.rateTransportFresh_&&!c.rateControl_.active&&c.legacyHostHTRate()==12);
}
static void bc_operating_diagnostics() {
    for(unsigned width:{20U,40U,80U}) {
        RTWUSBController c;ieee80211_node n;fresh(c,n);c.family_=RTW_WIFI_RTL8822BU;
        c.r88FirmwareReady_=true;c.r88Rates_.rates.ht=1;c.r88Rates_.rates.sgi20=1;
        c.r88Rates_.vht=1;c.r88Rates_.width_mhz=uint8_t(width);
        c.publishRateControlDiagnostics();
        CHECK(c.properties["RTWDiagVHTNegotiated"]==1&&c.properties["RTWDiagHTChannelWidthMHz"]==width);
        CHECK(c.properties["RTWDiagHTSGI20Negotiated"]==(width==20));
        CHECK(c.properties["RTWDiagHTSGI40Negotiated"]==(width==40));
        CHECK(c.properties["RTWDiagVHTSGI80Negotiated"]==(width==80));
        c.core_.sc_ic.ic_state=0;c.publishRateControlDiagnostics();
        CHECK(c.properties["RTWDiagVHTNegotiated"]==0&&c.properties["RTWDiagHTChannelWidthMHz"]==0);
    }
}
static void family_rx_dispatch() {
    std::array<uint8_t,176> b{};put32(b.data(),24);put32(b.data()+8,0x10000000);
    put32(b.data()+12,4);b[24]=0x80;
    // CU ignores gen2 C2H bit28; second gen1 unit starts at128.
    put32(b.data()+128,24);put32(b.data()+140,11);b[152]=0x08;
    RTWUSBController c;c.family_=RTW_WIFI_RTL8192CU;rtw_rx_iterator it{};rtw_8192eu_rx_frame frame{};uint8_t kind=99;
    rtw_rx_iterator_init(&it,b.data(),b.size());CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);
    CHECK(kind==0&&frame.report_is_c2h==0&&frame.payload.mpdu==b.data()+24&&it.cursor==128);
    CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);CHECK(kind==0&&frame.payload.mpdu==b.data()+152);
    CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_DONE);
    // Same actual method retains gen2 eight-byte alignment and C2H classification.
    b.fill(0);put32(b.data(),10);put32(b.data()+8,0x10000000);auto ccx=fixture(19);std::memcpy(b.data()+24,ccx.data(),10);
    c.family_=RTW_WIFI_RTL8192EU;rtw_rx_iterator_init(&it,b.data(),40);
    CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);CHECK(kind==1&&frame.report_is_c2h&&it.cursor==40);
    CHECK(frame.payload.mpdu_length==10&&frame.payload.mpdu==b.data()+24);
    c.family_=RTW_WIFI_RTL8188EU;
    for(unsigned type=1;type<=3;type++) {
        b.fill(0);put32(b.data(),8);put32(b.data()+12,uint32_t(type)<<14);auto legacy=fixture88e();std::memcpy(b.data()+24,legacy.data(),8);
        rtw_rx_iterator_init(&it,b.data(),32);CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);
        CHECK(kind==type&&frame.payload.mpdu_length==8&&frame.payload.mpdu==b.data()+24);
    }
    for(auto family:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU}) {
        c.family_=family;
        // Foreign HT traffic is structurally valid USB data. Its radio-policy
        // rejection must not poison a legacy beacon later in the same transfer.
        for(auto mcs:{0U,3U,7U,15U}) {
            b.fill(0);put32(b.data(),24);put32(b.data()+12,0x40U|mcs);b[24]=0x08;
            put32(b.data()+128,24);put32(b.data()+140,0);b[152]=0x80;
            rtw_rx_iterator_init(&it,b.data(),b.size());
            CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);
            CHECK(kind==0&&frame.payload.rate_index==12U+mcs&&it.cursor==128);
            CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);
            CHECK(kind==0&&frame.payload.rate_index==0&&frame.payload.mpdu==b.data()+152&&frame.payload.mpdu[0]==0x80);
            CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_DONE);
        }
        b.fill(0);put32(b.data(),0x08500018);put32(b.data()+12,3);
        rtw_rx_iterator_init(&it,b.data(),48);CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_OK);
        CHECK(frame.security_code==5&&frame.sw_decrypt_required==1&&kind==0);
        rtw_rx_iterator_init(&it,b.data(),47);CHECK(c.decodeUSBFrame(&it,&frame,&kind)==RTW_PACKETS_TRUNCATED);
    }
}
static void ac8814_ownership_and_rx() {
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8814AU;fresh(c,n);
    n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active && n.ni_txrate==3);
    CHECK(rtw_8814au_build_rate_config(0xf,nullptr,2,0,0,0,&c.acRateConfig_));
    c.acRateReady_=c.acMediaConnected_=true;CHECK(c.uses8814FirmwareRate());
    CHECK(!c.usesFirmwareRate() && !c.usesLegacyFirmwareRate());
    c.rateControl_.node.amn_txcnt=11;c.rateControl_.node.amn_retrycnt=11;
    c.chooseRateControl();CHECK(n.ni_txrate==3 && c.rateControl_.changes==0);
    CHECK(c.properties["RTWDiagFirmwareRateControl"]==1 && c.properties["RTWDiagTXRate500Kbps"]==0);
    CHECK(c.properties["RTWDiagSiliconFamily"]==RTW_WIFI_RTL8814AU && c.properties["RTWDiagHTNegotiated"]==0);
    CHECK(c.properties["RTWDiagTXRateFeedbackActive"]==0);
    const uint8_t ra[8]={0xc,7,3,0,0,0x91,0,0x60};acPhyTrace={};
    c.handle8814C2H(ra,8);c.handle8814C2H(ra,8);
    CHECK(c.rateControl_.reports==2 && c.rateControl_.finalRate==3 && acPhyTrace.calls==2 && acPhyTrace.rate==3);
    CHECK(!c.rateControl_.failures && !c.rateControl_.retried && !c.rateControl_.repeatedSequences);
    CHECK(c.rateControl_.node.amn_txcnt==11 && c.rateControl_.node.amn_retrycnt==11);
    CHECK(!c.rateControl_.pendingReports && !c.rateControl_.requests);
    auto old=fixture(7);c.handle8814C2H(old.data(),old.size());CHECK(c.rateControl_.reports==2);
    auto legacy=fixture88e();c.handle8814C2H(legacy.data(),legacy.size());CHECK(c.rateControl_.reports==2);
    c.acMediaConnected_=false;c.handle8814C2H(ra,8);CHECK(c.rateControl_.reports==2 && acPhyTrace.calls==2);
    c.acMediaConnected_=true;c.acRateReady_=false;CHECK(!c.uses8814FirmwareRate());
    c.acRateReady_=true;c.core_.sc_ic.ic_state=0;CHECK(!c.uses8814FirmwareRate());c.core_.sc_ic.ic_state=IEEE80211_S_RUN;
    c.running_=false;CHECK(!c.uses8814FirmwareRate());c.running_=true;c.stopping_=true;CHECK(!c.uses8814FirmwareRate());c.stopping_=false;
    // Normal RX retains the raw codec's PHY/SHIFT pointers but the actual
    // controller removes exactly one four-byte FCS (RCR.APP_FCS is enabled).
    std::array<uint8_t,128> b{};
    const uint8_t header[24]={0x1c,0,0x44,0x0e, 3,0,0,0, 0,0,0,0x40,
        11,0,0,0, 0,0,0,0, 1,2,3,4};
    std::memcpy(b.data(),header,24);b[26]=64;b[27]=68;b[28]=6;b[49]=68;b[50]=72;
    b[58]=0x80;b[82]=0xde;b[83]=0xad;b[84]=0xbe;b[85]=0xef;
    // Unit2 contains a C2H RA payload; it must never lose four bytes as FCS.
    put32(b.data()+88,8);put32(b.data()+96,0x10000000);std::memcpy(b.data()+112,ra,8);
    rtw_rx_iterator it{};rtw_8192eu_rx_frame f{};uint8_t kind=99;
    rtw_rx_iterator_init(&it,b.data(),120);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_OK);
    CHECK(!kind && !f.report_is_c2h && f.payload.mpdu==b.data()+58 && f.payload.mpdu_length==24);
    CHECK(f.payload.phy_info==b.data()+26 && f.payload.phy_info_length==32 && it.cursor==88);
    CHECK(f.security_code==4 && f.sw_decrypt_required && f.payload.timestamp==0x04030201);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_OK);
    CHECK(kind==4 && f.report_is_c2h && f.payload.mpdu_length==8 && f.payload.mpdu==b.data()+112);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_DONE);
    for(size_t length:{size_t(4),size_t(3),size_t(1)}) {
        b.fill(0);put32(b.data(),uint32_t(length));rtw_rx_iterator_init(&it,b.data(),24+length);
        CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_MALFORMED);
    }
    b.fill(0);put32(b.data(),28);put32(b.data()+12,43);b[24]=0x08;
    put32(b.data()+56,28);b[80]=0x80;
    rtw_rx_iterator_init(&it,b.data(),108);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_OK && f.payload.rate_index==43 && f.payload.mpdu_length==24);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_OK && f.payload.mpdu==b.data()+80 && f.payload.mpdu_length==24);
    rtw_rx_iterator_init(&it,b.data(),107);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_OK);
    CHECK(c.decodeUSBFrame(&it,&f,&kind)==RTW_PACKETS_TRUNCATED);
}
static void bc_rate_diagnostics_and_ownership(){
    for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        RTWUSBController c;ieee80211_node n;c.family_=family;c.r88FirmwareReady_=true;fresh(c,n);
        n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active&&n.ni_txrate==3);
        c.r88Rates_.rates.ht=1;c.r88Rates_.rates.mask=0x000ff010;c.r88Rates_.width_mhz=20;
        c.r88Rates_.rates.tx_streams=1;c.r88Rates_.rates.sgi20=1;c.r88Rates_.rates.rate_id=5;
        c.chooseRateControl();CHECK(n.ni_txrate==3&&c.rateControl_.changes==0);
        CHECK(c.properties["RTWDiagFirmwareRateControl"]==1&&c.properties["RTWDiagTXRateFeedbackActive"]==0);
        CHECK(c.properties["RTWDiagHTNegotiated"]==1&&c.properties["RTWDiagHTRateMask"]==0x000ff010);
        CHECK(c.properties["RTWDiagHTTXStreams"]==1&&c.properties["RTWDiagHTSGI20Negotiated"]==1);
        CHECK(c.properties["RTWDiagHTChannelWidthMHz"]==20&&c.properties["RTWDiagHTRAID"]==5);
    }
}
static void au_negotiated_mode_diagnostics(){
    for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU}){
        RTWUSBController c;ieee80211_node n;c.family_=family;c.auFirmwareReady_=true;fresh(c,n);
        n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active&&n.ni_txrate==3);
        c.auRateConfig_.ht=c.auRateConfig_.vht=c.auRateConfig_.short_gi=1;
        c.auRateConfig_.bandwidth_mhz=80;c.auRateConfig_.streams=1;c.auRateConfig_.mask=0x003ff010;c.auRateConfig_.rate_id=10;
        c.chooseRateControl();CHECK(n.ni_txrate==3&&c.rateControl_.changes==0);
        CHECK(c.properties["RTWDiagVHTNegotiated"]==1&&c.properties["RTWDiagVHTSGI80Negotiated"]==1);
        CHECK(c.properties["RTWDiagHTNegotiated"]==1&&c.properties["RTWDiagHTRateMask"]==0x003ff010);
        CHECK(c.properties["RTWDiagHTChannelWidthMHz"]==80&&c.properties["RTWDiagHTTXStreams"]==1);
        CHECK(c.properties["RTWDiagHTSGI20Negotiated"]==0&&c.properties["RTWDiagHTSGI40Negotiated"]==0);
        c.auRateConfig_.vht=0;c.auRateConfig_.bandwidth_mhz=40;c.chooseRateControl();
        CHECK(c.properties["RTWDiagVHTNegotiated"]==0&&c.properties["RTWDiagHTSGI40Negotiated"]==1);
    }
}
static void du_negotiated_rate_diagnostics(){
    for(unsigned streams:{1U,2U}){
        RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8192DU;c.duFirmwareReady_=true;fresh(c,n);
        n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active&&n.ni_txrate==3);
        c.duMAC_.rates.ht=c.duMAC_.rates.sgi20=1;c.duMAC_.rates.raid=3;
        c.duMAC_.rates.mask=streams==1?0x000ff010:0x0ffff010;c.chooseRateControl();
        CHECK(c.properties["RTWDiagFirmwareRateControl"]==1&&c.properties["RTWDiagHTNegotiated"]==1);
        CHECK(c.properties["RTWDiagHTRateMask"]==c.duMAC_.rates.mask&&c.properties["RTWDiagHTTXStreams"]==streams);
        CHECK(c.properties["RTWDiagHTSGI20Negotiated"]==1&&c.properties["RTWDiagHTChannelWidthMHz"]==20&&c.properties["RTWDiagHTRAID"]==3);
    }
}
static void bu_negotiated_rate_diagnostics(){
 for(unsigned streams:{1U})for(unsigned width:{20U,40U}){
  RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8710BU;c.buFirmwareReady_=true;fresh(c,n);n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active&&n.ni_txrate==3);
  c.buRate_.ht=c.buRate_.sgi=1;c.buRate_.width_mhz=width;c.buRate_.rate_id=streams==2?2:3;c.buRate_.mask=streams==2?0x0ffff010:0x000ff010;c.chooseRateControl();
  CHECK(c.properties["RTWDiagFirmwareRateControl"]==1&&c.properties["RTWDiagTXRate500Kbps"]==0&&c.properties["RTWDiagHTNegotiated"]==1);CHECK(c.properties["RTWDiagHTRateMask"]==c.buRate_.mask&&c.properties["RTWDiagHTTXStreams"]==streams&&c.properties["RTWDiagHTRAID"]==c.buRate_.rate_id);CHECK(c.properties["RTWDiagHTSGI20Negotiated"]==(width==20)&&c.properties["RTWDiagHTSGI40Negotiated"]==(width==40)&&c.properties["RTWDiagHTChannelWidthMHz"]==width);CHECK(c.properties["RTWDiagVHTNegotiated"]==0);
  c.buRate_.ht=0;c.chooseRateControl();CHECK(c.properties["RTWDiagHTNegotiated"]==0&&c.properties["RTWDiagHTChannelWidthMHz"]==0);
 }
 RTWUSBController c;c.family_=RTW_WIFI_RTL8710BU;std::array<uint8_t,64> b{};b[0]=40;b[12]=4;b[24]=0x80;b[63]=0xa5;rtw_rx_iterator it{b.data(),b.size(),0,0};rtw_8192eu_rx_frame f{};uint8_t report=0;CHECK(c.decodeUSBFrame(&it,&f,&report)==RTW_PACKETS_OK&&!report&&f.payload.mpdu_length==40&&f.payload.mpdu[39]==0xa5);
}
static void fu_negotiated_rate_diagnostics(){
 for(unsigned streams:{1U,2U})for(unsigned width:{20U,40U}){
  RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8192FU;c.fuFirmwareReady_=true;fresh(c,n);n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active&&n.ni_txrate==3);
  c.fuRate_.ht=c.fuRate_.sgi=1;c.fuRate_.width_mhz=width;c.fuRate_.rate_id=streams==2?2:3;c.fuRate_.mask=streams==2?0x0ffff010:0x000ff010;c.chooseRateControl();
  CHECK(c.properties["RTWDiagFirmwareRateControl"]==1&&c.properties["RTWDiagTXRate500Kbps"]==0&&c.properties["RTWDiagHTNegotiated"]==1);CHECK(c.properties["RTWDiagHTRateMask"]==c.fuRate_.mask&&c.properties["RTWDiagHTTXStreams"]==streams&&c.properties["RTWDiagHTRAID"]==c.fuRate_.rate_id);CHECK(c.properties["RTWDiagHTSGI20Negotiated"]==(width==20)&&c.properties["RTWDiagHTSGI40Negotiated"]==(width==40)&&c.properties["RTWDiagHTChannelWidthMHz"]==width);CHECK(c.properties["RTWDiagVHTNegotiated"]==0);
  c.fuRate_.ht=0;c.chooseRateControl();CHECK(c.properties["RTWDiagHTNegotiated"]==0&&c.properties["RTWDiagHTChannelWidthMHz"]==0);
 }
 RTWUSBController c;c.family_=RTW_WIFI_RTL8192FU;std::array<uint8_t,64> b{};b[0]=40;b[12]=4;b[24]=0x80;b[63]=0xa5;rtw_rx_iterator it{b.data(),b.size(),0,0};rtw_8192eu_rx_frame f{};uint8_t report=0;CHECK(c.decodeUSBFrame(&it,&f,&report)==RTW_PACKETS_OK&&!report&&f.payload.mpdu_length==40&&f.payload.mpdu[39]==0xa5);
}
static void r23_negotiated_rate_diagnostics(){
 RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8723BU;c.r23FirmwareReady_=true;fresh(c,n);n.ni_txrate=3;c.resetRateControl();CHECK(!c.rateControl_.active&&n.ni_txrate==3);c.r23Rates_.mask=0xff010;c.r23Rates_.sgi=1;c.r23Rates_.raid=3;c.chooseRateControl();CHECK(c.properties["RTWDiagFirmwareRateControl"]==1&&c.properties["RTWDiagTXRate500Kbps"]==0&&c.properties["RTWDiagHTNegotiated"]==1);CHECK(c.properties["RTWDiagHTRateMask"]==c.r23Rates_.mask&&c.properties["RTWDiagHTTXStreams"]==1&&c.properties["RTWDiagHTRAID"]==3);CHECK(c.properties["RTWDiagHTSGI20Negotiated"]==1&&c.properties["RTWDiagHTSGI40Negotiated"]==0&&c.properties["RTWDiagHTChannelWidthMHz"]==20&&c.properties["RTWDiagVHTNegotiated"]==0);
 std::array<uint8_t,64> b{};b[0]=40;b[12]=4;b[24]=0x80;b[59]=0xa5;rtw_rx_iterator it{b.data(),b.size(),0,0};rtw_8192eu_rx_frame f{};uint8_t report=0;CHECK(c.decodeUSBFrame(&it,&f,&report)==RTW_PACKETS_OK&&!report&&f.payload.mpdu_length==36&&f.payload.mpdu[35]==0xa5);
 b[11]=0x10;it={b.data(),b.size(),0,0};CHECK(c.decodeUSBFrame(&it,&f,&report)==RTW_PACKETS_OK&&report==7&&f.payload.mpdu_length==40);
}
int main() {
    r23_negotiated_rate_diagnostics();
    bu_negotiated_rate_diagnostics();
    fu_negotiated_rate_diagnostics();
    du_negotiated_rate_diagnostics();
    au_negotiated_mode_diagnostics();
    bc_rate_diagnostics_and_ownership();
    bc_operating_diagnostics();ht_feedback88e();legacy_ownership();feedback88e();family_rx_dispatch();ac8814_ownership_and_rx();
    auto valid=fixture(17,0xc0,7,3); rtw_92e_tx_report report{};
    CHECK(rtw_92e_parse_tx_report(valid.data(),valid.size(),&report));
    CHECK(report.event_sequence==17&&report.retries==7&&report.final_rate==3);
    CHECK(report.lifetime_expired&&report.retry_over&&report.queue_time==0x1234);
    for(size_t n=0;n<10;++n)CHECK(!rtw_92e_parse_tx_report(valid.data(),n,&report));
    std::array<uint8_t,11> tooLong{};std::memcpy(tooLong.data(),valid.data(),valid.size());
    CHECK(!rtw_92e_parse_tx_report(tooLong.data(),tooLong.size(),&report));
    for(unsigned index : {0U,2U,3U,7U}) {
      auto bad=fixture(0); bad[index]=index==0?0x83:index==2?0x20:index==3?1:28;
      CHECK(!rtw_92e_parse_tx_report(bad.data(),bad.size(),&report));
    }
    for(uint8_t qsel : {uint8_t(8),uint8_t(15),uint8_t(0x12)}) {
      auto bad=fixture(0,qsel);CHECK(!rtw_92e_parse_tx_report(bad.data(),bad.size(),&report));
    }
    for(unsigned rate=12;rate<=27;++rate)for(unsigned qsel=0;qsel<=7;++qsel) {
      auto ht=fixture(19,uint8_t(qsel|0xc0U),5,uint8_t(rate));
      CHECK(rtw_92e_parse_tx_report(ht.data(),ht.size(),&report));
      CHECK(report.final_rate==rate&&report.retries==5&&report.lifetime_expired&&report.retry_over);
    }
    for(uint8_t invalid : {uint8_t(0x20),uint8_t(0x27),uint8_t(0x12)}) {
      auto bmc=fixture(1,invalid,0,27);CHECK(!rtw_92e_parse_tx_report(bmc.data(),bmc.size(),&report));
    }
    { auto reserved=valid;reserved[4]|=0xc0;CHECK(rtw_92e_parse_tx_report(reserved.data(),reserved.size(),&report));CHECK(report.retries==7); }
    CHECK(!rtw_92e_parse_tx_report(nullptr,10,&report)); CHECK(!rtw_92e_parse_tx_report(valid.data(),10,nullptr));
    std::array<uint8_t,40> d{}; CHECK(rtw_92e_make_tx_descriptor(64,1,0,0,0,123,d.data()));
    CHECK(rtw_92e_tx_report_request(1,d.data())); CHECK((le32(d.data()+8)&0x00090000)==0x00090000);
    unsigned sum=0;for(unsigned i=0;i<32;i+=2)sum^=unsigned(d[i])|(unsigned(d[i+1])<<8);CHECK(sum==0);
    CHECK(rtw_92e_tx_report_request(0,d.data()));CHECK((le32(d.data()+8)&0x80000)==0);
    CHECK(!rtw_92e_tx_report_request(2,d.data()));CHECK(!rtw_92e_tx_report_request(1,nullptr));
    CHECK(rtw_92e_make_tx_descriptor(64,0,1,1,0,123,d.data()));CHECK(!rtw_92e_tx_report_request(1,d.data()));
    CHECK(rtw_92e_make_tx_descriptor(64,0,0,1,0,123,d.data()));CHECK(!rtw_92e_tx_report_request(1,d.data()));
    /* Pass a C2H event through the actual USB aggregate parser, including
     * padding and the 10-byte payload that is shorter than an 802.11 header. */
    std::array<uint8_t,40> aggregate{};put32(aggregate.data(),10);put32(aggregate.data()+8,0x10000000);
    auto b=fixture(99);std::memcpy(aggregate.data()+24,b.data(),10);
    rtw_rx_iterator it;rtw_rx_iterator_init(&it,aggregate.data(),aggregate.size());rtw_8192eu_rx_frame frame{};
    CHECK(rtw_8192eu_rx_next(&it,&frame)==RTW_PACKETS_OK);CHECK(frame.report_is_c2h&&frame.payload.mpdu_length==10);
    CHECK(rtw_92e_parse_tx_report(frame.payload.mpdu,frame.payload.mpdu_length,&report));CHECK(report.event_sequence==99);
    CHECK(rtw_8192eu_rx_next(&it,&frame)==RTW_PACKETS_DONE);
    RTWUSBController c;ieee80211_node n;fresh(c,n);
    CHECK(c.rateControl_.policy.amrr_min_success_threshold==1&&c.rateControl_.policy.amrr_max_success_threshold==15);
    CHECK(c.rateControl_.active);uint8_t seq=0;
    /* No report, including USB activity alone, can increase the rate. */
    for(unsigned i=0;i<4;++i)c.chooseRateControl();CHECK(n.ni_txrate==0);
    reports(c,11,seq);CHECK(c.rateControl_.node.amn_txcnt==11);c.chooseRateControl();CHECK(n.ni_txrate==1);
    CHECK(c.rateControl_.changes==1&&c.rateControl_.reports==11);
    reports(c,11,seq,1);c.chooseRateControl();CHECK(n.ni_txrate==0);
    CHECK(c.rateControl_.node.amn_success_threshold==2);
    /* Two clean windows are needed after a failed rate probe. */
    reports(c,11,seq);c.chooseRateControl();CHECK(n.ni_txrate==0);
    reports(c,11,seq);c.chooseRateControl();CHECK(n.ni_txrate==1);
    reports(c,11,seq,0,0x40);c.chooseRateControl();CHECK(n.ni_txrate==0);CHECK(c.rateControl_.failures==11);
    CHECK(c.rateControl_.retried==22); // retry reports plus zero-retry lifetime failures
    auto same=fixture(uint8_t(seq-1),0x40);sent(c);auto before=c.rateControl_.reports;
    c.handleTxReport(same.data(),same.size());CHECK(c.rateControl_.reports==before+1);CHECK(c.rateControl_.repeatedSequences==1);
    auto bad=fixture(seq++);bad[3]=1;before=c.rateControl_.reports;c.handleTxReport(bad.data(),bad.size());CHECK(c.rateControl_.reports==before);
    n.ni_port_valid=false;auto good=fixture(seq++);c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before);
    c.chooseRateControl();CHECK(c.properties["RTWDiagTXRateFeedbackActive"]==0);n.ni_port_valid=true;
    n.ni_bssid[5]^=1;c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before);n.ni_bssid[5]^=1;
    /* An association reset with old report requests poisons the transport
     * epoch until a complete firmware/DMA/USB reinitialization marks it fresh. */
    c.resetRateControl();CHECK(!c.rateTransportFresh_&&!c.rateControl_.active);CHECK(n.ni_txrate==0);
    sent(c);c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before);
    c.rateControl_.requestedSinceReset=false;c.rateTransportFresh_=true;c.resetRateControl();CHECK(c.rateControl_.active);
    before=c.rateControl_.reports;c.handleTxReport(good.data(),good.size());CHECK(c.rateControl_.reports==before); // no new report requested
    seq=254;reports(c,11,seq);c.chooseRateControl();CHECK(n.ni_txrate==1); // sequence 255 -> 0
    n.ni_txrate=3;reports(c,11,seq);c.chooseRateControl();CHECK(n.ni_txrate==3);
    CHECK(c.properties["RTWDiagTXRate500Kbps"]==22);
    { RTWUSBController empty;ieee80211_node station;fresh(empty,station);station.ni_rates.rs_nrates=0;
      sent(empty);auto event=fixture(1);empty.handleTxReport(event.data(),event.size());empty.chooseRateControl();CHECK(station.ni_txrate==0);CHECK(empty.rateControl_.reports==0); }
    { RTWUSBController invalid;ieee80211_node station;fresh(invalid,station);station.ni_rates.rs_nrates=255;
      sent(invalid);auto event=fixture(1);invalid.handleTxReport(event.data(),event.size());invalid.chooseRateControl();CHECK(invalid.rateControl_.reports==0); }
    { RTWUSBController saturated;ieee80211_node station;fresh(saturated,station);saturated.rateControl_.node.amn_txcnt=UINT_MAX;
      saturated.rateControl_.node.amn_retrycnt=UINT_MAX;sent(saturated);auto event=fixture(1,0x80);saturated.handleTxReport(event.data(),event.size());
      CHECK(saturated.rateControl_.node.amn_txcnt==UINT_MAX);CHECK(saturated.rateControl_.node.amn_retrycnt==UINT_MAX); }
    { RTWUSBController reused;ieee80211_node station;fresh(reused,station);auto event=fixture(0);
      for(unsigned i=0;i<11;++i){sent(reused);reused.handleTxReport(event.data(),event.size());}
      CHECK(reused.rateControl_.reports==11);CHECK(reused.rateControl_.repeatedSequences==10);
      reused.chooseRateControl();CHECK(station.ni_txrate==1); }
    { RTWUSBController ht;ieee80211_node station;fresh(ht,station);
      const uint8_t mcs[2]={0xff,0xff};
      CHECK(rtw_92e_build_rate_config(0xfff,mcs,2,1,1,1,&ht.core_.rate_config));
      ht.core_.fw_station_connected=1;station.ni_flags=IEEE80211_NODE_HT;
      CHECK(ht.usesFirmwareRate());station.ni_txrate=2;
      ht.rateControl_.node.amn_txcnt=11;ht.rateControl_.node.amn_retrycnt=0;
      for(unsigned rate=12;rate<=27;++rate) {
        sent(ht);auto event=fixture(uint8_t(rate),uint8_t(rate&7U),2,uint8_t(rate));
        ht.handleTxReport(event.data(),event.size());
      }
      CHECK(ht.rateControl_.reports==16&&ht.rateControl_.pendingReports==0&&ht.rateControl_.finalRate==27);
      CHECK(ht.rateControl_.node.amn_txcnt==11&&ht.rateControl_.node.amn_retrycnt==0);
      station.ni_tx_ba[6].ba_state=IEEE80211_BA_AGREED;station.ni_rx_ba[2].ba_state=IEEE80211_BA_AGREED;
      ht.chooseRateControl();CHECK(station.ni_txrate==2&&ht.rateControl_.changes==0);
      CHECK(ht.rateControl_.node.amn_txcnt==11&&ht.rateControl_.node.amn_retrycnt==0);
      CHECK(ht.properties["RTWDiagHTNegotiated"]==1&&ht.properties["RTWDiagTXRate500Kbps"]==0);
      CHECK(ht.properties["RTWDiagFirmwareRateControl"]==1&&ht.properties["RTWDiagSiliconFamily"]==RTW_WIFI_RTL8192EU);
      CHECK(ht.properties["RTWDiagHTRateMask"]==0x0fffffffU&&ht.properties["RTWDiagHTSGI20Negotiated"]==1);
      CHECK(ht.properties["RTWDiagHTTXStreams"]==2&&ht.properties["RTWDiagHTRAID"]==2);
      CHECK(ht.properties["RTWDiagHTTXBATIDs"]==(1U<<6)&&ht.properties["RTWDiagHTRXBATIDs"]==(1U<<2));
      ht.core_.rate_config.rate_mask&=~(1U<<20);sent(ht);auto disabled=fixture(31,0,0,20);
      auto pending=ht.rateControl_.pendingReports;ht.handleTxReport(disabled.data(),disabled.size());
      CHECK(ht.rateControl_.invalid==1&&ht.rateControl_.reports==16&&ht.rateControl_.pendingReports==pending);
      ht.core_.fw_station_connected=0;CHECK(!ht.usesFirmwareRate());
      auto mcsEvent=fixture(32,0,0,12);ht.handleTxReport(mcsEvent.data(),mcsEvent.size());
      CHECK(ht.rateControl_.invalid==2&&ht.rateControl_.reports==16);
      ht.core_.fw_station_connected=1;station.ni_flags=0;CHECK(!ht.usesFirmwareRate());
      station.ni_flags=IEEE80211_NODE_HT;ht.core_.rate_config.ht=0;CHECK(!ht.usesFirmwareRate());
      ht.core_.rate_config.ht=1;ht.stopping_=true;CHECK(!ht.usesFirmwareRate());
      ht.stopping_=false;ht.running_=false;CHECK(!ht.usesFirmwareRate());
      ht.running_=true;ht.core_.sc_ic.ic_state=0;CHECK(!ht.usesFirmwareRate());
      ht.core_.sc_ic.ic_state=IEEE80211_S_RUN;CHECK(ht.usesFirmwareRate()); }
    std::printf("rate feedback: %u checks passed (real controller methods, CCX parser and vendor AMRR)\n",checks);
}
