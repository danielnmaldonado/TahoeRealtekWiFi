#!/usr/bin/env python3
"""Offline receive-rate diagnostics using actual controller methods/types."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import re
import subprocess


def block(source, marker):
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:opening + token.end()]
    raise ValueError(marker)


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-rtwn-performance")
    parser.add_argument("--red-baseline", action="store_true",
                        help="Allow inert absent methods only before implementation exists")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    controller = (project / "kext/RTWUSBController.cpp").read_text()
    header = (project / "kext/RTWUSBController.hpp").read_text()
    wire = (project / "vendor/itl80211/openbsd/net80211/ieee80211.h").read_text()
    var = (project / "vendor/itl80211/openbsd/net80211/ieee80211_var.h").read_text()
    implementation = project / "kext/RTWPerformanceDiagnostics.inc"
    if args.red_baseline:
        if implementation.exists():
            parser.error("RED baseline is only permitted while methods are absent")
        structure = "struct PerformanceDiagnostics { uint64_t rxRatePackets[12]{}, rxRateBytes[12]{}; uint64_t rxUnknownPackets=0, rxUnknownBytes=0; uint8_t rxLastCode=255, rxLastRate500Kbps=0; }"
        methods = "void RTWUSBController::observeReceivedRate(const rtw_rx_packet&) {}\nvoid RTWUSBController::publishPerformanceDiagnostics() {}\n"
    else:
        structure = block(header, "struct PerformanceDiagnostics")
        methods = implementation.read_text()
    constants = []
    for name in ("IEEE80211_ADDR_LEN", "IEEE80211_FC0_VERSION_MASK", "IEEE80211_FC0_TYPE_MASK",
                 "IEEE80211_FC0_TYPE_DATA", "IEEE80211_FC0_SUBTYPE_NODATA", "IEEE80211_FC0_SUBTYPE_QOS", "IEEE80211_FC1_DIR_MASK",
                 "IEEE80211_FC1_DIR_FROMDS", "IEEE80211_F_USEPROT"):
        match = re.search(r"^#define\s+" + name + r"\s+[^\n]+", wire + "\n" + var, re.M)
        if not match:
            raise ValueError(name)
        constants.append(match.group())
    closure = '''#pragma once
#include <cstdint>
#include "rtw/DiagnosticPublishPolicy.hpp"
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include "rtw/USBPackets.h"
#include "rtw/USBWiFiProfiles.h"
#define __packed __attribute__((packed))
#define IEEE80211_S_RUN 4
#define kIOReturnSuccess 0
#define IEEE80211_ADDR_EQ(a,b) (std::memcmp(a,b,6)==0)
''' + "\n".join(constants) + "\n" + block(wire, "struct ieee80211_frame {") + ''' __packed;
struct ieee80211_node { uint8_t ni_bssid[6]{2,0,0,0,0,1}; bool ni_port_valid=true; uint8_t ni_erp=0; };
struct ieee80211com { int ic_state=IEEE80211_S_RUN; ieee80211_node *ic_bss=nullptr;
    uint8_t ic_myaddr[6]{2,0,0,0,0,2}; unsigned ic_flags=0, ic_rtsthreshold=2346; };
class RTWUSBController {
public:
    bool dfsTXBlocked()const{return false;}
    bool attached_=true, running_=true, radioReady_=true, joinRequested_=true;
    bool present_=true, sleeping_=false, stopping_=false;
    int usbError_=0;
    rtw_wifi_family family_=RTW_WIFI_RTL8192EU;
    struct { int io_error=0; bool phy_initialized=true; } acCore_;
    struct {bool ht=false,sgi20=false;uint64_t mask=0;unsigned tx_streams=0,rate_id=0;} acRateConfig_;
    bool uses8814FirmwareRate() const { return true; }
    bool is8812Family()const{return family_==RTW_WIFI_RTL8812AU||family_==RTW_WIFI_RTL8821AU;}
    bool is88Family()const{return family_==RTW_WIFI_RTL8821CU||family_==RTW_WIFI_RTL8822BU||family_==RTW_WIFI_RTL8822CU;}
    bool auFirmwareReady_=false,r88FirmwareReady_=false;
    bool uses8812FirmwareRate()const{return auFirmwareReady_;}
    bool uses88FirmwareRate()const{return r88FirmwareReady_;}
    bool is8723Family()const{return family_==RTW_WIFI_RTL8723BU;}
    bool r23FirmwareReady_=false;
    bool uses8723FirmwareRate()const{return is8723Family()&&r23FirmwareReady_;}
    bool is8710BUFamily()const{return family_==RTW_WIFI_RTL8710BU;}
    bool buFirmwareReady_=false;
    bool uses8710BUFirmwareRate()const{return is8710BUFamily()&&buFirmwareReady_;}
    bool is8192FUFamily()const{return family_==RTW_WIFI_RTL8192FU;}
    bool uses8192FUFirmwareRate()const{return fuFirmwareReady_;}
    bool fuFirmwareReady_=false;
    bool uses8192DUFirmwareRate()const{return false;}

    struct { ieee80211com sc_ic; int core_error=0; } core_;
''' + structure + ''' perfDiag_{};
    std::map<std::string,uint64_t> properties;
    void setProperty(const char *name,uint64_t value,unsigned bits) {
        if (bits!=64) std::abort(); properties[name]=value;
    }
    void publishDiagnosticCounter(const char *name, uint64_t value) {
        const auto previous = properties.find(name);
        if (previous == properties.end() ||
            rtw::diagnosticCounterNeedsWrite(true,64,previous->second,value)) {
            setProperty(name,value,64);
        }
    }
    bool authorized() const;
    void observeReceivedRate(const rtw_rx_packet&);
    void publishPerformanceDiagnostics();
};
'''
    (args.out / "actual_performance_host.hpp").write_text(closure)
    generated = args.out / "actual_performance.cpp"
    generated.write_text('#include "actual_performance_host.hpp"\n' +
                         block(controller, "bool RTWUSBController::authorized() const") + "\n" + methods)
    executable = args.out / "rtwn-performance-tests"
    subprocess.run([*compiler(), "-std=c++17", "-g", "-O1",
        *sanitizer_flags(), "-fno-omit-frame-pointer", "-Wall", "-Wextra", "-Werror",
        "-I" + str(args.out), "-I" + str(project / "include"), str(generated),
        str(project / "tests/rtwn_performance_tests.cpp"), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
