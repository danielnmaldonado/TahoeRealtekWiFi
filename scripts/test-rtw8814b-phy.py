#!/usr/bin/env python3
"""Actual pinned B14 algorithms with an isolated, bounded HAL/DSP boundary."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rtwb14-phy-') as t:
 d=Path(t);flags=['-O1','-g',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'tests')]
 subprocess.run([*compiler('c'),'-std=c11',*flags,'-Wno-parentheses-equality','-c',str(r/'src/RTW8814BPHY.c'),'-o',str(d/'phy.o')],check=True)
 for suite in ['rtw8814b_phy_tests.cpp','rtw8814b_phy_failure_tests.cpp']:
  subprocess.run([*compiler(),'-std=c++17',*flags,str(r/'tests'/suite),str(d/'phy.o'),'-o',str(d/'tests')],check=True)
  subprocess.run([str(d/'tests')],check=True)
