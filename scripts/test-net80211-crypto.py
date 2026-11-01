#!/usr/bin/env python3
"""Run imported crypto vectors in a host process, never on USB or in the kernel."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import argparse
import subprocess

project = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--out", type=Path, default=project / "out-net80211-host")
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
vendor = project / "vendor/itl80211/openbsd"
host = project / "tests/net80211_host"
sources = ["crypto/sha1.c", "crypto/sha1-pbkdf2.c", "crypto/hmac.c",
           "crypto/sha2.c", "crypto/md5.c", "crypto/aes.c", "crypto/key_wrap.c",
           "net80211/_string.c", "net80211/ieee80211_crypto_ccmp.c"]
command = [*compiler(), "-std=c++14", "-x", "c++", "-g", "-O1",
           *sanitizer_flags(), "-fno-omit-frame-pointer",
           "-DIEEE80211_STA_ONLY", "-Wno-macro-redefined", "-Wno-format",
           "-I" + str(host / "include"), "-I" + str(project / "tests/host_compat"), "-I" + str(vendor),
           "-I" + str(vendor / "crypto"), "-include", str(host / "host_platform.hpp")]
command += [str(vendor / source) for source in sources]
command += [str(project / "tests/net80211_crypto_tests.cpp"), "-o", str(args.out / "net80211-crypto-tests")]
subprocess.run(command, check=True)
subprocess.run([str(args.out / "net80211-crypto-tests")], check=True)
