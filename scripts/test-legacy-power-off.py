#!/usr/bin/env python3
"""Run the complete production CU/88E/92E HAL over a register USB boundary."""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile
from host_cpp import compiler, sanitizer_flags

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out', type=Path)
parser.add_argument('--hal-source', type=Path)
args = parser.parse_args()
header = (root / 'vendor/rtwn/rtwnvar.h').read_text()
ops_start = header.index('struct rtwn_ops {')
ops = header[ops_start:header.index('\n};', ops_start) + 3]
defines = '\n'.join(re.findall(r'^#define RTWN_(?:CHIP|FLAG)_[^\n]+', header, re.M))
hal = (args.hal_source or root / 'kext/RTW8192EUHAL.cpp').read_text()
hal = hal.replace('#include "RTW8192EUHAL.hpp"', '')
if 'rtwLegacyPowerOffChecked(' not in hal:
    # RED baseline: no checked shutdown exists in the old native HAL.
    hal += '\nint rtwLegacyPowerOffChecked(rtwn_softc *) { return ENOTSUP; }\n'
host = '''#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <cerrno>
using mbuf_t = void *;
using u_char = unsigned char;
struct ieee80211_node;
#define __packed __attribute__((packed))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#include "r92creg.h"
''' + ops + '\n' + defines + '''
struct device { char dv_xname[32] = "host"; };
struct rtwn_softc { rtwn_ops sc_ops{}; int core_error = 0;
 device *sc_pdev = nullptr; uint32_t chip = 0, sc_flags = 0;
 uint8_t board_type = 0, crystal_cap = 0; };
static int rtwn_io_error(rtwn_softc *s) {
 return s->core_error ? s->core_error : s->sc_ops.io_error(s->sc_ops.cookie);
}
static void rtwn_delay(rtwn_softc *s,uint32_t us) { s->sc_ops.delay_us(s->sc_ops.cookie,us); }
'''
with tempfile.TemporaryDirectory(prefix='legacy-power-') as temp:
    output = args.out or Path(temp)
    output.mkdir(parents=True, exist_ok=True)
    source = output / 'actual_legacy_power.cpp'
    source.write_text(host + '\n' + hal + '\n' + (root / 'tests/legacy_power_off_tests.cpp').read_text())
    binary = output / 'legacy-power-off-tests'
    subprocess.run([*compiler(), '-std=c++17', '-O1', '-g', '-Wall', '-Wextra',
                    '-Werror', '-Wno-sign-compare', *sanitizer_flags(),
                    '-fno-omit-frame-pointer', '-I' + str(root / 'vendor/rtwn'),
                    str(source), '-o', str(binary)], check=True)
    subprocess.run([str(binary.resolve())], check=True)
