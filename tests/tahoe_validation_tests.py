#!/usr/bin/env python3
"""Controlled fixtures only: never run system diagnostics or contact a driver."""
import importlib.util
import pathlib
import plistlib
import struct
import sys
import tempfile
import unittest

MODULE_PATH = pathlib.Path(__file__).resolve().parents[1] / "tools/rtw-tahoe-validate.py"
spec = importlib.util.spec_from_file_location("rtw_tahoe_validate", MODULE_PATH)
validator = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = validator
spec.loader.exec_module(validator)

BUNDLE = "org.research.TahoeRealtekWiFi"
UUID = "00112233-4455-6677-8899-AABBCCDDEEFF"
HEADER = "Index Refs Address Size Wired Architecture Name (Version) UUID <Linked Against>\n"
ROW = f"123 0 0xffffff7f12340000 0x10000 0x10000 x86_64 {BUNDLE} (11) {UUID} <1 2>\n"
ATTACHED = f'''+-o controller  <class RTWUSBController, id 0x123, registered, matched, active, busy 0, retain 5>
  {{
    "CFBundleIdentifier" = "{BUNDLE}"
  }}
'''
LIBRARIES = plistlib.dumps({"OSBundleLibraries": {"com.apple.iokit.IOUSBHostFamily": "1.2"}}).decode()


def macho(signature=False):
    commands = struct.pack("<II", 0x1B, 24) + bytes.fromhex(UUID.replace("-", ""))
    if signature:
        commands += struct.pack("<IIII", 0x1D, 16, 72, 8)
    header = struct.pack("<IIIIIIII", 0xFEEDFACF, 0x01000007, 3, 11,
                         2 if signature else 1, len(commands), 0, 0)
    return header + commands + (b"\xfa\xde\x0c\xc0\x00\x00\x00\x08" if signature else b"")


class FixtureRunner:
    def __init__(self, overrides=None):
        self.calls = []
        self.overrides = overrides or {}

    def __call__(self, argv):
        self.calls.append(argv)
        executable = pathlib.Path(argv[0]).name
        key = argv[1] if executable == "kmutil" else executable
        default = {
            "plutil": (0, "Info.plist: OK\n", ""),
            "file": (0, "Mach-O 64-bit kext bundle x86_64\n", ""),
            "libraries": (0, "No variant specified, falling back to release\n" + LIBRARIES, ""),
            "print-diagnostics": (0, "Diagnosing sample.kext...\n[org.research.TahoeRealtekWiFi]\n"
                "[v] Flags: validated, authenticated, approvalChecked\n", ""),
            "showloaded": (0, HEADER, ""),
            "ioreg": (0, "", ""),
        }
        rc, out, err = self.overrides.get(key, default[key])
        return validator.CommandResult(argv, rc, out, err)


class ValidationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.bundle = pathlib.Path(self.temporary.name) / "TahoeRealtekWiFi.kext"
        contents = self.bundle / "Contents"
        (contents / "MacOS").mkdir(parents=True)
        (contents / "Info.plist").write_bytes(plistlib.dumps({
            "CFBundleIdentifier": BUNDLE, "CFBundlePackageType": "KEXT",
            "CFBundleVersion": "11", "CFBundleExecutable": "TahoeRealtekWiFi",
            "OSBundleLibraries": {"com.apple.iokit.IOUSBHostFamily": "1.0.1"},
        }))
        (contents / "MacOS/TahoeRealtekWiFi").write_bytes(macho())

    def audit(self, overrides=None):
        runner = FixtureRunner(overrides)
        return validator.validate(self.bundle, runner=runner, architecture="x86_64"), runner

    # Catches trusting kmutil's zero exit and its optimistic Flags line.
    def test_diagnostics_zero_exit_with_real_errors_fails(self):
        diagnostic = """[v] Flags: validated, authenticated, approvalChecked
Error: Authentication failed.
Error: Validation failed.
Error:
  Filesystem error: Invalid ownership (501:20) should be (0:0)
  Authenticating extension failed: Bad code signature
Error: Unable to resolve dependencies: names a dependency on IOUSBHostFamily, but could not find version compatible with '1.0'.
"""
        report, _ = self.audit({"print-diagnostics": (0, diagnostic, "")})
        check = report["checks"]["diagnostics"]
        self.assertEqual(check["status"], "failed")
        self.assertTrue({"ownership", "signature", "dependencies"}.issubset(check["categories"]))
        self.assertEqual(report["exit_code"], 1)

    def test_errors_on_stderr_are_not_ignored(self):
        report, _ = self.audit({"print-diagnostics": (0, "Diagnosing...\n", "Error: Validation failed.\n")})
        self.assertEqual(report["checks"]["diagnostics"]["status"], "failed")

    def test_empty_diagnostic_with_zero_exit_is_not_success(self):
        report, _ = self.audit({"print-diagnostics": (0, "No variant specified, falling back to release\n", "")})
        self.assertEqual(report["checks"]["diagnostics"]["status"], "error")

    def test_libraries_zero_exit_with_missing_symbols_fails(self):
        report, _ = self.audit({"libraries": (0, "Undefined symbols for architecture x86_64:\n _missing\n", "")})
        self.assertEqual(report["checks"]["libraries"]["status"], "failed")
        self.assertEqual(report["exit_code"], 1)

    def test_libraries_missing_or_wrong_shape_xml_is_a_tool_error(self):
        for output in ("", plistlib.dumps(["not a dependency dictionary"]).decode()):
            with self.subTest(output=output):
                report, _ = self.audit({"libraries": (0, output, "")})
                self.assertEqual(report["checks"]["libraries"]["status"], "error")

    # Catches relying on transitive dependency resolution to cover an omitted
    # direct symbol provider in the kext's own OSBundleLibraries declaration.
    def test_direct_symbol_provider_must_be_declared_even_when_dependencies_ok(self):
        providers = plistlib.dumps({"OSBundleLibraries": {
            "com.apple.iokit.IOUSBHostFamily": "1.2", "com.apple.kpi.mach": "25.6.0"}}).decode()
        diagnostic = "Diagnosing sample.kext...\nDependencies: OK\n"
        report, _ = self.audit({"libraries": (0, providers, ""),
                               "print-diagnostics": (0, diagnostic, "")})
        self.assertEqual(report["checks"]["diagnostics"]["status"], "passed")
        self.assertEqual(report["checks"]["provider_declarations"]["status"], "failed")
        self.assertEqual(report["checks"]["provider_declarations"]["missing_provider_ids"],
                         ["com.apple.kpi.mach"])
        self.assertEqual(report["exit_code"], 1)

    def test_provider_version_differences_are_not_missing_declarations(self):
        report, _ = self.audit()
        # The fixture deliberately declares IOUSBHostFamily1.0.1 while the
        # current provider version from libraries is1.2.
        self.assertEqual(report["checks"]["provider_declarations"]["status"], "passed")
        self.assertEqual(report["checks"]["provider_declarations"]["missing_provider_ids"], [])

    def test_transitive_diagnostics_list_does_not_create_direct_provider_requirement(self):
        diagnostic = ('Diagnosing sample.kext...\n'
                      '["com.apple.kpi.unsupported", "com.apple.kpi.private"]\nDependencies: OK\n')
        report, _ = self.audit({"print-diagnostics": (0, diagnostic, "")})
        self.assertEqual(report["checks"]["provider_declarations"]["status"], "passed")

    def test_static_success_without_load_is_runtime_not_validated(self):
        report, runner = self.audit()
        self.assertTrue(report["static_validation_passed"])
        self.assertEqual(report["runtime"]["loaded"]["status"], "not_loaded")
        self.assertEqual(report["runtime"]["attached"]["status"], "not_attached")
        self.assertEqual(report["status"], "runtime_not_validated")
        self.assertEqual(report["exit_code"], 2)
        self.assertEqual(report["live_connectivity"], "not_tested")
        self.assertEqual([pathlib.Path(c[0]).name for c in runner.calls],
                         ["plutil", "file", "kmutil", "kmutil", "kmutil", "ioreg"])
        for call in runner.calls:
            self.assertNotIn("--do-staging", call)
            self.assertNotIn("load", call)

    def test_other_bundle_or_prefixed_identifier_is_not_loaded_evidence(self):
        for other in (BUNDLE + ".shadow", "com.example.other"):
            with self.subTest(other=other):
                report, _ = self.audit({"showloaded": (0, HEADER + ROW.replace(BUNDLE, other), "")})
                self.assertEqual(report["runtime"]["loaded"]["status"], "not_loaded")

    def test_loaded_is_not_attached(self):
        report, _ = self.audit({"showloaded": (0, HEADER + ROW, "")})
        self.assertEqual(report["runtime"]["loaded"]["status"], "loaded")
        self.assertEqual(report["runtime"]["attached"]["status"], "not_attached")
        self.assertEqual(report["exit_code"], 2)

    def test_list_only_without_header_is_a_complete_inventory(self):
        report, _ = self.audit({"showloaded": (0, ROW.replace(BUNDLE,"com.example.other"), "")})
        self.assertEqual(report["runtime"]["loaded"]["status"], "not_loaded")

    def test_class_property_or_inactive_service_is_not_attachment(self):
        for output in ('"IOClass" = "RTWUSBController"\n',
                       ATTACHED.replace("active,", "inactive,"),
                       ATTACHED.replace(BUNDLE, "com.example.other"),
                       ATTACHED.replace("class RTWUSBController,", "class RTWUSBControllerFake,")):
            with self.subTest(output=output):
                report, _ = self.audit({"ioreg": (0, output, "")})
                self.assertEqual(report["runtime"]["attached"]["status"], "not_attached")

    def test_observed_attachment_never_claims_live_connection(self):
        report, _ = self.audit({"showloaded": (0, HEADER + ROW, ""), "ioreg": (0, ATTACHED, "")})
        self.assertEqual(report["exit_code"], 0)
        self.assertEqual(report["status"], "kernel_attachment_observed")
        self.assertEqual(report["live_connectivity"], "not_tested")

    def test_loaded_old_uuid_does_not_validate_new_artifact(self):
        old = ROW.replace(UUID, "AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE")
        report, _ = self.audit({"showloaded": (0, HEADER + old, ""), "ioreg": (0, ATTACHED, "")})
        self.assertEqual(report["runtime"]["loaded"]["artifact_match"], "mismatch")
        self.assertEqual(report["exit_code"], 2)

    def test_unsupported_mode_with_zero_exit_is_tool_error(self):
        report, _ = self.audit({"print-diagnostics": (0, "kextutil: -n is not a supported kmutil mode\n", "")})
        self.assertEqual(report["checks"]["diagnostics"]["status"], "error")
        self.assertEqual(report["exit_code"], 3)

    def test_signalled_showloaded_is_unknown_not_not_loaded(self):
        report, _ = self.audit({"showloaded": (-11, "", "No variant specified, falling back to release\n")})
        self.assertEqual(report["runtime"]["loaded"]["status"], "unknown")
        self.assertEqual(report["exit_code"], 3)

    def test_empty_showloaded_with_zero_exit_is_unknown(self):
        report, _ = self.audit({"showloaded": (0, "No variant specified, falling back to release\n", "")})
        self.assertEqual(report["runtime"]["loaded"]["status"], "unknown")

    def test_plist_non_dictionary_is_reported_without_a_traceback(self):
        (self.bundle / "Contents/Info.plist").write_bytes(plistlib.dumps(["invalid"]))
        report, _ = self.audit()
        self.assertEqual(report["checks"]["plist"]["status"], "failed")

    def test_signature_metadata_is_not_cryptographic_verification(self):
        unsigned = validator.inspect_macho(macho(), "x86_64")
        signed = validator.inspect_macho(macho(True), "x86_64")
        self.assertEqual(unsigned["signature"]["status"], "unsigned")
        self.assertEqual(signed["signature"]["status"], "present_unverified")
        self.assertFalse(signed["signature"]["verified"])

    def test_malformed_load_command_fails_structural_validation(self):
        malformed = bytearray(macho())
        struct.pack_into("<I", malformed, 36, 0xffffffff)
        with self.assertRaises(ValueError):
            validator.inspect_macho(bytes(malformed), "x86_64")

    def test_evidence_directory_cannot_modify_the_kext(self):
        with self.assertRaises(ValueError):
            validator.prepare_log_directory(self.bundle / "logs", self.bundle)
        self.assertFalse((self.bundle / "logs").exists())


if __name__ == "__main__":
    unittest.main()
