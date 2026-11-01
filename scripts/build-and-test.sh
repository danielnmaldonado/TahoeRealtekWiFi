#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-only
set -eu
PROJECT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
JOBS=${RTW_BUILD_JOBS:-4}
case "$JOBS" in ''|*[!0-9]*) printf '%s\n' "RTW_BUILD_JOBS must be an integer from 1 to 64." >&2; exit 2;; esac
if [ "$JOBS" -lt 1 ] || [ "$JOBS" -gt 64 ]; then
    printf '%s\n' "RTW_BUILD_JOBS must be an integer from 1 to 64." >&2
    exit 2
fi
SANITIZERS=${RTW_HOST_SANITIZERS:-address,undefined}
case "$SANITIZERS" in
    address,undefined|undefined) ;;
    *) printf '%s\n' "RTW_HOST_SANITIZERS must be address,undefined or undefined." >&2; exit 2;;
esac
BUILD_DIR=${RTW_BUILD_DIR:-"$PROJECT_DIR/out"}
cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
    -DRTW_ENABLE_SANITIZERS=ON "-DRTW_HOST_SANITIZERS=$SANITIZERS"
cmake --build "$BUILD_DIR" --parallel "$JOBS"
ctest --test-dir "$BUILD_DIR" --output-on-failure --parallel "$JOBS"
printf '\nRead-only hardware probe (macOS): %s/rtw-probe --all\n' "$BUILD_DIR"
