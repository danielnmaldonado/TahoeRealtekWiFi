#!/usr/bin/env python3
"""Trace the actual IQK chain routine against a host BB-register transport."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

project = Path(__file__).resolve().parents[3]
import runpy
_host = runpy.run_path(str(project / "scripts/host_cpp.py"))
_host_cxx = _host["compiler"]()
_host_sanitizers = _host["sanitizer_flags"]()
source = (project / "vendor/rtwn/rtwn.cpp").read_text()

def function(name, returns):
    start = source.index("\n" + name + "(") + 1
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return returns + "\n" + source[start:end] + "\n"

prelude = r"""
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a) / sizeof((a)[0]))
#include "vendor/rtwn/r92creg.h"
#define RTWN_CHIP_88E 0x20
#define RTWN_CHIP_92E 0x40
struct Write { uint16_t address; uint32_t value; };
struct rtwn_softc {
    uint32_t chip = RTWN_CHIP_92E;
    int ntxchains = 2;
    std::map<uint16_t, uint32_t> regs;
    std::vector<Write> writes;
    std::vector<uint32_t> delays;
};
static void rtwn_bb_write(rtwn_softc *s, uint16_t a, uint32_t v) {
    s->regs[a] = v; s->writes.push_back({a, v});
}
static uint32_t rtwn_bb_read(rtwn_softc *s, uint16_t a) {
    return s->regs[a];
}
static void rtwn_delay(rtwn_softc *s, uint32_t us) {
    s->delays.push_back(us);
}
static unsigned checks;
static void require(bool ok, const char *description) {
    ++checks;
    if (!ok) { fprintf(stderr, "FAIL: %s\n", description); exit(1); }
}
static unsigned count(const rtwn_softc &s, uint16_t a, uint32_t v) {
    unsigned n = 0;
    for (const auto &w : s.writes) if (w.address == a && w.value == v) ++n;
    return n;
}
"""
cases = r"""
static rtwn_softc context(uint32_t chip, int chains, int chain) {
    rtwn_softc s; s.chip = chip; s.ntxchains = chains;
    const int offset = chain * 0x20;
    s.regs[R92C_TX_POWER_BEFORE_IQK_A + offset] = 0x100U << 16;
    s.regs[R92C_TX_POWER_AFTER_IQK_A + offset] = 0x20U << 16;
    s.regs[R92C_RX_POWER_BEFORE_IQK_A_2 + offset] = 0x101U << 16;
    s.regs[R92C_RX_POWER_AFTER_IQK_A_2 + offset] = 0x21U << 16;
    return s;
}
int main() {
    for (uint32_t chip : {0U, unsigned(RTWN_CHIP_88E), unsigned(RTWN_CHIP_92E)}) {
        const bool e = chip == RTWN_CHIP_92E;
        for (int chains : {1, 2}) {
            auto s = context(chip, chains, 0);
            uint16_t tx[2] = {}, rx[2] = {};
            require(rtwn_iq_calib_chain(&s, 0, tx, rx) == 3, "valid chain A results");
            require(count(s, R92C_TX_IQK_TONE_A, e ? 0x18008c1c : 0x10008c1f) == 1,
                    "program TX tone A once");
            require(count(s, R92C_RX_IQK_TONE_A, e ? 0x38008c1c : 0x10008c1f) == 1,
                    "program RX tone A exactly once");
            size_t rx_index = s.writes.size(), trigger_index = s.writes.size();
            size_t finish_index = s.writes.size();
            for (size_t i = 0; i < s.writes.size(); ++i) {
                if (s.writes[i].address == R92C_RX_IQK_TONE_A) rx_index = i;
                if (s.writes[i].address == R92C_IQK_AGC_PTS &&
                    s.writes[i].value == 0xf9000000) trigger_index = i;
                if (s.writes[i].address == R92C_IQK_AGC_PTS &&
                    s.writes[i].value == 0xf8000000) finish_index = i;
            }
            require(rx_index < trigger_index, "RX tone A is configured before the one-shot trigger");
            require(trigger_index < finish_index, "one-shot f9 precedes f8");
            unsigned b = 0;
            for (const auto &w : s.writes)
                if (w.address == R92C_RX_IQK_TONE_B) ++b;
            require(b == (chains == 2 ? 1U : 0U), "RX tone B is not a substitute for A");
            require(count(s, R92C_IQK_AGC_PTS, 0xf9000000) == 1 &&
                    count(s, R92C_IQK_AGC_PTS, 0xf8000000) == 1, "one-shot trigger pair");
            require(s.delays == std::vector<uint32_t>{1000}, "retain vendor 1ms IQK delay");
            require(tx[0] == 0x100 && tx[1] == 0x20 && rx[0] == 0x101 && rx[1] == 0x21,
                    "return measured coefficients");
        }
        for (int chain : {0, 1}) {
            for (int failure : {0, 1, 2, 3, 4, 5}) {
                auto s = context(chip, 2, chain);
                const int off = chain * 0x20;
                if (failure == 0) s.regs[0xeac] |= 1U << (28 + chain * 3);
                if (failure == 1) s.regs[R92C_TX_POWER_BEFORE_IQK_A + off] = 0x142U << 16;
                if (failure == 2) s.regs[R92C_TX_POWER_AFTER_IQK_A + off] = 0x042U << 16;
                if (failure == 3) s.regs[0xeac] |= 1U << (27 + chain * 3);
                if (failure == 4) s.regs[R92C_RX_POWER_BEFORE_IQK_A_2 + off] = 0x132U << 16;
                if (failure == 5) s.regs[R92C_RX_POWER_AFTER_IQK_A_2 + off] = 0x036U << 16;
                uint16_t tx[2] = {}, rx[2] = {};
                require(rtwn_iq_calib_chain(&s, chain, tx, rx) == (failure < 3 ? 0 : 1),
                        "hardware failure flags and sentinels retain their meaning");
                if (chain == 1)
                    require(s.writes.size() == 2 && count(s, R92C_IQK_AGC_CONT, 2) == 1 &&
                            count(s, R92C_IQK_AGC_CONT, 0) == 1, "chain B trigger unchanged");
            }
        }
    }
    for (int chain : {0, 1}) {
        for (uint32_t gain : {1U, 128U, 256U, 511U, 1023U}) {
            for (uint16_t a : {0U, 1U, 0x7fU, 0x100U, 0x1ffU, 0x200U, 0x3ffU}) {
                for (uint16_t b : {0U, 1U, 0x7fU, 0x100U, 0x1ffU, 0x200U, 0x3ffU}) {
                    rtwn_softc s;
                    const uint32_t initial = 0xa5a5a5a5;
                    s.regs[R92C_OFDM0_ECCATHRESHOLD] = initial;
                    s.regs[R92C_OFDM0_TXIQIMBALANCE(chain)] = gain << 22;
                    uint16_t tx[] = {a, b}, rx[] = {0xffff, 0xffff};
                    rtwn_iq_calib_write_results(&s, tx, rx, chain);
                    const int x = (a & 0x200) ? int(a) - 1024 : int(a);
                    const int y = (b & 0x200) ? int(b) - 1024 : int(b);
                    const unsigned xbit = chain == 0 ? 31 : 27;
                    const unsigned ybit = chain == 0 ? 29 : 25;
                    const uint32_t mask = (1U << xbit) | (1U << ybit);
                    const uint32_t expected = (initial & ~mask) |
                        (uint32_t(((x * int(gain)) >> 7) & 1) << xbit) |
                        (uint32_t(((y * int(gain)) >> 7) & 1) << ybit);
                    require(s.regs[R92C_OFDM0_ECCATHRESHOLD] == expected,
                            "IQ compensation uses this path's ECCA bits and preserves the other path");
                    const uint32_t imbalance = s.regs[R92C_OFDM0_TXIQIMBALANCE(chain)];
                    require((imbalance & 0x3ff) == (uint32_t((x * int(gain)) >> 8) & 0x3ff),
                            "signed TX X compensation remains correct");
                    require(((imbalance >> 16) & 0x3f) == (uint32_t((y * int(gain)) >> 8) & 0x3f),
                            "signed TX Y compensation remains correct");
                }
            }
        }
    }
    for (uint32_t chip : {0U, unsigned(RTWN_CHIP_92E)}) {
        auto s = context(chip, 2, 0);
        s.regs[R92C_OFDM0_TXIQIMBALANCE(0)] = 0x40000100;
        uint16_t tx[] = {0xff, 0}, rx[] = {0xff, 0};
        rtwn_iq_calib_write_results(&s, tx, rx, 0);
        if (chip == RTWN_CHIP_92E) {
            require(!s.writes.empty(), "gen2 coefficient 0x00ff is valid, not the failure sentinel");
            require((s.regs[R92C_OFDM0_TXIQIMBALANCE(0)] & 0x3ff) == 0xff,
                    "gen2 applies legal TX coefficient 0x00ff");
            require((s.regs[R92C_OFDM0_RXIQIMBALANCE(0)] & 0x3ff) == 0xff,
                    "gen2 applies legal RX coefficient 0x00ff");
            s.writes.clear(); tx[0] = 0xffff;
            rtwn_iq_calib_write_results(&s, tx, rx, 0);
            require(s.writes.empty(), "gen2 rejects missing TX measurement sentinel 0xffff");
        } else {
            require(s.writes.empty(), "legacy sentinel 0xff remains rejected");
        }
    }
    printf("IQ chain/matrix trace: %u checks passed on actual rtwn functions\n", checks);
}
"""
with tempfile.TemporaryDirectory(prefix="rtwn-iq-chain-") as temporary:
    path = Path(temporary)
    cpp = path / "test.cpp"
    cpp.write_text(prelude + function("rtwn_iq_calib_chain", "int") +
                   function("rtwn_iq_calib_write_results", "void") + cases)
    compiler = _host_cxx
    subprocess.run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror",
                               *_host_sanitizers, "-fno-omit-frame-pointer",
                               "-I", str(project), str(cpp), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
