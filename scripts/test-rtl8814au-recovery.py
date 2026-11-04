#!/usr/bin/env python3
"""Actual AU startup/stop/control and native HAL; only OS/USB boundaries doubled."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--out', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]

def function(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

backend = (root / 'kext/RTW8814Backend.inc').read_text()
controller = (root / 'kext/RTWUSBController.cpp').read_text()
units = [function(backend, marker) for marker in (
    'static uint16_t rtw8814RateMask(',
    'rtw_8814au_station_params RTWUSBController::station8814Params()',
    'int RTWUSBController::tick8814()',
    'int RTWUSBController::start8814(',
    'void RTWUSBController::stop8814(',
    'int RTWUSBController::set8814Channel(',
    'int RTWUSBController::state8814(',
    'void RTWUSBController::initTask8814(')]
units += [function((root / 'kext/RTWFamilyDataPath.inc').read_text(),
                   'bool RTWUSBController::uses8814FirmwareRate()')]
units += [function(controller, marker) for marker in (
    'static IOReturn fromErrno(',
    'void RTWUSBController::eraseProfile(',
    'IOReturn RTWUSBController::enableStation(',
    'IOReturn RTWUSBController::handleControl(')]
recovery = root / 'kext/RTW8814Recovery.inc'
members = root / 'kext/RTW8814RecoveryMembers.inc'
if recovery.exists():
    units.append(recovery.read_text())
has_finalizer = recovery.exists() and 'void RTWUSBController::finish8814StopAfterDrain(' in recovery.read_text()
if not has_finalizer:
    # The previous callback path had no AU stop finalizer. Preserve that
    # behavior for the RED regression while keeping the real stop/HAL code.
    units.append('void RTWUSBController::finish8814StopAfterDrain() {}')

with tempfile.TemporaryDirectory(prefix='rtl8814au-recovery-') as temp:
    temp = Path(temp)
    text = (root / 'tests/rtl8814au_recovery_tests.cpp').read_text()
    text = text.replace('/* RECOVERY_MEMBERS */', members.read_text() if members.exists() else '')
    text = text.replace('/* CAPTURE_IDENTITY */', 'assert(capture8814RecoveryIdentity());' if recovery.exists() else '')
    text = text.replace('/* DEFERRED_STOP_DECL */', '' if has_finalizer else 'void finish8814StopAfterDrain();')
    text = text.replace('/* ACTUAL_METHODS */', '\n'.join(units))
    source = temp / 'recovery.cpp'
    source.write_text(text)
    common = ['-Wall', '-Wextra', '-Werror', *sanitizer_flags(),
              '-fno-omit-frame-pointer', '-g', '-I', str(root / 'include'), '-I', str(root / 'kext')]
    objects = []
    names = ['RTL8814AU' + part for part in ('Boot', 'Efuse', 'MAC', 'PHY', 'Station', 'DataPath')]
    names += ['USBPackets', 'RTL8192EUDataPath']
    for name in names:
        candidate = root / ('src/' + name + '.c')
        if not candidate.exists():
            continue
        obj = temp / (name + '.o')
        subprocess.run([*compiler('c'), '-std=c11', *common, '-c', str(candidate), '-o', str(obj)], check=True)
        objects.append(str(obj))
    output = args.out or temp / 'recovery-tests'
    output.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([*compiler(), '-std=c++17', *common, str(source), *objects, '-o', str(output)], check=True)
    subprocess.run([str(output.resolve())], check=True)
