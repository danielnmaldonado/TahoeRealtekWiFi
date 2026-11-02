#!/usr/bin/env python3
"""Actual net80211 scan loop plus the controller's foreground completion hook.

IOKit timers and RF transitions are host boundaries. The real begin/next/end
scan, BSS choice, completion handler, status and diagnostic transitions run.
The optional source root allows the same behavioral regression against a
release made before the callback existed (its real default is no callback).
"""
from pathlib import Path
import argparse
import importlib.util
import re
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags


def main():
    project = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--source-root", type=Path, default=project)
    args = parser.parse_args()
    source_root = args.source_root.resolve()
    spec = importlib.util.spec_from_file_location("extraction", project / "scripts/test-rtwn-wpa-recovery.py")
    extraction = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extraction)
    node = (source_root / "vendor/itl80211/openbsd/net80211/ieee80211_node.c").read_text()
    controller = (source_root / "kext/RTWUSBController.cpp").read_text()
    closure = '#include "controller_scan_completion_host.hpp"\n'
    for name in ("ieee80211_node_checkrssi", "ieee80211_reset_scan", "ieee80211_begin_scan",
                 "ieee80211_next_scan", "ieee80211_node_choose_bss", "ieee80211_end_scan"):
        closure += extraction.function(node, r"^(?:int|void|struct ieee80211_node \*)\s*\n" + name + r"\(")
    for name in ("observeScanTransition", "status"):
        closure += extraction.function(controller, r"^void RTWUSBController::" + name + r"\(")
    if "int RTWUSBController::scanComplete(" in controller:
        closure += extraction.function(controller, r"^int RTWUSBController::scanComplete\(")
    # Execute the real callback registration statement, when present. Older
    # production sources leave the optional net80211 callback at its null default.
    registration = re.search(r"core_\.sc_ic\.ic_scan_done\s*=[^;]+;", controller)
    closure += "void RTWUSBController::registerScanCallback() {" + (registration.group() if registration else "") + "}\n"
    common = (project / "tests/net80211_scan_tests.cpp").read_text()
    common = common[:common.index("static void complete_dual_band(")]
    common = common.replace("bool observeTwo=true,observeFive=true,pending=false;",
                            "bool observeTwo=true,observeFive=true,pending=false,rejectCandidates=false;")
    common = common.replace("return isclr(ic->ic_chan_active,", "return fixture(ic).rejectCandidates || isclr(ic->ic_chan_active,")
    common = common.replace("ic->ic_state=state;const unsigned channel=", "const auto previous=ic->ic_state;ic->ic_state=state;const unsigned channel=")
    common = common.replace("f.visits.push_back(channel);f.pending=true;", """f.visits.push_back(channel);f.pending=true;
    if(ic->ic_softc){auto*core=static_cast<rtwn_softc*>(ic->ic_softc);
        RTWUSBController::owner(core->sc_ops.cookie)->observeScanTransition(previous,state,channel,0);}""")
    common = common.replace('#include "net80211_scan_host.hpp"', '#include "controller_scan_completion_host.hpp"')
    with tempfile.TemporaryDirectory(prefix="rtw-controller-scan-") as temporary:
        out = args.out or Path(temporary)
        out.mkdir(parents=True, exist_ok=True)
        host = (project / "tests/net80211_scan_host.hpp").read_text()
        host = host.replace("unsigned ni_fails=", "uint8_t ni_esslen=0,ni_essid[32]{};\n    unsigned ni_fails=")
        host = host.replace("void (*ic_event_handler)", "void*ic_softc=nullptr;\n    void (*ic_event_handler)")
        (out / "net80211_scan_host.hpp").write_text(host)
        actual = out / "actual_controller_scan.cpp"
        actual.write_text(closure)
        tests = out / "controller_scan_tests.cpp"
        tests.write_text(common + (project / "tests/controller_scan_completion_tests.inc").read_text())
        binary = out / "controller-scan-completion-tests"
        subprocess.run([*compiler(), "-std=c++17", "-O1", "-g", "-DIEEE80211_STA_ONLY",
                        "-Wall", "-Wextra", "-Werror", "-Wno-unused-but-set-variable", *sanitizer_flags(),
                        "-fno-omit-frame-pointer", "-I" + str(out), "-I" + str(project / "tests"),
                        "-I" + str(source_root / "include"), "-I" + str(source_root / "kext"),
                        "-I" + str(project / "vendor/itl80211/openbsd"), str(actual), str(tests),
                        "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
