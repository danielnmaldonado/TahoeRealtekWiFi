/* Actual controller startup/control/stop plus production AU HAL. Only OS/USB
 * and net80211 host ownership are doubled; RFK completion is a register model. */
#include "rtw/RTL8814AU.h"
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8814AUStation.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/USBDescriptors.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/ChipIdentity.h"
#include "rtw/StationChannels.h"
#include "rtw/RTL8192EUDataPath.h"
#include "RTWControl.h"
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <map>
#include <string>
#include <vector>
#include "RTL8814AUFirmware.inc"
inline void explicit_bzero(void *p,size_t size){std::memset(p,0,size);}
using IOReturn = unsigned;
enum : IOReturn {kIOReturnSuccess=0, kIOReturnBadArgument=1, kIOReturnNoMemory=2,
 kIOReturnBusy=3,kIOReturnNoDevice=4,kIOReturnNotReady=5,kIOReturnTimeout=6,
 kIOReturnIOError=7,kIOReturnUnsupported=8};
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_M_STA=1,IEEE80211_F_SHSLOT=2,IEEE80211_RATE_BASIC=128,
 IEEE80211_NODE_HT=1,IEEE80211_HTCAP_SGI20=32,IEEE80211_CHAN_5GHZ=0x100,
 IEEE80211_CHAN_PASSIVE=0x200,IFF_UP=1,IFF_RUNNING=2,IEEE80211_F_PSK=4,
 IEEE80211_WPA_PROTO_WPA2=1,IEEE80211_WPA_AKM_PSK=1,IEEE80211_WPA_CIPHER_CCMP=8,RTWN_FLAG_BUSY=1,
 IEEE80211_F_RSNON=8,IEEE80211_F_AUTO_JOIN=16};
struct ieee80211_channel {int number=1;unsigned ic_flags=0;};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
struct ieee80211_rateset {uint8_t rs_nrates=0,rs_rates[32]{};};
struct ieee80211_node {unsigned ni_flags=0,ni_htcaps=0;uint8_t ni_rxmcs[10]{},ni_bssid[6]{};
 ieee80211_channel *ni_chan=nullptr;uint16_t ni_associd=0,ni_intval=100;ieee80211_rateset ni_rates;};
struct _ifnet {unsigned if_flags=0,if_timer=0;int if_snd=0;};
struct ieee80211com {int ic_opmode=IEEE80211_M_STA;ieee80211_state ic_state=IEEE80211_S_INIT;
 unsigned ic_flags=0,ic_htcaps=32;uint8_t ic_myaddr[6]{},ic_psk[32]{},ic_des_essid[32]{};unsigned ic_des_esslen=0;
 ieee80211_node *ic_bss=nullptr;ieee80211_channel *ic_ibss_chan=nullptr;_ifnet ic_if;
 int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;void *ic_softc=nullptr;};
