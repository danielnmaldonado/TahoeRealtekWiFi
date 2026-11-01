#!/usr/bin/env python3
"""Regression tests for the offline RTL8812AU/RTL8821AU capture CLI."""
import argparse
import binascii
import pathlib
import struct
import subprocess
import tempfile


def make_capture(with_fcs: bool, corrupt_fcs: bool = False) -> bytes:
    mac = bytes.fromhex("023344556677")
    frame = bytearray(36)
    frame[0] = 0x80
    frame[4:10] = b"\xff" * 6
    frame[10:16] = mac
    frame[16:22] = mac
    frame[32:34] = struct.pack("<H", 100)
    frame[34:36] = struct.pack("<H", 1)
    frame += bytes([0, 5]) + b"Tahoe"
    frame += bytes([1, 8, 0x82, 0x84, 0x8b, 0x96, 12, 18, 24, 36])
    frame += bytes([3, 1, 6])
    if with_fcs:
        crc = binascii.crc32(frame) & 0xFFFFFFFF
        frame += struct.pack("<I", crc ^ (1 if corrupt_fcs else 0))
    rx = bytearray(24)
    rx[0:4] = struct.pack("<I", len(frame))
    rx[12:16] = struct.pack("<I", 4)
    packet = rx + frame
    packet += bytes((-len(packet)) % 8)
    return bytes(packet)


def check(args: list[str], expected_code: int, required: str = "",
          forbidden: str = "") -> None:
    result = subprocess.run(args, capture_output=True, text=True, timeout=10)
    assert result.returncode == expected_code, (
        f"args={args}\nrc={result.returncode}\n"
        f"stdout={result.stdout}\nstderr={result.stderr}"
    )
    assert required in result.stdout, (required, result.stdout)
    if forbidden:
        assert forbidden not in result.stdout, (forbidden, result.stdout)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--decoder", required=True)
    args = parser.parse_args()
    exe = str(pathlib.Path(args.decoder).resolve())
    with tempfile.TemporaryDirectory(prefix="rtw-r12au-cli-") as dirname:
        path = pathlib.Path(dirname) / "capture.bin"
        path.write_bytes(make_capture(False))
        check([exe, "rtl8812au", str(path), "--channel", "6"],
              0, "security=open ssid_hex=5461686f65")
        check([exe, "rtl8821au", str(path), "--channel", "6"],
              0, "beacon_channel=6")
        check([exe, "rtl8812au", str(path), "--channel", "11"],
              0, "frames=1 errors=0", "beacon_channel")
        check([exe, "rtl8812au", str(path)],
              0, "frames=1 errors=0", "beacon_channel")
        check([exe, "rtl8812au", str(path), "--fcs"],
              2)
        check([exe, "rtl8812au", str(path), "--channel", "0"],
              2)
        check([exe, "rtl8812au", str(path), "--channel", "6", "--channel", "6"],
              2)
        check([exe, "rtl8812au", str(path), "--channel", "xyz"],
              2)
        path.write_bytes(make_capture(True))
        check([exe, "rtl8812au", str(path), "--channel", "6", "--fcs"],
              0, "security=open ssid_hex=5461686f65")
        path.write_bytes(make_capture(True, True))
        check([exe, "rtl8812au", str(path), "--channel", "6", "--fcs"],
              0, "frames=1 errors=0", "beacon_channel")
        path.write_bytes(b"\x01" * 24)
        check([exe, "rtl8812au", str(path), "--channel", "6"],
              1)
    print("RTL8812AU/RTL8821AU capture CLI regression passed (11 cases)")


if __name__ == "__main__":
    main()
