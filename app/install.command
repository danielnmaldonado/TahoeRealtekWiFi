#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Open the canonical app-only Installer; never duplicate privileged copy logic.
set -euo pipefail
APP_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$APP_DIR/.." && pwd)"
[[ $# -eq 0 ]] || { printf '[ERROR] Usage: bash ./app/install.command\n' >&2; exit 2; }
[[ "$(/usr/bin/uname -s)" == Darwin ]] || { printf '[ERROR] Requires macOS.\n' >&2; exit 1; }
[[ "$(/usr/bin/id -u)" != 0 ]] || { printf '[ERROR] Run without sudo; Installer requests authorization.\n' >&2; exit 1; }
BUNDLE="$ROOT/Release/Tahoe Wi-Fi USB.app"
[[ -d "$BUNDLE" && ! -L "$BUNDLE" ]] || {
    printf '[ERROR] Build first: bash ./build.command --app\n' >&2
    exit 1
}
# The package builder verifies all signatures, structure and supported metadata.
bash "$APP_DIR/package.command" --no-build
VERSION=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$BUNDLE/Contents/Info.plist")
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || exit 1
PACKAGE="$ROOT/Release/Tahoe-WiFi-USB-$VERSION.pkg"
[[ -f "$PACKAGE" && ! -L "$PACKAGE" ]] || exit 1
/usr/bin/open "$PACKAGE"
printf 'Opened Installer: %s\n' "$PACKAGE"
printf 'Installation occurs only after you complete Installer. The kernel extension is not included.\n'
