#!/usr/bin/env python3
"""Offline scan regression using actual net80211 code and backend channel maps.

The per-family board/active-mask profiles are modeled contracts, not physical
adapter, RF, USB or association certification.
"""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import importlib.util
import re
import subprocess

project = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("pae_helpers", project / "scripts/test-net80211-pae.py")
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--out", type=Path, default=project / "build/net80211-scan")
parser.add_argument("--node-source", type=Path, default=project / "vendor/itl80211/openbsd/net80211/ieee80211_node.c")
parser.add_argument("--backend-source", type=Path, default=project / "kext/RTW8814Backend.inc")
parser.add_argument("--backend-8192du", type=Path, default=project / "kext/RTW8192DUBackend.inc")
parser.add_argument("--backend-8812", type=Path, default=project / "kext/RTW8812Backend.inc")
parser.add_argument("--backend-88", type=Path, default=project / "kext/RTW88Backend.inc")
parser.add_argument("--case", choices=["all", "au", "dual", "families", "8192du", "8812au", "8814au", "8821au", "8821cu", "8822bu", "8822cu"], default="all")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
source = args.node_source.read_text()
closure = '#include "net80211_scan_host.hpp"\n#include "rtw/StationChannels.h"\n#include "rtw/RTW88Station.h"\n'
for name in ["ieee80211_node_checkrssi", "ieee80211_reset_scan", "ieee80211_begin_scan", "ieee80211_next_scan",
             "ieee80211_node_choose_bss", "ieee80211_end_scan"]:
    closure += '\n// Verbatim production function\n' + helpers.function(source, name)
# Execute the real base capabilities and channel registration statements. HAL
# board support and HT/VHT geometry are independent tested boundaries; supply
# an admitted board here. Active scan profiles below restrict the registered
# map without claiming that all ROM/board combinations are physically known.
backends = [
    ("8814", args.backend_source, "attach8814", "    uint8_t initial=0;", "    if(!initial)return ENOTSUP;"),
    ("8192du", args.backend_8192du, "attach8192DU", "    for(unsigned ch=1;ch<=11;ch++){", "    ic.ic_ibss_chan="),
    ("8812", args.backend_8812, "attach8812", "    uint8_t initial=0;", "    if(!initial)return ENOTSUP;"),
    ("88", args.backend_88, "attach88", " for(uint8_t ch=1;ch<=11;ch++){", " ic.ic_ibss_chan="),
]
for name, path, method, start, end in backends:
    attach = path.read_text().split("int RTWUSBController::" + method + "()", 1)[1]
    caps = re.search(r"ic\.ic_caps\s*=[^;]+;", attach)
    if not caps:
        raise ValueError("No base capability assignment for " + name)
    registration = attach[attach.index(start):attach.index(end, attach.index(start))]
    closure += '\nvoid register_' + name + '_scan_channels(ieee80211com &ic) {\n'
    closure += 'auto permitsDFSChannelByCountry=[](uint8_t) { return true; };\n'
    if name == "88":
        closure += 'auto permits88Channel=[](uint8_t ch) { return (ch>=1&&ch<=11)||rtw_station_5ghz_channel(ch); };\n'
    if name == "8814":
        closure += 'int acCore_=0;\n'
    elif name == "8812":
        closure += '''struct { struct { int phy=0; } board; } auCore_;
bool auHTSupported_=false, auVHTSupported_=false;
auto supports8812Band=[](bool) { return true; };
auto channel8812Modes=[](uint8_t) { return unsigned(IEEE80211_CHAN_HT20 | IEEE80211_CHAN_VHT20); };
'''
    closure += caps.group() + '\n' + registration + '\n}\n'
closure += '\nunsigned au_scan_capabilities() { ieee80211com ic{}; register_8814_scan_channels(ic); return ic.ic_caps; }\n'
(args.out / "actual_scan.cpp").write_text(closure)
# The actual B/C initializer calls the production station geometry gate.
# Link its real C dependency closure; do not replace it with a fixture model.
objects=[]
for name in ["RTW88Station", "RTL8814AUDataPath", "RTW88Firmware", "RTW88USBDataPath", "USBPackets"]:
    obj=args.out/(name+".o");objects.append(str(obj))
    subprocess.run([*compiler("c"), "-std=c11", "-g", "-O1", *sanitizer_flags(),
                    "-I"+str(project/"include"), "-c", str(project/"src"/(name+".c")), "-o", str(obj)],check=True)
command = [*compiler(), "-std=c++14", "-g", "-O1", *sanitizer_flags(),
           "-fno-omit-frame-pointer", "-DIEEE80211_STA_ONLY", "-Werror=return-type",
           "-I" + str(project / "include"), "-I" + str(project / "tests"), "-I" + str(project / "vendor/itl80211/openbsd"),
           str(args.out / "actual_scan.cpp"), str(project / "tests/net80211_scan_tests.cpp"),
           *objects,
           "-o", str(args.out / "net80211-scan-tests")]
subprocess.run(command, check=True)
subprocess.run([str(args.out / "net80211-scan-tests"), args.case], check=True)
