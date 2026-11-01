#!/usr/bin/env python3
"""Reproduce the unchanged, licensed RTL8723BU firmware include from pinned bytes."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parents[1];record=json.loads((r/'firmware/rtl8723bu-firmware-provenance.json').read_text());raw=(r/'firmware'/record['file']).read_bytes()
if len(raw)!=record['bytes'] or hashlib.sha256(raw).hexdigest()!=record['sha256']:raise SystemExit('Pinned RTL8723BU firmware hash mismatch')
license=(r/'firmware/LICENCE.rtlwifi_firmware.txt').read_text()
out='/* Unmodified RTL8723BU NIC firmware35.0.\n * Source: '+record['source']+'\n * SHA256: '+record['sha256']+'\n'+''.join(' * '+line+'\n' for line in license.splitlines())+' */\nstatic const uint8_t RTW8723BUFirmware[]={\n'
for at in range(0,len(raw),16):out+=' '+','.join('0x%02x'%b for b in raw[at:at+16])+',\n'
(r/'kext/RTL8723BUFirmware.inc').write_text(out+'};\n')
