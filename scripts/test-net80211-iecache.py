#!/usr/bin/env python3
"""Run actual cache ownership and span statements on local byte buffers only.

No wireless frames are constructed, parsed, received or transmitted. Allocator
failure is injected at the host allocation boundary; no kernel/device access.
"""
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
parser.add_argument("--out", type=Path, default=project / "build/net80211-iecache")
parser.add_argument("--input-source", type=Path,
                    default=project / "vendor/itl80211/openbsd/net80211/ieee80211_input.c")
parser.add_argument("--case", default="all")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
source = args.input_source.read_text()
receive = helpers.function(source, "ieee80211_recv_probe_resp")
start = receive.index("        struct ieee80211_rsnparams rsn, wpa;")
start = receive.index("\n", start) + 1
end = receive.index("        ni->ni_rsnprotos =", start)
cache_statements = receive[start:end]
closure = '#include "net80211_iecache_host.hpp"\n'
closure += '#define malloc iecache_allocate\n#define free iecache_release\n'
closure += helpers.function(source, "ieee80211_save_ie_tlv")
closure += '#undef malloc\n#undef free\n'
closure += '''
void actual_iecache_update(const uint8_t *storage, size_t valid_length, iecache_node *ni)
{
    iecache_buffer buffer{storage, valid_length};
    iecache_buffer *m = &buffer;
    const iecache_prefix *wh = (const iecache_prefix *)storage;
    const uint8_t *ie_start = storage + sizeof(*wh) + 12;
    const uint8_t *efrm = storage + valid_length;
'''
closure += cache_statements + '\n}\n'
(args.out / "actual_iecache.cpp").write_text(closure)
command = [*compiler(), "-std=c++14", "-g", "-O1", *sanitizer_flags(),
           "-fno-omit-frame-pointer", "-Werror=return-type",
           "-I" + str(project / "tests"), str(args.out / "actual_iecache.cpp"),
           str(project / "tests/net80211_iecache_tests.cpp"),
           "-o", str(args.out / "net80211-iecache-tests")]
subprocess.run(command, check=True)
subprocess.run([str(args.out / "net80211-iecache-tests"), args.case], check=True)
