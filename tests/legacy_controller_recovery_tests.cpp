/* SPDX-License-Identifier: GPL-2.0-only */
/* Real controller SCAN/JOIN, bus initialization, rtwn_init/stop, immutable OTP
 * decode, shared recovery, and complete legacy HAL. OS ownership/submission and
 * unrelated net80211/RF calibration/firmware execution are host boundaries. */
#include "rtw/StationRecovery.hpp"
#include "rtw/FamilyBackend.h"
#include "rtw/RTL8812AUBootstrap.h"
#include "rtw/RTW88Boot.h"
#include "rtw/RTL8192FU.h"
#include "rtw/RTL8710BU.h"
#include "rtw/RTL8723BU.h"
#include "rtw/RTL8192DUEfuse.h"
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/Efuse.h"
#include "rtw/USBPackets.h"
#include "rtw/USBRXCapacity.h"
#include "RTWControl.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>
using mbuf_t=void*;using u_char=unsigned char;using IOReturn=int;using IOInterruptState=int;
enum {kIOReturnSuccess=0,kIOReturnBadArgument=101,kIOReturnNoMemory=102,kIOReturnBusy=103,
 kIOReturnNoDevice=104,kIOReturnNotReady=105,kIOReturnTimeout=106,kIOReturnIOError=107,
 kIOReturnUnsupported=108,kIOReturnNotResponding=109,kIOReturnUnderrun=110};
enum {IFF_UP=1,IFF_RUNNING=2,IEEE80211_M_MONITOR=1,IEEE80211_M_STA=2,
 IEEE80211_F_WEPON=1,IEEE80211_F_PSK=2,IEEE80211_WEP_NKID=4,
 IEEE80211_WPA_PROTO_WPA2=1,IEEE80211_WPA_AKM_PSK=1,IEEE80211_WPA_CIPHER_CCMP=8,
 kUSBHostConnectionSpeedHigh=3};
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
#define __packed __attribute__((packed))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#include "r92creg.h"
struct ieee80211_node;
/* RTWN_OPS */
/* RTWN_DEFINES */
struct _ifnet {void*if_softc=nullptr,*iface=this;unsigned if_flags=0,if_timer=0;int if_snd=0;};
struct ieee80211_channel {unsigned number=1;};
struct ieee80211_node {ieee80211_channel *ni_chan=nullptr;};
struct ieee80211com {
 _ifnet ic_if;ieee80211_node*ic_bss=nullptr;ieee80211_channel*ic_ibss_chan=nullptr;
 uint8_t ic_myaddr[6]{2,1,2,3,4,5},ic_psk[32]{},ic_des_essid[32]{};
 unsigned ic_flags=0,ic_des_esslen=0;int ic_opmode=IEEE80211_M_STA,ic_nw_keys[4]{};
 ieee80211_state ic_state=IEEE80211_S_INIT;
 int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;
 int(*ic_set_key)(ieee80211com*,void*,int*)=nullptr;
};
struct device {char dv_xname[32]="host";};
struct rtwn_softc {
 rtwn_ops sc_ops{};int core_error=0;device*sc_pdev=nullptr;ieee80211com sc_ic;
 uint32_t chip=0,sc_flags=0;uint8_t board_type=0,crystal_cap=0,iq_calibration_mask=0;
 int fwcur=0,fw_station_connected=0,rate_config=0,sc_tx_timer=0,init_task=0;
};
static uint8_t rtwn_read_1(rtwn_softc*s,uint16_t a){return s->sc_ops.read_1(s->sc_ops.cookie,a);}
static uint16_t rtwn_read_2(rtwn_softc*s,uint16_t a){return s->sc_ops.read_2(s->sc_ops.cookie,a);}
static uint32_t rtwn_read_4(rtwn_softc*s,uint16_t a){return s->sc_ops.read_4(s->sc_ops.cookie,a);}
static void rtwn_write_1(rtwn_softc*s,uint16_t a,uint8_t v){s->sc_ops.write_1(s->sc_ops.cookie,a,v);}
static void rtwn_write_2(rtwn_softc*s,uint16_t a,uint16_t v){s->sc_ops.write_2(s->sc_ops.cookie,a,v);}
static void rtwn_write_4(rtwn_softc*s,uint16_t a,uint32_t v){s->sc_ops.write_4(s->sc_ops.cookie,a,v);}
#define rtwn_bb_read rtwn_read_4
#define rtwn_bb_write rtwn_write_4
static void rtwn_delay(rtwn_softc*s,uint32_t us){s->sc_ops.delay_us(s->sc_ops.cookie,us);}
static void explicit_bzero(void*p,size_t n){std::memset(p,0,n);}
static int rtwn_io_error(rtwn_softc*s){return s->core_error?s->core_error:s->sc_ops.io_error(s->sc_ops.cookie);}
static void ifq_clr_oactive(int*){}
static int splnet(){return 0;}static void splx(int){}static int systq;static void task_del(int,int*){}
static int ieee80211_new_state(ieee80211com*ic,ieee80211_state state,int arg){return ic->ic_newstate(ic,state,arg);}
struct ieee80211_wpaparams {int i_enabled,i_protos,i_akms,i_ciphers,i_groupcipher;};
static int ieee80211_ioctl_setwpaparms(ieee80211com*,const ieee80211_wpaparams*){return 0;}
static void ieee80211_begin_scan(_ifnet*){}
static int IOSimpleLockLockDisableInterrupt(void*){return 0;}static void IOSimpleLockUnlockEnableInterrupt(void*,int){}
static void rtwn_enable_intr(rtwn_softc*){}static void rtwn_disable_intr(rtwn_softc*){}
static void rtwn_set_nettype(rtwn_softc*,int){}static void rtwn_rxfilter_init(rtwn_softc*){}
static void rtwn_edca_init(rtwn_softc*){}static void rtwn_rate_fallback_init(rtwn_softc*){}
static void rtwn_rf_init(rtwn_softc*){}static void rtwn_cam_init(rtwn_softc*){}
static void rtwn_iq_calib(rtwn_softc*s){s->iq_calibration_mask=1;}
static void rtwn_lc_calib(rtwn_softc*){}static void rtwn_pa_bias_init(rtwn_softc*){}
static void rtwn_set_chan(rtwn_softc*,ieee80211_channel*,void*){}
static int rtwn_load_firmware(rtwn_softc*);
int rtwn_init(_ifnet*);void rtwn_stop(_ifnet*);void rtwn_init_task(void*);
void rtwn_efuse_read(rtwn_softc*,uint8_t*,size_t);void rtwn_efuse_switch_power(rtwn_softc*);
/* HAL_METHODS */

