#!/usr/bin/env python3
"""Native controller C2H/TX -> actual 8814 PHY thermal state; no hardware I/O."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse
import runpy
import subprocess


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-rtwn-8814-rate-thermal")
    parser.add_argument("--case", default="all")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    body = runpy.run_path(str(project / "scripts/test-rtwn-ht-tx.py"))["body"]
    controller = (project / "kext/RTWUSBController.cpp").read_text()
    family = (project / "kext/RTWFamilyDataPath.inc").read_text()
    ht = (project / "kext/RTWHTDataPath.inc").read_text()
    output = (project / "vendor/itl80211/openbsd/net80211/ieee80211_output.c").read_text()
    closure = '#include "rtwn_ht_tx_host.hpp"\n'
    closure += body(output, "ieee80211_up_to_ac(", "enum ieee80211_edca_ac\n")
    closure += body(ht, "bool RTWUSBController::usesFirmwareRate(")
    for result, name in (("bool", "usesLegacyFirmwareRate"),
                         ("bool", "uses8814FirmwareRate"),
                         ("void", "handle8814C2H")):
        closure += body(family, result + " RTWUSBController::" + name + "(")
    closure += body(controller, "int RTWUSBController::sendFrame(")
    generated = args.out / "actual_controller_rate_thermal.cpp"
    generated.write_text(closure)
    flags = ["-g", "-O1", *sanitizer_flags(), "-fno-omit-frame-pointer",
             "-Wall", "-Wextra", "-Werror", "-I" + str(project / "include"),
             "-I" + str(project / "tests"), "-I" + str(project)]
    objects = []
    sources = [project / "src" / (name + ".c") for name in (
        "RTL8814AUBoot", "RTL8814AUEfuse", "RTL8814AUMAC",
        "RTL8814AUDataPath", "RTL8192EUDataPath", "RTL8192CDataPath",
        "RTL8192EUHT", "USBPackets")]
    sources.append(project / "tests/rtwn_8814_rate_phy_fixture.c")
    for source in sources:
        obj = args.out / (source.stem + ".o")
        subprocess.run([*compiler("c"), "-std=c11", *flags, "-c", str(source),
                        "-o", str(obj)], check=True)
        objects.append(str(obj))
    binary = args.out / "rtwn-8814-rate-thermal-tests"
    subprocess.run([*compiler(), "-std=c++17", "-DRTW_HT_TX_NATIVE_8814_PHY=1",
                    *flags, str(generated),
                    str(project / "tests/rtwn_8814_rate_thermal_tests.cpp"),
                    *objects, "-o", str(binary)], check=True)
    subprocess.run([str(binary), args.case], check=True)


if __name__ == "__main__":
    main()
