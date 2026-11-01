/* SPDX-License-Identifier: GPL-2.0-only
 * Generated from tobyw121/Openwrt_RTL@85ef7eb7ccbd4336cd6e628a743ddc211333901c.
 * Vendor notices and primary algorithms preserved; checked host adapter only. */
/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

static boolean _do_dpk_8814b(void *dm_void)
{
 u32 mac_bp[DPK_MAC_NUM_8814B], bb_bp[DPK_BB_NUM_8814B];
 u32 rf_bp[DPK_RF_NUM_8814B][SS_8814B];
 u32 bp_mac_reg[DPK_MAC_NUM_8814B] = {0x520};
 u32 bp_bb_reg[DPK_BB_NUM_8814B] = {0x1c38, 0x1d58, 0x1d60, 0x180c, 0x410c,
           0x520c, 0x530c, 0x1d70, 0x1a00, 0x824,
           0x1a14, 0x1830, 0x4130, 0x5230, 0x5330,
           0x1860, 0x4160, 0x5260, 0x5360};
 u32 bp_rf_reg[DPK_RF_NUM_8814B] = {0x88, 0x8c, 0x8e, 0xde};
 u8 path = 0x0, retry_cnt = 0, tx_agc_search, k, dpk_step = 0x0;
 u8 gsk1_retry_count = 0x0, gsk3_retry_count = 0x0, gl_retry_count = 0x0;
 u16 i;
 boolean returnflag = false, KFAIL = true, gonext = false;
 
 u32 rf0 = 0x0;
 boolean log_result = true;
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 dpk_info->dpk_path_ok = 0x0;
 k = dpk_channel_transfer_8814b(dm);
 _dpk_bp_macbb_8814b(dm, mac_bp, bb_bp, bp_mac_reg, bp_bb_reg);
 _dpk_bp_rf_8814b(dm, rf_bp, bp_rf_reg);
 _dpk_mac_bb_setting_8814b(dm);
 _dpk_manual_txagc_8814b(dm, true);
 _dpk_con_tx_8814b(dm, true);
 _dpk_set_afe_8814b(dm, true);
 _dpk_rfe_setting_8814b(dm);
 for (path = 0; path < SS_8814B; path++) {
                                                                   ;
  _dpk_rfsetting_8814b(dm, path);
  gonext = true;
  dpk_step = 0x0;
  odm_set_bb_reg(dm, 0x1b20, BIT(26), 0x0);
  odm_set_bb_reg(dm, 0x1b20, BIT(25), 0x0);
  do {
  switch(dpk_step) {
  case 0:
   gonext = false;
   gsk3_retry_count = 0x0;
   while (1) {
    KFAIL = _dpk_pas_agc_8814b(dm, path, GSK3);
    if (DPK_PAS_DBG_8814B)
     _dpk_pas_gl_read_8814b(dm, path);
    if (KFAIL && (gsk3_retry_count < 2))
     gsk3_retry_count++;
    else if (KFAIL) {
     gonext = true;
    } else {
     dpk_step++;
     gonext = true;
    }
    if (gonext)
     break;
   }
    break;
  case 1:
   gonext = false;
   gsk1_retry_count = 0x0;
   while (1) {
    KFAIL = _dpk_pas_agc_8814b(dm, path, GSK1);
    if (DPK_PAS_DBG_8814B)
     _dpk_pas_gl_read_8814b(dm, path);
    if (KFAIL && (gsk1_retry_count < 4))
     gsk1_retry_count++;
    else if (KFAIL) {
     dpk_step = 5;
     gonext = true;
    } else {
     dpk_step++;
     gonext = true;
    }
    if (gonext)
     break;
   }
   break;
  case 2:
   gonext = true;
   dpk_info->tx_gain = _dpk_gainloss_8814b(dm, path);
   dpk_step++;
   gl_retry_count++;
   break;
  case 3:
   KFAIL = _dpk_by_path_8814b(dm, dpk_info->tx_gain, path, 0);
   dpk_step++;
   gonext = true;
   break;
  case 4:
   KFAIL = _dpk_check_fail_8814b(dm, KFAIL, path, k);
   dpk_step++;
   gonext = true;
   break;
  case 5:
   gonext = false;
   break;
  }
  if (dpk_step == 5) {
   if (!KFAIL) {
    dpk_info->dpk_path_ok = dpk_info->dpk_path_ok | BIT(path);
    dpk_info->is_dpk_enable = true;
   } else {
    dpk_info->dpk_path_ok = dpk_info->dpk_path_ok & ~BIT(path);
    dpk_info->is_dpk_enable = false;
   }
   for (i = 0; i < SS_8814B; i++) {
    odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | i << 1);
    odm_write_1byte(dm, 0x1bcc, 0x0);
   }
  _dpk_reload_8814b(dm, k, path);
  gonext = false;
  }
 } while (gonext);
 }
 dpk_thermal_read_8814b(dm);
 _dpk_set_afe_8814b(dm, false);
 _dpk_init_8814b(dm, false);
 _dpk_con_tx_8814b(dm, false);
 _dpk_manual_txagc_8814b(dm, false);
 _dpk_reload_macbb_8814b(dm, mac_bp, bb_bp, bp_mac_reg,
    bp_bb_reg);
 _dpk_reload_rf_8814b(dm, bp_rf_reg, rf_bp);
 return false;
}

static void _dpk_bp_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
    u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg)
{
 u32 i;
 for (i = 0; i < DPK_MAC_NUM_8814B; i++)
  mac_bp[i] = odm_read_4byte(dm, bp_mac_reg[i]);
 for (i = 0; i < DPK_BB_NUM_8814B; i++)
  bb_bp[i] = odm_read_4byte(dm, bp_bb_reg[i]);
}

static void _dpk_bp_rf_8814b(struct dm_struct *dm, u32 rf_bp[][SS_8814B],
        u32 *bp_reg)
{
 u8 i, s;
 for (s = 0; s < SS_8814B; s++) {
  for (i = 0; i < DPK_RF_NUM_8814B; i++) {
   rf_bp[i][s] = odm_get_rf_reg(dm, (enum rf_path)s,
           bp_reg[i], MASK20BITS);
  }
 }
}

