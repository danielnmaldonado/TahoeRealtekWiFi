#!/usr/bin/env python3
"""Actual FU power-derived channel flags -> imported HT20/40 -> actual rate config.
Only host types and logging are isolated. No pre-set negotiated NODE_HT/VHT.
"""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import re,tempfile,subprocess,argparse
p=argparse.ArgumentParser();p.add_argument('--out',type=Path);p.add_argument('--backend-source',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1];base=r/'vendor/itl80211/openbsd/net80211'
core=(base/'ieee80211.c').read_text();proto=(base/'ieee80211_proto.c').read_text();var=(base/'ieee80211_var.h').read_text();node=(base/'ieee80211_node.h').read_text();ieee=(base/'ieee80211.h').read_text();backend=(a.backend_source or r/'kext/RTW8192FUBackend.inc').read_text()
def body(s,name):
 m=re.search(re.escape(name)+r'\([^;]*?\)(?: const)?\s*\{',s);assert m,name;start=m.start();brace=s.index('{',m.end()-1);depth=1;end=brace+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
macros={}
for s in [(base/'ieee80211_ioctl.h').read_text(),ieee,node,var]:
 for line in s.replace('\\\n',' ').splitlines():
  if re.match(r'^#define\s+IEEE80211_(?:CHAN_|IS_CHAN_|F_|HTCAP_|HTOP|HT_|VHT|NODE_(?:HT|VHT)|C_SUPPORTS)',line):macros[re.match(r'^#define\s+(\w+)',line).group(1)]=line
