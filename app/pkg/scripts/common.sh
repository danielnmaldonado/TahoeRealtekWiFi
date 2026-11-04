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
    [[ "$major" =~ ^[0-9]+$ ]] && (( major >= 13 )) || fail "Requires macOS Ventura 13 or later."
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
    local listing status=0
    listing=$(/bin/launchctl print "system/$SERVICE_ID" 2>&1) || status=$?
    if [[ "$status" -eq 0 ]]; then return 0; fi
    if [[ "$status" -eq 113 && "$listing" == *"Could not find service \"$SERVICE_ID\" in domain for system"* ]]; then
        return 1
    fi
    fail "Could not inspect the driver service. Installation was canceled before changing it."
}

stop_owned_stale_service() {
    local listing
    listing=$(/bin/launchctl print "system/$SERVICE_ID" 2>&1) ||
        fail "Could not verify the stale service; nothing was stopped."
    # Only this exact SM-managed parent and BundleProgram may be retired.
    # A label by itself must never authorize stopping an unrelated job.
    [[ "$listing" == "system/$SERVICE_ID = {"* ]] &&
    [[ "$listing" =~ $'\n'[[:space:]]*managed_by[[:space:]]=[[:space:]]com\.apple\.xpc\.ServiceManagement$'\n' ]] &&
    [[ "$listing" =~ $'\n'[[:space:]]*parent[[:space:]]bundle[[:space:]]identifier[[:space:]]=[[:space:]]org\.research\.TahoeUSBWiFi$'\n' ]] &&
    [[ "$listing" =~ $'\n'[[:space:]]*program[[:space:]]identifier[[:space:]]=[[:space:]]Contents/Helpers/TahoeWiFiHelper[[:space:]]\(mode:[[:space:]]2\)$'\n' ]] ||
        fail "The registered job does not match this app's managed service. It was preserved."
    identify_console_user || fail "Sign in to a graphical user session to repair the existing driver service."
    /bin/launchctl bootout "system/$SERVICE_ID" ||
        fail "macOS could not stop this app's stale service. Installation was canceled."
    local attempt
    for (( attempt=0; attempt<10; ++attempt )); do
        if ! service_is_registered; then
            log "Stopped this app's stale job. The new protected app will repair its ServiceManagement registration."
            return 0
        fi
        /bin/sleep 1
    done
    fail "The stale driver service did not stop. Installation was canceled."
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
