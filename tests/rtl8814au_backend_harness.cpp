#include "rtw/RTL8814AU.h"
#include "rtw/RTL8814AUStation.h"
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/StationChannels.h"
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <vector>
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_M_STA=1,IEEE80211_F_SHSLOT=0x20000,IEEE80211_RATE_BASIC=128};
struct ieee80211_rateset {uint8_t rs_nrates,rs_rates[32];};
struct ieee80211_channel {int number;unsigned ic_flags=0;};
enum {IEEE80211_CHAN_5GHZ=0x0100,IEEE80211_CHAN_PASSIVE=0x0200};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
enum {IEEE80211_NODE_HT=1,IEEE80211_HTCAP_SGI20=32};
struct ieee80211_node {
 unsigned ni_flags=0,ni_htcaps=0;uint8_t ni_rxmcs[10]{};ieee80211_channel *ni_chan;uint8_t ni_bssid[6];uint16_t ni_associd,ni_intval;ieee80211_rateset ni_rates;};
struct _ifnet {};
struct ieee80211com {unsigned ic_htcaps=0;int ic_opmode;ieee80211_state ic_state;unsigned ic_flags;uint8_t ic_myaddr[6];ieee80211_node *ic_bss;_ifnet ic_if;};
struct rtwn_softc {ieee80211com sc_ic;int core_error=0;int(*sc_newstate)(ieee80211com *,ieee80211_state,int)=nullptr;};
static std::vector<int> events;static int injected=0,generic_error=0;static bool allow_5g=true;static uint8_t denied_channel=0;static rtw_8814au_station_params last_station{};
static int event(int e){events.push_back(e);return injected==e?EIO:0;}
static int ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return c->number;}
extern "C" int rtw_8814au_phy_channel_supported(const rtw_8814au*,uint8_t ch,unsigned width){return width==20&&ch!=denied_channel&&((ch>=1&&ch<=11)||(allow_5g&&rtw_station_5ghz_channel(ch)));}
extern "C" int rtw_8814au_phy_supports_5g(const rtw_8814au*){return allow_5g;}
extern "C" int rtw_8814au_phy_set_channel(rtw_8814au *c,uint8_t,unsigned){int e=event(10);c->phy_initialized=0;return e;}
extern "C" int rtw_8814au_phy_calibrate(rtw_8814au*c){int e=event(11);c->phy_initialized=!e;return e;}
extern "C" int rtw_8814au_phy_set_scan_channel(rtw_8814au*,uint8_t,unsigned){return event(12);}
extern "C" int rtw_8814au_phy_set_passive_5g_channel(rtw_8814au*c,uint8_t,unsigned){int e=event(13);c->phy_initialized=0;return e;}
extern "C" int rtw_8814au_station_configure(rtw_8814au*,rtw_8814au_station_state st,const rtw_8814au_station_params*p){if(p)last_station=*p;return event(20+st);}
extern "C" int rtw_8814au_h2c_mailbox(rtw_8814au*,const uint8_t b[8]){return event(b[0]==0x40?30:b[0]==0x46?31:b[1]?32:33);}
extern "C" int rtw_8814au_write(rtw_8814au*,uint16_t a,unsigned,uint32_t){assert(a==0x554);return event(40);}
struct Timer {void setTimeoutMS(unsigned){events.push_back(51);}};
class RTWUSBController {public:rtwn_softc core_{};rtw_8814au acCore_{};rtw_8814au_rate_config acRateConfig_{};bool acMediaConnected_=false,acRateReady_=false;uint8_t acChannel_=0,acRSSI_=0;Timer timer;Timer *calibTimer_=&timer;
 enum{AUChannel=7,AURFK=8};bool acStarting_=false;struct{uint32_t stage=0;}acRecoveryDiag_;
 // Recovery is exercised with the real startup/control/HAL in its own suite.
 void record8814Failure(uint32_t,int){}
 rtw_8814au_station_params station8814Params()const;int set8814Channel(uint8_t);int state8814(ieee80211com*,ieee80211_state,int);
 // Factory policy is a boundary here; actual country denial is covered by
 // rtl8814au_channel_tests and dfs_regulatory_controller_tests.
 bool permitsDFSChannelByCountry(uint8_t)const{return true;}
 static void cancelScan(void*){events.push_back(52);}static void cancelCalibration(void*){events.push_back(53);}static void nextScan(void*){events.push_back(54);}static void nextCalibration(void*){events.push_back(55);}
 static void update8814EDCA(ieee80211com*){events.push_back(56);} };