static boolean _dpk_by_path_8814b(struct dm_struct *dm, u8 tx_agc_search, u8 path,
         u8 group)
{
 u8 reg_1b67;
 u8 tx_agc = 0x0, result = 1;
 u32 tx_agc_ori = 0x0;
 u8 tmp, i;
 u8 rf_0x56 = 0x0;
 boolean log_result;
 tx_agc_ori = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0x0001f);
 tmp = (u8)(tx_agc_ori & 0x1f);
 tx_agc = tmp - tx_agc_search;
 reg_1b67 = tx_agc;
 odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK,
         (0x50000 | tx_agc));
 ODM_delay_us(50);
 rf_0x56 = (u8) odm_get_rf_reg(dm, (enum rf_path)path, RF_0x56, 0x0001f);
                                                                           ;
                             ;
 if (dm->cut_version < 0x2) {
  if (rf_0x56 == 0xd) {
   reg_1b67++;
   tx_agc++;
   rf_56_txbb_flag = true;
                                                                 ;
                                                                      ;
  } else {
   rf_56_txbb_flag = false;
                                                                      ;
  }
 }
 if (*dm->band_type == ODM_BAND_5G) {
  if ((*dm->band_width == 2) ){
                                                    ;
   reg_1b67 = reg_1b67 + 1;
                                         ;
                                                        ;
  } else if ( *dm->band_width == 1) {
   reg_1b67 = reg_1b67 + 1;
                                         ;
                                                        ;
  }
 }
                             ;
 odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, RFREG_MASK,
         (0x50000 | tx_agc));
 ODM_delay_us(50);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_write_1byte(dm, R_0x1b67, reg_1b67);
 odm_write_1byte(dm, 0x1b49, 0x0);
 odm_set_bb_reg(dm, 0x1b40, 0xffff0000, 0x2000);
                                                         ;
 result = _dpk_one_shot_8814b(dm, path, 0x1);
 odm_write_1byte(dm, 0x1b49, 0x0);
                                                                         ;
 if (DPK_PAS_DBG_8814B)
  _dpk_pas_dpk_read_8814b(dm, path);
 return result;
}

static boolean _dpk_check_fail_8814b(struct dm_struct *dm, boolean is_fail, u8 path,
    u8 group)
{
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 reg1b67 = 0x0;
 boolean fail = true;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 reg1b67 = odm_read_1byte(dm, R_0x1b67);
 if (reg1b67 <= 0x12)
  is_fail = true;
 if (!is_fail) {
  if (_dpk_lut_sram_read_8814b(dm, group, path)) {
                      ;
   if (group < 3) {
    dpk_info->pwsf_2g[path][group] =
    odm_read_1byte(dm, R_0x1b67);
    dpk_info->dpk_result[path][group] = 1;
   } else {
    dpk_info->pwsf_5g[path][group - G_2G_N] =
     odm_read_1byte(dm, R_0x1b67);
    dpk_info->dpk_5g_result[path][group - G_2G_N] =
               1;
   }
   fail = false;
  } else {
   if (group < 3) {
    dpk_info->pwsf_2g[path][group] = 0;
    dpk_info->dpk_result[path][group] = 0;
   } else {
    dpk_info->pwsf_5g[path][group - G_2G_N] = 0;
    dpk_info->dpk_5g_result[path][group - G_2G_N] =
               0;
   }
   fail = false;
  }
 } else {
  if(group<G_2G_N){
   dpk_info->pwsf_2g[path][group]=0;
   dpk_info->dpk_result[path][group]=0;
  }else{
   dpk_info->pwsf_5g[path][group-G_2G_N]=0;
   dpk_info->dpk_5g_result[path][group-G_2G_N]=0;
  }
  fail = true;
 }
 return fail;
}

static boolean
_dpk_check_nctl_done_8814b(struct dm_struct *dm, u8 path, u32 IQK_CMD)
{
 boolean notready = true;
 boolean fail = true;
 u32 delay_count = 0x0;
 while (notready) {
  if (odm_read_1byte(dm, 0x2d9c) == 0x55)
   notready = false;
  else
   notready = true;
  if (notready) {
   ODM_delay_us(50);
   delay_count++;
  } else {
   fail = (boolean)odm_get_bb_reg(dm, 0x1b08, BIT(26));
   break;
  }
  if (delay_count >= 100) {
               ;
   break;
  }
 }
 odm_write_1byte(dm, 0x1b10, 0x0);
 odm_write_1byte(dm, 0x2d9c, 0x0);
 return fail;
}

static void _dpk_con_tx_8814b(struct dm_struct *dm, boolean is_contx)
{
}

static u8 _dpk_gainloss_8814b(struct dm_struct *dm, u8 path)
{
 u8 k = 0x0, tx_agc_search = 0x0, result[5] = {0x0, 0x0, 0x0, 0x0, 0x0};
 s8 txidx_offset = 0x0;
 u8 i = 0x0;
 boolean log_result;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_write_1byte(dm, 0x1b49, 0x0);
 for (k = 0; k < 1; k++) {
  _dpk_one_shot_8814b(dm, path, 0x0);
  odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | 0x8);
  odm_set_bb_reg(dm, R_0x1bd4, MASKDWORD, 0x00060001);
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, 0x00000000);
  odm_write_1byte(dm, 0x1b49, 0x40);
  result[k] = (u8)((odm_read_1byte(dm, R_0x1bfc) & 0xf0) >> 4);
                      ;
 }
 tx_agc_search = result[0];
                            ;
 odm_write_1byte(dm, 0x1b49, 0x00);
 if (DPK_PAS_DBG_8814B)
  _dpk_pas_gl_read_8814b(dm, path);
 return tx_agc_search;
}

