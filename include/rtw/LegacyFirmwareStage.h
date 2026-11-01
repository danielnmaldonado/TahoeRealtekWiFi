/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_LEGACY_FIRMWARE_STAGE_H
#define RTW_LEGACY_FIRMWARE_STAGE_H

#include "rtw/FamilyBackend.h"
#include "rtw/PowerSequence.h"
#include "rtw/USBRegisterIO.h"
#include "rtw/USBWiFiFirmware.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTW_LEGACY_FW_PAGE_SIZE 4096U
#define RTW_LEGACY_FW_MAX_PAGES 8U

typedef enum {
    RTW_LEGACY_FW_OK = 0,
    RTW_LEGACY_FW_BAD_ARGUMENT,
    RTW_LEGACY_FW_UNSUPPORTED,
    RTW_LEGACY_FW_BAD_IMAGE,
    RTW_LEGACY_FW_IDENTITY_UNVERIFIED,
    RTW_LEGACY_FW_CAPACITY_EXCEEDED,
    RTW_LEGACY_FW_CANCELLED,
    RTW_LEGACY_FW_PERMISSION_REQUIRED,
    RTW_LEGACY_FW_BUSY,
    RTW_LEGACY_FW_IO_ERROR,
    RTW_LEGACY_FW_CHECKSUM_TIMEOUT,
    RTW_LEGACY_FW_READY_TIMEOUT,
    RTW_LEGACY_FW_WAIT_FAILED
} rtw_legacy_fw_result;

typedef struct {
    rtw_wifi_family family;
    uint16_t firmware_signature;
    uint16_t firmware_version;
    uint16_t mcu_download_register;
    uint16_t firmware_window_register;
    uint8_t pages;
    uint8_t boot_experimental_available;
    const uint8_t *payload;
    size_t payload_size;
} rtw_legacy_fw_plan;

typedef int (*rtw_legacy_fw_chunk)(void *context, uint8_t page,
    uint16_t vendor_register, const uint8_t *data, uint16_t size);

typedef struct {
    /* Both must be explicitly set by a hardware-lab caller. */
    uint8_t confirm_unstable_hardware;
    uint8_t exclusive_interface_claimed;
    uint16_t max_polls;  /* 1..2000 */
    uint16_t delay_us;   /* 1..10000 */
} rtw_legacy_fw_options;

typedef struct {
    size_t bytes_written;
    size_t vendor_writes;
    uint8_t pages_written;
    uint8_t checksum_seen;
    uint8_t firmware_ready;
    uint8_t download_disabled;
    uint32_t last_mcu_status;
} rtw_legacy_fw_report;

/* Capability levels for explicit diagnostics. Return 0 for unsupported
   families, 1 for validated download-layout planning, and 2 for an
   experimental USB firmware-boot routine. No value implies kext support. */
uint8_t rtw_legacy_fw_capability(rtw_wifi_family family);

/* Uses a descriptor-validated USB plan. This does not access hardware,
   allocate buffers, or enable the chipset. All families here are layout-only; their typed
   native backends own actual chip firmware/reset/power/readiness. The old
   generic physical boot path is unavailable. Other families fail closed.
   The firmware memory must remain valid for the lifetime of this plan. */
rtw_legacy_fw_result rtw_legacy_fw_prepare(
    const rtw_backend_usb_plan *usb, const uint8_t *firmware,
    size_t length, rtw_legacy_fw_plan *out);

/* Deterministic, allocation-free page/USB vendor transfer planning.
   No writes are issued. Callback failure stops immediately. */
rtw_legacy_fw_result rtw_legacy_fw_walk(
    const rtw_legacy_fw_plan *plan, rtw_legacy_fw_chunk callback,
    void *context, size_t *count);

/* LIVE USB writes. Explicitly experimental, NOT called by the kext.
   Caller must ensure chip-specific MAC power-on and an exclusive USB
   interface claim. No RF, station, association, or TX is enabled. The
   8051 download-enable bit is cleared on both success and failure.
   A failed write may require unplug/replug to recover the adapter. */
rtw_legacy_fw_result rtw_legacy_fw_boot_experimental(
    const rtw_legacy_fw_plan *plan, const rtw_usb_rw_transport *transport,
    const rtw_waiter *waiter, const rtw_legacy_fw_options *options,
    rtw_legacy_fw_report *report);

const char *rtw_legacy_fw_status_name(rtw_legacy_fw_result result);

#ifdef __cplusplus
}
#endif
#endif
