#!/usr/bin/env python3
"""Execute actual probe/association IE and HT/VHT negotiation code offline."""
from pathlib import Path
import argparse
import re
import subprocess
from host_cpp import compiler, sanitizer_flags
from test_import_helpers import block

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path, required=True)
parser.add_argument('--source-dir', type=Path, default=root/'vendor/itl80211/openbsd/net80211')
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
definitions = {}
for name in ('ieee80211.h', 'ieee80211_var.h', 'ieee80211_node.h', 'ieee80211_crypto.h', 'ieee80211_ioctl.h', 'ieee80211_priv.h'):
    text = (args.source_dir/name).read_text()
    for match in re.finditer(r'(?m)^\s*#define[ \t]+(IEEE80211_\w+)(?:[^\n]*\\\n)*[^\n]*', text):
        definitions.setdefault(match[1], match[0].strip())
constants = '\n'.join(definitions.values())+'\n'
main = (args.source_dir/'ieee80211.h').read_text()
constants += block(main, 'enum ieee80211_chan_width {')+';\n'
constants += block(main, 'enum {\n\tIEEE80211_ELEMID_SSID')+';\n'
constants += block((args.source_dir/'ieee80211_crypto.h').read_text(), 'enum ieee80211_cipher {')+';\n'
(args.out/'channel_constants.hpp').write_text(constants)
output = (args.source_dir/'ieee80211_output.c').read_text()
proto = (args.source_dir/'ieee80211_proto.c').read_text()
generated = '#include "net80211_channel_capabilities_host.hpp"\n'
for marker in ('static uint16_t ieee80211_htcaps_for_channel(',
               'static int ieee80211_vht_for_channel(',
               'static uint8_t *\nieee80211_add_htcaps_channel(',
               'static uint8_t *\nieee80211_add_vhtcaps_channel('):
    if marker in output:
        generated += block(output, marker)+'\n'
for name, result in (('ieee80211_add_htcaps', 'uint8_t *'),
                     ('ieee80211_add_vhtcaps', 'uint8_t *'),
                     ('ieee80211_get_probe_req', 'mbuf_t'),
                     ('ieee80211_get_assoc_req', 'mbuf_t')):
    generated += result+'\n'+block(output, '\n'+name+'(')+'\n'
for name in ('ieee80211_ht_negotiate_chw', 'ieee80211_vht_negotiate'):
    generated += 'void\n'+block(proto, '\n'+name+'(')+'\n'
(args.out/'actual_channel_capabilities.cpp').write_text(generated)
subprocess.run([*compiler(), '-std=c++14', '-O1', '-g', *sanitizer_flags(),
               '-fno-omit-frame-pointer', '-Wall', '-Wextra', '-Werror',
               '-Wno-macro-redefined', '-Wno-parentheses', '-Wno-implicit-fallthrough',
               '-I'+str(args.out), '-I'+str(root/'tests'),
               str(args.out/'actual_channel_capabilities.cpp'),
               str(root/'tests/net80211_channel_capabilities_tests.cpp'),
               '-o', str(args.out/'tests')], check=True)
subprocess.run([str(args.out/'tests')], check=True)
