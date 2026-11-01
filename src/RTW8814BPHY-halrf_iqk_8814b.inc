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

static void _iq_calibrate_8814b_init(struct dm_struct *dm)
{
 struct dm_iqk_info *iqk = &dm->IQK_info;
 u8 i, j, k, m;
 
 if (dm->port->iqk_first) {
  dm->port->iqk_first = false;
                                                   ;
  for (i = 0; i < SS_8814B; i++) {
   for (j = 0; j < 2; j++) {
    iqk->lok_fail[i] = true;
    iqk->iqk_fail[j][i] = true;
    iqk->iqc_matrix[j][i] = DIQC;
   }
  }
  for (i = 0; i < 2; i++) {
   iqk->iqk_channel[i] = 0x0;
   for (j = 0; j < SS_8814B; j++) {
    iqk->lok_idac[i][j] = 0x0;
    iqk->rxiqk_agc[i][j] = 0x0;
    iqk->bypass_iqk[i][j] = 0x0;
    for (k = 0; k < 2; k++) {
     iqk->iqk_fail_report[i][j][k] = true;
     for (m = 0; m < 8; m++) {
      iqk->iqk_cfir_real[i][j][k][m]
       = 0x0;
      iqk->iqk_cfir_imag[i][j][k][m]
       = 0x0;
     }
    }
    for (k = 0; k < 3; k++)
     iqk->retry_count[i][j][k] = 0x0;
   }
  }
 }
}

static void _iqk_backup_iqk_8814b(struct dm_struct *dm, u8 step, u8 path)
{
 struct dm_iqk_info *iqk = &dm->IQK_info;
 u8 i, j;
 switch (step) {
 case 0:
  _iqk_backup_iqk_8814b_subfunction(dm);
  for (i = 0; i < 4; i++) {
   iqk->rxiqk_fail_code[0][i] = 0x0;
   iqk->rxiqk_agc[0][i] = 0x0;
   for (j = 0; j < 2; j++) {
    iqk->iqk_fail_report[0][i][j] = true;
    iqk->gs_retry_count[0][i][j] = 0x0;
   }
   for (j = 0; j < 3; j++)
    iqk->retry_count[0][i][j] = 0x0;
  }
  iqk->iqk_channel[0] = iqk->rf_reg18;
  break;
 case 1:
  iqk->lok_idac[0][path] = odm_get_rf_reg(dm, (enum rf_path)path,
       RF_0x58, MASK20BITS);
  break;
 case 2:
 case 3:
  phydm_get_iqk_cfir(dm, (step - 2), path, false);
  break;
 }
}

static void _iqk_backup_iqk_8814b_subfunction(struct dm_struct *dm)
{
 struct dm_iqk_info *iqk = &dm->IQK_info;
 u8 i, j, k;
 iqk->iqk_channel[1] = iqk->iqk_channel[0];
 for (i = 0; i < 2; i++) {
  iqk->lok_idac[1][i] = iqk->lok_idac[0][i];
  iqk->rxiqk_agc[1][i] = iqk->rxiqk_agc[0][i];
  iqk->bypass_iqk[1][i] = iqk->bypass_iqk[0][i];
  iqk->rxiqk_fail_code[1][i] = iqk->rxiqk_fail_code[0][i];
  for (j = 0; j < 2; j++) {
   iqk->iqk_fail_report[1][i][j] =
      iqk->iqk_fail_report[0][i][j];
   for (k = 0; k < 8; k++) {
    iqk->iqk_cfir_real[1][i][j][k] =
      iqk->iqk_cfir_real[0][i][j][k];
    iqk->iqk_cfir_imag[1][i][j][k] =
      iqk->iqk_cfir_imag[0][i][j][k];
   }
  }
 }
}

static void _iqk_bp_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
    u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg)
{
 u32 i;
 for (i = 0; i < MAC_NUM_8814B; i++)
  mac_bp[i] = odm_read_4byte(dm, bp_mac_reg[i]);
 for (i = 0; i < BB_NUM_8814B; i++)
  bb_bp[i] = odm_read_4byte(dm, bp_bb_reg[i]);
}

static void _iqk_bp_rf_8814b(struct dm_struct *dm, u32 rf_bp[][SS_8814B],
        u32 *bp_reg)
{
 u8 i, s;
 for (s = 0; s < SS_8814B; s++) {
  for (i = 0; i < RF_NUM_8814B; i++) {
   rf_bp[i][s] = odm_get_rf_reg(dm, (enum rf_path)s,
           bp_reg[i], MASK20BITS);
  }
 }
}

