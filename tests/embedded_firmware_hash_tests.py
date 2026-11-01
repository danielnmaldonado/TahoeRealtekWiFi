#!/usr/bin/env python3
"""Verify the distributed opaque byte arrays against their source content pins."""
import hashlib
import json
import re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
records=json.loads((root/'firmware/dualband-firmware-provenance.json').read_text())
assert set(records)=={'RTL8192DU','RTL8821CU','RTL8822BU','RTL8822CU'}
records.update(json.loads((root/'firmware/rtl8710bu-firmware-provenance.json').read_text()))
for family,record in records.items():
    text=(root/record['include']).read_text()
    body=text.split(record['symbol']+'[] = {',1)[1].split('};',1)[0]
    data=bytes(int(x,16)for x in re.findall(r'0x([0-9a-f]{2})',body))
    assert len(data)==record['bytes'],family
    assert hashlib.sha256(data).hexdigest()==record['sha256'],family
    assert 'Redistribution and use in binary form' in text.replace(' * ',' '),family
    if 'signature'in record:
        assert int.from_bytes(data[:2],'little')==record['signature'],family
        notice=(root/record['license_file']).read_bytes()
        assert hashlib.sha1(b'blob '+str(len(notice)).encode()+b'\0'+notice).hexdigest()==record['license_git_blob_sha1'],family
    if 'git_blob_sha1'in record:
        assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==record['git_blob_sha1'],family
    if 'git_blob_sha'in record:
        assert hashlib.sha1(b'blob '+str(len(data)).encode()+b'\0'+data).hexdigest()==record['git_blob_sha'],family
print('Six embedded firmware images match their exact byte counts and SHA256 pins; notices retained')
