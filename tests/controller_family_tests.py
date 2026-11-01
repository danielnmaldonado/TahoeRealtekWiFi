#!/usr/bin/env python3
"""Compile the actual controller family seed into the real chip identification."""
from pathlib import Path
import argparse,runpy,subprocess,re
project=Path(__file__).resolve().parents[1]
import runpy
_host = runpy.run_path(str(project / "scripts/host_cpp.py"))
_host_cxx = _host["compiler"]()
_host_sanitizers = _host["sanitizer_flags"]()
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out',type=Path,default=project/'build/controller-family')
args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
extract=runpy.run_path(str(project/'scripts/test-rtwn-ht.py'))['function']
source=(project/'vendor/rtwn/rtwn.cpp').read_text()
controller=(project/'kext/RTWUSBController.cpp').read_text()
a=controller.index('    core_.chip=RTWN_CHIP_USB;',controller.index('IOReturn RTWUSBController::initializePlatform('))
b=controller.index('    setProperty("RTWSiliconFamily"',a)
seed=controller[a:b]
extract_member=runpy.run_path(str(project/'scripts/test-rtwn-wpa-recovery.py'))['function']
selectors=''
for filename,name in [('RTW8812Backend.inc','is8812Family'),('RTW88Backend.inc','is88Family'),('RTW8192FUBackend.inc','is8192FUFamily'),('RTW8710BUBackend.inc','is8710BUFamily'),('RTW8723BUBackend.inc','is8723Family')]:
    selectors+=extract_member((project/'kext'/filename).read_text(),r'^bool RTWUSBController::'+name+r'\(').replace('RTWUSBController::','Seed::')

