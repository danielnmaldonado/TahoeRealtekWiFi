#!/usr/bin/env python3
"""Compile complete FU adapter against the real macOS kernel/IOUSB headers.

Adds only declaration/include overlays to a temporary controller copy when the
parent controller has not yet wired these two includes. Never edits controller
sources, builds/loads a kext, contacts a USB device or installs anything.
"""
from pathlib import Path
import argparse,hashlib,json,subprocess,sys,tempfile,re
r=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path);a=p.parse_args()
if sys.platform!='darwin':raise SystemExit('The native FU adapter check requires macOS and the real MacKernelSDK.')
if not (r/'MacKernelSDK/Headers/IOKit/usb/IOUSBHostPipe.h').exists():raise SystemExit('Missing real MacKernelSDK headers; run the project SDK setup.')
with tempfile.TemporaryDirectory(prefix='rtw-fu-kdk-') as temp:
 d=Path(temp);header=(r/'kext/RTWUSBController.hpp').read_text();source=(r/'kext/RTWUSBController.cpp').read_text()
 header='#include "rtw/RTL8192FU.h"\n'+header
 if 'RTW8192FUBackendMembers.inc' not in header:
  at=header.rfind('\n};');assert at>=0;header=header[:at]+'\n#include "'+str(r/'kext/RTW8192FUBackendMembers.inc')+'"\n'+header[at:]
 if 'RTW8192FUBackend.inc' not in source:source+='\n#include "'+str(r/'kext/RTW8192FUBackend.inc')+'"\n'
 (d/'RTWUSBController.hpp').write_text(header);(d/'RTWUSBController.cpp').write_text(source)
 output=(a.out or d/'controller-fu.o').resolve();output.parent.mkdir(parents=True,exist_ok=True)
 command=['xcrun','clang++','-O2','-c','-std=gnu++14','-target','x86_64-apple-macos11.0',
  '-DKERNEL=1','-DKERNEL_PRIVATE=1','-DDRIVER_PRIVATE=1','-DIEEE80211_STA_ONLY=1','-D__PRIVATE_SPI__=1',
  '-fno-builtin','-fno-exceptions','-fno-rtti','-fapple-kext','-fno-stack-protector',
  '-Wno-deprecated-register','-Wno-c++17-extensions','-Wno-macro-redefined','-Wno-format','-Werror=return-type',
  '-include',str(r/'vendor/itl80211/RTWNet80211Prefix.hpp')]
 for include in ['MacKernelSDK/Headers','vendor/itl80211/openbsd','vendor/itl80211/linux','vendor/itl80211','include','kext','vendor/rtwn']:command+=['-I',str(r/include)]
 result=subprocess.run([*command,str(d/'RTWUSBController.cpp'),'-o',str(output)],capture_output=True,text=True)
 if a.out:output.with_suffix('.log').write_text(result.stdout+result.stderr)
 if result.returncode:
  print(result.stdout+result.stderr,file=sys.stderr);raise SystemExit(result.returncode)
 own_warnings=re.findall(r'[^\n]*RTW8192FUBackend(?:Members)?\.inc:[^\n]*warning:[^\n]*',result.stderr)
 if own_warnings:raise SystemExit('FU adapter warnings:\n'+'\n'.join(own_warnings))
 print('Real KDK/IOUSB complete FU controller compiled; zero FU adapter warnings (no hardware operations).')
 if a.out:
  manifest={str(f.relative_to(r)):hashlib.sha256(f.read_bytes()).hexdigest() for f in [r/'kext/RTW8192FUBackend.inc',r/'kext/RTW8192FUBackendMembers.inc',r/'kext/RTWUSBController.cpp',r/'kext/RTWUSBController.hpp']}
  output.with_suffix('.sources.json').write_text(json.dumps(manifest,indent=2)+'\n')
