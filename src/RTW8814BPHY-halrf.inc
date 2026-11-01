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

/*@************************************************************
 * include files
 * ************************************************************
 */

static void halrf_b_sort(void *dm_void, u32 *iv, u32 *qv)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u32 i, j;
                                                    ;
 for (i = 0; i < SN - 1; i++) {
  for (j = 0; j < (SN - 1 - i) ; j++) {
   halrf_bubble(dm, &iv[j], &iv[j + 1]);
   halrf_bubble(dm, &qv[j], &qv[j + 1]);
  }
 }
}

static void halrf_bp(void *dm_void, u32 *bp_reg, u32 *bp, u32 num)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u32 i;
 for (i = 0; i < num; i++)
  bp[i] = odm_read_4byte(dm, bp_reg[i]);
}

static void halrf_bprf(void *dm_void, u32 *bp_reg, u32 bp[][4], u32 num, u8 ss)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u32 i, path;
 for (i = 0; i < num; i++) {
  for (path = 0; path < ss; path++) {
   bp[i][path] =
    odm_get_rf_reg(dm, (enum rf_path)path,
            bp_reg[i], MASK20BITS);
  }
 }
}

static void halrf_bubble(void *dm_void, u32 *v1, u32 *v2)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 if (*v1 >= 0x200 && *v2 >= 0x200) {
  if (*v1 > *v2)
   halrf_swap(dm, v1, v2);
 } else if (*v1 < 0x200 && *v2 < 0x200) {
  if (*v1 > *v2)
   halrf_swap(dm, v1, v2);
 } else if (*v1 < 0x200 && *v2 >= 0x200) {
  halrf_swap(dm, v1, v2);
 }
}

static boolean halrf_compare(void *dm_void, u32 value)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 boolean fail = false;
 if (value >= 0x200 && (0x400 - value) > 0x64)
  fail = true;
 else if (value < 0x200 && value > 0x64)
  fail = true;
 if (fail)
                                                          ;
 return fail;
}

static void halrf_get_fw_version(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 struct _hal_rf_ *rf = &dm->rf_table;
 rf->fw_ver = (dm->fw_version << 16) | dm->fw_sub_version;
}

static void halrf_iqk_dbg(void *dm_void)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u8 rf_path, j;
 u32 tmp;
 boolean iqk_result[2][NUM][2];
 struct dm_iqk_info *iqk_info = &dm->IQK_info;
 struct _hal_rf_ *rf = &dm->rf_table;
                                                            ;
                     ;
 if (dm->fw_offload_ability & PHYDM_RF_IQK_OFFLOAD) {
  halrf_get_fw_version(dm);
                                                               ;
 } else {
                                                                 ;
 }
                                                 ;
                                          ;
                                                               ;
 if (*dm->band_width == CHANNEL_WIDTH_20)
                                                             ;
 else if (*dm->band_width == CHANNEL_WIDTH_40)
                                                             ;
 else if (*dm->band_width == CHANNEL_WIDTH_80)
                                                             ;
 else if (*dm->band_width == CHANNEL_WIDTH_160)
                                                              ;
 else if (*dm->band_width == CHANNEL_WIDTH_80_80)
                                                                ;
 else
                      ;
                                                     ;
                                                   ;
                                                           ;
 tmp = odm_read_4byte(dm, 0x1bf0);
 for (rf_path = RF_PATH_A; rf_path <= RF_PATH_B; rf_path++)
  for (j = 0; j < 2; j++)
   iqk_result[0][rf_path][j] = (boolean)
   (tmp & (BIT(rf_path + (j * 4)) >> (rf_path + (j * 4))));
                                                            ;
                                   ;
                                                        ;
                                                        ;
}

static void halrf_iqk_hwtx_check(void *dm_void, boolean is_check)
{
}

static void halrf_minmax_compare(void *dm_void, u32 value, u32 *min,
     u32 *max)
{
 if (value >= 0x200) {
  if (*min >= 0x200) {
   if (*min > value)
    *min = value;
  } else {
   *min = value;
  }
  if (*max >= 0x200) {
   if (*max < value)
    *max = value;
  }
 } else {
  if (*min < 0x200) {
   if (*min > value)
    *min = value;
  }
  if (*max >= 0x200) {
   *max = value;
  } else {
   if (*max < value)
    *max = value;
  }
 }
}

