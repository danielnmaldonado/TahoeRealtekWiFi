#!/usr/bin/env python3
"""Actual RTL8723BU core; only hardware bus callbacks are mocked."""
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
import argparse,subprocess
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--out',type=Path,default=root/'out-rtl8723bu-core');args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
flags=['-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(root/'include')]
objects=[]
for name in ['RTL8723BU']:
 obj=args.out/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(root/'src'/(name+'.c')),'-o',str(obj)],check=True)
binary=args.out/'rtl8723bu-core-tests';subprocess.run([*compiler(),'-std=c++17',*flags,str(root/'tests/rtl8723bu_core_tests.cpp'),*objects,'-o',str(binary)],check=True);subprocess.run([str(binary)],check=True)
