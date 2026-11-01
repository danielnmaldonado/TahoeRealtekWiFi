#!/bin/bash
# Native RTL8192EU USB register probe, no legacy kexts or system patches.
# Safe default: READ-ONLY; does not modify USB firmware, SIP, EFI or macOS.
set -u
ROOT="$(cd "$(dirname "$0")" && pwd -P)"
DEST="$ROOT/Release/RTL8192EU-Native"
mkdir -p "$DEST" || exit 2
LOG="$DEST/native-probe.log"
(
  echo 'TahoeRealtekWiFi - RTL8192EU native READ-ONLY USB probe'
  date '+%Y-%m-%d %H:%M:%S %z'
  echo "Project: $ROOT"
  echo 'Device: 2357:0108; no firmware or drivers are installed'
  if [[ "$(uname -s)" != Darwin ]]; then
    echo 'This probe must be run on macOS.'; exit 2
  fi
  if ! command -v cmake >/dev/null 2>&1; then
    echo 'cmake is missing; install build tools manually: brew install cmake pkgconf libusb'; exit 3
  fi
  if ! command -v pkg-config >/dev/null 2>&1 || ! pkg-config --exists libusb-1.0; then
    echo 'libusb not detected by pkg-config. Install manually: brew install pkgconf libusb';exit 3
  fi
  if ! cmake -S "$ROOT" -B "$ROOT/out" -DRTW_ENABLE_LIBUSB=ON -DCMAKE_BUILD_TYPE=Release; then
    echo 'CMake configure failed';exit 4
  fi
  if ! cmake --build "$ROOT/out" --target rtw-8192eu-native --parallel 4; then
    echo 'Native USB tool build failed';exit 4
  fi
  if [[ ! -x "$ROOT/out/rtw-8192eu-native" ]];then
    echo 'Expected executable missing';exit 4
  fi
  echo '=== Register probe: no USB writes ==='
  "$ROOT/out/rtw-8192eu-native" --probe
  code=$?
  echo "Exit code: $code"
  exit "$code"
) >"$LOG" 2>&1
code=$?
cat "$LOG"
echo "Log: $LOG"
echo "Result: $code"
exit "$code"
