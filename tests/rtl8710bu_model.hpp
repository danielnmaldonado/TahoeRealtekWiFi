/* SPDX-License-Identifier: GPL-2.0-only */
/* Physical boundary model only: actual8710BU low-register aperture, indirect
 * SYSON/OTP,4-byte firmware pages and measured RF/IQK completion registers. */
#ifndef RTW_8710BU_MODEL_HPP
#define RTW_8710BU_MODEL_HPP
#include "rtw/RTL8710BU.h"
#include <array>
#include <vector>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cerrno>
#include "../kext/RTL8710BUFirmware.inc"
struct BUModel {
 std::array<uint8_t,65536>reg{};std::array<uint8_t,512>otp{};std::array<uint32_t,128>syson{};std::array<uint32_t,256>rf{};
 std::vector<uint8_t>firmware;unsigned calls=0,fail=0,writes=0,otp_reads=0,firmware_max_chunk=0,firmware_last_chunk=0;
 bool ambiguous=false,firmware_ready=true,power_ready=true,llt_ready=true,cam_ready=true,indirect_ready=true,iqk_fail=false,zero_coefficients=false,lck_ready=true;
 uint32_t read(unsigned a,unsigned n=4)const{uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(reg[a+i])<<(8*i);return v;}
 void set(unsigned a,uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)reg[a+i]=uint8_t(v>>(8*i));}
 BUModel(uint8_t vendor=RTW_R10BU_SMIC){otp.fill(255);syson[0x1f0/4]=uint32_t(vendor)<<6;syson[0x1f8/4]=1;syson[0xe0/4]=32;set(0x284,2U<<17);}
 void packed(const std::array<uint8_t,512>&rom,uint8_t package){
  std::vector<uint8_t>bytes;for(unsigned section=0;section<64;++section){unsigned mask=15;
   for(unsigned word=0;word<4;++word)if(rom[section*8+word*2]!=255||rom[section*8+word*2+1]!=255)mask&=~(1U<<word);
   if(mask==15)continue;if(section<16)bytes.push_back(uint8_t(section*16+mask));
   else{bytes.push_back(uint8_t(((section&7)<<5)|15));bytes.push_back(uint8_t(((section&0x78)<<1)|mask));}
   for(unsigned word=0;word<4;++word)if(!(mask&(1U<<word))){bytes.push_back(rom[section*8+word*2]);bytes.push_back(rom[section*8+word*2+1]);}}
  assert(bytes.size()<0xf8);otp.fill(255);memcpy(otp.data(),bytes.data(),bytes.size());otp[0xf8]=package;
 }
 static int wait(void*,uint32_t us){assert(us&&us<=100000);return 1;}
 static rtw_usb_error in(void*p,const rtw_usb_setup*s,uint8_t*b,uint16_t*n){
  auto&m=*static_cast<BUModel*>(p);*n=0;
  assert(s->bm_request_type==0xc0&&s->b_request==5&&!s->w_index&&s->w_value>=0x100);
  if(++m.calls==m.fail)return RTW_USB_IO_ERROR;
  if(s->w_value==0x8020&&m.power_ready)m.reg[0x8020]&=~3U;
  if(s->w_value==0x224&&m.llt_ready)m.set(0x224,m.read(0x224)&~(1U<<16));
  if(s->w_value==0x670&&m.cam_ready)m.set(0x670,m.read(0x670)&~(1U<<31));
  if(s->w_value==0x284)m.set(0x284,m.read(0x284)|(1U<<17));
  if(s->w_value==0xe94||s->w_value==0xea4)m.set(s->w_value,m.zero_coefficients?0:0x100U<<16);
  if(s->w_value==0xe9c)m.set(s->w_value,0);
  if(s->w_value==0xeac)m.set(s->w_value,m.iqk_fail?0x18000000:0);
  for(unsigned i=0;i<s->w_length;++i)b[i]=m.reg[s->w_value+i];*n=s->w_length;return RTW_USB_OK;
 }
 static rtw_usb_error out(void*p,const rtw_usb_setup*s,const uint8_t*b,uint16_t*n){
  auto&m=*static_cast<BUModel*>(p);*n=0;assert(s->bm_request_type==0x40&&s->b_request==5&&!s->w_index&&s->w_value>=0x100);
  bool failing=++m.calls==m.fail;if(failing&&!m.ambiguous)return RTW_USB_IO_ERROR;++m.writes;
  const auto oldHSSI=m.read(0x824);const auto oldCPU=m.reg[0x8093];
  for(unsigned i=0;i<s->w_length;++i)m.reg[s->w_value+i]=b[i];
  if(s->w_value==0x806c&&s->w_length==4){
   const uint32_t op=m.read(0x806c),address=m.read(0x80f8);assert(op==0x83000000||op==0x84000000||op==0x85000000);
   if(op==0x85000000){assert(address<512);m.set(0x809c,m.otp[address]);++m.otp_reads;}
   else {assert(address>=0x40000000&&address<0x40000200&&!(address&3));
    if(op==0x83000000)m.set(0x809c,m.syson[(address&511)/4]);else m.syson[(address&511)/4]=m.read(0x809c);}
   if(m.indirect_ready)m.set(0x806c,op&~0x80000000U);
  }
  if(s->w_value>=0x1000&&s->w_value<0x2000){
   assert(s->w_length<=4&&unsigned(s->w_value)+s->w_length<=0x2000&&m.reg[0x8090]&1&&m.reg[0x8091]&1);
   if(m.firmware_max_chunk<s->w_length)m.firmware_max_chunk=s->w_length;
   m.firmware_last_chunk=s->w_length;
   const size_t offset=(m.reg[0x8092]&7)*4096U+s->w_value-0x1000;assert(offset+s->w_length<=32768);
   if(m.firmware.size()<offset+s->w_length)m.firmware.resize(offset+s->w_length);memcpy(m.firmware.data()+offset,b,s->w_length);m.reg[0x8090]|=4;
  }
  if(s->w_value==0x8090&&(b[0]&1)&&(b[0]&4))m.reg[0x8090]&=~4U;
  if(s->w_value==0x1cc)m.reg[0x1cc]=0;
  if(s->w_value==0x8093&&!(oldCPU&1)&&(b[0]&1)&&(m.reg[0x8090]&2)&&m.firmware_ready&&!m.firmware.empty())m.reg[0x8090]=0xc6;
  if(s->w_value==0x840&&s->w_length==4){auto value=m.read(0x840);unsigned address=(value>>20)&255;value&=0xfffff;if(address==0x18&&m.lck_ready)value&=~0x8000U;m.rf[address]=value;}
  if(s->w_value==0x824&&s->w_length==4&&!(oldHSSI&0x80000000)&&(m.read(0x824)&0x80000000)){
   unsigned address=(m.read(0x824)>>23)&255;m.set(0x8a0,m.rf[address]);m.set(0x8b8,m.rf[address]);}
  *n=failing?0:s->w_length;return failing?RTW_USB_IO_ERROR:RTW_USB_OK;
 }
};
static std::array<uint8_t,512> bu_rom(){
 std::array<uint8_t,512>r;r.fill(255);r[0]=0x95;r[1]=0x81;r[0xc8]=0x20;r[0xc9]=0x20;r[0xca]=24;
 r[0x1c0]=0xda;r[0x1c1]=0x0b;r[0x1c2]=0x11;r[0x1c3]=0xb7;uint8_t mac[6]={2,0x11,0x22,0x33,0x44,0x55};memcpy(r.data()+0x11a,mac,6);
 for(unsigned i=0;i<11;++i)r[0x20+i]=24;r[0x2b]=0x20;return r;
}
struct BUFixture {
 BUModel m;std::vector<std::max_align_t>memory;rtw_r10bu*c=nullptr;uint8_t vendor;
 explicit BUFixture(uint8_t v=RTW_R10BU_SMIC,unsigned endpoints=3):m(v),memory((rtw_r10bu_size()+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)),vendor(v){
  m.packed(bu_rom(),v==RTW_R10BU_SMIC?0xfe:0xee);rtw_usb_rw_transport bus{&m,BUModel::in,BUModel::out};rtw_waiter waiter{&m,BUModel::wait};
  rtw_r10bu_usb usb{0x81,{3,1,2},uint8_t(endpoints),512};assert(!rtw_r10bu_init(memory.data(),memory.size()*sizeof(std::max_align_t),&bus,&waiter,&usb,&c));
 }
 void identify(){assert(!rtw_r10bu_identify(c));assert(m.otp_reads==512);}
 rtw_r10bu_status status(){rtw_r10bu_status s{};assert(!rtw_r10bu_status_read(c,&s));return s;}
 int boot(){return vendor==RTW_R10BU_SMIC?rtw_r10bu_boot(c,RTW8710BUSMICFirmware,sizeof(RTW8710BUSMICFirmware),vendor):rtw_r10bu_boot(c,RTW8710BUUMCFirmware,sizeof(RTW8710BUUMCFirmware),vendor);}
};
#endif
