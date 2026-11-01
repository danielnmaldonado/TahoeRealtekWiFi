#!/usr/bin/env python3
"""Run actual FU/8710BU EDCA cores over fake USB; never opens hardware."""
from pathlib import Path
import argparse
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags

project = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--family', choices=['8192fu', '8710bu', 'both'], default='both')
parser.add_argument('--driver-source', type=Path,
                    help='Build a specified core for a red/green regression')
args = parser.parse_args()
if args.driver_source and args.family == 'both':
    parser.error('--driver-source requires one --family')
for family, prefix in [('8192fu', 'RTL8192FU'), ('8710bu', 'RTL8710BU')]:
    if args.family not in [family, 'both']:
        continue
    with tempfile.TemporaryDirectory(prefix='legacy-edca-' + family + '-') as temp:
        directory = Path(temp)
        flags = ['-O1', '-g', '-Wall', '-Wextra', '-Werror',
                 *sanitizer_flags(), '-I' + str(project / 'include'),
                 '-I' + str(project / 'src'), '-I' + str(project / 'tests')]
        objects = []
        for name in [prefix, prefix + 'DataPath', 'Efuse', 'USBControl', 'USBRegisterIO']:
            obj = directory / (name + '.o')
            source = args.driver_source if name == prefix and args.driver_source else project / 'src' / (name + '.c')
            subprocess.run([*compiler('c'), '-std=c11', *flags, '-c',
                            str(source), '-o', str(obj)], check=True)
            objects.append(str(obj))
        executable = directory / 'edca-tests'
        subprocess.run([*compiler(), '-std=c++17', *flags,
                        '-DRTW_EDCA_TEST_' + family.upper() + '=1',
                        str(project / 'tests/legacy_usb_edca_tests.cpp'),
                        *objects, '-o', str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
