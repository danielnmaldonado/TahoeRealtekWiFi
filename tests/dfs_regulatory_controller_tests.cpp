// SPDX-License-Identifier: GPL-2.0-only
#include "rtw/USBWiFiProfiles.h"
#include "rtw/RTL8814AU.h"
#include "rtw/RTL8812AUBootstrap.h"
#include "rtw/RTW88Boot.h"
#include <cstdio>
#include <cstring>
class RTWUSBController {
public:
    rtw_wifi_family family_=RTW_WIFI_UNKNOWN;
    rtw_8814au acCore_{};
    rtw_r12au_boot auCore_{};
    rtw88_boot_context r88Core_{};
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool permitsDFSChannelByCountry(uint8_t)const;
    int16_t factoryDFSChannelLimitMBM(uint8_t)const;
};
#include "../kext/RTWDFSRegulatory.inc"
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"Factory country line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main() {
    const rtw_wifi_family families[]={RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
    for(auto family:families) {
        RTWUSBController c;c.family_=family;
        std::memcpy(c.acCore_.board.country_code,"EG",2);
        std::memcpy(c.auCore_.board.country,"EG",2);
        std::memcpy(c.r88Core_.board.country_code,"EG",2);
        CHECK(c.permitsDFSChannelByCountry(52));
        CHECK(c.permitsDFSChannelByCountry(100)==(family==RTW_WIFI_RTL8192DU));
        CHECK(c.permitsDFSChannelByCountry(36));
        CHECK(c.factoryDFSChannelLimitMBM(52)==2000);
        std::memcpy(c.acCore_.board.country_code,"BR",2);
        std::memcpy(c.auCore_.board.country,"BR",2);
        std::memcpy(c.r88Core_.board.country_code,"BR",2);
        CHECK(c.permitsDFSChannelByCountry(140));
        CHECK(c.factoryDFSChannelLimitMBM(140)==(family==RTW_WIFI_RTL8192DU?2000:2700));
    }
    puts("Actual controller factory-country selection: all7 backends passed");
}
