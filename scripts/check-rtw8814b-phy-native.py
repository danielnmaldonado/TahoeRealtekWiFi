#!/usr/bin/env python3
"""Compile actual B14 PHY/RFK with kernel headers; never accesses hardware."""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rtwb14-kernel-') as t:
 subprocess.run(['xcrun','clang','-std=gnu11','-arch','x86_64','-mmacosx-version-min=11.0','-fno-builtin','-fno-stack-protector','-nostdinc','-DKERNEL=1','-DKERNEL_PRIVATE=1','-DDRIVER_PRIVATE=1','-I'+str(r/'MacKernelSDK/Headers'),'-I'+str(r/'include'),'-Werror=return-type','-Werror=frame-larger-than','-Wframe-larger-than=2048','-Wno-parentheses-equality','-Os','-c',str(r/'src/RTW8814BPHY.c'),'-o',str(Path(t)/'phy.o')],check=True)
 print('B14 actual PHY/RFK: native kernel compilation passed; 2048-byte frame limit')
