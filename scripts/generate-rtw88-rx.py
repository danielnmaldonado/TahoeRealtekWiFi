#!/usr/bin/env python3
"""Reproduce primary rtw88 RSSI tables/math from locally pinned source."""
from pathlib import Path
import argparse,re,json,hashlib
p=argparse.ArgumentParser(description=__doc__);p.add_argument('reference',type=Path);a=p.parse_args();r=Path(__file__).resolve().parents[1];meta=json.loads((r/'src/RTW88USBDataPath-Upstream.json').read_text());expected={f['path']:f['sha256'] for f in meta['files']};s=(a.reference/'phy.c').read_text();assert hashlib.sha256(s.encode()).hexdigest()==expected['phy.c']
out=['/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */','/* Copyright(c)2018-2019 Realtek Corporation. Pinned phy.c RSSI math. */','typedef int8_t s8;typedef uint8_t u8;typedef uint32_t u32;typedef uint64_t u64;','#define FRAC_BITS 3',re.search(r'static const u32 db_invert_table\[12\]\[8\] = \{.*?\n\};',s,re.S).group(0)]
for name in ['rtw_phy_power_2_db','rtw_phy_db_2_linear','rtw_phy_linear_2_db','rtw_phy_rf_power_2_rssi']:
 text=re.search(r'(?m)^(?:static )?(?:u8|u64) '+name+r'\(.*?\n\}',s,re.S).group(0);out.append(text if text.startswith('static') else 'static '+text)
(r/'src/RTW88RXRSSI.inc').write_text('\n\n'.join(out)+'\n')
print('Reproduced pinned four-function RSSI conversion and inverse-dB table')