static void _dpk_init_8814b(struct dm_struct *dm, boolean start_dpk)
{
 int path;
                                                      ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | RF_PATH_A << 1);
 if (start_dpk) {
  odm_write_4byte(dm, 0x1b40, 0x40000000);
  odm_write_4byte(dm, 0x1b44, 0x24002064);
  odm_write_4byte(dm, 0x1b48, 0x0005002D);
  odm_write_4byte(dm, 0x1b54, 0x00100002);
  odm_write_4byte(dm, 0x1b60, 0x1F0f0000);
  odm_write_4byte(dm, 0x1b64, 0x1C100000);
  odm_write_4byte(dm, 0x1b40, 0x00000000);
  odm_write_4byte(dm, 0x1b4c, 0x00000000);
  odm_write_4byte(dm, 0x1b4c, 0x008a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000377);
  odm_write_4byte(dm, 0x1b4c, 0x018a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000054B);
  odm_write_4byte(dm, 0x1b4c, 0x028a0000);
  odm_write_4byte(dm, 0x1b50, 0x000006A2);
  odm_write_4byte(dm, 0x1b4c, 0x038a0000);
  odm_write_4byte(dm, 0x1b50, 0x000007BF);
  odm_write_4byte(dm, 0x1b4c, 0x048a0000);
  odm_write_4byte(dm, 0x1b50, 0x000008B8);
  odm_write_4byte(dm, 0x1b4c, 0x058a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000997);
  odm_write_4byte(dm, 0x1b4c, 0x068a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000A64);
  odm_write_4byte(dm, 0x1b4c, 0x078a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000B23);
  odm_write_4byte(dm, 0x1b4c, 0x088a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000BD5);
  odm_write_4byte(dm, 0x1b4c, 0x098a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000C7D);
  odm_write_4byte(dm, 0x1b4c, 0x0a8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000D1D);
  odm_write_4byte(dm, 0x1b4c, 0x0b8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000DB6);
  odm_write_4byte(dm, 0x1b4c, 0x0c8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000E48);
  odm_write_4byte(dm, 0x1b4c, 0x0d8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000ED5);
  odm_write_4byte(dm, 0x1b4c, 0x0e8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000F5D);
  odm_write_4byte(dm, 0x1b4c, 0x0f8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00000FE0);
  odm_write_4byte(dm, 0x1b4c, 0x108a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000105F);
  odm_write_4byte(dm, 0x1b4c, 0x118a0000);
  odm_write_4byte(dm, 0x1b50, 0x000010DA);
  odm_write_4byte(dm, 0x1b4c, 0x128a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001152);
  odm_write_4byte(dm, 0x1b4c, 0x138a0000);
  odm_write_4byte(dm, 0x1b50, 0x000011C7);
  odm_write_4byte(dm, 0x1b4c, 0x148a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001239);
  odm_write_4byte(dm, 0x1b4c, 0x158a0000);
  odm_write_4byte(dm, 0x1b50, 0x000012A8);
  odm_write_4byte(dm, 0x1b4c, 0x168a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001314);
  odm_write_4byte(dm, 0x1b4c, 0x178a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000137E);
  odm_write_4byte(dm, 0x1b4c, 0x188a0000);
  odm_write_4byte(dm, 0x1b50, 0x000013E6);
  odm_write_4byte(dm, 0x1b4c, 0x198a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000144C);
  odm_write_4byte(dm, 0x1b4c, 0x1a8a0000);
  odm_write_4byte(dm, 0x1b50, 0x000014B0);
  odm_write_4byte(dm, 0x1b4c, 0x1b8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001512);
  odm_write_4byte(dm, 0x1b4c, 0x1c8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001573);
  odm_write_4byte(dm, 0x1b4c, 0x1d8a0000);
  odm_write_4byte(dm, 0x1b50, 0x000015D1);
  odm_write_4byte(dm, 0x1b4c, 0x1e8a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000162E);
  odm_write_4byte(dm, 0x1b4c, 0x1f8a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000168A);
  odm_write_4byte(dm, 0x1b4c, 0x208a0000);
  odm_write_4byte(dm, 0x1b50, 0x000016E4);
  odm_write_4byte(dm, 0x1b4c, 0x218a0000);
  odm_write_4byte(dm, 0x1b50, 0x0000173D);
  odm_write_4byte(dm, 0x1b4c, 0x228a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001794);
  odm_write_4byte(dm, 0x1b4c, 0x238a0000);
  odm_write_4byte(dm, 0x1b50, 0x000017EB);
  odm_write_4byte(dm, 0x1b4c, 0x248a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001840);
  odm_write_4byte(dm, 0x1b4c, 0x258a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001894);
  odm_write_4byte(dm, 0x1b4c, 0x268a0000);
  odm_write_4byte(dm, 0x1b50, 0x000018E6);
  odm_write_4byte(dm, 0x1b4c, 0x278a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001938);
  odm_write_4byte(dm, 0x1b4c, 0x288a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001989);
  odm_write_4byte(dm, 0x1b4c, 0x298a0000);
  odm_write_4byte(dm, 0x1b50, 0x000019D8);
  odm_write_4byte(dm, 0x1b4c, 0x2a8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001A27);
  odm_write_4byte(dm, 0x1b4c, 0x2b8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001A75);
  odm_write_4byte(dm, 0x1b4c, 0x2c8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001AC2);
  odm_write_4byte(dm, 0x1b4c, 0x2d8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001B0E);
  odm_write_4byte(dm, 0x1b4c, 0x2e8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001B59);
  odm_write_4byte(dm, 0x1b4c, 0x2f8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001BA4);
  odm_write_4byte(dm, 0x1b4c, 0x308a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001BEE);
  odm_write_4byte(dm, 0x1b4c, 0x318a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001C37);
  odm_write_4byte(dm, 0x1b4c, 0x328a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001C7F);
  odm_write_4byte(dm, 0x1b4c, 0x338a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001CC6);
  odm_write_4byte(dm, 0x1b4c, 0x348a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001D0D);
  odm_write_4byte(dm, 0x1b4c, 0x358a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001D53);
  odm_write_4byte(dm, 0x1b4c, 0x368a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001D99);
  odm_write_4byte(dm, 0x1b4c, 0x378a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001DDE);
  odm_write_4byte(dm, 0x1b4c, 0x388a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001E22);
  odm_write_4byte(dm, 0x1b4c, 0x398a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001E66);
  odm_write_4byte(dm, 0x1b4c, 0x3a8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001EA9);
  odm_write_4byte(dm, 0x1b4c, 0x3b8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001EEB);
  odm_write_4byte(dm, 0x1b4c, 0x3c8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001F2D);
  odm_write_4byte(dm, 0x1b4c, 0x3d8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001F6F);
  odm_write_4byte(dm, 0x1b4c, 0x3e8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001FB0);
  odm_write_4byte(dm, 0x1b4c, 0x3f8a0000);
  odm_write_4byte(dm, 0x1b50, 0x00001FF0);
  odm_write_4byte(dm, 0x1b4c, 0x00000000);
  odm_write_4byte(dm, 0x1b50, 0x00000000);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b58, 0x00898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0CA60FED);
  odm_write_4byte(dm, 0x1b58, 0x02898F00);
  odm_write_4byte(dm, 0x1b5C, 0x07FB0A0C);
  odm_write_4byte(dm, 0x1b58, 0x04898F00);
  odm_write_4byte(dm, 0x1b5C, 0x05090657);
  odm_write_4byte(dm, 0x1b58, 0x06898F00);
  odm_write_4byte(dm, 0x1b5C, 0x032D0400);
  odm_write_4byte(dm, 0x1b58, 0x08898F00);
  odm_write_4byte(dm, 0x1b5C, 0x02010286);
  odm_write_4byte(dm, 0x1b58, 0x0A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x01440198);
  odm_write_4byte(dm, 0x1b58, 0x0C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00CC0101);
  odm_write_4byte(dm, 0x1b58, 0x0E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x008100A2);
  odm_write_4byte(dm, 0x1b58, 0x10898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00510066);
  odm_write_4byte(dm, 0x1b58, 0x12898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00330041);
  odm_write_4byte(dm, 0x1b58, 0x14898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00200029);
  odm_write_4byte(dm, 0x1b58, 0x16898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0014001A);
  odm_write_4byte(dm, 0x1b58, 0x18898F00);
  odm_write_4byte(dm, 0x1b5C, 0x000D0010);
  odm_write_4byte(dm, 0x1b58, 0x1A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0008000A);
  odm_write_4byte(dm, 0x1b58, 0x1C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00050006);
  odm_write_4byte(dm, 0x1b58, 0x1E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00030004);
  odm_write_4byte(dm, 0x1b58, 0x20898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00020003);
  odm_write_4byte(dm, 0x1b58, 0x22898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00010002);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b5C, 0x00000000);
  odm_write_4byte(dm, 0x1b00, 0x0000000A);
  odm_write_4byte(dm, 0x1b54, 0x00100002);
  odm_write_4byte(dm, 0x1b60, 0x1F0f0000);
  odm_write_4byte(dm, 0x1b64, 0x1C100000);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b58, 0x00898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0CA60FED);
  odm_write_4byte(dm, 0x1b58, 0x02898F00);
  odm_write_4byte(dm, 0x1b5C, 0x07FB0A0C);
  odm_write_4byte(dm, 0x1b58, 0x04898F00);
  odm_write_4byte(dm, 0x1b5C, 0x05090657);
  odm_write_4byte(dm, 0x1b58, 0x06898F00);
  odm_write_4byte(dm, 0x1b5C, 0x032D0400);
  odm_write_4byte(dm, 0x1b58, 0x08898F00);
  odm_write_4byte(dm, 0x1b5C, 0x02010286);
  odm_write_4byte(dm, 0x1b58, 0x0A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x01440198);
  odm_write_4byte(dm, 0x1b58, 0x0C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00CC0101);
  odm_write_4byte(dm, 0x1b58, 0x0E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x008100A2);
  odm_write_4byte(dm, 0x1b58, 0x10898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00510066);
  odm_write_4byte(dm, 0x1b58, 0x12898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00330041);
  odm_write_4byte(dm, 0x1b58, 0x14898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00200029);
  odm_write_4byte(dm, 0x1b58, 0x16898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0014001A);
  odm_write_4byte(dm, 0x1b58, 0x18898F00);
  odm_write_4byte(dm, 0x1b5C, 0x000D0010);
  odm_write_4byte(dm, 0x1b58, 0x1A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0008000A);
  odm_write_4byte(dm, 0x1b58, 0x1C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00050006);
  odm_write_4byte(dm, 0x1b58, 0x1E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00030004);
  odm_write_4byte(dm, 0x1b58, 0x20898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00020003);
  odm_write_4byte(dm, 0x1b58, 0x22898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00010002);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b5C, 0x00000000);
  odm_write_4byte(dm, 0x1b00, 0x0000000C);
  odm_write_4byte(dm, 0x1b54, 0x00100002);
  odm_write_4byte(dm, 0x1b60, 0x1F0f0000);
  odm_write_4byte(dm, 0x1b64, 0x1C100000);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b58, 0x00898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0CA60FED);
  odm_write_4byte(dm, 0x1b58, 0x02898F00);
  odm_write_4byte(dm, 0x1b5C, 0x07FB0A0C);
  odm_write_4byte(dm, 0x1b58, 0x04898F00);
  odm_write_4byte(dm, 0x1b5C, 0x05090657);
  odm_write_4byte(dm, 0x1b58, 0x06898F00);
  odm_write_4byte(dm, 0x1b5C, 0x032D0400);
  odm_write_4byte(dm, 0x1b58, 0x08898F00);
  odm_write_4byte(dm, 0x1b5C, 0x02010286);
  odm_write_4byte(dm, 0x1b58, 0x0A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x01440198);
  odm_write_4byte(dm, 0x1b58, 0x0C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00CC0101);
  odm_write_4byte(dm, 0x1b58, 0x0E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x008100A2);
  odm_write_4byte(dm, 0x1b58, 0x10898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00510066);
  odm_write_4byte(dm, 0x1b58, 0x12898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00330041);
  odm_write_4byte(dm, 0x1b58, 0x14898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00200029);
  odm_write_4byte(dm, 0x1b58, 0x16898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0014001A);
  odm_write_4byte(dm, 0x1b58, 0x18898F00);
  odm_write_4byte(dm, 0x1b5C, 0x000D0010);
  odm_write_4byte(dm, 0x1b58, 0x1A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0008000A);
  odm_write_4byte(dm, 0x1b58, 0x1C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00050006);
  odm_write_4byte(dm, 0x1b58, 0x1E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00030004);
  odm_write_4byte(dm, 0x1b58, 0x20898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00020003);
  odm_write_4byte(dm, 0x1b58, 0x22898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00010002);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b5C, 0x00000000);
  odm_write_4byte(dm, 0x1b00, 0x0000000E);
  odm_write_4byte(dm, 0x1b54, 0x00100002);
  odm_write_4byte(dm, 0x1b60, 0x1F0f0000);
  odm_write_4byte(dm, 0x1b64, 0x1C100000);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b58, 0x00898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0CA60FED);
  odm_write_4byte(dm, 0x1b58, 0x02898F00);
  odm_write_4byte(dm, 0x1b5C, 0x07FB0A0C);
  odm_write_4byte(dm, 0x1b58, 0x04898F00);
  odm_write_4byte(dm, 0x1b5C, 0x05090657);
  odm_write_4byte(dm, 0x1b58, 0x06898F00);
  odm_write_4byte(dm, 0x1b5C, 0x032D0400);
  odm_write_4byte(dm, 0x1b58, 0x08898F00);
  odm_write_4byte(dm, 0x1b5C, 0x02010286);
  odm_write_4byte(dm, 0x1b58, 0x0A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x01440198);
  odm_write_4byte(dm, 0x1b58, 0x0C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00CC0101);
  odm_write_4byte(dm, 0x1b58, 0x0E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x008100A2);
  odm_write_4byte(dm, 0x1b58, 0x10898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00510066);
  odm_write_4byte(dm, 0x1b58, 0x12898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00330041);
  odm_write_4byte(dm, 0x1b58, 0x14898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00200029);
  odm_write_4byte(dm, 0x1b58, 0x16898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0014001A);
  odm_write_4byte(dm, 0x1b58, 0x18898F00);
  odm_write_4byte(dm, 0x1b5C, 0x000D0010);
  odm_write_4byte(dm, 0x1b58, 0x1A898F00);
  odm_write_4byte(dm, 0x1b5C, 0x0008000A);
  odm_write_4byte(dm, 0x1b58, 0x1C898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00050006);
  odm_write_4byte(dm, 0x1b58, 0x1E898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00030004);
  odm_write_4byte(dm, 0x1b58, 0x20898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00020003);
  odm_write_4byte(dm, 0x1b58, 0x22898F00);
  odm_write_4byte(dm, 0x1b5C, 0x00010002);
  odm_write_4byte(dm, 0x1b58, 0x00000000);
  odm_write_4byte(dm, 0x1b5C, 0x00000000);
 }
 return;
}

