/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTL8812AU_BOOTSTRAP_H
#define RTW_RTL8812AU_BOOTSTRAP_H
#include "rtw/RTL8812AUPHY.h"
#include "rtw/FirmwareBoot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {RTW_R12AU_USB_FULL=1,RTW_R12AU_USB_HIGH=2,RTW_R12AU_USB_SUPER=3} rtw_r12au_usb_speed;
typedef struct {
    rtw_r12au_usb_speed speed;
    uint16_t bulk_max_packet;
    uint8_t bulk_in,bulk_out[4],bulk_out_count;
} rtw_r12au_usb_config;
typedef struct {
    rtw_r12au_phy_board phy;
    uint8_t mac[6],channel_plan,country[2],regd,bt_setting,share_ant,vht;
    uint16_t eeprom_vendor,eeprom_product;
    uint32_t sys_cfg;
} rtw_r12au_board;
/* Typed AU context: never interchangeable with 8814AU or 88xxB/C contexts.
 * Exact family comes from USB matching; init never probes or claims a device.
 * Caller owns interface exclusivity and serializes ALL bus/PHY operations. */
typedef struct {
    rtw_r12au_chip chip;
    rtw_usb_rw_transport bus;
    rtw_waiter waiter;
    rtw_r12au_usb_config usb;
    rtw_r12au_board board;
    uint8_t logical_rom[512];
    uint8_t identified,powered,dma_prepared,firmware_running,mac_initialized;
    uint8_t h2c_box,cleanup,associated,rsvd_boundary,acq_pages;
    uint16_t firmware_version;
    uint8_t firmware_subversion;
    uint32_t rx_config;
    int io_error,cleanup_error;
    /* Last resident-firmware reset, retained through best-effort shutdown.
     * Stages: 0 none,1 MCU,2 BB/RF,3 MAC-off,4 MAC-on,5 complete. */
    uint32_t warm_reset_attempts,warm_reset_stage,warm_reset_mcu;
    int warm_reset_error;
} rtw_r12au_boot;
int rtw_r12au_boot_init(rtw_r12au_boot *,rtw_r12au_chip,
    const rtw_usb_rw_transport *,const rtw_waiter *,const rtw_r12au_usb_config *);
/* Pure strict board decode. physical_hints[4] are OTP1018..1021 for8812;
 *8821 must pass NULL and obtains BT presence from its actual hardware flag. */
int rtw_r12au_decode_board(rtw_r12au_chip,uint32_t sys_cfg,
    const uint8_t logical[512],const uint8_t physical_hints[4],unsigned bt_enabled,
    rtw_r12au_board *);
/* Both use512 packed/logical bytes. Only8812 reads physical NSS/VHT hints
 * above1018;8821 is bounded to its512-byte physical OTP geometry. */
int rtw_r12au_identify(rtw_r12au_boot *);
int rtw_r12au_power_on(rtw_r12au_boot *);
/* Selects950x/32-byte NIC header for8812 vs210x for8821. Size is verified
 * against the declared8051 RAM image length. No bus accesses. */
int rtw_r12au_fw_inspect(rtw_r12au_chip,const uint8_t *,size_t,rtw_fw_image *);
/* Requires LLT preparation first. Family-specific MCU wrapper reset and
 * bounded196/8/1-byte USB pages; success requires ALL firmware-ready flags. */
int rtw_r12au_download_fw(rtw_r12au_boot *,const uint8_t *,size_t,rtw_boot_report *);
int rtw_r12au_h2c(rtw_r12au_boot *,const uint8_t command[8]);
/* Best-effort bounded power-down is allowed after a latched transport error.
 * All readiness flags are invalidated before shutdown operations begin. */
#define RTW_R12AU_CHECKED_STOP 1
int rtw_r12au_power_off_checked(rtw_r12au_boot*);
int rtw_r12au_power_off(rtw_r12au_boot *);
/* Shared typed bus accessors for MAC/station adapters. Normal operations stop
 * after the first error; no implicit retry, power transition or RF operation. */
int rtw_r12au_read(rtw_r12au_boot *,uint16_t,uint8_t,uint32_t *);
int rtw_r12au_write(rtw_r12au_boot *,uint16_t,uint8_t,uint32_t);
int rtw_r12au_update(rtw_r12au_boot *,uint16_t,uint8_t,uint32_t mask,uint32_t bits);
int rtw_r12au_delay(rtw_r12au_boot *,uint32_t microseconds);
#ifdef __cplusplus
}
#endif
#endif
