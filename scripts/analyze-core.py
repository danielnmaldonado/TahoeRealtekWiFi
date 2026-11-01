#!/usr/bin/env python3
"""Analyze the portable production core with Clang; no native ABI claim."""
from __future__ import annotations
import argparse
import concurrent.futures
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=ROOT / "out" / "static-analysis")
    parser.add_argument("--jobs", type=int, default=4)
    args = parser.parse_args()
    if not 1 <= args.jobs <= 64:
        parser.error("--jobs must be between 1 and 64")
    args.out.mkdir(parents=True, exist_ok=True)
    compilers = {
        ".c": shlex.split(os.environ.get("CLANG", "clang")),
        ".cpp": shlex.split(os.environ.get("CLANGXX", "clang++")),
    }
    for command in compilers.values():
        if not command or not shutil.which(command[0]):
            parser.error("Clang is required; set CLANG and CLANGXX when necessary")
    files = sorted((*ROOT.joinpath("src").glob("*.c"),
                    *ROOT.joinpath("src").glob("*.cpp")))

    def analyze(path: Path) -> dict:
        report = args.out / (path.name + ".plist")
        command = compilers[path.suffix] + [
            "--analyze", "-Xanalyzer", "-analyzer-output=plist",
            "-std=c++17" if path.suffix == ".cpp" else "-std=c11",
            "-I" + str(ROOT / "include"), str(path), "-o", str(report)]
        try:
            result = subprocess.run(command, capture_output=True, text=True,
                                    timeout=120, check=False)
            return {"file": str(path.relative_to(ROOT)),
                    "returncode": result.returncode,
                    "diagnostics": result.stdout + result.stderr}
        except (OSError, subprocess.TimeoutExpired) as error:
            return {"file": str(path.relative_to(ROOT)), "returncode": 1,
                    "diagnostics": str(error)}

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(analyze, files))
    (args.out / "results.json").write_text(json.dumps(results, indent=2) + "\n",
                                         encoding="utf-8")
    for result in results:
        if result["diagnostics"]:
            print(result["file"] + ": " + result["diagnostics"], file=sys.stderr)
    failed = any(r["returncode"] or r["diagnostics"].strip() for r in results)
    print(f"Analyzed {len(results)} portable source units; "
          f"{'review diagnostics' if failed else 'no diagnostics'}.")
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
