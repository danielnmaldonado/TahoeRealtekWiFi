#!/usr/bin/env python3
"""Compare CLI source capabilities with the reviewed family integration scope."""
import argparse,json,subprocess
parser=argparse.ArgumentParser();parser.add_argument('--backends',required=True)
args=parser.parse_args()
data=json.loads(subprocess.check_output([args.backends,'--json'],text=True))
assert data['catalog_profiles']==270 and data['station_profiles']==262
assert 'rtw-legacy-fwdiag' in data['firmware_diagnostics_scope']
families={r['name']:r for r in data['families']}
expected={'RTL8192CU': 88, 'RTL8188EU': 18, 'RTL8192EU': 10, 'RTL8192DU': 5, 'RTL8192FU': 4, 'RTL8710BU': 2, 'RTL8723BU': 3, 'RTL8812AU': 36, 'RTL8814AU': 17, 'RTL8814BU': 0, 'RTL8821AU': 35, 'RTL8821CU': 12, 'RTL8822BU': 30, 'RTL8822CU': 2}
assert families.keys()==expected.keys()
legacy_headers={'RTL8192CU','RTL8188EU','RTL8192EU','RTL8192DU',
                'RTL8192FU','RTL8710BU','RTL8723BU'}
for name,count in expected.items():
    row=families[name]
    assert row['station_profiles']==count,name
    assert row['kext_station_integrated']==bool(count),name
    assert row['rx_wire_codec']==bool(count),name
    assert row['passive_radio_geometry']==bool(count),name
    assert row['firmware_header_inspector']==(name in legacy_headers),name
    assert not row['experimental_usb_firmware_boot'],name
for name in ('RTL8192FU','RTL8710BU','RTL8723BU'):
    assert families[name]['firmware_download_plan'],name
print('14 family CLI capabilities,262 integrated station profiles; generic physical firmware boot disabled')
