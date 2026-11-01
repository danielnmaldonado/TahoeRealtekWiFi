#!/usr/bin/env python3
"""Source/data contracts; these do not replace native IOKit or RF tests."""
import collections
import copy
import json
from pathlib import Path
import plistlib
import re
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from usb_wifi_catalog import load_catalog, primary_verified, ready, matching_constraints

class AdapterSupportContractTests(unittest.TestCase):
    def setUp(self):
        self.data, self.rows = load_catalog()

    def test_8814b_identity_proof_does_not_invent_an_operating_hal(self):
        row=next(r for r in self.rows if r['vid_pid']=='0bda:b814')
        self.assertTrue(primary_verified(row))
        self.assertFalse(ready(row))
        self.assertEqual(row['candidate_backend_family_from_personality'],'rtl8814bu')
        e=row['primary_usb_table_evidence'][0]
        self.assertEqual(e['driver_info'],'RTL8814B')
        self.assertEqual(e['commit'],'465ae7cb522a382adc85ea5c31f228af8d172148')
        self.assertEqual(e['compile_guard'],'CONFIG_RTL8814B')
        m=matching_constraints(row)
        for field in ('bInterfaceClass','bInterfaceSubClass','bInterfaceProtocol'):
            self.assertEqual(m[field],255)

    def test_two_additional_legacy_aliases_preserve_original_matching(self):
        for identity,family in [('056e:4010','rtl8821au'),('20f4:648c','rtl8192cu')]:
            row=next(r for r in self.rows if r['vid_pid']==identity)
            self.assertTrue(primary_verified(row))
            self.assertTrue(ready(row))
            self.assertEqual(row['candidate_backend_family_from_personality'],family)
            self.assertNotIn('interface_matching',row)
            self.assertEqual(matching_constraints(row)['bInterfaceNumber'],row['original_matching']['bInterfaceNumber'])

    def test_ccnc_2102_uses_numeric_linux_mapping(self):
        row=next(r for r in self.rows if r['vid_pid']=='0bda:2102')
        self.assertEqual(row['personality'],'CCNC_2102_8811CU')
        self.assertEqual(row['claimed_family_token'],'8811CU')
        self.assertEqual(row['candidate_backend_family_from_personality'],'rtl8822bu')
        self.assertTrue(primary_verified(row))
        self.assertTrue(ready(row))
        actual=matching_constraints(row)
        for key in ('bInterfaceClass','bInterfaceSubClass','bInterfaceProtocol'):
            self.assertEqual(actual[key],255)
        self.assertEqual(row['primary_usb_table_evidence'][0]['driver_info'],'rtw8822b_hw_spec')

    def test_counts_are_computed_from_rows(self):
        counts = self.data["counts"]
        self.assertEqual(dict(collections.Counter(r["family_mapping_status"] for r in self.rows)),
                         counts["mapping_status"])
        self.assertEqual(dict(collections.Counter(r["candidate_backend_family_from_personality"] for r in self.rows)),
                         counts["candidate_backend_families"])
        self.assertEqual(sum(map(primary_verified, self.rows)), 263)
        self.assertEqual(sum(map(ready, self.rows)), 262)

    def test_two_ctv_additions_are_source_pinned_and_not_broadened(self):
        for identity in ("0bda:0a8a", "cdab:8011"):
            with self.subTest(identity=identity):
                row = next(r for r in self.rows if r["vid_pid"] == identity)
                self.assertTrue(ready(row))
                self.assertEqual(row["candidate_backend_family_from_personality"], "rtl8192cu")
                evidence = row["primary_usb_table_evidence"]
                self.assertEqual(len(evidence), 1)
                self.assertEqual(evidence[0]["repository"], "pvaret/rtl8192cu-fixes")
                self.assertEqual(evidence[0]["commit"], "f0dfbb46a891820b27942ba3e213af83f2452957")
                self.assertEqual(evidence[0]["driver_info"], "RTL8192C_USB_IDS")
                self.assertEqual(evidence[0]["compile_guard"], "CONFIG_RTL8192C")
                m = row["original_matching"]
                self.assertEqual((m["bcdDevice"], m["bConfigurationValue"], m["bInterfaceNumber"]),
                                 (0x0200, 1, 0))
                self.assertEqual(evidence[0]["usb_device_arguments"],
                                 f"0x{m['idVendor']:04X}, 0x{m['idProduct']:04X}")

    def test_shadow_identity_cannot_disagree_with_original(self):
        for field in ("idVendor", "idProduct"):
            for value in (True, -1, 65536, 0):
                data = copy.deepcopy(self.data)
                data["devices"][0][field] = value
                with tempfile.TemporaryDirectory() as tmp:
                    path = Path(tmp) / "catalog.json"
                    path.write_text(json.dumps(data))
                    with self.assertRaises(ValueError):
                        load_catalog(path)

    def test_plist_matches_exactly_the_station_profiles(self):
        plist = plistlib.loads((ROOT / "kext/Info.plist").read_bytes())
        active = {(p["idVendor"],p["idProduct"]):p for p in plist["IOKitPersonalities"].values() if not p.get("RTWDiscoveryOnly",False)}
        discoveries=[p for p in plist["IOKitPersonalities"].values() if p.get("RTWDiscoveryOnly",False)]
        self.assertEqual(len(discoveries),1)
        d=discoveries[0]
        self.assertEqual(tuple(d[k] for k in ("idVendor","idProduct","bConfigurationValue","bInterfaceNumber","bInterfaceClass","bInterfaceSubClass","bInterfaceProtocol")),(0x13b1,0x0043,1,0,255,255,255))
        expected = {(r["idVendor"],r["idProduct"]):r for r in self.rows if ready(r)}
        self.assertEqual(active.keys(), expected.keys())
        for key, row in expected.items():
            for constraint in ("idVendor","idProduct","bcdDevice","bConfigurationValue","bInterfaceNumber","bInterfaceClass","bInterfaceSubClass","bInterfaceProtocol"):
                self.assertEqual(active[key].get(constraint), matching_constraints(row).get(constraint))
        self.assertIn((0x0bda,0x2102),active)
        self.assertIn((0x07b8,0x818b),active)
        for key in ((0x13b1,0x0043),(0x038b,0x0100),(0x0e0b,0x9071)):
            self.assertNotIn(key, active)

    def test_native_app_and_native_tests_link_same_new_sources(self):
        for path in ("app/build.command", "app/test.command"):
            script = (ROOT / path).read_text()
            for token in ("AdapterDiagnostics.mm", "USBWiFiProfiles.cpp", "-I../include",
                          "-framework IOKit", "-framework CoreFoundation"):
                self.assertIn(token, script)
        self.assertIn('#include "../src/USBWiFiProfiles.c"',
                      (ROOT / "app/USBWiFiProfiles.cpp").read_text())
        main = (ROOT / "app/Main.mm").read_text()
        self.assertIn('@selector(adapterSupport:)', main)
        self.assertIn("RTWAdapterSupportReport()", main)

    def test_diagnostic_uses_only_allowlisted_registry_properties(self):
        source = (ROOT / "app/AdapterDiagnostics.mm").read_text()
        keys = set(re.findall(r'CFSTR\("([^"]+)"\)', source))
        self.assertEqual(keys, {"idVendor","idProduct","bcdDevice","bConfigurationValue",
                                "bInterfaceNumber","CFBundleIdentifier","RTWSiliconFamily",
                                "RTWChipIdentityVerified","RTWHardwareChipCode",
                                "bInterfaceClass","bInterfaceSubClass","bInterfaceProtocol"})
        for prohibited_call in ("IOServiceOpen(", "IOConnectCallMethod(", "SecItemCopyMatching(",
                                "IORegistryEntryCreateCFProperties(", "IORegistryEntrySearchCFProperty(",
                                "system(", "popen("):
            self.assertNotIn(prohibited_call, source)
        self.assertIn("IORegistryEntryGetChildIterator(iface, kIOServicePlane", source)
        self.assertIn("IORegistryEntryGetChildIterator(device, kIOServicePlane", source)
        self.assertIn('IOObjectConformsTo(child, "IOUSBHostDevice")', source)
        self.assertIn('IOObjectConformsTo(child, "IOUSBHostInterface")', source)
        self.assertIn("IOObjectRelease(object_)", source)

if __name__ == "__main__":
    unittest.main()
