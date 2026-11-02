/* SPDX-License-Identifier: BSD-3-Clause */
/* Real bootstrap, Boot, firmware loader, efuse reader, feature parser and MAC
 * plan. Only USB registers, opaque firmware execution and unused PHY reset
 * are modeled. A success stub never replaces bootstrap or firmware loading. */
#include "rtw/RTW88PHYOperating.h"
#include "rtw/RTW88Station.h"
#include <memory>
#include "RTL8821CUFirmware.inc"
#include "RTL8822BUFirmware.inc"
#include "RTL8822CUFirmware.inc"
/* ACTUAL_BOOT_BUS_MODEL */
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL%u: %s\n",__LINE__,#x);std::exit(1);}}while(0)
extern "C" void rtw88_phy_radio_invalidate(rtw88_phy_context*p){if(p)*p={};}
class RTWUSBController {
public:
 bool validateFamilyRecoveryBoard()const{return true;}

 rtw_wifi_family family_;const rtw_usb_wifi_profile*profile_;Bus bus;
 rtw88_boot_context r88Core_{};rtw88_mac_context r88MAC_{};rtw88_phy_context r88PHY_{};rtw88_station_context r88Station_{};
 struct {unsigned ntxchains=0;}core_;
 std::vector<uint64_t> storage;void*r88Storage_;void*rxPipe_=(void*)1;void*txPipes_[4]={(void*)1,(void*)1,(void*)1,(void*)1};uint16_t txMPS_[4]={512,512,512,512};
 struct {unsigned num_bulk_in=1,num_bulk_out=2;}endpoints_;
 uint16_t r88PacketSequence_=0;bool r88BoardKnown_=false;uint8_t r88MACAddress_[6]{};
 bool authorityComplete=false;
 bool requested=false,stale_report=false;unsigned cpu_boots=0,bulks=0,requests=0,io_calls=0,fail_at=0;std::vector<unsigned> order;
 explicit RTWUSBController(rtw_wifi_family f):family_(f),profile_(profile(f)),bus(f),storage((RTW88_PHY_RADIO_SCRATCH_BYTES+RTW88_FW_SCRATCH_BYTES+RTW88_EFUSE_SCRATCH_BYTES+7)/8),r88Storage_(storage.data()){
  bus.reg[3]=4;bus.reg[0x1d]=1;bus.reg[9]=0x40;bus.reg[0x1082]=1;
 }
 rtw_chip_identity*r88ResolvedIdentity_=nullptr;alignas(uint64_t) uint8_t r88IdentityStorage_[RTW_CHIP_IDENTITY_STORAGE_BYTES]{};
 // This ordinary-profile fixture never enters discovery. The separate actual
 // Controller suite supplies real USB descriptors and both resolved families.
 bool matches88Discovery()const{assert(!r88ResolvedIdentity_);return true;}
 bool owns88Discovery()const{assert(!r88ResolvedIdentity_);return true;}
 void removeProperty(const char*){assert(false);}
 void setProperty(const char*,const char*){assert(false);}
 void setProperty(const char*,unsigned long long,unsigned){assert(false);}
 void setProperty(const char*,bool){assert(false);}
 void clear88Discovery(bool);int prepare88Identity(const rtw88_fw_bus&);int recheck88Identity();int publish88Identity();
 bool is88Family()const;int bootstrap88(bool);
 // Coexistence has not been initialized in this boot-only boundary.
 void coex88Stop(){}
 // Snapshot bookkeeping is observed here; identity/firmware I/O remains real.
 void remember88Authority(bool complete){authorityComplete=complete;}
 static int ioError(void*){return 0;}
 static int r88Read(void*p,uint16_t a,unsigned w,uint32_t*v){auto&s=*(RTWUSBController*)p;if(++s.io_calls==s.fail_at)return EIO;return Bus::read(&s.bus,a,w,v);}
 static int r88Write(void*p,uint16_t a,unsigned w,uint32_t v){auto&s=*(RTWUSBController*)p;if(++s.io_calls==s.fail_at)return EIO;
  if(a==0x1a0&&w==1&&v==0xfd){CHECK(s.r88Core_.firmware.powered);++s.requests;s.requested=s.cpu_boots==0&&s.bulks==0;s.order.push_back(1);}
  if(a==0x1208){v&=~0x80000000U;if(v&0x02000000U)v&=~0x08000000U;}
  int e=Bus::write(&s.bus,a,w,v);if(e)return e;
  // Actual FW loader resets/enables CPU at SYS_FUNC_EN+1. The firmware
  // only emits19 if the FD request existed before download/CPU boot.
  if(a==3&&(v&4U)&&(s.bus.reg[0x81]&0x40U)){
   ++s.cpu_boots;s.order.push_back(3);s.bus.reg[0x80]=0x78;s.bus.reg[0x81]=0xc0;
   if(s.requested){s.bus.reg[0x1a0]=0x19;uint32_t features=(7U<<16)|(2U<<19)|(1U<<21)|(3U<<26);put(s.bus.reg,0x1a6,4,features);}
   else if(!s.stale_report)s.bus.reg[0x1a0]=0;
  }
  return 0;
 }
 static int r88Bulk(void*p,uint8_t q,const uint8_t*b,size_t n){auto&s=*(RTWUSBController*)p;if(++s.io_calls==s.fail_at)return EIO;CHECK(q==16&&b&&n>=49&&n<=4145&&b[2]==48);++s.bulks;s.order.push_back(2);s.bus.reg[0x205]|=0x80;return 0;}
 static void delayUS(void*p,uint32_t us){Bus::delay(&((RTWUSBController*)p)->bus,us);}
};
/* ACTUAL_BOOTSTRAP_METHODS */
int main(){
 unsigned injected=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  auto s=std::make_unique<RTWUSBController>(family);
  int result=s->bootstrap88(false);if(result)std::fprintf(stderr,"family%u bootstrap error%d: report%02x requests%u CPU boots%u firmware%d board%d\n",unsigned(family),result,s->bus.reg[0x1a0],s->requests,s->cpu_boots,s->r88Core_.firmware.firmware_running,s->r88Core_.board_valid);CHECK(result==0);CHECK(s->authorityComplete);
  CHECK(s->requests==1&&s->cpu_boots==1&&s->bulks>0&&s->r88Core_.board.features_valid&&s->r88BoardKnown_);
  CHECK(!s->order.empty()&&s->order[0]==1);CHECK(s->bus.reg[0x1a0]==0);
  CHECK(s->r88Core_.board.tx_streams==1&&s->r88Core_.board.active_rf_paths==1&&s->r88Core_.board.feature_bw==80);
  CHECK(s->r88Core_.board.rf_paths==(family==RTW_WIFI_RTL8821CU?1:2));
  // Every control/bulk failure up to and including feature acknowledgment
  // must abort the actual bootstrap and leave features unvalidated.
  const unsigned operations=s->io_calls;
  for(unsigned cut=1;cut<=operations;cut++){
   auto failed=std::make_unique<RTWUSBController>(family);failed->fail_at=cut;
   CHECK(failed->bootstrap88(false)==EIO);CHECK(!failed->r88Core_.board.features_valid&&!failed->r88BoardKnown_&&!failed->authorityComplete);++injected;
  }
  // A stale preexisting report does not replace the cold-boot request.
  auto stale=std::make_unique<RTWUSBController>(family);stale->stale_report=true;stale->bus.reg[0x1a0]=0x19;
  CHECK(stale->bootstrap88(false)==0&&stale->requests==1&&stale->r88Core_.board.features_valid);
 }
 std::printf("rtw88 bootstrap: %u checks,all3 actual Boot/FW/OTP/feature paths, %u I/O failures; FD precedes CPU boot\n",checks,injected);
}
