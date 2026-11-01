/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW88_OPERATING_TEST_BUS_H
#define RTW88_OPERATING_TEST_BUS_H
#include "rtw/RTW88PHYOperating.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <vector>
struct OperatingBus {
 rtw_wifi_family family;std::array<uint8_t,65536>reg{};std::array<std::array<uint32_t,256>,2>rf{};
 unsigned reads=0,writes=0,bulks=0,gnt_override_write=0;uint64_t delays=0;int fail_read=0,fail_write=0;bool iqk_timeout=false,dac_timeout=false,iqk_bad=false,rf_lut_timeout=false,rfk_timeout=false,bad_samples=false;uint32_t coex=0x12345678;
 std::vector<std::array<uint8_t,80>>packets;
 int mcu_after_pause=-1;bool mcu_read_failure=false,health_fault_active=false;unsigned unpaused_after_fault=0;
 explicit OperatingBus(rtw_wifi_family f):family(f){for(auto&path:rf){path[0]=0x30000;path[0x42]=(0x24<<10)|(0x24<<1);path[0x3f]=0x1000;}reg[0x37]=0x80;}
 int rf_address(uint16_t a,unsigned n,unsigned&path,unsigned&index){if(n!=4)return 0;unsigned base=family==RTW_WIFI_RTL8822CU?0x3c00:0x2800;if(a>=base&&a<=base+0x3fc&&a%4==0){path=0;index=(a-base)/4;return 1;}base=family==RTW_WIFI_RTL8822CU?0x4c00:0x2c00;if(a>=base&&a<=base+0x3fc&&a%4==0){path=1;index=(a-base)/4;return 1;}return 0;}
 uint32_t get(unsigned a,unsigned n=4)const{uint32_t v=0;for(unsigned i=0;i<n;i++)v|=uint32_t(reg[a+i])<<(8*i);return v;}
 void put(unsigned a,uint32_t v,unsigned n=4){for(unsigned i=0;i<n;i++)reg[a+i]=uint8_t(v>>(i*8));}
 static int read(void*p,uint16_t a,unsigned n,uint32_t*out){auto&b=*static_cast<OperatingBus*>(p);++b.reads;*out=0;if(int(b.reads)==b.fail_read)return EIO;
  if(a==0x80&&n==2&&b.mcu_read_failure)return EIO;
  unsigned path=0,index=0;if(b.rf_address(a,n,path,index)){*out=b.rf[path][index];if(index==0x33&&b.rf_lut_timeout)*out=0;return 0;}
  *out=b.get(a,n);if(a==0x1700)*out=0x20000000;if(a==0x1708)*out=b.coex;
  if(b.family==RTW_WIFI_RTL8822CU&&(a==0x2808||a==0x2834||a==0x4508||a==0x4534))*out=b.dac_timeout?0:0x7fff80;
  if(b.family==RTW_WIFI_RTL8822CU&&(a==0x2824||a==0x2850||a==0x4524||a==0x4550))*out=b.dac_timeout?0:0x04000000;
  if(a==0x1bfc){uint32_t selection=b.get(0x1bd4);if((selection&0x001f0000)==0x00120000)*out=0;else if(selection==0x000900f0)*out=0x80000000;else if(selection==0x00060001)*out=0x00004000;else if((selection&0x00ff0000)==0x00040000)*out=0x04001fff;else if(selection==0x00060000)*out=0;else *out=0x84000032;}
  if(a==0x2dbc&&b.bad_samples){*out=0x100100;return 0;}
  if(a==0x2dbc){bool offset=(b.get(0x1868)&1023)==16||(b.get(0x4168)&1023)==16;*out=offset?0x10010:0;} // measured DAC sample follows the applied16-unit offset
  return 0;}
 static int write(void*p,uint16_t a,unsigned n,uint32_t v){auto&b=*static_cast<OperatingBus*>(p);++b.writes;if(int(b.writes)==b.fail_write)return EIO;
  if(n==4&&a==0x70&&(v&(1U<<26)))b.gnt_override_write=b.writes;unsigned path=0,index=0;if(b.rf_address(a,n,path,index)){b.rf[path][index]=v;return 0;}
  uint16_t sipi0=b.family==RTW_WIFI_RTL8822CU?0x1808:0xc90,sipi1=b.family==RTW_WIFI_RTL8822CU?0x4108:0xe90;
  if(n==4&&(a==sipi0||a==sipi1)){b.rf[a==sipi1?1:0][(v>>20)&255]=v&0xfffff;}
  if(a==0x1704)b.coex=v;
  if(a==0x1b00&&!b.rfk_timeout)b.reg[0x2d9c]=0x55;
  if(a==0x1b10&&(v&255)==0)b.reg[0x2d9c]=0;
  if(a==5&&n==1)v&=~3U;
  b.put(a,v,n);
  if(a==0x522&&n==1){if((v&255U)!=255U&&b.health_fault_active)++b.unpaused_after_fault;
   if((v&255U)==255U&&b.mcu_after_pause>=0){b.health_fault_active=true;if(b.mcu_after_pause==0x10000)b.mcu_read_failure=true;else b.put(0x80,unsigned(b.mcu_after_pause),2);b.mcu_after_pause=-1;}}
  // Firmware acknowledgement follows an actual RFK mailbox command.
  if(n==4&&a>=0x1d0&&a<=0x1dc&&(v&255)==0x6d)b.reg[0x49c]|=1;
  return 0;}
 static int bulk(void*p,uint8_t q,const uint8_t*packet,size_t bytes){auto&b=*static_cast<OperatingBus*>(p);assert(q==19&&bytes==80);++b.bulks;std::array<uint8_t,80>saved;std::memcpy(saved.data(),packet,80);b.packets.push_back(saved);
  assert(packet[0]==32&&packet[2]==0&&packet[3]==0&&packet[5]==19&&packet[48]==1&&packet[49]==255&&packet[50]==14&&packet[52]==9);
  if(!b.iqk_timeout){if(b.family==RTW_WIFI_RTL8822CU)b.reg[0x2d9c]=0xaa;else b.rf[0][8]=0xabcde;}
  if(b.iqk_bad)b.reg[0x1bf0]=1;return 0;}
 static void delay(void*p,uint32_t us){auto&b=*static_cast<OperatingBus*>(p);b.delays+=us;assert(us<=50000);}
 rtw88_fw_bus ops(){return {this,read,write,bulk,delay,nullptr,nullptr};}
};
inline const rtw_usb_wifi_profile*operating_profile(rtw_wifi_family f){for(size_t i=0;i<rtw_usb_wifi_profile_count();i++){auto*p=rtw_usb_wifi_profile_at(i);if(p->family==f&&p->primary_verified)return p;}return nullptr;}
inline void operating_fixture(OperatingBus&bus,rtw88_boot_context&boot,rtw88_mac_context&mac,rtw88_phy_context&phy){
 auto ops=bus.ops();assert(!rtw88_boot_init(&boot,operating_profile(bus.family),&ops));if(bus.family!=RTW_WIFI_RTL8821CU)bus.reg[0xf3]=8;assert(!rtw88_boot_identify(&boot));
 std::array<uint8_t,768>rom;rom.fill(255);for(unsigned p=0;p<4;p++){for(unsigned i=0;i<42;i++)rom[0x10+p*42+i]=0;for(unsigned i=0;i<11;i++)rom[0x10+p*42+i]=24;for(unsigned i=0;i<14;i++)rom[0x10+p*42+18+i]=24;}
 rom[0xca]=bus.family==RTW_WIFI_RTL8822BU?3:2;rom[0xb9]=0x2a;rom[0xba]=0x24;rom[0xd0]=rom[0xd1]=0x24;rom[0xc8]=0;
 unsigned offset=bus.family==RTW_WIFI_RTL8822CU?0x157:0x107;rom[offset]=2;for(unsigned i=1;i<6;i++)rom[offset+i]=uint8_t(0x10+i);
 assert(!rtw88_boot_parse_board(&boot,rom.data(),bus.family==RTW_WIFI_RTL8821CU?512:768));
 boot.firmware.powered=boot.firmware.firmware_running=1;bus.put(0x80,0xc078,2);boot.physical_valid=1;std::memset(boot.physical,255,sizeof(boot.physical));mac.device=&boot.firmware;mac.mac_ready=1;phy.boot=&boot;phy.mac=&mac;phy.tables_loaded=1;bus.reads=bus.writes=0;
}
#endif
