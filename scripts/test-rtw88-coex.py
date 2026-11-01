#!/usr/bin/env python3
"""Execute actual B/C callbacks and shared policy; no hardware operations."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='rtw88-coex-') as t:
 d=Path(t);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
 for name in ['RTW88Coex','RTW88Firmware','RTW88USBDataPath']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 out=(a.out or d/'tests').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(r/'tests/rtw88_coex_tests.cpp'),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
 mp=d/'mp-tests';subprocess.run([*compiler('c'),'-std=c11',*flags,str(r/'tests/coex_mp_tests.c'),*objects[1:],'-o',str(mp)],check=True);subprocess.run([str(mp)],check=True)
