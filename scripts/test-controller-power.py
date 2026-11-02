#!/usr/bin/env python3
"""Run actual controller PM/start/stop methods at documented IOKit boundaries.

The host superclass preserves Apple's start -> prepare -> policy-hook -> attach
contract. This does not reproduce physical USB or live kernel scheduling.
"""
from pathlib import Path
import argparse
import importlib.util
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path)
    parser.add_argument('--controller', type=Path, default=root/'kext/RTWUSBController.cpp')
    parser.add_argument('--header', type=Path, default=root/'kext/RTWUSBController.hpp')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('extraction', root/'scripts/test-rtwn-wpa-recovery.py')
    extract = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extract)
    source, header = args.controller.read_text(), args.header.read_text()
    with tempfile.TemporaryDirectory(prefix='rtw-controller-pm-') as temporary:
        out = args.out or Path(temporary)
        out.mkdir(parents=True, exist_ok=True)
        # Missing override retains the real superclass Unsupported default,
        # allowing the pre-fix production start() to fail at runtime.
        declarations, actual = '', ''
        for result, name in [('IOReturn', 'registerWithPolicyMaker'), ('void', 'stopPowerManagement')]:
            if result+' RTWUSBController::'+name+'(' in source:
                declarations += ('IOReturn registerWithPolicyMaker(IOService *) override;\n'
                    if name == 'registerWithPolicyMaker' else 'void stopPowerManagement();\n')
                actual += extract.function(source, '^'+result+' RTWUSBController::'+name+r'\(')
        # Keep actual field initializers, testing per-instance ownership rather
        # than supplying an independent PM implementation for the controller.
        fields = [line for line in header.splitlines() if 'powerRegistration' in line]
        (out/'power_declarations.hpp').write_text(declarations+'\n'.join(fields)+'\n')
        sleep = source[source.index('    case Sleep:'):source.index('    default:return kIOReturnBadArgument;', source.index('    case Sleep:'))]
        (out/'power_gate_cases.inc').write_text(sleep)
        for name, result in [('start', 'bool'), ('setPowerState', 'IOReturn'), ('stop', 'void')]:
            actual += extract.function(source, '^'+result+' RTWUSBController::'+name+r'\(')
        generated = out/'actual_controller_power.cpp'
        generated.write_text('#include "controller_power_host.hpp"\n#define super MockNetworkController\n'+actual+'\n#undef super\n')
        binary = out/'controller-power-tests'
        subprocess.run(compiler()+['-std=c++17', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
            *sanitizer_flags(), '-fno-omit-frame-pointer', '-UNDEBUG', '-I'+str(out),
            '-I'+str(root/'tests'), str(generated), str(root/'tests/controller_power_tests.cpp'),
            '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