static void _iqk_cal_path_off_8814b(struct dm_struct *dm)
{
 u8 path;
 for (path = 0; path < 4; path++) {
  odm_set_rf_reg(dm, (enum rf_path)path, 0x0, 0xfffff, 0x10000);
  odm_set_bb_reg(dm, R_0x1b00, MASKDWORD,
          IQK_CMD_8814B | path << 1);
  odm_write_1byte(dm, 0x1bcc, 0x3f);
 }
}

static boolean
_iqk_check_cal_8814b(struct dm_struct *dm, u8 path, u8 cmd)
{
 boolean notready = true, fail = true;
 u32 delay_count = 0x0;
 while (notready) {
  if (odm_read_1byte(dm, 0x2d9c) == 0x55) {
   if (cmd == 0x0)
    fail = false;
   else
    fail = (boolean)
           odm_get_bb_reg(dm, R_0x1b08, BIT(26));
   notready = false;
  } else {
   ODM_delay_us(10);
   delay_count++;
  }
  if (delay_count >= 20000) {
   fail = true;
                                                  ;
   break;
  }
 }
 odm_write_1byte(dm, 0x1b10, 0x0);
                                                                    ;
 return fail;
}

static void _iqk_clean_cfir_8814b(struct dm_struct *dm, u8 mode, u8 path)
{
 odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, 0x8 | path << 1);
 if (mode == TXIQK) {
  odm_set_bb_reg(dm, R_0x1b0c, BIT(13) | BIT(12), 0x3);
  odm_write_4byte(dm, 0x1bd8, 0xc0000003);
  odm_write_4byte(dm, 0x1bd8, 0xe0000001);
 } else {
  odm_set_bb_reg(dm, R_0x1b0c, BIT(13) | BIT(12), 0x1);
  odm_write_4byte(dm, 0x1bd8, 0x60000003);
  odm_write_4byte(dm, 0x1bd8, 0xe0000001);
 }
 odm_set_bb_reg(dm, R_0x1b0c, BIT(13) | BIT(12), 0x0);
 odm_write_4byte(dm, 0x1bd8, 0x0);
}

static void _iqk_fail_count_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 u8 i;
 dm->n_iqk_cnt++;
 if (odm_get_bb_reg(dm, RF_0x1bf0, BIT(16)) == 1)
  iqk->is_reload = true;
 else
  iqk->is_reload = false;
 if (!iqk->is_reload) {
  for (i = 0; i < 8; i++) {
   if (odm_get_bb_reg(dm, R_0x1bf0, BIT(i)) == 1)
    dm->n_iqk_fail_cnt++;
  }
 }
                           ;
}

static void _iqk_fill_iqk_report_8814b(void *dm_void, u8 ch)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 u32 tmp1 = 0x0, tmp2 = 0x0, tmp3 = 0x0, data;
 u8 i;
 for (i = 0; i < SS_8814B; i++) {
  tmp1 += ((iqk->iqk_fail_report[ch][i][TX_IQK] & 1) << i);
  tmp2 += ((iqk->iqk_fail_report[ch][i][RX_IQK] & 1) << (i + 4));
  tmp3 += ((iqk->rxiqk_fail_code[ch][i] & 0x3) << (i * 2 + 8));
 }
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | 0x3 << 1);
 odm_set_bb_reg(dm, R_0x1bf0, 0x0000ffff, tmp1 | tmp2 | tmp3);
 data = iqk->rxiqk_agc[ch][RF_PATH_B] << 16 |
   iqk->rxiqk_agc[ch][RF_PATH_A];
 odm_write_4byte(dm, 0x1bc4, data);
 data = iqk->rxiqk_agc[ch][RF_PATH_D] << 16 |
   iqk->rxiqk_agc[ch][RF_PATH_C];
 odm_write_4byte(dm, 0x1be0, data);
}

