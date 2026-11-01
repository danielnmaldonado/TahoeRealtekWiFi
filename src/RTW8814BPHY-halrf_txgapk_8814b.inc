/* SPDX-License-Identifier: GPL-2.0-only
 * Generated from tobyw121/Openwrt_RTL@85ef7eb7ccbd4336cd6e628a743ddc211333901c.
 * Vendor notices and primary algorithms preserved; checked host adapter only. */
// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2018  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

static void _phy_txgapk_calibrate_8814b(void *dm_void, boolean is_cck)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 struct _hal_rf_ *rf = &dm->rf_table;
 struct _halrf_txgapk_info *txgapk_info = &(rf->halrf_txgapk_info);
 u8 rf_path, rf0_idx, idx, rf0_idx_current, rf0_idx_next, i;
 u32 RFreg0;
 u8 delta_gain_retry = 3;
 int delta_gain_gap_pre, delta_gain_gap[11];
 u8 extr_0p5db[11];
 int gain_case[11];
 u32 rf56_current, rf56_next;
 u32 rf56_current_temp[11], rf56_next_temp[11];
 int psd_gap[11];
 long psd_value_current, psd_value_next;
 int psd_diff = 0;
 u32 rf3f[12],tmp2;
 u32 rf0x0_curr = 0x0, rf0x0_next = 0x0, tmp = 0x0;
 u8 end = 8;
 u8 rate = 24;
 u8 gs_times = 11;
 RFreg0 = odm_get_rf_reg(dm, RF_PATH_A, RF_0x0, 0xfffff);
 rate = 0x24;
                                              ;
 memset(rf3f, 0x0, sizeof(rf3f));
 memset(delta_gain_gap, 0x0, sizeof(delta_gain_gap));
 memset(rf56_current_temp, 0x0, sizeof(rf56_current_temp));
 memset(rf56_next_temp, 0x0, sizeof(rf56_next_temp));
 memset(extr_0p5db, 0x0, sizeof(extr_0p5db));
 memset(gain_case, 0x0, sizeof(gain_case));
 _txgapk_afe_setting_8814b(dm, true);
                                                                                                        ;
 if (!txgapk_info->txgapk_bp_done)
  _txgapk_bp_rf_gain_table_8814b(dm);
 for (rf_path = 0; rf_path < 0x4; rf_path++) {
 rf_path = (enum rf_path)rf_path;
 _txgapk_phy_8814b(dm, rf_path);
 _txgapk_rf_setting_8814b(dm, rf_path);
 if (txgapk_info->txgapk_bp_done)
  _txgapk_reload_rf_gain_table_8814b(dm, is_cck, rf_path);
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | rf_path << 1);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 odm_set_rf_reg(dm, rf_path, 0xde, BIT(16), 0x0);
 ODM_delay_us(10);
 if (is_cck) {
  odm_set_rf_reg(dm, rf_path, 0x1a, 0x00001, 0x1);
  odm_set_rf_reg(dm, rf_path, 0x1a, 0x10000, 0x1);
  gs_times = 0x8;
 }
 else {
  odm_set_rf_reg(dm, rf_path, 0x1a, 0x00001, 0x0);
  odm_set_rf_reg(dm, rf_path, 0x1a, 0x10000, 0x0);
 }
 for (idx = 1; idx <= gs_times; idx++) {
  rf0_idx_current = 3 * (idx - 1) + 1;
  odm_set_rf_reg(dm, rf_path, 0x0, 0x1f, rf0_idx_current);
  ODM_delay_us(100);
  rf3f[idx] = odm_get_rf_reg(dm, rf_path, 0x56, 0x7fff);
 }
 for (idx = 1; idx <= gs_times; idx++) {
  if (*dm->band_type == ODM_BAND_5G) {
   if ((((rf3f[idx] & 0xe0) >> 5) == 0x7) &&
       (((rf3f[idx] & 0xe00) >> 9) == 0x7)) {
    end = idx;
    break;
   }
  } else {
   if ((((rf3f[idx] & 0xe0) >> 5) == 0x7) &&
       (((rf3f[idx] & 0xe00) >> 9) == 0x7) &&
       (((rf3f[idx] & 0x6000) >> 13) == 0x3) && !is_cck) {
    end = idx;
    break;
   }
  }
 }
 if (end > 10)
  end = 10;
                                                      ;
 for (rf0_idx = end ; rf0_idx >= 1; rf0_idx--) {
  rf0_idx_current = 3 * (rf0_idx - 1) + 1;
  rf56_current_temp[rf0_idx] = rf3f[rf0_idx] & 0x7fff;
  rf0_idx_next = rf0_idx_current + 1;
  if (rf0_idx == end)
   rf56_next_temp[rf0_idx] = rf56_current_temp[rf0_idx];
  else
   rf56_next_temp[rf0_idx] = rf3f[rf0_idx + 1] - 2;
  if (((rf56_current_temp[rf0_idx] & 0xe0) !=
      (rf56_next_temp[rf0_idx] & 0xe0)) |
      ((rf56_current_temp[rf0_idx] & 0xe00) !=
      (rf56_next_temp[rf0_idx] & 0xe00))|
      (rf0_idx >= end)) {
   odm_set_rf_reg(dm, rf_path, 0x56, 0x7fff,
           rf56_current_temp[rf0_idx]);
   odm_set_rf_reg(dm, rf_path, 0xde, BIT(16), 0x1);
   odm_set_rf_reg(dm, rf_path, 0x56, 0x7fff,
           rf56_current_temp[rf0_idx]);
   psd_value_current = (long)_txgapk_one_shot_8814b(dm, rf_path);
   odm_set_rf_reg(dm, rf_path, 0x56, 0x7fff,
           rf56_next_temp[rf0_idx]);
   odm_set_rf_reg(dm, rf_path, 0xde, BIT(16), 0x1);
   odm_set_rf_reg(dm, rf_path, 0x56, 0x7fff,
           rf56_next_temp[rf0_idx]);
   psd_value_next = (long)_txgapk_one_shot_8814b(dm, rf_path);
   if ((psd_value_current == 0xabcdef) | (psd_value_next == 0xabcdef))
    psd_diff = 1222;
   else if ( psd_value_current > 0x1000)
    psd_diff = psd_value_next / (psd_value_current / 1000);
   else
    psd_diff = 1200;
   if (psd_diff > 1679)
    psd_gap[rf0_idx] = 3;
   else if (psd_diff > 1496)
    psd_gap[rf0_idx] = 2;
   else if (psd_diff > 1333)
    psd_gap[rf0_idx] = 1;
   else if (psd_diff > 1188)
    psd_gap[rf0_idx] = 0;
   else if (psd_diff > 1059)
    psd_gap[rf0_idx] = -1;
   else if (psd_diff > 944)
    psd_gap[rf0_idx] = -2;
   else if (psd_diff > 841)
    psd_gap[rf0_idx] = -3;
   else
    psd_gap[rf0_idx] = -4;
  } else {
   psd_gap[rf0_idx] = 0;
  }
  if(rf0_idx >= end)
   psd_gap[rf0_idx] = 0;
                                                      ;
 }
 gain_case[end] = psd_gap[end];
 if ((gain_case[end] % 2) != 0)
  extr_0p5db[end] = 0x1;
 else
  extr_0p5db[end] = 0x0;
 delta_gain_gap[end] = (gain_case[end] - extr_0p5db[end]) / 2;
                                               ;
 for (rf0_idx = end - 1; rf0_idx >= 0x1; rf0_idx--) {
  gain_case[rf0_idx] = gain_case[rf0_idx + 1] + psd_gap[rf0_idx];
  if ((gain_case[rf0_idx] % 2) != 0)
   extr_0p5db[rf0_idx] = 0x1;
  else
   extr_0p5db[rf0_idx] = 0x0;
  delta_gain_gap[rf0_idx] = (gain_case[rf0_idx] - extr_0p5db[rf0_idx]) / 2;
                                      ;
                                     ;
                                          ;
 }
 odm_set_rf_reg(dm, rf_path, 0xde, BIT(16), 0x0);
 for (rf0_idx = 1; rf0_idx < end; rf0_idx++) {
  rf3f[rf0_idx] = (rf56_current_temp[rf0_idx] +
     delta_gain_gap[rf0_idx]) & 0x07fff;
                                ;
 }
 odm_set_rf_reg(dm, rf_path, 0xee, BIT(16), 0x1);
 for (idx = 0x1; idx < end; idx++) {
  switch (*dm->band_type) {
  case ODM_BAND_2_4G:
   if (!is_cck)
    tmp = 0x20 | (idx - 1);
   else
    tmp = 0x60 | (idx - 1);
   break;
  case ODM_BAND_5G:
   if (*dm->channel > 144)
    tmp = 0x300 | (idx - 1);
   else if (*dm->channel > 64)
    tmp = 0x280 | (idx - 1);
   else
    tmp = 0x200 | (idx - 1);
   break;
  }
  odm_set_rf_reg(dm, rf_path, 0x33, 0xfffff, tmp);
  if (extr_0p5db[idx] == 0x1)
   tmp2 = BIT(12) | rf3f[idx];
  else
   tmp2 = rf3f[idx] & 0x07fff;
  odm_set_rf_reg(dm, rf_path, 0x3f, 0x07fff, tmp2);
                        ;
 }
 odm_set_rf_reg(dm, rf_path, 0xee, BIT(16), 0x0);
 odm_set_rf_reg(dm, rf_path, RF_0x0, 0xfffff, RFreg0);
 }
 _txgapk_phy_restore_8814b(dm);
 _txgapk_rf_restore_8814b(dm);
 _txgapk_afe_setting_8814b(dm, false);
}

