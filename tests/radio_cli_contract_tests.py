#!/usr/bin/env python3
"""End-to-end parse of generated inventory; never mistake silicon vs. station."""
import json
import subprocess
import sys

dual={'RTL8192DU','RTL8812AU','RTL8814AU','RTL8821AU','RTL8821CU','RTL8822BU','RTL8822CU'}
binary = sys.argv[1]
family = json.loads(subprocess.check_output([binary, '--json'], text=True))
assert len(family['families']) == 14
assert family['catalog_profiles'] == 270
assert family['station_profiles'] == 262
assert all(row['station_5ghz']==(row['name'] in dual) for row in family['families'])
assert next(row for row in family['families'] if row['name'] == 'RTL8814AU')['chip_family_5ghz']
assert next(row for row in family['families'] if row['name'] == 'RTL8814AU')['station_5ghz']
all_cards = json.loads(subprocess.check_output([binary, '--adapters-json'], text=True))
rows = all_cards['adapters']
assert len(rows) == all_cards['catalog_profiles'] == 270
assert sum(row['identity_verified'] for row in rows) == all_cards['verified_profiles'] == 263
assert sum(row['station_enabled'] for row in rows) == all_cards['station_profiles'] == 262
assert len(set(row['vid_pid'] for row in rows)) == 270
assert all(row['station_5ghz']==(row['station_enabled'] and row['family'] in dual) for row in rows)
assert sum(row['station_5ghz'] for row in rows)==137
assert all(not row['station_2ghz'] or row['station_enabled'] for row in rows)
assert all(not row['station_enabled'] or row['identity_verified'] for row in rows)
assert all(row['station_phy_ceiling_mbps'] <= row['family_phy_ceiling_mbps'] for row in rows)
assert all(not row['station_enabled'] or row['station_phy_ceiling_mbps'] > 0 for row in rows)
assert all(row['chip_family_5ghz'] or row['family_5g_width_mhz'] == 0 for row in rows)
assert all(not row['identity_verified'] or row['family'] != 'UNKNOWN' for row in rows)
print('Family and all-USB-ID JSON inventory contracts passed.')

only_five={"04bb:0959","04bb:095a","0bda:2003"}
assert {r["vid_pid"] for r in rows if r["station_5ghz"] and not r["station_2ghz"]}==only_five
