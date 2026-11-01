#include "rtw/RTL8812AURuntime.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdio>
struct Bus{
 std::array<uint8_t,65536>memory{};unsigned calls=0,fail=0;
 static rtw_usb_error in(void*p,const rtw_usb_setup*s,uint8_t*b,uint16_t*n){auto&x=*static_cast<Bus*>(p);if(++x.calls==x.fail)return RTW_USB_IO_ERROR;for(unsigned i=0;i<s->w_length;++i)b[i]=x.memory[s->w_value+i];*n=s->w_length;return RTW_USB_OK;}
 static rtw_usb_error out(void*p,const rtw_usb_setup*s,const uint8_t*b,uint16_t*n){auto&x=*static_cast<Bus*>(p);if(++x.calls==x.fail)return RTW_USB_IO_ERROR;for(unsigned i=0;i<s->w_length;++i)x.memory[s->w_value+i]=b[i];*n=s->w_length;return RTW_USB_OK;}
 static int wait(void*,uint32_t){return 1;}
 uint32_t read(unsigned a)const{return uint32_t(memory[a])|(uint32_t(memory[a+1])<<8)|(uint32_t(memory[a+2])<<16)|(uint32_t(memory[a+3])<<24);}
 rtw_r12au_boot context(){rtw_r12au_boot c{};c.chip=RTW_R12AU_CHIP_8812AU;c.bus={this,in,out};c.waiter={this,wait};c.identified=c.powered=c.firmware_running=c.mac_initialized=1;return c;}
};
static void rates(){
 for(unsigned mask=1;mask<4096;++mask){rtw_r12au_legacy_rate r{};assert(rtw_r12au_build_legacy_rate(uint16_t(mask),0,&r));assert(r.mask==mask&&r.initial_rate<12&&(mask&(1U<<r.initial_rate)));assert((mask>>(r.initial_rate+1))==0);uint8_t h2c[8];assert(rtw_r12au_make_rate_command(&r,1,h2c));assert(h2c[0]==0x40&&!h2c[1]&&h2c[2]==(r.rate_id|0x20)&&h2c[3]==0x40&&h2c[4]==uint8_t(mask)&&h2c[5]==uint8_t(mask>>8)&&!h2c[6]&&!h2c[7]);assert(bool(rtw_r12au_build_legacy_rate(uint16_t(mask),1,&r))==!(mask&15));}
 rtw_r12au_legacy_rate r{};assert(!rtw_r12au_build_legacy_rate(0,0,&r));assert(!rtw_r12au_build_legacy_rate(0x1000,0,&r));assert(rtw_r12au_build_legacy_rate(0xff0,0,&r)&&r.rate_id==7);uint8_t command[8];assert(rtw_r12au_make_media_command(1,command)&&command[0]==1&&command[1]==1&&!command[2]);assert(rtw_r12au_make_rssi_command(70,command)&&command[0]==0x42&&!command[1]&&command[3]==70);assert(!rtw_r12au_make_rssi_command(101,command));
 uint8_t event[6]={0xc,1,11,0,0,0},rate=0;assert(rtw_r12au_parse_rate_report(event,6,&r,&rate)==1&&rate==11);event[2]=3;assert(rtw_r12au_parse_rate_report(event,6,&r,&rate)==-1);event[2]=0x8b;assert(rtw_r12au_parse_rate_report(event,6,&r,&rate)==-1);event[0]=3;assert(!rtw_r12au_parse_rate_report(event,6,&r,&rate));
 rtw_r12au_tx_options options{};options.payload_bytes=100;options.data=1;options.rate=11;options.rf_chains=2;options.bandwidth_mhz=20;options.ack_expected=1;options.retry_limit=7;options.firmware_rate=1;uint8_t descriptor[40];assert(rtw_r12au_make_legacy_tx(&options,&r,descriptor));assert((descriptor[6]&31)==7);assert(rtw_r12au_tx_descriptor_valid(descriptor,2));options.rate=12;assert(!rtw_r12au_make_legacy_tx(&options,&r,descriptor));options.rate=11;options.bandwidth_mhz=40;assert(!rtw_r12au_make_legacy_tx(&options,&r,descriptor));
}
static void edca(){
 for(unsigned failure=1;failure<=9;++failure){Bus bad;auto ctx=bad.context();rtw_r12au_edca_ac values[4]={{2,3,4,0},{2,4,10,0},{2,3,4,94},{2,2,3,47}};bad.fail=failure;assert(rtw_r12au_station_edca(&ctx,1,0,values)==EIO&&bad.calls==failure&&ctx.io_error==EIO);}
 Bus b;auto c=b.context();rtw_r12au_edca_ac a[4]={{2,3,4,0},{2,4,10,0},{2,3,4,94},{2,2,3,47}};
 assert(!rtw_r12au_station_rates(&c,0x15,1,0));assert(b.memory[0x51b]==9&&(b.read(0x440)&0xfffff)==0x15);assert(!rtw_r12au_station_edca(&c,1,0,a));assert((b.read(0x508)&255)==28&&(b.read(0x500)>>16)==47);assert(!rtw_r12au_station_edca(&c,0,1,a));assert((b.read(0x508)&255)==34&&b.memory[0x51b]==9);assert(!rtw_r12au_station_edca(&c,0,0,a));assert((b.read(0x508)&255)==50&&b.memory[0x51b]==20);
 for(unsigned reg:{0x500U,0x504U,0x508U,0x50cU})b.memory[reg+3]=0xf8;
 a[3].txop=2047;assert(!rtw_r12au_station_edca(&c,0,0,a));for(unsigned reg:{0x500U,0x504U,0x508U,0x50cU})assert((b.read(reg)&0xf8000000)==0xf8000000);assert(((b.read(0x500)>>16)&0x7ff)==2047);
 auto calls=b.calls;a[3].txop=65535;assert(rtw_r12au_station_edca(&c,0,0,a)==EINVAL&&b.calls==calls);a[3].txop=0;assert(rtw_r12au_station_rates(&c,1,0,1)==EINVAL);a[3].ecwmin=15;a[3].ecwmax=1;assert(rtw_r12au_station_edca(&c,0,0,a)==EINVAL);assert(b.calls==calls);
}
static void phy(){
 uint8_t p[32]{};rtw_r12au_phy_sample s{};p[0]=50;p[1]=70;assert(rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8812AU,2,0,4,p,32,&s)&&s.signal_dbm==-40&&s.rssi_percent==60);assert(rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8821AU,1,0,4,p,32,&s)&&s.signal_dbm==-60&&s.rssi_percent==40);p[4]=0;p[5]=uint8_t((7<<5)|27);assert(rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8812AU,2,0,0,p,32,&s)&&s.signal_dbm==-94);p[5]=uint8_t((5<<5)|10);assert(rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8821AU,1,0,0,p,32,&s)&&s.signal_dbm==-58&&s.rssi_percent==42);assert(rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8812AU,2,0,27,p,32,&s)&&s.signal_dbm==-40);assert(rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8812AU,2,0,63,p,32,&s));p[5]=3<<5;assert(!rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8821AU,1,0,0,p,32,&s));assert(!rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8821AU,2,0,4,p,32,&s));assert(!rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8812AU,2,0,28,p,32,&s));assert(!rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8812AU,2,0,4,p,31,&s));
}
static void saturated_cck(){
 // The primary family formula returns +15dB for8821 LNA0/VGA0 and
 // +20dB for8812 LNA0/VGA0. Those measured reports saturate at100%.
 // Shared signal caches reserve zero for missing data, so the portable
 // sample clips saturated power to their highest valid negative dBm.
 for(auto chip:{RTW_R12AU_CHIP_8812AU,RTW_R12AU_CHIP_8821AU})for(unsigned vga=0;vga<8;++vga){uint8_t p[32]{};p[5]=uint8_t(vga);rtw_r12au_phy_sample s{};assert(rtw_r12au_decode_phy_sample(chip,1,0,0,p,32,&s));assert(s.signal_dbm==-1&&s.rssi_percent==100);}
 uint8_t p[32]{};rtw_r12au_phy_sample s{};assert(!rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8821AU,1,0,0,p,0,&s)&&s.signal_dbm==0&&s.rssi_percent==0);p[5]=3<<5;assert(!rtw_r12au_decode_phy_sample(RTW_R12AU_CHIP_8821AU,1,0,0,p,32,&s));
}
static void modes(){
 for(bool five:{false,true}){rtw_r12au_legacy_rate r{};assert(rtw_r12au_build_legacy_rate(five?0xff0:0xfff,five,&r));rtw_r12au_tx_options o{};o.payload_bytes=128;o.data=o.qos=o.firmware_rate=o.ack_expected=1;o.tid=5;o.rate=r.initial_rate;o.rf_chains=2;o.band_5ghz=five;o.bandwidth_mhz=20;o.retry_limit=7;uint8_t d[40];assert(rtw_r12au_make_tx(&o,&r,d));assert((d[5]&31)==5&&!(d[9]&0x10)&&!(d[14]&0x3e));o.ampdu=1;o.max_agg=31;assert(!rtw_r12au_make_tx(&o,&r,d));o.ampdu=o.max_agg=0;o.firmware_rate=0;o.eapol=1;o.rate=five?4:0;assert(rtw_r12au_make_tx(&o,nullptr,d)&&(d[13]&1)&&!(d[9]&0x10));}
 const uint8_t mcs[2]={255,255};
 for(unsigned streams:{1U,2U})for(unsigned width:{20U,40U,80U})for(bool sgi:{false,true}){
  rtw_r12au_rate_config c{};uint8_t command[8],descriptor[40];
  assert(rtw_r12au_build_rate_config(0xff0,mcs,0xfffa,streams,RTW_R12AU_MODE_VHT,width,1,1,sgi,&c));
  assert(c.ht&&c.vht&&c.short_gi==sgi&&c.bandwidth_mhz==width&&c.streams==streams);
  assert(c.initial_rate==(streams==1?(width==20?52:53):(width==20?62:63))&&c.rate_id==(streams==1?10:9));
  assert(rtw_r12au_make_rate_command(&c,1,command)&&command[0]==0x40&&command[2]==(c.rate_id|0x20|(sgi?0x80:0))&&command[3]==(0x50|(width==20?0:width==40?1:2)));
  assert(rtw_r12au_rate_selected(&c,c.initial_rate));assert(!rtw_r12au_rate_selected(&c,28));if(width==20)assert(!rtw_r12au_rate_selected(&c,53));
  rtw_r12au_tx_options o{};o.payload_bytes=128;o.sequence=7;o.data=o.qos=o.firmware_rate=o.ack_expected=1;o.rate=c.initial_rate;o.rf_chains=2;o.tid=5;o.band_5ghz=1;o.bandwidth_mhz=(uint8_t)width;o.short_gi=sgi;o.retry_limit=7;
  assert(rtw_r12au_make_tx(&o,&c,descriptor)&&rtw_r12au_tx_descriptor_valid(descriptor,2));assert((descriptor[6]&31)==c.rate_id);
  o.ampdu=1;o.density=7;o.max_agg=31;assert(rtw_r12au_make_tx(&o,&c,descriptor));assert((descriptor[9]&0x10)&&!(descriptor[10]&1)&&(descriptor[10]&0x70)==0x70&&(descriptor[14]&0x3e)==0x3e);
  o.eapol=1;assert(!rtw_r12au_make_tx(&o,&c,descriptor));o.eapol=0;o.ampdu=o.density=o.max_agg=0;
  o.bandwidth_mhz=20;if(width!=20)assert(!rtw_r12au_make_tx(&o,&c,descriptor));
 }
 for(bool five:{false,true})for(unsigned width:{20U,40U}){
  rtw_r12au_rate_config c{};assert(rtw_r12au_build_rate_config(five?0xff0:0xfff,mcs,0xffff,2,RTW_R12AU_MODE_HT,width,five,1,1,&c));
  assert(c.ht&&!c.vht&&c.initial_rate==27&&c.rate_id==(five?4:width==40?0:2));
 }
 {rtw_r12au_tx_options o{};o.payload_bytes=100;o.data=1;o.rate=53;o.rf_chains=1;o.band_5ghz=1;o.bandwidth_mhz=20;uint8_t descriptor[40];assert(!rtw_r12au_tx_descriptor(&o,descriptor));}
 rtw_r12au_rate_config c{};assert(!rtw_r12au_build_rate_config(0xfff,mcs,0xfffa,2,RTW_R12AU_MODE_VHT,80,0,1,1,&c));assert(!rtw_r12au_build_rate_config(0xff0,mcs,0xffff,2,RTW_R12AU_MODE_VHT,80,1,1,1,&c));assert(!rtw_r12au_build_rate_config(0xff0,mcs,0xfffa,2,RTW_R12AU_MODE_HT,80,1,1,1,&c));
}
int main(){saturated_cck();modes();rates();edca();phy();std::puts("AU runtime:4095 legacy masks,HT/VHT20/40/80 NSS/SGI RAID,H2C,AMPDU,masked EDCA,distinct and saturated PHY RSSI passed");}
