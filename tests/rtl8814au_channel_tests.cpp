// SPDX-License-Identifier: BSD-3-Clause
// Real backend admission/SCAN/AUTH; PHY and net80211 are explicit boundaries.
#include "rtw/RTL8814AU.h"
#include "rtw/RTL8814AUPHY.h"
#include "rtw/RTL8814AUStation.h"
#include "rtw/RTL8814AUDataPath.h"
#include "rtw/DFSRegulatory.h"
#include "rtw/StationChannels.h"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static unsigned checks,tunes,passive,calibrations,configures,transitions;
static int tune_error;
static bool phy_supported=true;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"8814 backend line%d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
enum ieee80211_state {IEEE80211_S_INIT,IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN};
enum {IEEE80211_M_STA=1,IEEE80211_NODE_HT=1,IEEE80211_HTCAP_SGI20=32};
struct ieee80211_channel {unsigned number;};
#define IEEE80211_CHAN_ANYC ((ieee80211_channel*)-1)
struct rateset {};
struct ieee80211_node {ieee80211_channel*ni_chan=nullptr;rateset ni_rates;unsigned ni_flags=0,ni_htcaps=0;uint8_t ni_rxmcs[2]{};uint16_t ni_intval=100;};
struct ieee80211com {int ic_opmode=IEEE80211_M_STA;ieee80211_state ic_state=IEEE80211_S_INIT;ieee80211_node*ic_bss=nullptr;unsigned ic_htcaps=0;};
static unsigned ieee80211_chan2ieee(ieee80211com*,ieee80211_channel*c){return c->number;}
static int generic_state(ieee80211com*ic,ieee80211_state state,int){++transitions;ic->ic_state=state;return 0;}
static uint16_t rtw8814RateMask(const rateset&,bool){std::abort();}
class RTWUSBController {
public:
 rtw_8814au acCore_{};
 struct {int core_error=0;int(*sc_newstate)(ieee80211com*,ieee80211_state,int)=generic_state;}core_;
 bool acMediaConnected_=false,acRateReady_=false;uint8_t acChannel_=0,acRSSI_=0;
 rtw_8814au_rate_config acRateConfig_{};
 uint8_t country_[2]={'0','0'};
 bool permitsDFSChannelByCountry(uint8_t c)const{return !rtw_station_is_dfs_channel(c)||rtw_dfs_regulatory_channel(country_,c);}
 static void cancelScan(void*){}static void cancelCalibration(void*){}
 static void nextScan(void*){}static void nextCalibration(void*){}
 void update8814EDCA(ieee80211com*){std::abort();}
 rtw_8814au_station_params station8814Params(){rtw_8814au_station_params p{};p.band_5g=1;return p;}
 int set8814Channel(uint8_t);
 int state8814(ieee80211com*,ieee80211_state,int);
};
extern "C" int rtw_8814au_phy_channel_supported(const rtw_8814au*,uint8_t c,unsigned width){return phy_supported&&width==20&&((c>=1&&c<=11)||rtw_station_5ghz_channel(c));}
extern "C" int rtw_8814au_phy_set_channel(rtw_8814au*,uint8_t,unsigned){++tunes;return tune_error;}
extern "C" int rtw_8814au_phy_calibrate(rtw_8814au*c){++calibrations;c->phy_initialized=1;return 0;}
extern "C" int rtw_8814au_phy_set_passive_5g_channel(rtw_8814au*,uint8_t,unsigned){++passive;return tune_error;}
extern "C" int rtw_8814au_phy_set_scan_channel(rtw_8814au*,uint8_t,unsigned){++tunes;return tune_error;}
extern "C" int rtw_8814au_station_configure(rtw_8814au*,rtw_8814au_station_state state,const rtw_8814au_station_params*p){++configures;CHECK(p);CHECK(state==RTW_8814AU_STATION_SCAN||state==RTW_8814AU_STATION_AUTH);if(state==RTW_8814AU_STATION_SCAN)CHECK(p->passive_scan);return 0;}
extern "C" int rtw_8814au_h2c_mailbox(rtw_8814au*,const uint8_t[8]){std::abort();}
extern "C" int rtw_8814au_write(rtw_8814au*,uint16_t,unsigned,uint32_t){std::abort();}
extern "C" int rtw_8814au_build_rate_config(uint16_t,const uint8_t[2],unsigned,int,int,int,rtw_8814au_rate_config*){std::abort();}
extern "C" int rtw_8814au_build_ra_commands(const rtw_8814au_rate_config*,int,uint8_t[8],uint8_t[8]){std::abort();}
extern "C" int rtw_8814au_make_media_status(int,uint8_t[8]){std::abort();}
/* ACTUAL_BACKEND_METHODS */
static void reset(){tunes=passive=calibrations=configures=transitions=0;tune_error=0;phy_supported=true;}
int main(){
 const uint8_t channels[]=RTW_STATION_5GHZ_CHANNELS;
 for(uint8_t channel:channels){
  reset();RTWUSBController c;ieee80211_channel ch{channel};ieee80211_node peer;peer.ni_chan=&ch;ieee80211com ic;ic.ic_bss=&peer;
  CHECK(c.state8814(&ic,IEEE80211_S_SCAN,-1)==0);
  CHECK(passive==1&&!tunes&&!calibrations&&configures==1&&transitions==1&&c.acChannel_==channel);
  CHECK(c.state8814(&ic,IEEE80211_S_AUTH,-1)==0);
  CHECK(tunes==1&&calibrations==1&&configures==2&&transitions==2&&c.acChannel_==channel);
 }
 for(unsigned channel:{0U,12U,14U,50U,68U,96U,144U,148U,166U,300U}){
  reset();RTWUSBController c;ieee80211_channel ch{channel};ieee80211_node peer;peer.ni_chan=&ch;ieee80211com ic;ic.ic_bss=&peer;
  CHECK(c.state8814(&ic,IEEE80211_S_SCAN,-1)==EINVAL);CHECK(!tunes&&!passive&&!configures&&!transitions);
 }
 {reset();RTWUSBController c;std::memcpy(c.country_,"EG",2);ieee80211_channel ch{100};ieee80211_node peer;peer.ni_chan=&ch;ieee80211com ic;ic.ic_bss=&peer;
  CHECK(c.state8814(&ic,IEEE80211_S_SCAN,-1)==EACCES);CHECK(c.state8814(&ic,IEEE80211_S_AUTH,-1)==EACCES);CHECK(c.set8814Channel(100)==EACCES);
  CHECK(!tunes&&!passive&&!configures&&!transitions);ch.number=52;CHECK(c.state8814(&ic,IEEE80211_S_SCAN,-1)==0);}
 {reset();RTWUSBController c;phy_supported=false;CHECK(c.set8814Channel(52)==ENOTSUP);CHECK(!tunes&&!calibrations);}
 {reset();RTWUSBController c;tune_error=EIO;CHECK(c.set8814Channel(52)==EIO);CHECK(tunes==1&&!calibrations&&!c.acChannel_&&c.core_.core_error==EIO);}
 std::printf("Actual RTL8814 backend SCAN/AUTH: %u checks; all24primaries, factorydeny, invalid geometry and I/O passed\n",checks);
}
