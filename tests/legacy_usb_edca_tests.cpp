/* SPDX-License-Identifier: GPL-2.0-only */
/* Actual legacy chip cores, with only the USB/RF boundary modeled. */
#include <cstdlib>
#if defined(RTW_EDCA_TEST_8192FU)
#include "rtl8192fu_model.hpp"
using Fixture = FUFixture;
using Parameters = rtw_r92fu_edca;
#define UPDATE rtw_r92fu_edca_update
static const char *family = "RTL8192FU";
#elif defined(RTW_EDCA_TEST_8710BU)
#include "rtl8710bu_model.hpp"
using Fixture = BUFixture;
using Parameters = rtw_r10bu_edca;
#define UPDATE rtw_r10bu_edca_update
static const char *family = "RTL8710BU";
#else
#error Select the actual legacy core under test
#endif

static unsigned checks;
#define CHECK(test) do { ++checks; if (!(test)) { \
    std::fprintf(stderr, "%s line %d: %s\n", family, __LINE__, #test); \
    std::abort(); } } while (0)

/* Linux/vendor BE, BK, VI, VO register order. */
static constexpr unsigned registers[4] = {0x508, 0x50c, 0x504, 0x500};
static void parameters(Parameters ac[4]) {
    for (unsigned i = 0; i < 4; ++i) {
        ac[i].aifsn = uint8_t(i + 1);
        ac[i].ecwmin = 4;
        ac[i].ecwmax = 10;
        ac[i].txop = 0;
    }
}

static void full_words() {
    Fixture f; f.identify(); CHECK(!f.boot());
    Parameters ac[4]; parameters(ac);
    /* Golden AIFS/ECW bytes: long slot20 and short slot9.  The complete
     * u16 TXOP field belongs to this register on each pinned legacy chip. */
    static constexpr uint16_t low[2][4] = {
        {0xa41e, 0xa432, 0xa446, 0xa45a},
        {0xa413, 0xa41c, 0xa425, 0xa42e}
    };
    for (int slot = 0; slot < 2; ++slot) {
        for (unsigned reg : registers) f.m.set(reg, 0xffffffffU);
        for (unsigned txop = 0; txop <= 0xffff; ++txop) {
            for (unsigned i = 0; i < 4; ++i)
                ac[i].txop = uint16_t(txop + 0x1000U * i);
            const auto calls = f.m.calls;
            CHECK(!UPDATE(f.c, slot, ac));
            /* One firmware-health read, one slot byte and four DWORD
             * writes; no read or preservation of an old EDCA word. */
            CHECK(f.m.read(0x51b, 1) == (slot ? 9U : 20U));
            for (unsigned i = 0; i < 4; ++i)
                CHECK(f.m.read(registers[i]) ==
                    ((uint32_t(ac[i].txop) << 16) | low[slot][i]));
            CHECK(f.m.calls == calls + 6);
        }
        /* A previous maximum TXOP must disappear completely on downgrade. */
        for (auto &entry : ac) entry.txop = 0xffff;
        CHECK(!UPDATE(f.c, slot, ac));
        for (auto &entry : ac) entry.txop = 0;
        CHECK(!UPDATE(f.c, slot, ac));
        for (unsigned i = 0; i < 4; ++i)
            CHECK(f.m.read(registers[i]) == low[slot][i]);
    }
}

static void invalid_before_writes() {
    Fixture f; f.identify(); CHECK(!f.boot());
    Parameters ac[4]; parameters(ac);
    for (unsigned kind = 0; kind < 8; ++kind) {
        parameters(ac);
        int slot = 1;
        if (kind == 0) ac[3].aifsn = 16;
        if (kind == 1) ac[3].ecwmin = 16;
        if (kind == 2) ac[3].ecwmax = 16;
        if (kind == 3) { ac[3].ecwmin = 11; ac[3].ecwmax = 10; }
        if (kind == 4) { ac[3].aifsn = 13; slot = 0; }
        if (kind == 5) slot = -1;
        if (kind == 6) slot = 2;
        const auto before = f.m.calls;
        const auto writes = f.m.writes;
        CHECK(UPDATE(f.c, slot, kind == 7 ? nullptr : ac) == EINVAL);
        CHECK(f.m.calls == before + 1 && f.m.writes == writes);
    }
}

static void failures() {
    Parameters ac[4]; parameters(ac);
    for (auto &entry : ac) entry.txop = 0xffff;
    for (bool after_effect : {false, true}) {
        for (unsigned fault = 1; fault <= 6; ++fault) {
            Fixture f; f.identify(); CHECK(!f.boot());
            const auto before = f.m.calls;
            f.m.fail = before + fault;
            f.m.ambiguous = after_effect;
            CHECK(UPDATE(f.c, 1, ac) == EIO);
            CHECK(f.m.calls == before + fault);
            /* The transport may fail after a device accepted the write.
             * Do not replay that word or continue into a later AC. */
            const unsigned first_untouched = fault <= 2 ? 0 : fault - 2;
            for (unsigned later = first_untouched; later < 4; ++later)
                CHECK(f.m.read(registers[later]) !=
                    (0xffff0000U | uint32_t(0xa413 + 9 * later)));
        }
    }
}

int main() {
    full_words(); invalid_before_writes(); failures();
    std::printf("%s: %u actual-core checks, all 65536 TXOP values/all 4 ACs/both slots, stale-word downgrade, 12 I/O faults passed\n", family, checks);
}
