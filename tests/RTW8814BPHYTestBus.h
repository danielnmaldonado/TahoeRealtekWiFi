/* SPDX-License-Identifier: GPL-2.0-only
 * Isolated HAL boundary for the actual B14 PHY/RFK algorithms, no USB/hardware.
 */
#ifndef RTWB14_PHY_TEST_BUS_H
#define RTWB14_PHY_TEST_BUS_H
#include "rtw/RTW8814BPHY.h"
#include <array>
#include <vector>
#include <cassert>
#include <cerrno>
#include <cstring>
struct B14PHYBus {
 struct Write{uint16_t address;unsigned width;uint32_t value;};
 std::array<uint8_t,65536>reg{};std::array<std::array<uint32_t,256>,4>rf{};std::array<std::array<uint32_t,64>,2>syn{};
 std::vector<Write>writes;unsigned reads=0,operations=0,fail=0,after_fail=0;uint64_t delay=0;
 bool inject_thermal=true;std::array<unsigned,4>thermal{36,36,36,36};
 bool fail_after_effect=false,pending_error=false;
 bool adc_residual=false,dac_residual=false,unstable_samples=false,dac_phase=false;unsigned adc_reads=0;
 bool alive=true,timeout=false,dac_timeout=false,bad_samples=false,zero_psd=false;
 B14PHYBus(){put(0xfc,0x11,1);put(0x80,0xc078,2);for(auto&p:rf){p[0]=0x30000;p[0x56]=0x1234;p[0x3f]=0x1000;}}
 uint32_t get(unsigned a,unsigned w=4)const{uint32_t v=0;for(unsigned i=0;i<w;i++)v|=uint32_t(reg[a+i])<<(8*i);return v;}
 void put(unsigned a,uint32_t v,unsigned w=4){for(unsigned i=0;i<w;i++)reg[a+i]=uint8_t(v>>(8*i));}
 int operation(){if(!alive){++after_fail;return EIO;}if(++operations==fail){if(fail_after_effect){pending_error=true;return 0;}alive=false;return EIO;}return 0;}
 int complete(){if(pending_error){pending_error=false;alive=false;return EIO;}return 0;}
 int address(uint16_t a,unsigned w,unsigned&path,unsigned&index){if(w!=4||a%4)return 0;const unsigned bases[4]={0x3c00,0x4c00,0x5800,0x5c00};for(path=0;path<4;path++)if(a>=bases[path]&&a<bases[path]+1024){index=(a-bases[path])/4;return 1;}const unsigned synth[2]={0x3b00,0x4b00};for(path=0;path<2;path++)if(a>=synth[path]&&a<synth[path]+256){index=(a-synth[path])/4;return 2;}return 0;}
 int read(uint16_t a,unsigned w,uint32_t*out){int e=operation();if(e)return e;++reads;unsigned p,i;int kind=address(a,w,p,i);if(kind){*out=kind==1?rf[p][i]:syn[p][i];if(kind==1&&i==0x42&&inject_thermal)*out=(*out&~0xfc00U)|(thermal[p]<<10);return complete();}*out=get(a,w);
  if(a==0x2dbc){*out=bad_samples?0x100100:((adc_residual||dac_residual&&dac_phase||unstable_samples&&(++adc_reads&1))?0xa00a:0);return complete();}
  for(unsigned base:{0x2800U,0x4500U,0x5600U,0x5700U}){if(a==base+8||a==base+0x34)*out=dac_timeout?0:0x7fff80;if(a==base+0x10||a==base+0x3c)*out=dac_timeout?0:2;if(a==base+0x2c||a==base+0x58)*out=dac_timeout?0:0xff000;}
  if(a==0x1bfc){auto sel=get(0x1bd4);if(sel==0x00250001)*out=0;else if(sel==0x002e0001)*out=zero_psd?0:0x4000;else *out=0x10001000;}
  return complete();}
 int write(uint16_t a,unsigned w,uint32_t v){int e=operation();if(e)return e;writes.push_back({a,w,v});put(a,v,w);for(unsigned base:{0x1800U,0x4100U,0x5200U,0x5300U})if(a==base+0x10&&w==4){if(v==0x0005c304)dac_phase=true;else if(v==0x000dc304)dac_phase=false;}unsigned p,i;int kind=address(a,w,p,i);if(kind==2)syn[p][i]=v&0xfffff;
  const unsigned ports[4]={0x1808,0x4108,0x5208,0x5308};for(unsigned path=0;path<4;path++)if(a==ports[path]&&w==4)rf[path][(v>>20)&255]=v&0xfffff;
  if(a==0x1b10&&v==0)reg[0x2d9c]=0;if(a==0x1b00&&w==4&&(v&1)&&!timeout)reg[0x2d9c]=0x55;return complete();}
};
static B14PHYBus&phyBus(rtwb14_context*c){return*static_cast<B14PHYBus*>(c->bus.cookie);}
extern "C" int rtwb14_phy_claim(rtwb14_context*c,const void*p){if(c->private_owner)return EBUSY;c->private_owner=const_cast<void*>(p);return 0;}
extern "C" int rtwb14_phy_owned(rtwb14_context*c,const void*p){return c&&p&&c->private_owner==p&&phyBus(c).alive;}
extern "C" void rtwb14_phy_release(rtwb14_context*c,const void*p){if(c&&c->private_owner==p)c->private_owner=nullptr;}
extern "C" int rtwb14_live_firmware(rtwb14_context*c){auto&b=phyBus(c);if(!b.alive||c->last_error)return c->last_error?c->last_error:EIO;if(b.get(0xfc,1)!=0x11||b.get(0x80,2)!=0xc078){c->last_error=ENXIO;return ENXIO;}return 0;}
extern "C" int rtwb14_register_read(rtwb14_context*c,uint16_t a,unsigned w,uint32_t*out){int e=rtwb14_live_firmware(c);if(!e)e=phyBus(c).read(a,w,out);if(e)c->last_error=e;return e;}
extern "C" int rtwb14_register_write(rtwb14_context*c,uint16_t a,unsigned w,uint32_t v){int e=rtwb14_live_firmware(c);if(!e)e=phyBus(c).write(a,w,v);if(e)c->last_error=e;return e;}
extern "C" int rtwb14_delay_us(rtwb14_context*c,uint32_t n){int e=rtwb14_live_firmware(c);if(!e)phyBus(c).delay+=n;return e;}
struct B14PHYFixture {
 B14PHYBus bus;rtwb14_context hal{};std::array<uint8_t,1024>rom{};rtwb14_phy_board board{};rtwb14_phy_power power{};
 std::vector<uint64_t>scratch=std::vector<uint64_t>(RTWB14_PHY_SCRATCH_BYTES/8);rtwb14_phy*phy=nullptr;
 B14PHYFixture(unsigned rfe=0,unsigned flags=0){hal.bus.cookie=&bus;rom.fill(0);rom[0xc8]=0;for(unsigned i=0;i<4;i++)rom[0xd0+i]=0x24;board.rfe_type=(uint8_t)rfe;board.board_type=(uint8_t)flags;board.thermal_base=0x24;board.logical=rom.data();board.logical_bytes=rom.size();for(auto&v:power.cck)v=32;for(auto&v:power.ofdm)v=32;assert(!rtwb14_phy_open(scratch.data(),RTWB14_PHY_SCRATCH_BYTES,&hal,&board,&phy));}
};
#endif
