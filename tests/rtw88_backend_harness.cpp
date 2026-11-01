#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: BSD-3-Clause
 * Actual B/C controller methods,isolated host/USB/PHY boundaries. */
#include "rtw/RTW88PHYOperating.h"
#include "rtw/RTW88Coex.h"
#include "rtw/RTW88RX.h"
#include "rtw/RTW88USBDataPath.h"
#include "rtw/RTL8192EUDataPath.h"
#include "rtw/RTL8192EU.h"
#include "rtw/USBDescriptors.h"
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
enum {IEEE80211_M_STA=1,IEEE80211_NODE_HT=0x400,IEEE80211_HTCAP_SGI20=0x20,IEEE80211_F_SHSLOT=0x20000,IEEE80211_RATE_BASIC=128,IEEE80211_RATE_VAL=127,
 IEEE80211_NODE_QOS=0x2,IEEE80211_F_QOS=0x80000,IEEE80211_HTCAP_SMPS_MASK=12,IEEE80211_HTCAP_SMPS_SHIFT=2,IEEE80211_HTCAP_SMPS_DIS=3,
 IEEE80211_F_SHPREAMBLE=1,IEEE80211_F_USEPROT=2,IEEE80211_PROT_CTSONLY=1,IEEE80211_PROT_RTSCTS=2,
 IEEE80211_FC0_TYPE_MASK=0x0c,IEEE80211_FC0_TYPE_DATA=8,IEEE80211_FC0_SUBTYPE_QOS=0x80,
 IEEE80211_FC1_DIR_MASK=3,IEEE80211_FC1_DIR_DSTODS=3,IEEE80211_FC1_PROTECTED=0x40,IEEE80211_CIPHER_CCMP=4};
