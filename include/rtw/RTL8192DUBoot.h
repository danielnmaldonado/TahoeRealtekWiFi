/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192DU_BOOT_H
#define RTW_RTL8192DU_BOOT_H
#include "FamilyBackend.h"
#include "PowerSequence.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Typed/testable RTL8192DU boot primitives, shared by the kernel bridge and
 * host models. Boot alone never authorizes a station or bulk TX; RF/BB,
 * calibration, association and net80211 security are separate prerequisites. */
typedef enum {
    RTW_92DU_OK=0,
    RTW_92DU_BAD_ARGUMENT,
    RTW_92DU_IDENTITY_UNVERIFIED,
    RTW_92DU_UNSUPPORTED_CHIP,
    RTW_92DU_BAD_PROM,
    RTW_92DU_IO_NOT_CONFIRMED,
    RTW_92DU_WRONG_STATE,
    RTW_92DU_MCU_BUSY,
    RTW_92DU_BAD_FIRMWARE,
    RTW_92DU_IO_ERROR,
    RTW_92DU_WAIT_ERROR,
    RTW_92DU_POLL_TIMEOUT
} rtw_92du_result;
typedef enum { RTW_92DU_SINGLE_MAC=1, RTW_92DU_DUAL_MAC=2 } rtw_92du_mac_mode;
typedef struct {
    const rtw_usb_wifi_profile *profile;
    uint32_t sys_cfg;
    rtw_92du_mac_mode mode;
    uint8_t mac_index, rf_paths, initial_band, mac[6];
} rtw_92du_board;
/* Shared read-only prerequisite for all DU hardware operations. */
rtw_92du_result rtw_92du_validate_usb_plan(const rtw_backend_usb_plan *usb);

/* Decode a caller-supplied logical PROM, not raw packed eFuse. SYS_CFG must be
 * an actual read from 0xf0. No I/O; output is unchanged on failure. The USB plan
 * identifies the whole device; mac_index selects its internal MAC0/MAC1.
 * MAC1 requires dual-MAC PROM and exclusive ownership of BOTH USB interfaces. */
rtw_92du_result rtw_92du_identify(const rtw_backend_usb_plan *usb,
    uint32_t sys_cfg, const uint8_t *logical_prom, size_t prom_length,
    uint8_t mac_index, rtw_92du_board *out);

typedef struct {
    uint8_t confirm_experimental_io;
    uint8_t exclusive_whole_device;
    uint16_t max_polls; /* 1..1000, per register poll */
} rtw_92du_boot_options;
typedef struct {
    rtw_92du_board board;
    rtw_usb_rw_transport bus;
    rtw_waiter waiter;
    uint16_t max_polls, last_register;
    rtw_92du_result last_error;
    uint8_t power_ready, firmware_ready, firmware_reused, download_active;
    size_t firmware_bytes;
    void *phy; /* optional caller-owned DU PHY storage; no allocation here */
    uint16_t firmware_version; /* supplied image header; not RAM authentication */
    uint8_t firmware_subversion;
} rtw_92du_boot;

/* No I/O. A successful init does not assert that power or firmware is ready.
 * The caller must own/serialize both MACs for the entire init/power/FW session.
 * Callbacks are synchronous and must not reenter this context. */
rtw_92du_result rtw_92du_boot_init(rtw_92du_boot *out,
    const rtw_92du_board *board, const rtw_usb_rw_transport *bus,
    const rtw_waiter *waiter, const rtw_92du_boot_options *options);
/* Cold device only: refuses either MAC already on, then applies the DU USB
 * power/mode/buffer-boundary sequence and keeps all TX queues paused. On any
 * I/O/wait/poll failure the context is poisoned; do not retry without reset. */
rtw_92du_result rtw_92du_power_on(rtw_92du_boot *context);
/* DU header 0x92d0..0x92d3; 32-byte header, 4096-byte pages, <=32768 payload.
 * Uses USB 64/8/1 chunks without padding and DU's per-MAC ready register.
 * Existing RDY firmware can be reused (version is not authenticated/read back).
 * Running-but-not-RDY RAM code and stale CRC/download/MAC-ready are refused: the
 * DU firmware self-reset/recovery path is not implemented by this primitive. */
rtw_92du_result rtw_92du_firmware_start(rtw_92du_boot *context,
    const uint8_t *firmware, size_t firmware_length);
const char *rtw_92du_result_string(rtw_92du_result result);

#ifdef __cplusplus
}
#endif
#endif
