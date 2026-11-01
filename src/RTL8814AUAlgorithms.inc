/* SPDX-License-Identifier: BSD-3-Clause */
/* Adapted from Linux rtw88 0e135910deec43ffe97eca0b6af1f8f847285b59,
 * GPL-2.0 OR BSD-3-Clause. Copyright(c)2018-2019, 2025 Realtek Corporation.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the copyright, conditions and
 * disclaimer in RTL8814AUPHY-LICENSE.txt are retained. */
#define PHY14_TXGI_FACTOR 1
static void phy14_init_rfe_reg(struct rtw_8814au_phy *rtwdev)
{
	u8 rfe_option = rtwdev->efuse.rfe_option;

	if (rfe_option == 2 || rfe_option == 1) {
		rtw_write32_mask(rtwdev, 0x1994, 0xf, 0xf);
		rtw_write8_set(rtwdev, REG_GPIO_MUXCFG + 2, 0xf0);
	} else if (rfe_option == 0) {
		rtw_write32_mask(rtwdev, 0x1994, 0xf, 0xf);
		rtw_write8_set(rtwdev, REG_GPIO_MUXCFG + 2, 0xc0);
	}
}

static u32 phy14_get_bb_swing(struct rtw_8814au_phy *rtwdev, u8 band, u8 rf_path)
{
	static const u32 swing2setting[4] = {0x200, 0x16a, 0x101, 0x0b6};
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 tx_bb_swing;

	if (band == RTW_BAND_2G)
		tx_bb_swing = efuse->tx_bb_swing_setting_2g;
	else
		tx_bb_swing = efuse->tx_bb_swing_setting_5g;

	tx_bb_swing >>= 2 * rf_path;
	tx_bb_swing &= 0x3;

	return swing2setting[tx_bb_swing];
}

static u8 phy14_get_swing_index(struct rtw_8814au_phy *rtwdev)
{
	u32 swing, table_value;
	u8 i;

	swing = phy14_get_bb_swing(rtwdev, rtwdev->hal.current_band_type,
				      RF_PATH_A);

	for (i = 0; i < ARRAY_SIZE(phy14_txscale_tbl); i++) {
		table_value = phy14_txscale_tbl[i];
		if (swing == table_value)
			return i;
	}

	return 24;
}

static void phy14_pwrtrack_init(struct rtw_8814au_phy *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 path;

	dm_info->default_ofdm_index = phy14_get_swing_index(rtwdev);

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		ewma_thermal_init(&dm_info->avg_thermal[path]);
		dm_info->delta_power_index[path] = 0;
		dm_info->delta_power_index_last[path] = 0;
	}
	dm_info->pwr_trk_triggered = false;
	dm_info->pwr_trk_init_trigger = true;
	dm_info->thermal_meter_k = rtwdev->efuse.thermal_meter_k;
}

static void phy14_config_trx_path(struct rtw_8814au_phy *rtwdev)
{
	/* RX CCK disable 2R CCA */
	rtw_write32_clr(rtwdev, REG_CCK0_FAREPORT,
			BIT_CCK0_2RX | BIT_CCK0_MRC);
	/* pathB tx on, path A/C/D tx off */
	rtw_write32_mask(rtwdev, REG_CCK_RX, 0xf0000000, 0x4);
	/* pathB rx */
	rtw_write32_mask(rtwdev, REG_CCK_RX, 0x0f000000, 0x5);
}

static void phy14_config_cck_rx_antenna_init(struct rtw_8814au_phy *rtwdev)
{
	/* CCK 2R CCA parameters */

	/* Disable Ant diversity */
	rtw_write32_mask(rtwdev, REG_RXSB, BIT_RXSB_ANA_DIV, 0x0);
	/* Concurrent CCA at LSB & USB */
	rtw_write32_mask(rtwdev, REG_CCA, BIT_CCA_CO, 0);
	/* RX path diversity enable */
	rtw_write32_mask(rtwdev, REG_ANTSEL, BIT_ANT_BYCO, 0);
	/* r_en_mrc_antsel */
	rtw_write32_mask(rtwdev, REG_PRECTRL, BIT_DIS_CO_PATHSEL, 0);
	/* MBC weighting */
	rtw_write32_mask(rtwdev, REG_CCA_MF, BIT_MBC_WIN, 1);
	/* 2R CCA only */
	rtw_write32_mask(rtwdev, REG_CCKTX, BIT_CMB_CCA_2R, 1);
}

static void phy14_set_rfe_reg_24g(struct rtw_8814au_phy *rtwdev)
{
	switch (rtwdev->efuse.rfe_option) {
	case 2:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x72707270);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x72707270);
		rtw_write32(rtwdev, REG_RFE_PINMUX_C, 0x72707270);
		rtw_write32(rtwdev, REG_RFE_PINMUX_D, 0x77707770);

		rtw_write32_mask(rtwdev, REG_RFE_INVSEL_D,
				 BIT_RFE_SELSW0_D, 0x72);

		break;
	case 1:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_C, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_D, 0x77777777);

		rtw_write32_mask(rtwdev, REG_RFE_INVSEL_D,
				 BIT_RFE_SELSW0_D, 0x77);

		break;
	case 0:
	default:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_C, 0x77777777);
		/* Is it not necessary to set REG_RFE_PINMUX_D ? */

		rtw_write32_mask(rtwdev, REG_RFE_INVSEL_D,
				 BIT_RFE_SELSW0_D, 0x77);

		break;
	}
}

