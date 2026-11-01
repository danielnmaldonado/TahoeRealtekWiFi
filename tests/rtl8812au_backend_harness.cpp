#include "rtw/StationChannels.h"
/* SPDX-License-Identifier: BSD-3-Clause
 * Executes actual controller state/TX/RX methods with isolated USB, RF and
 * host scheduling boundaries. It does not emulate an AP or prove RF/CCMP. */
#include "rtw/RTL8812AURuntime.h"
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
#include <cstddef>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include "../kext/RTL8812AUFirmware.inc"
#include "../kext/RTL8821AUFirmware.inc"
enum {IEEE80211_HTCAP_SMPS_MASK=12,IEEE80211_HTCAP_SMPS_SHIFT=2,IEEE80211_HTCAP_SMPS_DIS=3};
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_M_STA=1,IEEE80211_F_SHSLOT=0x20000,IEEE80211_RATE_BASIC=128,IEEE80211_RATE_VAL=127,
 IEEE80211_F_SHPREAMBLE=1,IEEE80211_F_USEPROT=2,IEEE80211_PROT_CTSONLY=1,IEEE80211_PROT_RTSCTS=2,
 IEEE80211_FC0_TYPE_MASK=0x0c,IEEE80211_FC0_TYPE_DATA=8,IEEE80211_FC0_SUBTYPE_QOS=0x80,
 IEEE80211_FC1_DIR_MASK=3,IEEE80211_FC1_DIR_DSTODS=3,IEEE80211_FC1_PROTECTED=0x40,IEEE80211_CIPHER_CCMP=4};
enum {IEEE80211_MODE_AUTO=0,IEEE80211_MODE_11N=5,IEEE80211_MODE_11AC=6,IEEE80211_MODE_11AX=7,IEEE80211_MODE_11G=3,IEEE80211_F_HTON=0x10000,IEEE80211_F_VHTON=0x20000,IEEE80211_F_HEON=0x40000,IEEE80211_F_NOVHT=16,IEEE80211_NODE_HT=1,IEEE80211_NODE_VHT=0x10000,IEEE80211_CHAN_WIDTH_20_NOHT=0,IEEE80211_CHAN_WIDTH_20=1,IEEE80211_CHAN_WIDTH_40=2,IEEE80211_CHAN_WIDTH_80=3,
IEEE80211_HTCAP_SGI20=0x20,IEEE80211_HTCAP_SGI40=0x40,IEEE80211_VHTCAP_SHORT_GI_80=0x20,IEEE80211_HTOP0_SCO_MASK=3,IEEE80211_HTOP0_SCO_SCA=1,IEEE80211_HTOP0_SCO_SCB=3,
IEEE80211_CHAN_HT20=0x10000,IEEE80211_CHAN_HT40U=0x20000,IEEE80211_CHAN_HT40D=0x40000,IEEE80211_CHAN_VHT20=0x1000000,IEEE80211_CHAN_VHT40U=0x2000000,IEEE80211_CHAN_VHT40D=0x4000000,IEEE80211_CHAN_VHT80=0x8000000,
IEEE80211_F_RSNON=0x1000,IEEE80211_PROTO_RSN=1,IEEE80211_BA_AGREED=2,IEEE80211_BA_MAX_WINSZ=64,IEEE80211_QOS_TID=15,IEEE80211_AMPDU_PARAM_LE=3,IEEE80211_AMPDU_PARAM_SS=28};
static uint16_t le16toh(uint16_t x){return x;}
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
enum {IEEE80211_MODE_11B=1,IEEE80211_MODE_11A=2,IEEE80211_NODE_QOS=2,IEEE80211_F_QOS=0x80000};
struct ieee80211_edca_ac_params {uint8_t ac_ecwmin,ac_ecwmax,ac_aifsn;uint16_t ac_txoplimit;};
/* ACTUAL_DEFAULT_EDCA */
struct ieee80211com {ieee80211_edca_ac_params ic_edca_ac[4]{};int ic_nnodes=0;unsigned ic_htcaps=0,ic_vhtcaps=0,ic_tid_noack=0,ic_modecaps=0,ic_curmode=0,ic_userflags=0;void*ic_softc;int ic_opmode;ieee80211_state ic_state;_ifnet ic_if;ieee80211_channel*ic_ibss_chan;int(*ic_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;unsigned ic_flags,ic_rtsthreshold=2347,ic_protmode=0;ieee80211_node*ic_bss;};
struct rtwn_softc {struct{void*cookie=nullptr;}sc_ops;ieee80211com sc_ic{};int core_error=0;unsigned sc_tx_timer=0;int init_task=0;int(*sc_newstate)(ieee80211com*,ieee80211_state,int)=nullptr;};
struct rtw_r12au_phy {rtw_r12au_phy_status status{};rtw_r12au_phy_board board{};rtw_usb_rw_transport bus{};const void*coex=nullptr;rtw_r12au_coex_guard guard=nullptr;};
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
struct TestMemory{uint8_t b[4352]{};void*getBytesNoCopy(){return b;}};struct TestPipe{unsigned number;};
static std::vector<int> events;static unsigned ioCalls=0,failIO=0;static int failEvent=0,genericFail=-1;
static std::array<uint8_t,65536> regs;static std::vector<uint16_t> readAddresses;
struct Write{uint16_t address;uint8_t width;uint32_t value;};static std::vector<Write>writes;
static int event(int e){events.push_back(e);return failEvent==e?EIO:0;}
static int ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return c->number;}
extern "C" int rtw_r12au_read(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t*v){
 if(c->io_error&&!c->cleanup)return c->io_error;readAddresses.push_back(a);if(++ioCalls==failIO){c->io_error=EIO;return EIO;}
 if(a==0x670)regs[a+3]&=0x7f;if(a==0x1e0)regs[a+3]&=0x3f;*v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)regs[a+i]<<(8*i);return 0;}
extern "C" int rtw_r12au_write(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t v){
 if(c->io_error&&!c->cleanup)return c->io_error;if(++ioCalls==failIO){c->io_error=EIO;return EIO;}
 if(a==0x500){int e=event(56);if(e){c->io_error=e;return e;}}
 if((a==0x1e0&&!(v&0xff00))||(a==0x522&&!c->mac_initialized&&c->firmware_running)){int e=event(a==0x1e0?3:5);if(e){c->io_error=e;return e;}}writes.push_back({a,w,v});for(unsigned i=0;i<w;++i)regs[a+i]=(uint8_t)(v>>(8*i));return 0;}