static void _iqk_iqk_by_path_8814b(void *dm_void, boolean segment_iqk)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 boolean KFAIL = true;
 u8 i, kcount_limit;
 switch (iqk->iqk_step) {
 case 0:
  _iqk_lok_tune_8814b(dm, RF_PATH_A);
  iqk->iqk_step++;
  break;
 case 1:
  _iqk_txk_setting_8814b(dm, RF_PATH_A);
  KFAIL = _iqk_nbtxk_oneshot_8814b(dm, RF_PATH_A);
                                                               ;
  if(!KFAIL) {
   _iqk_txk_setting_8814b(dm, RF_PATH_A);
   KFAIL = _iqk_one_shot_8814b(dm, RF_PATH_A, TXIQK);
  }
  iqk->kcount++;
                                                               ;
  if (KFAIL && iqk->retry_count[0][RF_PATH_A][TXIQK] < 3)
   iqk->retry_count[0][RF_PATH_A][TXIQK]++;
  else
   iqk->iqk_step++;
  break;
 case 2:
  if (iqk->iqk_fail_report[0][RF_PATH_A][TXIQK] != true) {
  _iqk_rxiqk(dm, RF_PATH_A);
  } else {
   iqk->iqk_fail_report[0][RF_PATH_A][RXIQK] = true;
                                                 ;
   iqk->iqk_step++;
  }
  break;
 case 3:
  _iqk_lok_tune_8814b(dm, RF_PATH_B);
  iqk->iqk_step++;
  break;
 case 4:
  _iqk_txk_setting_8814b(dm, RF_PATH_B);
  KFAIL = _iqk_nbtxk_oneshot_8814b(dm, RF_PATH_B);
                                                               ;
  if(!KFAIL) {
   _iqk_txk_setting_8814b(dm, RF_PATH_B);
   KFAIL = _iqk_one_shot_8814b(dm, RF_PATH_B, TXIQK);
  }
  iqk->kcount++;
                                                               ;
  if (KFAIL && iqk->retry_count[0][RF_PATH_B][TXIQK] < 3)
   iqk->retry_count[0][RF_PATH_B][TXIQK]++;
  else
   iqk->iqk_step++;
  break;
 case 5:
  if (iqk->iqk_fail_report[0][RF_PATH_B][TXIQK] != true) {
  _iqk_rxiqk(dm, RF_PATH_B);
  } else {
   iqk->iqk_fail_report[0][RF_PATH_B][RXIQK] = true;
                                                 ;
   iqk->iqk_step++;
  }
  break;
 case 6:
  _iqk_lok_tune_8814b(dm, RF_PATH_C);
  iqk->iqk_step++;
  break;
 case 7:
  _iqk_txk_setting_8814b(dm, RF_PATH_C);
  KFAIL = _iqk_nbtxk_oneshot_8814b(dm, RF_PATH_C);
                                                               ;
  if(!KFAIL) {
   _iqk_txk_setting_8814b(dm, RF_PATH_C);
   KFAIL = _iqk_one_shot_8814b(dm, RF_PATH_C, TXIQK);
  }
  iqk->kcount++;
                                                               ;
  if (KFAIL && iqk->retry_count[0][RF_PATH_C][TXIQK] < 3)
   iqk->retry_count[0][RF_PATH_C][TXIQK]++;
  else
   iqk->iqk_step++;
  break;
 case 8:
  if (iqk->iqk_fail_report[0][RF_PATH_C][TXIQK] != true) {
  _iqk_rxiqk(dm, RF_PATH_C);
  } else {
   iqk->iqk_fail_report[0][RF_PATH_C][RXIQK] = true;
                                                 ;
   iqk->iqk_step++;
  }
  break;
 case 9:
  _iqk_lok_tune_8814b(dm, RF_PATH_D);
  iqk->iqk_step++;
  break;
 case 10:
  _iqk_txk_setting_8814b(dm, RF_PATH_D);
  KFAIL = _iqk_nbtxk_oneshot_8814b(dm, RF_PATH_D);
                                                               ;
  if(!KFAIL) {
   _iqk_txk_setting_8814b(dm, RF_PATH_D);
   KFAIL = _iqk_one_shot_8814b(dm, RF_PATH_D, TXIQK);
  }
  iqk->kcount++;
                                                               ;
  if (KFAIL && iqk->retry_count[0][RF_PATH_D][TXIQK] < 3)
   iqk->retry_count[0][RF_PATH_D][TXIQK]++;
  else
   iqk->iqk_step++;
  break;
 case 11:
  if (iqk->iqk_fail_report[0][RF_PATH_D][TXIQK] != true) {
  _iqk_rxiqk(dm, RF_PATH_D);
  } else {
   iqk->iqk_fail_report[0][RF_PATH_D][RXIQK] = true;
                                                 ;
   iqk->iqk_step++;
  }
  break;
 }
 if (iqk->iqk_step == IQK_STEP_8814B) {
  _iqk_summy_report_8814b(dm);
  odm_set_bb_reg(dm, R_0x1b08, BIT(7), 0x0);
  for (i = 0; i < SS_8814B; i++) {
   odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | i << 1);
   odm_write_1byte(dm, 0x1bcc, 0x0);
  }
 }
}

