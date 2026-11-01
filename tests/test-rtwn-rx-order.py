#!/usr/bin/env python3
"""Offline USB ordering fixtures against actual controller drain and ledger."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1] / "scripts"))
from host_cpp import compiler, sanitizer_flags
import argparse
import re
import subprocess


def method(source, name):
    start = source.index("void RTWUSBController::" + name + "(")
    opening = source.index("{", start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:opening + token.end()] + "\n"
    raise ValueError(name)


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    source = (project / "kext/RTWUSBController.cpp").read_text()
    generated = args.out / "actual_rx_order.cpp"
    generated.write_text('#include "rtwn_rx_order_host.hpp"\n' +
                         method(source, "recordCompletion") + method(source, "drainCompletions"))
    binary = args.out / "rtwn-rx-order-tests"
    subprocess.run([*compiler(), "-std=c++17", "-g", "-O1", "-x", "c++",
                    *sanitizer_flags(), "-fno-omit-frame-pointer", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(project / "tests"), "-I" + str(project / "include"),
                    str(generated), str(project / "src/RTL8192EUDataPath.c"),
                    str(project / "tests/rtwn_rx_order_tests.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
