#!/usr/bin/env python3
"""Read-only Tahoe kext diagnostics; never install, stage, load, or sign code.

Exit codes: 0 static checks pass and this build is observed loaded/attached;
1 static validation failed; 2 static checks pass, runtime not validated;
3 a required diagnostic is unavailable/failed to run. Connectivity is never
tested. A built exportable bundle can be packaged independently of exit 2.
Some authentication/staging tests require root and are intentionally excluded.
"""
from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import pathlib
import platform
import plistlib
import re
import struct
import subprocess
import sys
import uuid
from dataclasses import dataclass
from typing import Callable, Optional

BUNDLE_ID = "org.research.TahoeRealtekWiFi"
SERVICE_CLASS = "RTWUSBController"
MAX_BINARY_SIZE = 128 * 1024 * 1024


@dataclass
class CommandResult:
    argv: list[str]
    returncode: Optional[int]
    stdout: str = ""
    stderr: str = ""
    error: Optional[str] = None

    @property
    def text(self) -> str:
        return self.stdout + "\n" + self.stderr


def run_command(argv: list[str]) -> CommandResult:
    """Only invoked with the six fixed read-only commands defined below."""
    try:
        result = subprocess.run(argv, capture_output=True, text=True, errors="replace",
                                timeout=30, check=False)
        return CommandResult(argv, result.returncode, result.stdout, result.stderr)
    except (OSError, subprocess.TimeoutExpired) as error:
        return CommandResult(argv, None, error=str(error))


def _slice_macho(data: bytes, architecture: str) -> bytes:
    if len(data) < 4:
        raise ValueError("Truncated Mach-O header")
    fat_magic = {b"\xca\xfe\xba\xbe": (">", False), b"\xbe\xba\xfe\xca": ("<", False),
                 b"\xca\xfe\xba\xbf": (">", True), b"\xbf\xba\xfe\xca": ("<", True)}
    if data[:4] not in fat_magic:
        return data
    endian, wide = fat_magic[data[:4]]
    if len(data) < 8:
        raise ValueError("Truncated universal Mach-O header")
    count = struct.unpack_from(endian + "I", data, 4)[0]
    entry_size = 32 if wide else 20
    if not count or count > 32 or 8 + count * entry_size > len(data):
        raise ValueError("Invalid universal architecture table")
    desired = 0x01000007 if architecture == "x86_64" else 0x0100000C
    for index in range(count):
        fields = struct.unpack_from(endian + ("IIQQII" if wide else "IIIII"),
                                    data, 8 + index * entry_size)
        cpu, subtype, offset, size = fields[:4]
        if offset < 8 + count * entry_size or size < 32 or offset + size > len(data):
            raise ValueError("Universal Mach-O slice is outside the file")
        if cpu == desired and (architecture != "arm64e" or (subtype & 0xFFFFFF) == 2):
            return data[offset:offset + size]
    raise ValueError("No Mach-O slice for host architecture " + architecture)


def inspect_macho(data: bytes, architecture: str) -> dict:
    """Bounds-check a 64-bit kext's commands; do not claim signature validity."""
    image = _slice_macho(data, architecture)
    endian = {b"\xcf\xfa\xed\xfe": "<", b"\xfe\xed\xfa\xcf": ">"}.get(image[:4])
    if not endian or len(image) < 32:
        raise ValueError("Executable is not a 64-bit Mach-O")
    _, cpu, subtype, filetype, count, command_bytes, _, _ = struct.unpack_from(endian + "8I", image)
    actual_arch = {0x01000007: "x86_64", 0x0100000C: "arm64"}.get(cpu, "unknown")
    if actual_arch == "arm64" and (subtype & 0xFFFFFF) == 2:
        actual_arch = "arm64e"
    if actual_arch != architecture:
        raise ValueError("Mach-O architecture %s does not match %s" % (actual_arch, architecture))
    if filetype != 11:
        raise ValueError("Mach-O file type is not KEXTBUNDLE")
    if not count or count > 65536 or command_bytes > len(image) - 32:
        raise ValueError("Invalid Mach-O load-command table")
    offset, end = 32, 32 + command_bytes
    signature = {"status": "unsigned", "verified": False}
    image_uuid = None
    for _ in range(count):
        if offset + 8 > end:
            raise ValueError("Truncated Mach-O load command")
        command, size = struct.unpack_from(endian + "II", image, offset)
        if size < 8 or size % 8 or offset + size > end:
            raise ValueError("Invalid Mach-O load-command size")
        if command == 0x1B:
            if size != 24 or image_uuid is not None:
                raise ValueError("Invalid or duplicate Mach-O UUID command")
            image_uuid = str(uuid.UUID(bytes=image[offset + 8:offset + 24])).upper()
        elif command == 0x1D:
            if size != 16 or signature["status"] != "unsigned":
                raise ValueError("Invalid or duplicate code-signature command")
            location, signature_size = struct.unpack_from(endian + "II", image, offset + 8)
            if location < end or signature_size < 8 or location + signature_size > len(image):
                raise ValueError("Code-signature data is outside the Mach-O slice")
            signature = {"status": "present_unverified", "verified": False,
                         "size": signature_size}
        offset += size
    if offset != end:
        raise ValueError("Mach-O command count does not consume the command table")
    return {"status": "passed", "architecture": actual_arch, "filetype": "KEXTBUNDLE",
            "uuid": image_uuid, "signature": signature,
            "signature_note": "Signature bytes are inspected only; kmutil supplies authentication diagnostics."}