static void _iqk_lok_setting_8814b(struct dm_struct *dm, u8 path)
{
 _iqk_cal_path_off_8814b(dm);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_write_1byte(dm, 0x1b2c, 0x08);
 switch (*dm->band_type) {
 case ODM_BAND_2_4G:
  odm_write_1byte(dm, 0x1b2b, 0x00);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0x57, BIT(19), 0);
  odm_write_4byte(dm, 0x1b20, 0x030401e8);
  odm_write_1byte(dm, 0x1bcc, 0x9);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0xde, MASK20BITS, 0x90004);
  odm_set_rf_reg(dm, (enum rf_path)path,
          RF_0x56, MASK20BITS, 0xaeee2);
  odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x1);
  break;
 case ODM_BAND_5G:
  odm_write_1byte(dm, 0x1b2b, 0x80);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0x57, BIT(19), 0);
  odm_write_4byte(dm, 0x1b20, 0x030401e8);
  odm_write_1byte(dm, 0x1bcc, 0x9);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0xde, MASK20BITS, 0x90004);
  odm_set_rf_reg(dm, (enum rf_path)path,
          RF_0x56, MASK20BITS, 0xa8ee8);
  odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x1);
  break;
 }
}

static void _iqk_lok_tune_8814b(void *dm_void, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u8 idac_bs = 0x0;
 while (1) {
  odm_set_rf_reg(dm, (enum rf_path)path, 0x57, 0xe00, idac_bs);
  _iqk_lok_setting_8814b(dm, path);
  _lok_one_shot_8814b(dm, path, false);
                                                    ;
  if(!_lok_check(dm, path)) {
   if(idac_bs == 0)
    break;
   else
    idac_bs++;
  } else {
   break;
  }
 }
}

static boolean
_iqk_nbtxk_oneshot_8814b(void *dm_void, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 boolean fail = true;
 u32 IQK_CMD = 0x0;
                                                             ;
 IQK_CMD = IQK_CMD_8814B | 0x1 << 8 | (1 << (path + 4));
                                                             ;
 odm_write_1byte(dm, 0x1b10, 0x0);
 odm_write_4byte(dm, 0x1b00, IQK_CMD);
 odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
 fail = _iqk_check_cal_8814b(dm, path, 0x1);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
                                   ;
                                   ;
 return fail;
}

static boolean
_iqk_one_shot_8814b(void *dm_void, u8 path, u8 idx)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 boolean fail = true;
 u32 IQK_CMD = 0x0, tmp;
 u8 bw;
 if (*dm->band_width >= 2)
  bw = 2;
 else
  bw = *dm->band_width;
 if (idx == TXIQK)
                                                              ;
 else if (idx == RXIQK1)
              ;
 else
              ;
 if (idx == TXIQK) {
  IQK_CMD = IQK_CMD_8814B | 0x1 << 8 | (1 << (path + 4));
                                                              ;
 } else if (idx == RXIQK1) {
  IQK_CMD = IQK_CMD_8814B | 0x1 << 8 | (1 << (path + 4));
                                                               ;
 } else if (idx == RXIQK2) {
  IQK_CMD = IQK_CMD_8814B | 0x2 << 8 | (1 << (path + 4));
                                                               ;
 }
 odm_write_1byte(dm, 0x1b10, 0x0);
 odm_write_4byte(dm, 0x1b00, IQK_CMD);
 odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
 ODM_delay_us(10);
 fail = _iqk_check_cal_8814b(dm, path, 0x1);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
                                                                ;
  if (idx != TXIQK)
                                   ;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 if (idx == RXIQK2) {
  iqk->rxiqk_agc[0][path] = iqk->rxiqk_agc[0][path] |
   (u16)((odm_get_rf_reg(dm, (enum rf_path)path,
         RF_0x0, MASK20BITS) >> 5) & 0xff);
 }
 if (fail) {
  if (idx == TXIQK)
   _iqk_clean_cfir_8814b(dm, TXIQK, path);
  else if (idx == RXIQK2)
   _iqk_clean_cfir_8814b(dm, RXIQK, path);
 }
 if (idx == TXIQK)
  iqk->iqk_fail_report[0][path][TXIQK] = fail;
 else
  iqk->iqk_fail_report[0][path][RXIQK] = fail;
 return fail;
}

static void _iqk_reload_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
        u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg)
{
 ++dm->port->restore_depth;

 u32 i;
 odm_write_4byte(dm, 0x1d70, 0x50505050);
 for (i = 0; i < MAC_NUM_8814B; i++)
  odm_write_4byte(dm, bp_mac_reg[i], mac_bp[i]);
 for (i = 0; i < BB_NUM_8814B; i++)
  odm_write_4byte(dm, bp_bb_reg[i], bb_bp[i]);
 odm_set_bb_reg(dm, 0x180c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x410c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x520c, BIT(31), 0x1);
 odm_set_bb_reg(dm, 0x530c, BIT(31), 0x1);
--dm->port->restore_depth;
}

