/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88PHY.h"
#include <cassert>
#include <cerrno>
#include <array>
#include <vector>
#include <cstdio>
#include <cstring>
struct Trace {uint64_t hash=UINT64_C(1469598103934665603);unsigned count=0;std::vector<uint32_t>values;
 static int emit(void*p,rtw88_phy_table_kind k,uint8_t path,uint32_t address,uint32_t value){auto&t=*static_cast<Trace*>(p);for(uint32_t v:{uint32_t(k),uint32_t(path),address,value})for(unsigned j=0;j<4;j++){t.hash^=uint8_t(v>>(j*8));t.hash*=UINT64_C(1099511628211);}++t.count;t.values.push_back(value);return 0;}}
;
struct Bus {
 std::array<uint8_t,65536>reg{};struct Write {uint16_t address;unsigned width;uint32_t value;};std::vector<Write>writes;
 unsigned reads=0,delays=0;int fail_write=0,fail_read=0;uint64_t delay_us=0;
 static int read(void*p,uint16_t a,unsigned w,uint32_t*v){auto&b=*static_cast<Bus*>(p);++b.reads;*v=0;if(b.fail_read==int(b.reads))return EIO;for(unsigned i=0;i<w;i++)*v|=uint32_t(b.reg[a+i])<<(i*8);return 0;}
 static int write(void*p,uint16_t a,unsigned w,uint32_t v){auto&b=*static_cast<Bus*>(p);b.writes.push_back({a,w,v});if(b.fail_write==int(b.writes.size()))return EIO;for(unsigned i=0;i<w;i++)b.reg[a+i]=uint8_t(v>>(i*8));return 0;}
 static void delay(void*p,uint32_t u){auto&b=*static_cast<Bus*>(p);++b.delays;b.delay_us+=u;assert(u<=50000);}
};
static const rtw_usb_wifi_profile*profile(rtw_wifi_family f){for(size_t i=0;i<rtw_usb_wifi_profile_count();i++){auto*p=rtw_usb_wifi_profile_at(i);if(p->family==f&&p->primary_verified)return p;}return nullptr;}
static void phase_fixture(Bus&bus,rtw88_boot_context&boot,rtw88_mac_context&mac,rtw_wifi_family family){
 // These are isolated PHY tests: identity/ROM parsing runs real code; power,
 // firmware and MAC stages are mock prerequisites tested in their own suites.
 rtw88_fw_bus io{&bus,Bus::read,Bus::write,nullptr,Bus::delay,nullptr,nullptr};assert(!rtw88_boot_init(&boot,profile(family),&io));
 if(family!=RTW_WIFI_RTL8821CU)bus.reg[0xf3]=8;assert(!rtw88_boot_identify(&boot));
 std::array<uint8_t,768>rom;rom.fill(255);rom[0xca]=family==RTW_WIFI_RTL8822BU?3:2;rom[0xb9]=0x2a;
 unsigned address=family==RTW_WIFI_RTL8822CU?0x157:0x107;rom[address]=2;for(unsigned i=1;i<6;i++)rom[address+i]=uint8_t(0x10+i);
 assert(!rtw88_boot_parse_board(&boot,rom.data(),family==RTW_WIFI_RTL8821CU?512:768));boot.firmware.powered=boot.firmware.firmware_running=1;
 mac.device=&boot.firmware;mac.mac_ready=1;bus.reads=0;
}
int main(){
 const uint32_t branches[]={0x80000202,0,0x40000000,0,0x900,0x111,0x90000203,0,0x40000000,0,0x900,0x222,0xa0000000,0,0x900,0x333,0xb0000000,0,0x904,0x444};
 for(uint8_t rfe:{2,3,4}){Trace t;assert(!rtw88_phy_walk_table(0,0,rfe,RTW88_PHY_BB32,0,branches,sizeof(branches)/4,Trace::emit,&t));assert((t.values==std::vector<uint32_t>{rfe==2?0x111U:rfe==3?0x222U:0x333U,0x444}));}
 Trace empty;assert(rtw88_phy_walk_table(0,0,2,RTW88_PHY_BB32,0,branches,sizeof(branches)/4-3,Trace::emit,&empty)==EINVAL&&!empty.count);
 const uint32_t invalid_agc[]={0xfa,123};Trace invalid;
 assert(rtw88_phy_walk_table(0,0,2,RTW88_PHY_AGC32,0,invalid_agc,2,Trace::emit,&invalid)==EINVAL&&!invalid.count);
 struct Golden {rtw_wifi_family family;uint8_t cut,package,rfe;uint64_t hash;unsigned count;};
 const Golden goldens[]={
#include "RTW88PHYGoldens.inc"
 };
 for(const auto&gold:goldens){Trace t;assert(!rtw88_phy_visit_boot_tables(gold.family,gold.cut,gold.package,gold.rfe,Trace::emit,&t));assert(t.count==gold.count&&t.hash==gold.hash);}
 unsigned failures=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  Bus bus;rtw88_boot_context boot{};rtw88_mac_context mac{};phase_fixture(bus,boot,mac,family);rtw88_phy_context phy{&boot,&mac,0,{0,0,0},nullptr};
  assert(!rtw88_phy_write_rf(&phy,0,0x18,0xfffff,0x12345));auto actual=bus.writes.back();
  assert(actual.address==(family==RTW_WIFI_RTL8822CU?0x3c60:0xc90));assert(actual.width==4&&actual.value==(family==RTW_WIFI_RTL8822CU?0x12345:0x01812345));
  assert(bus.delay_us==(family==RTW_WIFI_RTL8822CU?1:13));unsigned base=family==RTW_WIFI_RTL8822CU?0x3c60:0x2860;
  for(unsigned i=0;i<4;i++)bus.reg[base+i]=uint8_t(0xfff54321U>>(i*8));uint32_t value=0;assert(!rtw88_phy_read_rf(&phy,0,0x18,0xff00,&value)&&value==0x43);
  assert(!rtw88_phy_write_rf(&phy,0,0x18,0xf00,0xa));assert(bus.writes.back().value==(family==RTW_WIFI_RTL8822CU?0xfff54a21U:0x01854a21U));
  size_t count=bus.writes.size();unsigned reads=bus.reads;assert(rtw88_phy_write_rf(&phy,0,0x18,0,0)==EINVAL);assert(rtw88_phy_write_rf(&phy,0,0x18,0xf00,16)==EINVAL);assert(bus.writes.size()==count&&bus.reads==reads);
  if(family==RTW_WIFI_RTL8821CU)assert(rtw88_phy_write_rf(&phy,1,0x18,0xfffff,1)==EINVAL);
  if(family==RTW_WIFI_RTL8822CU){assert(!rtw88_phy_write_rf(&phy,1,0,0xfffff,0x321));assert(bus.writes.back().address==0x4108&&bus.writes.back().value==0x321);}
  bus.writes.clear();bus.reads=bus.delays=0;bus.delay_us=0;assert(!rtw88_phy_load_boot_tables(&phy,&boot,&mac));assert(phy.tables_loaded&&bus.writes.size()>500);
  assert((bus.reg[2]&3)==3&&(bus.reg[0x1f]&7)==7);unsigned total=unsigned(bus.writes.size());
  for(unsigned position:{1U,2U,3U,10U,100U,500U,total}){
   Bus failed_bus;rtw88_boot_context failed_boot{};rtw88_mac_context failed_mac{};phase_fixture(failed_bus,failed_boot,failed_mac,family);rtw88_phy_context failed{};failed_bus.fail_write=int(position);
   assert(rtw88_phy_load_boot_tables(&failed,&failed_boot,&failed_mac)==EIO);assert(!failed.tables_loaded&&(failed_bus.reg[2]&3)==0&&(failed_bus.reg[0x1f]&7)==0);++failures;
  }
  mac.mac_ready=0;count=bus.writes.size();assert(rtw88_phy_load_boot_tables(&phy,&boot,&mac)==ENXIO&&!phy.tables_loaded);assert(bus.writes.size()==count);
 }
 std::printf("rtw88 PHY branches,%zu Linux-parser traces,RF wire goldens,actual boot tables and%u I/O failures passed\n",sizeof(goldens)/sizeof(goldens[0]),failures);
}
