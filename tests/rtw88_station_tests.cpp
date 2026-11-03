// SPDX-License-Identifier: BSD-3-Clause
#include "rtw/RTW88Station.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>
struct Write {uint16_t address;unsigned width;uint32_t value;};
struct Fixture {
 std::array<uint8_t,65536> memory{};std::vector<Write> writes;
 unsigned calls=0,fail=0,radio=2;int fail_error=EIO;bool busy=false,cam_stuck=false;uint64_t delay=0;
 rtw88_fw_context fw{};rtw88_mac_context mac{};rtw88_station_context station{};
 static int read(void*p,uint16_t a,unsigned n,uint32_t*out){auto&f=*(Fixture*)p;if(++f.calls==f.fail)return f.fail_error;
  *out=0;for(unsigned i=0;i<n;i++)*out|=uint32_t(f.memory[a+i])<<(8*i);
  if(a==0x670&&!f.cam_stuck)*out&=~0x80000000U;if(a==0x1cc&&f.busy)*out=15;return 0;}
 static int write(void*p,uint16_t a,unsigned n,uint32_t v){auto&f=*(Fixture*)p;if(++f.calls==f.fail)return f.fail_error;
  f.writes.push_back({a,n,v});for(unsigned i=0;i<n;i++)f.memory[a+i]=uint8_t(v>>(8*i));return 0;}
 static void wait(void*p,uint32_t us){((Fixture*)p)->delay+=us;}
 static unsigned radio_state(void*p,uint8_t){return ((Fixture*)p)->radio;}
 explicit Fixture(rtw_wifi_family family){fw.family=family;fw.identified=fw.powered=fw.firmware_running=1;
  fw.bus.cookie=this;fw.bus.read=read;fw.bus.write=write;fw.bus.delay_us=wait;mac.device=&fw;mac.fifo.family=family;mac.mac_ready=1;}
 uint32_t value(uint16_t a,unsigned n=4)const{uint32_t v=0;for(unsigned i=0;i<n;i++)v|=uint32_t(memory[a+i])<<(8*i);return v;}
 rtw88_station_params params(uint8_t ch=6)const{rtw88_station_params p{};p.mac[0]=2;p.mac[5]=1;p.bssid[0]=2;p.bssid[5]=2;
  p.channel=ch;p.short_slot=1;p.aid=7;p.basic_rates=ch>14?0x150:0x15f;return p;}
 int init(){auto p=params();return rtw88_station_initialize(&station,&mac,radio_state,this,p.mac);}
};
static constexpr rtw_wifi_family families[]={RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU};
static void negotiated_rates(){
 unsigned checks=0;const uint8_t mcs[2]={255,255};
 for(auto family:families)for(unsigned streams=1;streams<=(family==RTW_WIFI_RTL8821CU?1U:2U);++streams)for(unsigned width:{20U,40U,80U})for(unsigned map=0;map<3;++map){
  rtw88_station_rate rate{};uint16_t peer=(uint16_t)(0xfff0U|map|(map<<2));
  assert(!rtw88_station_prepare_rate(family,36,streams,0xff0,mcs,peer,1,1,width,1,1,&rate));
  const unsigned top=map==0?7:map==1||width==20?8:9;
  const uint32_t chunk=(1U<<(top+1))-1,expected=0xff0U|(chunk<<12)|(streams==2?chunk<<22:0);
  assert(rate.rates.mask==expected&&rate.rates.initial_rate==44+(streams-1)*10+top&&rate.rates.rate_id==(streams==1?10:9));
  assert(rate.command[0]==0x40&&rate.command[1]==0&&rate.command[2]==(rate.rates.rate_id|0xa0U)&&rate.command[3]==(0x50U|(width==80?2:width==40?1:0)));
  for(unsigned r=0;r<84;++r){bool selected=r>=4&&r<12;if(r>=44&&r<44+10*streams)selected=(r-44)%10<=top;assert(rtw88_station_rate_selected(&rate,(uint8_t)r)==selected);}
  auto saved=rate;assert(rtw88_station_prepare_rate(family,52,streams,0xff0,mcs,peer,1,1,width,1,1,&rate)==EINVAL&&!memcmp(&saved,&rate,sizeof(rate)));
  assert(rtw88_station_prepare_rate(family,36,streams,0xff0,mcs,0xffff,1,1,width,1,1,&rate)==EINVAL&&!memcmp(&saved,&rate,sizeof(rate)));++checks;
 }
 for(const auto&o:{rtw88_station_operating{36,38,40,2},rtw88_station_operating{48,46,40,1},rtw88_station_operating{36,42,80,4},rtw88_station_operating{161,155,80,3}})assert(!rtw88_station_operating_validate(&o));
 for(const auto&o:{rtw88_station_operating{6,8,40,2},rtw88_station_operating{52,58,80,4},rtw88_station_operating{165,171,80,4},rtw88_station_operating{36,58,80,4},rtw88_station_operating{36,42,80,1}})assert(rtw88_station_operating_validate(&o));
 printf("B/C exact primary RA wire: %u family/NSS/width/VHT-map goldens; full descriptor-rate membership and invalid-policy checks passed\n",checks);
}
static void advisory_rssi(){for(auto family:families)for(unsigned box=0;box<4;++box){
 Fixture f(family);assert(!f.init());f.fw.h2c_box=uint8_t(box);f.busy=true;
 const uint8_t command[8]={0x42,0,0,57};const auto writes=f.writes.size();
 assert(rtw88_station_h2c_rssi_monitor(&f.station,command)==EBUSY);
 assert(!f.fw.last_error&&f.station.initialized&&f.mac.mac_ready&&f.fw.firmware_running&&f.fw.h2c_box==box&&f.writes.size()==writes&&f.delay==3000);
 f.busy=false;assert(!rtw88_station_h2c_rssi_monitor(&f.station,command)&&f.fw.h2c_box==(box+1)%4);
 for(int error:{EIO,ETIMEDOUT,EBUSY})for(unsigned position=1;position<=3;++position){
  Fixture bad(family);assert(!bad.init());bad.fail_error=error;bad.fail=bad.calls+position;
  assert(rtw88_station_h2c_rssi_monitor(&bad.station,command)==error&&bad.fw.last_error==error&&!bad.station.initialized&&!bad.fw.h2c_box);}
 Fixture invalid(family);assert(!invalid.init());const auto calls=invalid.calls;
 for(unsigned index=0;index<8;++index){uint8_t bad[8];std::memcpy(bad,command,8);bad[index]=index==0?1:index==3?101:1;
  assert(rtw88_station_h2c_rssi_monitor(&invalid.station,bad)==EINVAL&&invalid.calls==calls&&invalid.station.initialized);}
 }std::puts("B/C optional RSSI:3 families/all boxes preserve busy state and retry; EIO/ETIMEDOUT/EBUSY bus faults stay fatal");}
