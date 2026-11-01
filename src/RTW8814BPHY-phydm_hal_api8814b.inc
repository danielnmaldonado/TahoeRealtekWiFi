/* SPDX-License-Identifier: GPL-2.0-only
 * Generated from tobyw121/Openwrt_RTL@85ef7eb7ccbd4336cd6e628a743ddc211333901c.
 * Vendor notices and primary algorithms preserved; checked host adapter only. */
/* ****************************************************************************
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
 * ****************************************************************************/

static void config_phydm_cca_dyn_pw_th(struct dm_struct *dm, enum channel_width bw)
{
 switch (bw) {
 case CHANNEL_WIDTH_20: {
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1a75e);
  odm_set_bb_reg(dm, R_0x8a4, 0x3f, 0x15);
  break;
 }
 case CHANNEL_WIDTH_40: {
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1769b);
  odm_set_bb_reg(dm, R_0x870, 0x3ffff, 0x1769b);
  odm_set_bb_reg(dm, R_0x8a4, MASK12BITS, 0x492);
  break;
 }
 case CHANNEL_WIDTH_80: {
  odm_set_bb_reg(dm, R_0x830, 0x3f, 0x14);
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x145d8);
  odm_set_bb_reg(dm, R_0x870, 0x3fffffff, 0x1a6d45d8);
  odm_set_bb_reg(dm, R_0x874, 0x3f, 0x17);
  odm_set_bb_reg(dm, R_0x8a4, 0x3ffff, 0x12249);
  break;
 }
 case CHANNEL_WIDTH_5: {
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1a75e);
  odm_set_bb_reg(dm, R_0x8a4, 0x3f, 0x15);
  break;
 }
 case CHANNEL_WIDTH_10: {
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1a75e);
  odm_set_bb_reg(dm, R_0x8a4, 0x3f, 0x15);
  break;
 }
 case CHANNEL_WIDTH_80_80: {
  odm_set_bb_reg(dm, R_0x830, 0x3f, 0x1a);
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1B79F);
  odm_set_bb_reg(dm, R_0x8a4, 0x3ffff, 0x18596);
  odm_set_bb_reg(dm, R_0x870, 0x3FFFFFFF, 0x2085D75E);
  odm_set_bb_reg(dm, R_0x874, 0x3f, 0x1d);
  break;
 }
 default:
                                             ;
 }
}

static boolean
config_phydm_parameter_init_8814b(struct dm_struct *dm,
      enum odm_parameter_init type)
{
 odm_set_bb_reg(dm, R_0x180c, 0x3, 0x3);
 odm_set_bb_reg(dm, R_0x180c, BIT(28), 0x1);
 odm_set_bb_reg(dm, R_0x410c, 0x3, 0x3);
 odm_set_bb_reg(dm, R_0x410c, BIT(28), 0x1);
 odm_set_bb_reg(dm, R_0x520c, 0x3, 0x3);
 odm_set_bb_reg(dm, R_0x520c, BIT(28), 0x1);
 odm_set_bb_reg(dm, R_0x530c, 0x3, 0x3);
 odm_set_bb_reg(dm, R_0x530c, BIT(28), 0x1);
 config_phydm_write_rf_syn_8814b(dm, RF_SYN0, RF_0x0, 0xf0000, 0x2);
 config_phydm_write_rf_syn_8814b(dm, RF_SYN1, RF_0x0, 0xf0000, 0x0);
 if (type == ODM_PRE_SETTING) {
  odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x0);
              ;
 } else if (type == ODM_POST_SETTING) {
  odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x3);
  phydm_config_set_rfc_8814b(dm, rfc_4x4);
              ;
 } else {
                                                               ;
  return false;
 }
 return true;
}

static void
config_phydm_path_adda_setting_8814b(struct dm_struct *dm)
{
 u8 available_path = dm->rx_ant_status | dm->tx_ant_status;
                      ;
 odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x1);
 odm_set_bb_reg(dm, R_0x4130, BIT(30), 0x1);
 odm_set_bb_reg(dm, R_0x5230, BIT(30), 0x1);
 odm_set_bb_reg(dm, R_0x5330, BIT(30), 0x1);
 if (!(available_path & BB_PATH_A))
  odm_set_bb_reg(dm, R_0x1830, BIT(30), 0x0);
 if (!(available_path & BB_PATH_B))
  odm_set_bb_reg(dm, R_0x4130, BIT(30), 0x0);
 if (!(available_path & BB_PATH_C))
  odm_set_bb_reg(dm, R_0x5230, BIT(30), 0x0);
 if (!(available_path & BB_PATH_D))
  odm_set_bb_reg(dm, R_0x5330, BIT(30), 0x0);
}

static u32 config_phydm_read_rf_reg_8814b(struct dm_struct *dm, enum rf_path path,
       u32 reg_addr, u32 bit_mask)
{
 u32 readback_value, direct_addr;
 u32 offset_read_rf[4] = {0x3c00, 0x4c00, 0x5800, 0x5c00};
 if (path > RF_PATH_D) {
                    ;
  return INVALID_RF_DATA;
 }
 reg_addr &= 0xff;
 direct_addr = offset_read_rf[path] + (reg_addr << 2);
 bit_mask &= RFREGOFFSETMASK;
 readback_value = odm_get_bb_reg(dm, direct_addr, bit_mask);
                                       ;
 return readback_value;
}

static u32 config_phydm_read_syn_reg_8814b(struct dm_struct *dm, enum rf_syn syn_path,
        u32 reg_addr, u32 bit_mask)
{
 u32 readback_value, direct_addr;
 u32 offset_read_rf[2] = {0x3b00, 0x4b00};
 if (syn_path > RF_SYN1) {
                        ;
  return INVALID_RF_DATA;
 }
 reg_addr &= 0xff;
 direct_addr = offset_read_rf[syn_path] + (reg_addr << 2);
 bit_mask &= RFREGOFFSETMASK;
 readback_value = odm_get_bb_reg(dm, direct_addr, bit_mask);
                                                 ;
 return readback_value;
}

static boolean
config_phydm_switch_band_8814b(struct dm_struct *dm,
          u8 central_ch)
{
 boolean rf_reg_status = true;
 return rf_reg_status;
}

