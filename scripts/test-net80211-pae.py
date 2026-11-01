#!/usr/bin/env python3
"""Offline actual-supplicant tests; no packet capture, injection, USB or kernel use."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags, dead_strip_flags
import argparse
import hashlib
import hmac
import re
import struct
import subprocess


def function(source, name):
    """Copy a complete upstream function verbatim into the host dependency closure."""
    position = source.index("\n" + name + "(") + 1
    start = source.rfind("\n", 0, position - 1) + 1
    opening = source.index("{", position)
    depth = 0
    # Ignore braces inside C strings/comments when locating the closing brace.
    tokens = re.finditer(r'/\*[\s\S]*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]', source[opening:])
    for token in tokens:
        if token.group() == "{": depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0: return source[start:opening + token.end()] + "\n"
    raise ValueError(name)


def derive(pmk, aa, spa, anonce, snonce):
    context = min(aa, spa) + max(aa, spa) + min(anonce, snonce) + max(anonce, snonce)
    return b"".join(hmac.new(pmk, b"Pairwise key expansion\0" + context + bytes([i]), hashlib.sha1).digest()
                    for i in range(4))[:64]


def wrap(kek, data):
    program = "const c=require('node:crypto');const x=c.createCipheriv('id-aes128-wrap',Buffer.from(process.argv[1],'hex'),Buffer.alloc(8,0xa6));process.stdout.write(Buffer.concat([x.update(Buffer.from(process.argv[2],'hex')),x.final()]));"
    return subprocess.run(["node", "-e", program, kek.hex(), data.hex()],
                          capture_output=True, check=True).stdout


def main():
    project = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=project / "out-net80211-pae")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    vendor = project / "vendor/itl80211/openbsd"
    host = project / "tests/net80211_pae_host"
    # Independent AP-side fixtures: Python HMAC/PRF and OpenSSL RFC3394 wrapping.
    pmk, snonce, anonce = bytes(range(32)), bytes(range(32, 64)), bytes(range(64, 96))
    aa, spa = bytes.fromhex("001122334455"), bytes.fromhex("66778899aabb")
    rsn = bytes.fromhex("30140100000fac040100000fac040100000fac020000")
    ptk = derive(pmk, aa, spa, anonce, snonce)
    new_anonce = bytes(range(96, 128))
    new_ptk = derive(pmk, aa, spa, new_anonce, snonce)

    def kde(gtk, kid): return bytes.fromhex("dd16000fac01") + bytes([kid, 0]) + gtk
    def packet(info, replay, nonce, data=b"", material=ptk, rsc=0):
        body = bytearray(99 + len(data))
        body[0:4] = bytes([1, 3]) + struct.pack(">H", len(body) - 4)
        body[4] = 2
        body[5:7] = struct.pack(">H", info)
        body[7:9] = struct.pack(">H", 16)
        body[9:17] = struct.pack(">Q", replay)
        body[17:49] = nonce
        body[65:73] = rsc.to_bytes(8, "little")
        body[97:99] = struct.pack(">H", len(data))
        body[99:] = data
        if info & 0x100:
            body[81:97] = hmac.new(material[:16], body, hashlib.sha1).digest()[:16]
        return spa + aa + bytes.fromhex("888e") + body

    gtk = bytes(range(0xa0, 0xb0))
    gtk2 = bytes(range(0xb0, 0xc0))
    data = wrap(ptk[16:32], rsn + kde(gtk, 1) + b"\xdd\0")
    fixtures = {
        "pmk": pmk, "snonce": snonce, "anonce": anonce, "ap": aa, "station": spa,
        "rsn": rsn, "ptk": ptk, "gtk": gtk, "gtk2": gtk2, "new_ptk": new_ptk,
        "msg1": packet(0x008a, 1, anonce),
        "msg3": packet(0x13ca, 2, anonce, data),
        "msg3_later": packet(0x13ca, 3, anonce, data),
        "msg3_bad_rsn": packet(0x13ca, 2, anonce, wrap(ptk[16:32], bytes([0x30, 0x14, 2]) + rsn[3:] + kde(gtk, 1) + b"\xdd\0")),
        "msg3_plain_gtk": packet(0x03ca, 2, anonce, rsn + kde(gtk, 1)),
        "msg3_bad_wrap": packet(0x13ca, 2, anonce, bytes([data[0] ^ 1]) + data[1:]),
        "msg3_wrong_nonce": packet(0x13ca, 2, bytes([anonce[0] ^ 1]) + anonce[1:], data),
        "group1": packet(0x1382, 4, bytes(32), wrap(ptk[16:32], kde(gtk2, 2)), rsc=7),
        "rekey1": packet(0x008a, 5, new_anonce),
        "rekey3": packet(0x13ca, 6, new_anonce, wrap(new_ptk[16:32], rsn + kde(gtk2, 1) + b"\xdd\0"), material=new_ptk),
    }
    header = "// Independently generated fixed AP-side test fixtures; no I/O transport.\n"
    for name, data in fixtures.items():
        header += "static const uint8_t fixture_" + name + "[] = {" + ",".join(hex(byte) for byte in data) + "};\n"
    (args.out / "pae_fixtures.hpp").write_text(header)
    selections = {
        "ieee80211_output.c": ["ieee80211_add_rsn_body", "ieee80211_add_rsn", "ieee80211_add_wpa"],
        "ieee80211_input.c": ["ieee80211_parse_rsn_cipher", "ieee80211_parse_rsn_akm", "ieee80211_parse_rsn_body", "ieee80211_parse_rsn"],
        "ieee80211_ioctl.c": ["ieee80211_disable_rsn"],
        "ieee80211_node.c": ["ieee80211_node_clear_rsn", "ieee80211_node_leave_rsn", "ieee80211_node_cleanup"],
    }
    closure = '#include <net80211/ieee80211_var.h>\n#include <net80211/ieee80211_priv.h>\n'
    for file, names in selections.items():
        source = (vendor / "net80211" / file).read_text()
        for name in names: closure += "\n// Verbatim from " + file + "\n" + function(source, name)
    ccmp = (vendor / "net80211/ieee80211_crypto_ccmp.c").read_text()
    closure += '\n#include <crypto/aes.h>\n'
    closure += re.search(r'struct ieee80211_ccmp_ctx\s*\{[\s\S]*?\};', ccmp).group() + '\n'
    for name in ["ieee80211_ccmp_set_key", "ieee80211_ccmp_delete_key"]:
        closure += "\n// Verbatim from ieee80211_crypto_ccmp.c\n" + function(ccmp, name)
    (args.out / "upstream_helpers.cpp").write_text(closure)
    sources = [vendor / name for name in ["crypto/sha1.c", "crypto/hmac.c", "crypto/sha2.c",
                "crypto/md5.c", "crypto/aes.c", "crypto/cmac.c", "crypto/key_wrap.c", "crypto/arc4.c",
                "net80211/_string.c", "net80211/ieee80211_crypto.c",
                "net80211/ieee80211_pae_input.c", "net80211/ieee80211_pae_output.c"]]
    sources += [args.out / "upstream_helpers.cpp", project / "tests/net80211_pae_tests.cpp"]
    command = [*compiler(), "-std=c++14", "-x", "c++", "-g", "-O1",
               *sanitizer_flags(), "-fno-omit-frame-pointer", *dead_strip_flags(),
               "-DIEEE80211_STA_ONLY", "-Werror=return-type", "-Werror=dangling",
               "-Wno-macro-redefined", "-Wno-format", "-Wno-conversion",
               "-I" + str(host / "include"), "-I" + str(project / "tests/host_compat"), "-I" + str(vendor), "-I" + str(vendor / "crypto"),
               "-I" + str(args.out), "-include", str(host / "platform.hpp")]
    command += [str(source) for source in sources] + ["-o", str(args.out / "net80211-pae-tests")]
    subprocess.run(command, check=True)
    subprocess.run([str(args.out / "net80211-pae-tests")], check=True)


if __name__ == "__main__": main()
