#!/usr/bin/env python3
"""Run the actual 8192EU IQK backend with a phase-sensitive register transport.

The peripheral model releases a measurement only after a matching RF mode,
TX/TXIMR/RX configuration, trigger pair and settling delay. It deliberately
starts result registers with plausible stale values. This checks the driver's
protocol and bookkeeping; it does not model analog RF or USB electrical faults.
"""
from pathlib import Path
import os
import shlex
import subprocess
import sys
import tempfile

project = Path(__file__).resolve().parents[3]
import runpy
_host = runpy.run_path(str(project / "scripts/host_cpp.py"))
_host_cxx = _host["compiler"]()
_host_sanitizers = _host["sanitizer_flags"]()
source = (project / "vendor/rtwn/rtwn.cpp").read_text()

def function(name, returns):
    start = source.index("\n" + name + "(") + 1
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return returns + "\n" + source[start:end] + "\n"

with tempfile.TemporaryDirectory(prefix="r92e-iqk-") as directory:
    temp = Path(directory)
    writer = function("rtwn_iq_calib_write_results", "void")
    (temp / "writer.inc").write_text(writer)
    backend = project / "vendor/rtwn/r92e_iqk.inc"
    (temp / "backend.inc").write_text('#include "vendor/rtwn/r92e_iqk.inc"\n')
    def run(label, content=None, expect_failure=False):
        if content is not None:
            (temp / "backend.inc").write_text(content)
        executable = temp / ("trace-" + label)
        subprocess.run(_host_cxx + [
            "-std=c++17", "-Wall", "-Wextra", "-Werror", "-Wno-sign-compare",
            *_host_sanitizers, "-fno-omit-frame-pointer",
            "-I", str(project), "-I", str(temp),
            str(Path(__file__).with_suffix(".cpp")), "-o", str(executable)
        ], check=True)
        result = subprocess.run([str(executable)], text=True,
                                capture_output=expect_failure)
        if expect_failure:
            if result.returncode != 1 or "FAIL:" not in result.stderr:
                raise RuntimeError(f"mutation {label} escaped the behavioral checks: {result.stderr}")
            print(f"caught IQK mutation: {label}")
        else:
            result.check_returncode()

    run("production")
    if "--mutations" in sys.argv:
        original = backend.read_text()
        mutations = {
            "missing-rx-shot": ("if (rtwn_r92e_iqk_shot(sc, chain, 10000))", "if (1)"),
            "wrong-b-trigger": ("chain ? 0xfa000000 : 0xf9000000", "chain ? 0xf9000000 : 0xf9000000"),
            "unsigned-y": ("if (ya & 0x200) ya -= 0x400;", "if (0) ya -= 0x400;"),
            "accept-zero-report": ("(!a && !b)", "false"),
            "omit-bb-restore": ("rtwn_bb_write(sc, rtwn_r92e_iqk_bb[i], saved->bb[i]);", "(void)i;"),
            "compare-only-a": ("pair < chains * 2", "pair < 2"),
            "tx-only-mask": ("if (result[candidate].valid & rxbit)\n\t\t\tsc->iq_calibration_mask", "if (result[candidate].valid & txbit)\n\t\t\tsc->iq_calibration_mask"),
        }
        for label, (before, after) in mutations.items():
            if original.count(before) != 1:
                raise RuntimeError(f"mutation {label} needs updating for the current implementation")
            run(label, original.replace(before, after), expect_failure=True)
