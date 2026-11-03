#!/usr/bin/env python3
"""Exercise the actual firmware-mailbox function using a host register trace."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import re
import subprocess


def function(source, name):
    position = source.index("\n" + name + "(") + 1
    start = source.rfind("\n", 0, position - 1) + 1
    opening = source.index("{", position)
    depth = 0
    tokens = re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:])
    for token in tokens:
        if token.group() == "{": depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0: return source[start:opening + token.end()] + "\n"
    raise ValueError(name)


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-rtwn-fw-command")
    parser.add_argument("--source", type=Path, default=project / "vendor/rtwn/rtwn.cpp")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    body = function(args.source.read_text(), "rtwn_fw_cmd") + function(args.source.read_text(), "rtwn_calib")
    generated = args.out / "actual_fw_command.cpp"
    generated.write_text('#include "rtwn_fw_command_host.hpp"\n' + body)
    command = [*compiler(), "-std=c++17", "-g", "-O1",
               *sanitizer_flags(), "-fno-omit-frame-pointer",
               "-Wall", "-Wextra", "-Werror", "-I" + str(project),
               "-I" + str(project / "tests"), str(generated),
               str(project / "tests/rtwn_fw_command_tests.cpp"),
               "-o", str(args.out / "rtwn-fw-command-tests")]
    subprocess.run(command, check=True)
    subprocess.run([str(args.out / "rtwn-fw-command-tests")], check=True)


if __name__ == "__main__": main()