static boolean
config_phydm_switch_bw_8814b(struct dm_struct *dm, u8 pri_ch,
        enum channel_width bandwidth)
{
 u32 rf_18, rf_syn1_18 = 0;
 boolean rf_reg_status = true;
 boolean en_cca_dyn_pw_th = false;
                                ;
 if (dm->is_disable_phy_api) {
                                                   ;
  return true;
 }
 if (bandwidth >= CHANNEL_WIDTH_MAX ||
     (bandwidth == CHANNEL_WIDTH_40 && pri_ch > 2) ||
     (bandwidth == CHANNEL_WIDTH_80 && pri_ch > 4)) {
                                 ;
  return false;
 }
 bw_8814b = bandwidth;
 if (bandwidth == CHANNEL_WIDTH_80_80) {
  config_phydm_write_rf_syn_8814b(dm, RF_SYN1, RF_0x0,
      0xf0000, 0x2);
  phydm_config_set_rfc_8814b(dm, rfc_2x2);
  rf_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN0, RF_0x18,
       RFREGOFFSETMASK);
  rf_syn1_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN1,
            RF_0x18,
            RFREGOFFSETMASK);
  if (rf_18 != INVALID_RF_DATA && rf_syn1_18 != INVALID_RF_DATA)
   rf_reg_status = true;
  else
   rf_reg_status = false;
 } else {
  phydm_config_set_rfc_8814b(dm, rfc_4x4);
  config_phydm_write_rf_syn_8814b(dm, RF_SYN1, RF_0x0,
      0xf0000, 0x0);
  rf_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN0, RF_0x18,
       RFREGOFFSETMASK);
  if (rf_18 != INVALID_RF_DATA)
   rf_reg_status = true;
  else
   rf_reg_status = false;
 }
 switch (bandwidth) {
 case CHANNEL_WIDTH_20: {
  odm_set_bb_reg(dm, R_0x9b0, 0x3, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xf00, pri_ch);
  odm_set_bb_reg(dm, R_0x9b0, 0xf000, pri_ch);
  odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0xdb6db6);
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1d822);
  odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x0);
  odm_set_bb_reg(dm, R_0x8a4, 0x3f, 0x18);
  rf_18 = (rf_18 | BIT(11) | BIT(10));
  rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  break;
 }
 case CHANNEL_WIDTH_40: {
  if (pri_ch == 1)
   odm_set_bb_reg(dm, R_0x1a00, BIT(4), pri_ch);
  else
   odm_set_bb_reg(dm, R_0x1a00, BIT(4), 0);
  odm_set_bb_reg(dm, R_0x9b0, 0x3, 0x1);
  odm_set_bb_reg(dm, R_0x9b0, 0xc, 0x1);
  odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xf00, pri_ch);
  odm_set_bb_reg(dm, R_0x9b0, 0xf000, pri_ch);
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x21926);
  odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x0);
  odm_set_bb_reg(dm, R_0x8a4, MASK12BITS, 0x71c);
  rf_18 = (rf_18 & (~(BIT(11) | BIT(10))));
  rf_18 = (rf_18 | BIT(11));
  rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  rf_18 = (rf_18 | BIT(12));
  if (central_ch_8814b <= 3 || (central_ch_8814b >= 9 &&
           central_ch_8814b < 14)) {
   rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
                              ;
  }
                             ;
  break;
 }
 case CHANNEL_WIDTH_80: {
  odm_set_bb_reg(dm, R_0x9b0, 0xf, 0xa);
  odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xff00, (pri_ch | (pri_ch << 4)));
  odm_set_bb_reg(dm, R_0x830, 0x3f, 0x1a);
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1B79F);
  odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x6);
  odm_set_bb_reg(dm, R_0x8a4, 0x3ffff, 0x18596);
  rf_18 &= ~(BIT(11) | BIT(10));
  rf_18 |= BIT(10);
  rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  rf_18 = (rf_18 | BIT(13));
  if (central_ch_8814b <= 3 || (central_ch_8814b >= 9 &&
           central_ch_8814b < 14)) {
   rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
   rf_18 = (rf_18 | BIT(12));
  }
                             ;
  break;
 }
 case CHANNEL_WIDTH_5: {
  odm_set_bb_reg(dm, R_0x810, MASKDWORD, 0x10b02ab0);
  odm_set_bb_reg(dm, R_0x9b0, 0x3, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x1);
  odm_set_bb_reg(dm, R_0x9b0, 0xff00, 0x0);
  odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0xdb4db2);
  rf_18 = (rf_18 | BIT(11) | BIT(10));
  rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  break;
 }
 case CHANNEL_WIDTH_10: {
  odm_set_bb_reg(dm, R_0x810, MASKDWORD, 0x10b02ab0);
  odm_set_bb_reg(dm, R_0x9b0, 0x3, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x2);
  odm_set_bb_reg(dm, R_0x9b0, 0xff00, 0x0);
  odm_set_bb_reg(dm, R_0x9b4, MASKH3BYTES, 0xdb4db4);
  rf_18 = (rf_18 | BIT(11) | BIT(10));
  rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  break;
 }
 case CHANNEL_WIDTH_80_80: {
  rf_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN0, RF_0x18,
       RFREGOFFSETMASK);
  rf_syn1_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN1,
            RF_0x18,
            RFREGOFFSETMASK);
  odm_set_bb_reg(dm, R_0x9b0, 0xf, 0xa);
  odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x3);
  odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);
  odm_set_bb_reg(dm, R_0x9b0, 0xff00, (pri_ch | (pri_ch << 4)));
  odm_set_bb_reg(dm, R_0x830, 0x3f, 0x1a);
  odm_set_bb_reg(dm, R_0x86c, 0xffffc000, 0x1B79F);
  odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x6);
  odm_set_bb_reg(dm, R_0x8a4, 0x3ffff, 0x18596);
  odm_set_bb_reg(dm, R_0x870, 0x3FFFFFFF, 0x2085D75E);
  odm_set_bb_reg(dm, R_0x874, 0x3f, 0x1d);
  rf_18 &= ~(BIT(11) | BIT(10));
  rf_18 |= BIT(10);
  rf_syn1_18 &= ~(BIT(11) | BIT(10));
  rf_syn1_18 |= BIT(10);
  rf_18 = (rf_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  rf_18 = (rf_18 | BIT(13));
  rf_syn1_18 = (rf_syn1_18 & (~(BIT(14) | BIT(13) | BIT(12))));
  rf_syn1_18 = (rf_syn1_18 | BIT(13));
  break;
 }
 default:
                                 ;
 }
 if (bandwidth == CHANNEL_WIDTH_80_80) {
  config_phydm_write_rf_syn_8814b(dm, RF_SYN0, RF_0x18,
      RFREGOFFSETMASK, rf_18);
  odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, 0xfffff, rf_18);
  odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, 0xfffff, rf_18);
  config_phydm_write_rf_syn_8814b(dm, RF_SYN1, RF_0x18,
      RFREGOFFSETMASK, rf_syn1_18);
  odm_set_rf_reg(dm, RF_PATH_C, RF_0x18, 0xfffff, rf_syn1_18);
  odm_set_rf_reg(dm, RF_PATH_D, RF_0x18, 0xfffff, rf_syn1_18);
 } else {
  config_phydm_write_rf_syn_8814b(dm, RF_SYN0, RF_0x18,
      RFREGOFFSETMASK, rf_18);
  odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREGOFFSETMASK, rf_18);
  odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREGOFFSETMASK, rf_18);
  odm_set_rf_reg(dm, RF_PATH_C, RF_0x18, RFREGOFFSETMASK, rf_18);
  odm_set_rf_reg(dm, RF_PATH_D, RF_0x18, RFREGOFFSETMASK, rf_18);
 }
 if (!rf_reg_status) {
                                 ;
                                              ;
  return false;
 }
 phydm_igi_toggle_8814b(dm);
 phydm_ccapar_by_rfe_8814b(dm);
 if (dm->rfe_type == 0) {
  en_cca_dyn_pw_th = (boolean)odm_get_bb_reg(dm, R_0x86c, BIT(0));
  if (en_cca_dyn_pw_th) {
                                         ;
   config_phydm_cca_dyn_pw_th(dm, bandwidth);
  }
 } else {
  odm_set_bb_reg(dm, R_0x86c, 0x1, 0x0);
 }
 phydm_bb_reset_8814b(dm);
                                ;
 return true;
}

