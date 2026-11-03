#ifndef RTW_FW_COMMAND_HOST_HPP
#define RTW_FW_COMMAND_HOST_HPP
/* Host-only register transport. The tested mailbox algorithm is extracted
 * verbatim from vendor/rtwn/rtwn.cpp on every run. */
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <vector>
#define __packed __attribute__((__packed__))
#define IEEE80211_ADDR_LEN 6
#define nitems(a) (sizeof(a)/sizeof((a)[0]))
#include "vendor/rtwn/r92creg.h"
#define RTWN_CHIP_92E 0x40U
#define RTWN_CHIP_PCI 0x40000000U
#define DPRINTFN(level, arguments) do {} while (0)
#define KASSERT(condition, ...) do { _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wsign-compare\"") if (!(condition)) std::abort(); _Pragma("clang diagnostic pop") } while (0)
struct device { const char *dv_xname = "host"; };
struct FWWrite { unsigned width; uint16_t address; uint32_t value; };
struct rtwn_softc {
    uint32_t chip = RTWN_CHIP_92E;
    int fwcur = 0, core_error = 0, fault_error=EIO;
    int avg_pwdb=57;
    unsigned thermalCalls=0,scheduled=0;
    struct {void*cookie=nullptr;void(*next_calib)(void*)=nullptr;} sc_ops;
    unsigned reads = 0, busy_reads = 0, fail_read_at = 0, fail_write_at = 0;
    std::vector<FWWrite> writes;
    std::vector<uint32_t> delays;
    device dev;
    device *sc_pdev = &dev;
};
int rtwn_io_error(rtwn_softc *);
uint8_t rtwn_read_1(rtwn_softc *, uint16_t);
void rtwn_write_2(rtwn_softc *, uint16_t, uint16_t);
void rtwn_write_4(rtwn_softc *, uint16_t, uint32_t);
void rtwn_delay(rtwn_softc *, uint32_t);
int rtwn_fw_cmd(rtwn_softc *, uint8_t, const void *, int);
void rtwn_calib(rtwn_softc *);
void rtwn_temp_calib(rtwn_softc *);
#endif
