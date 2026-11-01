/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8192EU native USB power and 8051 firmware boot stage.
 * No third-party kernel driver, OCLP, AirPort or private Wi-Fi APIs.
 * Hardware programming protocol adapted from GPL-2.0 rtl8xxxu 8192e.
 */
#ifndef RTW_RTL8192EU_HARDWARE_H
#define RTW_RTL8192EU_HARDWARE_H
#include "rtw/RTL8192EU.h"
#include "rtw/PowerSequence.h"
#include "rtw/USBRegisterIO.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RTW_92E_BOOT_OK = 0,
    RTW_92E_BOOT_INVALID_ARGUMENT,
    RTW_92E_BOOT_WRONG_DEVICE,
    RTW_92E_BOOT_BAD_FIRMWARE,
    RTW_92E_BOOT_USB_FAILED,
    RTW_92E_BOOT_POWER_TIMEOUT,
    RTW_92E_BOOT_MAC_TIMEOUT,
    RTW_92E_BOOT_CHECKSUM_TIMEOUT,
    RTW_92E_BOOT_FIRMWARE_TIMEOUT,
    RTW_92E_BOOT_SLEEP_FAILED
} rtw_92e_boot_result;

typedef enum {
    RTW_92E_STAGE_NONE = 0,
    RTW_92E_STAGE_IDENTITY,
    RTW_92E_STAGE_POWER,
    RTW_92E_STAGE_MAC,
    RTW_92E_STAGE_FW_DOWNLOAD,
    RTW_92E_STAGE_FW_CHECKSUM,
    RTW_92E_STAGE_FW_RUNNING
} rtw_92e_stage;

typedef struct {
    uint32_t max_polls;   /* 1..10000 */
    uint32_t sleep_us;    /* 1..100000 */
} rtw_92e_boot_config;

typedef struct {
    rtw_92e_stage last_stage;
    uint32_t sys_cfg;
    uint16_t firmware_version;
    uint8_t firmware_subversion;
    uint8_t pages;
    uint8_t firmware_ready;
    size_t firmware_bytes;
    size_t usb_firmware_writes;
    uint32_t last_mcu_status;
} rtw_92e_boot_report;

/* WARNING: Writes to physical USB registers. ONLY use with a validated,
 * exclusively claimed 2357:0108 device and an explicitly provided 0x92Ex
 * firmware image. Caller owns USB device and handles its disconnect/close.
 * Does not configure PHY/MAC RF paths or create network interfaces.
 * On a failed bring-up, physically unplug/replug before another attempt.
 * Context callbacks must guarantee synchronous, serialized control I/O.
 * On failures after FW download enable, attempts to clear that bit.
 */
rtw_92e_boot_result rtw_8192eu_boot_8051(
    const rtw_usb_rw_transport *transport,
    const rtw_waiter *waiter,
    uint16_t vid,
    uint16_t pid,
    const uint8_t *firmware,
    size_t firmware_length,
    const rtw_92e_boot_config *config,
    rtw_92e_boot_report *report);

const char *rtw_8192eu_boot_result_string(rtw_92e_boot_result value);
#ifdef __cplusplus
}
#endif
#endif