static boolean _dpk_lut_sram_read_8814b(void *dm_void, u8 group, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 i;
 u32 reg_1b58 = 0, reg_1bfc = 0;
 boolean result = true;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_set_bb_reg(dm, 0x1b54, BIT(1), 0x0);
 odm_write_4byte(dm, 0x1bd4, 0x00260001);
                    ;
 for (i = 0; i < 64; i++) {
  odm_write_4byte(dm, R_0x1b58, 0x00388800 | i << 24);
  reg_1b58 = odm_read_4byte(dm, R_0x1b58);
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
  if (group < G_2G_N)
   dpk_info->lut_2g[path][group][i] = reg_1bfc;
  else
   dpk_info->lut_5g[path][group - G_2G_N][i] = reg_1bfc;
 }
 odm_write_4byte(dm, 0x1b58, 0x00000000);
 return result;
}

static void _dpk_lut_sram_write_8814b(void *dm_void, u8 group, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 i;
 u32 reg_1b5c, tmp;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_write_1byte(dm, 0x1b54, 0x0);
 odm_write_4byte(dm, 0x1bd4, 0x00260001);
 for (i = 0; i < 64; i++) {
  if (group < G_2G_N)
   tmp = dpk_info->lut_2g[path][group][i];
  else
   tmp = dpk_info->lut_5g[path][group - G_2G_N][i];
  odm_write_4byte(dm, R_0x1b58, 0x00B88800 | i << 24 );
  odm_write_4byte(dm, R_0x1b5c, tmp);
 }
 odm_write_4byte(dm, R_0x1b58, 0x0);
 odm_write_4byte(dm, R_0x1b5c, 0x0);
}

