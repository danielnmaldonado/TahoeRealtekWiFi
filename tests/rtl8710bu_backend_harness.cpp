#include "rtw/StationRecovery.hpp"
/* SPDX-License-Identifier: BSD-3-Clause
 * Executes actual controller state/TX/RX methods with isolated USB, RF and
 * host scheduling boundaries. It does not emulate an AP or prove RF/CCMP. */
#include "rtw/RTL8710BU.h"
#include "rtl8710bu_model.hpp"
#include "rtw/USBPackets.h"
#include "rtw/RTL8192EU.h"
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/USBDescriptors.h"
#include "rtw/USBWiFiProfiles.h"
#include "rtw/SignalSample.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sys/types.h>


enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_M_STA=1,IEEE80211_F_SHSLOT=0x20000,IEEE80211_RATE_BASIC=128,IEEE80211_RATE_VAL=127,
 IEEE80211_F_SHPREAMBLE=1,IEEE80211_F_USEPROT=2,IEEE80211_PROT_CTSONLY=1,IEEE80211_PROT_RTSCTS=2,
 IEEE80211_FC0_TYPE_MASK=0x0c,IEEE80211_FC0_TYPE_DATA=8,IEEE80211_FC0_SUBTYPE_QOS=0x80,
 IEEE80211_FC1_DIR_MASK=3,IEEE80211_FC1_DIR_DSTODS=3,IEEE80211_FC1_PROTECTED=0x40,IEEE80211_CIPHER_CCMP=4};
enum {IEEE80211_MODE_AUTO=0,IEEE80211_MODE_11N=5,IEEE80211_MODE_11AC=6,IEEE80211_MODE_11AX=7,IEEE80211_MODE_11G=3,IEEE80211_MODE_11B=2,IEEE80211_F_QOS=0x800000,IEEE80211_NODE_QOS=0x10,IEEE80211_F_HTON=0x10000,IEEE80211_F_VHTON=0x20000,IEEE80211_F_HEON=0x40000,IEEE80211_F_NOVHT=16,IEEE80211_NODE_HT=1,IEEE80211_NODE_VHT=0x10000,IEEE80211_CHAN_WIDTH_20_NOHT=0,IEEE80211_CHAN_WIDTH_20=1,IEEE80211_CHAN_WIDTH_40=2,IEEE80211_CHAN_WIDTH_80=3,
IEEE80211_HTCAP_SMPS_MASK=12,IEEE80211_HTCAP_SMPS_SHIFT=2,IEEE80211_HTCAP_SMPS_DIS=3,IEEE80211_HTCAP_CBW20_40=2,IEEE80211_CHAN_HT40=0x60000,IEEE80211_HTCAP_SGI20=0x20,IEEE80211_HTCAP_SGI40=0x40,IEEE80211_VHTCAP_SHORT_GI_80=0x20,IEEE80211_HTOP0_SCO_MASK=3,IEEE80211_HTOP0_SCO_SCA=1,IEEE80211_HTOP0_SCO_SCB=3,
IEEE80211_CHAN_HT20=0x10000,IEEE80211_CHAN_HT40U=0x20000,IEEE80211_CHAN_HT40D=0x40000,IEEE80211_CHAN_VHT20=0x1000000,IEEE80211_CHAN_VHT40U=0x2000000,IEEE80211_CHAN_VHT40D=0x4000000,IEEE80211_CHAN_VHT80=0x8000000,
IEEE80211_F_RSNON=0x1000,IEEE80211_PROTO_RSN=1,IEEE80211_BA_AGREED=2,IEEE80211_BA_MAX_WINSZ=64,IEEE80211_QOS_TID=15,IEEE80211_AMPDU_PARAM_LE=3,IEEE80211_AMPDU_PARAM_SS=28};

