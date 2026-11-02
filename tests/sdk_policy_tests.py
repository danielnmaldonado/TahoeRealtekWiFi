#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Exercise SDK reuse/bootstrap against real disposable Git and archive inputs."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PAYLOAD = {
    "Headers/IOKit/usb/IOUSBHostInterface.h": b"USB header fixture\n",
    "Headers/IOKit/network/IOEthernetController.h": b"Ethernet header fixture\n",
    "Library/x86_64/libkmod.a": b"Kernel library fixture\n",
}


class SDKPolicyTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="rtw-sdk-policy-")
        self.addCleanup(temporary.cleanup)
        self.folder = Path(temporary.name)
        self.sdk = self.folder / "MacKernelSDK"
        for relative, data in PAYLOAD.items():
            path = self.sdk / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
        self.git("init", "-q")
        self.git("add", ".")
        self.git("-c", "user.name=SDK Policy Test", "-c",
                 "user.email=sdk-policy@example.invalid", "commit", "-qm", "pinned fixture")
        self.revision = self.git("rev-parse", "HEAD").stdout.strip()
        scripts = self.folder / "scripts"
        scripts.mkdir()
        checker = ROOT / "scripts/check-mackernelsdk.py"
        if checker.exists():
            shutil.copyfile(checker, scripts / checker.name)
        (scripts / "MacKernelSDK-SHA256.json").write_text(json.dumps({
            "repository": "https://github.com/acidanthera/MacKernelSDK",
            "revision": self.revision,
            "files": {name: hashlib.sha256(data).hexdigest()
                      for name, data in PAYLOAD.items()},
        }))
        source = (ROOT / "build.command").read_text()
        failure = source[source.index("fail() {"):source.index("\n# Rodar inclusive")]
        prepare = source[source.index("prepare_sdk() {"):source.index("\npreflight_kext() {")]
        self.runner = self.folder / "prepare.sh"
        self.runner.write_text('set -euo pipefail\nROOT="$1"\nSDK="$ROOT/MacKernelSDK"\n'
                               'SDK_REV="$2"\n' + failure + "\n" + prepare + "\nprepare_sdk\n")

    def git(self, *args):
        return subprocess.run(["git", "-C", str(self.sdk), *args], text=True,
                              capture_output=True, check=True, timeout=15)

    def prepare(self, **overrides):
        environment = dict(os.environ, RTW_OFFLINE="1")
        environment.update(overrides)
        return subprocess.run(["bash", str(self.runner), str(self.folder), self.revision],
                              text=True, capture_output=True, env=environment, timeout=20)

    def archive(self):
        shutil.rmtree(self.sdk / ".git")

    def test_pinned_git_checkout_is_reused_offline(self):
        result = self.prepare()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(self.git("status", "--porcelain").stdout)

    def test_wrong_git_revision_is_rejected_without_reset(self):
        (self.sdk / "README.md").write_text("A different upstream revision\n")
        self.git("add", ".")
        self.git("-c", "user.name=SDK Policy Test", "-c",
                 "user.email=sdk-policy@example.invalid", "commit", "-qm", "other revision")
        head = self.git("rev-parse", "HEAD").stdout
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(self.git("rev-parse", "HEAD").stdout, head)

    def test_modified_git_header_is_rejected_without_overwrite(self):
        header = self.sdk / next(iter(PAYLOAD))
        header.write_bytes(b"Local modified header\n")
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0, result.stdout)
        self.assertEqual(header.read_bytes(), b"Local modified header\n")

    def test_exact_archive_is_reused_without_git_metadata(self):
        self.archive()
        result = self.prepare()
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse((self.sdk / ".git").exists())

    def test_modified_archive_library_is_rejected(self):
        self.archive()
        (self.sdk / "Library/x86_64/libkmod.a").write_bytes(b"A different library\n")
        self.assertNotEqual(self.prepare().returncode, 0)

    def test_extra_archive_header_is_rejected(self):
        self.archive()
        (self.sdk / "Headers/Unexpected.h").write_bytes(b"An unpinned header\n")
        self.assertNotEqual(self.prepare().returncode, 0)

    def test_symlinked_archive_header_is_rejected(self):
        self.archive()
        header = self.sdk / next(iter(PAYLOAD))
        external = self.folder / "external-header"
        external.write_bytes(header.read_bytes())
        header.unlink()
        header.symlink_to(external)
        self.assertNotEqual(self.prepare().returncode, 0)

    def test_incomplete_sdk_is_preserved(self):
        library = self.sdk / "Library/x86_64/libkmod.a"
        library.unlink()
        result = self.prepare()
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse(library.exists())
        self.assertTrue(self.sdk.is_dir())

    def test_missing_sdk_offline_stays_missing(self):
        shutil.rmtree(self.sdk)
        self.assertNotEqual(self.prepare().returncode, 0)
        self.assertFalse(self.sdk.exists())

    def test_missing_sdk_can_bootstrap_from_fixed_revision(self):
        upstream = self.folder / "upstream"
        self.sdk.rename(upstream)
        tools = self.folder / "tools"
        tools.mkdir()
        # Route only the external remote to a local real repository; fetch,
        # checkout, content validation and publication run unchanged.
        git = tools / "git"
        git.write_text("#!" + sys.executable + "\nimport os, sys\n"
                       "args = [os.environ['SDK_TEST_UPSTREAM'] if value == "
                       "'https://github.com/acidanthera/MacKernelSDK.git' else value "
                       "for value in sys.argv[1:]]\n"
                       "os.execv(os.environ['SDK_TEST_GIT'], ['git'] + args)\n")
        git.chmod(0o700)
        result = self.prepare(RTW_OFFLINE="0", SDK_TEST_GIT=shutil.which("git"),
                              SDK_TEST_UPSTREAM=str(upstream),
                              PATH=str(tools) + os.pathsep + os.environ["PATH"])
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.git("rev-parse", "HEAD").stdout.strip(), self.revision)
        for relative, data in PAYLOAD.items():
            self.assertEqual((self.sdk / relative).read_bytes(), data)


if __name__ == "__main__":
    unittest.main(verbosity=2)