static void _dpk_mac_bb_setting_8814b(struct dm_struct *dm)
{
 odm_write_1byte(dm, REG_TXPAUSE, 0xff);
 odm_set_bb_reg(dm, 0x1e24, BIT(17), 0x1);
 odm_set_bb_reg(dm, 0x1cd0, 0xf0000000, 0x7);
 odm_set_bb_reg(dm, 0x1834, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x4134, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x5234, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x5334, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x1d60, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x1a14, 0x300, 0x3);
 odm_set_bb_reg(dm, 0x1a00, 0x3, 0x2);
 odm_set_bb_reg(dm, 0x824, 0xf0000, 0xf);
 odm_set_bb_reg(dm, 0x186c, BIT(7), 0x0);
 odm_set_bb_reg(dm, 0x416c, BIT(7), 0x0);
 odm_set_bb_reg(dm, 0x526c, BIT(7), 0x0);
 odm_set_bb_reg(dm, 0x536c, BIT(7), 0x0);
 odm_set_bb_reg(dm, 0x180c, 0x3, 0x0);
 odm_set_bb_reg(dm, 0x410c, 0x3, 0x0);
 odm_set_bb_reg(dm, 0x520c, 0x3, 0x0);
 odm_set_bb_reg(dm, 0x530c, 0x3, 0x0);
 odm_set_bb_reg(dm, R_0x1b08, BIT(7), 0x1);
                                                              ;
}

static void _dpk_manual_txagc_8814b(struct dm_struct *dm, boolean is_manual)
{
 odm_set_bb_reg(dm, R_0x18a4, BIT(7), is_manual);
 odm_set_bb_reg(dm, R_0x41a4, BIT(7), is_manual);
 odm_set_bb_reg(dm, R_0x52a4, BIT(7), is_manual);
 odm_set_bb_reg(dm, R_0x53a4, BIT(7), is_manual);
}

static void _dpk_on_8814b(struct dm_struct *dm, u8 path)
{
 u8 i = 0x0;
 boolean log_result;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 rf_0x56 = 0x0;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_write_1byte(dm, 0x1b54, 0x01);
 odm_write_1byte(dm, 0x1b59, 0x88);
 if (rf_56_txbb_flag == true) {
  odm_set_bb_reg(dm, 0x1b54, 0x001fff00, 0x169a);
                                                       ;
 }
 else {
  odm_set_bb_reg(dm, 0x1b54, 0x001fff00, 0x1425);
                                                       ;
 }
 odm_write_1byte(dm, 0x1b58, 0x00);
 odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x0);
                                                                   ;
}

static boolean _dpk_one_shot_8814b(struct dm_struct *dm, u8 path, u8 action)
{
 u8 temp = 0x0;
 u16 dpk_cmd = 0x0;
 boolean result = false;
 temp = ((1 << (path + 4)) | (8 + (path << 1)));
 dpk_cmd = 0x1000 | (action << 8) | temp;
 odm_write_1byte(dm, 0x2d9c, 0x0);
 odm_write_1byte(dm, 0x1b10, 0x0);
 odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, dpk_cmd);
 odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, dpk_cmd + 1);
 ODM_delay_us(50);
 result = _dpk_check_nctl_done_8814b(dm, path, dpk_cmd);
 odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, IQK_CMD_8814B | (path << 1));
 return result;
}

static boolean _dpk_pas_agc_8814b(
 struct dm_struct *dm,
 u8 path,
 u8 gs)
{
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u32 rf0;
 boolean fail = true, kfail = true;
                                                                   ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_write_1byte(dm, 0x1b49, 0x0);
 rf0 = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xfffff);
                                                  ;
 kfail = _dpk_one_shot_8814b(dm, path, 0x0);
 fail = _dpk_pas_rxbbagc_8814b(dm, path, gs);
 return fail;
}

static void _dpk_pas_dpk_read_8814b(struct dm_struct *dm, u8 path)
{
 u8 k, j;
 u32 reg_1bfc, tmp;
                             ;
                                                         ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | 0x8);
 odm_write_1byte(dm, 0x1b49, 0x00);
 odm_write_4byte(dm, 0x1bd4, 0x00060001);
                                                ;
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x00090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x08090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x10090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x18090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x20090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x28090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x30090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x38090000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
                             ;
 odm_write_4byte(dm, 0x1b4c, 0x00000000);
}

static u32 _dpk_pas_get_iq_8814b(
 struct dm_struct *dm,
 u8 gs_type)
{
 s32 i_val = 0, q_val = 0;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | 0x8);
 odm_write_1byte(dm, 0x1b49, 0x00);
 if (gs_type == GSK3) {
  odm_write_4byte(dm, 0x1bd4, 0x00060001);
  odm_write_4byte(dm, 0x1b4c, 0x00080000);
 } else if (gs_type == GSK2){
  odm_write_4byte(dm, 0x1bd4, 0x00060001);
  odm_write_4byte(dm, 0x1b4c, 0x37080000);
 } else if(gs_type == GSK1) {
  odm_write_4byte(dm, 0x1bd4, 0x00060001);
  odm_write_4byte(dm, 0x1b4c, 0x3F080000);
 }
 i_val = odm_get_bb_reg(dm, R_0x1bfc, MASKHWORD);
 q_val = odm_get_bb_reg(dm, R_0x1bfc, MASKLWORD);
 if (i_val >> 15 != 0)
  i_val = 0x10000 - i_val;
 if (q_val >> 15 != 0)
  q_val = 0x10000 - q_val;
 odm_write_4byte(dm, 0x1b4c, 0x0);
 return i_val*i_val + q_val*q_val;
}

static void _dpk_pas_gl_read_8814b(struct dm_struct *dm, u8 path)
{
 u8 k, j;
 u32 reg_1bfc, reg_1b4c, tmp;
                             ;
                                                        ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | 0x8);
 odm_write_1byte(dm, 0x1b49, 0x00);
 odm_write_4byte(dm, 0x1bd4, 0x00060001);
                                                ;
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x00080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
  }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x08080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x10080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x18080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x20080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x28080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x30080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
 for (k = 0; k < 8; k++) {
  odm_set_bb_reg(dm, R_0x1b4c, MASKDWORD, (0x38080000 | k << 24));
  reg_1bfc = odm_read_4byte(dm, R_0x1bfc);
                                                         ;
 }
                             ;
 odm_write_4byte(dm, 0x1b4c, 0x00000000);
}

