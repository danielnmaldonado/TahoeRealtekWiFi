/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8723BU_INTERNAL_H
#define RTW_RTL8723BU_INTERNAL_H
#include "rtw/RTL8723BU.h"
#ifdef KERNEL
#include <libkern/libkern.h>
#include <sys/errno.h>
#else
#include <string.h>
#include <errno.h>
#endif
#include <stdbool.h>
typedef uint8_t u8;typedef int8_t s8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;
typedef uint16_t __le16;typedef uint32_t __le32;
#define __packed __attribute__((packed))
#define __LITTLE_ENDIAN 1
#define ETH_ALEN 6
#define BIT(n) (1U<<(n))
#define GENMASK(h,l) ((0xffffffffU>>(31U-(h)))&(0xffffffffU<<(l)))
#include "RTL8723BURegisters.inc"
#include "RTL8723BUDefinitions.inc"
struct rtw_8723bu_private {
 rtw_8723bu *owner;bool cleanup;
 union {u8 raw[512];struct rtl8723bu_efuse efuse8723bu;}efuse_wifi;
 u8 rf_paths,tx_paths,rx_paths,default_crystal_cap,chip_cut;
 u8 cck_tx_power_index_A[6],cck_tx_power_index_B[6];
 u8 ht40_1s_tx_power_index_A[6],ht40_1s_tx_power_index_B[6];
 struct {s8 a,b;}ofdm_tx_power_diff[4],ht20_tx_power_diff[4],ht40_tx_power_diff[4];
 struct rtl8xxxu_btcoex bt_coex;
 u32 adda_backup[16],mac_backup[4],bb_backup[9],bb_recovery_backup[9];
 u32 rege94,rege9c,regeb4,regebc;
 bool iqk_ok,power_attempted;u8 last_thermal;
 unsigned rtl_chip;
 const struct rtl8xxxu_fileops *fops;
 const struct rtl8xxxu_firmware_header *fw_data;size_t fw_size;
 u8 ep_tx_count,ep_tx_high_queue,ep_tx_low_queue,ep_tx_normal_queue;
 u8 out_ep[4],pipe_out[0x20];
 struct rtl8xxxu_cfo_tracking cfo_tracking;
};
struct rtl8xxxu_fileops {
 void(*reset_8051)(struct rtw_8723bu_private*);
 u32 adda_1t_init,adda_1t_path_on,adda_2t_path_on_a,adda_2t_path_on_b;
 u16 max_aggr_num;u8 ampdu_max_time,has_s0s1,init_reg_hmtfr;
 u8 page_num_hi,page_num_lo,page_num_norm,total_page_num,ustime_tsf_edca;
};
enum {NL80211_CHAN_WIDTH_20_NOHT,NL80211_CHAN_WIDTH_20,NL80211_CHAN_WIDTH_40,NL80211_CHAN_WIDTH_80};
struct rtw_8723bu_channel {unsigned hw_value,center_freq;};
struct rtw_8723bu_hw {struct rtw_8723bu_private*priv;struct {struct {struct rtw_8723bu_channel*chan;unsigned width,center_freq1;}chandef;}conf;};
#endif
