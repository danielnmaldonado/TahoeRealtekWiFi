#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-only
# Canonical fail-closed signing policy, shared by build/package/install hooks.
# This file is sourced; it never changes shell options or system security policy.

rtw_require_adhoc_identity() {
    if [[ "${RTW_SIGN_IDENTITY:--}" != "-" ]]; then
        printf '[ERROR] This source release requires ad-hoc signing (RTW_SIGN_IDENTITY=-).\n' >&2
        return 1
    fi
    if [[ -n "${RTW_NOTARY_PROFILE:-}" ]]; then
        printf '[ERROR] Ad-hoc code is not a Developer ID notarization build. Remove RTW_NOTARY_PROFILE.\n' >&2
        return 1
    fi
}

rtw_verify_adhoc() {
    local path="$1" signature
    /usr/bin/codesign --verify --strict --verbose=2 "$path" || return 1
    signature=$(/usr/bin/codesign -d --verbose=4 "$path" 2>&1) || return 1
    if ! printf '%s\n' "$signature" | /usr/bin/grep -qx 'Signature=adhoc'; then
        printf '[ERROR] A valid ad-hoc signature is required: %s\n' "$path" >&2
        return 1
    fi
}

rtw_verify_app() {
    local bundle="$1"
    rtw_verify_adhoc "$bundle/Contents/Helpers/TahoeWiFiHelper" || return 1
    rtw_verify_adhoc "$bundle/Contents/MacOS/TahoeUSBWiFi" || return 1
    rtw_verify_adhoc "$bundle" || return 1
    /usr/bin/codesign --verify --deep --strict "$bundle"
}

rtw_sign_app() {
    local bundle="$1"
    rtw_require_adhoc_identity || return 1
    /usr/bin/codesign --force --sign - --timestamp=none --options runtime \
        --identifier org.research.TahoeUSBWiFi.driver \
        "$bundle/Contents/Helpers/TahoeWiFiHelper" || return 1
    # Sign inside-out; --deep is used only for verification.
    /usr/bin/codesign --force --sign - --timestamp=none --options runtime "$bundle" || return 1
    rtw_verify_app "$bundle"
}

rtw_sign_kext() {
    local bundle="$1"
    rtw_require_adhoc_identity || return 1
    /usr/bin/codesign --force --sign - --timestamp=none "$bundle" || return 1
    rtw_verify_adhoc "$bundle"
}
