#!/usr/bin/env python3
"""End-to-end parse of generated inventory; never mistake silicon vs. station."""
import argparse
import csv
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('binary', help='compiled rtw-backends inventory tool')
parser.add_argument('--docs-root', type=Path,
                    default=Path(__file__).resolve().parents[1] / 'docs')
args = parser.parse_args()

dual={'RTL8192DU','RTL8812AU','RTL8814AU','RTL8821AU','RTL8821CU','RTL8822BU','RTL8822CU'}
binary = args.binary
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
only_five={"04bb:0959","04bb:095a","0bda:2003"}
assert {r["vid_pid"] for r in rows if r["station_5ghz"] and not r["station_2ghz"]}==only_five

# Published support must describe the same immutable IDs, eligibility and PHY
# limits as the compiled catalog. A valid CLI result alone missed stale files.
documented = json.loads((args.docs_root / 'USB-PHY-CAPABILITIES.json').read_text())
assert documented == all_cards, 'committed PHY JSON differs from compiled catalog/capabilities'
with (args.docs_root / 'USB-PHY-CAPABILITIES.csv').open(newline='') as stream:
    reader = csv.DictReader(stream)
    assert reader.fieldnames == list(rows[0]), 'committed PHY CSV fields differ from CLI'
    documented_csv = list(reader)
expected_csv = [{key: str(value) for key, value in row.items()} for row in rows]
assert documented_csv == expected_csv, 'committed PHY CSV differs from compiled catalog/capabilities'
print('Family, all-USB-ID inventory and committed JSON/CSV freshness contracts passed.')
