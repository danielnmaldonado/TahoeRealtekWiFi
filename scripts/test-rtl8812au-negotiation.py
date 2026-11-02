#!/usr/bin/env python3
"""Actual AU power-derived channel flags -> imported HT/VHT -> actual rate config.
Only host types and logging are isolated. No pre-set negotiated NODE_HT/VHT.
"""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import re,tempfile,subprocess,argparse
p=argparse.ArgumentParser();p.add_argument('--out',type=Path);p.add_argument('--backend-source',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1];base=r/'vendor/itl80211/openbsd/net80211'
core=(base/'ieee80211.c').read_text();proto=(base/'ieee80211_proto.c').read_text();var=(base/'ieee80211_var.h').read_text();node=(base/'ieee80211_node.h').read_text();ieee=(base/'ieee80211.h').read_text();backend=(a.backend_source or r/'kext/RTW8812Backend.inc').read_text()
def body(s,name):
 m=re.search(re.escape(name)+r'\([^;]*?\)(?: const)?\s*\{',s);assert m,name;start=m.start();brace=s.index('{',m.end()-1);depth=1;end=brace+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
macros={}
for s in [(base/'ieee80211_ioctl.h').read_text(),ieee,node,var]:
 for line in s.replace('\\\n',' ').splitlines():
  if re.match(r'^#define\s+IEEE80211_(?:CHAN_|IS_CHAN_|F_|HTCAP_|HTOP|HT_|VHT|NODE_(?:HT|VHT)|C_SUPPORTS)',line):macros[re.match(r'^#define\s+(\w+)',line).group(1)]=line
