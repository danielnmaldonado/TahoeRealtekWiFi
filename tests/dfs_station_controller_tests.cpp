#include "platform.hpp"
#include <cstdio>
#include <cstdlib>
uint64_t RTWUSBController::clockMS=100;
unsigned DFSPowerBoundary::calls=0;uint8_t DFSPowerBoundary::db=0;int DFSPowerBoundary::error=0;int32_t DFSPowerBoundary::limit=INT32_MAX;std::vector<int>DFSPowerBoundary::events;
extern "C" int rtw_8814au_phy_set_power_limit_dbm(rtw_8814au*,int8_t db){return DFSPowerBoundary::cap(db==127?INT32_MAX:(int32_t)db*100);}
extern "C" int rtw_r12au_phy_set_power_limit_mbm(rtw_r12au_phy*,int32_t mbm){return DFSPowerBoundary::cap(mbm);}
extern "C" int rtw88_phy_radio_set_power_limit_mbm(rtw88_phy_context*,int32_t mbm){return DFSPowerBoundary::cap(mbm);}
extern "C" int rtw_8814au_phy_set_power_constraint(rtw_8814au*,uint8_t db){return DFSPowerBoundary::apply(db);}
extern "C" int rtw_r12au_phy_set_power_reduction(rtw_r12au_phy*,uint8_t db){return DFSPowerBoundary::apply(db);}
extern "C" int rtw88_phy_radio_set_power_reduction(rtw88_phy_context*,uint8_t db){return DFSPowerBoundary::apply(db);}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static RTWUSBController*controller(ieee80211com*ic){return RTWUSBController::owner(static_cast<rtwn_softc*>(ic->ic_softc)->sc_ops.cookie);}
RTWUSBController::RTWUSBController(){core_.sc_ic.ic_softc=&core_;core_.sc_ops.cookie=this;core_.sc_ic.ic_if.if_softc=&core_;core_.sc_ic.ic_if.if_snd.queue=&pendingQueue_;core_.sc_ic.ic_if.if_watchdog=[](_ifnet*){};}
int rtwn_io_error(rtwn_softc*c){return c->core_error?EIO:0;}
static int transition_boundary(ieee80211com*ic,ieee80211_state state){auto*c=controller(ic);c->transitions.push_back(state);DFSPowerBoundary::events.push_back(2);ic->ic_state=state;return c->transitionError;}
int rtwn_newstate(ieee80211com*ic,ieee80211_state state,int){return transition_boundary(ic,state);}
#define STATE(name) int RTWUSBController::name(ieee80211com*ic,ieee80211_state state,int){return transition_boundary(ic,state);}
STATE(state8814) STATE(state8812) STATE(state88) STATE(state8192DU) STATE(state8192FU) STATE(state8710BU) STATE(state8723)
#define STOP(name) void RTWUSBController::name(){running_=false;}
void RTWUSBController::stop8814(){if(dfsTXHold_){uint8_t old=0x3f;CHECK(vendorRequest(false,0x522,&old,1)==0);}running_=false;}
STOP(stop8812) STOP(stop88) STOP(stop8192DU) STOP(stop8192FU) STOP(stop8710BU) STOP(stop8723)
void rtwn_start(_ifnet*){}
void rtwn_stop(_ifnet*p){controller(&static_cast<rtwn_softc*>(p->if_softc)->sc_ic)->running_=false;}
void ieee80211_begin_scan(_ifnet*p){auto*c=controller(&static_cast<rtwn_softc*>(p->if_softc)->sc_ic);++c->scanCalls;RTWUSBController::stateTransition(&c->core_.sc_ic,IEEE80211_S_SCAN,-1);}
IOReturn RTWUSBController::enableStation(){running_=enabled_=radioReady_=true;return 0;}
static std::vector<uint8_t> beacon(const uint8_t*ap){
 std::vector<uint8_t>b(36);b[0]=0x80;std::memset(b.data()+4,255,6);std::memcpy(b.data()+10,ap,6);std::memcpy(b.data()+16,ap,6);b[32]=100;b[34]=1;b.insert(b.end(),{0,1,'x',3,1,52});return b;
}
static void setup(RTWUSBController&c,ieee80211_node&n){n.ni_chan=reinterpret_cast<void*>(52);n.ni_essid[0]='x';n.ni_bssid[0]=2;n.ni_bssid[5]=1;c.core_.sc_ic.ic_bss=&n;c.family_=RTW_WIFI_RTL8814AU;c.core_.sc_ic.ic_state=IEEE80211_S_SCAN;c.dfsTunedChannel_=52;c.core_.sc_ic.ic_channels[52].ic_flags=IEEE80211_CHAN_5GHZ;}
static std::vector<uint8_t> hidden_beacon(const uint8_t*ap){auto b=beacon(ap);b.erase(b.begin()+38);b[37]=0;return b;}
static void bind_visible(RTWUSBController&c,ieee80211_node&n,ieee80211_state phase){
 RTWUSBController::clockMS=100;setup(c,n);auto b=beacon(n.ni_bssid);b.insert(b.end(),{32,1,3});c.observeDFSFrame(b.data(),b.size(),52);
 CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==0);
 if(phase==IEEE80211_S_ASSOC||phase==IEEE80211_S_RUN)CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_ASSOC,0)==0);
 if(phase==IEEE80211_S_RUN)CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_RUN,0)==0);
}
int main(){
 RTWUSBController missing;ieee80211_node unobserved;setup(missing,unobserved);
 // Removing the DFS state gate allows authentication to a never-observed AP.
 CHECK(RTWUSBController::stateTransition(&missing.core_.sc_ic,IEEE80211_S_AUTH,0)==EACCES);CHECK(missing.transitions.empty());
 CHECK(missing.dfsStation_.pending&&missing.dfsTXBlocked());
 RTWUSBController c;ieee80211_node n;setup(c,n);auto b=beacon(n.ni_bssid);
 c.observeDFSFrame(b.data(),b.size(),56);CHECK(!c.dfsStateAllowed(IEEE80211_S_AUTH,52));c.observeDFSFrame(b.data(),b.size(),256);CHECK(!c.dfsStateAllowed(IEEE80211_S_AUTH,52));
 c.observeDFSFrame(b.data(),b.size(),52);CHECK(c.dfsStateAllowed(IEEE80211_S_AUTH,52));c.core_.sc_ic.ic_channels[52].ic_flags=0;CHECK(!c.dfsStateAllowed(IEEE80211_S_AUTH,52));c.core_.sc_ic.ic_channels[52].ic_flags=IEEE80211_CHAN_5GHZ;
 CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_AUTH,0)==0);CHECK(c.dfsBound_);CHECK(DFSPowerBoundary::calls==1&&DFSPowerBoundary::db==0);CHECK(DFSPowerBoundary::limit==2000);CHECK(DFSPowerBoundary::events.size()==3&&DFSPowerBoundary::events[0]==0&&DFSPowerBoundary::events[1]==1&&DFSPowerBoundary::events[2]==2);
 Packet auth;auth.bytes.resize(24);auth.bytes[0]=0xb0;std::memcpy(auth.bytes.data()+4,n.ni_bssid,6);std::memcpy(auth.bytes.data()+10,c.core_.sc_ic.ic_myaddr,6);std::memcpy(auth.bytes.data()+16,n.ni_bssid,6);CHECK(c.dfsFrameAllowed(&auth,&n));
 Packet probe=auth;probe.bytes[0]=0x40;CHECK(!c.dfsFrameAllowed(&probe,&n));
 CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_ASSOC,0)==0);CHECK(RTWUSBController::stateTransition(&c.core_.sc_ic,IEEE80211_S_RUN,0)==0);n.ni_port_valid=true;c.core_.sc_ic.ic_psk[2]=0xa5;
 Packet data=auth;data.bytes[0]=8;CHECK(c.dfsFrameAllowed(&data,&n));
 auto foreign=b;foreign[10]=4;foreign[16]=4;foreign.insert(foreign.end(),{37,3,1,100,1});c.observeDFSFrame(foreign.data(),foreign.size(),52);CHECK(!c.dfsTXBlocked());
 Packet queued1,queued2;c.outputQueue_.packets.push_back(&queued1);c.pendingQueue_.packets.push_back(&queued2);
 auto csa=b;csa.insert(csa.end(),{37,3,1,255,1});c.observeDFSFrame(csa.data(),csa.size(),52);
 CHECK(c.dfsTXBlocked()&&c.dfsStation_.pending&&!n.ni_port_valid);
 // The common wire boundary holds every queue through INIT/stop writes.
 uint8_t unpause=0;CHECK(c.vendorRequest(false,0x522,&unpause,1)==0);CHECK(c.usbValue_.wires.back().bytes[0]==255);
 uint8_t wide[4]={1,2,0,4};CHECK(c.vendorRequest(false,0x520,wide,4)==0);CHECK(c.usbValue_.wires.back().bytes[2]==255);
 RTWUSBController du;du.family_=RTW_WIFI_RTL8192DU;du.dfsTXHold_=true;uint8_t mac1=0;CHECK(du.vendorRequest(false,0x4522,&mac1,1)==0);CHECK(du.usbValue_.wires.back().bytes[0]==255);
 uint8_t mac0=0;CHECK(du.vendorRequest(false,0x522,&mac0,1)==0);CHECK(du.usbValue_.wires.back().bytes[0]==255);
