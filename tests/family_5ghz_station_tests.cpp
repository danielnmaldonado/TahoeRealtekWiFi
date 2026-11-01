/* SPDX-License-Identifier: GPL-2.0-only
 * Test actual 802.11 5-GHz management-frame preparation for the seven
 * dual-band families. These synthetic frames do NOT verify radio/TX/WPA2. */
#include "rtw/Family5GHzStation.h"
#include "rtw/Family5GHzScan.h"
#include "rtw/FCS.h"
#include <array>
#include <cassert>
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

static size_t checks;
#define CHECK(c) do { ++checks; if (!(c)) { \
    std::fprintf(stderr,"5-GHz station contract %s:%d: %s\n", \
        __FILE__,__LINE__,#c); std::abort(); } } while (0)

static constexpr std::array<uint8_t, 6> sta{{0x02,0xaa,0xbb,0xcc,0xdd,0xee}};
static constexpr std::array<uint8_t, 6> bssid{{0x02,0x11,0x22,0x33,0x44,0x55}};
static constexpr std::array<rtw_wifi_family, 7> families{{
    RTW_WIFI_RTL8192DU, RTW_WIFI_RTL8812AU, RTW_WIFI_RTL8814AU,
    RTW_WIFI_RTL8821AU, RTW_WIFI_RTL8821CU, RTW_WIFI_RTL8822BU,
    RTW_WIFI_RTL8822CU
}};
static const rtw_usb_wifi_profile *verified(rtw_wifi_family family) {
    for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        auto *p = rtw_usb_wifi_profile_at(i);
        if (p && p->family == family && p->primary_verified) return p;
    }
    return nullptr;
}
static rtw_r12au_ap make_ap(uint8_t channel = 36, uint8_t width = 80) {
    rtw_r12au_ap ap{};
    std::memcpy(ap.bssid, bssid.data(), 6U);
    const uint8_t ssid[] = {'T','a','h','o','e','5','G'};
    std::memcpy(ap.ssid, ssid, sizeof(ssid));
    ap.ssid_length = sizeof(ssid);
    const uint8_t rates[] = {0x8c,0x12,0x98,0x24,0x30,0x48,0x60,0x6c};
    std::memcpy(ap.rates, rates, sizeof(rates));
    ap.rate_count = sizeof(rates);
    ap.primary_channel = channel;
    ap.center_channel = width == 80 ? (channel == 52 ? 58 : 42) :
        (width == 40 ? (uint8_t)(channel + 2U) : channel);
    ap.operating_width_mhz = width;
    ap.ht_capable = width >= 40;
    ap.vht_capable = width == 80;
    ap.capability = 0x0011; /* ESS + privacy */
    ap.security = RTW_R12AU_SECURITY_WPA2_PSK_CCMP;
    ap.beacon_interval = 100;
    return ap;
}
static void emit_ap_response(const rtw_family_5g_join_session &s,
                             uint8_t *out, uint8_t subtype) {
    std::memset(out, 0, 64U);
    out[0] = subtype;
    std::memcpy(out + 4, s.management.station, 6U);
    std::memcpy(out + 10, s.management.ap.bssid, 6U);
    std::memcpy(out + 16, s.management.ap.bssid, 6U);
}
static void roundtrip(rtw_wifi_family family) {
    const auto *profile = verified(family);
    CHECK(profile);
    const uint8_t width = family == RTW_WIFI_RTL8192DU ? 40U : 80U;
    auto ap = make_ap(36, width);
    rtw_family_5g_join_session s{};
    CHECK(rtw_family_5g_join_prepare(profile,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_OK);
    CHECK(s.profile == profile && s.observed_channel.is_5ghz);
    CHECK(s.observed_channel.passive_only && !s.observed_channel.dfs_required);
    CHECK(s.management.state == RTW_R12AU_JOIN_SELECTED);
    uint8_t outgoing[RTW_R12AU_ASSOC_FRAME_MAX]{};
    size_t n = 999U;
    CHECK(rtw_r12au_join_assoc_request(&s.management,0,10,outgoing,
          sizeof(outgoing),&n) == RTW_R12AU_JOIN_BAD_STATE && n == 0);
    CHECK(rtw_r12au_join_auth_request(&s.management,15,outgoing,
          sizeof(outgoing),&n) == RTW_R12AU_JOIN_OK && n == 30U);
    CHECK(outgoing[0] == 0xb0 && outgoing[26] == 1);
    uint8_t reply[64]{};
    emit_ap_response(s,reply,0xb0);
    reply[26] = 2U; /* Open system authentication response */
    CHECK(rtw_r12au_join_auth_response(&s.management,reply,30)
          == RTW_R12AU_JOIN_OK);
    CHECK(s.management.state == RTW_R12AU_JOIN_AUTHENTICATED);
    CHECK(rtw_r12au_join_assoc_request(&s.management,16,10,outgoing,
          sizeof(outgoing),&n) == RTW_R12AU_JOIN_OK && n > 40);
    CHECK(outgoing[0] == 0x00 && outgoing[n-22] == 48); /* WPA2 RSN IE */
    emit_ap_response(s,reply,0x10);
    reply[24] = 0x11; /* ESS + privacy */
    reply[28] = 0x01; reply[29] = 0xc0; /* AID=1 */
    reply[30] = 1U; reply[31] = 8U;
    std::memcpy(reply+32,ap.rates,8U);
    CHECK(rtw_r12au_join_assoc_response(&s.management,reply,40)
          == RTW_R12AU_JOIN_OK);
    CHECK(s.management.state == RTW_R12AU_JOIN_WPA2_KEYS_REQUIRED);
    /* Completing management negotiation is not proof of WPA2 keys, carrier
     * readiness, calibrated RF or regulatory TX authorization. */
    CHECK(bool(rtw_wifi_radio_profile_capabilities(profile).station_bands &
             RTW_WIFI_BAND_5GHZ)==(station5gFamily(profile->family)));
    rtw_family_5g_join_reset(&s);
    CHECK(s.profile == nullptr && s.management.state == RTW_R12AU_JOIN_IDLE);
}
static void negative() {
    const auto *p = verified(RTW_WIFI_RTL8812AU);
    const auto *du = verified(RTW_WIFI_RTL8192DU);
    CHECK(p && du);
    auto ap = make_ap();
    rtw_family_5g_join_session s{};
    rtw_usb_wifi_profile forged = *p;
    CHECK(rtw_family_5g_join_prepare(&forged,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_UNVERIFIED_USB);
    CHECK(s.profile == nullptr);
    CHECK(rtw_family_5g_join_prepare(nullptr,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_BAD_ARGUMENT);
    CHECK(rtw_family_5g_join_prepare(p,&ap,nullptr,36,&s)
          == RTW_FAMILY_5G_JOIN_BAD_ARGUMENT);
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),36,nullptr)
          == RTW_FAMILY_5G_JOIN_BAD_ARGUMENT);
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),40,&s)
          == RTW_FAMILY_5G_JOIN_BAD_CHANNEL);
    CHECK(rtw_family_5g_join_prepare(du,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_BAD_CHANNEL); /* 8192DU has no VHT80 */
    ap = make_ap(36,40); ap.ht_capable = 0;
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_UNSUPPORTED_AP);
    ap = make_ap(); ap.vht_capable = 0;
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_UNSUPPORTED_AP);
    ap = make_ap(); ap.pmf_required = 1U;
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_UNSUPPORTED_AP);
    ap = make_ap(); ap.center_channel = 58U;
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_BAD_CHANNEL);
    ap = make_ap(52,20);
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),52,&s)
          == RTW_FAMILY_5G_JOIN_OK);
    CHECK(s.observed_channel.dfs_required && s.observed_channel.passive_only);
    ap = make_ap(6,20);
    CHECK(rtw_family_5g_join_prepare(p,&ap,sta.data(),6,&s)
          == RTW_FAMILY_5G_JOIN_BAD_CHANNEL);
    CHECK(s.profile == nullptr); /* no stale join state after failed call */
    const auto *single = verified(RTW_WIFI_RTL8192EU);
    CHECK(single);
    ap = make_ap();
    CHECK(rtw_family_5g_join_prepare(single,&ap,sta.data(),36,&s)
          == RTW_FAMILY_5G_JOIN_UNSUPPORTED_FAMILY);
    CHECK(s.profile == nullptr);
    /* Check all 270 catalogued USB identities, not just seven exemplars. */
    size_t accepted = 0U;
    for (size_t i = 0; i < rtw_usb_wifi_profile_count(); ++i) {
        const auto *profile = rtw_usb_wifi_profile_at(i);
        CHECK(profile);
        const auto caps = rtw_wifi_radio_profile_capabilities(profile);
        const bool dual = (caps.family_bands & RTW_WIFI_BAND_5GHZ) != 0U;
        auto candidate = make_ap(36,20);
        const auto result = rtw_family_5g_join_prepare(profile,&candidate,
                                                       sta.data(),36,&s);
        const bool enabled = profile->primary_verified && dual;
        CHECK((result == RTW_FAMILY_5G_JOIN_OK) == enabled);
        if (enabled) {
            ++accepted;
            CHECK(s.profile == profile && s.observed_channel.passive_only);
            CHECK(bool(caps.station_bands & RTW_WIFI_BAND_5GHZ)==(station5gFamily(profile->family)));
        } else CHECK(s.profile == nullptr);
    }
    CHECK(accepted > 0U && accepted < rtw_usb_wifi_profile_count());
}
/* Connect the EXISTING validated USB RX path to the new management
 * preparation path. No mock RF driver is used and nothing transmits. */