mode=re.search(r'enum ieee80211_phymode\s*\{.*?\};',var,re.S).group(0);width=re.search(r'enum ieee80211_chan_width\s*\{.*?\};',ieee,re.S).group(0)
text='''#include "rtw/StationChannels.h"
#include "rtw/RTL8812AURuntime.h"
#include "rtw/USBWiFiProfiles.h"
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#define XYLog(...) ((void)0)
#define setbit(a,b) ((a)[(b)/8]|=(1U<<((b)%8)))
#define isset(a,b) ((a)[(b)/8]&(1U<<((b)%8)))
static uint16_t le16toh(uint16_t v){return v;}
static uint32_t u32_get_bits(uint32_t value,uint32_t mask){return (value&mask)>>__builtin_ctz(mask);}
'''+mode+'\n'+width+'\n'+'\n'.join(macros.values())+'''
enum {IEEE80211_M_STA=1,IEEE80211_CIPHER_USEGROUP=1,IEEE80211_CIPHER_TKIP=4,IEEE80211_CIPHER_CCMP=8};
struct ieee80211_channel{uint32_t ic_freq=0,ic_flags=0,ic_center_freq1=0,ic_center_freq2=0;};
struct ieee80211_node{ieee80211_channel*ni_chan=nullptr;uint32_t ni_flags=0,ni_htcaps=0,ni_vhtcaps=0,ni_rsnciphers=0;uint8_t ni_chw=0,ni_htop0=0,ni_rxmcs[10]{},ni_basic_mcs[10]{},ni_vht_chan1=0,ni_vht_chan2=0,ni_vht_chanwidth=0;uint16_t ni_htop1=0;};
struct ieee80211com{ieee80211_channel ic_channels[256]{};uint8_t ic_chan_avail[32]{},ic_sup_mcs[10]{};uint32_t ic_modecaps=0,ic_curmode=0,ic_flags=0,ic_htcaps=0,ic_vhtcaps=0,ic_caps=0,ic_userflags=0,ic_opmode=IEEE80211_M_STA;ieee80211_node*ic_bss=nullptr;ieee80211_channel*ic_des_chan=nullptr;struct{unsigned is_ht_nego_no_mandatory_mcs=0,is_ht_nego_no_basic_mcs=0,is_ht_nego_bad_crypto=0,is_vht_nego_no_mandatory_mcs=0,is_vht_nego_bad_crypto=0;}ic_stats;};
#define _ifnet ieee80211com
static int ieee80211_chan2ieee(ieee80211com*c,ieee80211_channel*p){return int(p-c->ic_channels);}
static unsigned ieee80211_ieee2mhz(unsigned c,unsigned flags){return flags&IEEE80211_CHAN_2GHZ?2407+c*5:5000+c*5;}
'''
for name in ['ieee80211_node_supports_ht','ieee80211_node_supports_ht_sgi20','ieee80211_node_supports_ht_sgi40','ieee80211_node_supports_vht','ieee80211_node_supports_vht_sgi80','ieee80211_node_supports_vht_sgi160']:text+='static int '+body(node,name)+'\n'
for source,name in [(core,'ieee80211_channel_init'),(proto,'ieee80211_ht_negotiate_chw'),(proto,'ieee80211_ht_negotiate'),(proto,'ieee80211_vht_negotiate')]:text+='static void '+body(source,name)+'\n'
text+='''struct RTWUSBController {bool permitsDFSChannelByCountry(uint8_t)const{return true;}struct {ieee80211com sc_ic;}core_;rtw_usb_wifi_profile profileValue_{};const rtw_usb_wifi_profile*profile_=&profileValue_;rtw_r12au_boot auCore_{};bool auHTSupported_=true,auVHTSupported_=true;
uint32_t channel8812Modes(uint8_t) const;void refresh8812Modes();int peer8812Operating(rtw_r12au_operating*) const;};\n'''
for ret,name in [('uint32_t','channel8812Modes'),('void','refresh8812Modes'),('int','peer8812Operating')]:text+=ret+' '+body(backend,'RTWUSBController::'+name)+'\n'
text+='static rtw_r12au_phy_board '+body((r/'tests/rtl8812au_phy_tests.cpp').read_text(),'board')+'\n'
htcaps=re.findall(r'ic\.ic_htcaps\s*=\s*([^;]+);',backend)[-1];vhtcaps=re.findall(r'ic\.ic_vhtcaps\s*=\s*([^;]+);',backend)[-1]
text+='''int main(){unsigned checks=0;
for(bool one:{false,true}){for(unsigned ch:{6U,36U,52U,56U,60U,64U,100U,104U,108U,112U,116U,120U,124U,128U,132U,136U,140U,149U}){for(unsigned width:{20U,40U,80U}){
 if(ch==6&&width==80)continue;bool dfs=rtw_station_is_dfs_channel(uint8_t(ch));unsigned admitted=dfs?20:width;RTWUSBController driver;driver.auCore_.board.phy=board(one);driver.auCore_.board.vht=1;auto&ic=driver.core_.sc_ic;auto*c=&ic.ic_channels[ch];
 c->ic_freq=ieee80211_ieee2mhz(ch,ch<14?IEEE80211_CHAN_2GHZ:IEEE80211_CHAN_5GHZ);c->ic_flags=driver.channel8812Modes(ch)|(ch<14?IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_CCK|IEEE80211_CHAN_DYN:IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM);
 if(dfs){assert(driver.channel8812Modes(uint8_t(ch))==IEEE80211_CHAN_HT20);ic.ic_channels[36].ic_freq=5180;ic.ic_channels[36].ic_flags=driver.channel8812Modes(36)|IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM;}
 ic.ic_htcaps='''+htcaps+''';ic.ic_vhtcaps='''+vhtcaps+''';ic.ic_sup_mcs[0]=255;if(!one)ic.ic_sup_mcs[1]=255;ic.ic_flags=IEEE80211_F_RSNON;
 ieee80211_channel_init(&ic);driver.refresh8812Modes();assert(ic.ic_modecaps&(1U<<IEEE80211_MODE_11N));assert(!(ic.ic_modecaps&(1U<<IEEE80211_MODE_11AX))&&!(ic.ic_flags&IEEE80211_F_HEON));
 ieee80211_node peer;ic.ic_bss=&peer;peer.ni_chan=c;peer.ni_flags=IEEE80211_NODE_HTCAP|(ch>14?IEEE80211_NODE_VHTCAP:0);peer.ni_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;peer.ni_vhtcaps=IEEE80211_VHTCAP_SHORT_GI_80;peer.ni_rxmcs[0]=255;if(!one)peer.ni_rxmcs[1]=255;peer.ni_basic_mcs[0]=1;peer.ni_rsnciphers=IEEE80211_CIPHER_CCMP;peer.ni_htop0=width>20?IEEE80211_HTOP0_SCO_SCA:0;peer.ni_vht_chanwidth=width==80?IEEE80211_VHT_CHANWIDTH_80MHZ:IEEE80211_VHT_CHANWIDTH_USE_HT;peer.ni_vht_chan1=ch<100?42:155;
 assert(!(peer.ni_flags&(IEEE80211_NODE_HT|IEEE80211_NODE_VHT)));ieee80211_ht_negotiate(&ic,&peer);ieee80211_vht_negotiate(&ic,&peer);
 assert(peer.ni_flags&IEEE80211_NODE_HT);assert(((peer.ni_flags&IEEE80211_NODE_VHT)!=0)==(ch>14&&!dfs));rtw_r12au_operating op{};assert(!driver.peer8812Operating(&op));if(op.primary!=ch||op.width_mhz!=admitted)fprintf(stderr,"one=%d ch=%u width=%u opwidth=%u opcenter=%u flags=%x chanflags=%x\\n",one,ch,width,op.width_mhz,op.center,peer.ni_flags,c->ic_flags);assert(op.primary==ch&&op.width_mhz==admitted&&op.center==(admitted==80?(ch<100?42:155):admitted==40?ch+2:ch));if(dfs)assert(op.mode==RTW_R12AU_MODE_HT);
 rtw_r12au_rate_config rate{};assert(rtw_r12au_build_rate_config(ch<14?0xfff:0xff0,peer.ni_rxmcs,one?0xfffe:0xfffa,one?1:2,op.mode,admitted,ch>14,1,1,&rate));assert(rate.ht&&rate.vht==(ch>14&&!dfs)&&rate.bandwidth_mhz==admitted);
 peer.ni_rsnciphers=IEEE80211_CIPHER_TKIP;ieee80211_ht_negotiate(&ic,&peer);ieee80211_vht_negotiate(&ic,&peer);assert(!(peer.ni_flags&(IEEE80211_NODE_HT|IEEE80211_NODE_VHT)));
 peer.ni_rsnciphers=IEEE80211_CIPHER_CCMP;ic.ic_userflags=IEEE80211_F_NOVHT;driver.refresh8812Modes();ieee80211_ht_negotiate(&ic,&peer);ieee80211_vht_negotiate(&ic,&peer);assert((peer.ni_flags&IEEE80211_NODE_HT)&&!(peer.ni_flags&IEEE80211_NODE_VHT));++checks;
 }}}
printf("AU %u actual imported HT/VHT negotiations, power-derived channel flags, final profiles/RA, CCMP/TKIP/NOVHT passed\\n",checks);}\n'''
with tempfile.TemporaryDirectory(prefix='r12au-negotiation-') as temp:
 d=Path(temp);src=d/'negotiation.cpp';src.write_text(text);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
 for name in ['RTL8812AUPHY','RTL8812AURuntime','RTL8812AUDataPath','RTL8812AUBootstrap','USBControl','USBRegisterIO','Efuse','USBWiFiProfiles']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 out=(a.out or d/'test').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(src),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
