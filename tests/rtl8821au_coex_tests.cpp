/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTL8821AUCoex.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
struct Bus{
 std::array<uint8_t,65536> reg{};unsigned calls=0,fail=0;bool ambiguous=false;
 std::vector<std::array<uint8_t,8>>commands;
 static rtw_usb_error in(void*arg,const rtw_usb_setup*s,uint8_t*b,uint16_t*n){auto&m=*(Bus*)arg;if(++m.calls==m.fail)return RTW_USB_IO_ERROR;for(unsigned j=0;j<s->w_length;++j)b[j]=m.reg[s->w_value+j];*n=s->w_length;return RTW_USB_OK;}
 static rtw_usb_error out(void*arg,const rtw_usb_setup*s,const uint8_t*b,uint16_t*n){auto&m=*(Bus*)arg;bool bad=++m.calls==m.fail;if(bad&&!m.ambiguous)return RTW_USB_IO_ERROR;for(unsigned j=0;j<s->w_length;++j)m.reg[s->w_value+j]=b[j];if(s->w_value>=0x1d0&&s->w_value<=0x1dc&&s->w_length==4){std::array<uint8_t,8>cmd{};auto box=s->w_value-0x1d0;for(unsigned j=0;j<4;++j){cmd[j]=b[j];cmd[j+4]=m.reg[0x1f0+box+j];}m.commands.push_back(cmd);}*n=s->w_length;return bad?RTW_USB_IO_ERROR:RTW_USB_OK;}
 static int wait(void*,uint32_t){return 1;}
 uint32_t read(unsigned a){uint32_t v=0;for(unsigned j=0;j<4;++j)v|=uint32_t(reg[a+j])<<(j*8);return v;}
};
struct CoexFixture{
 Bus bus;rtw_r12au_boot boot{};std::vector<std::max_align_t>storage;rtw_r21au_coex*c=nullptr;rtw_r21au_coex_input input{};
 CoexFixture(bool shared=false):storage((rtw_r21au_coex_size()+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)){
  boot.bus={&bus,Bus::in,Bus::out};boot.waiter={&bus,Bus::wait};boot.chip=boot.board.phy.chip=RTW_R12AU_CHIP_8821AU;boot.board.phy.rf_paths=boot.board.phy.btcoex=1;boot.board.share_ant=boot.board.bt_setting=shared;boot.identified=boot.powered=boot.firmware_running=boot.mac_initialized=1;bus.reg[0x80]=0xc6;bus.reg[0x770]=1;
  input.now_ms=1000;input.primary=input.center=6;input.width_mhz=20;input.beacon_interval=100;input.rssi_percent=60;input.rx_rate=input.tx_rate=4;
  assert(!rtw_r21au_coex_init(storage.data(),storage.size()*sizeof(std::max_align_t),&boot,&c)&&c&&!bus.calls);
 }
 rtw_r21au_coex_status status(){rtw_r21au_coex_status s{};rtw_r21au_coex_get_status(c,&s);return s;}
 void start(){assert(!rtw_r21au_coex_start(c,&input));assert(status().active);}
 void connect(){input.connected=1;input.tx_packets=101;assert(!rtw_r21au_coex_notify(c,RTW_R21CX_ASSOC_START,&input));assert(status().frozen);assert(!rtw_r21au_coex_notify(c,RTW_R21CX_ASSOC_FINISH,&input));assert(!status().frozen);assert(!rtw_r21au_coex_notify(c,RTW_R21CX_MEDIA_CONNECT,&input));}
};
static void traces(){
 for(bool shared:{false,true}){CoexFixture f(shared);uint8_t media[8]={1,0};assert(!rtw_r12au_h2c(&f.boot,media));unsigned cursor=f.boot.h2c_box;f.start();assert(f.boot.h2c_box==(cursor+f.bus.commands.size()-1)%4);assert(f.bus.reg[0xcb4]==(shared?0x66:0x77));assert((f.bus.read(0x4c)&0x01800000)==0x01000000);assert(f.bus.reg[0x765]&0x18);assert((f.bus.reg[0x76e]&0x0c)==(shared?0x0c:0x04));assert(f.bus.reg[0x778]==3);
  assert(!rtw_r21au_coex_notify(f.c,RTW_R21CX_SCAN_START,&f.input));assert(!rtw_r21au_coex_notify(f.c,RTW_R21CX_SCAN_FINISH,&f.input));f.connect();assert(!rtw_r21au_coex_watchdog(f.c,&f.input));
  const uint8_t a2dp[9]={9,0,1,0x49,0,20,0,0,40};assert(!rtw_r21au_coex_c2h(f.c,a2dp,sizeof(a2dp),f.input.now_ms));assert(f.status().bt_profiles==1&&f.status().bt_info_events==1);if(f.status().table!=(shared?12:112)||f.status().tdma!=(shared?13:112)){auto s=f.status();fprintf(stderr,"A2DP shared%d table%u tdma%u rssi%u freeze%u runs%u\n",shared,s.table,s.tdma,s.bt_rssi,s.frozen,s.policy_runs);abort();}
  bool tdma=false;for(auto cmd:f.bus.commands)if(cmd[0]==0x60){tdma=true;if(!shared&&cmd[1]==0x51)assert(cmd[2]==8&&cmd[3]==3&&cmd[4]==0x10&&cmd[5]==0x14);}assert(tdma);
  f.input.primary=36;f.input.center=42;f.input.width_mhz=80;assert(!rtw_r21au_coex_notify(f.c,RTW_R21CX_SWITCH_BAND,&f.input));assert(!rtw_r21au_coex_notify(f.c,RTW_R21CX_MEDIA_CONNECT,&f.input));assert(f.status().under_5g&&f.status().table==(shared?0:100)&&f.status().tdma==(shared?0:100));assert(f.bus.reg[0xcb4]==0x77&&((f.bus.read(0xcb4)>>28)&3)==(shared?2:1));
  f.input.now_ms+=6000;assert(!rtw_r21au_coex_watchdog(f.c,&f.input));assert(f.bus.commands.back()[0]==0x61);assert(!rtw_r21au_coex_stop(f.c)&&!f.status().active&&!f.status().pending_work);auto calls=f.bus.calls;rtw_r21au_coex_forget(f.c);assert(calls==f.bus.calls);assert(rtw_r21au_coex_watchdog(f.c,&f.input)==ENXIO&&calls==f.bus.calls);
 }
}
static void boundaries(){
 CoexFixture ff(true);ff.boot.board.bt_setting=255;assert(!rtw_r21au_coex_init(ff.storage.data(),ff.storage.size()*sizeof(std::max_align_t),&ff.boot,&ff.c));ff.start();
 CoexFixture f;auto calls=f.bus.calls;auto bad=f.input;bad.rssi_percent=101;assert(rtw_r21au_coex_start(f.c,&bad)==EINVAL&&f.bus.calls==calls);f.boot.chip=RTW_R12AU_CHIP_8812AU;rtw_r21au_coex*other=f.c;assert(rtw_r21au_coex_init(f.storage.data(),f.storage.size()*sizeof(std::max_align_t),&f.boot,&other)==ENOTSUP&&!other&&f.bus.calls==calls);
 CoexFixture running;running.start();calls=running.bus.calls;uint8_t malformed[32]={9,0,1};for(unsigned n=0;n<9;++n)if(n!=0){int e=rtw_r21au_coex_c2h(running.c,malformed,n,running.input.now_ms);assert(e==EINVAL);}assert(calls==running.bus.calls);malformed[0]=0x0c;assert(rtw_r21au_coex_c2h(running.c,malformed,6,running.input.now_ms)==ENOMSG&&calls==running.bus.calls);
 for(uint8_t health:{uint8_t(0xc7),uint8_t(0xff),uint8_t(0),uint8_t(0xc2)}){CoexFixture unhealthy(true);unhealthy.start();unhealthy.connect();unhealthy.bus.reg[0x80]=health;assert(rtw_r21au_coex_watchdog(unhealthy.c,&unhealthy.input)==ENXIO&&!unhealthy.status().active);calls=unhealthy.bus.calls;assert(rtw_r21au_coex_watchdog(unhealthy.c,&unhealthy.input)==ENXIO&&unhealthy.bus.calls==calls);assert(!unhealthy.boot.firmware_running);assert(rtw_r21au_coex_stop(unhealthy.c)==ENXIO&&unhealthy.bus.reg[0xcb4]==0x77&&((unhealthy.bus.read(0xcb4)>>28)&3)==2);}
}
static void asynchronous_mp(){
 for(bool late:{false,true}){CoexFixture f;f.start();f.connect();assert(!rtw_r21au_coex_watchdog(f.c,&f.input));uint8_t report[9]={9,0,1,0x49,0,20,0x20,0,40};
  for(unsigned n=0;n<4;++n){report[8]=uint8_t(40+n);++f.input.now_ms;assert(!rtw_r21au_coex_c2h(f.c,report,sizeof(report),f.input.now_ms));}
  bool query=false;for(auto cmd:f.bus.commands)query|=cmd[0]==0x67&&cmd[2]==0x2d;assert(query);auto calls=f.bus.calls;uint8_t response[10]={11,0,1,0,0,1};f.input.now_ms+=late?10:1;int e=rtw_r21au_coex_c2h(f.c,response,sizeof(response),f.input.now_ms);
  if(late){assert(e==EAGAIN&&f.status().mp_timeouts==1&&f.bus.calls==calls&&!f.status().bt_init_scan);}else{assert(!e&&f.status().bt_ble_scan_type==1&&f.status().bt_init_scan);}
 }
}
static void live_phy_guard(){
 Fixture p(true);p.b.btcoex=1;rtw_usb_rw_transport bus{&p.m,Model::in,Model::out};rtw_waiter wait{&p.m,Model::wait};assert(!rtw_r12au_phy_init(p.memory.data(),p.memory.size()*sizeof(std::max_align_t),&p.b,&bus,&wait,&p.p));p.load();p.tune(6);assert(!rtw_r12au_phy_calibrate(p.p));assert(rtw_r12au_phy_resume_tx(p.p)==EBUSY&&!p.status().tx_enabled);
 CoexFixture cx(true);cx.boot.board.phy=p.b;for(size_t n=offsetof(rtw_r12au_phy_board,txpwr_idx)+sizeof(p.b.txpwr_idx);n<sizeof(p.b);++n)((uint8_t*)&cx.boot.board.phy)[n]=0xa5;cx.boot.bus=bus;cx.boot.waiter=wait;p.m.reg[0x80]=0xc6;p.m.reg[0x770]=1;assert(!rtw_r21au_coex_init(cx.storage.data(),cx.storage.size()*sizeof(std::max_align_t),&cx.boot,&cx.c));assert(rtw_r21au_coex_bind_phy(cx.c,p.p)==ENOTSUP);cx.start();assert(!rtw_r21au_coex_bind_phy(cx.c,p.p));assert(!rtw_r12au_phy_resume_tx(p.p));assert(p.status().tx_enabled);
 CoexFixture wrong(true);wrong.start();assert(rtw_r21au_coex_bind_phy(wrong.c,p.p)==EBUSY);assert(!rtw_r12au_phy_calibrate(p.p));assert(rtw_r21au_coex_bind_phy(wrong.c,p.p)==ENOTSUP);auto calls=p.m.io;rtw_r21au_coex_forget(cx.c);assert(p.m.io==calls);assert(rtw_r12au_phy_resume_tx(p.p)==EBUSY&&!p.status().tx_enabled&&p.m.io==calls);
}
static void inactive_counter_saturation(){
 for(bool shared:{false,true}){CoexFixture f(shared);f.bus.reg[0x770]=0;f.start();bool disabled=false;
  for(unsigned n=0;n<300;++n){f.input.now_ms+=2000;assert(!rtw_r21au_coex_watchdog(f.c,&f.input));f.input.scanning=1;assert(!rtw_r21au_coex_notify(f.c,RTW_R21CX_SCAN_START,&f.input));if(disabled)assert(f.status().bt_disabled);disabled|=f.status().bt_disabled;f.input.scanning=0;assert(!rtw_r21au_coex_notify(f.c,RTW_R21CX_SCAN_FINISH,&f.input));if(disabled)assert(f.status().bt_disabled);disabled|=f.status().bt_disabled;}
  assert(disabled&&f.status().bt_disabled);
 }
}
static void immutable_owner(){
 for(unsigned action=0;action<5;++action)for(unsigned mutation=0;mutation<8;++mutation){CoexFixture a(true),b(false);if(action)a.start();
  switch(mutation){case 0:a.boot.bus.context=&b.bus;break;case 1:a.boot.bus.control_in=nullptr;break;case 2:a.boot.bus.control_out=nullptr;break;case 3:a.boot.waiter.context=&b.bus;break;case 4:a.boot.waiter.wait_us=nullptr;break;case 5:a.boot.board.phy.rfe=1;break;case 6:a.boot.board.mac[1]^=2;break;default:a.boot.chip=a.boot.board.phy.chip=RTW_R12AU_CHIP_8812AU;break;}
  auto before=a.bus.calls,foreign=b.bus.calls;uint8_t report[9]={9,0,1,0x49,0,20,0,0,40};int e=0;
  switch(action){case 0:e=rtw_r21au_coex_start(a.c,&a.input);break;case 1:e=rtw_r21au_coex_notify(a.c,RTW_R21CX_SCAN_START,&a.input);break;case 2:e=rtw_r21au_coex_watchdog(a.c,&a.input);break;case 3:e=rtw_r21au_coex_c2h(a.c,report,sizeof(report),a.input.now_ms);break;default:e=rtw_r21au_coex_stop(a.c);break;}
  assert(e==ENXIO&&a.bus.calls==before&&b.bus.calls==foreign&&!a.status().active&&!a.status().pending_work);
 }
}
static void direct_stop_health(){
 for(uint8_t flags:{uint8_t(0xc7),uint8_t(0xff),uint8_t(0xc2),uint8_t(0)}){CoexFixture f(true);f.start();f.connect();uint8_t event[9]={9,0,1,0x49,0,20,0,0,40};assert(!rtw_r21au_coex_c2h(f.c,event,sizeof(event),f.input.now_ms));f.boot.associated=1;auto sent=f.bus.commands.size();f.bus.reg[0x80]=flags;assert(rtw_r21au_coex_stop(f.c)==ENXIO);assert(!f.boot.firmware_running&&!f.boot.associated&&!f.status().active&&!f.status().pending_work&&f.bus.commands.size()==sent);assert(f.bus.reg[0xcb4]==0x77&&((f.bus.read(0xcb4)>>28)&3)==2);}
}
static void guard_live_health(){
 Fixture p(true);p.b.btcoex=1;rtw_usb_rw_transport bus{&p.m,Model::in,Model::out};rtw_waiter wait{&p.m,Model::wait};assert(!rtw_r12au_phy_init(p.memory.data(),p.memory.size()*sizeof(std::max_align_t),&p.b,&bus,&wait,&p.p));p.load();p.tune(6);assert(!rtw_r12au_phy_calibrate(p.p));CoexFixture cx(true);cx.boot.board.phy=p.b;cx.boot.bus=bus;cx.boot.waiter=wait;p.m.reg[0x80]=0xc6;assert(!rtw_r21au_coex_init(cx.storage.data(),cx.storage.size()*sizeof(std::max_align_t),&cx.boot,&cx.c));cx.start();assert(!rtw_r21au_coex_bind_phy(cx.c,p.p));p.m.reg[0x80]=0xc7;assert(rtw_r12au_phy_resume_tx(p.p)==EBUSY&&!p.status().tx_enabled);assert(!cx.status().active&&!cx.boot.firmware_running);
}
static void failures(){
 unsigned total=0;
 for(bool shared:{false,true}){CoexFixture measure(shared);measure.start();unsigned count=measure.bus.calls;
  for(bool ambiguous:{false,true})for(unsigned step=1;step<=count;++step){CoexFixture f(shared);f.bus.fail=step;f.bus.ambiguous=ambiguous;assert(rtw_r21au_coex_start(f.c,&f.input)==EIO);assert(!f.status().active&&f.status().error==EIO);auto calls=f.bus.calls;rtw_r21au_coex_forget(f.c);assert(f.bus.calls==calls);++total;}
 }
 printf("8821AU coex: %u startup I/O failures before/after effect; PTA/antenna/TDMA,H2C cursor,A2DP,5GHz,lifecycle,health and malformed C2H passed\n",total);
}
static void busyFirmwareDoesNotBlockFullStop(){
 for(bool shared:{false,true})for(unsigned box=0;box<4;++box){
  CoexFixture f(shared);f.start();f.connect();uint8_t a2dp[9]={9,0,1,0x49,0,20,0,0,40};
  assert(!rtw_r21au_coex_c2h(f.c,a2dp,sizeof(a2dp),f.input.now_ms));assert(f.status().bt_profiles==1);
  f.boot.h2c_box=box;f.bus.reg[0x1cc]=0xf;const auto sent=f.bus.commands.size();
  assert(rtw_r21au_coex_stop(f.c)==0);
  assert(!f.status().error&&!f.boot.io_error&&!f.status().active&&!f.status().pending_work);
  assert(f.bus.commands.size()==sent&&f.boot.h2c_box==box);
  assert((f.bus.read(0xaa)&0xffffU)==0x8000U);
  assert(f.bus.reg[0xcb4]==0x77&&((f.bus.read(0xcb4)>>28)&3)==(shared?2U:1U));
  f.bus.reg[0x1cc]=0;f.start();assert(f.status().active&&!f.status().error);assert(!rtw_r21au_coex_stop(f.c));
 }
 puts("8821AU full stop: active BT/A2DP, both antennas, all4 busy firmware boxes retire without H2C and restart");
}
static void fullStopIoFailuresRemainSticky(){
 unsigned faults=0;
 for(bool shared:{false,true}){
  CoexFixture measured(shared);measured.start();measured.connect();auto begin=measured.bus.calls;
  assert(!rtw_r21au_coex_stop(measured.c));const unsigned operations=measured.bus.calls-begin;assert(operations);
  for(bool ambiguous:{false,true})for(unsigned step=1;step<=operations;++step){
   CoexFixture f(shared);f.start();f.connect();f.bus.ambiguous=ambiguous;f.bus.fail=f.bus.calls+step;
   assert(rtw_r21au_coex_stop(f.c)==EIO);
   assert(f.status().error==EIO&&f.boot.io_error==EIO&&!f.status().active&&!f.status().pending_work);
   f.bus.fail=0;assert(rtw_r21au_coex_stop(f.c)==EIO&&f.boot.io_error==EIO);++faults;
  }
 }
 std::printf("8821AU full stop: %u antenna/scoreboard/power-off I/O faults before/after effect stay sticky\n",faults);
}
int main(){busyFirmwareDoesNotBlockFullStop();fullStopIoFailuresRemainSticky();traces();boundaries();direct_stop_health();immutable_owner();inactive_counter_saturation();asynchronous_mp();live_phy_guard();guard_live_health();failures();}