struct ieee80211_frame {uint8_t i_fc[2],i_dur[2],i_addr1[6],i_addr2[6],i_addr3[6],i_seq[2];};
static_assert(sizeof(ieee80211_frame)==24,"wireheader");
struct ieee80211_rateset {uint8_t rs_nrates,rs_rates[32];};
struct ieee80211_channel {int number;unsigned ic_flags=0;};
enum {IEEE80211_CHAN_5GHZ=0x100,IEEE80211_CHAN_PASSIVE=0x200};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel*)-1)
struct ieee80211_node {ieee80211_channel*ni_chan;uint8_t ni_bssid[6];uint16_t ni_associd,ni_intval;ieee80211_rateset ni_rates;bool ni_port_valid=false;unsigned ni_flags=0,ni_htcaps=0;uint8_t ni_rxmcs[2]{};};
enum{IFF_RUNNING=1,IFF_UP=2};
struct _ifnet {unsigned if_flags=0,if_timer=0;int if_snd=0;};
static void ifq_clr_oactive(int*){}
static const int systq=0;static void task_del(int,int*){}
enum {IEEE80211_MODE_11B,IEEE80211_MODE_11A,IEEE80211_MODE_11G};
struct ieee80211_edca_ac_params {uint8_t ac_ecwmin,ac_ecwmax,ac_aifsn;uint16_t ac_txoplimit;};
/* ACTUAL_DEFAULT_EDCA */
struct ieee80211com {void*ic_softc;int ic_opmode;ieee80211_state ic_state;_ifnet ic_if;ieee80211_channel*ic_ibss_chan;int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;unsigned ic_flags,ic_htcaps=0,ic_rtsthreshold=2347,ic_protmode=0;ieee80211_node*ic_bss;ieee80211_edca_ac_params ic_edca_ac[4]{};};
struct rtwn_softc {ieee80211com sc_ic{};struct{void*cookie=nullptr;}sc_ops;int core_error=0;unsigned sc_tx_timer=0;int init_task=0;int(*sc_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;};

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
class OSObject;class IOTimerEventSource;
struct TestMemory{uint8_t b[4352]{};void*getBytesNoCopy(){return b;}};struct TestPipe{unsigned number;unsigned getSpeed()const{return 3;}};
static std::vector<int>events;static unsigned ioCalls=0,failIO=0;static int failEvent=0,genericFail=-1;
static std::array<uint8_t,65536>regs{};static std::vector<std::array<uint8_t,8>>commands;
static int event(int e){events.push_back(e);return failEvent==e?EIO:0;}
static int ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return c->number;}
static int rawRead(void*,uint16_t a,unsigned w,uint32_t*v){*v=0;if(++ioCalls==failIO)return EIO;for(unsigned i=0;i<w;i++)*v|=(uint32_t)regs[a+i]<<(i*8);if(a==0x670)*v&=~0x80000000U;return 0;}
static int rawWrite(void*,uint16_t a,unsigned w,uint32_t v){if(++ioCalls==failIO)return EIO;for(unsigned i=0;i<w;i++)regs[a+i]=(uint8_t)(v>>(i*8));return 0;}
static void delay(void*,uint32_t){}
struct FakePHY{rtw88_phy_radio_status status{};uint16_t*sequence=nullptr;};
extern "C" rtw88_phy_radio_status rtw88_phy_radio_get_status(const rtw88_phy_context*c){return c&&c->radio_state?static_cast<FakePHY*>(c->radio_state)->status:rtw88_phy_radio_status{};}
extern "C" int rtw88_phy_radio_ready(const rtw88_phy_context*c){auto s=rtw88_phy_radio_get_status(c);return c&&c->boot&&c->boot->firmware.powered&&c->boot->firmware.firmware_running&&s.channel_ready&&s.calibrated_ready&&!s.error;}
extern "C" int rtw88_phy_radio_tune(rtw88_phy_context*c,uint8_t p,uint8_t center,uint8_t width,uint8_t index){auto*s=static_cast<FakePHY*>(c->radio_state);s->status.calibrated_ready=0;int e=event(10);s->status.primary_channel=p;s->status.center_channel=center;s->status.width_mhz=width;s->status.primary_index=index;s->status.channel_ready=!e;s->status.error=e;return e;}
extern "C" int rtw88_phy_radio_calibrate(rtw88_phy_context*c){auto*s=static_cast<FakePHY*>(c->radio_state);int e=event(11);s->status.calibrated_ready=!e;s->status.error=e;return e;}
extern "C" int rtw88_phy_radio_set_associated(rtw88_phy_context*c,int a){if(a&&!rtw88_phy_radio_ready(c))return ENXIO;return event(a?36:37);}
extern "C" int rtw88_phy_radio_set_tx_rate(rtw88_phy_context*,uint8_t r){return r<=27?0:EINVAL;}
extern "C" int rtw88_phy_radio_set_h2c_sequence(rtw88_phy_context*c,uint16_t*s){static_cast<FakePHY*>(c->radio_state)->sequence=s;return 0;}
extern "C" int rtw88_phy_radio_thermal_track(rtw88_phy_context*c){return rtw88_phy_radio_ready(c)?event(38):ENXIO;}
extern "C" void rtw88_phy_radio_invalidate(rtw88_phy_context*c){if(c)c->radio_state=nullptr;events.push_back(12);}
extern "C" int rtw88_boot_stop(rtw88_boot_context*c){event(91);c->firmware.powered=c->firmware.firmware_running=c->board_valid=0;return c->firmware.last_error;}
extern "C" int rtw88_fw_h2c(rtw88_fw_context*c,const uint8_t*b){if(c->last_error)return c->last_error;std::array<uint8_t,8>x{};memcpy(x.data(),b,8);commands.push_back(x);int e=event(b[0]==0x40?30:b[0]==1?(b[1]?32:33):b[0]==0x42?34:35);if(e)c->last_error=e;return e;}
extern "C" int rtw88_mac_queue_endpoint(const rtw88_mac_plan*p,unsigned q,uint8_t*out){if(!p||!out||q>=20||p->queue_endpoint[q]>=p->bulk_out_count)return EINVAL;*out=p->queue_endpoint[q];return 0;}
extern "C" int rtw88_mac_initialize(rtw88_mac_context*m,rtw88_fw_context*c,unsigned,rtw88_usb_speed){int e=event(5);m->device=c;m->mac_ready=!e;return e;}
extern "C" int rtw88_phy_load_boot_tables(rtw88_phy_context*p,rtw88_boot_context*b,rtw88_mac_context*m){p->boot=b;p->mac=m;int e=event(6);p->tables_loaded=!e;return e;}
extern "C" int rtw88_phy_radio_initialize(rtw88_phy_context*p,void*s,size_t n,uint8_t){assert(n>=sizeof(FakePHY));int e=event(7);p->radio_state=e?nullptr:s;if(!e)*static_cast<FakePHY*>(s)=FakePHY{};return e;}
constexpr unsigned kUSBHostConnectionSpeedSuper=4,kUSBHostConnectionSpeedHigh=3;
class RTWUSBController{public:
 bool permitsDFSChannelByCountry(uint8_t)const{return true;}

