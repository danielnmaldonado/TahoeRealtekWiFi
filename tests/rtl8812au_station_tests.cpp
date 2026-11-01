#include "rtw/RTL8812AURadio.h"
#include "rtw/RTL8812AUStation.h"
#include "rtw/FCS.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

static unsigned long long checks = 0;
#define CHECK(x) do { ++checks; if (!(x)) { \
    std::cerr << "FAIL " << __LINE__ << ": " << #x << '\n'; \
    std::exit(1); } } while (0)

static constexpr std::array<uint8_t, 6> ap_mac = {0x02,0x33,0x44,0x55,0x66,0x77};
static constexpr std::array<uint8_t, 6> sta_mac = {0x02,0xaa,0xbb,0xcc,0xdd,0xee};
static constexpr uint8_t rsn_psk_ccmp[] = {
    1,0, 0,15,172,4, 1,0,0,15,172,4,
    1,0,0,15,172,2, 0,0
};
static void put16(std::vector<uint8_t> &v, size_t at, uint16_t val) {
    v[at] = (uint8_t)val;
    v[at+1] = (uint8_t)(val>>8U);
}
static void ie(std::vector<uint8_t> &v, uint8_t id,
               const std::vector<uint8_t> &bytes) {
    CHECK(bytes.size() <= 255U);
    v.push_back(id);
    v.push_back(static_cast<uint8_t>(bytes.size()));
    v.insert(v.end(),bytes.begin(),bytes.end());
}
static std::vector<uint8_t> beacon(int channel, bool secured,
                                   bool use_fcs = false) {
    std::vector<uint8_t> v(36, 0);
    v[0] = 0x80; /* beacon */
    std::fill(v.begin()+4,v.begin()+10,0xff);
    std::copy(ap_mac.begin(),ap_mac.end(),v.begin()+10);
    std::copy(ap_mac.begin(),ap_mac.end(),v.begin()+16);
    put16(v,32,100U);
    put16(v,34, secured ? 0x0411U : 0x0401U);
    ie(v,0,{'T','a','h','o','e'});
    ie(v,1, channel < 30 ? std::vector<uint8_t>{0x82,0x84,0x8b,0x96,0x0c,0x12,0x18,0x24}
                          : std::vector<uint8_t>{0x8c,0x12,0x98,0x24,0x30,0x48,0x60,0x6c});
    if (channel < 30) {
        ie(v,50,{0x30,0x48,0x60,0x6c});
        ie(v,3,{(uint8_t)channel});
    }
    if (secured) ie(v,48,{std::begin(rsn_psk_ccmp),std::end(rsn_psk_ccmp)});
    if (use_fcs) {
        auto crc=rtw_fcs32(v.data(),v.size());
        for(unsigned i=0;i<4;i++) v.push_back((uint8_t)(crc>>(8U*i)));
    }
    return v;
}
static std::vector<uint8_t> beacon_with_rsn(
    const std::vector<uint8_t>& rsn) {
    auto v=beacon(6,true);
    CHECK(rsn.size()<=255U);
    const auto original=std::search(v.begin(),v.end(),
        std::begin(rsn_psk_ccmp),std::end(rsn_psk_ccmp));
    CHECK(original!=v.end());
    const auto start=static_cast<size_t>(original-v.begin());
    CHECK(start>=2U && v[start-2U]==48U &&
          v[start-1U]==sizeof(rsn_psk_ccmp));
    v[start-1U]=static_cast<uint8_t>(rsn.size());
    v.erase(v.begin()+static_cast<std::ptrdiff_t>(start),
            v.begin()+static_cast<std::ptrdiff_t>(start+
                sizeof(rsn_psk_ccmp)));
    v.insert(v.begin()+static_cast<std::ptrdiff_t>(start),
             rsn.begin(),rsn.end());
    return v;
}
static rtw_r12au_scan_error scan(std::vector<uint8_t> &v,
                                int channel, rtw_r12au_ap &ap,
                                bool fcs=false) {
    rtw_r12au_rx_frame rx{};
    rx.payload=v.data();
    rx.payload_length=v.size();
    rx.bandwidth_mhz=20;
    rx.rate=4;
    return rtw_r12au_scan_parse(&rx,(uint8_t)channel,fcs?1:0,&ap);
}
static void rf_tests() {
    rtw_r12au_channel_plan out{};
    for (auto chip:{RTW_R12AU_CHIP_8812AU,RTW_R12AU_CHIP_8821AU}) {
        for (uint8_t ch=1;ch<=11;ch++) {
            const rtw_r12au_channel_request r{chip,ch,20,ch};
            CHECK(rtw_r12au_channel_prepare(&r,&out)==RTW_R12AU_CHANNEL_OK);
            CHECK(!out.is_5ghz && !out.dfs_required && out.passive_only);
            CHECK(out.primary_frequency_mhz==(2407U+5U*ch));
            CHECK(out.rf_chains==(chip==RTW_R12AU_CHIP_8812AU?2:1));
        }
        for (uint8_t ch:{36,40,44,48,52,56,60,64,100,104,108,112,
                         116,120,124,128,132,136,140,144,149,153,157,161,165}) {
            const rtw_r12au_channel_request r{chip,ch,20,ch};
            CHECK(rtw_r12au_channel_prepare(&r,&out)==RTW_R12AU_CHANNEL_OK);
            CHECK(out.is_5ghz && out.passive_only);
            CHECK(out.dfs_required==(ch>=52 && ch<=144));
            CHECK(out.primary_frequency_mhz==5000U+5U*ch);
        }
        for (const auto pair:std::array<std::array<uint8_t,2>,4>{{
             {{36,38}},{{44,46}},{{149,151}},{{157,159}}}}) {
            rtw_r12au_channel_request r{chip,pair[0],40,pair[1]};
            CHECK(rtw_r12au_channel_prepare(&r,&out)==RTW_R12AU_CHANNEL_OK);
            r.center_channel++;
            CHECK(rtw_r12au_channel_prepare(&r,&out)==RTW_R12AU_CHANNEL_UNSUPPORTED);
        }
        for (auto start: {36,52,100,116,132,149}) {
            rtw_r12au_channel_request r{
                chip,(uint8_t)start,80,(uint8_t)(start+6)};
            CHECK(rtw_r12au_channel_prepare(&r,&out)==RTW_R12AU_CHANNEL_OK);
        }
        for (const rtw_r12au_channel_request bad: {
             rtw_r12au_channel_request{chip,0,20,0},
             {chip,14,20,14},{chip,165,40,167},
             {chip,36,80,38},{chip,6,40,8},
             {chip,36,10,36},{chip,6,20,10}}) {
            CHECK(rtw_r12au_channel_prepare(&bad,&out)==RTW_R12AU_CHANNEL_UNSUPPORTED);
            CHECK(out.primary_frequency_mhz==0 && !out.passive_only);
        }
    }
    CHECK(rtw_r12au_channel_prepare(nullptr,&out)==RTW_R12AU_CHANNEL_INVALID_ARGUMENT);
    rtw_r12au_channel_request valid{RTW_R12AU_CHIP_8812AU,6,20,6};
    CHECK(rtw_r12au_channel_prepare(&valid,nullptr)==RTW_R12AU_CHANNEL_INVALID_ARGUMENT);
    valid.chip=(rtw_r12au_chip)5;
    CHECK(rtw_r12au_channel_prepare(&valid,&out)==RTW_R12AU_CHANNEL_UNSUPPORTED);
}
static void scan_tests() {
    rtw_r12au_ap ap{};
    auto v=beacon(6,false);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_OPEN);
    CHECK(ap.ssid_length==5 && ap.ssid[0]=='T');
    CHECK(ap.rate_count==12);
    CHECK(ap.beacon_interval==100 && ap.primary_channel==6);
    CHECK(scan(v,11,ap)==RTW_R12AU_SCAN_WRONG_CHANNEL);
    CHECK(ap.ssid_length==0);
    v=beacon(36,false);
    CHECK(scan(v,36,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.primary_channel==36 && ap.rate_count==8);
    v=beacon(6,true);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_WPA2_PSK_CCMP);
    /* Transition mode: CCMP+TKIP, PSK+SAE, PMF optional.
     * This enables offline eligibility only, never hardware connection. */
    const std::vector<uint8_t> mixed_rsn = {
        1,0, 0,15,172,4,
        2,0, 0,15,172,2, 0,15,172,4,
        2,0, 0,15,172,8, 0,15,172,2,
        0x80,0
    };
    v=beacon_with_rsn(mixed_rsn);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_WPA2_PSK_CCMP);
    CHECK(!ap.pmf_required);
    auto unsupported_rsn=mixed_rsn;
    unsupported_rsn[25]=1; /* PSK -> 802.1X, SAE alone is insufficient. */
    v=beacon_with_rsn(unsupported_rsn);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_UNSUPPORTED);
    unsupported_rsn=mixed_rsn;
    unsupported_rsn[15]=2; /* CCMP pairwise -> TKIP, no compatible suite. */
    v=beacon_with_rsn(unsupported_rsn);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_UNSUPPORTED);
    unsupported_rsn=mixed_rsn;
    unsupported_rsn[26]=0xc0; /* Mandatory PMF is unsupported. */
    v=beacon_with_rsn(unsupported_rsn);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.pmf_required && ap.security==RTW_R12AU_SECURITY_UNSUPPORTED);
    unsupported_rsn=mixed_rsn;
    unsupported_rsn[6]=200; /* Truncated pairwise suite list is malformed. */
    v=beacon_with_rsn(unsupported_rsn);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_MALFORMED);
    v=beacon(6,true,true);
    CHECK(scan(v,6,ap,true)==RTW_R12AU_SCAN_OK);
    v.back()^=1U;
    CHECK(scan(v,6,ap,true)==RTW_R12AU_SCAN_BAD_FCS);
    v=beacon(6,true);
    /* Status: RSN's PMF required (capability bits 6 and 7). */
    auto pos=std::search(v.begin(),v.end(),std::begin(rsn_psk_ccmp),std::end(rsn_psk_ccmp));
    CHECK(pos!=v.end());
    *(pos+18)=0xc0;
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.pmf_required && ap.security==RTW_R12AU_SECURITY_UNSUPPORTED);
    v=beacon(6,true);
    v.back()=0x01; /* advertised capabilities may contain other bits */
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    v=beacon(6,true);
    pos=std::search(v.begin(),v.end(),std::begin(rsn_psk_ccmp),std::end(rsn_psk_ccmp));
    *(pos+11)=2; /* TKIP pairwise suite */
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_UNSUPPORTED);
    v=beacon(6,true);
    v[34]&=(uint8_t)~0x10;
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_MALFORMED);
    v=beacon(6,false);
    v[34]|=0x10;
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(ap.security==RTW_R12AU_SECURITY_UNSUPPORTED);
    v=beacon(6,false);
    v[36+1]=0U;
    v.erase(v.begin()+38,v.begin()+43);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_UNSUPPORTED);
    v=beacon(6,false);
    v[0]=0xb0;
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_NOT_BEACON);
    v=beacon(6,false);
    v[16]^=1U;
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_MALFORMED);
    v=beacon(6,false);
    v[v.size()-2U]=255U;
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_MALFORMED);
    v=beacon(6,false);
    v.insert(v.end(),{3,1,6});
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_MALFORMED);
    v=beacon(6,false);
    v.insert(v.end(),{0,1,'Z'});
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_MALFORMED);
    v=beacon(6,false);
    rtw_r12au_rx_frame rx{};
    rx.payload=v.data();rx.payload_length=v.size();rx.crc_error=1;
    CHECK(rtw_r12au_scan_parse(&rx,6,0,&ap)==RTW_R12AU_SCAN_MALFORMED);
    rx.crc_error=0;rx.is_c2h=1;
    CHECK(rtw_r12au_scan_parse(&rx,6,0,&ap)==RTW_R12AU_SCAN_MALFORMED);
    CHECK(rtw_r12au_scan_parse(nullptr,6,0,&ap)==RTW_R12AU_SCAN_BAD_ARGUMENT);
    CHECK(rtw_r12au_scan_parse(&rx,6,0,nullptr)==RTW_R12AU_SCAN_BAD_ARGUMENT);
    CHECK(rtw_r12au_scan_parse(&rx,6,5,&ap)==RTW_R12AU_SCAN_BAD_ARGUMENT);
}
static std::vector<uint8_t> response(const rtw_r12au_join_session &s,
                                     uint8_t subtype,size_t n) {
    std::vector<uint8_t> frame(n,0);
    frame[0]=subtype;
    std::copy(s.station,s.station+6,frame.begin()+4);
    std::copy(s.ap.bssid,s.ap.bssid+6,frame.begin()+10);
    std::copy(s.ap.bssid,s.ap.bssid+6,frame.begin()+16);
    return frame;
}
static void join_tests() {
    auto v=beacon(6,false);
    rtw_r12au_ap ap{};
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    rtw_r12au_join_session s{};
    CHECK(rtw_r12au_join_select(&s,&ap,sta_mac.data(),RTW_R12AU_CHIP_8812AU)==RTW_R12AU_JOIN_OK);
    CHECK(s.state==RTW_R12AU_JOIN_SELECTED);
    std::array<uint8_t,128> bytes{};
    size_t n=0U;
    CHECK(rtw_r12au_join_auth_request(&s,12,bytes.data(),29,&n)==RTW_R12AU_JOIN_BUFFER_TOO_SMALL);
    CHECK(s.state==RTW_R12AU_JOIN_SELECTED && n==0);
    CHECK(rtw_r12au_join_auth_request(&s,12,bytes.data(),bytes.size(),&n)==RTW_R12AU_JOIN_OK);
    CHECK(n==30 && bytes[0]==0xb0 && bytes[22]==0xc0);
    CHECK(rtw_r12au_join_auth_request(&s,12,bytes.data(),bytes.size(),&n)==RTW_R12AU_JOIN_BAD_STATE);
    auto resp=response(s,0xb0,30);
    put16(resp,26,2);
    resp[10]^=0x08;
    CHECK(rtw_r12au_join_auth_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_WRONG_PEER);
    resp[10]^=0x08;
    CHECK(rtw_r12au_join_auth_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_OK);
    CHECK(s.state==RTW_R12AU_JOIN_AUTHENTICATED);
    CHECK(rtw_r12au_join_assoc_request(&s,27,10,bytes.data(),4,&n)==RTW_R12AU_JOIN_BUFFER_TOO_SMALL);
    CHECK(rtw_r12au_join_assoc_request(&s,27,10,bytes.data(),bytes.size(),&n)==RTW_R12AU_JOIN_OK);
    CHECK(n<128 && bytes[0]==0);
    CHECK(bytes[n-6]==50U && bytes[n-5]==4U);
    CHECK(s.state==RTW_R12AU_JOIN_ASSOC_SENT);
    resp=response(s,0x10,33);
    put16(resp,24,ap.capability);
    put16(resp,28,0xc001U);
    resp[30]=1;resp[31]=1;resp[32]=12;
    {
        auto wrong=resp;
        put16(wrong,24,0U);
        CHECK(rtw_r12au_join_assoc_response(&s,wrong.data(),wrong.size())==RTW_R12AU_JOIN_MALFORMED);
        CHECK(s.state==RTW_R12AU_JOIN_ASSOC_SENT);
        wrong=resp;wrong[32]=127U;
        CHECK(rtw_r12au_join_assoc_response(&s,wrong.data(),wrong.size())==RTW_R12AU_JOIN_MALFORMED);
        wrong=resp;wrong.push_back(50U);wrong.push_back(1U);wrong.push_back(12U);
        CHECK(rtw_r12au_join_assoc_response(&s,wrong.data(),wrong.size())==RTW_R12AU_JOIN_MALFORMED);
        wrong=resp;wrong[31]=0U;
        CHECK(rtw_r12au_join_assoc_response(&s,wrong.data(),wrong.size())==RTW_R12AU_JOIN_MALFORMED);
    }
    CHECK(rtw_r12au_join_assoc_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_OK);
    CHECK(s.aid==1 && s.state==RTW_R12AU_JOIN_OPEN_ASSOCIATED_UNVERIFIED);
    CHECK(rtw_r12au_join_assoc_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_BAD_STATE);
    resp=response(s,0xc0,26);
    put16(resp,24,3);
    CHECK(rtw_r12au_join_disconnect(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_OK);
    CHECK(s.aid==0 && s.state==RTW_R12AU_JOIN_DISCONNECTED);
    CHECK(rtw_r12au_join_auth_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_BAD_STATE);
    /* State machine may be reselected only explicitly after disconnect. */
    CHECK(rtw_r12au_join_select(&s,&ap,sta_mac.data(),RTW_R12AU_CHIP_8821AU)==RTW_R12AU_JOIN_OK);
    rtw_r12au_join_expire(&s);
    CHECK(s.state==RTW_R12AU_JOIN_DISCONNECTED);
    CHECK(rtw_r12au_join_auth_request(&s,2,bytes.data(),bytes.size(),&n)==RTW_R12AU_JOIN_BAD_STATE);
    ap.security=RTW_R12AU_SECURITY_UNSUPPORTED;
    CHECK(rtw_r12au_join_select(&s,&ap,sta_mac.data(),RTW_R12AU_CHIP_8812AU)==RTW_R12AU_JOIN_UNSUPPORTED_AP);
    CHECK(s.state==RTW_R12AU_JOIN_IDLE);
    v=beacon(6,true);
    CHECK(scan(v,6,ap)==RTW_R12AU_SCAN_OK);
    CHECK(rtw_r12au_join_select(&s,&ap,sta_mac.data(),RTW_R12AU_CHIP_8812AU)==RTW_R12AU_JOIN_OK);
    CHECK(rtw_r12au_join_auth_request(&s,1,bytes.data(),bytes.size(),&n)==RTW_R12AU_JOIN_OK);
    resp=response(s,0xb0,30);
    put16(resp,26,2);
    CHECK(rtw_r12au_join_auth_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_OK);
    CHECK(rtw_r12au_join_assoc_request(&s,2,50,bytes.data(),bytes.size(),&n)==RTW_R12AU_JOIN_OK);
    CHECK(bytes[n-22]==48U && bytes[n-21]==20U);
    CHECK(bytes[24]&0x10U);
    resp=response(s,0x10,33);
    put16(resp,24,ap.capability);
    put16(resp,28,0xc123U);
    resp[30]=1;resp[31]=1;resp[32]=12;
    CHECK(rtw_r12au_join_assoc_response(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_OK);
    CHECK(s.state==RTW_R12AU_JOIN_WPA2_KEYS_REQUIRED);
    resp=response(s,0xa0,26);
    put16(resp,24,4);
    CHECK(rtw_r12au_join_disconnect(&s,resp.data(),resp.size())==RTW_R12AU_JOIN_OK);
    CHECK(s.state==RTW_R12AU_JOIN_DISCONNECTED);
}
/* The parser must never erase an untrusted incoming USB frame or its
 * descriptor when the caller accidentally supplies overlapping storage. */
static void scan_alias_rejection() {
    const auto original=beacon(36,false);
    std::vector<uint8_t> storage(std::max(sizeof(rtw_r12au_ap),original.size()),0);
    std::memcpy(storage.data(),original.data(),original.size());
    const auto saved=storage;
    rtw_r12au_rx_frame rx{};
    rx.payload=storage.data(); rx.payload_length=original.size();
    CHECK(rtw_r12au_scan_parse(&rx,36,0,
          reinterpret_cast<rtw_r12au_ap*>(storage.data())) ==
          RTW_R12AU_SCAN_BAD_ARGUMENT);
    CHECK(storage==saved);

    alignas(rtw_r12au_ap) uint8_t desc[
        sizeof(rtw_r12au_ap)>sizeof(rtw_r12au_rx_frame) ?
        sizeof(rtw_r12au_ap):sizeof(rtw_r12au_rx_frame)]{};
    rx.payload=original.data(); rx.payload_length=original.size();
    std::memcpy(desc,&rx,sizeof(rx));
    uint8_t before[sizeof(desc)]{};
    std::memcpy(before,desc,sizeof(desc));
    CHECK(rtw_r12au_scan_parse(reinterpret_cast<const rtw_r12au_rx_frame*>(desc),
          36,0,reinterpret_cast<rtw_r12au_ap*>(desc)) ==
          RTW_R12AU_SCAN_BAD_ARGUMENT);
    CHECK(std::memcmp(desc,before,sizeof(desc))==0);
}
static void fuzz_tests() {
    std::mt19937 gen(0x88128821U);
    std::array<uint8_t,512> data{};
    for (unsigned n=0;n<50000;n++) {
        for (auto &byte:data) byte=static_cast<uint8_t>(gen());
        size_t len=gen()%(data.size()+1U);
        rtw_r12au_rx_frame rx{};
        rx.payload=data.data();rx.payload_length=len;
        rtw_r12au_ap ap{};
        auto result=rtw_r12au_scan_parse(&rx,(n&1)?6:36,0,&ap);
        CHECK(result<=RTW_R12AU_SCAN_UNSUPPORTED);
        if (result==RTW_R12AU_SCAN_OK) {
            CHECK(ap.ssid_length>=1 && ap.ssid_length<=32);
            CHECK(ap.rate_count>=1 && ap.rate_count<=12);
        } else {
            CHECK(ap.ssid_length==0);
        }
    }
}
int main() {
    rf_tests();scan_tests();join_tests();scan_alias_rejection();fuzz_tests();
    std::cout << "RTL8812AU/RTL8821AU offline RF and join checks: "
              << checks << '\n';
}