static void phy14_set_channel_bb_swing(struct rtw_8814au_phy *rtwdev, u8 band)
{
	rtw_write32_mask(rtwdev, REG_TXSCALE_A, BB_SWING_MASK,
			 phy14_get_bb_swing(rtwdev, band, RF_PATH_A));
	rtw_write32_mask(rtwdev, REG_TXSCALE_B, BB_SWING_MASK,
			 phy14_get_bb_swing(rtwdev, band, RF_PATH_B));
	rtw_write32_mask(rtwdev, REG_TXSCALE_C, BB_SWING_MASK,
			 phy14_get_bb_swing(rtwdev, band, RF_PATH_C));
	rtw_write32_mask(rtwdev, REG_TXSCALE_D, BB_SWING_MASK,
			 phy14_get_bb_swing(rtwdev, band, RF_PATH_D));
	phy14_pwrtrack_init(rtwdev);
}

static void phy14_switch_channel(struct rtw_8814au_phy *rtwdev, u8 channel)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 fc_area, rf_mod_ag, cfgch;
	u8 path;

	switch (channel) {
	case 36 ... 48:
		fc_area = 0x494;
		break;
	case 50 ... 64:
		fc_area = 0x453;
		break;
	case 100 ... 116:
		fc_area = 0x452;
		break;
	default:
		if (channel >= 118)
			fc_area = 0x412;
		else
			fc_area = 0x96a;
		break;
	}

	rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, fc_area);

	for (path = 0; path < hal->rf_path_num; path++) {
		switch (channel) {
		case 36 ... 64:
			rf_mod_ag = 0x101;
			break;
		case 100 ... 140:
			rf_mod_ag = 0x301;
			break;
		default:
			if (channel > 140)
				rf_mod_ag = 0x501;
			else
				rf_mod_ag = 0x000;
			break;
		}

		cfgch = (rf_mod_ag << 8) | channel;

		rtw_write_rf(rtwdev, path, RF_CFGCH,
			     RF18_RFSI_MASK | RF18_BAND_MASK | RF18_CHANNEL_MASK, cfgch);
	}

	switch (channel) {
	case 36 ... 64:
		rtw_write32_mask(rtwdev, REG_AGC_TABLE, 0x1f, 1);
		break;
	case 100 ... 144:
		rtw_write32_mask(rtwdev, REG_AGC_TABLE, 0x1f, 2);
		break;
	default:
		if (channel >= 149)
			rtw_write32_mask(rtwdev, REG_AGC_TABLE, 0x1f, 3);

		break;
	}
}

static void phy14_set_bw_reg_adc(struct rtw_8814au_phy *rtwdev, u8 bw)
{
	u32 adc = 0;

	if (bw == RTW_CHANNEL_WIDTH_20)
		adc = 0;
	else if (bw == RTW_CHANNEL_WIDTH_40)
		adc = 1;
	else if (bw == RTW_CHANNEL_WIDTH_80)
		adc = 2;

	rtw_write32_mask(rtwdev, REG_ADCCLK, BIT(1) | BIT(0), adc);
}

static void phy14_set_bw_reg_agc(struct rtw_8814au_phy *rtwdev, u8 new_band, u8 bw)
{
	u32 agc = 7;

	if (bw == RTW_CHANNEL_WIDTH_20) {
		agc = 6;
	} else if (bw == RTW_CHANNEL_WIDTH_40) {
		if (new_band == RTW_BAND_5G)
			agc = 8;
		else
			agc = 7;
	} else if (bw == RTW_CHANNEL_WIDTH_80) {
		agc = 3;
	}

	rtw_write32_mask(rtwdev, REG_CCASEL, 0xf000, agc);
}

static void phy14_24g_cck_tx_dfir(struct rtw_8814au_phy *rtwdev, u8 channel)
{
	if (channel >= 1 && channel <= 11) {
		rtw_write32(rtwdev, REG_CCK0_TX_FILTER1, 0x1a1b0030);
		rtw_write32(rtwdev, REG_CCK0_TX_FILTER2, 0x090e1317);
		rtw_write32(rtwdev, REG_CCK0_DEBUG_PORT, 0x00000204);
	} else if (channel >= 12 && channel <= 13) {
		rtw_write32(rtwdev, REG_CCK0_TX_FILTER1, 0x1a1b0030);
		rtw_write32(rtwdev, REG_CCK0_TX_FILTER2, 0x090e1217);
		rtw_write32(rtwdev, REG_CCK0_DEBUG_PORT, 0x00000305);
	} else if (channel == 14) {
		rtw_write32(rtwdev, REG_CCK0_TX_FILTER1, 0x1a1b0030);
		rtw_write32(rtwdev, REG_CCK0_TX_FILTER2, 0x00000E17);
		rtw_write32(rtwdev, REG_CCK0_DEBUG_PORT, 0x00000000);
	}
}

static void phy14_set_bw_reg_mac(struct rtw_8814au_phy *rtwdev, u8 bw)
{
	u16 val16 = rtw_read16(rtwdev, REG_WMAC_TRXPTCL_CTL);

	val16 &= ~BIT_RFMOD;
	if (bw == RTW_CHANNEL_WIDTH_80)
		val16 |= BIT_RFMOD_80M;
	else if (bw == RTW_CHANNEL_WIDTH_40)
		val16 |= BIT_RFMOD_40M;

	rtw_write16(rtwdev, REG_WMAC_TRXPTCL_CTL, val16);
}

