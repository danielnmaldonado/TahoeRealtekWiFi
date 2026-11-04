#!/usr/bin/env python3
"""Offline IOKit retry ownership against actual gated/output/wake methods."""
from pathlib import Path
import argparse
import runpy
import subprocess

project = Path(__file__).resolve().parents[1]
host = runpy.run_path(str(project / "scripts/host_cpp.py"))
extract = runpy.run_path(str(project / "scripts/test-rtwn-wpa-recovery.py"))["function"]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--out", type=Path, required=True)
parser.add_argument("--controller", type=Path, default=project / "kext/RTWUSBController.cpp")
parser.add_argument("--case", choices=("all", "admission", "wake", "completion"), default="all")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
source = args.controller.read_text()
header = (project / "kext/RTWUSBController.hpp").read_text()
start = header.index("    enum WPARecoveryState")
end = header.index("} wpaRecovery_{};", start) + len("} wpaRecovery_{};")
(args.out / "wpa_recovery_state.hpp").write_text(header[start:end] + "\n")
closure = '#include "platform.hpp"\n'
for name in ("supported5GHzFlags", "authorized", "setLinkStatus", "stateTransition", "observeScanTransition", "publishScanDiagnostics", "watchdogAction", "eraseProfile", "quiesce", "enableStation", "handleControl", "gated", "status", "resetWpaRecovery", "cancelWpaRecovery", "wpaStateChanged", "advanceWpaRecovery", "publishWpaRecoveryDiagnostics", "outputPacket"):
    closure += extract(source, r"^(?:bool|void|int|IOReturn|UInt32|uint32_t) RTWUSBController::" + name + r"\(")
# Baseline build53 has no wake method. Preserve its actual absence for RED.
if "void RTWUSBController::serviceTXOutputQueue(" in source:
    closure += extract(source, r"^void RTWUSBController::serviceTXOutputQueue\(")
else:
    closure += "void RTWUSBController::serviceTXOutputQueue() {}\n"
closure += extract((project / "vendor/rtwn/rtwn.cpp").read_text(), r"^void\nrtwn_start\(")
closure += extract((project / "vendor/itl80211/openbsd/sys/_ifq.cpp").read_text(), r"^mbuf_t ifq_dequeue\(")
generated = args.out / "actual_tx_backpressure.cpp"
generated.write_text(closure)
binary = args.out / "rtwn-tx-backpressure-tests"
subprocess.run(host["compiler"]() + ["-std=c++17", "-g", "-O1", "-Wall", "-Wextra", "-Werror", *host["sanitizer_flags"](), "-fno-omit-frame-pointer", "-I" + str(args.out), "-I" + str(project / "tests/rtwn_wpa_recovery_host"), "-I" + str(project / "tests"), "-I" + str(project / "include"), "-I" + str(project / "kext"), str(generated), str(project / "tests/rtwn_tx_backpressure_tests.cpp"), "-o", str(binary)], check=True)
if args.case != "completion":
    subprocess.run([str(binary)] + ([] if args.case == "all" else [args.case]), check=True)
if args.case in ("all", "completion"):
    rx_method = runpy.run_path(str(project / "tests/test-rtwn-rx-order.py"))["method"]
    generated = args.out / "actual_completion_wake.cpp"
    generated.write_text('#include "rtwn_rx_order_host.hpp"\n' + rx_method(source, "recordCompletion") + rx_method(source, "drainCompletions"))
    binary = args.out / "rtwn-tx-completion-wake-tests"
    subprocess.run(host["compiler"]() + ["-std=c++17", "-g", "-O1", "-x", "c++", "-Wall", "-Wextra", "-Werror", *host["sanitizer_flags"](), "-fno-omit-frame-pointer", "-I" + str(project / "tests"), "-I" + str(project / "include"), str(generated), str(project / "src/RTL8192EUDataPath.c"), str(project / "tests/rtwn_tx_completion_wake_tests.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
