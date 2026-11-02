#!/usr/bin/env python3
"""Real controller identity/lifecycle methods over explicit hardware boundaries."""
from pathlib import Path
from host_cpp import compiler,sanitizer_flags
from family_control_host import function
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'kext/RTWFamilyRecovery.inc').read_text();au=(r/'kext/RTW8814Recovery.inc').read_text();r12=(r/'kext/RTW8812Backend.inc').read_text()
markers=['bool RTWUSBController::isLegacyRecoveryFamily(','bool RTWUSBController::familyRecoveryAvailable(','bool RTWUSBController::familyRecoveryDrained(','int RTWUSBController::captureFamilyRecoveryIdentity(','bool RTWUSBController::matchesFamilyRecoveryIdentity(','bool RTWUSBController::validateFamilyRecoveryBoard(','int RTWUSBController::beginFamilyStart(','void RTWUSBController::failFamilyStart(','void RTWUSBController::finishFamilyStart(','bool RTWUSBController::familyStopDrained(','void RTWUSBController::recordFamilyCleanup(','int RTWUSBController::recoverFamilyForControl(','void RTWUSBController::finishFamilyStopAfterDrain(','void RTWUSBController::publishFamilyRecoveryDiagnostics(']
registry=(r/'src/FamilyBackend.c').read_text();actual=registry[registry.index('#define USB_ONLY'):registry.index('static int catalog_id_exists')]
actual+=function(au,'static bool same8814Descriptor(')+'\n'+function(r12,'static bool rtw8812SamePHYIdentity(')+'\n'+'\n'.join(function(s,m) for m in markers)
text=(r/'tests/family_controller_recovery_tests.cpp').read_text().replace('/* ACTUAL_METHODS */',actual)
with tempfile.TemporaryDirectory(prefix='family-controller-recovery-') as t:
 t=Path(t);obj=t/'ledger.o';subprocess.run([*compiler('c'),'-std=c11','-g','-O1',*sanitizer_flags(),'-I'+str(r/'include'),'-c',str(r/'src/RTL8192EUDataPath.c'),'-o',str(obj)],check=True)
 source=t/'test.cpp';source.write_text(text);out=t/'test';subprocess.run([*compiler(),'-std=c++17','-g','-O1','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'kext'),str(source),str(obj),'-o',str(out)],check=True);subprocess.run([str(out)],check=True)
