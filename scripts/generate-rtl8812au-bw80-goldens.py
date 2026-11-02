#!/usr/bin/env python3
"""Generate AU VHT80 vectors from unchanged pinned Linux structs and PHY code."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import tempfile
from host_cpp import compiler
from test_import_helpers import block


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=root/'tests/RTL8812AUBW80Golden.inc')
    args = parser.parse_args()
    manifest = json.loads((root/'src/RTL8812AUPHY-Upstream.json').read_text())
    files = {}
    for name in ('main.h', 'phy.c', 'rtw8812a.c', 'rtw8821a.c',
                 'rtw8812a_table.c', 'rtw8821a_table.c'):
        data = (args.reference/name).read_bytes()
        if hashlib.sha256(data).hexdigest() != manifest['sources'][name]:
            raise ValueError('Pinned source mismatch: '+name)
        files[name] = data.decode()
    factors = [int(re.search(r'\.txgi_factor\s*=\s*(\d+)', files[name])[1])
               for name in ('rtw8812a.c', 'rtw8821a.c')]
    assert factors == [1, 1]
    source = '''#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8; typedef int8_t s8;
#define __packed __attribute__((packed))
#define __LITTLE_ENDIAN 1
#define WARN_ON(x) ((void)(x))
#define fallthrough __attribute__((fallthrough))
struct rtw_chip_info {u8 txgi_factor;};
struct rtw_dev {const struct rtw_chip_info *chip;};
'''
    for marker in ('enum rtw_bandwidth {', 'enum rtw_trx_desc_rate {',
                   'struct rtw_5g_ht_1s_pwr_idx_diff {',
                   'struct rtw_5g_ht_ns_pwr_idx_diff {',
                   'struct rtw_5g_ofdm_ns_pwr_idx_diff {',
                   'struct rtw_5g_vht_ns_pwr_idx_diff {',
                   'struct rtw_5g_txpwr_idx {'):
        source += block(files['main.h'], marker)
        source += ' __packed;\n' if marker.startswith('struct ') else ';\n'
    source += block(files['phy.c'], 'static u8 rtw_phy_get_5g_tx_power_index(')
    # Deliberately unequal signed BW160/BW80 nibbles. Offsets are obtained
    # from the actual primary struct, never copied from the port under test.
    source += '''
int main(void) {
 struct rtw_chip_info chip={1};struct rtw_dev dev={&chip};
 const unsigned char pairs[][2]={{0x27,0x15},{0xe3,0xf4},{0x3e,0x4f},{0x8f,0x97}};
 printf("%zu %zu %zu\\n",sizeof(struct rtw_5g_txpwr_idx),
        offsetof(struct rtw_5g_txpwr_idx,vht_1s_diff)+18,
        offsetof(struct rtw_5g_txpwr_idx,vht_2s_diff)+18);
 for(unsigned v=0;v<4;v++)for(unsigned g=0;g<2;g++)for(unsigned rate=44;rate<64;rate++) {
  unsigned char raw[42];memset(raw,24,sizeof(raw));
  raw[32]=0x30;raw[33]=0xe2;
  struct rtw_5g_txpwr_idx p;memcpy(&p,raw+18,sizeof(p));
  memcpy(&p.vht_1s_diff,&pairs[v][0],1);memcpy(&p.vht_2s_diff,&pairs[v][1],1);
  printf("%u\\n",rtw_phy_get_5g_tx_power_index(&dev,&p,RTW_CHANNEL_WIDTH_80,rate,g?10:0));
 }
}
'''
    with tempfile.TemporaryDirectory(prefix='au-bw80-primary-') as directory:
        src = Path(directory)/'reference.c'
        executable = Path(directory)/'reference'
        src.write_text(source)
        subprocess.run([*compiler('c'), '-std=c11', '-Wall', '-Wextra', '-Werror',
                        str(src), '-o', str(executable)], check=True)
        output = subprocess.check_output([str(executable)], text=True).splitlines()
    assert list(map(int, output[0].split())) == [24, 38, 39]
    bases = list(map(int, output[1:]))
    assert len(bases) == 160

    def records(name, filename):
        text = block(files[filename], 'static const struct '+
                     ('rtw_phy_pg_cfg_pair ' if '_bb_pg' in name else 'rtw_txpwr_lmt_cfg_pair ')+name+'[]')
        return [[int(x.strip(), 0) for x in row.split(',') if x.strip()]
                for row in re.findall(r'\{([^{}]+)\}', text)]

    rows = []
    pairs = ((0x27, 0x15), (0xe3, 0xf4), (0x3e, 0x4f), (0x8f, 0x97))
    rates = (44, 51, 53, 54, 61, 63)
    for one, rfe in ((False, 0), (False, 3), (True, 0)):
        chip = 'rtw8821a' if one else 'rtw8812a'
        pg = records(chip+'_bb_pg'+('_rfe3' if rfe else ''), chip+'_table.c')
        limits = records(chip+'_txpwr_lmt', chip+'_table.c')
        byrate = {}
        for band, path, txnum, address, mask, data in pg:
            if band != 1:
                continue
            for byte in range(4):
                if (mask >> (byte*8)) & 255 != 255:
                    continue
                rate = address-(0xe20 if path else 0xc20)+byte
                if rate >= 28:
                    rate += 16
                value = (data >> (byte*8)) & 255
                byrate[path, rate] = (value >> 4)*10+(value & 15)
        for v, pair in enumerate(pairs):
            for g, (primary, center, subcenter) in enumerate(((36, 42, 38), (149, 155, 151))):
                indices = []
                for path in range(2):
                    for rate in rates:
                        if one and (path or rate >= 54):
                            indices.append(0)
                            continue
                        section = 4 if rate < 54 else 5
                        reference = 51 if rate < 54 else 61
                        ceiling = min(value for regd, band, width, rs, ch, value in limits
                                      if band == 1 and rs == section and
                                      (width, ch) in ((0, primary), (1, subcenter), (2, center)))
                        offset = min(byrate[path, rate]-byrate[path, reference],
                                     ceiling-byrate[0, reference])
                        base = bases[(v*2+g)*20+rate-44]
                        indices.append(max(0, min(63, base+offset)))
                rows.append(' {%s,%u,%u,%u,0x%02x,0x%02x,{%s}},' %
                            ('true' if one else 'false', rfe, primary, center, *pair,
                             ','.join(map(str, indices))))
    args.output.write_text('''/* SPDX-License-Identifier: BSD-3-Clause */
/* Generated by unchanged pinned Linux main.h layouts and
 * phy.c:rtw_phy_get_5g_tx_power_index, then independent PG/limit decoding.
 * torvalds/linux '''+manifest['commit']+'''; hashes: RTL8812AUPHY-Upstream.json.
 * Factory base24; HT20 diff+3/2SS-2; VHT80 bytes38/39; byte37 remains0x18.
 * Each row covers VHT1SS rates44/51/53 and VHT2SS54/61/63, both RF paths. */
struct AUBW80Golden {bool one;unsigned rfe,primary,center;uint8_t diff1,diff2,index[12];};
static const AUBW80Golden au_bw80_goldens[]={
'''+ '\n'.join(rows)+'\n};\n')
    print(f'{len(rows)} AU VHT80 power vectors; primary layout sizeof24, VHT1SS/2SS ROM offsets38/39')


if __name__ == '__main__':
    main()