struct rtwn_softc {ieee80211com sc_ic;int core_error=0,sc_tx_timer=0,init_task=0;unsigned iq_calibration_mask=0,sc_flags=0;
 int(*sc_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;};
struct ieee80211_wpaparams {int i_enabled,i_protos,i_akms,i_ciphers,i_groupcipher;};
static int ieee80211_ioctl_setwpaparms(ieee80211com*,const ieee80211_wpaparams*){return 0;}
static void ieee80211_begin_scan(_ifnet*){}
static unsigned ieee80211_chan2ieee(ieee80211com*,ieee80211_channel *c){return unsigned(c->number);}
static void ifq_clr_oactive(int*){}
static int systq;
static void task_del(int,int*){}
using IOInterruptState = int;
[[maybe_unused]] static IOInterruptState IOSimpleLockLockDisableInterrupt(void*){return 0;}
[[maybe_unused]] static void IOSimpleLockUnlockEnableInterrupt(void*,IOInterruptState){}
struct Bus {
 std::array<uint8_t,65536> reg{};std::array<uint8_t,1024> otp{};
 unsigned otpAddress=0,reads=0,writes=0,bulks=0,rfkStarts=0,otpReads=0,powerOns=0;
 unsigned mailboxReads=0,mailboxWrites=0;
 bool rfkFail=false,powerOffFail=false,identityReadFail=false;
 unsigned failReadAddress=0x10000,failWriteAddress=0x10000;
 int failureError=EIO;
 uint32_t get(unsigned a,unsigned w) const {uint32_t v=0;for(unsigned i=0;i<w;i++)v|=uint32_t(reg[a+i])<<(8*i);return v;}
 void put(unsigned a,unsigned w,uint32_t v){for(unsigned i=0;i<w;i++)reg[a+i]=uint8_t(v>>(8*i));}
 Bus(){otp.fill(255);put(0x100,1,0xea);put(0xf0,4,0x044411b5);put(0xfc,4,0x80000008);put(4,4,0x30453f15);
  std::array<uint8_t,512> logical{};logical[0]=0x29;logical[1]=0x81;
  for(unsigned i=0;i<168;i++)logical[0x10+i]=24;
  logical[0xb9]=31;logical[0xba]=31;logical[0xc1]=1;logical[0xc9]=255;logical[0xca]=1;
  const uint8_t mac[6]={2,0x11,0x22,0x33,0x44,0x55};memcpy(logical.data()+0xd8,mac,6);
  size_t j=0;for(unsigned block=0;block<64;block++){if(block<16)otp[j++]=uint8_t(block<<4);
   else{otp[j++]=uint8_t(((block&7)<<5)|15);otp[j++]=uint8_t((block&0x78)<<1);}
   for(unsigned k=0;k<8;k++)otp[j++]=logical[block*8+k];}otp[j]=255;
 }
 int read(uint16_t a,unsigned w,uint32_t *v){reads++;if(a==0x1cc)++mailboxReads;if((identityReadFail&&a==0xf0)||a==failReadAddress)return failureError;*v=get(a,w);
  if(a==0x30&&w==4){otpReads++;*v=(*v&~255U)|otp[otpAddress]|0x80000000U;}
  if(a==5)*v&=~3U;if(a==6)*v|=2U;if(a==0x208)*v&=~1U;
  if(a==0x1208&&w==4)*v&=~0x80000000U;
  if(a==0x670)*v&=~0x80000000U;
  /* Fail only physical pathA; the remaining three paths still calibrate. */
  if(a==0x1b00&&w==4&&(!rfkFail||((*v>>4)&15U)!=1U))*v&=~1U;
  if(a==0x1b08&&w==4)*v=0;if(a==0x1bfc&&w==4)*v=0x00100010U;return 0;
 }
 int write(uint16_t a,unsigned w,uint32_t v){if((powerOffFail&&a==5&&(v&2))||a==failWriteAddress)return failureError;writes++;if((a>=0x1d0&&a<0x1e0)||(a>=0x1f0&&a<0x200))++mailboxWrites;put(a,w,v);
  if(a==0x30&&w==4)otpAddress=(v>>8)&1023;
  if(a==5&&(v&2))put(0x100,1,0xea);else if(a==5&&(v&1)){powerOns++;put(0x100,1,0);}
  if(a==3&&(v&4)&&(get(0x80,2)&0x4000))put(0x80,2,0xc078);
  if(a==0x1b00&&(v&1))rfkStarts++;
  const uint16_t sipi[4]={0xc90,0xe90,0x1890,0x1a90},base[4]={0x2800,0x2c00,0x3800,0x3c00};
  for(unsigned p=0;p<4;p++)if(a==sipi[p]&&w==4)put(base[p]+((v>>20)&255)*4,4,v&0xfffffU);return 0;
 }
 int bulk(){bulks++;put(0x205,1,get(0x205,1)|0x80);return 0;}
};
class RTWUSBController {
public:
 struct ControlCall{uint32_t selector;const void*input;size_t length;void*output;size_t*outputLength;};
 rtwn_softc core_{};rtw_8814au acCore_{};rtw_8814au_rate_config acRateConfig_{};
 void *acPHYStorage_=nullptr;size_t acPHYSize_=0;bool acMediaConnected_=false,acRateReady_=false;
 uint8_t acChannel_=0,acRSSI_=0;bool attached_=true,present_=true,sleeping_=false,closing_=false,stopping_=false;
 bool running_=false,enabled_=false,radioReady_=false,joinRequested_=false,usbOpened_=true,userRadioDisabled_=false,browseScanComplete_=false;
 IOReturn usbError_=0;void *usb_=this,*netif_=this,*stats_=this,*completionLock_=this;
 rtw_wifi_family family_=RTW_WIFI_RTL8814AU;rtw_usb_endpoints endpoints_{};rtw_transfer_ledger ledger_{};
 rtw_usb_wifi_profile profileValue_{0x2357,0x0106,0,0,1,0,RTW_WIFI_RTL8814AU,1,"AU",0,0,0,0,0};
 const rtw_usb_wifi_profile *profile_=&profileValue_;
 rtw_chip_identity_descriptor descriptor_{0x2357,0x0106,0,1,0,255,255,255,0};
 struct{bool pending=false;uint32_t initialDrainMask=0;IOReturn initialDrainError=0;}rxRecovery_;unsigned rxSubmitted=0,starts=0,allocations=0;int initialError=0;
 bool abortLeavesPending=false,descriptorValid=true;
 std::map<std::string,uint64_t> published;
 Bus bus;std::vector<uint64_t> storage;ieee80211_node node;ieee80211_channel channel;
 /* RECOVERY_MEMBERS */
 /* DEFERRED_STOP_DECL */
 RTWUSBController(){endpoints_.num_bulk_in=1;endpoints_.num_bulk_out=3;endpoints_.bulk_in[0]=0x81;
  endpoints_.bulk_out[0]=2;endpoints_.bulk_out[1]=3;endpoints_.bulk_out[2]=4;
  rtw_transfer_reset(&ledger_);core_.sc_ic.ic_softc=this;core_.sc_ic.ic_bss=&node;node.ni_chan=&channel;
  core_.sc_ic.ic_ibss_chan=&channel;core_.sc_newstate=generic;core_.sc_ic.ic_newstate=transition;
  const rtw_8814au_bus_ops ops{this,acRead,acWrite,acBulk,delayUS};rtw_8814au_init(&acCore_,&ops);
  assert(!rtw_8814au_identify_read_board(&acCore_,acSpeed()));acCore_.board.tx_streams=acCore_.board.rx_streams=2;
  acCore_.board.antenna_tx=acCore_.board.antenna_rx=3;memcpy(core_.sc_ic.ic_myaddr,acCore_.board.mac,6);
  acPHYSize_=rtw_8814au_phy_size();storage.resize((acPHYSize_+7)/8);acPHYStorage_=storage.data();
  /* CAPTURE_IDENTITY */
 }
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
 static int acRead(void*p,uint16_t a,unsigned w,uint32_t*v){return owner(p)->bus.read(a,w,v);}
 static int acWrite(void*p,uint16_t a,unsigned w,uint32_t v){return owner(p)->bus.write(a,w,v);}
 static int acBulk(void*p,uint8_t,const uint8_t*,size_t){return owner(p)->bus.bulk();}
 static void delayUS(void*,uint32_t){}
 rtw_8814au_speed acSpeed() const{return RTW_8814AU_SUPER_SPEED;}
 bool read88IdentityDescriptor(rtw_chip_identity_descriptor*out)const{if(!descriptorValid)return false;*out=descriptor_;return true;}
 static int generic(ieee80211com*ic,ieee80211_state s,int){ic->ic_state=s;return 0;}
 static int transition(ieee80211com*ic,ieee80211_state s,int a){return owner(ic->ic_softc)->state8814(ic,s,a);}
 int allocateTransfers(){allocations++;return 0;}
 void freeTransfers(){}
 void abortTransfers(){running_=radioReady_=false;rtw_transfer_stop(&ledger_);if(!abortLeavesPending)ledger_.pending=ledger_.completed=0;finish8814StopAfterDrain();}
 static int busInitialize(void*p){auto*c=owner(p);if(c->initialError)return c->initialError;if(!rtw_transfer_restart(&c->ledger_))return EBUSY;c->running_=true;c->rxSubmitted+=4;return 0;}
 void quiesce(bool erase){if(stopping_)return;stopping_=true;radioReady_=enabled_=false;core_.sc_ic.ic_if.if_flags&=~IFF_UP;stop8814();if(erase){joinRequested_=false;core_.sc_ic.ic_des_esslen=0;}stopping_=false;}
 static void cancelScan(void*){}static void cancelCalibration(void*){}static void nextScan(void*){}static void nextCalibration(void*){}
 static void update8814EDCA(ieee80211com*){}
 bool permitsDFSChannelByCountry(uint8_t)const{return true;}
 template<class...T>void setProperty(T...){}
 void publishDiagnosticCounter(const char*name,uint64_t value){published[name]=value;}
 bool isLegacyRecoveryFamily()const{return false;}
 void failFamilyStart(int){}void finishFamilyStart(){}int recoverFamilyForControl(){return core_.core_error;}
 bool is8723Family()const{return false;}bool is8710BUFamily()const{return false;}bool is8192FUFamily()const{return false;}
 bool is88Family()const{return false;}bool is8812Family()const{return false;}
 int start8723(){return ENOTSUP;}int start8710BU(){return ENOTSUP;}int start8192FU(){return ENOTSUP;}
 int start8192DU(){return ENOTSUP;}int start88(){return ENOTSUP;}int start8812(){return ENOTSUP;}
 void status(RTWControlStatus*){}void scanResults(RTWControlScanResults*){}void channelMap(RTWControlChannelMap*){}
 int start8814();void stop8814();int set8814Channel(uint8_t);int state8814(ieee80211com*,ieee80211_state,int);
 int tick8814();bool uses8814FirmwareRate()const;
 static void initTask8814(void*);
 void resetDFSStation(){}void resetWpaRecovery(){}void eraseProfile();
 rtw_8814au_station_params station8814Params()const;IOReturn enableStation();IOReturn handleControl(ControlCall*);
 IOReturn request(uint32_t selector){RTWControlJoin input{};input.version=RTW_CONTROL_VERSION;input.ssidLength=1;input.ssid[0]='x';
  RTWControlRequest scan{};scan.version=RTW_CONTROL_VERSION;RTWControlReply output{};size_t size=sizeof(output);
  ControlCall call{selector,selector==RTW_CONTROL_JOIN?static_cast<void*>(&input):static_cast<void*>(&scan),selector==RTW_CONTROL_JOIN?sizeof(input):sizeof(scan),&output,&size};return handleControl(&call);}
};
static int rtwn_init(_ifnet*){return ENOTSUP;}
static int rtwn_io_error(rtwn_softc*c){return c->core_error;}
/* ACTUAL_METHODS */
static unsigned checks,failures;
#define CHECK(x) do{checks++;if(!(x)){failures++;std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);}}while(0)
static std::unique_ptr<RTWUSBController> failedFirstStart(){auto c=std::make_unique<RTWUSBController>();c->bus.rfkFail=true;
 CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnTimeout);CHECK(c->core_.core_error==ETIMEDOUT);CHECK(!c->running_&&!c->acCore_.phy_initialized);
 CHECK(c->rxSubmitted==0&&c->core_.sc_ic.ic_state==IEEE80211_S_INIT);CHECK(c->bus.rfkStarts>0);return c;}