static void _iqk_reload_rf_8814b(struct dm_struct *dm, u32 *reg,
      u32 data[][SS_8814B])
{
 ++dm->port->restore_depth;

 u32 i;
 odm_set_rf_reg(dm, RF_PATH_A, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_B, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_C, RF_0xef, MASK20BITS, 0x0);
 odm_set_rf_reg(dm, RF_PATH_D, RF_0xef, MASK20BITS, 0x0);
 for (i = 0; i < RF_NUM_8814B; i++) {
  odm_set_rf_reg(dm, RF_PATH_A, reg[i], 0xfffff, data[i][0]);
  odm_set_rf_reg(dm, RF_PATH_B, reg[i], 0xfffff, data[i][1]);
  odm_set_rf_reg(dm, RF_PATH_C, reg[i], 0xfffff, data[i][2]);
  odm_set_rf_reg(dm, RF_PATH_D, reg[i], 0xfffff, data[i][3]);
 }
--dm->port->restore_depth;
}

static boolean
_iqk_rx_iqk_by_path_8814b(void *dm_void, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 boolean KFAIL = true, gonext = false, gs_limit = false;
 switch (iqk->rxiqk_step) {
 case 0:
  _iqk_rxk2_setting_8814b(dm, path, true);
  iqk->isbnd = false;
  while (1) {
   KFAIL = _iqk_rxk_gsearch_fail_8814b(dm, path, RXIQK2);
   if (iqk->gs_retry_count[0][path][1] < rxiqk_gs_limit)
    gs_limit = true;
   else
    gs_limit = false;
   if (KFAIL && gs_limit) {
    iqk->gs_retry_count[0][path][1]++;
   } else {
    iqk->rxiqk_step++;
    break;
   }
  }
  break;
 case 1:
  _iqk_rxk2_setting_8814b(dm, path, false);
  gonext = false;
  while (1) {
   KFAIL = _iqk_one_shot_8814b(dm, path, RXIQK2);
   if (KFAIL && iqk->retry_count[0][path][RXIQK2] < 2) {
    iqk->retry_count[0][path][RXIQK2]++;
   } else if (KFAIL) {
    iqk->rxiqk_fail_code[0][path] = 2;
    iqk->rxiqk_step = 2;
    gonext = true;
   } else {
    iqk->rxiqk_step++;
    gonext = true;
   }
   if (gonext)
    break;
  }
  break;
 }
 return KFAIL;
}

static void _iqk_rxiqk(void *dm_void, u8 rf_path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 boolean KFAIL = true;
 u8 c = 0;
 while (c < 100) {
  c++;
  KFAIL = _iqk_rx_iqk_by_path_8814b(dm, rf_path);
                                                      ;
  if (iqk->rxiqk_step == RXK_STEP_8814B) {
   iqk->iqk_step++;
   iqk->rxiqk_step = 0;
   if (KFAIL)
                                            ;
   break;
  }
 }
 iqk->kcount++;
}

static void _iqk_rxk2_setting_8814b(struct dm_struct *dm, u8 path, boolean is_gs)
{
 struct dm_iqk_info *iqk = &dm->IQK_info;
 _iqk_cal_path_off_8814b(dm);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x15);
 switch (*dm->band_type) {
 case ODM_BAND_2_4G:
  if (is_gs) {
   iqk->tmp1bcc = 0x9;
   iqk->txgain56 = 0xaa6e8;
  }
  odm_write_1byte(dm, 0x1bcc, iqk->tmp1bcc);
  odm_write_4byte(dm, 0x1b24, 0x00070848);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0xde, MASK20BITS, 0x90004);
  odm_set_rf_reg(dm, (enum rf_path)path,
          RF_0x56, MASK20BITS, iqk->txgain56);
  break;
 case ODM_BAND_5G:
  if (is_gs) {
   iqk->tmp1bcc = 0x09;
   iqk->lna_idx = 0x1;
  }
  odm_write_1byte(dm, 0x1bcc, iqk->tmp1bcc);
  odm_write_4byte(dm, 0x1b24, 0x00070048 | iqk->lna_idx << 10);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0xde, MASK20BITS, 0x90004);
  odm_set_rf_reg(dm, (enum rf_path)path,
          RF_0x56, MASK20BITS, 0xa8248);
  break;
 }
}

