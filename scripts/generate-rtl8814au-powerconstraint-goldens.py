#!/usr/bin/env python3
"""Run the unchanged pinned Linux SAR dBm-to-TXGI conversion, without hardware."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from host_cpp import compiler

ROOT = Path(__file__).resolve().parents[1]


def function(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth = 0
    for token in re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|[{}]', source[opening:]):
        if token.group() == '{': depth += 1
        elif token.group() == '}':
            depth -= 1
            if not depth: return source[start:opening + token.end()]
    raise ValueError(marker)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'tests/RTL8814AUPowerConstraintGolden.inc')
    args = parser.parse_args()
    for name in ('RTL8814AUPHY-Upstream.json', 'RTL8814AUPowerConstraint-Upstream.json'):
        manifest = json.loads((ROOT / 'src' / name).read_text())
        if manifest['commit'] != '0e135910deec43ffe97eca0b6af1f8f847285b59':
            raise ValueError('Unexpected pinned Linux commit')
        for item in manifest['files']:
            if hashlib.sha256((args.source / item['path']).read_bytes()).hexdigest() != item['sha256']:
                raise ValueError('Pinned-source mismatch: ' + item['path'])
    sar = (args.source / 'sar.c').read_text()
    chip = (args.source / 'rtw8814a.c').read_text()
    txgi = int(re.search(r'\.txgi_factor\s*=\s*(\d+)', chip)[1])
    maximum = int(re.search(r'\.max_power_index\s*=\s*(0x[0-9a-fA-F]+|\d+)', chip)[1], 0)
    source = '''#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8;typedef int8_t s8;typedef int32_t s32;
#define clamp_t(t,x,l,h) ((t)(x)<(t)(l)?(t)(l):(t)(x)>(t)(h)?(t)(h):(t)(x))
enum {RTW_SAR_BAND_0};
struct rtw_sar_arg {u8 sar_band,path,rs;};
struct rtw_hal {s8 tx_pwr_by_rate_base_2g[4][10],tx_pwr_by_rate_base_5g[4][10];};
struct rtw_chip_info {u8 txgi_factor,max_power_index;};
struct rtw_dev {struct rtw_hal hal;const struct rtw_chip_info *chip;};
'''
    source += function(sar, 'static s8 rtw_sar_to_phy(')
    source += f'''\nint main(void){{
 struct rtw_chip_info chip={{{txgi},{maximum}}};struct rtw_dev dev={{.chip=&chip}};
 struct rtw_sar_arg arg={{1,0,0}};
 for(unsigned db=0;db<=({maximum}>>{txgi});db++)printf("%u\\n",(unsigned)rtw_sar_to_phy(&dev,0,(s32)db,&arg));
 for(int db=-32;db<=31;db++)for(unsigned base=0;base<64;base++){{
  dev.hal.tx_pwr_by_rate_base_5g[0][0]=(s8)base;
  printf("%d\\n",(int)rtw_sar_to_phy(&dev,0,(s32)db,&arg));
 }}
 return 0;
}}
'''
    with tempfile.TemporaryDirectory(prefix='8814-constraint-reference-') as directory:
        src = Path(directory) / 'reference.c'; binary = Path(directory) / 'reference'
        src.write_text(source)
        subprocess.run([*compiler('c'), '-std=c11', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(binary)], check=True)
        results = list(map(int, subprocess.check_output([str(binary)], text=True).splitlines()))
    values, sar_offsets = results[:32], results[32:]
    assert len(sar_offsets) == 64 * 64
    tables = (args.source / 'rtw8814a_table.c').read_text()
    channels = (36,40,44,48,52,56,60,64,100,104,108,112,116,120,
                124,128,132,136,140,149,153,157,161,165)
    rows = []
    for rfe in range(2):
        pg = [[0] * 28 for _ in range(4)]
        name = 'rtw8814a_bb_pg' + ('_type0' if rfe == 0 else '')
        text = function(tables, 'static const struct rtw_phy_pg_cfg_pair ' + name + '[]')
        for row in re.findall(r'\{([^{}]+)\}', text):
            band, path, txnum, address, mask, data = [int(x.strip(), 0) for x in row.split(',') if x.strip()]
            first = {0x24:4,0x28:8,0x2c:12,0x30:16,0x34:20,0x38:24}.get(address & 255)
            if band != 1 or first is None: continue
            for byte in range(4):
                bcd = (data >> (byte * 8)) & 255
                pg[path][first + byte] = (bcd >> 4) * 10 + (bcd & 15)
        text = function(tables, 'static const struct rtw_txpwr_lmt_cfg_pair rtw8814a_txpwr_lmt_type' + str(rfe) + '[]')
        limits = [[int(x.strip(), 0) for x in row.split(',') if x.strip()]
                  for row in re.findall(r'\{([^{}]+)\}', text)]
        limits_by_key = {}
        for row in limits:
            if row[1] == 1 and row[2] == 0 and row[3] in (1,2,3):
                limits_by_key.setdefault((row[4],row[3]), []).append(row)
        for domain in range(4):
            for channel in channels:
                for ceiling in (-25,-10,-3,10,17,20):
                    for constraint in (0,3,6):
                        power = []; supported = True; effective = ceiling - constraint
                        for path in range(4):
                            for rate in range(4,28):
                                section = 1 if rate < 12 else 2 if rate < 20 else 3
                                base_rate = {1:11,2:19,3:27}[section]
                                candidates = limits_by_key[channel,section]
                                selected = min([row[5] for row in candidates if row[0] == domain] + [63])
                                if selected == 63: selected = min(row[5] for row in candidates)
                                sar_offset = sar_offsets[(effective + 32) * 64 + pg[path][base_rate]]
                                offset = min(pg[path][rate] - pg[path][base_rate],
                                             selected - pg[0][base_rate], sar_offset)
                                # Factory ROM base63 and all signed ROM deltas0.
                                logical = 63 + offset
                                if logical < 0: supported = False
                                power.append(min(63, max(0,logical) + 2))
                        rows.append(' {' + ','.join(map(str,[rfe,domain,channel,ceiling,constraint,int(supported)])) + ',{' +
                                    (','.join(map(str,power)) if supported else '') + '}},')
    args.output.write_text('''/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright(c) 2018-2021, 2025 Realtek Corporation.
 * BSD-3-Clause; see src/RTL8814AUPHY-LICENSE.txt.
 * Generated by unchanged pinned Linux sar.c:rtw_sar_to_phy, PG base=0.
 * RTL8814A chip txgi_factor/max_power_index are read from pinned rtw8814a.c.
 * torvalds/linux 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Source hashes: RTL8814AUPHY-Upstream.json and RTL8814AUPowerConstraint-Upstream.json. */
static const unsigned char power_constraint_txgi[]={''' + ','.join(map(str, values)) + '''};
/* Full primary SAR offset, by-rate PG and domain limits. ROM base63, deltas0. */
struct absolute_power_golden {unsigned rfe,domain,channel;int ceiling;unsigned constraint,supported;unsigned char power[96];};
static const absolute_power_golden absolute_power_goldens[]={
''' + '\n'.join(rows) + '\n};\n')
    print(f'{len(values)} whole-dBm to TXGI conversions from unchanged pinned Linux SAR function: {args.output}')
    print(f'{len(rows)} absolute SAR/PG/regulatory/constraint rows, including negative ceilings')


if __name__ == '__main__':
    main()