static void retryStartsFromNativeReset(){auto c=failedFirstStart();const auto before=c->bus.rfkStarts;c->bus.rfkFail=false;
 const auto oldOtpReads=c->bus.otpReads,oldBulks=c->bus.bulks;
 const auto retry=c->request(RTW_CONTROL_SCAN);
 CHECK(retry==kIOReturnSuccess);CHECK(c->rxSubmitted==4&&c->running_&&c->radioReady_);
 CHECK(c->acCore_.phy_initialized&&c->core_.sc_ic.ic_state==IEEE80211_S_SCAN);CHECK(c->bus.rfkStarts>before);CHECK(!c->core_.core_error&&!c->acCore_.io_error);
 CHECK(c->bus.otpReads>=oldOtpReads+3072&&c->bus.bulks>oldBulks&&c->allocations==2);
 c->publish8814RecoveryDiagnostics();CHECK(c->published["RTWDiagAUStage"]==13&&c->published["RTWDiagAUResult"]==0);
 CHECK(c->published["RTWDiagAUFirstFailureStage"]==8&&c->published["RTWDiagAUFirstFailureResult"]==ETIMEDOUT);
 CHECK(c->published["RTWDiagAUStarts"]==2&&c->published["RTWDiagAURecoveryAttempts"]==1&&c->published["RTWDiagAURecoverySuccesses"]==1);
 CHECK(c->published["RTWDiagAUConsecutiveRecoveries"]==0&&c->published["RTWDiagAUCleanupResult"]==0&&c->published["RTWDiagAUIdentityResult"]==0);}
