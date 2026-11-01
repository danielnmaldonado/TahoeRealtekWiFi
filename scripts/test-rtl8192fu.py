#!/usr/bin/env python3
"""Actual dedicated FU source over deterministic USB/RF model; no hardware IO."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,tempfile,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);p.add_argument('--driver-source',type=Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='r92fu-') as temp:
 d=Path(temp);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'src')];objs=[]
 for n in ['RTL8192FU','RTL8192FUDataPath','Efuse','USBControl','USBRegisterIO']:
  obj=d/(n+'.o');objs.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(a.driver_source if n=='RTL8192FU' and a.driver_source else r/'src'/(n+'.c')),'-o',str(obj)],check=True)
 out=(a.out or d/'tests').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(r/'tests/rtl8192fu_tests.cpp'),*objs,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
 wire=d/'wire-tests';subprocess.run([*compiler(),'-std=c++17',*flags,str(r/'tests/rtl8192fu_datapath_tests.cpp'),*objs,'-o',str(wire)],check=True);subprocess.run([str(wire)],check=True)

