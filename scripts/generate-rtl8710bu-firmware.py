#!/usr/bin/env python3
"""Embed both unchanged, pinned 8710BU vendor images. No download or hardware I/O."""
import argparse,hashlib,json,runpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--firmware-dir',type=Path,required=True)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    render=runpy.run_path(str(ROOT/'scripts/generate-dualband-firmware.py'))['render']
    records=json.loads((ROOT/'firmware/rtl8710bu-firmware-provenance.json').read_text())
    sections=[]
    for variant,record in records.items():
        data=(args.firmware_dir/record['file']).read_bytes()
        blob=hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()
        if len(data)!=record['bytes']or hashlib.sha256(data).hexdigest()!=record['sha256']or blob!=record['git_blob_sha1']:
            parser.error('Firmware content differs from primary byte pin: '+variant)
        if int.from_bytes(data[:2],'little')!=record['signature']:
            parser.error('Firmware vendor/signature pair differs: '+variant)
        license_bytes=(ROOT/record['license_file']).read_bytes()
        license_blob=hashlib.sha1(b'blob '+str(len(license_bytes)).encode()+b'\0'+license_bytes).hexdigest()
        if license_blob!=record['license_git_blob_sha1']:
            parser.error('Firmware license differs from primary byte pin: '+variant)
        notice=license_bytes.decode('utf-8')
        sections.append(render(record,data,notice))
    output=ROOT/'kext/RTL8710BUFirmware.inc';text='\n'.join(sections)
    if args.check:
        if not output.is_file()or output.read_text()!=text:parser.error('Generated 8710BU include differs')
    else:output.write_text(text)
    print(('Verified'if args.check else'Generated')+' both unmodified 8710BU vendor images; no hardware access')
if __name__=='__main__':main()
