/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_BOOT_H
#define RTW_RTW88_BOOT_H
#include "rtw/RTW88Firmware.h"
#include "rtw/ChipIdentity.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTW88_EFUSE_SCRATCH_BYTES 1024U
#define RTW88_EFUSE_LOGICAL_MAX 768U
typedef struct {
 uint16_t physical_bytes,logical_bytes,protected_bytes;
 uint16_t usb_mac_offset;
} rtw88_efuse_geometry;
typedef struct {
 uint32_t sys_cfg,sys_cfg2,power_control;
 uint8_t cut,vendor,rf_paths,mp_chip;
 uint8_t active_rf_paths,tx_streams,feature_bw,feature_protocol,features_valid;
 uint8_t logical[RTW88_EFUSE_LOGICAL_MAX],mac[6];
 uint8_t rfe_option,package_type,rfe_btg,crystal_cap,channel_plan,country_code[2];
 uint8_t rf_board_option,bt_setting,share_ant,btcoex,regd;
 uint8_t pa_type_2g,pa_type_5g,lna_type_2g,lna_type_5g;
 uint8_t ext_pa_2g,ext_pa_5g,ext_lna_2g,ext_lna_5g;
 uint8_t thermal_meter[2],thermal_meter_k,power_track_type;
 uint8_t tx_bb_swing_setting_2g,tx_bb_swing_setting_5g,usb_mode_switch;
 uint8_t txpwr_idx_table[4][42];
} rtw88_board;
/* Keep this object at a stable address after init: firmware.bus.cookie points
 * back to it. These are B/C contexts, never cast to an AU controller. */
typedef struct {
 rtw88_fw_context firmware;
 rtw88_fw_bus raw_bus;
 const rtw_usb_wifi_profile *identity;
 rtw_chip_identity *resolved_identity; /* optional exact13b1 proof; never catalog authority */
 rtw88_board board;
 uint8_t board_valid;
 int cleanup_error;
 uint32_t power_epoch; /* invalidates derived PHY state after identify/power/stop/features */
 uint8_t physical[RTW88_EFUSE_SCRATCH_BYTES],physical_valid; /* read-only trim cache */
} rtw88_boot_context;
int rtw88_boot_geometry(rtw_wifi_family,rtw88_efuse_geometry*);
/* Profile must be an immutable, primary-verified catalog pointer returned by
 * the full USB descriptor lookup. Initialization performs no bus operations. */
int rtw88_boot_init(rtw88_boot_context*,const rtw_usb_wifi_profile*,const rtw88_fw_bus*);
/* Separate runtime constructor. The original conflict row remains unverified;
 * only the opaque, same-object/bus-bound read proof can supply this profile. */
int rtw88_boot_init_resolved(rtw88_boot_context*,rtw_chip_identity*,const rtw88_fw_bus*);
/* Repeat the checked chip-code read before power or firmware writes. Ordinary
 * immutable catalog contexts retain their existing zero-read validation. */
int rtw88_boot_revalidate_identity(rtw88_boot_context*);
/* Three bounded register reads; USB identity supplies family, SYS_CFG1 supplies
 * the real silicon cut/RF paths. Test silicon and unsupported cut masks fail. */
int rtw88_boot_identify(rtw88_boot_context*);
int rtw88_boot_power_on(rtw88_boot_context*);
#define RTW88_CHECKED_STOP 1
int rtw88_boot_stop_checked(rtw88_boot_context*);
int rtw88_boot_stop(rtw88_boot_context*);
/* Logical decoder excludes each family's protected physical tail and bounds
 * every header/pair. Input/output must not overlap. No USB operations. */
int rtw88_boot_decode_efuse(rtw_wifi_family,const uint8_t*,size_t,uint8_t*,size_t);
int rtw88_boot_parse_board(rtw88_boot_context*,const uint8_t*,size_t);
/* Matches upstream's powered/running-firmware efuse stage. Caller supplies
 * nonoverlapping1024-byte scratch. This reads OTP only:2.5V programming voltage
 * is disabled at the chip's actual gate; no programming command is exposed.
 * Bank/voltage restoration is attempted after errors; failed restoration is
 * reported. board_valid requires a complete read and valid decoded board. */
/* Actual firmware C2H19 hardware feature report. NSS is limited by physical
 * RF paths; physical RF transport and active antenna topology stay distinct. */
int rtw88_boot_read_features(rtw88_boot_context*);
int rtw88_boot_read_board(rtw88_boot_context*,uint8_t*,size_t);
#ifdef __cplusplus
}
#endif
#endif
