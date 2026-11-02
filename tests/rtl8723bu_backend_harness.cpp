#include "rtw/StationRecovery.hpp"
/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual8723 controller + actual typed core. Only OS/net80211 and USB/device
 * execution boundaries are modeled; no success boot/PHY/firmware stubs. */
#include "rtw/RTL8723BUDataPath.h"
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/USBDescriptors.h"
#include "rtw/SignalSample.h"
#include <memory>
/* ACTUAL_CORE_USB_MODEL */
/* Private OS boundary: kept independent of other chip harnesses. Stack flag
 * values and default EDCA parameters are imported by this runner. */
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
/* ACTUAL_STACK_FLAGS */
enum {IEEE80211_M_STA=1};
struct ieee80211_frame {uint8_t i_fc[2],i_dur[2],i_addr1[6],i_addr2[6],i_addr3[6],i_seq[2];};
static_assert(sizeof(ieee80211_frame)==24,"wireheader");
struct ieee80211_rateset {uint8_t rs_nrates,rs_rates[32];};
struct ieee80211_channel {int number;unsigned ic_flags=0;};

#define IEEE80211_CHAN_ANYC ((ieee80211_channel*)-1)
struct ieee80211_node {ieee80211_channel*ni_chan;uint8_t ni_bssid[6];uint16_t ni_associd,ni_intval;ieee80211_rateset ni_rates;bool ni_port_valid=false;unsigned ni_flags=0,ni_htcaps=0;uint8_t ni_rxmcs[2]{};};
enum{IFF_RUNNING=1,IFF_UP=2};
struct _ifnet {unsigned if_flags=0,if_timer=0;int if_snd=0;};
static void ifq_clr_oactive(int*){}
static const int systq=0;static void task_del(int,int*){}
enum {IEEE80211_MODE_11B,IEEE80211_MODE_11A,IEEE80211_MODE_11G};
struct ieee80211_edca_ac_params{uint8_t ac_ecwmin,ac_ecwmax,ac_aifsn;uint16_t ac_txoplimit;};
/* ACTUAL_DEFAULT_EDCA */
struct ieee80211com {void*ic_softc;int ic_opmode;ieee80211_state ic_state;_ifnet ic_if;ieee80211_channel*ic_ibss_chan;int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;unsigned ic_flags,ic_htcaps=0,ic_rtsthreshold=2347,ic_protmode=0;ieee80211_node*ic_bss;ieee80211_edca_ac_params*ic_edca_ac=nullptr;};
struct rtwn_softc {ieee80211com sc_ic{};int core_error=0;unsigned sc_tx_timer=0;int init_task=0;struct{void*cookie=nullptr;}sc_ops;int(*sc_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;};

struct TestPacket {std::vector<uint8_t> bytes;};using mbuf_t=TestPacket*;
static unsigned frees=0,releases=0,cryptoCalls=0;static bool keyAvailable=false,encryptSuccess=false;
static size_t mbuf_pkthdr_len(mbuf_t p){return p->bytes.size();}
static int mbuf_copydata(mbuf_t p,size_t off,size_t n,void*out){if(off>p->bytes.size()||n>p->bytes.size()-off)return EINVAL;memcpy(out,p->bytes.data()+off,n);return 0;}
static void mbuf_freem(mbuf_t p){if(p){++frees;delete p;}}
struct TestKey{unsigned k_cipher=IEEE80211_CIPHER_CCMP;};static TestKey key;
static TestKey*ieee80211_get_txkey(ieee80211com*,ieee80211_frame*,ieee80211_node*){return keyAvailable?&key:nullptr;}
static mbuf_t ieee80211_encrypt(ieee80211com*,mbuf_t p,TestKey*){++cryptoCalls;if(encryptSuccess){p->bytes.resize(p->bytes.size()+16,0x5a);return p;}mbuf_freem(p);return nullptr;}
static void ieee80211_release_node(ieee80211com*,ieee80211_node*){++releases;}
using IOInterruptState=int;static int IOSimpleLockLockDisableInterrupt(void*){return 0;}static void IOSimpleLockUnlockEnableInterrupt(void*,int){}
constexpr int kIOReturnSuccess=0;
struct TestMemory{uint8_t b[4352]{};void*getBytesNoCopy(){return b;}};struct TestPipe{unsigned number;unsigned getSpeed()const{return 3;}};


static int ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return c->number;}
static void*IOMalloc(size_t n){return std::calloc(1,n);}static void IOFree(void*p,size_t){std::free(p);}
static std::vector<int>events;static int genericFail=-1;static unsigned initFail=0;
constexpr unsigned kUSBHostConnectionSpeedHigh=3,kUSBHostConnectionSpeedFull=2;
class RTWUSBController{public:

