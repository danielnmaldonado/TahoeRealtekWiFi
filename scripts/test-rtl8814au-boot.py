#!/usr/bin/env python3
"""Compile and exercise the real RTL8814AU boot code under host sanitizers."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--out', type=Path)
parser.add_argument('--firmware', type=Path, help='Optional canonical firmware image; no USB I/O occurs')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='rtl8814au-boot-') as temp:
    output = args.out or Path(temp) / 'rtl8814au-boot-tests'
    output.parent.mkdir(parents=True, exist_ok=True)
    command = [*compiler("c"), '-std=c11', '-Wall', '-Wextra', '-Werror',
               *sanitizer_flags(), '-fno-omit-frame-pointer', '-g',
               '-I', str(root / 'include'), str(root / 'tests/rtl8814au_boot.c')]
    command += [str(root / ('src/RTL8814AU' + part + '.c')) for part in ('Boot', 'Efuse', 'MAC')]
    subprocess.run(command + ['-o', str(output)], check=True)
    for group in ('efuse', 'power', 'mac', 'fw', 'h2c', 'startup'):
        subprocess.run([str(output.resolve()), group], check=True)
    if args.firmware:
        subprocess.run([str(output.resolve()), 'image', str(args.firmware.resolve())], check=True)