static boolean
_iqk_rxk_gsearch_fail_8814b(struct dm_struct *dm, u8 path, u8 step)
{
 struct dm_iqk_info *iqk = &dm->IQK_info;
 boolean fail = true;
 u32 IQK_CMD = 0x0, rf_reg0, tmp, bb_idx;
 u8 IQMUX[4] = {0x9, 0x12, 0x1b, 0x24};
 u32 txgain[3] = {0xaaee8, 0xaa6e8, 0xa8ee8};
 u8 idx1, idx2 = 0;
             ;
 if (step == RXIQK1) {
              ;
  IQK_CMD = 0x208 | (1 << (path + 4));
                       ;
  odm_write_1byte(dm, 0x1b10, 0x0);
  odm_write_4byte(dm, 0x1b00, IQK_CMD);
  odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
  ODM_delay_us(10);
  fail = _iqk_check_cal_8814b(dm, path, 0x1);
 } else if (step == RXIQK2) {
  for (idx1 = 0; idx1 < 4; idx1++) {
   if (iqk->tmp1bcc == IQMUX[idx1])
    break;
  }
  if (idx1 == 4) {
                                                   ;
   return fail;
  }
  if (*dm->band_type == ODM_BAND_2_4G) {
   for (idx2 = 0; idx2 < 3; idx2++) {
    if (iqk->txgain56 == txgain[idx2])
     break;
   }
   if (idx2 == 3) {
                                    ;
    return fail;
   }
  }
  odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
  odm_write_4byte(dm, 0x1bcc, iqk->tmp1bcc);
  IQK_CMD = 0x208 | (1 << (path + 4));
                       ;
  odm_write_4byte(dm, 0x1b00, IQK_CMD);
  odm_write_4byte(dm, 0x1b00, IQK_CMD + 0x1);
  ODM_delay_us(10);
  fail = _iqk_check_cal_8814b(dm, path, 0x1);
  rf_reg0 = odm_get_rf_reg(dm, (enum rf_path)path,
      RF_0x0, MASK20BITS);
  odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
                                                  ;
                                    ;
  tmp = (rf_reg0 & 0x1fe0) >> 5;
  iqk->lna_idx = tmp >> 5;
  bb_idx = tmp & 0x1f;
  if (*dm->band_type == ODM_BAND_5G) {
  if (bb_idx == 0x1) {
     if (idx1 != 3)
     idx1++;
   else
    iqk->isbnd = true;
   fail = true;
   } else if (bb_idx >= 0xa) {
    if (iqk->lna_idx < 0x3)
    iqk->lna_idx++;
   else
    iqk->isbnd = true;
   fail = true;
  } else {
   fail = false;
  }
   idx2 = 0x0;
  } else {
   if (bb_idx == 0x1) {
     if (idx1 != 3)
      idx1++;
    else if (idx2 != 2)
     idx2++;
    else
     iqk->isbnd = true;
    fail = true;
   } else if (bb_idx >= 0xa) {
    if (idx1 != 0)
     idx1--;
    else if (idx2 != 0)
     idx2--;
    else
     iqk->isbnd = true;
    fail = true;
   } else {
    fail = false;
   }
  }
  if (iqk->isbnd)
   fail = false;
  iqk->tmp1bcc = IQMUX[idx1];
  iqk->txgain56 = txgain[idx2];
  iqk->rxiqk_agc[0][path]= (idx2 <<10) | (idx1 << 8);
  if (fail && (*dm->band_type == ODM_BAND_5G)) {
   odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
   tmp = (odm_read_4byte(dm, 0x1b24) & 0xffffe3ff) |
    (iqk->lna_idx << 10);
   odm_write_4byte(dm, 0x1b24, tmp);
  } else {
   odm_set_rf_reg(dm, (enum rf_path)path, RF_0x56,
           MASK20BITS, iqk->txgain56);
  }
 }
 return fail;
}

static void _iqk_set_afe_8814b(struct dm_struct *dm)
{
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
}

static void _iqk_set_macbb_8814b(struct dm_struct *dm)
{
 odm_write_1byte(dm, REG_TXPAUSE, 0xff);
 odm_set_bb_reg(dm, 0x1e24, BIT(17), 0x1);
 odm_set_bb_reg(dm, 0x1cd0, 0xf0000000, 0x7);
 odm_set_bb_reg(dm, 0x1d58, 0xff8, 0x1ff);
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
}

static void _iqk_set_rfe_8814b(struct dm_struct *dm)
{
 odm_write_4byte(dm, 0x183c, 0x0);
 odm_write_4byte(dm, 0x413c, 0x0);
 odm_write_4byte(dm, 0x523c, 0x0);
 odm_write_4byte(dm, 0x533c, 0x0);
 odm_write_4byte(dm, 0x1840, 0x77777777);
 odm_write_4byte(dm, 0x1844, 0x77777777);
 odm_write_4byte(dm, 0x4140, 0x77777777);
 odm_write_4byte(dm, 0x4144, 0x77777777);
 odm_write_4byte(dm, 0x5240, 0x77777777);
 odm_write_4byte(dm, 0x5244, 0x77777777);
 odm_write_4byte(dm, 0x5340, 0x77777777);
 odm_write_4byte(dm, 0x5344, 0x77777777);
}