int main(){
 advisory_rssi();
 negotiated_rates();
 unsigned failure_positions=0;
 for(auto family:families){Fixture f(family);assert(f.init()==0);assert(f.station.initialized&&f.value(0x680,2)==0);
  assert(f.value(0x610,1)==2&&f.value(0x615,1)==1);assert(f.memory[0x522]==0x3f);
  for(uint8_t ch:{uint8_t(6),uint8_t(36)}){
   auto p=f.params(ch);p.passive_scan=1;f.radio=1;
   assert(rtw88_station_configure(&f.station,RTW88_STATION_SCAN,&p)==0&&f.memory[0x522]==0x3f);
   assert((f.value(0x440)&0xfffff)==p.basic_rates);
   auto calls=f.calls;assert(rtw88_station_configure(&f.station,RTW88_STATION_AUTH,&p)==ENXIO&&f.calls==calls);
   f.writes.clear();f.radio=2;assert(rtw88_station_configure(&f.station,RTW88_STATION_AUTH,&p)==0);
   size_t rate_write=f.writes.size(),unpause=f.writes.size();
   for(size_t i=0;i<f.writes.size();i++){
    if(f.writes[i].address==0x440)rate_write=i;
    if(f.writes[i].address==0x522&&f.writes[i].value==15)unpause=i;
   }
   assert(rate_write<unpause);
   assert(rtw88_station_configure(&f.station,RTW88_STATION_ASSOC,&p)==0);
   assert(rtw88_station_configure(&f.station,RTW88_STATION_RUN,&p)==0);
   assert((f.value(0x100)&0x30000)==0x20000&&f.value(0x6a8,2)==7&&f.value(0x440)==p.basic_rates);
   assert(f.memory[0x51b]==9&&f.memory[0x522]==0);
   assert(f.value(0x6a2,2)==0);p.ht=1;
   assert(rtw88_station_configure(&f.station,RTW88_STATION_RUN,&p)==0&&f.value(0x6a2,2)==0x100);
   p.ht=0;assert(rtw88_station_configure(&f.station,RTW88_STATION_RUN,&p)==0&&f.value(0x6a2,2)==0);

   rtw88_station_edca ac[4]={{3,4,10,0},{7,4,10,0},{2,3,4,94},{2,2,3,47}};
   assert(rtw88_station_set_edca(&f.station,1,ac)==0);
   assert((f.value(0x508)&255)==(ch>14?43:37));
   calls=f.calls;ac[3].ecwmin=15;assert(rtw88_station_set_edca(&f.station,1,ac)==EINVAL&&f.calls==calls);
   ac[3].ecwmin=2;ac[3].txop=65535;
   assert(rtw88_station_set_edca(&f.station,1,ac)==EINVAL&&f.calls==calls);
   ac[3].txop=2047;f.memory[0x503]=0xa8;
   assert(rtw88_station_set_edca(&f.station,1,ac)==0);
   assert(((f.value(0x500)>>16)&0x7ff)==2047);
   assert((f.value(0x500)&0xf8000000U)==0xa8000000U);
   calls=f.calls;
   p.basic_rates=1;if(ch>14)assert(rtw88_station_configure(&f.station,RTW88_STATION_RUN,&p)==EINVAL&&f.calls==calls);
  }
  const uint8_t cmd[8]={0x42,0,0,60,0,0,0,0};
  for(unsigned box=0;box<8;box++){f.writes.clear();assert(rtw88_station_h2c(&f.station,cmd)==0);
   assert(f.writes.size()==2&&f.writes[0].address==0x1f0+4*(box%4)&&f.writes[1].address==0x1d0+4*(box%4));}
  f.busy=true;assert(rtw88_station_h2c(&f.station,cmd)==ETIMEDOUT&&f.delay<=10000);
  assert(!f.station.initialized&&f.memory[0x522]==0x3f);
  for(unsigned streams=1;streams<=(family==RTW_WIFI_RTL8821CU?1U:2U);streams++)for(bool five:{false,true}){uint8_t mcs[2]={255,255};rtw88_station_rate r{};
   assert(rtw88_station_prepare_ht20(family,five?36:6,streams,five?0xff0:0xfff,mcs,1,1,1,&r)==0);
   assert(r.rates.initial_rate==(streams==1?19:27));
   assert(r.command[0]==0x40&&r.rates.ht&&r.rates.sgi20);
   if(five)assert(!(r.rates.mask&15));
   auto saved=r;assert(rtw88_station_prepare_ht20(family,36,streams,0xfff,mcs,1,1,1,&r)==EINVAL);
   assert(!memcmp(&r,&saved,sizeof(r)));}
  Fixture timeout(family);timeout.cam_stuck=true;
  assert(timeout.init()==ETIMEDOUT&&!timeout.station.initialized&&timeout.delay<10000);
  Fixture shared(family);assert(shared.init()==0);assert(rtw88_fw_h2c(&shared.fw,cmd)==0);
  shared.writes.clear();assert(rtw88_station_h2c(&shared.station,cmd)==0&&shared.writes[0].address==0x1f4);
  for(unsigned fault=1;fault<=3;fault++){
   Fixture mailbox(family);assert(mailbox.init()==0);mailbox.fail=mailbox.calls+fault;
   assert(rtw88_station_h2c(&mailbox.station,cmd)==EIO&&!mailbox.station.initialized&&mailbox.fw.h2c_box==0);
   assert(mailbox.memory[0x522]==0x3f);++failure_positions;
  }
  Fixture measure(family);assert(measure.init()==0);auto p=measure.params(36);assert(rtw88_station_configure(&measure.station,RTW88_STATION_RUN,&p)==0);
  unsigned total=measure.calls;
  for(unsigned failure=1;failure<=total;failure++){Fixture bad(family);bad.fail=failure;int e=bad.init();
   if(!e)e=rtw88_station_configure(&bad.station,RTW88_STATION_RUN,&p);
   assert(e==EIO&&!bad.station.initialized&&bad.memory[0x522]==0x3f);++failure_positions;}
 }
 Fixture unsupported(RTW_WIFI_RTL8814AU);assert(unsupported.init()==EINVAL&&unsupported.calls==0);
 std::printf("RTW88 station MAC/H2C/HT20:3 families,both bands,%u I/O failures passed\n",failure_positions);
}
