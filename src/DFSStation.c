/* SPDX-License-Identifier: BSD-3-Clause */
/* Managed-station policy follows Linux cfg80211/mac80211 semantics at
 *0e135910deec43ffe97eca0b6af1f8f847285b59; this bounded parser is original code.
 * It provides no AP CAC/radar capability and never authorizes scan probes. */
#include "rtw/DFSStation.h"
#include <string.h>
int rtw_dfs_channel(uint8_t ch){return (ch>=52&&ch<=64&&ch%4==0)||(ch>=100&&ch<=140&&ch%4==0);}
void rtw_dfs_txpause_guard(uint8_t held,uint8_t du,uint16_t address,uint8_t*bytes,size_t n){
 if(!held||!bytes)return;
 if(address<=0x522U&&(size_t)(0x522U-address)<n)bytes[0x522U-address]=255;
 /* RTL8192DUUSB maps MAC1's logical MAC registers with physical|0x4000. */
 if(du&&address<=0x4522U&&(size_t)(0x4522U-address)<n)bytes[0x4522U-address]=255;
}
void rtw_dfs_station_reset(rtw_dfs_station*s){if(s)memset(s,0,sizeof(*s));}
static int broadcast(const uint8_t*a){unsigned i;for(i=0;i<6;i++)if(a[i]!=255)return 0;return 1;}
typedef struct {uint8_t ssid,ssid_match,ds,ht,power,country,csa,mode,target,count,reduction,has_limit;int8_t max_power;} elements;
static int parse_elements(const uint8_t*f,size_t n,size_t off,uint8_t channel,const uint8_t*ssid,size_t ssid_n,int beacon,elements*out){
 elements e;memset(&e,0,sizeof(e));
 while(off<n){size_t size;unsigned id;if(n-off<2)return 0;id=f[off];size=f[off+1];off+=2;if(size>n-off)return 0;
  switch(id){
  case 0:if(e.ssid||size>32)return 0;e.ssid=1;e.ssid_match=(uint8_t)(size==ssid_n&&!memcmp(f+off,ssid,size));break;
  case 3:if(e.ds||size!=1||f[off]!=channel)return 0;e.ds=1;break;
  case 61:if(e.ht||size!=22||f[off]!=channel)return 0;e.ht=1;break;
  case 7:{size_t k;
   if(e.country||size<6||(size&1)||f[off]<'A'||f[off]>'Z'||f[off+1]<'A'||f[off+1]>'Z')return 0;e.country=1;
   for(k=3;k+3<=size;k+=3){unsigned first=f[off+k],count=f[off+k+1],step=channel>14?4U:1U;int power=f[off+k+2]<128?f[off+k+2]:(int)f[off+k+2]-256;
    if(first>=201)continue;
    if(!first||!count)return 0;
    if(channel>=first&&(unsigned)(channel-first)%step==0&&(unsigned)(channel-first)/step<count){if(!e.has_limit||power<e.max_power)e.max_power=(int8_t)power;e.has_limit=1;}
   }
   if(k<size&&(size-k!=1||f[off+k]!=0))return 0;
   break;
  }
  case 32:if(e.power||size!=1)return 0;e.power=1;e.reduction=f[off];break;
  case 37:case 60:{uint8_t mode,target,count;size_t k=id==60?1:0;if(size!=(id==60?4U:3U))return 0;mode=f[off];target=f[off+1+k];count=f[off+2+k];if(mode>1||!target)return 0;if(e.csa&&(mode!=e.mode||target!=e.target||count!=e.count))return 0;e.csa=1;e.mode=mode;e.target=target;e.count=count;break;}
  default:break;
  }off+=size;
 }
 if(beacon&&!e.ssid)return 0;*out=e;return 1;
}
int rtw_dfs_station_observe(rtw_dfs_station*s,const uint8_t*f,size_t n,uint8_t channel,uint8_t physical,const uint8_t*ssid,size_t ssid_n,const uint8_t local[6],const uint8_t associated[6],uint64_t now){
 elements e;memset(&e,0,sizeof(e));int beacon,action,bound;unsigned i,slot,found=0;
 if(!s||!f||n<24||!channel||(physical&&physical!=channel)||!ssid||!ssid_n||ssid_n>32||!local)return RTW_DFS_IGNORED;
 beacon=f[0]==0x80;action=f[0]==0xd0;
 if((!beacon&&!action)||(f[1]&0x43)||(f[10]&1)||memcmp(f+10,f+16,6))return RTW_DFS_IGNORED;
 if(beacon){if(n<36||!broadcast(f+4)||!(f[34]&1)||(f[34]&2)||!(f[32]|f[33])||!parse_elements(f,n,36,channel,ssid,ssid_n,1,&e))return RTW_DFS_IGNORED;}
 else{
  if(!associated||memcmp(f+10,associated,6)||(!broadcast(f+4)&&memcmp(f+4,local,6))||n<26)return RTW_DFS_IGNORED;
  if(f[24]==0&&f[25]==4){if(!parse_elements(f,n,26,channel,ssid,ssid_n,0,&e)||!e.csa)return RTW_DFS_IGNORED;}
  else if(f[24]==4&&f[25]==4){if(n!=30||f[26]>1||!f[28])return RTW_DFS_IGNORED;e.csa=1;e.mode=f[26];e.target=f[28];e.count=f[29];}
  else return RTW_DFS_IGNORED;
 }
 bound=associated&&!memcmp(f+10,associated,6);
 if(e.csa&&bound){
  /* The target is telemetry only. Recovery may scan registered channels; no
   * untrusted advertised target is ever passed to an RF programming API. */
  memset(s->observations,0,sizeof(s->observations));s->blocked=1;s->pending=1;s->target_channel=e.target;s->mode=e.mode;s->count=e.count;return RTW_DFS_SWITCH;
 }
 if(!beacon||(!e.ssid_match&&!bound)||s->blocked||(rtw_dfs_channel(channel)&&!physical&&!e.ds&&!e.ht))return RTW_DFS_IGNORED;
 slot=s->next;
 for(i=0;i<RTW_DFS_OBSERVATIONS;i++)if(s->observations[i].valid&&!memcmp(s->observations[i].bssid,f+10,6)&&s->observations[i].channel==channel){slot=i;found=1;break;}
 if(!found)s->next=(uint8_t)((s->next+1U)%RTW_DFS_OBSERVATIONS);
 {rtw_dfs_observation*o=s->observations+slot;
  /* Power evidence and visible discovery have separate clocks. A trusted
   * hidden beacon may tighten an existing link without renewing AUTH. */
  const uint8_t auth_valid=(uint8_t)(found&&o->auth_valid&&o->ssid_length==ssid_n&&!memcmp(o->ssid,ssid,ssid_n));
  const uint64_t auth_seen=auth_valid?o->auth_seen_ms:0;
  memset(o,0,sizeof(*o));o->seen_ms=now;o->auth_valid=e.ssid_match?1:auth_valid;o->auth_seen_ms=e.ssid_match?now:auth_seen;memcpy(o->bssid,f+10,6);memcpy(o->ssid,ssid,ssid_n);o->ssid_length=(uint8_t)ssid_n;o->channel=channel;o->power_reduction=e.reduction;o->has_power_limit=e.has_limit?1:(e.country?2:0);o->max_power_dbm=e.max_power;o->valid=1;}
 return RTW_DFS_OBSERVED;
}
static const rtw_dfs_observation*candidate(const rtw_dfs_station*s,const uint8_t bssid[6],const uint8_t*ssid,size_t ssid_n,uint8_t channel,uint64_t now){unsigned i;if(!s||s->blocked||!bssid||!ssid||!ssid_n||ssid_n>32)return NULL;
 for(i=0;i<RTW_DFS_OBSERVATIONS;i++){const rtw_dfs_observation*o=s->observations+i;if(o->valid&&o->channel==channel&&o->ssid_length==ssid_n&&!memcmp(o->ssid,ssid,ssid_n)&&!memcmp(o->bssid,bssid,6)&&now>=o->seen_ms&&now-o->seen_ms<=RTW_DFS_BEACON_FRESH_MS)return o;}return NULL;}
