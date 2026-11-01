#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Shared Installer hooks. No code is fetched from the network or run as the GUI user
# until the installed bundle has passed its structure and signature checks.
PATH=/usr/bin:/bin:/usr/sbin:/sbin
export PATH
umask 022
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/signing.sh"
APP="/Applications/Tahoe Wi-Fi USB.app"
BUNDLE_ID="org.research.TahoeUSBWiFi"
SERVICE_ID="org.research.TahoeUSBWiFi.driver"
DAEMON_PLIST="$APP/Contents/Library/LaunchDaemons/$SERVICE_ID.plist"
EXECUTABLE="$APP/Contents/MacOS/TahoeUSBWiFi"
CONSOLE_UID=""

log() { printf '[TahoeWiFi.pkg] %s\n' "$*"; }
fail() { log "ERROR: $*" >&2; exit 1; }

require_target() {
    [[ "${1:-}" == "/" ]] || fail "Install on the running system volume (/), not on another volume."
    [[ "$(/usr/bin/id -u)" == 0 ]] || fail "This script must be run by Installer."
    [[ "$(/usr/bin/uname -s)" == Darwin ]] || fail "Requires macOS."
    local version major arm
    version=$(/usr/bin/sw_vers -productVersion)
    major="${version%%.*}"
    [[ "$major" =~ ^[0-9]+$ ]] && (( major >= 26 )) || fail "Requires macOS 26 or later."
    arm=$(/usr/sbin/sysctl -n hw.optional.arm64 2>/dev/null || printf '0')
    [[ "$(/usr/bin/uname -m)" == x86_64 && "$arm" != 1 ]] || fail "This project requires an Intel x86_64 Mac."
    [[ -d /Applications && ! -L /Applications ]] || fail "/Applications is not a regular directory."
}

identify_console_user() {
    CONSOLE_UID=$(/usr/bin/stat -f '%u' /dev/console 2>/dev/null) || { CONSOLE_UID=""; return 1; }
    [[ "$CONSOLE_UID" =~ ^[0-9]+$ ]] || { CONSOLE_UID=""; return 1; }
    (( CONSOLE_UID >= 501 )) || { CONSOLE_UID=""; return 1; }
    /usr/bin/id -nu "$CONSOLE_UID" >/dev/null 2>&1 || { CONSOLE_UID=""; return 1; }
    /bin/launchctl print "gui/$CONSOLE_UID" >/dev/null 2>&1 || { CONSOLE_UID=""; return 1; }
}

run_as_console_user() {
    [[ "$CONSOLE_UID" =~ ^[0-9]+$ ]] && (( CONSOLE_UID >= 501 )) || return 1
    # asuser selects the GUI bootstrap domain; sudo also drops UID/GID and fixes HOME.
    # Never execute the app directly as the installer's root account.
    /bin/launchctl asuser "$CONSOLE_UID" /usr/bin/sudo -H -u "#$CONSOLE_UID" -- "$@"
}

check_bundle_tree() {
    [[ -d "$APP" && ! -L "$APP" ]] || fail "The application is missing or the destination is a symbolic link."
    local invalid
    invalid=$(/usr/bin/find "$APP" ! -type d ! -type f -print -quit) || fail "Could not inspect the application."
    [[ -z "$invalid" ]] || fail "Unexpected file type in the application: $invalid"
    local relative
    for relative in \
        Contents/Info.plist \
        Contents/MacOS/TahoeUSBWiFi \
        Contents/Helpers/TahoeWiFiHelper \
        Contents/Library/LaunchDaemons/org.research.TahoeUSBWiFi.driver.plist \
        Contents/_CodeSignature/CodeResources \
        Contents/Resources/AppIcon.icns; do
        [[ -f "$APP/$relative" && -s "$APP/$relative" ]] || fail "Incomplete bundle: $relative"
    done
    [[ -x "$EXECUTABLE" && -x "$APP/Contents/Helpers/TahoeWiFiHelper" ]] ||
        fail "The app executable or helper is not executable."
    [[ "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$APP/Contents/Info.plist")" == "$BUNDLE_ID" ]] ||
        fail "Unexpected identifier; no other application will be replaced."
    /usr/bin/plutil -lint "$APP/Contents/Info.plist" "$DAEMON_PLIST" >/dev/null ||
        fail "Invalid Info.plist or service declaration."
    /usr/bin/codesign --verify --deep --strict "$APP" ||
        fail "The application signature is invalid."
}

bundle_is_protected() {
    local unprotected
    unprotected=$(/usr/bin/find "$APP" \( ! -user root -o -perm -0020 -o -perm -0002 \) -print -quit) || return 1
    [[ -z "$unprotected" ]]
}

service_is_registered() {
    /bin/launchctl print "system/$SERVICE_ID" >/dev/null 2>&1
}

check_app_closed() {
    local result=0
    /usr/bin/pgrep -x TahoeUSBWiFi >/dev/null 2>&1 || result=$?
    case "$result" in
        0) fail "Quit Tahoe Wi-Fi USB using its Quit menu item and run this .pkg again. No process was terminated." ;;
        1) return 0 ;;
        *) fail "Could not determine whether the application is running." ;;
    esac
}
