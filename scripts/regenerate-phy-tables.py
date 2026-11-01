#!/usr/bin/env python3
"""Reproduce RTL8812VendorTables.inc from an exact pinned GPL-2.0 source file.

Usage (read-only): python3 scripts/regenerate-phy-tables.py path/to/tables.py
Usage (rewrite):   python3 scripts/regenerate-phy-tables.py path/to/tables.py --write

Does NOT download anything, execute upstream Python or touch Wi-Fi hardware.
Defaults to verification without modifying project files.
"""
from __future__ import annotations
import argparse
import ast
import hashlib
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / 'src/RTL8812VendorTables.inc'
UPSTREAM_GIT_BLOB = 'fd36aa0117b20b74a7ca3a80547dd23a419e53e3'
TABLES = {
    'MAC_REG': (107, 0xd4ae639d, 'profile_mac_reg', 'RTW_PHY_MAC8'),
    'PHY_REG': (215, 0x599f8c51, 'profile_phy_reg', 'RTW_PHY_BB32'),
    'AGC_TAB': (132, 0x507fd42a, 'profile_agc_tab', 'RTW_PHY_BB32'),
    'RADIO_A': (210, 0xa08738a2, 'profile_radio_a', 'RTW_PHY_RF_A'),
    'RADIO_B': (193, 0xa26163ee, 'profile_radio_b', 'RTW_PHY_RF_B'),
}
MAC_ALLOWLIST = {0x0010, 0x0011, 0x0025, 0x0072}
DELAY_MARKERS = {0xfe:50000, 0xffe:50000, 0xfd:5000,
                 0xfc:1000, 0xfb:50, 0xfa:5, 0xf9:1}
HEADER = [
    '/* SPDX-License-Identifier: GPL-2.0-only',
    ' * RTL8812AU B-cut/USB reference MAC/PHY/AGC/RF tables, generated from',
    ' * xen-proc/rtl8812au-macos rtl8812au/tables.py',
    ' * Git blob fd36aa0117b20b74a7ca3a80547dd23a419e53e3.',
    ' * Data derived from aircrack-ng/rtl8812au GPL-2.0-only vendor tables.',
    ' * Not validated on user hardware. Never use on unverified cut/RFE.',
    ' */',
]

def git_blob_sha(payload: bytes) -> str:
    return hashlib.sha1(b'blob ' + str(len(payload)).encode('ascii') +
                        b'\0' + payload).hexdigest()

def unpack_tables(source: bytes) -> dict[str, list[tuple[int,int]]]:
    tree = ast.parse(source, filename='tables.py')  # NEVER execute external Python
    result = {}
    for statement in tree.body:
        if not isinstance(statement, ast.Assign):
            continue
        if len(statement.targets) != 1 or not isinstance(statement.targets[0], ast.Name):
            continue
        name = statement.targets[0].id
        if name not in TABLES:
            continue
        pairs = ast.literal_eval(statement.value)
        if not isinstance(pairs, list):
            raise ValueError(f'{name} is not a list')
        unpacked=[]
        for item in pairs:
            if not isinstance(item, tuple) or len(item)!=2 or not all(
                    isinstance(v,int) and not isinstance(v,bool) for v in item):
                raise ValueError(f'{name} has an invalid record')
            addr,value=item
            if addr<0 or addr>65535 or value<0 or value>0xffffffff:
                raise ValueError(f'{name} contains out-of-range values')
            unpacked.append((addr,value))
        expected_count, expected_fnv, _, _ = TABLES[name]
        packed=b''.join(struct.pack('<HI',*pair) for pair in unpacked)
        fnv=2166136261
        for octet in packed:
            fnv=((fnv ^ octet)*16777619) & 0xffffffff
        if (len(unpacked),fnv)!=(expected_count,expected_fnv):
            raise ValueError(f'{name} deviates from pinned vendor reference')
        result[name]=unpacked
    if set(result)!=set(TABLES):
        raise ValueError(f'missing tables: {sorted(set(TABLES)-set(result))}')
    return result

def render(tables: dict[str,list[tuple[int,int]]]) -> bytes:
    lines=HEADER.copy()
    for name,(_, _,symbol,op) in TABLES.items():
        lines.append(f'static const rtw_phy_entry {symbol}[] = {{')
        for addr,value in tables[name]:
            if name != 'MAC_REG' and addr in DELAY_MARKERS:
                code='RTW_PHY_DELAY_US'; target_addr=0; target_value=DELAY_MARKERS[addr]
            else:
                code=op;target_addr=addr;target_value=value
                if name=='MAC_REG' and not ((0x100<=addr<0x800 or addr in MAC_ALLOWLIST)
                                                and value<=255):
                    raise ValueError(f'unsafe MAC register: 0x{addr:04x}')
                if name in ('PHY_REG','AGC_TAB') and not (
                    0x800<=addr<0xf00 and addr%4==0):
                    raise ValueError(f'unsafe BB register: 0x{addr:04x}')
                if name.startswith('RADIO_') and not (addr<=0xff and value<=0xfffff):
                    raise ValueError(f'unsafe RF register: 0x{addr:04x}')
            lines.append(f'    {{0x{target_addr:04X}U, {code}, 0x{target_value:08X}U}},')
        lines.append('};')
    return ('\n'.join(lines)+'\n').encode('utf-8')

def main() -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source',type=Path,help='upstream rtl8812au/tables.py')
    parser.add_argument('--write',action='store_true',help='explicitly overwrite generated .inc')
    args=parser.parse_args()
    try:
        source=args.source.read_bytes()
        sha=git_blob_sha(source)
        if sha!=UPSTREAM_GIT_BLOB:
            raise ValueError(f'upstream file SHA differs: {sha} != {UPSTREAM_GIT_BLOB}')
        generated=render(unpack_tables(source))
        if args.write:
            OUTPUT.write_bytes(generated)
            print(f'wrote pinned tables: {OUTPUT} ({len(generated)} bytes)')
        elif OUTPUT.read_bytes()!=generated:
            raise ValueError('generated tables mismatch: no files were modified')
        else:
            print('OK: source SHA, 857 table entries, FNV and generated file are identical')
    except (OSError,ValueError,SyntaxError) as exc:
        print(f'ERROR: {exc}',file=sys.stderr)
        return 1
    return 0
if __name__=='__main__':
    sys.exit(main())
