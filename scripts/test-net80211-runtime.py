#!/usr/bin/env python3
"""Exercise actual shared-runtime and taskqueue lifetime using IOKit host shims."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse,subprocess
p=Path(__file__).resolve().parents[1]
a=argparse.ArgumentParser();a.add_argument('--out',type=Path,default=p/'out-net80211-runtime');v=a.parse_args();v.out.mkdir(parents=True,exist_ok=True)
h=p/'tests/net80211_runtime_host'
subprocess.run([*compiler(),'-std=c++17','-g','-O1',*sanitizer_flags(),'-fno-omit-frame-pointer','-pthread','-I'+str(h/'include'),'-I'+str(p/'vendor/itl80211/openbsd'),'-include',str(h/'platform.hpp'),str(p/'kext/RTWNet80211Globals.cpp'),str(p/'vendor/itl80211/openbsd/sys/_task.cpp'),str(p/'tests/net80211_runtime_tests.cpp'),'-o',str(v.out/'tests')],check=True)
subprocess.run([str(v.out/'tests')],check=True)
