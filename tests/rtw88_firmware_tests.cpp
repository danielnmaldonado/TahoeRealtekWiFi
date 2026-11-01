/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88Firmware.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstring>
#include <cerrno>
#include <cstdio>
static void put32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=uint8_t(v>>(i*8));}
static std::vector<uint8_t> firmware(rtw_wifi_family f){
 std::vector<uint8_t>d(64+40+40+24,0);d[0]=f==RTW_WIFI_RTL8821CU?0x21:0x22;d[1]=0x88;d[4]=1;d[24]=0x10;
 put32(d.data()+32,0x80200000);put32(d.data()+36,32);put32(d.data()+48,32);put32(d.data()+52,16);
 put32(d.data()+56,0x80100000);put32(d.data()+60,f==RTW_WIFI_RTL8822BU?0x80000000:0x80030000);return d;
}
struct Mock {
 std::array<uint8_t,65536>registers{};unsigned reads=0,writes=0,bulks=0,delays=0;int failAt=0;
 bool badChecksum=false,busyDMA=false,noReady=false;uint32_t coex=0x3456789a;
 std::vector<uint32_t> destinations,dmaLengths;unsigned padded=0;
 Mock(){registers[3]=4;registers[0x1d]=1;registers[9]=0x40;registers[0x1082]=1;registers[0x10d]=0x52;registers[0x100]=0x7f;registers[0x230]=0x44;registers[0x22c]=0x55;registers[0x550]=0x08;}
 static int read(void*p,uint16_t a,unsigned w,uint32_t*v){auto&m=*static_cast<Mock*>(p);++m.reads;*v=0;for(unsigned i=0;i<w;i++)*v|=uint32_t(m.registers[a+i])<<(8*i);if(a==0x1208&&m.busyDMA)*v|=0x80000000;return 0;}
 static int write(void*p,uint16_t a,unsigned w,uint32_t v){auto&m=*static_cast<Mock*>(p);++m.writes;if(m.failAt&&m.writes==unsigned(m.failAt))return EIO;
  if(a==0x1204)m.destinations.push_back(v);
  if(a==0x1208&&(v&0x80000000U))m.dmaLengths.push_back(v&0x3ffffU);
  if(a==0x1200)assert(v==0x18780030U);
  if(a==0x1208){v&=~0x80000000U;if(v&0x02000000U)v&=~0x08000000U;else if(m.badChecksum)v|=0x08000000U;}
  for(unsigned i=0;i<w;i++)m.registers[a+i]=uint8_t(v>>(8*i));
  if(a==3&&(v&4)&&!m.noReady&&(m.registers[0x81]&0x40)){m.registers[0x80]=0x78;m.registers[0x81]=0xc0;}
  return 0;}
 static int bulk(void*p,uint8_t queue,const uint8_t*b,size_t n){auto&m=*static_cast<Mock*>(p);assert(queue==16&&n>=49&&n<=4145);assert(b[2]==48);assert(b[3]==4&&b[12]==0&&b[13]==0&&b[33]==0);if((n&511)==1){assert(b[n-1]==0);++m.padded;}++m.bulks;m.registers[0x205]|=0x80;return 0;}
 static void delay(void*p,uint32_t u){auto&m=*static_cast<Mock*>(p);assert(u<=1000);++m.delays;}
 static int cr(void*p,uint16_t a,uint32_t*v){assert(a==0x38);*v=static_cast<Mock*>(p)->coex;return 0;}
 static int cw(void*p,uint16_t a,uint32_t v){assert(a==0x38);static_cast<Mock*>(p)->coex=v;return 0;}
 rtw88_fw_context context(rtw_wifi_family f){return {f,{this,read,write,bulk,delay,cr,cw},1,1,0,0};}
};
int main(){
 {auto c_image=firmware(RTW_WIFI_RTL8822CU);rtw88_fw_image parsed{};assert(rtw88_fw_inspect(RTW_WIFI_RTL8822BU,c_image.data(),c_image.size(),&parsed)==EINVAL);}
 std::array<uint8_t,RTW88_FW_SCRATCH_BYTES>scratch{};
 for(auto f:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  auto bytes=firmware(f);rtw88_fw_image image{};assert(rtw88_fw_inspect(f,bytes.data(),bytes.size(),&image)==0);assert(image.dmem.length==40&&image.imem.length==40&&image.emem.length==24);
  Mock m;auto c=m.context(f);assert(rtw88_fw_download(&c,bytes.data(),bytes.size(),scratch.data(),scratch.size())==0);assert(c.firmware_running&&m.bulks==3);assert(m.registers[0x10d]==0x52&&m.registers[0x100]==0x7f&&m.registers[0x230]==0x44&&m.registers[0x550]==8&&m.coex==0x3456789a);
 }
 auto f=RTW_WIFI_RTL8822CU;auto bytes=firmware(f);Mock m;auto c=m.context(f);c.powered=0;assert(rtw88_fw_download(&c,bytes.data(),bytes.size(),scratch.data(),scratch.size())==ENXIO);assert(!m.reads&&!m.writes);
 c.powered=1;bytes[0]=0;assert(rtw88_fw_download(&c,bytes.data(),bytes.size(),scratch.data(),scratch.size())==EINVAL);assert(!m.reads&&!m.writes);
 bytes=firmware(f);m.badChecksum=true;assert(rtw88_fw_download(&c,bytes.data(),bytes.size(),scratch.data(),scratch.size())!=0);assert(!c.firmware_running&&!(m.registers[0x80]&1));assert(m.registers[0x10d]==0x52&&m.registers[0x100]==0x7f&&m.registers[0x550]==8);
 // Bound every failure: a stale busy DMA must time out and restore snapshots.
 {Mock bus;bus.busyDMA=true;auto ctx=bus.context(f);auto fw=firmware(f);
  assert(rtw88_fw_download(&ctx,fw.data(),fw.size(),scratch.data(),scratch.size())==ETIMEDOUT);
  assert(!ctx.firmware_running&&bus.reads<1100&&bus.delays==999&&!(bus.registers[0x80]&1));
  assert(bus.registers[0x10d]==0x52&&bus.registers[0x100]==0x7f&&bus.registers[0x550]==8);}
 {Mock bus;bus.noReady=true;auto ctx=bus.context(f);auto fw=firmware(f);
  assert(rtw88_fw_download(&ctx,fw.data(),fw.size(),scratch.data(),scratch.size())==ETIMEDOUT);
  assert(!ctx.firmware_running&&bus.reads<1200&&bus.delays==999&&!(bus.registers[0x80]&1));}
 // Final464-byte DMEM chunk would be512 bytes including TX48; the port
 // appends an explicit zero and IDDMA still copies exactly464 bytes.
 {auto fw=firmware(f);fw.resize(64+464+4204);fw[24]=0;put32(fw.data()+36,456);put32(fw.data()+48,4196);put32(fw.data()+52,0);
  Mock bus;auto ctx=bus.context(f);assert(!rtw88_fw_download(&ctx,fw.data(),fw.size(),scratch.data(),scratch.size()));
  assert(bus.padded==1&&bus.bulks==3);assert((bus.dmaLengths==std::vector<uint32_t>{464,4096,108}));
  assert((bus.destinations==std::vector<uint32_t>{0x200000,0x30000,0x31000}));}
 for(int at=1;at<=48;at++){
  Mock bus;bus.failAt=at;auto ctx=bus.context(f);auto fw=firmware(f);
  int e=rtw88_fw_download(&ctx,fw.data(),fw.size(),scratch.data(),scratch.size());
  assert(e==EIO&&!ctx.firmware_running&&!(bus.registers[0x80]&1));
  assert(bus.registers[0x10d]==0x52&&bus.registers[0x100]==0x7f&&bus.registers[0x550]==8);
 }
 {auto fw=firmware(f);rtw88_fw_image im{};
  assert(rtw88_fw_inspect(RTW_WIFI_RTL8814BU,fw.data(),fw.size(),&im)==EINVAL);
  assert(rtw88_fw_inspect(f,fw.data(),63,&im)==EINVAL);
  put32(fw.data()+48,0xffffffffU);assert(rtw88_fw_inspect(f,fw.data(),fw.size(),&im)==EINVAL);
  fw=firmware(f);put32(fw.data()+60,0x80000000);assert(rtw88_fw_inspect(f,fw.data(),fw.size(),&im)==EINVAL);
  fw=firmware(f);Mock bus;auto ctx=bus.context(f);
  assert(rtw88_fw_download(&ctx,fw.data(),fw.size(),fw.data(),fw.size())==EINVAL);assert(!bus.reads&&!bus.writes);
 }
 std::puts("rtw88 firmware: chip boot traces,48 write failures, bounded timeouts, fragmentation, zero padding and input guards passed");
}
