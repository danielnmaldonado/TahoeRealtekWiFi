#!/usr/bin/env python3
"""Reproducible complete USB adapter inventory, not a blind kext ID enabler.

The matrix includes every ID from the pinned original packages, with exact
matching constraints and an independent working-station-backend flag.
"""
import argparse
import csv
import io
import json
from pathlib import Path
from usb_wifi_catalog import ROOT, load_catalog, primary_verified, ready, matching_constraints, INTERFACE_FIELDS, discovery_only

DEFAULT_OUTPUT = ROOT / 'docs/USB-WIFI-ADAPTERS.csv'
FIELDS = (
    'vid_pid', 'personality', 'chipset_candidate', 'source_evidence',
    'station_backend', 'kext_personality_enabled', 'bcdDevice',
    'bConfigurationValue', 'bInterfaceNumber', 'bInterfaceClass', 'bInterfaceSubClass',
    'bInterfaceProtocol', 'station_band_restriction', 'source_bundle', 'catalog_origin', 'hardware_validation',
)

def record(row):
    m = matching_constraints(row)
    status = ('implemented' if ready(row) else
              'pending_backend' if primary_verified(row) else
              'unverified_or_conflict')
    return {
        'vid_pid': row['vid_pid'],
        'personality': row['personality'],
        'chipset_candidate': row['candidate_backend_family_from_personality'],
        'source_evidence': row['family_mapping_status'],
        'station_backend': status,
        'kext_personality_enabled': 'yes' if ready(row) else 'discovery_only' if discovery_only(row) else 'no',
        'bcdDevice': f"0x{m['bcdDevice']:04x}" if 'bcdDevice' in m else '',
        'bConfigurationValue': str(m['bConfigurationValue']),
        'bInterfaceNumber': str(m['bInterfaceNumber']),
        'source_bundle': row['bundle'],
        'station_band_restriction': {0:'family_default',1:'2ghz_only',2:'5ghz_only',3:'dual_band'}[row.get('station_band_restriction',0)],
        'catalog_origin': row.get('origin', 'legacy'),
        'hardware_validation': 'not_performed',
        **{field: f"0x{m[field]:02x}" if field in m else '' for field in INTERFACE_FIELDS},
    }

def matrix(rows):
    output=io.StringIO(newline='')
    writer=csv.DictWriter(output,fieldnames=FIELDS,lineterminator='\n')
    writer.writeheader()
    for row in rows:
        writer.writerow(record(row))
    return output.getvalue()

def numeric(text):
    return int(text,0)

def main(argv=None):
    parser=argparse.ArgumentParser(description=__doc__)
    actions=parser.add_mutually_exclusive_group(required=True)
    actions.add_argument('--check',action='store_true',help='check committed complete CSV is current')
    actions.add_argument('--write',action='store_true',help='write the complete adapter matrix')
    actions.add_argument('--lookup',help='inspect one VID:PID (e.g. 2357:0108)')
    actions.add_argument('--summary',action='store_true',help='show backend and evidence counts')
    parser.add_argument('--output',type=Path,default=DEFAULT_OUTPUT)
    parser.add_argument('--bcd',type=numeric,help='optional actual bcdDevice from USB descriptor (0xNNNN)')
    parser.add_argument('--config',type=numeric,help='optional actual USB configuration')
    parser.add_argument('--interface',type=numeric,help='optional actual USB interface number')
    parser.add_argument('--class', dest='interface_class', type=numeric, help='actual bInterfaceClass')
    parser.add_argument('--subclass', type=numeric, help='actual bInterfaceSubClass')
    parser.add_argument('--protocol', type=numeric, help='actual bInterfaceProtocol')
    args=parser.parse_args(argv)
    for name, value, maximum in (
        ('bcd', args.bcd, 65535), ('config', args.config, 255), ('interface', args.interface, 255),
        ('class', args.interface_class, 255), ('subclass', args.subclass, 255), ('protocol', args.protocol, 255)):
        if value is not None and not 0 <= value <= maximum:
            parser.error(f'--{name} is out of range')
    if args.config == 0:
        parser.error('--config must select an active USB configuration')
    _,rows=load_catalog()
    csv_text=matrix(rows)
    if args.check:
        if not args.output.is_file() or args.output.read_text(encoding='utf-8')!=csv_text:
            parser.error(f'{args.output} differs from authoritative data; run --write')
        print(f'Matrix verified: {len(rows)} profiles, {sum(ready(r) for r in rows)} with an integrated backend')
    elif args.write:
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_bytes(csv_text.encode('utf-8'))
        print(f'Wrote {len(rows)} adapter profiles: {args.output}')
    elif args.summary:
        by_family={}
        for r in rows:
            name=r['candidate_backend_family_from_personality']
            counter=by_family.setdefault(name,{'total':0,'verified':0,'implemented':0})
            counter['total']+=1
            counter['verified']+=int(primary_verified(r))
            counter['implemented']+=int(ready(r))
        print(json.dumps({'devices':len(rows),'kext_personalities':sum(ready(r) for r in rows),
                          'families':by_family},indent=2,sort_keys=True))
    else:
        key=args.lookup.lower().removeprefix('0x').replace('-',':')
        try:
            parts=key.split(':')
            if len(parts)!=2 or not all(len(p)==4 for p in parts):raise ValueError
            vendor,product=(int(p,16) for p in parts)
        except ValueError:
            parser.error('--lookup requires VID:PID in four-digit hex (e.g. 2357:0108)')
        selected=next((r for r in rows if r['idVendor']==vendor and r['idProduct']==product),None)
        if selected is None:
            print(json.dumps({'vid_pid':key,'found_in_catalog':False},sort_keys=True))
            return 1
        info=record(selected)
        match=matching_constraints(selected)
        conditions={
            'bcdDevice':(args.bcd,match.get('bcdDevice')),
            'bConfigurationValue':(args.config,match['bConfigurationValue']),
            'bInterfaceNumber':(args.interface,match['bInterfaceNumber']),
            'bInterfaceClass':(args.interface_class,match.get('bInterfaceClass')),
            'bInterfaceSubClass':(args.subclass,match.get('bInterfaceSubClass')),
            'bInterfaceProtocol':(args.protocol,match.get('bInterfaceProtocol')),
        }
        info['specified_usb_identity_matches']=all(got is None or expected is None or got==expected
                                                    for got,expected in conditions.values())
        info['complete_identity_supplied']=(args.config is not None and args.interface is not None and
            all(got is not None for got,expected in conditions.values() if expected is not None))
        info['could_bind_if_physically_verified']=(ready(selected) and
            info['specified_usb_identity_matches'] and info['complete_identity_supplied'])
        info['warning']='Inventory match is not evidence of physical compatibility or verified throughput.'
        print(json.dumps(info,indent=2,sort_keys=True,ensure_ascii=False))
    return 0

if __name__=='__main__':
    raise SystemExit(main())
