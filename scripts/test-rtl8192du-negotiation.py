#!/usr/bin/env python3
"""Execute complete imported channel-init and HT-negotiation functions.
Backend channel/capability expressions are read from production attach code.
DU channel/cap/MCS expressions are production source. Both1T DMDP and2T SMSP
use the actual imported channel-init/AUTO/HT20/CCMP negotiation. No hardware IO.
"""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import re,tempfile,subprocess,argparse
p=argparse.ArgumentParser();p.add_argument('--source',type=Path);a=p.parse_args();r=Path(__file__).resolve().parents[1];base=r/'vendor/itl80211/openbsd/net80211'
core=(base/'ieee80211.c').read_text();proto=(base/'ieee80211_proto.c').read_text();var=(base/'ieee80211_var.h').read_text();node=(base/'ieee80211_node.h').read_text();ieee=(base/'ieee80211.h').read_text()
def function(s,name):
 m=re.search(r'(?m)^'+name+r'\([^;]*?\)\s*\{',s);assert m,name;start=m.start();brace=s.index('{',start);depth=1;end=brace+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
macros={}
for s in [(base/'ieee80211_ioctl.h').read_text(),ieee,node,var]:
 for line in s.replace('\\\n',' ').splitlines():
  if re.match(r'^#define\s+IEEE80211_(?:CHAN_|IS_CHAN_|F_(?:HTON|VHTON|HEON|WEPON|RSNON|NOHT40)|HTCAP_|HTOP0_|HT_NUM_MCS|NODE_HT)',line):macros[re.match(r'^#define\s+(\w+)',line).group(1)]=line
