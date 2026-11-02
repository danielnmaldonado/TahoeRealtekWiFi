#!/usr/bin/env python3
"""Shared production station recovery lifecycle over isolated hardware boundaries."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
import argparse,subprocess,tempfile
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--baseline',action='store_true');a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='family-recovery-') as t:
 out=Path(t)/'test';subprocess.run([*compiler(),'-std=c++17','-g','-O1','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),*(['-DRTW_RECOVERY_BASELINE'] if a.baseline else []),str(r/'tests/family_recovery_tests.cpp'),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
