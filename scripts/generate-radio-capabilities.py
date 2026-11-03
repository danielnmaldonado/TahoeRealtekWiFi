#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Refresh the PHY support documents from compiled, hardware-free inventory."""
import argparse
import csv
import io
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('binary', type=Path, help='compiled rtw-backends inventory tool')
parser.add_argument('--docs-root', type=Path,
                    default=Path(__file__).resolve().parents[1] / 'docs')
parser.add_argument('--check', action='store_true', help='report drift without writing')
args = parser.parse_args()

json_text = subprocess.check_output(
    [str(args.binary.resolve()), '--adapters-json'], text=True)
inventory = json.loads(json_text)
rows = inventory['adapters']
csv_stream = io.StringIO(newline='')
writer = csv.DictWriter(csv_stream, fieldnames=list(rows[0]), lineterminator='\n')
writer.writeheader()
writer.writerows(rows)
documents = {'USB-PHY-CAPABILITIES.json': json_text,
             'USB-PHY-CAPABILITIES.csv': csv_stream.getvalue()}

for name, content in documents.items():
    path = args.docs_root / name
    if args.check:
        if not path.exists() or path.read_text() != content:
            raise SystemExit(f'{path}: differs from compiled catalog/capabilities')
    else:
        path.write_text(content)
print(f'PHY documents {"checked" if args.check else "updated"}: '
      f'{len(rows)} IDs, {inventory["station_profiles"]} station profiles; '
      'source eligibility, not hardware certification')
