#ifdef NDEBUG
#error "Regression assertions must remain active in Release builds"
#endif
/* SPDX-License-Identifier: GPL-2.0-only
 * Regression across the REAL existing six RX descriptor formats/families.
 * All frames are synthetic; zero hardware 5-GHz functionality is claimed. */
#include "rtw/Family5GHzScan.h"
#include "rtw/FCS.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static bool station5gFamily(rtw_wifi_family f) {
    switch(f) {
    case RTW_WIFI_RTL8192DU: case RTW_WIFI_RTL8812AU: case RTW_WIFI_RTL8814AU:
    case RTW_WIFI_RTL8821AU: case RTW_WIFI_RTL8821CU: case RTW_WIFI_RTL8822BU:
    case RTW_WIFI_RTL8822CU: return true;
    default: return false;
    }
}

static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { \
    std::fprintf(stderr, "5ghz scan failed at %s:%d: %s\n", \
                 __FILE__, __LINE__, #x); std::abort(); } } while(0)

static void put32(std::vector<uint8_t> &v, size_t at, uint32_t n) {
    for (unsigned i=0;i<4;i++)v[at+i]=static_cast<uint8_t>(n>>(i*8U));
}
static void ie(std::vector<uint8_t>&v, uint8_t type, const std::vector<uint8_t>&b) {
    v.push_back(type); v.push_back(static_cast<uint8_t>(b.size()));
    v.insert(v.end(),b.begin(),b.end());
}
static std::vector<uint8_t> beacon(uint8_t ch=36) {
    std::vector<uint8_t> v(36,0);
    v[0]=0x80;
    std::fill(v.begin()+4,v.begin()+10,0xff);
    const std::array<uint8_t,6> mac={0x02,0x11,0x22,0x33,0x44,0x55};
    std::copy(mac.begin(),mac.end(),v.begin()+10);
    std::copy(mac.begin(),mac.end(),v.begin()+16);
    v[32]=100;v[34]=1;
    ie(v,0,{'T','a','h','o','e','5','G'});
    ie(v,1,{0x8c,0x12,0x98,0x24,0x30,0x48,0x60,0x6c});
    std::vector<uint8_t> ht(22,0);
    ht[0]=ch;ht[1]=0x05;
    ie(v,61,ht);
    ie(v,192,{1,42,0,0xff,0xff});
    const uint32_t crc=rtw_fcs32(v.data(),v.size());
    for(unsigned i=0;i<4;i++)v.push_back(static_cast<uint8_t>(crc>>(i*8U)));
    return v;
}
static const rtw_usb_wifi_profile* verified(rtw_wifi_family f) {
    for(size_t i=0;i<rtw_usb_wifi_profile_count();i++) {
        const auto *p=rtw_usb_wifi_profile_at(i);
        if(p&&p->family==f&&p->primary_verified)return p;
    }
    return nullptr;
}
static const rtw_usb_wifi_profile* unverified() {
    for(size_t i=0;i<rtw_usb_wifi_profile_count();i++) {
        const auto *p=rtw_usb_wifi_profile_at(i);
        const auto c=rtw_wifi_radio_profile_capabilities(p);
        if(p&&!p->primary_verified&&(c.family_bands&RTW_WIFI_BAND_5GHZ))return p;
    }
    return nullptr;
}
static std::vector<uint8_t> descriptor(rtw_wifi_family family,
                                        const std::vector<uint8_t>&frame,
                                        bool crc=false, bool event=false) {
    const size_t n=24U+frame.size();
    std::vector<uint8_t> v((n+7U)&~size_t(7),0);
    uint32_t w0=static_cast<uint32_t>(frame.size());
    if(crc)w0|=1U<<14;
    put32(v,0,w0);
    put32(v,12,4U); // OFDM 6Mbps is valid on all six desc formats.
    if(event)put32(v,8,1U<<28);
    std::copy(frame.begin(),frame.end(),v.begin()+24);
    (void)family;
    return v;
}
struct Seen {unsigned count=0;uint8_t ch=0,width=0,center=0;};
static int on_ap(void*cookie,const rtw_r12au_ap *ap,
                 const rtw_r12au_rx_frame *frame) {
    auto &seen=*static_cast<Seen*>(cookie);
    CHECK(ap&&frame&&frame->payload&&frame->payload_length>=36U);
    ++seen.count;
    seen.ch=ap->primary_channel;
    seen.width=ap->operating_width_mhz;
    seen.center=ap->center_channel;
    return 1;
}
static int stop(void*,const rtw_r12au_ap*,const rtw_r12au_rx_frame*) {return 0;}

static void exercise(rtw_wifi_family family) {
    auto p=verified(family);CHECK(p);
    const auto b=beacon();
    auto bytes=descriptor(family,b);
    Seen seen{};
    rtw_family_5g_scan_stats stats{};
    CHECK(rtw_family_5g_scan_bulk(p,bytes.data(),bytes.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_OK);
    CHECK(seen.count==1U && seen.ch==36U && seen.width==80U && seen.center==42U);
    CHECK(stats.aggregates==1U && stats.frames==1U && stats.accepted_beacons==1U);
    CHECK(rtw_family_5g_scan_bulk(p,bytes.data(),bytes.size(),40,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_OK);
    CHECK(stats.wrong_channel==1U && stats.accepted_beacons==0U);
    CHECK(rtw_family_5g_scan_bulk(p,bytes.data(),bytes.size(),36,1,stop,nullptr,&stats)
          ==RTW_FAMILY_5G_SCAN_CALLBACK_STOP);
    CHECK(stats.accepted_beacons==0U);
    auto corrupted=bytes;
    corrupted[24+42]^=1U; // bad FCS without changing descriptor
    CHECK(rtw_family_5g_scan_bulk(p,corrupted.data(),corrupted.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_OK);
    CHECK(stats.bad_fcs==1U && stats.accepted_beacons==0U);
    corrupted=bytes;corrupted[1]|=0x40U; // descriptor CRC flag
    CHECK(rtw_family_5g_scan_bulk(p,corrupted.data(),corrupted.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_OK);
    CHECK(stats.crc_drops==1U && stats.accepted_beacons==0U);
    corrupted=bytes;
    corrupted.insert(corrupted.end(),24,0);
    corrupted[bytes.size()]=0xffU;corrupted[bytes.size()+1]=0x3fU;
    seen={};
    CHECK(rtw_family_5g_scan_bulk(p,corrupted.data(),corrupted.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_BAD_AGGREGATE);
    CHECK(stats.malformed_aggregates==1U && seen.count==0U && stats.frames==0U);
    /* An empty transfer carries no access to radio state and is valid. */
    CHECK(rtw_family_5g_scan_bulk(p,nullptr,0,36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_OK);
    CHECK(stats.aggregates==0U);
    CHECK(rtw_family_5g_scan_bulk(p,bytes.data(),bytes.size(),6,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_BAD_ARGUMENT);
    CHECK(rtw_family_5g_scan_bulk(p,bytes.data(),bytes.size(),36,2,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_BAD_ARGUMENT);
}
int main() {
    for(auto f:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8814AU,
                RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})
        exercise(f);
    rtw_family_5g_scan_stats stats{};
    Seen seen{};
    const auto b=beacon();const auto bytes=descriptor(RTW_WIFI_RTL8812AU,b);
    /* Exhaustive USB catalog: 270 immutable IDs and all 14 families. This
     * tests offline receiver availability ONLY, never physical band support. */
    CHECK(rtw_usb_wifi_profile_count()==270U);
    size_t supported_ids=0, dual_ids=0;
    for(size_t i=0;i<rtw_usb_wifi_profile_count();++i) {
        const auto *identity=rtw_usb_wifi_profile_at(i);
        CHECK(identity);
        const auto cap=rtw_wifi_radio_profile_capabilities(identity);
        const bool dual=(cap.family_bands&RTW_WIFI_BAND_5GHZ)!=0U;
        const bool codec=identity->family==RTW_WIFI_RTL8812AU ||
            identity->family==RTW_WIFI_RTL8821AU ||
            identity->family==RTW_WIFI_RTL8814AU ||
            identity->family==RTW_WIFI_RTL8821CU ||
            identity->family==RTW_WIFI_RTL8822BU ||
            identity->family==RTW_WIFI_RTL8822CU;
        if(dual)++dual_ids;
        const auto result=rtw_family_5g_scan_bulk(identity,nullptr,0,36,1,
                                                 on_ap,&seen,&stats);
        const bool can_decode=dual&&identity->primary_verified&&codec;
        CHECK(result==(can_decode?RTW_FAMILY_5G_SCAN_OK:
                       RTW_FAMILY_5G_SCAN_UNAVAILABLE));
        if(can_decode)++supported_ids;
        CHECK(bool(cap.station_bands&RTW_WIFI_BAND_5GHZ)==(identity->primary_verified&&station5gFamily(identity->family)));
    }
    CHECK(dual_ids>supported_ids && supported_ids>0U);
    const auto *du=verified(RTW_WIFI_RTL8192DU);
    CHECK(du);
    CHECK(rtw_family_5g_scan_bulk(du,bytes.data(),bytes.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_UNAVAILABLE);
    const auto *u=unverified();CHECK(u);
    CHECK(rtw_family_5g_scan_bulk(u,bytes.data(),bytes.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_UNAVAILABLE);
    rtw_usb_wifi_profile forged=*verified(RTW_WIFI_RTL8812AU);
    CHECK(rtw_family_5g_scan_bulk(&forged,bytes.data(),bytes.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_UNAVAILABLE);
    CHECK(rtw_family_5g_scan_bulk(nullptr,bytes.data(),bytes.size(),36,1,on_ap,&seen,&stats)
          ==RTW_FAMILY_5G_SCAN_UNAVAILABLE);
    CHECK(rtw_family_5g_scan_bulk(verified(RTW_WIFI_RTL8812AU),bytes.data(),bytes.size(),36,1,
          nullptr,&seen,&stats)==RTW_FAMILY_5G_SCAN_BAD_ARGUMENT);
    CHECK(rtw_family_5g_scan_bulk(verified(RTW_WIFI_RTL8812AU),bytes.data(),bytes.size(),36,1,
          on_ap,&seen,nullptr)==RTW_FAMILY_5G_SCAN_BAD_ARGUMENT);
    /* The diagnostics storage must not overwrite its own immutable transfer.
     * A corrupt caller may alias both pointers; reject without touching either. */
    rtw_family_5g_scan_stats aliased{};
    std::memset(&aliased,0xa5,sizeof(aliased));
    std::array<uint8_t,sizeof(aliased)> untouched{};
    std::memcpy(untouched.data(),&aliased,sizeof(aliased));
    CHECK(rtw_family_5g_scan_bulk(verified(RTW_WIFI_RTL8812AU),
          reinterpret_cast<const uint8_t*>(&aliased),sizeof(aliased),
          36,1,on_ap,&seen,&aliased)==RTW_FAMILY_5G_SCAN_BAD_ARGUMENT);
    CHECK(std::memcmp(untouched.data(),&aliased,sizeof(aliased))==0);
    CHECK(rtw_family_5g_scan_bulk(verified(RTW_WIFI_RTL8812AU),
          reinterpret_cast<const uint8_t*>(&aliased)+3U,sizeof(aliased)-3U,
          36,1,on_ap,&seen,&aliased)==RTW_FAMILY_5G_SCAN_BAD_ARGUMENT);
    CHECK(std::memcmp(untouched.data(),&aliased,sizeof(aliased))==0);
    std::printf("six-family offline 5GHz scan: %u checks passed (NO RF/TX)\n",checks);
}