static void phy14_set_bw_rf(struct rtw_8814au_phy *rtwdev, u8 bw)
{
	u8 path;

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		switch (bw) {
		case RTW_CHANNEL_WIDTH_5:
		case RTW_CHANNEL_WIDTH_10:
		case RTW_CHANNEL_WIDTH_20:
		default:
			rtw_write_rf(rtwdev, path, RF_CFGCH, RF18_BW_MASK, 3);
			break;
		case RTW_CHANNEL_WIDTH_40:
			rtw_write_rf(rtwdev, path, RF_CFGCH, RF18_BW_MASK, 1);
			break;
		case RTW_CHANNEL_WIDTH_80:
			rtw_write_rf(rtwdev, path, RF_CFGCH, RF18_BW_MASK, 0);
			break;
		}
	}
}

static void phy14_adc_clk(struct rtw_8814au_phy *rtwdev)
{
	static const u32 rxiqc_reg[2][4] = {
		{ REG_RX_IQC_AB_A, REG_RX_IQC_AB_B,
		  REG_RX_IQC_AB_C, REG_RX_IQC_AB_D },
		{ REG_RX_IQC_CD_A, REG_RX_IQC_CD_B,
		  REG_RX_IQC_CD_C, REG_RX_IQC_CD_D }
	};
	u32 bb_reg_8fc, bb_reg_808, rxiqc[4];
	u32 i = 0, mac_active = 1;
	u8 mac_reg_522;

	if (rtwdev->hal.cut_version != RTW_CHIP_VER_CUT_A)
		return;

	/* 1 Step1. MAC TX pause */
	mac_reg_522 = rtw_read8(rtwdev, REG_TXPAUSE);
	bb_reg_8fc = rtw_read32(rtwdev, REG_DBGSEL);
	bb_reg_808 = rtw_read32(rtwdev, REG_RXPSEL);
	/* 1 Step 2. Backup rxiqc & rxiqc = 0 */
	for (i = 0; i < 4; i++) {
		rxiqc[i] = rtw_read32(rtwdev, rxiqc_reg[0][i]);
	}
	if (rtwdev->ctx->io_error) return;
	rtw_write8(rtwdev, REG_TXPAUSE, 0x3f);
	for (i = 0; i < 4; i++) {
		rtw_write32(rtwdev, rxiqc_reg[0][i], 0x0);
		rtw_write32(rtwdev, rxiqc_reg[1][i], 0x0);
	}
	rtw_write32_mask(rtwdev, REG_PRECTRL, BIT_IQ_WGT, 0x3);
	i = 0;

	/* 1 Step 3. Monitor MAC IDLE */
	rtw_write32(rtwdev, REG_DBGSEL, 0x0);
	while (mac_active) {
		mac_active = rtw_read32(rtwdev, REG_DBGRPT) & 0x803e0008;
		i++;
		if (i > 1000)
			break;
	}

	/* 1 Step 4. ADC clk flow */
	rtw_write8(rtwdev, REG_RXPSEL, 0x11);
	rtw_write32_mask(rtwdev, REG_DAC_RSTB, BIT(13), 0x1);
	rtw_write8_mask(rtwdev, REG_GNT_BT, BIT(2) | BIT(1), 0x3);
	rtw_write32_mask(rtwdev, REG_CCK_RPT_FORMAT, BIT(2), 0x1);

	/* 0xc1c/0xe1c/0x181c/0x1a1c[4] must=1 to ensure table can be
	 * written when bbrstb=0
	 * 0xc60/0xe60/0x1860/0x1a60[15] always = 1 after this line
	 * 0xc60/0xe60/0x1860/0x1a60[14] always = 0 bcz its error in A-cut
	 */

	/* power_off/clk_off @ anapar_state=idle mode */
	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x15800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x01808003);
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x15800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x01808003);
	rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x15800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x01808003);
	rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x15800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x01808003);

	rtw_write8_mask(rtwdev, REG_GNT_BT, BIT(2), 0x0);
	rtw_write32_mask(rtwdev, REG_CCK_RPT_FORMAT, BIT(2), 0x0);
	/* [19] = 1 to turn off ADC */
	rtw_write32(rtwdev, REG_CK_MONHA, 0x0D080058);
	rtw_write32(rtwdev, REG_CK_MONHB, 0x0D080058);
	rtw_write32(rtwdev, REG_CK_MONHC, 0x0D080058);
	rtw_write32(rtwdev, REG_CK_MONHD, 0x0D080058);

	/* power_on/clk_off */
	/* [19] = 0 to turn on ADC */
	rtw_write32(rtwdev, REG_CK_MONHA, 0x0D000058);
	rtw_write32(rtwdev, REG_CK_MONHB, 0x0D000058);
	rtw_write32(rtwdev, REG_CK_MONHC, 0x0D000058);
	rtw_write32(rtwdev, REG_CK_MONHD, 0x0D000058);

	/* power_on/clk_on @ anapar_state=BT mode */
	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x05808032);
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x05808032);
	rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x05808032);
	rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x05808032);
	rtw_write8_mask(rtwdev, REG_GNT_BT, BIT(2), 0x1);
	rtw_write32_mask(rtwdev, REG_CCK_RPT_FORMAT, BIT(2), 0x1);

	/* recover original setting @ anapar_state=BT mode */
	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x05808032);
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x05808032);
	rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x05808032);
	rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x05808032);

	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x05800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x07808003);
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x05800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x07808003);
	rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x05800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x07808003);
	rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x05800002);
	rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x07808003);

	rtw_write8_mask(rtwdev, REG_GNT_BT, BIT(2) | BIT(1), 0x0);
	rtw_write32_mask(rtwdev, REG_CCK_RPT_FORMAT, BIT(2), 0x0);
	rtw_write32_mask(rtwdev, REG_DAC_RSTB, BIT(13), 0x0);

	/* 1 Step 5. Recover MAC TX & IQC */
	rtwdev->cleanup = true;
	rtw_write8(rtwdev, REG_TXPAUSE, mac_reg_522);
	rtw_write32(rtwdev, REG_DBGSEL, bb_reg_8fc);
	rtw_write32(rtwdev, REG_RXPSEL, bb_reg_808);
	for (i = 0; i < 4; i++) {
		rtw_write32(rtwdev, rxiqc_reg[0][i], rxiqc[i]);
		rtw_write32(rtwdev, rxiqc_reg[1][i], 0x01000000);
	}
	rtw_write32_mask(rtwdev, REG_PRECTRL, BIT_IQ_WGT, 0x0);
	rtwdev->cleanup = false;
}

