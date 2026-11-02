#!/usr/bin/env python3
"""Compile the actual complete controller with real SDK and a 2048-byte frame cap.

No include overlays, device operations, kext loading, or installation occur.
"""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
import sys
import tempfile


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    if sys.platform != 'darwin':
        raise SystemExit('Requires macOS and real MacKernelSDK headers.')
    if not (root/'MacKernelSDK/Headers/IOKit/network/IONetworkController.h').is_file():
        raise SystemExit('Missing real MacKernelSDK headers.')
    with tempfile.TemporaryDirectory(prefix='rtw-controller-power-native-') as temporary:
        output = (args.out or Path(temporary)/'controller.o').resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        command = ['xcrun', 'clang++', '-O2', '-c', '-std=gnu++14', '-target',
            'x86_64-apple-macos11.0', '-DKERNEL=1', '-DKERNEL_PRIVATE=1',
            '-DDRIVER_PRIVATE=1', '-DIEEE80211_STA_ONLY=1', '-D__PRIVATE_SPI__=1',
            '-fno-builtin', '-fno-exceptions', '-fno-rtti', '-fapple-kext',
            '-fno-stack-protector', '-Wno-deprecated-register',
            '-Wno-c++17-extensions', '-Wno-macro-redefined', '-Wno-format',
            '-Werror=return-type', '-Wframe-larger-than=2048',
            '-Werror=frame-larger-than', '-include',
            str(root/'vendor/itl80211/RTWNet80211Prefix.hpp')]
        for include in ['MacKernelSDK/Headers', 'vendor/itl80211/openbsd',
                'vendor/itl80211/linux', 'vendor/itl80211', 'include', 'kext',
                'vendor/rtwn']:
            command += ['-I', str(root/include)]
        command += [str(root/'kext/RTWUSBController.cpp'), '-o', str(output)]
        result = subprocess.run(command, capture_output=True, text=True)
        if args.out:
            output.with_suffix('.log').write_text(result.stdout+result.stderr)
            paths = [root/'kext/RTWUSBController.cpp', root/'kext/RTWUSBController.hpp']
            output.with_suffix('.sources.json').write_text(json.dumps({str(p.relative_to(root)):
                hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2)+'\n')
        if result.returncode:
            print(result.stdout+result.stderr, file=sys.stderr)
            raise SystemExit(result.returncode)
        print('Actual complete controller: native SDK compilation passed; 2048-byte frame limit; no overlays or hardware operations.')


if __name__ == '__main__':
    main()
