/* SPDX-License-Identifier: GPL-2.0-only */
/* The native HAL above is complete; only USB callbacks and elapsed time are
 * modeled. Literal shutdown fixtures are from the pinned Linux rtl8xxxu flow. */
#include <array>
#include <vector>
#include <cstdlib>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while (0)
struct LegacyBus {
 struct Access { bool write; uint16_t address; unsigned width; uint32_t value; };
 std::array<uint8_t,65536> reg{};
 std::vector<Access> trace;
 unsigned calls=0,failAt=0,delays=0,rxPolls=0,txPolls=0,macPolls=0,cpuPolls=0;
 int error=0;
 bool rxStuck=false,txStuck=false,macStuck=false,cpuStuck=false,afterEffect=false;
 uint32_t get(unsigned a,unsigned w) const { uint32_t v=0; for(unsigned i=0;i<w;i++)v|=uint32_t(reg[a+i])<<(8*i);return v; }
 void put(unsigned a,unsigned w,uint32_t v) { for(unsigned i=0;i<w;i++)reg[a+i]=uint8_t(v>>(8*i)); }
 uint32_t read(uint16_t a,unsigned w) {
  ++calls; uint32_t v=get(a,w);
  if(a==0x284&&w==4){++rxPolls;if(!rxStuck)v|=0x20000;}
  if(a==0x5f8&&w==4){++txPolls;v=txStuck?1:0;}
  if(a==5&&w==1&&(v&2)){++macPolls;if(!macStuck)v&=~2U;}
  if(a==2&&w==2&&(get(0x1cf,1)&0x20)&&(v&0x400)){++cpuPolls;if(!cpuStuck)v&=~0x400U;}
  trace.push_back({false,a,w,v});if(calls==failAt){error=EIO;return 0xffffffffU;}return v;
 }
 void write(uint16_t a,unsigned w,uint32_t v) {
  ++calls;trace.push_back({true,a,w,v});
  if(calls==failAt){error=EIO;if(!afterEffect)return;}put(a,w,v);
 }
 static LegacyBus &bus(void *p){return *static_cast<LegacyBus*>(p);}
 static uint8_t r1(void*p,uint16_t a){return uint8_t(bus(p).read(a,1));}
 static uint16_t r2(void*p,uint16_t a){return uint16_t(bus(p).read(a,2));}
 static uint32_t r4(void*p,uint16_t a){return bus(p).read(a,4);}
 static void w1(void*p,uint16_t a,uint8_t v){bus(p).write(a,1,v);}
 static void w2(void*p,uint16_t a,uint16_t v){bus(p).write(a,2,v);}
 static void w4(void*p,uint16_t a,uint32_t v){bus(p).write(a,4,v);}
 static int io(void*p){return bus(p).error;}
 static void wait(void*p,uint32_t){++bus(p).delays;}
};
struct LegacyFixture {
 LegacyBus b; rtwn_softc c; device d;
 explicit LegacyFixture(uint32_t chip) {
  c.chip=RTWN_CHIP_USB|chip;c.sc_pdev=&d;c.sc_ops.cookie=&b;
  c.sc_ops.read_1=LegacyBus::r1;c.sc_ops.read_2=LegacyBus::r2;c.sc_ops.read_4=LegacyBus::r4;
  c.sc_ops.write_1=LegacyBus::w1;c.sc_ops.write_2=LegacyBus::w2;c.sc_ops.write_4=LegacyBus::w4;
  c.sc_ops.io_error=LegacyBus::io;c.sc_ops.delay_us=LegacyBus::wait;
  b.put(2,2,0xffff);b.put(0x80,1,0x82);b.put(0x4ec,1,0xff);
  b.put(0x8a0,4,0xabcde);b.put(0xf0,4,0x00081000);
 }
 size_t write(uint16_t a,unsigned w,uint32_t v,size_t first=0) const {
  for(size_t i=first;i<b.trace.size();++i){const auto&t=b.trace[i];if(t.write&&t.address==a&&t.width==w&&t.value==v)return i;}
  return b.trace.size();
 }
 void ordered(std::initializer_list<LegacyBus::Access> writes) const {
  size_t p=0;for(auto t:writes){p=write(t.address,t.width,t.value,p);CHECK(p<b.trace.size());++p;}
 }
};
static const uint32_t chips[]={RTWN_CHIP_92C,RTWN_CHIP_88C,RTWN_CHIP_88E,RTWN_CHIP_92E};

/* Missing full native shutdown, wrong family dispatch, or sticky bootstrap
 * error blocking cleanup all fail this literal sequence and final state. */
static void successAndOrder() {
 for(uint32_t chip:chips){LegacyFixture f(chip);f.c.core_error=ETIMEDOUT;
  CHECK(!rtwLegacyPowerOffChecked(&f.c));CHECK(f.c.core_error==ETIMEDOUT);CHECK(!f.b.error);
  if(chip&(RTWN_CHIP_92C|RTWN_CHIP_88C)){
   f.ordered({{true,0x522,1,0xff},{true,0x840,4,0xabc00},
    {true,2,1,0x16},{true,2,1,0x14},{true,0x80,1,0},
    {true,0x130,1,0x20},{true,0x1cf,1,0x20},{true,3,1,0x54},
    {true,0x11,1,0x2b},{true,4,2,0x4802},{true,0x1c,1,0x0e}});
   CHECK(f.b.get(0x600,1)&0x40);CHECK(f.b.get(0x87c,4)&0x80000000U);
  } else {
   f.ordered({{true,0x522,1,0xff},{true,0x214,2,0},{true,0x200,4,0x80000000U},
    {true,0x4ec,1,0xfd},{true,0x1f,1,0},{true,0x522,1,chip==RTWN_CHIP_88E?0x7fU:0xffU},
    {true,0x1cf,1,0x20},{true,0x80,1,0}});
   CHECK(!(f.b.get(2,1)&1));
   if(chip==RTWN_CHIP_88E){
    CHECK(f.b.get(0x23,1)&0x10);CHECK(f.b.get(0xfe10,1)&0x10);
    f.ordered({{true,0x64,4,0x00080808},{true,0x1c,1,0},{true,0x40,4,0}});
   } else {
    CHECK(!(f.b.get(2,1)&2));CHECK(f.b.get(5,1)==8);CHECK(f.b.macPolls>0);
    CHECK(f.b.get(2,2)&0x400); // Linux reset_8051 re-enables the CPU in EMU.
   }
  }
 }
}
/* A failed/short control transfer must not become an all-ones RMW write or
 * a successful cleanup, even if that transfer reached the device. */