static boolean
config_phydm_switch_channel_8814b(struct dm_struct *dm, u8 central_ch)
{
 u32 rf_18 = 0;
 u8 band_index = 0, i = 0;
 boolean is_2g_ch = true;
                                                                       ;
 if (dm->is_disable_phy_api) {
                                                   ;
  return true;
 }
 if ((central_ch > 14 && central_ch < 36) ||
     (central_ch > 64 && central_ch < 100) ||
     (central_ch > 144 && central_ch < 149) ||
     central_ch > 177) {
                                                            ;
  return false;
 }
 central_ch_8814b = central_ch;
 if (central_ch_8814b <= 14)
  band_index = 1;
 else
  band_index = 2;
 if (dm->rfe_hwsetting_band != band_index)
  phydm_rfe_8814b(dm, central_ch_8814b);
 rf_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN0, RF_0x18,
      RFREGOFFSETMASK);
 if (rf_18 == INVALID_RF_DATA) {
                                                    ;
  return false;
 }
 is_2g_ch = (central_ch <= 14) ? true : false;
 phydm_agc_tab_sel_8814b(dm, central_ch);
 phydm_sco_trk_fc_setting_8814b(dm, central_ch);
 if (is_2g_ch) {
                                                     ;
  phydm_cck_tx_shaping_filter_8814b(dm, central_ch);
  phydm_dis_cck_trx_8814b(dm, PHYDM_REVERT);
  odm_set_mac_reg(dm, R_0x454, BIT(7), 0x0);
  odm_set_bb_reg(dm, R_0x1a80, BIT(18), 0x0);
  odm_set_bb_reg(dm, R_0x1c80, 0x3F000000, 0xF);
  for (i = RF_PATH_A; i <= RF_PATH_D; i++) {
   if (*dm->band_width == CHANNEL_WIDTH_20)
    odm_set_rf_reg(dm, i, RF_0x5d, 0xf0000, 0x8);
   else if (*dm->band_width == CHANNEL_WIDTH_40)
    odm_set_rf_reg(dm, i, RF_0x5d, 0xf0000, 0xa);
  }
 } else {
                                                   ;
  odm_set_bb_reg(dm, R_0x1a80, BIT(18), 0x1);
  odm_set_mac_reg(dm, R_0x454, BIT(7), 0x1);
  phydm_dis_cck_trx_8814b(dm, PHYDM_SET);
  odm_set_bb_reg(dm, R_0x1c80, 0x3F000000, 0x22);
 }
 rf_18 &= ~0x703ff;
 rf_18 |= central_ch;
 if (!is_2g_ch) {
  rf_18 |= (BIT(16) | BIT(8));
  if (central_ch > 144)
   rf_18 |= BIT(18);
  else if (central_ch >= 80)
   rf_18 |= BIT(17);
 } else {
  if (bw_8814b == CHANNEL_WIDTH_40) {
   rf_18 = rf_18 & (~(BIT(14) | BIT(13) | BIT(12)));
   if (central_ch > 3 && central_ch < 9)
    rf_18 = rf_18 | BIT(12);
  }
 }
 config_phydm_write_rf_syn_8814b(dm, RF_SYN0, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_A, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_B, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_C, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_D, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 rf_18 = 0;
 rf_18 = config_phydm_read_syn_reg_8814b(dm, RF_SYN0, RF_0x18,
      RFREGOFFSETMASK);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_A, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_B, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_C, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 config_phydm_write_rf_reg_8814b(dm, RF_PATH_D, RF_0x18,
     RFREGOFFSETMASK, rf_18);
 phydm_igi_toggle_8814b(dm);
 if (!phydm_rfe_8814b(dm, central_ch))
  return false;
 if (*dm->mp_mode) {
  dm->is_psd_in_process = 1;
  phydm_dynamic_spur_det_eliminate_8814b(dm);
  dm->is_psd_in_process = 0;
 }
 phydm_dyn_dpd_8814b(dm, central_ch);
 phydm_bb_reset_8814b(dm);
 phydm_ccapar_by_rfe_8814b(dm);
               ;
 return true;
}

static boolean
config_phydm_trx_mode_8814b(struct dm_struct *dm, enum bb_path tx_path,
       enum bb_path rx_path)
{
 u32 test = 0;
                                                        ;
                                                                        ;
 if (dm->is_disable_phy_api) {
                                                    ;
  return true;
 }
 if (((tx_path & ~BB_PATH_ABCD) != 0) ||
     ((rx_path & ~BB_PATH_ABCD) != 0)) {
                                                       ;
  return false;
 }
 dm->tx_ant_status = (tx_path & 0xf);
 dm->rx_ant_status = (rx_path & 0xf);
 if ((tx_path | rx_path) & BB_PATH_A)
  odm_set_bb_reg(dm, 0x1800, MASK20BITS, 0x33312);
 else
  odm_set_bb_reg(dm, 0x1800, MASK20BITS, 0x11111);
 if ((tx_path | rx_path) & BB_PATH_B)
  odm_set_bb_reg(dm, 0x4100, MASK20BITS, 0x33312);
 else
  odm_set_bb_reg(dm, 0x4100, MASK20BITS, 0x11111);
 if ((tx_path | rx_path) & BB_PATH_C)
  odm_set_bb_reg(dm, 0x5200, MASK20BITS, 0x33312);
 else
  odm_set_bb_reg(dm, 0x5200, MASK20BITS, 0x11111);
 if ((tx_path | rx_path) & BB_PATH_D)
  odm_set_bb_reg(dm, 0x5300, MASK20BITS, 0x33312);
 else
  odm_set_bb_reg(dm, 0x5300, MASK20BITS, 0x11111);
 if (*dm->mp_mode)
  test = 0;
 else
  config_phydm_write_rf_syn_8814b(dm, RF_SYN1, RF_0x0, 0xf0000,
      0x0);
 odm_set_bb_reg(dm, R_0x9b0, BIT4 | BIT5, 0x0);
 phydm_config_cck_tx_path_8814b(dm, tx_path);
 phydm_config_ofdm_tx_path_8814b(dm, tx_path);
 odm_set_bb_reg(dm, R_0x1c28, 0xf, tx_path);
 phydm_config_cck_rx_path_8814b(dm, rx_path);
 phydm_config_ofdm_rx_path_8814b(dm, rx_path);
 if (rx_path == BB_PATH_A || rx_path == BB_PATH_B ||
     rx_path == BB_PATH_C || rx_path == BB_PATH_D) {
  odm_set_bb_reg(dm, R_0xc44, BIT(17), 0x0);
  odm_set_bb_reg(dm, R_0xc54, BIT(20), 0x0);
  odm_set_bb_reg(dm, R_0xc38, BIT(24), 0x0);
 } else {
  odm_set_bb_reg(dm, R_0xc44, BIT(17), 0x1);
  odm_set_bb_reg(dm, R_0xc54, BIT(20), 0x1);
  odm_set_bb_reg(dm, R_0xc38, BIT(24), 0x1);
 }
 phydm_igi_toggle_8814b(dm);
 phydm_ccapar_by_rfe_8814b(dm);
                     ;
 return true;
}

