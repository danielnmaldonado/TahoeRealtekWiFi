#!/usr/bin/env python3
"""Embed unchanged, SHA256-pinned AU NIC firmware; no network/download/build."""
from pathlib import Path
import argparse,hashlib,json
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--8812',dest='fw12',type=Path,required=True);p.add_argument('--8821',dest='fw21',type=Path,required=True);args=p.parse_args()
expected={'8812':'abdcca4e8bf76ebfba23d433de310ffefebd0ff9d01990639d4cd9602b32b71a','8821':'4693ece655044d96f81bd3f3e8db14dc352b493219c4fd0a75788a6fbd9eac3c'}
license=(root/'firmware/LICENCE.rtlwifi_firmware.txt').read_text()
images={chip:path.read_bytes() for chip,path in [('8812',args.fw12),('8821',args.fw21)]}
for chip,data in images.items():
 if hashlib.sha256(data).hexdigest()!=expected[chip]:raise SystemExit('Pinned firmware SHA256 mismatch: '+chip)
records={}
for chip,data in images.items():
 url='https://gitlab.com/kernel-firmware/linux-firmware/-/raw/main/rtw88/rtw'+chip+'a_fw.bin'
 text='/* Unmodified RTL'+chip+'A NIC firmware data. Source: '+url+'\n * Content SHA256: '+expected[chip]+'\n * The origin ref is main; exact content is pinned by SHA256.\n * Redistribution notice follows; also firmware/LICENCE.rtlwifi_firmware.txt.\n'
 text+='\n'.join((' * '+line).rstrip() for line in license.splitlines())+'\n */\nstatic const uint8_t RTW'+chip+'AUFirmware[] = {\n'
 for offset in range(0,len(data),16):text+='    '+','.join('0x%02x'%b for b in data[offset:offset+16])+',\n'
 text+='};\n';(root/'kext'/('RTL'+chip+'AUFirmware.inc')).write_text(text)
 records[chip]={'source':url,'source_ref':'main','sha256':expected[chip],'bytes':len(data),'version':str(int.from_bytes(data[4:6],'little'))+'.'+str(data[6]),'license':'LICENCE.rtlwifi_firmware.txt','representation':'unmodified binary bytes in RTL'+chip+'AUFirmware.inc'}
(root/'firmware/rtl8812au-8821au-firmware-provenance.json').write_text(json.dumps(records,indent=2)+'\n')
print('Generated unmodified, SHA256-pinned AU firmware includes')