struct LegacyControlBus {
 std::array<uint8_t,65536> reg{};std::array<uint8_t,512> physical{};
 unsigned writes=0,reads=0,otpReads=0,otpAddress=0,stops=0;bool powerTimeout=false;rtw_wifi_family family;
 explicit LegacyControlBus(rtw_wifi_family f):family(f){physical.fill(255);physical[0]=0x0e;physical[1]=0x29;physical[2]=0x81;
  put(4,4,0x20002);put(0xf0,4,0x00011000);put(0x8a0,4,0xabcde);}
 uint32_t get(unsigned a,unsigned w)const{uint32_t v=0;for(unsigned i=0;i<w;++i)v|=uint32_t(reg[a+i])<<(8*i);return v;}
 void put(unsigned a,unsigned w,uint32_t v){for(unsigned i=0;i<w;++i)reg[a+i]=uint8_t(v>>(8*i));}
 uint32_t read(uint16_t a,unsigned w){++reads;uint32_t v=get(a,w);
  if(a==4){v|=0x20002;v&=~0x100U;if(powerTimeout)v&=~0x20002U;}if(a==5)v&=~3U;
  if(a==0x1e0)v&=~0xc0000000U;if(a==0x224)v&=~0x10000U;
  if(a==0x284)v|=0x20000;if(a==0x5f8)v=0;
  if(a==2&&w==2&&(get(0x1cf,1)&0x20))v&=~0x400U;
  if(a==0x30&&w==4){++otpReads;v=(v&~0xffU)|physical[otpAddress]|0x80000000U;}
  return v;
 }
 void write(uint16_t a,unsigned w,uint32_t v){++writes;put(a,w,v);
  if(a==0x30&&w==4)otpAddress=(v>>8)&0x3ff;
  if((family==RTW_WIFI_RTL8192CU&&a==0x1c&&w==1&&v==0x0e)||
     (family==RTW_WIFI_RTL8188EU&&a==0x40&&w==4&&v==0)||
     (family==RTW_WIFI_RTL8192EU&&a==5&&w==1&&(v&0x18)==8))++stops;
 }
};
struct IOUSBHostPipe {unsigned clears=0;int clearResult=0;int getSpeed()const{return kUSBHostConnectionSpeedHigh;}
 int clearStall(bool){++clears;return clearResult;}};
