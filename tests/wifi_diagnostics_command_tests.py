#!/usr/bin/env python3
"""Exercise the real Bash 3.2 collector with offline macOS command shims."""
from pathlib import Path
import argparse
import csv
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
import zipfile


PROJECT = Path(__file__).resolve().parents[1]
arguments = argparse.ArgumentParser(add_help=False)
arguments.add_argument("--collector", type=Path, default=PROJECT / "WiFi-Diagnostics.command")
OPTIONS, unittest_arguments = arguments.parse_known_args()
sys.argv[1:] = unittest_arguments
COUNTERS = (
    "RTWDiagRXRecoveryErrors", "RTWDiagRXRecoveryAttempts", "RTWDiagRXRecoveryFailures",
    "RTWDiagTXUSBTransferErrors", "RTWDiagTXQueueDrops", "RTWDiagRXAggregateErrors",
    "RTWDiagRXAggregateTruncated", "RTWDiagRXAggregateMalformed",
    "RTWDiagRXAggregateTooMany", "RTWDiagRXAggregateBadArgument",
)
ADAPTER_A = 0x100000111
ADAPTER_B = 0x100000222
SECRETS = ("SecretMixedCaseNetwork", "aabbccddeeff", "aa:bb:cc:dd:ee:ff",
           "private-person@example.invalid", "SuperSecretPassword")


def adapter(entry_id, family, interface, vid, pid, values):
    return {"registry_id": entry_id, "family": family, "interface": interface,
            "vid": vid, "pid": pid, "values": dict(zip(COUNTERS, values))}


def registry_text(adapters):
    sections = []
    for entry in adapters:
        values = dict(entry["values"], SignalValidSamples=20, SignalLastDBM=-42)
        properties = "\n".join('  |   "%s" = %d' % pair for pair in values.items())
        properties += entry.get("extra_properties", "")
        sections.append('''+-o RTWUSBController <class RTWUSBController, id 0x%x, registered, matched, active, busy 0 (0 ms), retain 8>
  | {
  |   "RTWSiliconFamily" = "%s"
  |   "idVendor" = %d
  |   "idProduct" = %d
  |   "IOMACAddress" = <aabbccddeeff>
  |   "RTWDiagSensitiveText" = "DO-NOT-EXPORT-TEXT-AS-A-COUNTER"
%s
  | }
  +-o %s <class IOEthernetInterface, id 0x%x, registered, matched, active, busy 0 (0 ms), retain 6>
      {
        "BSD Name" = "%s"
      }
''' % (entry["registry_id"], entry["family"], entry["vid"], entry["pid"],
       properties, entry["interface"], entry["registry_id"] + 1, entry["interface"]))
    return "\n".join(sections)


