#!/usr/bin/env python3
"""Package regression tests.

The shell hooks and builder run in disposable copies. macOS commands are
substituted with recorded test doubles; no real .pkg, service or installation
is created. These tests do not replace a native Installer test on macOS.
"""
from __future__ import annotations

import json
import os
from pathlib import Path
import plistlib
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]


def macho_fixture(minos=0x000D0000, *, command=0x32, platform=1,
                  cpu=0x01000007, filetype=2):
    """Hand-assembled 64-bit Mach-O; no SDK or parser under test creates it."""
    load = (struct.pack("<6I", command, 24, platform, minos, 0x001A0500, 0)
            if command == 0x32 else struct.pack("<4I", command, 16, minos, 0x001A0500))
    return struct.pack("<8I", 0xFEEDFACF, cpu, 3, filetype, 1, len(load), 0, 0) + load


def fat_fixture(slice_bytes):
    return (struct.pack(">7I", 0xCAFEBABE, 1, 0x01000007, 3, 32, len(slice_bytes), 2)
            + bytes(4) + slice_bytes)

TOOLS = {
    "/usr/bin/id", "/usr/bin/uname", "/usr/bin/sw_vers", "/usr/sbin/sysctl",
    "/usr/bin/stat", "/bin/launchctl", "/usr/bin/sudo", "/usr/bin/open",
    "/usr/libexec/PlistBuddy", "/usr/bin/codesign", "/usr/bin/plutil",
    "/usr/bin/pgrep", "/usr/bin/find", "/bin/sleep", "/usr/bin/pkgbuild",
    "/usr/bin/productbuild", "/usr/sbin/pkgutil", "/usr/bin/ditto",
    "/usr/bin/lipo", "/bin/chmod", "/usr/bin/xcrun",
}
FAKE_TOOL = r"""
import json, os, pathlib, plistlib, shutil, subprocess, sys
tool = pathlib.Path(sys.argv[0]).name
args = sys.argv[1:]
def flag(name, default=""):
    return os.environ.get("PKG_TEST_" + name, default)
def record():
    with open(flag("EVENTS"), "a", encoding="utf-8") as stream:
        stream.write(json.dumps({"tool":tool, "args":args}) + "\n")
record()
if tool == "id":
    print(flag("EUID", "0") if args == ["-u"] else "testuser")
elif tool == "uname":
    print(flag("OS", "Darwin") if args == ["-s"] else flag("ARCH", "x86_64"))
elif tool == "sw_vers":
    print(flag("OS_VERSION", "26.0"))
elif tool == "sysctl":
    print(flag("ARM", "0"))
elif tool == "stat":
    print(flag("CONSOLE_UID", "501"))
elif tool == "PlistBuddy":
    try:
        command = args[args.index("-c") + 1]
        data = plistlib.loads(pathlib.Path(args[-1]).read_bytes())
        for key in command.removeprefix("Print :").split(":"):
            data = data[int(key)] if isinstance(data, list) else data[key]
        print(str(data).lower() if isinstance(data, bool) else data)
    except (ValueError, KeyError, IndexError, OSError, plistlib.InvalidFileException):
        sys.exit(1)
elif tool == "plutil":
    for name in args:
        if not name.startswith("-"):
            try: plistlib.loads(pathlib.Path(name).read_bytes())
            except Exception: sys.exit(1)
elif tool == "codesign":
    if flag("BAD_SIGNATURE") == "1": sys.exit(1)
    if "-d" in args:
        if flag("DEVELOPER_ID") == "1":
            print("Authority=Developer ID Application: Test", file=sys.stderr)
        elif flag("MISSING_ADHOC") != "1":
            print("Signature=adhoc", file=sys.stderr)
elif tool == "pgrep":
    sys.exit(int(flag("PGREP_STATUS", "1")))
elif tool == "find":
    # Model root ownership with the current test UID; still use real find for
    # type, symlink, mode and traversal tests. Installed root ownership is native-only.
    if "-user" in args:
        if flag("BAD_OWNER") == "1":
            print(args[0])
            sys.exit(0)
        args[args.index("-user") + 1] = str(os.geteuid())
    sys.exit(subprocess.call([flag("REAL_FIND")] + args))
elif tool == "launchctl":
    if args[:1] == ["print"]:
        if args[1].startswith("gui/"):
            sys.exit(0 if flag("GUI", "1") == "1" else 1)
        sys.exit(0 if pathlib.Path(flag("SERVICE")).exists() else 1)
    if args[:1] == ["asuser"]:
        if int(args[1]) < 501: sys.exit(90)
        sys.exit(subprocess.call(args[2:]))
    sys.exit(91)
elif tool == "sudo":
    if args[:3] != ["-H", "-u", "#501"]: sys.exit(92)
    if args[-1:] == ["--prepare-update"]:
        sys.exit(subprocess.call([flag("OLD_APP"), "--prepare-update"]))
    sys.exit(subprocess.call(args[4:]))
elif tool == "open":
    sys.exit(int(flag("OPEN_STATUS", "0")))
elif tool == "sleep":
    pass
elif tool == "chmod":
    if args[:1] != ["-RN"]:
        sys.exit(subprocess.call([flag("REAL_CHMOD")] + args))
elif tool == "ditto":
    shutil.copytree(args[-2], args[-1], copy_function=shutil.copy2)
elif tool == "lipo":
    print(flag("BINARY_ARCH", "x86_64"))
elif tool == "pkgbuild":
    if flag("FAIL_PKGBUILD") == "1": sys.exit(1)
    payload = pathlib.Path(args[args.index("--root") + 1])
    manifest = {"args":args, "payload":sorted(str(p.relative_to(payload))
                 for p in payload.rglob("*") if p.is_file())}
    pathlib.Path(args[-1]).write_text(json.dumps(manifest))
elif tool == "productbuild":
    if flag("FAIL_PRODUCTBUILD") == "1": sys.exit(1)
    stage = pathlib.Path(args[args.index("--package-path") + 1])
    component = json.loads((stage/"TahoeWiFi-component.pkg").read_text())
    pathlib.Path(args[-1]).write_text(json.dumps({"component":component, "args":args}))
elif tool == "pkgutil":
    if args[0] == "--expand":
        pathlib.Path(args[2]).mkdir()
        (pathlib.Path(args[2])/"Distribution").write_text("<installer-gui-script/>")
elif tool == "xcrun":
    if args[:2] == ["notarytool", "submit"]:
        sys.stdout.buffer.write(plistlib.dumps({"status":flag("NOTARY_STATUS", "Accepted")}))
else:
    sys.exit("Unexpected fake tool: " + tool)
"""
FAKE_APP = r"""
import json, os, pathlib, sys
with open(os.environ["PKG_TEST_EVENTS"], "a", encoding="utf-8") as stream:
    stream.write(json.dumps({"tool":"old-app", "args":sys.argv[1:]}) + "\n")
if sys.argv[1:] != ["--prepare-update"]:
    sys.exit(95)
if os.environ.get("PKG_TEST_PREPARE_FAIL") == "1":
    sys.exit(1)
if os.environ.get("PKG_TEST_KEEP_SERVICE") != "1":
    pathlib.Path(os.environ["PKG_TEST_SERVICE"]).unlink(missing_ok=True)
"""

