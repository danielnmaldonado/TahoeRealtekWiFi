#!/usr/bin/env python3
"""Fail-closed signing regression tests using recorded tools, not real codesign."""
from pathlib import Path
import json
import os
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
FAKE_TOOL = """
import json, os, sys
args=sys.argv[1:]
with open(os.environ["TEST_EVENTS"],"a") as stream:
    stream.write(json.dumps(args)+"\\n")
if os.environ.get("FAIL_MODE") in args: sys.exit(42)
if "-d" in args:
    print(os.environ.get("SIGNATURE","Signature=adhoc"),file=sys.stderr)
"""

class SigningPolicyTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="rtw-signing-")
        self.addCleanup(self.temporary.cleanup)
        self.folder = Path(self.temporary.name)
        self.events = self.folder / "events.jsonl"
        self.tool = self.folder / "codesign"
        self.tool.write_text("#!" + sys.executable + " -S\n" + FAKE_TOOL)
        self.tool.chmod(0o700)
        self.policy = self.folder / "signing.sh"
        self.policy.write_text((ROOT / "app/signing.sh").read_text().replace(
            "/usr/bin/codesign",str(self.tool)))
        self.env = dict(os.environ, TEST_EVENTS=str(self.events), RTW_SIGN_IDENTITY="-")
        for name in ("RTW_NOTARY_PROFILE","FAIL_MODE","SIGNATURE"):
            self.env.pop(name,None)

    def run_policy(self, expression, **environment):
        env = dict(self.env, **environment)
        return subprocess.run(["bash","-c",'source "$1"; '+expression,
                               "test",str(self.policy)],env=env,text=True,
                              capture_output=True,timeout=20)

    def calls(self):
        return [json.loads(line) for line in self.events.read_text().splitlines()] if self.events.exists() else []

    def test_default_policy(self):
        self.assertEqual(self.run_policy("rtw_require_adhoc_identity").returncode,0)
        self.assertFalse(self.calls())

    def test_rejects_certificate_identity(self):
        self.assertNotEqual(self.run_policy("rtw_sign_app '/tmp/App with spaces.app'",
            RTW_SIGN_IDENTITY="Developer ID Application: Test").returncode,0)
        self.assertFalse(self.calls())

    def test_rejects_notarization_profile(self):
        self.assertNotEqual(self.run_policy("rtw_sign_app '/tmp/App.app'",
            RTW_NOTARY_PROFILE="profile").returncode,0)
        self.assertFalse(self.calls())

    def test_inside_out_signing_and_recursive_verification(self):
        self.assertEqual(self.run_policy("rtw_sign_app '/tmp/App with spaces.app'").returncode,0)
        calls = self.calls()
        signs = [args for args in calls if "--sign" in args]
        self.assertEqual(len(signs),2)
        self.assertTrue(signs[0][-1].endswith("/Contents/Helpers/TahoeWiFiHelper"))
        self.assertEqual(signs[1][-1],"/tmp/App with spaces.app")
        for args in signs:
            self.assertEqual(args[args.index("--sign")+1],"-")
            self.assertIn("--timestamp=none",args)
            self.assertNotIn("--deep",args)
        self.assertEqual(calls[-1],["--verify","--deep","--strict","/tmp/App with spaces.app"])
        self.assertEqual(sum("-d" in args for args in calls),3)

    def test_sign_failure_stops_before_outer_signature(self):
        self.assertNotEqual(self.run_policy("rtw_sign_app '/tmp/App.app'",
                                           FAIL_MODE="--sign").returncode,0)
        self.assertEqual(len(self.calls()),1)

    def test_verify_failure_is_fatal(self):
        self.assertNotEqual(self.run_policy("rtw_sign_app '/tmp/App.app'",
                                           FAIL_MODE="--verify").returncode,0)
        self.assertEqual(len(self.calls()),3)

    def test_missing_or_certificate_metadata_is_rejected(self):
        for signature in ("","Authority=Developer ID Application: Test",
                          "Signature=adhoc-invalid","prefix Signature=adhoc"):
            with self.subTest(signature=signature):
                self.assertNotEqual(self.run_policy("rtw_verify_adhoc '/tmp/App.app'",
                                                    SIGNATURE=signature).returncode,0)

    def test_display_failure_is_fatal(self):
        self.assertNotEqual(self.run_policy("rtw_verify_adhoc '/tmp/App.app'",
                                           FAIL_MODE="-d").returncode,0)

    def test_kext_signed_before_verified(self):
        self.assertEqual(self.run_policy("rtw_sign_kext '/tmp/Driver.kext'").returncode,0)
        calls=self.calls()
        self.assertEqual(calls[0],["--force","--sign","-","--timestamp=none","/tmp/Driver.kext"])
        self.assertIn("--verify",calls[1])
        self.assertIn("-d",calls[2])

    def test_build_and_package_use_one_policy(self):
        for relative in ("build.command","app/build.command","app/package.command"):
            with self.subTest(relative=relative):
                source=(ROOT/relative).read_text()
                self.assertIn("signing.sh",source)
                self.assertIn("rtw_require_adhoc_identity",source)
        self.assertIn('rtw_verify_app "$APP"',(ROOT/"app/pkg/scripts/postinstall").read_text())
        self.assertIn('signing.sh',(ROOT/"app/pkg/scripts/common.sh").read_text())
        self.assertIn('rtw_sign_kext "$temporary"',(ROOT/"build.command").read_text())
        generator=(ROOT/"scripts/generate-kext-project.py").read_text()
        project=(ROOT/"TahoeRealtekWiFi.xcodeproj/project.pbxproj").read_text()
        self.assertIn('rtw_sign_kext "$TARGET_BUILD_DIR/$FULL_PRODUCT_NAME"',generator)
        self.assertIn('name="Require ad-hoc signature"',project)
        self.assertIn('app/signing.sh',project)

    def test_shell_syntax_for_all_entry_points(self):
        paths=[p for p in ROOT.rglob("*") if p.is_file() and (
            p.suffix in {".sh",".command"} or p.name in {"preinstall","postinstall"})]
        self.assertGreater(len(paths),10)
        for path in paths:
            with self.subTest(path=str(path.relative_to(ROOT))):
                result=subprocess.run(["bash","-n",str(path)],text=True,
                                      capture_output=True,timeout=10)
                self.assertEqual(result.returncode,0,result.stderr)

if __name__=="__main__":
    unittest.main(verbosity=2)