static void _txgapk_afe_setting_8814b(struct dm_struct *dm, boolean do_txgapk)
{
 if(!do_txgapk)++dm->port->restore_depth;

 if (do_txgapk) {
  odm_set_bb_reg(dm, 0x180c, 0x3, 0x0);
  odm_set_bb_reg(dm, 0x410c, 0x3, 0x0);
  odm_set_bb_reg(dm, 0x520c, 0x3, 0x0);
  odm_set_bb_reg(dm, 0x530c, 0x3, 0x0);
  odm_set_bb_reg(dm, 0x1b08, BIT(7), 0x1);
  odm_set_bb_reg(dm, 0x1e24, BIT(17), 0x1);
  odm_set_bb_reg(dm, 0x1cd0, 0xf000000, 0x7);
  odm_set_bb_reg(dm, 0x1c68, 0x0f00000, 0x7);
  odm_set_bb_reg(dm, 0x1834, BIT(15), 0x1);
  odm_set_bb_reg(dm, 0x4134, BIT(15), 0x1);
  odm_set_bb_reg(dm, 0x5234, BIT(15), 0x1);
  odm_set_bb_reg(dm, 0x5334, BIT(15), 0x1);
  odm_set_bb_reg(dm, 0x1d60, BIT(31), 0x1);
  odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0xffffffff);
  odm_set_bb_reg(dm, 0x1830, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x1860, 0xfffff000, 0xf0001);
  odm_set_bb_reg(dm, 0x4130, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x4160, 0xfffff000, 0xf0001);
  odm_set_bb_reg(dm, 0x5230, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x5260, 0xfffff000, 0xf0001);
  odm_set_bb_reg(dm, 0x5330, BIT(30), 0x0);
  odm_set_bb_reg(dm, 0x5360, 0xfffff000, 0xf0001);
 } else {
  odm_set_bb_reg(dm, 0x1830, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x4130, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x5230, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x5330, BIT(30), 0x1);
  odm_set_bb_reg(dm, 0x1c38, MASKDWORD, 0xFFA1005E);
  odm_set_bb_reg(dm, 0x1c68, BIT(24), 0x0);
  odm_set_bb_reg(dm, 0x1c68, BIT(25), 0x0);
  odm_set_bb_reg(dm, 0x1c68, BIT(26), 0x0);
  odm_set_bb_reg(dm, 0x1c68, BIT(27), 0x0);
  odm_set_bb_reg(dm, 0x1834, BIT(15), 0x0);
  odm_set_bb_reg(dm, 0x4134, BIT(15), 0x0);
  odm_set_bb_reg(dm, 0x5234, BIT(15), 0x0);
  odm_set_bb_reg(dm, 0x5334, BIT(15), 0x0);
  odm_set_bb_reg(dm, 0x1d60, BIT(31), 0x0);
  odm_set_bb_reg(dm, 0x1b08, BIT(07), 0x1);
  odm_set_bb_reg(dm, 0x180c, 0x3, 0x3);
  odm_set_bb_reg(dm, 0x410c, 0x3, 0x3);
  odm_set_bb_reg(dm, 0x520c, 0x3, 0x3);
  odm_set_bb_reg(dm, 0x530c, 0x3, 0x3);
 }