static void phy14_spur_calibration_ch140(struct rtw_8814au_phy *rtwdev, u8 channel)
{
	struct rtw_hal *hal = &rtwdev->hal;

	/* Add for 8814AE module ch140 MP Rx */
	if (channel == 140) {
		if (hal->ch_param[0] == 0)
			hal->ch_param[0] = rtw_read32(rtwdev, REG_CCASEL);
		if (hal->ch_param[1] == 0)
			hal->ch_param[1] = rtw_read32(rtwdev, REG_PDMFTH);

		rtw_write32(rtwdev, REG_CCASEL, 0x75438170);
		rtw_write32(rtwdev, REG_PDMFTH, 0x79a18a0a);
	} else {
		if (rtw_read32(rtwdev, REG_CCASEL) == 0x75438170 &&
		    hal->ch_param[0] != 0)
			rtw_write32(rtwdev, REG_CCASEL, hal->ch_param[0]);

		if (rtw_read32(rtwdev, REG_PDMFTH) == 0x79a18a0a &&
		    hal->ch_param[1] != 0)
			rtw_write32(rtwdev, REG_PDMFTH, hal->ch_param[1]);

		hal->ch_param[0] = rtw_read32(rtwdev, REG_CCASEL);
		hal->ch_param[1] = rtw_read32(rtwdev, REG_PDMFTH);
	}
}

static void phy14_set_nbi_reg(struct rtw_8814au_phy *rtwdev, u32 tone_idx)
{
	/* tone_idx X 10 */
	static const u32 nbi_128[] = {
		25, 55, 85, 115, 135,
		155, 185, 205, 225, 245,
		265, 285, 305, 335, 355,
		375, 395, 415, 435, 455,
		485, 505, 525, 555, 585, 615, 635
	};
	u32 reg_idx = 0;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(nbi_128); i++) {
		if (tone_idx < nbi_128[i]) {
			reg_idx = i + 1;
			break;
		}
	}

	rtw_write32_mask(rtwdev, REG_NBI_SETTING, 0xfc000, reg_idx);
}

static void phy14_nbi_setting(struct rtw_8814au_phy *rtwdev, u32 ch, u32 f_intf)
{
	u32 fc, int_distance, tone_idx;

	fc = 2412 + (ch - 1) * 5;
	int_distance = abs_diff(fc, f_intf);

	/* 10 * (int_distance / 0.3125) */
	tone_idx = int_distance << 5;

	phy14_set_nbi_reg(rtwdev, tone_idx);

	rtw_write32_mask(rtwdev, REG_NBI_SETTING, BIT_NBI_ENABLE, 1);
}

static void phy14_spur_nbi_setting(struct rtw_8814au_phy *rtwdev)
{
	u8 primary_channel = rtwdev->hal.current_channel;
	u8 rfe_type = rtwdev->efuse.rfe_option;

	if (rfe_type != 0 && rfe_type != 1 && rfe_type != 6 && rfe_type != 7)
		return;

	if (primary_channel == 14)
		phy14_nbi_setting(rtwdev, primary_channel, 2480);
	else if (primary_channel >= 4 && primary_channel <= 8)
		phy14_nbi_setting(rtwdev, primary_channel, 2440);
	else
		rtw_write32_mask(rtwdev, REG_NBI_SETTING, BIT_NBI_ENABLE, 0);
}

