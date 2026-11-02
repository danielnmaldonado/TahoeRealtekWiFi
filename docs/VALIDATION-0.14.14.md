# Validation — driver0.14.14/build61, app0.3.10/build52

Project/macOS adaptation: MaLd0n. Development.

The delivery contains corresponding kext, app, helper, test,
generator, reference and license source. PERFORMANCE-0.14.14.md describes implemented
modes and their limits. ADAPTER-TESTS-20261101.md retains analysis of the
14 historical logs captured with build55 on macOS12.7.6.

VERIFICATION.json records exact results for host and sanitizer tests,
ZIP reconstruction, native app suites, collector and native products. Ad-hoc
signatures validate local bundle integrity; they do not equate to
notarization or certification of universal operation.

There was no new app installation, kext loading, EFI,
router, macOS security, credential change or reboot in this round. There is no
hardware speed benchmark for this build. New HT/VHT modes and the power
fix require hardware regression on the corresponding adapters/revisions.

Zero native RX accounting remains under investigation. The exact cause of
2357:0120 failure and omission of 2.4 GHz was not demonstrated by the old archive;
new boot/power fields remain available for the next capture.
Edimax 7392:a611 needs discovery confirmation in its actual topology with the
previous fix. Certification of the 14 cases or every mode is not claimed.