def _error_lines(text: str) -> list[str]:
    patterns = (r"^\s*Error(?:\s|:)", r"\bFilesystem error:",
                r"\b(?:Authentication|Validation) failed\b", r"Authenticating extension failed",
                r"Unable to resolve dependencies", r"\bnot (?:loadable|authentic|valid)\b",
                r"\b(?:Undefined|unresolved|multiply[- ]defined) symbols\b",
                r"not found in any library")
    return [line.strip() for line in text.splitlines()
            if any(re.search(pattern, line, re.IGNORECASE) for pattern in patterns)]


def _categories(text: str) -> list[str]:
    patterns = {"ownership": r"invalid ownership|incorrect owner|not owned by|ownership.*(?:root|0:0)",
                "signature": r"(?:bad|invalid|missing) code signature|not signed|unsigned",
                "dependencies": r"dependenc|undefined symbols|unresolved symbols|not found in any library",
                "authentication": r"authenticat.*fail|not authentic",
                "validation": r"validation failed|not valid|not loadable",
                "approval": r"approval (?:is )?required|not approved|authorization failed"}
    return sorted(name for name, pattern in patterns.items() if re.search(pattern, text, re.I))


def evaluate_kmutil(result: CommandResult, kind: str) -> dict:
    messages = _error_lines(result.text)
    unsupported = re.search(r"not a supported kmutil mode|unknown (?:subcommand|command|option)|"
                            r"unrecognized (?:subcommand|command|option)|unsupported (?:mode|command)",
                            result.text, re.I)
    if result.error or result.returncode is None or result.returncode < 0 or unsupported:
        status = "error"
    elif result.returncode != 0 or messages:
        status = "failed"
    else:
        status = "passed"
    check = {"status": status, "exit_code": result.returncode,
             "categories": _categories("\n".join(messages)), "messages": messages[:12]}
    if result.error:
        check["messages"].append(result.error)
    if unsupported:
        check["messages"].append("Diagnostic mode is unsupported; a zero exit is not validation.")
    if result.returncode is not None and result.returncode < 0:
        check["messages"].append("Diagnostic terminated by signal %d." % -result.returncode)
    if kind == "libraries" and status == "passed":
        start = result.stdout.find("<?xml")
        if start >= 0:
            try:
                payload = plistlib.loads(result.stdout[start:].encode())
                if not isinstance(payload, dict) or not isinstance(payload.get("OSBundleLibraries"), dict):
                    raise ValueError("Library XML is not a dependency dictionary.")
                check["suggested_dependencies"] = payload["OSBundleLibraries"]
            except (ValueError, plistlib.InvalidFileException):
                check["status"] = "error"
                check["messages"].append("Unable to parse the library XML response.")
        else:
            check["status"] = "error"
            check["messages"].append("Library diagnostic did not return its requested XML response.")
    elif kind == "diagnostics" and status == "passed" and not re.search(
            r"Diagnosing\s|\[" + re.escape(BUNDLE_ID) + r"\]", result.text):
        check["status"] = "error"
        check["messages"].append("Diagnostic did not identify a kext it examined.")
    return check


