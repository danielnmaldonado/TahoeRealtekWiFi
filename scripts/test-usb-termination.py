#!/usr/bin/env python3
"""Compile real controller provider-close methods with a reentrant IOKit mock."""
from pathlib import Path
import argparse
import importlib.util
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags


def main():
    root=Path(__file__).resolve().parent.parent
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    spec=importlib.util.spec_from_file_location('wpa_extraction',root/'scripts/test-rtwn-wpa-recovery.py')
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    cpp=(root/'kext/RTWUSBController.cpp').read_text()
    # Test the lifecycle gate's real transition contract alongside the
    # extracted methods; abort/drain gets independent deferred-cleanup tests.
    arm=cpp[cpp.index('    case Terminated:'):cpp.index('    case Sleep:',cpp.index('    case Terminated:'))]
    assert arm.index('closing_=true') < arm.index('quiesce(true)')
    assert arm.index('present_=false') < arm.index('quiesce(true)')
    with tempfile.TemporaryDirectory(prefix='rtw-usb-termination-') as temporary:
        out=args.out or Path(temporary)
        out.mkdir(parents=True,exist_ok=True)
        actual=out/'actual_usb_termination.cpp'
        actual.write_text('#include "usb_termination_host.hpp"\n#define super MockSuper\n'+
            ''.join(module.function(cpp,r'^void RTWUSBController::'+name+r'\(')
                    for name in ('closeUSBProvider',))+
            module.function(cpp,r'^IOReturn RTWUSBController::message\(')+
            '#undef super\n')
        binary=out/'usb-termination-tests'
        command=compiler()+['-std=c++17','-O1','-g','-Wall','-Wextra','-Werror',
            *sanitizer_flags(),'-fno-omit-frame-pointer','-pthread','-UNDEBUG',
            '-I'+str(root/'tests'),str(actual),str(root/'tests/usb_termination_tests.cpp'),
            '-o',str(binary)]
        subprocess.run(command,check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__':main()
