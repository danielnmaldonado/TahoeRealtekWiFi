/* SPDX-License-Identifier: GPL-2.0-only */
/* Hardware boundary: exact OTP gates, FU0x4000 firmware pages, 2 RF wires and
 * measured calibration registers. No driver algorithm is reimplemented. */
#include "rtw/RTL8192FU.h"
#include <array>
#include <vector>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <cstdio>
#include <cerrno>
#include "../kext/RTL8192FUFirmware.inc"
struct FUModel {
 std::array<uint8_t,65536>reg{};std::array<uint8_t,512>otp{};std::array<std::array<uint32_t,256>,2>rf{};
 std::vector<uint8_t>firmware;unsigned calls=0,fail=0,writes=0,otp_reads=0,firmware_max_chunk=0;bool pendingB=false;unsigned addressB=0;bool ambiguous=false,firmware_ready=true,checksum_ready=true,power_ready=true,llt_ready=true,iqk_ready=true,iqk_fail=false,zero_coefficients=false,lck_ready=true;
 uint32_t read(unsigned a,unsigned n=4)const{uint32_t v=0;for(unsigned i=0;i<n;++i)v|=uint32_t(reg[a+i])<<(8*i);return v;}
 void set(unsigned a,uint32_t v,unsigned n=4){for(unsigned i=0;i<n;++i)reg[a+i]=uint8_t(v>>(8*i));}
 FUModel(){otp.fill(255);set(4,1U<<17);set(0xF0,1U<<12);set(0x284,2U<<17);set(0x5f8,~uint32_t(0));set(0xe90,1);set(0xe98,1);set(0xeb0,1);set(0xeb8,1);}
 void packed(const std::array<uint8_t,512>&rom){std::vector<uint8_t> bytes;for(unsigned section=0;section<64;++section){unsigned mask=15;for(unsigned word=0;word<4;++word)if(rom[section*8+word*2]!=255||rom[section*8+word*2+1]!=255)mask&=~(1U<<word);if(mask==15)continue;if(section<16)bytes.push_back(uint8_t(section*16+mask));else{bytes.push_back(uint8_t(((section&7)<<5)|15));bytes.push_back(uint8_t(((section&0x78)<<1)|mask));}for(unsigned word=0;word<4;++word)if(!(mask&(1U<<word))){bytes.push_back(rom[section*8+word*2]);bytes.push_back(rom[section*8+word*2+1]);}}assert(bytes.size()<490);otp.fill(255);memcpy(otp.data(),bytes.data(),bytes.size());}
 static int wait(void*,uint32_t){return 1;}
 static rtw_usb_error in(void*p,const rtw_usb_setup*s,uint8_t*b,uint16_t*n){
  auto&m=*static_cast<FUModel*>(p);*n=0;assert(s->bm_request_type==0xc0&&s->b_request==5&&!s->w_index);if(++m.calls==m.fail)return RTW_USB_IO_ERROR;
  if(s->w_value==4&&s->w_length==4&&m.power_ready)m.set(4,(m.read(4)|(1U<<17))&~0x300U);
  if(s->w_value==0x224&&m.llt_ready)m.set(0x224,m.read(0x224)&~(1U<<16));
  if(s->w_value==0x284)m.set(0x284,m.read(0x284)|(1U<<17));
  if(s->w_value==0xe98||s->w_value==0xea8||s->w_value==0xeb8||s->w_value==0xec8)m.set(s->w_value,m.iqk_ready?1:0);
  if(s->w_value==0xe94||s->w_value==0xea4||s->w_value==0xeb4||s->w_value==0xec4)m.set(s->w_value,m.zero_coefficients?0:0x100U<<16);
  if(s->w_value==0xe9c||s->w_value==0xeac||s->w_value==0xebc||s->w_value==0xecc)m.set(s->w_value,m.iqk_fail?0x38000000:0);
  for(unsigned i=0;i<s->w_length;++i)b[i]=m.reg[s->w_value+i];*n=s->w_length;return RTW_USB_OK;
 }
 static rtw_usb_error out(void*p,const rtw_usb_setup*s,const uint8_t*b,uint16_t*n){
  auto&m=*static_cast<FUModel*>(p);*n=0;assert(s->bm_request_type==0x40&&s->b_request==5&&!s->w_index);bool failing=++m.calls==m.fail;if(failing&&!m.ambiguous)return RTW_USB_IO_ERROR;++m.writes;
  auto oldHSSI=m.read(0x824);auto oldCPU=m.read(2,2);auto oldMCU=m.reg[0x80];for(unsigned i=0;i<s->w_length;++i)m.reg[s->w_value+i]=b[i];
  if(s->w_value>=0x4000&&s->w_value<0x5000){assert(s->w_length<=254&&unsigned(s->w_value)+s->w_length<=0x5000);if(m.firmware_max_chunk<s->w_length)m.firmware_max_chunk=s->w_length;size_t offset=(m.reg[0x82]&7)*4096U+s->w_value-0x4000;assert(offset+s->w_length<=32768);if(m.firmware.size()<offset+s->w_length)m.firmware.resize(offset+s->w_length);memcpy(m.firmware.data()+offset,b,s->w_length);if(m.checksum_ready)m.reg[0x80]|=4;}
  if(s->w_value==0x80&&(b[0]&1)&&(b[0]&4))m.reg[0x80]&=~4U;
  if(s->w_value==0x33&&!(b[0]&0x80)){assert(m.reg[0xcf]==0x69);unsigned offset=m.reg[0x31]|((m.reg[0x32]&3U)<<8);assert(offset<512);m.reg[0x30]=m.otp[offset];m.reg[0x33]|=0x80;++m.otp_reads;}
  if(s->w_value==0x1cc)m.reg[0x1cc]=0;
  if(s->w_value==2&&s->w_length==2&&!(oldCPU&0x400)&&(m.read(2,2)&0x400)&&(oldMCU&2)&&m.firmware_ready&&!m.firmware.empty()){m.reg[0x80]=0xc6;m.reg[0x1a0]=0x19;m.reg[0x1a8]=6;}
  if((s->w_value==0x840||s->w_value==0x844)&&s->w_length==4){unsigned path=s->w_value==0x844;auto value=m.read(s->w_value);unsigned address=(value>>20)&255;value&=0xfffff;if(address==0x18&&m.lck_ready)value&=~0x8000U;m.rf[path][address]=value;}
  if(s->w_value==0x82c&&s->w_length==4){m.pendingB=true;m.addressB=(m.read(0x82c)>>23)&255;}
  if(s->w_value==0x824&&s->w_length==4&&!(oldHSSI&0x80000000)&&(m.read(0x824)&0x80000000)){unsigned path=m.pendingB?1:0,address=m.pendingB?m.addressB:(m.read(0x824)>>23)&255;m.set(0x8a0+4*path,m.rf[path][address]);m.set(0x8b8+4*path,m.rf[path][address]);m.pendingB=false;}
  *n=failing?0:s->w_length;return failing?RTW_USB_IO_ERROR:RTW_USB_OK;
 }
};
static std::array<uint8_t,512> fu_rom(){std::array<uint8_t,512>r;r.fill(255);r[0]=0x29;r[1]=0x81;r[0xb8]=0x20;r[0xb9]=0x20;r[0xba]=24;r[0xca]=5;r[0x100]=0xda;r[0x101]=0x0b;r[0x102]=0x92;r[0x103]=0xf1;uint8_t mac[6]={2,0x11,0x22,0x33,0x44,0x55};memcpy(r.data()+0x107,mac,6);for(unsigned offset:{0x10U,0x3aU}){for(unsigned i=0;i<11;++i)r[offset+i]=24;r[offset+11]=0x20;for(unsigned i=12;i<18;++i)r[offset+i]=0;}return r;}
struct FUFixture{FUModel m;std::vector<std::max_align_t>memory;rtw_r92fu*c=nullptr;FUFixture(unsigned endpoints=3):memory((rtw_r92fu_size()+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)){m.packed(fu_rom());rtw_usb_rw_transport bus{&m,FUModel::in,FUModel::out};rtw_waiter waiter{&m,FUModel::wait};rtw_r92fu_usb usb{0x81,{3,1,2},uint8_t(endpoints),512};assert(!rtw_r92fu_init(memory.data(),memory.size()*sizeof(std::max_align_t),&bus,&waiter,&usb,&c));}
 void identify(){assert(!rtw_r92fu_identify(c));assert(m.reg[0xcf]==0&&m.otp_reads==512);}
 rtw_r92fu_status status(){rtw_r92fu_status s{};assert(!rtw_r92fu_status_read(c,&s));return s;}
 int boot(){return rtw_r92fu_boot(c,RTW8192FUFirmware,sizeof(RTW8192FUFirmware));}
};
