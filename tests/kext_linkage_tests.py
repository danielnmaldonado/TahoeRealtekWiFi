#!/usr/bin/env python3
"""Regressions for builds that succeed but omit driver implementation files."""
import importlib.util
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from host_cpp import compiler

class LinkageTests(unittest.TestCase):
    def test_controller_cleanup_allocation_links_against_public_mach_kpi(self):
        source = (ROOT / 'kext/RTWUSBController.cpp').read_text()
        allocation = re.search(r'cleanupCall_=(thread_call_allocate[^;]+);', source)
        self.assertIsNotNone(allocation)
        # Header availability does not establish KPI export. Tahoe Mach.kext
        # exports allocate[_with_options]; the priority-only API is private.
        # Compile the actual allocation against only that public surface.
        fixture = r'''
#include <cassert>
using thread_call_func_t = void (*)(void *, void *);
enum thread_call_priority_t {THREAD_CALL_PRIORITY_HIGH, THREAD_CALL_PRIORITY_KERNEL};
struct Call {thread_call_func_t callback; void *context; thread_call_priority_t priority; unsigned options;};
using thread_call_t = Call *;
Call call;
thread_call_t thread_call_allocate_with_options(thread_call_func_t fn,void *context,
        thread_call_priority_t priority,unsigned options) {
    call={fn,context,priority,options};return &call;
}
thread_call_t thread_call_allocate(thread_call_func_t fn,void *context) {
    return thread_call_allocate_with_options(fn,context,THREAD_CALL_PRIORITY_HIGH,0);
}
struct Controller {
    bool invoked=false;thread_call_t cleanupCall_=nullptr;
    static void deferredCleanup(void *self,void *) {static_cast<Controller *>(self)->invoked=true;}
    void allocate() {cleanupCall_=ACTUAL_ALLOCATION;}
};
int main() {
    Controller controller;controller.allocate();
    assert(controller.cleanupCall_ && call.context==&controller);
    assert(call.priority==THREAD_CALL_PRIORITY_KERNEL && call.options==0);
    call.callback(call.context,nullptr);assert(controller.invoked);
}
'''.replace('ACTUAL_ALLOCATION', allocation.group(1))
        with tempfile.TemporaryDirectory(prefix='rtw-cleanup-kpi-') as directory:
            source_path=pathlib.Path(directory)/'allocation.cpp'
            executable=pathlib.Path(directory)/'allocation'
            source_path.write_text(fixture)
            built=subprocess.run(compiler()+['-std=c++17','-Wall','-Wextra','-Werror',
                str(source_path),'-o',str(executable)],capture_output=True,text=True)
            self.assertEqual(built.returncode,0,built.stderr)
            subprocess.run([str(executable)],check=True)

    def test_every_core_source_is_in_xcode_compile_phase(self):
        project = (ROOT / 'TahoeRealtekWiFi.xcodeproj/project.pbxproj').read_text()
        phase = re.search(r'isa=PBXSourcesBuildPhase;.*?files=\((.*?)\)', project).group(1)
        missing = []
        for source in sorted((ROOT / 'src').glob('*.c')):
            path = str(source.relative_to(ROOT))
            ref = re.search(r'([A-F0-9]{24}) = \{isa=PBXFileReference;[^\n]*path="' + re.escape(path) + r'";', project)
            build = re.search(r'([A-F0-9]{24}) = \{isa=PBXBuildFile; fileRef=' + ref.group(1) + ';', project) if ref else None
            if not build or build.group(1) not in phase:
                missing.append(path)
        self.assertEqual(missing, [], 'Xcode silently omitted driver implementations')

    def test_local_symbols_cannot_be_left_for_kernel_to_resolve(self):
        path = ROOT / 'scripts/check-kext-linkage.py'
        spec = importlib.util.spec_from_file_location('linkage', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        missing = module.unresolved_driver_symbols('''_IOMalloc
__ZN20IOEthernetController10gMetaClassE
_rtw_92e_make_media_status
_rtw_8814au_phy_init
_rtw_usb_wifi_lookup
__ZN16RTWLocalInternal5startEv
_ieee80211_input
''')
        self.assertEqual(missing, ['__ZN16RTWLocalInternal5startEv', '_ieee80211_input',
            '_rtw_8814au_phy_init', '_rtw_92e_make_media_status', '_rtw_usb_wifi_lookup'])

if __name__ == '__main__':
    unittest.main()
