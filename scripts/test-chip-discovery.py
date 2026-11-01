#!/usr/bin/env python3
"""Actual Controller discovery and Boot proof, isolated USB host boundaries."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
subprocess.run([sys.executable,'-B',str(r/'tests/chip_discovery_personality_tests.py')],check=True)
def function(s,marker):
 start=s.index(marker);first=s.index('{',start);depth=1;end=first+1
 while depth:
  if s[end]=='{':depth+=1
  elif s[end]=='}':depth-=1
  end+=1
 return s[start:end]
controller=(r/'kext/RTWUSBController.cpp').read_text();backend=(r/'kext/RTW88Backend.inc').read_text();registry=(r/'src/FamilyBackend.c').read_text();h=(r/'tests/chip_identity_controller_tests.cpp').read_text()
start=registry.index('#define USB_ONLY');end=registry.index('};',registry.index('static const rtw_family_backend backends[]'))+2
h=h.replace('/* ACTUAL_REGISTRY */',registry[start:end]+'\n'+function(registry,'size_t rtw_backend_family_count(')+'\n'+function(registry,'const rtw_family_backend *rtw_backend_for_family('))
markers=['void RTWUSBController::fault(', 'bool RTWUSBController::read88IdentityDescriptor(', 'bool RTWUSBController::matches88Discovery(', 'bool RTWUSBController::permits88Transport(', 'bool RTWUSBController::owns88Discovery(', 'void RTWUSBController::clear88Discovery(', 'int RTWUSBController::prepare88Identity(', 'int RTWUSBController::recheck88Identity(', 'int RTWUSBController::publish88Identity(', 'int RTWUSBController::vendorRequest(', 'uint32_t RTWUSBController::readRegister(', 'int RTWUSBController::writeRegister(', 'bool RTWUSBController::discoverUSB(']
h=h.replace('/* ACTUAL_BOOT_BUS_MODEL */',(r/'tests/rtw88_boot_tests.cpp').read_text().split('static void ready(')[0])
h=h.replace('/* ACTUAL_CONTROLLER_METHODS */','\n'.join(function(controller,x) for x in markers)+'\n'+'\n'.join(function(backend,x) for x in ['int RTWUSBController::r88Read(', 'int RTWUSBController::r88Write(', 'int RTWUSBController::bootstrap88(', 'void RTWUSBController::remember88Authority(', 'bool RTWUSBController::owns88Authority(']))
with tempfile.TemporaryDirectory(prefix='rtw-chip-discovery-') as td:
 d=Path(td);source=d/'controller.cpp';source.write_text(h);flags=['-O1','-g','-Wall','-Wextra','-Werror',*sanitizer_flags(),'-I'+str(r/'include'),'-I'+str(r/'kext')];objects=[]
 for name in ['DFSStation','ChipIdentity','USBWiFiProfiles','USBDescriptors','RTW88Boot','RTW88Firmware','RTW88MAC','RTW88USBDataPath']:
  obj=d/(name+'.o');objects.append(str(obj));subprocess.run([*compiler('c'),'-std=c11',*flags,'-c',str(r/'src'/(name+'.c')),'-o',str(obj)],check=True)
 exe=d/'discovery';subprocess.run([*compiler(),'-std=c++17',*flags,str(source),*objects,'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
