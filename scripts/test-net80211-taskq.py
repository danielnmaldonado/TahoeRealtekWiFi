#!/usr/bin/env python3
"""Run taskq lifecycle tests against a host event-loop model."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import subprocess

project = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--out", type=Path, default=project / "out-net80211-taskq")
parser.add_argument("--source", type=Path, default=project / "vendor/itl80211/openbsd/sys/_task.cpp")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
host = project / "tests/net80211_taskq_host"
command = [*compiler(), "-std=c++14", "-g", "-O1", *sanitizer_flags(),
           "-fno-omit-frame-pointer", "-I" + str(host / "include"),
           "-I" + str(project / "vendor/itl80211/openbsd"), "-include", str(host / "platform.hpp"),
           str(args.source), str(project / "tests/net80211_taskq_tests.cpp"),
           "-o", str(args.out / "net80211-taskq-tests")]
subprocess.run(command, check=True)
subprocess.run([str(args.out / "net80211-taskq-tests")], check=True)
