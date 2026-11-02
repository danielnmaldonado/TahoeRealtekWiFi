#!/bin/bash
# Build the complete ad-hoc signed app alongside the kext in Release/.
# Alternative destination (compatibility): ./app/build.command /path/to/directory
# --check-only [directory]: build/sign/verify a disposable bundle without publishing.
# This script does not install or load the kext or app.
set -euo pipefail

APP_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$APP_DIR/.." && pwd)"
source "$APP_DIR/signing.sh"
rtw_require_adhoc_identity
CHECK_ONLY=0
if [[ "${1:-}" == "--check-only" ]]; then
  CHECK_ONLY=1
  shift
fi
DESTINATION="${1:-$ROOT/Release}"

if [[ $# -gt 1 ]]; then
  printf '[ERROR] Usage: %s [--check-only] [output-directory]\n' "$0" >&2
  exit 2
fi
mkdir -p "$DESTINATION"
DESTINATION="$(cd "$DESTINATION" && pwd)"
LOG="$DESTINATION/build-app.log"
if [[ "$CHECK_ONLY" -eq 1 ]]; then LOG="$DESTINATION/build-app-check.log"; fi
BUNDLE="$DESTINATION/Tahoe Wi-Fi USB.app"
[[ ! -L "$LOG" && ( ! -e "$LOG" || -f "$LOG" ) ]] || {
  printf '[ERROR] Unexpected build log destination: %s\n' "$LOG" >&2
  exit 1
}

build_app() (
  cd "$APP_DIR"
  printf 'Tahoe Wi-Fi USB v0.3.11 - macOS build (Intel x86_64)\n'
  printf 'Source: %s\nDestination: %s\nLog: %s\n' "$ROOT" "$BUNDLE" "$LOG"
  printf 'Started: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %z')"

  [[ "$(uname -s)" == "Darwin" ]] || { printf '[ERROR] This AppKit application must be built on macOS.\n' >&2; return 1; }
  local required_tools=(xcrun iconutil codesign plutil lipo python3)
  for command_name in "${required_tools[@]}"; do
    command -v "$command_name" >/dev/null 2>&1 || {
      printf '[ERROR] Missing tool: %s (install/select the full Xcode application).\n' "$command_name" >&2
      return 1
    }
  done
  xcrun --find clang++ >/dev/null || return 1
  xcrun --find swift >/dev/null || return 1
  xcrun --sdk macosx --show-sdk-path >/dev/null || return 1

  # Preserve the old bundle if compilation, icon generation or signing fails.
  local staging previous staged_bundle iconset
  staging="$(mktemp -d "$DESTINATION/.TahoeWiFi-build.XXXXXXXX")" || return 1
  previous="$DESTINATION/.Tahoe-WiFi-USB-previous.$$"
  staged_bundle="$staging/Tahoe Wi-Fi USB.app"
  iconset="$staging/AppIcon.iconset"
  trap 'if [[ -d "$previous" && ! -e "$BUNDLE" ]]; then mv "$previous" "$BUNDLE"; fi; rm -rf "$staging"' EXIT

  mkdir -p "$staged_bundle/Contents/MacOS" "$staged_bundle/Contents/Helpers" "$staged_bundle/Contents/Resources" "$staged_bundle/Contents/Library/LaunchDaemons"
  local flags=(-std=c++17 -O2 -arch x86_64 -mmacosx-version-min=13.0 -Wall -Wextra -Werror -Werror=unguarded-availability -Werror=unguarded-availability-new -I. -I../kext -I../include)

  printf '\n[1/5] Building the privileged helper...\n'
  xcrun clang++ "${flags[@]}" -fobjc-arc PrivilegedService.mm \
    -framework Foundation -framework IOKit -framework CoreFoundation -framework Security -framework SystemConfiguration \
    -o "$staged_bundle/Contents/Helpers/TahoeWiFiHelper"

  printf '\n[2/5] Building the menu-bar application...\n'
  xcrun clang++ "${flags[@]}" -fobjc-arc Main.mm LoginItem.mm CredentialStore.mm DriverConnection.mm ControlDiagnostics.mm DiagnosticCLI.mm AdapterDiagnostics.mm USBWiFiProfiles.cpp Transport.cpp \
    -framework AppKit -framework ServiceManagement -framework Security -framework CoreServices -framework IOKit -framework CoreFoundation \
    -o "$staged_bundle/Contents/MacOS/TahoeUSBWiFi"

  cp Info.plist "$staged_bundle/Contents/Info.plist"
  cp org.research.TahoeUSBWiFi.driver.plist "$staged_bundle/Contents/Library/LaunchDaemons/"
  cp "$ROOT/LICENSE-GPL-2.0.txt" "$staged_bundle/Contents/Resources/LICENSE-GPL-2.0.txt"

  printf '\n[3/5] Generating the icon...\n'
  xcrun swift -target x86_64-apple-macosx13.0 -warnings-as-errors icon.swift "$iconset"
  iconutil -c icns "$iconset" -o "$staged_bundle/Contents/Resources/AppIcon.icns"

  printf '\n[4/5] Signing the application and helper...\n'
  rtw_sign_app "$staged_bundle"

  printf '\n[5/5] Validating the bundle before publishing...\n'
  [[ -s "$staged_bundle/Contents/MacOS/TahoeUSBWiFi" && \
     -s "$staged_bundle/Contents/Helpers/TahoeWiFiHelper" && \
     -s "$staged_bundle/Contents/Resources/AppIcon.icns" && \
     -s "$staged_bundle/Contents/Info.plist" ]] || {
    printf '[ERROR] Incomplete bundle. The previous application was preserved.\n' >&2
    return 1
  }
  lipo -archs "$staged_bundle/Contents/MacOS/TahoeUSBWiFi" | tr ' ' '\n' | grep -qx x86_64 || {
    printf '[ERROR] The application executable does not contain the x86_64 architecture.\n' >&2
    return 1
  }
  lipo -archs "$staged_bundle/Contents/Helpers/TahoeWiFiHelper" | tr ' ' '\n' | grep -qx x86_64 || {
    printf '[ERROR] The helper does not contain the x86_64 architecture.\n' >&2
    return 1
  }
  plutil -lint "$staged_bundle/Contents/Info.plist"
  plutil -lint "$staged_bundle/Contents/Library/LaunchDaemons/org.research.TahoeUSBWiFi.driver.plist"
  rtw_verify_app "$staged_bundle"
  # Validate the actual staged executables before replacing the previous app.
  python3 "$APP_DIR/check-macos-compatibility.py" --max-minos 13.0 \
    "$staged_bundle/Contents/MacOS/TahoeUSBWiFi" \
    "$staged_bundle/Contents/Helpers/TahoeWiFiHelper"
  if [[ "$CHECK_ONLY" -eq 1 ]]; then
    printf '\n[OK] Native compilation, ad-hoc signing and verification completed.\n'
    printf '[OK] Check only: the temporary signed bundle is removed; nothing was published or installed.\n'
    return 0
  fi

  # Move within the same directory/volume; restore the old bundle if replacement fails.
  if [[ -e "$BUNDLE" || -L "$BUNDLE" ]]; then
    [[ -d "$BUNDLE" && ! -L "$BUNDLE" ]] || {
      printf '[ERROR] The destination exists and is not a regular .app directory: %s\n' "$BUNDLE" >&2
      return 1
    }
    [[ ! -e "$previous" && ! -L "$previous" ]] || {
      printf '[ERROR] The previous temporary copy still exists: %s\n' "$previous" >&2
      return 1
    }
    mv "$BUNDLE" "$previous"
  fi
  if ! mv "$staged_bundle" "$BUNDLE"; then
    printf '[ERROR] Could not publish the new .app; restoring the previous version.\n' >&2
    if [[ -d "$previous" && ! -e "$BUNDLE" ]]; then mv "$previous" "$BUNDLE"; fi
    return 1
  fi
  if [[ -d "$previous" ]]; then rm -rf "$previous"; fi
  printf '\n[OK] APP BUILT: %s\n' "$BUNDLE"
  printf '[OK] Signature: verified ad-hoc (application and helper)\n'
  printf '[OK] Log: %s\n' "$LOG"
  printf 'The application was not installed in /Applications.\n'
)

# Log preflight and compiler errors even if the build does not finish.
set +e
(set -euo pipefail; build_app) 2>&1 | tee "$LOG"
results=("${PIPESTATUS[@]}")
set -e
if [[ "${results[0]}" -ne 0 || "${results[1]}" -ne 0 ]]; then
  printf '\n[ERROR] Application build/verification failed. See: %s\n' "$LOG" >&2
  exit 1
fi
