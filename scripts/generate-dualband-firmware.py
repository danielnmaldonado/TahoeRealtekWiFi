#!/usr/bin/env python3
"""Embed unchanged, content-pinned firmware. Never downloads or loads a device."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

def render(record,data,notice):
    lines=['/* Opaque, unchanged Realtek firmware data.',
           ' * Source: '+record['source'],
           ' * SHA256: '+record['sha256'],
           ' * Firmware version: '+record['version'],
           ' * Redistribution notice (also firmware/LICENCE.rtlwifi_firmware.txt):']
    lines+=[' * '+line for line in notice.splitlines()]
    lines+=[' */','static const uint8_t '+record['symbol']+'[] = {']
    for offset in range(0,len(data),16):
        lines.append('    '+','.join('0x%02x'%byte for byte in data[offset:offset+16])+',')
    return '\n'.join(lines+['};',''])

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--firmware-dir',type=Path,required=True)
    p.add_argument('--du-dir',type=Path,help='optional separate directory for rtl8192dufw.bin')
    p.add_argument('--check',action='store_true')
    args=p.parse_args()
    records=json.loads((ROOT/'firmware/dualband-firmware-provenance.json').read_text())
    notice=(ROOT/'firmware/LICENCE.rtlwifi_firmware.txt').read_text()
    outputs=[]
    # Validate every input before modifying any generated file.
    for family,record in records.items():
        directory=args.du_dir if family=='RTL8192DU'and args.du_dir else args.firmware_dir
        data=(directory/record['file']).read_bytes()
        if len(data)!=record['bytes']or hashlib.sha256(data).hexdigest()!=record['sha256']:
            p.error('Firmware content does not match the pinned image: '+family)
        outputs.append((ROOT/record['include'],render(record,data,notice)))
    for path,text in outputs:
        if args.check:
            if not path.is_file()or path.read_text()!=text:p.error('Generated include differs: '+path.name)
        else:path.write_text(text)
    print(('Verified'if args.check else'Generated')+' four unmodified firmware includes; no hardware access')

if __name__=='__main__':main()
