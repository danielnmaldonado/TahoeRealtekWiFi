#!/usr/bin/env python3
"""Actual8710BU controller methods and real portable8710BU driver over modeled USB."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,tempfile,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args();s=(r/'kext/RTW8710BUBackend.inc').read_text()
def method(marker):
 start=s.index(marker);brace=s.index('{',start);depth=1;end=brace+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
markers=['static uint16_t rtw8710BURateMask(','rtw_usb_error RTWUSBController::buControlIn(','rtw_usb_error RTWUSBController::buControlOut(','int RTWUSBController::buWait(','bool RTWUSBController::is8710BUFamily(','bool RTWUSBController::supports8710BUBand(','int RTWUSBController::initialize8710BUCore(','uint32_t RTWUSBController::channel8710BUModes(','void RTWUSBController::refresh8710BUModes(','int RTWUSBController::start8710BU(','void RTWUSBController::stop8710BU(','uint16_t RTWUSBController::station8710BURates(','int RTWUSBController::set8710BUOperating(','int RTWUSBController::set8710BUChannel(','int RTWUSBController::peer8710BUOperating(','int RTWUSBController::station8710BUHardware(','int RTWUSBController::state8710BU(','void RTWUSBController::update8710BUEDCA(','bool RTWUSBController::uses8710BUFirmwareRate(','bool RTWUSBController::uses8710BUHighThroughput(','bool RTWUSBController::accepts8710BUReceiveRate(','int RTWUSBController::ampdu8710BUTXStart(','int RTWUSBController::tick8710BU(','int RTWUSBController::sample8710BURSSI(','void RTWUSBController::handle8710BUC2H(','rtw_packet_error RTWUSBController::decode8710BUUSBFrame(','int RTWUSBController::send8710BUFrame(']
with tempfile.TemporaryDirectory(prefix='r10bu-backend-') as temp:
 d=Path(temp);h=(r/'tests/rtl8710bu_backend_harness.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(map(method,markers))).replace('#include "../kext/RTW8710BUBackendMembers.inc"','#include "'+str(r/'kext/RTW8710BUBackendMembers.inc')+'"');source=d/'backend.cpp';source.write_text(h);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')];objects=[]
 for name in ['RTL8710BU','RTL8710BUDataPath','Efuse','USBControl','USBRegisterIO','USBPackets']:
  o=d/(name+'.o');objects.append(str(o));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(o)],check=True)
 out=(a.out or d/'test').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(source),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