def loaded_state(result: CommandResult, bundle_id: str, artifact_uuid: Optional[str]) -> dict:
    if result.error or result.returncode != 0:
        return {"status": "unknown", "exit_code": result.returncode,
                "artifact_match": "unknown", "message": result.error or "showloaded did not complete successfully."}
    # Match a load-information row and the exact Name token, never a substring
    # in another identifier, header, warning, or diagnostic message.
    row = re.compile(r"^\s*\d+\s+\d+\s+0x[0-9a-f]+\s+0x[0-9a-f]+\s+0x[0-9a-f]+\s+"
                     r"(?:\S+\s+)?(?P<name>[\w.-]+)\s+\([^)]+\)(?P<rest>.*)$", re.I)
    inventory_rows = 0
    for line in result.stdout.splitlines():
        match = row.match(line)
        if match:
            inventory_rows += 1
        if not match or match.group("name") != bundle_id:
            continue
        found_uuid = re.search(r"\b[0-9A-F]{8}(?:-[0-9A-F]{4}){3}-[0-9A-F]{12}\b", match.group("rest"), re.I)
        value = found_uuid.group().upper() if found_uuid else None
        same_build = "unknown" if not artifact_uuid or not value else (
            "match" if value == artifact_uuid.upper() else "mismatch")
        return {"status": "loaded", "exit_code": 0, "uuid": value, "artifact_match": same_build}
    if not inventory_rows and not re.search(r"\bName\s+\(Version\)", result.stdout):
        return {"status": "unknown", "exit_code": 0, "artifact_match": "unknown",
                "message": "showloaded returned neither a load-information header nor a matching row."}
    return {"status": "not_loaded", "exit_code": 0, "artifact_match": "not_observed"}


def attached_state(result: CommandResult, bundle_id: str) -> dict:
    if result.error or result.returncode != 0:
        return {"status": "unknown", "exit_code": result.returncode,
                "message": result.error or "ioreg did not complete successfully."}
    blocks = re.split(r"(?m)(?=^.*\+-o )", result.stdout)
    for block in blocks:
        first = block.splitlines()[0] if block.splitlines() else ""
        if not re.search(r"<class " + re.escape(SERVICE_CLASS) + r"\s*[,>]", first):
            continue
        if not all(re.search(r"\b" + state + r"\b", first) for state in ("registered", "matched", "active")):
            continue
        identity = re.search(r'"CFBundleIdentifier"\s*=\s*"([^"\n]+)"', block)
        if identity and identity.group(1) == bundle_id:
            return {"status": "attached", "exit_code": 0}
    return {"status": "not_attached", "exit_code": 0}


def _command_metadata(result: CommandResult) -> dict:
    value = {"argv": result.argv, "exit_code": result.returncode}
    if result.error:
        value["error"] = result.error
    return value


def provider_declarations(libraries: dict, declared) -> dict:
    """libraries/XML lists the kext's symbol providers, unlike the recursive
    dependency closure from print-diagnostics. Compare IDs only: current
    provider versions need not equal a valid requested compatibility minimum.
    """
    if libraries["status"] != "passed":
        return {"status": "error", "messages": ["Direct providers could not be established by libraries/XML."]}
    found = libraries.get("suggested_dependencies")
    if not isinstance(declared, dict) or not isinstance(found, dict) or any(
            not isinstance(name, str) or not name for name in list(declared) + list(found)):
        return {"status": "failed", "categories": ["dependencies"],
                "messages": ["OSBundleLibraries/provider identifiers are not a valid dictionary."]}
    missing = sorted(set(found) - set(declared))
    return {"status": "failed" if missing else "passed",
            "provider_ids": sorted(found), "missing_provider_ids": missing,
            "categories": ["dependencies"] if missing else [],
            "messages": ["Direct symbol provider is absent from OSBundleLibraries: " + name for name in missing],
            "scope": "Identifier completeness only; version compatibility is diagnosed by kmutil."}


