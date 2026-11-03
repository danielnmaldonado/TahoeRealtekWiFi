#!/usr/bin/env python3
"""Offline allocation/read/recovery and actual wire codecs for13 station backends."""
import argparse
from pathlib import Path
import runpy
import subprocess
import tempfile


def main():
    root=Path(__file__).resolve().parents[1]
    helper=runpy.run_path(str(root/'scripts/test-rtwn-rx-recovery.py'))
    host=runpy.run_path(str(root/'scripts/host_cpp.py'))
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path)
    parser.add_argument('--controller',type=Path)
    args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='rtw-rx-capacity-') as temporary:
        out=args.out or Path(temporary);out.mkdir(parents=True,exist_ok=True)
        source=(args.controller or root/'kext/RTWUSBController.cpp').read_text()
        header=(root/'kext/RTWUSBController.hpp').read_text()
        for name,file,member in [('RXRecovery','rx_recovery_state.hpp','rxRecovery_'),('CalibrationDiagnostics','calibration_state.hpp','calibDiag_'),('RXDiagnostics','rx_diagnostics_state.hpp','rxDiag_')]:
            # Member arrays can also contain "{};"; terminate at the named
            # controller member rather than the first in-struct initializer.
            start=header.index('    struct '+name)
            terminator='} '+member+'{};'
            end=header.index(terminator,start)+len(terminator)
            (out/file).write_text(header[start:end]+'\n')
        closure='#include "rtw/RTL8192EUDataPath.h"\n#include "platform.hpp"\n'
        registry=(root/'src/FamilyBackend.c').read_text()
        closure+=registry[registry.index('#define USB_ONLY'):registry.index('static int catalog_id_exists')]
        for name in ('allocateTransfers','freeTransfers','busInitialize','submit','recordCompletion','usbCompletion','completionAction','drainCompletions','abortTransfers','beginRXRecovery','advanceRXRecovery','failRXRecovery','publishRXRecoveryDiagnostics'):
            closure+=helper['function'](source,name)
        # Exact structural preflight/diagnostics, before macOS mbuf delivery.
        receive=helper['function'](source,'receive')
        closure+=receive[:receive.index('    /* A second parse')]+ '}\n'
        cpp=out/'actual_rx_capacity.cpp';cpp.write_text(closure)
        objects=[]
        for source_name in ('USBPackets','RTL8814AUDataPath','RTL8192EUDataPath','RTL8192EU','Efuse','USBControl','USBRegisterIO','RTL8192CDataPath','RTL8192DUDataPath','FCS','RTL8192FUDataPath','RTL8710BUDataPath','RTL8723BUDataPath','RTL8812AUDataPath','RTW88RX'):
            obj=out/(source_name+'.o');objects.append(str(obj))
            subprocess.run(host['compiler']('c')+['-std=c11','-ffunction-sections','-fdata-sections','-g','-O1','-Wall','-Wextra','-Werror',*host['sanitizer_flags'](),'-I'+str(root/'include'),'-c',str(root/'src'/(source_name+'.c')),'-o',str(obj)],check=True)
        binary=out/'rtwn-rx-capacity-tests'
        subprocess.run(host['compiler']()+['-std=c++17','-g','-O1','-Wall','-Wextra','-Werror',*host['sanitizer_flags'](),'-fno-omit-frame-pointer','-DRTW_RX_CAPACITY_PREFLIGHT','-I'+str(out),'-I'+str(root/'tests/rtwn_rx_recovery_host'),'-I'+str(root/'include'),str(cpp),str(root/'tests/rtwn_rx_capacity_tests.cpp'),*objects,*host['dead_strip_flags'](),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__=='__main__':main()
