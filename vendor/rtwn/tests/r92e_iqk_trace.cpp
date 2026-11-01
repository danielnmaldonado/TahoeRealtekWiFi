// SPDX-License-Identifier: GPL-2.0-only
// Host protocol tests for the production backend. Expected samples are literal.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <array>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a) / sizeof((a)[0]))
#include "vendor/rtwn/r92creg.h"
#define RTWN_CHIP_88E 0x20
#define RTWN_CHIP_92E 0x40
struct Applied { int chain; std::array<uint16_t, 2> tx, rx; };
struct rtwn_softc {
    uint32_t chip = RTWN_CHIP_92E;
    int ntxchains = 2, nrxchains = 2, core_error = 0;
    uint8_t iq_calibration_mask = 0;
    std::map<uint16_t, uint32_t> bb;
    std::map<unsigned, uint32_t> rf;
    std::vector<Applied> applied;
    uint16_t samples[3][8] = {
        {0x100, 0x3ff, 0x101, 1, 0x102, 2, 0x103, 3},
        {0x102, 1, 0x103, 3, 0x104, 4, 0x105, 5},
        {0x101, 0, 0x102, 2, 0x103, 3, 0x104, 4}};
    unsigned shots[3][2][3] = {}, protocol_errors = 0;
    int run = -1, arm = -1, pending = -1, pending_chain = 0;
    int suppress_rx = -1, fail_tx = -1, fail_imr = -1;
    bool zeros = false, ones = false, first_tx_failure = false;
    bool fault_on_shot = false, fault_on_result = false;
    bool fault_on_snapshot = false, fault_on_restore = false;
    bool fault_on_apply = false, suppress_settle = false;
    bool restoring = false;
    bool fault_on_rf_snapshot = false;
    bool imr_ready[2] = {};
    unsigned reads = 0, legacy_b = 0;
};
static unsigned checks;
static void require(bool value, const char *what) {
    ++checks;
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}
static bool measurement(uint16_t a) {
    return a == 0xe94 || a == 0xe9c || a == 0xea4 || a == 0xeac ||
           a == 0xeb4 || a == 0xebc || a == 0xec4 || a == 0xecc;
}
static void release(rtwn_softc *s) {
    const int c = s->pending_chain, p = s->pending;
    s->pending = -1;
    if (p < 0 || s->run < 0 || s->run > 2) return;
    ++s->shots[s->run][c][p];
    s->bb[0xeac] = 0;
    if (s->fault_on_shot) { s->core_error = 5; return; }
    const bool fail = (p == 0 && s->fail_tx == c) ||
        (p == 1 && s->fail_imr == c) ||
        (p == 0 && c == 0 && s->first_tx_failure && s->shots[s->run][0][0] == 1);
    if (fail) { s->bb[0xeac] = 1U << (28 + c * 3); return; }
    if (p == 2 && s->suppress_rx == c) {
        s->bb[0xeac] = 1U << (27 + c * 3); return;
    }
    if (p == 1) s->imr_ready[c] = true;
    const int i = c * 4 + (p == 2 ? 2 : 0);
    const uint32_t x = s->zeros ? 0 : s->samples[s->run][i];
    const uint32_t y = s->zeros ? 0 : s->samples[s->run][i + 1];
    s->bb[(p == 2 ? 0xea4 : 0xe94) + c * 0x20] = x << 16;
    s->bb[(p == 2 ? 0xeac : 0xe9c) + c * 0x20] = y << 16;
}
static void start_shot(rtwn_softc *s, int c) {
    const uint32_t response = s->bb[0xe4c];
    int p = response == 0x00462911 ? 0 :
            response == 0x0046a911 ? 1 : response == 0x0046a891 ? 2 : -1;
    bool ok = p >= 0 && s->bb[0xe28] == 0x80800000;
    const unsigned key = c * 256;
    ok &= s->rf[key + 0xef] == 0x800a0;
    ok &= s->rf[key + 0x30] == (p == 0 ? 0x20000U : 0x30000U);
    ok &= s->rf[key + 0x31] == 0xf;
    ok &= s->rf[key + 0x32] == (p == 0 ? 0x7f77U : p == 1 ? 0xf1173U : 0xf7ff2U);
    ok &= s->rf[key + 0xdf] == (p == 0 ? 0x180U : 0x980U);
    if (p > 0) ok &= s->rf[key + 0x56] == (p == 1 ? 0x511e0U : 0x510e0U);
    // Preserve pinned Linux RX-B PI_A; vendor PHYDM uses PI_B.
    // This reference choice does not prove either setting on real RF.
    const uint16_t pi = p == 2 ? 0xe38 : uint16_t(0xe38 + c * 0x20);
    ok &= s->bb[pi] == (p == 0 ? 0x82140303U : p == 1 ? 0x8216031fU : 0x821608ffU);
    ok &= s->bb[pi + 4] == (p == 0 ? 0x68160000U : p == 1 ? 0x6816031fU : 0x281608ffU);
    const uint16_t active = (p == 2 ? 0xe34 : 0xe30) + c * 0x20;
    for (uint16_t a : {0xe30, 0xe34, 0xe50, 0xe54})
        ok &= s->bb[a] == (a == active ? 0x18008c1cU : 0x38008c1cU);
    if (p == 1) s->imr_ready[c] = false;
    if (p == 2) {
        ok &= s->imr_ready[c];
        const int i = c * 4;
        ok &= s->bb[0xe40] == (0x80007c00U | (uint32_t(s->samples[s->run][i]) << 16) | s->samples[s->run][i + 1]);
        ok &= s->bb[0xe44] == 0x01004800;
    }
    if (p > 0) {
        const unsigned other = (1 - c) * 256;
        ok &= s->rf[other + 0xef] == 0x800a0;
        ok &= s->rf[other + 0x30] == 0x30000;
        ok &= s->rf[other + 0x31] == 0xf;
        ok &= s->rf[other + 0x32] == (p == 1 ? 0xf1173U : 0xf7ff2U);
    }
    if (p == 2) ok &= s->shots[s->run][c][0] > 0 && s->shots[s->run][c][1] > 0;
    if (!ok) { ++s->protocol_errors; s->bb[0xeac] = (1U << (28 + c * 3)) | (1U << (27 + c * 3)); return; }
    // This scenario models a reported failed/unsettled shot, not an
    // undetectable stale-success report. Linux has no freshness token.
    if (s->suppress_settle) s->bb[0xeac] = (1U << (28 + c * 3)) | (1U << (27 + c * 3));
    s->pending = p; s->pending_chain = c;
}
static uint32_t rtwn_bb_read(rtwn_softc *s, uint16_t a) {
    ++s->reads;
    if (s->fault_on_snapshot && s->reads == 8) s->core_error = 5;
    if (measurement(a) && s->fault_on_result && s->run >= 0) s->core_error = 5;
    return measurement(a) && s->ones ? 0xffffffffU : s->bb[a];
}
static void rtwn_bb_write(rtwn_softc *s, uint16_t a, uint32_t v) {
    if (a == 0xc04 && v == 0x03a05600) ++s->run;
    if (a == 0xe60) ++s->legacy_b;
    if (s->run >= 0 && a == 0xe28 && v == 0) s->restoring = true;
    if (s->fault_on_restore && s->restoring && a == 0x85c && v == 0x1357085c) s->core_error = 5;
    s->bb[a] = v;
    if (a == 0xe48) {
        if (v == 0xf9000000) s->arm = 0;
        else if (v == 0xfa000000) s->arm = 1;
        else if (v == 0xf8000000 && s->arm >= 0) {
            start_shot(s, s->arm); s->arm = -1;
        }
    }
}
static uint32_t rtwn_read_4(rtwn_softc *s, uint16_t a) { return rtwn_bb_read(s, a); }
static void rtwn_write_4(rtwn_softc *s, uint16_t a, uint32_t v) { rtwn_bb_write(s, a, v); }
static uint8_t rtwn_read_1(rtwn_softc *s, uint16_t a) { return uint8_t(rtwn_bb_read(s, a)); }
static void rtwn_write_1(rtwn_softc *s, uint16_t a, uint8_t v) {
    rtwn_bb_write(s, a, (s->bb[a] & 0xffffff00U) | v);
}
[[maybe_unused]] static uint32_t rtwn_rf_read(rtwn_softc *s, int c, uint8_t a) {
    // Mirror transport-helper BB side effects, including the other path's edge.
    s->bb[0x824] ^= 0x80000000;
    s->bb[0x824 + c * 8] = 0x80000000 | (uint32_t(a) << 23);
    if (s->fault_on_rf_snapshot) s->core_error = 5;
    return s->rf[c * 256 + a];
}
[[maybe_unused]] static void rtwn_rf_write(rtwn_softc *s, int c, uint16_t a, uint32_t v) {
    s->rf[c * 256 + a] = v;
    s->bb[0x818] |= 0x20000;
    s->bb[0x840 + c * 4] = (uint32_t(a) << 20) | v;
}
static void rtwn_delay(rtwn_softc *s, uint32_t us) {
    if (s->pending >= 0 && us >= (s->pending == 0 && s->pending_chain == 1 ? 1000U : 10000U) && !s->suppress_settle) release(s);
}
[[maybe_unused]] static int rtwn_io_error(rtwn_softc *s) { return s->core_error; }
#define rtwn_iq_calib_write_results actual_write_results
#include "writer.inc"
#undef rtwn_iq_calib_write_results
static void rtwn_iq_calib_write_results(rtwn_softc *s, uint16_t tx[2], uint16_t rx[2], int c) {
    s->applied.push_back({c, {tx[0], tx[1]}, {rx[0], rx[1]}});
    actual_write_results(s, tx, rx, c);
    if (s->fault_on_apply) s->core_error = 5;
}
#include "backend.inc"
static rtwn_softc context(int chains = 2) {
    rtwn_softc s; s.ntxchains = s.nrxchains = chains;
    for (unsigned a = 0; a < 0xf00; ++a) s.bb[a] = 0x13570000U | a;
    for (unsigned c = 0; c < 2; ++c)
        for (unsigned a = 0; a < 256; ++a) s.rf[c * 256 + a] = 0x40000 | (c << 12) | a;
    for (uint16_t a : {0xe94, 0xe9c, 0xea4, 0xeac, 0xeb4, 0xebc, 0xec4, 0xecc}) s.bb[a] = 0x01110000;
    s.bb[0xc80] = s.bb[0xc88] = 0x40000000;
    return s;
}
static bool matrix(uint16_t a) {
    return a == 0xc80 || a == 0xc88 || a == 0xc94 || a == 0xc9c ||
        a == 0xc4c || a == 0xc14 || a == 0xc1c || a == 0xca0 || a == 0xc78;
}
static void restored(const rtwn_softc &s, const rtwn_softc &before, bool apply) {
    for (const auto &entry : before.bb) {
        if (measurement(entry.first) || (apply && matrix(entry.first))) continue;
        require(s.bb.at(entry.first) == entry.second, "restore all touched BB/MAC state");
    }
    for (const auto &entry : before.rf) {
        const unsigned address = entry.first & 255;
        // Linux programs banked IQK LUT entries; a scalar-map equality
        // cannot demonstrate their restoration and is deliberately excluded.
        if (address == 0xef || address == 0x30 || address == 0x31 || address == 0x32) continue;
        require(s.rf.at(entry.first) == entry.second, "restore RF controls, both paths");
    }
}
int main() {
    for (int chains : {1, 2}) {
        auto s = context(chains), before = s;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == (chains == 1 ? 1 : 3), "stable calibrated chain mask");
        require(s.protocol_errors == 0 && s.legacy_b == 0, "gen2 RF/tones/PI/trigger protocol");
        require(s.applied.size() == unsigned(chains), "apply active chains once");
        require(s.applied[0].rx == std::array<uint16_t,2>{0x101, 1}, "RX coefficients come from RX phase, never stale TX-only registers");
        require(s.shots[0][0][0] == 1 && s.shots[0][0][1] == 1 && s.shots[0][0][2] == 1, "separate TX, TXIMR, RX measurements");
        if (chains == 1) require(s.shots[0][1][0] == 0, "one-chain bounds");
        else {
            require(s.shots[0][1][2] == 1, "B RX requires fa/f8 shot");
            require(s.applied[1].rx == std::array<uint16_t,2>{0x103, 3}, "B RX comes from B RX measurement");
        }
        restored(s, before, true);
    }
    for (int mode = 0; mode < 9; ++mode) {
        auto s = context(), before = s;
        if (mode == 0) s.zeros = true;
        if (mode == 1) s.ones = true;
        if (mode == 2) s.fault_on_snapshot = true;
        if (mode == 3) s.fault_on_shot = true;
        if (mode == 4) s.fault_on_result = true;
        if (mode == 5) s.fault_on_restore = true;
        if (mode == 6) s.suppress_settle = true;
        if (mode == 7) s.core_error = 5;
        if (mode == 8) s.fault_on_rf_snapshot = true;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 0 && s.applied.empty(), "no matrix/mask for unmeasured, sentinel, or I/O-error result");
        restored(s, before, false);
    }
    {
        auto s = context(); s.suppress_rx = 1;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 1, "failed RX B never claims chain B calibrated");
        require(s.applied.size() == 2 && s.applied[1].rx[0] == 0xffff, "stable TX B may apply without RX B");
        require(s.shots[0][1][2] == 2, "failed RX retried twice");
    }
    {
        auto s = context(); s.fail_imr = 1;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 1 && s.shots[0][1][2] == 0, "TXIMR failure cannot run or accept RX B");
        require(s.rf[0x1df] == (0x41000U | 0xdf), "B cleanup restores B RF");
    }
    {
        auto s = context(1); s.first_tx_failure = true;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 1 && s.shots[0][0][0] == 2, "failed TX retried then valid sample accepted");
    }
    {
        auto s = context(1);
        for (int r = 0; r < 3; ++r) s.samples[r][0] = r == 1 ? 0x3ff : 1;
        s.samples[0][1] = 0x200; s.samples[1][1] = 0x200; s.samples[2][1] = 0x210;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 0 && s.applied.empty(), "X unsigned and Y signed differences cannot create false TX candidate");
    }
    for (int choice = 0; choice < 3; ++choice) {
        auto s = context(1);
        const uint16_t cases[3][3][4] = {
            {{0x100, 1, 0x101, 1}, {0x180, 0x80, 0x181, 0x80}, {0x102, 3, 0x103, 3}},
            {{0x180, 0x80, 0x181, 0x80}, {0x100, 1, 0x101, 1}, {0x102, 3, 0x103, 3}},
            {{0x100, 1, 0x180, 0x80}, {0x102, 3, 0x1c0, 0xc0}, {0x140, 0x40, 0x182, 0x82}}
        };
        for (int r = 0; r < 3; ++r)
            for (int i = 0; i < 4; ++i) s.samples[r][i] = cases[choice][r][i];
        rtwn_r92e_iq_calib(&s);
        require(s.run == 2 && s.iq_calibration_mask == 1, "third-run candidate and partial-pair assembly");
        require(s.applied.size() == 1 && s.applied[0].tx == std::array<uint16_t, 2>{0x100, 1}, "0/2, 1/2 or partial candidate preserves selected TX");
        const uint16_t rx = choice == 2 ? 0x180 : 0x101;
        require(s.applied[0].rx[0] == rx, "partial candidate combines independently stable RX pair");
    }
    {
        auto s = context(); s.fail_tx = 1;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 1 && s.applied.size() == 1, "RX success without main TX never applies chain B");
        require(s.shots[0][1][0] == 2 && s.shots[0][1][2] == 1, "main TX validity remains independent of TXIMR and RX");
    }
    for (int chain : {0, 1}) {
        for (int failure = 0; failure < 4; ++failure) {
            auto s = context();
            for (int r = 0; r < 3; ++r)
                s.samples[r][chain * 4 + failure] = failure == 0 ? 0x142 : failure == 1 ? 0x042 : failure == 2 ? 0x132 : 0x036;
            rtwn_r92e_iq_calib(&s);
            require(s.iq_calibration_mask == (chain == 0 ? 2 : 1), "vendor TX/RX failure signatures invalidate only measured chain");
            if (failure < 2) require(s.applied.size() == 1, "failed TX signature cannot apply RX-only result");
            else require(s.applied.size() == 2 && s.applied[chain].rx[0] == 0xffff, "failed RX signature permits stable TX-only compensation");
        }
    }
    for (int difference : {5, 6}) {
        auto s = context(1);
        for (int r = 0; r < 3; ++r) {
            s.samples[r][0] = uint16_t(0x100 + r * difference);
            s.samples[r][1] = uint16_t(1 + r * difference);
        }
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == (difference == 5 ? 1 : 0), "gen2 tolerance inclusive five, rejects six");
    }
    {
        auto s = context(1);
        for (int r = 0; r < 3; ++r) s.samples[r][0] = 0xff;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 1 && (s.bb[0xc80] & 0x3ff) == 0xff, "0x00ff is a legal 92E coefficient");
    }
    {
        auto s = context(), before = s; s.fault_on_apply = true;
        rtwn_r92e_iq_calib(&s);
        require(s.iq_calibration_mask == 0, "matrix USB failure never advertises calibration");
        restored(s, before, false);
    }
    std::printf("r92e IQK protocol: %u checks passed\n", checks);
}