if(!do_txgapk)--dm->port->restore_depth;
}

static void _txgapk_bp_rf_gain_table_8814b(struct dm_struct *dm)
{
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 struct _hal_rf_ *rf = &dm->rf_table;
 struct _halrf_txgapk_info *txgapk_info = &(rf->halrf_txgapk_info);
 u8 idx = 0x0, rf_path = 0x0, band = 0x0;
 u8 rf0_idx_current = 0x0;
 u8 ch_num[5] = {0x0, 0x0, 0x1, 0x3, 0x5};
 u32 rf18 = 0x0;
 u8 end = 10;
                                                           ;
 rf18 = odm_get_rf_reg(dm, RF_PATH_A, 0x18, 0xfffff);
 for (rf_path = 0; rf_path < 4; rf_path++) {
 for(band = 0; band < 5; band++) {
  odm_set_rf_reg(dm, rf_path, 0x18, 0x70000, ch_num[band]);
  if (band == 0) {
   odm_set_rf_reg(dm, rf_path, 0x1a, 0x00001, 0x1);
   odm_set_rf_reg(dm, rf_path, 0x1a, 0x10000, 0x1);
  }
  else {
   odm_set_rf_reg(dm, rf_path, 0x1a, 0x00001, 0x0);
   odm_set_rf_reg(dm, rf_path, 0x1a, 0x10000, 0x0);
  }
                                                                                               ;
                                                                                               ;
  odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | rf_path << 1);
  odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
  odm_set_rf_reg(dm, rf_path, 0xde, BIT(16), 0x0);
  for (idx = 1; idx <= end; idx++) {
   rf0_idx_current = 3 * (idx - 1) + 1;
   odm_set_rf_reg(dm, rf_path, 0x0, 0x1f, rf0_idx_current);
   ODM_delay_us(10);
   txgapk_info->txgapk_rf3f_bp[band][idx][rf_path] = odm_get_rf_reg(dm, rf_path, 0x56, 0x7fff);
                                                                                          ;
  }
                                 ;
  odm_set_rf_reg(dm, rf_path, 0xde, BIT(16), 0x1);
 }
 }
 odm_set_rf_reg(dm, RF_PATH_A, 0x18, 0xfffff, rf18);
 txgapk_info->txgapk_bp_done = true;
 return;
}

