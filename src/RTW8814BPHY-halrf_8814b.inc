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

static void _phy_rtc_8814b(struct dm_struct *dm)
{
 u32 tmp0xc, cv_offset;
                                                  ;
 tmp0xc = config_phydm_read_syn_reg_8814b(dm, RF_SYN0, 0xc, RFREG_MASK);
 config_phydm_write_rf_syn_8814b(dm, RF_SYN0, 0xc, RFREG_MASK, tmp0xc & ~BIT(2));
 config_phydm_write_rf_syn_8814b(dm, RF_SYN0, 0xc, RFREG_MASK, tmp0xc | BIT(2));
 tmp0xc = config_phydm_read_syn_reg_8814b(dm, RF_SYN1, 0xc, RFREG_MASK);
 config_phydm_write_rf_syn_8814b(dm, RF_SYN1, 0xc, RFREG_MASK, tmp0xc & ~BIT(2));
 config_phydm_write_rf_syn_8814b(dm, RF_SYN1, 0xc, RFREG_MASK, tmp0xc | BIT(2));
                                                ;
}

static void halrf_dac_cal_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 
 u8 i;
 u32 temp = 0, v1 = 0, v2 = 0;
 u32 iadc[4]={0}, qadc[4]={0}, idac[4]={0}, qdac[4]={0};
 u32 bp[DACK_REG_8814B];
 u32 bp_reg[DACK_REG_8814B] = {0x180c, 0x410c, 0x520c, 0x530c, 0x1c3c,
          0x1c24, 0x1d70, 0x9b4, 0x1a00, 0x1a14,
          0x1d58, 0x1c38, 0x1e24, 0x1e28, 0x1860,
          0x4160, 0x5260, 0x5360, 0x1810, 0x4110,
          0x5210, 0x5310, 0x18bc, 0x41bc, 0x52bc,
          0x53bc};
 u32 bp_rf[DACK_RF_8814B][4];
 u32 bp_rfreg[DACK_RF_8814B] = {0x8f};
 dm->port->dack_runs++;
                                                   ;
                                                                       ;
 halrf_bp(dm, bp_reg, bp, DACK_REG_8814B);
 halrf_bprf(dm, bp_rfreg, bp_rf,DACK_RF_8814B, 4);
 halrf_dack_setbb_8814b(dm);
 for (i = 0; i < 4; i++) {
  halrf_dack1_8814b(dm, i, iadc, qadc, &v1, &v2);
  halrf_dack2_8814b(dm, i, idac, qdac, v2, v1);
 }
 halrf_dack_reload_8814b(dm);
 halrf_reload_bp(dm, bp_reg, bp, DACK_REG_8814B);
 halrf_reload_bprf(dm,bp_rfreg, bp_rf, DACK_RF_8814B, 4);
 for (i = 0; i < 4; i++)
                          ;
 for (i = 0; i < 4; i++)
                          ;
                                                   ;
}

