#!/usr/bin/env python3
"""Exercise actual controller transfer allocation and platform teardown methods.

Only the IOKit allocation/event-source boundary and unrelated family hardware
cleanup are substituted. A surviving sibling WorkLoop makes stale event sources
observable independently of whether the detached controller was the last device.
"""
from pathlib import Path
import argparse
import importlib.util
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--source-root", type=Path, default=project,
                        help="production source to compile (supports baseline regression checks)")
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    spec = importlib.util.spec_from_file_location(
        "wpa_extraction", project / "scripts/test-rtwn-wpa-recovery.py")
    extraction = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extraction)
    controller = (source_root / "kext/RTWUSBController.cpp").read_text()
    backend = (source_root / "kext/RTW88Backend.inc").read_text()
    methods = [(controller, "int", "allocateTransfers"),
               (controller, "void", "freeTransfers"),
               (controller, "void", "releasePlatform"),
               (backend, "void", "release88Storage")]
    with tempfile.TemporaryDirectory(prefix="rtw-kext-resources-") as temporary:
        out = args.out or Path(temporary)
        out.mkdir(parents=True, exist_ok=True)
        actual = out / "actual_kext_resources.cpp"
        actual.write_text('#include "kext_resource_host.hpp"\n' + "\n".join(
            extraction.function(text, "^" + kind + " RTWUSBController::" + name + r"\(")
            for text, kind, name in methods))
        binary = out / "kext-resource-tests"
        subprocess.run([*compiler(), "-std=c++17", "-O1", "-g", "-UNDEBUG",
                        "-Wall", "-Wextra", "-Werror", *sanitizer_flags(),
                        "-fno-omit-frame-pointer", "-I" + str(project / "tests"),
                        "-I" + str(source_root / "include"), str(actual),
                        str(project / "tests/kext_resource_tests.cpp"),
                        "-x", "c++", str(source_root / "src/RTL8192EUDataPath.c"),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
