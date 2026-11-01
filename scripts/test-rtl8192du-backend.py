#!/usr/bin/env python3
"""Run the actual DU kernel state methods against the real C DU HAL with a
fake USB boundary. No USB, installation or running kext is accessed."""
from pathlib import Path
import argparse, subprocess, tempfile
from host_cpp import compiler, sanitizer_flags
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--library',type=Path,required=True);p.add_argument('--out',type=Path);args=p.parse_args()
f=root/'kext/RTW8192DUBackend.inc'
assert f.exists(), 'missing concrete RTL8192DU kernel station adapter'
s=f.read_text()
def function(marker):
    start=s.index(marker);brace=s.index('{',start);depth=1;end=brace+1
    while depth:
        if s[end]=='{':depth+=1
        elif s[end]=='}':depth-=1
        end+=1
    return s[start:end]
markers=['bool RTWUSBController::claim8192DUDevice(', 'bool RTWUSBController::is8192DUUSBProvider(', 'void RTWUSBController::release8192DUStorage(', 'void RTWUSBController::close8192DUProviders(', 'static int rtw8192DUError(', 'static uint16_t rtw8192DURateMask(',
 'int RTWUSBController::prepare8192DUPorts(', 'int RTWUSBController::select8192DUPort(', 'int RTWUSBController::switch8192DUBand(',
 'rtw_92du_bss RTWUSBController::station8192DUParams(', 'int RTWUSBController::set8192DUChannel(', 'int RTWUSBController::set8192DUPowerReduction(', 'int RTWUSBController::set8192DUPowerLimitMBM(',
 'void RTWUSBController::stop8192DU(', 'int RTWUSBController::state8192DU(', 'void RTWUSBController::update8192DUEDCA(',
 'bool RTWUSBController::permits8192DUChannel(', 'bool RTWUSBController::supports8192DUBand(', 'bool RTWUSBController::uses8192DUFirmwareRate(',
 'int RTWUSBController::tick8192DU(', 'rtw_packet_error RTWUSBController::decode8192DUUSBFrame(',
 'int RTWUSBController::sample8192DURSSI(', 'int RTWUSBController::send8192DUFrame(']
with tempfile.TemporaryDirectory(prefix='rtw-du-backend-') as tmp:
    d=Path(tmp);source=d/'backend.cpp';source.write_text((root/'tests/rtl8192du_backend_harness.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(function(m) for m in markers)))
    output=args.out or d/'backend-tests';output.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run([*compiler(),'-std=c++17','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I',str(root/'include'),'-I',str(root/'tests'),str(source),str(args.library.resolve()),'-o',str(output)],check=True)
    subprocess.run([str(output.resolve())],check=True)
