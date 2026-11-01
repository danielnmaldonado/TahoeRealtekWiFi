#!/usr/bin/env python3
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
import argparse,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=r/'out-rtl8812au-runtime');a=p.parse_args();a.out.mkdir(parents=True,exist_ok=True)
f=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
for n in ['RTL8812AURuntime','RTL8812AUDataPath','RTL8812AUBootstrap','RTL8812AUPHY','Efuse','USBControl','USBRegisterIO']:
 o=a.out/(n+'.o');objects.append(str(o));subprocess.run([*compiler('c'),'-std=c11',*f,'-c',str(r/'src'/(n+'.c')),'-o',str(o)],check=True)
subprocess.run([*compiler(),'-std=c++17',*f,str(r/'tests/rtl8812au_runtime_tests.cpp'),*objects,'-o',str(a.out/'tests')],check=True);subprocess.run([str(a.out/'tests')],check=True)
