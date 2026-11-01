#!/usr/bin/env python3
"""Test the real thermal calibration decisions with offline RF register input."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


project = Path(__file__).resolve().parents[3]
source = (project / "vendor/rtwn/rtwn.cpp").read_text()
start = source.index("\nrtwn_temp_calib(") + 1
brace = source.index("{", start)
depth, end = 1, brace + 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1

prelude = r"""
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a) / sizeof((a)[0]))
#define DPRINTF(x) do {} while (0)
#define DPRINTFN(n,x) do {} while (0)
#include "vendor/rtwn/r92creg.h"
#define RTWN_CHIP_92E 0x40
struct rtwn_softc {
    uint32_t chip = RTWN_CHIP_92E, raw = 0, written = 0;
    int thcal_state = 0, thcal_lctemp = 0;
    unsigned reads = 0, writes = 0, iq = 0, lc = 0;
    uint16_t address = 0;
};
static unsigned checks;
static void require(bool condition, const char *description) {
    ++checks;
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", description);
        exit(1);
    }
}
static void rtwn_rf_write(rtwn_softc *s, int chain, uint16_t address,
                          uint32_t value) {
    require(chain == 0, "thermal measurement uses RF path A");
    s->writes++; s->address = address; s->written = value;
}
static uint32_t rtwn_rf_read(rtwn_softc *s, int chain, uint8_t address) {
    require(chain == 0, "thermal reading uses RF path A");
    s->reads++; s->address = address; return s->raw;
}
static void rtwn_iq_calib(rtwn_softc *s) { s->iq++; }
static void rtwn_lc_calib(rtwn_softc *s) { s->lc++; }
"""
cases = r"""
static void sample(uint32_t chip, int before, uint32_t raw, int after,
                   unsigned iq, const char *description) {
    rtwn_softc s;
    s.chip = chip; s.thcal_state = 1; s.thcal_lctemp = before; s.raw = raw;
    rtwn_temp_calib(&s);
    require(s.thcal_lctemp == after, description);
    require(s.iq == iq && s.lc == iq, "IQ and LC follow thermal threshold");
    require(s.thcal_state == 0 && s.reads == 1 && s.writes == 0,
            "read phase consumes one sample and restarts measurement phase");
    require(s.address == (chip ? R92E_RF_T_METER : R92C_RF_T_METER),
            "read phase uses the chipset thermal register");
}
int main() {
    sample(RTWN_CHIP_92E, 0, (45U << 10) | 0x18U, 45, 0,
           "92E temperature is the six-bit field at bits 15:10");
    for (unsigned low = 0; low < 32; ++low)
        sample(RTWN_CHIP_92E, 45, (45U << 10) | low, 45, 0,
               "92E low control bits do not change temperature");
    sample(RTWN_CHIP_92E, 40, (44U << 10) | 0x18U, 40, 0,
           "92E increase of four units does not recalibrate");
    sample(RTWN_CHIP_92E, 40, (45U << 10) | 0x18U, 45, 1,
           "92E increase above four units recalibrates");
    sample(RTWN_CHIP_92E, 40, (36U << 10) | 0x18U, 40, 0,
           "92E decrease of four units does not recalibrate");
    sample(RTWN_CHIP_92E, 40, (35U << 10) | 0x18U, 35, 1,
           "92E decrease above four units recalibrates");
    sample(RTWN_CHIP_92E, 45, 0x18U, 45, 0,
           "92E zero temperature is ignored even when control bits are set");
    sample(RTWN_CHIP_92E, 63, (63U << 10) | 0x18U, 63, 0,
           "92E accepts the full six-bit temperature range");
    sample(0, 0, (45U << 10) | 13U, 13, 0,
           "gen1 retains its low-five-bit temperature field");
    sample(0, 13, 14U, 13, 0, "gen1 retains one-unit threshold");
    sample(0, 13, 15U, 15, 1, "gen1 still recalibrates above one unit");
    sample(0, 13, 0U, 13, 0, "gen1 zero reading is ignored");
    for (unsigned chip : {0U, unsigned(RTWN_CHIP_92E)}) {
        rtwn_softc s;
        s.chip = chip;
        rtwn_temp_calib(&s);
        require(s.writes == 1 && s.reads == 0 && s.thcal_state == 1,
                "measurement phase triggers RF and defers the read");
        require(s.address == (chip ? R92E_RF_T_METER : R92C_RF_T_METER),
                "measurement uses the chipset thermal register");
        require(s.written == (chip ? 0x37cf8U : 0x60U),
                "measurement preserves the chipset trigger sequence");
        require(s.iq == 0 && s.lc == 0,
                "measurement trigger alone never recalibrates");
    }
    printf("Thermal calibration: %u checks passed on actual rtwn function\n",
           checks);
}
"""

with tempfile.TemporaryDirectory(prefix="rtwn-thermal-") as temporary:
    path = Path(temporary)
    cpp = path / "test.cpp"
    cpp.write_text(prelude + "\nvoid\n" + source[start:end] + cases)
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    subprocess.run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror",
                               "-I" + str(project), str(cpp), "-o",
                               str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