SHIM = r'''#!/bin/bash
name="${0##*/}"
printf '%s\t%s\n' "$name" "$*" >> "$SHIM_TRACE"
if [ "$name" = sudo ]; then
  printf 'sudo-invocation' >> "$SHIM_TRACE"
  printf '\t%q' "$@" >> "$SHIM_TRACE"
  printf '\n' >> "$SHIM_TRACE"
fi
case "$name" in
  uname)
    case "${1:-}" in -s) echo Darwin;; -m) echo x86_64;; *) echo 'Darwin fixture-kernel';; esac;;
  system_profiler)
    case "$*" in
      -listDataTypes) echo 'SPUSBHostDataType';;
      *SPHardwareDataType*)
        echo 'hardware stdout captured before completion'
        echo 'hardware stderr captured before completion' >&2
        if [ "${SHIM_SLOW_COMMAND:-}" = descendants ]; then
          trap '' TERM
          printf '%s\n' "$$" > "$SHIM_STATE/command-pid"
          /bin/bash -c 'trap "" TERM; printf "%s\n" "$$" > "$SHIM_STATE/descendant-pid"; /bin/sleep 15' &
          wait "$!"
        fi
        if [ "${SHIM_SLOW_COMMAND:-}" = hardware ]; then /bin/sleep 15; fi;;
      *-json*) echo '{"SPUSBHostDataType": []}';;
      *) echo 'Realtek USB fixture idVendor=0x0bda idProduct=0x8813';;
    esac;;
  ioreg)
    case "$*" in
      *RTWUSBController*)
        number=0
        if [ -f "$SHIM_STATE/rtw-count" ]; then IFS= read -r number < "$SHIM_STATE/rtw-count"; fi
        number=$((number + 1)); printf '%s\n' "$number" > "$SHIM_STATE/rtw-count"
        if [ "$number" -eq 1 ]; then
          /bin/cat "$SHIM_BEFORE"
        else
          case "${SHIM_SECOND_IOREG:-}" in
            empty) echo 'ioreg fixture failure before output' >&2; exit 77;;
            partial) /usr/bin/head -n 15 "$SHIM_AFTER"; echo 'ioreg fixture failure after partial output' >&2; exit 77;;
            *) /bin/cat "$SHIM_AFTER";;
          esac
        fi;;
      *IOUSBHostInterface*)
        if [ -n "${SHIM_USB_INTERFACES:-}" ]; then /bin/cat "$SHIM_USB_INTERFACES"; else echo 'IORegistry fixture'; fi
        if [ "${SHIM_USB_INTERFACES_PARTIAL:-}" = yes ]; then echo 'partial USB fixture' >&2; exit 77; fi;;
      *IOUSBHostDevice*)
        printf '+-o Realtek USB <class IOUSBHostDevice, id 0x99, registered>\n  "idVendor" = 3034\n  "idProduct" = 34835\n';;
      *) echo 'IORegistry fixture';;
    esac;;
  log)
    [ "${1:-}" = show ] || { echo FORBIDDEN_LOG_MUTATION >&2; exit 97; }
    window=""; previous=""
    for argument in "$@"; do
      if [ "$previous" = --last ]; then window="$argument"; fi
      previous="$argument"
    done
    if [ "${SHIM_LOG_SCENARIO:-}" = error ]; then echo 'log fixture unavailable' >&2; exit 77; fi
    if [ "${SHIM_LOG_SCENARIO:-}" = empty_timeouts ]; then /bin/sleep 15; exit 0; fi
    if [ "$window" = 2m ]; then
      echo '2026-11-01 18:29:00.100000-0500 Realtek USB WiFi recent-window first'
    else
      echo '2026-11-01 18:00:00.100000-0500 Realtek USB WiFi historical-window first'
    fi
    echo 'Realtek USB WiFi en7 sSiD = "SecretMixedCaseNetwork"'
    echo 'WiFi IOMACAddress = <aabbccddeeff> ether aa:bb:cc:dd:ee:ff'
    echo 'Network WiFi Password = "SuperSecretPassword" contact private-person@example.invalid'
    echo 'Realtek partial stdout before completion'
    echo 'Realtek partial stderr before completion' >&2
    if [ "$window" = 2m ]; then
      echo '2026-11-01 18:29:30.200000-0500 Realtek USB WiFi recent-window last'
    else
      echo '2026-11-01 18:01:00.200000-0500 Realtek USB WiFi historical-window last'
    fi
    if [ "${SHIM_LOG_SCENARIO:-}" = history_timeout_recent_ok ] && [ "$window" != 2m ]; then /bin/sleep 15; fi
    if [ "${SHIM_LOG_SCENARIO:-}" = both_timeout ]; then /bin/sleep 15; fi
    if [ "${SHIM_SLOW_COMMAND:-}" = log ]; then /bin/sleep 15; fi;;
  find)
    case "${1:-}" in */DiagnosticReports) exit 0;; *) exec /usr/bin/find "$@";; esac;;
  networksetup)
    case "${1:-}" in -listallhardwareports|-listallnetworkservices) echo 'Hardware Port: fixture USB';;
      *) echo FORBIDDEN_NETWORK_CHANGE >&2; exit 97;; esac;;
  ifconfig)
    [ "${1:-}" = -a ] || { echo FORBIDDEN_NETWORK_CHANGE >&2; exit 97; }
    echo 'en7: ether aa:bb:cc:dd:ee:ff inet 192.168.23.42';;
  scutil) [ "${1:-}" = --nwi ] || exit 97; echo 'Network fixture en7';;
  netstat) [ "${1:-}" = -rn ] || exit 97; echo 'route fixture';;
  wdutil) [ "${1:-}" = info ] || exit 97; echo 'SSID: SecretMixedCaseNetwork';;
  csrutil) [ "${1:-}" = status ] || exit 97; echo 'System Integrity Protection status: enabled.';;
  spctl)
    [ "${1:-}" = --status ] || exit 97
    case "${SHIM_GATEKEEPER:-}" in
      disabled) echo 'assessments disabled'; exit 1;;
      error) echo 'assessment query failed' >&2; exit 1;;
      disabled_extra) printf 'assessments disabled\nadditional error\n'; exit 1;;
      disabled_blank) printf 'assessments disabled\n\n'; exit 1;;
      disabled_rc2) echo 'assessments disabled'; exit 2;;
      *) echo 'assessments enabled';;
    esac;;
  nvram) [ "${1:-}" = boot-args ] || exit 97; echo 'boot-args fixture';;
  pmset) [ "${1:-}" = -g ] || exit 97; echo 'power fixture';;
  kmutil) [ "${1:-}" = showloaded ] || exit 97; echo 'org.research.TahoeRealtekWiFi';;
  kextstat) [ "${1:-}" = -l ] || exit 97; echo 'org.research.TahoeRealtekWiFi';;
  systemextensionsctl) [ "${1:-}" = list ] || exit 97; echo 'No system extensions';;
  ls) echo 'fixture directory listing';;
  sw_vers) printf 'ProductName: macOS\nProductVersion: 26.0\n';;
  sudo)
    if [ "${SHIM_ALLOW_ADMIN:-}" = yes ]; then
      case "${1:-}" in -v) exit 0;; -n) shift; exec "$@";; esac
    fi
    echo FORBIDDEN_sudo >&2; exit 97;;
  curl|ping|networkQuality|kextload|kextunload|osascript)
    echo "FORBIDDEN_$name" >&2; exit 97;;
  *) echo "Unhandled fixture command: $name" >&2; exit 98;;
esac
'''


class CollectorHarness:
    def __init__(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="wifi diagnostics test ")
        self.root = Path(self.temporary.name)
        self.home = self.root / "User Home With Spaces"
        self.home.mkdir()
        self.default_destination = self.home / "Library/Logs/Tahoe-WiFi-USB/Diagnostics"
        self.output = self.root / "Custom Output With Spaces"
        self.tools = self.root / "fake commands"
        self.tools.mkdir()
        self.state = self.root / "shim state"
        self.state.mkdir()
        self.trace = self.root / "command trace.tsv"
        self.before_file = self.root / "before.txt"
        self.after_file = self.root / "after.txt"
        self.script = self.root / "Collector App With Spaces/WiFi-Diagnostics.command"
        self.script.parent.mkdir()
        shutil.copyfile(OPTIONS.collector, self.script)
        for name in ("uname", "system_profiler", "ioreg", "log", "find", "networksetup",
                     "ifconfig", "scutil", "netstat", "wdutil", "csrutil", "spctl", "nvram",
                     "pmset", "kmutil", "kextstat", "systemextensionsctl", "ls", "sw_vers",
                     "sudo", "curl", "ping", "networkQuality", "kextload", "kextunload", "osascript"):
            path = self.tools / name
            path.write_text(SHIM)
            path.chmod(0o755)
        self.before = [adapter(ADAPTER_A, "RTL8814AU", "en7", 3034, 34835, range(20, 20 + len(COUNTERS))),
                       adapter(ADAPTER_B, "RTL8192EU", "en8", 9047, 264, range(50, 50 + len(COUNTERS)))]
        self.deltas = {ADAPTER_A: range(1, 1 + len(COUNTERS)), ADAPTER_B: range(11, 11 + len(COUNTERS))}
        self.after = []
        for entry in reversed(self.before):
            values = [entry["values"][key] + delta for key, delta in
                      zip(COUNTERS, self.deltas[entry["registry_id"]])]
            self.after.append(adapter(entry["registry_id"], entry["family"], entry["interface"],
                                      entry["vid"], entry["pid"], values))

    def close(self):
        self.temporary.cleanup()

    def run(self, *arguments, default=False, slow="", second_ioreg="", usb_interfaces="", usb_partial=False,
            gatekeeper="", log_scenario="", shim_admin=False):
        self.before_file.write_text(registry_text(self.before))
        self.after_file.write_text(registry_text(self.after))
        env = os.environ.copy()
        env.update(HOME=str(self.home), PATH=str(self.tools) + ":/usr/bin:/bin:/usr/sbin:/sbin",
                   SHIM_TRACE=str(self.trace), SHIM_STATE=str(self.state),
                   SHIM_BEFORE=str(self.before_file), SHIM_AFTER=str(self.after_file),
                   SHIM_SLOW_COMMAND=slow, SHIM_SECOND_IOREG=second_ioreg,
                   SHIM_GATEKEEPER=gatekeeper, SHIM_LOG_SCENARIO=log_scenario,
                   SHIM_ALLOW_ADMIN="yes" if shim_admin else "",
                   SHIM_USB_INTERFACES_PARTIAL="yes" if usb_partial else "")
        if usb_interfaces:
            interfaces = self.root / "USB interfaces.txt"
            interfaces.write_text(usb_interfaces)
            env["SHIM_USB_INTERFACES"] = str(interfaces)
        command = ["/bin/bash", str(self.script)]
        if not default:
            command += ["--no-pause", "--output-dir", str(self.output)]
        started = time.monotonic()
        result = subprocess.run(command + list(arguments), env=env, capture_output=True,
                                stdin=subprocess.DEVNULL, text=True, timeout=20)
        self.elapsed = time.monotonic() - started
        self.result = result
        if result.returncode:
            raise AssertionError("Collector exit %d\n%s\n%s" %
                                 (result.returncode, result.stdout, result.stderr))
        destination = self.default_destination if default else self.output
        archives = list(destination.glob("*.zip")) if destination.is_dir() else []
        if len(archives) != 1:
            raise AssertionError("Expected one ZIP in %s; actual archives: %s\n%s" %
                                 (destination, list(self.root.rglob("*.zip")), result.stdout))
        self.archive = archives[0]
        self.report = self.archive.with_suffix("")
        if not self.report.is_dir():
            raise AssertionError("Collector must retain the report directory beside its ZIP")
        return self.report


