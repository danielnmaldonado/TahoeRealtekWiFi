/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtl8710bu_model.hpp"
#include <array>
#include <cassert>
#include <cstdio>
static void runtime_and_inputs(){
 for(uint8_t vendor:{uint8_t(0),uint8_t(2)})for(uint8_t package:{uint8_t(0xee),uint8_t(0xfe)})for(unsigned endpoints:{1U,2U,3U}){
  BUFixture f(vendor,endpoints);f.m.packed(bu_rom(),package);f.identify();assert(!f.boot());
  rtw_r10bu_board b{};assert(!rtw_r10bu_board_info(f.c,&b)&&b.vendor==vendor&&b.package==package&&b.rom_revision==1);
  assert(f.m.read(0xa0c)==(package==0xee?0x2e20100fU:0x2a20100fU));
  const auto*image=vendor==0?RTW8710BUSMICFirmware:RTW8710BUUMCFirmware;
  const size_t bytes=vendor==0?sizeof(RTW8710BUSMICFirmware):sizeof(RTW8710BUUMCFirmware);
  assert(f.m.firmware.size()==bytes-32&&!memcmp(f.m.firmware.data(),image+32,bytes-32));
  assert(f.m.firmware_max_chunk==4&&f.m.firmware_last_chunk==2);
  assert(!f.m.reg[0x680]&&!f.m.reg[0x423]&&!(f.m.read(0x670)&0x80000000U));
  for(auto op:{rtw_r10bu_operating{6,6,20,0},rtw_r10bu_operating{6,6,20,1},rtw_r10bu_operating{6,8,40,1},rtw_r10bu_operating{6,4,40,1}}){
   assert(!rtw_r10bu_set_operating(f.c,&op));assert((f.m.rf[0x18]&255)==op.center&&!f.status().tx_enabled);
   assert((f.m.rf[0x18]&0xc00)==(op.width_mhz==40?0x400U:0xc00U));assert(!rtw_r10bu_resume(f.c)&&f.status().tx_enabled);}
  uint8_t bssid[6]={2,1,2,3,4,5};assert(!rtw_r10bu_station(f.c,1,bssid,42,100,0x15,1,1));
  assert(f.m.read(0x6a2,2)==0x100&&f.m.read(0x6a4,2)==0xffff&&f.m.read(0x51b,1)==9);
  assert(!rtw_r10bu_tick(f.c));f.m.rf[0x42]=32U<<10;assert(!rtw_r10bu_tick(f.c)&&f.status().thermal==32);
  assert(!rtw_r10bu_stop(f.c)&&!f.status().powered&&!f.status().firmware&&!f.status().tx_enabled&&f.m.reg[0x522]==255);
  assert(f.m.syson[0x138/4]&32U);
 }
 for(unsigned mode=0;mode<6;++mode){BUFixture f;
  if(mode==0)f.m.syson[0x1f0/4]|=1U<<16;if(mode==1)f.m.syson[0x1f0/4]=1U<<6;
  if(mode==2)f.m.otp[0xf8]=0x42;if(mode==3)f.m.indirect_ready=false;
  if(mode==4){auto r=bu_rom();r[0]=0x29;f.m.packed(r,0xfe);}if(mode==5){auto r=bu_rom();r[0x11a]=1;f.m.packed(r,0xfe);}
  assert(rtw_r10bu_identify(f.c)&&!f.status().identified&&!f.status().tx_enabled);
 }
 for(unsigned mode=0;mode<6;++mode){BUFixture f;f.identify();
  if(mode==0)f.m.zero_coefficients=true;if(mode==1)f.m.iqk_fail=true;if(mode==2)f.m.lck_ready=false;
  if(mode==3)f.m.firmware_ready=false;if(mode==4)f.m.llt_ready=false;if(mode==5)f.m.cam_ready=false;
  assert(f.boot()&&!f.status().firmware&&!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==255);
 }
 for(unsigned legacy=1;legacy<4096;++legacy)for(unsigned width:{20U,40U}){
  uint8_t mcs[2]={255,255};rtw_r10bu_rate r{};assert(rtw_r10bu_rate_build(uint16_t(legacy),mcs,1,width,0,0,&r));
  assert(r.mask==(legacy|0xff000U)&&r.initial==19&&!r.sgi&&r.width_mhz==width);
  uint8_t command[8]{};assert(rtw_r10bu_rate_command(&r,1,command)&&command[0]==0x40);
 }
}
static void failures(){
 unsigned identified=0,boot=0,tune=0,total=0;
 {BUFixture f;f.identify();identified=f.m.calls;auto before=f.m.calls;assert(!f.boot());boot=f.m.calls-before;
 before=f.m.calls;rtw_r10bu_operating op{6,8,40,1};assert(!rtw_r10bu_set_operating(f.c,&op));tune=f.m.calls-before;}
 for(bool after:{false,true}){
  for(unsigned i=1;i<=identified;++i){BUFixture f;f.m.fail=i;f.m.ambiguous=after;
   assert(rtw_r10bu_identify(f.c)==EIO&&!f.status().identified&&!f.status().tx_enabled);++total;}
  for(unsigned i=1;i<=boot;++i){BUFixture f;f.identify();f.m.fail=f.m.calls+i;f.m.ambiguous=after;
   int e=f.boot();if(e!=EIO){printf("BU boot fault%u/%u after=%d got%d\n",i,boot,after,e);abort();}
   assert(!f.status().firmware&&!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==255);++total;}
  for(unsigned i=1;i<=tune;++i){BUFixture f;f.identify();assert(!f.boot());f.m.fail=f.m.calls+i;f.m.ambiguous=after;
   rtw_r10bu_operating op{6,8,40,1};assert(rtw_r10bu_set_operating(f.c,&op)==EIO&&!f.status().calibrated&&!f.status().tx_enabled&&f.m.reg[0x522]==255);++total;}
 }
 printf("8710BU %u I/O faults before/after effects: %u identify,%u firmware+MAC+RFK,%u HT40/RFK\n",total,identified,boot,tune);
}
int main(){std::array<uint8_t,512>rom{};rom[0]=0x95;rom[1]=0x81;rom[0x11a]=2;
 {
  rtw_r10bu_tx_options o{};o.payload_bytes=64;o.rf_chains=1;o.bandwidth_mhz=20;o.sequence=0x123;
  uint8_t descriptor[40]{};assert(rtw_r10bu_tx_descriptor(&o,descriptor));
  assert(!(descriptor[33]&0x80)&&descriptor[37]==0x30&&descriptor[38]==0x12);
  o.rate=1;o.short_preamble=1;
  assert(rtw_r10bu_tx_descriptor(&o,descriptor)&&rtw_r10bu_tx_descriptor_valid(descriptor,1));
 }
 rom[0xc8]=0;rom[0xc9]=0x20;rom[0xca]=0x18;for(unsigned i=0;i<11;++i)rom[0x20+i]=24;
 rtw_r10bu_board b{};assert(!rtw_r10bu_decode_rom(rom.data(),rom.size(),&b));assert(b.mac[0]==2&&b.crystal==0x20);
 rom[0]=0x29;assert(rtw_r10bu_decode_rom(rom.data(),rom.size(),&b));
 {
  BUFixture f;f.identify();const auto calls=f.m.calls;
  assert(rtw_r10bu_boot(f.c,RTW8710BUUMCFirmware,sizeof(RTW8710BUUMCFirmware),RTW_R10BU_SMIC)==EINVAL);
  assert(f.m.calls==calls&&!f.status().powered);
 }
 {BUFixture f;f.identify();f.m.cam_ready=false;assert(f.boot()==ETIMEDOUT&&!f.status().firmware&&!f.status().tx_enabled);}
 {
  BUFixture f;auto r=bu_rom();r[0xc9]=0;f.m.packed(r,0xfe);f.m.syson[0x60/4]=0x3ffc0000;
  f.identify();assert(!f.boot());assert(!(f.m.syson[0x60/4]&0x3ffc0000));
 }

 for(uint8_t vendor:{uint8_t(0),uint8_t(2)}){BUFixture f(vendor);f.identify();int e=f.boot();if(e){printf("8710BU boot vendor%u failed:%d calls%u\n",vendor,e,f.m.calls);return 1;}
 assert(f.status().firmware&&f.status().calibrated&&!f.status().tx_enabled&&f.status().tx_iqk_mask==1&&f.status().rx_iqk_mask==1);
 assert(f.m.firmware_max_chunk==4&&f.m.reg[0x522]==255&&f.status().primary==1);}
 runtime_and_inputs();failures();puts("RTL8710BU own indirect ROM,SMIC/UMC firmware,MAC/PHY/HT20-40/RFK/runtime verified");}