struct ieee80211_ba {int ba_state=0;unsigned ba_winsize=0;void*ba_buf=nullptr;};
struct TestVHT {uint16_t rx_mcs_map=0xffff,tx_mcs_map=0xffff;};
struct ieee80211_frame {uint8_t i_fc[2],i_dur[2],i_addr1[6],i_addr2[6],i_addr3[6],i_seq[2];};
static_assert(sizeof(ieee80211_frame)==24,"wireheader");
struct ieee80211_rateset {uint8_t rs_nrates,rs_rates[32];};
struct ieee80211_channel {int number;unsigned ic_flags=0;};
enum {IEEE80211_CHAN_5GHZ=0x100,IEEE80211_CHAN_PASSIVE=0x200};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel*)-1)
struct ieee80211_node {unsigned ni_flags=0,ni_htcaps=0,ni_vhtcaps=0,ni_rsnprotos=0;uint8_t ni_rxmcs[10]{},ni_chw=IEEE80211_CHAN_WIDTH_20,ni_htop0=0,ni_vht_chan1=0,ni_ampdu_param=0;TestVHT ni_vht_mcsinfo;ieee80211_ba ni_tx_ba[16]{},ni_rx_ba[16]{};ieee80211_channel*ni_chan;uint8_t ni_bssid[6];uint16_t ni_associd,ni_intval;ieee80211_rateset ni_rates;bool ni_port_valid=false;};
enum{IFF_RUNNING=1,IFF_UP=2};
struct _ifnet {unsigned if_flags=0,if_timer=0;int if_snd=0;};
static void ifq_clr_oactive(int*){}
static const int systq=0;static void task_del(int,int*){}
struct TestEdca{uint8_t ac_aifsn=2,ac_ecwmin=3,ac_ecwmax=4;uint16_t ac_txoplimit=0;};
static const TestEdca ieee80211_edca_table[8][4]={};
struct ieee80211com {ieee80211_channel ic_channels[256]{};TestEdca ic_edca_ac[4]{};unsigned ic_htcaps=0,ic_vhtcaps=0,ic_tid_noack=0,ic_modecaps=0,ic_curmode=0,ic_userflags=0;void*ic_softc;int ic_opmode;ieee80211_state ic_state;_ifnet ic_if;ieee80211_channel*ic_ibss_chan;int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;unsigned ic_flags,ic_rtsthreshold=2347,ic_protmode=0;ieee80211_node*ic_bss;};
struct rtwn_softc {struct{void*cookie=nullptr;}sc_ops;ieee80211com sc_ic{};int core_error=0;unsigned sc_tx_timer=0;int init_task=0;int(*sc_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;};

struct TestPacket {std::vector<uint8_t> bytes;};using mbuf_t=TestPacket*;
static unsigned frees=0,releases=0,cryptoCalls=0;static bool keyAvailable=false,encryptSuccess=false;
static size_t mbuf_pkthdr_len(mbuf_t p){return p->bytes.size();}
static int mbuf_copydata(mbuf_t p,size_t off,size_t n,void*out){if(off>p->bytes.size()||n>p->bytes.size()-off)return EINVAL;memcpy(out,p->bytes.data()+off,n);return 0;}
static void mbuf_freem(mbuf_t p){if(p){++frees;delete p;}}
struct TestKey{unsigned k_cipher=IEEE80211_CIPHER_CCMP;};static TestKey key;
static TestKey*ieee80211_get_txkey(ieee80211com*,ieee80211_frame*,ieee80211_node*){return keyAvailable?&key:nullptr;}
static mbuf_t ieee80211_encrypt(ieee80211com*,mbuf_t p,TestKey*){++cryptoCalls;if(encryptSuccess)return p;mbuf_freem(p);return nullptr;}
static void ieee80211_release_node(ieee80211com*,ieee80211_node*){++releases;}
using IOInterruptState=int;static int IOSimpleLockLockDisableInterrupt(void*){return 0;}static void IOSimpleLockUnlockEnableInterrupt(void*,int){}
constexpr int kIOReturnSuccess=0;
struct TestMemory{uint8_t b[4352]{};void*getBytesNoCopy(){return b;}};enum {kUSBHostConnectionSpeedFull=1,kUSBHostConnectionSpeedHigh=3};
struct TestPipe{unsigned number;unsigned getSpeed()const{return kUSBHostConnectionSpeedHigh;}};
static std::vector<int> events;static int failEvent=0,genericFail=-1;


static int event(int e){events.push_back(e);return failEvent==e?EIO:0;}
static int ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return c->number;}
class RTWUSBController {public:

 rtwn_softc core_{};BUModel*model=nullptr;rtw_wifi_family family_=RTW_WIFI_RTL8710BU;TestPipe receive{};TestPipe*rxPipe_=&receive;bool running_=true,stopping_=false,present_=true,radioReady_=true,sleeping_=false;int usbError_=0;
 uint64_t txHTPackets_=0,txAMPDUPackets_=0;
 void*completionLock_=nullptr;rtw_transfer_ledger ledger_{};rtw_usb_endpoints endpoints_{};uint16_t txMPS_[4]{};TestPipe pipes[4]{};TestPipe*txPipes_[4]{};
 struct Transfer{TestMemory*memory;};Transfer transfers_[16]{};TestMemory memories[16]{};
 enum{RXSlots=4,TXCapacity=4352};struct{unsigned invalid=0,ignored=0,reports=0;uint8_t finalRate=0;}rateControl_;
#include "../kext/RTW8710BUBackendMembers.inc"
 int vendorRequest(bool input,uint16_t address,void*bytes,uint16_t count){rtw_usb_setup setup{(uint8_t)(input?0xc0:0x40),5,address,0,count,1000};uint16_t actual=0;rtw_usb_error result=input?BUModel::in(model,&setup,(uint8_t*)bytes,&actual):BUModel::out(model,&setup,(const uint8_t*)bytes,&actual);return result==RTW_USB_OK&&actual==count?0:EIO;}
 static int writeRegion(void*p,uint16_t address,const uint8_t*bytes,uint16_t count){return owner(p)->vendorRequest(false,address,const_cast<uint8_t*>(bytes),count);}
 static void delayUS(void*,uint32_t){}
 static void cancelScan(void*){events.push_back(52);}static void cancelCalibration(void*){events.push_back(53);}
 static void nextScan(void*){events.push_back(54);}static void nextCalibration(void*){events.push_back(55);}