static boolean
config_phydm_write_rf_reg_8814b(struct dm_struct *dm, enum rf_path path,
    u32 reg_addr, u32 bit_mask, u32 data)
{
 u32 data_and_addr = 0, data_original = 0;
 u32 offset_write_rf[4] = {0x1808, 0x4108, 0x5208, 0x5308};
 if (path > RF_PATH_D) {
                                                   ;
  return false;
 }
 reg_addr &= 0xff;
 bit_mask = bit_mask & RFREGOFFSETMASK;
 if (bit_mask != RFREGOFFSETMASK) {
  data_original = config_phydm_read_rf_reg_8814b(dm, path,
              reg_addr,
              RFREGOFFSETMASK);
  if (!(data_original != INVALID_RF_DATA)) {
                                                  ;
   return false;
  }
  data = phydm_check_bit_mask_8814b(bit_mask, data_original,
        data);
 }
 data_and_addr = ((reg_addr << 20) | (data & 0x000fffff)) & 0x0fffffff;
 odm_set_bb_reg(dm, offset_write_rf[path], MASKDWORD, data_and_addr);
                                                            ;
 return true;
}

static u32 config_phydm_write_rf_syn_8814b(struct dm_struct *dm,
        enum rf_syn syn_path, u32 reg_addr,
        u32 bit_mask, u32 data)
{
 u32 direct_addr;
 u32 offset_read_rf[2] = {0x3b00, 0x4b00};
 if (syn_path > RF_SYN1) {
                        ;
  return false;
 }
 reg_addr &= 0xff;
 direct_addr = offset_read_rf[syn_path] + (reg_addr << 2);
 bit_mask &= RFREGOFFSETMASK;
 odm_set_bb_reg(dm, direct_addr, bit_mask, data);
                                       ;
 return true;
}

static boolean
config_phydm_write_txagc_diff_8814b(struct dm_struct *dm, s8 power_index1,
        s8 power_index2, s8 power_index3,
        s8 power_index4, u8 hw_rate)
{
 u32 offset_txagc = 0x3a00;
 u8 rate_idx = (hw_rate & 0xfc);
 u8 power_idx1 = 0;
 u8 power_idx2 = 0;
 u8 power_idx3 = 0;
 u8 power_idx4 = 0;
 u32 pw_all = 0;
 power_idx1 = power_index1 & 0x7f;
 power_idx2 = power_index2 & 0x7f;
 power_idx3 = power_index3 & 0x7f;
 power_idx4 = power_index4 & 0x7f;
 pw_all = power_idx1 | (power_idx2 << 8) | (power_idx3 << 16) |
       (power_idx4 << 24);
 odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);
 if (dm->is_disable_phy_api) {
                                                     ;
  return true;
 }
 if (hw_rate > 0x53) {
                                          ;
  return false;
 }
 if (power_idx1 > 0x7f || power_idx2 > 0x7f ||
     power_idx3 > 0x7f || power_idx4 > 0x7f) {
                                           ;
  return false;
 }
 odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKDWORD, pw_all);
                                              ;
 return true;
}

static boolean
config_phydm_write_txagc_ref_8814b(struct dm_struct *dm, u8 power_index,
       enum rf_path path,
       enum PDM_RATE_TYPE mod_type)
{
 u32 txagc_ofdm_ref[4] = {0x18e8, 0x41e8, 0x52e8, 0x53e8};
 u32 txagc_cck_ref[4] = {0x18a0, 0x41a0, 0x52a0, 0x53a0};
 u8 pw_idx = power_index & 0x7f;
 odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);
 if (dm->is_disable_phy_api) {
                                                     ;
  return true;
 }
 if (path > RF_PATH_D) {
                                                     ;
  return false;
 }
 if (power_index > 0x7f) {
                                           ;
  return false;
 }
 if (mod_type == PDM_CCK)
  odm_set_bb_reg(dm, txagc_cck_ref[path], 0x7F0000, pw_idx);
 else if (mod_type == PDM_OFDM)
  odm_set_bb_reg(dm, txagc_ofdm_ref[path], 0x1FC00, pw_idx);
                                                      ;
 return true;
}

static void
phydm_agc_tab_sel_8814b(struct dm_struct *dm, u8 central_ch)
{
 struct phydm_dig_struct *dig_tab = &dm->dm_dig_table;
                                                       ;
 if (central_ch <= 14) {
  odm_set_bb_reg(dm, R_0x18ac, 0x1f0, 0x0);
  odm_set_bb_reg(dm, R_0x41ac, 0x1f0, 0x0);
  odm_set_bb_reg(dm, R_0x52ac, 0x1f0, 0x0);
  odm_set_bb_reg(dm, R_0x53ac, 0x1f0, 0x0);
  dig_tab->agc_table_idx = 0x0;
 } else if (central_ch >= 36 && central_ch <= 64) {
  odm_set_bb_reg(dm, R_0x18ac, 0x1f0, 0x1);
  odm_set_bb_reg(dm, R_0x41ac, 0x1f0, 0x1);
  odm_set_bb_reg(dm, R_0x52ac, 0x1f0, 0x1);
  odm_set_bb_reg(dm, R_0x53ac, 0x1f0, 0x1);
  dig_tab->agc_table_idx = 0x1;
 } else if ((central_ch >= 100) && (central_ch <= 144)) {
  odm_set_bb_reg(dm, R_0x18ac, 0x1f0, 0x2);
  odm_set_bb_reg(dm, R_0x41ac, 0x1f0, 0x2);
  odm_set_bb_reg(dm, R_0x52ac, 0x1f0, 0x2);
  odm_set_bb_reg(dm, R_0x53ac, 0x1f0, 0x2);
  dig_tab->agc_table_idx = 0x2;
 } else if (central_ch >= 149) {
  odm_set_bb_reg(dm, R_0x18ac, 0x1f0, 0x3);
  odm_set_bb_reg(dm, R_0x41ac, 0x1f0, 0x3);
  odm_set_bb_reg(dm, R_0x52ac, 0x1f0, 0x3);
  odm_set_bb_reg(dm, R_0x53ac, 0x1f0, 0x3);
  dig_tab->agc_table_idx = 0x3;
 } else {
                                               ;
 }
}