static u64 _txgapk_one_shot_8814b(struct dm_struct *dm, u8 path)
{
 u32 txgap_k1 = 1, txgap_k2 = 2;
 u64 rx_dsp_power = 0;
 u32 temp2 = 0x0;
 u32 temp1 = 0x0;
 u32 bitmask2616 = 0x07ff0000;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x1);
 odm_write_4byte(dm, 0x1b34, 0x00000001);
 odm_write_4byte(dm, 0x1b34, 0x00000000);
 ODM_delay_us(100);
 odm_write_4byte(dm, 0x1bd4 , 0x00250001);
 txgap_k1 = odm_get_bb_reg(dm, 0x1bfc, bitmask2616);
 odm_write_4byte(dm, 0x1bd4 , 0x002e0001);
 txgap_k2 = odm_read_4byte(dm, 0x1bfc);
 rx_dsp_power = (u64)((u64)(txgap_k1 * 4294967296) + (u64)txgap_k2);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 temp1 = odm_get_rf_reg(dm, path, 0xde, BIT(16)) & 0x1;
 if (temp1 != 0x1) {
  odm_set_rf_reg(dm, path, 0xde, BIT(16), 0x1);
  temp2 = odm_get_rf_reg(dm, path, 0xde, BIT(16));
  if (temp2 != 0x1)
   rx_dsp_power = 0xabcdef;
 } else {
  odm_set_rf_reg(dm, path, 0xde, BIT(16), 0x0);
  temp2 = odm_get_rf_reg(dm, path, 0xde, BIT(16));
  if (temp2 != 0x0)
   rx_dsp_power = 0xabcdef;
 }
 if(!rx_dsp_power||rx_dsp_power==0xabcdefULL)phy_fail(dm,EIO);
 return rx_dsp_power;
}

