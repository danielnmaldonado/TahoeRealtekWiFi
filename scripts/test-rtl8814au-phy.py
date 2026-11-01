#!/usr/bin/env python3
"""Actual portable PHY and core functions, with only register/bulk callbacks faked."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse,subprocess
p=Path(__file__).resolve().parents[1]
a=argparse.ArgumentParser();a.add_argument('--out',type=Path,default=p/'out-rtl8814au-phy');v=a.parse_args();v.out.mkdir(parents=True,exist_ok=True)
flags=['-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(p/'include')]
objects=[]
for name in ['RTL8814AUBoot','RTL8814AUEfuse','RTL8814AUMAC','RTL8814AUPHY']:
    obj=v.out/(name+'.o');objects.append(str(obj))
    subprocess.run([*compiler("c"),'-std=c11',*flags,'-c',str(p/'src'/(name+'.c')),'-o',str(obj)],check=True)
subprocess.run([*compiler(),'-std=c++17',*flags,str(p/'tests/rtl8814au_phy_tests.cpp'),*objects,'-o',str(v.out/'tests')],check=True)
subprocess.run([str(v.out/'tests')],check=True)
