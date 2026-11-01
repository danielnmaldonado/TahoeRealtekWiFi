#!/usr/bin/env python3
"""Run primary imported net80211 mode/HT guards with actual backend channels.
Independent review reproducer; no device access or implementation changes.
--require-ht returns nonzero while advertised HT backends cannot reach HT.
"""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,re,tempfile,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--require-ht',action='store_true');a=p.parse_args()
core=(r/'vendor/itl80211/openbsd/net80211/ieee80211.c').read_text();proto=(r/'vendor/itl80211/openbsd/net80211/ieee80211_proto.c').read_text();var=(r/'vendor/itl80211/openbsd/net80211/ieee80211_var.h').read_text()
def function(s,name):
 m=re.search(r'(?m)^'+name+r'\([^;]*?\)\s*\{',s);assert m,name;start=m.start();brace=s.index('{',start);level=1;end=brace+1
 while level:level+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
init=function(core,'ieee80211_channel_init');ht=function(proto,'ieee80211_ht_negotiate');gate=ht[ht.index('/* Check if we support HT. */'):ht.index('/*\n\t * Check if the peer supports HT.')]
media=function(core,'ieee80211_media_change');begin=media.index('ic->ic_flags &= ~(IEEE80211_F_HTON | IEEE80211_F_VHTON);');end=media.index('if ((ic->ic_flags & (IEEE80211_F_HTON | IEEE80211_F_VHTON)) == 0)',begin);select=media[begin:end]
macros=[]
for line in var.replace('\\\n',' ').splitlines():
 if re.match(r'^#define\s+IEEE80211_(?:CHAN_|IS_CHAN_|F_(?:HTON|VHTON|HEON))',line):macros.append(line)
mode=re.search(r'enum ieee80211_phymode\s*\{.*?\};',var,re.S).group(0)
pre='''#include <stdint.h>
#include <cstring>
#include <cstdio>
#define XYLog(...) ((void)0)
#define setbit(a,b) ((a)[(b)/8]|=(1U<<((b)%8)))
'''+mode+'\n'+'\n'.join(macros)+'''
struct ieee80211_channel{uint32_t ic_freq,ic_flags;};
struct ieee80211com{ieee80211_channel ic_channels[256]{};uint8_t ic_chan_avail[32]{};uint32_t ic_modecaps=0,ic_curmode=0,ic_flags=0;ieee80211_channel*ic_des_chan=nullptr;};
#define IEEE80211_CHAN_MAX 255
#define _ifnet ieee80211com
static int ieee80211_chan2ieee(ieee80211com*c,ieee80211_channel*p){return int(p-c->ic_channels);}
static void ieee80211_configure_ampdu_tx(ieee80211com*,int){}
static void '''+init+'''
static bool negotiated=false;
static void ht_gate(ieee80211com*ic){'''+gate+'''negotiated=true;}
static void select_auto(ieee80211com*ic){const auto newphymode=IEEE80211_MODE_AUTO;'''+select+'''}
int main(){unsigned missing=0;
'''
for name in ['RTW8814Backend.inc','RTW88Backend.inc']:
 s=(r/'kext'/name).read_text();m=re.search(r'ic\.ic_channels\[ch\]\.ic_flags\s*=\s*([^;]+);',s);assert m,name;flags=m.group(1)
 pre+=''' {ieee80211com ic;ic.ic_channels[6].ic_freq=2437;ic.ic_channels[6].ic_flags='''+flags+''';ieee80211_channel_init(&ic);ic.ic_flags|=IEEE80211_F_HTON;negotiated=false;ht_gate(&ic);if(!negotiated)++missing;std::printf("'''+name+''': HT reachable=%u modecaps=0x%x\\n",negotiated,ic.ic_modecaps);}
'''
pre+=''' {ieee80211com ic;ic.ic_channels[36].ic_freq=5180;ic.ic_channels[36].ic_flags=IEEE80211_CHAN_5GHZ|IEEE80211_CHAN_OFDM|IEEE80211_CHAN_HT20|IEEE80211_CHAN_VHT80;ieee80211_channel_init(&ic);select_auto(&ic);std::printf("VHT candidate: AUTO selects HE=%u VHT=%u; inferred11AX=%u\\n",!!(ic.ic_flags&IEEE80211_F_HEON),!!(ic.ic_flags&IEEE80211_F_VHTON),!!(ic.ic_modecaps&(1U<<IEEE80211_MODE_11AX)));}
 return '''+('missing?1:0' if a.require_ht else '(void)missing,0')+''';}
'''
with tempfile.TemporaryDirectory(prefix='realtek-mode-review-') as tmp:
 d=Path(tmp);source=d/'review.cpp';source.write_text(pre);output=d/'review';subprocess.run([*compiler(),'-std=c++17','-Wall','-Wextra','-Werror',*sanitizer_flags(),str(source),'-o',str(output)],check=True);subprocess.run([str(output)],check=True)