# Execute the real attach/start choices with isolated hardware boundaries.
initialize=extract_member(controller,r'^IOReturn RTWUSBController::initializePlatform\(')
enable=extract_member(controller,r'^IOReturn RTWUSBController::enableStation\(')
attach_dispatch=re.search(r'int result=[^;]+;',initialize).group().replace('rtwn_attach(&device_,&core_,this)','legacyDispatch()')
start_dispatch=re.search(r'int result=[^;]+;',enable).group().replace('rtwn_init(&core_.sc_ic.ic_if)','legacyDispatch()')
closure='''#include "rtwn_ht_host.hpp"
#include "rtw/USBWiFiProfiles.h"
#define RTWN_CHIP_USB 0x80000000U
#define kIOReturnUnsupported -1
class Seed { public:bool is8812Family()const;bool is88Family()const;bool is8192FUFamily()const;bool is8710BUFamily()const;bool is8723Family()const;rtwn_softc core_;rtw_wifi_family family_=RTW_WIFI_UNKNOWN;
rtw_wifi_family dispatched=RTW_WIFI_UNKNOWN;int expected=EIO;
int dispatch(rtw_wifi_family f){dispatched=f;return expected;}
int legacyDispatch(){return dispatch(RTW_WIFI_RTL8192CU);}
int attach8723(){return dispatch(RTW_WIFI_RTL8723BU);}int attach8710BU(){return dispatch(RTW_WIFI_RTL8710BU);}int attach8192FU(){return dispatch(RTW_WIFI_RTL8192FU);}int attach8192DU(){return dispatch(RTW_WIFI_RTL8192DU);}int attach8812(){return dispatch(RTW_WIFI_RTL8812AU);}int attach8814(){return dispatch(RTW_WIFI_RTL8814AU);}int attach88(){return dispatch(RTW_WIFI_RTL8822BU);}
int start8723(){return dispatch(RTW_WIFI_RTL8723BU);}int start8710BU(){return dispatch(RTW_WIFI_RTL8710BU);}int start8192FU(){return dispatch(RTW_WIFI_RTL8192FU);}int start8192DU(){return dispatch(RTW_WIFI_RTL8192DU);}int start8812(){return dispatch(RTW_WIFI_RTL8812AU);}int start8814(){return dispatch(RTW_WIFI_RTL8814AU);}int start88(){return dispatch(RTW_WIFI_RTL8822BU);}
int attach(){'''+attach_dispatch+'''return result;}int start(){'''+start_dispatch+'''return result;}int select(){
'''+seed+''' return 0;}};
'''+selectors+extract(source,'rtwn_read_chipid')+'''
int main(){
 for(auto family:{RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8723BU,RTW_WIFI_RTL8710BU})for(int result:{0,EIO,ENXIO,EINVAL}){Seed s;s.family_=family;s.expected=result;require(s.attach()==result&&s.dispatched==family,"typed attach route preserves result");s.dispatched=RTW_WIFI_UNKNOWN;require(s.start()==result&&s.dispatched==family,"typed start route preserves result");}
 for(uint32_t cfg:{0U,uint32_t(R92C_SYS_CFG_TYPE_92C),uint32_t(R92C_SYS_CFG_VENDOR_UMC)}){
  Seed s;s.family_=RTW_WIFI_RTL8192CU;require(s.select()==0,"CU family selected");
  rtwn_write_4(&s.core_,R92C_SYS_CFG,cfg);
  require(rtwn_read_chipid(&s.core_)==0,"controller CU seed is accepted by actual core chip identification");
  require((s.core_.chip&(RTWN_CHIP_92C|RTWN_CHIP_88C))==(cfg&R92C_SYS_CFG_TYPE_92C?RTWN_CHIP_92C:RTWN_CHIP_88C),"CU silicon flags refined from register");
  require(!(s.core_.sc_flags&RTWN_FLAG_EXT_HDR),"CU uses ordinary eFuse headers");
 }
 for(auto f:{RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU}){Seed s;s.family_=f;require(s.select()==0,"EU family selected");require(rtwn_read_chipid(&s.core_)==0,"EU core identify");require(s.core_.sc_flags&RTWN_FLAG_EXT_HDR,"EU extended headers");}
 for(auto f:{RTW_WIFI_RTL8192DU,RTW_WIFI_RTL8192FU,RTW_WIFI_RTL8710BU,RTW_WIFI_RTL8723BU,RTW_WIFI_RTL8812AU,RTW_WIFI_RTL8821AU,RTW_WIFI_RTL8814AU,RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
  Seed s;s.family_=f;require(s.select()==0,"typed family seed selected");
  require(s.core_.chip==RTWN_CHIP_USB,"new family has no borrowed legacy silicon identity");
 }
 for(auto f:{RTW_WIFI_UNKNOWN,RTW_WIFI_RTL8814BU}){Seed s;s.family_=f;require(s.select()!=0,"unimplemented families refused before programming");}

 for(auto f:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU}){
  Seed s;s.family_=f;require(s.select()==0,"family selected before fault injection");
  s.core_.io_error=EIO;
  require(rtwn_read_chipid(&s.core_)==EIO,"pre-existing USB error is preserved");
  require(s.core_.read4_count==0,"no register reads after known USB failure");
  s.core_.io_error=0;s.core_.read4_fail_reg=R92C_SYS_CFG;
  const auto before=s.core_.chip;
  require(rtwn_read_chipid(&s.core_)==EIO,"failed SYS_CFG read cannot select a silicon variant");
  require(s.core_.read4_count==1,"chip identification stops at failed first read");
  require(s.core_.chip==before,"failed SYS_CFG leaves family seed unchanged");
 }
 for(auto f:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8192EU}){
  Seed s;s.family_=f;require(s.select()==0,"bonding test family selected");
  rtwn_write_4(&s.core_,R92C_SYS_CFG,R92C_SYS_CFG_TYPE_92C);
  s.core_.read4_fail_reg=R92C_HPON_FSM;
  require(rtwn_read_chipid(&s.core_)==EIO,"failed chain-bonding read is not a successful attach");
  require(s.core_.read4_count==2,"bonding failure stops at second read");
 }
 for(auto f:{RTW_WIFI_RTL8192CU,RTW_WIFI_RTL8188EU,RTW_WIFI_RTL8192EU}){
  Seed s;s.family_=f;require(s.select()==0,"test silicon family selected");
  rtwn_write_4(&s.core_,R92C_SYS_CFG,UINT32_MAX);
  require(rtwn_read_chipid(&s.core_)!=0,"all-ones/test silicon register refused");
 }
 {
  Seed s;s.family_=RTW_WIFI_RTL8188EU;require(s.select()==0,"I-cut family selected");
  rtwn_write_4(&s.core_,R92C_SYS_CFG,8U<<R92C_SYS_CFG_CHIP_VER_RTL_S);
  require(rtwn_read_chipid(&s.core_)!=0,"unsupported 8188EU I-cut not enabled by broad ID");
 }
 puts("Actual controller family selection and chip identification passed");
}
'''
generated=args.out/'actual_family_seed.cpp';generated.write_text(closure)
subprocess.run(_host_cxx + ['-std=c++17','-g','-O1',*_host_sanitizers,'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(project),'-I'+str(project/'include'),'-I'+str(project/'tests'),str(generated),'-o',str(args.out/'family-tests')],check=True)
subprocess.run([str(args.out/'family-tests')],check=True)
