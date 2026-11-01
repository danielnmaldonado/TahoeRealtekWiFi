#!/bin/bash
# Read-only, quick RTL8192EU identification. No macOS unified-log searches.
# SPDX-License-Identifier: GPL-2.0-only
set -uo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/Release/RTL8192EU-QuickProbe"
mkdir -p "$OUT" || exit 1
STAMP="$(date '+%Y%m%d-%H%M%S')"
REPORT="$OUT/RTL8192EU-$STAMP.txt"
ARCHIVE="$OUT/RTL8192EU-$STAMP.zip"
{
  printf 'RTL8192EU QUICK PROBE (READ ONLY)\n'
  printf 'Timestamp: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %z')"
  printf 'Target: USB VID:PID 2357:0108\n'
  printf 'No hardware writes, no driver installation, no SIP changes.\n\n'
  if command -v sw_vers >/dev/null 2>&1; then
    sw_vers 2>&1
  fi
  printf '\nUSB host devices (IDs and non-serial metadata):\n'
  if command -v ioreg >/dev/null 2>&1; then
    # VID/PID and bcdDevice are normally properties of IOUSBHostDevice,
    # NOT IOUSBHostInterface. Query both to avoid an empty ID report.
    ioreg -r -c IOUSBHostDevice -l -w0 2>&1 |
      grep -Ei '^[[:space:]]*[| +`\\-]*\+-o |idVendor|idProduct|bcdDevice|USB Product Name' || true
    printf '\nUSB host interfaces (class, subclass, protocol, endpoint count):\n'
    ioreg -r -c IOUSBHostInterface -l -w0 2>&1 |
      grep -Ei '^[[:space:]]*[| +`\\-]*\+-o |bNumEndpoints|bInterface(Class|SubClass|Protocol|Number)' || true
  else
    printf 'ioreg unavailable on this host.\n'
  fi
  printf '\nRTL8192EU register probe (4 read-only USB vendor requests):\n'
  if [ -x "$ROOT/out/rtw-probe" ]; then
    "$ROOT/out/rtw-probe" --read-8192eu 2>&1 || printf 'Register probe did not complete; USB access or libusb may be unavailable.\n'
  else
    printf 'Not built. On macOS with Xcode, CMake and libusb installed:\n'
    printf '  cmake -S . -B out -DRTW_ENABLE_LIBUSB=ON\n'
    printf '  cmake --build out --target rtw-probe\n'
    printf '  ./out/rtw-probe --read-8192eu\n'
  fi
  printf '\nIMPORTANT: The USB ID matches RTL8192EU reference devices, but silicon cut, bonding and actual radio support require device-level verification.\n'
} > "$REPORT"
if command -v ditto >/dev/null 2>&1; then
  ditto -c -k --keepParent "$REPORT" "$ARCHIVE" >/dev/null 2>&1 || true
elif command -v zip >/dev/null 2>&1; then
  zip -q -j "$ARCHIVE" "$REPORT" || true
fi
printf '\nQuick USB probe completed.\nReport: %s\n' "$REPORT"
if [ -s "$ARCHIVE" ]; then printf 'ZIP: %s\n' "$ARCHIVE"; fi
printf 'This collector does NOT search 24-hour system logs.\n'
if [ -t 0 ]; then
  printf '\nPress Enter to close...'
  read -r _ || true
fi