struct ScanContext {
    const rtw_usb_wifi_profile *profile;
    rtw_family_5g_join_session session{};
    rtw_family_5g_join_result result{RTW_FAMILY_5G_JOIN_BAD_ARGUMENT};
    unsigned candidates{0U};
};
static int select_from_rx(void *cookie, const rtw_r12au_ap *ap,
                          const rtw_r12au_rx_frame *frame) {
    auto &ctx = *static_cast<ScanContext *>(cookie);
    CHECK(frame && frame->payload && ap);
    ++ctx.candidates;
    ctx.result = rtw_family_5g_join_prepare(ctx.profile,ap,sta.data(),
                                              36,&ctx.session);
    return 1;
}
static void put_ie(std::vector<uint8_t> &bytes, uint8_t type,
                   const std::vector<uint8_t> &value) {
    CHECK(value.size() <= 255U);
    bytes.push_back(type);
    bytes.push_back(static_cast<uint8_t>(value.size()));
    bytes.insert(bytes.end(),value.begin(),value.end());
}
static std::vector<uint8_t> make_rx_beacon() {
    std::vector<uint8_t> bytes(36U,0U);
    bytes[0]=0x80U;
    for(unsigned i=4U;i<10U;++i) bytes[i]=0xffU;
    std::memcpy(bytes.data()+10U,bssid.data(),6U);
    std::memcpy(bytes.data()+16U,bssid.data(),6U);
    bytes[32]=100;bytes[34]=0x11; /* ESS+privacy */
    put_ie(bytes,0,{'T','a','h','o','e','5','G'});
    put_ie(bytes,1,{0x8c,0x12,0x98,0x24,0x30,0x48,0x60,0x6c});
    put_ie(bytes,48,{1,0,0,15,172,4,1,0,0,15,172,4,
                     1,0,0,15,172,2,0,0});
    std::vector<uint8_t> ht(22,0);
    ht[0]=36; ht[1]=5;
    put_ie(bytes,61,ht);
    put_ie(bytes,192,{1,42,0,0xff,0xff});
    const uint32_t crc=rtw_fcs32(bytes.data(),bytes.size());
    for(unsigned i=0;i<4U;++i)
        bytes.push_back(static_cast<uint8_t>(crc>>(8U*i)));
    return bytes;
}
static void rx_to_management() {
    const auto beacon = make_rx_beacon();
    std::vector<uint8_t> bulk((24U+beacon.size()+7U)&~size_t(7U),0U);
    bulk[0]=static_cast<uint8_t>(beacon.size());
    bulk[1]=static_cast<uint8_t>(beacon.size()>>8U);
    bulk[12]=4U; /* OFDM 6Mbps RX */
    std::memcpy(bulk.data()+24U,beacon.data(),beacon.size());
    for (auto family: families) {
        if (family == RTW_WIFI_RTL8192DU) continue; /* no verified RX codec */
        ScanContext ctx{};
        ctx.profile=verified(family);
        CHECK(ctx.profile);
        rtw_family_5g_scan_stats stats{};
        CHECK(rtw_family_5g_scan_bulk(ctx.profile,bulk.data(),bulk.size(),
            36,1,select_from_rx,&ctx,&stats) == RTW_FAMILY_5G_SCAN_OK);
        CHECK(ctx.candidates == 1U && ctx.result == RTW_FAMILY_5G_JOIN_OK);
        CHECK(ctx.session.management.ap.security ==
              RTW_R12AU_SECURITY_WPA2_PSK_CCMP);
        CHECK(ctx.session.observed_channel.width_mhz == 80U);
        CHECK(ctx.session.observed_channel.passive_only);
        rtw_family_5g_join_reset(&ctx.session);
    }
}
/* Caller output must not overwrite any read-only input, even on an
 * unsuccessful validation. This is a C ABI and overlapping pointers are
 * representable; do not clear or write output before validating aliases. */