def validate(bundle: pathlib.Path, *, runner: Callable = run_command,
             architecture: Optional[str] = None) -> dict:
    bundle = pathlib.Path(bundle).resolve()
    architecture = architecture or platform.machine()
    report = {"schema_version": 1, "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "bundle_path": str(bundle), "host": {"system": platform.system(),
              "kernel_release": platform.release(), "architecture": architecture},
              "checks": {}, "commands": {}, "live_connectivity": "not_tested",
              "scope": "Read-only diagnostics without staging, installation, kernel loading, or USB I/O."}
    checks, commands = report["checks"], report["commands"]
    report["_raw_results"] = {}

    def run(name, argv):
        result = runner(argv)
        commands[name] = _command_metadata(result)
        report["_raw_results"][name] = result
        return result

    info_path = bundle / "Contents/Info.plist"
    executable = None
    try:
        info = plistlib.loads(info_path.read_bytes())
        if not isinstance(info, dict):
            raise ValueError("Bundle Info.plist is not a dictionary.")
        name = info.get("CFBundleExecutable")
        if (info.get("CFBundleIdentifier") != BUNDLE_ID or info.get("CFBundlePackageType") != "KEXT" or
            not isinstance(name, str) or not name or name in (".", "..") or "/" in name or "\\" in name):
            raise ValueError("Bundle identity/type or executable name is invalid.")
        report["bundle_identifier"] = BUNDLE_ID
        report["bundle_version"] = info.get("CFBundleVersion")
        report["declared_dependencies"] = info.get("OSBundleLibraries", {})
        executable = bundle / "Contents/MacOS" / name
        result = run("plutil", ["/usr/bin/plutil", "-lint", str(info_path)])
        if result.error or result.returncode is None or result.returncode < 0:
            checks["plist"] = {"status": "error", "messages": [result.error or "plutil did not complete."]}
        else:
            passed = result.returncode == 0 and bool(re.search(r": OK\s*$", result.stdout))
            checks["plist"] = {"status": "passed" if passed else "failed", "exit_code": result.returncode,
                               "messages": [] if passed else [result.text.strip()]}
    except (OSError, ValueError, plistlib.InvalidFileException) as error:
        checks["plist"] = {"status": "failed", "messages": [str(error)]}

    if executable is not None:
        try:
            if executable.stat().st_size > MAX_BINARY_SIZE:
                raise ValueError("Executable exceeds the diagnostic size limit.")
            data = executable.read_bytes()
            checks["macho"] = inspect_macho(data, architecture)
            report["executable_sha256"] = hashlib.sha256(data).hexdigest()
            owner = executable.stat()
            report["executable_ownership"] = {"uid": owner.st_uid, "gid": owner.st_gid,
                                             "mode": oct(owner.st_mode & 0o7777)}
            result = run("file", ["/usr/bin/file", str(executable)])
            commands["file"]["description"] = result.stdout.strip()
            if result.error or result.returncode != 0:
                checks["macho"]["status"] = "error"
                checks["macho"]["messages"] = [result.error or "file command did not complete."]
        except (OSError, ValueError, struct.error) as error:
            checks["macho"] = {"status": "failed", "messages": [str(error)]}
    else:
        checks["macho"] = {"status": "failed", "messages": ["No valid bundle executable was found."]}

    for name, arguments in (("libraries", ["libraries", "-p", str(bundle), "-a", architecture, "--xml", "-c"]),
                            ("diagnostics", ["print-diagnostics", "-p", str(bundle), "-a", architecture, "-v"])):
        checks[name] = evaluate_kmutil(run(name, ["/usr/bin/kmutil"] + arguments), name)
    checks["provider_declarations"] = provider_declarations(checks["libraries"],
                                                           report.get("declared_dependencies"))
    # Tahoe's filtered --arch-info invocation can crash; --list-only returns
    # the complete inventory and UUIDs. loaded_state matches the exact ID.
    loaded = loaded_state(run("showloaded", ["/usr/bin/kmutil", "showloaded", "--list-only"]),
                          BUNDLE_ID, checks["macho"].get("uuid"))
    attached = attached_state(run("ioreg", ["/usr/sbin/ioreg", "-r", "-c", SERVICE_CLASS, "-l", "-w0"]), BUNDLE_ID)
    report["runtime"] = {"loaded": loaded, "attached": attached}
    report["static_validation_passed"] = all(check["status"] == "passed" for check in checks.values())
    if any(check["status"] == "failed" for check in checks.values()):
        report.update(status="static_validation_failed", exit_code=1)
    elif any(check["status"] == "error" for check in checks.values()) or any(
            value["status"] == "unknown" for value in (loaded, attached)):
        report.update(status="diagnostic_error", exit_code=3)
    elif loaded["status"] == "loaded" and loaded["artifact_match"] == "match" and attached["status"] == "attached":
        report.update(status="kernel_attachment_observed", exit_code=0)
    else:
        report.update(status="runtime_not_validated", exit_code=2)
    return report


