cmake_minimum_required(VERSION 3.16)
project(TahoeRealtekWiFi VERSION 0.14.19 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(RTW_HOST_SANITIZERS "address,undefined" CACHE STRING
    "Sanitizers for native host-only regression tests")
set_property(CACHE RTW_HOST_SANITIZERS PROPERTY STRINGS "address,undefined" "undefined")
if(NOT RTW_HOST_SANITIZERS MATCHES "^(address,undefined|undefined)$")
    message(FATAL_ERROR "RTW_HOST_SANITIZERS must be address,undefined or undefined")
endif()

option(RTW_ENABLE_SANITIZERS "Instrument every host target (never the kext)" OFF)
if(RTW_ENABLE_SANITIZERS)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "RTW_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
    add_compile_options(-fsanitize=${RTW_HOST_SANITIZERS}
        -fno-sanitize-recover=all -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${RTW_HOST_SANITIZERS})
endif()

# Pinned RTL8814B USB HAL; station remains disabled until the full NIC path is qualified.
add_library(rtw8814b_hal STATIC
    src/RTW8814BHAL.c
    src/RTW8814BUSBCodec.c
    src/RTW8814BPHY.c
    src/rtl8814b/halmac/halmac_api.c
    src/rtl8814b/halmac/halmac_dbg.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_bb_rf_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_cfg_wmac_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_common_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_efuse_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_flash_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_fw_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_gpio_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_init_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_mimo_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_pcie_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_sdio_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_usb_88xx_v1.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_gpio_8814b.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_init_8814b.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_pcie_8814b.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_phy_8814b.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_pwr_seq_8814b.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_sdio_8814b.c
    src/rtl8814b/halmac/halmac_88xx_v1/halmac_8814b/halmac_usb_8814b.c
)
target_include_directories(rtw8814b_hal PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/rtl8814b/halmac")
set_target_properties(rtw8814b_hal PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)

add_library(rtwcore STATIC
    src/Frame.cpp
    src/FrameWire.c
    src/Beacon.cpp
    src/LinkState.cpp
    src/USBIDs.cpp
    src/USBWiFiProfiles.c
    src/ChipIdentity.c
    src/FamilyBackend.c
    src/FamilyRX24.c
    src/Family5GHzScan.c
    src/Family5GHzStation.c
    src/FamilyRadioPolicy.c
    src/DFSStation.c
    src/DFSRegulatory.c
    src/USBWiFiFirmware.c
    src/LegacyFirmwareStage.c
    src/RTL8192CDataPath.c
    src/RTL8814AUBoot.c
    src/RTL8814AUEfuse.c
    src/RTL8814AUMAC.c
    src/RTL8814AUPHY.c
    src/RTL8814AUDataPath.c
    src/RTL8723BU.c
    src/RTL8723BUDataPath.c
    src/RTL8710BU.c
    src/RTL8710BUDataPath.c
    src/RTL8192FU.c
    src/RTL8192FUDataPath.c
    src/RTL8192DUPHY.c
    src/RTL8192DUUSB.c
    src/RTL8192DUStation.c
    src/RTL8192DUBoot.c
    src/RTL8192DUEfuse.c
    src/RTL8192DUDataPath.c
    src/RTL8812AUBootstrap.c
    src/RTL8812AUMAC.c
    src/RTL8812AURuntime.c
    src/RTL8821AUCoex.c
    src/RTL8812AUBoot.c
    src/RTL8812AUPHY.c
    src/RTW88USBDataPath.c
    src/RTW88Firmware.c
    src/RTW88Boot.c
    src/RTW88PHY.c
    src/RTW88PHYOperating.c
    src/RTW88Coex.c
    src/RTW88RX.c
    src/RTW88MAC.c
    src/RTW88Station.c
    src/RTL8812AUDataPath.c
    src/RTL8812AURadio.c
    src/RTL8812AUStation.c
    src/RTL8812AUPassive.c
    src/RTL8814AUStation.c
    src/USBControl.c
    src/USBDescriptors.c
    src/StationPort.c
    src/StationMgmt.c
    src/FCS.c
    src/Chip.c
    src/USBRegisterIO.c
    src/Efuse.c
    src/Firmware.c
    src/USBPackets.c
    src/USBBulk.c
    src/PowerSequence.c
    src/LLT.c
    src/FirmwareBoot.c
    src/MacDMA.c
    src/HAL8812.c
    src/PHYRegisters.c
    src/PHYProfile.c
    src/RXMonitor.c
    src/RFReadback.c
    src/RFCalibration.c
    src/RTL8192EU.c
    src/RTL8192EUHardware.c
    src/RTL8192EUHT.c
    src/RTL8192EUDataPath.c
    src/USBProbe.c
    src/EthernetBridge.c
)
target_include_directories(rtwcore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rtwcore PUBLIC rtw8814b_hal)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(rtwcore PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(rtw-backends tools/rtw-backends.cpp)
target_link_libraries(rtw-backends PRIVATE rtwcore)

add_executable(rtw-inspect tools/rtw-inspect.cpp)
target_link_libraries(rtw-inspect PRIVATE rtwcore)

add_executable(rtw-acusb-decode tools/rtw-acusb-decode.cpp)
target_link_libraries(rtw-acusb-decode PRIVATE rtwcore)

add_executable(rtw-probe tools/rtw-probe.cpp)
target_link_libraries(rtw-probe PRIVATE rtwcore)
if(APPLE)
    add_executable(rtw-8814au-board tools/rtw-8814au-board.cpp)
    target_link_libraries(rtw-8814au-board PRIVATE rtwcore "-framework IOKit" "-framework CoreFoundation")
    add_executable(rtw-8814au-bringup tools/rtw-8814au-bringup.cpp)
    target_link_libraries(rtw-8814au-bringup PRIVATE rtwcore "-framework IOKit" "-framework CoreFoundation")
    target_link_libraries(rtw-probe PRIVATE "-framework IOKit" "-framework CoreFoundation")
endif()

# libusb is OPTIONAL. rtw-probe (8812AU / 8192EU) and rtw-hwdiag --probe read registers only.
# rtw-hwdiag --boot-pre-rf explicitly writes hardware registers and firmware
# ONLY when passed a firmware path and --confirm-unstable-hardware.
# Neither target changes SIP or installs a kernel extension.
option(RTW_ENABLE_LIBUSB "Build optional Realtek USB register-read diagnostics" ON)
if(RTW_ENABLE_LIBUSB)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUSB QUIET IMPORTED_TARGET libusb-1.0)
        if(LIBUSB_FOUND)
            target_compile_definitions(rtw-probe PRIVATE RTW_WITH_LIBUSB=1)
            target_link_libraries(rtw-probe PRIVATE PkgConfig::LIBUSB)
            add_executable(rtw-legacy-fwdiag tools/rtw-legacy-fwdiag.cpp)
            target_link_libraries(rtw-legacy-fwdiag PRIVATE rtwcore PkgConfig::LIBUSB)
            add_executable(rtw-hwdiag tools/rtw-hwdiag.cpp)
            target_link_libraries(rtw-hwdiag PRIVATE rtwcore PkgConfig::LIBUSB)
            add_executable(rtw-8192eu-native tools/rtw-8192eu-native.cpp)
            target_link_libraries(rtw-8192eu-native PRIVATE rtwcore PkgConfig::LIBUSB)
        endif()
    endif()
endif()

if(APPLE)
    add_executable(rtw-8814au-readonly tools/rtw-8814au-readonly.cpp)
    target_link_libraries(rtw-8814au-readonly PRIVATE "-framework IOKit" "-framework CoreFoundation")
    add_executable(rtw-usb-match tools/rtw-usb-match.cpp)
    target_link_libraries(rtw-usb-match PRIVATE "-framework IOKit" "-framework CoreFoundation")
    add_executable(rtw-control tools/rtw-control.cpp)
    target_include_directories(rtw-control PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/kext")
    target_link_libraries(rtw-control PRIVATE "-framework IOKit" "-framework CoreFoundation")
endif()

include(CTest)
if(BUILD_TESTING)
    # Tests use assert for executable invariants in both Debug and Release.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-UNDEBUG)
    endif()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME rtw-chip-discovery-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-chip-discovery.py")
    add_test(NAME rtw-chip-identity-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-chip-identity.py")
    add_executable(rtw-embedded-realtek-fw-tests tests/embedded_realtek_firmware_tests.cpp)
    target_link_libraries(rtw-embedded-realtek-fw-tests PRIVATE rtwcore)
    add_test(NAME rtw-embedded-realtek-fw-tests COMMAND rtw-embedded-realtek-fw-tests)
    add_test(NAME rtw-embedded-firmware-hash-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/embedded_firmware_hash_tests.py")
    foreach(part rtl8710bu rtl8710bu_datapath rtl8192du_dual rtl8192du_phy rtl8192du_usb rtl8192du_station rtw88_rx rtl8812au_bootstrap rtl8812au_runtime rtw88_phy_operating rtl8192du_rom rtl8192du_datapath rtw88_boot rtw88_phy rtl8192du_boot rtl8812au_boot rtl8812au_phy rtw88_usb_tx rtw88_firmware rtw88_mac rtw88_station)
        add_executable(rtw-${part}-tests tests/${part}_tests.cpp)
        target_link_libraries(rtw-${part}-tests PRIVATE rtwcore)
        target_compile_options(rtw-${part}-tests PRIVATE -UNDEBUG)
        add_test(NAME rtw-${part}-tests COMMAND rtw-${part}-tests)
    endforeach()

    if(APPLE)
        add_executable(rtw-usb-match-native-tests tests/usb_match_native_tests.cpp)
        target_compile_options(rtw-usb-match-native-tests PRIVATE -UNDEBUG)
        target_link_libraries(rtw-usb-match-native-tests PRIVATE "-framework IOKit" "-framework CoreFoundation")
        add_test(NAME rtw-usb-match-native-tests COMMAND rtw-usb-match-native-tests)
        add_executable(rtw-native-8192eu-probe-tests tests/native_8192eu_probe_tests.cpp)
        target_compile_options(rtw-native-8192eu-probe-tests PRIVATE -UNDEBUG)
        target_link_libraries(rtw-native-8192eu-probe-tests PRIVATE rtwcore "-framework IOKit" "-framework CoreFoundation")
        add_test(NAME rtw-native-8192eu-probe-tests COMMAND rtw-native-8192eu-probe-tests)
    endif()
    add_executable(rtw-family-5ghz-station-tests tests/family_5ghz_station_tests.cpp)
    target_link_libraries(rtw-family-5ghz-station-tests PRIVATE rtwcore)
    add_test(NAME rtw-family-5ghz-station-tests COMMAND rtw-family-5ghz-station-tests)

    add_executable(rtw-family-5ghz-scan-tests tests/family_5ghz_scan_tests.cpp)
    target_link_libraries(rtw-family-5ghz-scan-tests PRIVATE rtwcore)
    add_test(NAME rtw-family-5ghz-scan-tests COMMAND rtw-family-5ghz-scan-tests)

    add_executable(rtw-radio-5ghz-rx-tests tests/radio_5ghz_rx_tests.cpp)
    target_link_libraries(rtw-radio-5ghz-rx-tests PRIVATE rtwcore)
    add_test(NAME rtw-radio-5ghz-rx-tests COMMAND rtw-radio-5ghz-rx-tests)

    add_executable(rtw-radio-capabilities-tests tests/radio_capabilities_tests.cpp)
    target_link_libraries(rtw-radio-capabilities-tests PRIVATE rtwcore)
    add_test(NAME rtw-radio-capabilities-tests COMMAND rtw-radio-capabilities-tests)
    add_test(NAME rtw-radio-cli-contract-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/radio_cli_contract_tests.py" $<TARGET_FILE:rtw-backends>)
    add_test(NAME rtw-network-scan-contract-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/network_scan_contract_tests.py")

    add_executable(rtw-diagnostic-publish-policy-tests tests/diagnostic_publish_policy_tests.cpp)
    target_include_directories(rtw-diagnostic-publish-policy-tests PRIVATE include)
    # Mirror the kext: a shared policy header must compile without C++ STL headers.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_compile_options(rtw-diagnostic-publish-policy-tests PRIVATE -nostdinc++)
    endif()
    add_test(NAME rtw-diagnostic-publish-policy-tests COMMAND rtw-diagnostic-publish-policy-tests)

    add_executable(rtw-network-security-tests tests/network_security_tests.cpp)
    target_include_directories(rtw-network-security-tests PRIVATE include)
    add_test(NAME rtw-network-security-tests COMMAND rtw-network-security-tests)
    add_executable(rtw-menu-adapter-support-tests app/tests/adapter-support-tests.cpp)
    target_include_directories(rtw-menu-adapter-support-tests PRIVATE app)
    target_link_libraries(rtw-menu-adapter-support-tests PRIVATE rtwcore)
    add_test(NAME rtw-menu-adapter-support-tests COMMAND rtw-menu-adapter-support-tests)
    add_executable(rtw-menu-reconnect-policy-tests app/tests/reconnect-policy-tests.cpp)
    target_include_directories(rtw-menu-reconnect-policy-tests PRIVATE kext)
    target_include_directories(rtw-menu-reconnect-policy-tests PRIVATE app)
    add_test(NAME rtw-menu-reconnect-policy-tests COMMAND rtw-menu-reconnect-policy-tests)
    add_executable(rtw-menu-instance-lock-tests app/tests/instance-lock-tests.cpp)
    target_include_directories(rtw-menu-instance-lock-tests PRIVATE app)
    add_test(NAME rtw-menu-instance-lock-tests COMMAND rtw-menu-instance-lock-tests)
    add_executable(rtw-timeout-lifecycle-tests
        tests/timeout_lifecycle_tests.cpp vendor/itl80211/openbsd/net80211/CTimeout.cpp)
    target_include_directories(rtw-timeout-lifecycle-tests PRIVATE
        tests/timeout_host tests/timeout_host/include vendor/itl80211/openbsd)
    add_test(NAME rtw-timeout-lifecycle-tests COMMAND rtw-timeout-lifecycle-tests)
    add_executable(rtw-menu-credential-policy-tests app/tests/credential-policy-tests.cpp)
    target_include_directories(rtw-menu-credential-policy-tests PRIVATE app kext)
    add_test(NAME rtw-menu-credential-policy-tests COMMAND rtw-menu-credential-policy-tests)
    add_executable(rtw-signal-sample-tests tests/signal_sample_tests.cpp)
    target_include_directories(rtw-signal-sample-tests PRIVATE include)
    add_test(NAME rtw-signal-sample-tests COMMAND rtw-signal-sample-tests)
    add_executable(rtw-menu-signal-policy-tests app/tests/signal-policy-tests.cpp)
    target_include_directories(rtw-menu-signal-policy-tests PRIVATE app kext)
    add_test(NAME rtw-menu-signal-policy-tests COMMAND rtw-menu-signal-policy-tests)
    add_executable(rtw-menu-model-tests app/tests/model-tests.cpp)
    target_include_directories(rtw-menu-model-tests PRIVATE app kext)
    add_test(NAME rtw-menu-model-tests COMMAND rtw-menu-model-tests)
    find_package(Threads REQUIRED)
    add_executable(rtw-menu-work-epoch-tests app/tests/work-epoch-tests.cpp)
    target_include_directories(rtw-menu-work-epoch-tests PRIVATE app)
    target_link_libraries(rtw-menu-work-epoch-tests PRIVATE Threads::Threads)
    add_test(NAME rtw-menu-work-epoch-tests COMMAND rtw-menu-work-epoch-tests)
    add_executable(rtw-menu-poll-policy-tests app/tests/poll-policy-tests.cpp)
    target_include_directories(rtw-menu-poll-policy-tests PRIVATE app)
    add_test(NAME rtw-menu-poll-policy-tests COMMAND rtw-menu-poll-policy-tests)
    if(APPLE)
        enable_language(OBJCXX)
        set(CMAKE_OBJCXX_STANDARD 17)
        set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
        add_executable(rtw-menu-login-item-tests app/tests/login-item-tests.mm app/LoginItem.mm)
        target_include_directories(rtw-menu-login-item-tests PRIVATE app)
        target_compile_options(rtw-menu-login-item-tests PRIVATE -fobjc-arc -Wall -Wextra -Werror -Werror=unguarded-availability -Werror=unguarded-availability-new -mmacosx-version-min=13.0)
        target_link_options(rtw-menu-login-item-tests PRIVATE -mmacosx-version-min=13.0)
        target_link_libraries(rtw-menu-login-item-tests PRIVATE "-framework Foundation" "-framework ServiceManagement" "-framework CoreServices")
        add_test(NAME rtw-menu-login-item-tests COMMAND rtw-menu-login-item-tests)
        add_executable(rtw-menu-credential-store-tests app/tests/credential-store-tests.mm app/CredentialStore.mm)
        target_include_directories(rtw-menu-credential-store-tests PRIVATE app kext)
        target_compile_options(rtw-menu-credential-store-tests PRIVATE -fobjc-arc -Wall -Wextra -Werror -Werror=unguarded-availability -Werror=unguarded-availability-new -mmacosx-version-min=13.0)
        target_link_options(rtw-menu-credential-store-tests PRIVATE -mmacosx-version-min=13.0)
        target_link_libraries(rtw-menu-credential-store-tests PRIVATE "-framework Foundation" "-framework Security")
        add_test(NAME rtw-menu-credential-store-tests COMMAND rtw-menu-credential-store-tests)
        add_executable(rtw-menu-transport-tests app/tests/transport-tests.cpp app/Transport.cpp)
        target_include_directories(rtw-menu-transport-tests PRIVATE app kext)
        add_test(NAME rtw-menu-transport-tests COMMAND rtw-menu-transport-tests)
        add_executable(rtw-menu-diagnostic-cli-tests app/tests/diagnostic-cli-tests.mm
            app/DiagnosticCLI.mm app/ControlDiagnostics.mm app/DriverConnection.mm)
        target_include_directories(rtw-menu-diagnostic-cli-tests PRIVATE app kext)
        target_compile_options(rtw-menu-diagnostic-cli-tests PRIVATE -fobjc-arc -Wall -Wextra -Werror -Werror=unguarded-availability -Werror=unguarded-availability-new -mmacosx-version-min=13.0)
        target_link_options(rtw-menu-diagnostic-cli-tests PRIVATE -mmacosx-version-min=13.0)
        target_link_libraries(rtw-menu-diagnostic-cli-tests PRIVATE "-framework Foundation"
            "-framework ServiceManagement" "-framework Security" "-framework IOKit" "-framework CoreFoundation")
        add_test(NAME rtw-menu-diagnostic-cli-tests COMMAND rtw-menu-diagnostic-cli-tests)
        add_executable(rtw-menu-control-diagnostics-tests app/tests/control-diagnostics-tests.mm app/ControlDiagnostics.mm)
        target_include_directories(rtw-menu-control-diagnostics-tests PRIVATE app kext)
        target_compile_options(rtw-menu-control-diagnostics-tests PRIVATE -fobjc-arc -Wall -Wextra -Werror -Werror=unguarded-availability -Werror=unguarded-availability-new -mmacosx-version-min=13.0)
        target_link_options(rtw-menu-control-diagnostics-tests PRIVATE -mmacosx-version-min=13.0)
        target_link_libraries(rtw-menu-control-diagnostics-tests PRIVATE "-framework Foundation")
        add_test(NAME rtw-menu-control-diagnostics-tests COMMAND rtw-menu-control-diagnostics-tests)
        set_tests_properties(rtw-menu-control-diagnostics-tests PROPERTIES TIMEOUT 10)
        add_executable(rtw-menu-driver-diagnostics-tests app/tests/driver-diagnostics-tests.mm)
        target_include_directories(rtw-menu-driver-diagnostics-tests PRIVATE app kext)
        target_compile_options(rtw-menu-driver-diagnostics-tests PRIVATE -fobjc-arc -Wall -Wextra -Werror -Werror=unguarded-availability -Werror=unguarded-availability-new -mmacosx-version-min=13.0)
        target_link_options(rtw-menu-driver-diagnostics-tests PRIVATE -mmacosx-version-min=13.0)
        target_link_libraries(rtw-menu-driver-diagnostics-tests PRIVATE "-framework Foundation" "-framework IOKit" "-framework CoreFoundation")
        add_test(NAME rtw-menu-driver-diagnostics-tests COMMAND rtw-menu-driver-diagnostics-tests)

    endif()

    add_executable(rtw-control-tests tests/control_tests.cpp)
    target_include_directories(rtw-control-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/kext")
    add_test(NAME rtw-control-tests COMMAND rtw-control-tests)
    add_executable(rtw-dfs-regulatory-tests tests/dfs_regulatory_tests.c)
    target_link_libraries(rtw-dfs-regulatory-tests PRIVATE rtwcore)
    add_test(NAME rtw-dfs-regulatory-tests COMMAND rtw-dfs-regulatory-tests)
    add_executable(rtw-dfs-regulatory-controller-tests tests/dfs_regulatory_controller_tests.cpp)
    target_link_libraries(rtw-dfs-regulatory-controller-tests PRIVATE rtwcore)
    add_test(NAME rtw-dfs-regulatory-controller-tests COMMAND rtw-dfs-regulatory-controller-tests)

    add_executable(rtw-r12au-station-tests tests/rtl8812au_station_tests.cpp)
    target_link_libraries(rtw-r12au-station-tests PRIVATE rtwcore)
    add_test(NAME rtw-r12au-station-tests COMMAND rtw-r12au-station-tests)

    add_executable(rtw-r12au-passive-tests tests/rtl8812au_passive_tests.cpp)
    target_link_libraries(rtw-r12au-passive-tests PRIVATE rtwcore)
    add_test(NAME rtw-r12au-passive-tests COMMAND rtw-r12au-passive-tests)
    add_executable(rtw-r12au-rf-readback-tests tests/rtl8812au_rf_readback_tests.cpp)
    target_link_libraries(rtw-r12au-rf-readback-tests PRIVATE rtwcore)
    add_test(NAME rtw-r12au-rf-readback-tests COMMAND rtw-r12au-rf-readback-tests)

    add_executable(rtw-r12au-rf-calibration-tests tests/rtl8812au_rf_calibration_tests.cpp)
    target_link_libraries(rtw-r12au-rf-calibration-tests PRIVATE rtwcore)
    add_test(NAME rtw-r12au-rf-calibration-tests COMMAND rtw-r12au-rf-calibration-tests)
    add_executable(rtw-r12au-datapath-tests tests/rtl8812au_datapath_tests.cpp)
    target_link_libraries(rtw-r12au-datapath-tests PRIVATE rtwcore)
    add_test(NAME rtw-r12au-datapath-tests COMMAND rtw-r12au-datapath-tests)

    add_executable(rtw-92e-datapath-tests tests/rtl8192eu_datapath_tests.cpp)
    target_link_libraries(rtw-92e-datapath-tests PRIVATE rtwcore)
    add_test(NAME rtw-92e-datapath-tests COMMAND rtw-92e-datapath-tests)

    add_executable(rtw-92e-ht-descriptor-tests tests/rtl8192eu_ht_datapath_tests.cpp)
    target_link_libraries(rtw-92e-ht-descriptor-tests PRIVATE rtwcore)
    add_test(NAME rtw-92e-ht-descriptor-tests COMMAND rtw-92e-ht-descriptor-tests)

    add_executable(rtw-rx-admission-reason-tests tests/rx_admission_reason_tests.cpp)
    target_link_libraries(rtw-rx-admission-reason-tests PRIVATE rtwcore)
    add_test(NAME rtw-rx-admission-reason-tests COMMAND rtw-rx-admission-reason-tests)

    add_executable(rtw-rx-control-filter-tests tests/rx_control_filter_tests.cpp)
    target_link_libraries(rtw-rx-control-filter-tests PRIVATE rtwcore)
    add_test(NAME rtw-rx-control-filter-tests COMMAND rtw-rx-control-filter-tests)
    add_test(NAME rtw-rx-control-accounting-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rx-control-accounting.py"
        --out "${CMAKE_BINARY_DIR}/rx-control-accounting")

    add_executable(rtw-family-rx24-tests tests/family_rx24_tests.cpp)
    target_link_libraries(rtw-family-rx24-tests PRIVATE rtwcore)
    add_test(NAME rtw-family-rx24-tests COMMAND rtw-family-rx24-tests)
    add_executable(rtw-legacy-firmware-stage-tests tests/legacy_firmware_stage_tests.cpp)
    target_link_libraries(rtw-legacy-firmware-stage-tests PRIVATE rtwcore)
    add_test(NAME rtw-legacy-firmware-stage-tests COMMAND rtw-legacy-firmware-stage-tests)
    add_executable(rtw-family-firmware-radio-tests tests/family_firmware_radio_tests.cpp)
    target_link_libraries(rtw-family-firmware-radio-tests PRIVATE rtwcore)
    add_test(NAME rtw-family-firmware-radio-tests COMMAND rtw-family-firmware-radio-tests)

    add_test(NAME rtw-backend-status-cli-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/backend_status_cli_tests.py"
        --backends "$<TARGET_FILE:rtw-backends>")
    add_executable(rtw-family-backend-tests tests/family_backend_tests.cpp)
    target_link_libraries(rtw-family-backend-tests PRIVATE rtwcore)
    add_test(NAME rtw-family-backend-tests COMMAND rtw-family-backend-tests)

    add_executable(rtw-usb-wifi-profiles-tests tests/usb_wifi_profiles_tests.c)
    target_link_libraries(rtw-usb-wifi-profiles-tests PRIVATE rtwcore)
    add_test(NAME rtw-usb-wifi-profiles-tests COMMAND rtw-usb-wifi-profiles-tests)
    add_executable(rtw-usb-wifi-firmware-tests tests/usb_wifi_firmware_tests.cpp)
    target_link_libraries(rtw-usb-wifi-firmware-tests PRIVATE rtwcore)
    add_test(NAME rtw-usb-wifi-firmware-tests COMMAND rtw-usb-wifi-firmware-tests)
    add_executable(rtw-legacy-hal-tests vendor/rtwn/tests/legacy_hal_trace.cpp)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
        target_compile_options(rtw-legacy-hal-tests PRIVATE -fsanitize=${RTW_HOST_SANITIZERS} -fno-omit-frame-pointer)
        target_link_options(rtw-legacy-hal-tests PRIVATE -fsanitize=${RTW_HOST_SANITIZERS})
    endif()
    add_test(NAME rtw-legacy-hal-tests COMMAND rtw-legacy-hal-tests)

    add_executable(rtw-92e-hal-trace-tests vendor/rtwn/tests/hal_trace.cpp)
    add_test(NAME rtw-92e-hal-trace-tests COMMAND rtw-92e-hal-trace-tests)

    add_test(NAME rtw-r12au-capture-cli-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/rtl8812au_capture_cli_tests.py"
        --decoder "$<TARGET_FILE:rtw-acusb-decode>")
    add_test(NAME rtw-signing-policy-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/signing_policy_tests.py")
    add_test(NAME rtw-sdk-policy-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/sdk_policy_tests.py")
    add_test(NAME rtw-app-pkg-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app_pkg_tests.py")
    add_test(NAME rtw-menu-socket-exchange-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-app-socket-exchange.py")
    add_test(NAME rtw-menu-source-contract-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app_source_contract_tests.py")
    if(APPLE)
        # Compile/link real native sources, not just portable policy models.
        # No binary is run and no app or service is installed.
        add_test(NAME rtw-menu-native-build-check COMMAND bash
            "${CMAKE_CURRENT_SOURCE_DIR}/app/build.command" --check-only
            "${CMAKE_BINARY_DIR}/menu-build-check")
        set_tests_properties(rtw-menu-native-build-check PROPERTIES RUN_SERIAL TRUE)
    endif()
    add_test(NAME rtw-deferred-cleanup-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-deferred-cleanup.py"
        --out "${CMAKE_BINARY_DIR}/deferred-cleanup")
    add_test(NAME rtw-kext-resource-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-kext-resources.py"
        --out "${CMAKE_BINARY_DIR}/kext-resources")
    add_test(NAME rtw-controller-scan-completion-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-controller-scan-completion.py"
        --out "${CMAKE_BINARY_DIR}/controller-scan-completion")
    add_test(NAME rtw-control-scan-admission-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-control-scan-admission.py"
        --out "${CMAKE_BINARY_DIR}/control-scan-admission")
    add_test(NAME rtw-usb-termination-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-usb-termination.py"
        --out "${CMAKE_BINARY_DIR}/usb-termination")
    foreach(part core datapath backend imported-ht)
        set(station_control_args)
        if(part STREQUAL "backend")
            list(APPEND station_control_args --control)
        endif()
        if(part STREQUAL "imported-ht")
            add_test(NAME rtw-rtl8723bu-${part}-tests COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8723bu-${part}.py")
        else()
            add_test(NAME rtw-rtl8723bu-${part}-tests COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8723bu-${part}.py"
                --out "${CMAKE_BINARY_DIR}/rtl8723bu-${part}" ${station_control_args})
        endif()
    endforeach()
    foreach(part backend negotiation)
        set(station_control_args)
        if(part STREQUAL "backend")
            list(APPEND station_control_args --control)
        endif()
        add_test(NAME rtw-rtl8710bu-${part}-tests COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8710bu-${part}.py" ${station_control_args})
    endforeach()
    foreach(part core backend negotiation)
        set(station_control_args)
        if(part STREQUAL "backend")
            list(APPEND station_control_args --control)
        endif()
        if(part STREQUAL "core")
            set(fu_runner "test-rtl8192fu.py")
        else()
            set(fu_runner "test-rtl8192fu-${part}.py")
        endif()
        add_test(NAME rtw-rtl8192fu-${part}-tests COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/${fu_runner}" ${station_control_args})
    endforeach()
    add_test(NAME rtw-legacy-usb-edca-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-legacy-usb-edca.py")
    add_test(NAME rtw-rtl8814b-phy-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtw8814b-phy.py")
    add_test(NAME rtw-rtl8814b-hal-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8814b-hal.py" --sanitize)
    add_test(NAME rtw-rtl8821au-coex-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8821au-coex.py")
    add_test(NAME rtw-rtw88-coex-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtw88-coex.py")
    add_test(NAME rtw-rtw88-coex-backend-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtw88-coex-backend.py")
    add_test(NAME rtw-rtl8192du-negotiation-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8192du-negotiation.py")
    add_test(NAME rtw-rtl8192du-backend-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8192du-backend.py" --library "$<TARGET_FILE:rtwcore>" --control)
    add_test(NAME rtw-rtw88-bootstrap-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtw88-bootstrap.py")
    add_test(NAME rtw-rtl8812au-negotiation-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8812au-negotiation.py")
    add_test(NAME rtw-rtl8812au-modes-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8812au-modes.py")
    add_test(NAME rtw-realtek-imported-ht-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-realtek-imported-ht.py")
    add_test(NAME rtw-rtl8812au-backend-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8812au-backend.py" --control)
    add_test(NAME rtw-rtw88-backend-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtw88-backend.py" --control)
    add_test(NAME rtw-wpa-recovery-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-wpa-recovery.py"
        --out "${CMAKE_BINARY_DIR}/rtwn-wpa-recovery")
    add_test(NAME rtw-english-ui-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/english_ui_tests.py")
    add_test(NAME rtw-signal-pipeline-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/signal_pipeline_tests.py")
    add_test(NAME rtw-rssi-feedback-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rssi-feedback.py"
        --out "${CMAKE_BINARY_DIR}/rssi-feedback")
    add_test(NAME rtw-controller-family-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/controller_family_tests.py"
        --out "${CMAKE_BINARY_DIR}/controller-family")
    add_test(NAME rtw-adapter-support-contract-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/adapter_support_contract_tests.py")
    add_test(NAME rtw-usb-wifi-catalog-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/usb_wifi_catalog_tests.py")
    add_test(NAME rtw-legacy-efuse-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/legacy_efuse_core.py")
    add_test(NAME rtw-legacy-ra-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/legacy_ra_core.py")
    add_test(NAME rtw-92e-iq-calib-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/iq_calib_core.py")
    add_test(NAME rtw-92e-iq-chain-trace-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/iq_chain_trace.py")
    add_test(NAME rtw-92e-iqk-protocol-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/r92e_iqk_trace.py")
    add_test(NAME rtw-92e-thermal-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/thermal_core.py")
    add_test(NAME rtw-kext-personality-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/kext_personality_tests.py")
    add_test(NAME rtw-tahoe-validation-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/tahoe_validation_tests.py")
    add_test(NAME rtw-pack-diagnostics-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/pack_diagnostics_tests.py")
    add_test(NAME rtw-wifi-diagnostics-command-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/wifi_diagnostics_command_tests.py")
    add_test(NAME rtw-dfs-regulatory-generator-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/dfs_regulatory_generator_tests.py")
    add_test(NAME rtw-controller-power-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-controller-power.py"
        --out "${CMAKE_BINARY_DIR}/controller-power")
    add_test(NAME rtw-dfs-station-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-dfs-station.py"
        --out "${CMAKE_BINARY_DIR}/dfs-station")
    add_test(NAME rtw-8814au-channel-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8814au-channel.py"
        --out "${CMAKE_BINARY_DIR}/8814-channel")
    add_test(NAME rtw-channel-map-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-channel-map.py"
        --out "${CMAKE_BINARY_DIR}/channel-map")
    set_tests_properties(rtw-wifi-diagnostics-command-tests PROPERTIES TIMEOUT 120)
    add_test(NAME rtw-92e-efuse-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rtwn/tests/efuse_core.py")
    add_test(NAME rtw-gen1-datapath-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8192c-datapath.py"
        --out "${CMAKE_BINARY_DIR}/gen1-datapath")
    foreach(ht_script rtwn-ht net80211-ht net80211-ba rtwn-ht-tx)
        add_test(NAME rtw-${ht_script}-tests COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-${ht_script}.py"
            --out "${CMAKE_BINARY_DIR}/${ht_script}")
    endforeach()
    add_test(NAME rtw-net80211-input-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-input.py"
        --out "${CMAKE_BINARY_DIR}/net80211-input")
    add_test(NAME rtw-fw-command-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-fw-command.py"
        --out "${CMAKE_BINARY_DIR}/rtwn-fw-command")
    add_test(NAME rtw-rate-control-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-rate-control.py"
        --out "${CMAKE_BINARY_DIR}/rtwn-rate-control")
    add_test(NAME rtw-wpa-diagnostics-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-wpa-diagnostics.py")

    add_test(NAME rtw-rx-order-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test-rtwn-rx-order.py"
        --out "${CMAKE_BINARY_DIR}/rtwn-rx-order")
    add_test(NAME rtw-performance-diagnostics-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-performance.py"
        --out "${CMAKE_BINARY_DIR}/rtwn-performance")
    add_test(NAME rtw-net80211-crypto-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-crypto.py"
        --out "${CMAKE_BINARY_DIR}/net80211-crypto")
    add_test(NAME rtw-net80211-taskq-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-taskq.py"
        --out "${CMAKE_BINARY_DIR}/net80211-taskq")
    foreach(part boot phy datapath station backend attach recovery)
        set(RTW_BACKEND_TEST_ARGS)
        if(NOT part STREQUAL "station")
            list(APPEND RTW_BACKEND_TEST_ARGS
                --out "${CMAKE_BINARY_DIR}/rtl8814au-${part}")
        endif()
        add_test(NAME rtw-8814au-${part}-tests COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtl8814au-${part}.py"
            ${RTW_BACKEND_TEST_ARGS})
    endforeach()
    add_test(NAME rtw-usb-rx-recovery-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-rx-recovery.py" --out "${CMAKE_BINARY_DIR}/usb-rx-recovery")
    add_test(NAME rtw-usb-rx-capacity-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-rx-capacity.py" --out "${CMAKE_BINARY_DIR}/usb-rx-capacity")
    add_test(NAME rtw-8814-rate-thermal-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-8814-rate-thermal.py" --out "${CMAKE_BINARY_DIR}/8814-rate-thermal")
    add_test(NAME rtwn-publication-regression COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-publication.py" --out "${CMAKE_BINARY_DIR}/rtwn-publication")
    add_test(NAME rtw-tx-backpressure-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-rtwn-tx-backpressure.py" --out "${CMAKE_BINARY_DIR}/rtwn-tx-backpressure")
    add_test(NAME rtw-net80211-scan-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-scan.py" --out "${CMAKE_BINARY_DIR}/net80211-scan")
    add_test(NAME rtw-net80211-runtime-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-runtime.py"
        --out "${CMAKE_BINARY_DIR}/net80211-runtime")
    add_test(NAME rtw-net80211-pae-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-pae.py"
        --out "${CMAKE_BINARY_DIR}/net80211-pae")
    add_test(NAME rtw-usb-wifi-extensions-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/usb_wifi_extensions_tests.py")
    add_test(NAME rtw-usb-wifi-matrix-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/usb_wifi_matrix_tests.py")
    add_executable(rtw-rx-preflight-tests tests/rx_preflight_tests.cpp)
    target_link_libraries(rtw-rx-preflight-tests PRIVATE rtwcore)
    add_test(NAME rtw-rx-preflight-tests COMMAND rtw-rx-preflight-tests)

    add_executable(rtw-usb-probe-tests tests/usb_probe_tests.cpp)
    target_link_libraries(rtw-usb-probe-tests PRIVATE rtwcore)
    add_test(NAME rtw-kext-linkage-tests COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/kext_linkage_tests.py")
    add_test(NAME rtw-usb-probe-tests COMMAND rtw-usb-probe-tests)

    add_executable(rtw-ethernet-bridge-tests tests/ethernet_bridge_tests.cpp)
    target_link_libraries(rtw-ethernet-bridge-tests PRIVATE rtwcore)
    add_test(NAME rtw-ethernet-bridge-tests COMMAND rtw-ethernet-bridge-tests)

    add_executable(rtw-tests tests/tests.cpp)
    target_link_libraries(rtw-tests PRIVATE rtwcore)
    add_test(NAME rtw-tests COMMAND rtw-tests)

    add_executable(rtw-driver-tests tests/driver_core_tests.cpp)
    target_link_libraries(rtw-driver-tests PRIVATE rtwcore)
    add_test(NAME rtw-driver-tests COMMAND rtw-driver-tests)

    add_executable(rtw-hal-tests tests/hal_tests.cpp)
    target_link_libraries(rtw-hal-tests PRIVATE rtwcore)
    add_test(NAME rtw-hal-tests COMMAND rtw-hal-tests)

    add_executable(rtw-hw-tests tests/hw_tests.cpp)
    target_link_libraries(rtw-hw-tests PRIVATE rtwcore)
    add_test(NAME rtw-hw-tests COMMAND rtw-hw-tests)

    add_executable(rtw-monitor-tests tests/monitor_tests.cpp)
    target_link_libraries(rtw-monitor-tests PRIVATE rtwcore)
    add_test(NAME rtw-monitor-tests COMMAND rtw-monitor-tests)

    add_executable(rtw-scan-diagnostics-tests tests/scan_diagnostics_tests.cpp)
    target_include_directories(rtw-scan-diagnostics-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
    add_test(NAME rtw-scan-diagnostics-tests COMMAND rtw-scan-diagnostics-tests)

    add_executable(rtw-station-tests tests/station_mgmt_tests.cpp)
    target_link_libraries(rtw-station-tests PRIVATE rtwcore)
    add_test(NAME rtw-station-tests COMMAND rtw-station-tests)

    add_executable(rtw-8192eu-tests tests/rtl8192eu_tests.cpp)
    target_link_libraries(rtw-8192eu-tests PRIVATE rtwcore)
    add_test(NAME rtw-8192eu-tests COMMAND rtw-8192eu-tests)

    add_executable(rtw-8192eu-hardware-tests tests/rtl8192eu_hardware_tests.cpp)
    target_link_libraries(rtw-8192eu-hardware-tests PRIVATE rtwcore)
    add_test(NAME rtw-8192eu-hardware-tests COMMAND rtw-8192eu-hardware-tests)
endif()

if(BUILD_TESTING)
    add_test(NAME rtw-family-recovery-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-family-recovery.py")
    add_test(NAME rtw-family-controller-recovery-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-family-controller-recovery.py")
    add_test(NAME rtw-legacy-controller-recovery-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-legacy-controller-recovery.py")
    add_test(NAME rtw-legacy-power-off-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-legacy-power-off.py"
        --out "${CMAKE_BINARY_DIR}/legacy-power-off")
    add_test(NAME rtw-net80211-iecache-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-iecache.py"
        --out "${CMAKE_BINARY_DIR}/net80211-iecache")
    add_test(NAME rtw-net80211-channel-capabilities-tests COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-net80211-channel-capabilities.py"
        --out "${CMAKE_BINARY_DIR}/net80211-channel-capabilities")
    get_property(RTW_TEST_NAMES DIRECTORY PROPERTY TESTS)
    set_tests_properties(${RTW_TEST_NAMES} PROPERTIES
        ENVIRONMENT "RTW_HOST_SANITIZERS=${RTW_HOST_SANITIZERS}"
        TIMEOUT 180)
    # The installer suite launches many bounded subprocess fixtures. Keep it
    # separate from sanitizer compilation to avoid scheduler-induced timeouts.
    set_tests_properties(rtw-app-pkg-tests PROPERTIES RUN_SERIAL TRUE)
    if(APPLE)
        # Invalid-request fixtures call the production logger before XPC.
        # Keep their simulated errors outside the installed app's user log.
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/diagnostic-test-home")
        set_property(TEST rtw-menu-diagnostic-cli-tests APPEND PROPERTY ENVIRONMENT
            "CFFIXED_USER_HOME=${CMAKE_BINARY_DIR}/diagnostic-test-home")
    endif()
endif()
