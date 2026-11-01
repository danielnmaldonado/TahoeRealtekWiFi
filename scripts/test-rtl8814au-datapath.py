#!/usr/bin/env python3
"""Build actual RTL8814AU wire fixtures with ASan/UBSan; optional old-code RED."""
import argparse
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import subprocess


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "build/8814au-codec")
    parser.add_argument("--gen2-baseline", action="store_true")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    sources = [project / "src/USBPackets.c", project / "src/RTL8814AUDataPath.c"]
    flags = []
    if args.gen2_baseline:
        sources[1] = project / "src/RTL8192EUDataPath.c"
        flags = ["-DRTW_8814AU_GEN2_BASELINE"]
    common = ["-g", "-O1", *sanitizer_flags(), "-fno-omit-frame-pointer",
              "-Wall", "-Wextra", "-Werror", "-Wconversion", "-Wshadow", "-Wpedantic",
              "-I" + str(project / "include")]
    objects = []
    for i, source in enumerate(sources):
        obj = args.out / f"{i}.o"
        subprocess.run([*compiler("c"), "-std=c11", *common, "-c", str(source), "-o", str(obj)], check=True)
        objects.append(obj)
    binary = args.out / "rtl8814au-datapath-tests"
    subprocess.run([*compiler(), "-std=c++14", *common, *flags,
                    str(project / "tests/rtl8814au_datapath_tests.cpp"),
                    *map(str, objects), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