static void _txgapk_phy_8814b(struct dm_struct *dm, u8 path)
{
                                                                      ;
 odm_set_bb_reg(dm, R_0x1b00, BIT(2) | BIT(1), path);
 odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x3f);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 odm_set_bb_reg(dm, 0x1b14, BIT(8), 0x0);
 odm_set_bb_reg(dm, 0x1b1c, BIT(29), 0x0);
 odm_set_bb_reg(dm, 0x1b1c, BIT(1), 0x0);
 odm_set_bb_reg(dm, 0x1b1c, BIT(0), 0x1);
 odm_set_bb_reg(dm, 0x1b1c, BIT(8), 0x1);
 odm_set_bb_reg(dm, 0x1b3c, MASKDWORD, 0x40000000);
 odm_set_bb_reg(dm, 0x1b24, 0x000fffff, 0x50008);
 odm_set_bb_reg(dm, 0x1bcc, 0x0000003f, 0x1b);
 odm_set_bb_reg(dm, 0x1b2c, MASKDWORD, 0x00180018);
 odm_set_bb_reg(dm, 0x1b30, MASKDWORD, 0x40000000);
 odm_set_bb_reg(dm, 0x1b18, BIT(1), 0x0);
}

static void _txgapk_phy_restore_8814b(struct dm_struct *dm)
{
 ++dm->port->restore_depth;

 u8 path = 0x0;
 for (path = 0x0; path < 0x4; path++) {
  odm_set_bb_reg(dm, R_0x1b00, BIT(2)| BIT(1), path);
  odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
  odm_set_bb_reg(dm, 0x1bcc, 0x000000ff, 0x0);
  odm_set_bb_reg(dm, R_0x1b00, BIT(2)| BIT(1), 0x0);
 }
--dm->port->restore_depth;
}

