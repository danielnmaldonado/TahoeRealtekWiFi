#!/usr/bin/env python3
"""The new aliases must reach existing backends, never synthesize a new HAL."""
import copy
import json
from pathlib import Path
import plistlib
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from usb_wifi_catalog import load_catalog, matching_constraints, ready

EXPECTED = {
    "04bb:0950","050d:11f2","0846:9042","0846:9043","0bda:17c0",
    "0bda:8170","0bda:817c","0bda:817d","0bda:817e","0bda:8186",
    "0bda:8191","0bda:819a","0bda:8754","0eb0:9071","1058:0631",
    "13d3:3358","13d3:3359","20f4:809b","2357:0100","9846:9041"
}

class ExtensionTests(unittest.TestCase):
    def setUp(self):
        self.data, self.rows = load_catalog()

    def rejected(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "catalog.json"
            path.write_text(json.dumps(data), encoding="utf-8")
            with self.assertRaises(ValueError):
                load_catalog(path)

    def test_exact_extension_set_and_complete_station_dispatch(self):
        extra = [r for r in self.rows if r.get("origin") == "upstream_extension"]
        self.assertEqual({r["vid_pid"] for r in extra}, EXPECTED)
        self.assertEqual(sum(ready(r) for r in self.rows), 262)
        self.assertEqual(sum(r.get("origin", "legacy") == "legacy" for r in self.rows), 250)
        for row in extra:
            self.assertTrue(ready(row))
            match = matching_constraints(row)
            self.assertEqual(match["bInterfaceClass"], 255)
            self.assertEqual((match["bConfigurationValue"], match["bInterfaceNumber"]), (1, 0))
            self.assertNotIn("bcdDevice", match)
            self.assertNotIn("original_matching", row)
            self.assertEqual(row["hardware_validation"], "not_performed")
            expected = "rtl8814au" if row["vid_pid"] == "20f4:809b" else "rtl8192cu"
            self.assertEqual(row["candidate_backend_family_from_personality"], expected)

    def test_all_original_identity_constraints_are_pinned(self):
        for alteration in ("remove", "revision", "interface", "origin"):
            data = copy.deepcopy(self.data)
            row = next(r for r in data["devices"] if r["vid_pid"] == "0bda:8176")
            if alteration == "remove": data["devices"].remove(row)
            elif alteration == "revision": row["original_matching"].pop("bcdDevice")
            elif alteration == "interface": row["original_matching"]["bInterfaceNumber"] = 2
            else: row["origin"] = "upstream_extension"
            self.rejected(data)

    def test_extension_requires_exact_pinned_family_evidence(self):
        for alteration in ("missing", "wrong_id", "floating", "wrong_family", "duplicate", "fake_original"):
            data = copy.deepcopy(self.data)
            row = next(r for r in data["devices"] if r["vid_pid"] == "2357:0100")
            evidence = row["primary_usb_table_evidence"][0]
            if alteration == "missing": row["primary_usb_table_evidence"] = []
            elif alteration == "wrong_id": evidence["usb_device_arguments"] = "0x038b, 0x0100"
            elif alteration == "floating": evidence["commit"] = "main"
            elif alteration == "wrong_family": evidence["driver_info"] = "rtw8812a_hw_spec"
            elif alteration == "duplicate": data["devices"].append(copy.deepcopy(row))
            else: row["original_matching"] = copy.deepcopy(row["matching"])
            self.rejected(data)

    def test_interface_fields_are_not_coerced_or_silently_ignored(self):
        for field in ("bInterfaceClass", "bInterfaceSubClass", "bInterfaceProtocol"):
            for value in (-1, 256, True, "255", None):
                data = copy.deepcopy(self.data)
                row = next(r for r in data["devices"] if r["vid_pid"] == "20f4:809b")
                row["interface_matching"][field] = value
                self.rejected(data)
        data = copy.deepcopy(self.data)
        row = next(r for r in data["devices"] if r["vid_pid"] == "2357:0100")
        row["interface_matching"]["unrecognized"] = 255
        self.rejected(data)

    def test_extensions_cannot_claim_unrelated_composite_usb_interfaces(self):
        for identity, action in (
            ("2357:0100", "remove_class"),
            ("2357:0100", "mass_storage_class"),
            ("20f4:809b", "remove_subclass"),
            ("20f4:809b", "wrong_protocol"),
            ("20f4:809b", "malformed_source_descriptor"),
        ):
            data = copy.deepcopy(self.data)
            row = next(r for r in data["devices"] if r["vid_pid"] == identity)
            constraints = row["interface_matching"]
            if action == "remove_class":
                del constraints["bInterfaceClass"]
            elif action == "mass_storage_class":
                constraints["bInterfaceClass"] = 8
            elif action == "remove_subclass":
                del constraints["bInterfaceSubClass"]
            elif action == "wrong_protocol":
                constraints["bInterfaceProtocol"] = 0
            else:
                row["primary_usb_table_evidence"][0]["interface_match_arguments"] = "0xff, malformed, 0xff"
            self.rejected(data)

    def test_au_match_is_vendor_specific_and_legacy_typos_stay_disabled(self):
        plist = plistlib.loads((ROOT / "kext/Info.plist").read_bytes())
        personalities = plist["IOKitPersonalities"].values()
        for entry in personalities:
            if entry["idVendor"] == 0x20f4 and entry["idProduct"] == 0x809b:
                for field in ("bInterfaceClass", "bInterfaceSubClass", "bInterfaceProtocol"):
                    self.assertEqual(entry[field], 255)
        pairs = {(e["idVendor"], e["idProduct"]) for e in personalities}
        self.assertIn((0x2357, 0x0100), pairs)
        self.assertIn((0x0eb0, 0x9071), pairs)
        self.assertNotIn((0x038b, 0x0100), pairs)
        self.assertNotIn((0x0e0b, 0x9071), pairs)
        self.assertNotIn((0x0bda, 0x8192), pairs)

    def test_runtime_and_observer_use_full_descriptor_api(self):
        driver = (ROOT / "kext/RTWUSBController.cpp").read_text()
        app = (ROOT / "app/AdapterSupport.hpp").read_text()
        self.assertIn("profile_=rtw_usb_wifi_lookup_descriptor(", driver)
        for field in ("i->bInterfaceClass", "i->bInterfaceSubClass", "i->bInterfaceProtocol"):
            self.assertIn(field, driver)
        self.assertIn("rtw_usb_wifi_lookup_descriptor(", app)
        self.assertIn("if (incomplete) { missing = true; continue; }", app)
        self.assertIn("RTW_USB_MATCH_PROTOCOL", (ROOT / "src/USBWiFiProfiles.c").read_text())

if __name__ == "__main__":
    unittest.main()