extern "C" int rtw_r12au_update(rtw_r12au_boot*c,uint16_t a,uint8_t w,uint32_t mask,uint32_t bits){uint32_t v=0;int e=rtw_r12au_read(c,a,w,&v);return e?e:rtw_r12au_write(c,a,w,(v&~mask)|(bits&mask));}
extern "C" int rtw_r12au_delay(rtw_r12au_boot*,uint32_t){return 0;}
extern "C" int rtw_r12au_phy_channel_supported(const rtw_r12au_phy_board*b,uint8_t ch,unsigned w){return w==20&&((ch>=1&&ch<=11)||(rtw_station_5ghz_channel(ch)&&b->rfe!=9));}
extern "C" void rtw_r12au_phy_get_status(const rtw_r12au_phy*p,rtw_r12au_phy_status*s){*s=p?p->status:rtw_r12au_phy_status{};}
extern "C" int rtw_r12au_phy_set_channel(rtw_r12au_phy*p,uint8_t ch,unsigned w){assert(w==20);p->status.calibrated=p->status.tx_enabled=0;int e=event(10);p->status.channel=ch;if(e)p->status.io_error=e;return e;}
extern "C" int rtw_r12au_phy_power_plan(const rtw_r12au_phy_board*,const rtw_r12au_operating*o,rtw_r12au_power_plan*out){*out={};if(o->primary!=6&&!rtw_station_5ghz_channel(o->primary))return ENOTSUP;if(rtw_station_is_dfs_channel(o->primary)&&o->width_mhz!=20)return ENOTSUP;if(o->width_mhz==80&&o->center!=42)return ENOTSUP;if(o->width_mhz==40&&o->center!=o->primary+2)return ENOTSUP;if(o->width_mhz==20&&o->center!=o->primary)return ENOTSUP;out->valid_rates=~UINT64_C(0);return 0;}
extern "C" int rtw_r12au_phy_set_operating(rtw_r12au_phy*p,const rtw_r12au_operating*o){int e=rtw_r12au_phy_set_channel(p,o->primary,20);p->status.primary=p->status.channel=o->primary;p->status.center=o->center;p->status.width_mhz=o->width_mhz;p->status.mode=(uint8_t)o->mode;return e;}
extern "C" int rtw_r12au_phy_tick(rtw_r12au_phy*){return event(35);}
extern "C" int rtw_r12au_phy_set_tx_rate(rtw_r12au_phy*,uint8_t){return 0;}
extern "C" int rtw_r12au_phy_calibrate(rtw_r12au_phy*p){int e=event(11);p->status.calibrated=!e;if(e)p->status.io_error=e;return e;}
extern "C" void rtw_r12au_phy_forget(rtw_r12au_phy*p){p->status={};p->coex=nullptr;p->guard=nullptr;}
extern "C" int rtw_r12au_phy_stop(rtw_r12au_phy*p){p->status.calibrated=p->status.tx_enabled=0;return event(12);}
extern "C" int rtw_r12au_phy_resume_tx(rtw_r12au_phy*p){assert(p->status.calibrated);if(p->board.btcoex&&(!p->guard||!p->guard(p->coex,&p->board,&p->bus)))return EBUSY;int e=event(13);p->status.tx_enabled=!e;return e;}
extern "C" int rtw_r12au_phy_init(void*storage,size_t bytes,const rtw_r12au_phy_board*b,const rtw_usb_rw_transport*bus,const rtw_waiter*,rtw_r12au_phy**out){assert(bytes>=sizeof(rtw_r12au_phy));int e=event(6);*out=e?nullptr:static_cast<rtw_r12au_phy*>(storage);if(!e){(*out)->status={};(*out)->board=*b;(*out)->bus=*bus;(*out)->coex=nullptr;(*out)->guard=nullptr;}return e;}
extern "C" int rtw_r12au_phy_bind_coex(rtw_r12au_phy*p,const void*c,rtw_r12au_coex_guard guard){if(!p||!c||!guard)return EINVAL;if(p->status.tx_enabled)return EBUSY;if(!guard(c,&p->board,&p->bus))return ENOTSUP;p->coex=c;p->guard=guard;return 0;}
extern "C" int rtw_r12au_phy_load_tables(rtw_r12au_phy*p){int e=event(7);p->status.loaded=!e;if(e)p->status.io_error=e;return e;}
extern "C" int rtw_r12au_power_on(rtw_r12au_boot*c){int e=event(2);c->powered=!e;return e;}
extern "C" int rtw_r12au_fw_inspect(rtw_r12au_chip chip,const uint8_t*image,size_t n,rtw_fw_image*out){assert(n==(chip==RTW_R12AU_CHIP_8812AU?27030U:31898U));assert(image[0]==(chip==RTW_R12AU_CHIP_8812AU?1:1)&&image[1]==(chip==RTW_R12AU_CHIP_8812AU?0x95:0x21));*out={};return 0;}
extern "C" int rtw_r12au_download_fw(rtw_r12au_boot*c,const uint8_t*image,size_t n,rtw_boot_report*out){assert(c->powered&&c->dma_prepared);rtw_fw_image fw{};assert(!rtw_r12au_fw_inspect(c->chip,image,n,&fw));int e=event(4);c->firmware_running=!e;if(!e)regs[0x80]=0xc6;*out={};return e;}
extern "C" int rtw_r12au_power_off(rtw_r12au_boot*c){event(91);c->powered=c->firmware_running=c->mac_initialized=c->dma_prepared=0;return c->io_error;}
static std::vector<std::array<uint8_t,8>> commands;
extern "C" int rtw_r12au_h2c(rtw_r12au_boot*c,const uint8_t*b){
 if(c->io_error)return c->io_error;std::array<uint8_t,8> command{};memcpy(command.data(),b,8);commands.push_back(command);
 assert(b[0]==0x40||b[0]==1||b[0]==0x42||b[0]>=0x60);c->h2c_box=(uint8_t)((c->h2c_box+1)%4);int e=event(b[0]>=0x60?36:b[0]==0x40?30:b[0]==1?(b[1]?32:33):34);if(e)c->io_error=e;return e;}
static uint64_t testClock=1000;
class RTWUSBController {public:
 bool permitsDFSChannelByCountry(uint8_t)const{return true;}

 static uint64_t nowMS(){return testClock;}
 rtwn_softc core_{};rtw_usb_wifi_profile testProfile_{};const rtw_usb_wifi_profile*profile_=&testProfile_;rtw_wifi_family family_=RTW_WIFI_RTL8812AU;bool running_=true,stopping_=false,present_=true,radioReady_=true,sleeping_=false;int usbError_=0;
 uint64_t txHTPackets_=0,txAMPDUPackets_=0;void*usb_=nullptr;TestPipe*rxPipe_=nullptr;bool usbOpened_=true;
 void*completionLock_=nullptr;rtw_transfer_ledger ledger_{};rtw_usb_endpoints endpoints_{};uint16_t txMPS_[4]{};TestPipe pipes[4]{};TestPipe*txPipes_[4]{};
 struct Transfer{TestMemory*memory;};Transfer transfers_[16]{};TestMemory memories[16]{};
 enum{RXSlots=4,TXCapacity=4352};struct{unsigned invalid=0,ignored=0,reports=0;uint8_t finalRate=0;}rateControl_;
#include "../kext/RTW8812BackendMembers.inc"
 static void cancelScan(void*){events.push_back(52);}static void cancelCalibration(void*){events.push_back(53);}
 static void nextScan(void*){events.push_back(54);}static void nextCalibration(void*){events.push_back(55);}

 bool authorized()const{return radioReady_&&present_&&running_&&!stopping_&&!core_.core_error&&uses8812FirmwareRate()&&core_.sc_ic.ic_bss->ni_port_valid;}
 static RTWUSBController*owner(void*p){return static_cast<RTWUSBController*>(p);}
 static int ioError(void*p){auto*s=owner(p);return !s->present_||s->sleeping_||s->usbError_?EIO:0;}
 int allocateTransfers(){return event(1);}void abortTransfers(){events.push_back(90);running_=radioReady_=false;}void freeTransfers(){events.push_back(92);}
 static int busInitialize(void*p){auto*s=owner(p);int e=event(14);s->running_=!e;return e;}
 unsigned submitted=0,submittedPipe=0,submittedIndex=0;std::vector<uint8_t>submittedBytes;int submitError=0;
 int submit(unsigned index,TestPipe*pipe,uint32_t len){++submitted;submittedIndex=index;submittedPipe=pipe->number;submittedBytes.assign(memories[index].b,memories[index].b+len);return submitError;}
};
int RTWUSBController::initialize8812Core(){auCore_.identified=1;auCore_.io_error=0;int e=event(15);if(!e)remember8812Authority();return e;}

static int generic(ieee80211com*ic,ieee80211_state state,int){events.push_back(60+state);if(genericFail==state)return EINVAL;
 if(state==IEEE80211_S_RUN)assert(!static_cast<RTWUSBController*>(ic->ic_softc)->auRateReady_);else if(ic->ic_bss)ic->ic_bss->ni_port_valid=false;ic->ic_state=state;return 0;}
