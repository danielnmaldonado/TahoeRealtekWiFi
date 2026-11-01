#!/usr/bin/env python3
"""Offline actual RX BA/BAR/ADDBA algorithms followed by actual software CCMP."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import importlib.util
import subprocess
project = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("pae_helpers", project / "scripts/test-net80211-pae.py")
helpers = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helpers)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--out", type=Path, default=project / "build/net80211-ba")
parser.add_argument("--input-source", type=Path, default=project / "vendor/itl80211/openbsd/net80211/ieee80211_input.c")
parser.add_argument("--case", default="all")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
vendor = project / "vendor/itl80211/openbsd"
host = project / "tests/net80211_ht_host"
closure = '#include "net80211_ba_host.hpp"\n'
source = args.input_source.read_text()
for name in ["ieee80211_input_ba", "ieee80211_input_ba_seq", "ieee80211_input_ba_flush", "ieee80211_input_ba_gap_skip", "ieee80211_input_ba_gap_timeout", "ieee80211_ba_move_window", "ieee80211_recv_bar", "ieee80211_bar_tid", "ieee80211_recv_addba_req", "ieee80211_addba_req_accept", "ieee80211_addba_req_refuse", "ieee80211_recv_addba_resp", "ieee80211_addba_resp_accept", "ieee80211_addba_resp_refuse"]:
    closure += '\n// Verbatim production function\n' + helpers.function(source, name)
node = (vendor / "net80211/ieee80211_node.c").read_text()
for name in ["ieee80211_ba_del", "ieee80211_ba_free"]:
    closure += '\n// Verbatim production function\n' + helpers.function(node, name)
(args.out / "upstream_ba.cpp").write_text(closure)
command = [*compiler(), "-std=c++14", "-x", "c++", "-g", "-O1", *sanitizer_flags(), "-fno-omit-frame-pointer", "-DIEEE80211_STA_ONLY", "-Wno-macro-redefined", "-Wno-format", "-Wno-unused-value", "-I" + str(host / "include"), "-I" + str(project / "tests/host_compat"), "-I" + str(project / "tests"), "-I" + str(vendor), "-I" + str(vendor / "crypto"), "-include", str(host / "platform.hpp"), str(vendor / "crypto/aes.c"), str(vendor / "net80211/_string.c"), str(vendor / "net80211/ieee80211_crypto_ccmp.c"), str(args.out / "upstream_ba.cpp"), str(project / "tests/net80211_ba_tests.cpp"), "-o", str(args.out / "net80211-ba-tests")]
subprocess.run(command, check=True)
subprocess.run([str(args.out / "net80211-ba-tests"), args.case], check=True)
