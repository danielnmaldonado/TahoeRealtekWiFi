#!/bin/bash
# Build the kext, menu-bar app and tests; does not install or load the kext.
# macOS: double-click or run ./build.command [--all|--kext|--app|--pkg|--tests]
# To disable network access: RTW_OFFLINE=1 ./build.command --kext
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
source "$ROOT/app/signing.sh"
rtw_require_adhoc_identity
RELEASE="$ROOT/Release"
SDK="$ROOT/MacKernelSDK"
SDK_REV="3f750085caa17ec3a7880f11c11bf4f48cd6a164"
PROJECT="$ROOT/TahoeRealtekWiFi.xcodeproj"
SCHEME="TahoeRealtekWiFi"
DERIVED="$RELEASE/.DerivedData"
PRODUCT="$DERIVED/Build/Products/Release/TahoeRealtekWiFi.kext"
OUTPUT="$RELEASE/TahoeRealtekWiFi.kext"
LOG_KEXT="$RELEASE/build-kext.log"
LOG_APP="$RELEASE/build-app.log"
LOG_TESTS="$RELEASE/build-and-test.log"
MODE="${1:---all}"

mkdir -p "$RELEASE"

usage() {
  cat <<'HELP'
TahoeRealtekWiFi - development build (does not install drivers)
  ./build.command           Build the kext, app and tests; write products to Release/
  ./build.command --all     Same as above
  ./build.command --kext    Build only the kext and copy it to Release/
  ./build.command --app     Build only the app and copy it to Release/
  ./build.command --pkg     Build the app and create its .pkg installer (no kext)
  ./build.command --tests   Build the core/tests; also check the app and helper on macOS
  ./build.command --help    Show this help

Kext requirements: Intel Mac, full Xcode, git, and Internet access only when
MacKernelSDK is missing. A pinned revision of the Acidanthera SDK is obtained
in MacKernelSDK/ (ignored by Git).
Use RTW_OFFLINE=1 to disable automatic dependency downloads.

The generated kext is experimental and ad-hoc signed. The package targets Intel
Ventura 13, Sonoma 14, Sequoia 15 and Tahoe 26; each OS needs hardware validation.
Ad-hoc signing does not replace the certificate/consent required to load kexts
on macOS installations with default security policies.
This command does not install, load or change SIP/AMFI.
Apple has not supported IONetworkingFamily + IOUSBHostFamily since Big Sur.
HELP
}

fail() {
  printf '\n[ERROR] %s\n' "$*" >&2
  return 1
}

# Log preflight through tee as well, so Release/build-kext.log exists
# when Xcode is missing, SDK validation fails or the build reports errors.
run_logged() {
  local logfile="$1"
  shift
  set +e
  (set -euo pipefail; "$@") 2>&1 | tee "$logfile"
  local results=("${PIPESTATUS[@]}")
  local status="${results[0]}"
  if [[ "${results[1]}" -ne 0 ]]; then status="${results[1]}"; fi
  set -e
  if [[ "$status" -ne 0 ]]; then
    printf '\n[ERROR] The step failed (code %s). See: %s\n' "$status" "$logfile" >&2
  fi
  return "$status"
}

