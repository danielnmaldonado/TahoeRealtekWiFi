#!/usr/bin/env python3
"""Real common Controller admission/recovery and pure DFS parsing; offline only."""
from pathlib import Path
import argparse,runpy,subprocess,tempfile,re
p=argparse.ArgumentParser();p.add_argument('--out',type=Path);p.add_argument('--controller',type=Path);a=p.parse_args()
r=Path(__file__).resolve().parents[1];h=runpy.run_path(str(r/'scripts/host_cpp.py'));extract=runpy.run_path(str(r/'scripts/test-rtwn-wpa-recovery.py'))['function']
with tempfile.TemporaryDirectory(prefix='rtw-dfs-station-') as td:
 out=a.out or Path(td);out.mkdir(parents=True,exist_ok=True)
 s=(a.controller or r/'kext/RTWUSBController.cpp').read_text();header=(r/'kext/RTWUSBController.hpp').read_text();i=header.index('    enum WPARecoveryState');j=header.index('} wpaRecovery_{};',i)+len('} wpaRecovery_{};');(out/'wpa_recovery_state.hpp').write_text(header[i:j])
 closure='#include "platform.hpp"\n'
 for name in ('vendorRequest','supported5GHzFlags','authorized','setLinkStatus','stateTransition','observeScanTransition','publishScanDiagnostics','watchdogAction','eraseProfile','quiesce','handleControl','gated','status','resetWpaRecovery','cancelWpaRecovery','wpaStateChanged','advanceWpaRecovery','publishWpaRecoveryDiagnostics'):
  closure+=extract(s,r'^(?:bool|void|int|IOReturn|uint32_t) RTWUSBController::'+name+r'\(')
 dfs=r/'kext/RTWDFSStation.inc'
 if dfs.exists():
  for name in ('candidateDFSPower','prepareDFSPower','dfsTXBlocked','stopDFSStation','resetDFSStation','dfsStateAllowed','dfsStateChanged','dfsFrameAllowed','observeDFSFrame','advanceDFSRecovery'):
   closure+=extract(dfs.read_text(),r'^(?:bool|void|int) RTWUSBController::'+name+r'\(')
 cpp=out/'controller.cpp';cpp.write_text(closure);binary=out/'controller-tests'
 subprocess.run(h['compiler']()+['-DRTW_DFS_ACTUAL=1','-std=c++17','-Wall','-Wextra','-Werror',*h['sanitizer_flags'](),'-I'+str(out),'-I'+str(r/'tests/rtwn_wpa_recovery_host'),'-I'+str(r/'include'),'-I'+str(r/'kext'),str(cpp),str(r/'tests/dfs_station_controller_tests.cpp'),'-x','c++',str(r/'src/DFSStation.c'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)

# Standalone policy/parser uses the same implementation as the kext.
with tempfile.TemporaryDirectory(prefix='rtw-dfs-parser-') as td:
 binary=Path(td)/'parser'
 subprocess.run(h['compiler']()+['-x','c++','-std=c++17','-Wall','-Wextra','-Werror',*h['sanitizer_flags'](),'-I'+str(r/'include'),str(r/'tests/dfs_station_tests.cpp'),str(r/'src/DFSStation.c'),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
