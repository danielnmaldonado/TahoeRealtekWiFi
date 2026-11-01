#!/usr/bin/env python3
"""Regenerate the pinned RTL8814B HALMAC closure without network or USB access.
Pass the upstream HalMac88XX directory. --check verifies bytes without writing.
Every original file must match the immutable provenance before output changes.
"""
from pathlib import Path
import argparse, hashlib, json
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('reference',type=Path)
p.add_argument('--check',action='store_true')
a=p.parse_args();root=Path(__file__).resolve().parents[1]
proof=json.loads((root/'src/RTW8814BHAL-Upstream.json').read_text())
prefix=b'/* Generated boundary include; original Realtek source and notices follow. */\n#include "RTW8814BPlatform.h"\n'
outputs=[]
for f in proof['files']:
 rel=Path(f['path'])
 if rel.is_absolute() or '..' in rel.parts:raise SystemExit('Unsafe provenance path: '+str(rel))
 b=(a.reference/rel).read_bytes()
 if len(b)!=f['bytes'] or hashlib.sha256(b).hexdigest()!=f['sha256']:raise SystemExit('Pinned upstream mismatch: '+str(rel))
 for patch in proof.get('patches',[]):
  if patch['path']==f['path']:
   old=patch['old'].encode();new=patch['new'].encode()
   if b.count(old)!=1:raise SystemExit('Portability patch mismatch: '+str(rel))
   b=b.replace(old,new)
 out=prefix+b if f['kind']=='translation_unit' else b
 if hashlib.sha256(out).hexdigest()!=f['output_sha256']:raise SystemExit('Generation mismatch: '+str(rel))
 outputs.append((root/'src/rtl8814b/halmac'/rel,out))
for dest,b in outputs:
 if a.check:
  if dest.read_bytes()!=b:raise SystemExit('Generated source differs: '+str(dest))
 else:dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(b)
print(('Verified' if a.check else 'Generated'),len(outputs),'pinned RTL8814B HALMAC files')
