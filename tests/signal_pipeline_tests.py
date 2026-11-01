#!/usr/bin/env python3
"""Exercise the production RSSI decoder, not a replacement implementation."""
from pathlib import Path
import runpy
import subprocess
import tempfile

project = Path(__file__).resolve().parents[1]
host = runpy.run_path(str(project / "scripts/host_cpp.py"))
extract = runpy.run_path(str(project / "scripts/test-rtwn-ht.py"))["function"]
source = (project / "vendor/rtwn/rtwn.cpp").read_text()
function = extract(source, "rtwn_r88e_get_rssi")
prefix = r"""
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <initializer_list>
#ifndef le32toh
#define le32toh(x) (x)
#endif
#define RTWN_FLAG_CCK_HIPWR 1U
struct rtwn_softc { unsigned sc_flags; };
struct r88e_rx_phystat { uint8_t agc_rpt; uint32_t sq_rpt; };
"""
tests = r"""
int main() {
    unsigned checked=0, changed=0;
    for(unsigned high:{0U,RTWN_FLAG_CCK_HIPWR}) {
        rtwn_softc sc{high};
        for(unsigned agc=0;agc<256;++agc) {
            r88e_rx_phystat phy{static_cast<uint8_t>(agc),0};
            int offset[]={20,14,10,-4,-16,-22,-38,-40};
            unsigned lna=agc>>5,vga=agc&31;
            int expected=(lna==7&&vga>27)?-94:
                offset[lna]-2*int(vga)+(high&&lna==2?6:0);
            int previous=vga>27?-94:offset[lna]-2*int(vga)+(high&&lna==2?6:0);
            for(int rate=0;rate<=3;++rate) {
                assert(rtwn_r88e_get_rssi(&sc,rate,&phy)==expected);++checked;
            }
            if(expected!=previous)++changed;
        }
        for(unsigned quality=0;quality<256;++quality) {
            r88e_rx_phystat phy{0,quality};
            assert(rtwn_r88e_get_rssi(&sc,4,&phy)==int((quality>>1)&127)-110);
            ++checked;
        }
    }
    rtwn_softc sc{};
    r88e_rx_phystat a{27,0},b{28,0};
    assert(rtwn_r88e_get_rssi(&sc,0,&a)==-34);
    assert(rtwn_r88e_get_rssi(&sc,0,&b)==-36); // formerly -94: false 60 dB drop
    r88e_rx_phystat saturated{uint8_t((7<<5)|28),0};
    assert(rtwn_r88e_get_rssi(&sc,0,&saturated)==-94); // legitimate floor preserved
    std::printf("Production RSSI decoder: %u checks, %u changed AGC/high-power combinations passed\n",checked,changed);
}
"""
with tempfile.TemporaryDirectory(prefix="rtw-signal-") as temporary:
    path=Path(temporary)
    (path/"test.cpp").write_text(prefix+function+tests)
    subprocess.run(host["compiler"]()+["-std=c++17","-Wall","-Wextra","-Werror"]+
        host["sanitizer_flags"]()+[str(path/"test.cpp"),"-o",str(path/"test")],check=True)
    subprocess.run([str(path/"test")],check=True)

controller=(project/"kext/RTWUSBController.cpp").read_text()
receive=controller[controller.index("void RTWUSBController::receive("):
    controller.index("\n}",controller.index("void RTWUSBController::receive("))+2]
status=controller[controller.index("void RTWUSBController::status("):
    controller.index("\n}",controller.index("void RTWUSBController::status("))+2]
assert "rtw_signal_observe" in receive
assert receive.index("rtw_signal_observe")<receive.index("ieee80211_inputm")
assert "rtw_signal_from_ap" in receive and "info.rxi_rssi=node->ni_rssi" in receive
assert "rtw_signal_value" in status and "RTW_CONTROL_SIGNAL_TRACKED" in status
assert "ni_rssi" not in status
assert controller.count("rtw_signal_reset(")>=2
model=(project/"app/Main.mm").read_text()
assert "_signal.level([self uptime])" in model
assert "_signalHistory.record(" in model
assert "wifiImageForLevel(level,disconnected)" in model
print("Production RX/status/UI signal hooks passed")