 bool authorized()const{return radioReady_&&present_&&running_&&!stopping_&&!core_.core_error&&uses8710BUFirmwareRate()&&core_.sc_ic.ic_bss->ni_port_valid;}
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
 static int ioError(void*p){auto*s=owner(p);return !s->present_||s->sleeping_||s->usbError_?EIO:0;}
 int allocateTransfers(){return event(1);}void abortTransfers(){events.push_back(90);running_=radioReady_=false;}void freeTransfers(){events.push_back(92);}
 static int busInitialize(void*p){auto*s=owner(p);int e=event(14);s->running_=!e;return e;}
 unsigned submitted=0,submittedPipe=0,submittedIndex=0;std::vector<uint8_t>submittedBytes;int submitError=0;
 int submit(unsigned index,TestPipe*pipe,uint32_t len){++submitted;submittedIndex=index;submittedPipe=pipe->number;submittedBytes.assign(memories[index].b,memories[index].b+len);return submitError;}

#include "family_recovery_backend_host.hpp"
};
static int generic(ieee80211com*ic,ieee80211_state state,int){events.push_back(60+state);if(genericFail==state)return EINVAL;if(state==IEEE80211_S_RUN)assert(!static_cast<RTWUSBController*>(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie)->buRateReady_);else if(ic->ic_bss)ic->ic_bss->ni_port_valid=false;ic->ic_state=state;return 0;}
static int route(ieee80211com*ic,ieee80211_state state,int argument){return static_cast<RTWUSBController*>(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie)->state8710BU(ic,state,argument);}
/* ACTUAL_BACKEND_METHODS */
struct Fixture{
 BUModel m;RTWUSBController c;std::vector<std::max_align_t>storage;ieee80211_channel channel{6,0};ieee80211_node n{};
 Fixture(unsigned pipes=3):storage((rtw_r10bu_size()+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t)){
 events.clear();failEvent=0;genericFail=-1;m.packed(bu_rom(),0xfe);c.model=&m;c.buStorage_=storage.data();c.buSize_=storage.size()*sizeof(std::max_align_t);c.running_=c.radioReady_=false;c.endpoints_.num_bulk_in=1;c.endpoints_.bulk_in[0]=0x81;c.endpoints_.num_bulk_out=uint8_t(pipes);uint8_t eps[3]={3,1,2};for(unsigned i=0;i<pipes;++i){c.endpoints_.bulk_out[i]=eps[i];c.pipes[i].number=i;c.txPipes_[i]=&c.pipes[i];c.txMPS_[i]=512;}
 for(unsigned i=0;i<16;++i)c.transfers_[i].memory=&c.memories[i];c.core_.sc_ops.cookie=&c;c.core_.sc_ic.ic_softc=&c.core_;c.core_.sc_ic.ic_opmode=IEEE80211_M_STA;c.core_.sc_ic.ic_bss=&n;c.core_.sc_ic.ic_ibss_chan=&channel;c.core_.sc_newstate=generic;c.core_.sc_ic.ic_newstate=route;n.ni_chan=&channel;n.ni_bssid[0]=2;n.ni_associd=0xc001;n.ni_intval=100;n.ni_rates={3,{0x82,12,108}};c.buHTSupported_=true;assert(!c.initialize8710BUCore());
 }
 int state(ieee80211_state s){return c.state8710BU(&c.core_.sc_ic,s,-1);}
 void start(){assert(!c.start8710BU());}
 void run(bool ht=false,unsigned width=20){start();assert(!state(IEEE80211_S_AUTH));if(ht){n.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_QOS;n.ni_rxmcs[0]=n.ni_rxmcs[1]=255;n.ni_htcaps=c.core_.sc_ic.ic_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);n.ni_chw=width==40?IEEE80211_CHAN_WIDTH_40:IEEE80211_CHAN_WIDTH_20;n.ni_htop0=IEEE80211_HTOP0_SCO_SCA;}assert(!state(IEEE80211_S_RUN));}
};
static mbuf_t packet(bool eapol=false,bool qos=false){auto*p=new TestPacket;p->bytes.resize(qos?42:40);p->bytes[0]=uint8_t(8|(qos?0x80:0));p->bytes[4]=2;unsigned start=qos?26:24;uint8_t llc[8]={0xaa,0xaa,3,0,0,0,uint8_t(eapol?0x88:8),uint8_t(eapol?0x8e:0)};memcpy(p->bytes.data()+start,llc,8);return p;}
int main(){

 for(unsigned smps:{0U,1U,2U,3U})for(unsigned width:{20U,40U}){
  Fixture f;f.start();assert(!f.state(IEEE80211_S_AUTH));f.n.ni_flags=IEEE80211_NODE_HT;f.n.ni_rxmcs[0]=f.n.ni_rxmcs[1]=255;f.n.ni_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40|(smps<<IEEE80211_HTCAP_SMPS_SHIFT);f.c.core_.sc_ic.ic_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;f.n.ni_chw=width==40?IEEE80211_CHAN_WIDTH_40:IEEE80211_CHAN_WIDTH_20;f.n.ni_htop0=IEEE80211_HTOP0_SCO_SCA;
  assert(!f.state(IEEE80211_S_RUN));bool two=false;assert((f.c.buRate_.mask&0x0ff00000U)==(two?0x0ff00000U:0));assert(f.c.buRate_.initial==(two?27:19));assert(f.c.buRate_.rate_id==(width==40?(two?0:1):(two?2:3)));f.n.ni_port_valid=true;assert(!f.c.send8710BUFrame(packet(),&f.n));assert((f.c.submittedBytes[6]&31)==f.c.buRate_.rate_id);
  rtw_rx_packet rx{};rx.rate_index=19;rx.bandwidth_code=width==40?1:0;assert(f.c.accepts8710BUReceiveRate(rx));uint8_t report[6]={0x0c,0,27,0,0,0};f.c.handle8710BUC2H(report,6);assert(f.c.rateControl_.reports==(two?1U:0U)&&f.c.rateControl_.invalid==(two?0U:1U));
 }

 for(unsigned pipes:{1U,2U,3U})for(unsigned width:{20U,40U}){
  Fixture f(pipes);f.run(true,width);assert(f.c.uses8710BUFirmwareRate()&&f.c.uses8710BUHighThroughput()&&!f.c.authorized()&&!f.n.ni_port_valid);assert(f.m.reg[0x1d5]==0x21&&!(f.m.reg[0x1d2]&0x60)&&!(f.m.reg[0x1d3]&0xfe));assert(f.m.read(0x6a2,2)==0x100);assert(f.c.send8710BUFrame(packet(),&f.n)==ENETDOWN);assert(!f.c.send8710BUFrame(packet(true),&f.n));
  f.n.ni_port_valid=true;assert(f.c.authorized());f.n.ni_rsnprotos=IEEE80211_PROTO_RSN;f.c.core_.sc_ic.ic_flags|=IEEE80211_F_RSNON;f.n.ni_tx_ba[5].ba_winsize=8;f.n.ni_tx_ba[5].ba_state=IEEE80211_BA_AGREED;f.n.ni_ampdu_param=3|(7<<2);keyAvailable=encryptSuccess=true;auto*p=packet(false,true);p->bytes[1]=IEEE80211_FC1_PROTECTED;p->bytes[24]=5;assert(!f.c.send8710BUFrame(p,&f.n));if(!(f.c.txAMPDUPackets_==1&&(f.c.submittedBytes[13]&0x30)==0x30&&((f.c.submittedBytes[14]>>1)&31)==8&&f.c.submittedPipe<pipes)){printf("AMPDU diag count%llu d13=%02x d14=%02x pipe%u/%u ht%u rate%u\n",(unsigned long long)f.c.txAMPDUPackets_,f.c.submittedBytes[13],f.c.submittedBytes[14],f.c.submittedPipe,pipes,f.c.buRate_.ht,f.c.buRate_.initial);fflush(stdout);abort();}keyAvailable=encryptSuccess=false;
  assert(!RTWUSBController::ampdu8710BUTXStart(&f.c.core_.sc_ic,&f.n,5));assert(RTWUSBController::ampdu8710BUTXStart(&f.c.core_.sc_ic,&f.n,8)==EINVAL);rtw_rx_packet rx{};rx.rate_index=19;rx.bandwidth_code=width==40?1:0;assert(f.c.accepts8710BUReceiveRate(rx));rx.rate_index=44;assert(!f.c.accepts8710BUReceiveRate(rx));f.c.stop8710BU();assert(!f.n.ni_port_valid&&!f.c.uses8710BUFirmwareRate()&&f.m.reg[0x522]==255);
 }
 {Fixture a,b;a.run(true,20);b.run(true,40);a.n.ni_port_valid=b.n.ni_port_valid=true;a.c.stop8710BU();assert(b.c.uses8710BUFirmwareRate()&&b.c.authorized()&&b.m.reg[0x8090]==0xc6);assert(!b.c.send8710BUFrame(packet(),&b.n));}
 {Fixture f;f.run();assert(!f.c.uses8710BUHighThroughput());keyAvailable=encryptSuccess=true;auto qos=[](bool eapol){auto*p=packet(eapol,true);p->bytes[1]=IEEE80211_FC1_PROTECTED;p->bytes[24]=5;return p;};assert(!f.c.send8710BUFrame(qos(true),&f.n));assert((f.c.submittedBytes[13]&1)&&!(f.c.submittedBytes[9]&0x10));f.n.ni_port_valid=true;assert(!f.c.send8710BUFrame(qos(false),&f.n));assert(!(f.c.submittedBytes[13]&1)&&!f.c.txAMPDUPackets_);keyAvailable=encryptSuccess=false;}
 {Fixture f;f.run();std::vector<uint8_t> raw(96);auto word=[&](unsigned offset,uint32_t value){for(unsigned i=0;i<4;++i)raw[offset+i]=uint8_t(value>>(8*i));};word(0,40|(4U<<16)|(1U<<26));word(12,4);raw[24]=1;raw[25]=50;raw[26]=60;raw[56]=0x80;memcpy(raw.data()+66,f.n.ni_bssid,6);memcpy(raw.data()+72,f.n.ni_bssid,6);raw[95]=0xa5;rtw_rx_iterator iterator{raw.data(),raw.size(),0,0};rtw_8192eu_rx_frame frame{};uint8_t report=0;assert(f.c.decode8710BUUSBFrame(&iterator,&frame,&report)==RTW_PACKETS_OK&&!report&&frame.payload.mpdu_length==40&&frame.payload.mpdu[39]==0xa5);assert(f.c.sample8710BURSSI(frame.payload)==-60&&f.c.buRSSIValid_&&f.c.buRSSI_==40);uint8_t message[6]={0x0c,0,11,0,0,0};f.c.handle8710BUC2H(message,6);assert(f.c.rateControl_.reports==1&&f.c.rateControl_.finalRate==11);message[2]=0x8b;f.c.handle8710BUC2H(message,6);assert(f.c.rateControl_.invalid==1);}
 {Fixture f;f.run();f.n.ni_port_valid=true;f.m.reg[0x8090]=0x46;assert(f.c.tick8710BU()==ENXIO);assert(!f.c.uses8710BUFirmwareRate());}
 unsigned auth=0,run=0;{Fixture f;f.start();auto before=f.m.calls;assert(!f.state(IEEE80211_S_AUTH));auth=f.m.calls-before;before=f.m.calls;assert(!f.state(IEEE80211_S_RUN));run=f.m.calls-before;}
 for(unsigned i=1;i<=auth;++i){Fixture f;f.start();f.m.fail=f.m.calls+i;assert(f.state(IEEE80211_S_AUTH)==EIO&&!f.n.ni_port_valid&&!f.c.buRateReady_&&!f.c.uses8710BUFirmwareRate()&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&f.m.reg[0x522]==255);}
 for(unsigned i=1;i<=run;++i){Fixture f;f.start();assert(!f.state(IEEE80211_S_AUTH));f.m.fail=f.m.calls+i;assert(f.state(IEEE80211_S_RUN)==EIO&&!f.n.ni_port_valid&&!f.c.buRateReady_&&!f.c.uses8710BUFirmwareRate()&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&f.m.reg[0x522]==255);}
 printf("8710BU actual backend + actual8710BU radio model: 1/2/3 OUT pipes, HT20/40, controlled port/EAPOL/software-crypto boundary/BA/TX and %u AUTH + %u RUN I/O failures passed\n",auth,run);
}
