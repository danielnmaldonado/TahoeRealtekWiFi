#!/usr/bin/env python3
"""RTL8723BU real gen2 wire codecs, no hardware or mocked codec implementation."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,subprocess
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=r/'out-rtl8723bu-datapath');a=p.parse_args();a.out.mkdir(exist_ok=True,parents=True)
flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
for name in ['RTL8723BUDataPath','USBPackets']:
 obj=a.out/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
out=a.out/'rtl8723bu-datapath-tests';subprocess.run([*compiler(),'-std=c++17',*flags,str(r/'tests/rtl8723bu_datapath_tests.cpp'),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