static void everyIoFaultFailsClosed() {
 for(uint32_t chip:chips)for(unsigned variant=0;variant<3;++variant){
  LegacyFixture clean(chip);clean.b.cpuStuck=variant==1;
  if(variant==2)clean.b.put(0x80,1,0);
  CHECK(!rtwLegacyPowerOffChecked(&clean.c));
  for(bool ambiguous:{false,true})for(unsigned fault=1;fault<=clean.b.calls;++fault){
   LegacyFixture f(chip);f.b.cpuStuck=variant==1;if(variant==2)f.b.put(0x80,1,0);
   f.c.core_error=ETIMEDOUT;f.b.failAt=fault;f.b.afterEffect=ambiguous;
   CHECK(rtwLegacyPowerOffChecked(&f.c)==EIO);CHECK(f.c.core_error==ETIMEDOUT);
   CHECK(f.b.calls==fault);CHECK(f.b.error==EIO);
  }
 }
}
static void boardAndFirmwareBranches() {
 for(uint32_t chip:chips){LegacyFixture f(chip);f.b.put(0x80,1,0);
  f.c.sc_flags=0x80;f.c.crystal_cap=21;
  CHECK(!rtwLegacyPowerOffChecked(&f.c));CHECK(f.c.chip==(RTWN_CHIP_USB|chip));
  CHECK(f.c.sc_flags==0x80&&f.c.crystal_cap==21&&!f.c.core_error);
  CHECK(f.write(0x1cf,1,0x20)==f.b.trace.size());
 }
 for(uint32_t syscfg:{0U,0x00080000U,0x00081000U,0x00082000U}){
  LegacyFixture f(RTWN_CHIP_88C);f.b.put(0xf0,4,syscfg);
  f.c.board_type=R92C_BOARD_TYPE_HIGHPA;
  f.b.put(0x820,4,0x100);f.b.put(0x8b8,4,0x54321);
  CHECK(!rtwLegacyPowerOffChecked(&f.c));
  f.ordered({{true,0x87c,4,2},{true,0x840,4,0x54300},
   {true,0x87c,4,0x80000002U},{true,0x11,1,syscfg==0x00081000U?0x2bU:0x23U}});
 }
}
static void boundsAndFallback() {
 for(uint32_t chip:{RTWN_CHIP_88E,RTWN_CHIP_92E})for(unsigned mode=0;mode<4;++mode){
  LegacyFixture f(chip);f.c.core_error=EBUSY;
  f.b.rxStuck=mode==0;f.b.txStuck=mode==1;f.b.cpuStuck=mode==2;f.b.macStuck=mode==3;
  int result=rtwLegacyPowerOffChecked(&f.c);
  CHECK(result==((mode==2||(mode==3&&chip==RTWN_CHIP_88E))?0:ETIMEDOUT));
  CHECK(f.c.core_error==EBUSY);CHECK(f.b.calls<900);CHECK(!f.b.error);
  CHECK(f.write(0x80,1,0)<f.b.trace.size()); // continue independent cleanup after poll timeout
  if(mode==2)CHECK(f.b.cpuPolls==101); // 100 polls, then the forced-reset RMW read
  if(mode==0)CHECK(f.b.rxPolls==101); // one RMW read plus exactly 100 poll attempts
  if(mode==1)CHECK(f.b.txPolls==100);
  if(mode==3&&chip==RTWN_CHIP_92E)CHECK(f.b.macPolls==501); // 500 polls, then suspend RMW
 }
 LegacyFixture cu(RTWN_CHIP_92C);cu.b.cpuStuck=true;CHECK(!rtwLegacyPowerOffChecked(&cu.c));
 CHECK(cu.b.cpuPolls==100);cu.ordered({{true,3,1,0x50},{true,3,1,0x54},{true,0x1c,1,0x0e}});
}
static void gates() {
 CHECK(rtwLegacyPowerOffChecked(nullptr)==EINVAL);
 for(unsigned mode=0;mode<7;++mode){LegacyFixture f(RTWN_CHIP_92E);
  if(mode==0)f.c.chip=RTWN_CHIP_92E;
  if(mode==1)f.c.chip|=RTWN_CHIP_88E;
  if(mode==2)f.c.chip|=RTWN_CHIP_88F;
  if(mode==3)f.c.sc_ops.read_4=nullptr;
  if(mode==4)f.c.sc_ops.cookie=nullptr;
  if(mode==5)f.c.sc_ops.delay_us=nullptr;
  if(mode==6)f.b.error=EIO;
  CHECK(rtwLegacyPowerOffChecked(&f.c)==(mode==6?EIO:EINVAL));CHECK(!f.b.calls);
 }
}
int main(){successAndOrder();everyIoFaultFailsClosed();boardAndFirmwareBranches();boundsAndFallback();gates();
 std::printf("Legacy native HAL: %u checks, CU/88C/88E/92E shutdown order, every USB I/O fault, bounded polls, sticky-error preservation passed\n",checks);}
