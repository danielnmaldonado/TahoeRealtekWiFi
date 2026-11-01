#!/usr/bin/env python3
"""Compile actual native8814 channel and state methods, not geometry replicas."""
import argparse
from pathlib import Path
import runpy
import subprocess
import tempfile
from host_cpp import compiler,sanitizer_flags

def main():
    root=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    extract=runpy.run_path(str(root/'scripts/test-rtl8814au-attach.py'))['function']
    source=(root/'kext/RTW8814Backend.inc').read_text()
    actual='\n'.join(extract(source,'int RTWUSBController::'+name+'(') for name in ['set8814Channel','state8814'])
    fixture=(root/'tests/rtl8814au_channel_tests.cpp').read_text().replace('/* ACTUAL_BACKEND_METHODS */',actual)
    with tempfile.TemporaryDirectory(prefix='rtw8814-channel-') as directory:
        out=args.out or Path(directory)
        out.mkdir(parents=True,exist_ok=True)
        cpp=out/'actual_channel.cpp';cpp.write_text(fixture)
        binary=out/'channel-tests'
        subprocess.run([*compiler(),'-std=c++17','-Wall','-Wextra','-Werror','-g','-O1',*sanitizer_flags(),'-I'+str(root/'include'),str(cpp),'-x','c++',str(root/'src/DFSRegulatory.c'),'-o',str(binary)],check=True)
        subprocess.run([str(binary.resolve())],check=True)

if __name__=='__main__':main()
