// SPDX-License-Identifier: BSD-3-Clause
#include "net80211_channel_capabilities_host.hpp"
static unsigned checks=0;
static void require(bool value,const char*message){++checks;if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static const uint8_t*element(mbuf_t m,unsigned start,unsigned id){
 for(size_t i=start;i+2<=m->length;){size_t n=m->bytes[i+1];require(i+2+n<=m->length,"generated IE is bounded");if(m->bytes[i]==id)return m->bytes.data()+i;i+=2+n;}
 return nullptr;
}
static void wire_capabilities(){
 struct Case {uint32_t flags;bool ht,wide,vht;};
 const Case cases[]={
  {IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20,true,false,false},
  {IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20|IEEE80211_CHAN_HT40U,true,true,false},
  {IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_DFS|IEEE80211_CHAN_HT20,true,false,false},
  {IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_HT20|IEEE80211_CHAN_VHT20,true,false,true},
  {IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_HT20|IEEE80211_CHAN_HT40U|IEEE80211_CHAN_VHT20|IEEE80211_CHAN_VHT40U|IEEE80211_CHAN_VHT80,true,true,true},
  {IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM,false,false,false},
  {IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_VHT80,false,false,false},
 };
 for(const auto&test:cases)for(bool probe:{false,true}){
  ieee80211_channel target{test.flags,5180,0,0},other{IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_HT20|IEEE80211_CHAN_HT40D|IEEE80211_CHAN_VHT80,5200,0,0};
  ieee80211_node node,bss;node.ni_chan=&target;bss.ni_chan=&other;node.ni_htcaps=0xffff;
  ieee80211com ic;ic.ic_bss=&bss;ic.ic_flags=IEEE80211_F_HTON|IEEE80211_F_VHTON;
  ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;
  const auto flags=ic.ic_flags;const auto caps=ic.ic_htcaps;
  mbuf_t m=probe?ieee80211_get_probe_req(&ic,&node):ieee80211_get_assoc_req(&ic,&node,0);
  require(m!=nullptr,"frame allocation succeeds");
  const auto*ht=element(m,probe?0:4,IEEE80211_ELEMID_HTCAPS);
  const auto*vht=element(m,probe?0:4,IEEE80211_ELEMID_VHT_CAP);
  require((ht!=nullptr)==test.ht,"target registered HT permission controls emitted IE");
  require((vht!=nullptr)==test.vht,"target registered 5GHz VHT plus HT controls emitted VHT IE");
  if(ht){uint16_t emitted=ht[2]|uint16_t(ht[3])<<8;require(emitted==(test.wide?caps:uint16_t(caps&~(IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI40))),"HT IE uses only own capabilities and permitted target width");}
  require(ic.ic_flags==flags&&ic.ic_htcaps==caps&&target.ic_flags==test.flags,"wire gating preserves global capabilities and channel catalog");delete m;
 }
 for(auto*channel:{(ieee80211_channel*)nullptr,IEEE80211_CHAN_ANYC}){
  ieee80211com ic;ieee80211_node node;node.ni_chan=channel;ic.ic_bss=&node;ic.ic_flags=IEEE80211_F_HTON|IEEE80211_F_VHTON;ic.ic_htcaps=0xffff;
  for(bool probe:{false,true}){auto*m=probe?ieee80211_get_probe_req(&ic,&node):ieee80211_get_assoc_req(&ic,&node,0);require(m&&!element(m,probe?0:4,45)&&!element(m,probe?0:4,191),"unknown target emits no unsupported HT/VHT capabilities");delete m;}
 }
}
static void negotiated_channel_policy(){
 const uint32_t five=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_HT20;
 struct Case{uint32_t flags;uint8_t sco,apwidth;ieee80211_chan_width width;bool vht;};
 const Case cases[]={
  {IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_HT20,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_80MHZ,IEEE80211_CHAN_WIDTH_20,false},
  {five|IEEE80211_CHAN_DFS,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_80MHZ,IEEE80211_CHAN_WIDTH_20,false},
  {five|IEEE80211_CHAN_VHT20,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_80MHZ,IEEE80211_CHAN_WIDTH_20,true},
  {five|IEEE80211_CHAN_HT40U|IEEE80211_CHAN_VHT20|IEEE80211_CHAN_VHT40U,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_80MHZ,IEEE80211_CHAN_WIDTH_40,true},
  {five|IEEE80211_CHAN_HT40D|IEEE80211_CHAN_VHT20|IEEE80211_CHAN_VHT40D,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_USE_HT,IEEE80211_CHAN_WIDTH_20,true},
  {five|IEEE80211_CHAN_HT40U|IEEE80211_CHAN_VHT20|IEEE80211_CHAN_VHT40U|IEEE80211_CHAN_VHT80,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_80MHZ,IEEE80211_CHAN_WIDTH_80,true},
  {five|IEEE80211_CHAN_HT40U|IEEE80211_CHAN_VHT20|IEEE80211_CHAN_VHT40U|IEEE80211_CHAN_VHT80,IEEE80211_HTOP0_SCO_SCA,IEEE80211_VHT_CHANWIDTH_160MHZ,IEEE80211_CHAN_WIDTH_40,true},
 };
 for(const auto&test:cases){ieee80211_channel channel{test.flags,5180,0,0};ieee80211_node node;node.ni_chan=&channel;node.ni_flags=IEEE80211_NODE_HT;node.ni_htcaps=IEEE80211_HTCAP_CBW20_40;node.ni_htop0=test.sco;node.ni_vht_chanwidth=test.apwidth;node.ni_vht_chan1=42;
  ieee80211com ic;ic.ic_flags=IEEE80211_F_HTON|IEEE80211_F_VHTON;ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40;
  ieee80211_ht_negotiate_chw(&ic,&node);ieee80211_vht_negotiate(&ic,&node);
  require(node.ni_chw==test.width,"actual HT/VHT negotiation is bounded by registered width and sideband");
  require(bool(node.ni_flags&IEEE80211_NODE_VHT)==test.vht,"actual VHT negotiation needs target registered VHT permission");
  require(channel.ic_flags==test.flags,"negotiation preserves registered channel policy");
 }
 for(auto*channel:{(ieee80211_channel*)nullptr,IEEE80211_CHAN_ANYC}){ieee80211_node node;node.ni_chan=channel;node.ni_flags=IEEE80211_NODE_HT|IEEE80211_NODE_VHT;node.ni_chw=IEEE80211_CHAN_WIDTH_80;ieee80211com ic;ic.ic_flags=IEEE80211_F_HTON|IEEE80211_F_VHTON;ieee80211_ht_negotiate_chw(&ic,&node);ieee80211_vht_negotiate(&ic,&node);require(node.ni_chw==IEEE80211_CHAN_WIDTH_20&&!(node.ni_flags&IEEE80211_NODE_VHT),"unknown channel clears stale wide/VHT negotiation without dereferencing ANYC");}
 ieee80211_channel channel{five|IEEE80211_CHAN_HT40U|IEEE80211_CHAN_VHT80,5180,0,0};ieee80211_node node;node.ni_chan=&channel;node.ni_flags=IEEE80211_NODE_VHT;node.ni_htcaps=IEEE80211_HTCAP_CBW20_40;node.ni_htop0=IEEE80211_HTOP0_SCO_SCA;node.ni_chw=IEEE80211_CHAN_WIDTH_80;ieee80211com ic;ic.ic_flags=IEEE80211_F_HTON|IEEE80211_F_VHTON;ic.ic_htcaps=IEEE80211_HTCAP_CBW20_40;ieee80211_vht_negotiate(&ic,&node);require(node.ni_chw==IEEE80211_CHAN_WIDTH_20&&!(node.ni_flags&IEEE80211_NODE_VHT),"VHT negotiation requires a negotiated HT node");
}
int main(){wire_capabilities();negotiated_channel_policy();std::printf("PASS actual net80211 channel capabilities and negotiation (%u assertions)\n",checks);}
