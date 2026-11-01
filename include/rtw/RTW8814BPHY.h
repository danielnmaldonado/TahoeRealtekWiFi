/* SPDX-License-Identifier: GPL-2.0-only
 * RTL8814B PHY/RFK boundary. Vendor algorithms retain their original notices.
 * Distinct four-path/two-synthesizer silicon; never an RTL8814AU context.
 */
#ifndef RTW_RTW8814B_PHY_H
#define RTW_RTW8814B_PHY_H
#include "rtw/RTW8814BHAL.h"
#ifdef __cplusplus
extern "C" {
#endif
#define RTWB14_PHY_SCRATCH_BYTES 131072U
#define RTWB14_PHY_RATE_COUNT 84U
#define RTWB14_PHY_TSSI_GROUPS 25U
#define RTWB14_BOARD_EXT_PA (1U<<3)
#define RTWB14_BOARD_EXT_PA_5G (1U<<6)
typedef struct rtwb14_phy rtwb14_phy;
/* The CE branch supplies exact logical TSSI/thermal offsets. Front-end and
 * global thermal-reference provenance remain explicit board inputs; the AP
 * branch gets them from flash/MIB. Unknown metadata never selects TX power. */
typedef struct {
 uint8_t rfe_type,package_type,board_type,crystal_cap,thermal_base;
 uint16_t type_glna,type_gpa,type_alna,type_apa;
 const uint8_t *logical;
 size_t logical_bytes;
} rtwb14_phy_board;
/* Source txagc_table_8814b:4 CCK/OFDM references and one signed rate table.
 * Power must already include the actual factory and regulatory channel limit.
 * No association or station capability is inferred from a supplied plan. */
typedef struct {
 uint8_t cck[4],ofdm[4];
 int8_t difference[RTWB14_PHY_RATE_COUNT];
} rtwb14_phy_power;
typedef struct {
 uint8_t tables,channel,calibrated,tx_enabled,rf_paths,synthesizers;
 uint8_t dack_paths,iqk_paths,dpk_paths,lck_synths,txgapk_paths;
 uint8_t dpk_skipped,txgapk_skipped;
 int error;
} rtwb14_phy_status;
typedef struct {int8_t tssi[4][RTWB14_PHY_TSSI_GROUPS];uint8_t thermal[4],power_track_type;} rtwb14_phy_factory;
/* Pure decoded snapshot from the actual primary CE logical-map routines. */
int rtwb14_phy_get_factory(const rtwb14_phy*,rtwb14_phy_factory*);
size_t rtwb14_phy_size(void);
int rtwb14_phy_board_validate(const rtwb14_phy_board*);
/* Caller-owned aligned scratch and HAL stay at stable nonoverlapping addresses
 * until forget. All operations are serialized with firmware and station TX. */
int rtwb14_phy_open(void*,size_t,rtwb14_context*,const rtwb14_phy_board*,rtwb14_phy**);
int rtwb14_phy_load(rtwb14_phy*);
/* Initially20MHz, non-DFS channels1..11,36..48 and149..165 only. Geometry
 * does not grant regulatory permission; the station owner supplies that. */
int rtwb14_phy_tune(rtwb14_phy*,uint8_t,const rtwb14_phy_power*);
int rtwb14_phy_calibrate(rtwb14_phy*);
int rtwb14_phy_track(rtwb14_phy*);
int rtwb14_phy_resume(rtwb14_phy*);
int rtwb14_phy_stop(rtwb14_phy*);
/* Direct checked interfaces execute the original B14 RF/synth protocols. */
int rtwb14_phy_rf_read(rtwb14_phy*,unsigned,uint8_t,uint32_t,uint32_t*);
int rtwb14_phy_rf_write(rtwb14_phy*,unsigned,uint8_t,uint32_t,uint32_t);
int rtwb14_phy_synth_read(rtwb14_phy*,unsigned,uint8_t,uint32_t,uint32_t*);
int rtwb14_phy_synth_write(rtwb14_phy*,unsigned,uint8_t,uint32_t,uint32_t);
/* Pure cached status, no I/O. Forget revokes host state without touching USB. */
rtwb14_phy_status rtwb14_phy_get_status(const rtwb14_phy*);
void rtwb14_phy_forget(rtwb14_phy*);
#ifdef __cplusplus
}
#endif
#endif