static void phydm_bb_reset_8814b(struct dm_struct *dm)
{
 odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
 odm_set_mac_reg(dm, R_0x0, BIT(16), 0);
 odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
}

static void phydm_ccapar_by_rfe_8814b(struct dm_struct *dm)
{
}

static void
phydm_cck_tx_shaping_filter_8814b(struct dm_struct *dm, u8 central_ch)
{
 if (central_ch == 14) {
  odm_set_bb_reg(dm, R_0x1a24, MASKDWORD, 0xb81c);
  odm_set_bb_reg(dm, R_0x1a28, MASKLWORD, 0x0);
  odm_set_bb_reg(dm, R_0x1aac, MASKDWORD, 0x3667);
 } else {
  odm_set_bb_reg(dm, R_0x1a24, MASKDWORD, 0x64b80c1c);
  odm_set_bb_reg(dm, R_0x1a28, MASKLWORD,
          (0x8810 & MASKLWORD));
  odm_set_bb_reg(dm, R_0x1aac, MASKDWORD, 0x1235667);
 }
}

static u32 phydm_check_bit_mask_8814b(u32 bit_mask, u32 data_original, u32 data)
{
 u8 bit_shift;
 if (bit_mask != 0xfffff) {
  for (bit_shift = 0; bit_shift <= 19; bit_shift++) {
   if (((bit_mask >> bit_shift) & 0x1) == 1)
    break;
  }
  return ((data_original) & (~bit_mask)) | (data << bit_shift);
 }
 return data;
}

static boolean
phydm_config_cck_rx_path_8814b(struct dm_struct *dm, enum bb_path rx_path)
{
 boolean set_result = PHYDM_SET_FAIL;
 odm_set_bb_reg(dm, R_0x1ac0, 0x80000000, 0x0);
 if (rx_path == BB_PATH_A || rx_path == BB_PATH_B ||
     rx_path == BB_PATH_C || rx_path == BB_PATH_D) {
  odm_set_bb_reg(dm, R_0x1a2c, 0x00000020, 0x1);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x0);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x0);
 } else if (rx_path == BB_PATH_AB) {
  odm_set_bb_reg(dm, R_0x1a2c, 0x00000020, 0x0);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x1);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x1);
 } else if (rx_path == BB_PATH_ABC) {
  odm_set_bb_reg(dm, R_0x1a2c, 0x00000020, 0x0);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x2);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x2);
 } else if (rx_path == BB_PATH_ABCD || rx_path == BB_PATH_AC ||
     rx_path == BB_PATH_AD || rx_path == BB_PATH_BC ||
     rx_path == BB_PATH_BD || rx_path == BB_PATH_CD ||
     rx_path == BB_PATH_ABD || rx_path == BB_PATH_ACD ||
     rx_path == BB_PATH_BCD) {
  odm_set_bb_reg(dm, R_0x1a2c, 0x00000020, 0x0);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x3);
  odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x3);
 }
 odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
 odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x0);
 if (rx_path == BB_PATH_A) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x0);
 } else if (rx_path == BB_PATH_B) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x5);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x5);
 } else if (rx_path == BB_PATH_C) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0xa);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xa);
 } else if (rx_path == BB_PATH_D) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0xf);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xf);
 } else if (rx_path == BB_PATH_AB) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1);
 } else if (rx_path == BB_PATH_AC) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xa);
 } else if (rx_path == BB_PATH_AD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xf);
 } else if (rx_path == BB_PATH_BC) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x5);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xa);
 } else if (rx_path == BB_PATH_BD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x5);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xf);
 } else if (rx_path == BB_PATH_CD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0xb);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xb);
 } else if (rx_path == BB_PATH_ABC) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0x8);
 } else if (rx_path == BB_PATH_ABD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xf);
 } else if (rx_path == BB_PATH_ACD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xb);
 } else if (rx_path == BB_PATH_BCD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x5);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xb);
 } else if (rx_path == BB_PATH_ABCD) {
  odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1);
  odm_set_bb_reg(dm, R_0x1a84, 0x0f000000, 0xb);
 }
 set_result = PHYDM_SET_SUCCESS;
 return set_result;
}

static boolean
phydm_config_cck_tx_path_8814b(struct dm_struct *dm, enum bb_path tx_path)
{
 boolean set_result = PHYDM_SET_FAIL;
 odm_set_bb_reg(dm, R_0x1e5c, BIT(30), 0x1);
 if (tx_path == BB_PATH_A)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x8);
 else if (tx_path == BB_PATH_B)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x4);
 else if (tx_path == BB_PATH_C)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x2);
 else if (tx_path == BB_PATH_D)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x1);
 if (tx_path == BB_PATH_AB)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xc);
 else if (tx_path == BB_PATH_AC)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xa);
 else if (tx_path == BB_PATH_AD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x9);
 else if (tx_path == BB_PATH_BC)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x6);
 else if (tx_path == BB_PATH_BD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x5);
 else if (tx_path == BB_PATH_CD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x3);
 if (tx_path == BB_PATH_ABC)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xe);
 else if (tx_path == BB_PATH_ABD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xd);
 else if (tx_path == BB_PATH_ACD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xb);
 else if (tx_path == BB_PATH_BCD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x7);
 if (tx_path == BB_PATH_ABCD)
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xf);
 set_result = PHYDM_SET_SUCCESS;
 return set_result;
}