struct HostTimer {void setTimeoutMS(unsigned){}};
class RTWUSBController {public:
 static constexpr unsigned RXSlots=4;
 static constexpr unsigned RXCapacity=32768; // pre-recovery controller baseline
 rtwn_softc core_{};rtw_wifi_family family_;rtw_usb_wifi_profile catalog_{};
 const rtw_usb_wifi_profile*profile_=&catalog_;
 bool attached_=true,present_=true,usbOpened_=true,sleeping_=false,closing_=false,stopping_=false;
 bool running_=false,enabled_=false,radioReady_=false,joinRequested_=false,rateTransportFresh_=false;
 void*usb_=this,*completionLock_=this,*netif_=this,*stats_=this;IOReturn usbError_=0;
 IOUSBHostPipe pipes_[5],*rxPipe_=&pipes_[0],*txPipes_[4]{&pipes_[1],nullptr,nullptr,nullptr};
 uint16_t txMPS_[4]{512,0,0,0};rtw_usb_endpoints endpoints_{};rtw_chip_identity_descriptor descriptor_{};
 rtw_transfer_ledger ledger_{};
 struct {bool pending=false;uint32_t initialDrainMask=0,initialDrainGeneration[4]{};
  uint32_t stopDrainMask=0,stopDrainGeneration[RTW_TRANSFER_SLOTS]{};IOReturn stopDrainError=0;uint64_t stopDrainFailures=0;
  IOReturn initialDrainError=0,initialOriginalError=0,initialClearResult=0;
  unsigned initialErrors=0,initialConsecutive=0,initialFailures=0,initialExhausted=0,initialClearAttempts=0;}rxRecovery_;
 struct {unsigned generation=0,submitted=0,actual=0;IOReturn result=0;}transfers_[RTW_TRANSFER_SLOTS];
 HostTimer timer_;HostTimer*watchdogTimer_=&timer_;
 rtw_r12au_boot auCore_{};rtw88_boot_context r88Core_{};rtw_r92fu_board fuBoard_{};rtw_r10bu_board buBoard_{};rtw_8723bu r23Core_{};
 rtw_92du_rom duROM_{},duPortROM_[2]{};
 struct DUDataPort {rtw_usb_endpoints endpoints{};IOUSBHostPipe*rx=nullptr,*tx[4]{};uint16_t mps[4]{};}duPorts_[2];
 void*duDevice_=nullptr,*duSibling_=nullptr;bool duWholeDeviceOwned_=false;uint8_t duPortCount_=0,duActivePort_=0;
 int duPlan_=0;bool deferred_=false,descriptorValid_=true,powerOffFault_=false,firmwareReached_=false;int initialError_=0,firmwareError_=0;
 unsigned allocations_=0,frees_=0,submissions_=0,firmwares_=0;LegacyControlBus bus_;device device_;ieee80211_node node_;ieee80211_channel channel_;
#include "RTWFamilyRecoveryMembers.inc"
 struct ControlCall {uint32_t selector;const void*input;size_t length;void*output;size_t*outputLength;};
 explicit RTWUSBController(rtw_wifi_family f):family_(f),bus_(f){
  catalog_.family=f;catalog_.primary_verified=1;descriptor_.vendor=0xbda;descriptor_.product=uint16_t(f);descriptor_.configuration=1;
  endpoints_.num_bulk_in=endpoints_.num_bulk_out=1;endpoints_.bulk_in[0]=0x81;endpoints_.bulk_out[0]=2;
  rtw_transfer_reset(&ledger_);core_.sc_pdev=&device_;core_.sc_ic.ic_if.if_softc=&core_;core_.sc_ic.ic_bss=&node_;
  core_.sc_ic.ic_ibss_chan=&channel_;core_.sc_ic.ic_newstate=state;node_.ni_chan=&channel_;
  core_.chip=RTWN_CHIP_USB|(f==RTW_WIFI_RTL8192CU?RTWN_CHIP_92C:f==RTW_WIFI_RTL8188EU?RTWN_CHIP_88E:RTWN_CHIP_92E);
  auto&o=core_.sc_ops;o.cookie=this;o.read_1=read1;o.read_2=read2;o.read_4=read4;o.write_1=write1;o.write_2=write2;o.write_4=write4;
  o.io_error=ioError;o.delay_us=delayUS;o.power_on=busPower;o.dma_init=busDMA;o.mac_init=busMAC;o.bb_init=busBB;o.aggr_init=busAggregation;
  o.alloc_buffers=busAllocate;o.init=busInitialize;o.stop=busStop;o.wait_async=noop;o.cancel_scan=noop;o.cancel_calib=noop;
  assert(!captureFamilyRecoveryIdentity());
 }
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
 static uint8_t read1(void*p,uint16_t a){return uint8_t(owner(p)->bus_.read(a,1));}
 static uint16_t read2(void*p,uint16_t a){return uint16_t(owner(p)->bus_.read(a,2));}
 static uint32_t read4(void*p,uint16_t a){return owner(p)->bus_.read(a,4);}
 static void write1(void*p,uint16_t a,uint8_t v){auto*c=owner(p);
  if(c->powerOffFault_&&c->firmwareReached_&&a==R92C_TXPAUSE&&v==0xff){c->usbError_=kIOReturnIOError;return;}
  c->bus_.write(a,1,v);}
 static void write2(void*p,uint16_t a,uint16_t v){owner(p)->bus_.write(a,2,v);}
 static void write4(void*p,uint16_t a,uint32_t v){owner(p)->bus_.write(a,4,v);}
 uint32_t readRegister(uint16_t a,unsigned w){return bus_.read(a,w);}
 static void delayUS(void*,uint32_t){}static void noop(void*){}
 static int state(ieee80211com*ic,ieee80211_state s,int){ic->ic_state=s;return 0;}
 static int ioError(void*);static int busPower(void*);static int busDMA(void*);static void busMAC(void*);static void busBB(void*);static void busAggregation(void*);
 static int busAllocate(void*);static int busInitialize(void*);static void busStop(void*);
 bool is8812Family()const{return false;}bool is88Family()const{return false;}bool is8192FUFamily()const{return false;}bool is8710BUFamily()const{return false;}bool is8723Family()const{return false;}
 bool owns8812Authority()const{return false;}bool owns88Discovery()const{return false;}
 bool read88IdentityDescriptor(rtw_chip_identity_descriptor*out)const{if(!descriptorValid_)return false;*out=descriptor_;return true;}
 void freeTransfers(){assert(familyRecoveryDrained());++frees_;}
 int allocateTransfers(){++allocations_;return 0;}
 int submit(unsigned i,IOUSBHostPipe*,uint32_t length){++submissions_;assert(rtw_transfer_begin(&ledger_,i));
  auto&t=transfers_[i];++t.generation;t.submitted=length;t.actual=0;t.result=0;
  if(initialError_&&i==1){t.result=kIOReturnNotResponding;assert(rtw_transfer_complete(&ledger_,i));return initialError_;}return 0;}
 void drainBoundary(){for(unsigned i=0;i<RTW_TRANSFER_SLOTS;++i)if(ledger_.pending&(1U<<i)){
  if(!(ledger_.completed&(1U<<i)))assert(rtw_transfer_complete(&ledger_,i));assert(rtw_transfer_take(&ledger_,i));
  rxRecovery_.initialDrainMask&=~(1U<<i);}}
 void abortTransfers(){running_=radioReady_=false;rtw_transfer_stop(&ledger_);if(!deferred_)drainBoundary();}
 void quiesce(bool erase){if(stopping_)return;stopping_=true;enabled_=radioReady_=false;core_.sc_ic.ic_if.if_flags&=~IFF_UP;
  rtwn_stop(&core_.sc_ic.ic_if);if(erase){joinRequested_=false;core_.sc_ic.ic_des_esslen=0;}stopping_=false;}
 void fault(IOReturn error){usbError_=error;}
 template<class...T>void setProperty(T...){}void publishDiagnosticCounter(const char*,uint64_t){}
 void status(RTWControlStatus*){}void scanResults(RTWControlScanResults*){}void channelMap(RTWControlChannelMap*){}
 int start8723(){std::abort();}int start8710BU(){std::abort();}int start8192FU(){std::abort();}int start8192DU(){std::abort();}
 int start88(){std::abort();}int start8812(){std::abort();}int start8814(){std::abort();}int recover8814ForControl(){std::abort();}
 int initialize8812Core(){std::abort();}int initialize8192FUCore(){std::abort();}int initialize8710BUCore(){std::abort();}int initialize8192DUCore(){std::abort();}
 int bootstrap8723(bool){std::abort();}int prepare88Identity(const rtw88_fw_bus&){std::abort();}
 static int r88Read(void*,uint16_t,unsigned,uint32_t*){std::abort();}static int r88Write(void*,uint16_t,unsigned,uint32_t){std::abort();}
 static int r88Bulk(void*,uint8_t,const uint8_t*,size_t){std::abort();}
 static rtw_usb_error duControlIn(void*,const rtw_usb_setup*,uint8_t*,uint16_t*){std::abort();}
 static rtw_usb_error duControlOut(void*,const rtw_usb_setup*,const uint8_t*,uint16_t*){std::abort();}
 static int duWait(void*,uint32_t){std::abort();}
 void stop8723(){std::abort();}void stop8710BU(){std::abort();}void stop8192FU(){std::abort();}void stop8192DU(){std::abort();}void stop88(){std::abort();}void stop8812(){std::abort();}
 IOReturn enableStation();IOReturn handleControl(ControlCall*);
 IOReturn request(uint32_t selector){RTWControlRequest request{};request.version=RTW_CONTROL_VERSION;
  RTWControlJoin join{};join.version=RTW_CONTROL_VERSION;join.ssidLength=1;join.ssid[0]='x';RTWControlReply reply{};size_t size=sizeof(reply);
  ControlCall call{selector,selector==RTW_CONTROL_JOIN?(const void*)&join:(const void*)&request,
   selector==RTW_CONTROL_JOIN?sizeof(join):sizeof(request),&reply,&size};return handleControl(&call);}
};
static int rtwn_load_firmware(rtwn_softc*s){auto*c=RTWUSBController::owner(s->sc_ops.cookie);++c->firmwares_;c->firmwareReached_=true;
 if(c->firmwareError_)return c->firmwareError_;c->bus_.put(0x80,1,0x82);c->bus_.put(2,2,c->bus_.get(2,2)|0x400);return 0;}