def numeric_properties(record):
    properties = dict(record)
    for key in ("properties", "diagnostics", "counters"):
        if isinstance(record.get(key), dict):
            properties.update(record[key])
    return {key: value for key, value in properties.items()
            if key.startswith("RTWDiag") or key.startswith("Signal")}


def identity(value):
    return int(value, 0) if isinstance(value, str) and value.lower().startswith("0x") else int(value)


class WiFiDiagnosticsCommandTests(unittest.TestCase):
    def setUp(self):
        if sys.platform != "darwin":
            self.skipTest("Native collector regression requires macOS Bash/Perl/ditto")
        self.harness = CollectorHarness()
        self.addCleanup(self.harness.close)

    def snapshots(self, report):
        for name in ("RTW-before.txt", "RTW-after.txt", "RTW-before.json", "RTW-after.json"):
            self.assertTrue((report / "drivers" / name).is_file(), "Missing driver snapshot: " + name)
        before = json.loads((report / "drivers/RTW-before.json").read_text())
        after = json.loads((report / "drivers/RTW-after.json").read_text())
        self.assertIsInstance(before, list)
        self.assertIsInstance(after, list)
        return ({identity(row["registry_id"]): row for row in before},
                {identity(row["registry_id"]): row for row in after})

    def assert_safe_commands(self):
        trace = self.harness.trace.read_text()
        names = {line.split("\t", 1)[0] for line in trace.splitlines()}
        self.assertTrue({"ioreg", "system_profiler", "log", "find"}.issubset(names))
        self.assertFalse(names.intersection({"sudo", "curl", "ping", "networkQuality",
                                             "kextload", "kextunload", "osascript"}), trace)
        allowed = {"networksetup": {"-listallhardwareports", "-listallnetworkservices"},
                   "ifconfig": {"-a"}, "nvram": {"boot-args"}, "csrutil": {"status"},
                   "spctl": {"--status"}, "pmset": {"-g"}, "kmutil": {"showloaded"},
                   "log": {"show"}}
        for line in trace.splitlines():
            name, _, arguments = line.partition("\t")
            if name in allowed:
                self.assertIn(arguments.split(" ", 1)[0], allowed[name], line)

    def assert_archive_matches(self, report):
        with zipfile.ZipFile(self.harness.archive) as archive:
            self.assertIsNone(archive.testzip())
            for relative in ("drivers/RTW-before.txt", "drivers/RTW-after.txt",
                             "drivers/RTW-before.json", "drivers/RTW-after.json",
                             "drivers/USB-STARTUP.json", "USB-STARTUP.txt",
                             "RTW-SUMMARY.txt", "STATUS.tsv", "logs/USB_Realtek.log"):
                self.assertEqual(archive.read(report.name + "/" + relative),
                                 (report / relative).read_bytes(), relative)

    def test_default_double_click_destination(self):
        report = self.harness.run(default=True)
        self.assertEqual(report.parent, self.harness.default_destination)
        self.assert_safe_commands()

    def test_failed_usb_start_is_reported_without_controller(self):
        self.harness.before = []
        self.harness.after = []
        fixture = '''+-o Bluetooth Radio@0 <class IOUSBHostInterface, id 0x100001776, registered>
  "idVendor" = 29586
  "idProduct" = 42513
  "bInterfaceNumber" = 0
+-o Wi-Fi@2 <class IOUSBHostInterface, id 0x100001777, registered>
  "idVendor" = 29586
  "idProduct" = 42513
  "bConfigurationValue" = 1
  "bInterfaceNumber" = 2
  "bAlternateSetting" = 0
  "bInterfaceClass" = 255
  "bInterfaceSubClass" = 255
  "bInterfaceProtocol" = 255
  "bNumEndpoints" = 6
  "RTWStartStage" = "usb-discovery"
  "RTWStartResult" = 18446744073172681415
  "USB Serial Number" = "DO-NOT-EXPORT-SERIAL"
  +-o unrelated <class IOService, id 0x100001778, registered>
    "RTWStartStage" = "not-the-provider"
    "RTWStartResult" = 0
'''
        report = self.harness.run(usb_interfaces=fixture)
        records = json.loads((report / "drivers/USB-STARTUP.json").read_text())
        self.assertEqual(len(records), 1)
        record = records[0]
        self.assertEqual((record["vid"], record["pid"], record["interface_number"]), (0x7392, 0xa611, 2))
        self.assertEqual(record["start_result"], 0xe00002c7)
        self.assertEqual(record["start_stage"], "usb-discovery")
        summary = (report / "USB-STARTUP.txt").read_text()
        self.assertIn("7392:a611", summary)
        self.assertIn("0xe00002c7", summary)
        self.assertIn("usb-discovery", summary)
        self.assertNotIn("DO-NOT-EXPORT-SERIAL", json.dumps(records) + summary)
        self.assert_safe_commands()

    def test_usb_startup_keeps_provider_and_controller_properties_separate(self):
        # A child's success must never overwrite the provider's failed discovery.
        fixture = '''+-o Wi-Fi@2 <class IOUSBHostInterface, id 0x100001777, registered>
  "idVendor" = 29586
  "idProduct" = 42513
  "bConfigurationValue" = 1
  "bInterfaceNumber" = 2
  "bAlternateSetting" = 0
  "bInterfaceClass" = 255
  "bInterfaceSubClass" = 255
  "bInterfaceProtocol" = 255
  "bNumEndpoints" = 6
  "RTWStartStage" = "usb-discovery"
  "RTWStartResult" = -536870201
  "RTWUSBDiscoveryStage" = "endpoint-descriptor"
  "RTWDiagUSBDiscoveryEndpointCount" = 2
  "RTWDiagUSBDiscoveryBulkOutAddresses" = 515
  "RTWDiagUSBDiscoveryText" = "DO-NOT-EXPORT-DISCOVERY-TEXT"
  +-o RTWUSBController <class RTWUSBController, id 0x100001778, registered>
    "idVendor" = 3034
    "idProduct" = 47122
    "RTWStartStage" = "ready"
    "RTWStartResult" = 0
    "RTWUSBDiscoveryStage" = "ready"
    "RTWDiagUSBDiscoveryEndpointCount" = 4
    +-o unrelated <class IOService, id 0x100001779, registered>
      "RTWDiagUSBDiscoveryEndpointCount" = 99
'''
        report = self.harness.run(usb_interfaces=fixture)
        records = json.loads((report / "drivers/USB-STARTUP.json").read_text())
        provider, controller = records
        self.assertEqual(provider["source_class"], "IOUSBHostInterface")
        self.assertEqual(provider["start_result"], 0xe00002c7)
        self.assertEqual((provider["configuration_value"], provider["alternate_setting"],
                          provider["interface_class"], provider["interface_subclass"],
                          provider["interface_protocol"], provider["endpoint_count"]), (1, 0, 255, 255, 255, 6))
        self.assertEqual(provider["RTWDiagUSBDiscoveryEndpointCount"], 2)
        self.assertEqual(provider["discovery_stage"], "endpoint-descriptor")
        self.assertEqual(controller["source_class"], "RTWUSBController")
        self.assertEqual((controller["vid"], controller["pid"], controller["start_result"]), (3034, 47122, 0))
        self.assertNotIn("interface_number", controller)
        self.assertEqual(controller["RTWDiagUSBDiscoveryEndpointCount"], 4)
        self.assertNotIn("DO-NOT-EXPORT-DISCOVERY-TEXT", json.dumps(records))
        self.assertIn("USB-STARTUP.txt", (report / "RTW-SUMMARY.txt").read_text())
        self.assert_archive_matches(report)

    def test_partial_usb_capture_preserves_observation_without_absence_claim(self):
        self.harness.before = self.harness.after = []
        fixture = '''+-o Wi-Fi@2 <class IOUSBHostInterface, id 0x100001777, registered>
  "idVendor" = 29586
  "idProduct" = 42513
  "RTWStartResult" = 18446744073172681415
'''
        report = self.harness.run(usb_partial=True, usb_interfaces=fixture)
        records = json.loads((report / "drivers/USB-STARTUP.json").read_text())
        self.assertEqual(records[0]["start_result"], 0xe00002c7)
        summary = (report / "USB-STARTUP.txt").read_text()
        self.assertIn("partial", summary)
        self.assertIn("77", summary)
        self.assertNotIn("No RTW startup properties observed", summary)
        self.assert_safe_commands()

    def test_usb_startup_rejects_unbounded_or_non_ascii_stage_text(self):
        fixture = '''+-o Wi-Fi@2 <class IOUSBHostInterface, id 0x100001777, registered>
  "idVendor" = 29586
  "idProduct" = 42513
  "RTWStartResult" = 0
  "RTWStartStage" = "%s"
  "RTWUSBDiscoveryStage" = "DO-NOT-EXPORT-é"
''' % ("DO-NOT-EXPORT-" + "x" * 100)
        report = self.harness.run(usb_interfaces=fixture)
        records = json.loads((report / "drivers/USB-STARTUP.json").read_text())
        self.assertEqual(len(records), 1)
        self.assertNotIn("start_stage", records[0])
        self.assertNotIn("discovery_stage", records[0])
        self.assertNotIn("DO-NOT-EXPORT", (report / "USB-STARTUP.txt").read_text())

    def test_au_bootstrap_and_channel_power_failures_survive_numeric_export(self):
        # Missing channel power and the first boot failure must remain distinct
        # from a later reset failure; only named numeric/stage fields are safe.
        expected = {
            "RTWAUBootstrapStage": 8, "RTWAUBootstrapResult": 6,
            "RTWAUBootstrapFirstError": 6, "RTWAUBootstrapFailures": 2,
            "RTWAUWarmResetAttempts": 1, "RTWAUWarmResetStage": 3,
            "RTWAUWarmResetMCU": 128, "RTWAUWarmResetResult": 5,
            "RTWAU2GHzChannel1PowerResult": 22, "RTWAU5GHzChannel36PowerResult": 0,
        }
        for entries in (self.harness.before, self.harness.after):
            for entry in entries:
                if entry["registry_id"] == ADAPTER_A:
                    entry["values"].update(expected)
                    entry["extra_properties"] = '''
  |   "RTWAUBootstrapStageName" = "channel-power"
  |   "RTWAUWarmResetStageName" = "firmware-reset"
  |   "RTWAUFactoryPower" = <0011223344556677>
  |   "RTWAUFactoryPowerText" = "DO-NOT-EXPORT-FACTORY-TEXT"
  |   "RTWAUUnexpectedCounter" = 123
'''
                else:
                    entry["extra_properties"] = '''
  |   "RTWAUBootstrapStageName" = "DO-NOT-EXPORT-é"
  |   "RTWAUWarmResetStageName" = "%s"
''' % ("DO-NOT-EXPORT-" + "x" * 100)
        report = self.harness.run()
        before, after = self.snapshots(report)
        for snapshot in (before, after):
            record = snapshot[ADAPTER_A]
            for key, value in expected.items():
                self.assertEqual(record[key], value, key)
            self.assertEqual(record["RTWAUBootstrapStageName"], "channel-power")
            self.assertEqual(record["RTWAUWarmResetStageName"], "firmware-reset")
            self.assertNotIn("RTWAUFactoryPower", record)
            self.assertNotIn("RTWAUFactoryPowerText", record)
            self.assertNotIn("RTWAUUnexpectedCounter", record)
            self.assertNotIn("RTWAUBootstrapStageName", snapshot[ADAPTER_B])
            self.assertNotIn("RTWAUWarmResetStageName", snapshot[ADAPTER_B])
        summary = (report / "RTW-SUMMARY.txt").read_text()
        for key, value in expected.items():
            self.assertIn("%s=%d" % (key, value), summary)
        self.assertIn("RTWAUBootstrapStageName=channel-power", summary)
        self.assertIn("RTWAUWarmResetStageName=firmware-reset", summary)
        self.assertNotIn("DO-NOT-EXPORT", summary)
        self.assertNotIn("RTWAUFactoryPower", summary)
        self.assert_archive_matches(report)
        self.assert_safe_commands()

    def test_publication_generation_is_compared_per_controller(self):
        for phase, entries in (("before", self.harness.before), ("after", self.harness.after)):
            for entry in entries:
                if entry["registry_id"] == ADAPTER_A:
                    entry["values"].update(RTWDiagPublicationGeneration=10 + 2 * (phase == "after"),
                                           RTWDiagPublicationMonotonicMS=1000 + 50 * (phase == "after"))
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        au = summary.split("Controller %d" % ADAPTER_A)[1].split("Controller ")[0]
        eu = summary.split("Controller %d" % ADAPTER_B)[1].split("Controller ")[0]
        self.assertIn("Publication: generation advanced 10 -> 12", au)
        self.assertIn("RTWDiagPublicationMonotonicMS=1050", au)
        self.assertIn("Publication: freshness unknown", eu)
        self.assertIn("not atomic", summary)
        self.assert_safe_commands()

    def test_unchanged_publication_generation_is_not_called_live(self):
        for entries in (self.harness.before, self.harness.after):
            for entry in entries:
                entry["values"].update(RTWDiagPublicationGeneration=8,
                                       RTWDiagPublicationMonotonicMS=555)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertEqual(summary.count("Publication: no new publication observed; generation=8"), 2)
        self.assertNotIn("Publication: generation advanced", summary)

    def test_regressed_or_incomplete_publication_is_unknown(self):
        for entry in self.harness.before:
            entry["values"].update(RTWDiagPublicationGeneration=12,
                                   RTWDiagPublicationMonotonicMS=5000)
        self.harness.after[0]["values"].update(RTWDiagPublicationGeneration=2,
                                              RTWDiagPublicationMonotonicMS=100)
        self.harness.after[1]["values"].update(RTWDiagPublicationGeneration=14)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertIn("Publication: generation/time regressed; comparison unavailable", summary)
        self.assertIn("Publication: freshness unknown", summary)
        self.assertNotIn("Publication: generation advanced", summary)

    def test_odd_publication_generation_is_in_progress(self):
        for entry in self.harness.before:
            entry["values"].update(RTWDiagPublicationGeneration=10,
                                   RTWDiagPublicationMonotonicMS=1000)
        for entry in self.harness.after:
            entry["values"].update(RTWDiagPublicationGeneration=11,
                                   RTWDiagPublicationMonotonicMS=1000)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertEqual(summary.count("Publication: in progress; comparison unavailable"), 2)
        self.assertNotIn("Publication: generation advanced", summary)

    def test_invalid_native_ifnet_statistics_are_not_zero_traffic(self):
        for entry in self.harness.before:
            entry["values"].update(RTWDiagNativeIfnetStatsValid=1,
                                   RTWDiagNativeIfnetIndex=7,
                                   RTWDiagNativeIfnetInputPackets=200)
            entry["values"].update(RTWDiagNativeIfnetOutputPackets=100)
        for entry in self.harness.after:
            entry["values"].update(RTWDiagNativeIfnetStatsValid=0,
                                   RTWDiagNativeIfnetStatsError=6,
                                   RTWDiagNativeIfnetIndex=0,
                                   RTWDiagNativeIfnetInputPackets=0)
            entry["values"].update(RTWDiagNativeIfnetOutputPackets=0)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertIn("RTWDiagNativeIfnetStatsValid=0", summary)
        self.assertIn("RTWDiagNativeIfnetInputPackets native read unavailable/incomparable", summary)
        self.assertNotRegex(summary, r"RTWDiagNativeIfnetInputPackets[^\n]*delta=")
        self.assertIn("RTWDiagNativeIfnetOutputPackets native read unavailable/incomparable", summary)

    def test_native_boundary_samples_are_gauges_per_interface(self):
        for phase in (self.harness.before, self.harness.after):
            rows={entry["registry_id"]:entry for entry in phase}
            rows[ADAPTER_A]["values"].update(RTWDiagRXNativeSampleCount=1,
                RTWDiagRXNativeSampleBatch=1025,RTWDiagRXNativeSampleOfferedPackets=3,
                RTWDiagRXNativeSampleOfferedBytes=500,RTWDiagRXNativeSampleImmediateReturns=0,
                RTWDiagRXNativeSampleFlushPackets=3,RTWDiagRXNativeSampleIfnetStable=1,
                RTWDiagRXInputInterfaceMatchesChild=1,RTWDiagRXNativeBeforeStatsValid=1,
                RTWDiagRXNativeAfterStatsValid=1,RTWDiagRXNativeBeforeInputPackets=20,
                RTWDiagRXNativeAfterInputPackets=23)
            rows[ADAPTER_B]["values"].update(RTWDiagRXNativeSampleCount=0,
                RTWDiagRXNativeBeforeStatsValid=0,RTWDiagRXNativeAfterStatsValid=0,
                RTWDiagRXNativeBeforeInputPackets=0,RTWDiagRXInputInterfaceMatchesChild=0)
        report = self.harness.run()
        before,after = self.snapshots(report)
        self.assertEqual(numeric_properties(after[ADAPTER_A])["RTWDiagRXNativeAfterInputPackets"],23)
        self.assertEqual(numeric_properties(after[ADAPTER_B])["RTWDiagRXNativeSampleCount"],0)
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertIn("RTWDiagRXNativeSampleOfferedPackets=3",summary)
        self.assertIn("RTWDiagRXNativeBeforeInputPackets=20",summary)
        self.assertIn("RTWDiagRXNativeAfterInputPackets=23",summary)
        self.assertIn("RTWDiagRXInputInterfaceMatchesChild=1",summary)
        self.assertIn("RTWDiagRXInputInterfaceMatchesChild=0",summary)
        self.assertNotRegex(summary,r"RTWDiagRXNative[^\n]*delta=")

    def test_explicit_output_snapshots_match_adapters_and_counter_deltas(self):
        report = self.harness.run()
        before, after = self.snapshots(report)
        self.assertEqual(set(before), {ADAPTER_A, ADAPTER_B})
        self.assertEqual(set(after), {ADAPTER_A, ADAPTER_B})
        for expected in self.harness.before:
            entry_id = expected["registry_id"]
            for row in (before[entry_id], after[entry_id]):
                for key in ("family", "interface", "vid", "pid"):
                    self.assertEqual(row[key], expected[key], (entry_id, key, row))
                properties = numeric_properties(row)
                self.assertNotIn("RTWDiagSensitiveText", properties)
                self.assertTrue(all(type(value) in (int, float) for value in properties.values()))
                self.assertEqual(properties["SignalLastDBM"], -42)
            self.assertEqual({key: numeric_properties(before[entry_id])[key] for key in COUNTERS},
                             expected["values"])
            self.assertEqual([numeric_properties(after[entry_id])[key] - expected["values"][key]
                              for key in COUNTERS], list(self.harness.deltas[entry_id]))
        summary = (report / "RTW-SUMMARY.txt").read_text()
        current = None
        found = {}
        for line in summary.splitlines():
            for entry_id in (ADAPTER_A, ADAPTER_B):
                if str(entry_id) in line or ("0x%x" % entry_id) in line.lower():
                    current = entry_id
            for key in COUNTERS:
                marker = re.search(r"(?:RTWDiag)?" + re.escape(key.removeprefix("RTWDiag")) + r"\b", line)
                if marker and current is not None:
                    tail = re.split(r"(?:RTWDiag)?(?:RXRecovery|TXUSBTransfer|TXQueue|RXAggregate)",
                                    line[marker.end():], maxsplit=1)[0]
                    numbers = re.findall(r"(?<![\w])[-+]?\d+(?![\w])", tail)
                    if numbers:
                        found[current, key] = int(numbers[-1])
        for entry_id, values in self.harness.deltas.items():
            for key, delta in zip(COUNTERS, values):
                self.assertEqual(found.get((entry_id, key)), delta, summary)
        self.assert_archive_matches(report)
        self.assert_safe_commands()

    def test_redaction_covers_binary_mac_and_case_insensitive_ssid(self):
        report = self.harness.run()
        self.snapshots(report)
        combined = "\n".join(path.read_text(errors="replace") for path in report.rglob("*")
                             if path.is_file() and path.suffix in (".txt", ".log", ".json", ".tsv"))
        for secret in SECRETS:
            self.assertNotIn(secret.lower(), combined.lower(), "Unredacted fixture: " + secret)
        with zipfile.ZipFile(self.harness.archive) as archive:
            for name in archive.namelist():
                if name.endswith((".txt", ".log", ".json", ".tsv")):
                    contents = archive.read(name).decode(errors="replace").lower()
                    for secret in SECRETS:
                        self.assertNotIn(secret.lower(), contents, "Unredacted ZIP member: " + name)
        self.assertIn("RTWDiagRXRecoveryErrors", combined)
        self.assert_archive_matches(report)

    def test_reset_and_disappeared_registry_ids_remain_explicit(self):
        original = self.harness.before[0]
        self.harness.after = [adapter(ADAPTER_A, original["family"], original["interface"],
                                      original["vid"], original["pid"], [1] * len(COUNTERS))]
        report = self.harness.run()
        before, after = self.snapshots(report)
        self.assertEqual(set(before), {ADAPTER_A, ADAPTER_B})
        self.assertEqual(set(after), {ADAPTER_A})
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertRegex(summary.lower(), r"reset|reinici|zerou")
        self.assertRegex(summary.lower(), r"disappear|ausent|remov|desconect|missing")
        self.assertTrue(str(ADAPTER_A) in summary or ("0x%x" % ADAPTER_A) in summary.lower())
        self.assertTrue(str(ADAPTER_B) in summary or ("0x%x" % ADAPTER_B) in summary.lower())
        self.assertNotRegex(summary, r"(?:delta|diferen[cç]a)\s*[=:]?\s*-\d")
        self.assert_archive_matches(report)

    def test_command_timeout_preserves_partial_output_and_packages(self):
        report = self.harness.run("--command-timeout", "1", slow="hardware")
        self.assertLess(self.harness.elapsed, 12)
        self.snapshots(report)
        with (report / "STATUS.tsv").open() as stream:
            rows = list(csv.reader(stream, delimiter="\t"))
        matching = [row for row in rows[1:] if row[0] == "system/hardware.txt"]
        self.assertEqual(len(matching), 1, rows)
        self.assertEqual(matching[0][2], "124", matching)
        hardware = (report / "system/hardware.txt").read_text()
        self.assertIn("hardware stdout captured", hardware)
        self.assertIn("hardware stderr captured", hardware)
        self.assertIn("Realtek partial stdout", (report / "logs/USB_Realtek.log").read_text())
        self.assert_archive_matches(report)
        self.assert_safe_commands()

    def test_unified_log_timeout_preserves_partial_categories(self):
        report = self.harness.run("--command-timeout", "1", slow="log")
        self.assertLess(self.harness.elapsed, 12)
        with (report / "STATUS.tsv").open() as stream:
            rows = list(csv.reader(stream, delimiter="\t"))
        self.assertTrue(any(row[0].startswith("logs/") and row[2] == "124" for row in rows[1:]), rows)
        self.assertIn("Realtek partial stdout", (report / "logs/USB_Realtek.log").read_text())
        self.assertIn("Realtek partial stderr", (report / "logs/log-show.stderr.txt").read_text())
        self.snapshots(report)
        self.assert_archive_matches(report)

    def status_rows(self, report):
        with (report / "STATUS.tsv").open() as stream:
            return list(csv.DictReader(stream, delimiter="\t"))

    def log_commands(self):
        return [line.split("\t", 1)[1] for line in self.harness.trace.read_text().splitlines()
                if line.startswith("log\t")]

    def coverage(self, report, recent=False):
        directory = report / "logs/recent" if recent else report / "logs"
        path = directory / "unified-coverage.json"
        self.assertTrue(path.is_file(), "Collector did not record query coverage: " + str(path))
        return json.loads(path.read_text())

    def test_gatekeeper_disabled_state_preserves_exit_code_without_failure(self):
        report = self.harness.run(gatekeeper="disabled")
        row = next(row for row in self.status_rows(report) if row["file"] == "system/gatekeeper.txt")
        self.assertEqual(row["result"], "expected_state")
        self.assertEqual(row["exit_status"], "1")
        self.assertEqual((report / "system/gatekeeper.txt").read_text(), "assessments disabled\n")
        self.assertIn("Unavailable / denied / failed commands: 0", (report / "RESULT.txt").read_text())
        self.assert_safe_commands()

    def test_gatekeeper_query_error_remains_a_collection_failure(self):
        report = self.harness.run(gatekeeper="error")
        row = next(row for row in self.status_rows(report) if row["file"] == "system/gatekeeper.txt")
        self.assertEqual((row["result"], row["exit_status"]), ("failed", "1"))
        self.assertIn("assessment query failed", (report / "system/gatekeeper.txt").read_text())
        self.assertIn("Unavailable / denied / failed commands: 1", (report / "RESULT.txt").read_text())
        self.assert_safe_commands()

    def test_gatekeeper_disabled_state_requires_exact_output_and_exit_one(self):
        for scenario, expected_rc in (("disabled_extra", "1"), ("disabled_blank", "1"),
                                      ("disabled_rc2", "2")):
            with self.subTest(scenario=scenario):
                harness = CollectorHarness()
                self.addCleanup(harness.close)
                report = harness.run(gatekeeper=scenario)
                row = next(row for row in self.status_rows(report) if row["file"] == "system/gatekeeper.txt")
                self.assertEqual((row["result"], row["exit_status"]), ("failed", expected_rc))
                self.assertIn("Unavailable / denied / failed commands: 1", (report / "RESULT.txt").read_text())

    def test_successful_unified_query_records_coverage_without_retry(self):
        report = self.harness.run()
        commands = self.log_commands()
        self.assertEqual(len(commands), 1, commands)
        self.assertIn("--last 30m", commands[0])
        self.assertFalse((report / "logs/recent").exists())
        coverage = self.coverage(report)
        self.assertEqual(coverage["requested_window_minutes"], 30)
        self.assertEqual(coverage["command_exit_status"], 0)
        self.assertEqual(coverage["capture_status"], "query_completed")
        self.assertEqual(coverage["first_received_timestamp"], "2026-11-01 18:00:00.100000-0500")
        self.assertEqual(coverage["last_received_timestamp"], "2026-11-01 18:01:00.200000-0500")
        row = next(row for row in self.status_rows(report) if row["file"] == "system/gatekeeper.txt")
        self.assertEqual((row["result"], row["exit_status"]), ("ok", "0"))
        self.assert_safe_commands()

    def test_historical_timeout_captures_recent_window_without_completing_history(self):
        report = self.harness.run("--hours", "1", "--debug", "--command-timeout", "1",
                                  log_scenario="history_timeout_recent_ok")
        commands = self.log_commands()
        self.assertEqual(len(commands), 2, commands)
        self.assertIn("--last 60m", commands[0])
        self.assertIn("--last 2m", commands[1])
        self.assertEqual(re.sub(r"--last \S+", "--last WINDOW", commands[0]),
                         re.sub(r"--last \S+", "--last WINDOW", commands[1]))
        self.assertTrue(all("--debug" in command and "--info" in command for command in commands))
        rows = {row["file"]: row for row in self.status_rows(report)}
        self.assertEqual((rows["logs/unified-onepass"]["result"], rows["logs/unified-onepass"]["exit_status"]),
                         ("failed", "124"))
        self.assertEqual((rows["logs/recent/unified-onepass"]["result"], rows["logs/recent/unified-onepass"]["exit_status"]),
                         ("ok", "0"))
        main, recent = self.coverage(report), self.coverage(report, recent=True)
        self.assertEqual((main["requested_window_minutes"], main["command_exit_status"], main["capture_status"]),
                         (60, 124, "partial_or_unavailable"))
        self.assertEqual((recent["requested_window_minutes"], recent["command_exit_status"], recent["capture_status"]),
                         (2, 0, "query_completed"))
        self.assertEqual(main["last_received_timestamp"], "2026-11-01 18:01:00.200000-0500")
        self.assertEqual(recent["first_received_timestamp"], "2026-11-01 18:29:00.100000-0500")
        self.assertEqual(recent["last_received_timestamp"], "2026-11-01 18:29:30.200000-0500")
        self.assertEqual(main["present_time_coverage"], "not_established")
        self.assertEqual(recent["present_time_coverage"], "not_established")
        historical = (report / "logs/USB_Realtek.log").read_text()
        recent_log = (report / "logs/recent/USB_Realtek.log").read_text()
        self.assertIn("historical-window first", historical)
        self.assertNotIn("recent-window", historical)
        self.assertIn("recent-window first", recent_log)
        self.assertNotIn("historical-window", recent_log)
        self.assertIn("partial", (report / "logs/unified-summary.txt").read_text().lower())
        self.assertIn("Unavailable / denied / failed commands: 1", (report / "RESULT.txt").read_text())
        with zipfile.ZipFile(self.harness.archive) as archive:
            for path in ("logs/recent/USB_Realtek.log", "logs/recent/unified-coverage.json"):
                text = archive.read(report.name + "/" + path).decode()
                self.assertEqual(text, (report / path).read_text())
                for secret in SECRETS:
                    self.assertNotIn(secret.lower(), text.lower())
        self.assert_safe_commands()

    def test_recent_timeout_remains_partial_and_stops_after_two_queries(self):
        report = self.harness.run("--command-timeout", "1", log_scenario="both_timeout")
        self.assertLess(self.harness.elapsed, 12)
        self.assertEqual(len(self.log_commands()), 2)
        for recent in (False, True):
            coverage = self.coverage(report, recent=recent)
            self.assertEqual((coverage["command_exit_status"], coverage["capture_status"]),
                             (124, "partial_or_unavailable"))
            self.assertEqual(coverage["present_time_coverage"], "not_established")
        rows = [row for row in self.status_rows(report) if row["file"].endswith("unified-onepass")]
        self.assertEqual(len(rows), 2, rows)
        self.assertTrue(all(row["exit_status"] == "124" for row in rows), rows)
        self.assertIn("Unavailable / denied / failed commands: 2", (report / "RESULT.txt").read_text())
        self.assert_safe_commands()

    def test_non_timeout_unified_failure_is_not_retried(self):
        report = self.harness.run(log_scenario="error")
        self.assertEqual(len(self.log_commands()), 1)
        self.assertFalse((report / "logs/recent").exists())
        coverage = self.coverage(report)
        self.assertEqual((coverage["command_exit_status"], coverage["capture_status"]),
                         (77, "partial_or_unavailable"))
        self.assertIsNone(coverage["first_received_timestamp"])
        self.assertIsNone(coverage["last_received_timestamp"])
        self.assert_safe_commands()

    def test_empty_partial_unified_capture_does_not_claim_absence_of_events(self):
        report = self.harness.run("--command-timeout", "1", log_scenario="empty_timeouts")
        self.assertEqual(len(self.log_commands()), 2)
        for recent in (False, True):
            coverage = self.coverage(report, recent=recent)
            self.assertIsNone(coverage["first_received_timestamp"])
            self.assertIsNone(coverage["last_received_timestamp"])
            self.assertEqual(coverage["capture_status"], "partial_or_unavailable")
            directory = report / "logs/recent" if recent else report / "logs"
            for name in ("USB_Realtek", "WiFi_80211", "Network_Ethernet"):
                text = (directory / (name + ".log")).read_text()
                self.assertNotIn("No matching events were found", text)
                self.assertIn("No matching line received", text)
            summary = (directory / "unified-summary.txt").read_text().lower()
            self.assertIn("unavailable", summary)
            self.assertIn("partial", summary)
        self.assert_safe_commands()

    def test_recent_fallback_uses_the_same_administrative_read_path(self):
        report = self.harness.run("--admin", "--command-timeout", "1",
                                  log_scenario="history_timeout_recent_ok", shim_admin=True)
        self.assertEqual(len(self.log_commands()), 2)
        sudo_reads = [line for line in self.harness.trace.read_text().splitlines()
                      if line.startswith("sudo-invocation\t-n\t") and "\tlog\tshow\t" in line]
        self.assertEqual(len(sudo_reads), 2, sudo_reads)
        self.assertEqual(self.coverage(report)["command_exit_status"], 124)
        self.assertEqual(self.coverage(report, recent=True)["command_exit_status"], 0)

    def test_timeout_kills_owned_descendants_and_preserves_unrelated_process(self):
        unrelated = subprocess.Popen(["/bin/sleep", "15"])
        try:
            report = self.harness.run("--command-timeout", "1", slow="descendants")
            self.assertLess(self.harness.elapsed, 12)
            self.assertIsNone(unrelated.poll(), "Collector killed an unrelated process outside its command group")
            for name in ("command-pid", "descendant-pid"):
                pid = int((self.harness.state / name).read_text())
                state = subprocess.run(["/bin/ps", "-p", str(pid), "-o", "stat="],
                                       capture_output=True, text=True, timeout=3)
                self.assertTrue(state.returncode == 1 or state.stdout.strip().startswith("Z"),
                                "Timed-out process still alive: %s %s" % (name, state.stdout))
            with (report / "STATUS.tsv").open() as stream:
                rows = list(csv.reader(stream, delimiter="\t"))
            self.assertTrue(any(row[0] == "system/hardware.txt" and row[2] == "124" for row in rows[1:]), rows)
            self.assert_archive_matches(report)
        finally:
            if unrelated.poll() is None:
                unrelated.terminate()
            unrelated.wait(timeout=3)

    def assert_failed_second_snapshot(self, fault):
        if fault == "partial":
            self.harness.after.insert(0, adapter(0x100000333, "RTL8814AU", "en9", 3034, 34835,
                                                  [1] * len(COUNTERS)))
        report = self.harness.run(second_ioreg=fault)
        before, _ = self.snapshots(report)
        self.assertEqual(set(before), {ADAPTER_A, ADAPTER_B})
        with (report / "STATUS.tsv").open() as stream:
            rows = list(csv.reader(stream, delimiter="\t"))
        self.assertTrue(any(row[0] == "drivers/RTW-after.txt" and row[2] == "77" for row in rows[1:]), rows)
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertRegex(summary.lower(), r"indispon|parcial|incomplet|unavailable|partial|falh|failed")
        self.assertNotRegex(summary, r"(?im)^Removed/missing|^New Registry ID|^RTWDiag[^\n]*reset/restarted")
        self.assert_archive_matches(report)

    def test_empty_failed_second_snapshot_does_not_claim_removal(self):
        self.assert_failed_second_snapshot("empty")

    def test_partial_failed_second_snapshot_does_not_claim_removal(self):
        self.assert_failed_second_snapshot("partial")

    def test_timeout_range_rejects_invalid_values_before_capture(self):
        for value in ("0", "121", "-1", "nope"):
            with self.subTest(value=value):
                env = os.environ.copy()
                env.update(HOME=str(self.harness.home), PATH=str(self.harness.tools) + ":/usr/bin:/bin:/usr/sbin:/sbin",
                           SHIM_TRACE=str(self.harness.trace), SHIM_STATE=str(self.harness.state))
                result = subprocess.run(["/bin/bash", str(self.harness.script), "--no-pause",
                                         "--command-timeout", value], env=env,
                                        capture_output=True, text=True, timeout=5)
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertFalse(self.harness.default_destination.exists())


if __name__ == "__main__":
    unittest.main()
