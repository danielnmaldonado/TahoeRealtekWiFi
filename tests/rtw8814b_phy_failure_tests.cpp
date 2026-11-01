/* SPDX-License-Identifier: GPL-2.0-only
 * Actual B14 algorithms; fault injection stops at the isolated HAL boundary.
 */
#include "RTW8814BPHYTestBus.h"
#include <cstdio>
#include <set>

static unsigned channel=36;
static void prepare(B14PHYFixture &f,unsigned action){
 if(action==0)return;
 assert(!rtwb14_phy_load(f.phy));
 if(action==1)return;
 assert(!rtwb14_phy_tune(f.phy,(uint8_t)channel,&f.power));
 if(action>=3&&action<=5)assert(!rtwb14_phy_calibrate(f.phy));
 if(action==3||action==5)assert(!rtwb14_phy_resume(f.phy));
}
static int execute(B14PHYFixture &f,unsigned action){
 uint32_t value=0;
 switch(action){
 case 0:return rtwb14_phy_load(f.phy);
 case 1:return rtwb14_phy_tune(f.phy,(uint8_t)(channel<=11?11:channel<=48?48:149),&f.power);
 case 2:return rtwb14_phy_calibrate(f.phy);
 case 3:return rtwb14_phy_track(f.phy);
 case 4:return rtwb14_phy_resume(f.phy);
 case 5:return rtwb14_phy_stop(f.phy);
 case 6:return rtwb14_phy_rf_read(f.phy,3,0x18,0xfffff,&value);
 case 7:return rtwb14_phy_rf_write(f.phy,3,0x18,0xff0,0x55);
 case 8:return rtwb14_phy_synth_read(f.phy,1,0x18,0xfffff,&value);
 default:return rtwb14_phy_synth_write(f.phy,1,0x18,0xff0,0x55);
 }
}
static unsigned failures=0,healthCases=0;
int main(){
 for(unsigned bandChannel:{6U,36U,165U}){channel=bandChannel;
 for(unsigned action=0;action<10;action++){
  B14PHYFixture baseline;prepare(baseline,action);
  unsigned begin=baseline.bus.operations;
  assert(!execute(baseline,action));unsigned count=baseline.bus.operations-begin;
  assert(count&&count<=500000);
  std::set<unsigned>points{1,count};
  for(unsigned sample=0;sample<96;sample++)points.insert(1+(count-1)*sample/95);
  for(unsigned point:points)for(bool after:{false,true}){
   B14PHYFixture f;prepare(f,action);unsigned before=f.bus.operations;
   f.bus.fail=before+point;f.bus.fail_after_effect=after;
   assert(execute(f,action)==EIO);
   auto status=rtwb14_phy_get_status(f.phy);
   assert(status.error==EIO&&!status.calibrated&&!status.tx_enabled);
   assert(f.bus.operations==before+point&&f.bus.after_fail==0);
   auto stopped=f.bus.operations;
   for(unsigned next=0;next<10;next++)assert(execute(f,next)==EIO);
   assert(f.bus.operations==stopped&&f.bus.after_fail==0);
   ++failures;
  }
  for(unsigned fault=0;fault<6;fault++){
   B14PHYFixture f;prepare(f,action);auto before=f.bus.operations;
   if(fault<3)f.bus.put(0x80,std::array<unsigned,3>{0xc079,0xffff,0xc070}[fault],2);
   else if(fault==3)f.bus.put(0xfc,0xff,1);
   else if(fault==4)f.hal.last_error=EIO;
   else f.bus.alive=false;
   assert(execute(f,action));auto status=rtwb14_phy_get_status(f.phy);
   assert(status.error&&!status.calibrated&&!status.tx_enabled);
   assert(f.bus.operations==before&&f.bus.after_fail==0);
   ++healthCases;
  }
  printf("B14 channel%u action%u source-I/O%u sampled-points%zu before/after faults\n",channel,action,count,points.size());
 }
 }
 /* Saved DACK MAC/BB and RF values must survive an owned measurement failure. */
 constexpr unsigned saved[]={0x180c,0x410c,0x520c,0x530c,0x1c3c,0x1c24,0x1d70,0x9b4,0x1a00,0x1a14,0x1d58,0x1c38,0x1e24,0x1e28,0x1860,0x4160,0x5260,0x5360,0x1810,0x4110,0x5210,0x5310,0x18bc,0x41bc,0x52bc,0x53bc};
 for(unsigned fault=0;fault<2;fault++){
  B14PHYFixture f;prepare(f,2);
  unsigned index=0;for(auto address:saved)f.bus.put(address,0x11223300+index++);
  for(unsigned path=0;path<4;path++)f.bus.rf[path][0x8f]=0x12000+path;
  f.bus.dac_timeout=fault==0;f.bus.bad_samples=fault==1;
  assert(rtwb14_phy_calibrate(f.phy)==(fault==0?ETIMEDOUT:EIO));
  index=0;for(auto address:saved)assert(f.bus.get(address)==0x11223300+index++);
  for(unsigned path=0;path<4;path++)assert(f.bus.rf[path][0x8f]==0x12000+path);
  assert(f.bus.alive&&f.bus.reg[0x522]==255);
  auto before=f.bus.operations;assert(rtwb14_phy_resume(f.phy));assert(f.bus.operations==before);
 }
 /* A copied opaque context has no ownership, even with copied ready flags. */
 {B14PHYFixture f;prepare(f,4);std::vector<uint64_t>copy(RTWB14_PHY_SCRATCH_BYTES/8);std::memcpy(copy.data(),f.scratch.data(),rtwb14_phy_size());auto *original=f.phy;f.phy=reinterpret_cast<rtwb14_phy*>(copy.data());auto before=f.bus.operations;for(unsigned action=0;action<10;action++)assert(execute(f,action)==ENXIO);assert(rtwb14_phy_get_status(f.phy).error==ENXIO&&f.bus.operations==before);f.phy=original;assert(!rtwb14_phy_resume(f.phy));}
 printf("B14 %u before/after I/O faults, %u live-health cases, owned restoration and clone rejection passed\n",failures,healthCases);
}