static void _iqk_start_iqk_8814b(struct dm_struct *dm, boolean segment_iqk)
{
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 u8 c = 0;
 u8 kcount_limit;
 if (*dm->band_width >= 2)
  kcount_limit = kcount_limit_80m;
 else
  kcount_limit = kcount_limit_others;
 while (c < 100) {
  c++;
  _iqk_iqk_by_path_8814b(dm, segment_iqk);
  if (iqk_info->iqk_step == IQK_STEP_8814B)
   break;
  if (segment_iqk && iqk_info->kcount == kcount_limit)
   break;
 }
}

static void _iqk_summy_report_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
                                                    ;
                                                           ;
                                                           ;
                                                    ;
                                                  ;
                                                  ;
                                                  ;
                                                  ;
                                              ;
                                              ;
                                               ;
                                               ;
                                               ;
                                               ;
                                             ;
                                             ;
                                             ;
                                             ;
}

static void _iqk_txcfir_rate_en_8814b(struct dm_struct *dm)
{
 odm_write_4byte(dm, 0xa74, 0x0e0e0000);
 odm_write_4byte(dm, 0xa78, 0xf83e0e0e);
 odm_set_bb_reg(dm, 0xa7c, 0xfffff, 0xf83e0);
}

static void _iqk_txk_setting_8814b(struct dm_struct *dm, u8 path)
{
 _iqk_cal_path_off_8814b(dm);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_set_bb_reg(dm, 0x1b2c, 0xfff, 0x15);
 switch (*dm->band_type) {
 case ODM_BAND_2_4G:
  odm_write_1byte(dm, 0x1b2b, 0x00);
  odm_write_4byte(dm, 0x1b20, 0x030401e8);
  odm_write_1byte(dm, 0x1bcc, 0x9);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0xde, MASK20BITS, 0x90004);
  odm_set_rf_reg(dm, (enum rf_path)path,
          RF_0x56, MASK20BITS, 0xaeee8);
  odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x0);
  break;
 case ODM_BAND_5G:
  odm_write_1byte(dm, 0x1b2b, 0x80);
  odm_write_4byte(dm, 0x1b20, 0x030401e8);
  odm_write_1byte(dm, 0x1bcc, 0x09);
  odm_set_rf_reg(dm, (enum rf_path)path,
          0xde, MASK20BITS, 0x90004);
  odm_set_rf_reg(dm, (enum rf_path)path,
          RF_0x56, MASK20BITS, 0xa8ee8);
  odm_set_rf_reg(dm, (enum rf_path)path, RF_0xef, BIT(4), 0x0);
  break;
 }
}

static boolean
_lok_check(void *dm_void, u8 path)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u32 tmp0x58;
 u8 idac_i, idac_q;
 tmp0x58 = odm_get_rf_reg(dm, path, 0x58, RFREG_MASK);
 idac_i = (u8)((tmp0x58 & 0xfc000) >> 14);
 idac_q = (u8)((tmp0x58 & 0x3f00) >> 8);
                                                                      ;
 if (idac_i > 60 || idac_i < 3 || idac_q > 60 || idac_q < 3)
  return false;
 else
  return true;
}

static boolean
_lok_one_shot_8814b(void *dm_void, u8 path, boolean for_rxk)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 struct _hal_rf_ *rf = &dm->rf_table;
 boolean LOK_notready = false;
 u32 LOK_temp = 0;
 u32 IQK_CMD = 0x0;
 u32 temp;
 if (for_rxk) {
                                                    ;
  IQK_CMD = 0x1008 | (1 << (4 + path)) | (path << 1);
 } else {
                                            ;
  IQK_CMD = IQK_CMD_8814B | (1 << (4 + path)) | (path << 1);
 }
                                                             ;
 odm_write_1byte(dm, 0x1b10, 0x0);
 odm_write_4byte(dm, 0x1b00, IQK_CMD);
 odm_write_4byte(dm, 0x1b00, IQK_CMD + 1);
 ODM_delay_us(10);
 LOK_notready = _iqk_check_cal_8814b(dm, path, 0x0);
 if (rf->rf_dbg_comp & DBG_RF_IQK) {
  temp = odm_get_rf_reg(dm, (enum rf_path)path, RF_0x58, 0xfffff);
  if (!LOK_notready)
                                                     ;
  else
               ;
 }
 iqk->lok_fail[path] = LOK_notready;
 return LOK_notready;
}

