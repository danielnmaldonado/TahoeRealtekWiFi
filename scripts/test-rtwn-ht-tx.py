#!/usr/bin/env python3
"""Exercise verbatim controller TX/BA decisions with offline ownership/USB boundaries."""
from host_cpp import compiler, sanitizer_flags
from pathlib import Path
import argparse
import re
import subprocess

def body(source, marker, prefix=""):
    start=source.index(marker)
    opening=source.index("{",start)
    depth=0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]',source[opening:]):
        if token.group()=="{": depth+=1
        elif token.group()=="}":
            depth-=1
            if depth==0:return prefix+source[start:opening+token.end()]+"\n"
    raise ValueError(marker)

def main():
    project=Path(__file__).resolve().parents[1]
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out",type=Path,default=project/"out-rtwn-ht-tx")
    parser.add_argument("--case",choices=["all","au-ba","au-ampdu"],default="all")
    args=parser.parse_args();args.out.mkdir(parents=True,exist_ok=True)
    controller=(project/"kext/RTWUSBController.cpp").read_text()
    ht=(project/"kext/RTWHTDataPath.inc").read_text()
    family=(project/"kext/RTWFamilyDataPath.inc").read_text()
    output=(project/"vendor/itl80211/openbsd/net80211/ieee80211_output.c").read_text()
    closure='#include "rtwn_ht_tx_host.hpp"\n'
    boot=(project/"src/RTL8814AUBoot.c").read_text()
    closure+=body(boot,"int rtw14_fail(")
    closure+=body(boot,"int rtw_8814au_write(")
    closure+=body(output,"ieee80211_up_to_ac(","enum ieee80211_edca_ac\n")
    for result,name in (("bool","usesFirmwareRate"),("int","ampduRXStart"),("int","ampduTXStart"),("void","ampduRXStop"),("void","ampduTXStop")):
        closure+=body(ht,result+" RTWUSBController::"+name+"(")
    closure+=body(family,"bool RTWUSBController::usesLegacyFirmwareRate(")
    closure+=body(family,"bool RTWUSBController::uses8814FirmwareRate(")
    closure+=body(controller,"int RTWUSBController::sendFrame(")
    generated=args.out/"actual_ht_tx.cpp";generated.write_text(closure)
    subprocess.run([*compiler(),"-std=c++17","-g","-O1","-x","c++",*sanitizer_flags(),"-fno-omit-frame-pointer","-Wall","-Wextra","-Werror",
        "-I"+str(project),"-I"+str(project/"include"),"-I"+str(project/"tests"),str(generated),
        str(project/"src/RTL8192EUDataPath.c"),str(project/"src/RTL8192CDataPath.c"),str(project/"src/RTL8814AUDataPath.c"),str(project/"src/RTL8192EUHT.c"),str(project/"src/USBPackets.c"),
        str(project/"tests/rtwn_ht_tx_tests.cpp"),"-o",str(args.out/"rtwn-ht-tx-tests")],check=True)
    subprocess.run([str(args.out/"rtwn-ht-tx-tests"),args.case],check=True)

if __name__=="__main__":main()
