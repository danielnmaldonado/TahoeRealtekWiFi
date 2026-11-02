/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation.
 * Checked primary Linux rtw88 RX24,page0/page1 PHY and C2H RA adaptation.
 * Commit0e135910deec43ffe97eca0b6af1f8f847285b59. No IO or allocation. */
#include "rtw/RTW88RX.h"
#include <string.h>
#include "RTW88RXRSSI.inc"
static int family(rtw_wifi_family f){return f==RTW_WIFI_RTL8821CU||f==RTW_WIFI_RTL8822BU||f==RTW_WIFI_RTL8822CU;}
static uint32_t get32(const uint8_t*b){return(uint32_t)b[0]|((uint32_t)b[1]<<8U)|((uint32_t)b[2]<<16U)|((uint32_t)b[3]<<24U);}
static int valid(const rtw88_rx_phy_config*c){return c&&family(c->family)&&c->rf_paths>=1&&c->rf_paths<=(c->family==RTW_WIFI_RTL8821CU?1:2)&&c->current_width<=2;}
static int8_t signed_power(int p){return(int8_t)(p<-128?-128:p>127?127:p);}
int rtw88_rx_parse_phy(const rtw88_rx_phy_config*c,const rtw_rx_packet*p,rtw88_rx_phy_sample*out){
 static const int8_t gain0[8]={22,8,-6,-22,-31,-40,-46,-52};
 static const int8_t gain1[16]={10,6,2,-2,-6,-10,-14,-17,-20,-24,-28,-31,-34,-37,-40,-44};
 rtw88_rx_phy_sample s;const uint8_t*b;unsigned page,paths;
 if(out)memset(out,0,sizeof(*out));
 if(!valid(c)||!p||!out||!p->phy_status_present||!p->phy_info||p->phy_info_length!=32||p->rate_index>=84)return 0;
 memset(&s,0,sizeof(s));b=p->phy_info;page=b[0]&15U;
 if(page>1||(page==0)!=(p->rate_index<=3U))return 0;
 if(page==0){int power;
  paths=1;
  if(c->family==RTW_WIFI_RTL8821CU){unsigned lna=((b[14]>>7U)<<3U)|(b[13]>>5U),vga=b[13]&31U;if((!c->rfe_option&&lna>=8)||lna>=16)return 0;power=(c->rfe_option?gain1[lna]:gain0[lna])-2*(int)vga;}
  else if(c->family==RTW_WIFI_RTL8822BU)power=(int)b[1]-110;
  else{if(c->cck_gain_low>c->cck_gain_high||c->cck_gain_high>63)return 0;int gain=b[2]&63U;power=(int)b[1];if(gain<c->cck_gain_low)power+=2*((int)c->cck_gain_low-gain);else if(gain>c->cck_gain_high)power-=2*(gain-c->cck_gain_high);power-=110;s.channel=b[6];}
  s.path_dbm[0]=signed_power(power);s.signal_dbm=s.path_dbm[0]<-120?-120:s.path_dbm[0];s.bandwidth=0;
 }else{
  unsigned rxsc=(p->rate_index<12?b[5]&15U:b[5]>>4U);
  paths=c->rf_paths;s.bandwidth=(uint8_t)(rxsc==0?(c->family==RTW_WIFI_RTL8822CU?c->current_width:(b[15]>>4U)&3U):rxsc<=8?0:rxsc<=12?1:2);
  if(s.bandwidth>2)return 0;
  s.path_dbm[0]=signed_power((int)b[1]-110);s.path_dbm[1]=signed_power((int)b[2]-110);
  s.signal_dbm=s.path_dbm[0];if(paths==2&&s.path_dbm[1]>s.signal_dbm)s.signal_dbm=s.path_dbm[1];if(s.signal_dbm<-120)s.signal_dbm=-120;
  if(c->family==RTW_WIFI_RTL8822CU)s.channel=b[6];
 }
 s.rssi=rtw_phy_rf_power_2_rssi(s.path_dbm,(uint8_t)paths);s.measured_paths=(uint8_t)paths;*out=s;return 1;
}
rtw_packet_error rtw88_rx_next(const rtw88_rx_phy_config*c,rtw_rx_iterator*it,rtw_8814au_rx_frame*out){
 if(!valid(c)||!it||!out){if(out)memset(out,0,sizeof(*out));return RTW_PACKETS_BAD_ARGUMENT;}
 size_t cursor=it->cursor;rtw_packet_error e=rtw_8814au_rx_next(it,out);if(e!=RTW_PACKETS_OK)return e;
 /* Shared RX24 decoder's Jaguar PHY interpretation is overridden by the
  * B/C descriptor then the chip-specific page format,never retained. */
 out->packet.bandwidth_code=(uint8_t)((get32(it->buffer+cursor+16U)>>4U)&3U);
 rtw88_rx_phy_sample sample;if(!out->is_c2h&&rtw88_rx_parse_phy(c,&out->packet,&sample))out->packet.bandwidth_code=sample.bandwidth;
 return RTW_PACKETS_OK;
}
int rtw88_rx_parse_rate_report(const uint8_t*b,size_t n,const rtw_8814au_rate_config*c,uint8_t*rate){
 if(rate)*rate=0;if(!b||!rate||n<2||n>257)return-1;if(b[0]!=0xc)return 0;
 if(n!=9||!c||!c->mask||c->tx_streams<1||c->tx_streams>2||b[3]||b[8]||((b[2]&0x80)&&!c->sgi20))return-1;
 uint8_t r=b[2]&127U;if(r>27||!(c->mask&(UINT64_C(1)<<r))||(r>=12&&!c->ht)||(r>=20&&c->tx_streams<2))return-1;*rate=r;return 1;
}
int rtw88_rx_parse_station_rate_report(const uint8_t*b,size_t n,const rtw88_station_rate*c,uint8_t*rate){
 unsigned width;if(rate)*rate=0;if(!b||!rate||n<2||n>257)return-1;if(b[0]!=0xc)return 0;
 if(n!=9||!c||b[3]||((b[2]&0x80U)&&!c->rates.sgi20))return-1;
 width=c->width_mhz==80?2U:c->width_mhz==40?1U:0U;
 if(b[8]>width||(b[8]&&((b[2]&127U)<12U))||
    (b[8]==2U&&((b[2]&127U)<44U))||!rtw88_station_rate_selected(c,b[2]&127U))return-1;
 if(!b[8]&&(b[2]&127U)>=44U&&(((b[2]&127U)-44U)%10U)==9U)return-1;
 *rate=b[2]&127U;return 1;
}
