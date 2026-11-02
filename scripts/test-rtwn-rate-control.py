#!/usr/bin/env python3
"""Offline CCX fixtures against actual controller methods and vendor AMRR."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import re
import subprocess


def method(source, name):
    match = re.search(r"\b(?:void|bool|uint8_t|rtw_packet_error) RTWUSBController::" + re.escape(name) + r"\(", source)
    if not match: raise ValueError(name)
    start = match.start()
    opening = source.index("{", start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:]):
        if token.group() == "{": depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0: return source[start:opening + token.end()] + "\n"
    raise ValueError(name)


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-rtwn-rate-control")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    controller = (project / "kext/RTWUSBController.cpp").read_text()
    ht = (project / "kext/RTWHTDataPath.inc").read_text()
    family = (project / "kext/RTWFamilyDataPath.inc").read_text()
    r23 = (project / "kext/RTW8723BUBackend.inc").read_text()
    bu = (project / "kext/RTW8710BUBackend.inc").read_text()
    fu = (project / "kext/RTW8192FUBackend.inc").read_text()
    au = (project / "kext/RTW8812Backend.inc").read_text()
    du = (project / "kext/RTW8192DUBackend.inc").read_text()
    bc = (project / "kext/RTW88Backend.inc").read_text()
    amrr = (project / "vendor/itl80211/openbsd/net80211/ieee80211_amrr.c").read_text()
    # All AMRR algorithm text is compiled verbatim, including static helpers.
    closure = '#include "rtwn_rate_control_host.hpp"\n' + amrr[amrr.index("#define is_success"):]
    closure += "\n" + method(ht, "usesFirmwareRate")
    closure += "\n" + method(family, "usesLegacyFirmwareRate")
    for name in ("usesLegacyHighThroughput","acceptsLegacyReceiveRate","legacyHostHTRate"):
        closure += "\n" + method(family,name)
    closure += "\n" + method(family, "uses8814FirmwareRate")
    closure += "\n" + method(family, "handle8814C2H")
    closure += "\n" + method(r23,"decode8723USBFrame")
    closure += "\n" + method(bu,"decode8710BUUSBFrame")
    closure += "\n" + method(fu,"decode8192FUUSBFrame")
    closure += "\n" + method(au,"decode8812USBFrame")
    closure += "\n" + method(bc,"decode88USBFrame")
    closure += "\n" + method(du,"decode8192DUUSBFrame")
    closure += "\n" + method(family, "decodeUSBFrame")
    for name in ("resetRateControl", "handleTxReport", "chooseRateControl", "publishRateControlDiagnostics"):
        closure += "\n" + method(controller, name)
    generated = args.out / "actual_rate_control.cpp"
    generated.write_text(closure)
    subprocess.run([*compiler(), "-std=c++17", "-g", "-O1", "-x", "c++",
        *sanitizer_flags(), "-fno-omit-frame-pointer", "-Wall", "-Wextra", "-Werror",
        "-I" + str(project), "-I" + str(project / "include"), "-I" + str(project / "tests"),
        str(generated), str(project / "src/RTL8192EUDataPath.c"), str(project / "src/RTL8192CDataPath.c"),
        str(project / "src/RTL8723BUDataPath.c"), str(project / "src/RTL8710BUDataPath.c"), str(project / "src/RTL8192FUDataPath.c"), str(project / "src/RTL8814AUDataPath.c"), str(project / "src/RTL8812AUDataPath.c"), str(project / "src/RTL8192DUDataPath.c"), str(project / "src/FCS.c"), str(project / "src/RTW88RX.c"),
        str(project / "src/RTW88Station.c"),str(project / "src/RTW88Firmware.c"),str(project / "src/RTW88USBDataPath.c"),str(project / "src/FamilyRX24.c"), str(project / "src/USBWiFiProfiles.c"), str(project / "src/RTL8192EUHT.c"),
        str(project / "src/USBPackets.c"), str(project / "src/RTL8192EU.c"),
        str(project / "src/Efuse.c"), str(project / "src/USBRegisterIO.c"), str(project / "src/USBControl.c"),
        str(project / "tests/rtwn_rate_control_tests.cpp"),
        "-o", str(args.out / "rtwn-rate-control-tests")], check=True)
    subprocess.run([str(args.out / "rtwn-rate-control-tests")], check=True)


if __name__ == "__main__": main()
