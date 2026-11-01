#!/usr/bin/env python3
"""Emit validated station families plus one guarded, read-only discovery match.

An output path is required for writes. Run after the corresponding station HAL
and codec ports pass their checks; recognizing a family in the full catalog is
not sufficient to publish its personality.
"""
import argparse
from pathlib import Path
import plistlib
from usb_wifi_catalog import DEFAULT_DATA, ROOT, load_catalog, ready, matching_constraints, INTERFACE_FIELDS, discovery_only

def personalities(rows, bundle):
    result = {}
    for row in rows:
        discovery = discovery_only(row)
        if not ready(row) and not discovery: continue
        m = matching_constraints(row); family = row['candidate_backend_family_from_personality']
        match = {'CFBundleIdentifier':bundle,'IOClass':'RTWUSBController',
                 'IOProviderClass':'IOUSBHostInterface','IOMatchCategory':'IODefaultMatchCategory',
                 'idVendor':m['idVendor'],'idProduct':m['idProduct'],
                 'bConfigurationValue':m['bConfigurationValue'],'bInterfaceNumber':m['bInterfaceNumber'],
                 'IOProbeScore':100}
        if 'bcdDevice' in m: match['bcdDevice'] = m['bcdDevice']
        for field in INTERFACE_FIELDS:
            if field in m: match[field] = m[field]
        if discovery:
            match.update({'bInterfaceClass':255, 'bInterfaceSubClass':255,
                          'bInterfaceProtocol':255, 'RTWDiscoveryOnly':True})
            name = 'RTL8822BC-DISCOVERY-13b1-0043'
        else:
            name = f'{family.upper()}-{m["idVendor"]:04x}-{m["idProduct"]:04x}'
        result[name] = match
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data',type=Path,default=DEFAULT_DATA)
    parser.add_argument('--input',type=Path,default=ROOT/'kext/Info.plist')
    parser.add_argument('--output',type=Path)
    parser.add_argument('--check',action='store_true')
    args = parser.parse_args()
    if not args.check and args.output is None: parser.error('--output is required for a write')
    _, rows = load_catalog(args.data)
    info = plistlib.loads(args.input.read_bytes())
    expected = personalities(rows,info['CFBundleIdentifier'])
    if args.check:
        if info.get('IOKitPersonalities') != expected:
            parser.exit(1,'USB personalities differ from validated station and guarded discovery profiles\n')
    else:
        info['IOKitPersonalities'] = expected
        args.output.parent.mkdir(parents=True,exist_ok=True)
        args.output.write_bytes(plistlib.dumps(info,sort_keys=False))
        discoveries = sum(bool(p.get('RTWDiscoveryOnly')) for p in expected.values())
        print(f'Wrote {len(expected)-discoveries} source-validated station + {discoveries} guarded discovery personalities to {args.output}')
if __name__ == '__main__': main()