static void halrf_dack1_8814b(struct dm_struct *dm, u8 path, u32 *iadc, u32 *qadc,
               u32 *v1, u32 *v2)
{
 u8 i;
 u32 p, idc, qdc, ic, qc, abs_idc, abs_qdc, temp, adc_temp;
 p = halrf_pathsel1_8814b(dm, path);
                                                                       ;
 odm_set_bb_reg(dm, p + 0x30, BIT(30), 0x0);
 odm_write_4byte(dm, p + 0x60, 0xf0040ff0);
 odm_write_4byte(dm, 0x9b4, 0xdb66db00);
 odm_write_4byte(dm, p + 0x10, 0x000dc304);
 odm_write_4byte(dm, p + 0x0c, 0x10000260);
 i = 0;
 while (i < 10) {
  ic = qc = 0;
  i++;
                                                    ;
  odm_write_4byte(dm, 0x1c3c, 0x00088003 | path << 17);
  odm_write_4byte(dm, 0x1c24, 0x00010002);
  halrf_mode(dm, &idc, &qdc);
                  ;
  if (idc != 0x0)
   ic = 0x400 - idc;
  if (qdc != 0x0)
   qc = 0x400 - qdc;
  *v1 = (ic & 0x3ff) | ((qc & 0x3ff) << 10);
  odm_write_4byte(dm, p + 0x68, *v1);
                                                           ;
  odm_write_4byte(dm, 0x1c3c, 0x00088103 | path << 17);
  halrf_mode(dm, &idc, &qdc);
  iadc[path] = idc;
  qadc[path] = qdc;
                  ;
  if (idc >= 0x200)
   abs_idc = 0x400 - idc;
  else
   abs_idc = idc;
  if (qdc >= 0x200)
   abs_qdc = 0x400 - qdc;
  else
   abs_qdc = qdc;
  if (abs_idc < 5 && abs_qdc < 5)
   break;
 }
 if (ic < 0x300) {
  ic = ic + 0x80;
 } else {
  ic = 0x80 - (0x400 - ic);
 }
 if (qc < 0x300) {
  qc = qc + 0x80;
 } else {
  qc = 0x80 - (0x400 - qc);
 }
 *v2 = (ic & 0x3ff) | ((qc & 0x3ff) << 10);
if(i==10 && !(abs_idc<5&&abs_qdc<5)){phy_fail(dm,EIO);return;}
}

static void halrf_dack2_8814b(struct dm_struct *dm, u8 path, u32 *idac, u32 *qdac,
         u32 v1, u32 v2)
{
 u8 i, c;
 u32 p, pr, idc, qdc;
                                                                       ;
 p = halrf_pathsel1_8814b(dm, path);
 pr = halrf_pathsel2_8814b(dm, path);
 i = 0;
 while (i < 10) {
  i++;
                                                    ;
  odm_write_4byte(dm, p + 0x68, v1);
                                                           ;
  odm_write_4byte(dm, p + 0x10, 0x0005C304);
  odm_write_4byte(dm, p + 0x0c, 0x50000260);
  odm_set_rf_reg(dm, (enum rf_path)path, 0x8f, BIT(13), 0x1);
  odm_write_4byte(dm, p + 0x10, 0x0005c305);
  odm_write_4byte(dm, p + 0xbc, 0x0003ff81);
  odm_write_4byte(dm, p + 0xb0, 0x0a187ff8);
  odm_write_4byte(dm, p + 0xb4, 0x00000000);
  odm_write_4byte(dm, p + 0xb8, 0x00000000);
  odm_write_4byte(dm, p + 0xbc, 0x2403ff81);
  odm_write_4byte(dm, p + 0xc0, 0x80005308);
  odm_write_4byte(dm, p + 0xc4, 0x01000000);
  odm_write_4byte(dm, p + 0xc8, 0x001dffe0);
  odm_write_4byte(dm, p + 0xcc, 0x0a187fe8);
  odm_write_4byte(dm, p + 0xd0, 0x00000000);
  odm_write_4byte(dm, p + 0xd4, 0x00000000);
  odm_write_4byte(dm, p + 0xd8, 0x2403ff81);
  odm_write_4byte(dm, p + 0xdc, 0x80005308);
  odm_write_4byte(dm, p + 0xe0, 0x01000000);
  odm_write_4byte(dm, p + 0xe4, 0x001dffe0);
  odm_write_4byte(dm, p + 0xb0, 0x0a187fe9);
  odm_write_4byte(dm, p + 0xcc, 0x0a187fe9);
  odm_write_4byte(dm, p + 0xbc, 0x2503ff81);
  odm_write_4byte(dm, p + 0xd8, 0x2503ff81);
  odm_write_4byte(dm, p + 0xb8, 0x60000000);
  odm_write_4byte(dm, p + 0xd4, 0x60000000);
  ODM_delay_ms(1);
  odm_write_4byte(dm, p + 0xb8, 0x62000000);
  odm_write_4byte(dm, p + 0xd4, 0x62000000);
  ODM_delay_ms(10);
  c = 0;
  while (c < 30) {
   if (odm_get_bb_reg(dm, pr + 0x08, 0x7fff80) == 0xffff &&
       odm_get_bb_reg(dm, pr + 0x34, 0x7fff80) == 0xffff &&
       odm_get_bb_reg(dm, pr + 0x10, 0x2) == 0x1 &&
       odm_get_bb_reg(dm, pr + 0x3c, 0x2) == 0x1)
    break;
   else
    c++;
   ODM_delay_ms(1);
   if(c==30){phy_fail(dm,ETIMEDOUT);return;}
  }
  odm_write_4byte(dm, p + 0xb8, 0x02000000);
  odm_write_4byte(dm, p + 0xd4, 0x02000000);
  odm_write_4byte(dm, p + 0xc8, 0x000dffe0);
  odm_write_4byte(dm, p + 0xe4, 0x000dffe0);
  odm_write_4byte(dm, p + 0xbc, 0x2d03ff81);
  ODM_delay_ms(1);
  c = 0;
  while (c < 30) {
   if (odm_get_bb_reg(dm, pr + 0x2c, 0xff000) == 0xff)
    break;
   else
    c++;
   ODM_delay_ms(1);
   if(c==30){phy_fail(dm,ETIMEDOUT);return;}
  }
  odm_write_4byte(dm, p + 0xd8, 0x2d03ff81);
  ODM_delay_ms(1);
  c = 0;
  while (c < 30) {
   if (odm_get_bb_reg(dm, pr + 0x58, 0xff000) == 0xff)
    break;
   else
    c++;
   ODM_delay_ms(1);
   if(c==30){phy_fail(dm,ETIMEDOUT);return;}
  }
  odm_write_4byte(dm, p + 0xc8, 0x001dffe0);
  odm_write_4byte(dm, p + 0xe4, 0x001dffe0);
  odm_write_4byte(dm, p + 0x68, v2);
                                                          ;
  odm_write_4byte(dm, 0x9b4, 0xdb6db600);
  odm_write_4byte(dm, 0x1c3c, 0x00088103 | path << 17);
  odm_write_4byte(dm, p + 0x10, 0x0005c305);
  odm_write_4byte(dm, p + 0xbc, 0x2d03ff81);
  odm_write_4byte(dm, p + 0x60, 0xf0040ff0);
  halrf_mode(dm, &idc, &qdc);
  idac[path] = idc;
  qdac[path] = qdc;
                  ;
  if (idc >= 0x200)
   idc = 0x400 - idc;
  if (qdc >= 0x200)
   qdc = 0x400 - qdc;
  if (idc < 5 && qdc < 5)
   break;
 }
if(i==10 && !(idc<5&&qdc<5)){phy_fail(dm,EIO);return;}
}