static void reject_aliased_inputs() {
    const auto *profile=verified(RTW_WIFI_RTL8812AU);
    CHECK(profile);
    const auto ap=make_ap();
    constexpr size_t n=sizeof(rtw_family_5g_join_session);
    alignas(rtw_family_5g_join_session) uint8_t storage[n]{};
    uint8_t snapshot[n]{};
    std::memcpy(storage,&ap,sizeof(ap));
    std::memcpy(snapshot,storage,n);
    auto *out=reinterpret_cast<rtw_family_5g_join_session*>(storage);
    auto *ap_alias=reinterpret_cast<const rtw_r12au_ap*>(storage);
    CHECK(rtw_family_5g_join_prepare(profile,ap_alias,sta.data(),40,out)
          == RTW_FAMILY_5G_JOIN_BAD_ARGUMENT);
    CHECK(std::memcmp(storage,snapshot,n)==0);

    std::memset(storage,0,sizeof(storage));
    std::memcpy(storage,sta.data(),sta.size());
    std::memcpy(snapshot,storage,n);
    CHECK(rtw_family_5g_join_prepare(profile,&ap,storage,40,out)
          == RTW_FAMILY_5G_JOIN_BAD_ARGUMENT);
    CHECK(std::memcmp(storage,snapshot,n)==0);

    const rtw_usb_wifi_profile forged=*profile;
    std::memset(storage,0,sizeof(storage));
    std::memcpy(storage,&forged,sizeof(forged));
    std::memcpy(snapshot,storage,n);
    const auto *profile_alias=reinterpret_cast<const rtw_usb_wifi_profile*>(storage);
    CHECK(rtw_family_5g_join_prepare(profile_alias,&ap,sta.data(),36,out)
          == RTW_FAMILY_5G_JOIN_BAD_ARGUMENT);
    CHECK(std::memcmp(storage,snapshot,n)==0);
}
int main() {
    for (auto family: families) roundtrip(family);
    negative();
    reject_aliased_inputs();
    rx_to_management();
    std::printf("5-GHz dual-band 802.11 management preparation: %zu assertions (NO RF/TX)\n",checks);
}
