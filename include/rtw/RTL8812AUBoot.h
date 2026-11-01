/* SPDX-License-Identifier: BSD-3-Clause */
/* Adapted Linux rtw88 algorithms, Copyright(c)2024 Realtek Corporation.
 * License/notice: src/RTL8812AUBoot-LICENSE.txt. */
#ifndef RTW_RTL8812AU_BOOT_H
#define RTW_RTL8812AU_BOOT_H
#include "rtw/FirmwareBoot.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Standalone USB bootstrap primitives for a descriptor-verified RTL8812AU.
 * The caller owns interface exclusivity and external serialization. This layer
 * never enables the RF, calibrates it, sends 802.11 frames or publishes link.
 * RTL8821AU has a different OTP identity, power flow and calibration: refused.
 */
typedef struct {
    uint8_t mac[6], raw_cut, cut, rfe_option, crystal_cap, thermal_meter;
    uint8_t thermal_valid, channel_plan, country_code[2], regd;
    uint8_t pa_type, lna_type_2g, lna_type_5g;
    uint8_t ext_pa_2g, ext_pa_5g, ext_lna_2g, ext_lna_5g;
    uint8_t gpa_type, apa_type, glna_type, alna_type;
    uint8_t rf_board_option, bt_setting, btcoex, share_ant;
    uint8_t tx_bb_swing_2g, tx_bb_swing_5g, tx_streams, rx_streams, vht;
    uint8_t txpwr_idx_table[2][42];
    uint16_t eeprom_vendor, eeprom_product;
    uint32_t sys_cfg;
} rtw_8812au_board;
typedef struct {
    rtw_usb_rw_transport bus;
    rtw_waiter waiter;
    rtw_8812au_board board;
    uint8_t identified, powered, fw_running, h2c_box;
    int io_error;
} rtw_8812au_boot;
void rtw_8812au_boot_init(rtw_8812au_boot *,const rtw_usb_rw_transport *,const rtw_waiter *);
/* Pure EEPROM decode. physical_hints are actual OTP bytes1018..1021, not
 * guessed USB speed. A successful parse does not imply RFE table support. */
int rtw_8812au_parse_board(uint32_t sys_cfg,const uint8_t logical[512],
    const uint8_t physical_hints[4],rtw_8812au_board *);
/* Bounded OTP access, with ELDR/ANA8M/loader clocks and unconditional gate
 * closure. No OTP programming. No MAC power-on and no RF enablement. */
int rtw_8812au_identify_read_board(rtw_8812au_boot *);
/* Cold bootstrap: refuses resident firmware rather than guessing a hot-reset
 * procedure. Exact Linux8812A USB card-enable flow, DMA/security clocks only;
 * MACTXEN/MACRXEN remain clear and queues remain paused. */
int rtw_8812au_power_on(rtw_8812au_boot *);
/* Existing tested 8051 page downloader; no RF-ready flag is exposed. Supplied
 * firmware format is checked before I/O; firmware authenticity is external. */
int rtw_8812au_download_firmware(rtw_8812au_boot *,const uint8_t *,size_t,rtw_boot_report *);
/* Four legacy H2C mailboxes: bounded100us polling, extended word then main.
 * No command construction or station authorization occurs in this function. */
int rtw_8812au_h2c_mailbox(rtw_8812au_boot *,const uint8_t command[8]);
#ifdef __cplusplus
}
#endif
#endif