static void joinAlsoRecovers(){auto c=failedFirstStart();c->bus.rfkFail=false;CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);CHECK(c->joinRequested_&&c->rxSubmitted==4);}
static void persistentTimeoutIsBounded(){auto c=failedFirstStart();CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnTimeout);CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnTimeout);
 const unsigned before=c->bus.writes;CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);CHECK(c->bus.writes==before);CHECK(!c->rxSubmitted);}
static void negativeGates(){for(unsigned mode=0;mode<11;mode++){auto c=failedFirstStart();c->bus.rfkFail=false;
  if(mode==0)c->usbError_=kIOReturnIOError;if(mode==1)c->sleeping_=true;if(mode==2)c->present_=false;if(mode==3)c->closing_=true;
  if(mode==4)c->family_=RTW_WIFI_RTL8192EU;if(mode==5)c->descriptor_.product=0x0108;
  if(mode==6)c->profile_=nullptr;if(mode==7)c->endpoints_.bulk_out[0]=9;if(mode==8)c->core_.core_error=ENOTSUP;
  if(mode==9)c->acCore_.io_error=EINVAL;
  if(mode==10)c->rxRecovery_.initialDrainError=kIOReturnIOError;
  const unsigned before=c->bus.writes;CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);CHECK(c->bus.writes==before);CHECK(!c->rxSubmitted);}}
static void failedPowerOffNeverRestarts(){auto c=failedFirstStart();c->bus.rfkFail=false;c->bus.powerOffFail=true;
 c->bus.put(0x100,1,0); // Model a card still active when checked cleanup runs.
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);const unsigned before=c->bus.rfkStarts;c->bus.powerOffFail=false;
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);CHECK(c->bus.rfkStarts==before&&!c->rxSubmitted);}
static void changedRomNeverRestarts(){auto c=failedFirstStart();c->bus.rfkFail=false;c->bus.otp[2]^=1;
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);CHECK(!c->rxSubmitted&&!c->acCore_.phy_initialized);}
static void failedIdentityReadNeverRestarts(){auto c=failedFirstStart();c->bus.rfkFail=false;c->bus.identityReadFail=true;
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);const auto before=c->bus.rfkStarts;c->bus.identityReadFail=false;
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);CHECK(c->bus.rfkStarts==before&&!c->rxSubmitted);}
static void changedChipNeverRestarts(){auto c=failedFirstStart();c->bus.rfkFail=false;c->bus.put(0xf0,4,c->bus.get(0xf0,4)^0x1000U);
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess);CHECK(!c->rxSubmitted&&!c->acCore_.phy_initialized);}
static void drainMustFinish(){auto c=failedFirstStart();c->bus.rfkFail=false;c->abortLeavesPending=true;c->ledger_.pending=1;
 const auto before=c->bus.bulks;CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnBusy);CHECK(!c->rxSubmitted&&c->bus.bulks==before);c->ledger_.pending=0;c->abortLeavesPending=false;
 CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);CHECK(c->rxSubmitted==4);}
