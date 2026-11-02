#!/usr/bin/env python3
"""Actual legacy control/init/stop/recovery and HAL over host USB boundaries."""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile
from family_control_host import function
from host_cpp import compiler, sanitizer_flags

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path)
parser.add_argument('--controller-source', type=Path)
args = parser.parse_args()
controller = (args.controller_source or root / 'kext/RTWUSBController.cpp').read_text()
native = (root / 'vendor/rtwn/rtwn.cpp').read_text()
recovery = (root / 'kext/RTWFamilyRecovery.inc').read_text()
header = (root / 'vendor/rtwn/rtwnvar.h').read_text()
ops_start = header.index('struct rtwn_ops {')
ops = header[ops_start:header.index('\n};', ops_start) + 3]
defines = '\n'.join(re.findall(r'^#define RTWN_(?:CHIP|FLAG)_[^\n]+', header, re.M))
hal = (root / 'kext/RTW8192EUHAL.cpp').read_text().replace('#include "RTW8192EUHAL.hpp"', '')
registry = (root / 'src/FamilyBackend.c').read_text()
actual_common = registry[registry.index('#define USB_ONLY'):registry.index('static int catalog_id_exists')]
actual_common += function((root / 'kext/RTW8814Recovery.inc').read_text(), 'static bool same8814Descriptor(')
actual_common += function((root / 'kext/RTW8812Backend.inc').read_text(), 'static bool rtw8812SamePHYIdentity(')
for marker in (
    'bool RTWUSBController::isLegacyRecoveryFamily(',
    'bool RTWUSBController::familyRecoveryAvailable(',
    'bool RTWUSBController::familyRecoveryDrained(',
    'int RTWUSBController::readLegacyRecoveryIdentity(',
    'int RTWUSBController::captureFamilyRecoveryIdentity(',
    'bool RTWUSBController::matchesFamilyRecoveryIdentity(',
    'bool RTWUSBController::validateFamilyRecoveryBoard(',
    'int RTWUSBController::reidentifyFamilyRecovery(',
    'int RTWUSBController::beginFamilyStart(',
    'void RTWUSBController::failFamilyStart(',
    'void RTWUSBController::finishFamilyStart(',
    'bool RTWUSBController::familyStopDrained(',
    'void RTWUSBController::recordFamilyCleanup(',
    'int RTWUSBController::recoverFamilyForControl(',
    'void RTWUSBController::finishFamilyStopAfterDrain(',
    'void RTWUSBController::publishFamilyRecoveryDiagnostics('):
    actual_common += '\n' + function(recovery, marker)
actual_controller = '\n'.join(function(controller, marker) for marker in (
    'static IOReturn fromErrno(',
    'int RTWUSBController::ioError(',
    'int RTWUSBController::busPower(',
    'int RTWUSBController::busDMA(',
    'void RTWUSBController::busMAC(',
    'void RTWUSBController::busBB(',
    'void RTWUSBController::busAggregation(',
    'int RTWUSBController::busAllocate(',
    'int RTWUSBController::busInitialize(',
    'void RTWUSBController::busStop(',
    'IOReturn RTWUSBController::enableStation(',
    'IOReturn RTWUSBController::handleControl('))
actual_native = '\n'.join(function(native, marker) for marker in (
    'uint8_t\nrtwn_efuse_read_1(',
    'void\nrtwn_efuse_read(',
    'void\nrtwn_efuse_switch_power(',
    'int\nrtwn_init(',
    'void\nrtwn_stop(',
    'void\nrtwn_init_task('))
text = (root / 'tests/legacy_controller_recovery_tests.cpp').read_text()
for marker, value in [('RTWN_OPS', ops), ('RTWN_DEFINES', defines),
                      ('HAL_METHODS', hal), ('COMMON_METHODS', actual_common),
                      ('CONTROLLER_METHODS', actual_controller), ('RTWN_METHODS', actual_native)]:
    text = text.replace('/* ' + marker + ' */', value)
with tempfile.TemporaryDirectory(prefix='legacy-controller-recovery-') as temp:
    output = args.out or Path(temp)
    output.mkdir(parents=True, exist_ok=True)
    common = ['-g', '-O1', '-Wall', '-Wextra', '-Werror', '-Wno-sign-compare',
              *sanitizer_flags(), '-fno-omit-frame-pointer',
              '-I' + str(root / 'include'), '-I' + str(root / 'kext'),
              '-I' + str(root / 'vendor/rtwn')]
    objects = []
    for name in ['RTL8192EUDataPath', 'Efuse', 'USBRegisterIO', 'USBControl']:
        obj = output / (name + '.o')
        subprocess.run([*compiler('c'), '-std=c11', *common, '-c',
                        str(root / 'src' / (name + '.c')), '-o', str(obj)], check=True)
        objects.append(str(obj))
    source = output / 'actual_legacy_controller.cpp'
    source.write_text(text)
    binary = output / 'legacy-controller-recovery-tests'
    subprocess.run([*compiler(), '-std=c++17', *common, str(source), *objects,
                    '-o', str(binary)], check=True)
    subprocess.run([str(binary.resolve())], check=True)