template<class...T>static int hostOtherFamily(T...){std::abort();}
#define rtw88_boot_identify(...) hostOtherFamily(__VA_ARGS__)
#define rtw_92du_read_rom(...) hostOtherFamily(__VA_ARGS__)
static int rtw8192DUError(int){std::abort();}
/* COMMON_METHODS */
/* RTWN_METHODS */
/* CONTROLLER_METHODS */

static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static constexpr rtw_wifi_family families[]={RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU};
static void transientRequests(){for(auto family:families)for(uint32_t selector:{uint32_t(RTW_CONTROL_SCAN),uint32_t(RTW_CONTROL_JOIN)}){
 auto c=std::make_unique<RTWUSBController>(family);c->initialError_=EAGAIN;
 CHECK(c->request(selector)==kIOReturnBusy);CHECK(!c->core_.core_error&&!c->usbError_&&!c->running_&&!c->radioReady_);
 CHECK(c->pipes_[0].clears==1&&c->familyRecovery_.hardwareStopped&&c->bus_.stops==1);
 const auto firmware=c->firmwares_;c->initialError_=0;CHECK(c->request(selector)==kIOReturnSuccess);
 CHECK(c->firmwares_==firmware+1&&c->running_&&c->radioReady_&&!c->core_.core_error);
 CHECK(c->core_.sc_ic.ic_state==IEEE80211_S_SCAN&&c->ledger_.pending==15);c->quiesce(false);
}}
static void delayedDrain(){for(auto family:families)for(uint32_t selector:{uint32_t(RTW_CONTROL_SCAN),uint32_t(RTW_CONTROL_JOIN)}){
 auto c=std::make_unique<RTWUSBController>(family);c->initialError_=EAGAIN;c->deferred_=true;
 CHECK(c->request(selector)==kIOReturnBusy);CHECK(!c->core_.core_error&&!c->bus_.stops&&c->familyRecovery_.pendingStop);
 const auto firmware=c->firmwares_,writes=c->bus_.writes;c->initialError_=0;
 CHECK(c->request(selector)==kIOReturnBusy);CHECK(c->firmwares_==firmware&&c->bus_.writes==writes&&!c->bus_.stops);
 c->drainBoundary();c->deferred_=false;c->finishFamilyStopAfterDrain();
 CHECK(c->bus_.stops==1&&!c->familyRecovery_.pendingStop&&c->familyRecovery_.hardwareStopped&&!c->running_);
 CHECK(c->request(selector)==kIOReturnSuccess);CHECK(c->firmwares_==firmware+1&&c->running_&&!c->core_.core_error);c->quiesce(false);
}}
static void hardwareRecovery(){for(auto family:families)for(int original:{EIO,ETIMEDOUT}){
 auto c=std::make_unique<RTWUSBController>(family);c->firmwareError_=original;
 CHECK(c->request(RTW_CONTROL_SCAN)==(original==EIO?kIOReturnIOError:kIOReturnTimeout));
 CHECK(c->core_.core_error==original&&c->familyRecovery_.diag.firstFailure==unsigned(original)&&c->bus_.stops==1);
 const auto otp=c->bus_.otpReads;c->firmwareError_=0;CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
 CHECK(!c->core_.core_error&&c->running_&&c->familyRecovery_.diag.attempts==1&&c->familyRecovery_.diag.successes==1);
 CHECK(c->bus_.otpReads>=otp+(family==RTW_WIFI_RTL8188EU?256:512));c->quiesce(false);
 auto p=std::make_unique<RTWUSBController>(family);p->firmwareError_=original;
 for(unsigned attempt=0;attempt<3;++attempt)CHECK(p->request(attempt==1?RTW_CONTROL_JOIN:RTW_CONTROL_SCAN)==(original==EIO?kIOReturnIOError:kIOReturnTimeout));
 CHECK(p->familyRecovery_.diag.attempts==2&&p->familyRecovery_.diag.consecutive==2&&p->allocations_==3);
 const auto writes=p->bus_.writes,allocations=p->allocations_,reads=p->bus_.reads;
 CHECK(p->request(RTW_CONTROL_JOIN)==(original==EIO?kIOReturnIOError:kIOReturnTimeout));
 CHECK(p->bus_.writes==writes&&p->bus_.reads==reads&&p->allocations_==allocations&&p->core_.core_error==original);
}}
static void identityGates(){for(auto family:families)for(unsigned mutation=0;mutation<8;++mutation){
 auto c=std::make_unique<RTWUSBController>(family);c->firmwareError_=ETIMEDOUT;CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnTimeout);
 c->firmwareError_=0;if(mutation==0)c->bus_.physical[1]^=1;if(mutation==1)c->descriptor_.product^=1;
 if(mutation==2)c->usb_=&c->catalog_;if(mutation==3)c->endpoints_.bulk_out[0]^=1;
 if(mutation==4)c->profile_=nullptr;if(mutation==5)c->txMPS_[0]=64;if(mutation==6)c->sleeping_=true;if(mutation==7)c->usbError_=kIOReturnIOError;
 const auto allocations=c->allocations_,stops=c->bus_.stops;CHECK(c->request(RTW_CONTROL_JOIN)!=kIOReturnSuccess);
 CHECK(c->core_.core_error==ETIMEDOUT&&!c->running_&&c->allocations_==allocations&&c->bus_.stops==stops);
 if(mutation==0)CHECK(c->familyRecovery_.blocked&&c->familyRecovery_.diag.identity==ENODEV);
}}
static void failedCleanupAndAdapterIsolation(){for(auto family:families){
 auto failed=std::make_unique<RTWUSBController>(family),healthy=std::make_unique<RTWUSBController>(family);
 failed->firmwareError_=ETIMEDOUT;failed->powerOffFault_=true;
 CHECK(failed->request(RTW_CONTROL_SCAN)==kIOReturnTimeout);
 CHECK(failed->core_.core_error==ETIMEDOUT&&failed->familyRecovery_.diag.failure==ETIMEDOUT);
 CHECK(failed->familyRecovery_.blocked&&failed->familyRecovery_.diag.cleanup==EIO&&!failed->bus_.stops);
 const auto writes=failed->bus_.writes,allocations=failed->allocations_;
 failed->usbError_=0;failed->powerOffFault_=false;failed->firmwareError_=0;
 CHECK(failed->request(RTW_CONTROL_JOIN)==kIOReturnIOError);
 CHECK(failed->core_.core_error==ETIMEDOUT&&failed->bus_.writes==writes&&failed->allocations_==allocations);
 CHECK(healthy->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 CHECK(healthy->running_&&!healthy->core_.core_error&&healthy->familyRecovery_.diag.attempts==0);healthy->quiesce(false);
}}
static void watchdogErrorProvenance(){for(auto family:families){
 auto c=std::make_unique<RTWUSBController>(family);c->core_.sc_ic.ic_if.if_flags|=IFF_UP;c->bus_.powerTimeout=true;rtwn_init_task(&c->core_);
 CHECK(c->core_.core_error==ETIMEDOUT);CHECK(c->familyRecovery_.diag.failure==ETIMEDOUT&&c->familyRecovery_.eligible&&c->familyRecovery_.hardwareStopped&&c->familyRecovery_.diag.failures==1);
 const auto writes=c->bus_.writes,allocations=c->allocations_;c->bus_.powerTimeout=false;rtwn_init_task(&c->core_);
 CHECK(c->core_.core_error==ETIMEDOUT&&c->bus_.writes==writes&&c->allocations_==allocations);
 CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess&&c->running_&&c->familyRecovery_.diag.attempts==1);c->quiesce(false);
 for(int transient:{EAGAIN,EBUSY}){auto t=std::make_unique<RTWUSBController>(family);t->initialError_=EAGAIN;t->core_.sc_ic.ic_if.if_flags|=IFF_UP;if(transient==EBUSY)t->deferred_=true;rtwn_init_task(&t->core_);if(t->core_.core_error||t->running_)std::fprintf(stderr,"legacy task family=%u transient=%d core=%d running=%d cleanup=%u\n",unsigned(family),transient,t->core_.core_error,t->running_,t->familyRecovery_.diag.cleanup);CHECK(!t->core_.core_error&&!t->running_);if(transient==EBUSY){t->drainBoundary();t->deferred_=false;t->finishFamilyStopAfterDrain();}t->initialError_=0;CHECK(t->request(RTW_CONTROL_SCAN)==kIOReturnSuccess&&t->running_);t->quiesce(false);}
}}
int main(){watchdogErrorProvenance();transientRequests();delayedDrain();hardwareRecovery();identityGates();failedCleanupAndAdapterIsolation();
 std::printf("Actual legacy SCAN/JOIN + rtwn_init/stop + common recovery + full HAL/OTP: %u checks across CU/88E/92E passed\n",checks);}
