#!/usr/bin/env python3
"""Exercise the real gated WPA diagnostic publisher without RF or keys."""
from pathlib import Path
from host_cpp import compiler, sanitizer_flags
import subprocess
import tempfile
from importlib.util import spec_from_file_location, module_from_spec

project = Path(__file__).resolve().parent.parent
spec = spec_from_file_location("rate_test", project / "scripts/test-rtwn-rate-control.py")
helpers = module_from_spec(spec)
spec.loader.exec_module(helpers)
actual = helpers.method((project / "kext/RTWUSBController.cpp").read_text(),
                        "publishWpaDiagnostics")
cases = r'''
#include "rtwn_rate_control_host.hpp"
ACTUAL
static unsigned checks;
static void check(bool ok) { ++checks; if (!ok) std::abort(); }
int main() {
    RTWUSBController c;
    c.core_.sc_ic.ic_stats = {27, 3, 2};
    c.core_.sc_ic.ic_assoc_status = 17;
    c.core_.sc_ic.ic_deauth_reason = 15;
    c.core_.sc_ic.ic_protmode = 2;
    c.publishWpaDiagnostics();
    check(c.properties.size() == 13);
    check(c.properties.at("RTWDiagWPAEAPOLReceived") == 27);
    check(c.properties.at("RTWDiagWPAEAPOLReplays") == 3);
    check(c.properties.at("RTWDiagWPAEAPOLBadMIC") == 2);
    check(c.properties.at("RTWDiagWPAAssocStatus") == 17);
    check(c.properties.at("RTWDiagWPADeauthReason") == 15);
    check(c.properties.at("RTWDiagWPAProtectionMode") == 2);
    check(c.properties.at("RTWDiagWPASupplicantState") == 0);
    check(c.properties.at("RTWDiagWPAPortValid") == 0);
    check(c.properties.at("RTWDiagWPAProtocol") == 0);
    check(c.properties.at("RTWDiagWPACipher") == 0);
    check(c.properties.at("RTWDiagWPAProfileActive") == 0);
    check(c.properties.at("RTWDiagWPARSNEnabled") == 0);
    check(c.properties.at("RTWDiagWPAPSKConfigured") == 0);
    ieee80211_node n;
    c.core_.sc_ic.ic_bss = &n;
    c.joinRequested_ = true;
    c.core_.sc_ic.ic_flags = IEEE80211_F_RSNON | IEEE80211_F_PSK;
    n.ni_rsnprotos = 2; n.ni_rsncipher = 8;
    for (unsigned state : {0U, 1U, 2U, 3U}) {
        n.ni_rsn_supp_state = state; n.ni_port_valid = state == 3;
        c.publishWpaDiagnostics();
        check(c.properties.at("RTWDiagWPASupplicantState") == state);
        check(c.properties.at("RTWDiagWPAPortValid") == (state == 3));
        check(c.properties.at("RTWDiagWPAProfileActive") == 1);
        check(c.properties.at("RTWDiagWPARSNEnabled") == 1);
        check(c.properties.at("RTWDiagWPAPSKConfigured") == 1);
        check(c.properties.at("RTWDiagWPAProtocol") == 2);
        check(c.properties.at("RTWDiagWPACipher") == 8);
    }
    c.joinRequested_ = false; c.core_.sc_ic.ic_flags = 0;
    c.core_.sc_ic.ic_bss = nullptr;
    c.publishWpaDiagnostics();
    check(c.properties.at("RTWDiagWPASupplicantState") == 0);
    check(c.properties.at("RTWDiagWPAPortValid") == 0);
    check(c.properties.at("RTWDiagWPAProfileActive") == 0);
    check(c.properties.at("RTWDiagWPARSNEnabled") == 0);
    check(c.properties.at("RTWDiagWPAPSKConfigured") == 0);
    check(c.properties.size() == 13);
    printf("WPA diagnostics: %u checks passed on real controller publisher\n", checks);
}
'''.replace("ACTUAL", actual)
with tempfile.TemporaryDirectory(prefix="rtw-wpa-diag-") as temporary:
    folder = Path(temporary)
    cpp = folder / "test.cpp"
    cpp.write_text(cases)
    subprocess.run([*compiler(), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    *sanitizer_flags(), "-I" + str(project),
                    "-I" + str(project / "tests"), "-I" + str(project / "include"),
                    str(cpp), "-o", str(folder / "test")], check=True)
    subprocess.run([str(folder / "test")], check=True)
