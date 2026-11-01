/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192DU_EFUSE_H
#define RTW_RTL8192DU_EFUSE_H
#include "RTL8192DUBoot.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_92DU_PHYSICAL_BYTES 512U
#define RTW_92DU_LOGICAL_BYTES 256U
typedef enum { RTW_92DU_CUT_C=1, RTW_92DU_CUT_D=2, RTW_92DU_CUT_E=3 } rtw_92du_cut;
typedef struct {
    rtw_92du_board board;
    uint8_t physical[RTW_92DU_PHYSICAL_BYTES];
    uint8_t logical[RTW_92DU_LOGICAL_BYTES];
    uint16_t cut_word;
    rtw_92du_cut cut;
    uint8_t crystal, thermal, thermal_valid, regulatory;
    uint8_t internal_pa_5g[2], pa_valid, pa_bias; /* physical OTP0x3fa */
} rtw_92du_rom;
/* Packed DU OTP has 32 logical sections, not the AU 512-byte logical map.
 * Supports extended records, latest-word overrides and header padding. */
rtw_92du_result rtw_92du_unpack_rom(const uint8_t *physical,size_t length,
    uint8_t *logical,size_t capacity);
/* Explicit read-only OTP access (control-register writes only, never OTP
 * programming): requires exclusive whole-device ownership, eFuse boot with
 * autoload success, both MACs off, and a poll limit 1..10. Uses the DU Linux
 * 50us read settle and reads cut signature separately at physical 0x3fe/3ff.
 * Unknown cuts and EEPROM-boot boards fail closed. Output is zero on failure;
 * malformed aliases are rejected without modifying input/output storage. */
rtw_92du_result rtw_92du_read_rom(const rtw_backend_usb_plan *usb,
    const rtw_usb_rw_transport *bus,const rtw_waiter *waiter,
    const rtw_92du_boot_options *options,uint8_t mac_index,rtw_92du_rom *out);
#ifdef __cplusplus
}
#endif
#endif
