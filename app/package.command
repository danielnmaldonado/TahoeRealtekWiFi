#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Build an app-only macOS Installer product. Never installs or loads anything.
set -euo pipefail
APP_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$APP_DIR/.." && pwd)"
source "$APP_DIR/signing.sh"
OUTPUT="$ROOT/Release"
SOURCE_APP=""
BUILD=1
INSTALLER_IDENTITY="${RTW_INSTALLER_SIGN_IDENTITY:-}"

usage() {
    cat <<'HELP'
Tahoe Wi-Fi USB - .pkg builder (application only)
  bash ./app/package.command                      Builds the app and creates the .pkg.
  bash ./app/package.command --no-build           Packages the app from Release/.
  bash ./app/package.command --app "/folder/Tahoe Wi-Fi USB.app"
                                                  Packages this app without rebuilding.
  bash ./app/package.command --output "/folder"     Selects the output directory.
  bash ./app/package.command --help               Displays this help.

Do not use sudo. The .pkg requests authorization only when it is installed.
The package installs the complete app in /Applications, without installing the kernel extension.
Targets Intel Macs with macOS Ventura 13, Sonoma 14, Sequoia 15 or Tahoe 26.
Building requires Xcode with the macOS 13 SDK or later and Python 3.
The packaged app must be version 0.2.5 or later.

The app and helper must have verified ad-hoc signatures; this is mandatory.
RTW_SIGN_IDENTITY is accepted only when unset or "-".
RTW_NOTARY_PROFILE is rejected: this is not a Developer ID app release.
RTW_INSTALLER_SIGN_IDENTITY may sign the .pkg container with a Developer ID
Installer certificate. It does not replace the app/helper ad-hoc signatures.
Without that certificate the .pkg container is unsigned. No notarization is claimed.
HELP
}
fail() { printf '[ERROR] %s\n' "$*" >&2; exit 1; }
while [[ $# -gt 0 ]]; do
    case "$1" in
        --help|-h) usage; exit 0 ;;
        --no-build) BUILD=0; shift ;;
        --app|--output)
            option="$1"
            [[ $# -ge 2 && -n "$2" && "$2" != --* ]] || fail "Missing path for $option."
            if [[ "$option" == --app ]]; then SOURCE_APP="$2"; BUILD=0; else OUTPUT="$2"; fi
            shift 2 ;;
        *) fail "Unknown option: $1. Use --help." ;;
    esac
done
rtw_require_adhoc_identity
[[ "$(/usr/bin/uname -s)" == Darwin ]] || fail "The .pkg must be built on macOS with pkgbuild/productbuild."
[[ "$(/usr/bin/id -u)" != 0 ]] || fail "Run without sudo; only Installer needs administrative authorization."
for tool in /usr/bin/pkgbuild /usr/bin/productbuild /usr/sbin/pkgutil /usr/bin/ditto /usr/bin/python3 \
    /usr/bin/codesign /usr/bin/plutil /usr/bin/lipo /usr/libexec/PlistBuddy; do
    [[ -x "$tool" ]] || fail "Missing tool: $tool."
done
mkdir -p "$OUTPUT"
OUTPUT="$(cd "$OUTPUT" && pwd)"
LOG="$OUTPUT/build-pkg.log"
[[ ! -L "$LOG" && ( ! -e "$LOG" || -f "$LOG" ) ]] || fail "Unexpected log destination: $LOG."
if [[ -z "$SOURCE_APP" ]]; then SOURCE_APP="$ROOT/Release/Tahoe Wi-Fi USB.app"; fi

package_app() (
    printf 'Tahoe Wi-Fi USB - application-only .pkg\n'
    printf 'Started: %s\n' "$(date '+%Y-%m-%d %H:%M:%S %z')"
    if [[ "$BUILD" -eq 1 ]]; then
        /bin/bash "$APP_DIR/build.command" "$ROOT/Release"
    fi
    [[ -d "$SOURCE_APP" && ! -L "$SOURCE_APP" ]] || fail "Built app not found: $SOURCE_APP."
    SOURCE_APP="$(cd "$(dirname "$SOURCE_APP")" && pwd)/$(basename "$SOURCE_APP")"
    local invalid relative version minimum bundle_id build_number installer_protocol
    invalid=$(/usr/bin/find "$SOURCE_APP" ! -type f ! -type d -print -quit)
    [[ -z "$invalid" ]] || fail "Bundle contains links or special file types: $invalid."
    for relative in Contents/Info.plist Contents/MacOS/TahoeUSBWiFi \
        Contents/Helpers/TahoeWiFiHelper \
        Contents/Library/LaunchDaemons/org.research.TahoeUSBWiFi.driver.plist \
        Contents/_CodeSignature/CodeResources Contents/Resources/AppIcon.icns; do
        [[ -f "$SOURCE_APP/$relative" && -s "$SOURCE_APP/$relative" ]] ||
            fail "Incomplete app: $relative."
    done
    [[ -x "$SOURCE_APP/Contents/MacOS/TahoeUSBWiFi" && -x "$SOURCE_APP/Contents/Helpers/TahoeWiFiHelper" ]] ||
        fail "The app or helper is not executable."
    /usr/bin/plutil -lint "$SOURCE_APP/Contents/Info.plist" \
        "$SOURCE_APP/Contents/Library/LaunchDaemons/org.research.TahoeUSBWiFi.driver.plist"
    bundle_id=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$SOURCE_APP/Contents/Info.plist")
    version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$SOURCE_APP/Contents/Info.plist")
    build_number=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$SOURCE_APP/Contents/Info.plist")
    minimum=$(/usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' "$SOURCE_APP/Contents/Info.plist")
    installer_protocol=$(/usr/libexec/PlistBuddy -c 'Print :RTWInstallerProtocolVersion' "$SOURCE_APP/Contents/Info.plist" 2>/dev/null || true)
    [[ "$bundle_id" == org.research.TahoeUSBWiFi ]] || fail "This is not the Tahoe Wi-Fi USB application."
    [[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ && "$build_number" =~ ^[0-9]+$ ]] ||
        fail "Invalid version or build number."
    # The existing 0.2.5 app understands --complete-install/--prepare-update.
    # Future packages must explicitly declare protocol compatibility.
    [[ "$version" == 0.2.5 || "$installer_protocol" == 1 ]] ||
        fail "The app has no supported installer protocol; use version 0.2.5 or this updated project."
    [[ "$minimum" == 13.0 ]] || fail "This installer requires LSMinimumSystemVersion=13.0."
    for relative in Contents/MacOS/TahoeUSBWiFi Contents/Helpers/TahoeWiFiHelper; do
        /usr/bin/lipo -archs "$SOURCE_APP/$relative" | /usr/bin/tr ' ' '\n' |
            /usr/bin/grep -qx x86_64 || fail "Executable does not contain the x86_64 architecture: $relative."
    done
    # Bundle metadata cannot lower a binary's actual deployment requirement.
    # Inspect both executables before producing a component or product archive.
    /usr/bin/python3 "$APP_DIR/check-macos-compatibility.py" --max-minos "$minimum" \
        "$SOURCE_APP/Contents/MacOS/TahoeUSBWiFi" \
        "$SOURCE_APP/Contents/Helpers/TahoeWiFiHelper" || fail "Incompatible app/helper Mach-O deployment target."
    rtw_verify_app "$SOURCE_APP"

    local stage payload staged_app component product final checksum
    stage="$(/usr/bin/mktemp -d "$OUTPUT/.TahoeWiFi-pkg.XXXXXXXX")"
    trap '/bin/rm -rf "$stage"' EXIT
    payload="$stage/payload"
    staged_app="$payload/Applications/Tahoe Wi-Fi USB.app"
    component="$stage/TahoeWiFi-component.pkg"
    product="$stage/Tahoe-WiFi-USB-$version.pkg"
    final="$OUTPUT/Tahoe-WiFi-USB-$version.pkg"
    [[ ! -L "$final" && ( ! -e "$final" || -f "$final" ) ]] || fail "Unexpected .pkg destination."
    [[ ! -L "$final.sha256" && ( ! -e "$final.sha256" || -f "$final.sha256" ) ]] || fail "Unexpected SHA-256 destination."
    mkdir -p "$payload/Applications" "$stage/scripts" "$stage/resources"

    printf '\n[1/4] Preparing the complete app and payload permissions...\n'
    /usr/bin/ditto --noqtn "$SOURCE_APP" "$staged_app"
    # Normalize this disposable copy, not the installed app or developer's source.
    /bin/chmod -RN "$payload"
    /usr/bin/find "$payload" -type d -exec /bin/chmod 755 {} +
    /usr/bin/find "$payload" -type f -exec /bin/chmod 644 {} +
    /bin/chmod 755 "$staged_app/Contents/MacOS/TahoeUSBWiFi" \
        "$staged_app/Contents/Helpers/TahoeWiFiHelper"
    rtw_verify_app "$staged_app"
    /bin/cp "$APP_DIR/pkg/scripts/"* "$stage/scripts/"
    /bin/cp "$APP_DIR/signing.sh" "$stage/scripts/signing.sh"
    /bin/cp "$staged_app/Contents/Info.plist" "$stage/scripts/package-info.plist"
    /bin/chmod 755 "$stage/scripts/preinstall" "$stage/scripts/postinstall"
    /bin/chmod 644 "$stage/scripts/common.sh" "$stage/scripts/signing.sh"
    /bin/cp "$APP_DIR/pkg/resources/"*.html "$stage/resources/"

    printf '\n[2/4] Building the component with a fixed destination and root ownership...\n'
    /usr/bin/pkgbuild --root "$payload" --identifier org.research.TahoeUSBWiFi.pkg \
        --version "$version" --install-location / --ownership recommended \
        --component-plist "$APP_DIR/pkg/components.plist" \
        --scripts "$stage/scripts" "$component"

    printf '\n[3/4] Building the graphical installer...\n'
    local product_args=(--distribution "$APP_DIR/pkg/Distribution.xml" \
        --resources "$stage/resources" --package-path "$stage")
    if [[ -n "$INSTALLER_IDENTITY" ]]; then
        product_args+=(--sign "$INSTALLER_IDENTITY" --timestamp)
    fi
    /usr/bin/productbuild "${product_args[@]}" "$product"
    [[ -s "$product" ]] || fail "productbuild did not produce the installer."
    printf '\n[4/4] Verifying the archive and publishing in Release...\n'
    /usr/sbin/pkgutil --expand "$product" "$stage/expanded"
    [[ -f "$stage/expanded/Distribution" ]] || fail "Product is missing its Distribution file."
    if [[ -n "$INSTALLER_IDENTITY" ]]; then /usr/sbin/pkgutil --check-signature "$product"; fi
    checksum=$(/usr/bin/shasum -a 256 "$product")
    printf '%s  %s\n' "${checksum%% *}" "$(basename "$final")" > "$stage/package.sha256"
    /bin/mv -f "$product" "$final"
    /bin/mv -f "$stage/package.sha256" "$final.sha256"
    printf '\n[OK] Installer: %s\n[OK] SHA-256: %s\n[OK] Log: %s\n' "$final" "$final.sha256" "$LOG"
    if [[ -z "$INSTALLER_IDENTITY" ]]; then
        printf '[WARNING] Local .pkg without a Developer ID Installer signature or notarization.\n'
    else
        printf '[WARNING] Only the .pkg container is certificate-signed; the app is ad-hoc and NOT notarized.\n'
    fi
    printf 'Open the .pkg in Finder to install. Do not run install.command afterward.\n'
    printf 'The installer does not include the kernel extension, remove passwords or change SIP/AMFI/Gatekeeper.\n'
)

# Do not call the build function in an if/! test: that would suppress errexit
# inside nested functions. Preserve both build and logging exit statuses.
set +e
(set -euo pipefail; package_app) 2>&1 | /usr/bin/tee "$LOG"
results=("${PIPESTATUS[@]}")
set -e
if [[ "${results[0]}" -ne 0 || "${results[1]}" -ne 0 ]]; then
    printf '\n[ERROR] Could not build the .pkg. See: %s\n' "$LOG" >&2
    exit 1
fi
