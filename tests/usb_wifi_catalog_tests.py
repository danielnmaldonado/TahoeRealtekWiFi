#!/usr/bin/env python3
"""Exercise generated C lookup and plist output against the pinned data model."""
from pathlib import Path
import ctypes
import copy
import json
import plistlib
import subprocess
import sys
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'scripts'))
from usb_wifi_catalog import matching_constraints, primary_verified, load_catalog

class OEMIdentityTests(unittest.TestCase):
    def test_new_exact_bindings_are_admitted_and_true_conflicts_stay_off(self):
        data=json.loads((ROOT/'data/legacy-usb-adapters.json').read_text())
        rows={r['vid_pid']:r for r in data['devices']}
        expected={
            'rtl8821au':('0411:029d','04bb:0959','04bb:095a','07b8:0811','0bda:0170','0bda:2003','0df6:007a','0e66:0024','1668:8108','1eda:2520','1eda:2525','2357:0121'),
            'rtl8812au':('08c4:0115','0bda:a812'),
            'rtl8192cu':('056e:4009',), 'rtl8192eu':('07b8:818b',),
            'rtl8821cu':('0b05:18e9','2357:0112','2357:0113','2357:0114','2357:0128','2357:0129','2357:012c'),
            'rtl8188eu':('0bda:a179','2357:0127','2357:012a','2357:012f','2357:0132'),
            'rtl8814au':('0df6:007b','2001:3320','2c2b:0002'), 'rtl8723bu':('20f4:108a',),
        }
        for family, keys in expected.items():
            for key in keys:
                with self.subTest(key=key):
                    self.assertEqual(rows[key]['candidate_backend_family_from_personality'],family)
                    self.assertTrue(primary_verified(rows[key]))
        for key in ('0bda:8192','13b1:0043'):
            self.assertFalse(primary_verified(rows[key]))

    def fixture(self, key):
        data=json.loads((ROOT/'data/legacy-usb-adapters.json').read_text())
        row=copy.deepcopy(next(r for r in data['devices'] if r['vid_pid']==key))
        proof=json.loads((ROOT/'data/primary-oem-usb-identities.json').read_text())
        binding=next(b for b in proof['bindings'] if b['vid_pid']==key)
        evidence={**proof['sources'][binding['source_id']], **binding, 'kind':'oem_inf'}
        row['family_mapping_status']='exact_numeric_id_correspondence'
        row['primary_usb_table_families']=[row['candidate_backend_family_from_personality']]
        row['primary_usb_table_evidence']=[evidence]
        if key in ('04bb:0959','04bb:095a','0bda:2003'):
            row['station_band_restriction']=2
        return row

    def test_screenbeam_signed_dual_band_binding_is_exact(self):
        data=json.loads((ROOT/'data/legacy-usb-adapters.json').read_text())
        row=copy.deepcopy(next(r for r in data['devices'] if r['vid_pid']=='1668:8108'))
        self.assertTrue(primary_verified(row), 'signed OEM8108 INF must admit its exact RTL8811AU binding')
        proof=row['primary_usb_table_evidence'][0]
        self.assertEqual(proof['hardware_id'],'USB\\VID_1668&PID_8108')
        self.assertEqual(proof['install_section'],'Dual_Actiontec8811A.ndi')
        self.assertIn('RTL8811AU_COMMON.reg',proof['install_section_addreg'])
        self.assertNotIn('effective_matching',row)
        self.assertEqual(row.get('station_band_restriction',0),0)
        for field,value in [('hardware_id','USB\\VID_1668&PID_8105'),
                            ('install_section','5G_Actiontec8811A.ndi'),
                            ('entry','%x%=Dual_Actiontec8811A.ndi, USB\\VID_1668&PID_8108'),
                            ('line',167),('inf_sha256','0'*64),('archive_sha256','0'*64),
                            ('catalog_sha256','0'*64),('inf_path','Driver/AddDrv/Win7x86/sbt10.inf'),
                            ('archive_url','https://example.com/ScreenBeam_Setup.exe')]:
            with self.subTest(field=field):
                changed=copy.deepcopy(row); changed['primary_usb_table_evidence'][0][field]=value
                self.assertFalse(primary_verified(changed))
        changed=copy.deepcopy(row)
        changed['primary_usb_table_evidence'][0]['signature_verification']['result']='unverified'
        self.assertFalse(primary_verified(changed))
        changed=copy.deepcopy(row); changed['station_band_restriction']=2
        self.assertFalse(primary_verified(changed))
        sibling=next(r for r in data['devices'] if r['vid_pid']=='1668:8105')
        self.assertFalse(primary_verified(sibling))

    def test_official_oem_exact_hardware_binding_admits_cu_family(self):
        row=self.fixture('0b05:18e9')
        self.assertTrue(primary_verified(row), 'exact OEM RTL8811cu binding must establish RTL8821CU family')
        self.assertTrue(primary_verified(self.fixture('07b8:818b')), 'catalog-authenticated RTL8192eu binding must establish EU family')

    def test_oem_proof_rejects_changed_pair_ndi_hash_and_source(self):
        original=self.fixture('0b05:18e9')
        for field,value in [('hardware_id','USB\\VID_0B05&PID_18F1'),
                            ('install_section','RTL8188eu.ndi'),
                            ('entry','%x%=RTL8811cu.ndi, USB\\VID_0B05&PID_18E9'),
                            ('line',342),('inf_sha256','0'*64),('cab_sha256','0'*64),
                            ('catalog_details_url','https://example.com/driver'),
                            ('cab_url','https://example.com/driver.cab')]:
            with self.subTest(field=field):
                row=copy.deepcopy(original); row['primary_usb_table_evidence'][0][field]=value
                self.assertFalse(primary_verified(row))

    def test_oem_5ghz_binding_cannot_erase_or_fabricate_band_restriction(self):
        for key in ('04bb:0959','04bb:095a','0bda:2003'):
            row=self.fixture(key)
            self.assertTrue(primary_verified(row))
            for mask in (None,0,1,3,4,True,'2'):
                with self.subTest(key=key,mask=mask):
                    changed=copy.deepcopy(row)
                    if mask is None: changed.pop('station_band_restriction',None)
                    else: changed['station_band_restriction']=mask
                    self.assertFalse(primary_verified(changed))
        row=self.fixture('0b05:18e9'); row['station_band_restriction']=2
        self.assertFalse(primary_verified(row), 'a non-5GOnly OEM record cannot invent a board band restriction')

    def test_effective_interface_override_requires_exact_oem_interface_proof(self):
        row=self.fixture('0b05:18e9'); row['effective_matching']={'bInterfaceNumber':2}
        with self.assertRaises(ValueError): matching_constraints(row)

    def test_oem_interface_suffix_cannot_authorize_the_bluetooth_interface(self):
        row=self.fixture('0bda:0170')
        row['original_matching']['bInterfaceNumber']=0
        self.assertFalse(primary_verified(row))
        row['effective_matching']={'bInterfaceNumber':2}
        self.assertTrue(primary_verified(row))
        row=self.fixture('0bda:0170'); row['effective_matching']={'bInterfaceNumber':2}
        self.assertEqual(matching_constraints(row)['bInterfaceNumber'],2)
        row['effective_matching']={'bInterfaceNumber':0}
        with self.assertRaises(ValueError): matching_constraints(row)

    def test_invalid_band_mask_is_rejected_by_catalog_reader(self):
        data=json.loads((ROOT/'data/legacy-usb-adapters.json').read_text())
        for mask in (-1,4,True,'2'):
            with self.subTest(mask=mask), tempfile.TemporaryDirectory() as td:
                changed=copy.deepcopy(data); changed['devices'][0]['station_band_restriction']=mask
                path=Path(td)/'data.json';path.write_text(json.dumps(changed))
                with self.assertRaises(ValueError): load_catalog(path)

