#!/usr/bin/env python3
"""Run actual controller bootstrap with actual portable Boot/FW/MAC/efuse/feature code."""
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
import argparse,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--out',type=Path)
parser.add_argument('--omit-request',action='store_true',help='Replay the missing-request regression in temporary source')
parser.add_argument('--late-request',action='store_true',help='Replay a request after CPU boot in temporary source')
args=parser.parse_args();source=(root/'kext/RTW88Backend.inc').read_text()+'\n'+(root/'kext/RTWUSBController.cpp').read_text()
def method(marker):
 start=source.index(marker);opening=source.index('{',start);depth=1;end=opening+1
 while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
 return source[start:end]
methods='\n'.join(method(marker) for marker in ['void RTWUSBController::clear88Discovery(','int RTWUSBController::prepare88Identity(','int RTWUSBController::recheck88Identity(','int RTWUSBController::publish88Identity(','bool RTWUSBController::is88Family(','int RTWUSBController::bootstrap88('])
request='e=r88Write(this,0x1a0,1,0xfd);if(e)return e;'
if args.omit_request or args.late_request:methods=methods.replace(request,'')
if args.late_request:methods=methods.replace('e=rtw88_boot_read_board(',request+'\n e=rtw88_boot_read_board(')
bus=(root/'tests/rtw88_boot_tests.cpp').read_text().split('static void ready(')[0]
text=(root/'tests/rtw88_bootstrap_harness.cpp').read_text().replace('/* ACTUAL_BOOT_BUS_MODEL */',bus).replace('/* ACTUAL_BOOTSTRAP_METHODS */',methods)
with tempfile.TemporaryDirectory(prefix='rtw88-bootstrap-') as tmp:
 out=args.out or Path(tmp);out.mkdir(parents=True,exist_ok=True);src=out/'bootstrap.cpp';src.write_text(text)
 flags=['-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(root/'include'),'-I'+str(root/'kext')]
 objects=[]
 for name in ['ChipIdentity','RTW88Boot','RTW88Firmware','RTW88MAC','RTW88USBDataPath','USBWiFiProfiles']:
  obj=out/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(root/'src'/(name+'.c')),'-o',str(obj)],check=True)
 binary=out/'rtw88-bootstrap-tests';subprocess.run([*compiler(),'-std=c++17',*flags,str(src),*objects,'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