static void phy14_spur_calibration(struct rtw_8814au_phy *rtwdev, u8 channel, u8 bw)
{
	u8 rfe_type = rtwdev->efuse.rfe_option;
	bool reset_nbi_csi = true;

	if (rfe_type == 0) {
		switch (bw) {
		case RTW_CHANNEL_WIDTH_40:
			if (channel == 54 || channel == 118) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x3e >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK7, 0);

				reset_nbi_csi = false;
			} else if (channel == 151) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x1e >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK0,
						 BIT(16), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK7, 0);

				reset_nbi_csi = false;
			}
			break;
		case RTW_CHANNEL_WIDTH_80:
			if (channel == 58 || channel == 122) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x3a >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK7,
						 BIT(0), 1);

				reset_nbi_csi = false;
			} else if (channel == 155) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x5a >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK6,
						 BIT(16), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK7, 0);

				reset_nbi_csi = false;
			}
			break;
		case RTW_CHANNEL_WIDTH_20:
			if (channel == 153) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x1e >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK7,
						 BIT(16), 1);

				reset_nbi_csi = false;
			}

			phy14_spur_calibration_ch140(rtwdev, channel);
			break;
		default:
			break;
		}
	} else if (rfe_type == 1 || rfe_type == 2) {
		switch (bw) {
		case RTW_CHANNEL_WIDTH_20:
			if (channel == 153) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x1E >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK7,
						 BIT(16), 1);

				reset_nbi_csi = false;
			}
			break;
		case RTW_CHANNEL_WIDTH_40:
			if (channel == 151) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x1e >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK0,
						 BIT(16), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK7, 0);

				reset_nbi_csi = false;
			}
			break;
		case RTW_CHANNEL_WIDTH_80:
			if (channel == 155) {
				rtw_write32_mask(rtwdev, REG_NBI_SETTING,
						 0x000fe000, 0x5a >> 1);
				rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1,
						 BIT(0), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
				rtw_write32_mask(rtwdev, REG_CSI_FIX_MASK6,
						 BIT(16), 1);
				rtw_write32(rtwdev, REG_CSI_FIX_MASK7, 0);

				reset_nbi_csi = false;
			}
			break;
		default:
			break;
		}
	}

	if (reset_nbi_csi) {
		rtw_write32_mask(rtwdev, REG_NBI_SETTING,
				 0x000fe000, 0xfc >> 1);
		rtw_write32_mask(rtwdev, REG_CSI_MASK_SETTING1, BIT(0), 0);
		rtw_write32(rtwdev, REG_CSI_FIX_MASK0, 0);
		rtw_write32(rtwdev, REG_CSI_FIX_MASK1, 0);
		rtw_write32(rtwdev, REG_CSI_FIX_MASK6, 0);
		rtw_write32(rtwdev, REG_CSI_FIX_MASK7, 0);
	}

	phy14_spur_nbi_setting(rtwdev);
}

static void phy14_iqk_backup_mac_bb(struct rtw_8814au_phy *rtwdev,
				       u32 *mac_backup, u32 *bb_backup,
				       const u32 *mac_regs,
				       const u32 *bb_regs)
{
	u32 i;

	/* save MACBB default value */
	for (i = 0; i < MAC_REG_NUM_8814; i++)
		mac_backup[i] = rtw_read32(rtwdev, mac_regs[i]);

	for (i = 0; i < BB_REG_NUM_8814; i++)
		bb_backup[i] = rtw_read32(rtwdev, bb_regs[i]);
}

static void phy14_iqk_backup_rf(struct rtw_8814au_phy *rtwdev,
				   u32 rf_backup[][4], const u32 *rf_regs)
{
	u32 i;

	/* Save RF Parameters */
	for (i = 0; i < RF_REG_NUM_8814; i++) {
		rf_backup[i][RF_PATH_A] = rtw_read_rf(rtwdev, RF_PATH_A,
						      rf_regs[i], RFREG_MASK);
		rf_backup[i][RF_PATH_B] = rtw_read_rf(rtwdev, RF_PATH_B,
						      rf_regs[i], RFREG_MASK);
		rf_backup[i][RF_PATH_C] = rtw_read_rf(rtwdev, RF_PATH_C,
						      rf_regs[i], RFREG_MASK);
		rf_backup[i][RF_PATH_D] = rtw_read_rf(rtwdev, RF_PATH_D,
						      rf_regs[i], RFREG_MASK);
	}
}

static void phy14_iqk_afe_setting(struct rtw_8814au_phy *rtwdev, bool do_iqk)
{
	if (do_iqk) {
		/* IQK AFE setting RX_WAIT_CCA mode */
		rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x0e808003);
		rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x0e808003);
		rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x0e808003);
		rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x0e808003);
	} else {
		rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x07808003);
		rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x07808003);
		rtw_write32(rtwdev, REG_AFE_PWR1_C, 0x07808003);
		rtw_write32(rtwdev, REG_AFE_PWR1_D, 0x07808003);
	}

	rtw_write32_mask(rtwdev, REG_DAC_RSTB, BIT(13), 0x1);

	rtw_write8_set(rtwdev, REG_GNT_BT, BIT(2) | BIT(1));
	rtw_write8_clr(rtwdev, REG_GNT_BT, BIT(2) | BIT(1));

	rtw_write32_set(rtwdev, REG_CCK_RPT_FORMAT, BIT(2));
	rtw_write32_clr(rtwdev, REG_CCK_RPT_FORMAT, BIT(2));
}

static void phy14_iqk_restore_mac_bb(struct rtw_8814au_phy *rtwdev,
					u32 *mac_backup, u32 *bb_backup,
					const u32 *mac_regs,
					const u32 *bb_regs)
{
	u32 i;

	/* Reload MacBB Parameters */
	for (i = 0; i < MAC_REG_NUM_8814; i++)
		rtw_write32(rtwdev, mac_regs[i], mac_backup[i]);

	for (i = 0; i < BB_REG_NUM_8814; i++)
		rtw_write32(rtwdev, bb_regs[i], bb_backup[i]);
}

