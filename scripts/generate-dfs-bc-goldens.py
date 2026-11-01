#!/usr/bin/env python3
"""Independent DFS20 MCS7 vectors from pinned primary Linux PG/limit data."""
from pathlib import Path
import argparse,hashlib,json,re
p=argparse.ArgumentParser();p.add_argument('--reference',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
r=Path(__file__).resolve().parents[1];meta=json.loads((r/'src/RTW88USBDataPath-Upstream.json').read_text());hashes={f['path']:f['sha256'] for f in meta['files']};files={}
for chip in ['8821c','8822b','8822c']:
 name='rtw'+chip+'_table.c';b=(a.reference/name).read_bytes();assert hashlib.sha256(b).hexdigest()==hashes[name];files[chip]=b.decode()
def records(name,chip):
 body=re.search(r'\b'+name+r'\[\]\s*=\s*\{(.*?)\n\};',files[chip],re.S)[1]
 return [[int(x.strip(),0) for x in row.split(',') if x.strip()] for row in re.findall(r'\{([^{}]*)\}',body)]
channels=[52,56,60,64,100,104,108,112,116,120,124,128,132,136,140];groups=[2,2,3,3,4,4,5,5,6,6,7,7,8,8,9]
out=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Independent primary PG/limit MCS7 vectors; Linux '+meta['commit']+'. ROMbase24+group,differentials0. */','struct BCDfsGolden {rtw_wifi_family family;uint8_t rfe,channel,index,limited,ceiling;};','static const BCDfsGolden bc_dfs_goldens[] = {']
profiles=[('8821c',r,0,0) for r in [0,2,4,6]]+[('8822b',r,r,0 if r==3 else r) for r in [2,3,5]]+[('8822c',r,0,5 if r==5 else 0) for r in range(7)]
for chip,rfe,pg,limit in profiles:
 rows=records('rtw'+chip+'_bb_pg_type'+str(pg),chip);limits=records('rtw'+chip+'_txpwr_lmt_type'+str(limit),chip)
 word=next(v for band,path,txnum,addr,mask,v in rows if band==1 and path==0 and addr==0xc30);byte=(word>>24)&255;ref=byte if chip=='8822c' else (byte>>4)*10+(byte&15)
 for ch,g in zip(channels,groups):
  lim=[v for regd,band,bw,rs,c,v in limits if band==1 and bw==0 and rs==2 and c==ch]
  if not lim:lim=[v for regd,band,bw,rs,c,v in limits if band==1 and bw==0 and rs==4 and c==ch]
  assert lim,(chip,rfe,ch);idx=max(0,min(127 if chip=='8822c' else 63,24+g+min(0,min(lim)-ref)))
  absolute=69 if chip=='8822c' else 34;ceil=max(0,min(127 if chip=='8822c' else 63,24+g+absolute-ref));limited=min(idx,ceil)
  family={'8821c':'RTW_WIFI_RTL8821CU','8822b':'RTW_WIFI_RTL8822BU','8822c':'RTW_WIFI_RTL8822CU'}[chip];out.append(' {%s,%u,%u,%u,%u,%u},'%(family,rfe,ch,idx,limited,ceil))
out.append('};');a.output.write_text('\n'.join(out)+'\n');print('210 independent primary B/C DFS20 MCS7 power vectors')
