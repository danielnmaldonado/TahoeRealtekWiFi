#!/usr/bin/env python3
"""Exercise actual 8814 net80211 state methods with isolated host boundaries."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse
import subprocess
import tempfile
p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1]
f=r/'kext/RTW8814Backend.inc'
assert f.exists(), 'missing concrete RTL8814AU net80211 backend'
s=f.read_text()
def function(marker):
    start=s.index(marker);brace=s.index('{',start);depth=1;end=brace+1
    while depth:
        if s[end]=='{':depth+=1
        elif s[end]=='}':depth-=1
        end+=1
    return s[start:end]
units=[function('static uint16_t rtw8814RateMask('),
       function('rtw_8814au_station_params RTWUSBController::station8814Params()'),
       function('int RTWUSBController::set8814Channel('),
       function('int RTWUSBController::state8814(')]
with tempfile.TemporaryDirectory(prefix='rtw8814-backend-') as d:
    source=Path(d)/'backend.cpp';source.write_text((r/'tests/rtl8814au_backend_harness.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */','\n'.join(units)))
    output=a.out or Path(d)/'backend-tests';output.parent.mkdir(parents=True,exist_ok=True)
    subprocess.run([*compiler(),'-std=c++17','-x','c++','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I',str(r/'include'),str(source),str(r/'src/RTL8814AUDataPath.c'),str(r/'src/USBPackets.c'),'-o',str(output)],check=True)
    subprocess.run([str(output.resolve())],check=True)
