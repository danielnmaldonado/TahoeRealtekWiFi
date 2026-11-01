#!/usr/bin/env python3
"""Compile the pinned macOS net80211 stack without an Intel or USB HAL.

This only writes build artifacts. It never loads a kext or opens hardware.
"""
from pathlib import Path
import argparse
import concurrent.futures
import json
import subprocess
import sys


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-net80211")
    parser.add_argument("--sdk", type=Path, default=project / "MacKernelSDK")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    vendor = project / "vendor/itl80211"
    sources = [vendor / name for name in json.loads((vendor / "sources.json").read_text())]
    sources.append(project / "kext/RTWNet80211Globals.cpp")
    args.out.mkdir(parents=True, exist_ok=True)
    flags = [
        "xcrun", "clang++", "-x", "c++", "-target", "x86_64-apple-macos11.0",
        "-nostdinc", "-std=gnu++14", "-fno-builtin", "-fno-exceptions", "-fno-rtti",
        "-fno-sanitize=vptr", "-mkernel", "-fapple-kext", "-fvisibility=hidden",
        "-DKERNEL", "-DKERNEL_PRIVATE", "-DDRIVER_PRIVATE", "-DAPPLE", "-DNeXT",
        "-DIEEE80211_STA_ONLY", "-D__PRIVATE_SPI__",
        "-Wno-deprecated-register", "-Wno-deprecated-declarations", "-Werror=return-type",
        "-Wno-c++17-extensions", "-Wno-macro-redefined", "-Wno-format",
        "-Wno-conversion", "-Wno-sign-conversion", "-Wno-shorten-64-to-32",
        "-Wno-implicit-int-conversion",
        "-I" + str(vendor / "openbsd"), "-I" + str(vendor / "linux"),
        "-I" + str(vendor), "-I" + str(project / "include"),
        "-I" + str(args.sdk / "Headers"),
        "-include", str(vendor / "RTWNet80211Prefix.hpp"),
    ]

    def compile_source(source):
        obj = args.out / (source.name + ".o")
        proc = subprocess.run(flags + ["-c", str(source), "-o", str(obj)],
                              capture_output=True, text=True)
        log = args.out / (source.name + ".log")
        log.write_text(proc.stdout + proc.stderr)
        return {"source": str(source.relative_to(project)), "object": str(obj),
                "status": proc.returncode, "log": str(log)}

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        results = list(pool.map(compile_source, sources))
    (args.out / "compile-results.json").write_text(json.dumps(results, indent=2) + "\n")
    failed = [result for result in results if result["status"]]
    for result in failed:
        print("FAIL " + result["source"], file=sys.stderr)
        print(Path(result["log"]).read_text(), file=sys.stderr)
    if failed:
        return 1
    objects = [result["object"] for result in results]
    subprocess.run(["xcrun", "ld", "-r", "-keep_private_externs", "-arch", "x86_64", "-o",
                    str(args.out / "itl80211-stack.o")] + objects, check=True)
    subprocess.run(["xcrun", "libtool", "-static", "-o",
                    str(args.out / "libitl80211.a")] + objects, check=True)
    undefined = subprocess.run(["xcrun", "nm", "-u", "-j", str(args.out / "itl80211-stack.o")],
                               capture_output=True, text=True, check=True)
    (args.out / "undefined-symbols.txt").write_text(undefined.stdout)
    print(f"Compiled and partial-linked {len(sources)} kernel translation units")
    print(args.out / "itl80211-stack.o")
    return 0


if __name__ == "__main__":
    sys.exit(main())
