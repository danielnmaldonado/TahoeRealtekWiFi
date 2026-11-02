#!/usr/bin/env python3
"""Actual B/C policy -> controller -> CCX/SMPS/timer integration, isolated OS/RF.
The canonical firmware mailbox implementation is linked unchanged.
"""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,re,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args()
s=(r/'kext/RTW88Backend.inc').read_text()+'\n'+(r/'kext/RTW88CoexBackend.inc').read_text()+'\n'+(r/'kext/RTWUSBController.cpp').read_text()
def method(source,marker):
 start=source.index(marker);brace=source.index('{',start);depth=1;end=brace+1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
markers=['bool RTWUSBController::owns88Discovery(','void RTWUSBController::clear88Discovery(','bool RTWUSBController::owns88Authority(','void RTWUSBController::remember88Authority(','bool RTWUSBController::cleanup88Authorized(','static uint16_t rtw88RateMask(','bool RTWUSBController::is88Family(','void RTWUSBController::refresh88Modes(','bool RTWUSBController::supports88Band(','bool RTWUSBController::permits88Channel(','unsigned RTWUSBController::r88RadioState(','int RTWUSBController::set88Channel(','int RTWUSBController::set88Operating(','int RTWUSBController::peer88Operating(','bool RTWUSBController::uses88HighThroughput(','bool RTWUSBController::accepts88ReceiveRate(','int RTWUSBController::ampdu88TXStart(','void RTWUSBController::update88Width(','uint16_t RTWUSBController::station88Rates(','int RTWUSBController::station88Hardware(','int RTWUSBController::state88(','void RTWUSBController::update88EDCA(','bool RTWUSBController::uses88FirmwareRate(','int RTWUSBController::tick88(','int RTWUSBController::start88(','void RTWUSBController::stop88(','int RTWUSBController::send88Startup(','rtw_packet_error RTWUSBController::decode88USBFrame(','int RTWUSBController::sample88RSSI(','void RTWUSBController::handle88C2H(','int RTWUSBController::send88Frame(']
for name,kind in [('coex88Ready','bool'),('coex88Input','rtw88_coex_input'),('coex88Start','int'),('coex88Notify','int'),('coex88Stop','void'),('coex88Schedule','void'),('coex88Tick','int'),('coex88TimerAction','void'),('prepare88Rate','int'),('send88SMPS','int'),('coex88Mimo','int'),('handle88SMPSReport','bool')]:markers.append(kind+' RTWUSBController::'+name+'(')
text=(r/'tests/rtw88_backend_harness.cpp').read_text();text=text[:text.index('static void firmwareRegression')]
text=text.replace(method(text,'extern "C" int rtw88_fw_h2c('),'')
text=text.replace('class RTWUSBController{public:','class RTWUSBController:public OSObject{public:\n unsigned quiesces=0;void quiesce(bool){++quiesces;stop88();}')
text=text.replace('uint16_t ni_associd,ni_intval;','uint16_t ni_associd,ni_intval,ni_txseq=0;')
text=text.replace('ieee80211_edca_ac_params ic_edca_ac[4]{};','ieee80211_edca_ac_params ic_edca_ac[4]{};uint8_t ic_myaddr[6]{2};int ic_nnodes=3;')
surface='''
using UInt32=uint32_t;static uint64_t testNow=1000;static uint64_t nowMS(){return testNow;}
class OSObject{public:virtual ~OSObject()=default;};
class IOTimerEventSource{public:uint64_t due=0;bool queued=false;void cancelTimeout(){queued=false;}void setTimeoutMS(UInt32 n){due=testNow+n;queued=true;}};
'''
text=text.replace('class OSObject;class IOTimerEventSource;',surface)
extra='''
static unsigned phyMimoCalls=0;static int phyMimoFail=0;
extern "C" int rtw88_phy_radio_set_txrx_1ss(rtw88_phy_context*c,int enabled){++phyMimoCalls;if(phyMimoFail)return phyMimoFail;static_cast<FakePHY*>(c->radio_state)->status.txrx_1ss=(uint8_t)enabled;return 0;}
extern "C" int rtw88_phy_write_rf(rtw88_phy_context*c,uint8_t path,uint8_t,uint32_t mask,uint32_t){assert(path<c->boot->board.rf_paths&&mask);uint32_t value=0;return c->boot->firmware.bus.read(c->boot->firmware.bus.cookie,0,4,&value);}
static int testIndirectRead(void*p,uint16_t,uint32_t*v){return rawRead(p,0x1708,4,v);}
static int testIndirectWrite(void*p,uint16_t,uint32_t v){return rawWrite(p,0x1708,4,v);}
enum{MBUF_DONTWAIT=0,MT_DATA=1};static bool noMbuf=false;
static int mbuf_gethdr(int,int,mbuf_t*out){*out=noMbuf?nullptr:new TestPacket;return noMbuf?ENOBUFS:0;}
static void mbuf_setlen(mbuf_t p,size_t n){p->bytes.resize(n);}static void mbuf_pkthdr_setlen(mbuf_t p,size_t n){assert(p->bytes.size()==n);}static void*mbuf_data(mbuf_t p){return p->bytes.data();}
static void ieee80211_ref_node(ieee80211_node*){}
'''
text=text.replace('/* ACTUAL_BACKEND_METHODS */',extra+'\n'+'\n'.join(method(s,m) for m in markers))
old=method(text,'static int rawWrite(')
text=text.replace(old,old[:-1]+'''if(a>=0x1d0&&a<=0x1dc&&w==4){std::array<uint8_t,8>b{};memcpy(b.data(),regs.data()+a,4);memcpy(b.data()+4,regs.data()+0x1f0+a-0x1d0,4);commands.push_back(b);}return 0;}'''.replace('return 0;}if','if'))
# Remove the original early return before the newly appended mailbox capture.
text=text.replace('return 0;if(a>=0x1d0','if(a>=0x1d0')
edca=(r/'vendor/itl80211/openbsd/net80211/ieee80211_output.c').read_text();defaults=[]
for mode in ['11B','11A','11G']:
 block=re.search(r'\[IEEE80211_MODE_'+mode+r'\]\s*=\s*\{(.*?)\n\s*\}',edca,re.S).group(1)
 records=re.findall(r'\[EDCA_AC_(?:BK|BE|VI|VO)\]\s*=\s*(\{[^}]+\})',block);assert len(records)==4;defaults.append('{'+','.join(records)+'}')
text=text.replace('/* ACTUAL_DEFAULT_EDCA */','static const ieee80211_edca_ac_params ieee80211_edca_table[3][4]={'+','.join(defaults)+'};')
text=text.replace('#include "../kext/RTW88BackendMembers.inc"','#include "'+str(r/'kext/RTW88BackendMembers.inc')+'"')
text+=(r/'tests/rtw88_coex_backend_tests.inc').read_text()
with tempfile.TemporaryDirectory(prefix='rtw88-coex-backend-') as tmp:
 d=Path(tmp);src=d/'backend.cpp';src.write_text(text);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')];objects=[]
 for name in ['ChipIdentity','USBWiFiProfiles','RTW88Coex','RTW88Firmware','RTW88Station','RTW88USBDataPath','RTW88RX','RTL8814AUDataPath','USBPackets']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 out=(a.out or d/'tests').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(src),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