static int generic(ieee80211com *ic,ieee80211_state s,int){events.push_back(60+s);if(!generic_error)ic->ic_state=s;return generic_error;}
/* ACTUAL_BACKEND_METHODS */
int main(){RTWUSBController c;ieee80211_channel ch{6};ieee80211_node n{};n.ni_chan=&ch;n.ni_associd=0xc001;n.ni_intval=100;n.ni_bssid[0]=2;n.ni_rates.rs_nrates=3;n.ni_rates.rs_rates[0]=0x82;n.ni_rates.rs_rates[1]=12;n.ni_rates.rs_rates[2]=108;c.core_.sc_ic.ic_opmode=IEEE80211_M_STA;c.core_.sc_ic.ic_bss=&n;c.core_.sc_ic.ic_myaddr[0]=2;c.core_.sc_newstate=generic;c.acCore_.phy_initialized=c.acCore_.powered=c.acCore_.fw_running=c.acCore_.board_valid=c.acCore_.mac_initialized=1;c.acCore_.board.tx_streams=2;
 assert(c.station8814Params().basic_rates==1);assert(rtw8814RateMask(n.ni_rates,false)==0x811);
 assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_AUTH,0));assert(c.acChannel_==6&&c.core_.sc_ic.ic_state==IEEE80211_S_AUTH);assert((events==std::vector<int>{52,10,11,22,62}));
 events.clear();assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0));assert(c.acRateReady_&&c.acMediaConnected_);assert((events==std::vector<int>{52,30,31,32,40,24,56,64,55}));
 events.clear();assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_SCAN,0));assert(!c.acRateReady_&&!c.acMediaConnected_);assert((events==std::vector<int>{52,53,33,12,21,61,54}));
 events.clear();injected=11;assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==EIO);assert(c.core_.sc_ic.ic_state==IEEE80211_S_SCAN&&!c.acCore_.phy_initialized);assert((events==std::vector<int>{52,10,11}));
 injected=0;c.core_.core_error=0;c.acCore_.phy_initialized=1;c.acChannel_=6;events.clear();n.ni_rates.rs_nrates=0;assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0)==EINVAL);assert(!c.acMediaConnected_&&!c.acRateReady_);assert((events==std::vector<int>{52}));
 n.ni_rates.rs_nrates=3;events.clear();injected=31;assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0)==EIO);assert(!c.acMediaConnected_&&!c.acRateReady_);assert((events==std::vector<int>{52,30,31,20}));
 injected=0;events.clear();generic_error=EINVAL;assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0)==EINVAL);assert(!c.acMediaConnected_&&!c.acRateReady_);assert((events==std::vector<int>{52,30,31,32,40,24,56,64,33,20}));
 generic_error=0;injected=20;events.clear();assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_INIT,0)==EIO);assert(c.core_.sc_ic.ic_state==IEEE80211_S_INIT);assert((events==std::vector<int>{52,53,20,60}));

 /* Passive scan preserves pausedTX. AUTH must use operating5G path and
  * finish calibration before association. 2G return also recalibrates. */
 injected=0;events.clear();n.ni_chan->number=36;n.ni_chan->ic_flags=IEEE80211_CHAN_PASSIVE|IEEE80211_CHAN_5GHZ;
 c.acCore_.phy_initialized=1;c.acChannel_=6;c.core_.sc_ic.ic_state=IEEE80211_S_SCAN;
 assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
 assert((events==std::vector<int>{52,13,21,61,54}));
 assert(!c.acCore_.phy_initialized && c.acChannel_==36);
 events.clear();assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==0);
 assert((events==std::vector<int>{52,10,11,22,62}));
 assert(c.acCore_.phy_initialized&&c.acChannel_==36);
 auto params5=c.station8814Params();assert(params5.band_5g&&params5.short_slot&&params5.basic_rates==0x10);
 events.clear();assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0)==0);
 assert((c.acRateConfig_.mask&15)==0);
 allow_5g=false;events.clear();assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==ENOTSUP);
 assert((events==std::vector<int>{52,53,33}));allow_5g=true;
 c.acMediaConnected_=false;c.core_.sc_ic.ic_state=IEEE80211_S_SCAN;
 events.clear();n.ni_chan->number=6;n.ni_chan->ic_flags=0;
 assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_SCAN,0)==0);
 assert(c.acCore_.phy_initialized && c.acChannel_==6);
 assert((events==std::vector<int>{52,10,11,21,61,54}));
 for(bool five:{false,true})for(bool sgi:{false,true}){
  events.clear();n.ni_chan->number=five?36:6;n.ni_chan->ic_flags=five?IEEE80211_CHAN_5GHZ:0;
  n.ni_flags=IEEE80211_NODE_HT;n.ni_htcaps=sgi?IEEE80211_HTCAP_SGI20:0;n.ni_rxmcs[0]=n.ni_rxmcs[1]=255;
  c.core_.sc_ic.ic_htcaps=IEEE80211_HTCAP_SGI20;c.acChannel_=five?36:6;c.acCore_.phy_initialized=1;
  assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0)==0);
  assert(c.acRateConfig_.ht&&c.acRateConfig_.sgi20==sgi&&c.acRateConfig_.initial_rate==27);
  assert((c.acRateConfig_.mask>>12)==65535);
  assert(c.acRateConfig_.rate_id==(five?4:2));
 }
 const uint8_t all5GHz[]=RTW_STATION_5GHZ_CHANNELS;
 for(unsigned channel:all5GHz){
  assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_INIT,0));
  n.ni_chan->number=channel;n.ni_chan->ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_PASSIVE;
  events.clear();assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_SCAN,0));
  assert(last_station.passive_scan&&!c.acCore_.phy_initialized&&c.acChannel_==channel);
  assert((events==std::vector<int>{52,13,21,61,54}));
  events.clear();assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_AUTH,0));
  assert((events==std::vector<int>{52,10,11,22,62}));
  assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_RUN,0));assert(last_station.ht&&(c.acRateConfig_.mask&15)==0);
 }
 assert(!c.state8814(&c.core_.sc_ic,IEEE80211_S_INIT,0));
 denied_channel=40;n.ni_chan->number=40;c.core_.sc_ic.ic_state=IEEE80211_S_SCAN;events.clear();
 assert(c.state8814(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==ENOTSUP);
 assert((events==std::vector<int>{52}));
 puts("RTL8814AU backend state trace passed");}