static void phy14_iqk_restore_rf(struct rtw_8814au_phy *rtwdev,
				    const u32 rf_backup[][4],
				    const u32 *rf_regs)
{
	u32 i;

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x0);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWE, RFREG_MASK, 0x0);
	rtw_write_rf(rtwdev, RF_PATH_C, RF_LUTWE, RFREG_MASK, 0x0);
	rtw_write_rf(rtwdev, RF_PATH_D, RF_LUTWE, RFREG_MASK, 0x0);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_RXBB2, RFREG_MASK, 0x88001);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_RXBB2, RFREG_MASK, 0x88001);
	rtw_write_rf(rtwdev, RF_PATH_C, RF_RXBB2, RFREG_MASK, 0x88001);
	rtw_write_rf(rtwdev, RF_PATH_D, RF_RXBB2, RFREG_MASK, 0x88001);

	for (i = 0; i < RF_REG_NUM_8814; i++) {
		rtw_write_rf(rtwdev, RF_PATH_A, rf_regs[i],
			     RFREG_MASK, rf_backup[i][RF_PATH_A]);
		rtw_write_rf(rtwdev, RF_PATH_B, rf_regs[i],
			     RFREG_MASK, rf_backup[i][RF_PATH_B]);
		rtw_write_rf(rtwdev, RF_PATH_C, rf_regs[i],
			     RFREG_MASK, rf_backup[i][RF_PATH_C]);
		rtw_write_rf(rtwdev, RF_PATH_D, rf_regs[i],
			     RFREG_MASK, rf_backup[i][RF_PATH_D]);
	}
}

static void phy14_iqk_reset_nctl(struct rtw_8814au_phy *rtwdev)
{
	rtw_write32(rtwdev, 0x1b00, 0xf8000000);
	rtw_write32(rtwdev, 0x1b80, 0x00000006);

	rtw_write32(rtwdev, 0x1b00, 0xf8000000);
	rtw_write32(rtwdev, 0x1b80, 0x00000002);
}

static void phy14_iqk_configure_mac(struct rtw_8814au_phy *rtwdev)
{
	rtw_write8(rtwdev, REG_TXPAUSE, 0x3f);
	rtw_write32_clr(rtwdev, REG_BCN_CTRL,
			(BIT_EN_BCN_FUNCTION << 8) | BIT_EN_BCN_FUNCTION);

	/* RX ante off */
	rtw_write8(rtwdev, REG_RXPSEL, 0x00);
	/* CCA off */
	rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf, 0xe);
	/* CCK RX path off */
	rtw_write32_set(rtwdev, REG_PRECTRL, BIT_IQ_WGT);
	rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
	rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
	rtw_write32(rtwdev, REG_RFE_PINMUX_C, 0x77777777);
	rtw_write32(rtwdev, REG_RFE_PINMUX_D, 0x77777777);
	rtw_write32_mask(rtwdev, REG_RFE_INVSEL_D, BIT_RFE_SELSW0_D, 0x77);
	rtw_write32_mask(rtwdev, REG_PSD, BIT_PSD_INI, 0x0);

	rtw_write32_mask(rtwdev, REG_RFE_INV0, 0xf, 0x0);
}

static void phy14_lok_one_shot(struct rtw_8814au_phy *rtwdev, u8 path)
{
	u32 lok_temp1, lok_temp2;
	bool lok_ready;
	u8 ii;

	/* ADC Clock source */
	rtw_write32_mask(rtwdev, REG_FAS, BIT(21) | BIT(20), path);
	/* LOK: CMD ID = 0
	 * {0xf8000011, 0xf8000021, 0xf8000041, 0xf8000081}
	 */
	rtw_write32(rtwdev, 0x1b00, 0xf8000001 | (BIT(path) << 4));

	usleep_range(1000, 1100);

	if (phy_poll_ready(rtwdev, &lok_ready, 10000)) {
		

		phy14_iqk_reset_nctl(rtwdev);

		rtw_write_rf(rtwdev, path, RF_DTXLOK, RFREG_MASK, 0x08400);

		return;
	}

	rtwdev->lok_ok[path] = true;
	rtw_write32(rtwdev, 0x1b00, 0xf8000000 | ((u32)path << 1));
	rtw_write32(rtwdev, 0x1bd4, 0x003f0001);

	lok_temp2 = rtw_read32_mask(rtwdev, 0x1bfc, 0x003e0000);
	lok_temp2 = (lok_temp2 + 0x10) & 0x1f;

	lok_temp1 = rtw_read32_mask(rtwdev, 0x1bfc, 0x0000003e);
	lok_temp1 = (lok_temp1 + 0x10) & 0x1f;

	for (ii = 1; ii < 5; ii++) {
		lok_temp1 += (lok_temp1 & BIT(4 - ii)) << (ii * 2);
		lok_temp2 += (lok_temp2 & BIT(4 - ii)) << (ii * 2);
	}

	

	rtw_write_rf(rtwdev, path, RF_DTXLOK, 0x07c00, lok_temp1 >> 4);
	rtw_write_rf(rtwdev, path, RF_DTXLOK, 0xf8000, lok_temp2 >> 4);
}