static boolean
phydm_config_ofdm_rx_path_8814b(struct dm_struct *dm,
    enum bb_path rx_path)
{
 u8 rfe_type = dm->rfe_type;
 boolean is_channel_2g = (*dm->channel <= 14) ? true : false;
 boolean set_result = PHYDM_SET_SUCCESS;
 odm_set_bb_reg(dm, R_0x824, MASKBYTE3LOWNIBBLE, rx_path);
 odm_set_bb_reg(dm, R_0x824, 0xF0000, rx_path);
 dm->rx_ant_status = rx_path;
 config_phydm_path_adda_setting_8814b(dm);
 if (!(rx_path & BB_PATH_A))
  config_phydm_write_rf_reg_8814b(dm, 0, RF_0x0, 0xf0000, 0x1);
 if (!(rx_path & BB_PATH_B))
  config_phydm_write_rf_reg_8814b(dm, 1, RF_0x0, 0xf0000, 0x1);
 if (!(rx_path & BB_PATH_C))
  config_phydm_write_rf_reg_8814b(dm, 2, RF_0x0, 0xf0000, 0x1);
 if (!(rx_path & BB_PATH_D))
  config_phydm_write_rf_reg_8814b(dm, 3, RF_0x0, 0xf0000, 0x1);
 if (rx_path == BB_PATH_A || rx_path == BB_PATH_B ||
     rx_path == BB_PATH_C || rx_path == BB_PATH_D) {
  odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x0);
  odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x0);
  odm_set_bb_reg(dm, R_0x884, 0x1c000, 0x5);
 } else if (rx_path == BB_PATH_AB || rx_path == BB_PATH_AC ||
     rx_path == BB_PATH_AD || rx_path == BB_PATH_BC ||
     rx_path == BB_PATH_BD || rx_path == BB_PATH_CD) {
  odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x1);
  odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x1);
  odm_set_bb_reg(dm, R_0x884, 0x1c000, 0x5);
 } else if (rx_path == BB_PATH_ABC || rx_path == BB_PATH_ABD ||
     rx_path == BB_PATH_ACD || rx_path == BB_PATH_BCD) {
  odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x2);
  odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x2);
  odm_set_bb_reg(dm, R_0x884, 0x1c000, 0x4);
 } else if (rx_path == BB_PATH_ABCD) {
  odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x3);
  odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x3);
  odm_set_bb_reg(dm, R_0x884, 0x1c000, 0x4);
 }
 if (*dm->mp_mode)
  phydm_dynamic_spur_det_eliminate_8814b(dm);
 if (rfe_type == 6) {
  if (is_channel_2g) {
                                               ;
   if (rx_path == BB_PATH_A || rx_path == BB_PATH_AD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f2a0500);
   else if (rx_path == BB_PATH_B || rx_path == BB_PATH_D ||
     rx_path == BB_PATH_AB ||
     rx_path == BB_PATH_BD ||
     rx_path == BB_PATH_CD ||
     rx_path == BB_PATH_BCD ||
     rx_path == BB_PATH_ACD ||
     rx_path == BB_PATH_ABD ||
     rx_path == BB_PATH_ABCD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f2a1500);
   else if (rx_path == BB_PATH_C ||
     rx_path == BB_PATH_BC ||
     rx_path == BB_PATH_ABC)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x2f2a1500);
   else if (rx_path == BB_PATH_AC)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x2f2a1100);
  } else {
                                             ;
   if (rx_path == BB_PATH_A || rx_path == BB_PATH_AB ||
       rx_path == BB_PATH_AC || rx_path == BB_PATH_ABC ||
       rx_path == BB_PATH_ABD || rx_path == BB_PATH_ACD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f2a1500);
   else if (rx_path == BB_PATH_B ||
     rx_path == BB_PATH_BC ||
     rx_path == BB_PATH_BCD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f2a1510);
   else if (rx_path == BB_PATH_C || rx_path == BB_PATH_CD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f2a1500);
   else if (rx_path == BB_PATH_D || rx_path == BB_PATH_BD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f3a1510);
   else if (rx_path == BB_PATH_AD)
    odm_set_bb_reg(dm, R_0x1c98, MASKDWORD,
            0x3f3a1500);
  }
 }
 return set_result;
}

static boolean
phydm_config_ofdm_tx_path_8814b(struct dm_struct *dm,
    enum bb_path tx_path)
{
 boolean set_result = PHYDM_SET_SUCCESS;
 odm_set_bb_reg(dm, R_0x1e28, 0xf, tx_path);
 dm->tx_ant_status = tx_path;
 config_phydm_path_adda_setting_8814b(dm);
 if (tx_path == BB_PATH_A) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x1);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x0);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x1);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x0);
 } else if (tx_path == BB_PATH_B) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x2);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x0);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x2);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x0);
 } else if (tx_path == BB_PATH_C) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x4);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x0);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x4);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x0);
 } else if (tx_path == BB_PATH_D) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x8);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x0);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x8);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x0);
 } else if (tx_path == BB_PATH_AB) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x3);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x4);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x3);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x4);
 } else if (tx_path == BB_PATH_AC) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x5);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x10);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x5);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x10);
 } else if (tx_path == BB_PATH_AD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x9);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x40);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x9);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x40);
 } else if (tx_path == BB_PATH_BC) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x6);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x10);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x6);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x10);
 } else if (tx_path == BB_PATH_BD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xa);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x40);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0xa);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x40);
 } else if (tx_path == BB_PATH_CD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xc);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x40);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0xc);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x40);
 } else if (tx_path == BB_PATH_ABC) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0x7);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x24);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0x7);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x24);
 } else if (tx_path == BB_PATH_ABD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xb);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x84);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0xb);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x84);
 } else if (tx_path == BB_PATH_ACD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xd);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x90);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0xd);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x90);
 } else if (tx_path == BB_PATH_BCD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xe);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0x90);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0xe);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0x90);
 } else if (tx_path == BB_PATH_ABCD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000000f, 0xf);
  odm_set_bb_reg(dm, R_0x1e2c, 0x000000ff, 0xe4);
  odm_set_bb_reg(dm, R_0x820, 0x000f0000, 0xf);
  odm_set_bb_reg(dm, R_0x1e30, 0x000000ff, 0xe4);
 }
 if (tx_path == BB_PATH_AB) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x3);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x4);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0x3);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x4);
 } else if (tx_path == BB_PATH_AC) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x5);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x10);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0x5);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x10);
 } else if (tx_path == BB_PATH_AD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x9);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x40);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0x9);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x40);
 } else if (tx_path == BB_PATH_BC) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x6);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x10);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0x6);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x10);
 } else if (tx_path == BB_PATH_BD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xa);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x40);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0xa);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x40);
 } else if (tx_path == BB_PATH_CD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xc);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x40);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0xc);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x40);
 } else if (tx_path == BB_PATH_ABC) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0x7);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x24);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0x7);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x24);
 } else if (tx_path == BB_PATH_ABD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xb);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x84);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0xb);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x84);
 } else if (tx_path == BB_PATH_ACD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xd);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x90);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0xd);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x90);
 } else if (tx_path == BB_PATH_BCD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xe);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0x90);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0xe);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0x90);
 } else if (tx_path == BB_PATH_ABCD) {
  odm_set_bb_reg(dm, R_0x820, 0x000000f0, 0xf);
  odm_set_bb_reg(dm, R_0x1e2c, 0x0000ff00, 0xe4);
  odm_set_bb_reg(dm, R_0x820, 0x00f00000, 0xf);
  odm_set_bb_reg(dm, R_0x1e30, 0x0000ff00, 0xe4);
 }
 if (tx_path == BB_PATH_ABC) {
  odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0x7);
  odm_set_bb_reg(dm, R_0x1e2c, 0x00ff0000, 0x24);
  odm_set_bb_reg(dm, R_0x820, 0x0f000000, 0x7);
  odm_set_bb_reg(dm, R_0x1e30, 0x00ff0000, 0x24);
 } else if (tx_path == BB_PATH_ABD) {
  odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xb);
  odm_set_bb_reg(dm, R_0x1e2c, 0x00ff0000, 0x84);
  odm_set_bb_reg(dm, R_0x820, 0x0f000000, 0xb);
  odm_set_bb_reg(dm, R_0x1e30, 0x00ff0000, 0x84);
 } else if (tx_path == BB_PATH_ACD) {
  odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xd);
  odm_set_bb_reg(dm, R_0x1e2c, 0x00ff0000, 0x90);
  odm_set_bb_reg(dm, R_0x820, 0x0f000000, 0xd);
  odm_set_bb_reg(dm, R_0x1e30, 0x00ff0000, 0x90);
 } else if (tx_path == BB_PATH_BCD) {
  odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xe);
  odm_set_bb_reg(dm, R_0x1e2c, 0x00ff0000, 0x90);
  odm_set_bb_reg(dm, R_0x820, 0x0f000000, 0xe);
  odm_set_bb_reg(dm, R_0x1e30, 0x00ff0000, 0x90);
 } else if (tx_path == BB_PATH_ABCD) {
  odm_set_bb_reg(dm, R_0x820, 0x00000f00, 0xf);
  odm_set_bb_reg(dm, R_0x1e2c, 0x00ff0000, 0xe4);
  odm_set_bb_reg(dm, R_0x820, 0x0f000000, 0xf);
  odm_set_bb_reg(dm, R_0x1e30, 0x00ff0000, 0xe4);
 }
 if (tx_path == BB_PATH_ABCD) {
  odm_set_bb_reg(dm, R_0x820, 0x0000f000, 0xf);
  odm_set_bb_reg(dm, R_0x1e2c, 0xff000000, 0xe4);
  odm_set_bb_reg(dm, R_0x820, 0xf0000000, 0xf);
  odm_set_bb_reg(dm, R_0x1e30, 0xff000000, 0xe4);
 }
 return set_result;
}

