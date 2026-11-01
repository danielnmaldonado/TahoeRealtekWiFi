#!/usr/bin/env python3
"""Compile real controller publication methods; no hardware or control IPC."""
from pathlib import Path
import argparse
import re
import subprocess
from host_cpp import compiler, sanitizer_flags
from test_import_helpers import block


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-rtwn-publication")
    parser.add_argument("--controller-dir", type=Path, default=project / "kext")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    controller = (args.controller_dir / "RTWUSBController.cpp").read_text()
    header = (args.controller_dir / "RTWUSBController.hpp").read_text()
    host = (project / "tests/rtwn_rate_control_host.hpp").read_text()
    ioctl = (project / "vendor/itl80211/openbsd/net80211/ieee80211_ioctl.h").read_text()
    stats = block(ioctl, "struct ieee80211_stats {").replace("u_int32_t", "uint32_t") + ";\n"
    host = host.replace("struct ieee80211com {", '#include "vendor/itl80211/openbsd/sys/_input_diagnostics.h"\n' + stats + "struct ieee80211com { struct { rtw_if_input_diagnostics if_input_diag{}; } ic_if;")
    host = host.replace("struct { uint32_t is_rx_eapol_key = 0, is_rx_eapol_replay = 0, is_rx_eapol_badmic = 0; } ic_stats;", "ieee80211_stats ic_stats{};")
    # Minimal platform boundary only; the native Controller build checks the
    # actual KPI types. Keep portable host tests independent of the Mac SDK.
    native_host = '''
struct ifnet_stats_param { uint64_t packets_in=0,bytes_in=0,errors_in=0; };
struct publication_host_ifnet { ifnet_stats_param stats{}; int error=0; unsigned reads=0,index=7; };
using ifnet_t=publication_host_ifnet*;
inline int ifnet_stat(ifnet_t iface, ifnet_stats_param *out) { ++iface->reads; *out=iface->stats; return iface->error; }
inline unsigned ifnet_index(ifnet_t iface) { return iface->index; }
struct publication_host_interface { ifnet_t iface=nullptr; ifnet_t getIfnet() const { return iface; } };
'''
    host = host.replace("class RTWUSBController {", native_host + "class RTWUSBController {")
    additions = '''
    struct IONetworkMedium { uint64_t speed; static IONetworkMedium *medium(unsigned,uint64_t speed) { return new IONetworkMedium{speed}; } };
    using IOReturn = int;
    static constexpr int kIOReturnSuccess=0, kIOReturnBadArgument=1;
    static constexpr unsigned kIOMediumEthernetAuto=0;
    unsigned selectedCalls=0,currentCalls=0;
    bool setSelectedMedium(const IONetworkMedium *m) { ++selectedCalls; return m; }
    bool setCurrentMedium(const IONetworkMedium *m) { ++currentCalls; return m; }
    IOReturn selectMedium(const IONetworkMedium *);
    uint64_t actualAutomaticMediumSpeed();
    rtw_signal_sample signal_{};
    struct { uint64_t inputPackets=0,inputErrors=0; } statsStorage_, *stats_=&statsStorage_;
    uint64_t txQueueDrops_=0,txUSBTransferErrors_=0,txLinkDrops_=0,rxPackets_=0;
    uint64_t diagnosticPublicationGeneration_=0,diagnosticPublicationMonotonicMS_=0;
    bool attached_=true,closing_=false;
    publication_host_interface *netif_=nullptr;
    static uint64_t nowMS() { return publicationHostNow; }
    void publishDiagnostics();
    void publishCalibrationDiagnostics() {}
    void publishScanDiagnostics() {}
    void publishRXRecoveryDiagnostics() {}
    void publishWpaRecoveryDiagnostics() {}
    void publishPerformanceDiagnostics() {}
'''
    host = host.replace("class RTWUSBController {", '#include "rtw/SignalSample.h"\ninline uint64_t publicationHostNow=100;\nclass RTWUSBController {')
    host = host.replace("public:\n", "public:\n" + additions + block(header, "struct RXDiagnostics {") + " rxDiag_{};\n", 1)
    host = host.replace("unsigned propertyWrites=0;", "unsigned propertyWrites=0; std::vector<std::pair<std::string,uint64_t>> writes;")
    host = host.replace("properties[name]=value;", "properties[name]=value; writes.emplace_back(name,value);")
    host = host.replace("#include <map>", "#include <map>\n#include <vector>")
    (args.out / "publication_host.hpp").write_text(host)
    methods = [block(controller, "void RTWUSBController::publishDiagnostics()")]
    if "void RTWUSBController::publishRateControlDiagnostics()" in controller:
        methods.append(block(controller, "void RTWUSBController::publishRateControlDiagnostics()"))
    methods.append(block(controller, "IOReturn RTWUSBController::selectMedium("))
    methods[-1] = methods[-1].replace("IOReturn RTWUSBController::", "RTWUSBController::IOReturn RTWUSBController::", 1)
    medium = re.search(r"IONetworkMedium\*automatic=(IONetworkMedium::medium\([^;]+\));", controller)
    assert medium, "startup automatic medium expression missing"
    methods.append("uint64_t RTWUSBController::actualAutomaticMediumSpeed() { auto *medium=" + medium.group(1) + "; const auto speed=medium->speed; delete medium; return speed; }")
    family = (project / "kext/RTWFamilyDataPath.inc").read_text()
    ht = (project / "kext/RTWHTDataPath.inc").read_text()
    methods += [block(ht, "bool RTWUSBController::usesFirmwareRate() const"),
                block(family, "bool RTWUSBController::usesLegacyFirmwareRate() const"),
                block(family, "bool RTWUSBController::uses8814FirmwareRate() const")]
    generated = args.out / "actual_publication.cpp"
    generated.write_text('#include "publication_host.hpp"\n' + "\n".join(methods))
    executable = args.out / "rtwn-publication-tests"
    subprocess.run([*compiler(), "-std=c++17", "-O1", "-g", *sanitizer_flags(),
                    "-fno-omit-frame-pointer", "-Wall", "-Wextra", "-Werror",
                    "-I" + str(project), "-I" + str(project / "include"), "-I" + str(args.out),
                    str(generated), str(project / "tests/rtwn_publication_tests.cpp"),
                    "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