static void phy14_iqk_tx_one_shot(struct rtw_8814au_phy *rtwdev, u8 path,
				     u32 *tx_matrix, bool *tx_ok)
{
	u8 bw = rtwdev->hal.current_band_width;
	u8 cal_retry;
	u32 iqk_cmd;

	for (cal_retry = 0; cal_retry < 4; cal_retry++) {
		rtw_write32_mask(rtwdev, REG_FAS, BIT(21) | BIT(20), path);

		iqk_cmd = 0xf8000001 | (((u32)bw + 3U) << 8) | (BIT(path) << 4);

		

		rtw_write32(rtwdev, 0x1b00, iqk_cmd);

		usleep_range(10000, 11000);

		if (phy_poll_ready(rtwdev, tx_ok, 20000)) {
			

			phy14_iqk_reset_nctl(rtwdev);
		} else {
			*tx_ok = !rtw_read32_mask(rtwdev, 0x1b08, BIT(26));

			if (*tx_ok)
				break;
		}
	}

	
	
	

	rtw_write32(rtwdev, 0x1b00, 0xf8000000 | ((u32)path << 1));

	rtwdev->tx_ok[path] = *tx_ok;
	if (*tx_ok) {
		*tx_matrix = rtw_read32(rtwdev, 0x1b38);

		
	}
}

static void phy14_iqk_rx_one_shot(struct rtw_8814au_phy *rtwdev, u8 path,
				     u32 *tx_matrix, bool *tx_ok)
{
	static const u16 iqk_apply[RTW_RF_PATH_MAX] = {
		REG_TXAGCIDX, REG_TX_AGC_B, REG_TX_AGC_C, REG_TX_AGC_D
	};
	u8 band = rtwdev->hal.current_band_type;
	u8 bw = rtwdev->hal.current_band_width;
	
	u8 cal_retry;
	u32 iqk_cmd;
	bool rx_ok = false;

	for (cal_retry = 0; cal_retry < 4; cal_retry++) {
		rtw_write32_mask(rtwdev, REG_FAS, BIT(21) | BIT(20), path);

		if (band == RTW_BAND_2G) {
			rtw_write_rf(rtwdev, path, RF_LUTDBG, BIT(11), 0x1);
			rtw_write_rf(rtwdev, path, RF_GAINTX, 0xfffff, 0x51ce1);

			switch (path) {
			case 0:
			case 1:
				rtw_write32(rtwdev, REG_RFE_PINMUX_B,
					    0x54775477);
				break;
			case 2:
				rtw_write32(rtwdev, REG_RFE_PINMUX_C,
					    0x54775477);
				break;
			case 3:
				rtw_write32(rtwdev, REG_RFE_INVSEL_D, 0x75400000);
				rtw_write32(rtwdev, REG_RFE_PINMUX_D,
					    0x77777777);
				break;
			}
		}

		iqk_cmd = 0xf8000001 | ((9U - (u32)bw) << 8) | (BIT(path) << 4);

		

		rtw_write32(rtwdev, 0x1b00, iqk_cmd);

		usleep_range(10000, 11000);

		if (phy_poll_ready(rtwdev, &rx_ok, 20000)) {
			

			phy14_iqk_reset_nctl(rtwdev);
		} else {
			rx_ok = !rtw_read32_mask(rtwdev, 0x1b08, BIT(26));

			if (rx_ok)
				break;
		}
	}

	
	
	

	rtw_write32(rtwdev, 0x1b00, 0xf8000000 | ((u32)path << 1));

	rtwdev->rx_ok[path] = rx_ok;
	if (rx_ok) {
		rtw_write32(rtwdev, 0x1b3c, 0x20000000);
		(void)rtw_read32(rtwdev, 0x1b3c);

		
	}

	if (*tx_ok)
		rtw_write32(rtwdev, 0x1b38, *tx_matrix);
	else
		rtw_write32_mask(rtwdev, iqk_apply[path], BIT(0), 0x0);

	if (!rx_ok)
		rtw_write32_mask(rtwdev, iqk_apply[path],
				 BIT(11) | BIT(10), 0x0);

	if (band == RTW_BAND_2G)
		rtw_write_rf(rtwdev, path, RF_LUTDBG, BIT(11), 0x0);
}

static void phy14_iqk(struct rtw_8814au_phy *rtwdev)
{
	u8 band = rtwdev->hal.current_band_type;
	u32 tx_matrix[RTW_RF_PATH_MAX] = {0};
	bool tx_ok[RTW_RF_PATH_MAX] = {false};
	u8 path;

	

	rtw_write_rf(rtwdev, RF_PATH_A, RF_TXMOD, BIT(19), 0x1);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_TXMOD, BIT(19), 0x1);
	rtw_write_rf(rtwdev, RF_PATH_C, RF_TXMOD, BIT(19), 0x1);
	rtw_write_rf(rtwdev, RF_PATH_D, RF_TXMOD, BIT(19), 0x1);

	rtw_write32_mask(rtwdev, REG_TXAGCIDX,
			 (BIT(11) | BIT(10) | BIT(0)), 0x401);
	rtw_write32_mask(rtwdev, REG_TX_AGC_B,
			 (BIT(11) | BIT(10) | BIT(0)), 0x401);
	rtw_write32_mask(rtwdev, REG_TX_AGC_C,
			 (BIT(11) | BIT(10) | BIT(0)), 0x401);
	rtw_write32_mask(rtwdev, REG_TX_AGC_D,
			 (BIT(11) | BIT(10) | BIT(0)), 0x401);

	if (band == RTW_BAND_5G)
		rtw_write32(rtwdev, 0x1b00, 0xf8000ff1);
	else
		rtw_write32(rtwdev, 0x1b00, 0xf8000ef1);

	usleep_range(1000, 1100);

	rtw_write32(rtwdev, 0x810, 0x20101063);
	rtw_write32(rtwdev, REG_DAC_RSTB, 0x0B00C000);

	for (path = RF_PATH_A; path < RTW_RF_PATH_MAX; path++)
		phy14_lok_one_shot(rtwdev, path);

	for (path = RF_PATH_A; path < RTW_RF_PATH_MAX; path++)
		phy14_iqk_tx_one_shot(rtwdev, path,
					 &tx_matrix[path], &tx_ok[path]);

	for (path = RF_PATH_A; path < RTW_RF_PATH_MAX; path++)
		phy14_iqk_rx_one_shot(rtwdev, path,
					 &tx_matrix[path], &tx_ok[path]);
}

