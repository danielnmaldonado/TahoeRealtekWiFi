#!/bin/bash
# Native regression tests. Keychain/login backends are mocked; user settings
# and real passwords are not changed. The menu suite needs a GUI session.
set -euo pipefail
cd "$(dirname "$0")"
[[ "$(uname -s)" == "Darwin" ]] || { printf '[ERROR] Requires macOS and Xcode.\n' >&2; exit 1; }
# Compile/link the production app and helper before tests using mocks.
# No GUI, installed service, login setting or Keychain is touched here.
bash ./build.command --check-only ../out/menu-build-check
mkdir -p ../out/menu-tests
sanitizers="${RTW_HOST_SANITIZERS:-address,undefined}"
case "$sanitizers" in address,undefined|undefined) ;;
  *) printf '[ERROR] Invalid RTW_HOST_SANITIZERS selection.\n' >&2; exit 2;;
esac
flags=(-fsanitize="$sanitizers" -fno-sanitize-recover=all -fno-omit-frame-pointer -std=c++17 -Wall -Wextra -Werror -UNDEBUG -mmacosx-version-min=26.0 -I. -I../kext -I../include)
for suite in model signal-policy poll-policy work-epoch credential-policy reconnect-policy instance-lock; do
    xcrun clang++ "${flags[@]}" "tests/$suite-tests.cpp" -o "../out/menu-tests/$suite-tests"
    "../out/menu-tests/$suite-tests"
done
xcrun clang++ "${flags[@]}" tests/adapter-support-tests.cpp USBWiFiProfiles.cpp -o ../out/menu-tests/adapter-support-tests
../out/menu-tests/adapter-support-tests
xcrun clang++ "${flags[@]}" -fobjc-arc tests/registry-metadata-tests.mm USBWiFiProfiles.cpp \
    -framework Foundation -framework IOKit -framework CoreFoundation -o ../out/menu-tests/registry-metadata-tests
../out/menu-tests/registry-metadata-tests
xcrun clang++ "${flags[@]}" tests/transport-tests.cpp Transport.cpp -o ../out/menu-tests/transport-tests
../out/menu-tests/transport-tests
xcrun clang++ "${flags[@]}" -fobjc-arc tests/login-item-tests.mm LoginItem.mm \
    -framework Foundation -framework ServiceManagement -framework CoreServices \
    -o ../out/menu-tests/login-item-tests
../out/menu-tests/login-item-tests
xcrun clang++ "${flags[@]}" -fobjc-arc tests/credential-store-tests.mm CredentialStore.mm \
    -framework Foundation -framework Security -o ../out/menu-tests/credential-store-tests
../out/menu-tests/credential-store-tests
for suite in menu credential-flow channel-map-reconnect; do
    xcrun clang++ "${flags[@]}" -fobjc-arc "tests/$suite-tests.mm" LoginItem.mm CredentialStore.mm DriverConnection.mm AdapterDiagnostics.mm USBWiFiProfiles.cpp Transport.cpp \
        -framework AppKit -framework ServiceManagement -framework Security -framework CoreServices -framework IOKit -framework CoreFoundation \
        -o "../out/menu-tests/$suite-tests"
    "../out/menu-tests/$suite-tests"
done
xcrun clang++ "${flags[@]}" tests/cli-tests.cpp -framework IOKit -framework CoreFoundation -o ../out/menu-tests/cli-tests
../out/menu-tests/cli-tests