int rtw_dfs_station_power_reduction(const rtw_dfs_station*s,const uint8_t bssid[6],const uint8_t*ssid,size_t ssid_n,uint8_t channel,uint64_t now){const rtw_dfs_observation*o=candidate(s,bssid,ssid,ssid_n,channel,now);return o&&o->has_power_limit!=2?o->power_reduction:-1;}
int rtw_dfs_station_power_limit(const rtw_dfs_station*s,const uint8_t bssid[6],const uint8_t*ssid,size_t ssid_n,uint8_t channel,uint64_t now){const rtw_dfs_observation*o=candidate(s,bssid,ssid,ssid_n,channel,now);return !o||o->has_power_limit==2?RTW_DFS_AP_POWER_INVALID:o->has_power_limit?o->max_power_dbm:RTW_DFS_AP_POWER_UNSPECIFIED;}
int rtw_dfs_station_authorize(const rtw_dfs_station*s,const uint8_t bssid[6],const uint8_t*ssid,size_t ssid_n,uint8_t channel,uint64_t now){const rtw_dfs_observation*o=candidate(s,bssid,ssid,ssid_n,channel,now);return o&&o->has_power_limit!=2&&o->auth_valid&&now>=o->auth_seen_ms&&now-o->auth_seen_ms<=RTW_DFS_BEACON_FRESH_MS;}