static int route(ieee80211com*ic,ieee80211_state state,int argument){return static_cast<RTWUSBController*>(ic->ic_softc)->state8812(ic,state,argument);}
/* ACTUAL_BACKEND_METHODS */
struct Fixture{RTWUSBController c;rtw_r12au_phy phy;std::vector<std::max_align_t>coexMemory;ieee80211_channel channel{6,0};ieee80211_node n{};
 Fixture(rtw_wifi_family family=RTW_WIFI_RTL8812AU){testClock=1000;events.clear();commands.clear();writes.clear();readAddresses.clear();regs.fill(0);regs[0x80]=0xc6;ioCalls=failIO=0;failEvent=0;genericFail=-1;
 c.family_=family;n.ni_chan=&channel;n.ni_associd=0xc001;n.ni_intval=100;n.ni_bssid[0]=2;n.ni_rates={3,{0x82,12,108}};
 c.core_.sc_ic.ic_ibss_chan=&channel;c.core_.sc_ic.ic_newstate=route;c.core_.sc_ic.ic_softc=&c; c.core_.sc_ops.cookie=&c;c.core_.sc_ic.ic_opmode=IEEE80211_M_STA;c.core_.sc_ic.ic_bss=&n;c.core_.sc_newstate=generic;
 c.auCore_.chip=c.auCore_.board.phy.chip=family==RTW_WIFI_RTL8812AU?RTW_R12AU_CHIP_8812AU:RTW_R12AU_CHIP_8821AU;c.auCore_.board.phy.rf_paths=family==RTW_WIFI_RTL8812AU?2:1;
 c.auCore_.board.mac[0]=2;c.auCore_.identified=c.auCore_.powered=c.auCore_.dma_prepared=c.auCore_.firmware_running=c.auCore_.mac_initialized=1;
 c.auPHY_=&phy;phy.status.loaded=1;for(unsigned i=0;i<16;++i)c.transfers_[i].memory=&c.memories[i];usb(3);}
 void usb(unsigned count){c.auCore_.usb.speed=RTW_R12AU_USB_HIGH;c.auCore_.usb.bulk_max_packet=512;c.auCore_.usb.bulk_out_count=c.endpoints_.num_bulk_out=(uint8_t)count;static const uint8_t ep[4]={3,1,2,5};for(unsigned i=0;i<count;++i){c.auCore_.usb.bulk_out[i]=c.endpoints_.bulk_out[i]=ep[i];c.pipes[i].number=i;c.txPipes_[i]=&c.pipes[i];c.txMPS_[i]=512;}c.remember8812Authority();}
 void coex(bool shared){c.auCore_.board.phy.btcoex=1;c.auCore_.board.bt_setting=c.auCore_.board.share_ant=shared;coexMemory.resize((rtw_r21au_coex_size()+sizeof(std::max_align_t)-1)/sizeof(std::max_align_t));c.auCoexStorage_=coexMemory.data();c.auCoexSize_=coexMemory.size()*sizeof(std::max_align_t);regs[0x770]=1;}
 /* A model positive-identify boundary, before resetting mutable readiness. */
 void cold(){c.remember8812Authority();c.auCore_.powered=c.auCore_.firmware_running=c.auCore_.mac_initialized=c.auCore_.dma_prepared=0;c.running_=c.radioReady_=false;c.auPHYStorage_=&phy;c.auPHYSize_=sizeof(phy);c.auPHY_=nullptr;}
 void five(unsigned ch=36){channel.number=int(ch);channel.ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_PASSIVE;}
 int state(ieee80211_state s){return c.state8812(&c.core_.sc_ic,s,-1);}
 void run(){assert(state(IEEE80211_S_AUTH)==0);assert(state(IEEE80211_S_ASSOC)==0);assert(state(IEEE80211_S_RUN)==0);}
};
static uint32_t reg(uint16_t a,unsigned width){uint32_t v=0;for(unsigned i=0;i<width;++i)v|=(uint32_t)regs[a+i]<<(8*i);return v;}
static mbuf_t packet(bool data,bool eapol,size_t len=80){auto*p=new TestPacket;p->bytes.assign(len,0);auto*h=(ieee80211_frame*)p->bytes.data();h->i_fc[0]=data?8:0x40;h->i_addr1[0]=2;h->i_seq[0]=0x10;
 if(eapol){const uint8_t llc[8]={0xaa,0xaa,3,0,0,0,0x88,0x8e};memcpy(p->bytes.data()+24,llc,8);}return p;}
static void set32(std::vector<uint8_t>&v,size_t off,uint32_t x){for(unsigned i=0;i<4;++i)v[off+i]=(uint8_t)(x>>(8*i));}
static void appendRX(std::vector<uint8_t>&v,unsigned rate,bool c2h){size_t off=v.size();size_t payload=c2h?6:28;v.resize(off+24+payload);set32(v,off,(uint32_t)payload);set32(v,off+8,c2h?0x10000000:0);set32(v,off+12,rate);v[off+24]=c2h?0x0c:0x80;v[off+24+payload-1]=0x7b;while(v.size()%8)v.push_back(0);}
static void shiftedPHYReports(){
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})for(unsigned shift=0;shift<4;++shift)for(unsigned rate:{0U,4U}){
  Fixture f(family);f.run();
  // Linux rx.c: descriptor, SHIFT padding, 32-byte PHY, then the MPDU.
  // Poison padding so decoding it as gain cannot satisfy these literals.
  std::vector<uint8_t> bytes((24U+shift+32U+24U+7U)&~size_t(7U),0);
  set32(bytes,0,24U|0x04040000U|(shift<<24U));set32(bytes,12,rate);
  for(unsigned i=0;i<shift;++i)bytes[24U+i]=0x7f;
  bytes[24U+shift]=bytes[25U+shift]=70; // OFDM: 70-110=-40 dBm.
  bytes[29U+shift]=uint8_t((5U<<5U)|10U); // Distinct chip CCK formulas.
  const size_t mpdu=56U+shift;bytes[mpdu]=0x80;
  memcpy(bytes.data()+mpdu+10U,f.n.ni_bssid,6);
  memcpy(bytes.data()+mpdu+16U,f.n.ni_bssid,6);
  rtw_rx_iterator it{bytes.data(),bytes.size(),0,0};rtw_8192eu_rx_frame out{};uint8_t report=0;
  assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_OK&&!report);
  const int dbm=rate==4U?-40:family==RTW_WIFI_RTL8812AU?-42:-58;
  const unsigned percent=rate==4U?60:family==RTW_WIFI_RTL8812AU?61:42;
  assert(f.c.sample8812RSSI(out.payload)==dbm);
  assert(out.payload.phy_info==bytes.data()+24U+shift&&out.payload.mpdu==bytes.data()+mpdu);
  assert(f.c.auRSSIValid_&&f.c.auRSSI_==percent);
  assert(f.c.input8812Coex().rssi_percent==percent);
  assert(!f.c.tick8812());
  assert(commands.back()[0]==0x42&&commands.back()[3]==percent);
 }
 puts("AU RXSHIFT0..3 CCK/OFDM PHY samples reach firmware RSSI and coex inputs without padding");
}
/* Every state enters through the real adapter before MAC/mailbox actions.
 * The isolated PHY/power helpers are also observed: zero bus calls alone
 * cannot prove that a helper containing real USB writes was not dispatched. */
