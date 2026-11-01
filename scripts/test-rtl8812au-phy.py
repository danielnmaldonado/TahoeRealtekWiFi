#!/usr/bin/env python3
"""Compile real8812/8821 PHY functions against a paged register/RF USB model."""
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
import argparse,subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,default=root/'out-rtl8812au-phy');args=p.parse_args();args.out.mkdir(parents=True,exist_ok=True)
flags=['-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(root/'include')]
objects=[]
for name in ['RTL8812AUPHY','USBRegisterIO','USBControl']:
 obj=args.out/(name+'.o');objects.append(str(obj))
 subprocess.run([*compiler('c'),'-std=c11',*flags,'-Wpedantic','-Wconversion','-Wshadow','-c',str(root/'src'/(name+'.c')),'-o',str(obj)],check=True)
subprocess.run([*compiler(),'-std=c++17',*flags,str(root/'tests/rtl8812au_phy_tests.cpp'),*objects,'-o',str(args.out/'tests')],check=True)
subprocess.run([str(args.out/'tests')],check=True)
