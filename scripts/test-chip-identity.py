#!/usr/bin/env python3
"""Actual read-only USB resolver/Boot constructor guards; no device access."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import subprocess, tempfile
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rtw-chip-identity-') as td:
 d=Path(td);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include')];objects=[]
 for name in ['ChipIdentity','USBWiFiProfiles','RTW88Boot']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 for name in ['chip_identity_tests.c','chip_identity_boot_tests.cpp']:
  cpp=name.endswith('.cpp');exe=d/name
  subprocess.run([*compiler('c++' if cpp else 'c'),'-std=c++17' if cpp else '-std=c11',*flags,str(r/'tests'/name),*objects,'-o',str(exe)],check=True)
  subprocess.run([str(exe)],check=True)