 fake_8723 hw;rtwn_softc core_{};rtw_wifi_family family_=RTW_WIFI_RTL8723BU;const rtw_usb_wifi_profile*profile_=nullptr;
 bool running_=false,stopping_=false,present_=true,radioReady_=false,sleeping_=false;int usbError_=0;
 void*completionLock_=nullptr;rtw_transfer_ledger ledger_{};rtw_usb_endpoints endpoints_{};uint16_t txMPS_[4]{};TestPipe pipes[4]{};TestPipe*txPipes_[4]{};TestPipe*rxPipe_=&pipes[0];
 struct Transfer{TestMemory*memory;};Transfer transfers_[16]{};TestMemory memories[16]{};uint64_t txHTPackets_=0;
 enum{RXSlots=4,TXCapacity=4352};struct{unsigned invalid=0,ignored=0,reports=0;uint8_t finalRate=0;}rateControl_;
#include "RTW8723BUBackendMembers.inc"
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
 static int ioError(void*p){auto*s=owner(p);return !s->present_||s->sleeping_||s->usbError_?EIO:0;}
 uint32_t readRegister(uint16_t a,uint8_t w){uint32_t value=0;if(ioError(this))return 0;int e=fake_8723::read(&hw,a,w,&value);if(e)usbError_=e;return value;}
 int writeRegister(uint16_t a,uint8_t w,uint32_t v){if(ioError(this))return EIO;int e=fake_8723::write(&hw,a,w,v);if(e)usbError_=e;return e;}
 static int writeRegion(void*p,uint16_t a,const uint8_t*b,size_t n){auto*s=owner(p);CHECK(n<=196);if(ioError(s))return EIO;int e=fake_8723::region(&s->hw,a,b,n);if(e)s->usbError_=e;return e;}
 static void delayUS(void*p,uint32_t us){fake_8723::delay(&owner(p)->hw,us);}
 static void cancelScan(void*){events.push_back(52);}static void cancelCalibration(void*){events.push_back(53);}static void nextScan(void*){events.push_back(54);}static void nextCalibration(void*){events.push_back(55);}
 bool authorized()const{return radioReady_&&present_&&running_&&!stopping_&&!core_.core_error&&uses8723FirmwareRate()&&core_.sc_ic.ic_bss->ni_port_valid;}
 int allocateTransfers(){events.push_back(1);return initFail==1?EIO:0;}void abortTransfers(){events.push_back(90);running_=radioReady_=false;}void freeTransfers(){events.push_back(92);}
 static int busInitialize(void*p){auto*s=owner(p);events.push_back(14);s->running_=initFail!=14;return initFail==14?EIO:0;}
 unsigned submitted=0,submittedPipe=0;std::vector<uint8_t>submittedBytes;int submitError=0;
 int submit(unsigned index,TestPipe*pipe,uint32_t length){++submitted;submittedPipe=pipe->number;submittedBytes.assign(memories[index].b,memories[index].b+length);return submitError;}

#include "family_recovery_backend_host.hpp"
};
static RTWUSBController*from_ic(ieee80211com*ic){return static_cast<RTWUSBController*>(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie);}
static int generic(ieee80211com*ic,ieee80211_state state,int){events.push_back(60+state);if(genericFail==state)return EINVAL;auto*s=from_ic(ic);if(state==IEEE80211_S_RUN)CHECK(!s->r23RateReady_&&!s->authorized());else if(ic->ic_bss)ic->ic_bss->ni_port_valid=false;ic->ic_state=state;return 0;}
static int route(ieee80211com*ic,ieee80211_state state,int arg){return from_ic(ic)->state8723(ic,state,arg);}
/* ACTUAL_BACKEND_METHODS */
struct Fixture{
 RTWUSBController c;ieee80211_channel channel{6,0};ieee80211_node peer{};ieee80211_edca_ac_params edca[4]{};
 Fixture(unsigned pipes=2){memcpy(edca,ieee80211_edca_table[IEEE80211_MODE_11G],sizeof(edca));events.clear();genericFail=-1;initFail=0;c.hw.simulate=true;
  for(size_t i=0;i<rtw_usb_wifi_profile_count();i++){auto*p=rtw_usb_wifi_profile_at(i);if(p->family==RTW_WIFI_RTL8723BU&&p->primary_verified&&p->vendor==(pipes==4?0x7392:0xbda)&&p->product==(pipes==4?0xa611:0xb720)){c.profile_=p;break;}}CHECK(c.profile_);
  c.endpoints_.num_bulk_in=1;c.endpoints_.bulk_in[0]=0x81;c.endpoints_.num_bulk_out=uint8_t(pipes);for(unsigned i=0;i<pipes&&i<4;++i)c.endpoints_.bulk_out[i]=uint8_t(2+i);
  for(unsigned i=0;i<pipes&&i<4;i++){c.pipes[i].number=i;c.txPipes_[i]=&c.pipes[i];c.txMPS_[i]=512;}for(unsigned i=0;i<16;i++)c.transfers_[i].memory=&c.memories[i];
  fake_8723::write(&c.hw,0xf0,4,2U<<12);fake_8723::write(&c.hw,0x68,4,4U|(1U<<18));std::array<uint8_t,512>rom;valid_board_image(rom);encode_otp(rom,c.hw.otp);
  c.r23PrivateSize_=rtw_8723bu_private_size();c.r23Storage_=IOMalloc(c.r23PrivateSize_);CHECK(c.r23Storage_&&c.bootstrap8723(false)==0);
  peer.ni_chan=&channel;peer.ni_associd=0xc001;peer.ni_intval=100;peer.ni_bssid[0]=2;peer.ni_bssid[5]=1;peer.ni_rates={3,{0x82,12,108}};peer.ni_flags=IEEE80211_NODE_HT;peer.ni_htcaps=IEEE80211_HTCAP_SGI20;peer.ni_rxmcs[0]=255;
  auto&ic=c.core_.sc_ic;ic.ic_softc=&c.core_;ic.ic_edca_ac=edca;c.core_.sc_ops.cookie=&c;ic.ic_opmode=IEEE80211_M_STA;ic.ic_bss=&peer;ic.ic_ibss_chan=&channel;ic.ic_newstate=route;ic.ic_htcaps=IEEE80211_HTCAP_SGI20;ic.ic_flags=IEEE80211_F_SHSLOT|IEEE80211_F_QOS;c.core_.sc_newstate=generic;
 }
 ~Fixture(){c.release8723Storage();}
 int state(ieee80211_state value){return c.state8723(&c.core_.sc_ic,value,-1);}
 void start(){CHECK(c.start8723()==0&&c.running_&&c.radioReady_&&!c.authorized());}
 void run(){start();CHECK(state(IEEE80211_S_AUTH)==0);CHECK(state(IEEE80211_S_ASSOC)==0);CHECK(state(IEEE80211_S_RUN)==0);}
};
static mbuf_t packet(bool data,bool eapol,bool qos=false,size_t length=80){auto*p=new TestPacket;p->bytes.assign(length,0);auto*h=(ieee80211_frame*)p->bytes.data();h->i_fc[0]=data?uint8_t(8|(qos?0x80:0)):0x40;h->i_addr1[0]=2;h->i_seq[0]=0x10;if(eapol){const uint8_t llc[8]={0xaa,0xaa,3,0,0,0,0x88,0x8e};memcpy(p->bytes.data()+24+(qos?2:0),llc,8);}return p;}
static uint32_t register_word(const Fixture&f,unsigned a){uint32_t value=0;for(unsigned i=0;i<4;++i)value|=uint32_t(f.c.hw.memory[a+i])<<(8*i);return value;}
static void edca_policy(){
 const unsigned address[4]={0x508,0x50c,0x504,0x500};
 for(bool legacy_b:{false,true})for(bool short_slot:{false,true})for(bool local_qos:{false,true})for(bool peer_qos:{false,true}){
  Fixture f;if(legacy_b)f.peer.ni_rates={2,{0x82,22}};f.peer.ni_flags=peer_qos?IEEE80211_NODE_QOS:0;
  f.c.core_.sc_ic.ic_flags=(local_qos?IEEE80211_F_QOS:0U)|(short_slot?IEEE80211_F_SHSLOT:0U);
  for(auto&ac:f.edca)ac={5,10,4,0x123};f.run();
  const auto*expected=local_qos&&peer_qos?f.edca:ieee80211_edca_table[legacy_b?IEEE80211_MODE_11B:IEEE80211_MODE_11G];
  for(unsigned i=0;i<4;++i){const auto&p=expected[i];uint32_t word=uint32_t(p.ac_txoplimit)<<16|uint32_t(p.ac_ecwmax)<<12|uint32_t(p.ac_ecwmin)<<8|uint32_t(10+(short_slot?9:20)*p.ac_aifsn);CHECK(register_word(f,address[i])==word);}
  // A prior TXOP's high bits belong to that parameter, not to a 3081 profile.
  f.c.hw.memory[0x50b]=0xf8;RTWUSBController::update8723EDCA(&f.c.core_.sc_ic);CHECK(register_word(f,0x508)>>27==0);
 }
}
static void firmware_health(){
 for(unsigned byte=0;byte<256;++byte){Fixture f;f.run();f.c.hw.fw_ready=false;f.c.hw.memory[0x80]=uint8_t(byte);int result=f.c.tick8723();
  if((byte&0xc7U)==0xc6U)CHECK(result==0&&f.c.r23Core_.firmware_running&&f.c.r23Core_.calibrated);
  else{CHECK(result==EIO&&!f.c.r23Core_.firmware_running&&!f.c.r23Core_.calibrated);auto before=f.c.hw.calls;CHECK(f.c.tick8723()==ENXIO&&f.c.hw.calls==before);}
 }
 unsigned tick_io=0;{Fixture f;f.run();f.c.r23Core_.rssi_valid=1;f.c.r23Core_.rssi_percent=50;auto before=f.c.hw.calls;CHECK(f.c.tick8723()==0);tick_io=f.c.hw.calls-before;}
 for(unsigned cut=1;cut<=tick_io;++cut){Fixture f;f.run();f.c.r23Core_.rssi_valid=1;f.c.r23Core_.rssi_percent=50;f.c.hw.fail_at=f.c.hw.calls+cut;CHECK(f.c.tick8723()==EIO);CHECK(!f.c.r23Core_.firmware_running&&!f.c.r23Core_.calibrated&&!f.c.r23RateReady_&&!f.c.uses8723FirmwareRate());}
}
static void sparse_peer_rates(){
 for(unsigned mcs:{2U,0x20U,0x80U,0xfeU})for(bool sgi:{false,true}){
  Fixture f;f.peer.ni_rxmcs[0]=uint8_t(mcs);if(!sgi)f.peer.ni_htcaps&=~unsigned(IEEE80211_HTCAP_SGI20);f.run();
  CHECK((f.c.r23Rates_.mask>>12)==mcs&&f.c.r23Rates_.sgi==sgi);CHECK(f.c.hw.memory[0x6a2]==0&&f.c.hw.memory[0x6a3]==1&&!(f.c.hw.memory[0x609]&0x10U));
  bool configured=false;for(const auto&cmd:f.c.hw.h2c)if(cmd[0]==0x40&&cmd[1]==2){uint32_t mask=uint32_t(cmd[4])|uint32_t(cmd[5])<<8|uint32_t(cmd[6])<<16|uint32_t(cmd[7])<<24;configured|=(mask>>12)==mcs;}CHECK(configured);
  f.peer.ni_port_valid=true;CHECK(f.c.send8723Frame(packet(true,false,true),&f.peer)==0);unsigned rate=f.c.submittedBytes[16]&127U;CHECK(rate>=12&&rate<=19&&(mcs&(1U<<(rate-12))));
 }
}
// Catches a fourth physical pipe being dropped at bootstrap or used as
// an unprogrammed DMA queue. Exercises the production state and TX methods.
static void four_pipe_station_and_rollback(){
 {Fixture f(4);f.run();f.peer.ni_port_valid=true;
  const unsigned tids[]={7,5,0};const unsigned pipes[]={0,1,2};
  for(unsigned i=0;i<3;++i){auto*p=packet(true,false,true);p->bytes[24]=uint8_t(tids[i]);CHECK(f.c.send8723Frame(p,&f.peer)==0&&f.c.submittedPipe==pipes[i]);}
  CHECK(f.c.send8723Frame(packet(false,false),&f.peer)==0&&f.c.submittedPipe==0);
  f.c.stop8723();CHECK(!f.c.running_&&!f.c.authorized()&&!f.c.r23Core_.firmware_running&&!f.peer.ni_port_valid);f.start();
 }
 for(unsigned bad=0;bad<3;++bad){Fixture f(4);auto before=f.c.hw.calls;
  if(bad==0)f.c.txPipes_[3]=nullptr;if(bad==1)f.c.txMPS_[3]=64;if(bad==2)f.c.endpoints_.num_bulk_out=5;
  CHECK(f.c.bootstrap8723(false)==(bad==2?ENXIO:EINVAL)&&f.c.hw.calls==before);
 }
 unsigned start_io=0;{Fixture f(4);auto before=f.c.hw.calls;f.start();start_io=f.c.hw.calls-before;}
 for(unsigned cut=1;cut<=start_io;++cut){Fixture f(4);f.c.hw.fail_at=f.c.hw.calls+cut;
  CHECK(f.c.start8723()==EIO);CHECK(!f.c.authorized()&&!f.c.running_&&!f.c.r23RateReady_&&!f.c.r23Core_.calibrated&&!f.c.r23Core_.firmware_running);
 }
 std::printf("8723BU four physical pipes: three DMA queue TX routes, restart and all %u startup I/O failures passed\n",start_io);
}
int main(){
 four_pipe_station_and_rollback();
 edca_policy();
 firmware_health();
 sparse_peer_rates();
 {Fixture f;f.run();CHECK(f.c.r23Rates_.mask==(0xff000U|0x811U)&&f.c.r23Rates_.raid==3&&f.c.r23RateReady_&&f.c.r23Core_.associated&&!f.c.authorized());
  CHECK(f.c.send8723Frame(packet(true,false,true),&f.peer)==ENETDOWN);CHECK(f.c.send8723Frame(packet(true,true,true,472),&f.peer)==0);CHECK(f.c.submittedBytes.size()==513&&(f.c.submittedBytes[13]&1));
  f.peer.ni_port_valid=true;CHECK(f.c.authorized());CHECK(f.c.send8723Frame(packet(true,false,true),&f.peer)==0);CHECK((f.c.submittedBytes[4]&127)==2&&!(f.c.submittedBytes[13]&1)&&f.c.txHTPackets_==1);
  encryptSuccess=keyAvailable=true;auto*p=packet(true,false,true);p->bytes[1]|=IEEE80211_FC1_PROTECTED;CHECK(f.c.send8723Frame(p,&f.peer)==0);CHECK(f.c.submittedBytes.size()==136&&!(f.c.submittedBytes[6]&0xc0));encryptSuccess=false;
  auto before=f.c.hw.calls;f.c.r23Core_.calibrated=0;CHECK(f.c.send8723Frame(packet(false,false),&f.peer)==ENETDOWN&&f.c.hw.calls==before);f.c.r23Core_.calibrated=1;
  uint8_t report[]={0xc,0,0x93,2,0,0,0,0,0};f.c.handle8723C2H(report,sizeof(report));CHECK(f.c.rateControl_.reports==1&&f.c.rateControl_.finalRate==19);
  CHECK(f.c.tick8723()==0);f.c.stop8723();CHECK(!f.peer.ni_port_valid&&!f.c.running_&&!f.c.r23Core_.calibrated&&!f.c.r23Core_.firmware_running);
 }
 {Fixture f;f.start();f.c.hw.writes.clear();CHECK(f.state(IEEE80211_S_AUTH)==0);bool rates=false;for(const auto&w:f.c.hw.writes){if(w.address==0x440)rates=true;if(w.address==0x522&&!(w.value&0x10U))CHECK(rates);}}
 unsigned auth_io=0,run_io=0;
 {Fixture f;f.start();auto before=f.c.hw.calls;CHECK(f.state(IEEE80211_S_AUTH)==0);auth_io=f.c.hw.calls-before;before=f.c.hw.calls;CHECK(f.state(IEEE80211_S_RUN)==0);run_io=f.c.hw.calls-before;}
 for(unsigned cut=1;cut<=auth_io;cut++){Fixture f;f.start();f.c.hw.fail_at=f.c.hw.calls+cut;CHECK(f.state(IEEE80211_S_AUTH)==EIO);CHECK(!f.c.authorized()&&!f.c.r23RateReady_&&!f.c.r23Core_.calibrated&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 for(unsigned cut=1;cut<=run_io;cut++){Fixture f;f.start();CHECK(f.state(IEEE80211_S_AUTH)==0);f.c.hw.fail_at=f.c.hw.calls+cut;CHECK(f.state(IEEE80211_S_RUN)==EIO);CHECK(!f.c.authorized()&&!f.c.r23RateReady_&&!f.c.r23Core_.associated);}
 for(bool gone:{false,true}){Fixture f;f.run();f.peer.ni_port_valid=true;if(gone)f.c.present_=false;else f.c.sleeping_=true;auto before=f.c.hw.calls;f.c.stop8723();CHECK(f.c.hw.calls==before&&!f.peer.ni_port_valid&&!f.c.authorized()&&!f.c.r23Core_.powered);}
 {Fixture f;f.run();f.peer.ni_port_valid=true;f.channel.ic_flags|=IEEE80211_CHAN_PASSIVE;CHECK(f.state(IEEE80211_S_SCAN)==0);CHECK(!f.c.r23Core_.associated&&!f.c.r23Core_.calibrated&&!f.peer.ni_port_valid&&f.c.hw.memory[0x522]==255);CHECK((f.c.hw.memory[0x608]&0xc0)==0);}
 for(unsigned stage:{1U,14U}){Fixture f;initFail=stage;CHECK(f.c.start8723()==EIO&&!f.c.running_&&!f.c.radioReady_);}
 {Fixture f;f.start();genericFail=IEEE80211_S_RUN;CHECK(f.state(IEEE80211_S_AUTH)==0);CHECK(f.state(IEEE80211_S_RUN)==EINVAL&&!f.c.authorized()&&!f.c.r23RateReady_);}
 std::printf("RTL8723BU actual controller/core: %u checks,all %u AUTH+%u RUN I/O failures,CCMP/MACID2/port/lifecycle; USB/OS boundary only\n",checks,auth_io,run_io);
}
