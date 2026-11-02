#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Verify the pinned SDK payload offline, including SDKs extracted without Git."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys


def validate(sdk, manifest_path, revision):
    manifest = json.loads(manifest_path.read_text())
    if manifest.get("revision") != revision:
        raise ValueError("SDK manifest revision does not match the required revision")
    expected = manifest.get("files")
    if not isinstance(expected, dict) or not expected:
        raise ValueError("SDK manifest has no payload files")
    for relative in expected:
        path = PurePosixPath(relative)
        if (path.is_absolute() or ".." in path.parts or
                not path.parts or path.parts[0] not in ("Headers", "Library")):
            raise ValueError("SDK manifest contains an invalid payload path")
    if not sdk.is_dir():
        raise ValueError("MacKernelSDK directory is missing")
    if (sdk / ".git").exists():
        result = subprocess.run(["git", "-C", str(sdk), "rev-parse", "HEAD"],
                                capture_output=True, text=True, check=False)
        if result.returncode or result.stdout.strip() != revision:
            raise ValueError("MacKernelSDK Git revision differs from the required revision")
    actual = set()
    for area in ("Headers", "Library"):
        directory = sdk / area
        if directory.is_symlink():
            raise ValueError("SDK payload directory is a symbolic link: " + area)
        for folder, directories, files in os.walk(directory):
            for name in directories + files:
                path = Path(folder) / name
                if path.is_symlink():
                    raise ValueError("SDK payload contains a symbolic link: " +
                                     str(path.relative_to(sdk)))
            actual.update(str((Path(folder) / name).relative_to(sdk)) for name in files)
    if actual != set(expected):
        missing = sorted(set(expected) - actual)
        extra = sorted(actual - set(expected))
        detail = ("missing " + missing[0]) if missing else ("unexpected " + extra[0])
        raise ValueError("SDK payload differs from the pinned manifest: " + detail)
    for relative, digest in expected.items():
        if hashlib.sha256((sdk / relative).read_bytes()).hexdigest() != digest:
            raise ValueError("SDK payload SHA256 mismatch: " + relative)
    print("[SDK] Verified payload for revision " + revision + "; " +
          str(len(expected)) + " pinned header/library files")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sdk", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("revision")
    args = parser.parse_args()
    try:
        validate(args.sdk, args.manifest, args.revision)
    except (OSError, ValueError) as error:
        print("[SDK] " + str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