class PackageContractTests(unittest.TestCase):
    def test_fixed_system_destination_and_intel(self):
        dist = ET.parse(ROOT/"app/pkg/Distribution.xml").getroot()
        self.assertEqual(dist.find("domains").attrib, {
            "enable_anywhere":"false", "enable_currentUserHome":"false",
            "enable_localSystem":"true"})
        self.assertEqual(dist.find("options").get("hostArchitectures"), "x86_64")
        self.assertEqual(dist.find("volume-check/allowed-os-versions/os-version").get("min"), "13.0")

    def test_bundle_cannot_be_relocated_or_merged(self):
        component = plistlib.loads((ROOT/"app/pkg/components.plist").read_bytes())[0]
        self.assertEqual(component["RootRelativeBundlePath"], "Applications/Tahoe Wi-Fi USB.app")
        self.assertFalse(component["BundleIsRelocatable"])
        self.assertTrue(component["BundleIsVersionChecked"])
        self.assertTrue(component["BundleHasStrictIdentifier"])
        self.assertEqual(component["BundleOverwriteAction"], "upgrade")

    def test_runtime_installation_is_guarded_and_deferred(self):
        main = (ROOT/"app/Main.mm").read_text()
        self.assertIn("pendingInstallation && RTWProtectedBundle", main)
        self.assertIn('boolForKey:@"RTWResumeServiceAfterInstall"', main)
        self.assertIn('boolForKey:@"RTWResumeLoginAfterInstall"', main)
        self.assertIn('boolForKey:@"RTWStartupChoiceMade"', main)

    def test_shell_syntax(self):
        files = [ROOT/"app/package.command", ROOT/"build-pkg.command",
                 ROOT/"app/build.command", ROOT/"build.command",
                 *sorted((ROOT/"app/pkg/scripts").iterdir())]
        for file in files:
            with self.subTest(file=file.name):
                result = subprocess.run(["bash","-n",str(file)],capture_output=True,text=True)
                self.assertEqual(result.returncode,0,result.stderr)

    def test_no_policy_or_keychain_deletion_in_hooks(self):
        text = "\n".join(p.read_text() for p in (ROOT/"app/pkg/scripts").iterdir())
        for command in ("security delete", "sfltool", "csrutil", "spctl",
                        "kextload", "kmutil", "defaults delete", "rm -rf"):
            with self.subTest(command=command):
                self.assertNotIn(command, text)
        self.assertIn("/usr/bin/sudo -H -u", text)

    def test_help_works_without_native_tools(self):
        result = subprocess.run(["bash",str(ROOT/"app/package.command"),"--help"],
                                capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertIn("--no-build", result.stdout)

class PackageScriptTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="tahoe-pkg-tests-")
        self.addCleanup(self.tmp.cleanup)
        self.folder = Path(self.tmp.name)
        self.project = self.folder/"project with spaces"
        self.appdir = self.project/"app"
        self.bindir = self.folder/"commands"
        self.bindir.mkdir()
        self.apps = self.folder/"Applications"
        self.apps.mkdir()
        self.bundle = self.apps/"Tahoe Wi-Fi USB.app"
        self.events = self.folder/"events.jsonl"
        self.service = self.folder/"service"
        old_app = self.folder/"old-app"
        old_app.write_text("#!"+sys.executable+" -S\n"+FAKE_APP)
        old_app.chmod(0o755)
        self.environment = os.environ.copy()
        self.environment.update({
            "PKG_TEST_EVENTS":str(self.events), "PKG_TEST_SERVICE":str(self.service),
            "PKG_TEST_OLD_APP":str(old_app),
            "PKG_TEST_REAL_FIND":shutil.which("find") or "/usr/bin/find",
            "PKG_TEST_REAL_CHMOD":shutil.which("chmod") or "/bin/chmod",
            "RTW_INSTALLER_SIGN_IDENTITY":"", "RTW_NOTARY_PROFILE":"", "RTW_SIGN_IDENTITY":"-",
        })
        self.toolmap = {}
        for tool in TOOLS:
            stub = self.bindir/Path(tool).name
            stub.write_text("#!"+sys.executable+" -S\n"+FAKE_TOOL)
            stub.chmod(0o755)
            self.toolmap[tool] = str(stub)
        shutil.copytree(ROOT/"app/pkg", self.appdir/"pkg")
        (self.appdir/"package.command").write_bytes((ROOT/"app/package.command").read_bytes())
        (self.appdir/"signing.sh").write_bytes((ROOT/"app/signing.sh").read_bytes())
        checker = ROOT/"app/check-macos-compatibility.py"
        if checker.exists():
            shutil.copyfile(checker, self.appdir/checker.name)
        (self.appdir/"pkg/scripts/signing.sh").write_bytes((ROOT/"app/signing.sh").read_bytes())
        # Rewrite ONLY disposable script copies, never the production scripts.
        for file in [self.appdir/"package.command", self.appdir/"signing.sh",
                     *(self.appdir/"pkg/scripts").iterdir()]:
            text = file.read_text()
            if file.name != "package.command":
                text = text.replace("/Applications", str(self.apps))
            for old,new in sorted(self.toolmap.items(),key=lambda pair: -len(pair[0])):
                text = text.replace(old,new)
            file.write_text(text)
        self.make_bundle()
        (self.appdir/"pkg/scripts/package-info.plist").write_bytes(
            (ROOT/"app/Info.plist").read_bytes())

    def make_bundle(self, version="0.2.7"):
        info = plistlib.loads((ROOT/"app/Info.plist").read_bytes())
        info["CFBundleShortVersionString"] = version
        required = ("Contents/MacOS/TahoeUSBWiFi",
                    "Contents/Helpers/TahoeWiFiHelper",
                    "Contents/_CodeSignature/CodeResources",
                    "Contents/Resources/AppIcon.icns",
                    "Contents/Info.plist",
                    "Contents/Library/LaunchDaemons/org.research.TahoeUSBWiFi.driver.plist")
        for name in required:
            path = self.bundle/name
            path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text("test fixture, not a macOS binary")
            path.chmod(0o644)
        (self.bundle/"Contents/Info.plist").write_bytes(plistlib.dumps(info))
        daemon = self.bundle/"Contents/Library/LaunchDaemons/org.research.TahoeUSBWiFi.driver.plist"
        daemon.write_bytes((ROOT/"app/org.research.TahoeUSBWiFi.driver.plist").read_bytes())
        exe = self.bundle/"Contents/MacOS/TahoeUSBWiFi"
        exe.write_bytes(macho_fixture())
        exe.chmod(0o755)
        helper = self.bundle/"Contents/Helpers/TahoeWiFiHelper"
        helper.write_bytes(macho_fixture())
        helper.chmod(0o755)
        for path in self.bundle.rglob("*"):
            if path.is_dir(): path.chmod(0o755)

    def run_hook(self, name, target="/", **environment):
        env = self.environment.copy()
        env.update({"PKG_TEST_"+key:str(value) for key,value in environment.items()})
        return subprocess.run(["bash",str(self.appdir/"pkg/scripts"/name),"test.pkg","/",target],
                              capture_output=True,text=True,env=env,timeout=20)

    def run_builder(self, **environment):
        env = self.environment.copy()
        env["PKG_TEST_EUID"]="501"
        env.update(environment)
        return subprocess.run(["bash",str(self.appdir/"package.command"),"--app",str(self.bundle),
                               "--output",str(self.folder/"output with spaces")],
                              capture_output=True,text=True,env=env,timeout=25)

    def history(self, tool=None):
        events = [json.loads(line) for line in self.events.read_text().splitlines()] if self.events.exists() else []
        return [event for event in events if event["tool"]==tool] if tool else events

    def test_preinstall_refuses_downgrade_before_changing_service(self):
        info_path=self.bundle/"Contents/Info.plist"
        info=plistlib.loads(info_path.read_bytes())
        info["CFBundleVersion"]=str(int(info["CFBundleVersion"])+1)
        info_path.write_bytes(plistlib.dumps(info))
        self.service.touch()
        result=self.run_hook("preinstall")
        self.assertNotEqual(result.returncode,0)
        self.assertTrue(self.service.exists())
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_repairs_incomplete_bundle_without_service(self):
        (self.bundle/"Contents/Helpers/TahoeWiFiHelper").unlink()
        result=self.run_hook("preinstall")
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_rejects_incomplete_bundle_with_active_service(self):
        (self.bundle/"Contents/Helpers/TahoeWiFiHelper").unlink()
        self.service.touch()
        result=self.run_hook("preinstall")
        self.assertNotEqual(result.returncode,0)
        self.assertTrue(self.service.exists())
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_fresh_install(self):
        shutil.rmtree(self.bundle)
        result=self.run_hook("preinstall")
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_rejects_running_app(self):
        result=self.run_hook("preinstall",PGREP_STATUS=0)
        self.assertNotEqual(result.returncode,0)
        self.assertIn("Quit",result.stderr)
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_rejects_unknown_process_state(self):
        self.assertNotEqual(self.run_hook("preinstall",PGREP_STATUS=2).returncode,0)

    def test_preinstall_refuses_other_volume(self):
        result=self.run_hook("preinstall",target="/Volumes/Other")
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_refuses_rootless_hook_or_wrong_platform(self):
        for values in ({"EUID":"501"},{"OS_VERSION":"12.7"},{"ARM":"1"},{"OS":"Linux"}):
            with self.subTest(values=values):
                self.assertNotEqual(self.run_hook("preinstall",**values).returncode,0)

    def test_preinstall_accepts_all_four_requested_intel_systems(self):
        shutil.rmtree(self.bundle)
        for version in ("13.0", "14.0", "15.0", "26.0"):
            with self.subTest(version=version):
                result = self.run_hook("preinstall", OS_VERSION=version)
                self.assertEqual(result.returncode, 0, result.stdout+result.stderr)

    def test_preinstall_rejects_symlink_destination(self):
        target=self.apps/"Elsewhere.app"
        self.bundle.rename(target)
        self.bundle.symlink_to(target)
        self.assertNotEqual(self.run_hook("preinstall").returncode,0)

    def test_preinstall_rejects_wrong_identifier(self):
        info_path=self.bundle/"Contents/Info.plist"
        info=plistlib.loads(info_path.read_bytes())
        info["CFBundleIdentifier"]="another.app"
        info_path.write_bytes(plistlib.dumps(info))
        self.assertNotEqual(self.run_hook("preinstall").returncode,0)

    def test_preinstall_prepares_upgrade_as_console_user(self):
        self.service.touch()
        result=self.run_hook("preinstall")
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertFalse(self.service.exists())
        self.assertEqual(self.history("old-app")[0]["args"],["--prepare-update"])
        self.assertIn("#501", self.history("sudo")[0]["args"])

    def test_preinstall_refuses_failed_prepare(self):
        self.service.touch()
        self.assertNotEqual(self.run_hook("preinstall",PREPARE_FAIL=1).returncode,0)
        self.assertTrue(self.service.exists())

    def test_preinstall_refuses_service_that_did_not_stop(self):
        self.service.touch()
        self.assertNotEqual(self.run_hook("preinstall",KEEP_SERVICE=1).returncode,0)

    def test_preinstall_allows_repair_of_unprotected_copy_without_service(self):
        result=self.run_hook("preinstall",BAD_OWNER=1)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_no_unprotected_execution_with_registered_service(self):
        self.service.touch()
        result=self.run_hook("preinstall",BAD_OWNER=1)
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(self.history("old-app"),[])

    def test_preinstall_refuses_active_service_without_gui(self):
        self.service.touch()
        self.assertNotEqual(self.run_hook("preinstall",GUI=0).returncode,0)
        self.assertEqual(self.history("old-app"),[])

    def test_postinstall_opens_verified_app_as_user(self):
        result=self.run_hook("postinstall")
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertEqual(self.history("open")[0]["args"],
                         [str(self.bundle),"--args","--complete-install"])
        self.assertIn("#501",self.history("sudo")[0]["args"])
        calls=[event["tool"] for event in self.history()]
        self.assertLess(calls.index("codesign"),calls.index("open"))

    def test_postinstall_does_not_open_app_without_gui(self):
        result=self.run_hook("postinstall",GUI=0)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(self.history("open"),[])

    def test_postinstall_never_opens_app_as_root(self):
        result=self.run_hook("postinstall",CONSOLE_UID=0)
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertEqual(self.history("open"),[])

    def test_postinstall_gui_failure_is_not_payload_failure(self):
        result=self.run_hook("postinstall",OPEN_STATUS=1)
        self.assertEqual(result.returncode,0)
        self.assertIn("WARNING",result.stdout)

    def test_postinstall_refuses_bad_signature(self):
        self.assertNotEqual(self.run_hook("postinstall",BAD_SIGNATURE=1).returncode,0)
        self.assertEqual(self.history("open"),[])

    def test_postinstall_refuses_group_writable_file(self):
        (self.bundle/"Contents/Info.plist").chmod(0o664)
        self.assertNotEqual(self.run_hook("postinstall").returncode,0)
        self.assertEqual(self.history("open"),[])

    def test_postinstall_refuses_missing_helper(self):
        (self.bundle/"Contents/Helpers/TahoeWiFiHelper").unlink()
        self.assertNotEqual(self.run_hook("postinstall").returncode,0)
        self.assertEqual(self.history("open"),[])

    def test_builder_packs_only_complete_app_with_root_ownership(self):
        before={str(p.relative_to(self.bundle)):p.read_bytes()
                for p in self.bundle.rglob("*") if p.is_file()}
        result=self.run_builder()
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        output=self.folder/"output with spaces/Tahoe-WiFi-USB-0.2.7.pkg"
        manifest=json.loads(output.read_text())
        self.assertIn("--ownership",manifest["component"]["args"])
        self.assertIn("recommended",manifest["component"]["args"])
        self.assertTrue(all(p.startswith("Applications/Tahoe Wi-Fi USB.app/")
                            for p in manifest["component"]["payload"]))
        self.assertFalse(any(".kext" in p for p in manifest["component"]["payload"]))
        self.assertTrue(Path(str(output)+".sha256").exists())
        self.assertEqual(before,{str(p.relative_to(self.bundle)):p.read_bytes()
                                for p in self.bundle.rglob("*") if p.is_file()})
        self.assertEqual(self.history("open"),[])

    def test_builder_preserves_previous_package_on_failure(self):
        output=self.folder/"output with spaces/Tahoe-WiFi-USB-0.2.7.pkg"
        output.parent.mkdir()
        output.write_bytes(b"previous package")
        result=self.run_builder(PKG_TEST_FAIL_PRODUCTBUILD="1")
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(output.read_bytes(),b"previous package")
        self.assertEqual(list(output.parent.glob(".TahoeWiFi-pkg.*")),[])

    def test_builder_rejects_invalid_signature(self):
        self.assertNotEqual(self.run_builder(PKG_TEST_BAD_SIGNATURE="1").returncode,0)
        self.assertEqual(self.history("pkgbuild"),[])

    def test_builder_rejects_symlink_inside_payload(self):
        (self.bundle/"Contents/link").symlink_to("/tmp")
        self.assertNotEqual(self.run_builder().returncode,0)
        self.assertEqual(self.history("pkgbuild"),[])

    def test_builder_rejects_missing_helper(self):
        (self.bundle/"Contents/Helpers/TahoeWiFiHelper").unlink()
        self.assertNotEqual(self.run_builder().returncode,0)

    def test_builder_rejects_app_requiring_macos26_despite_plist13(self):
        (self.bundle/"Contents/MacOS/TahoeUSBWiFi").write_bytes(macho_fixture(0x001A0000))
        result = self.run_builder()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("minimum macOS 26.0", result.stdout+result.stderr)
        self.assertEqual(self.history("pkgbuild"), [])

    def test_builder_rejects_helper_requiring_macos14(self):
        (self.bundle/"Contents/Helpers/TahoeWiFiHelper").write_bytes(macho_fixture(0x000E0000))
        result = self.run_builder()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("minimum macOS 14.0", result.stdout+result.stderr)
        self.assertEqual(self.history("pkgbuild"), [])

    def test_builder_rejects_non_macho_despite_lipo_double(self):
        (self.bundle/"Contents/Helpers/TahoeWiFiHelper").write_bytes(b"not Mach-O")
        result = self.run_builder()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Mach-O", result.stdout+result.stderr)
        self.assertEqual(self.history("pkgbuild"), [])

    def test_builder_accepts_existing_version_025(self):
        info_path=self.bundle/"Contents/Info.plist"
        info=plistlib.loads(info_path.read_bytes())
        info["CFBundleShortVersionString"]="0.2.5"
        del info["RTWInstallerProtocolVersion"]
        info_path.write_bytes(plistlib.dumps(info))
        result=self.run_builder()
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertTrue((self.folder/"output with spaces/Tahoe-WiFi-USB-0.2.5.pkg").exists())

    def test_builder_refuses_unsupported_version(self):
        info_path=self.bundle/"Contents/Info.plist"
        info=plistlib.loads(info_path.read_bytes())
        info["CFBundleShortVersionString"]="0.2.4"
        del info["RTWInstallerProtocolVersion"]
        info_path.write_bytes(plistlib.dumps(info))
        self.assertNotEqual(self.run_builder().returncode,0)

    def test_builder_refuses_root_execution(self):
        self.assertNotEqual(self.run_builder(PKG_TEST_EUID="0").returncode,0)

    def test_builder_notarization_requires_signatures(self):
        result=self.run_builder(RTW_NOTARY_PROFILE="test-profile")
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(self.history("pkgbuild"),[])

    def test_builder_notarization_requires_acceptance(self):
        result=self.run_builder(RTW_NOTARY_PROFILE="test-profile",
            RTW_INSTALLER_SIGN_IDENTITY="Developer ID Installer: Test",
            PKG_TEST_DEVELOPER_ID="1", PKG_TEST_NOTARY_STATUS="Invalid")
        self.assertNotEqual(result.returncode,0)
        self.assertFalse((self.folder/"output with spaces/Tahoe-WiFi-USB-0.2.7.pkg").exists())

    def test_builder_refuses_notarization_under_mandatory_adhoc_policy(self):
        result=self.run_builder(RTW_NOTARY_PROFILE="test-profile",
            RTW_INSTALLER_SIGN_IDENTITY="Developer ID Installer: Test",PKG_TEST_DEVELOPER_ID="1")
        self.assertNotEqual(result.returncode,0)
        self.assertEqual(self.history("xcrun"),[])
        self.assertEqual(self.history("pkgbuild"),[])

    def test_builder_refuses_certificate_signed_app(self):
        self.assertNotEqual(self.run_builder(PKG_TEST_DEVELOPER_ID="1").returncode,0)
        self.assertEqual(self.history("pkgbuild"),[])

    def test_builder_refuses_missing_adhoc_metadata(self):
        self.assertNotEqual(self.run_builder(PKG_TEST_MISSING_ADHOC="1").returncode,0)
        self.assertEqual(self.history("pkgbuild"),[])

    def test_postinstall_refuses_non_adhoc_payload(self):
        self.assertNotEqual(self.run_hook("postinstall", DEVELOPER_ID="1").returncode,0)
        self.assertEqual(self.history("open"),[])

    def test_container_signature_does_not_replace_adhoc_app(self):
        result=self.run_builder(RTW_INSTALLER_SIGN_IDENTITY="Developer ID Installer: Test")
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertIn("--sign",self.history("productbuild")[0]["args"])
        self.assertEqual(self.history("xcrun"),[])

class MachOCompatibilityTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory(prefix="rtw-macos-compatibility-")
        self.addCleanup(temporary.cleanup)
        self.folder = Path(temporary.name)
        self.binary = self.folder/"executable with spaces"

    def check(self, data, *arguments):
        self.binary.write_bytes(data)
        return subprocess.run([sys.executable, str(ROOT/"app/check-macos-compatibility.py"),
            "--json", *arguments, str(self.binary)], capture_output=True, text=True, timeout=10)

    def test_accepts_ventura_with_newer_sdk_and_emits_version_evidence(self):
        result = self.check(macho_fixture())
        self.assertEqual(result.returncode, 0, result.stderr)
        report = json.loads(result.stdout)
        self.assertTrue(report["compatible"])
        self.assertEqual(report["files"][0]["slices"], [{"arch":"x86_64", "filetype":"executable",
            "platform":"macOS", "minos":"13.0", "sdk":"26.5", "command":"LC_BUILD_VERSION"}])

    def test_accepts_legacy_version_load_command_and_fat_container(self):
        for data in (macho_fixture(command=0x24), fat_fixture(macho_fixture())):
            with self.subTest(data=data):
                result = self.check(data)
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_rejects_newer_minimum_even_with_supported_cpu(self):
        for version, expected in ((0x000D0001,"13.0.1"), (0x000E0000,"14.0"), (0x001A0000,"26.0")):
            with self.subTest(version=version):
                result = self.check(macho_fixture(version))
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("minimum macOS "+expected, result.stderr)
                self.assertFalse(json.loads(result.stdout)["compatible"])

    def test_rejects_wrong_arch_platform_and_non_executable(self):
        for data, expected in ((macho_fixture(cpu=0x0100000C),"x86_64"),
                (macho_fixture(platform=6),"macOS"), (macho_fixture(filetype=6),"executable")):
            with self.subTest(expected=expected):
                result = self.check(data)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(expected, result.stderr)

    def test_rejects_foreign_deployment_commands_without_macos(self):
        for command, name in ((0x25, "LC_VERSION_MIN_IPHONEOS"),
                (0x2F, "LC_VERSION_MIN_TVOS"), (0x30, "LC_VERSION_MIN_WATCHOS")):
            with self.subTest(command=name):
                result = self.check(macho_fixture(0x001A0000, command=command))
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(name, result.stderr)
                self.assertFalse(json.loads(result.stdout)["compatible"])

    def test_rejects_foreign_commands_before_or_after_either_macos_command(self):
        for foreign, name in ((0x25, "LC_VERSION_MIN_IPHONEOS"),
                (0x2F, "LC_VERSION_MIN_TVOS"), (0x30, "LC_VERSION_MIN_WATCHOS")):
            for macos in (0x24, 0x32):
                for foreign_first in (False, True):
                    with self.subTest(foreign=name, macos=macos, foreign_first=foreign_first):
                        commands = [macho_fixture(command=macos)[32:],
                            macho_fixture(0x001A0000, command=foreign)[32:]]
                        if foreign_first:
                            commands.reverse()
                        load = b"".join(commands)
                        header = struct.pack("<8I", 0xFEEDFACF, 0x01000007, 3, 2, 2, len(load), 0, 0)
                        result = self.check(header + load)
                        self.assertNotEqual(result.returncode, 0)
                        self.assertIn(name, result.stderr)
                        self.assertFalse(json.loads(result.stdout)["compatible"])

    def test_rejects_missing_duplicate_or_malformed_version_commands(self):
        valid = macho_fixture()
        duplicate = struct.pack("<8I", 0xFEEDFACF, 0x01000007, 3, 2, 2, 48, 0, 0) + valid[32:]*2
        missing = struct.pack("<8I", 0xFEEDFACF, 0x01000007, 3, 2, 0, 0, 0, 0)
        undersized = valid[:36] + struct.pack("<I", 8) + valid[40:]
        for data in (b"not Mach-O", valid[:20], valid[:-1], duplicate, missing,
                     undersized, fat_fixture(valid)[:-1], macho_fixture(0)):
            with self.subTest(data=data):
                result = self.check(data)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn("Mach-O", result.stderr)

    def test_optional_limit_cannot_hide_an_incompatible_second_file(self):
        self.binary.write_bytes(macho_fixture(0x001A0000))
        other = self.folder/"helper"
        other.write_bytes(macho_fixture(0x000E0000))
        result = subprocess.run([sys.executable, str(ROOT/"app/check-macos-compatibility.py"),
            "--json", "--max-minos", "14.0", str(other), str(self.binary)],
            capture_output=True, text=True, timeout=10)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("minimum macOS 26.0", result.stderr)
        self.assertEqual(len(json.loads(result.stdout)["files"]), 2)


