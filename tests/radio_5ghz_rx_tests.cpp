#ifdef NDEBUG
#error "Regression assertions must remain active in Release builds"
#endif
/* SPDX-License-Identifier: BSD-2-Clause
 * Portable 5-GHz receive/scan-geometry regression. This never tunes a radio
 * or certifies a USB board for operation on 5 GHz. */
#include "rtw/RTL8812AUStation.h"
#include "rtw/RTL8812AUPassive.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/FamilyRadioPolicy.h"
#include "rtw/FCS.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

static bool station5gFamily(rtw_wifi_family f) {
    switch(f) {
    case RTW_WIFI_RTL8192DU: case RTW_WIFI_RTL8812AU: case RTW_WIFI_RTL8814AU:
    case RTW_WIFI_RTL8821AU: case RTW_WIFI_RTL8821CU: case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU: return true;
    default: return false;
    }
}

static unsigned checks=0;
#define CHECK(c) do { ++checks; if (!(c)) { \
  std::fprintf(stderr,"5g-rx test failed at %s:%d: %s\n",__FILE__,__LINE__,#c); \
  std::abort(); } } while(0)

static constexpr std::array<uint8_t,6> mac={0x02,0x24,0x68,0x91,0x12,0x34};
static void ie(std::vector<uint8_t>&p,uint8_t id,const std::vector<uint8_t>&b){
    CHECK(b.size()<256);
    p.push_back(id);p.push_back(static_cast<uint8_t>(b.size()));
    p.insert(p.end(),b.begin(),b.end());
}
static std::vector<uint8_t> make_beacon(uint8_t primary, uint8_t sec,
                                        uint8_t vht_width, uint8_t center,
                                        bool vht, bool ht, bool fcs=false) {
    std::vector<uint8_t> p(36,0);
    p[0]=0x80;
    std::fill(p.begin()+4,p.begin()+10,0xff);
    std::copy(mac.begin(),mac.end(),p.begin()+10);
    std::copy(mac.begin(),mac.end(),p.begin()+16);
    p[32]=100;p[34]=1;
    ie(p,0,{'5','G','H','z'});
    ie(p,1,{0x8c,0x12,0x98,0x24,0x30,0x48,0x60,0x6c});
    if(ht){
        std::vector<uint8_t> operation(22,0);
        operation[0]=primary;
        operation[1]=static_cast<uint8_t>(sec|(sec?4U:0U));
        ie(p,61,operation);
    }
    if(vht)ie(p,192,{vht_width,center,0,0xff,0xff});
    if(fcs){
        uint32_t crc=rtw_fcs32(p.data(),p.size());
        for(unsigned n=0;n<4;n++)p.push_back(static_cast<uint8_t>(crc>>(8U*n)));
    }
    return p;
}
static rtw_r12au_scan_error scan(std::vector<uint8_t> &beacon,uint8_t tuned,
                                 rtw_r12au_ap &ap,bool fcs=false){
    rtw_r12au_rx_frame rx{};
    rx.payload=beacon.data(); rx.payload_length=beacon.size();
    return rtw_r12au_scan_parse(&rx,tuned,fcs?1:0,&ap);
}
static std::vector<uint8_t> bulk(const std::vector<uint8_t>&beacon){
    std::vector<uint8_t> p(24U+beacon.size(),0);
    p[0]=static_cast<uint8_t>(beacon.size());
    p[1]=static_cast<uint8_t>(beacon.size()>>8);
    p[12]=4U; // legacy OFDM6
    std::copy(beacon.begin(),beacon.end(),p.begin()+24);
    p.resize((p.size()+7U)&~size_t(7U),0);
    return p;
}
struct Found {int count=0; uint8_t width=0,center=0,channel=0;};
static int found(void*cookie,const rtw_r12au_ap*ap,const rtw_r12au_rx_frame*){
    auto&found=*static_cast<Found*>(cookie);
    ++found.count;found.width=ap->operating_width_mhz;
    found.center=ap->center_channel;found.channel=ap->primary_channel;
    return 1;
}
static void scan_geometry(){
    rtw_r12au_ap ap{};
    for(const auto c: {std::array<uint8_t,4>{36,1,1,42},
                      {40,3,1,42},{44,1,1,42},{48,3,1,42},
                      {52,1,1,58},{149,1,1,155},{161,3,1,155}}){
        auto beacon=make_beacon(c[0],c[1],c[2],c[3],true,true);
        CHECK(scan(beacon,c[0],ap)==RTW_R12AU_SCAN_OK);
        CHECK(ap.vht_capable && ap.ht_capable && ap.center_channel==c[3]);
        CHECK(ap.operating_width_mhz==80U);
        CHECK(scan(beacon,(uint8_t)(c[0]+4),ap)==RTW_R12AU_SCAN_WRONG_CHANNEL);
    }
    auto b=make_beacon(36,1,0,0,true,true);
    CHECK(scan(b,36,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.operating_width_mhz==40U && ap.center_channel==38);
    b=make_beacon(36,0,0,0,true,true);
    CHECK(scan(b,36,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.operating_width_mhz==20U && ap.center_channel==36);
    b=make_beacon(165,0,0,0,true,true);
    CHECK(scan(b,165,ap)==RTW_R12AU_SCAN_OK && ap.operating_width_mhz==20U);
    // VHT 80 with wrong centre channel, invalid HT secondary or 160 MHz:
    for(const auto spec: {std::array<uint8_t,4>{36,1,1,58},
                          {36,0,1,42},{36,2,1,42},
                          {36,1,2,42},{165,1,1,42}}){
        b=make_beacon(spec[0],spec[1],spec[2],spec[3],true,true);
        CHECK(scan(b,spec[0],ap)!=RTW_R12AU_SCAN_OK);
    }
    b=make_beacon(36,1,1,42,true,false); // VHT requires HT operation.
    CHECK(scan(b,36,ap)==RTW_R12AU_SCAN_UNSUPPORTED);
    b=make_beacon(36,1,1,42,true,true);
    const std::array<uint8_t,2> ht_ie_header={61U,22U};
    auto it=std::search(b.begin()+36,b.end(),
                ht_ie_header.begin(),ht_ie_header.end());
    CHECK(it!=b.end());
    *(it+3)=1U; /* Secondary offset set but STA width bit missing. */
    CHECK(scan(b,36,ap)==RTW_R12AU_SCAN_UNSUPPORTED);
    b=make_beacon(36,1,0,0,false,true);
    CHECK(scan(b,36,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.operating_width_mhz==40U && !ap.vht_capable);
    b=make_beacon(36,1,1,42,true,true,true);
    CHECK(scan(b,36,ap,true)==RTW_R12AU_SCAN_OK);
    b.back()^=1U;
    CHECK(scan(b,36,ap,true)==RTW_R12AU_SCAN_BAD_FCS);
    CHECK(ap.primary_channel==0U && ap.operating_width_mhz==0U);
}
static void passive_rx(){
    for(auto chip:{RTW_R12AU_CHIP_8812AU,RTW_R12AU_CHIP_8821AU}){
        for(auto channel:{36U,52U,149U,165U}){
            const auto ch=static_cast<uint8_t>(channel);
            const bool eighty=(channel!=165U);
            auto b=make_beacon(ch,eighty?1U:0U,eighty?1U:0U,
                  eighty?(uint8_t)(channel==36U?42U:channel==52U?58U:155U):0U,
                  true,true,true);
            auto payload=bulk(b);
            Found seen{};
            rtw_r12au_passive_stats stats{};
            CHECK(rtw_r12au_passive_on_channel(payload.data(),payload.size(),
                  chip,ch,found,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
            CHECK(seen.count==1 && seen.channel==ch &&
                  seen.width==(eighty?80U:20U));
            CHECK(stats.accepted_beacons==1U && stats.aggregates==1U);
            payload.push_back(0x80); // invalid tail must emit no callback
            seen={};stats={};
            CHECK(rtw_r12au_passive_on_channel(payload.data(),payload.size(),
                  chip,ch,found,&seen,&stats)==RTW_R12AU_PASSIVE_BAD_AGGREGATE);
            CHECK(seen.count==0 && stats.frames==0);
        }
        const auto b=bulk(make_beacon(36,1,1,42,true,true,true));
        Found seen{}; rtw_r12au_passive_stats stats{};
        CHECK(rtw_r12au_passive_on_channel(b.data(),b.size(),chip,14,
             found,&seen,&stats)==RTW_R12AU_PASSIVE_BAD_ARGUMENT);
        CHECK(rtw_r12au_passive_on_channel(b.data(),b.size(),chip,0,
             found,&seen,&stats)==RTW_R12AU_PASSIVE_BAD_ARGUMENT);
        CHECK(rtw_r12au_passive_on_channel(b.data(),b.size(),chip,6,
             found,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
        CHECK(seen.count==0 && stats.wrong_channel==1U);
    }
}
static void catalog(){
    size_t dual=0,verified=0;
    CHECK(rtw_usb_wifi_profile_count()==270U);
    for(size_t i=0;i<rtw_usb_wifi_profile_count();i++){
        const auto*p=rtw_usb_wifi_profile_at(i);
        CHECK(p);
        const auto c=rtw_wifi_radio_profile_capabilities(p);
        rtw_r12au_channel_plan plan{};
        const bool is5=(c.family_bands&RTW_WIFI_BAND_5GHZ)!=0;
        if(is5)++dual;
        if(p->primary_verified)++verified;
        CHECK(bool(c.station_bands&RTW_WIFI_BAND_5GHZ)==(p->primary_verified&&station5gFamily(p->family)));
        const auto result=rtw_family_radio_prepare(p->family,36,42,80,&plan);
        CHECK(result==(is5 && c.family_max_5g_width_mhz>=80 ?
           RTW_FAMILY_RADIO_OK : RTW_FAMILY_RADIO_UNSUPPORTED));
        if(result==RTW_FAMILY_RADIO_OK)
            CHECK(plan.is_5ghz && plan.passive_only && plan.center_channel==42);
        const auto scoped=rtw_family_radio_prepare_profile(p,36,42,80,&plan);
        CHECK(scoped==(p->primary_verified?result:RTW_FAMILY_RADIO_UNSUPPORTED));
        if(scoped==RTW_FAMILY_RADIO_OK)
            CHECK(plan.passive_only && plan.is_5ghz);
        else
            CHECK(plan.primary_channel==0 && !plan.passive_only);
    }
    CHECK(dual>0 && verified==263U);
    rtw_r12au_channel_plan plan{};
    const rtw_usb_wifi_profile forged{};
    CHECK(rtw_family_radio_prepare_profile(&forged,36,42,80,&plan)==
          RTW_FAMILY_RADIO_UNSUPPORTED);
    CHECK(plan.primary_channel==0 && !plan.passive_only);
    CHECK(rtw_family_radio_prepare_profile(nullptr,36,42,80,&plan)==
          RTW_FAMILY_RADIO_UNSUPPORTED);
    CHECK(rtw_family_radio_prepare_profile(rtw_usb_wifi_profile_at(0),
          36,42,80,nullptr)==RTW_FAMILY_RADIO_INVALID_ARGUMENT);
}
int main(){catalog();scan_geometry();passive_rx();
    std::printf("5-GHz RX/scan geometry: %u checks passed (offline only)\n",checks);
}
