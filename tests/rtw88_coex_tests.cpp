/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88Coex.h"
#include <array>
#include <vector>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
struct Bus {
 std::array<uint8_t,65536> reg{};std::array<uint32_t,65536> indirect{};
 std::array<std::array<uint32_t,256>,2> rf{};
 unsigned calls=0,fail=0;bool after=false;std::vector<std::array<uint8_t,8>> commands;
 unsigned mimoCalls=0;bool oneSS=false;unsigned mimoFail=0;
 static int read(void*p,uint16_t a,unsigned width,uint32_t*v){auto&b=*(Bus*)p;assert(width==1||width==2||width==4);assert(a<=65536U-width);*v=0;if(++b.calls==b.fail)return EIO;for(unsigned i=0;i<width;++i)*v|=(uint32_t)b.reg[a+i]<<(8*i);return 0;}
 static int write(void*p,uint16_t a,unsigned width,uint32_t v){auto&b=*(Bus*)p;assert(a<=65536U-width);bool bad=++b.calls==b.fail;if(bad&&!b.after)return EIO;for(unsigned i=0;i<width;++i)b.reg[a+i]=(uint8_t)(v>>(i*8));if(a>=0x1d0&&a<=0x1dc&&width==4){std::array<uint8_t,8>cmd{};memcpy(cmd.data(),b.reg.data()+a,4);memcpy(cmd.data()+4,b.reg.data()+0x1f0+a-0x1d0,4);b.commands.push_back(cmd);}return bad?EIO:0;}
 static int indirectRead(void*p,uint16_t a,uint32_t*v){auto&b=*(Bus*)p;*v=0;if(++b.calls==b.fail)return EIO;*v=b.indirect[a];return 0;}
 static int indirectWrite(void*p,uint16_t a,uint32_t v){auto&b=*(Bus*)p;bool bad=++b.calls==b.fail;if(!bad||b.after)b.indirect[a]=v;return bad?EIO:0;}
 static void delay(void*,uint32_t){}
 static int mimo(void*p,int enabled){auto&b=*(Bus*)p;assert(enabled==0||enabled==1);++b.mimoCalls;if(b.mimoCalls==b.mimoFail)return EIO;b.oneSS=enabled;return 0;}
 uint32_t read32(unsigned a)const{uint32_t v=0;for(unsigned i=0;i<4;++i)v|=(uint32_t)reg[a+i]<<(8*i);return v;}
};
static unsigned shift(uint32_t mask){assert(mask);return(unsigned)__builtin_ctz(mask);}
extern "C" int rtw88_phy_read_rf(rtw88_phy_context*c,uint8_t path,uint8_t a,uint32_t mask,uint32_t*v){auto&b=*(Bus*)c->boot->firmware.bus.cookie;assert(path<c->boot->board.rf_paths&&mask&&(mask&~0xfffffU)==0);*v=0;if(++b.calls==b.fail)return EIO;*v=(b.rf[path][a]&mask)>>shift(mask);return 0;}
extern "C" int rtw88_phy_write_rf(rtw88_phy_context*c,uint8_t path,uint8_t a,uint32_t mask,uint32_t v){auto&b=*(Bus*)c->boot->firmware.bus.cookie;assert(path<c->boot->board.rf_paths&&mask&&(mask&~0xfffffU)==0);bool bad=++b.calls==b.fail;if(!bad||b.after)b.rf[path][a]=(b.rf[path][a]&~mask)|((v<<shift(mask))&mask);return bad?EIO:0;}
struct Fixture {
 Bus bus;rtw88_boot_context boot{};rtw88_mac_context mac{};rtw88_phy_context phy{};
 std::vector<std::max_align_t>storage;rtw88_coex*c=nullptr;rtw88_coex_input input{};
 Fixture(rtw_wifi_family family=RTW_WIFI_RTL8821CU,bool shared=false):storage((RTW88_COEX_SCRATCH_BYTES+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)){
  boot.firmware.family=family;boot.firmware.identified=boot.firmware.powered=boot.firmware.firmware_running=1;boot.board_valid=boot.physical_valid=1;boot.board.features_valid=1;
  boot.board.rf_paths=boot.board.active_rf_paths=boot.board.tx_streams=family==RTW_WIFI_RTL8821CU?1:2;boot.board.rfe_option=family==RTW_WIFI_RTL8822BU?3:0;boot.board.btcoex=1;boot.board.rf_board_option=0x20;boot.board.share_ant=boot.board.bt_setting=shared;
  boot.firmware.bus={&bus,Bus::read,Bus::write,nullptr,Bus::delay,Bus::indirectRead,Bus::indirectWrite};mac.device=&boot.firmware;mac.mac_ready=1;mac.fifo.family=family;phy.boot=&boot;phy.mac=&mac;phy.tables_loaded=1;
  bus.reg[0x80]=0x78;bus.reg[0x81]=0xc0;bus.reg[0xaa]=2;bus.reg[0x770]=1;
  input.now_ms=1000;input.primary=input.center=6;input.width_mhz=20;input.beacon_interval=100;input.rssi_percent=60;input.rx_rate=input.tx_rate=4;
  assert(rtw88_coex_size()<=RTW88_COEX_SCRATCH_BYTES);assert(!rtw88_coex_init(storage.data(),storage.size()*sizeof(std::max_align_t),&phy,Bus::mimo,&bus,&c)&&c&&!bus.calls);
 }
 rtw88_coex_status status()const{rtw88_coex_status s{};rtw88_coex_get_status(c,&s);return s;}
 void start(){assert(!rtw88_coex_start(c,&input)&&status().active);}
 void connect(){input.connected=1;assert(!rtw88_coex_notify(c,RTW88_CX_ASSOC_START,&input));assert(!rtw88_coex_notify(c,RTW88_CX_ASSOC_FINISH,&input));assert(!rtw88_coex_notify(c,RTW88_CX_MEDIA_CONNECT,&input));}
};
static void traces(){
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  Fixture f(family,shared);uint8_t media[8]={1,0};assert(!rtw88_fw_h2c(&f.boot.firmware,media));unsigned before=f.bus.commands.size(),cursor=f.boot.firmware.h2c_box;f.start();assert(f.boot.firmware.h2c_box==(cursor+f.bus.commands.size()-before)%4);
  assert(f.bus.reg[0x550]&8);assert(f.bus.read32(0x40)&(1U<<5));assert(f.bus.reg[0x4c6]&(1U<<4));assert(f.bus.reg[0x763]&0x10);
  assert(!rtw88_coex_notify(f.c,RTW88_CX_SCAN_START,&f.input));assert(!rtw88_coex_notify(f.c,RTW88_CX_SCAN_FINISH,&f.input));f.connect();
  const uint8_t a2dp[9]={9,0,1,0x49,0,20,0,0,40};assert(!rtw88_coex_c2h(f.c,a2dp,sizeof(a2dp),&f.input));assert(f.status().bt_profiles==1&&f.status().bt_info_events==1&&f.status().policy_runs);
  bool tdma=false;for(auto cmd:f.bus.commands)tdma|=cmd[0]==0x60;assert(tdma);
  f.input.tx_packets=101;f.input.tx_bytes=2U<<18;f.input.rx_bytes=4U<<18;assert(!rtw88_coex_watchdog(f.c,&f.input));assert(f.status().tx_mbps==2&&f.status().rx_mbps==4);
  f.input.primary=f.input.center=36;assert(!rtw88_coex_notify(f.c,RTW88_CX_SWITCH_BAND,&f.input));assert(f.status().under_5g);f.input.primary=f.input.center=165;assert(!rtw88_coex_notify(f.c,RTW88_CX_SWITCH_BAND,&f.input));assert(f.status().under_5g);
  f.input.primary=f.input.center=6;assert(!rtw88_coex_notify(f.c,RTW88_CX_SWITCH_BAND,&f.input));assert(!f.status().under_5g);
  assert(!rtw88_coex_rfk(f.c,1)&&f.status().rfk_active);auto calls=f.bus.calls;assert(rtw88_coex_poll(f.c,f.input.now_ms+1)==EBUSY&&f.bus.calls==calls);assert(!rtw88_coex_rfk(f.c,0));
  f.input.now_ms+=6000;assert(!rtw88_coex_watchdog(f.c,&f.input));assert(!rtw88_coex_stop(f.c)&&!f.status().active&&!f.status().pending_work);calls=f.bus.calls;rtw88_coex_forget(f.c);assert(calls==f.bus.calls);assert(rtw88_coex_watchdog(f.c,&f.input)==ENXIO&&calls==f.bus.calls);++cases;
 }
 std::printf("B/C coexistence: %u family/antenna PTA/TDMA/BT-info/AFH/traffic/RFK/stop traces passed\n",cases);
}
static void boundaries(){
 {Fixture live,other;live.start();auto before=live.bus.calls;live.boot.firmware.bus.cookie=&other.bus;assert(rtw88_coex_stop(live.c)==ENXIO&&live.bus.calls==before&&!other.bus.calls&&!live.status().active&&!live.status().pending_work);}
 {Fixture alias;std::vector<uint8_t>before(rtw88_coex_size());memcpy(before.data(),alias.storage.data(),before.size());assert(rtw88_coex_init(alias.storage.data(),RTW88_COEX_SCRATCH_BYTES,&alias.phy,Bus::mimo,&alias.bus,reinterpret_cast<rtw88_coex**>(alias.storage.data()))==EINVAL&&!memcmp(before.data(),alias.storage.data(),before.size())&&!alias.bus.calls);}
 {Fixture live,other;live.start();auto before=live.bus.calls;live.phy.boot=&other.boot;assert(rtw88_coex_watchdog(live.c,&live.input)==ENXIO&&live.bus.calls==before&&!other.bus.calls&&!live.status().active);}
 Fixture f;auto calls=f.bus.calls;auto invalid=f.input;invalid.rssi_percent=101;assert(rtw88_coex_start(f.c,&invalid)==EINVAL&&calls==f.bus.calls);
 f.boot.board.btcoex=0;rtw88_coex*bad=f.c;assert(rtw88_coex_init(f.storage.data(),RTW88_COEX_SCRATCH_BYTES,&f.phy,Bus::mimo,&f.bus,&bad)==ENOTSUP&&!bad&&!f.bus.calls);
 Fixture running;running.start();calls=running.bus.calls;uint8_t malformed[32]={9,0,1};for(unsigned n=1;n<9;++n)assert(rtw88_coex_c2h(running.c,malformed,n,&running.input)==EINVAL);assert(calls==running.bus.calls);
 malformed[0]=0x0c;assert(rtw88_coex_c2h(running.c,malformed,6,&running.input)==ENOMSG&&calls==running.bus.calls);
 for(uint16_t ready:{0xc6U,0xc079U,0xffffU,0U,0x4078U}){Fixture badFirmware;badFirmware.start();badFirmware.bus.reg[0x80]=(uint8_t)ready;badFirmware.bus.reg[0x81]=(uint8_t)(ready>>8);assert(rtw88_coex_watchdog(badFirmware.c,&badFirmware.input)==ENXIO&&!badFirmware.status().active);}
}
static void failures(){
 unsigned faults=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  Fixture measure(family,shared);measure.start();unsigned count=measure.bus.calls;
  for(bool after:{false,true})for(unsigned step=1;step<=count;++step){Fixture f(family,shared);f.bus.fail=step;f.bus.after=after;assert(rtw88_coex_start(f.c,&f.input)==EIO&&!f.status().active&&f.status().error==EIO);auto calls=f.bus.calls;rtw88_coex_forget(f.c);assert(f.bus.calls==calls);++faults;}
 }
 std::printf("B/C coexistence: %u startup I/O faults before/after effect passed\n",faults);
}
static void wireReports(){
 unsigned checks=0;
 for(unsigned token=0;token<256;token+=4)for(unsigned status:{0U,64U,128U,192U}){
  uint8_t a[9]={3,0xad,(uint8_t)status,0,0,0,0,0,(uint8_t)(token|3)};
  uint8_t b[12]={255,0xad,15,0,0,0,0,0,0,0,(uint8_t)(token|3),(uint8_t)status};uint8_t got=0;int ack=0;
  assert(!rtw88_coex_parse_tx_status(a,sizeof(a),&got,&ack)&&got==token&&ack==(status==0));
  assert(!rtw88_coex_parse_tx_status(b,sizeof(b),&got,&ack)&&got==token&&ack==(status==0));checks+=2;
  for(unsigned n=0;n<9;++n){assert(rtw88_coex_parse_tx_status(a,n,&got,&ack)==EINVAL);++checks;}
  for(unsigned n=0;n<12;++n){assert(rtw88_coex_parse_tx_status(b,n,&got,&ack)==EINVAL);++checks;}
 }
 std::printf("B/C CCX wire: %u V0/V1 token/ACK/status/length cases passed\n",checks);
}
static void mpResponses(){
 unsigned checks=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool intervening:{false,true})for(bool late:{false,true}){
  Fixture f(family);f.start();f.connect();assert(!rtw88_coex_watchdog(f.c,&f.input));
  uint8_t report[9]={9,0,1,0x49,0,20,0x20,0,40};
  for(unsigned i=0;i<3;++i){report[8]=(uint8_t)(40+i);++f.input.now_ms;assert(!rtw88_coex_c2h(f.c,report,9,&f.input));}
  bool query=false;for(auto c:f.bus.commands)query|=c[0]==0x67&&c[2]==0x2d;assert(query);
  if(intervening){report[8]=43;++f.input.now_ms;assert(!rtw88_coex_c2h(f.c,report,9,&f.input));}
  uint8_t response[10]={11,0,1,0,0,1};auto before=f.bus.calls;
  if(late){f.input.now_ms+=10;assert(rtw88_coex_c2h(f.c,response,10,&f.input)==EINVAL&&f.bus.calls==before);assert(!rtw88_coex_poll(f.c,f.input.now_ms)&&f.status().mp_timeouts==1);}
  else{++f.input.now_ms;assert(!rtw88_coex_c2h(f.c,response,10,&f.input));assert(f.status().bt_ble_scan_type==1&&f.status().bt_init_scan&&f.status().bt_info_events==(intervening?4U:3U));}
  ++checks;
 }
 std::printf("B/C MP: %u three/four-event BLE,ACK/late-ACK/deadline cases passed\n",checks);
}