CHECK(c.pauseCalls==1);CHECK(c.running_);CHECK(c.outputQueue_.getSize()==0&&c.pendingQueue_.getSize()==0);CHECK(!c.dfsFrameAllowed(&data,&n));CHECK(!c.dfsFrameAllowed(&auth,&n));CHECK(c.core_.sc_ic.ic_state==IEEE80211_S_RUN);
 // Deferred recovery preserves credentials but drops every prior BSS lease.
 c.advanceDFSRecovery();CHECK(c.usbValue_.wires.back().bytes[0]==255);CHECK(!c.dfsTXBlocked()&&!c.dfsStation_.pending);CHECK(!c.dfsBound_&&c.dfsTunedChannel_==0);CHECK(c.core_.sc_ic.ic_psk[2]==0xa5&&c.joinRequested_);CHECK(!c.dfsStateAllowed(IEEE80211_S_AUTH,52));
 RTWUSBController cancelled;ieee80211_node cn;bind_visible(cancelled,cn,IEEE80211_S_RUN);auto cb=beacon(cn.ni_bssid);cb.insert(cb.end(),{37,3,0,100,1});cancelled.observeDFSFrame(cb.data(),cb.size(),52);CHECK(cancelled.dfsStation_.pending);cancelled.quiesce(true);cancelled.advanceDFSRecovery();CHECK(!cancelled.running_&&!cancelled.joinRequested_&&!cancelled.dfsStation_.pending);
 RTWUSBController broken;ieee80211_node bn;bind_visible(broken,bn,IEEE80211_S_RUN);broken.pauseError=EIO;broken.observeDFSFrame(cb.data(),cb.size(),52);CHECK(broken.core_.core_error);broken.advanceDFSRecovery();CHECK(!broken.running_);
 // Actual common dispatch forwards AP backoff before any family's AUTH/RFK.
 for(auto family:{RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU,RTW_WIFI_RTL8192DU}){
  RTWUSBController power;ieee80211_node pn;setup(power,pn);power.family_=family;auto pb=beacon(pn.ni_bssid);pb.insert(pb.end(),{32,1,3});power.observeDFSFrame(pb.data(),pb.size(),52);DFSPowerBoundary::events.clear();CHECK(RTWUSBController::stateTransition(&power.core_.sc_ic,IEEE80211_S_AUTH,0)==0);CHECK(DFSPowerBoundary::db==0&&power.dfsAppliedReduction_==3&&DFSPowerBoundary::limit==1700);CHECK(DFSPowerBoundary::events.size()==3&&DFSPowerBoundary::events[0]==0&&DFSPowerBoundary::events[1]==1&&DFSPowerBoundary::events[2]==2);
  power.core_.sc_ic.ic_state=IEEE80211_S_RUN;pb.back()=4;power.observeDFSFrame(pb.data(),pb.size(),52);CHECK(power.dfsTXBlocked()&&power.dfsStation_.pending);
 }
 for(auto family:{RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU,RTW_WIFI_RTL8192DU}){
  RTWUSBController once;ieee80211_node on;setup(once,on);once.family_=family;once.factoryMBM_=1734;auto ob=beacon(on.ni_bssid);ob.insert(ob.end(),{32,1,3});once.observeDFSFrame(ob.data(),ob.size(),52);CHECK(RTWUSBController::stateTransition(&once.core_.sc_ic,IEEE80211_S_AUTH,0)==0);CHECK(DFSPowerBoundary::db==0);CHECK(DFSPowerBoundary::limit==(family==RTW_WIFI_RTL8814AU?1400:1434));
 }
 RTWUSBController floor;ieee80211_node fn;setup(floor,fn);auto fb=beacon(fn.ni_bssid);fb.insert(fb.end(),{32,1,31});floor.observeDFSFrame(fb.data(),fb.size(),52);DFSPowerBoundary::error=ERANGE;CHECK(RTWUSBController::stateTransition(&floor.core_.sc_ic,IEEE80211_S_AUTH,0)==ERANGE);CHECK(floor.transitions.empty()&&floor.dfsTXBlocked());DFSPowerBoundary::error=0;
 // Hidden bound-AP control metadata is independent of AUTH discovery.
 for(auto phase:{IEEE80211_S_RUN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC})for(unsigned change:{0U,1U,2U}){
  RTWUSBController bound;ieee80211_node peer;bind_visible(bound,peer,phase);CHECK(bound.dfsAppliedLimitMBM_==1700);
  auto hidden=hidden_beacon(peer.ni_bssid);hidden.insert(hidden.end(),{32,1,(uint8_t)(change==0?4:3)});
  if(change==1)hidden.insert(hidden.end(),{7,6,'U','S',' ',36,8,16});
  if(change==2)hidden.insert(hidden.end(),{7,6,'U','S',' ',1,11,20});
  auto foreign=hidden;foreign[10]=4;foreign[16]=4;bound.observeDFSFrame(foreign.data(),foreign.size(),52);CHECK(!bound.dfsTXBlocked()&&!bound.dfsStation_.pending);
  bound.observeDFSFrame(hidden.data(),hidden.size(),52);CHECK(bound.dfsTXBlocked()&&bound.dfsStation_.pending&&!bound.dfsBound_);CHECK(bound.pauseCalls==1);
 }
 // Bound AUTH/ASSOC must quiet on CSA before keys or controlled port exist.
 for(auto phase:{IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN}){
  RTWUSBController bound;ieee80211_node peer;bind_visible(bound,peer,phase);auto hidden=hidden_beacon(peer.ni_bssid);hidden.insert(hidden.end(),{37,3,1,100,1});bound.observeDFSFrame(hidden.data(),hidden.size(),52);CHECK(bound.dfsTXBlocked()&&bound.dfsStation_.pending);
 }
 // Hidden beacons refresh power only; a visible lease still expires at30s.
 {RTWUSBController bound;ieee80211_node peer;bind_visible(bound,peer,IEEE80211_S_RUN);RTWUSBController::clockMS=30101;auto hidden=hidden_beacon(peer.ni_bssid);hidden.insert(hidden.end(),{32,1,3});bound.observeDFSFrame(hidden.data(),hidden.size(),52);CHECK(!bound.dfsTXBlocked());CHECK(rtw_dfs_station_power_reduction(&bound.dfsStation_,peer.ni_bssid,bound.core_.sc_ic.ic_des_essid,1,52,30101)==3);CHECK(!bound.dfsStateAllowed(IEEE80211_S_AUTH,52));}
 // State alone and mutable peer fields cannot impersonate a bound AP.
 for(auto phase:{IEEE80211_S_SCAN,IEEE80211_S_AUTH,IEEE80211_S_ASSOC,IEEE80211_S_RUN}){
  RTWUSBController unbound;ieee80211_node peer;setup(unbound,peer);unbound.core_.sc_ic.ic_state=phase;auto hidden=hidden_beacon(peer.ni_bssid);hidden.insert(hidden.end(),{32,1,4,37,3,1,100,1});unbound.observeDFSFrame(hidden.data(),hidden.size(),52);CHECK(!unbound.dfsTXBlocked()&&!unbound.dfsStation_.pending);CHECK(!unbound.dfsStateAllowed(IEEE80211_S_AUTH,52));
 }
 {RTWUSBController bound;ieee80211_node peer;bind_visible(bound,peer,IEEE80211_S_AUTH);peer.ni_bssid[0]=4;auto hidden=hidden_beacon(peer.ni_bssid);hidden.insert(hidden.end(),{32,1,4,37,3,1,100,1});bound.observeDFSFrame(hidden.data(),hidden.size(),52);CHECK(!bound.dfsTXBlocked()&&!bound.dfsStation_.pending);}
 {RTWUSBController bound;ieee80211_node peer;bind_visible(bound,peer,IEEE80211_S_ASSOC);bound.dfsTunedChannel_=56;auto hidden=hidden_beacon(peer.ni_bssid);hidden.back()=56;hidden.insert(hidden.end(),{32,1,4,37,3,1,100,1});bound.observeDFSFrame(hidden.data(),hidden.size(),56);CHECK(!bound.dfsTXBlocked()&&!bound.dfsStation_.pending);}
 // Existing non-DFS RUN also honors a CSA towards DFS without a DFS lease.
 {RTWUSBController ordinary;ieee80211_node peer;setup(ordinary,peer);ordinary.core_.sc_ic.ic_state=IEEE80211_S_RUN;peer.ni_chan=reinterpret_cast<void*>(36);ordinary.dfsTunedChannel_=36;auto hidden=hidden_beacon(peer.ni_bssid);hidden.back()=36;hidden.insert(hidden.end(),{37,3,1,100,1});ordinary.observeDFSFrame(hidden.data(),hidden.size(),36);CHECK(ordinary.dfsTXBlocked()&&ordinary.dfsStation_.pending);}
 RTWUSBController::clockMS=100;
 std::puts("DFS real Controller: admission, passive TX, CSA pause/defer, credential-preserving recovery and cancellation passed");
}
