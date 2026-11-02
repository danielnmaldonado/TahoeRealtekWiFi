#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Check Intel Mach-O deployment commands, independently of bundle metadata.

This verifies build compatibility only. It does not prove execution on an older
OS, API behavior, driver operation, or physical adapter support.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys


class MachOError(ValueError):
    pass


def version_tuple(text):
    parts = text.split(".")
    if not 1 <= len(parts) <= 3 or any(not part.isascii() or not part.isdecimal() for part in parts):
        raise ValueError("expected a macOS version such as 13.0")
    values = tuple(int(part) for part in parts)
    if values[0] < 1 or values[0] > 65535 or any(value > 255 for value in values[1:]):
        raise ValueError("macOS version is out of range")
    return values + (0,) * (3 - len(values))


def version_text(value):
    major, minor, patch = value >> 16, (value >> 8) & 255, value & 255
    return "%d.%d%s" % (major, minor, ".%d" % patch if patch else "")


def thin_slice(data, expected_cpu=None):
    orders = {b"\xcf\xfa\xed\xfe":"<", b"\xfe\xed\xfa\xcf":">"}
    order = orders.get(data[:4])
    if order is None or len(data) < 32:
        raise MachOError("invalid or truncated 64-bit Mach-O header")
    _, cpu, _, filetype, count, size, _, _ = struct.unpack_from(order + "8I", data)
    if expected_cpu is not None and cpu != expected_cpu:
        raise MachOError("Mach-O fat architecture does not match its slice")
    if cpu != 0x01000007:
        raise MachOError("Mach-O must contain only Intel x86_64 slices")
    end = 32 + size
    if end > len(data) or count > size // 8:
        raise MachOError("truncated Mach-O load commands")
    offset = 32
    deployment = None
    for _ in range(count):
        if offset + 8 > end:
            raise MachOError("truncated Mach-O load command")
        command, length = struct.unpack_from(order + "2I", data, offset)
        if length < 8 or length % 8 or offset + length > end:
            raise MachOError("invalid Mach-O load command size")
        foreign = {0x25:"LC_VERSION_MIN_IPHONEOS", 0x2F:"LC_VERSION_MIN_TVOS",
            0x30:"LC_VERSION_MIN_WATCHOS"}.get(command)
        if foreign is not None:
            raise MachOError("Mach-O deployment command %s is not macOS" % foreign)
        if command in (0x24, 0x32):
            if deployment is not None:
                raise MachOError("duplicate Mach-O deployment commands")
            if command == 0x32:
                if length < 24:
                    raise MachOError("short Mach-O LC_BUILD_VERSION")
                platform, minimum, sdk, tools = struct.unpack_from(order + "4I", data, offset + 8)
                if length != 24 + 8 * tools:
                    raise MachOError("invalid Mach-O LC_BUILD_VERSION tools")
                if platform != 1:
                    raise MachOError("Mach-O platform must be macOS")
                name = "LC_BUILD_VERSION"
            else:
                if length != 16:
                    raise MachOError("invalid Mach-O LC_VERSION_MIN_MACOSX")
                minimum, sdk = struct.unpack_from(order + "2I", data, offset + 8)
                name = "LC_VERSION_MIN_MACOSX"
            if minimum >> 16 == 0:
                raise MachOError("invalid Mach-O minimum macOS version")
            deployment = {"arch":"x86_64", "filetype":{2:"executable", 11:"kext"}.get(filetype, str(filetype)),
                "platform":"macOS", "minos":version_text(minimum), "sdk":version_text(sdk), "command":name}
        offset += length
    if offset != end or deployment is None:
        raise MachOError("Mach-O must have exactly one complete macOS deployment command")
    return deployment


def macho_slices(data):
    fat_formats = {b"\xca\xfe\xba\xbe":(">", False), b"\xbe\xba\xfe\xca":("<", False),
        b"\xca\xfe\xba\xbf":(">", True), b"\xbf\xba\xfe\xca":("<", True)}
    fat = fat_formats.get(data[:4])
    if fat is None:
        return [thin_slice(data)]
    order, wide = fat
    if len(data) < 8:
        raise MachOError("truncated Mach-O fat header")
    count = struct.unpack_from(order + "I", data, 4)[0]
    entry_size = 32 if wide else 20
    table_end = 8 + entry_size * count
    if count == 0 or count > 128 or table_end > len(data):
        raise MachOError("invalid Mach-O fat architecture table")
    ranges = []
    result = []
    for index in range(count):
        entry = struct.unpack_from(order + ("2I2Q2I" if wide else "5I"), data, 8 + index * entry_size)
        cpu, _, offset, size, alignment = entry[:5]
        if (alignment > 63 or offset % (1 << alignment) or offset < table_end or
                size < 32 or offset + size > len(data) or
                any(offset < end and start < offset + size for start, end in ranges)):
            raise MachOError("invalid or truncated Mach-O fat slice")
        ranges.append((offset, offset + size))
        result.append(thin_slice(data[offset:offset + size], cpu))
    if len(result) != 1:
        raise MachOError("Mach-O must contain one Intel x86_64 slice")
    return result


def inspect_macho(path, max_minos="13.0", filetype="executable"):
    """Return deployment evidence or raise MachOError/OSError; never run a binary."""
    slices = macho_slices(Path(path).read_bytes())
    limit = version_tuple(max_minos)
    for item in slices:
        if item["filetype"] != filetype:
            raise MachOError("Mach-O must be a %s (found %s)" % (filetype, item["filetype"]))
        if version_tuple(item["minos"]) > limit:
            raise MachOError("Mach-O minimum macOS %s exceeds allowed %s" % (item["minos"], max_minos))
    return slices


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max-minos", default="13.0", help="highest allowed deployment minimum (default: 13.0)")
    parser.add_argument("--filetype", choices=("executable", "kext"), default="executable")
    parser.add_argument("--json", action="store_true", help="write machine-readable deployment evidence")
    parser.add_argument("paths", nargs="+", type=Path)
    args = parser.parse_args(argv)
    try:
        version_tuple(args.max_minos)
    except ValueError as error:
        parser.error(str(error))
    report = {"compatible":True, "max_minos":args.max_minos, "files":[]}
    for path in args.paths:
        item = {"path":str(path)}
        try:
            item["slices"] = inspect_macho(path, args.max_minos, args.filetype)
        except (OSError, MachOError) as error:
            item["error"] = str(error)
            report["compatible"] = False
            print("[ERROR] %s: %s" % (path, error), file=sys.stderr)
        report["files"].append(item)
    if args.json:
        print(json.dumps(report, indent=2))
    elif report["compatible"]:
        for item in report["files"]:
            for arch in item["slices"]:
                print("[OK] %s: %s macOS minimum %s (SDK %s)" % (item["path"], arch["arch"], arch["minos"], arch["sdk"]))
    return 0 if report["compatible"] else 1


if __name__ == "__main__":
    sys.exit(main())