static void hidRegression(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  Fixture f(family);f.start();f.connect();const uint8_t hid[9]={9,0,1,0x29,0,20,0,0,40};assert(!rtw88_coex_c2h(f.c,hid,9,&f.input));
  auto before=f.bus.commands.size();f.input.now_ms+=2000;assert(!rtw88_coex_watchdog(f.c,&f.input));bool query=false;for(size_t i=before;i<f.bus.commands.size();++i)query|=f.bus.commands[i][0]==0x73&&f.bus.commands[i][1]==0;
  assert(query==(family==RTW_WIFI_RTL8822CU));
 }
 puts("B/C primary watchdog HID discovery: only8822C emits H2C73");
}

/* Real BT-info -> freerun -> LNA policy trace. The third BLE-enabled report
 * requests scan type and an LNA constraint after seeing two HID peers. */
static void overlappingPolicyRequests(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  Fixture f(family);f.start();f.connect();f.input.tx_packets=101;f.input.tx_bytes=2U<<18;
  assert(!rtw88_coex_watchdog(f.c,&f.input));
  uint8_t report[9]={9,0,1,0x49,0,20,0x20,0,40};
  for(unsigned event=0;event<3;++event){report[8]=uint8_t(40+event);if(event==2)report[7]=0x80;
   ++f.input.now_ms;assert(!rtw88_coex_c2h(f.c,report,sizeof(report),&f.input));}
  unsigned scans=0,lna=0;for(const auto&cmd:f.bus.commands)if(cmd[0]==0x67){scans+=cmd[2]==0x2d;lna+=cmd[2]==0x32;}
  assert(scans==1&&!lna);
  uint8_t response[10]={11,0,1,0,0,1};++f.input.now_ms;
  assert(!rtw88_coex_c2h(f.c,response,sizeof(response),&f.input));
  uint8_t level=255;for(const auto&cmd:f.bus.commands)if(cmd[0]==0x67&&cmd[2]==0x32){++lna;level=cmd[3];}
  assert(lna==1&&level==(family==RTW_WIFI_RTL8822BU?1:4));assert(!rtw88_coex_stop(f.c));
 }
 puts("B/C real BLE/HID freerun policy: third-report MP collision sends exact LNA H2C on ACK for all3 chips");
}

