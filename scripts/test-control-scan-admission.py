#!/usr/bin/env python3
"""Run the actual shared SCAN dispatcher against live-link race boundaries.

The control method is extracted verbatim. Hardware recovery and foreground
net80211 scanning are host boundaries; entering either is observable. A scan
boundary models net80211's documented BSS cleanup, so reaching it on a live
link fails even if a later transition could reconnect.
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
    parser.add_argument("--source-root", type=Path, default=project)
    args = parser.parse_args()
    root = args.source_root.resolve()
    spec = importlib.util.spec_from_file_location("extraction", project / "scripts/test-rtwn-wpa-recovery.py")
    extraction = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extraction)
    controller = (root / "kext/RTWUSBController.cpp").read_text()
    method = extraction.function(controller, r"^IOReturn RTWUSBController::handleControl\(")
    fixture = (project / "tests/control_scan_admission_tests.cpp").read_text()
    assert fixture.count("/* ACTUAL_CONTROL_DISPATCH */") == 1
    fixture = fixture.replace("/* ACTUAL_CONTROL_DISPATCH */", method)
    with tempfile.TemporaryDirectory(prefix="rtw-control-scan-") as temporary:
        out = args.out or Path(temporary)
        out.mkdir(parents=True, exist_ok=True)
        source = out / "actual_control_scan_tests.cpp"
        source.write_text(fixture)
        binary = out / "control-scan-admission-tests"
        subprocess.run([*compiler(), "-std=c++17", "-O1", "-g", "-Wall", "-Wextra", "-Werror",
                        "-UNDEBUG", *sanitizer_flags(), "-fno-omit-frame-pointer",
                        "-I" + str(root / "include"), "-I" + str(root / "kext"),
                        str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
