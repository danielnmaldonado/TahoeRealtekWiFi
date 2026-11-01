#!/usr/bin/env python3
"""Guard the USB interface matching combination required by IOUSBHostInterface."""
from pathlib import Path
import plistlib
import unittest

class PersonalityTests(unittest.TestCase):
    def test_target_interface_match_is_complete(self):
        info = plistlib.loads((Path(__file__).resolve().parents[1] / 'kext/Info.plist').read_bytes())
        matches = [p for p in info['IOKitPersonalities'].values()
                   if p.get('idVendor') == 0x2357 and p.get('idProduct') == 0x0108]
        self.assertEqual(len(matches), 1)
        p = matches[0]
        self.assertEqual(p['IOProviderClass'], 'IOUSBHostInterface')
        self.assertEqual(p.get('bConfigurationValue'), 1,
                         'VID/PID interface match also requires bConfigurationValue')
        self.assertEqual(p.get('bInterfaceNumber'), 0,
                         'VID/PID interface match also requires bInterfaceNumber')
        self.assertEqual(p['CFBundleIdentifier'], info['CFBundleIdentifier'])

if __name__ == '__main__':
    unittest.main()
