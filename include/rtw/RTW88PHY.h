/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_RTW88_PHY_H
#define RTW_RTW88_PHY_H
#include "rtw/RTW88Boot.h"
#include "rtw/RTW88MAC.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {RTW88_PHY_MAC8=0,RTW88_PHY_BB32=1,RTW88_PHY_AGC32=2,RTW88_PHY_RF20=3} rtw88_phy_table_kind;
typedef int (*rtw88_phy_emit)(void*,rtw88_phy_table_kind,uint8_t,uint32_t,uint32_t);
/* Pure pinned-rtw88 IF/ELIF/ELSE table selection. Validates the complete table
 * before emitting anything, including register bounds, delays and branch shape.
 * Conditions use actual silicon cut/package/RFE, USB interface2. */
int rtw88_phy_walk_table(uint8_t cut,uint8_t package,uint8_t rfe,
 rtw88_phy_table_kind,uint8_t path,const uint32_t*,size_t words,rtw88_phy_emit,void*);
/* Offline visit of the complete pinned per-family boot table set, including
 * RFE-specific BTG and8822C RFK initialization tables. Does not access USB. */
int rtw88_phy_visit_boot_tables(rtw_wifi_family,uint8_t cut,uint8_t package,uint8_t rfe,
 rtw88_phy_emit,void*);
typedef struct {
 rtw88_boot_context *boot;
 rtw88_mac_context *mac;
 uint8_t tables_loaded;
 uint32_t channel_filter[3];
 void *radio_state; /* caller-owned RTW88PHYOperating scratch,never an AU HAL */
} rtw88_phy_context;
/* Exact direct read + chip-specific SIPI/direct-mix write transport, with
 * checked I/O and mask bounds. Zero mask/invalid RF path produce no I/O. */
int rtw88_phy_read_rf(rtw88_phy_context*,uint8_t path,uint8_t address,uint32_t mask,uint32_t*);
int rtw88_phy_write_rf(rtw88_phy_context*,uint8_t path,uint8_t address,uint32_t mask,uint32_t);
/* Actual BB/RF domain reset,conditional MAC/BB/AGC/RF tables,8822C RFK init
 * table and crystal-cap setup. Requires real identity,power,FW,MAC and eFuse.
 * tables_loaded is NOT PHY-ready: TRX/RFE finalization,power-limit tables,
 * calibration and station remain separate prerequisites. No station promotion. */
int rtw88_phy_load_boot_tables(rtw88_phy_context*,rtw88_boot_context*,rtw88_mac_context*);
#ifdef __cplusplus
}
#endif
#endif
