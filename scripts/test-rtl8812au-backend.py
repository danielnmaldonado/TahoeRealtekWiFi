#!/usr/bin/env python3
"""Run actual AU controller methods; independent fake host/RF/USB boundaries."""
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
from family_control_host import augment_control
import argparse,re,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--control',action='store_true');p.add_argument('--out',type=Path);a=p.parse_args()
s=(r/'kext/RTW8812Backend.inc').read_text()
def method(marker):
 start=s.index(marker);brace=s.index('{',start);depth=1;end=brace+1
 while depth:
  if s[end]=='{':depth+=1
  elif s[end]=='}':depth-=1
  end+=1
 return s[start:end]
markers=['static bool rtw8812SamePHYIdentity(', 'void RTWUSBController::remember8812Authority(', 'bool RTWUSBController::owns8812Authority(', 'int RTWUSBController::require8812Hardware(', 'void RTWUSBController::update8812EDCA(', 'rtw_r21au_coex_input RTWUSBController::input8812Coex(', 'int RTWUSBController::notify8812Coex(', 'static uint16_t rtw8812RateMask(', 'bool RTWUSBController::is8812Family(', 'bool RTWUSBController::supports8812Band(',
 'void RTWUSBController::refresh8812Modes(', 'uint32_t RTWUSBController::channel8812Modes(', 'int RTWUSBController::peer8812Operating(', 'int RTWUSBController::set8812Operating(', 'bool RTWUSBController::uses8812HighThroughput(', 'int RTWUSBController::ampdu8812TXStart(', 'bool RTWUSBController::accepts8812ReceiveRate(', 'uint16_t RTWUSBController::station8812Rates(', 'int RTWUSBController::set8812Channel(', 'int RTWUSBController::station8812Hardware(',
 'int RTWUSBController::state8812(', 'bool RTWUSBController::uses8812FirmwareRate(', 'int RTWUSBController::tick8812(',
 'int RTWUSBController::start8812(', 'void RTWUSBController::stop8812(', 'rtw_packet_error RTWUSBController::decode8812USBFrame(', 'int RTWUSBController::sample8812RSSI(', 'void RTWUSBController::handle8812C2H(', 'int RTWUSBController::send8812Frame(']
with tempfile.TemporaryDirectory(prefix='r12au-backend-') as temp:
 d=Path(temp);source=d/'backend.cpp';h=(r/'tests/rtl8812au_backend_harness.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(map(method,markers))).replace('#include "../kext/RTW8812BackendMembers.inc"', '#include "'+str(r/'kext/RTW8812BackendMembers.inc')+'"');
 for name in ['RTL8812AUFirmware.inc','RTL8821AUFirmware.inc']:h=h.replace('#include \"../kext/'+name+'\"','#include \"'+str(r/'kext'/name)+'\"')
 table_source=(r/'vendor/itl80211/openbsd/net80211/ieee80211_output.c').read_text();tables=[]
 for mode in ['11B','11A','11G']:
  block=re.search(r'\[IEEE80211_MODE_'+mode+r'\]\s*=\s*\{(.*?)\n\s*\}',table_source,re.S)[1]
  records=dict(re.findall(r'\[EDCA_AC_(BK|BE|VI|VO)\]\s*=\s*(\{[^}]+\})',block));assert len(records)==4
  tables.append('{'+','.join(records[ac] for ac in ['BE','BK','VI','VO'])+'}')
 h=h.replace('/* ACTUAL_DEFAULT_EDCA */','static const ieee80211_edca_ac_params ieee80211_edca_table[4][4]={{},'+','.join(tables)+'};')
 source.write_text(augment_control(h,r,'8812') if a.control else h)
 output=(a.out or d/'tests').resolve();output.parent.mkdir(parents=True,exist_ok=True)
 flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')]
 objects=[]
 for name in ['RTL8812AURuntime','RTL8812AUDataPath','RTL8812AUMAC','RTL8821AUCoex','USBPackets','USBWiFiProfiles']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 subprocess.run([*compiler(),'-std=c++17',*flags,str(source),*objects,'-o',str(output)],check=True);subprocess.run([str(output)],check=True)
