// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "channel_constants.hpp"
#define XYLog(...) ((void)0)
#define DPRINTF(...) ((void)0)
#define min(a,b) ((a)<(b)?(a):(b))
#define MBUF_DONTWAIT 0
#define MT_DATA 0
#define LE_WRITE_2(p,v) do{(p)[0]=(uint8_t)(v);(p)[1]=(uint8_t)((v)>>8);}while(0)
#define LE_WRITE_4(p,v) do{LE_WRITE_2(p,v);LE_WRITE_2((p)+2,(v)>>16);}while(0)
#define u32_get_bits(v,mask) (((v)&(mask))/((mask)&-(mask)))
#define le16toh(v) (v)
enum {IEEE80211_MODE_11AC=5};
struct ieee80211_channel {uint32_t ic_flags=0;uint16_t ic_freq=0,ic_center_freq1=0,ic_center_freq2=0;};
struct ieee80211_rateset {uint8_t rs_nrates=1,rs_rates[16]={12};};
struct ieee80211_node {
 ieee80211_channel *ni_chan=nullptr;
 ieee80211_rateset ni_rates;
 uint32_t ni_flags=0,ni_vhtcaps=0,ni_rsnprotos=0,ni_rsnciphers=0;
 uint16_t ni_htcaps=0,ni_htop1=0;
 uint8_t ni_htop0=0,ni_vht_chan1=0,ni_vht_chan2=0,ni_vht_chanwidth=0;
 uint8_t ni_esslen=0,ni_essid[32]={},ni_bssid[6]={};
 ieee80211_chan_width ni_chw=IEEE80211_CHAN_WIDTH_20;
 bool mandatory_mcs=true;
};
struct ieee80211com {
 uint32_t ic_flags=0,ic_userflags=0,ic_modecaps=1U<<IEEE80211_MODE_11AC,ic_caps=0;
 uint16_t ic_htcaps=0,ic_max_rxrate=0,ic_htxcaps=0,ic_lintval=10;
 uint8_t ic_ampdu_params=0,ic_sup_mcs[10]={},ic_tx_mcs_set=0,ic_aselcaps=0;
 uint32_t ic_txbfcaps=0,ic_vhtcaps=0;
 uint16_t ic_vht_rx_mcs_map=0,ic_vht_rx_highest=0,ic_vht_tx_mcs_map=0,ic_vht_tx_highest=0;
 uint8_t ic_des_essid[32]={},ic_des_esslen=0;
 ieee80211_node *ic_bss=nullptr;
 ieee80211_rateset ic_sup_rates[1];
 struct {unsigned is_vht_nego_no_mandatory_mcs=0,is_vht_nego_bad_crypto=0;} ic_stats;
};
struct ieee80211_ie_htcap {uint8_t bytes[28];};
struct ieee80211_ie_vhtcap {uint8_t bytes[14];};
struct ieee80211_wme_info {uint8_t bytes[9];};
struct ieee80211_he_cap_elem {uint8_t bytes[17];};
struct ieee80211_he_mcs_nss_supp {uint8_t bytes[12];};
struct channel_test_mbuf {std::vector<uint8_t> bytes;size_t length=0;};
using mbuf_t=channel_test_mbuf*;
#define mtod(m,t) ((t)(m)->bytes.data())
inline mbuf_t ieee80211_getmgmt(int,int,unsigned length){auto*m=new channel_test_mbuf;m->bytes.resize(length);return m;}
inline void mbuf_pkthdr_setlen(mbuf_t m,size_t length){if(length>m->bytes.size())std::abort();m->length=length;}
inline void mbuf_setlen(mbuf_t m,size_t length){if(length>m->bytes.size())std::abort();m->length=length;}
inline unsigned ieee80211_chan2mode(ieee80211com*,const ieee80211_channel*){return 0;}
inline unsigned ieee80211_ieee2mhz(unsigned channel,unsigned flags){return (flags&IEEE80211_CHAN_5GHZ?5000:2407)+5*channel;}
inline bool ieee80211_node_supports_vht(const ieee80211_node*n){return n->mandatory_mcs;}
inline bool ieee80211_node_supports_vht_sgi80(const ieee80211_node*n){return n->ni_vhtcaps&IEEE80211_VHTCAP_SHORT_GI_80;}
inline bool ieee80211_node_supports_vht_sgi160(const ieee80211_node*n){return n->ni_vhtcaps&IEEE80211_VHTCAP_SHORT_GI_160;}
inline uint8_t*ieee80211_add_ssid(uint8_t*p,const uint8_t*name,unsigned length){*p++=0;*p++=(uint8_t)length;std::memcpy(p,name,length);return p+length;}
inline uint8_t*ieee80211_add_rates(uint8_t*p,const ieee80211_rateset*r){*p++=1;*p++=r->rs_nrates;std::memcpy(p,r->rs_rates,r->rs_nrates);return p+r->rs_nrates;}
inline uint8_t*ieee80211_add_xrates(uint8_t*p,const ieee80211_rateset*){return p;}
inline uint8_t*ieee80211_add_wme_info(uint8_t*p,ieee80211com*){*p++=221;*p++=0;return p;}
inline uint8_t*ieee80211_add_rsn(uint8_t*p,ieee80211com*,ieee80211_node*){return p;}
inline uint8_t*ieee80211_add_wpa(uint8_t*p,ieee80211com*,ieee80211_node*){return p;}
inline uint8_t*ieee80211_add_qos_capability(uint8_t*p,ieee80211com*){return p;}
inline uint8_t*ieee80211_add_hecaps(uint8_t*p,ieee80211com*){return p;}
mbuf_t ieee80211_get_probe_req(ieee80211com*,ieee80211_node*);
mbuf_t ieee80211_get_assoc_req(ieee80211com*,ieee80211_node*,int);
void ieee80211_ht_negotiate_chw(ieee80211com*,ieee80211_node*);
void ieee80211_vht_negotiate(ieee80211com*,ieee80211_node*);
