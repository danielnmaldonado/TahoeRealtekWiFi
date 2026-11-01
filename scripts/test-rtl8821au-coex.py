#!/usr/bin/env python3
"""Compile the actual8821A coex policy against a bounded register model."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='r21au-coex-') as t:
 d=Path(t);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
 for name in ['RTL8821AUCoex','RTL8812AUBootstrap','RTL8812AUPHY','USBRegisterIO','USBControl','Efuse']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 prefix=(r/'tests/rtl8812au_phy_tests.cpp').read_text().split('static void success(){',1)[0];source=d/'tests.cpp';source.write_text(prefix+(r/'tests/rtl8821au_coex_tests.cpp').read_text());out=(a.out or d/'tests').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(source),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
 mp=d/'mp-tests';subprocess.run([*compiler('c'),'-std=c11','-DCOEX_MP_AU',*flags,str(r/'tests/coex_mp_tests.c'),*objects[1:],'-o',str(mp)],check=True);subprocess.run([str(mp)],check=True)