static boolean
phydm_config_set_rfc_8814b(struct dm_struct *dm, enum rfc_mode mode)
{
 boolean set_result = PHYDM_SET_FAIL;
 u8 i;
                                                                    ;
 if (mode == rfc_4x4) {
  for (i = RF_PATH_A; i <= RF_PATH_D; i++) {
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0xef, BIT19,
           0x1);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xf, 1);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e, 0xfffff,
           0x364);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f, 0xfffff,
           0x280f7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xf, 2);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e, 0xfffff,
           0x3fc);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f, 0xfffff,
           0x280f7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xf, 3);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e, 0xfffff,
           0x265);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f, 0xfffff,
           0xafcf7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 4);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003F6);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x0F9F7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 5);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003FC);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xBF8FF);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003FD);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xEFCF7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0xef, BIT(19),
           0x0);
  }
  set_result = PHYDM_SET_SUCCESS;
 } else if (mode == rfc_2x2) {
  for (i = RF_PATH_A; i <= RF_PATH_D; i += 3) {
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0xef,
           BIT19, 0x1);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 1);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x00364);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x280f7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 2);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003fc);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x280f7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 3);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x00265);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xafcf7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 4);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003F6);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x0F9F7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 5);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003FC);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xBF8FF);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003FD);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xEFCF7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0xef,
           BIT(19), 0x0);
   }
  for (i = RF_PATH_B; i <= RF_PATH_C; i++) {
                  ;
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0xef,
           BIT19, 0x1);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 1);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x00364);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x280c7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 2);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003fc);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x280c7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 3);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x00365);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xafcc7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 4);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003F6);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0x0F9C7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 5);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003FC);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xBF8CF);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x33, 0xF, 7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3e,
           0xfffff, 0x003FD);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0x3f,
           0xfffff, 0xEFCC7);
   odm_set_rf_reg(dm, (enum rf_path)i, RF_0xef,
           BIT(19), 0x0);
  }
  set_result = PHYDM_SET_SUCCESS;
 } else {
  set_result = PHYDM_SET_FAIL;
 }
 return set_result;
}

static void phydm_dis_cck_trx_8814b(void *dm_void, u8 set_type)
{
 struct dm_struct *dm = (struct dm_struct *)dm_void;
 if (set_type == PHYDM_SET) {
  odm_set_bb_reg(dm, R_0x1a14, 0x300, 0x3);
  odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x0);
 } else if (set_type == PHYDM_REVERT) {
  odm_set_bb_reg(dm, R_0x1a14, 0x300, 0x0);
  phydm_config_cck_tx_path_8814b(dm,
            (enum bb_path)dm->tx_ant_status);
 }
}

static void phydm_dyn_dpd_8814b(struct dm_struct *dm, u8 central_ch)
{
 u8 rfe_type = dm->rfe_type;
 u8 disable_6m = 0;
 if (central_ch == 140)
  if (rfe_type == 0 || rfe_type == 2 || rfe_type == 3 ||
      rfe_type == 7 || rfe_type == 8 || rfe_type == 9)
   disable_6m = 1;
 odm_set_bb_reg(dm, R_0xa70, BIT(0), disable_6m);
}

static void phydm_dynamic_spur_det_eliminate_8814b(struct dm_struct *dm)
{
}

static void phydm_igi_toggle_8814b(struct dm_struct *dm)
{
 u32 igi = 0x20;
 igi = odm_get_bb_reg(dm, R_0x1d70, 0x7f);
 odm_set_bb_reg(dm, R_0x1d70, 0x7f, (igi - 2));
 odm_set_bb_reg(dm, R_0x1d70, 0x7f, igi);
 odm_set_bb_reg(dm, R_0x1d70, 0x7f00, (igi - 2));
 odm_set_bb_reg(dm, R_0x1d70, 0x7f00, igi);
 odm_set_bb_reg(dm, R_0x1d70, 0x7f0000, (igi - 2));
 odm_set_bb_reg(dm, R_0x1d70, 0x7f0000, igi);
 odm_set_bb_reg(dm, R_0x1d70, 0x7f000000, (igi - 2));
 odm_set_bb_reg(dm, R_0x1d70, 0x7f000000, igi);
}

static boolean
phydm_rfe_8814b(struct dm_struct *dm, u8 channel)
{
 boolean is_channel_2g = (channel <= 14) ? true : false;
 u8 rfe_type = dm->rfe_type;
                              ;
 if ((channel > 14 && channel < 36) || channel == 0)
  return false;
 dm->rfe_hwsetting_band = (is_channel_2g) ? 1 : 2;
 if (rfe_type == 1)
  phydm_rfe_8814b_efem(dm, channel);
 else if (rfe_type == 3)
  phydm_rfe_8814b_ifem(dm, channel);
 else if (rfe_type == 6)
  phydm_rfe_8814b_efem(dm, channel);
 return true;
}