class CatalogTests(unittest.TestCase):
    def test_catalog_deliverables_exist(self):
        self.assertTrue((ROOT/'include/rtw/USBWiFiProfiles.h').is_file(), 'family-aware matching API is missing')
        self.assertTrue((ROOT/'data/legacy-usb-adapters.json').is_file(), 'pinned support inventory is missing')
    def test_generated_c_matches_each_full_identity_and_rejects_wrong_constraint(self):
        data=json.loads((ROOT/'data/legacy-usb-adapters.json').read_text())
        class Profile(ctypes.Structure):
            _fields_=[('vendor',ctypes.c_uint16),('product',ctypes.c_uint16),('bcd',ctypes.c_uint16),('match_bcd',ctypes.c_uint8),('config',ctypes.c_uint8),('iface',ctypes.c_uint8),('family',ctypes.c_int),('verified',ctypes.c_uint8),('name',ctypes.c_char_p),('mask',ctypes.c_uint8),('cls',ctypes.c_uint8),('sub',ctypes.c_uint8),('proto',ctypes.c_uint8),('band_restriction',ctypes.c_uint8)]
        with tempfile.TemporaryDirectory() as td:
            libpath=Path(td)/'profiles.dylib'
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-shared','-fPIC','-I',str(ROOT/'include'),str(ROOT/'src/USBWiFiProfiles.c'),'-o',str(libpath)],check=True)
            lib=ctypes.CDLL(str(libpath)); lib.rtw_usb_wifi_lookup.argtypes=[ctypes.c_uint16]*3+[ctypes.c_uint8]*2; lib.rtw_usb_wifi_lookup.restype=ctypes.POINTER(Profile)
            lib.rtw_usb_wifi_lookup_descriptor.argtypes=[ctypes.c_uint16]*3+[ctypes.c_uint8]*5
            lib.rtw_usb_wifi_lookup_descriptor.restype=ctypes.POINTER(Profile)
            lib.rtw_usb_wifi_profile_count.restype=ctypes.c_size_t
            lib.rtw_usb_wifi_station_backend.argtypes=[ctypes.POINTER(Profile)]
            lib.rtw_usb_wifi_station_backend.restype=ctypes.c_int
            ready_count=0
            self.assertEqual(lib.rtw_usb_wifi_profile_count(),270)
            for row in data['devices']:
                m=matching_constraints(row)
                args=[m['idVendor'],m['idProduct'],m.get('bcdDevice',0x1234),
                      m['bConfigurationValue'],m['bInterfaceNumber'],
                      m.get('bInterfaceClass',0),m.get('bInterfaceSubClass',0),m.get('bInterfaceProtocol',0)]
                p=lib.rtw_usb_wifi_lookup_descriptor(*args)
                self.assertTrue(p,row['vid_pid']); self.assertEqual(p.contents.name.decode(),row['personality'])
                self.assertEqual(bool(lib.rtw_usb_wifi_lookup(*args[:5])), p.contents.mask == 0, row['vid_pid'])
                for index, field in enumerate(('bInterfaceClass','bInterfaceSubClass','bInterfaceProtocol'), 5):
                    if field in m:
                        for value in range(256):
                            wrong=args.copy(); wrong[index]=value
                            self.assertEqual(bool(lib.rtw_usb_wifi_lookup_descriptor(*wrong)),value==m[field],row['vid_pid'])
                # Independent dispatch model: exact primary correspondence plus
                # one of thirteen implementations. Other families/conflicts stay off.
                expected={'rtl8192cu':1,'rtl8188eu':2,'rtl8192eu':3,'rtl8192du':4,'rtl8192fu':5,'rtl8710bu':6,'rtl8723bu':7,
                          'rtl8812au':8,'rtl8814au':9,'rtl8821au':11,
                          'rtl8821cu':12,'rtl8822bu':13,'rtl8822cu':14}.get(row['candidate_backend_family_from_personality'],0)
                if row['family_mapping_status']!='exact_numeric_id_correspondence': expected=0
                self.assertEqual(lib.rtw_usb_wifi_station_backend(p),expected,row['vid_pid'])
                ready_count += bool(expected)
                wrong=args.copy(); wrong[3]=2; self.assertFalse(lib.rtw_usb_wifi_lookup_descriptor(*wrong),row['vid_pid'])
                wrong=args.copy(); wrong[4]=1; self.assertFalse(lib.rtw_usb_wifi_lookup_descriptor(*wrong),row['vid_pid'])
                if 'bcdDevice' in m:
                    wrong=args.copy(); wrong[2]^=1; self.assertFalse(lib.rtw_usb_wifi_lookup_descriptor(*wrong),row['vid_pid'])
            self.assertEqual(ready_count,262)
    def test_personality_generation_excludes_unsupported_and_preserves_constraints(self):
        with tempfile.TemporaryDirectory() as td:
            target=Path(td)/'Info.plist'
            subprocess.run([sys.executable,str(ROOT/'scripts/generate-usb-personalities.py'),'--output',str(target)],check=True)
            personalities=plistlib.loads(target.read_bytes())['IOKitPersonalities']
            identities={(p['idVendor'],p['idProduct']):p for p in personalities.values()}
            self.assertIn((0x2357,0x0108),identities)
            self.assertIn((0x0bda,0x8176),identities)
            self.assertIn((0x2357,0x010c),identities)
            for pair in [(0x0bda,0x8192),(0x0bda,0xb814)]: self.assertNotIn(pair,identities)
            discovery=identities[(0x13b1,0x0043)]
            self.assertIs(discovery["RTWDiscoveryOnly"],True)
            self.assertEqual(tuple(discovery[k] for k in ("bInterfaceNumber","bConfigurationValue","bInterfaceClass","bInterfaceSubClass","bInterfaceProtocol")),(0,1,255,255,255))
            for pair in [(0x07b8,0x818b),(0x0bda,0x2102),(0x0bda,0x8812)]: self.assertIn(pair,identities)
            self.assertEqual(identities[(0x0bda,0x8176)]['bcdDevice'],0x0200)
            self.assertEqual(identities[(0x0bda,0x818c)]['bInterfaceNumber'],2)
            for p in identities.values():
                self.assertEqual(p['IOProviderClass'],'IOUSBHostInterface')
                self.assertEqual(p['bConfigurationValue'],1)
            # Repeat execution must be deterministic and --check must detect drift.
            original=target.read_bytes()
            subprocess.run([sys.executable,str(ROOT/'scripts/generate-usb-personalities.py'),'--output',str(target)],check=True)
            self.assertEqual(original,target.read_bytes())
            subprocess.run([sys.executable,str(ROOT/'scripts/generate-usb-personalities.py'),'--input',str(target),'--check'],check=True)
            info=plistlib.loads(original); info['IOKitPersonalities'].clear();target.write_bytes(plistlib.dumps(info))
            result=subprocess.run([sys.executable,str(ROOT/'scripts/generate-usb-personalities.py'),'--input',str(target),'--check'],capture_output=True)
            self.assertNotEqual(result.returncode,0)
    def test_generator_does_not_upgrade_wrong_primary_family_evidence(self):
        data=json.loads((ROOT/'data/legacy-usb-adapters.json').read_text())
        # C82C is a C-generation AC identity. A mistaken human edit to its
        # inferred family must not turn its original 8822c evidence into CU.
        row=next(r for r in data['devices'] if r['vid_pid']=='0bda:c82c')
        row['candidate_backend_family_from_personality']='rtl8192cu'
        row['primary_usb_table_families']=['rtl8192cu']
        with tempfile.TemporaryDirectory() as td:
            source=Path(td)/'data.json'; target=Path(td)/'Info.plist'
            source.write_text(json.dumps(data))
            subprocess.run([sys.executable,str(ROOT/'scripts/generate-usb-personalities.py'),'--data',str(source),'--output',str(target)],check=True)
            identities={(p['idVendor'],p['idProduct']) for p in plistlib.loads(target.read_bytes())['IOKitPersonalities'].values()}
            self.assertNotIn((0x0bda,0xc82c),identities)
    def test_generated_c_artifact_is_current(self):
        subprocess.run([sys.executable,str(ROOT/'scripts/generate-usb-profiles.py'),'--check'],check=True)
if __name__=='__main__':unittest.main()