@unittest.skipUnless(sys.platform == "darwin" and shutil.which("xcrun"), "requires native macOS/Xcode")
class NativeBuildDeploymentTests(unittest.TestCase):
    def test_incompatible_staged_helper_does_not_replace_previous_app(self):
        # Compile real production sources and sign normally. Only force the
        # helper compiler's target upward, modelling a stale/wrong build flag.
        with tempfile.TemporaryDirectory(prefix="rtw-native-deployment-") as folder:
            project = Path(folder)/"project with spaces"
            shutil.copytree(ROOT/"app", project/"app")
            shutil.copyfile(ROOT/"LICENSE-GPL-2.0.txt", project/"LICENSE-GPL-2.0.txt")
            for relative in ("include", "kext", "src"):
                (project/relative).symlink_to(ROOT/relative, target_is_directory=True)
            commands = project/"commands"
            commands.mkdir()
            real_xcrun = shutil.which("xcrun")
            wrapper = commands/"xcrun"
            wrapper.write_text("#!"+sys.executable+" -S\nimport os,sys\n"
                "args=sys.argv[1:]\n"
                "if args[:1]==['clang++'] and 'PrivilegedService.mm' in args:\n"
                "    args=[('-mmacosx-version-min=26.0' if arg.startswith('-mmacosx-version-min=') else arg) for arg in args]\n"
                "os.execv("+repr(real_xcrun)+", ["+repr(real_xcrun)+"]+args)\n")
            wrapper.chmod(0o755)
            output = project/"output with spaces"
            previous = output/"Tahoe Wi-Fi USB.app"
            previous.mkdir(parents=True)
            sentinel = previous/"previous-app"
            sentinel.write_bytes(b"keep this previous app")
            environment = dict(os.environ, PATH=str(commands)+os.pathsep+os.environ["PATH"],
                RTW_SIGN_IDENTITY="-", RTW_NOTARY_PROFILE="")
            result = subprocess.run(["bash", str(project/"app/build.command"), str(output)],
                capture_output=True, text=True, env=environment, timeout=90)
            self.assertNotEqual(result.returncode, 0, result.stdout+result.stderr)
            self.assertIn("minimum macOS 26.0", result.stdout+result.stderr)
            self.assertEqual(sentinel.read_bytes(), b"keep this previous app")
            self.assertEqual(list(output.glob(".TahoeWiFi-build.*")), [])
            self.assertEqual(list(output.glob(".Tahoe-WiFi-USB-previous.*")), [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
