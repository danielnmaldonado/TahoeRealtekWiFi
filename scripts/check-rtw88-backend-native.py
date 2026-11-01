#!/usr/bin/env python3
"""Compile the complete B/C controller backend against the real MacKernelSDK.
Uses a temporary shadow header until parent controller hooks are integrated.
Never opens USB or loads a kext.
"""
from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rtw88-kernel-') as tmp:
 d=Path(tmp);header=(r/'kext/RTWUSBController.hpp').read_text()
 headers=['RTW88PHYOperating','RTW88Station','RTW88RX','RTW88USBDataPath']
 for name in headers:
  include='#include "rtw/'+name+'.h"'
  if include not in header:header=include+'\n'+header
 if 'RTW88BackendMembers.inc' not in header:
  at=header.rfind('\n};');assert at>=0;header=header[:at]+'\n#include "RTW88BackendMembers.inc"'+header[at:]
 (d/'RTWUSBController.hpp').write_text(header);(d/'backend.cpp').write_text('#include "RTWUSBController.hpp"\n#include "RTW88Backend.inc"\n')
 includes=[d,r/'MacKernelSDK/Headers',r/'vendor/itl80211/openbsd',r/'vendor/itl80211/linux',r/'vendor/itl80211',r/'include',r/'kext',r/'vendor/rtwn']
 cmd=['xcrun','clang++','-std=gnu++14','-arch','x86_64','-mmacosx-version-min=11.0','-fno-builtin','-fno-exceptions','-fno-rtti','-fno-stack-protector','-nostdinc','-DKERNEL=1','-DKERNEL_PRIVATE=1','-DDRIVER_PRIVATE=1','-DIEEE80211_STA_ONLY=1','-D__PRIVATE_SPI__=1',*['-I'+str(p) for p in includes],'-Wno-deprecated-register','-Wno-c++17-extensions','-Wno-macro-redefined','-Wno-format','-Werror=return-type','-Werror=frame-larger-than','-Wframe-larger-than=2048','-Os','-c',str(d/'backend.cpp'),'-o',str(d/'backend.o')]
 subprocess.run(cmd,cwd=r,check=True)
 print('Complete B/C controller backend: native kernel compilation passed,2048-byte stack-frame limit')
