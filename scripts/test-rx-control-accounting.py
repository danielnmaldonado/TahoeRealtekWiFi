#!/usr/bin/env python3
"""Compile the actual receive admission/accounting block; offline only.

The host supplies HT/association and family-rate boundary decisions. The
production drop block, diagnostics struct and pure C admission helpers are
compiled unchanged. No USB device, kernel interface or network is opened.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from host_cpp import compiler, sanitizer_flags
from test_import_helpers import block


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--source-root", type=Path, default=project,
                        help="Production source, including immutable RED baselines")
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    with tempfile.TemporaryDirectory(prefix="rtw-rx-control-accounting-") as temp:
        out = (args.out or Path(temp)).resolve()
        out.mkdir(parents=True, exist_ok=True)
        controller = (source_root / "kext/RTWUSBController.cpp").read_text()
        header = (source_root / "kext/RTWUSBController.hpp").read_text()
        receive = block(controller, "void RTWUSBController::receive(")
        start = receive.index("        uint64_t*drop=nullptr;")
        end = receive.index("        if((p.mpdu[0]&IEEE80211_FC0_TYPE_MASK)", start)
        admission = receive[start:end]
        diagnostics = block(header, "struct RXDiagnostics {") + " rxDiag_{};\n"
        generated = out / "rx_control_accounting_actual.hpp"
        generated.write_text(
            '#include "rx_control_accounting_host.hpp"\n'
            'struct ControllerHarness : RXControlBoundary {\n'
            '    explicit ControllerHarness(rtw_wifi_family family) '
            ': RXControlBoundary(family) {}\n' + diagnostics +
            '    uint64_t rxErrors_=0;\n'
            '    bool dispatch(const RXControlFrame& frame, size_t length) {\n'
            '        bool forwarded=false;\n'
            '        for (unsigned once=0; once<1; ++once) {\n'
            '            const auto& p=frame.payload;\n' + admission +
            '            forwarded=true;\n'
            '        }\n'
            '        return forwarded;\n'
            '    }\n'
            '};\n')
        flags = ["-O1", "-g", "-Wall", "-Wextra", "-Werror",
                 *sanitizer_flags(), "-fno-omit-frame-pointer",
                 "-I" + str(source_root / "include"),
                 "-I" + str(out), "-I" + str(project / "tests")]
        obj = out / "RTL8192EUDataPath.o"
        executable = out / "rx-control-accounting-tests"
        commands = [
            [*compiler("c"), "-std=c11", *flags, "-c",
             str(source_root / "src/RTL8192EUDataPath.c"), "-o", str(obj)],
            [*compiler(), "-std=c++17", *flags,
             str(project / "tests/rx_control_accounting_tests.cpp"), str(obj),
             "-o", str(executable)],
            [str(executable)],
        ]
        results = []
        for command in commands:
            result = subprocess.run(command, text=True, capture_output=True)
            results.append(dict(command=command, exit_code=result.returncode,
                                stdout=result.stdout, stderr=result.stderr))
            print(result.stdout, end="")
            print(result.stderr, end="")
            if result.returncode:
                break
        paths = [source_root / "kext/RTWUSBController.cpp",
                 source_root / "kext/RTWUSBController.hpp",
                 source_root / "src/RTL8192EUDataPath.c",
                 source_root / "include/rtw/RTL8192EUDataPath.h",
                 project / "tests/rx_control_accounting_tests.cpp",
                 project / "tests/rx_control_accounting_host.hpp",
                 Path(__file__).resolve()]
        evidence = dict(results=results,
                        admission_block_sha256=hashlib.sha256(admission.encode()).hexdigest(),
                        source_sha256={str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                                       for p in paths})
        (out / "result.json").write_text(json.dumps(evidence, indent=2) + "\n")
        return results[-1]["exit_code"]


if __name__ == "__main__":
    raise SystemExit(main())
