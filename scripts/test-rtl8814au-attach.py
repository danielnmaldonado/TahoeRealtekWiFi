#!/usr/bin/env python3
"""Exercise actual pre-stack attach before power/FW/MAC with strict PHY readiness."""
import argparse
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import re
import subprocess
import tempfile


def function(source, marker):
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{": depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0: return source[start:opening + token.end()]
    raise ValueError(marker)


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--premature-phy-baseline", action="store_true",
                        help="Replay the previous attach ordering to demonstrate its readiness failure")
    args = parser.parse_args()
    backend = (root / "kext/RTW8814Backend.inc").read_text()
    boot = (root / "src/RTL8814AUBoot.c").read_text()
    phy = (root / "src/RTL8814AUPHY.c").read_text()
    attach = function(backend, "int RTWUSBController::attach8814(")
    # Finish at the first net80211 setup operation. Everything before this
    # boundary is compiled verbatim, including any premature PHY initializer.
    prefix = attach[:attach.index("    core_.sc_pdev=")] + "    return 0;\n}\n"
    if args.premature_phy_baseline:
        # Exact call removed by the fix, evaluated against the actual PHY
        # precondition below. Never modifies the production source.
        prefix = prefix.replace("    return 0;\n}",
            "    e=rtw_8814au_phy_init(&acCore_,acPHYStorage_,acPHYSize_);if(e)return e;\n    return 0;\n}")
    source = (root / "tests/rtl8814au_attach_tests.cpp").read_text()
    source = source.replace("/* ACTUAL_ATTACH_PREFIX */", prefix)
    source = source.replace("/* ACTUAL_CONTEXT_INITIALIZER */", function(boot, "void rtw_8814au_init("))
    source = source.replace("/* ACTUAL_PHY_BOARD_PRECONDITION */", function(phy, "static int validate_board("))
    with tempfile.TemporaryDirectory(prefix="rtw8814-attach-") as directory:
        out = args.out or Path(directory)
        out.mkdir(parents=True, exist_ok=True)
        cpp = out / "actual_attach.cpp"
        cpp.write_text(source)
        binary = out / "rtl8814au-attach-tests"
        subprocess.run([*compiler(), "-std=c++17", "-Wall", "-Wextra", "-Werror", "-g", "-O1",
                        *sanitizer_flags(), "-fno-omit-frame-pointer", "-I" + str(root / "include"),
                        str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary.resolve())], check=True)


if __name__ == "__main__":
    main()
