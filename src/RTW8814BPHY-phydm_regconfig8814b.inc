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

static void odm_config_bb_agc_8814b(struct dm_struct *dm, u32 addr, u32 bitmask,
        u32 data)
{
 if (dm->fw_offload_ability & PHYDM_PHY_PARAM_OFFLOAD)
  phydm_set_reg_by_fw(dm,
        PHYDM_HALMAC_CMD_BB_W32,
        addr,
        data,
        bitmask,
        (enum rf_path)0,
        0);
 else
  odm_set_bb_reg(dm, addr, bitmask, data);
               ;
}

static void odm_config_bb_phy_8814b(struct dm_struct *dm, u32 addr, u32 bitmask,
        u32 data)
{
 if (dm->fw_offload_ability & PHYDM_PHY_PARAM_OFFLOAD) {
  u32 delay_time = 0;
  if (addr >= 0xf9 && addr <= 0xfe) {
   if (addr == 0xfe || addr == 0xfb)
    delay_time = 50;
   else if (addr == 0xfd || addr == 0xfa)
    delay_time = 5;
   else
    delay_time = 1;
   if (addr >= 0xfc && addr <= 0xfe)
    phydm_set_reg_by_fw(dm,
          PHYDM_HALMAC_CMD_DELAY_MS,
          addr,
          data,
          bitmask,
          (enum rf_path)0,
          delay_time);
   else
    phydm_set_reg_by_fw(dm,
          PHYDM_HALMAC_CMD_DELAY_US,
          addr,
          data,
          bitmask,
          (enum rf_path)0,
          delay_time);
  } else {
   phydm_set_reg_by_fw(dm,
         PHYDM_HALMAC_CMD_BB_W32,
         addr,
         data,
         bitmask,
         (enum rf_path)0,
         0);
  }
 } else {
  if (addr == 0xfe)
   ODM_delay_ms(50);
  else if (addr == 0xfd)
   ODM_delay_ms(5);
  else if (addr == 0xfc)
   ODM_delay_ms(1);
  else if (addr == 0xfb)
   ODM_delay_us(50);
  else if (addr == 0xfa)
   ODM_delay_us(5);
  else if (addr == 0xf9)
   ODM_delay_us(1);
  else
   odm_set_bb_reg(dm, addr, bitmask, data);
 }
               ;
}

static void odm_config_rf_radio_a_8814b(struct dm_struct *dm, u32 addr, u32 data)
{
 u32 content = 0x1000;
 u32 maskfor_phy_set = (u32)(content & 0xE000);
 odm_config_rf_reg_8814b(dm, addr, data, RF_PATH_A,
    addr | maskfor_phy_set);
               ;
}

static void odm_config_rf_radio_b_8814b(struct dm_struct *dm, u32 addr, u32 data)
{
 u32 content = 0x1001;
 u32 maskfor_phy_set = (u32)(content & 0xE000);
 odm_config_rf_reg_8814b(dm, addr, data, RF_PATH_B,
    addr | maskfor_phy_set);
               ;
}

static void odm_config_rf_radio_c_8814b(struct dm_struct *dm, u32 addr, u32 data)
{
 u32 content = 0x1001;
 u32 maskfor_phy_set = (u32)(content & 0xE000);
 odm_config_rf_reg_8814b(dm, addr, data, RF_PATH_C,
    addr | maskfor_phy_set);
               ;
}

static void odm_config_rf_radio_d_8814b(struct dm_struct *dm, u32 addr, u32 data)
{
 u32 content = 0x1001;
 u32 maskfor_phy_set = (u32)(content & 0xE000);
 odm_config_rf_reg_8814b(dm, addr, data, RF_PATH_D,
    addr | maskfor_phy_set);
               ;
}

static void odm_config_rf_radio_syn0_8814b(struct dm_struct *dm, u32 addr, u32 data)
{
 u32 content = 0x1000;
 u32 maskfor_phy_set = (u32)(content & 0xE000);
 odm_config_syn_reg_8814b(dm, addr, data, RF_SYN0,
     addr | maskfor_phy_set);
               ;
}

static void odm_config_rf_radio_syn1_8814b(struct dm_struct *dm, u32 addr, u32 data)
{
 u32 content = 0x1001;
 u32 maskfor_phy_set = (u32)(content & 0xE000);
 odm_config_syn_reg_8814b(dm, addr, data, RF_SYN1,
     addr | maskfor_phy_set);
               ;
}

static void odm_config_rf_reg_8814b(struct dm_struct *dm, u32 addr, u32 data,
        enum rf_path rf_path, u32 reg_addr)
{
 if (dm->fw_offload_ability & PHYDM_PHY_PARAM_OFFLOAD) {
  if (addr == 0xffe) {
   phydm_set_reg_by_fw(dm,
         PHYDM_HALMAC_CMD_DELAY_MS,
         reg_addr,
         data,
         RFREGOFFSETMASK,
         rf_path,
         50);
  } else if (addr == 0xfe) {
   phydm_set_reg_by_fw(dm,
         PHYDM_HALMAC_CMD_DELAY_US,
         reg_addr,
         data,
         RFREGOFFSETMASK,
         rf_path,
         100);
  } else {
   phydm_set_reg_by_fw(dm,
         PHYDM_HALMAC_CMD_RF_W,
         reg_addr,
         data,
         RFREGOFFSETMASK,
         rf_path,
         0);
   phydm_set_reg_by_fw(dm,
         PHYDM_HALMAC_CMD_DELAY_US,
         reg_addr,
         data,
         RFREGOFFSETMASK,
         rf_path,
         1);
  }
 } else {
  if (addr == 0xffe) {
   ODM_delay_ms(50);
  } else if (addr == 0xfe) {
   ODM_delay_us(100);
  } else if (addr == 0xffff) {
   ODM_delay_us(1);
  } else {
   odm_set_rf_reg(dm, rf_path, reg_addr, RFREGOFFSETMASK,
           data);
   ODM_delay_us(1);
  }
 }
}

static void odm_config_syn_reg_8814b(struct dm_struct *dm, u32 addr, u32 data,
         enum rf_syn syn_path, u32 reg_addr)
{
 if (dm->fw_offload_ability & PHYDM_PHY_PARAM_OFFLOAD) {
                          ;
 } else {
  if (addr == 0xffe) {
   ODM_delay_ms(50);
  } else if (addr == 0xfe) {
   ODM_delay_us(100);
  } else if (addr == 0xffff) {
   ODM_delay_us(1);
  } else {
   config_phydm_write_rf_syn_8814b(dm, syn_path, reg_addr,
       MASKDWORD, data);
   ODM_delay_us(1);
  }
 }
}