static void phydm_rfe_8814b_efem(struct dm_struct *dm, u8 channel)
{
 u8 rfe_type = dm->rfe_type;
                                    ;
 if (rfe_type == 1) {
 } else if (rfe_type == 6) {
  phydm_rfe_type_6(dm, channel);
 } else {
  odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x77027770);
  odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x02777707);
  odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777702);
  odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x02777707);
  odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777702);
  odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x02777707);
  odm_set_bb_reg(dm, R_0x1c98, MASKDWORD, 0x00fa50e4);
 }
}

static void phydm_rfe_8814b_ifem(struct dm_struct *dm, u8 channel)
{
 boolean is_channel_2g = (channel <= 14) ? true : false;
 u8 rfe_type = dm->rfe_type;
                                    ;
 if (rfe_type == 3) {
  if (is_channel_2g) {
   odm_set_bb_reg(dm, R_0x183c, MASKDWORD, 0x40);
   odm_set_bb_reg(dm, R_0x413c, MASKDWORD, 0x1000);
   odm_set_bb_reg(dm, R_0x523c, MASKDWORD, 0x200);
   odm_set_bb_reg(dm, R_0x533c, MASKDWORD, 0x800);
   odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x1844, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x4144, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x77777777);
  } else {
   odm_set_bb_reg(dm, R_0x183c, MASKDWORD, 0x10);
   odm_set_bb_reg(dm, R_0x413c, MASKDWORD, 0x2000);
   odm_set_bb_reg(dm, R_0x523c, MASKDWORD, 0x100);
   odm_set_bb_reg(dm, R_0x533c, MASKDWORD, 0x400);
   odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x1844, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x4144, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777777);
   odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x77777777);
  }
 } else {
  odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x77027770);
  odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x02777707);
  odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777702);
  odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x02777707);
  odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777702);
  odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x02777707);
  odm_set_bb_reg(dm, R_0x1c98, MASKDWORD, 0x00fa50e4);
 }
}

static void phydm_rfe_8814b_init(struct dm_struct *dm)
{
                 ;
 odm_set_bb_reg(dm, R_0x66, BIT(13) | BIT(12), 0x3);
 odm_set_bb_reg(dm, R_0x1c94, 0x3f, 0x32);
 odm_set_bb_reg(dm, R_0x1ca0, MASKDWORD, 0x0);
}

static void phydm_rfe_type_6(struct dm_struct *dm, u8 channel)
{
 boolean is_channel_2g = (channel <= 14) ? true : false;
 u8 rx_path;
 rx_path = dm->rx_ant_status;
 if (is_channel_2g) {
  odm_set_bb_reg(dm, R_0x183c, MASKDWORD, 0x40);
  odm_set_bb_reg(dm, R_0x413c, MASKDWORD, 0x40);
  odm_set_bb_reg(dm, R_0x523c, MASKDWORD, 0x4000);
  odm_set_bb_reg(dm, R_0x533c, MASKDWORD, 0x4000);
  odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x74777720);
  odm_set_bb_reg(dm, R_0x1844, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x74207777);
  odm_set_bb_reg(dm, R_0x4144, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x74777720);
  odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x74207777);
 } else {
  odm_set_bb_reg(dm, R_0x1c94, MASKDWORD, 0xffffffff);
  odm_set_bb_reg(dm, R_0x183c, MASKDWORD, 0x4);
  odm_set_bb_reg(dm, R_0x413c, MASKDWORD, 0x4);
  odm_set_bb_reg(dm, R_0x523c, MASKDWORD, 0x400);
  odm_set_bb_reg(dm, R_0x533c, MASKDWORD, 0x400);
  odm_set_bb_reg(dm, R_0x1840, MASKDWORD, 0x77777420);
  odm_set_bb_reg(dm, R_0x1844, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x4140, MASKDWORD, 0x77207477);
  odm_set_bb_reg(dm, R_0x4144, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x5240, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x5244, MASKDWORD, 0x77777420);
  odm_set_bb_reg(dm, R_0x5340, MASKDWORD, 0x77777777);
  odm_set_bb_reg(dm, R_0x5344, MASKDWORD, 0x77207477);
 }
}

static void
phydm_sco_trk_fc_setting_8814b(struct dm_struct *dm, u8 central_ch)
{
                                                                 ;
 if (central_ch == 13 || central_ch == 14) {
  odm_set_bb_reg(dm, R_0xc30, MASK12BITS, 0x969);
 } else if (central_ch == 11 || central_ch == 12) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x96a);
 } else if (central_ch >= 1 && central_ch <= 10) {
  odm_set_bb_reg(dm, R_0xc30, MASK12BITS, 0x9aa);
 } else if (central_ch >= 36 && central_ch <= 51) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x494);
 } else if (central_ch >= 52 && central_ch <= 55) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x493);
 } else if ((central_ch >= 56) && (central_ch <= 111)) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x453);
 } else if ((central_ch >= 112) && (central_ch <= 119)) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x452);
 } else if ((central_ch >= 120) && (central_ch <= 172)) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x412);
 } else if ((central_ch >= 173) && (central_ch <= 177)) {
  odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x411);
 } else {
                                                  ;
 }
}

static void phydm_set_txagc_by_table_8814b(struct dm_struct *dm,
        struct txagc_table_8814b *tab)
{
 u8 i = 0;
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_cck[0], RF_PATH_A,
        PDM_CCK);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_cck[1], RF_PATH_B,
        PDM_CCK);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_cck[2], RF_PATH_C,
        PDM_CCK);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_cck[3], RF_PATH_D,
        PDM_CCK);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_ofdm[0], RF_PATH_A,
        PDM_OFDM);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_ofdm[1], RF_PATH_B,
        PDM_OFDM);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_ofdm[2], RF_PATH_C,
        PDM_OFDM);
 config_phydm_write_txagc_ref_8814b(dm, tab->ref_pow_ofdm[3], RF_PATH_D,
        PDM_OFDM);
 for (i = ODM_RATE1M; i <= ODM_RATEMCS31; i++) {
  if (i % 4 == 3) {
   config_phydm_write_txagc_diff_8814b(dm,
           tab->diff_t[i - 3],
           tab->diff_t[i - 2],
           tab->diff_t[i - 1],
           tab->diff_t[i],
           i - 3);
  }
 }
 for (i = ODM_RATEVHTSS1MCS0; i <= ODM_RATEVHTSS4MCS9; i++) {
  if (i % 4 == 3) {
   config_phydm_write_txagc_diff_8814b(dm,
           tab->diff_t[i - 3],
           tab->diff_t[i - 2],
           tab->diff_t[i - 1],
           tab->diff_t[i],
           i - 3);
  }
 }
}
