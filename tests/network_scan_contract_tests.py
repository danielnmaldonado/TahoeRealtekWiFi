#!/usr/bin/env python3
"""Check the macOS-only scan's security wiring without claiming a native build."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]

class NetworkScanContractTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "kext/RTWUSBController.cpp").read_text()
        cls.body = source.split("void RTWUSBController::scanResults(", 1)[1].split(
            "IOReturn RTWUSBController::handleControl(", 1)[0]
        cls.node_header = (ROOT / "vendor/itl80211/openbsd/net80211/ieee80211_node.h").read_text()

    def test_security_fields_exist_in_vendor_node(self):
        for field in ("ni_capinfo", "ni_rsnprotos", "ni_rsnakms",
                      "ni_rsnciphers", "ni_rsngroupcipher", "ni_rsncaps"):
            self.assertIn(field, self.node_header)

    def test_group_cipher_is_not_pairwise_cipher(self):
        self.assertIn("node->ni_rsngroupcipher==IEEE80211_CIPHER_CCMP", self.body)
        self.assertNotIn("node->ni_rsncipher==IEEE80211_CIPHER_CCMP", self.body)

    def test_policy_is_fail_closed(self):
        self.assertIn("rtw_network_security_classify(&capabilities)", self.body)
        self.assertIn("RTW_CONTROL_SECURITY_UNSUPPORTED", self.body)
        self.assertIn("IEEE80211_RSNCAP_MFPR", self.body)
        self.assertIn("IEEE80211_PROTO_RSN", self.body)
        self.assertIn("IEEE80211_AKM_PSK", self.body)

if __name__ == "__main__":
    unittest.main()
