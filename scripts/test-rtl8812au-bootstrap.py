#!/usr/bin/env python3
"""Real AU OTP,bootstrap,8051 firmware,LLT and MAC functions over modeledUSB."""
from host_cpp import compiler,sanitizer_flags
from pathlib import Path
import argparse,subprocess
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--firmware-8812',type=Path);p.add_argument('--firmware-8821',type=Path);p.add_argument('--out',type=Path,default=root/'out-rtl8812au-bootstrap');args=p.parse_args();args.out.mkdir(parents=True,exist_ok=True)
flags=['-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(root/'include')]
objects=[]
for name in ['RTL8812AUBootstrap','RTL8812AUMAC','RTL8812AUPHY','Efuse','USBRegisterIO','USBControl']:
 obj=args.out/(name+'.o');objects.append(str(obj))
 subprocess.run([*compiler('c'),'-std=c11',*flags,'-Wpedantic','-Wconversion','-Wshadow','-c',str(root/'src'/(name+'.c')),'-o',str(obj)],check=True)
subprocess.run([*compiler(),'-std=c++17',*flags,str(root/'tests/rtl8812au_bootstrap_tests.cpp'),*objects,'-o',str(args.out/'tests')],check=True)
extra=[]
if args.firmware_8812 or args.firmware_8821:
 if not (args.firmware_8812 and args.firmware_8821):p.error('Both firmware paths are required together')
 extra=[str(args.firmware_8812),str(args.firmware_8821)]
subprocess.run([str(args.out/'tests'),*extra],check=True)
