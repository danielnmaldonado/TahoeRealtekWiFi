#!/usr/bin/env python3
"""Actual RTL8723BU adapter and actual core, modeled USB/OS boundaries only."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,re,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--out',type=Path);a=p.parse_args()
s=(r/'kext/RTW8723BUBackend.inc').read_text()
def method(marker):
 start=s.index(marker);opening=s.index('{',start);depth=1;end=opening+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
markers=['static uint16_t rtw8723RateMask(','bool RTWUSBController::is8723Family(','bool RTWUSBController::supports8723Band(','bool RTWUSBController::permits8723Channel(','int RTWUSBController::r23Read(','int RTWUSBController::r23Write(','int RTWUSBController::r23Region(','int RTWUSBController::bootstrap8723(','unsigned RTWUSBController::r23RadioState(','int RTWUSBController::set8723Channel(','int RTWUSBController::start8723(','void RTWUSBController::stop8723(','void RTWUSBController::release8723Storage(','uint16_t RTWUSBController::station8723Rates(','int RTWUSBController::station8723Hardware(','int RTWUSBController::state8723(','void RTWUSBController::update8723EDCA(','bool RTWUSBController::uses8723FirmwareRate(','int RTWUSBController::tick8723(','rtw_packet_error RTWUSBController::decode8723USBFrame(','int RTWUSBController::sample8723RSSI(','void RTWUSBController::handle8723C2H(','int RTWUSBController::send8723Frame(']
core=(r/'tests/rtl8723bu_core_tests.cpp').read_text().split('static void read_factory_from_otp(')[0]
# This family owns its OS shapes; no mutable B/C harness is imported.
harness=(r/'tests/rtl8723bu_backend_harness.cpp').read_text()
stack=r/'vendor/itl80211/openbsd/net80211'
used=set(re.findall(r'\bIEEE80211_\w+\b',harness+s))
flags={}
for name in ['ieee80211.h','ieee80211_var.h','ieee80211_node.h','ieee80211_crypto.h']:
 for line in (stack/name).read_text().replace('\\\n',' ').splitlines():
  match=re.match(r'^#define\s+(IEEE80211_\w+)\s+(.+)',line)
  if match and match[1] in used and match[1]!='IEEE80211_CHAN_ANYC':flags[match[1]]=line
cipher=re.search(r'enum ieee80211_cipher\s*\{.*?\};',(stack/'ieee80211_crypto.h').read_text(),re.S).group()
edca=(stack/'ieee80211_output.c').read_text();rows=[]
for mode in ['11B','11A','11G']:
 block=re.search(r'\[IEEE80211_MODE_'+mode+r'\]\s*=\s*\{(.*?)\n\s*\}',edca,re.S).group(1)
 records=[re.search(r'\[EDCA_AC_'+ac+r'\]\s*=\s*(\{[^}]+\})',block).group(1) for ac in ['BE','BK','VI','VO']]
 rows.append('{'+','.join(records)+'}')
defaults='static const ieee80211_edca_ac_params ieee80211_edca_table[3][4]={'+','.join(rows)+'};'
text=harness.replace('/* ACTUAL_CORE_USB_MODEL */',core).replace('/* ACTUAL_STACK_FLAGS */','\n'.join(flags.values())+'\n'+cipher).replace('/* ACTUAL_DEFAULT_EDCA */',defaults).replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(map(method,markers)))
with tempfile.TemporaryDirectory(prefix='rtl8723bu-backend-') as tmp:
 d=a.out or Path(tmp);d.mkdir(exist_ok=True,parents=True);src=d/'backend.cpp';src.write_text(text)
 flags=['-g','-O1','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'kext')];objects=[]
 for name in ['RTL8723BU','RTL8723BUDataPath','USBPackets','RTL8192EUDataPath','USBWiFiProfiles']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 binary=d/'rtl8723bu-backend-tests';subprocess.run([*compiler(),'-std=c++17',*flags,str(src),*objects,'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