static void phy14_do_iqk(struct rtw_8814au_phy *rtwdev)
{
	static const u32 backup_mac_reg[MAC_REG_NUM_8814] = {0x520, 0x550};
	static const u32 backup_bb_reg[BB_REG_NUM_8814] = {
		0xa14, 0x808, 0x838, 0x90c, 0x810, 0xcb0, 0xeb0,
		0x18b4, 0x1ab4, 0x1abc, 0x9a4, 0x764, 0xcbc, 0x910
	};
	static const u32 backup_rf_reg[RF_REG_NUM_8814] = {0x0};
	u32 rf_backup[RF_REG_NUM_8814][RTW_RF_PATH_MAX];
	u32 mac_backup[MAC_REG_NUM_8814];
	u32 bb_backup[BB_REG_NUM_8814];

	phy14_iqk_backup_mac_bb(rtwdev, mac_backup, bb_backup,
				   backup_mac_reg, backup_bb_reg);
	phy14_iqk_backup_rf(rtwdev, rf_backup, backup_rf_reg);
	if (rtwdev->ctx->io_error) return;
	phy14_iqk_afe_setting(rtwdev, true);
	phy14_iqk_configure_mac(rtwdev);
	phy14_iqk(rtwdev);
	rtwdev->cleanup = true;
	phy14_iqk_reset_nctl(rtwdev); /* for 3-wire to BB use */
	phy14_iqk_afe_setting(rtwdev, false);
	phy14_iqk_restore_mac_bb(rtwdev, mac_backup, bb_backup,
				    backup_mac_reg, backup_bb_reg);
	phy14_iqk_restore_rf(rtwdev, rf_backup, backup_rf_reg);
	rtwdev->cleanup = false;
}

static void phy14_txagc_swing_offset(struct rtw_8814au_phy *rtwdev, u8 path,
					u8 tx_pwr_idx_offset,
					s8 *txagc_idx, u8 *swing_idx)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 swing_upper_bound = dm_info->default_ofdm_index + 10;
	s8 delta_pwr_idx = dm_info->delta_power_index[path];
	u8 swing_index = dm_info->default_ofdm_index;
	u8 max_tx_pwr_idx_offset = 0xf;
	u8 swing_lower_bound = 0;
	s8 agc_index = 0;

	tx_pwr_idx_offset = min_t(u8, tx_pwr_idx_offset, max_tx_pwr_idx_offset);

	if (delta_pwr_idx >= 0) {
		if (delta_pwr_idx <= tx_pwr_idx_offset) {
			agc_index = delta_pwr_idx;
			swing_index = dm_info->default_ofdm_index;
		} else if (delta_pwr_idx > tx_pwr_idx_offset) {
			agc_index = (s8)tx_pwr_idx_offset;
			swing_index = (u8)(dm_info->default_ofdm_index +
					delta_pwr_idx - tx_pwr_idx_offset);
			swing_index = min_t(u8, swing_index, swing_upper_bound);
		}
	} else {
		if (dm_info->default_ofdm_index > phy_abs(delta_pwr_idx))
			swing_index =
				(u8)(dm_info->default_ofdm_index + delta_pwr_idx);
		else
			swing_index = swing_lower_bound;
		swing_index = max_t(u8, swing_index, swing_lower_bound);

		agc_index = 0;
	}

	if (swing_index >= RTW_TXSCALE_SIZE) {
		rtw_warn(rtwdev, "swing index overflow\n");
		swing_index = RTW_TXSCALE_SIZE - 1;
	}
	*txagc_idx = agc_index;
	*swing_idx = swing_index;
}

static void phy14_pwrtrack_set_pwr(struct rtw_8814au_phy *rtwdev, u8 path,
				      u8 pwr_idx_offset)
{
	static const u32 txagc_reg[RTW_RF_PATH_MAX] = {
		REG_TX_AGC_A, REG_TX_AGC_B, REG_TX_AGC_C, REG_TX_AGC_D
	};
	static const u32 txscale_reg[RTW_RF_PATH_MAX] = {
		REG_TXSCALE_A, REG_TXSCALE_B, REG_TXSCALE_C, REG_TXSCALE_D
	};
	s8 txagc_idx;
	u8 swing_idx;

	phy14_txagc_swing_offset(rtwdev, path, pwr_idx_offset,
				    &txagc_idx, &swing_idx);
	rtw_write32_mask(rtwdev, txagc_reg[path], GENMASK(29, 25),
			 (u32)(u8)txagc_idx);
	rtw_write32_mask(rtwdev, txscale_reg[path], BB_SWING_MASK,
			 phy14_txscale_tbl[swing_idx]);
}
