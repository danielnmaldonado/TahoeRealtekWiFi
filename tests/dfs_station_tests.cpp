#include "rtw/DFSStation.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static const uint8_t ap[6]={2,1,2,3,4,5},local[6]={2,6,7,8,9,10},ssid[3]={'a','p','x'};
static std::vector<uint8_t> beacon(uint8_t ch=52){std::vector<uint8_t>b(36);b[0]=0x80;std::memset(b.data()+4,255,6);std::memcpy(b.data()+10,ap,6);std::memcpy(b.data()+16,ap,6);b[32]=100;b[34]=1;b.insert(b.end(),{0,3,'a','p','x',3,1,ch});return b;}
static int observe(rtw_dfs_station&s,const std::vector<uint8_t>&b,uint64_t now=100,uint8_t ch=52){return rtw_dfs_station_observe(&s,b.data(),b.size(),ch,0,ssid,3,local,nullptr,now);}
int main(){rtw_dfs_station s{};auto b=beacon();
 // Skipping the prior-beacon requirement would authorize an arbitrary cached scan node.
 CHECK(!rtw_dfs_station_authorize(&s,ap,ssid,3,52,100));
 CHECK(observe(s,b)==RTW_DFS_OBSERVED);CHECK(rtw_dfs_station_authorize(&s,ap,ssid,3,52,100));
 CHECK(!rtw_dfs_station_authorize(&s,ap,ssid,3,56,100));CHECK(!rtw_dfs_station_authorize(&s,ap,ssid,3,52,30101));CHECK(!rtw_dfs_station_authorize(&s,ap,ssid,3,52,99));
 auto other=b;other[10]=4;CHECK(observe(s,other)==RTW_DFS_IGNORED);other=b;other[34]=2;CHECK(observe(s,other)==RTW_DFS_IGNORED);
 other=b;other.back()=56;CHECK(observe(s,other)==RTW_DFS_IGNORED);CHECK(rtw_dfs_station_observe(&s,b.data(),b.size(),52,56,ssid,3,local,nullptr,100)==RTW_DFS_IGNORED);
 other=b;other[40]='z';rtw_dfs_station isolated{};CHECK(observe(isolated,other)==RTW_DFS_IGNORED);CHECK(!rtw_dfs_station_authorize(&isolated,ap,ssid,3,52,100));
 for(size_t n=0;n<b.size();++n){rtw_dfs_station x{};CHECK(rtw_dfs_station_observe(&x,b.data(),n,52,0,ssid,3,local,nullptr,100)==RTW_DFS_IGNORED);}
 other=b;other.insert(other.end(),{32,1,3});CHECK(observe(isolated,other)==RTW_DFS_OBSERVED);CHECK(rtw_dfs_station_authorize(&isolated,ap,ssid,3,52,100));CHECK(rtw_dfs_station_power_reduction(&isolated,ap,ssid,3,52,100)==3);
 auto country=b;country.insert(country.end(),{7,6,'U','S',' ',36,8,17});rtw_dfs_station country_state{};CHECK(observe(country_state,country)==RTW_DFS_OBSERVED);CHECK(country_state.observations[0].has_power_limit==1&&country_state.observations[0].max_power_dbm==17);
 // A foreign announcement cannot invalidate this AP or choose a target RF channel.
 other=b;other.insert(other.end(),{37,3,1,100,1});uint8_t foreign[6]={2,9,9,9,9,9};CHECK(rtw_dfs_station_observe(&s,other.data(),other.size(),52,0,ssid,3,local,foreign,100)==RTW_DFS_OBSERVED);CHECK(!s.blocked);
 CHECK(rtw_dfs_station_observe(&s,other.data(),other.size(),52,0,ssid,3,local,ap,100)==RTW_DFS_SWITCH);CHECK(s.blocked&&s.pending&&s.target_channel==100);CHECK(!rtw_dfs_station_authorize(&s,ap,ssid,3,52,100));
 rtw_dfs_station_reset(&s);CHECK(!s.blocked&&!s.pending);CHECK(!rtw_dfs_station_authorize(&s,ap,ssid,3,52,100));
 // Unknown/unsupported targets request a stop, never directly retune.
 other=b;other.insert(other.end(),{37,3,0,255,0});CHECK(rtw_dfs_station_observe(&s,other.data(),other.size(),52,0,ssid,3,local,ap,100)==RTW_DFS_SWITCH);CHECK(s.target_channel==255&&s.pending);
 rtw_dfs_station_reset(&s);other=b;other.insert(other.end(),{37,2,1,100});CHECK(rtw_dfs_station_observe(&s,other.data(),other.size(),52,0,ssid,3,local,ap,100)==RTW_DFS_IGNORED);CHECK(!s.blocked);
 std::vector<uint8_t>a(26);a[0]=0xd0;std::memcpy(a.data()+4,local,6);std::memcpy(a.data()+10,ap,6);std::memcpy(a.data()+16,ap,6);a[24]=0;a[25]=4;a.insert(a.end(),{37,3,1,100,3});CHECK(rtw_dfs_station_observe(&s,a.data(),a.size(),52,0,ssid,3,local,ap,100)==RTW_DFS_SWITCH);
 rtw_dfs_station_reset(&s);a[4]^=2;CHECK(rtw_dfs_station_observe(&s,a.data(),a.size(),52,0,ssid,3,local,ap,100)==RTW_DFS_IGNORED);
 // A hidden/changed SSID cannot admit AUTH, but a valid associated AP CSA
 // must still quiet TX when that AP subsequently hides or changes its SSID.
 for(unsigned hidden=0;hidden<2;++hidden){
  auto changed=beacon();
  if(hidden){changed.erase(changed.begin()+38,changed.begin()+41);changed[37]=0;}
  else changed[38]='z';
  rtw_dfs_station candidate{};CHECK(observe(candidate,changed)==RTW_DFS_IGNORED);
  CHECK(!rtw_dfs_station_authorize(&candidate,ap,ssid,3,52,100));
  changed.insert(changed.end(),{37,3,1,100,1});
  CHECK(rtw_dfs_station_observe(&candidate,changed.data(),changed.size(),52,52,ssid,3,local,foreign,100)==RTW_DFS_IGNORED);
  CHECK(!candidate.blocked);
  CHECK(rtw_dfs_station_observe(&candidate,changed.data(),changed.size(),52,52,ssid,3,local,ap,100)==RTW_DFS_SWITCH);
  CHECK(candidate.blocked&&candidate.pending&&!rtw_dfs_station_authorize(&candidate,ap,ssid,3,52,100));
 }
 // A bound hidden beacon supplies power without creating an AUTH lease.
 for(unsigned changed=0;changed<2;++changed){
  auto hidden=beacon();if(changed)hidden[38]='z';else{hidden.erase(hidden.begin()+38,hidden.begin()+41);hidden[37]=0;}hidden.insert(hidden.end(),{32,1,4});
  rtw_dfs_station control{};
  CHECK(rtw_dfs_station_observe(&control,hidden.data(),hidden.size(),52,52,ssid,3,local,foreign,200)==RTW_DFS_IGNORED);
  CHECK(rtw_dfs_station_observe(&control,hidden.data(),hidden.size(),52,52,ssid,3,local,ap,200)==RTW_DFS_OBSERVED);
  CHECK(rtw_dfs_station_power_reduction(&control,ap,ssid,3,52,200)==4);CHECK(!rtw_dfs_station_authorize(&control,ap,ssid,3,52,200));
  rtw_dfs_station_reset(&control);CHECK(observe(control,b,100)==RTW_DFS_OBSERVED);
  CHECK(rtw_dfs_station_observe(&control,hidden.data(),hidden.size(),52,52,ssid,3,local,ap,200)==RTW_DFS_OBSERVED);CHECK(rtw_dfs_station_authorize(&control,ap,ssid,3,52,200));CHECK(!rtw_dfs_station_authorize(&control,ap,ssid,3,52,30101));
  CHECK(rtw_dfs_station_observe(&control,hidden.data(),hidden.size(),52,52,ssid,3,local,ap,30101)==RTW_DFS_OBSERVED);
  CHECK(rtw_dfs_station_power_reduction(&control,ap,ssid,3,52,30101)==4);CHECK(!rtw_dfs_station_authorize(&control,ap,ssid,3,52,30101));
  hidden.insert(hidden.end(),{7,6,'U','S',' ',36,8,16});CHECK(rtw_dfs_station_observe(&control,hidden.data(),hidden.size(),52,52,ssid,3,local,ap,30102)==RTW_DFS_OBSERVED);CHECK(rtw_dfs_station_power_limit(&control,ap,ssid,3,52,30102)==16);CHECK(!rtw_dfs_station_authorize(&control,ap,ssid,3,52,30102));
 }
 {rtw_dfs_station missing{};CHECK(observe(missing,b)==RTW_DFS_OBSERVED);auto hidden=beacon();hidden.erase(hidden.begin()+38,hidden.begin()+41);hidden[37]=0;hidden.insert(hidden.end(),{7,6,'U','S',' ',1,11,20});CHECK(rtw_dfs_station_observe(&missing,hidden.data(),hidden.size(),52,52,ssid,3,local,ap,200)==RTW_DFS_OBSERVED);CHECK(rtw_dfs_station_power_limit(&missing,ap,ssid,3,52,200)==RTW_DFS_AP_POWER_INVALID);CHECK(!rtw_dfs_station_authorize(&missing,ap,ssid,3,52,200));}
 std::puts("DFS parser/state: beacon binding, stale/malformed/channel spoof, power constraint and CSA passed");}
