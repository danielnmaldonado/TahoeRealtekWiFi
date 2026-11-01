#!/usr/bin/env python3
"""Embed unmodified Realtek FU NIC firmware after exact SHA256 verification."""
from pathlib import Path
import argparse,hashlib,json
r=Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--firmware',type=Path,required=True);a=p.parse_args();data=a.firmware.read_bytes();digest=hashlib.sha256(data).hexdigest()
if digest!='3001f8b50176ede76d0cde20c7c7ff84ebe12331049e65e1a1a768b1d45b74de':raise SystemExit('Pinned FU firmware SHA256 mismatch')
notice=(r/'firmware/LICENCE.rtlwifi_firmware.txt').read_text();lines=['/* Realtek firmware redistribution notice:']+[' * '+s.rstrip() for s in notice.splitlines()]+[' */','static const uint8_t RTW8192FUFirmware[] = {']
lines+=['    '+','.join('0x%02x'%b for b in data[i:i+16])+',' for i in range(0,len(data),16)];lines+=['};','']
(r/'kext/RTL8192FUFirmware.inc').write_text('\n'.join(lines))
meta={'source':'https://gitlab.com/kernel-firmware/linux-firmware/-/raw/main/rtlwifi/rtl8192fufw.bin','source_ref':'main','sha256':digest,'bytes':len(data),'version':'7.3','license':'LICENCE.rtlwifi_firmware.txt','representation':'unmodified binary bytes in RTL8192FUFirmware.inc'};(r/'firmware/rtl8192fu-firmware-provenance.json').write_text(json.dumps(meta,indent=2)+'\n');print('Pinned FU NIC firmware 7.3 embedded byte-exact')