def prepare_log_directory(directory: pathlib.Path, bundle: pathlib.Path) -> pathlib.Path:
    directory, bundle = pathlib.Path(directory).resolve(), pathlib.Path(bundle).resolve()
    if directory == bundle or bundle in directory.parents:
        raise ValueError("Evidence directory must be outside the validated kext.")
    if directory.exists() and (not directory.is_dir() or any(directory.iterdir())):
        raise ValueError("Evidence directory must be new or empty; existing evidence is never overwritten.")
    directory.mkdir(parents=True, exist_ok=True)
    return directory


def write_evidence(report: dict, directory: pathlib.Path) -> None:
    for name, result in report["_raw_results"].items():
        stdout_name, stderr_name = name + ".stdout.txt", name + ".stderr.txt"
        (directory / stdout_name).write_text(result.stdout, encoding="utf-8")
        (directory / stderr_name).write_text(result.stderr, encoding="utf-8")
        report["commands"][name]["stdout_log"] = stdout_name
        report["commands"][name]["stderr_log"] = stderr_name
    report["log_directory"] = str(directory)
    public = {key: value for key, value in report.items() if not key.startswith("_")}
    (directory / "report.json").write_text(json.dumps(public, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def print_report(report: dict) -> None:
    print("Tahoe kext validation: " + report["status"])
    print("Bundle: " + report["bundle_path"])
    print("Static: " + ", ".join(name + "=" + value["status"] for name, value in report["checks"].items()))
    signature = report["checks"]["macho"].get("signature", {})
    if signature:
        print("Signature metadata: " + signature["status"] + " (cryptographic validity not established)")
    for name, check in report["checks"].items():
        if check["status"] != "passed":
            categories = ", ".join(check.get("categories", []))
            messages = check.get("messages", [])
            print("  " + name + ": " + (categories or (messages[0] if messages else check["status"])))
    loaded, attached = report["runtime"]["loaded"], report["runtime"]["attached"]
    print("Runtime: %s; %s; artifact=%s. Live connectivity: not tested." %
          (loaded["status"], attached["status"], loaded["artifact_match"]))
    if "log_directory" in report:
        print("Evidence: " + report["log_directory"])
    print("Exit %d: no installation, staging, loading, or USB I/O performed." % report["exit_code"])


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("kext", type=pathlib.Path, help="Path to the built TahoeRealtekWiFi.kext")
    parser.add_argument("--json", action="store_true", help="Emit a single JSON report instead of prose")
    parser.add_argument("--log-dir", type=pathlib.Path, help="New/empty evidence directory outside the kext")
    args = parser.parse_args(argv)
    if platform.system() != "Darwin" or os.geteuid() == 0:
        message = "Run this diagnostic as a regular user on macOS, without sudo."
        if args.json:
            print(json.dumps({"schema_version": 1, "status": "diagnostic_error", "exit_code": 3, "message": message}))
        else:
            print(message, file=sys.stderr)
        return 3
    try:
        directory = prepare_log_directory(args.log_dir, args.kext) if args.log_dir else None
        report = validate(args.kext)
        if directory:
            write_evidence(report, directory)
    except (OSError, ValueError) as error:
        if args.json:
            print(json.dumps({"schema_version": 1, "status": "diagnostic_error", "exit_code": 3, "message": str(error)}))
        else:
            print("Diagnostic error: " + str(error), file=sys.stderr)
        return 3
    if args.json:
        print(json.dumps({key: value for key, value in report.items() if not key.startswith("_")}, indent=2, sort_keys=True))
    else:
        print_report(report)
    return report["exit_code"]


if __name__ == "__main__":
    raise SystemExit(main())