static void kernelHealthSurface(){
 unsigned cases=0;
 for(unsigned board=0;board<4;++board)for(unsigned action=0;action<7;++action)for(unsigned fault=0;fault<9;++fault){
  Fixture f(board==0?RTW_WIFI_RTL8812AU:RTW_WIFI_RTL8821AU);if(board>=2)f.coex(board==3);f.cold();assert(!f.c.start8812());
  if(action<3||action==6)f.run();if(action>=4)assert(!f.state(IEEE80211_S_AUTH));if(action==5)assert(!f.state(IEEE80211_S_ASSOC));
  f.n.ni_port_valid=true;auto sent=commands.size();auto calls=ioCalls;size_t written=writes.size();events.clear();readAddresses.clear();
  if(fault<3)regs[0x80]=fault==0?0xc7:fault==1?0xff:0xc2;
  else if(fault==3)failIO=ioCalls+1;else if(fault==4)f.c.auCore_.io_error=EIO;
  else if(fault==5)f.c.auCore_.bus.context=(void*)uintptr_t(1);
  else if(fault==6)f.c.present_=false;else if(fault==7)f.c.sleeping_=true;else f.c.usbError_=EIO;
  const ieee80211_state target[]={IEEE80211_S_INIT,IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
  int e=0;if(action==0)f.c.stop8812();else if(action==6)RTWUSBController::update8812EDCA(&f.c.core_.sc_ic);else e=f.state(target[action]);
  assert(commands.size()==sent&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&!f.n.ni_port_valid);
  assert(!f.c.auCore_.firmware_running&&!f.c.auCore_.associated&&!f.c.auRateReady_&&!f.c.auMediaConnected_&&!f.c.radioReady_);
  assert(!f.phy.status.calibrated&&!f.phy.status.tx_enabled&&!f.c.authorized());
  int expected=fault==3||fault==4?EIO:ENXIO;if(action&&action!=6)assert(e==expected);assert(f.c.core_.core_error==expected);
  if(fault>=4){assert(ioCalls==calls&&readAddresses.empty());assert(std::find(events.begin(),events.end(),12)==events.end()&&std::find(events.begin(),events.end(),91)==events.end());}
  else assert(!readAddresses.empty()&&readAddresses[0]==0x80);
  for(size_t n=written;n<writes.size();++n){auto a=writes[n].address;assert(a!=0x6a8&&a!=0x6a2&&a!=0x6a4&&a!=0x618&&a!=0x61c&&a!=0x554&&a!=0x440&&a!=0x51b);}
  ++cases;
 }
 printf("AU kernel health admission: %u full-state/stop fault cases; invalid MCU, read error, poisoned transport and foreign owner denied before MAC/H2C\n",cases);
}

static void healthyEDCA(){
 for(unsigned board=0;board<4;++board)for(bool five:{false,true})for(bool shortSlot:{false,true}){
  Fixture f(board==0?RTW_WIFI_RTL8812AU:RTW_WIFI_RTL8821AU);if(board>=2)f.coex(board==3);if(five)f.five();f.cold();assert(!f.c.start8812());f.run();f.n.ni_port_valid=true;
  auto&ic=f.c.core_.sc_ic;if(shortSlot)ic.ic_flags|=IEEE80211_F_SHSLOT;else ic.ic_flags&=~unsigned(IEEE80211_F_SHSLOT);
  ic.ic_flags|=IEEE80211_F_QOS;f.n.ni_flags|=IEEE80211_NODE_QOS;
  const ieee80211_edca_ac_params ap[4]={{3,7,2,47},{4,10,7,0},{2,4,2,94},{1,3,2,47}};
  for(unsigned i=0;i<4;++i){ic.ic_edca_ac[i]=ap[i];for(unsigned b=0;b<4;++b)regs[0x500+4*i+b]=uint8_t(0xf8a5a5a5U>>(8*b));}
  for(bool negotiated:{true,false}){
   if(!negotiated){ic.ic_flags&=~unsigned(IEEE80211_F_QOS);f.n.ni_flags&=~unsigned(IEEE80211_NODE_QOS);}
   auto before=ioCalls;RTWUSBController::update8812EDCA(&ic);assert(ioCalls==before+10&&!f.c.core_.core_error&&f.n.ni_port_valid&&f.c.uses8812FirmwareRate());
   unsigned slot=five||shortSlot?9:20,sifs=five?16:10;
   const auto*expected=negotiated?ap:ieee80211_edca_table[five?IEEE80211_MODE_11A:IEEE80211_MODE_11G];
   for(unsigned i=0;i<4;++i){const auto&a=expected[i];uint32_t value=(uint32_t(a.ac_txoplimit)<<16)|(unsigned(a.ac_ecwmax)<<12)|(unsigned(a.ac_ecwmin)<<8)|(a.ac_aifsn*slot+sifs);const uint16_t address[4]={0x508,0x50c,0x504,0x500};assert(reg(address[i],4)==(0xf8000000U|value));}
  }
 }
}

static void globalOwnerSurface(){
 unsigned cases=0;
 for(unsigned board=0;board<4;++board)for(unsigned mutation=0;mutation<19;++mutation)for(unsigned action=0;action<13;++action){
  Fixture f(board==0?RTW_WIFI_RTL8812AU:RTW_WIFI_RTL8821AU);if(board>=2)f.coex(board==3);
  if(action==9){f.c.auHTSupported_=true;f.n.ni_flags|=IEEE80211_NODE_HT;f.n.ni_rxmcs[0]=255;f.n.ni_htcaps=IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT;}
  f.cold();assert(!f.c.start8812());if(action<3||action>=6)f.run();if(action==4||action==5)assert(!f.state(IEEE80211_S_AUTH));if(action==5)assert(!f.state(IEEE80211_S_ASSOC));
  f.n.ni_port_valid=true;f.n.ni_rsnprotos=IEEE80211_PROTO_RSN;f.c.core_.sc_ic.ic_flags|=IEEE80211_F_RSNON;f.n.ni_tx_ba[5].ba_winsize=32;
  if(action==9)assert(!RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,5));
  auto calls=ioCalls;auto sent=commands.size();auto submitted=f.c.submitted;auto crypto=cryptoCalls;events.clear();readAddresses.clear();
  switch(mutation){
   case 0:f.c.auCore_.bus.context=(void*)uintptr_t(1);break;
   case 1:f.c.auCore_.bus.control_in=reinterpret_cast<decltype(f.c.auCore_.bus.control_in)>(uintptr_t(1));break;
   case 2:f.c.auCore_.bus.control_out=reinterpret_cast<decltype(f.c.auCore_.bus.control_out)>(uintptr_t(1));break;
   case 3:f.c.auCore_.waiter.context=(void*)uintptr_t(1);break;
   case 4:f.c.auCore_.waiter.wait_us=reinterpret_cast<decltype(f.c.auCore_.waiter.wait_us)>(uintptr_t(1));break;
   case 5:f.c.auCore_.chip=f.c.auCore_.board.phy.chip=f.c.auCore_.chip==RTW_R12AU_CHIP_8812AU?RTW_R12AU_CHIP_8821AU:RTW_R12AU_CHIP_8812AU;break;
   case 6:f.c.auCore_.board.phy.rfe^=1;break;
   case 7:f.c.auCore_.board.phy.txpwr_idx[0][0]^=1;break;
   case 8:f.c.auCore_.logical_rom[10]^=1;break;
   case 9:f.c.auCore_.usb.bulk_out[0]^=1;break;
   case 10:f.c.family_=f.c.family_==RTW_WIFI_RTL8812AU?RTW_WIFI_RTL8821AU:RTW_WIFI_RTL8812AU;break;
   case 11:f.c.usb_=(void*)uintptr_t(1);break;
   case 12:f.c.rxPipe_=reinterpret_cast<TestPipe*>(uintptr_t(1));break;
   case 13:f.c.txPipes_[0]=reinterpret_cast<TestPipe*>(uintptr_t(1));break;
   case 14:f.c.txMPS_[0]^=1;break;
   case 15:f.c.endpoints_.interface_number^=1;break;
   case 16:f.c.auCore_.board.mac[0]^=4;break;
   case 17:f.c.auCore_.board.country[0]^=1;break;
   default:f.c.usbOpened_=false;break;
  }
  if(action==0)f.c.stop8812();
  else if(action<6){const ieee80211_state target[]={IEEE80211_S_INIT,IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};assert(f.state(target[action])==ENXIO);}
  else if(action==6)RTWUSBController::update8812EDCA(&f.c.core_.sc_ic);
  else if(action==7){auto*p=packet(true,false);p->bytes[1]|=IEEE80211_FC1_PROTECTED;keyAvailable=encryptSuccess=true;assert(f.c.send8812Frame(p,&f.n)==ENETDOWN);keyAvailable=encryptSuccess=false;}
  else if(action==8)assert(!f.c.uses8812FirmwareRate());
  else if(action==9)assert(RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,5)==EINVAL);
  else if(action==10){rtw_rx_packet rx{};rx.rate_index=0;assert(!f.c.accepts8812ReceiveRate(rx));}
  else if(action==11){auto reports=f.c.rateControl_.reports;uint8_t report[6]={0x0c,0,11,0,0,0};f.c.handle8812C2H(report,sizeof(report));assert(f.c.rateControl_.reports==reports);}
  else assert(f.c.tick8812()==ENXIO);
  assert(ioCalls==calls&&readAddresses.empty()&&commands.size()==sent&&f.c.submitted==submitted&&cryptoCalls==crypto);
  assert(std::find(events.begin(),events.end(),12)==events.end()&&std::find(events.begin(),events.end(),91)==events.end());
  if(action<7||action==12)assert(f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&!f.n.ni_port_valid&&!f.c.auCore_.firmware_running&&!f.c.auRateReady_&&!f.c.radioReady_&&!f.phy.status.calibrated&&!f.phy.status.tx_enabled);
  ++cases;
 }
 printf("AU global ownership: %u BT/non-BT state, stop, EDCA, protected TX, rate, BA, RX, C2H and timer cases; every changed binding denied without I/O or foreign cleanup\n",cases);
}