static void cleanupSeparatesOriginalError(){auto c=failedFirstStart();c->acCore_.io_error=ETIMEDOUT;c->bus.put(0x100,1,0);c->bus.powerOffFail=true;
 CHECK(rtw_8814au_stop_checked(&c->acCore_)==EIO);CHECK(c->acCore_.io_error==ETIMEDOUT&&!c->acCore_.powered&&!c->acCore_.phy_initialized);
 c->bus.powerOffFail=false;CHECK(rtw_8814au_stop_checked(&c->acCore_)==0);CHECK(c->acCore_.io_error==ETIMEDOUT);}
static void healthyStopMustDrainBeforeRestart(){auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 c->abortLeavesPending=true;c->ledger_.pending=1;c->stop8814();const auto powerOns=c->bus.powerOns,bulks=c->bus.bulks,rfk=c->bus.rfkStarts;
 CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnBusy);CHECK(c->bus.powerOns==powerOns&&c->bus.bulks==bulks&&c->bus.rfkStarts==rfk);
 c->ledger_.pending=0;c->abortLeavesPending=false;CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);CHECK(c->rxSubmitted==8);}
static void initialTransientDoesNotLatch(){for(int transient:{EAGAIN,EBUSY}){auto c=std::make_unique<RTWUSBController>();c->initialError=transient;
 CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnBusy);CHECK(!c->core_.core_error&&!c->acCore_.io_error&&!c->acFailureEligible_&&!c->running_);
 CHECK(c->acRecoveryDiag_.stage==10&&c->acRecoveryDiag_.recoveryAttempts==0);
 c->core_.sc_ic.ic_if.if_flags|=IFF_UP;RTWUSBController::initTask8814(c.get());CHECK(!c->core_.core_error&&!c->running_);
 const auto before=c->bus.otpReads;c->initialError=0;CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 CHECK(c->rxSubmitted==4&&c->bus.otpReads==before&&c->acRecoveryDiag_.recoveryAttempts==0);}}
static void directStartCannotBypassLatchedRecovery(){auto c=failedFirstStart();const auto bulks=c->bus.bulks,rfk=c->bus.rfkStarts;c->bus.rfkFail=false;
 CHECK(c->start8814()==ETIMEDOUT);CHECK(c->bus.bulks==bulks&&c->bus.rfkStarts==rfk&&!c->rxSubmitted);
 c->core_.sc_ic.ic_if.if_flags|=IFF_UP;RTWUSBController::initTask8814(c.get());CHECK(c->bus.bulks==bulks&&c->bus.rfkStarts==rfk&&!c->rxSubmitted);}
static void finalCallbackCompletesDisabledStop(){auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 c->abortLeavesPending=true;c->ledger_.pending=1;c->quiesce(false);CHECK(c->acCore_.powered&&c->acRecoveryDiag_.cleanupResult==EBUSY);
 const auto starts=c->acRecoveryDiag_.starts,attempts=c->acRecoveryDiag_.recoveryAttempts;const auto bulks=c->bus.bulks,rfk=c->bus.rfkStarts;
 c->ledger_.pending=0;c->abortLeavesPending=false;c->finish8814StopAfterDrain();
 CHECK(!c->acCore_.powered&&!c->acCore_.fw_running&&!c->acCore_.mac_initialized&&!c->acCore_.phy_initialized&&c->acRecoveryDiag_.cleanupResult==0);
 CHECK(!c->running_&&!c->enabled_&&c->acRecoveryDiag_.starts==starts&&c->acRecoveryDiag_.recoveryAttempts==attempts&&c->bus.bulks==bulks&&c->bus.rfkStarts==rfk);}
static void deferredStopDoesNotAccessUnavailableUSB(){for(unsigned mode=0;mode<8;mode++){auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 c->abortLeavesPending=true;c->ledger_.pending=1;c->quiesce(false);c->abortLeavesPending=false;c->ledger_.pending=mode==4?1:0;
 if(mode==0)c->sleeping_=true;if(mode==1)c->closing_=true;if(mode==2)c->usbError_=kIOReturnIOError;if(mode==3)c->present_=false;if(mode==5)c->stopping_=true;
 if(mode==6)c->rxRecovery_.initialDrainMask=1;if(mode==7)c->rxRecovery_.initialDrainError=kIOReturnIOError;
 const auto reads=c->bus.reads,writes=c->bus.writes;c->finish8814StopAfterDrain();CHECK(c->bus.reads==reads&&c->bus.writes==writes&&!c->running_);}}