prepare_sdk() {
  local header="$SDK/Headers/IOKit/usb/IOUSBHostInterface.h"
  local ethernet="$SDK/Headers/IOKit/network/IOEthernetController.h"
  local kmod="$SDK/Library/x86_64/libkmod.a"
  if [[ -f "$header" && -f "$ethernet" && -f "$kmod" ]]; then
    /usr/bin/python3 "$ROOT/scripts/check-mackernelsdk.py" "$SDK" \
      "$ROOT/scripts/MacKernelSDK-SHA256.json" "$SDK_REV" || {
      fail 'MacKernelSDK does not match the pinned revision/content. Its directory was preserved; provide the correct SDK before building.'
      return 1
    }
    printf '[SDK] MacKernelSDK verified: %s\n' "$SDK"
    return 0
  fi

  if [[ -e "$SDK" ]]; then
    fail "MacKernelSDK exists but is incomplete. Expected: $header, $ethernet and $kmod. Its directory was not changed." || return 1
  fi
  if [[ "${RTW_OFFLINE:-0}" == "1" ]]; then
    fail 'MacKernelSDK is missing and RTW_OFFLINE=1. Obtain https://github.com/acidanthera/MacKernelSDK in the MacKernelSDK directory before building.' || return 1
  fi
  if ! command -v git >/dev/null 2>&1; then
    fail 'git is unavailable: install full Xcode / Command Line Tools to obtain MacKernelSDK.' || return 1
  fi

  printf '[SDK] Downloading the pinned MacKernelSDK revision (build dependencies only).\n'
  printf '[SDK] Repository: https://github.com/acidanthera/MacKernelSDK\n'
  printf '[SDK] Commit: %s\n' "$SDK_REV"
  local temp_sdk
  temp_sdk="$(mktemp -d "$ROOT/.MacKernelSDK-download.XXXXXXXX")" || return 1
  if ! (git -C "$temp_sdk" init -q &&
        git -C "$temp_sdk" remote add origin https://github.com/acidanthera/MacKernelSDK.git &&
        git -C "$temp_sdk" -c advice.detachedHead=false fetch --depth 1 origin "$SDK_REV" &&
        git -C "$temp_sdk" -c advice.detachedHead=false checkout -q --detach FETCH_HEAD); then
    rm -rf "$temp_sdk"
    fail 'Could not obtain MacKernelSDK. Check HTTPS/GitHub connectivity or provide the dependency manually in MacKernelSDK/.' || return 1
  fi
  if [[ "$(git -C "$temp_sdk" rev-parse HEAD 2>/dev/null || true)" != "$SDK_REV" ]]; then
    rm -rf "$temp_sdk"
    fail 'The downloaded MacKernelSDK revision does not match the required pinned commit.' || return 1
  fi
  if [[ ! -f "$temp_sdk/Headers/IOKit/usb/IOUSBHostInterface.h" ||
        ! -f "$temp_sdk/Headers/IOKit/network/IOEthernetController.h" ||
        ! -f "$temp_sdk/Library/x86_64/libkmod.a" ]]; then
    rm -rf "$temp_sdk"
    fail 'The downloaded MacKernelSDK revision does not contain all required headers and libkmod.' || return 1
  fi
  if ! /usr/bin/python3 "$ROOT/scripts/check-mackernelsdk.py" "$temp_sdk" \
      "$ROOT/scripts/MacKernelSDK-SHA256.json" "$SDK_REV"; then
    rm -rf "$temp_sdk"
    fail 'The downloaded MacKernelSDK content does not match the pinned manifest.' || return 1
  fi
  if ! mv "$temp_sdk" "$SDK"; then
    rm -rf "$temp_sdk"
    fail 'Could not prepare MacKernelSDK/.' || return 1
  fi
  printf '[SDK] Prepared successfully, without installing anything on the system.\n'
}

preflight_kext() {
  [[ "$(uname -s)" == "Darwin" ]] || { fail 'This x86_64 kext must be built on a Mac with Xcode.'; return 1; }
  command -v xcodebuild >/dev/null 2>&1 || { fail 'xcodebuild was not found. Install full Xcode and select it with xcode-select.'; return 1; }
  command -v xcrun >/dev/null 2>&1 || { fail 'xcrun was not found (Xcode tools are required).'; return 1; }
  command -v lipo >/dev/null 2>&1 || { fail 'lipo was not found.'; return 1; }
  command -v ditto >/dev/null 2>&1 || { fail 'ditto was not found.'; return 1; }
  command -v plutil >/dev/null 2>&1 || { fail 'plutil was not found.'; return 1; }
  command -v codesign >/dev/null 2>&1 || { fail 'codesign was not found. Ad-hoc signing is mandatory.'; return 1; }
  if ! xcodebuild -version; then
    fail 'xcodebuild is unavailable: check that full Xcode is selected (sudo xcode-select -s /Applications/Xcode.app/Contents/Developer).' || return 1
  fi
  if ! xcrun --sdk macosx --show-sdk-path; then
    fail 'The macOS SDK was not found in the selected Xcode.' || return 1
  fi
  [[ -d "$PROJECT" ]] || { fail "Xcode project is missing: $PROJECT"; return 1; }
  prepare_sdk
}