mode=re.search(r'enum ieee80211_phymode\s*\{.*?\};',var,re.S).group(0);width=re.search(r'enum ieee80211_chan_width\s*\{.*?\};',ieee,re.S).group(0)
text='''#include "rtw/RTL8192FU.h"
#include <cstdint>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <initializer_list>
#include <array>
#define XYLog(...) ((void)0)
#define setbit(a,b) ((a)[(b)/8]|=(1U<<((b)%8)))
#define isset(a,b) ((a)[(b)/8]&(1U<<((b)%8)))


'''+mode+'\n'+width+'\n'+'\n'.join(macros.values())+'''
enum {IEEE80211_M_STA=1,IEEE80211_CIPHER_USEGROUP=1,IEEE80211_CIPHER_TKIP=4,IEEE80211_CIPHER_CCMP=8};
struct ieee80211_channel{uint32_t ic_freq=0,ic_flags=0,ic_center_freq1=0,ic_center_freq2=0;};
struct ieee80211_node{ieee80211_channel*ni_chan=nullptr;uint32_t ni_flags=0,ni_htcaps=0,ni_vhtcaps=0,ni_rsnciphers=0;uint8_t ni_chw=0,ni_htop0=0,ni_rxmcs[10]{},ni_basic_mcs[10]{},ni_vht_chan1=0,ni_vht_chan2=0,ni_vht_chanwidth=0;uint16_t ni_htop1=0;};
struct ieee80211com{ieee80211_channel ic_channels[256]{};uint8_t ic_chan_avail[32]{},ic_sup_mcs[10]{};uint32_t ic_modecaps=0,ic_curmode=0,ic_flags=0,ic_htcaps=0,ic_vhtcaps=0,ic_caps=0,ic_userflags=0,ic_opmode=IEEE80211_M_STA;ieee80211_node*ic_bss=nullptr;ieee80211_channel*ic_des_chan=nullptr;struct{unsigned is_ht_nego_no_mandatory_mcs=0,is_ht_nego_no_basic_mcs=0,is_ht_nego_bad_crypto=0,is_vht_nego_no_mandatory_mcs=0,is_vht_nego_bad_crypto=0;}ic_stats;};
#define _ifnet ieee80211com
static int ieee80211_chan2ieee(ieee80211com*c,ieee80211_channel*p){return int(p-c->ic_channels);}
static unsigned ieee80211_ieee2mhz(unsigned c,unsigned flags){return flags&IEEE80211_CHAN_2GHZ?2407+c*5:5000+c*5;}
'''
for name in ['ieee80211_node_supports_ht','ieee80211_node_supports_ht_sgi20','ieee80211_node_supports_ht_sgi40']:text+='static int '+body(node,name)+'\n'
for source,name in [(core,'ieee80211_channel_init'),(proto,'ieee80211_ht_negotiate_chw'),(proto,'ieee80211_ht_negotiate')]:text+='static void '+body(source,name)+'\n'
text+='''struct RTWUSBController {struct {ieee80211com sc_ic;}core_;rtw_r92fu_board fuBoard_{};bool fuHTSupported_=true;
uint32_t channel8192FUModes(uint8_t) const;void refresh8192FUModes();int peer8192FUOperating(rtw_r92fu_operating*) const;};\n'''
for ret,name in [('uint32_t','channel8192FUModes'),('void','refresh8192FUModes'),('int','peer8192FUOperating')]:text+=ret+' '+body(backend,'RTWUSBController::'+name)+'\n'
text+='static std::array<uint8_t,512> '+body((r/'tests/rtl8192fu_model.hpp').read_text(),'fu_rom')+'\n'
htcaps=re.findall(r'ic\.ic_htcaps\s*=\s*([^;]+);',backend)[-1]
text+='''int main(){unsigned checks=0;
for(unsigned ch:{1U,6U,11U}){for(unsigned width:{20U,40U}){
 RTWUSBController driver;auto rom=fu_rom();assert(!rtw_r92fu_decode_rom(rom.data(),512,&driver.fuBoard_));auto&ic=driver.core_.sc_ic;auto*c=&ic.ic_channels[ch];
 c->ic_freq=ieee80211_ieee2mhz(ch,IEEE80211_CHAN_2GHZ);c->ic_flags=driver.channel8192FUModes(ch)|IEEE80211_CHAN_2GHZ|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_CCK|IEEE80211_CHAN_DYN;
 ic.ic_htcaps='''+htcaps+''';ic.ic_sup_mcs[0]=ic.ic_sup_mcs[1]=255;ic.ic_flags=IEEE80211_F_RSNON;ieee80211_channel_init(&ic);driver.refresh8192FUModes();assert(ic.ic_modecaps&(1U<<IEEE80211_MODE_11N));assert(!(ic.ic_modecaps&((1U<<IEEE80211_MODE_11AC)|(1U<<IEEE80211_MODE_11AX))));
 ieee80211_node peer;ic.ic_bss=&peer;peer.ni_chan=c;peer.ni_flags=IEEE80211_NODE_HTCAP;peer.ni_htcaps=IEEE80211_HTCAP_CBW20_40|IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_SGI40;peer.ni_rxmcs[0]=peer.ni_rxmcs[1]=255;peer.ni_basic_mcs[0]=1;peer.ni_rsnciphers=IEEE80211_CIPHER_CCMP;peer.ni_htop0=width==20?0:ch==11?IEEE80211_HTOP0_SCO_SCB:IEEE80211_HTOP0_SCO_SCA;
 assert(!(peer.ni_flags&IEEE80211_NODE_HT));ieee80211_ht_negotiate(&ic,&peer);assert(peer.ni_flags&IEEE80211_NODE_HT);rtw_r92fu_operating op{};assert(!driver.peer8192FUOperating(&op));assert(op.primary==ch&&op.width_mhz==width&&op.center==(width==20?ch:ch==11?ch-2:ch+2));
 rtw_r92fu_rate rate{};assert(rtw_r92fu_rate_build(0xfff,peer.ni_rxmcs,op.ht,width,1,1,&rate));assert(rate.ht&&rate.initial==27&&rate.width_mhz==width);
 peer.ni_rsnciphers=IEEE80211_CIPHER_TKIP;ieee80211_ht_negotiate(&ic,&peer);assert(!(peer.ni_flags&IEEE80211_NODE_HT));peer.ni_rsnciphers=IEEE80211_CIPHER_CCMP;ic.ic_curmode=IEEE80211_MODE_11G;driver.refresh8192FUModes();ieee80211_ht_negotiate(&ic,&peer);assert(!(peer.ni_flags&IEEE80211_NODE_HT));ic.ic_curmode=IEEE80211_MODE_AUTO;driver.refresh8192FUModes();ic.ic_userflags=IEEE80211_F_NOHT40;ieee80211_ht_negotiate(&ic,&peer);assert((peer.ni_flags&IEEE80211_NODE_HT)&&peer.ni_chw==IEEE80211_CHAN_WIDTH_20);++checks;
 }}
 printf("FU %u actual imported HT20/40 CCMP negotiations, source power-derived channel flags, final profiles/RA and TKIP/media/NOHT40 guards passed\\n",checks);}
'''
with tempfile.TemporaryDirectory(prefix='r92fu-negotiation-') as temp:
 d=Path(temp);src=d/'negotiation.cpp';src.write_text(text);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
 for name in ['RTL8192FU','RTL8192FUDataPath','USBControl','USBRegisterIO','Efuse']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 out=(a.out or d/'test').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(src),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
