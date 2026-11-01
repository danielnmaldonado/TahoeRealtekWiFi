#!/usr/bin/env python3
"""Compile the actual deferred teardown methods with deterministic IOKit boundaries."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse
import importlib.util
import platform
import shutil
import subprocess
import tempfile

def main():
    project=Path(__file__).resolve().parent.parent
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out",type=Path)
    args=parser.parse_args()
    spec=importlib.util.spec_from_file_location("wpa_extraction",project/"scripts/test-rtwn-wpa-recovery.py")
    module=importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    with tempfile.TemporaryDirectory(prefix="rtw-cleanup-") as temp:
        out=args.out or Path(temp)
        out.mkdir(parents=True,exist_ok=True)
        source=(project/"kext/RTWUSBController.cpp").read_text()
        methods=("shutdownPlatform","scheduleDeferredCleanup","markTeardownReady","deferredCleanup")
        generated=out/"actual_deferred_cleanup.cpp"
        generated.write_text('#include "deferred_cleanup_host.hpp"\n'+
            "".join(module.function(source,r"^void RTWUSBController::"+name+r"\(") for name in methods))
        binary=out/"deferred-cleanup-tests"
        command=compiler()+["-std=c++17","-g","-O1","-Wall","-Wextra","-Werror",
            *sanitizer_flags(),"-fno-omit-frame-pointer","-UNDEBUG",
            "-I"+str(project/"tests"),"-I"+str(project/"include"),str(generated),
            str(project/"tests/deferred_cleanup_tests.cpp"),"-x","c++",
            str(project/"src/RTL8192EUDataPath.c"),"-o",str(binary)]
        subprocess.run(command,check=True)
        subprocess.run([str(binary)],check=True)
if __name__=="__main__":
    main()
