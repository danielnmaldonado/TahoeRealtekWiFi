#!/usr/bin/env python3
"""Guard explicit WiFiApp ivar references without requiring a macOS SDK.

This source-level check catches stale self->_member accesses on portable CI.
It does not type-check AppKit or replace the native compile/link check.
"""
from pathlib import Path
import argparse
import re
import sys
import unittest

PROJECT = Path(__file__).resolve().parent.parent
SOURCE = PROJECT / "app/Main.mm"
NON_CODE = re.compile(
    r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
    re.DOTALL,
)


def code_only(source: str) -> str:
    """Mask comments/literals, retaining offsets for compiler-style diagnostics."""
    return NON_CODE.sub(
        lambda match: "".join("\n" if char == "\n" else " " for char in match[0]),
        source,
    )


def missing_members(source: str) -> list:
    code = code_only(source)
    interface = re.search(r"@interface\s+WiFiApp\b[^{}]*\{", code)
    implementation = re.search(r"@implementation\s+WiFiApp\b", code)
    if not interface or not implementation:
        raise ValueError("WiFiApp interface/implementation not found")

    start = interface.end()
    depth = 1
    end = start
    while end < len(code) and depth:
        if code[end] == "{":
            depth += 1
        elif code[end] == "}":
            depth -= 1
        end += 1
    if depth:
        raise ValueError("Unterminated WiFiApp ivar block")
    # The project's interface uses simple declarations, including comma lists.
    declarations = set(re.findall(
        r"\b(_[A-Za-z]\w*)\s*(?=[,;=\[])",
        code[start:end - 1],
    ))
    implementation_end = code.find("@end", implementation.end())
    if implementation_end < 0:
        raise ValueError("Unterminated WiFiApp implementation")
    body = code[implementation.end():implementation_end]
    missing = []
    for reference in re.finditer(r"\bself\s*->\s*(_[A-Za-z]\w*)", body):
        name = reference[1]
        if name not in declarations:
            offset = implementation.end() + reference.start(1)
            line = code.count("\n", 0, offset) + 1
            missing.append((line, name))
    return missing


def fixture(declarations: str, body: str) -> str:
    return (
        "@interface WiFiApp : NSObject {\n" + declarations + "\n}\n@end\n"
        "@implementation WiFiApp\n- (void)check {\n" + body + "\n}\n@end\n"
    )


class SourceContractTests(unittest.TestCase):
    def test_real_app_uses_declared_members(self):
        source = SOURCE.read_text(encoding="utf-8")
        self.assertGreater(len(re.findall(r"\bself\s*->", code_only(source))), 0)
        missing = missing_members(source)
        errors = "\n".join(
            f"{SOURCE}:{line}: error: WiFiApp has no declared member '{name}'"
            for line, name in missing
        )
        self.assertEqual(missing, [], errors)

    def test_xpc_malformed_payload_is_erased_and_connection_closed(self):
        source = (PROJECT / "app/DriverConnection.mm").read_text(encoding="utf-8")
        body = source.split("const bool validPayload=", 1)[1].split("return 0;", 1)[0]
        self.assertIn("if(!validPayload)", body)
        self.assertIn("memset(output,0,capacity);", body)
        self.assertIn("[self close];", body)
        self.assertIn("return rtwui::protocolError;", body)
        self.assertLess(body.index("memset(output,0,capacity);"),
                        body.index("return rtwui::protocolError;"))

    def test_socket_exchange_clears_partial_or_invalid_replies(self):
        # Linux cannot compile the macOS transport implementation. This
        # source-level contract complements the native socketpair test.
        source = (PROJECT / "app/Transport.cpp").read_text(encoding="utf-8")
        body = source.split("uint32_t exchange(", 1)[1].split("bool makeJoin(", 1)[0]
        self.assertIn("if(!readExact(fd,out,cap))", body)
        self.assertIn("wipe(out,cap);", body)
        self.assertIn("if(!validPayload(selector,out))", body)
        policy = (PROJECT / "app/Protocol.hpp").read_text(encoding="utf-8")
        self.assertIn("validDevices(*static_cast<const DeviceList*>(output))", policy)
        self.assertIn("rtw_control_channel_map_valid", policy)
        self.assertIn("rtw_control_load32(static_cast<const uint8_t*>(output))", policy)
        partial = body.split("if(!readExact(fd,out,cap))", 1)[1].split("if(!validPayload(selector,out))", 1)[0]
        self.assertIn("wipe(out,cap);", partial)
        self.assertLess(partial.index("wipe(out,cap);"), partial.index("return transportError;"))
        payload_error = body.split("if(!validPayload(selector,out))", 1)[1]
        self.assertLess(payload_error.index("wipe(out,cap);"), payload_error.index("return protocolError;"))

    def test_delayed_auto_join_checks_current_user_choice(self):
        source = SOURCE.read_text(encoding="utf-8")
        body = source.split("- (void)loadPasswordForSSID:", 1)[1].split(
            "- (void)promptPasswordForSSID:", 1)[0]
        guard = "credentialJoinStillAllowed(automatic,self->_autoJoinEnabled,"
        self.assertIn(guard, body)
        self.assertIn("self->_autoJoin.paused", body)
        self.assertIn("RTWWipePassword(saved);self->_credentialBusy=NO;", body)
        self.assertLess(body.index(guard), body.index("if(useSaved&&validSaved)"))

    def test_comma_separated_and_template_declarations(self):
        source = fixture(
            "BOOL _joining,_busy;\nstd::atomic<uint64_t> _epoch;",
            "self->_joining=NO; self -> _busy=NO; self->_epoch.store(0);",
        )
        self.assertEqual(missing_members(source), [])

    def test_stale_member_is_rejected(self):
        source = fixture("Policy _autoJoin;", "self->_automaticAttempt=NO;")
        self.assertEqual([name for _, name in missing_members(source)],
                         ["_automaticAttempt"])

    def test_missing_field_inside_callback_is_rejected(self):
        source = fixture("BOOL _busy;",
                         "dispatch_async(queue, ^{self->_removed=NO;});")
        self.assertEqual([name for _, name in missing_members(source)], ["_removed"])

    def test_comments_and_literals_are_not_members(self):
        source = fixture(
            "BOOL _busy; // BOOL _removed;",
            'self->_busy=NO; @"self->_notCode"; '
            '/* self->_ignored=NO; */ self->_removed=NO;',
        )
        self.assertEqual([name for _, name in missing_members(source)], ["_removed"])

    def test_other_objects_and_classes_are_out_of_scope(self):
        source = fixture("BOOL _busy;", "other->_external=NO; self->_busy=NO;")
        source += "\n@implementation Other\n- (void)check {self->_external=NO;}\n@end\n"
        self.assertEqual(missing_members(source), [])

    def test_malformed_input_fails_closed(self):
        with self.assertRaises(ValueError):
            missing_members("@implementation WiFiApp\n@end")
        with self.assertRaises(ValueError):
            missing_members("@interface WiFiApp { BOOL _busy;\n@implementation WiFiApp\n@end")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, default=SOURCE)
    args = parser.parse_args()
    SOURCE = args.source.resolve()
    unittest.main(argv=[sys.argv[0]], verbosity=2)
