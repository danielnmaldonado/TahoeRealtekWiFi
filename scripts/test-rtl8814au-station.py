#!/usr/bin/env python3
"""Compile actual station register algorithms with bounded mock USB I/O."""
from host_cpp import compiler, sanitizer_flags
import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rtw-8814-station-') as tmp:
    objects=[]
    common=['-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-Wall','-Wextra','-Werror','-I'+str(root/'include')]
    for name in ('RTL8814AUBoot','RTL8814AUStation'):
        obj=str(pathlib.Path(tmp)/(name+'.o'));objects.append(obj)
        subprocess.run([*compiler("c"),'-std=c11',*common,'-c',str(root/'src'/str(name+'.c')),'-o',obj],check=True)
    binary=str(pathlib.Path(tmp)/'station')
    subprocess.run([*compiler(),'-std=c++17',*common,str(root/'tests/rtl8814au_station_tests.cpp'),*objects,'-o',binary],check=True)
    subprocess.run([binary],check=True)
