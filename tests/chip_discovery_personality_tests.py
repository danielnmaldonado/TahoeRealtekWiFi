#!/usr/bin/env python3
"""Guarded discovery is separate from statically verified station authority."""
from pathlib import Path
import importlib.util,sys,copy
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'scripts'))
from usb_wifi_catalog import load_catalog,DEFAULT_DATA,ready
spec=importlib.util.spec_from_file_location('personality_generator',root/'scripts/generate-usb-personalities.py')
gen=importlib.util.module_from_spec(spec);spec.loader.exec_module(gen)
_,rows=load_catalog(DEFAULT_DATA)
row=next(x for x in rows if x['vid_pid']=='13b1:0043');before=copy.deepcopy(row)
result=gen.personalities(rows,'test.bundle');name='RTL8822BC-DISCOVERY-13b1-0043'
assert name in result,'The exact ambiguous ID needs its separate guarded discovery personality'
p=result[name]
assert p['RTWDiscoveryOnly'] is True
assert (p['idVendor'],p['idProduct'],p['bConfigurationValue'],p['bInterfaceNumber'])==(0x13b1,0x43,1,0)
assert tuple(p[k] for k in ('bInterfaceClass','bInterfaceSubClass','bInterfaceProtocol'))==(255,255,255)
assert not ready(row) and row==before
assert sum(ready(r) for r in rows)==262
assert len(result)==263 and sum(p.get('RTWDiscoveryOnly',False) for p in result.values())==1
assert all(not gen.discovery_only(r) for r in rows if r is not row)
for field,value in [('idVendor',0x0bda),('idProduct',0x8192),('bConfigurationValue',2),('bInterfaceNumber',1)]:
 bad=copy.deepcopy(row);bad['original_matching'][field]=value
 assert not gen.discovery_only(bad)
for key,value in [('family_mapping_status','verified'),('primary_usb_table_families',['rtl8822bu']),('vid_pid','0bda:8192')]:
 bad=copy.deepcopy(row);bad[key]=value;assert not gen.discovery_only(bad)
assert row==before
print('Personality discovery:262 station +1 exact FF/FF/FF discovery;immutable conflict remains unverified;8192 untouched')
