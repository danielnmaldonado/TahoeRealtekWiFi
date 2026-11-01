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
      *IOUSBHostDevice*)
        printf '+-o Realtek USB <class IOUSBHostDevice, id 0x99, registered>\n  "idVendor" = 3034\n  "idProduct" = 34835\n';;
      *) echo 'IORegistry fixture';;
    esac;;
  log)
    [ "${1:-}" = show ] || { echo FORBIDDEN_LOG_MUTATION >&2; exit 97; }
    echo 'Realtek USB WiFi en7 sSiD = "SecretMixedCaseNetwork"'
    echo 'WiFi IOMACAddress = <aabbccddeeff> ether aa:bb:cc:dd:ee:ff'
    echo 'Network WiFi Password = "SuperSecretPassword" contact private-person@example.invalid'
    echo 'Realtek partial stdout before completion'
    echo 'Realtek partial stderr before completion' >&2
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
  spctl) [ "${1:-}" = --status ] || exit 97; echo 'assessments enabled';;
  nvram) [ "${1:-}" = boot-args ] || exit 97; echo 'boot-args fixture';;
  pmset) [ "${1:-}" = -g ] || exit 97; echo 'power fixture';;
  kmutil) [ "${1:-}" = showloaded ] || exit 97; echo 'org.research.TahoeRealtekWiFi';;
  kextstat) [ "${1:-}" = -l ] || exit 97; echo 'org.research.TahoeRealtekWiFi';;
  systemextensionsctl) [ "${1:-}" = list ] || exit 97; echo 'No system extensions';;
  ls) echo 'fixture directory listing';;
  sw_vers) printf 'ProductName: macOS\nProductVersion: 26.0\n';;
  sudo|curl|ping|networkQuality|kextload|kextunload|osascript)
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

    def run(self, *arguments, default=False, slow="", second_ioreg=""):
        self.before_file.write_text(registry_text(self.before))
        self.after_file.write_text(registry_text(self.after))
        env = os.environ.copy()
        env.update(HOME=str(self.home), PATH=str(self.tools) + ":/usr/bin:/bin:/usr/sbin:/sbin",
                   SHIM_TRACE=str(self.trace), SHIM_STATE=str(self.state),
                   SHIM_BEFORE=str(self.before_file), SHIM_AFTER=str(self.after_file),
                   SHIM_SLOW_COMMAND=slow, SHIM_SECOND_IOREG=second_ioreg)
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
                             "RTW-SUMMARY.txt", "STATUS.tsv", "logs/USB_Realtek.log"):
                self.assertEqual(archive.read(report.name + "/" + relative),
                                 (report / relative).read_bytes(), relative)

    def test_default_double_click_destination(self):
        report = self.harness.run(default=True)
        self.assertEqual(report.parent, self.harness.default_destination)
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
        self.assertIn("Publicacao: geracao avancou 10 -> 12", au)
        self.assertIn("RTWDiagPublicationMonotonicMS=1050", au)
        self.assertIn("Publicacao: frescor desconhecido", eu)
        self.assertIn("nao atomica", summary)
        self.assert_safe_commands()

    def test_unchanged_publication_generation_is_not_called_live(self):
        for entries in (self.harness.before, self.harness.after):
            for entry in entries:
                entry["values"].update(RTWDiagPublicationGeneration=8,
                                       RTWDiagPublicationMonotonicMS=555)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertEqual(summary.count("Publicacao: sem nova publicacao observada; geracao=8"), 2)
        self.assertNotIn("Publicacao: geracao avancou", summary)

    def test_regressed_or_incomplete_publication_is_unknown(self):
        for entry in self.harness.before:
            entry["values"].update(RTWDiagPublicationGeneration=12,
                                   RTWDiagPublicationMonotonicMS=5000)
        self.harness.after[0]["values"].update(RTWDiagPublicationGeneration=2,
                                              RTWDiagPublicationMonotonicMS=100)
        self.harness.after[1]["values"].update(RTWDiagPublicationGeneration=14)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertIn("Publicacao: geracao/tempo regrediu; comparacao indisponivel", summary)
        self.assertIn("Publicacao: frescor desconhecido", summary)
        self.assertNotIn("Publicacao: geracao avancou", summary)

    def test_odd_publication_generation_is_in_progress(self):
        for entry in self.harness.before:
            entry["values"].update(RTWDiagPublicationGeneration=10,
                                   RTWDiagPublicationMonotonicMS=1000)
        for entry in self.harness.after:
            entry["values"].update(RTWDiagPublicationGeneration=11,
                                   RTWDiagPublicationMonotonicMS=1000)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertEqual(summary.count("Publicacao: em andamento; comparacao indisponivel"), 2)
        self.assertNotIn("Publicacao: geracao avancou", summary)

    def test_invalid_native_ifnet_statistics_are_not_zero_traffic(self):
        for entry in self.harness.before:
            entry["values"].update(RTWDiagNativeIfnetStatsValid=1,
                                   RTWDiagNativeIfnetIndex=7,
                                   RTWDiagNativeIfnetInputPackets=200)
        for entry in self.harness.after:
            entry["values"].update(RTWDiagNativeIfnetStatsValid=0,
                                   RTWDiagNativeIfnetStatsError=6,
                                   RTWDiagNativeIfnetIndex=0,
                                   RTWDiagNativeIfnetInputPackets=0)
        report = self.harness.run()
        summary = (report / "RTW-SUMMARY.txt").read_text()
        self.assertIn("RTWDiagNativeIfnetStatsValid=0", summary)
        self.assertIn("RTWDiagNativeIfnetInputPackets leitura nativa indisponivel/incomparavel", summary)
        self.assertNotRegex(summary, r"RTWDiagNativeIfnetInputPackets[^\n]*delta=")

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
        self.assertNotRegex(summary, r"(?im)^Removido/ausente|^Novo Registry ID|^RTWDiag[^\n]*reset/reiniciado")
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
