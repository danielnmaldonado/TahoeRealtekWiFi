#ifdef NDEBUG
#error "Regression assertions must remain active in Release builds"
#endif
// SPDX-License-Identifier: GPL-2.0-only
// All 270 immutable profiles: hardware family ceilings MUST NOT silently
// become implemented modes or grant USB matching, RF transmit or DFS access.
#include "rtw/USBWiFiProfiles.h"
#include "rtw/FamilyRadioPolicy.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>

struct FamilyExpectation {
    rtw_wifi_family family;
    unsigned hardware_bands, nominal_phy_mbps, driver_bands, driver_phy_mbps;
    unsigned family_nss;
};
static constexpr std::array<FamilyExpectation,14> expected{{
    {RTW_WIFI_RTL8192CU,1,300,1,144,2},
    {RTW_WIFI_RTL8188EU,1,150,1,72,1},
    {RTW_WIFI_RTL8192EU,1,300,1,300,2},
    {RTW_WIFI_RTL8192DU,3,300,3,144,2},
    {RTW_WIFI_RTL8192FU,1,300,1,300,2},
    {RTW_WIFI_RTL8710BU,1,150,1,150,1},
    {RTW_WIFI_RTL8723BU,1,150,1,72,1},
    {RTW_WIFI_RTL8812AU,3,867,3,867,2},
    {RTW_WIFI_RTL8814AU,3,1300,3,144,3},
    {RTW_WIFI_RTL8814BU,0,0,0,0,0},
    {RTW_WIFI_RTL8821AU,3,433,3,433,1},
    {RTW_WIFI_RTL8821CU,3,433,3,433,1},
    {RTW_WIFI_RTL8822BU,3,867,3,867,2},
    {RTW_WIFI_RTL8822CU,3,867,3,867,2}
}};
int main() {
    assert(rtw_usb_wifi_profile_count()==270);
    assert(rtw_wifi_radio_family_capabilities(RTW_WIFI_UNKNOWN).family_bands==0);
    assert(rtw_wifi_radio_profile_capabilities(nullptr).family_bands==0);
    rtw_usb_wifi_profile forged{};
    forged.primary_verified=1;
    forged.family=RTW_WIFI_RTL8814AU;
    assert(rtw_wifi_radio_profile_capabilities(&forged).family_bands==0);
    // Even a rejected, forged identity must not be overwritten if the
    // caller aliases the output channel plan with its input profile.
    alignas(rtw_r12au_channel_plan) uint8_t alias[
        sizeof(rtw_usb_wifi_profile)>sizeof(rtw_r12au_channel_plan) ?
        sizeof(rtw_usb_wifi_profile):sizeof(rtw_r12au_channel_plan)]{};
    std::memcpy(alias,&forged,sizeof(forged));
    uint8_t before[sizeof(alias)]{};
    std::memcpy(before,alias,sizeof(alias));
    assert(rtw_family_radio_prepare_profile(
        reinterpret_cast<const rtw_usb_wifi_profile*>(alias),36,36,20,
        reinterpret_cast<rtw_r12au_channel_plan*>(alias)) ==
        RTW_FAMILY_RADIO_INVALID_ARGUMENT);
    assert(std::memcmp(alias,before,sizeof(alias))==0);

    unsigned profiles=0, ready=0, verified=0, dual_family=0;
    for(const auto& e:expected) {
        const auto c=rtw_wifi_radio_family_capabilities(e.family);
        assert(c.family_bands==e.hardware_bands);
        assert(c.station_bands==e.driver_bands);
        assert(c.family_max_phy_mbps==e.nominal_phy_mbps);
        assert(c.station_max_phy_mbps==e.driver_phy_mbps);
        assert(c.family_max_nss==e.family_nss);
        assert((c.station_bands & ~c.family_bands)==0);
        assert(c.station_max_nss<=c.family_max_nss);
        assert(c.station_max_2g_width_mhz<=c.family_max_2g_width_mhz);
        assert(c.station_max_5g_width_mhz<=c.family_max_5g_width_mhz);
        const bool wide=e.family==RTW_WIFI_RTL8812AU||e.family==RTW_WIFI_RTL8821AU||e.family==RTW_WIFI_RTL8821CU||e.family==RTW_WIFI_RTL8822BU||e.family==RTW_WIFI_RTL8822CU;
        assert(c.station_max_5g_width_mhz==((e.driver_bands&RTW_WIFI_BAND_5GHZ)?(wide?80:20):0));
        assert(!c.identity_verified); // A family name cannot verify a USB ID
        rtw_r12au_channel_plan plan{};
        assert(rtw_family_radio_prepare(e.family,6,6,20,&plan)==
               (e.hardware_bands ? RTW_FAMILY_RADIO_OK : RTW_FAMILY_RADIO_UNSUPPORTED));
        if(e.hardware_bands) {
            assert(plan.passive_only && plan.primary_frequency_mhz==2437);
            assert(plan.rf_chains==e.family_nss);
        }
        assert(rtw_family_radio_prepare(e.family,36,36,20,&plan)==
               (e.hardware_bands&RTW_WIFI_BAND_5GHZ ? RTW_FAMILY_RADIO_OK :
               RTW_FAMILY_RADIO_UNSUPPORTED));
        if(e.hardware_bands&RTW_WIFI_BAND_5GHZ)
            assert(plan.passive_only && plan.is_5ghz);
        assert(rtw_family_radio_prepare(e.family,36,42,80,&plan)==
               (c.family_max_5g_width_mhz>=80 ? RTW_FAMILY_RADIO_OK :
               RTW_FAMILY_RADIO_UNSUPPORTED));
        if(c.family_max_5g_width_mhz>=80)
            assert(plan.passive_only && plan.is_5ghz);
        assert(rtw_family_radio_prepare(e.family,52,52,20,&plan)==
               (e.hardware_bands&RTW_WIFI_BAND_5GHZ ? RTW_FAMILY_RADIO_OK :
               RTW_FAMILY_RADIO_UNSUPPORTED));
        if(e.hardware_bands&RTW_WIFI_BAND_5GHZ)
            assert(plan.passive_only && plan.dfs_required);
    }
    for(size_t i=0;i<rtw_usb_wifi_profile_count();++i) {
        const auto* p=rtw_usb_wifi_profile_at(i);
        assert(p);
        const auto c=rtw_wifi_radio_profile_capabilities(p);
        const auto family=rtw_wifi_radio_family_capabilities(p->family);
        const bool enabled=rtw_usb_wifi_station_backend(p)==p->family;
        assert(c.identity_verified==p->primary_verified);
        const bool onlyFive=(p->vendor==0x04bb&&(p->product==0x0959||p->product==0x095a))||(p->vendor==0x0bda&&p->product==0x2003);
        const unsigned permitted=onlyFive?2U:3U;
        assert(c.family_bands==(family.family_bands&permitted));
        assert(rtw_usb_wifi_profile_allowed_bands(p)==permitted);
        if(onlyFive){assert(!c.station_max_2g_width_mhz&&!c.family_max_2g_width_mhz);rtw_r12au_channel_plan denied{};assert(rtw_family_radio_prepare_profile(p,6,6,20,&denied)==RTW_FAMILY_RADIO_UNSUPPORTED);}

        assert(c.family_max_phy_mbps==family.family_max_phy_mbps);
        assert(c.station_bands==(enabled?(family.station_bands&permitted):0));
        assert(c.station_max_phy_mbps==(enabled?family.station_max_phy_mbps:0));
        assert(bool(c.station_bands & RTW_WIFI_BAND_5GHZ)==(enabled&&bool(family.station_bands&RTW_WIFI_BAND_5GHZ)));
        assert(!(c.station_bands & ~c.family_bands));
        if(!p->primary_verified) assert(!enabled && !c.station_bands);
        if(c.family_bands & RTW_WIFI_BAND_5GHZ)++dual_family;
        ++profiles; ready+=enabled; verified+=p->primary_verified;
    }
    assert(profiles==270 && ready==262 && verified==263);
    std::printf("Radio capability policy: %u USB IDs, %u verified, %u station, %u nominal dual-band candidates; verified dual-band station paths enabled (ROM/RFE runtime checks apply).\n",
                profiles,verified,ready,dual_family);
}