mode=re.search(r'enum ieee80211_phymode\s*\{.*?\};',var,re.S).group(0);width=re.search(r'enum ieee80211_chan_width\s*\{.*?\};',ieee,re.S).group(0)
media=function(core,'ieee80211_media_change');start=media.index('ic->ic_flags &= ~(IEEE80211_F_HTON | IEEE80211_F_VHTON);');end=media.index('if ((ic->ic_flags & (IEEE80211_F_HTON | IEEE80211_F_VHTON)) == 0)',start);select=media[start:end]
text='''#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <cassert>
#define XYLog(...) ((void)0)
#define setbit(a,b) ((a)[(b)/8]|=(1U<<((b)%8)))
#define isset(a,b) ((a)[(b)/8]&(1U<<((b)%8)))
'''+mode+'\n'+width+'\n'+'\n'.join(macros.values())+'''
enum {IEEE80211_M_STA=1,IEEE80211_CIPHER_USEGROUP=1,IEEE80211_CIPHER_TKIP=4,IEEE80211_CIPHER_CCMP=8};
struct ieee80211_channel{uint32_t ic_freq=0,ic_flags=0,ic_center_freq1=0;};
struct ieee80211_node{ieee80211_channel*ni_chan=nullptr;uint32_t ni_flags=0,ni_htcaps=0,ni_rsnciphers=0;uint8_t ni_chw=0,ni_htop0=0,ni_rxmcs[10]{},ni_basic_mcs[10]{};};
struct ieee80211com{ieee80211_channel ic_channels[256]{};uint8_t ic_chan_avail[32]{},ic_sup_mcs[10]{};uint32_t ic_modecaps=0,ic_curmode=0,ic_flags=0,ic_htcaps=0,ic_userflags=0,ic_opmode=IEEE80211_M_STA;ieee80211_channel*ic_des_chan=nullptr;struct{unsigned is_ht_nego_no_mandatory_mcs=0,is_ht_nego_no_basic_mcs=0,is_ht_nego_bad_crypto=0;}ic_stats;};
#define IEEE80211_CHAN_MAX 255
#define _ifnet ieee80211com
static int ieee80211_chan2ieee(ieee80211com*c,ieee80211_channel*p){return int(p-c->ic_channels);}
static void ieee80211_configure_ampdu_tx(ieee80211com*,int){}
'''
for name in ['ieee80211_node_supports_ht','ieee80211_node_supports_ht_sgi20','ieee80211_node_supports_ht_sgi40']:text+='static int '+function(node,name)+'\n'
text+='static void '+function(core,'ieee80211_channel_init')+'\nstatic void '+function(proto,'ieee80211_ht_negotiate_chw')+'\nstatic void '+function(proto,'ieee80211_ht_negotiate')+'\n'
text+='static void select_auto(ieee80211com*ic){const auto newphymode=IEEE80211_MODE_AUTO;'+select+'}\nint main(){\n'
for name in ['RTW8192DUBackend.inc']:
 source=(a.source or r/'kext'/name).read_text();mcs=re.search(r'(memset\(ic\.ic_sup_mcs.*?)(?=ic\.ic_tx_mcs_set)',source,re.S).group(1);flags=re.findall(r'ic\.ic_channels\[ch\]\.ic_flags\s*=\s*([^;]+);',source);caps=re.search(r'ic\.ic_htcaps\s*=\s*([^;]+);',source).group(1)
 assert len(flags)>=2,name
 for paths in [1,2]:
  for five,expression in enumerate(flags[:2]):
   text+=''' {ieee80211com ic;struct {struct {unsigned rf_paths;}board;}duROM_{{'''+str(paths)+'''}};auto*c=&ic.ic_channels['''+('36' if five else '6')+'''];c->ic_freq='''+('5180' if five else '2437')+''';c->ic_flags='''+expression+''';ic.ic_htcaps='''+caps+''';'''+mcs+'''assert(ic.ic_sup_mcs[1]=='''+str(255 if paths==2 else 0)+''');ic.ic_flags=IEEE80211_F_RSNON;ieee80211_channel_init(&ic);select_auto(&ic);
 ieee80211_node peer;peer.ni_chan=c;peer.ni_flags=IEEE80211_NODE_HTCAP;peer.ni_htcaps=IEEE80211_HTCAP_SGI20|IEEE80211_HTCAP_CBW20_40;peer.ni_rxmcs[0]=255;peer.ni_basic_mcs[0]=1;peer.ni_rsnciphers=IEEE80211_CIPHER_CCMP;assert(!(peer.ni_flags&IEEE80211_NODE_HT));ieee80211_ht_negotiate(&ic,&peer);
 if(!(peer.ni_flags&IEEE80211_NODE_HT)){std::fprintf(stderr,"'''+name+''' '''+str(paths)+'''T band'''+str(five)+''': actual imported negotiation rejected HT:modecaps=0x%x flags=0x%x\\n",ic.ic_modecaps,ic.ic_flags);return 1;}
 assert(peer.ni_flags&IEEE80211_NODE_HT_SGI20);assert(peer.ni_chw==IEEE80211_CHAN_WIDTH_20&&c->ic_center_freq1==c->ic_freq);
 peer.ni_rsnciphers=IEEE80211_CIPHER_TKIP;ieee80211_ht_negotiate(&ic,&peer);assert(!(peer.ni_flags&IEEE80211_NODE_HT)&&ic.ic_stats.is_ht_nego_bad_crypto==1);
 peer.ni_rsnciphers=IEEE80211_CIPHER_CCMP;peer.ni_basic_mcs['''+str(paths)+''']=1;ieee80211_ht_negotiate(&ic,&peer);assert(!(peer.ni_flags&IEEE80211_NODE_HT)&&ic.ic_stats.is_ht_nego_no_basic_mcs==1);
 std::puts("'''+name+''' '''+str(paths)+'''T band'''+str(five)+''': real channel-init/AUTO/HT20-SGI/CCMP and negative peer negotiation passed");}
'''
text+='return 0;}\n'
with tempfile.TemporaryDirectory(prefix='rtl8192du-imported-ht-') as tmp:
 d=Path(tmp);src=d/'ht.cpp';src.write_text(text);exe=d/'ht';subprocess.run([*compiler(),'-std=c++17','-UNDEBUG','-Wall','-Wextra','-Werror',*sanitizer_flags(),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
