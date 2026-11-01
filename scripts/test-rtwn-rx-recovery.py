#!/usr/bin/env python3
"""Exercise real controller RX transport methods against host-only USB boundaries."""
from pathlib import Path
import argparse
import re
import runpy
import subprocess
import tempfile


def function(source, name):
    match = re.search(r"^(?:bool|void|int|IOReturn) RTWUSBController::" + name + r"\(", source, re.MULTILINE)
    if not match:
        return ""
    opening = source.index("{", match.end())
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{": depth += 1
        elif token.group() == "}":
            depth -= 1
            if not depth:
                return source[match.start():opening + token.end()] + "\n"
    raise ValueError(name)


def main():
    root = Path(__file__).resolve().parent.parent
    host = runpy.run_path(str(root / "scripts/host_cpp.py"))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--case", help="Run one named controller recovery regression")
    parser.add_argument("--controller", type=Path, help="Controller snapshot for regression sensitivity checks")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="rtw-rx-recovery-") as temporary:
        out = args.out or Path(temporary)
        out.mkdir(parents=True, exist_ok=True)
        source = (args.controller or root / "kext/RTWUSBController.cpp").read_text()
        header = (root / "kext/RTWUSBController.hpp").read_text()
        marker = header.find("    struct RXRecovery")
        state = ""
        if marker >= 0:
            end = header.index("} rxRecovery_{};", marker) + len("} rxRecovery_{};")
            state = header[marker:end]
        (out / "rx_recovery_state.hpp").write_text(state)
        marker = header.index("    struct CalibrationDiagnostics")
        end = header.index("} calibDiag_{};", marker) + len("} calibDiag_{};")
        (out / "calibration_state.hpp").write_text(header[marker:end])
        closure = '#include "platform.hpp"\n'
        registry = (root / "src/FamilyBackend.c").read_text()
        closure += registry[registry.index("#define USB_ONLY"):registry.index("static int catalog_id_exists")]
        for name in ("submit", "recordCompletion", "usbCompletion", "completionAction", "drainCompletions", "abortTransfers", "beginRXRecovery", "advanceRXRecovery", "failRXRecovery", "publishRXRecoveryDiagnostics", "scanAction", "calibAction", "watchdogAction"):
            closure += function(source, name)
        cpp = out / "actual_rx_recovery.cpp"
        cpp.write_text(closure)
        binary = out / "rtwn-rx-recovery-tests"
        subprocess.run(host["compiler"]() + ["-x", "c++", "-std=c++17", "-g", "-O1", "-Wall", "-Wextra", "-Werror", *host["sanitizer_flags"](),
            "-fno-omit-frame-pointer", "-I" + str(out), "-I" + str(root / "tests/rtwn_rx_recovery_host"), "-I" + str(root / "include"),
            str(cpp), str(root / "tests/rtwn_rx_recovery_tests.cpp"), str(root / "src/RTL8192EUDataPath.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)] + ([args.case] if args.case else []), check=True)


if __name__ == "__main__":
    main()
