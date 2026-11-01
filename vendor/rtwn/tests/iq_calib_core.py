#!/usr/bin/env python3
"""Exercise the actual imported IQ comparison function without USB or a kext."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


project = Path(__file__).resolve().parents[3]
source = (project / "vendor/rtwn/rtwn.cpp").read_text()
start = source.index("#define RTWN_IQ_CAL_MAX_TOLERANCE")
end_marker = "#undef RTWN_IQ_CAL_MAX_TOLERANCE"
end = source.index(end_marker, start) + len(end_marker)

prelude = r"""
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
"""
cases = r"""
struct Results {
    uint16_t tx[2][2] = {{0x120, 0x20}, {0x130, 0x30}};
    uint16_t rx[2][2] = {{0x140, 0x40}, {0x150, 0x50}};
};
static unsigned checks;
static void expect(Results a, Results b, int chains, int want,
                   const char *description) {
    ++checks;
    int actual = rtwn_iq_calib_compare_results(a.tx, a.rx, b.tx, b.rx,
                                              chains);
    if (actual != want) {
        fprintf(stderr, "FAIL: %s (chains=%d, got=%d, want=%d)\n",
                description, chains, actual, want);
        exit(1);
    }
}
int main() {
    Results a, b;
    b.tx[0][0] = 0x160;
    expect(a, b, 2, 0, "unstable first TX coefficient must reject calibration");
    expect(a, a, 1, 1, "identical one-chain measurements");
    expect(a, a, 2, 1, "identical two-chain measurements");

    for (int chains = 1; chains <= 2; ++chains) {
        for (int chain = 0; chain < chains; ++chain) {
            for (int coefficient = 0; coefficient < 2; ++coefficient) {
                for (bool receive : {false, true}) {
                    for (int delta : {-6, -5, 5, 6}) {
                        b = a;
                        auto &values = receive ? b.rx : b.tx;
                        values[chain][coefficient] = uint16_t(
                            values[chain][coefficient] + delta);
                        const int want = (delta == -5 || delta == 5) ? 1 : 0;
                        expect(a, b, chains, want,
                               "every TX/RX coefficient must meet tolerance");
                        expect(b, a, chains, want,
                               "tolerance is symmetric between runs");
                    }
                    b = a;
                    auto &values = receive ? b.rx : b.tx;
                    values[chain][coefficient] = 0xff;
                    expect(a, b, chains, 0, "failed coefficient in later run");
                    expect(b, a, chains, 0, "failed coefficient in earlier run");
                    expect(b, b, chains, 0, "repeated failure is never stable IQ");
                }
            }
        }
    }
    b = a;
    b.tx[1][0] = b.tx[1][1] = b.rx[1][0] = b.rx[1][1] = 0xff;
    expect(a, b, 1, 1, "one-chain device ignores unused second chain");
    printf("IQ calibration comparison: %u checks passed on actual rtwn function\n",
           checks);
}
"""

with tempfile.TemporaryDirectory(prefix="rtwn-iq-calib-") as temporary:
    path = Path(temporary)
    cpp = path / "test.cpp"
    cpp.write_text(prelude + source[start:end] + cases)
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    subprocess.run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror",
                               str(cpp), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
