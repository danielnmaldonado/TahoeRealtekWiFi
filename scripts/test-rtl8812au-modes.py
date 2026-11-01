#!/usr/bin/env python3
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='r12au-modes-') as temp:
 d=Path(temp);prefix=(r/'tests/rtl8812au_phy_tests.cpp').read_text().split('static void success(){',1)[0]
 source=d/'modes.cpp';source.write_text(prefix+(r/'tests/rtl8812au_modes_tests.cpp').read_text());flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')];objects=[]
 for n in ['RTL8812AUPHY','USBControl','USBRegisterIO']:
  o=d/(n+'.o');objects.append(str(o));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(n+'.c')),'-o',str(o)],check=True)
 out=(a.out or d/'tests').resolve();out.parent.mkdir(parents=True,exist_ok=True);subprocess.run([*compiler(),'-std=c++17',*flags,str(source),*objects,'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