compile_kext() {
  printf '%s\n' '=================================================='
  printf '%s\n' 'TahoeRealtekWiFi - x86_64 kext build'
  printf 'Date: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %z')"
  printf 'Source: %s\n' "$ROOT"
  printf '%s\n' 'NOTICE: experimental ad-hoc binary; radio and physical connectivity validation are pending.'
  printf '%s\n' '=================================================='
  preflight_kext

  # New core sources must be present in both CMake and the Xcode product.
  /usr/bin/python3 "$ROOT/scripts/generate-kext-project.py"

  printf '\n[Xcode] Compilando target %s (Release, x86_64)...\n' "$SCHEME"
  # MacKernelSDK fornece tanto headers legados quanto libkmod.a,
  # vinculado explicitamente no projeto Xcode. Sem instalacao/carregamento.
  xcodebuild -project "$PROJECT" -scheme "$SCHEME" \
    -configuration Release -derivedDataPath "$DERIVED" \
    ARCHS=x86_64 ONLY_ACTIVE_ARCH=NO CODE_SIGNING_ALLOWED=NO \
    build

  printf '\n[Validation] Checking the kext Mach-O binary...\n'
  local executable="$PRODUCT/Contents/MacOS/TahoeRealtekWiFi"
  local info="$PRODUCT/Contents/Info.plist"
  [[ -d "$PRODUCT" ]] || { fail "Xcode reported success but did not produce: $PRODUCT"; return 1; }
  [[ -s "$executable" ]] || { fail "The kext has no executable (not a compiled binary): $executable"; return 1; }
  [[ -s "$info" ]] || { fail "Kext has no Info.plist: $info"; return 1; }
  plutil -lint "$info"
  /usr/bin/python3 "$ROOT/scripts/check-kext-linkage.py" "$executable"
  if ! lipo -archs "$executable" | tr ' ' '\n' | grep -qx x86_64; then
    fail 'The executable has no x86_64 architecture. It will not be copied to Release/.' || return 1
  fi

  # Preserve the previous destination until the new bundle is
  # complete, signed and verified.
  local temporary="$RELEASE/.TahoeRealtekWiFi.kext-new-$$"
  local previous="$RELEASE/.TahoeRealtekWiFi.kext-previous-$$"
  [[ ! -e "$temporary" && ! -L "$temporary" && ! -e "$previous" && ! -L "$previous" ]] || {
    fail 'Build staging already exists for this session; nothing was overwritten.'; return 1;
  }
  if ! ditto "$PRODUCT" "$temporary"; then
    rm -rf "$temporary"
    fail 'Erro ao copiar a kext para Release/.' || return 1
  fi
  if [[ ! -s "$temporary/Contents/MacOS/TahoeRealtekWiFi" ||
        ! -s "$temporary/Contents/Info.plist" ]]; then
    rm -rf "$temporary"
    fail 'The kext copy is incomplete; the previous build was preserved.' || return 1
  fi
  printf '\n[Signing] Ad-hoc signing the kext before publishing...\n'
  if ! rtw_sign_kext "$temporary"; then
    rm -rf "$temporary"
    fail 'Kext ad-hoc signing or verification failed; the previous build was preserved.' || return 1
  fi
  printf '[Signing] Signed-code details:\n'
  codesign -dv --verbose=2 "$temporary" 2>&1
  if [[ -e "$OUTPUT" || -L "$OUTPUT" ]]; then
    [[ -d "$OUTPUT" && ! -L "$OUTPUT" ]] || {
      rm -rf "$temporary"
      fail 'The kext destination exists and is not a regular bundle. Nothing was overwritten.'; return 1
    }
    if ! mv "$OUTPUT" "$previous"; then
      rm -rf "$temporary";fail 'Could not preserve the previous kext.';return 1
    fi
  fi
  if ! mv "$temporary" "$OUTPUT"; then
    if [[ -d "$previous" && ! -e "$OUTPUT" ]]; then mv "$previous" "$OUTPUT"; fi
    rm -rf "$temporary"
    fail 'Publishing failed; the previous kext was restored.' || return 1
  fi
  if [[ -d "$previous" ]]; then rm -rf "$previous"; fi
  printf '\n[OK] KEXT BUILT: %s\n' "$OUTPUT"
  printf '[OK] Signature: ad-hoc (kext)\n'
  printf '[OK] Executable: %s\n' "$OUTPUT/Contents/MacOS/TahoeRealtekWiFi"
  if command -v shasum >/dev/null 2>&1; then
    shasum -a 256 "$OUTPUT/Contents/MacOS/TahoeRealtekWiFi" "$OUTPUT/Contents/Info.plist"
  fi
  printf '\nIMPORTANT: a successful build/ad-hoc signature does not guarantee kext loading on Tahoe.\n'
  printf 'It was not installed, loaded or tested on the USB adapter.\n'

}

compile_app() {
  bash "$ROOT/app/build.command" "$RELEASE"
}

compile_tests() {
  printf '%s\n' 'TahoeRealtekWiFi - suites CMake (sem kext)'
  bash "$ROOT/scripts/build-and-test.sh"
}

case "$MODE" in
  --pkg|pkg)
    bash "$ROOT/app/package.command"
    ;;
  --help|-h|help) usage ;;
  --tests|tests)
    run_logged "$LOG_TESTS" compile_tests
    ;;
  --kext|kext)
    run_logged "$LOG_KEXT" compile_kext
    ;;
  --app|app)
    compile_app
    ;;
  --all|all)
    # Even if the kext fails, try building the app independently of MacKernelSDK.
    local_kext_status=0
    local_app_status=0
    # Fresh shell processes preserve errexit inside each stage. Calling a
    # function on the left of "||" disables errexit in its nested functions.
    set +e
    bash "$ROOT/build.command" --kext
    local_kext_status=$?
    bash "$ROOT/build.command" --app
    local_app_status=$?
    set -e
    if [[ "$local_kext_status" -ne 0 || "$local_app_status" -ne 0 ]]; then
      printf '\n[ERROR] Failure(s): kext=%s app=%s. Check the logs in %s\n' \
        "$local_kext_status" "$local_app_status" "$RELEASE" >&2
      exit 1
    fi
    run_logged "$LOG_TESTS" compile_tests
    printf '\n[OK] Kext at: %s\n' "$OUTPUT"
    printf '[OK] App at: %s\n' "$RELEASE/Tahoe Wi-Fi USB.app"
    printf '[OK] Logs: %s, %s and %s\n' "$LOG_KEXT" "$LOG_APP" "$LOG_TESTS"
    ;;
  *)
    usage >&2
    exit 2
    ;;
esac
