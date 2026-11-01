#include "actual_performance_host.hpp"
#include <array>
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
static std::array<uint8_t,160> downlink(const RTWUSBController& c,const ieee80211_node& n) {
    std::array<uint8_t,160> b{};b[0]=0x08;b[1]=0x42;
    std::memcpy(b.data()+4,c.core_.sc_ic.ic_myaddr,6);std::memcpy(b.data()+10,n.ni_bssid,6);
    b[16]=2;b[21]=3;return b;
}
static rtw_rx_packet packet(const uint8_t* b,size_t n,uint8_t rate) {
    rtw_rx_packet p{};p.mpdu=b;p.mpdu_length=n;p.rate_index=rate;return p;
}
static uint64_t property(RTWUSBController& c,const char* name) {
    auto found=c.properties.find(name);CHECK(found!=c.properties.end());return found->second;
}
static void ignored(RTWUSBController& c,const rtw_rx_packet& p) {
    const auto before=c.perfDiag_;c.observeReceivedRate(p);
    for(unsigned i=0;i<12;++i){CHECK(c.perfDiag_.rxRatePackets[i]==before.rxRatePackets[i]);CHECK(c.perfDiag_.rxRateBytes[i]==before.rxRateBytes[i]);}
    for(unsigned i=0;i<16;++i){CHECK(c.perfDiag_.rxMCSPackets[i]==before.rxMCSPackets[i]);CHECK(c.perfDiag_.rxMCSBytes[i]==before.rxMCSBytes[i]);}
    for(unsigned i=0;i<20;i++){CHECK(c.perfDiag_.rxVHTPackets[i]==before.rxVHTPackets[i]);CHECK(c.perfDiag_.rxVHTBytes[i]==before.rxVHTBytes[i]);}
    CHECK(c.perfDiag_.rxUnknownPackets==before.rxUnknownPackets&&c.perfDiag_.rxUnknownBytes==before.rxUnknownBytes);
    CHECK(c.perfDiag_.rxLastCode==before.rxLastCode&&c.perfDiag_.rxLastRate500Kbps==before.rxLastRate500Kbps);
}
static void observed_vht_rates(){
    RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8812AU;c.auFirmwareReady_=true;
    c.core_.sc_ic.ic_bss=&n;auto bytes=downlink(c,n);
    for(uint8_t code=44;code<64;code++){
        auto p=packet(bytes.data(),100+code,code);p.bandwidth_code=2;c.observeReceivedRate(p);
        CHECK(c.perfDiag_.rxVHTPackets[code-44]==1&&c.perfDiag_.rxVHTBytes[code-44]==100+code);
        c.publishPerformanceDiagnostics();std::string key="RTWDiagRXVHT"+std::to_string((code-44)/10+1)+"SSMCS"+std::to_string((code-44)%10);
        CHECK(property(c,(key+"Packets").c_str())==1);
        CHECK(property(c,"RTWDiagRXLastVHTNSS")==unsigned((code-44)/10+1));
        CHECK(property(c,"RTWDiagRXLastVHTMCS")==unsigned((code-44)%10));
        n.ni_port_valid=false;ignored(c,p);n.ni_port_valid=true;
    }
    CHECK(c.perfDiag_.rxUnknownPackets==0&&c.perfDiag_.rxVHT80Packets==20);
    for(unsigned width=0;width<3;width++){auto p=packet(bytes.data(),128,44);p.bandwidth_code=width;c.observeReceivedRate(p);}
    c.publishPerformanceDiagnostics();CHECK(property(c,"RTWDiagRXVHT20Packets")==1&&property(c,"RTWDiagRXVHT40Packets")==1&&property(c,"RTWDiagRXVHT80Packets")==21);
}
static void bu_actual_bandwidth_observations(){
 RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8710BU;c.buFirmwareReady_=true;c.core_.sc_ic.ic_bss=&n;auto bytes=downlink(c,n);
 for(unsigned width:{0U,1U}){auto p=packet(bytes.data(),128,19);p.bandwidth_code=width;c.observeReceivedRate(p);}c.publishPerformanceDiagnostics();CHECK(property(c,"RTWDiagRXHT20Packets")==1&&property(c,"RTWDiagRXHT40Packets")==1&&property(c,"RTWDiagRXMCS7Packets")==2);
 c.buFirmwareReady_=false;ignored(c,packet(bytes.data(),128,12));
}
static void fu_actual_bandwidth_observations(){
 RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8192FU;c.fuFirmwareReady_=true;c.core_.sc_ic.ic_bss=&n;auto bytes=downlink(c,n);
 for(unsigned width:{0U,1U}){auto p=packet(bytes.data(),128,27);p.bandwidth_code=width;c.observeReceivedRate(p);}c.publishPerformanceDiagnostics();CHECK(property(c,"RTWDiagRXHT20Packets")==1&&property(c,"RTWDiagRXHT40Packets")==1&&property(c,"RTWDiagRXMCS15Packets")==2);
 c.fuFirmwareReady_=false;ignored(c,packet(bytes.data(),128,12));
}
static void r23_actual_bandwidth_observations(){
 RTWUSBController c;ieee80211_node n;c.family_=RTW_WIFI_RTL8723BU;c.r23FirmwareReady_=true;c.core_.sc_ic.ic_bss=&n;auto bytes=downlink(c,n);for(unsigned width:{0U,1U}){auto p=packet(bytes.data(),128,19);p.bandwidth_code=width;c.observeReceivedRate(p);}c.publishPerformanceDiagnostics();CHECK(property(c,"RTWDiagRXHT20Packets")==1&&property(c,"RTWDiagRXHT40Packets")==0&&property(c,"RTWDiagRXHTUnknownBWPackets")==1&&property(c,"RTWDiagRXMCS7Packets")==2);c.r23FirmwareReady_=false;ignored(c,packet(bytes.data(),128,12));
}
int main() {
    r23_actual_bandwidth_observations();
    bu_actual_bandwidth_observations();
    fu_actual_bandwidth_observations();
    observed_vht_rates();
    RTWUSBController c;ieee80211_node n;c.core_.sc_ic.ic_bss=&n;auto bytes=downlink(c,n);
    /* A valid associated downlink must populate a literal PHY54 bucket. */
    c.observeReceivedRate(packet(bytes.data(),128,11));c.publishPerformanceDiagnostics();
    CHECK(property(c,"RTWDiagRXRate11Packets")==1);CHECK(property(c,"RTWDiagRXRate11Bytes")==128);
    CHECK(property(c,"RTWDiagRXLastRateCode")==11);CHECK(property(c,"RTWDiagRXLastRate500Kbps")==108);
    static const uint8_t expected[12]={2,4,11,22,12,18,24,36,48,72,96,108};
    for(uint8_t code=0;code<12;++code){
        c.observeReceivedRate(packet(bytes.data(),64+code,code));c.publishPerformanceDiagnostics();
        std::string prefix="RTWDiagRXRate"+std::to_string(code);
        CHECK(property(c,(prefix+"Packets").c_str())==(code==11?2U:1U));
        CHECK(property(c,(prefix+"Bytes").c_str())==(code==11?203U:64U+code));
        CHECK(property(c,"RTWDiagRXLastRateCode")==code);CHECK(property(c,"RTWDiagRXLastRate500Kbps")==expected[code]);
    }
    for(uint8_t mcs=0;mcs<16;++mcs){
        c.observeReceivedRate(packet(bytes.data(),100+mcs,12+mcs));c.publishPerformanceDiagnostics();
        std::string prefix="RTWDiagRXMCS"+std::to_string(mcs);
        CHECK(property(c,(prefix+"Packets").c_str())==1);
        CHECK(property(c,(prefix+"Bytes").c_str())==100+mcs);
        CHECK(property(c,"RTWDiagRXLastMCS")==mcs);CHECK(property(c,"RTWDiagRXLastRate500Kbps")==0);
    }
    /* Unknown codes remain separate, including maximum byte code. */
    for(uint8_t code : {uint8_t(28),uint8_t(127),uint8_t(255)}){
        c.observeReceivedRate(packet(bytes.data(),100,code));c.publishPerformanceDiagnostics();
        CHECK(property(c,"RTWDiagRXLastRateCode")==code);CHECK(property(c,"RTWDiagRXLastRate500Kbps")==0);
    }
    CHECK(property(c,"RTWDiagRXRateUnknownPackets")==3);CHECK(property(c,"RTWDiagRXRateUnknownBytes")==300);
    /* Every null/data-less subtype is excluded; management/control are excluded. */
    for(uint8_t fc : {uint8_t(0x00),uint8_t(0x80),uint8_t(0x04),uint8_t(0x48),uint8_t(0x58),uint8_t(0x68),uint8_t(0x78),uint8_t(0xc8),uint8_t(0xd8),uint8_t(0xe8),uint8_t(0xf8),uint8_t(0x09)}){
        auto b=bytes;b[0]=fc;ignored(c,packet(b.data(),b.size(),11));
    }
    for(uint8_t direction : {uint8_t(0),uint8_t(1),uint8_t(3)}){
        auto b=bytes;b[1]=direction;ignored(c,packet(b.data(),b.size(),11));
    }
    for(unsigned offset : {4U,10U}) {auto b=bytes;b[offset+5]^=0x10;ignored(c,packet(b.data(),b.size(),11));}
    {auto b=bytes;b[4]|=1;ignored(c,packet(b.data(),b.size(),11));}
    {auto p=packet(bytes.data(),bytes.size(),11);p.crc_error=1;ignored(c,p);p.crc_error=0;p.icv_error=1;ignored(c,p);}
    for(size_t length=0;length<24;++length)ignored(c,packet(bytes.data(),length,11));
    {auto b=bytes;b[0]=0x88;ignored(c,packet(b.data(),24,11));ignored(c,packet(b.data(),25,11));}
    ignored(c,packet(nullptr,128,11));ignored(c,packet(bytes.data(),4097,11));
    /* Actual authorized() gates each lifecycle condition and controlled port. */
    for(bool* flag : {&c.attached_,&c.running_,&c.radioReady_,&c.joinRequested_,&c.present_}){
        *flag=false;ignored(c,packet(bytes.data(),128,11));*flag=true;
    }
    c.sleeping_=true;ignored(c,packet(bytes.data(),128,11));c.sleeping_=false;
    c.stopping_=true;ignored(c,packet(bytes.data(),128,11));c.stopping_=false;
    c.usbError_=1;ignored(c,packet(bytes.data(),128,11));c.usbError_=0;
    c.core_.core_error=1;ignored(c,packet(bytes.data(),128,11));c.core_.core_error=0;
    c.core_.sc_ic.ic_state=0;ignored(c,packet(bytes.data(),128,11));c.core_.sc_ic.ic_state=IEEE80211_S_RUN;
    n.ni_port_valid=false;ignored(c,packet(bytes.data(),128,11));n.ni_port_valid=true;
    c.core_.sc_ic.ic_bss=nullptr;ignored(c,packet(bytes.data(),128,11));c.core_.sc_ic.ic_bss=&n;
    /* Observation changes no MPDU bytes and remains cumulative across publishing. */
    const auto before=bytes;c.observeReceivedRate(packet(bytes.data(),128,0));CHECK(bytes==before);
    c.publishPerformanceDiagnostics();CHECK(property(c,"RTWDiagRXRate0Packets")==2);
    CHECK(property(c,"RTWDiagRXRate0Bytes")==192);
    c.core_.sc_ic.ic_flags=IEEE80211_F_USEPROT;c.core_.sc_ic.ic_rtsthreshold=2346;n.ni_erp=7;c.publishPerformanceDiagnostics();
    CHECK(property(c,"RTWDiagRXUseProtection")==1);CHECK(property(c,"RTWDiagRXRTSThreshold")==2346);CHECK(property(c,"RTWDiagRXNodeERP")==7);
    c.core_.sc_ic.ic_flags=0;c.core_.sc_ic.ic_rtsthreshold=512;c.core_.sc_ic.ic_bss=nullptr;c.publishPerformanceDiagnostics();
    CHECK(property(c,"RTWDiagRXUseProtection")==0);CHECK(property(c,"RTWDiagRXRTSThreshold")==512);CHECK(property(c,"RTWDiagRXNodeERP")==0);
    for(const auto& entry:c.properties)CHECK(entry.first.find("RTWDiagRX")==0);
    std::printf("performance diagnostics: %u checks passed (actual methods, frame/packet types and authorization)\n",checks);
}