 const rtw_usb_wifi_profile*profile_=nullptr;void*usb_=nullptr;
 void removeProperty(const char*){assert(!r88ResolvedIdentity_);}
 rtwn_softc core_{};rtw_wifi_family family_=RTW_WIFI_RTL8822BU;bool running_=true,stopping_=false,present_=true,radioReady_=true,sleeping_=false;int usbError_=0;
 struct { bool pending=false; } rxRecovery_;
 void*completionLock_=nullptr;rtw_transfer_ledger ledger_{};rtw_usb_endpoints endpoints_{};uint16_t txMPS_[4]{};TestPipe pipes[4]{};TestPipe*txPipes_[4]{};TestPipe*rxPipe_=&pipes[0];
 struct Transfer{TestMemory*memory;};Transfer transfers_[16]{};TestMemory memories[16]{};uint64_t txHTPackets_=0;
 enum{RXSlots=4,TXCapacity=4352};struct{unsigned invalid=0,ignored=0,reports=0;uint8_t finalRate=0;}rateControl_;
#include "../kext/RTW88BackendMembers.inc"
 static void cancelScan(void*){events.push_back(52);}static void cancelCalibration(void*){events.push_back(53);}static void nextScan(void*){events.push_back(54);}static void nextCalibration(void*){events.push_back(55);}
 bool authorized()const{return radioReady_&&present_&&running_&&!stopping_&&!core_.core_error&&uses88FirmwareRate()&&core_.sc_ic.ic_bss->ni_port_valid;}
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}static int ioError(void*p){auto*s=owner(p);return !s->present_||s->sleeping_||s->usbError_?EIO:0;}
 int allocateTransfers(){return event(1);}void abortTransfers(){events.push_back(90);running_=radioReady_=false;}void freeTransfers(){events.push_back(92);}
 static int busInitialize(void*p){auto*s=owner(p);int e=event(14);s->running_=!e;return e;}
 unsigned submitted=0,submittedPipe=0;std::vector<uint8_t>submittedBytes;int submitError=0;std::vector<std::array<uint8_t,80>>packets;
 int submit(unsigned index,TestPipe*pipe,uint32_t len){++submitted;submittedPipe=pipe->number;submittedBytes.assign(memories[index].b,memories[index].b+len);return submitError;}
};
int RTWUSBController::r88Read(void*p,uint16_t a,unsigned w,uint32_t*v){int e=rawRead(p,a,w,v);if(e)owner(p)->r88Core_.firmware.last_error=e;return e;}
int RTWUSBController::r88Write(void*p,uint16_t a,unsigned w,uint32_t v){int e=rawWrite(p,a,w,v);if(e)owner(p)->r88Core_.firmware.last_error=e;return e;}
int RTWUSBController::r88Bulk(void*p,uint8_t q,const uint8_t*b,size_t n){assert(q==19&&n==80);auto*s=owner(p);std::array<uint8_t,80>saved{};memcpy(saved.data(),b,n);s->packets.push_back(saved);return event(39);}
int RTWUSBController::bootstrap88(bool compare){assert(compare);int e=event(2);if(!e)r88Core_.firmware.identified=r88Core_.firmware.powered=r88Core_.firmware.firmware_running=r88Core_.board_valid=1;return e;}
static RTWUSBController*controller(ieee80211com*ic){return static_cast<RTWUSBController*>(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie);}
static int generic(ieee80211com*ic,ieee80211_state state,int){events.push_back(60+state);if(genericFail==state)return EINVAL;if(state==IEEE80211_S_RUN)assert(!controller(ic)->r88RateReady_);else if(ic->ic_bss)ic->ic_bss->ni_port_valid=false;ic->ic_state=state;return 0;}
static int route(ieee80211com*ic,ieee80211_state state,int arg){return controller(ic)->state88(ic,state,arg);}
/* ACTUAL_BACKEND_METHODS */
struct Fixture{RTWUSBController c;FakePHY phy{};ieee80211_channel channel{6,0};ieee80211_node n{};
 Fixture(rtw_wifi_family f=RTW_WIFI_RTL8822BU){events.clear();commands.clear();regs.fill(0);ioCalls=failIO=0;failEvent=0;genericFail=-1;c.family_=f;for(size_t i=0;i<rtw_usb_wifi_profile_count();++i){auto*p=rtw_usb_wifi_profile_at(i);if(p->family==f&&p->primary_verified&&rtw_usb_wifi_profile_allowed_bands(p)==3){c.profile_=p;break;}}assert(c.profile_);n.ni_chan=&channel;n.ni_associd=0xc001;n.ni_intval=100;n.ni_bssid[0]=2;n.ni_rates={3,{0x82,12,108}};n.ni_flags=IEEE80211_NODE_HT;n.ni_htcaps=IEEE80211_HTCAP_SGI20|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);n.ni_rxmcs[0]=n.ni_rxmcs[1]=255;
 c.core_.sc_ic.ic_ibss_chan=&channel;c.core_.sc_ic.ic_newstate=route;c.core_.sc_ic.ic_softc=&c.core_;c.core_.sc_ops.cookie=&c;c.core_.sc_ic.ic_opmode=IEEE80211_M_STA;c.core_.sc_ic.ic_bss=&n;c.core_.sc_ic.ic_htcaps=IEEE80211_HTCAP_SGI20;c.core_.sc_ic.ic_flags=IEEE80211_F_QOS;c.core_.sc_newstate=generic;
 c.r88BoardKnown_=true;c.r88MACAddress_[0]=2;c.r88Core_.board.mac[0]=2;c.r88Core_.board.rf_paths=f==RTW_WIFI_RTL8821CU?1:2;c.r88Core_.board.active_rf_paths=c.r88Core_.board.rf_paths;c.r88Core_.board.tx_streams=c.r88Core_.board.rf_paths;c.r88Core_.board.features_valid=1;c.r88Core_.physical_valid=1;c.r88Core_.firmware.family=f;c.r88Core_.firmware.identified=c.r88Core_.firmware.powered=c.r88Core_.firmware.firmware_running=c.r88Core_.board_valid=1;c.r88Core_.firmware.bus={&c,rawRead,rawWrite,nullptr,delay,nullptr,nullptr};c.r88MAC_.device=&c.r88Core_.firmware;c.r88MAC_.mac_ready=1;c.r88MAC_.fifo.family=f;
 c.r88PHY_.boot=&c.r88Core_;c.r88PHY_.mac=&c.r88MAC_;c.r88PHY_.radio_state=&phy;c.r88PHY_.tables_loaded=1;c.r88RX_={f,c.r88Core_.board.rf_paths,2,0,63,0};assert(!rtw88_station_initialize(&c.r88Station_,&c.r88MAC_,RTWUSBController::r88RadioState,&c,c.r88MACAddress_));for(unsigned i=0;i<16;i++)c.transfers_[i].memory=&c.memories[i];usb(3);regs[0x80]=0x78;regs[0x81]=0xc0;c.remember88Authority(true);}
 void usb(unsigned count){c.r88MAC_.fifo.bulk_out_count=c.endpoints_.num_bulk_out=(uint8_t)count;c.endpoints_.num_bulk_in=1;for(unsigned i=0;i<count;i++){c.pipes[i].number=i;c.txPipes_[i]=&c.pipes[i];c.txMPS_[i]=512;}for(unsigned q=0;q<20;q++)c.r88MAC_.fifo.queue_endpoint[q]=q<8?(count>=3?2:1):0;}
 void five(unsigned ch=36){channel.number=(int)ch;channel.ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_PASSIVE;}
 int state(ieee80211_state s){return c.state88(&c.core_.sc_ic,s,-1);}void run(){assert(!state(IEEE80211_S_AUTH));assert(!state(IEEE80211_S_ASSOC));assert(!state(IEEE80211_S_RUN));}
 void cold(){c.r88Core_.firmware.powered=c.r88Core_.firmware.firmware_running=0;c.r88Station_.initialized=0;c.r88MAC_.mac_ready=0;c.r88PHY_.tables_loaded=0;c.running_=c.radioReady_=false;c.r88Storage_=&phy;c.r88PHY_.radio_state=nullptr;}
};
static mbuf_t packet(bool data,bool eapol,bool qos=false,size_t length=80){auto*p=new TestPacket;p->bytes.assign(length,0);auto*h=(ieee80211_frame*)p->bytes.data();h->i_fc[0]=data?uint8_t(8|(qos?0x80:0)):0x40;h->i_addr1[0]=2;h->i_seq[0]=0x10;if(eapol){const uint8_t llc[8]={0xaa,0xaa,3,0,0,0,0x88,0x8e};memcpy(p->bytes.data()+24+(qos?2:0),llc,8);}return p;}
static uint32_t get32(const uint8_t*b){return(uint32_t)b[0]|((uint32_t)b[1]<<8)|((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);}
static void firmwareRegression(){
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned channel:{6U,36U,52U,56U,60U,64U,100U,104U,108U,112U,116U,120U,124U,128U,132U,136U,140U,165U}){
  for(unsigned clock:{0U,0x1000U,0x2000U,0x3000U}){Fixture f(family);if(channel>14)f.five(channel);f.run();uint16_t ready=(uint16_t)(0xc078U|clock);regs[0x80]=(uint8_t)ready;regs[0x81]=(uint8_t)(ready>>8);f.c.r88RSSI_=55;f.c.r88RSSIValid_=true;assert(!f.c.tick88()&&commands.back()[0]==0x42&&f.c.uses88FirmwareRate());++cases;}
  for(uint16_t invalid:{0U,0xc6U,0xc070U,0x4078U,0xc079U,0xc0f8U}){Fixture f(family);if(channel>14)f.five(channel);f.run();regs[0x80]=(uint8_t)invalid;regs[0x81]=(uint8_t)(invalid>>8);assert(f.c.tick88()==EIO&&!f.c.r88Core_.firmware.firmware_running&&!f.c.uses88FirmwareRate());++cases;}
  {Fixture f(family);if(channel>14)f.five(channel);f.run();failIO=ioCalls+1;assert(f.c.tick88()==EIO&&!f.c.uses88FirmwareRate());++cases;}
 }
 std::printf("B/C production watchdog: %u 3081-ready/CPU-clock/stale/legacy/IO cases passed\n",cases);
}
static void smpsRegression(){
 unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned channel:{6U,36U,52U,56U,60U,64U,100U,104U,108U,112U,116U,120U,124U,128U,132U,136U,140U,165U})for(unsigned smps=0;smps<4;++smps)for(bool localSGI:{false,true})for(bool peerSGI:{false,true})for(bool peer2SS:{false,true}){
  Fixture f(family);if(channel>14)f.five(channel);f.n.ni_htcaps=(smps<<IEEE80211_HTCAP_SMPS_SHIFT)|(peerSGI?IEEE80211_HTCAP_SGI20:0);f.n.ni_rxmcs[1]=peer2SS?255:0;f.c.core_.sc_ic.ic_htcaps=localSGI?IEEE80211_HTCAP_SGI20:0;f.run();
  const unsigned streams=family!=RTW_WIFI_RTL8821CU&&peer2SS&&smps==IEEE80211_HTCAP_SMPS_DIS?2:1;const bool sgi=localSGI&&peerSGI;const uint64_t mask=(channel>14?0x810U:0x811U)|((streams==2?UINT64_C(0xffff):UINT64_C(0xff))<<12);
  assert(f.c.r88Rates_.rates.tx_streams==streams&&f.c.r88Rates_.rates.mask==mask&&f.c.r88Rates_.rates.sgi20==sgi&&f.c.r88Rates_.rates.initial_rate==(streams==2?27:19));
  assert(get32(commands.front().data()+4)==mask&&((commands.front()[2]>>7)&1)==sgi);
  f.n.ni_port_valid=true;keyAvailable=encryptSuccess=true;auto*p=packet(true,false,true);p->bytes[1]|=IEEE80211_FC1_PROTECTED;assert(!f.c.send88Frame(p,&f.n));encryptSuccess=false;
  const auto&d=f.c.submittedBytes;assert(d.size()==144&&d[16]==(streams==2?27:19)&&!(d[13]&1)&&((d[20]>>4)&1)==sgi&&((d[6]>>6)&3)==0&&!((get32(d.data()+8)>>12)&1));assert(((get32(d.data()+4)>>16)&31)==f.c.r88Rates_.rates.rate_id);
  uint8_t report[9]={0xc,0,(uint8_t)(19|(sgi?128:0)),0,0,0,0,0,0};f.c.handle88C2H(report,9);assert(f.c.rateControl_.reports==1);report[2]=27;f.c.handle88C2H(report,9);assert(f.c.rateControl_.reports==(streams==2?2U:1U)&&f.c.rateControl_.invalid==(streams==2?0U:1U));
  // TX SMPS restriction never rewrites the peer or the board's RX capacity.
  assert(f.n.ni_rxmcs[1]==(peer2SS?255:0)&&f.c.r88RX_.rf_paths==(family==RTW_WIFI_RTL8821CU?1:2));
  if(f.c.r88RX_.rf_paths==2){uint8_t bytes[56]{};bytes[0]=28;bytes[12]=27;rtw_rx_iterator it{bytes,sizeof(bytes),0,0};rtw_8192eu_rx_frame out{};uint8_t type=0;assert(f.c.decode88USBFrame(&it,&out,&type)==RTW_PACKETS_OK&&out.payload.rate_index==27&&!type);}
  ++cases;
 }
 std::printf("B/C production SMPS: %u peer/SGI/NSS/RA/protected-TX/C2H/RX cases passed\n",cases);
}
static void edcaRegression(){
 static const unsigned addresses[4]={0x508,0x50c,0x504,0x500};unsigned cases=0;
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(unsigned channel:{6U,36U,52U,56U,60U,64U,100U,104U,108U,112U,116U,120U,124U,128U,132U,136U,140U,165U})for(bool cckOnly:{false,true})for(bool globalQOS:{false,true})for(bool peerQOS:{false,true})for(bool stale:{false,true})for(bool shortSlot:{false,true}){
  if(channel>14&&cckOnly)continue;
  Fixture f(family);if(channel>14)f.five(channel);if(cckOnly)f.n.ni_rates={3,{0x82,4,22}};f.n.ni_flags=peerQOS?IEEE80211_NODE_QOS:0;auto&ic=f.c.core_.sc_ic;ic.ic_flags=(globalQOS?IEEE80211_F_QOS:0)|(shortSlot?IEEE80211_F_SHSLOT:0);
  if(stale||peerQOS)for(unsigned i=0;i<4;++i)ic.ic_edca_ac[i]={(uint8_t)(3+i),(uint8_t)(5+i),(uint8_t)(2+i),(uint16_t)(30+i)};
  f.run();const auto*ac=(globalQOS&&peerQOS)?ic.ic_edca_ac:ieee80211_edca_table[channel>14?IEEE80211_MODE_11A:cckOnly?IEEE80211_MODE_11B:IEEE80211_MODE_11G];unsigned slot=channel>14||shortSlot?9:20,sifs=channel>14?16:10;
  for(unsigned i=0;i<4;++i){const auto&a=ac[i];uint32_t expected=(uint32_t)a.ac_txoplimit<<16|(uint32_t)a.ac_ecwmax<<12|(uint32_t)a.ac_ecwmin<<8|(a.ac_aifsn*slot+sifs);assert((get32(regs.data()+addresses[i])&0x7ffffff)==expected);}assert(regs[0x51b]==slot);++cases;
 }
 std::printf("B/C production EDCA: %u WMM/non-WMM/stale/default/slot cases passed\n",cases);
}
int main(int argc,char**argv){
 if(argc==2){if(!strcmp(argv[1],"firmware"))firmwareRegression();else if(!strcmp(argv[1],"smps"))smpsRegression();else{assert(!strcmp(argv[1],"edca"));edcaRegression();}return 0;}
 firmwareRegression();smpsRegression();edcaRegression();
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU})for(bool five:{false,true}){
  Fixture f(family);if(five)f.five();assert(!f.state(IEEE80211_S_SCAN));assert(!f.c.r88RateReady_&&!f.c.authorized());assert(RTWUSBController::r88RadioState(&f.c,(uint8_t)f.channel.number)==(five?1:2));
  assert(f.c.send88Frame(packet(true,true),&f.n)==ENETDOWN);f.run();assert(f.c.uses88FirmwareRate()&&!f.c.authorized());assert(f.c.r88Rates_.rates.ht&&f.c.r88Rates_.rates.tx_streams==(family==RTW_WIFI_RTL8821CU?1:2));assert(commands.size()==2&&commands[0][0]==0x40&&commands[1][0]==1);
  f.phy.status.center_channel++;assert(!f.c.uses88FirmwareRate());--f.phy.status.center_channel;
  assert(f.c.send88Frame(packet(true,false,true),&f.n)==ENETDOWN);{int result=f.c.send88Frame(packet(true,true,true,464),&f.n);if(result)std::fprintf(stderr,"TX error%d family%u five%d radio%u flags%d\n",result,unsigned(family),five,RTWUSBController::r88RadioState(&f.c,f.c.r88Channel_),f.c.r88Station_.initialized);assert(!result);}assert(f.c.submittedBytes.size()==513&&(f.c.submittedBytes[13]&1));
  f.n.ni_port_valid=true;assert(f.c.authorized());assert(!f.c.send88Frame(packet(true,false,true),&f.n));assert(!(f.c.submittedBytes[13]&1)&&f.c.submittedBytes[2]==48&&f.c.txHTPackets_==1);
  encryptSuccess=true;keyAvailable=true;auto*protectedPacket=packet(true,false,true);protectedPacket->bytes[1]|=IEEE80211_FC1_PROTECTED;assert(!f.c.send88Frame(protectedPacket,&f.n));assert(f.c.submittedBytes.size()==48+80+16&&!((f.c.submittedBytes[6]>>6)&3));encryptSuccess=false;
  f.n.ni_port_valid=false;auto*p=packet(true,true,true);p->bytes[1]|=IEEE80211_FC1_PROTECTED;keyAvailable=false;assert(f.c.send88Frame(p,&f.n)==EINVAL);keyAvailable=true;p=packet(true,true);p->bytes[1]|=IEEE80211_FC1_PROTECTED;assert(f.c.send88Frame(p,&f.n)==ENOBUFS);
  unsigned before=releases;f.c.submitError=EIO;assert(f.c.send88Frame(packet(false,false),&f.n)==EIO&&releases==before);f.c.submitError=0;
  uint8_t report[9]={0xc,0,f.c.r88Rates_.rates.initial_rate,0,0,0,0,0,0};f.c.handle88C2H(report,9);assert(f.c.rateControl_.reports==1);report[8]=1;f.c.handle88C2H(report,9);assert(f.c.rateControl_.invalid==1);
  f.n.ni_port_valid=true;f.c.stop88();assert(!f.n.ni_port_valid&&!f.c.authorized()&&!f.c.running_&&!f.c.r88PHY_.radio_state&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);
 }
 unsigned authIO=0,runIO=0;{Fixture f;unsigned before=ioCalls;assert(!f.state(IEEE80211_S_AUTH));authIO=ioCalls-before;before=ioCalls;assert(!f.state(IEEE80211_S_RUN));runIO=ioCalls-before;}
 for(unsigned i=1;i<=authIO;i++){Fixture f;failIO=ioCalls+i;assert(f.state(IEEE80211_S_AUTH)==EIO);assert(!f.c.authorized()&&!f.c.r88RateReady_&&!f.c.r88PHY_.radio_state&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 for(unsigned i=1;i<=runIO;i++){Fixture f;assert(!f.state(IEEE80211_S_AUTH));failIO=ioCalls+i;assert(f.state(IEEE80211_S_RUN)==EIO);assert(!f.c.authorized()&&!f.c.r88RateReady_&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 for(int stage:{10,11,30,32,36,64}){Fixture f;if(stage==10||stage==11){failEvent=stage;assert(f.state(IEEE80211_S_AUTH)==EIO);}else{assert(!f.state(IEEE80211_S_AUTH));failEvent=stage;if(stage==64)genericFail=4;assert(f.state(IEEE80211_S_RUN)==(stage==64?EINVAL:EIO));}assert(!f.c.authorized()&&!f.c.r88PHY_.radio_state);}
 for(auto family:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){Fixture f(family);f.cold();assert(!f.c.start88());assert(f.c.radioReady_&&f.c.running_&&f.c.r88Station_.initialized&&f.c.r88PHY_.radio_state==&f.phy&&f.c.packets.size()==2);assert(f.c.packets[0][50]==13&&f.c.packets[0][54]==0&&f.c.packets[1][50]==17&&f.c.packets[1][54]==1&&f.phy.sequence==&f.c.r88PacketSequence_);f.c.stop88();}
 for(int stage:{1,2,5,6,7,14,39,10,11,61}){Fixture f;f.cold();failEvent=stage;if(stage==61)genericFail=1;assert(f.c.start88()==(stage==61?EINVAL:EIO));assert(!f.c.running_&&!f.c.radioReady_&&!f.c.r88PHY_.radio_state&&!f.c.r88Core_.firmware.powered);}
 for(int gone:{0,1}){Fixture f;f.run();f.n.ni_port_valid=true;if(gone)f.c.present_=false;else f.c.sleeping_=true;unsigned before=ioCalls;f.c.stop88();assert(ioCalls==before&&!f.n.ni_port_valid&&!f.c.running_&&!f.c.r88Core_.firmware.firmware_running);}
 {Fixture f;f.run();std::vector<uint8_t>transfer;
  auto put32=[](uint8_t*b,uint32_t v){for(unsigned i=0;i<4;i++)b[i]=(uint8_t)(v>>(8*i));};
  transfer.resize(24+2+32+28);put32(transfer.data(),28U|(4U<<16)|(4U<<20)|(2U<<24)|(1U<<26)|(1U<<27));put32(transfer.data()+12,12);auto*phy=transfer.data()+26;phy[0]=1;phy[1]=50;phy[2]=70;phy[5]=0x10;auto*mpdu=phy+32;mpdu[0]=0x80;memcpy(mpdu+10,f.n.ni_bssid,6);memcpy(mpdu+16,f.n.ni_bssid,6);mpdu[23]=0x7b;for(unsigned i=24;i<28;i++)mpdu[i]=0xff;while(transfer.size()%8)transfer.push_back(0);
  size_t next=transfer.size();transfer.resize(next+24+9);put32(transfer.data()+next,9);put32(transfer.data()+next+8,1U<<28);transfer[next+24]=0xc;while(transfer.size()%8)transfer.push_back(0);
  rtw_rx_iterator it{transfer.data(),transfer.size(),0,0};rtw_8192eu_rx_frame decoded{};uint8_t report=0;assert(f.c.decode88USBFrame(&it,&decoded,&report)==RTW_PACKETS_OK&&!report&&decoded.payload.mpdu_length==24&&decoded.payload.mpdu[23]==0x7b&&decoded.payload.phy_info[0]==1&&decoded.security_code==4&&decoded.sw_decrypt_required);assert(f.c.sample88RSSI(decoded.payload)==-40&&f.c.r88RSSIValid_);assert(f.c.decode88USBFrame(&it,&decoded,&report)==RTW_PACKETS_OK&&report==6&&decoded.payload.mpdu_length==9);assert(f.c.decode88USBFrame(&it,&decoded,&report)==RTW_PACKETS_DONE);
  it.cursor=0;it.frames=0;it.length=25;assert(f.c.decode88USBFrame(&it,&decoded,&report)==RTW_PACKETS_TRUNCATED&&it.cursor==0);
 }
 std::printf("B/C actual controller:3 families/2 bands,HT20,TX48/CCMP/port/start-stop/PHY tuple; %u AUTH+%u RUN IO failures passed\n",authIO,runIO);
}
