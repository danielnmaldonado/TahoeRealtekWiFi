#include "rtw/RTW88RX.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){
 std::array<uint8_t,32>status{};rtw_rx_packet p{};p.phy_status_present=1;p.phy_info=status.data();p.phy_info_length=32;p.rate_index=12;
 for(auto f:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  rtw88_rx_phy_config c{f,uint8_t(f==RTW_WIFI_RTL8821CU?1:2),2,20,40,0};rtw88_rx_phy_sample s{};
  status.fill(0);status[0]=1;status[1]=50;status[2]=70;status[5]=0x10;status[6]=36;
  assert(rtw88_rx_parse_phy(&c,&p,&s));assert(s.signal_dbm==(f==RTW_WIFI_RTL8821CU?-60:-40));assert(s.bandwidth==0&&s.rssi>0);
  status[5]=0x90;assert(rtw88_rx_parse_phy(&c,&p,&s)&&s.bandwidth==1);status[0]=2;assert(!rtw88_rx_parse_phy(&c,&p,&s));
 }
 rtw88_rx_phy_config c{RTW_WIFI_RTL8821CU,1,0,0,0,0};status.fill(0);status[13]=uint8_t((5<<5)|10);p.rate_index=0;rtw88_rx_phy_sample sample{};assert(rtw88_rx_parse_phy(&c,&p,&sample)&&sample.signal_dbm==-60);
 c.rfe_option=2;assert(rtw88_rx_parse_phy(&c,&p,&sample)&&sample.signal_dbm==-30);
 status[14]=0x80;c.rfe_option=0;assert(!rtw88_rx_parse_phy(&c,&p,&sample));
 c={RTW_WIFI_RTL8822CU,2,2,20,40,0};status.fill(0);status[1]=60;status[2]=10;status[6]=6;status[16]=70;status[19]=50;assert(rtw88_rx_parse_phy(&c,&p,&sample)&&sample.signal_dbm==-30);
 std::array<uint8_t,9>report{0xc,1,19,0,0,0,0,0,0};rtw_8814au_rate_config rates{};rates.mask=1U<<19;rates.ht=1;rates.tx_streams=1;uint8_t rate=0;assert(rtw88_rx_parse_rate_report(report.data(),report.size(),&rates,&rate)==1&&rate==19);
 report[8]=1;assert(rtw88_rx_parse_rate_report(report.data(),9,&rates,&rate)==-1);report[8]=0;report[2]=20;assert(rtw88_rx_parse_rate_report(report.data(),9,&rates,&rate)==-1);report[0]=3;assert(!rtw88_rx_parse_rate_report(report.data(),9,&rates,&rate));
 std::puts("rtw88 RX PHY pages and seven-byte HT20 C2H golden vectors passed");
}