static void deferredStopRetainsCleanupFailure(){auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 c->abortLeavesPending=true;c->ledger_.pending=1;c->quiesce(false);c->abortLeavesPending=false;c->ledger_.pending=0;c->bus.powerOffFail=true;c->finish8814StopAfterDrain();
 CHECK(c->acRecoveryDiag_.cleanupResult==EIO&&c->acRecoveryBlocked_);const auto attempts=c->acRecoveryDiag_.recoveryAttempts;
 c->bus.powerOffFail=false;c->quiesce(false);CHECK(c->acRecoveryDiag_.cleanupResult==EIO);
 CHECK(c->request(RTW_CONTROL_SCAN)!=kIOReturnSuccess&&c->acRecoveryDiag_.recoveryAttempts==attempts);}
static void unvalidatedInitialRXBlocksBootstrap(){auto c=std::make_unique<RTWUSBController>();c->rxRecovery_.initialDrainMask=1;
 const auto reads=c->bus.reads,writes=c->bus.writes;CHECK(c->start8814()==EBUSY);CHECK(c->bus.reads==reads&&c->bus.writes==writes&&!c->rxSubmitted);}
static std::unique_ptr<RTWUSBController> periodicFixture(){
 auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess);
 c->core_.sc_ic.ic_state=IEEE80211_S_RUN;c->acMediaConnected_=c->acRateReady_=true;c->acRSSI_=52;
 c->bus.put(0x2908,4,31U<<10);return c;
}
static void advisoryRSSIBusyKeepsActualPHYAndContextHealthy(){
 auto c=periodicFixture();const auto rfk=c->bus.rfkStarts;const auto box=c->acCore_.h2c_box;
 c->bus.put(0x1cc,1,1U<<box);
 for(unsigned retry=0;retry<32;retry++){
  CHECK(c->tick8814()==0);CHECK(!c->core_.core_error&&!c->acCore_.io_error);
  CHECK(c->acCore_.phy_initialized&&c->running_&&c->radioReady_);
  CHECK(c->acCore_.h2c_box==box&&c->bus.rfkStarts==rfk);
 }
 c->publish8814RecoveryDiagnostics();
 CHECK(c->published["RTWDiagAUPeriodicCalls"]==32&&c->published["RTWDiagAUPeriodicRSSIAttempts"]==32);
 CHECK(c->published["RTWDiagAUPeriodicRSSIBusy"]==32&&c->published["RTWDiagAUPeriodicResult"]==EBUSY);
 CHECK(c->published["RTWDiagAUPeriodicPHYFailures"]==0&&c->published["RTWDiagAUPeriodicRSSIFailures"]==0);
 CHECK(c->published["RTWDiagAUPeriodicLastFailurePhase"]==0&&c->published["RTWDiagAUPeriodicLastFailureResult"]==0);
 c->bus.put(0x1cc,1,0);CHECK(c->tick8814()==0);CHECK(c->acCore_.h2c_box==((box+1)&3));
 c->publish8814RecoveryDiagnostics();CHECK(c->published["RTWDiagAUPeriodicResult"]==0);
 CHECK(c->published["RTWDiagAUPeriodicRSSIBusy"]==32&&c->published["RTWDiagAUPeriodicRSSIAttempts"]==33);
}
static void periodicHardErrorsRetainTypedPhase(){
 for(int error:{EIO,EBUSY,ETIMEDOUT})for(unsigned boundary=0;boundary<3;boundary++){
  auto c=periodicFixture();if(boundary)c->bus.failWriteAddress=(boundary==1?0x1f0:0x1d0)+c->acCore_.h2c_box*4;
  else c->bus.failReadAddress=0x1cc;
  c->bus.failureError=error;CHECK(c->tick8814()==error&&c->acCore_.io_error==error);
  c->publish8814RecoveryDiagnostics();CHECK(c->published["RTWDiagAUPeriodicRSSIFailures"]==1);
  CHECK(c->published["RTWDiagAUPeriodicLastFailurePhase"]==2&&c->published["RTWDiagAUPeriodicLastFailureResult"]==(uint32_t)error);
  CHECK(c->published["RTWDiagAUPeriodicRSSIBusy"]==0&&c->published["RTWDiagAUPeriodicPHYFailures"]==0);
 }
 for(int error:{EBUSY,EIO,ETIMEDOUT}){
  auto c=periodicFixture();c->acCore_.io_error=error;const auto reads=c->bus.reads,writes=c->bus.writes;
  CHECK(c->tick8814()==error&&c->acCore_.io_error==error);
  CHECK(c->bus.reads==reads&&c->bus.writes==writes);
  c->publish8814RecoveryDiagnostics();CHECK(c->published["RTWDiagAUPeriodicPHYFailures"]==1);
  CHECK(c->published["RTWDiagAUPeriodicLastFailurePhase"]==1&&c->published["RTWDiagAUPeriodicLastFailureResult"]==(uint32_t)error);
  CHECK(c->published["RTWDiagAUPeriodicRSSIBusy"]==0&&c->published["RTWDiagAUPeriodicRSSIAttempts"]==0);
 }
 auto c=periodicFixture();CHECK(c->tick8814()==0);c->bus.put(0x2908,4,63U<<10);c->bus.rfkFail=true;
 CHECK(c->tick8814()==ETIMEDOUT&&!c->acCore_.phy_initialized);
 c->publish8814RecoveryDiagnostics();CHECK(c->published["RTWDiagAUPeriodicPHYFailures"]==1);
 CHECK(c->published["RTWDiagAUPeriodicLastFailurePhase"]==1&&c->published["RTWDiagAUPeriodicLastFailureResult"]==ETIMEDOUT);
 CHECK(c->published["RTWDiagAUPeriodicRSSIAttempts"]==1&&c->published["RTWDiagAUPeriodicRSSIFailures"]==0);
}
static void repeatedHealthyStopPreservesProfileAndRestarts(){
 auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
 c->core_.sc_ic.ic_psk[7]=0xa3;c->quiesce(false);c->quiesce(false);
 CHECK(!c->running_&&!c->radioReady_&&!c->core_.core_error&&c->acRecoveryDiag_.cleanupResult==0);
 CHECK(c->joinRequested_&&c->core_.sc_ic.ic_des_esslen==1&&c->core_.sc_ic.ic_des_essid[0]=='x'&&c->core_.sc_ic.ic_psk[7]==0xa3);
 CHECK(c->enableStation()==kIOReturnSuccess&&c->running_&&c->radioReady_&&!c->core_.core_error);
 CHECK(c->joinRequested_&&c->core_.sc_ic.ic_psk[7]==0xa3);c->quiesce(false);
}
static void sleepingLeaveDoesNotPoisonCompletedStop(){
 auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
 c->quiesce(false);c->quiesce(false);CHECK(c->acRecoveryDiag_.cleanupResult==0&&!c->acRecoveryBlocked_);
 c->sleeping_=true;const auto reads=c->bus.reads,writes=c->bus.writes;
 CHECK(c->request(RTW_CONTROL_LEAVE)==kIOReturnSuccess);
 CHECK(c->bus.reads==reads&&c->bus.writes==writes);
 CHECK(c->acRecoveryDiag_.cleanupResult==0&&!c->acRecoveryBlocked_&&!c->core_.core_error&&!c->acCore_.io_error);
 c->sleeping_=false;CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess&&c->running_);c->quiesce(false);
}
static void sleepingLeaveRetainsPendingDrain(){
 auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
 c->abortLeavesPending=true;c->ledger_.pending=1;c->quiesce(false);c->sleeping_=true;
 CHECK(c->acRecoveryDiag_.cleanupResult==EBUSY&&c->ledger_.pending==1);
 const auto reads=c->bus.reads,writes=c->bus.writes;
 CHECK(c->request(RTW_CONTROL_LEAVE)==kIOReturnSuccess);
 CHECK(c->bus.reads==reads&&c->bus.writes==writes&&c->ledger_.pending==1);
 CHECK(c->acRecoveryDiag_.cleanupResult==EBUSY&&!c->acRecoveryBlocked_&&!c->joinRequested_);
 c->sleeping_=false;c->abortLeavesPending=false;c->ledger_.pending=0;
 CHECK(c->request(RTW_CONTROL_SCAN)==kIOReturnSuccess&&c->running_);c->quiesce(false);
}
static std::unique_ptr<RTWUSBController> actualRunningStation(){
 auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
 c->node.ni_associd=1;c->node.ni_rates.rs_nrates=1;c->node.ni_rates.rs_rates[0]=0x82;c->node.ni_bssid[0]=2;
 CHECK(c->state8814(&c->core_.sc_ic,IEEE80211_S_RUN,0)==0&&c->acMediaConnected_&&c->acRateReady_);
 return c;
}
static void powerOffDoesNotNeedFirmwareDisconnectMailbox(){
 for(bool busy:{false,true})for(unsigned box=0;box<4;++box){
  auto c=actualRunningStation();c->core_.sc_ic.ic_psk[7]=0x69;c->acCore_.h2c_box=box;
  c->bus.put(0x1cc,1,busy?0xf:0);const auto reads=c->bus.mailboxReads,writes=c->bus.mailboxWrites;
  c->quiesce(false);c->quiesce(false);
  CHECK(c->bus.mailboxReads==reads&&c->bus.mailboxWrites==writes&&c->acCore_.h2c_box==box);
  CHECK(!c->acCore_.io_error&&!c->core_.core_error&&!c->acRecoveryBlocked_&&c->acRecoveryDiag_.cleanupResult==0);
  CHECK(!c->acCore_.powered&&!c->acCore_.fw_running&&!c->running_&&c->core_.sc_ic.ic_state==IEEE80211_S_INIT);
  CHECK(c->joinRequested_&&c->core_.sc_ic.ic_psk[7]==0x69);
  c->bus.put(0x1cc,1,0);CHECK(c->enableStation()==kIOReturnSuccess&&c->running_);
  CHECK(c->joinRequested_&&c->core_.sc_ic.ic_psk[7]==0x69);c->quiesce(false);
 }
}
static void nonPowerOffDisconnectAndRunRemainMandatory(){
 for(auto state:{IEEE80211_S_INIT,IEEE80211_S_SCAN}){
  auto c=actualRunningStation();c->bus.put(0x1cc,1,0xf);
  CHECK(c->state8814(&c->core_.sc_ic,state,-1)==ETIMEDOUT&&c->acCore_.io_error==ETIMEDOUT);
 }
 {
  auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
  c->node.ni_associd=1;c->node.ni_rates.rs_nrates=1;c->node.ni_rates.rs_rates[0]=0x82;c->node.ni_bssid[0]=2;
  c->bus.put(0x1cc,1,0xf);
  CHECK(c->state8814(&c->core_.sc_ic,IEEE80211_S_RUN,0)==ETIMEDOUT&&c->acCore_.io_error==ETIMEDOUT);
  CHECK(!c->acMediaConnected_&&!c->acRateReady_);
 }
 for(int error:{EIO,EBUSY,ETIMEDOUT})for(unsigned command=0;command<3;++command){
  auto c=std::make_unique<RTWUSBController>();CHECK(c->request(RTW_CONTROL_JOIN)==kIOReturnSuccess);
  c->node.ni_associd=1;c->node.ni_rates.rs_nrates=1;c->node.ni_rates.rs_rates[0]=0x82;c->node.ni_bssid[0]=2;
  c->bus.failureError=error;c->bus.failWriteAddress=0x1f0+command*4;
  CHECK(c->state8814(&c->core_.sc_ic,IEEE80211_S_RUN,0)==error&&c->acCore_.io_error==error);
  CHECK(!c->acMediaConnected_&&!c->acRateReady_);
 }
}
static void busyMailboxStopStillPausesBeforeDeferredDrain(){
 auto c=actualRunningStation();c->bus.put(0x1cc,1,0xf);
 c->abortLeavesPending=true;c->ledger_.pending=1;c->quiesce(false);
 CHECK(c->acRecoveryDiag_.cleanupResult==EBUSY&&!c->acCore_.io_error&&!c->acRecoveryBlocked_);
 CHECK(c->bus.get(0x522,1)==0x3f&&!c->acMediaConnected_&&!c->acRateReady_&&c->acCore_.powered);
 c->abortLeavesPending=false;c->ledger_.pending=0;c->finish8814StopAfterDrain();
 CHECK(c->acRecoveryDiag_.cleanupResult==0&&!c->acCore_.powered&&!c->acCore_.io_error);
 c->bus.put(0x1cc,1,0);CHECK(c->enableStation()==kIOReturnSuccess&&c->running_);c->quiesce(false);
}
static void powerOffPreservesTypedHardwareFailures(){
 for(int error:{EIO,EBUSY,ETIMEDOUT})for(unsigned address:{0x6a4U,0x1fU}){
  auto c=actualRunningStation();c->bus.failureError=error;c->bus.failWriteAddress=address;
  c->quiesce(false);CHECK(c->acCore_.io_error==error&&!c->running_);
  c->bus.failWriteAddress=0x10000;CHECK(c->enableStation()!=kIOReturnSuccess&&c->acCore_.io_error==error);
 }
 auto failed=actualRunningStation();failed->bus.powerOffFail=true;failed->quiesce(false);
 CHECK(failed->acCore_.io_error==EIO&&failed->acRecoveryDiag_.cleanupResult==EIO&&failed->acRecoveryBlocked_);
 for(int original:{EIO,EBUSY,ETIMEDOUT}){auto c=actualRunningStation();c->acCore_.io_error=original;c->quiesce(false);CHECK(c->acCore_.io_error==original&&!c->running_);}
}
int main(){powerOffDoesNotNeedFirmwareDisconnectMailbox();nonPowerOffDisconnectAndRunRemainMandatory();busyMailboxStopStillPausesBeforeDeferredDrain();powerOffPreservesTypedHardwareFailures();sleepingLeaveRetainsPendingDrain();sleepingLeaveDoesNotPoisonCompletedStop();repeatedHealthyStopPreservesProfileAndRestarts();retryStartsFromNativeReset();joinAlsoRecovers();persistentTimeoutIsBounded();negativeGates();failedPowerOffNeverRestarts();changedRomNeverRestarts();failedIdentityReadNeverRestarts();changedChipNeverRestarts();drainMustFinish();cleanupSeparatesOriginalError();healthyStopMustDrainBeforeRestart();initialTransientDoesNotLatch();directStartCannotBypassLatchedRecovery();finalCallbackCompletesDisabledStop();deferredStopDoesNotAccessUnavailableUSB();deferredStopRetainsCleanupFailure();unvalidatedInitialRXBlocksBootstrap();
 advisoryRSSIBusyKeepsActualPHYAndContextHealthy();periodicHardErrorsRetainTypedPhase();
 std::printf("AU actual startup/control recovery: %u checks, %u failures\n",checks,failures);return failures?1:0;}
