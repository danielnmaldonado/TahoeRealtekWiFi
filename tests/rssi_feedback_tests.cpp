// SPDX-License-Identifier: GPL-2.0-only
#include "production.hpp"
#include <array>

int main() {
    constexpr rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,
        RTW_WIFI_RTL8192EU,RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,
        RTW_WIFI_RTL8723BU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8821AU,
        RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
    unsigned checks=0;
    for(auto family:families) {
      for(int previous:{-1,73}) {
        for(unsigned scenario=0;scenario<13;++scenario) {
            RTWUSBController s;s.family_=family;
            s.core_.avg_pwdb=previous;
            ieee80211_node ap;s.core_.sc_ic.ic_bss=&ap;
            std::array<uint8_t,32> phy{};
            std::array<uint8_t,26> frame{};
            frame[0]=0x88;frame[1]=2;
            std::memcpy(frame.data()+10,ap.ni_bssid,6);
            std::memcpy(frame.data()+16,ap.ni_bssid,6);
            rtw_rx_packet p{};p.mpdu=frame.data();p.mpdu_length=frame.size();
            p.phy_info=phy.data();p.phy_info_length=phy.size();p.phy_status_present=1;p.rate_index=4;
            measured=-55;percent=51;
            bool accept=scenario<3;
            switch(scenario) {
            case 1: frame[0]=0x80;frame[1]=0;break;
            case 2: frame[1]|=0x40;break; // AP during WPA2/key exchange
            case 3: frame[10]^=4;break;
            case 4: frame[0]=0x80;frame[1]=0;frame[16]^=4;break;
            case 5: frame[1]=1;break;
            case 6: frame[1]=3;break;
            case 7: s.core_.sc_ic.ic_state=1;break; // scan keeps per-frame RSSI
            case 8: s.core_.sc_ic.ic_state=3;break;
            case 9: s.core_.sc_ic.ic_bss=nullptr;break;
            case 10: measured=0;break;
            case 11: p.phy_status_present=0;break;
            case 12: p.mpdu_length=23;break;
            }
            const auto nodeRSSI=s.receiveSignal(p);
            if(scenario!=10&&scenario!=11) assert(nodeRSSI==45);
            else assert(nodeRSSI==0);
            const bool legacy=family==RTW_WIFI_RTL8192CU||family==RTW_WIFI_RTL8188EU||family==RTW_WIFI_RTL8192EU;
            const int expected=accept?(legacy||previous<0?45:(previous*19+45)/20):previous;
            assert(s.core_.avg_pwdb==expected);
            assert(s.core_.updates==(accept&&legacy?1U:0U));
            assert(s.acRSSI_==(accept&&family==RTW_WIFI_RTL8814AU?51:17));
            ++checks;
        }
      }
    }
    std::printf("Production RSSI feedback: %u cases across 13 families passed; scan/node RSSI preserved\n",checks);
}