static void halrf_dack_reload_8814b(struct dm_struct *dm)
{
 u8 i;
 u32 p;
 for (i = 0; i < 4; i++) {
  p = halrf_pathsel1_8814b(dm, i);
  odm_set_bb_reg(dm, p + 0x30, BIT(30), 0x1);
  odm_write_4byte(dm, 0x1b00, 0x00000008 | i << 1);
  odm_write_1byte(dm, 0x1bcc, 0x00);
 }
}

static void halrf_dack_setbb_8814b(struct dm_struct *dm)
{
 odm_set_bb_reg(dm, 0x1d58, 0xff8, 0x1ff);
 odm_set_bb_reg(dm, 0x1a00, 0x3, 0x2);
 odm_set_bb_reg(dm, 0x1a14, 0x300, 0x3);
 odm_write_4byte(dm, 0x1d70, 0x7e7e7e7e);
 odm_set_bb_reg(dm, 0x180c, 0x3, 0x0);
 odm_set_bb_reg(dm, 0x410c, 0x3, 0x0);
 odm_set_bb_reg(dm, 0x520c, 0x3, 0x0);
 odm_set_bb_reg(dm, 0x530c, 0x3, 0x0);
 odm_write_4byte(dm, 0x1b00, 0x00000008);
 odm_write_1byte(dm, 0x1bcc, 0x3f);
 odm_write_4byte(dm, 0x1b00, 0x0000000a);
 odm_write_1byte(dm, 0x1bcc, 0x3f);
 odm_write_4byte(dm, 0x1b00, 0x0000000c);
 odm_write_1byte(dm, 0x1bcc, 0x3f);
 odm_write_4byte(dm, 0x1b00, 0x0000000e);
 odm_write_1byte(dm, 0x1bcc, 0x3f);
 odm_set_bb_reg(dm, 0x1e24, BIT(31), 0x0);
 odm_set_bb_reg(dm, 0x1e28, 0xf, 0x3);
 odm_set_rf_reg(dm, RF_PATH_A, 0x0, RFREG_MASK, 0x10000);
 odm_set_rf_reg(dm, RF_PATH_B, 0x0, RFREG_MASK, 0x10000);
 odm_set_rf_reg(dm, RF_PATH_C, 0x0, RFREG_MASK, 0x10000);
 odm_set_rf_reg(dm, RF_PATH_D, 0x0, RFREG_MASK, 0x10000);
}

