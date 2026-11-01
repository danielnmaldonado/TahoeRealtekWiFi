#!/usr/bin/env python3
"""Compile actual isolated B14 HALMAC+typed boundary against a bounded fake bus.
No hardware access. --sanitize runs AddressSanitizer and UndefinedBehaviorSanitizer.
"""
from pathlib import Path
import argparse, json, subprocess, tempfile, concurrent.futures, hashlib
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--sanitize',action='store_true');a=p.parse_args()
r=Path(__file__).resolve().parents[1];proof=json.loads((r/'src/RTW8814BHAL-Upstream.json').read_text());v=r/'src/rtl8814b/halmac'
for f in proof['files']:
 if hashlib.sha256((v/f['path']).read_bytes()).hexdigest()!=f['output_sha256']:raise SystemExit('Generated source mismatch: '+f['path'])
flags=['-O1','-g','-I'+str(r/'include'),'-I'+str(v),'-Wno-unused-value','-Wno-pointer-sign']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
sources=[v/f for f in proof['sources']]+[r/'src/RTW8814BHAL.c',r/'src/RTW8814BUSBCodec.c',r/'tests/rtl8814b_codec_goldens.c']
with tempfile.TemporaryDirectory(prefix='rtwb14-hal-') as td:
 out=Path(td)
 def compile(pair):
  i,f=pair;obj=out/(str(i)+'.o');res=subprocess.run(['xcrun','clang','-std=gnu11',*flags,'-c',str(f),'-o',str(obj)],capture_output=True,text=True)
  if res.returncode:raise RuntimeError(str(f)+'\n'+res.stderr)
  return obj
 with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:objects=list(pool.map(compile,enumerate(sources)))
 exe=out/'rtl8814b-hal';subprocess.run(['xcrun','clang++','-std=c++17',*flags,str(r/'tests/rtl8814b_hal_tests.cpp'),*map(str,objects),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