static void busyFirmwareDoesNotBlockFullStop(){
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true})for(unsigned box=0;box<4;++box){
  Fixture f(family,shared);f.start();f.connect();uint8_t a2dp[9]={9,0,1,0x49,0,20,0,0,40};
  assert(!rtw88_coex_c2h(f.c,a2dp,sizeof(a2dp),&f.input));assert(f.status().bt_profiles==1);
  f.boot.firmware.h2c_box=box;f.bus.reg[0x1cc]=0xf;const auto sent=f.bus.commands.size();
  assert(rtw88_coex_stop(f.c)==0);
  assert(!f.status().error&&!f.boot.firmware.last_error&&!f.status().active&&!f.status().pending_work);
  assert(f.bus.commands.size()==sent&&f.boot.firmware.h2c_box==box);
  assert((f.bus.read32(0xaa)&0xffffU)==0x8000U&&f.status().antenna_phase==2);
  f.bus.reg[0x1cc]=0;f.start();assert(f.status().active&&!f.status().error);assert(!rtw88_coex_stop(f.c));
 }
 puts("B/C full stop: 3 families, active BT/A2DP, both antennas, all4 busy firmware boxes retire without H2C and restart");
}
static void fullStopIoFailuresRemainSticky(){
 unsigned faults=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool shared:{false,true}){
  Fixture measured(family,shared);measured.start();measured.connect();auto begin=measured.bus.calls;
  assert(!rtw88_coex_stop(measured.c));const unsigned operations=measured.bus.calls-begin;assert(operations);
  for(bool after:{false,true})for(unsigned step=1;step<=operations;++step){
   Fixture f(family,shared);f.start();f.connect();f.bus.after=after;f.bus.fail=f.bus.calls+step;
   assert(rtw88_coex_stop(f.c)==EIO);
   assert(f.status().error==EIO&&f.boot.firmware.last_error==EIO&&!f.status().active&&!f.status().pending_work);
   f.bus.fail=0;assert(rtw88_coex_stop(f.c)==EIO&&f.boot.firmware.last_error==EIO);++faults;
  }
 }
 std::printf("B/C full stop: %u antenna/scoreboard/direct+indirect I/O faults before/after effect stay sticky\n",faults);
}
int main(){busyFirmwareDoesNotBlockFullStop();fullStopIoFailuresRemainSticky();hidRegression();traces();boundaries();failures();wireReports();mpResponses();overlappingPolicyRequests();}