static s32 _dpk_pas_iq_check_8814b(
 struct dm_struct *dm,
 u8 path,
 u8 gs_type)
{
 u8 result = 0;
 u32 i_val = 0, q_val = 0, loss = 0, gain = 0;
 s32 loss_db = 0;
 s32 gain_db = 0;
 s32 gain_db2 = 0;
 gain = _dpk_pas_get_iq_8814b(dm, gs_type) >> 3;
 gain_db = (s32)(3 *(halrf_psd_log2base(gain)/10 - 280));
 gain_db2 = halrf_psd_log2base(gain);
                                                                     ;
 return gain_db;
}

static boolean _dpk_pas_rxbbagc_8814b(
 struct dm_struct *dm,
 u8 path,
 u8 gs_type)
{
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 tmp_txagc, tmp_rxbb, i = 0;
 s32 gain_db;
 u32 rf0;
 boolean fail = true, isbnd = false;
 if (gs_type == GSK3) {
  gain_db = _dpk_pas_iq_check_8814b(dm, path, gs_type);
  tmp_rxbb = (u8)odm_get_rf_reg(dm, (enum rf_path)path,
       RF_0x00, 0x003e0);
  rf0 = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xfffff);
                                                             ;
  if (gain_db > -110) {
   tmp_rxbb = tmp_rxbb - 1;
   fail = true;
                                                 ;
  } else {
   fail = false;
                                                 ;
  }
  odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, RF_0x00, 0x003e0, tmp_rxbb);
  rf0 = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xfffff);
                                                             ;
 }
 if (gs_type == GSK2)
  fail = false;
 if (gs_type == GSK1) {
  gain_db = _dpk_pas_iq_check_8814b(dm, path, gs_type);
  tmp_txagc = (u8)odm_get_rf_reg(dm, (enum rf_path)path,
       RF_0x00, 0x0001f);
  tmp_rxbb = (u8)odm_get_rf_reg(dm, (enum rf_path)path,
       RF_0x00, 0x003e0);
                             ;
  if (gain_db < -185){
   if (tmp_rxbb >= 0x1d) {
    isbnd = true;
    tmp_rxbb = 0x1f;
    tmp_txagc = tmp_txagc + 2;
   } else
    tmp_rxbb = tmp_rxbb + 2;
   fail = true;
                                                 ;
  } else if (gain_db > -115){
   if(tmp_rxbb <= 0x3) {
    isbnd = true;
    tmp_rxbb = 0x1;
    tmp_txagc = tmp_txagc - 2;
   } else
    tmp_rxbb = tmp_rxbb - 2;
   fail = true;
                                                 ;
  } else {
   fail = false;
                                                 ;
  }
  if (isbnd == true)
   fail = true;
  if (fail) {
   odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
   odm_set_rf_reg(dm, (enum rf_path)path,
        RF_0x00, 0x003e0, tmp_rxbb);
   odm_set_rf_reg(dm, (enum rf_path)path,
        RF_0x00, 0x0001f, tmp_txagc);
  } else {
   dpk_info->txbb[path] = tmp_txagc;
   dpk_info->rxbb[path] = tmp_rxbb;
  }
                             ;
  rf0 = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x00, 0xfffff);
                                                       ;
 }
 return fail;
}

static void _dpk_reload_8814b(void *dm_void, u8 group, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 _dpk_lut_sram_write_8814b(dm, group, path);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 if ((dpk_info->dpk_path_ok & BIT(path)) >> path) {
                                                             ;
  _dpk_on_8814b(dm, path);
 } else {
  odm_set_bb_reg(dm, R_0x1b54, BIT(0), 0x0);
  odm_set_bb_reg(dm, R_0x1b20, BIT(25), 0x1);
                                                             ;
 }
 odm_set_bb_reg(dm, R_0x1b20, BIT(26), 0x1);
 odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0xf0000, 0x3);
                                                                ;
                             ;
}

static void _dpk_reload_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
        u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg)
{
 ++dm->port->restore_depth;

 u32 i;
 odm_write_4byte(dm, 0x1d70, 0x50505050);
 for (i = 0; i < DPK_MAC_NUM_8814B; i++)
  odm_write_4byte(dm, bp_mac_reg[i], mac_bp[i]);
 for (i = 0; i < DPK_BB_NUM_8814B; i++)
  odm_write_4byte(dm, bp_bb_reg[i], bb_bp[i]);
 odm_set_bb_reg(dm, 0x180c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x410c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x520c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x530c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x1834, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x4134, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x5234, BIT(15), 0x1);
 odm_set_bb_reg(dm, 0x5334, BIT(15), 0x1);
--dm->port->restore_depth;
}

static void _dpk_reload_rf_8814b(struct dm_struct *dm, u32 *reg,
      u32 data[][SS_8814B])
{
 ++dm->port->restore_depth;

 u32 i;
 odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_B, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_C, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_D, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_A, RF_0xdf, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_B, RF_0xdf, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_C, RF_0xdf, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_D, RF_0xdf, MASK20BITS, 0x0);
 for (i = 0; i < DPK_RF_NUM_8814B; i++) {
                                                                    ;
  odm_set_rf_reg(dm, RF_PATH_A, reg[i], 0xfffff, data[i][0]);
  odm_set_rf_reg(dm, RF_PATH_B, reg[i], 0xfffff, data[i][1]);
  odm_set_rf_reg(dm, RF_PATH_C, reg[i], 0xfffff, data[i][2]);
  odm_set_rf_reg(dm, RF_PATH_D, reg[i], 0xfffff, data[i][3]);
 }
--dm->port->restore_depth;
}

static void _dpk_rfe_setting_8814b(struct dm_struct *dm)
{
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 switch (dm->rfe_type) {
 case 0x3 :
                                                             ;
  odm_set_bb_reg(dm, 0x183c, BIT(4), 0x1);
  odm_set_bb_reg(dm, 0x413c, BIT(13), 0x1);
  odm_set_bb_reg(dm, 0x523c, BIT(8), 0x1);
  odm_set_bb_reg(dm, 0x533c, BIT(10), 0x1);
  odm_set_bb_reg(dm, 0x1840, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x1844, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x4140, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x4144, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x5240, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x5244, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x5340, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, 0x5344, MASKDWORD, 0x77777777);
  break;
 default:
  break;
 }
}

static void _dpk_rfsetting_8814b(struct dm_struct *dm, u8 path)
{
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 u32 rf_reg00 = 0x0l, rf0;
 u8 tx_agc_init_value = 0x1c;
 u8 rxbb = 0x0, mixgain = 0x0;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 if (*dm->band_type == ODM_BAND_2_4G) {
  tx_agc_init_value = 0x1a;
  rf_reg00 = 0x50000 + tx_agc_init_value;
  odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0xfffff, rf_reg00);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x5c, 0x3000, 0x1);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x5c, 0xf0000, 0xe);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x80, BIT(10), 0x1);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0x3e0, 0x5);
  odm_set_rf_reg(dm, (enum rf_path)path, 0xdf, BIT(16), 0x1);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x88, 0x300, 0x2);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x88, 0xc00, 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8e, BIT(5), 0x0);
 } else {
  if (path == 0x0) {
   tx_agc_init_value = 0x1c;
   rxbb = 0x7;
   mixgain = 0xb;
  } else if (path == 0x1) {
   tx_agc_init_value = 0x1c;
   rxbb = 0x7;
   mixgain = 0xb;
  } else if (path == 0x2) {
   tx_agc_init_value = 0x1c;
   rxbb = 0x7;
   mixgain = 0xb;
  } else if (path == 0x3) {
   tx_agc_init_value = 0x1c;
   rxbb = 0x7;
   mixgain = 0xb;
  }
  rf_reg00 = 0x50000 + tx_agc_init_value;
  odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0xfffff, rf_reg00);
   odm_set_rf_reg(dm, (enum rf_path)path, 0x63, 0x30000, 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x63, 0xf000, 0xe);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8c, BIT(4), 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8c, 0x1e0, mixgain);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0x3e0, rxbb);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8e, BIT(5), 0x0);
  }
}

