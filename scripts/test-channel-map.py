#!/usr/bin/env python3
"""Offline exact registered-channel ABI, actual controller/model/transport tests."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import subprocess
import sys
import tempfile

parser=argparse.ArgumentParser();parser.add_argument('--out',type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[1]

def extract(source,marker):
    start=source.index(marker);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

with tempfile.TemporaryDirectory(prefix='rtw-channel-map-') as temporary:
    out=args.out or Path(temporary);out.mkdir(parents=True,exist_ok=True)
    flags=['-std=c++17','-Wall','-Wextra','-Werror',*sanitizer_flags(),
        '-I'+str(root/'app'),'-I'+str(root/'kext'),'-I'+str(root/'include')]
    def build(name,sources):
        binary=(out/name).resolve()
        subprocess.run([*compiler(),*flags,'-x','c++',*map(str,sources),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
    build('model-tests',[root/'tests/channel_map_tests.cpp',root/'src/DFSRegulatory.c'])
    source=(root/'kext/RTWUSBController.cpp').read_text()
    query=extract(source,'void RTWUSBController::channelMap(')
    dispatch=extract(source,'IOReturn RTWUSBController::handleControl(')
    # Run the complete production validation and read-only dispatch prefix;
    # writable operations are deliberately outside this query-only fixture.
    dispatch=dispatch[:dispatch.index('    auto*reply=')]+ '    return kIOReturnUnsupported;\n}\n'
    fixture=(root/'tests/channel_map_controller_tests.cpp').read_text()
    generated=out/'controller.cpp';generated.write_text(fixture.replace('/* ACTUAL_CONTROLLER_METHODS */',query+'\n'+dispatch))
    build('controller-tests',[generated,root/'src/DFSRegulatory.c'])
    if sys.platform=='darwin':
        build('transport-tests',[root/'tests/channel_map_transport_tests.cpp',root/'app/Transport.cpp'])
        binary=(out/'reconnect-tests').resolve()
        sources=['tests/channel-map-reconnect-tests.mm','LoginItem.mm','CredentialStore.mm',
            'DriverConnection.mm','ControlDiagnostics.mm','DiagnosticCLI.mm','AdapterDiagnostics.mm','USBWiFiProfiles.cpp','Transport.cpp']
        command=[*compiler(),*flags,'-fobjc-arc',*[str(root/'app'/p) for p in sources]]
        for framework in ['AppKit','ServiceManagement','Security','CoreServices','IOKit','CoreFoundation']:
            command+=['-framework',framework]
        subprocess.run(command+['-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
