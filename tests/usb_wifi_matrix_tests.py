#!/usr/bin/env python3
"""Guard coverage of the complete inventory without false driver binding."""
import csv
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'scripts/usb_wifi_matrix.py'
CSV=ROOT/'docs/USB-WIFI-ADAPTERS.csv'

class USBMatrixTests(unittest.TestCase):
    def invoke(self,*args,expected=0):
        process=subprocess.run([sys.executable,str(SCRIPT),*args],capture_output=True,text=True)
        self.assertEqual(process.returncode,expected,(process.stdout,process.stderr))
        return process.stdout

    def test_full_matrix_and_kext_personalities(self):
        rows=list(csv.DictReader(io.StringIO(CSV.read_text(encoding='utf-8'))))
        self.assertEqual(len(rows),270)
        self.assertEqual(len({r['vid_pid'] for r in rows}),270)
        self.assertEqual(sum(r['kext_personality_enabled']=='yes' for r in rows),262)
        self.assertEqual([r['vid_pid'] for r in rows if r['kext_personality_enabled']=='discovery_only'],['13b1:0043'])
        self.assertEqual(sum(r['station_backend']=='pending_backend' for r in rows),1)
        self.assertEqual(sum(r['station_backend']=='unverified_or_conflict' for r in rows),7)
        self.assertEqual(sum(r['chipset_candidate']=='rtl8192eu' and r['station_backend']=='implemented' for r in rows),10)
        self.assertEqual(sum(r['chipset_candidate']=='rtl8192cu' and r['station_backend']=='implemented' for r in rows),88)
        self.assertEqual(sum(r['chipset_candidate']=='rtl8188eu' and r['station_backend']=='implemented' for r in rows),18)
        self.assertEqual(sum(r['chipset_candidate']=='rtl8814au' and r['station_backend']=='implemented' for r in rows),17)
        self.assertEqual(len({r['chipset_candidate'] for r in rows}),14)
        self.invoke('--check')

    def test_lookup_and_constraints(self):
        r=json.loads(self.invoke('--lookup','2357:0108','--bcd','0x0100','--config','1','--interface','0'))
        self.assertTrue(r['could_bind_if_physically_verified'])
        self.assertEqual(r['chipset_candidate'],'rtl8192eu')
        r=json.loads(self.invoke('--lookup','0bda:8176','--bcd','0x0200','--config','1','--interface','0'))
        self.assertTrue(r['could_bind_if_physically_verified'])
        r=json.loads(self.invoke('--lookup','0bda:8176','--bcd','0x0201','--config','1','--interface','0'))
        self.assertFalse(r['could_bind_if_physically_verified'])
        r=json.loads(self.invoke('--lookup','0bda:8176'))
        self.assertFalse(r['complete_identity_supplied'])
        self.assertFalse(r['could_bind_if_physically_verified'])
        r=json.loads(self.invoke('--lookup','0bda:8812','--bcd','0x0001','--config','1','--interface','0'))
        self.assertTrue(r['could_bind_if_physically_verified'])
        self.assertEqual(r['station_backend'],'implemented')
        r=json.loads(self.invoke('--lookup','0bda:2102','--bcd','0x0001','--config','1','--interface','0'))
        self.assertEqual(r['station_backend'],'implemented')
        self.assertEqual(r['chipset_candidate'],'rtl8822bu')
        self.assertFalse(r['complete_identity_supplied'])
        self.assertFalse(r['could_bind_if_physically_verified'])
        r=json.loads(self.invoke('--lookup','ffff:ffff',expected=1))
        self.assertFalse(r['found_in_catalog'])

    def test_extension_requires_actual_class_descriptors(self):
        base=('--lookup','20f4:809b','--config','1','--interface','0')
        self.assertFalse(json.loads(self.invoke(*base))['complete_identity_supplied'])
        for value in ('0','0xe0','0xff'):
            info=json.loads(self.invoke(*base,'--class',value,'--subclass','0xff','--protocol','0xff'))
            self.assertEqual(info['could_bind_if_physically_verified'],value=='0xff')
        for option in ('--config','--interface','--class','--subclass','--protocol'):
            self.invoke('--lookup','2357:0100',option,'256',expected=2)
        self.invoke('--lookup','2357:0100','--bcd','65536',expected=2)
        self.invoke('--lookup','2357:0100','--config','0',expected=2)
        cu=json.loads(self.invoke('--lookup','2357:0100','--config','1','--interface','0','--class','0xff'))
        self.assertTrue(cu['could_bind_if_physically_verified'])
        self.assertEqual(cu['catalog_origin'],'upstream_extension')

    def test_output_is_generated_not_hand_edited(self):
        original=CSV.read_bytes()
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'matrix.csv'
            self.invoke('--write','--output',str(path))
            self.assertEqual(path.read_bytes(),original)
            self.invoke('--check','--output',str(path))
            path.write_text('wrong')
            self.invoke('--check','--output',str(path),expected=2)

if __name__=='__main__': unittest.main()
