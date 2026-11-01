/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88MAC.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
struct Bus {
 std::array<uint8_t,65536>reg{};unsigned reads=0,writes=0,delays=0;int fail_write=0,fail_read=0;bool llt_busy=false,h2c_busy=false;
 static int read(void*p,uint16_t a,unsigned n,uint32_t*v){auto&b=*static_cast<Bus*>(p);++b.reads;*v=0;if(b.fail_read==int(b.reads))return EIO;for(unsigned j=0;j<n;j++)*v|=uint32_t(b.reg[a+j])<<(j*8);if(a==0x208&&b.llt_busy)*v|=1;if(a==0x10d4&&b.h2c_busy)*v=128;return 0;}
 static int write(void*p,uint16_t a,unsigned n,uint32_t v){auto&b=*static_cast<Bus*>(p);++b.writes;if(b.fail_write==int(b.writes))return EIO;if(a==0x208)v&=~1U;for(unsigned j=0;j<n;j++)b.reg[a+j]=uint8_t(v>>(j*8));return 0;}
 static void delay(void*p,uint32_t n){assert(n==10);++static_cast<Bus*>(p)->delays;}
 rtw88_fw_context context(rtw_wifi_family f){return {f,{this,read,write,nullptr,delay,nullptr,nullptr},1,1,1,0};}
 uint32_t get(uint16_t a,unsigned n){uint32_t v;read(this,a,n,&v);return v;}
};
int main(){
 for(auto f:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned pipes=2;pipes<=4;pipes++){
  rtw88_mac_plan p{};assert(!rtw88_mac_prepare(f,pipes,&p));
  assert(p.tx_pages==(f==RTW_WIFI_RTL8821CU?512:2048));assert(p.reserved_boundary==(f==RTW_WIFI_RTL8821CU?460:f==RTW_WIFI_RTL8822BU?1996:1938));
  assert(p.reserved_pages==(f==RTW_WIFI_RTL8822CU?110:52));assert(p.public_pages+p.high_pages+p.low_pages+p.normal_pages+p.extra_pages+1==p.reserved_boundary);
  assert(p.queue_mapping==(pipes==2?0xfea0:pipes==3?0xf5a0:0xc5a0));
  for(unsigned q=0;q<20;q++){uint8_t ep=99;int e=rtw88_mac_queue_endpoint(&p,q,&ep);if(q>=8&&q<=15)assert(e==EINVAL&&ep==99);else assert(!e&&ep<pipes);}
  for(auto speed:{RTW88_USB_FULL_SPEED,RTW88_USB_HIGH_SPEED,RTW88_USB_SUPER_SPEED}){
   Bus b;auto dev=b.context(f);rtw88_mac_context c{};assert(!rtw88_mac_initialize(&c,&dev,pipes,speed));assert(c.mac_ready);
   assert(b.get(0x100,1)==0xff);assert(b.get(0x204,2)==p.reserved_boundary);assert(b.get(0x230,2)==p.high_pages);assert(b.get(0x240,2)==p.public_pages);
   assert(b.get(0x11c,4)==p.rx_fifo_bytes-257);assert(b.get(0x244,4)==uint32_t(p.h2c_page)*128);assert(b.get(0x248,4)==uint32_t(p.h2c_page+8)*128);
   assert(b.get(0x290,1)==(speed==RTW88_USB_FULL_SPEED?0x2e:speed==RTW88_USB_HIGH_SPEED?0x1e:0x0e));assert(b.get(0x280,2)==0x2005);
   assert(b.get(0x60f,1)==4);assert(b.get(0x608,4)&0x10000000);assert(b.get(0x718,1)&0x40);
  }
 }
 Bus b;auto dev=b.context(RTW_WIFI_RTL8822CU);rtw88_mac_context c{};dev.identified=0;
 assert(rtw88_mac_initialize(&c,&dev,3,RTW88_USB_HIGH_SPEED)==ENXIO&&!b.reads&&!b.writes);
 dev.identified=1;b.llt_busy=true;assert(rtw88_mac_initialize(&c,&dev,3,RTW88_USB_HIGH_SPEED)==ETIMEDOUT);assert(!c.mac_ready&&b.get(0x100,1)==0&&b.delays==999);
 unsigned failure_cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  Bus baseline;auto reference=baseline.context(family);rtw88_mac_context reference_state{};
  assert(!rtw88_mac_initialize(&reference_state,&reference,3,RTW88_USB_HIGH_SPEED));
  for(unsigned at=1;at<=baseline.writes;at++){
   Bus bus;auto d=bus.context(family);bus.fail_write=int(at);rtw88_mac_context state{};
   assert(rtw88_mac_initialize(&state,&d,3,RTW88_USB_HIGH_SPEED)==EIO);assert(!state.mac_ready&&bus.get(0x100,1)==0);++failure_cases;
  }
  for(unsigned at=1;at<=baseline.reads;at++){
   Bus bus;auto d=bus.context(family);bus.fail_read=int(at);rtw88_mac_context state{};
   assert(rtw88_mac_initialize(&state,&d,3,RTW88_USB_HIGH_SPEED)==EIO);assert(!state.mac_ready&&bus.get(0x100,1)==0);++failure_cases;
  }
  Bus ones;ones.reg.fill(255);auto ones_device=ones.context(family);rtw88_mac_context ones_state{};
  assert(!rtw88_mac_initialize(&ones_state,&ones_device,3,RTW88_USB_HIGH_SPEED));
  assert(ones.get(0x7d4,4)==0xfffffc98U);assert(ones.get(0x115,1)==255);
 }
 {Bus bus;auto device=bus.context(RTW_WIFI_RTL8821CU);rtw88_mac_context state{};state.mac_ready=1;
  assert(rtw88_mac_initialize(&state,&device,3,static_cast<rtw88_usb_speed>(99))==EINVAL);assert(!state.mac_ready&&!bus.writes&&!bus.reads);}
 {Bus bus;bus.h2c_busy=true;auto device=bus.context(RTW_WIFI_RTL8821CU);rtw88_mac_context state{};
  assert(rtw88_mac_initialize(&state,&device,3,RTW88_USB_HIGH_SPEED)==EBUSY);assert(!state.mac_ready&&bus.get(0x100,1)==0);}
 rtw88_mac_plan plan{};assert(rtw88_mac_prepare(RTW_WIFI_RTL8814BU,3,&plan)==EINVAL);assert(rtw88_mac_prepare(RTW_WIFI_RTL8821CU,1,&plan)==EINVAL);assert(rtw88_mac_prepare(RTW_WIFI_RTL8821CU,5,&plan)==EINVAL);
 std::printf("rtw88 MAC FIFO/USB mapping,27 chip/pipe/speed traces,LLT/H2C failures and%u injected I/O failures passed\n",failure_cases);
}