static void halrf_mode(void *dm_void, u32 *i_value, u32 *q_value)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 u32 iv[SN], qv[SN], im[SN], qm[SN], temp;
 u32 p, m, t;
 u32 i_max = 0, q_max = 0, i_min = 0x0, q_min = 0x0, c = 0x0;
 u32 i_delta, q_delta;
 u8 i;
 boolean fail = false;
 ODM_delay_ms(10);
 for (i = 0; i < SN; i++) {
  im[i] = 0;
  qm[i] = 0;
 }
 i = 0;
 c = 0;
 while (i < SN && c < 1000) {
  c++;
  temp = odm_get_bb_reg(dm, 0x2dbc, 0x3fffff);
  iv[i] = (temp & 0x3ff000) >> 12;
  qv[i] = temp & 0x3ff;
  fail = false;
  if (halrf_compare(dm, iv[i]))
   fail = true;
  if (halrf_compare(dm, qv[i]))
   fail = true;
  if (!fail)
   i++;
 }
 if(i!=SN){*i_value=*q_value=0;phy_fail(dm,EIO);return;}
 c = 0;
 do {
  c++;
  i_min = iv[0];
  i_max = iv[0];
  q_min = qv[0];
  q_max = qv[0];
  for (i = 0; i < SN; i++) {
   halrf_minmax_compare(dm, iv[i], &i_min, &i_max);
   halrf_minmax_compare(dm, qv[i], &q_min, &q_max);
  }
                      ;
                      ;
  if (i_max < 0x200 && i_min < 0x200)
   i_delta = i_max - i_min;
  else if (i_max >= 0x200 && i_min >= 0x200)
   i_delta = i_max - i_min;
  else
   i_delta = i_max + (0x400 - i_min);
  if (q_max < 0x200 && q_min < 0x200)
   q_delta = q_max - q_min;
  else if (q_max >= 0x200 && q_min >= 0x200)
   q_delta = q_max - q_min;
  else
   q_delta = q_max + (0x400 - q_min);
                          ;
  halrf_b_sort(dm, iv, qv);
  if (i_delta > 5 || q_delta > 5) {
   temp = odm_get_bb_reg(dm, 0x2dbc, 0x3fffff);
   iv[0] = (temp & 0x3ff000) >> 12;
   qv[0] = temp & 0x3ff;
   temp = odm_get_bb_reg(dm, 0x2dbc, 0x3fffff);
   iv[SN - 1] = (temp & 0x3ff000) >> 12;
   qv[SN - 1] = temp & 0x3ff;
  } else {
   break;
  }
 } while (c < 100);
 if(i_delta>5||q_delta>5){*i_value=*q_value=0;phy_fail(dm,EIO);return;}
 m = 0;
 p = 0;
 for (i = 10; i < SN - 10; i++) {
  if (iv[i] > 0x200)
   m = (0x400 - iv[i]) + m;
  else
   p = iv[i] + p;
 }
 if (p > m) {
  t = p - m;
  t = t / (SN - 20);
 } else {
  t = m - p;
  t = t / (SN - 20);
  if (t != 0x0)
   t = 0x400 - t;
 }
 *i_value = t;
 m = 0;
 p = 0;
 for (i = 10; i < SN - 10; i++) {
  if (qv[i] > 0x200)
   m = (0x400 - qv[i]) + m;
  else
   p = qv[i] + p;
 }
 if (p > m) {
  t = p - m;
  t = t / (SN - 20);
 } else {
  t = m - p;
  t = t / (SN - 20);
  if (t != 0x0)
   t = 0x400 - t;
 }
 *q_value = t;
}

static u32 halrf_psd_log2base(u32 val)
{
 u8 j;
 u32 tmp, tmp2, val_integerd_b = 0, tindex, shiftcount = 0;
 u32 result, val_fractiond_b = 0;
 u32 table_fraction[21] = {
  0, 432, 332, 274, 232, 200, 174, 151, 132, 115,
  100, 86, 74, 62, 51, 42, 32, 23, 15, 7, 0};
 if (val == 0)
  return 0;
 tmp = val;
 while (1) {
  if (tmp == 1)
   break;
  tmp = (tmp >> 1);
  shiftcount++;
 }
 val_integerd_b = shiftcount + 1;
 tmp2 = 1;
 for (j = 1; j <= val_integerd_b; j++)
  tmp2 = tmp2 * 2;
 tmp = (val * 100) / tmp2;
 tindex = tmp / 5;
 if (tindex > 20)
  tindex = 20;
 val_fractiond_b = table_fraction[tindex];
 result = val_integerd_b * 100 - val_fractiond_b;
 return result;
}

static void halrf_reload_bp(void *dm_void, u32 *bp_reg, u32 *bp, u32 num)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 ++dm->port->restore_depth;

 u32 i;
 for (i = 0; i < num; i++)
  odm_write_4byte(dm, bp_reg[i], bp[i]);
--dm->port->restore_depth;
}

static void halrf_reload_bprf(void *dm_void, u32 *bp_reg, u32 bp[][4], u32 num,
         u8 ss)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 ++dm->port->restore_depth;

 u32 i, path;
 for (i = 0; i < num; i++) {
  for (path = 0; path < ss; path++)
   odm_set_rf_reg(dm, (enum rf_path)path, bp_reg[i],
           MASK20BITS, bp[i][path]);
 }
--dm->port->restore_depth;
}

static void halrf_swap(void *dm_void, u32 *v1, u32 *v2)
{
 u32 temp;
 temp = *v1;
 *v1 = *v2;
 *v2 = temp;
}

static void phydm_get_iqk_cfir(void *dm_void, u8 idx, u8 path, boolean debug)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 switch (dm->support_ic_type) {
 case ODM_RTL8814B:
  phy_get_iqk_cfir_8814b(dm, idx, path, debug);
 break;
 default:
 break;
 }
}
