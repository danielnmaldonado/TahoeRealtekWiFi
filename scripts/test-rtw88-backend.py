#!/usr/bin/env python3
"""Execute actual B/C controller methods against isolated host/USB/PHY boundaries."""
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
from family_control_host import augment_control
import argparse,re,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--control',action='store_true');p.add_argument('--out',type=Path);p.add_argument('--backend-source',type=Path);p.add_argument('--case',choices=['firmware','smps','edca']);a=p.parse_args();s=(a.backend_source or r/'kext/RTW88Backend.inc').read_text()+'\n'+(r/'kext/RTW88CoexBackend.inc').read_text()+'\n'+(r/'kext/RTWUSBController.cpp').read_text()
def method(marker):
 start=s.index(marker);brace=s.index('{',start);depth=1;end=brace+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
markers=['bool RTWUSBController::owns88Discovery(','void RTWUSBController::clear88Discovery(','bool RTWUSBController::owns88Authority(','void RTWUSBController::remember88Authority(','bool RTWUSBController::cleanup88Authorized(','static uint16_t rtw88RateMask(','bool RTWUSBController::is88Family(','void RTWUSBController::refresh88Modes(','bool RTWUSBController::supports88Band(','bool RTWUSBController::permits88Channel(','unsigned RTWUSBController::r88RadioState(','int RTWUSBController::set88Channel(','int RTWUSBController::set88Operating(','int RTWUSBController::peer88Operating(','bool RTWUSBController::uses88HighThroughput(','bool RTWUSBController::accepts88ReceiveRate(','int RTWUSBController::ampdu88TXStart(','void RTWUSBController::update88Width(','uint16_t RTWUSBController::station88Rates(','int RTWUSBController::station88Hardware(','int RTWUSBController::state88(','void RTWUSBController::update88EDCA(','bool RTWUSBController::uses88FirmwareRate(','int RTWUSBController::tick88(','int RTWUSBController::start88(','void RTWUSBController::stop88(','int RTWUSBController::send88Startup(','rtw_packet_error RTWUSBController::decode88USBFrame(','int RTWUSBController::sample88RSSI(','void RTWUSBController::handle88C2H(','int RTWUSBController::send88Frame(']
# Use the actual imported stack's defaults, not a second EDCA model.
edca=(r/'vendor/itl80211/openbsd/net80211/ieee80211_output.c').read_text()
defaults=[]
for mode in ['11B','11A','11G']:
 block=re.search(r'\[IEEE80211_MODE_'+mode+r'\]\s*=\s*\{(.*?)\n\s*\}',edca,re.S).group(1)
 records=re.findall(r'\[EDCA_AC_(?:BK|BE|VI|VO)\]\s*=\s*(\{[^}]+\})',block)
 assert len(records)==4
 defaults.append('{'+','.join(records)+'}')
default_table='static const ieee80211_edca_ac_params ieee80211_edca_table[3][4]={' + ','.join(defaults)+'};'
with tempfile.TemporaryDirectory(prefix='rtw88-backend-') as tmp:
 markers+=['int RTWUSBController::prepare88Rate(','bool RTWUSBController::coex88Ready(','bool RTWUSBController::handle88SMPSReport(']
 d=Path(tmp);src=d/'backend.cpp';text=(r/'tests/rtw88_backend_harness.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(map(method,markers))).replace('/* ACTUAL_DEFAULT_EDCA */',default_table).replace('#include "../kext/RTW88BackendMembers.inc"','#include "'+str(r/'kext/RTW88BackendMembers.inc')+'"');src.write_text(augment_control(text,r,'88') if a.control else text)
 text=src.read_text().replace('class OSObject;class IOTimerEventSource;','class OSObject;class IOTimerEventSource{public:void cancelTimeout(){}};').replace('struct FakePHY{','static uint64_t nowMS();\nstruct FakePHY{')
 text+='''
static uint64_t nowMS(){return 1000;}
extern "C" int rtw88_phy_write_rf(rtw88_phy_context*,uint8_t,uint8_t,uint32_t,uint32_t){assert(false);return ENOTSUP;}
extern "C" int rtw88_phy_radio_set_txrx_1ss(rtw88_phy_context*,int){assert(false);return ENOTSUP;}
int RTWUSBController::coex88Start(){assert(!r88Core_.board.btcoex);return 0;}
void RTWUSBController::coex88Stop(){assert(!r88Coex_);}
void RTWUSBController::coex88Schedule(){assert(!r88Coex_);}
int RTWUSBController::coex88Notify(rtw88_coex_event,ieee80211_state,uint8_t){assert(!r88Core_.board.btcoex);return 0;}
rtw88_coex_input RTWUSBController::coex88Input(ieee80211_state,uint8_t)const{assert(!r88Coex_);return {};}
int RTWUSBController::send88SMPS(bool){assert(false);return ENOTSUP;}
'''
 src.write_text(text)
 flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')];objects=[]
 for name in ['ChipIdentity','USBWiFiProfiles','RTW88Coex','RTW88Station','RTW88USBDataPath','RTW88RX','RTL8814AUDataPath','USBPackets']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 output=(a.out or d/'tests').resolve();output.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(src),*objects,'-o',str(output)],check=True);subprocess.run([str(output),*([a.case] if a.case else [])],check=True)
