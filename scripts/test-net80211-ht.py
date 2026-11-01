#!/usr/bin/env python3
"""Offline actual net80211 BA ownership, QoS sequence and software CCMP checks."""
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
parser.add_argument("--out", type=Path, default=project / "build/net80211-ht")
parser.add_argument("--node-source", type=Path, default=project / "vendor/itl80211/openbsd/net80211/ieee80211_node.c")
parser.add_argument("--output-source", type=Path, default=project / "vendor/itl80211/openbsd/net80211/ieee80211_output.c")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
vendor = project / "vendor/itl80211/openbsd"
host = project / "tests/net80211_ht_host"
closure = '#include <net80211/ieee80211_var.h>\n#include <net80211/ieee80211_priv.h>\n'
node = args.node_source.read_text().replace("void ieee80211_ba_free(", "void\nieee80211_ba_free(")
for name in ["ieee80211_node_clear_rsn", "ieee80211_node_trigger_addba_req", "ieee80211_node_addba_request", "ieee80211_node_addba_request_to", "ieee80211_ba_del", "ieee80211_ba_free", "ieee80211_node_cleanup", "ieee80211_node_free", "ieee80211_node_copy", "ieee80211_node_set_timeouts", "ieee80211_free_node", "ieee80211_free_allnodes", "ieee80211_node_leave_ht", "ieee80211_node_detach"]:
    closure += '\n// Verbatim production function\n' + helpers.function(node, name)
output = args.output_source.read_text()
for name in ["ieee80211_classify", "ieee80211_can_use_ampdu", "ieee80211_encap"]:
    closure += '\n// Verbatim production function\n' + helpers.function(output, name)
(args.out / "upstream_ht.cpp").write_text(closure)
command = [*compiler(), "-std=c++14", "-x", "c++", "-g", "-O1", *sanitizer_flags(), "-fno-omit-frame-pointer", "-DIEEE80211_STA_ONLY", "-Wno-macro-redefined", "-Wno-format", "-Wno-unused-value", "-Wno-incompatible-pointer-types-discards-qualifiers", "-I" + str(host / "include"), "-I" + str(project / "tests/host_compat"), "-I" + str(vendor), "-I" + str(vendor / "crypto"), "-include", str(host / "platform.hpp"), str(vendor / "crypto/aes.c"), str(vendor / "net80211/_string.c"), str(vendor / "net80211/ieee80211_crypto_ccmp.c"), str(args.out / "upstream_ht.cpp"), str(project / "tests/net80211_ht_tests.cpp"), "-o", str(args.out / "net80211-ht-tests")]
subprocess.run(command, check=True)
subprocess.run([str(args.out / "net80211-ht-tests")], check=True)
