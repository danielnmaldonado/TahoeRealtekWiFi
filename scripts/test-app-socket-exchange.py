#!/usr/bin/env python3
"""Compile and exercise the verbatim socket exchange implementation on a host.

Only the SDK-dependent code is excluded. readExact/writeExact/exchange are
extracted without edits from app/Transport.cpp, then tested with real socketpairs.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    source = (root / "app/Transport.cpp").read_text(encoding="utf-8")
    start = source.index("bool readExact(int fd,")
    end = source.index("bool makeJoin(", start)
    actual = source[start:end]
    # Fail closed on removal/renaming of the production function.
    for function in ("bool readExact(", "bool writeExact(", "uint32_t exchange("):
        if function not in actual:
            raise RuntimeError(f"Production function missing: {function}")
    with tempfile.TemporaryDirectory(prefix="rtw-socket-exchange-") as temp:
        target = args.out or Path(temp)
        target.mkdir(parents=True, exist_ok=True)
        implementation = target / "actual_socket_exchange.cpp"
        implementation.write_text(
            '#include "app/Transport.hpp"\n#include "app/Model.hpp"\n'
            "#include <cerrno>\n#include <climits>\n#include <sys/socket.h>\n"
            "namespace rtwui {\n" + actual + "\n}\n", encoding="utf-8")
        binary = target / "socket-exchange-tests"
        command = compiler() + ["-std=c++17", "-O1", "-g", "-Wall", "-Wextra",
                                "-Werror", "-UNDEBUG", *sanitizer_flags(),
                                "-fno-omit-frame-pointer", "-pthread",
                                "-I" + str(root), "-I" + str(root / "kext"),
                                str(implementation), str(root / "tests/socket_exchange_host_tests.cpp"),
                                "-o", str(binary)]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