static void halrf_get_efuse_thermal_pwrtype_8814b(
 void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
 struct _hal_rf_ *rf = &dm->rf_table;
 struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
 u32 thermal_tmp, pg_tmp;
 tssi->thermal[RF_PATH_A] = 0xff;
 tssi->thermal[RF_PATH_B] = 0xff;
 tssi->thermal[RF_PATH_C] = 0xff;
 tssi->thermal[RF_PATH_D] = 0xff;
 odm_efuse_logical_map_read(dm, 1, 0xd0, &thermal_tmp);
 tssi->thermal[RF_PATH_A] = (u8)thermal_tmp;
 odm_efuse_logical_map_read(dm, 1, 0xd1, &thermal_tmp);
 tssi->thermal[RF_PATH_B] = (u8)thermal_tmp;
 odm_efuse_logical_map_read(dm, 1, 0xd2, &thermal_tmp);
 tssi->thermal[RF_PATH_C] = (u8)thermal_tmp;
 odm_efuse_logical_map_read(dm, 1, 0xd3, &thermal_tmp);
 tssi->thermal[RF_PATH_D] = (u8)thermal_tmp;
 odm_efuse_logical_map_read(dm, 1, 0xc8, &pg_tmp);
 rf->power_track_type = (u8)((pg_tmp >> 4) & 0xf);
                             ;
}

static u32 halrf_pathsel1_8814b(struct dm_struct *dm, u8 path)
{
 switch (path) {
 case 0:
  return 0x1800;
  break;
 case 1:
  return 0x4100;
  break;
 case 2:
  return 0x5200;
  break;
 case 3:
  return 0x5300;
  break;
 default:
  return 0x1800;
  break;
 }
}

static u32 halrf_pathsel2_8814b(struct dm_struct *dm, u8 path)
{
 switch (path) {
 case 0:
  return 0x2800;
  break;
 case 1:
  return 0x4500;
  break;
 case 2:
  return 0x5600;
  break;
 case 3:
  return 0x5700;
  break;
 default:
  return 0x2800;
  break;
 }
}

static void halrf_tssi_get_efuse_8814b(
 void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct _hal_rf_ *rf = &dm->rf_table;
 struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
 u8 pg = 0xff, i, j;
 u32 pg_tmp;
                                                      ;
 j = 0;
 for (i = 0x10; i <= 0x1a; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[0][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 for (i = 0x22; i <= 0x2f; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[0][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 j = 0;
 for (i = 0x3a; i <= 0x44; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[1][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 for (i = 0x4c; i <= 0x59; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[1][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 j = 0;
 for (i = 0x64; i <= 0x6e; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[2][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 for (i = 0x76; i <= 0x83; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[2][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 j = 0;
 for (i = 0x8e; i <= 0x98; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[3][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
 for (i = 0xa0; i <= 0xad; i++) {
  odm_efuse_logical_map_read(dm, 1, i, &pg_tmp);
  tssi->tssi_efuse[3][j] = (s8)pg_tmp;
                                                                 ;
  j++;
 }
}

static void phy_lc_calibrate_8814b(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 _phy_rtc_8814b(dm);
}
