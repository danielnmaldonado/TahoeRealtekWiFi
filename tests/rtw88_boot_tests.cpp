/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88Boot.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <vector>
static void put(std::array<uint8_t,65536>&r,unsigned a,unsigned w,uint32_t v){for(unsigned j=0;j<w;j++)r[a+j]=uint8_t(v>>(j*8));}
static const rtw_usb_wifi_profile*profile(rtw_wifi_family f){for(size_t i=0;i<rtw_usb_wifi_profile_count();i++){auto*p=rtw_usb_wifi_profile_at(i);if(p->family==f&&p->primary_verified)return p;}return nullptr;}
static std::vector<uint8_t>logical(rtw_wifi_family f){
 std::vector<uint8_t>b(f==RTW_WIFI_RTL8821CU?512:768,255);b[0]=0x29;b[1]=0x81;
 for(unsigned i=0;i<168;i++)b[0x10+i]=uint8_t(16+i%32);
 b[0xb8]=0x7f;b[0xb9]=0x2a;b[0xba]=0x18;b[0xbc]=0x11;b[0xbd]=8;b[0xbf]=8;
 b[0xc1]=0x21;b[0xc3]=1;b[0xca]=f==RTW_WIFI_RTL8822BU?3:2;b[0xcb]='U';b[0xcc]='S';
 if(f==RTW_WIFI_RTL8822CU){b[0xd0]=0x21;b[0xd1]=0x23;b[0xc8]=0x40;}
 unsigned mac=f==RTW_WIFI_RTL8822CU?0x157:0x107;uint8_t addr[6]={2,0x11,0x22,0x33,0x44,0x55};std::memcpy(b.data()+mac,addr,6);return b;
}
static std::array<uint8_t,1024>physical(const std::vector<uint8_t>&b){
 std::array<uint8_t,1024>out;out.fill(255);size_t p=0;
 for(unsigned block=0;block<b.size()/8;block++){
  unsigned mask=15;for(unsigned word=0;word<4;word++)if(b[block*8+word*2]!=255||b[block*8+word*2+1]!=255)mask&=~(1U<<word);
  if(mask==15)continue;
  if(block<16)out[p++]=uint8_t(block*16+mask);else{out[p++]=uint8_t((block&7)*32+15);out[p++]=uint8_t(((block&0x78)*2)|mask);}
  for(unsigned word=0;word<4;word++)if(!(mask&(1U<<word))){out[p++]=b[block*8+word*2];out[p++]=b[block*8+word*2+1];}
 }
 assert(p<388);return out;
}
struct Bus {
 rtw_wifi_family family;std::array<uint8_t,65536>reg{};std::array<uint8_t,1024>otp;
 unsigned reads=0,writes=0,otp_reads=0,delay_calls=0;uint64_t delay_us=0;int fail_read=0,fail_write=0;bool otp_busy=false,power_busy=false,coex_busy=false;uint32_t coex=0x12345678;
 explicit Bus(rtw_wifi_family f):family(f),otp(physical(logical(f))){put(reg,0xf0,4,f==RTW_WIFI_RTL8821CU?0:0x08000000);put(reg,0xfc,4,0x1234);reg[6]=2;reg[0x100]=0xea;reg[0xf5]=1;reg[0x37]=0x80;reg[0x29]=1;}
 static int read(void*p,uint16_t a,unsigned n,uint32_t*v){auto&b=*static_cast<Bus*>(p);++b.reads;*v=0;if(int(b.reads)==b.fail_read)return EIO;
  for(unsigned j=0;j<n;j++)*v|=uint32_t(b.reg[a+j])<<(j*8);
  if(a==6&&b.power_busy)*v&=~2U;if(a==0x1700)*v=b.coex_busy?0:0x20000000;if(a==0x1708)*v=b.coex;return 0;}
 static int write(void*p,uint16_t a,unsigned n,uint32_t v){auto&b=*static_cast<Bus*>(p);++b.writes;if(int(b.writes)==b.fail_write)return EIO;
  if(a==0x30){assert((b.reg[0x35]&3)==0);assert(b.family==RTW_WIFI_RTL8822CU?!(b.reg[0x29]&1):!(b.reg[0x37]&0x80));unsigned at=(v>>8)&1023;assert(at<(b.family==RTW_WIFI_RTL8822BU?1024U:512U));++b.otp_reads;v=(v&~255U)|b.otp[at];if(!b.otp_busy)v|=0x80000000;}
  if(a==5&&n==1){if(v&1){v&=~1U;b.reg[0x100]=0;}if(v&2){v&=~2U;b.reg[0x100]=0xea;}}
  if(a==0x1704)b.coex=v;
  put(b.reg,a,n,v);return 0;}
 static void delay(void*p,uint32_t us){auto&b=*static_cast<Bus*>(p);++b.delay_calls;b.delay_us+=us;assert(us<=1000000);}
 rtw88_fw_bus ops(){return {this,read,write,nullptr,delay,nullptr,nullptr};}
};
static void ready(rtw88_boot_context&c,Bus&b){auto ops=b.ops();assert(!rtw88_boot_init(&c,profile(b.family),&ops));assert(!rtw88_boot_identify(&c));assert(!rtw88_boot_power_on(&c));c.firmware.firmware_running=1;}
int main(){
 unsigned injected=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  rtw88_efuse_geometry g{};assert(!rtw88_boot_geometry(family,&g));assert(g.physical_bytes==(family==RTW_WIFI_RTL8822BU?1024:512));assert(g.logical_bytes==(family==RTW_WIFI_RTL8821CU?512:768));assert(g.protected_bytes==(family==RTW_WIFI_RTL8822CU?124:96));
  auto log=logical(family);auto phy=physical(log);std::array<uint8_t,768>decoded{};
  assert(!rtw88_boot_decode_efuse(family,phy.data(),g.physical_bytes,decoded.data(),g.logical_bytes));assert(!std::memcmp(log.data(),decoded.data(),log.size()));
  Bus b(family);rtw88_boot_context c{};auto ops=b.ops();assert(!rtw88_boot_init(&c,profile(family),&ops));assert(!b.reads&&!b.writes);
  assert(!rtw88_boot_identify(&c));assert(c.firmware.identified&&c.board.rf_paths==(family==RTW_WIFI_RTL8821CU?1:2));assert(!rtw88_boot_power_on(&c));assert(c.firmware.powered);
  assert((b.reg[0xf5]&1)==0);assert((b.reg[0x1081]&1)==1);assert(b.reg[3]==(family==RTW_WIFI_RTL8822BU?0xdc:0xd8));
  c.firmware.firmware_running=1;std::array<uint8_t,1024>scratch{};assert(!rtw88_boot_read_board(&c,scratch.data(),scratch.size()));assert(c.board_valid&&c.board.mac[0]==2&&c.board.mac[5]==0x55);assert(b.otp_reads==g.physical_bytes);assert(c.physical_valid&&!std::memcmp(c.physical,b.otp.data(),g.physical_bytes));
  b.reg[0x1a0]=0x19;put(b.reg,0x1a6,4,(7U<<16)|(1U<<19)|((family==RTW_WIFI_RTL8821CU?1U:2U)<<21)|(3U<<26));assert(!rtw88_boot_read_features(&c));assert(c.board.features_valid&&c.board.tx_streams==1&&c.board.feature_bw==80&&c.board.feature_protocol==3&&b.reg[0x1a0]==0);
  for(unsigned fail=1;fail<=14;fail++){Bus hardware(family);rtw88_boot_context failed{};ready(failed,hardware);failed.board_valid=1;hardware.reg[0x1a0]=0x19;hardware.fail_read=int(hardware.reads+fail);assert(rtw88_boot_read_features(&failed)==EIO&&!failed.board.features_valid);}
  {Bus hardware(family);rtw88_boot_context state{};ready(state,hardware);state.board_valid=1;hardware.reg[0x1a0]=0x19;put(hardware.reg,0x1a6,4,(7U<<16)|(1U<<19)|(1U<<21));assert(!rtw88_boot_read_features(&state)&&state.board.active_rf_paths==1&&state.board.tx_streams==1);}
  assert(b.reg[0x37]==0x80&&b.reg[0x29]==(family==RTW_WIFI_RTL8822BU?0xf9:1));assert(c.board.thermal_meter[0]==(family==RTW_WIFI_RTL8822CU?0x21:0x18));assert(c.board.thermal_meter_k==(family==RTW_WIFI_RTL8822CU?0x22:0x18));
  assert(c.board.txpwr_idx_table[0][0]==(family==RTW_WIFI_RTL8821CU?log[0x10+42]:log[0x10]));
  uint32_t coex=0;assert(!c.firmware.bus.coex_read(c.firmware.bus.cookie,0x38,&coex)&&coex==0x12345678);assert(!c.firmware.bus.coex_write(c.firmware.bus.cookie,0x38,0x87654321)&&b.coex==0x87654321);
  assert(!rtw88_boot_stop(&c));assert(!c.firmware.powered&&!c.firmware.firmware_running&&!c.board_valid);
  for(int position:{1,2,3,4,5,15,50,250,500}){
   Bus bus(family);rtw88_boot_context state{};ready(state,bus);bus.fail_write=int(bus.writes)+position;
   assert(rtw88_boot_read_board(&state,scratch.data(),scratch.size())==EIO);assert(!state.board_valid&&!state.physical_valid);assert(bus.reg[0x37]==0x80&&bus.reg[0x29]==(family==RTW_WIFI_RTL8822BU?0xf9:1));++injected;
  }
  {Bus bus(family);rtw88_boot_context state{};ready(state,bus);bus.otp_busy=true;
   assert(rtw88_boot_read_board(&state,scratch.data(),scratch.size())==ETIMEDOUT);assert(!state.board_valid&&bus.reg[0x37]==0x80&&bus.reg[0x29]==(family==RTW_WIFI_RTL8822BU?0xf9:1));}
  {Bus bus(family);rtw88_boot_context state{};auto io=bus.ops();assert(!rtw88_boot_init(&state,profile(family),&io));assert(!rtw88_boot_identify(&state));bus.power_busy=true;
   assert(rtw88_boot_power_on(&state)==ETIMEDOUT);assert(!state.firmware.powered&&bus.delay_us<1100000);}
  {Bus bus(family);rtw88_boot_context state{};auto io=bus.ops();assert(!rtw88_boot_init(&state,profile(family),&io));
   assert(rtw88_boot_parse_board(&state,log.data(),log.size())==ENXIO&&!state.board_valid);}
  for(int position:{1,2,3,4,5,15,50,250,500}){
   Bus bus(family);rtw88_boot_context state{};ready(state,bus);bus.fail_read=int(bus.reads)+position;
   assert(rtw88_boot_read_board(&state,scratch.data(),scratch.size())==EIO);assert(!state.board_valid&&bus.reg[0x37]==0x80&&bus.reg[0x29]==(family==RTW_WIFI_RTL8822BU?0xf9:1));++injected;
  }
  {Bus baseline(family);rtw88_boot_context state{};auto io=baseline.ops();assert(!rtw88_boot_init(&state,profile(family),&io));assert(!rtw88_boot_identify(&state));
   unsigned before_reads=baseline.reads;assert(!rtw88_boot_power_on(&state));
   for(unsigned at=1;at<=baseline.writes;at++){Bus bus(family);rtw88_boot_context failed{};auto raw=bus.ops();assert(!rtw88_boot_init(&failed,profile(family),&raw));assert(!rtw88_boot_identify(&failed));bus.fail_write=int(at);
    assert(rtw88_boot_power_on(&failed)==EIO&&!failed.firmware.powered&&!failed.board_valid);++injected;}
   for(unsigned at=1;at<=baseline.reads-before_reads;at++){Bus bus(family);rtw88_boot_context failed{};auto raw=bus.ops();assert(!rtw88_boot_init(&failed,profile(family),&raw));assert(!rtw88_boot_identify(&failed));bus.fail_read=int(bus.reads+at);
    assert(rtw88_boot_power_on(&failed)==EIO&&!failed.firmware.powered&&!failed.board_valid);++injected;}
  }
  {Bus bus(family);bus.reg[0x100]=0;bus.reg[0xf5]=0;rtw88_boot_context state{};ready(state,bus);assert(state.firmware.powered);}
  {Bus bus(family);rtw88_boot_context state{};auto io=bus.ops();assert(!rtw88_boot_init(&state,profile(family),&io));bus.reg[0xf1]=0x20;assert(!rtw88_boot_identify(&state));assert(!rtw88_boot_power_on(&state));if(family==RTW_WIFI_RTL8822BU)assert(bus.reg[0x10a9]==0xef&&bus.reg[0x10aa]==0x0c);}
  {Bus bus(family);rtw88_boot_context state{};ready(state,bus);bus.otp.fill(255);assert(rtw88_boot_read_board(&state,scratch.data(),scratch.size())==EINVAL&&!state.board_valid);}
  {Bus bus(family);rtw88_boot_context state{};ready(state,bus);bus.coex_busy=true;uint32_t out=123;unsigned start=bus.delay_calls;assert(state.firmware.bus.coex_read(state.firmware.bus.cookie,0x38,&out)==ETIMEDOUT&&out==0);assert(bus.delay_calls-start==999);}
  {auto invalid=phy;invalid.fill(0x1f);invalid[g.physical_bytes-g.protected_bytes-1]=0x0f;
   assert(rtw88_boot_decode_efuse(family,invalid.data(),g.physical_bytes,decoded.data(),g.logical_bytes)==EINVAL);
   invalid=phy;invalid[0]=0xef;invalid[1]=0xfe;invalid[2]=0;invalid[3]=0;
   assert(rtw88_boot_decode_efuse(family,invalid.data(),g.physical_bytes,decoded.data(),g.logical_bytes)==EINVAL);
   assert(rtw88_boot_decode_efuse(family,phy.data(),g.physical_bytes,phy.data(),g.logical_bytes)==EINVAL);
  }
  auto bad=log;bad[0xca]=255;assert(rtw88_boot_parse_board(&c,bad.data(),bad.size())==ENODEV);assert(!c.board_valid);
  bad=log;bad[g.usb_mac_offset]|=1;assert(rtw88_boot_parse_board(&c,bad.data(),bad.size())==EINVAL);
  assert(rtw88_boot_decode_efuse(family,phy.data(),g.physical_bytes-1,decoded.data(),g.logical_bytes)==EINVAL);
 }
 {Bus b(RTW_WIFI_RTL8821CU);rtw88_boot_context c{};auto ops=b.ops();auto fake=*profile(b.family);assert(rtw88_boot_init(&c,&fake,&ops)==ENODEV);assert(!b.reads&&!b.writes&&!c.firmware.identified);}
 {Bus b(RTW_WIFI_RTL8822BU);rtw88_boot_context c{};auto ops=b.ops();assert(!rtw88_boot_init(&c,profile(b.family),&ops));put(b.reg,0xf0,4,0xffffffff);assert(rtw88_boot_identify(&c)==ENODEV&&!c.firmware.identified);}
 std::printf("rtw88 B/C chip power,OTP/board maps,LTE coex,timeouts and%u injected failures passed\n",injected);
}