static void _dpk_set_afe_8814b(struct dm_struct *dm, boolean is_do_dpk)
{
 if (is_do_dpk) {
  odm_set_bb_reg(dm, 0x1830, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x1860, 0xfffff000, 0xf0001);
  odm_set_bb_reg(dm, 0x4130, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x4160, 0xfffff000, 0xf0001);
  odm_set_bb_reg(dm, 0x5230, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x5260, 0xfffff000, 0xf0001);
  odm_set_bb_reg(dm, 0x5330, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x5360, 0xfffff000, 0xf0001);
  odm_write_4byte(dm, 0x1c38, 0x0);
  ODM_delay_us(10);
  odm_write_4byte(dm, 0x1c38, 0xffffffff);
 } else {
  odm_set_bb_reg(dm, 0x1830, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x4130, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x5230, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x5330, BIT(30), 0x1);
 }
}

static void _dpk_thermal_read_8814b(
 void *dm_void,
 u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 odm_set_rf_reg(dm, (enum rf_path)path, 0x42, BIT(17), 0x1);
 odm_set_rf_reg(dm, (enum rf_path)path, 0x42, BIT(17), 0x0);
 odm_set_rf_reg(dm, (enum rf_path)path, 0x42, BIT(17), 0x1);
 ODM_delay_us(50);
 dpk_info->thermal_dpk[path] = (u8)odm_get_rf_reg(dm, (enum rf_path)path,
        0x42, 0x0FC00) - 0x2;
                                    ;
}

static void do_dpk_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 boolean KFail;
 if (dm->ext_pa || dm->ext_pa_5g) {
                                                                  ;
  return;
 } else if (!dpk_info->is_dpk_pwr_on) {
                                                                  ;
  return;
 } else {
                                                                        ;
                                                                             ;
 }
 if (dpk_get_dpkbychannel_8814b(dm) == false) {
  dpk_by_group_8814b(dm);
  dpk_result_summary_8814b(dm);
 } else {
  KFail = _do_dpk_8814b(dm);
 }
}

static void dpk_by_group_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 boolean KFail;
 *dm->band_type = ODM_BAND_5G;
 *dm->band_width = CHANNEL_WIDTH_20;
 config_phydm_switch_band_8814b(dm, 48);
 config_phydm_switch_channel_8814b(dm, 48);
 *dm->channel = 48;
 KFail = _do_dpk_8814b(dm);
 config_phydm_switch_channel_8814b(dm, 120);
 *dm->channel = 120;
 KFail = _do_dpk_8814b(dm);
 config_phydm_switch_channel_8814b(dm, 157);
 *dm->channel = 157;
 KFail = _do_dpk_8814b(dm);
 *dm->band_type = ODM_BAND_5G;
 *dm->band_width = CHANNEL_WIDTH_80;
 config_phydm_switch_bw_8814b(dm, 0x1, CHANNEL_WIDTH_80);
 config_phydm_switch_channel_8814b(dm, 58);
 *dm->channel = 58;
 KFail = _do_dpk_8814b(dm);
 config_phydm_switch_channel_8814b(dm, 122);
 *dm->channel = 122;
 KFail = _do_dpk_8814b(dm);
 config_phydm_switch_channel_8814b(dm, 155);
 *dm->channel = 155;
 KFail = _do_dpk_8814b(dm);
}

static u8 dpk_channel_transfer_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u8 channel, bandwidth, i = 0x3;
 channel = *dm->channel;
 bandwidth = *dm->band_width;
 if (*dm->band_type == ODM_BAND_2_4G) {
  if (channel <= 4)
   i = 0;
  else if (channel >= 5 && channel <= 8)
   i = 1;
  else if (channel >= 9)
   i = 2;
 } else {
  if (bandwidth <= 0x1) {
   if (channel <= 64)
    i = 3;
   else if (channel >= 45 && channel <= 149)
    i = 4;
   else if (channel >= 150)
    i = 5;
  } else if (bandwidth == 0x2) {
   if (channel <= 58)
    i = 6;
   else if (channel >= 59 && channel <= 138)
    i = 7;
   else if (channel >= 139)
    i = 8;
  }
 }
                              ;
 return i;
}

static boolean dpk_get_dpkbychannel_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
                                    ;
 return dpk_info->is_dpk_by_channel;
}

static void dpk_result_summary_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 path, group;
                                                                      ;
 for (path = 0; path < DPK_RF_PATH_NUM_8814B; path++) {
  for (group = 0; group < G_2G_N; group++) {
                                                   ;
  }
  for (group = G_2G_N; group < DPK_GROUP_NUM_8814B; group++) {
                                                        ;
  }
                            ;
 }
                                                       ;
                                                                      ;
}

static void dpk_thermal_read_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 u8 path = 0x0;
 for (path = 0; path < 4; path++)
  _dpk_thermal_read_8814b(dm, path);
}