static void _phy_iq_calibrate_8814b(struct dm_struct *dm, boolean reset,
        boolean segment_iqk)
{
 u32 mac_bp[MAC_NUM_8814B], bb_bp[BB_NUM_8814B];
 u32 rf_bp[RF_NUM_8814B][SS_8814B];
 u32 bp_mac_reg[MAC_NUM_8814B] = {0x520};
 u32 bp_bb_reg[BB_NUM_8814B] = {0x1c38, 0x1d58, 0x1d60, 0x180c, 0x410c,
           0x520c, 0x530c, 0x1d70, 0x1a00, 0x824,
           0x1a14, 0x1830, 0x4130, 0x5230, 0x5330,
           0x1860, 0x4160, 0x5260, 0x5360, 0x183c,
           0x1840, 0x1844, 0x413c, 0x4140, 0x4144,
           0x523c, 0x5240, 0x5244, 0x533c, 0x5340,
           0x5344};
 u32 bp_rf_reg[RF_NUM_8814B] = {0xde};
 boolean is_mp = false;
 struct dm_iqk_info *iqk = &dm->IQK_info;
 if (*dm->mp_mode)
  is_mp = true;
                                                                    ;
                                  ;
 iqk->iqk_times++;
 iqk->kcount = 0;
 iqk->iqk_step = 0;
 iqk->rxiqk_step = 0;
 _iqk_backup_iqk_8814b(dm, 0x0, 0x0);
 _iqk_bp_macbb_8814b(dm, mac_bp, bb_bp, bp_mac_reg, bp_bb_reg);
 _iqk_bp_rf_8814b(dm, rf_bp, bp_rf_reg);
 while (1) {
  _iqk_set_macbb_8814b(dm);
  _iqk_set_afe_8814b(dm);
  _iqk_set_rfe_8814b(dm);
  _iqk_start_iqk_8814b(dm, segment_iqk);
  _iqk_reload_macbb_8814b(dm, mac_bp, bb_bp, bp_mac_reg,
     bp_bb_reg);
  _iqk_reload_rf_8814b(dm, bp_rf_reg, rf_bp);
  if (iqk->iqk_step == IQK_STEP_8814B)
   break;
  iqk->kcount = 0;
                                                ;
  ODM_delay_ms(50);
 };
 _iqk_txcfir_rate_en_8814b(dm);
 _iqk_fill_iqk_report_8814b(dm, 0);
                                                                  ;
}

static void phy_get_iqk_cfir_8814b(void *dm_void, u8 idx, u8 path, boolean debug)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 u8 i, ch;
 u32 tmp;
 u32 bit_mask_20_16 = BIT(20) | BIT(19) | BIT(18) | BIT(17) | BIT(16);
 if (debug)
  ch = 2;
 else
  ch = 0;
 odm_write_4byte(dm, 0x1b00, 0x8 | path << 1);
 if (idx == TX_IQK) {
  odm_set_bb_reg(dm, R_0x1b0c, BIT(13) | BIT(12), 0x3);
 } else {
  odm_set_bb_reg(dm, R_0x1b0c, BIT(13) | BIT(12), 0x1);
 }
 odm_set_bb_reg(dm, R_0x1bd4, BIT(21), 0x1);
 odm_set_bb_reg(dm, R_0x1bd4, bit_mask_20_16, 0x10);
 for (i = 0; i <= 16; i++) {
  odm_set_bb_reg(dm, R_0x1bd8, MASKDWORD, 0xe0000001 | i << 2);
  tmp = odm_get_bb_reg(dm, R_0x1bfc, MASKDWORD);
  iqk_info->iqk_cfir_real[ch][path][idx][i] =
      (u16)((tmp & 0x0fff0000) >> 16);
  iqk_info->iqk_cfir_imag[ch][path][idx][i] = (u16)(tmp & 0x0fff);
 }
 odm_set_bb_reg(dm, R_0x1b0c, BIT(13) | BIT(12), 0x0);
}

static void phy_iq_calibrate_8814b(void *dm_void, boolean clear, boolean segment_iqk)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 if (*dm->mp_mode)
  halrf_iqk_hwtx_check(dm, true);
 
  _iq_calibrate_8814b_init(dm);
  _phy_iq_calibrate_8814b(dm, clear, segment_iqk);
 
 _iqk_fail_count_8814b(dm);
 if (*dm->mp_mode)
  halrf_iqk_hwtx_check(dm, false);
 halrf_iqk_dbg(dm);
 phydm_bb_reset_8814b(dm);
}