static void healthyOwnerRestart(){
 for(unsigned board=0;board<4;++board){Fixture f(board==0?RTW_WIFI_RTL8812AU:RTW_WIFI_RTL8821AU);if(board>=2)f.coex(board==3);f.cold();
  for(unsigned cycle=0;cycle<2;++cycle){assert(!f.c.start8812());f.run();f.n.ni_port_valid=true;assert(f.c.owns8812Authority()&&f.c.uses8812FirmwareRate());
   ++f.c.auCore_.firmware_version;++f.c.auCore_.firmware_subversion;f.c.auCore_.rx_config^=1;assert(f.c.owns8812Authority()&&f.c.uses8812FirmwareRate());
   f.c.stop8812();assert(f.c.owns8812Authority()&&!f.n.ni_port_valid&&!f.c.auCore_.firmware_running&&!f.c.auPHY_);
  }
 }
}

static void profileBandAdmission(){
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU}){
  Fixture f(family);f.c.testProfile_.band_restriction=2;
  assert(!f.c.supports8812Band(false)&&f.c.supports8812Band(true));
  unsigned before=ioCalls;assert(f.c.set8812Channel(6)==ENOTSUP&&ioCalls==before);
  f.five();f.run();assert(f.c.uses8812FirmwareRate());
  f.c.auChannel_=6;assert(!f.c.uses8812FirmwareRate());
  f.n.ni_port_valid=1;before=ioCalls;assert(f.c.send8812Frame(packet(true,false),&f.n)==ENETDOWN&&ioCalls==before);
 }
}
int main(){shiftedPHYReports();profileBandAdmission();globalOwnerSurface();kernelHealthSurface();healthyEDCA();healthyOwnerRestart();
 for(bool shared:{false,true}){Fixture f(RTW_WIFI_RTL8821AU);f.coex(shared);f.cold();assert(!f.c.start8812());rtw_r21au_coex_status status{};rtw_r21au_coex_get_status(f.c.auCoex_,&status);assert(status.active&&status.share_ant==shared&&f.phy.status.tx_enabled);events.clear();testClock+=2000;assert(!f.c.tick8812()&&std::find(events.begin(),events.end(),35)==events.end());f.run();assert(f.c.uses8812FirmwareRate()&&!f.n.ni_port_valid);assert(f.c.send8812Frame(packet(true,false),&f.n)==ENETDOWN);assert(!f.c.send8812Frame(packet(true,true),&f.n));f.n.ni_port_valid=true;assert(!f.c.send8812Frame(packet(true,false),&f.n));assert(f.c.auCoexTxFrames_==2&&f.c.auCoexTxBytes_>0);uint8_t btinfo[9]={9,0,1,0x49,0,20,0,0,40};f.c.handle8812C2H(btinfo,9);rtw_r21au_coex_get_status(f.c.auCoex_,&status);assert(status.bt_profiles==1&&status.bt_info_events==1);testClock+=2000;assert(!f.c.tick8812());assert(!f.c.auCoexTxFrames_&&!f.c.auCoexTxBytes_);f.c.stop8812();assert(!f.c.auCoex_&&!f.c.auPHY_&&!f.c.auCore_.firmware_running&&!f.n.ni_port_valid);}

 unsigned coexFaults=0;
 for(bool shared:{false,true}){unsigned startup=0;{Fixture good(RTW_WIFI_RTL8821AU);good.coex(shared);good.cold();assert(!good.c.start8812());startup=ioCalls;}
  for(unsigned fault=1;fault<=startup;++fault){Fixture f(RTW_WIFI_RTL8821AU);f.coex(shared);f.cold();failIO=fault;assert(f.c.start8812()==EIO);assert(!f.c.auCoex_&&!f.c.auPHY_&&!f.c.running_&&!f.c.radioReady_&&!f.c.auCore_.firmware_running&&!f.n.ni_port_valid);++coexFaults;}
  {Fixture f(RTW_WIFI_RTL8821AU);f.coex(shared);f.cold();failEvent=36;assert(f.c.start8812()==EIO&&!f.c.running_&&!f.c.auCoex_&&!f.c.auPHY_);}
  {Fixture f(RTW_WIFI_RTL8821AU);f.coex(shared);f.five();f.c.auCore_.board.vht=1;f.c.auHTSupported_=f.c.auVHTSupported_=true;f.n.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_VHT;f.n.ni_rxmcs[0]=255;f.n.ni_vht_mcsinfo.rx_mcs_map=0xfffe;f.n.ni_chw=IEEE80211_CHAN_WIDTH_80;f.n.ni_vht_chan1=42;f.cold();assert(!f.c.start8812());f.run();rtw_r21au_coex_status s{};rtw_r21au_coex_get_status(f.c.auCoex_,&s);assert(s.active&&s.under_5g&&f.c.auOperating_.width_mhz==80&&f.c.auRateConfig_.streams==1);f.n.ni_port_valid=true;assert(!f.c.send8812Frame(packet(true,false),&f.n));assert(((f.c.submittedBytes[20]>>5)&3)==2);}
 }
 printf("AU actual coex bridge: %u startup I/O faults, both antenna routes, pre-RUN watchdog, WPA port/EAPOL, 1SS VHT80, and no-I/O removal passed\n",coexFaults);
 {Fixture f(RTW_WIFI_RTL8821AU);f.coex(true);f.cold();assert(!f.c.start8812());f.run();f.n.ni_port_valid=true;auto calls=ioCalls;auto sent=commands.size();events.clear();f.c.auCore_.bus.context=(void*)uintptr_t(1);f.c.stop8812();assert(ioCalls==calls&&commands.size()==sent&&!f.n.ni_port_valid&&!f.c.auCoex_&&!f.phy.status.calibrated&&!f.phy.status.tx_enabled);assert(std::find(events.begin(),events.end(),12)==events.end()&&std::find(events.begin(),events.end(),91)==events.end());}
 for(unsigned changed=0;changed<3;++changed){Fixture f(RTW_WIFI_RTL8821AU);f.coex(true);f.cold();assert(!f.c.start8812());f.run();f.n.ni_port_valid=true;auto calls=ioCalls;auto sent=commands.size();events.clear();if(changed==0)f.c.auCore_.board.phy.rfe=1;else if(changed==1)f.c.auCore_.chip=f.c.auCore_.board.phy.chip=RTW_R12AU_CHIP_8812AU;else f.c.auCore_.waiter.context=(void*)uintptr_t(1);f.c.stop8812();assert(ioCalls==calls&&commands.size()==sent&&!f.n.ni_port_valid&&!f.c.auCoex_&&!f.phy.status.calibrated&&!f.phy.status.tx_enabled);assert(std::find(events.begin(),events.end(),12)==events.end()&&std::find(events.begin(),events.end(),91)==events.end());}
 {Fixture f(RTW_WIFI_RTL8821AU);f.coex(true);f.cold();assert(!f.c.start8812());f.run();f.n.ni_port_valid=true;auto calls=ioCalls;f.c.present_=false;f.c.stop8812();assert(ioCalls==calls&&!f.n.ni_port_valid&&!f.c.auCoex_&&!f.phy.status.calibrated&&!f.phy.status.tx_enabled);}

 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU}){Fixture f(family);uint8_t phy[32]{};rtw_rx_packet p{};p.rate_index=0;p.phy_status_present=1;p.phy_info=phy;p.phy_info_length=32;int dbm=f.c.sample8812RSSI(p);assert(dbm==-1&&rtw_signal_valid_dbm(dbm));p.phy_status_present=0;assert(f.c.sample8812RSSI(p)==0);}

 for(unsigned topology:{2U,1U,0U})for(bool vht:{false,true})for(unsigned smps:{0U,1U,2U,3U})for(unsigned width:{20U,40U,80U}){
  const auto family=topology?RTW_WIFI_RTL8812AU:RTW_WIFI_RTL8821AU;
  if(!vht&&width==80)continue;Fixture f(family);f.c.auCore_.board.phy.rf_paths=topology==2?2:1;if(vht)f.five();f.c.auCore_.board.vht=vht;f.c.auHTSupported_=true;f.c.auVHTSupported_=vht;f.n.ni_flags=IEEE80211_NODE_HT|(vht?IEEE80211_NODE_VHT:0);f.n.ni_rxmcs[0]=f.n.ni_rxmcs[1]=255;f.n.ni_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40|(smps<<IEEE80211_HTCAP_SMPS_SHIFT);f.c.core_.sc_ic.ic_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;f.n.ni_vhtcaps=f.c.core_.sc_ic.ic_vhtcaps=IEEE80211_VHTCAP_SHORT_GI_80;f.n.ni_vht_mcsinfo.rx_mcs_map=0xfffa;f.n.ni_chw=width==80?IEEE80211_CHAN_WIDTH_80:width==40?IEEE80211_CHAN_WIDTH_40:IEEE80211_CHAN_WIDTH_20;f.n.ni_vht_chan1=42;f.n.ni_htop0=IEEE80211_HTOP0_SCO_SCA;f.c.remember8812Authority();f.run();
  bool two=topology==2&&smps==IEEE80211_HTCAP_SMPS_DIS;assert(f.c.auRateConfig_.streams==(two?2:1));if(!vht)assert((f.c.auRateConfig_.mask&0x0ff00000U)==(two?0x0ff00000U:0));else assert((f.c.auRateConfig_.mask&0xffc00000U)==(two?(width==20?0x7fc00000U:0xffc00000U):0));f.n.ni_port_valid=true;assert(!f.c.send8812Frame(packet(true,false),&f.n));assert((f.c.submittedBytes[6]&31)==f.c.auRateConfig_.rate_id);
  rtw_rx_packet rx{};rx.rate_index=vht?62:27;rx.bandwidth_code=width==80?2:width==40?1:0;assert(f.c.accepts8812ReceiveRate(rx)==(topology==2));uint8_t report[6]={0x0c,0,(uint8_t)(vht?62:27),0,0,0};f.c.handle8812C2H(report,6);assert(f.c.rateControl_.reports==(two?1U:0U)&&f.c.rateControl_.invalid==(two?0U:1U));
 }

 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})for(bool five:{false,true}){
  Fixture f(family);if(five)f.five();assert(f.c.supports8812Band(false)&&f.c.supports8812Band(true));
  assert(!f.state(IEEE80211_S_SCAN));assert(!f.c.auRateReady_&&!f.c.auMediaConnected_);assert(f.phy.status.tx_enabled==!five);
  assert(!f.state(IEEE80211_S_AUTH));auto a=std::find(events.begin(),events.end(),62);assert(a!=events.end()&&std::find(events.begin(),a,11)!=a);
  assert(reg(0x618,1)==2);assert(!f.state(IEEE80211_S_ASSOC));assert(!f.state(IEEE80211_S_RUN));
  assert(f.c.auMediaConnected_&&f.c.auRateReady_&&f.c.uses8812FirmwareRate());assert(!f.n.ni_port_valid&&!f.c.authorized());
  assert(f.c.auRateConfig_.mask==(five?0x810:0x811));assert(f.c.auRateConfig_.rate_id==(five?7:6));
  assert(commands.size()==2&&commands[0][0]==0x40&&commands[1][0]==1&&commands[1][1]==1);
  assert(reg(0x6a8,2)==1&&(reg(0x100,4)&0x30000)==0x20000);assert(reg(0x440,3)==(five?0x10:1));assert(reg(0x51b,1)==(five?9:20));
  f.n.ni_port_valid=true;assert(f.c.authorized());assert(!f.state(IEEE80211_S_INIT));assert(!f.n.ni_port_valid&&!f.c.authorized()&&!f.phy.status.tx_enabled);
 }
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU}){Fixture f(family);f.cold();regs[0x806]=1;assert(!f.c.start8812());assert(f.c.auCckHighPower_);assert(f.c.radioReady_&&f.c.running_&&f.c.auCore_.mac_initialized&&f.c.auCore_.firmware_running&&f.c.auPHY_==&f.phy&&f.phy.status.calibrated&&f.phy.status.tx_enabled);
  size_t last=0;for(int stage:{1,2,3,4,5,6,7,10,11,14,61}){auto it=std::find(events.begin()+last,events.end(),stage);assert(it!=events.end());last=(size_t)(it-events.begin())+1;}
  f.c.stop8812();assert(!f.c.radioReady_&&!f.c.running_&&!f.c.auPHY_&&!f.c.auCore_.firmware_running&&!f.c.auCore_.mac_initialized&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);
 }
 for(int stage:{1,2,3,4,5,6,7,10,11,14,61}){Fixture f;f.cold();failEvent=stage;if(stage==61)genericFail=IEEE80211_S_SCAN;assert(f.c.start8812()==(stage==61?EINVAL:EIO));assert(!f.c.auPHY_&&!f.c.running_&&!f.c.radioReady_&&!f.c.auCore_.powered&&!f.c.auCore_.mac_initialized&&!f.c.auCore_.firmware_running&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 {Fixture f;f.run();events.clear();assert(f.state(static_cast<ieee80211_state>(5))==EINVAL);assert(f.c.core_.sc_ic.ic_state==IEEE80211_S_RUN&&events.empty()&&f.c.auRateReady_);}
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})for(unsigned ch:{52U,56U,60U,64U,100U,104U,108U,112U,116U,120U,124U,128U,132U,136U,140U})for(unsigned width:{20U,40U,80U}){
  Fixture f(family);f.five(ch);f.c.auCore_.board.vht=1;f.c.auHTSupported_=f.c.auVHTSupported_=true;f.n.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_VHT;f.n.ni_rxmcs[0]=f.n.ni_rxmcs[1]=255;f.n.ni_chw=width==20?IEEE80211_CHAN_WIDTH_20:width==40?IEEE80211_CHAN_WIDTH_40:IEEE80211_CHAN_WIDTH_80;f.n.ni_vht_chan1=58;f.n.ni_htop0=IEEE80211_HTOP0_SCO_SCA;f.n.ni_vht_mcsinfo.rx_mcs_map=0xfffa;
  f.c.remember8812Authority();assert(f.c.channel8812Modes(uint8_t(ch))==IEEE80211_CHAN_HT20);
  assert(!f.state(IEEE80211_S_SCAN)&&!f.phy.status.tx_enabled);f.run();
  assert(f.c.auOperating_.primary==ch&&f.c.auOperating_.center==ch&&f.c.auOperating_.width_mhz==20&&f.c.auOperating_.mode==RTW_R12AU_MODE_HT);
  assert(f.c.auRateConfig_.ht&&!f.c.auRateConfig_.vht&&f.c.auRateConfig_.bandwidth_mhz==20&&!f.n.ni_port_valid);
 }
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})for(unsigned width:{20U,40U,80U}){
  Fixture f(family);f.five();f.c.auCore_.board.vht=1;f.c.auHTSupported_=f.c.auVHTSupported_=true;f.n.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_VHT;f.n.ni_rxmcs[0]=f.n.ni_rxmcs[1]=255;f.n.ni_chw=width==20?IEEE80211_CHAN_WIDTH_20:width==40?IEEE80211_CHAN_WIDTH_40:IEEE80211_CHAN_WIDTH_80;f.n.ni_vht_chan1=42;f.n.ni_htop0=IEEE80211_HTOP0_SCO_SCA;f.n.ni_vht_mcsinfo.rx_mcs_map=0xfffa;f.n.ni_htcaps=f.c.core_.sc_ic.ic_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40|(IEEE80211_HTCAP_SMPS_DIS<<IEEE80211_HTCAP_SMPS_SHIFT);f.n.ni_vhtcaps=f.c.core_.sc_ic.ic_vhtcaps=IEEE80211_VHTCAP_SHORT_GI_80;
  f.c.remember8812Authority();f.run();assert(f.c.auRateConfig_.vht&&f.c.auOperating_.width_mhz==width&&f.c.uses8812HighThroughput());assert(!f.n.ni_port_valid);assert(reg(0x6a2,2)==0x100);
  rtw_rx_packet rx{};rx.rate_index=family==RTW_WIFI_RTL8812AU?62:52;rx.bandwidth_code=width==80?2:width==40?1:0;assert(f.c.accepts8812ReceiveRate(rx));rx.rate_index=63;rx.bandwidth_code=2;if(family==RTW_WIFI_RTL8821AU||width<80)assert(!f.c.accepts8812ReceiveRate(rx));
  f.n.ni_port_valid=true;f.n.ni_rsnprotos=IEEE80211_PROTO_RSN;f.c.core_.sc_ic.ic_flags|=IEEE80211_F_RSNON;keyAvailable=encryptSuccess=true;f.n.ni_tx_ba[5].ba_state=IEEE80211_BA_AGREED;f.n.ni_tx_ba[5].ba_winsize=32;f.n.ni_ampdu_param=3|(7<<2);auto beforeBA=ioCalls;assert(!RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,5)&&ioCalls==beforeBA);f.n.ni_port_valid=false;assert(RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,5)==EINVAL);f.n.ni_port_valid=true;assert(RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,8)==EINVAL);f.c.core_.sc_ic.ic_tid_noack=1U<<5;assert(RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,5)==EINVAL);f.c.core_.sc_ic.ic_tid_noack=0;f.n.ni_tx_ba[5].ba_winsize=65;assert(RTWUSBController::ampdu8812TXStart(&f.c.core_.sc_ic,&f.n,5)==EINVAL);f.n.ni_tx_ba[5].ba_winsize=32;
  auto*packetWithQoS=packet(true,false);packetWithQoS->bytes.insert(packetWithQoS->bytes.begin()+24,2,0);packetWithQoS->bytes[0]|=0x80;packetWithQoS->bytes[1]|=IEEE80211_FC1_PROTECTED;packetWithQoS->bytes[24]=5;
  assert(!f.c.send8812Frame(packetWithQoS,&f.n));assert(f.c.txAMPDUPackets_==1&&(f.c.submittedBytes[9]&0x10));f.n.ni_tx_ba[5].ba_winsize=8;auto*small=packet(true,false);small->bytes.insert(small->bytes.begin()+24,2,0);small->bytes[0]|=0x80;small->bytes[1]|=IEEE80211_FC1_PROTECTED;small->bytes[24]=5;assert(!f.c.send8812Frame(small,&f.n));assert(((f.c.submittedBytes[14]>>1)&31)==8);encryptSuccess=keyAvailable=false;assert(!f.state(IEEE80211_S_INIT));assert(reg(0x6a2,2)==0&&reg(0x6a4,2)==0);
 }
 {Fixture f;f.c.auHTSupported_=f.c.auVHTSupported_=true;auto&ic=f.c.core_.sc_ic;ic.ic_modecaps=(1U<<IEEE80211_MODE_11N)|(1U<<IEEE80211_MODE_11AC)|(1U<<IEEE80211_MODE_11AX);ic.ic_flags=IEEE80211_F_HEON;f.c.refresh8812Modes();assert(!(ic.ic_modecaps&(1U<<IEEE80211_MODE_11AX))&&!(ic.ic_flags&IEEE80211_F_HEON)&&(ic.ic_flags&IEEE80211_F_HTON)&&(ic.ic_flags&IEEE80211_F_VHTON));ic.ic_userflags=IEEE80211_F_NOVHT;f.c.refresh8812Modes();assert((ic.ic_flags&IEEE80211_F_HTON)&&!(ic.ic_flags&IEEE80211_F_VHTON));ic.ic_curmode=IEEE80211_MODE_11G;f.c.refresh8812Modes();assert(!(ic.ic_flags&(IEEE80211_F_HTON|IEEE80211_F_VHTON)));}
 unsigned ioPositions=0;{Fixture f;assert(!f.state(IEEE80211_S_AUTH));ioPositions=ioCalls;}
 for(unsigned failure=1;failure<=ioPositions;++failure){Fixture f;failIO=failure;assert(f.state(IEEE80211_S_AUTH)==EIO);assert(!f.c.auRateReady_&&!f.c.auMediaConnected_&&!f.phy.status.tx_enabled&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 unsigned runIO=0;{Fixture f;assert(!f.state(IEEE80211_S_AUTH));unsigned initial=ioCalls;assert(!f.state(IEEE80211_S_RUN));runIO=ioCalls-initial;}
 for(unsigned failure=1;failure<=runIO;++failure){Fixture f;assert(!f.state(IEEE80211_S_AUTH));failIO=ioCalls+failure;assert(f.state(IEEE80211_S_RUN)==EIO);assert(!f.c.auRateReady_&&!f.c.auMediaConnected_&&!f.phy.status.tx_enabled&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 for(int point:{10,11,13}){Fixture f;failEvent=point;assert(f.state(IEEE80211_S_AUTH)==EIO);assert(!f.c.authorized()&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 for(int point:{30,32,56,64}){Fixture f;assert(!f.state(IEEE80211_S_AUTH));failEvent=point;if(point==64)genericFail=IEEE80211_S_RUN;
  assert(f.state(IEEE80211_S_RUN)==(point==64?EINVAL:EIO));assert(!f.c.auRateReady_&&!f.c.auMediaConnected_&&!f.c.authorized()&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 {Fixture f;f.run();failIO=ioCalls+1;f.n.ni_port_valid=true;assert(f.state(IEEE80211_S_INIT)==EIO);assert(!f.n.ni_port_valid&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT);}
 {Fixture f;f.run();failEvent=33;f.n.ni_port_valid=true;assert(f.state(IEEE80211_S_INIT)==EIO);assert(!f.n.ni_port_valid&&!f.phy.status.calibrated&&!f.phy.status.tx_enabled&&!f.c.auChannel_);}
 {Fixture f;f.five();f.c.auCore_.board.phy.rfe=9;f.c.remember8812Authority();assert(f.state(IEEE80211_S_AUTH)==ENOTSUP);assert(!f.c.auRateReady_);}
 {Fixture f;f.n.ni_rates.rs_nrates=0;assert(!f.state(IEEE80211_S_AUTH));assert(f.state(IEEE80211_S_RUN)==EINVAL);assert(!f.c.auRateReady_);}
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})for(bool five:{false,true}){
  Fixture f(family);if(five)f.five();f.run();assert(!f.c.uses8812HighThroughput());keyAvailable=encryptSuccess=true;
  auto qosPacket=[](bool eapol){auto*p=packet(true,eapol);p->bytes.insert(p->bytes.begin()+24,2,0);p->bytes[0]|=0x80;p->bytes[1]|=IEEE80211_FC1_PROTECTED;p->bytes[24]=5;return p;};
  auto before=cryptoCalls;assert(f.c.send8812Frame(qosPacket(false),&f.n)==ENETDOWN&&cryptoCalls==before);
  assert(!f.c.send8812Frame(qosPacket(true),&f.n));assert((f.c.submittedBytes[13]&1)==1&&!(f.c.submittedBytes[9]&0x10)&&!(f.c.submittedBytes[14]&0x3e)&&(f.c.submittedBytes[5]&31)==5);
  f.n.ni_port_valid=true;f.n.ni_tx_ba[5].ba_state=IEEE80211_BA_AGREED;f.n.ni_tx_ba[5].ba_winsize=64;f.n.ni_ampdu_param=31;
  assert(!f.c.send8812Frame(qosPacket(false),&f.n));assert((f.c.submittedBytes[13]&1)==0&&f.c.submittedBytes[6]==(five?7:6)&&!(f.c.submittedBytes[9]&0x10)&&!(f.c.submittedBytes[14]&0x3e)&&!f.c.txHTPackets_&&!f.c.txAMPDUPackets_);
  assert(!f.c.send8812Frame(qosPacket(true),&f.n));assert((f.c.submittedBytes[13]&1)==1&&!f.c.txAMPDUPackets_);keyAvailable=encryptSuccess=false;
 }
 for(unsigned count:{2,3,4}){Fixture f;f.usb(count);f.run();unsigned before=frees;
  assert(f.c.send8812Frame(packet(true,false),&f.n)==ENETDOWN&&frees==before+1&&f.c.submitted==0);
  before=releases;assert(!f.c.send8812Frame(packet(true,true,472),&f.n));assert(releases==before+1&&f.c.submittedBytes.size()==513&&f.c.submittedPipe==(count==2?1U:2U));
  assert((f.c.submittedBytes[13]&1)==1); /* fixed EAPOL rate */
  f.n.ni_port_valid=true;assert(!f.c.send8812Frame(packet(true,false),&f.n));assert((f.c.submittedBytes[13]&1)==0&&f.c.submittedBytes[6]==6);
  assert(!f.c.send8812Frame(packet(false,false),&f.n));assert(f.c.submittedPipe==(count==4?3U:0U));
  before=releases;f.c.submitError=EIO;assert(f.c.send8812Frame(packet(false,false),&f.n)==EIO&&releases==before);f.c.submitError=0;
  f.c.ledger_.pending=0xfff0;assert(f.c.send8812Frame(packet(false,false),&f.n)==ENOBUFS);f.c.ledger_.pending=0;
  auto*p=packet(true,true);p->bytes[0]|=0x80;p->bytes.resize(24);assert(f.c.send8812Frame(p,&f.n)==EINVAL);
  p=packet(true,true);p->bytes[1]|=0x40;keyAvailable=false;assert(f.c.send8812Frame(p,&f.n)==EINVAL);keyAvailable=true;
  p=packet(true,true);p->bytes[1]|=0x40;before=cryptoCalls;assert(f.c.send8812Frame(p,&f.n)==ENOBUFS&&cryptoCalls==before+1);
  f.c.auCore_.io_error=EIO;assert(f.c.send8812Frame(packet(false,false),&f.n)==ENETDOWN);
 }
 {Fixture f;f.n.ni_rates={2,{0x8c,108}};f.run();f.n.ni_port_valid=true;f.c.core_.sc_ic.ic_rtsthreshold=40;assert(!f.c.send8812Frame(packet(true,false),&f.n));assert((f.c.submittedBytes[6]&31)==7&&f.c.submittedBytes[19]==4);}
 {Fixture f;std::vector<uint8_t> v;appendRX(v,12,false);appendRX(v,4,false);appendRX(v,0,true);rtw_rx_iterator it{v.data(),v.size(),0,0};rtw_8192eu_rx_frame out{};uint8_t report=0;
  assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_OK&&out.payload.rate_index==12&&!report);
  assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_OK&&out.payload.mpdu_length==28&&out.payload.mpdu[27]==0x7b);
  assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_OK&&report==5&&out.payload.mpdu_length==6);
  assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_DONE);
  it.cursor=0;it.frames=65;assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_BAD_ARGUMENT);
  it.frames=0;it.length=8;assert(f.c.decode8812USBFrame(&it,&out,&report)==RTW_PACKETS_TRUNCATED&&it.cursor==0);
 }
 // Download mode and an all-ones/lost USB status cannot retain RUN or keys.
 // Execute the actual watchdog method for both AU firmware families.
 for(auto family:{RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU})for(int status:{0xc7,0xff,0,-1}){
  Fixture f(family);f.run();f.n.ni_port_valid=true;f.c.auCore_.associated=1;
  regs[0x80]=(uint8_t)status;if(status<0)failIO=ioCalls+1;
  const auto sent=commands.size();const auto thermal=std::count(events.begin(),events.end(),35);
  assert(f.c.tick8812()==EIO);
  assert(!f.c.auCore_.firmware_running&&!f.c.auCore_.associated&&!f.c.auRateReady_&&!f.c.auMediaConnected_&&!f.c.radioReady_&&!f.c.authorized());
  assert(!f.n.ni_port_valid&&f.c.core_.sc_ic.ic_state==IEEE80211_S_INIT&&f.c.core_.core_error==EIO);
  assert(!f.phy.status.calibrated&&!f.phy.status.tx_enabled&&commands.size()==sent);
  assert(std::count(events.begin(),events.end(),35)==thermal);
 }
 {Fixture f;f.run();regs[0x80]=0xc6;assert(!f.c.tick8812());f.c.auRSSI_=51;f.c.auRSSIValid_=true;assert(!f.c.tick8812()&&commands.back()[0]==0x42&&commands.back()[3]==51);
  uint8_t report[6]={0x0c,0,11,0,0,0};f.c.handle8812C2H(report,6);assert(f.c.rateControl_.reports==1&&f.c.rateControl_.finalRate==11);report[2]=12;f.c.handle8812C2H(report,6);assert(f.c.rateControl_.invalid==1);
  regs[0x80]=0;assert(f.c.tick8812()==EIO&&!f.c.auCore_.firmware_running&&!f.c.authorized());
 }
 printf("RTL8812AU/RTL8821AU actual backend state/TX/RX boundary traces passed; %u AUTH + %u RUN I/O failure positions\n",ioPositions,runIO);
}
