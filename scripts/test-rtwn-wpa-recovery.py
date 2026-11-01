#!/usr/bin/env python3
"""Run real controller WPA recovery methods on a scripted, host-only clock.

Hardware/net80211 transitions are boundary doubles; the policy, controller
lifecycle hooks, and imported transmit dequeue are compiled from real sources.
No USB device, kernel service, network, or credential storage is accessed.
"""
from pathlib import Path
import argparse
import runpy
import re
import subprocess
import tempfile


def function(source, pattern):
    match = re.search(pattern, source, re.MULTILINE)
    if not match:
        raise ValueError(pattern)
    start = match.start()
    opening = source.index("{", match.end())
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{": depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0: return source[start:opening + token.end()] + "\n"
    raise ValueError(pattern)


def main():
    project = Path(__file__).resolve().parent.parent
    host = runpy.run_path(str(project / "scripts/host_cpp.py"))
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="rtw-wpa-recovery-") as temporary:
        out = args.out or Path(temporary)
        out.mkdir(parents=True, exist_ok=True)
        source = (project / "kext/RTWUSBController.cpp").read_text()
        header = (project / "kext/RTWUSBController.hpp").read_text()
        state_start = header.index("    enum WPARecoveryState")
        state_end = header.index("} wpaRecovery_{};", state_start) + len("} wpaRecovery_{};")
        (out / "wpa_recovery_state.hpp").write_text(header[state_start:state_end] + "\n")
        closure = '#include "platform.hpp"\n'
        closure += function(source,r"^uint32_t RTWUSBController::supported5GHzFlags\(")
        for name in ("authorized", "setLinkStatus", "stateTransition", "observeScanTransition", "publishScanDiagnostics", "watchdogAction", "eraseProfile", "quiesce", "handleControl", "gated", "status"):
            closure += function(source, r"^(?:bool|void|int|IOReturn) RTWUSBController::" + name + r"\(")
        for name in ("resetWpaRecovery", "cancelWpaRecovery", "wpaStateChanged", "advanceWpaRecovery", "publishWpaRecoveryDiagnostics"):
            closure += function(source, r"^void RTWUSBController::" + name + r"\(")
        vendor = (project / "vendor/rtwn/rtwn.cpp").read_text()
        closure += function(vendor, r"^void\nrtwn_start\(")
        queue = (project / "vendor/itl80211/openbsd/sys/_ifq.cpp").read_text()
        closure += function(queue, r"^mbuf_t ifq_dequeue\(")
        cpp = out / "actual_wpa_recovery.cpp"
        cpp.write_text(closure)
        binary = out / "rtwn-wpa-recovery-tests"
        subprocess.run(host["compiler"]()+[ "-std=c++17", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
            *host["sanitizer_flags"](), "-fno-omit-frame-pointer",
            "-I" + str(out), "-I" + str(project / "tests/rtwn_wpa_recovery_host"), "-I" + str(project / "include"), "-I" + str(project / "kext"),
            str(cpp), str(project / "tests/rtwn_wpa_recovery_tests.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__": main()
