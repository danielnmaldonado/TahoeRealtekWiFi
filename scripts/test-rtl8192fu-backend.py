#!/usr/bin/env python3
"""Actual FU controller methods and real portable FU driver over modeled USB."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,tempfile,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args();s=(r/'kext/RTW8192FUBackend.inc').read_text()
def method(marker):
 start=s.index(marker);brace=s.index('{',start);depth=1;end=brace+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
markers=['static uint16_t rtw8192FURateMask(','rtw_usb_error RTWUSBController::fuControlIn(','rtw_usb_error RTWUSBController::fuControlOut(','int RTWUSBController::fuWait(','bool RTWUSBController::is8192FUFamily(','bool RTWUSBController::supports8192FUBand(','int RTWUSBController::initialize8192FUCore(','uint32_t RTWUSBController::channel8192FUModes(','void RTWUSBController::refresh8192FUModes(','int RTWUSBController::start8192FU(','void RTWUSBController::stop8192FU(','uint16_t RTWUSBController::station8192FURates(','int RTWUSBController::set8192FUOperating(','int RTWUSBController::set8192FUChannel(','int RTWUSBController::peer8192FUOperating(','int RTWUSBController::station8192FUHardware(','int RTWUSBController::state8192FU(','void RTWUSBController::update8192FUEDCA(','bool RTWUSBController::uses8192FUFirmwareRate(','bool RTWUSBController::uses8192FUHighThroughput(','bool RTWUSBController::accepts8192FUReceiveRate(','int RTWUSBController::ampdu8192FUTXStart(','int RTWUSBController::tick8192FU(','int RTWUSBController::sample8192FURSSI(','void RTWUSBController::handle8192FUC2H(','rtw_packet_error RTWUSBController::decode8192FUUSBFrame(','int RTWUSBController::send8192FUFrame(']
with tempfile.TemporaryDirectory(prefix='r92fu-backend-') as temp:
 d=Path(temp);h=(r/'tests/rtl8192fu_backend_harness.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(map(method,markers))).replace('#include "../kext/RTW8192FUBackendMembers.inc"','#include "'+str(r/'kext/RTW8192FUBackendMembers.inc')+'"');source=d/'backend.cpp';source.write_text(h);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')];objects=[]
 for name in ['RTL8192FU','RTL8192FUDataPath','Efuse','USBControl','USBRegisterIO','USBPackets']:
  o=d/(name+'.o');objects.append(str(o));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(o)],check=True)
 out=(a.out or d/'test').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(source),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