static void dpk_track_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_dpk_info *dpk_info = &dm->dpk_info;
 struct _hal_rf_ *rf = &dm->rf_table;
 s8 pwsf_a, pwsf_b, pwsf_c, pwsf_d;
 u8 i = 0, k = 0;
 u8 thermal_dpk_avg_count = 0;
 u8 delta_dpk[4] = {0, 0, 0, 0};
 u8 offset[4] = {0, 0, 0, 0};
 u32 thermal_dpk_avg[4] = {0, 0, 0, 0};
 u8 thermal_value[4] = {0, 0, 0, 0};
 u8 path, pwsf[4] = {0, 0, 0, 0};
 u8 is_increase[4] = {0, 0, 0, 0};
 s8 idx_offset[4] = {0, 0, 0, 0};
 u8 reg1b58[4] = {0, 0, 0, 0};
 if (dm->ext_pa) {
                                                            ;
  return;
 } else if (!dpk_info->is_dpk_pwr_on) {
                                                               ;
  return;
 } else {
                                                          ;
 }
 if (dpk_info->is_dpk_by_channel == false)
  for (i = 0; i < DPK_RF_NUM_8814B; i++)
   dpk_info->thermal_dpk[i] = rf->eeprom_thermal;
 odm_set_rf_reg(dm, RF_PATH_A, 0x42, BIT(17), 0x1);
 odm_set_rf_reg(dm, RF_PATH_A, 0x42, BIT(17), 0x0);
 odm_set_rf_reg(dm, RF_PATH_A, 0x42, BIT(17), 0x1);
 ODM_delay_us(50);
 thermal_value[0] = (u8)odm_get_rf_reg(dm, RF_PATH_A, 0x42, 0xfc00);
 odm_set_rf_reg(dm, RF_PATH_B, 0x42, BIT(17), 0x1);
 odm_set_rf_reg(dm, RF_PATH_B, 0x42, BIT(17), 0x0);
 odm_set_rf_reg(dm, RF_PATH_B, 0x42, BIT(17), 0x1);
 ODM_delay_us(50);
 thermal_value[1] = (u8)odm_get_rf_reg(dm, RF_PATH_B, 0x42, 0xfc00);
 odm_set_rf_reg(dm, RF_PATH_C, 0x42, BIT(17), 0x1);
 odm_set_rf_reg(dm, RF_PATH_C, 0x42, BIT(17), 0x0);
 odm_set_rf_reg(dm, RF_PATH_C, 0x42, BIT(17), 0x1);
 ODM_delay_us(50);
 thermal_value[2] = (u8)odm_get_rf_reg(dm, RF_PATH_C, 0x42, 0xfc00);
 odm_set_rf_reg(dm, RF_PATH_D, 0x42, BIT(17), 0x1);
 odm_set_rf_reg(dm, RF_PATH_D, 0x42, BIT(17), 0x0);
 odm_set_rf_reg(dm, RF_PATH_D, 0x42, BIT(17), 0x1);
 ODM_delay_us(50);
 thermal_value[3] = (u8)odm_get_rf_reg(dm, RF_PATH_D, 0x42, 0xfc00);
 dpk_info->thermal_dpk_avg[0][dpk_info->thermal_dpk_avg_index] =
  thermal_value[0];
 dpk_info->thermal_dpk_avg[1][dpk_info->thermal_dpk_avg_index] =
  thermal_value[1];
 dpk_info->thermal_dpk_avg[2][dpk_info->thermal_dpk_avg_index] =
  thermal_value[2];
 dpk_info->thermal_dpk_avg[3][dpk_info->thermal_dpk_avg_index] =
  thermal_value[3];
 dpk_info->thermal_dpk_avg_index++;
 if (dpk_info->thermal_dpk_avg_index == THERMAL_DPK_AVG_NUM)
  dpk_info->thermal_dpk_avg_index = 0;
 for (i = 0; i < THERMAL_DPK_AVG_NUM; i++) {
  if (dpk_info->thermal_dpk_avg[0][i] ||
      dpk_info->thermal_dpk_avg[1][i] ||
      dpk_info->thermal_dpk_avg[2][i] ||
      dpk_info->thermal_dpk_avg[3][i]) {
   thermal_dpk_avg[0] += dpk_info->thermal_dpk_avg[0][i];
   thermal_dpk_avg[1] += dpk_info->thermal_dpk_avg[1][i];
   thermal_dpk_avg[2] += dpk_info->thermal_dpk_avg[2][i];
   thermal_dpk_avg[3] += dpk_info->thermal_dpk_avg[3][i];
   thermal_dpk_avg_count++;
  }
 }
 if (thermal_dpk_avg_count) {
  thermal_value[0] = (u8)(thermal_dpk_avg[0] / thermal_dpk_avg_count);
  thermal_value[1] = (u8)(thermal_dpk_avg[1] / thermal_dpk_avg_count);
  thermal_value[2] = (u8)(thermal_dpk_avg[2] / thermal_dpk_avg_count);
  thermal_value[3] = (u8)(thermal_dpk_avg[3] / thermal_dpk_avg_count);
                                                   ;
                                                   ;
                                                   ;
                                                   ;
                                                    ;
                                                    ;
                                                    ;
                                                    ;
 }
 delta_dpk[0] = HALRF_ABS(thermal_value[0], dpk_info->thermal_dpk[0]);
 delta_dpk[1] = HALRF_ABS(thermal_value[1], dpk_info->thermal_dpk[1]);
 delta_dpk[2] = HALRF_ABS(thermal_value[2], dpk_info->thermal_dpk[2]);
 delta_dpk[3] = HALRF_ABS(thermal_value[3], dpk_info->thermal_dpk[3]);
 if (dpk_info->is_dpk_by_channel == true) {
  is_increase[0] = ((thermal_value[0] < dpk_info->thermal_dpk[0]) ? 0 : 1);
  is_increase[1] = ((thermal_value[1] < dpk_info->thermal_dpk[1]) ? 0 : 1);
  is_increase[2] = ((thermal_value[2] < dpk_info->thermal_dpk[2]) ? 0 : 1);
  is_increase[3] = ((thermal_value[3] < dpk_info->thermal_dpk[3]) ? 0 : 1);
 } else {
  is_increase[0] = ((thermal_value[0] < rf->eeprom_thermal) ? 0 : 1);
  is_increase[1] = ((thermal_value[1] < rf->eeprom_thermal) ? 0 : 1);
  is_increase[2] = ((thermal_value[2] < rf->eeprom_thermal) ? 0 : 1);
  is_increase[3] = ((thermal_value[3] < rf->eeprom_thermal) ? 0 : 1);
 }
 offset[0] = (u8) (delta_dpk[0] / DPK_THRESHOLD_8814B);
 offset[1] = (u8) (delta_dpk[1] / DPK_THRESHOLD_8814B);
 offset[2] = (u8) (delta_dpk[2] / DPK_THRESHOLD_8814B);
 offset[3] = (u8) (delta_dpk[3] / DPK_THRESHOLD_8814B);
 if ( offset[0] == 0x0 && offset[1] == 0x0 && offset[2] == 0x0 && offset[3] == 0x0)
  return;
 k = dpk_channel_transfer_8814b(dm);
 if (is_increase[0]){
  reg1b58[0] = offset[0];
 } else {
  reg1b58[0] = (0x3f - offset[0] + 1) & 0x3f;
 }
 if (is_increase[1]){
  reg1b58[1] = offset[1];
 } else {
  reg1b58[1] = (0x3f - offset[1] + 1) & 0x3f;
 }
 if (is_increase[2]){
  reg1b58[2] = offset[2];
 } else {
  reg1b58[2] = (0x3f - offset[2] + 1) & 0x3f;
 }
 if (is_increase[3]){
  reg1b58[3] = offset[3];
 } else {
  reg1b58[3] = (0x3f - offset[3] + 1) & 0x3f;
 }
                                                       ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | RF_PATH_A << 1);
 odm_set_bb_reg(dm, R_0x1b58, 0x3f, reg1b58[0]);
                                                       ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | RF_PATH_B << 1);
 odm_set_bb_reg(dm, R_0x1b58, 0x3f, reg1b58[1]);
                                                       ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | RF_PATH_C << 1);
 odm_set_bb_reg(dm, R_0x1b58, 0x3f, reg1b58[2]);
                                                       ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | RF_PATH_D << 1);
 odm_set_bb_reg(dm, R_0x1b58, 0x3f, reg1b58[3]);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | RF_PATH_A << 1);
}