static void _txgapk_reload_rf_gain_table_8814b(struct dm_struct *dm, boolean is_cck, u8 rf_path)
{
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 struct _hal_rf_ *rf = &dm->rf_table;
 struct _halrf_txgapk_info *txgapk_info = &(rf->halrf_txgapk_info);
 u8 idx = 0x0, band = 0x0;
 u32 tmp = 0x0, tmp2 = 0x0;
 u8 end = 10;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | rf_path << 1);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 odm_set_rf_reg(dm, rf_path, 0xee, BIT(16), 0x1);
                                                           ;
                                                                                              ;
 for (idx = 0x1; idx <= end; idx++) {
  switch (*dm->band_type) {
  case ODM_BAND_2_4G:
   if (is_cck) {
    tmp = 0x60 | (idx - 1);
    band = 0;
   } else {
    tmp = 0x20 | (idx - 1);
    band = 1;
   }
   break;
  case ODM_BAND_5G:
   if (*dm->channel > 144) {
    tmp = 0x300 | (idx - 1);
    band = 4;
   } else if (*dm->channel > 64) {
    tmp = 0x280 | (idx - 1);
    band = 3;
   } else {
    tmp = 0x200 | (idx - 1);
    band = 2;
   }
   break;
  }
  tmp2 = txgapk_info->txgapk_rf3f_bp[band][idx][rf_path];
  odm_set_rf_reg(dm, rf_path, 0x33, 0xfffff, tmp);
  odm_set_rf_reg(dm, rf_path, 0x3f, 0x07fff, tmp2);
                        ;
 }
                                ;
 odm_set_rf_reg(dm, rf_path, 0xee, BIT(16), 0x0);
}

static void _txgapk_rf_restore_8814b(struct dm_struct *dm)
{
 ++dm->port->restore_depth;

 odm_set_rf_reg(dm, RF_PATH_A, RF_0xde, BIT(16), 0x0);
 odm_set_rf_reg(dm, RF_PATH_B, RF_0xde, BIT(16), 0x0);
 odm_set_rf_reg(dm, RF_PATH_C, RF_0xde, BIT(16), 0x0);
 odm_set_rf_reg(dm, RF_PATH_D, RF_0xde, BIT(16), 0x0);
--dm->port->restore_depth;
}

static void _txgapk_rf_setting_8814b(struct dm_struct *dm,
         u8 path)
{
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 odm_write_4byte(dm, 0x1b00, IQK_CMD_8814B | path << 1);
 odm_set_bb_reg(dm, 0x1bb8, BIT(20), 0x0);
 switch (*dm->band_type) {
 case ODM_BAND_2_4G:
                                                                           ;
  odm_set_rf_reg(dm, (enum rf_path)path, 0x5c, 0x03000, 0x1);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x5c, 0xf0000, 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x80, BIT(10), 0x1);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0x3e0, 0xd);
  odm_set_rf_reg(dm, (enum rf_path)path, 0xdf, BIT(16), 0x1);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x88, 0x300, 0x3);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x88, 0xc00, 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8e, 0x00f, 0xb);
  break;
 case ODM_BAND_5G:
                                                                           ;
   odm_set_rf_reg(dm, (enum rf_path)path, 0x63, 0x30000, 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x63, 0x0f000, 0x7);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8c, BIT(4), 0x0);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8c, 0x1e0, 0x2);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x00, 0x3e0, 0x4);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8e, 0x00f, 0xb);
  break;
 }
}

static void phy_txgap_calibrate_8814b(void *dm_void,
          boolean clear)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 struct _ADAPTER *p_adapter = dm->adapter;
 struct _hal_rf_ *p_rf = &dm->rf_table;
 if (!(p_rf->rf_supportability & HAL_RF_TXGAPK))
  return;
 if (dm->ext_pa && (*dm->band_type == ODM_BAND_2_4G)) {
                                                                       ;
  return;
 } else if (dm->ext_pa_5g && (*dm->band_type == ODM_BAND_5G)) {
                                                                          ;
  return;
 }
                                                                          ;
 if (*dm->band_type == ODM_BAND_2_4G) {
  _phy_txgapk_calibrate_8814b(dm, true);
  _phy_txgapk_calibrate_8814b(dm, false);
 } else
  _phy_txgapk_calibrate_8814b(dm, false);
                                                                        ;
 return;
}
