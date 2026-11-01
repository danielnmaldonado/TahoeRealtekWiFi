/* SPDX-License-Identifier: BSD-3-Clause */
/* Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation. GPL-2.0 OR BSD-3-Clause.
 * This port selectsBSD-3-Clause; retain RTL8812AUPHY-LICENSE.txt. */
static void r12_chip12_config_1t(struct rtw_r12au_phy *rtwdev)
{
	/* BB OFDM RX Path_A */
	rtw_write32_mask(rtwdev, REG_RXPSEL, 0xff, 0x11);

	/* BB OFDM TX Path_A */
	rtw_write32_mask(rtwdev, REG_TXPSEL, MASKLWORD, 0x1111);

	/* BB CCK R/Rx Path_A */
	rtw_write32_mask(rtwdev, REG_CCK_RX, 0x0c000000, 0x0);

	/* MCS support */
	rtw_write32_mask(rtwdev, REG_RX_MCS_LIMIT, 0xc0000060, 0x4);

	/* RF Path_B HSSI OFF */
	rtw_write32_mask(rtwdev, REG_3WIRE_SWB, 0xf, 0x4);

	/* RF Path_B Power Down */
	rtw_write32_mask(rtwdev, REG_LSSI_WRITE_B, MASKDWORD, 0);

	/* ADDA Path_B OFF */
	rtw_write32_mask(rtwdev, REG_AFE_PWR1_B, MASKDWORD, 0);
	rtw_write32_mask(rtwdev, REG_AFE_PWR2_B, MASKDWORD, 0);
}

static u32 r12_get_bb_swing(struct rtw_r12au_phy *rtwdev, u8 band, u8 path)
{
	static const u32 swing2setting[4] = {0x200, 0x16a, 0x101, 0x0b6};
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 tx_bb_swing;

	if (band == RTW_BAND_2G)
		tx_bb_swing = efuse->tx_bb_swing_setting_2g;
	else
		tx_bb_swing = efuse->tx_bb_swing_setting_5g;

	if (path == RF_PATH_B)
		tx_bb_swing >>= 2;
	tx_bb_swing &= 0x3;

	return swing2setting[tx_bb_swing];
}

static u8 r12_get_swing_index(struct rtw_r12au_phy *rtwdev)
{
	u32 swing, table_value;
	u8 i;

	swing = r12_get_bb_swing(rtwdev, rtwdev->hal.current_band_type,
				      RF_PATH_A);

	for (i = 0; i < ARRAY_SIZE(r12_txscale_tbl); i++) {
		table_value = r12_txscale_tbl[i];
		if (swing == table_value)
			return i;
	}

	return 24;
}

static void r12_pwrtrack_init(struct rtw_r12au_phy *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 path;

	dm_info->default_ofdm_index = r12_get_swing_index(rtwdev);

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		dm_info->default_cck_index = 0;
	else
		dm_info->default_cck_index = 24;

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		ewma_thermal_init(&dm_info->avg_thermal[path]);
		dm_info->delta_power_index[path] = 0;
		dm_info->delta_power_index_last[path] = 0;
	}

	dm_info->pwr_trk_triggered = false;
	dm_info->pwr_trk_init_trigger = true;
	dm_info->thermal_meter_k = rtwdev->efuse.thermal_meter_k;
}

static void r12_set_channel_bb_swing(struct rtw_r12au_phy *rtwdev, u8 band)
{
	rtw_write32_mask(rtwdev, REG_TXSCALE_A, BB_SWING_MASK,
			 r12_get_bb_swing(rtwdev, band, RF_PATH_A));
	rtw_write32_mask(rtwdev, REG_TXSCALE_B, BB_SWING_MASK,
			 r12_get_bb_swing(rtwdev, band, RF_PATH_B));
	r12_pwrtrack_init(rtwdev);
}

static void r12_chip21_set_ext_band_switch(struct rtw_r12au_phy *rtwdev, u8 band)
{
	rtw_write32_mask(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN, 0);
	rtw_write32_mask(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL, 1);
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0xf, 7);
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0xf0, 7);

	if (band == RTW_BAND_2G)
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(29) | BIT(28), 1);
	else
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(29) | BIT(28), 2);
}

static void r12_chip21_phy_set_rfe_reg_24g(struct rtw_r12au_phy *rtwdev)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;

	/* Turn off RF PA and LNA */

	/* 0xCB0[15:12] = 0x7 (LNA_On)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF000, 0x7);
	/* 0xCB0[7:4] = 0x7 (PAPE_A)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF0, 0x7);

	if (efuse->ext_lna_2g) {
		/* Turn on 2.4G External LNA */
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(20), 1);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(22), 0);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, GENMASK(2, 0), 0x2);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, GENMASK(10, 8), 0x2);
	} else {
		/* Bypass 2.4G External LNA */
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(20), 0);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(22), 0);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, GENMASK(2, 0), 0x7);
		rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, GENMASK(10, 8), 0x7);
	}
}

static void r12_chip21_phy_set_rfe_reg_5g(struct rtw_r12au_phy *rtwdev)
{
	/* Turn ON RF PA and LNA */

	/* 0xCB0[15:12] = 0x7 (LNA_On)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF000, 0x5);
	/* 0xCB0[7:4] = 0x7 (PAPE_A)*/
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xF0, 0x4);

	/* Bypass 2.4G External LNA */
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(20), 0);
	rtw_write32_mask(rtwdev, REG_RFE_INV_A, BIT(22), 0);
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, GENMASK(2, 0), 0x7);
	rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, GENMASK(10, 8), 0x7);
}

static void r12_chip12_phy_set_rfe_reg_24g(struct rtw_r12au_phy *rtwdev)
{
	switch (rtwdev->efuse.rfe_option) {
	case 0:
	case 2:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x000);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		break;
	case 1:
		if (rtwdev->efuse.btcoex) {
			rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xffffff, 0x777777);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0x33f00000, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		} else {
			rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		}
		break;
	case 3:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x54337770);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x54337770);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_ANTSEL_SW, 0x00000303, 0x1);
		break;
	case 4:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x001);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x001);
		break;
	case 5:
		rtw_write8(rtwdev, REG_RFE_PINMUX_A + 2, 0x77);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777777);
		rtw_write8_clr(rtwdev, REG_RFE_INV_A + 3, BIT(0));
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		break;
	case 6:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x07772770);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x07772770);
		rtw_write32(rtwdev, REG_RFE_INV_A, 0x00000077);
		rtw_write32(rtwdev, REG_RFE_INV_B, 0x00000077);
		break;
	default:
		break;
	}
}

static void r12_chip12_phy_set_rfe_reg_5g(struct rtw_r12au_phy *rtwdev)
{
	switch (rtwdev->efuse.rfe_option) {
	case 0:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77337717);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337717);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		break;
	case 1:
		if (rtwdev->efuse.btcoex) {
			rtw_write32_mask(rtwdev, REG_RFE_PINMUX_A, 0xffffff, 0x337717);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337717);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, 0x33f00000, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		} else {
			rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77337717);
			rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337717);
			rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x000);
			rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x000);
		}
		break;
	case 2:
	case 4:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77337777);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337777);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		break;
	case 3:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x54337717);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x54337717);
		rtw_write32_mask(rtwdev, REG_RFE_INV_A, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		rtw_write32_mask(rtwdev, REG_ANTSEL_SW, 0x00000303, 0x1);
		break;
	case 5:
		rtw_write8(rtwdev, REG_RFE_PINMUX_A + 2, 0x33);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77337777);
		rtw_write8_set(rtwdev, REG_RFE_INV_A + 3, BIT(0));
		rtw_write32_mask(rtwdev, REG_RFE_INV_B, RFE_INV_MASK, 0x010);
		break;
	case 6:
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x07737717);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x07737717);
		rtw_write32(rtwdev, REG_RFE_INV_A, 0x00000077);
		rtw_write32(rtwdev, REG_RFE_INV_B, 0x00000077);
		break;
	default:
		break;
	}
}

static void r12_switch_band(struct rtw_r12au_phy *rtwdev, u8 new_band, u8 bw)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	u16 basic_rates;

	/* 8811au one antenna module doesn't support antenna div, so driver must
	 * control antenna band, otherwise one of the band will have issue
	 */
	if (chip->id == RTW_CHIP_TYPE_8821A && !rtwdev->efuse.btcoex &&
	    rtwdev->efuse.ant_div_cfg == 0)
		r12_chip21_set_ext_band_switch(rtwdev, new_band);

	if (new_band == RTW_BAND_2G) {
		rtw_write32_set(rtwdev, REG_RXPSEL, BIT_RX_PSEL_RST);

		if (chip->id == RTW_CHIP_TYPE_8821A) {
			r12_chip21_phy_set_rfe_reg_24g(rtwdev);

			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 0);
		} else {
			rtw_write32_mask(rtwdev, REG_BWINDICATION, 0x3, 0x1);
			rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(17, 13), 0x17);

			if (bw == RTW_CHANNEL_WIDTH_20 &&
			    rtwdev->hal.rf_type == RF_1T1R &&
			    !rtwdev->efuse.ext_lna_2g)
				rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(3, 1), 0x02);
			else
				rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(3, 1), 0x04);

			rtw_write32_mask(rtwdev, REG_CCASEL, 0x3, 0);

			r12_chip12_phy_set_rfe_reg_24g(rtwdev);
		}

		rtw_write32_mask(rtwdev, REG_TXPSEL, 0xf0, 0x1);
		rtw_write32_mask(rtwdev, REG_CCK_RX, 0x0f000000, 0x1);

		basic_rates = BIT(DESC_RATE1M) | BIT(DESC_RATE2M) |
			      BIT(DESC_RATE5_5M) | BIT(DESC_RATE11M) |
			      BIT(DESC_RATE6M) | BIT(DESC_RATE12M) |
			      BIT(DESC_RATE24M);
		rtw_write32_mask(rtwdev, REG_RRSR, 0xfffff, basic_rates);

		rtw_write8_clr(rtwdev, REG_CCK_CHECK, BIT_CHECK_CCK_EN);
	} else { /* RTW_BAND_5G */
		if (chip->id == RTW_CHIP_TYPE_8821A)
			r12_chip21_phy_set_rfe_reg_5g(rtwdev);

		rtw_write8_set(rtwdev, REG_CCK_CHECK, BIT_CHECK_CCK_EN);

		r12_wait_txempty(rtwdev);

		rtw_write32_set(rtwdev, REG_RXPSEL, BIT_RX_PSEL_RST);

		if (chip->id == RTW_CHIP_TYPE_8821A) {
			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 1);
		} else {
			rtw_write32_mask(rtwdev, REG_BWINDICATION, 0x3, 0x2);
			rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(17, 13), 0x15);
			rtw_write32_mask(rtwdev, REG_PDMFTH, GENMASK(3, 1), 0x04);

			rtw_write32_mask(rtwdev, REG_CCASEL, 0x3, 1);

			r12_chip12_phy_set_rfe_reg_5g(rtwdev);
		}

		rtw_write32_mask(rtwdev, REG_TXPSEL, 0xf0, 0);
		rtw_write32_mask(rtwdev, REG_CCK_RX, 0x0f000000, 0xf);

		basic_rates = BIT(DESC_RATE6M) | BIT(DESC_RATE12M) |
			      BIT(DESC_RATE24M);
		rtw_write32_mask(rtwdev, REG_RRSR, 0xfffff, basic_rates);
	}

	r12_set_channel_bb_swing(rtwdev, new_band);
}

static u32 r12_phy_read_rf(struct rtw_r12au_phy *rtwdev,
			 enum rtw_rf_path rf_path, u32 addr, u32 mask)
{
	static const u32 pi_addr[2] = { REG_3WIRE_SWA, REG_3WIRE_SWB };
	static const u32 read_addr[2][2] = {
		{ REG_SI_READ_A, REG_SI_READ_B },
		{ REG_PI_READ_A, REG_PI_READ_B }
	};
	const struct rtw_chip_info *chip = rtwdev->chip;
	const struct rtw_hal *hal = &rtwdev->hal;
	bool set_cca, pi_mode;
	u32 original_cca=0;
	bool cleanup=rtwdev->cleanup;
	u32 val;

	if (rf_path >= hal->rf_phy_num) {
		rtw_err(rtwdev, "unsupported rf path (%d)\n", rf_path);
		return INV_RF_DATA;
	}

	/* CCA off to avoid reading the wrong value.
	 * Toggling CCA would affect RF 0x0, skip it.
	 */
	set_cca = addr != 0x0 && chip->id == RTW_CHIP_TYPE_8812A &&
		  hal->cut_version != RTW_CHIP_VER_CUT_C;

	if (set_cca) {
		original_cca=rtw_read32(rtwdev,REG_CCA2ND);
		if(rtwdev->io_error)return INV_RF_DATA;
		rtw_write32(rtwdev,REG_CCA2ND,original_cca|BIT(3));
	}

	addr &= 0xff;

	pi_mode = rtw_read32_mask(rtwdev, pi_addr[rf_path], 0x4);

	rtw_write32_mask(rtwdev, REG_HSSI_READ, MASKBYTE0, addr);

	if (chip->id == RTW_CHIP_TYPE_8821A ||
	    hal->cut_version == RTW_CHIP_VER_CUT_C)
		udelay(20);

	val = rtw_read32_mask(rtwdev, read_addr[pi_mode][rf_path], mask);

	/* CCA on */
	if (set_cca) {
		rtwdev->cleanup=true;
		rtw_write32(rtwdev,REG_CCA2ND,original_cca);
		rtwdev->cleanup=cleanup;
	}

	return val;
}

static void r12_chip12_phy_fix_spur(struct rtw_r12au_phy *rtwdev, u8 channel, u8 bw)
{
	/* C cut Item12 ADC FIFO CLOCK */
	if (rtwdev->hal.cut_version == RTW_CHIP_VER_CUT_C) {
		if (bw == RTW_CHANNEL_WIDTH_40 && channel == 11)
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0xC00, 0x3);
		else
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0xC00, 0x2);

		/* A workaround to resolve 2480Mhz spur by setting ADC clock
		 * as 160M.
		 */
		if (bw == RTW_CHANNEL_WIDTH_20 && (channel == 13 || channel == 14)) {
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x3);
			rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 1);
		} else if (bw == RTW_CHANNEL_WIDTH_40 && channel == 11) {
			rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 1);
		} else if (bw != RTW_CHANNEL_WIDTH_80) {
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x2);
			rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0);
		}
	} else {
		/* A workaround to resolve 2480Mhz spur by setting ADC clock
		 * as 160M.
		 */
		if (bw == RTW_CHANNEL_WIDTH_20 && (channel == 13 || channel == 14))
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x3);
		else if (channel <= 14) /* 2.4G only */
			rtw_write32_mask(rtwdev, REG_ADCCLK, 0x300, 0x2);
	}
}

static void r12_switch_channel(struct rtw_r12au_phy *rtwdev, u8 channel, u8 bw)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 fc_area, rf_mod_ag;
	u8 path;

	switch (channel) {
	case 36: case 37: case 38: case 39: case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48:
		fc_area = 0x494;
		break;
	case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59: case 60: case 61: case 62: case 63: case 64:
		fc_area = 0x453;
		break;
	case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114: case 115: case 116:
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
		case 36: case 37: case 38: case 39: case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: case 48: case 49: case 50: case 51: case 52: case 53: case 54: case 55: case 56: case 57: case 58: case 59: case 60: case 61: case 62: case 63: case 64:
			rf_mod_ag = 0x101;
			break;
		case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107: case 108: case 109: case 110: case 111: case 112: case 113: case 114: case 115: case 116: case 117: case 118: case 119: case 120: case 121: case 122: case 123: case 124: case 125: case 126: case 127: case 128: case 129: case 130: case 131: case 132: case 133: case 134: case 135: case 136: case 137: case 138: case 139: case 140:
			rf_mod_ag = 0x301;
			break;
		default:
			if (channel > 140)
				rf_mod_ag = 0x501;
			else
				rf_mod_ag = 0x000;
			break;
		}

		rtw_write_rf(rtwdev, path, RF_CFGCH,
			     RF18_RFSI_MASK | RF18_BAND_MASK, rf_mod_ag);

		if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A)
			r12_chip12_phy_fix_spur(rtwdev, channel, bw);

		rtw_write_rf(rtwdev, path, RF_CFGCH, RF18_CHANNEL_MASK, channel);
	}
}

static void r12_set_reg_bw(struct rtw_r12au_phy *rtwdev, u8 bw)
{
	u16 val16 = rtw_read16(rtwdev, REG_WMAC_TRXPTCL_CTL);

	val16 &= ~BIT_RFMOD;
	if (bw == RTW_CHANNEL_WIDTH_80)
		val16 |= BIT_RFMOD_80M;
	else if (bw == RTW_CHANNEL_WIDTH_40)
		val16 |= BIT_RFMOD_40M;

	rtw_write16(rtwdev, REG_WMAC_TRXPTCL_CTL, val16);
}

static void r12_post_set_bw_mode(struct rtw_r12au_phy *rtwdev, u8 channel,
				      u8 bw, u8 primary_chan_idx)
{
	(void)channel;
	struct rtw_hal *hal = &rtwdev->hal;
	u8 txsc40 = 0, txsc20, txsc;
	u8 reg_837, l1pkval;

	r12_set_reg_bw(rtwdev, bw);

	txsc20 = primary_chan_idx;
	if (bw == RTW_CHANNEL_WIDTH_80) {
		if (txsc20 == RTW_SC_20_UPPER || txsc20 == RTW_SC_20_UPMOST)
			txsc40 = RTW_SC_40_UPPER;
		else
			txsc40 = RTW_SC_40_LOWER;
	}

	txsc = (u8)(BIT_TXSC_20M(txsc20) | BIT_TXSC_40M(txsc40));
	rtw_write8(rtwdev, REG_DATA_SC, txsc);

	reg_837 = rtw_read8(rtwdev, REG_BWINDICATION + 3);

	switch (bw) {
	default:
	case RTW_CHANNEL_WIDTH_20:
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x003003C3, 0x00300200);
		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0);

		if (hal->rf_type == RF_2T2R)
			rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, 7);
		else
			rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, 8);

		break;
	case RTW_CHANNEL_WIDTH_40:
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x003003C3, 0x00300201);
		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0);
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x3C, txsc);
		rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf0000000, txsc);

		if (reg_837 & BIT(2)) {
			l1pkval = 6;
		} else {
			if (hal->rf_type == RF_2T2R)
				l1pkval = 7;
			else
				l1pkval = 8;
		}

		rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, l1pkval);

		if (txsc == RTW_SC_20_UPPER)
			rtw_write32_set(rtwdev, REG_RXSB, BIT(4));
		else
			rtw_write32_clr(rtwdev, REG_RXSB, BIT(4));

		break;
	case RTW_CHANNEL_WIDTH_80:
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x003003C3, 0x00300202);
		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 1);
		rtw_write32_mask(rtwdev, REG_ADCCLK, 0x3C, txsc);
		rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf0000000, txsc);

		if (reg_837 & BIT(2)) {
			l1pkval = 5;
		} else {
			if (hal->rf_type == RF_2T2R)
				l1pkval = 6;
			else
				l1pkval = 7;
		}

		rtw_write32_mask(rtwdev, REG_L1PKTH, 0x03C00000, l1pkval);

		break;
	}
}

static void r12_set_channel_rf(struct rtw_r12au_phy *rtwdev, u8 channel, u8 bw)
{
	(void)channel;
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

static void r12_set_channel(struct rtw_r12au_phy *rtwdev, u8 channel, u8 bw,
			  u8 primary_chan_idx)
{
	u8 old_band, new_band;

	if (rtw_read8(rtwdev, REG_CCK_CHECK) & BIT_CHECK_CCK_EN)
		old_band = RTW_BAND_5G;
	else
		old_band = RTW_BAND_2G;

	if (channel > 14)
		new_band = RTW_BAND_5G;
	else
		new_band = RTW_BAND_2G;

	if (new_band != old_band)
		r12_switch_band(rtwdev, new_band, bw);

	r12_switch_channel(rtwdev, channel, bw);

	r12_post_set_bw_mode(rtwdev, channel, bw, primary_chan_idx);

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8812A)
		r12_chip12_phy_fix_spur(rtwdev, channel, bw);

	r12_set_channel_rf(rtwdev, channel, bw);
}

static void r12_iqk_backup_mac_bb(struct rtw_r12au_phy *rtwdev,
				u32 *macbb_backup,
				const u32 *backup_macbb_reg,
				u32 macbb_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* save MACBB default value */
	for (i = 0; i < macbb_num; i++)
		macbb_backup[i] = rtw_read32(rtwdev, backup_macbb_reg[i]);
}

static void r12_iqk_backup_afe(struct rtw_r12au_phy *rtwdev, u32 *afe_backup,
			     const u32 *backup_afe_reg, u32 afe_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Save AFE Parameters */
	for (i = 0; i < afe_num; i++)
		afe_backup[i] = rtw_read32(rtwdev, backup_afe_reg[i]);
}

static void r12_iqk_restore_mac_bb(struct rtw_r12au_phy *rtwdev,
				 u32 *macbb_backup,
				 const u32 *backup_macbb_reg,
				 u32 macbb_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Reload MacBB Parameters */
	for (i = 0; i < macbb_num; i++)
		rtw_write32(rtwdev, backup_macbb_reg[i], macbb_backup[i]);
}

static void r12_iqk_configure_mac(struct rtw_r12au_phy *rtwdev)
{
	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	rtw_write8(rtwdev, REG_TXPAUSE, 0x3f);
	rtw_write32_mask(rtwdev, REG_BCN_CTRL,
			 (BIT_EN_BCN_FUNCTION << 8) | BIT_EN_BCN_FUNCTION, 0x0);

	/* RX ante off */
	rtw_write8(rtwdev, REG_RXPSEL, 0x00);

	/* CCA off */
	rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf, 0xc);

	/* CCK RX path off */
	rtw_write8(rtwdev, REG_CCK_RX + 3, 0xf);
}

static bool r12_iqk_finish(int average, int threshold,
			 int *x_temp, int *y_temp, int *x, int *y,
			 bool break_inner, bool break_outer)
{
	bool finish = false;
	int i, ii, dx, dy;

	for (i = 0; i < average; i++) {
		for (ii = i + 1; ii < average; ii++) {
			dx = abs_diff(x_temp[i] >> 21, x_temp[ii] >> 21);
			dy = abs_diff(y_temp[i] >> 21, y_temp[ii] >> 21);

			if (dx < threshold && dy < threshold) {
				*x = ((x_temp[i] >> 21) + (x_temp[ii] >> 21));
				*y = ((y_temp[i] >> 21) + (y_temp[ii] >> 21));

				*x /= 2;
				*y /= 2;

				finish = true;

				if (break_inner)
					break;
			}
		}

		if (finish && break_outer)
			break;
	}

	return finish;
}
static void r12_chip12_do_lck(struct rtw_r12au_phy *rtwdev)
{
	u32 cont_tx, lc_cal, i;
	u8 pause=rtw_read8(rtwdev,REG_TXPAUSE);
	u32 original_lck=rtw_read_rf(rtwdev,RF_PATH_A,RF_LCK,RFREG_MASK);
	bool cleanup=rtwdev->cleanup;

	cont_tx = rtw_read32_mask(rtwdev, REG_SINGLE_TONE_CONT_TX, 0x70000);

	lc_cal = rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK);
	if(rtwdev->io_error)return;

	if (!cont_tx)
		rtw_write8(rtwdev, REG_TXPAUSE, 0xff);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LCK, BIT(14), 1);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, 0x08000, 1);

	mdelay(150);

	for (i = 0; i < 5; i++) {
		if (rtw_read_rf(rtwdev, RF_PATH_A, RF_CFGCH, 0x08000) != 1)
			break;

		mdelay(10);
	}

	if (i == 5) rtwdev->cal_timeout=true;

	rtwdev->cleanup=true;
	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LCK, RFREG_MASK, original_lck);

	if (!cont_tx)
		rtw_write8(rtwdev, REG_TXPAUSE, pause);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, lc_cal);
	rtwdev->cleanup=cleanup;
}

static void r12_chip12_iqk_backup_rf(struct rtw_r12au_phy *rtwdev, u32 *rfa_backup,
				   u32 *rfb_backup, const u32 *backup_rf_reg,
				   u32 rf_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Save RF Parameters */
	for (i = 0; i < rf_num; i++) {
		rfa_backup[i] = rtw_read_rf(rtwdev, RF_PATH_A,
					    backup_rf_reg[i], MASKDWORD);
		rfb_backup[i] = rtw_read_rf(rtwdev, RF_PATH_B,
					    backup_rf_reg[i], MASKDWORD);
	}
}

static void r12_chip12_iqk_restore_rf(struct rtw_r12au_phy *rtwdev,
				    enum rtw_rf_path path,
				    const u32 *backup_rf_reg,
				    u32 *RF_backup, u32 rf_reg_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	for (i = 0; i < rf_reg_num; i++)
		rtw_write_rf(rtwdev, path, backup_rf_reg[i],
			     RFREG_MASK, RF_backup[i]);

	rtw_write_rf(rtwdev, path, RF_LUTWE, RFREG_MASK, 0);
}

static void r12_chip12_iqk_restore_afe(struct rtw_r12au_phy *rtwdev, u32 *afe_backup,
				     const u32 *backup_afe_reg, u32 afe_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Reload AFE Parameters */
	for (i = 0; i < afe_num; i++)
		rtw_write32(rtwdev, backup_afe_reg[i], afe_backup[i]);

	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x0);
	rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x0);
	rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x0);
	rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x3c000000);
	rtw_write32_mask(rtwdev, REG_LSSI_WRITE_A, BIT(7), 1);
	rtw_write32_mask(rtwdev, REG_IQK_DPD_CFG, BIT(18), 1);
	rtw_write32_mask(rtwdev, REG_IQK_DPD_CFG, BIT(29), 1);
	rtw_write32_mask(rtwdev, REG_CFG_PMPD, BIT(29), 1);

	rtw_write32(rtwdev, REG_TXTONEB, 0x0);
	rtw_write32(rtwdev, REG_RXTONEB, 0x0);
	rtw_write32(rtwdev, REG_TXPITMB, 0x0);
	rtw_write32(rtwdev, REG_RXPITMB, 0x3c000000);
	rtw_write32_mask(rtwdev, REG_LSSI_WRITE_B, BIT(7), 1);
	rtw_write32_mask(rtwdev, REG_BPBDB, BIT(18), 1);
	rtw_write32_mask(rtwdev, REG_BPBDB, BIT(29), 1);
	rtw_write32_mask(rtwdev, REG_PHYTXONB, BIT(29), 1);
}

static void r12_chip12_iqk_rx_fill(struct rtw_r12au_phy *rtwdev, enum rtw_rf_path path,
				 unsigned int rx_x, unsigned int rx_y)
{
	switch (path) {
	case RF_PATH_A:
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		if (rx_x >> 1 >= 0x112 ||
		    (rx_y >> 1 >= 0x12 && rx_y >> 1 <= 0x3ee)) {
		rtwdev->rx_iqk_mask &= (u8)~BIT(path);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
					 0x000003ff, 0x100);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
					 0x03ff0000, 0);
		} else {
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
					 0x000003ff, rx_x >> 1);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
					 0x03ff0000, rx_y >> 1);
		}
		(void)0;
		(void)0;
		break;
	case RF_PATH_B:
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		if (rx_x >> 1 >= 0x112 ||
		    (rx_y >> 1 >= 0x12 && rx_y >> 1 <= 0x3ee)) {
		rtwdev->rx_iqk_mask &= (u8)~BIT(path);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_B,
					 0x000003ff, 0x100);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_B,
					 0x03ff0000, 0);
		} else {
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_B,
					 0x000003ff, rx_x >> 1);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_B,
					 0x03ff0000, rx_y >> 1);
		}
		(void)0;
		(void)0;
		break;
	default:
		break;
	}
}

static void r12_chip12_iqk_tx_fill(struct rtw_r12au_phy *rtwdev, enum rtw_rf_path path,
				 unsigned int tx_x, unsigned int tx_y)
{
	switch (path) {
	case RF_PATH_A:
		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);
		rtw_write32_mask(rtwdev, REG_PREDISTA, BIT(7), 0x1);
		rtw_write32_mask(rtwdev, REG_IQK_DPD_CFG, BIT(18), 0x1);
		rtw_write32_mask(rtwdev, REG_IQK_DPD_CFG, BIT(29), 0x1);
		rtw_write32_mask(rtwdev, REG_CFG_PMPD, BIT(29), 0x1);
		rtw_write32_mask(rtwdev, REG_IQC_Y, 0x000007ff, tx_y);
		rtw_write32_mask(rtwdev, REG_IQC_X, 0x000007ff, tx_x);
		(void)0;
		(void)0;
		break;
	case RF_PATH_B:
		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);
		rtw_write32_mask(rtwdev, REG_PREDISTB, BIT(7), 0x1);
		rtw_write32_mask(rtwdev, REG_BPBDB, BIT(18), 0x1);
		rtw_write32_mask(rtwdev, REG_BPBDB, BIT(29), 0x1);
		rtw_write32_mask(rtwdev, REG_PHYTXONB, BIT(29), 0x1);
		rtw_write32_mask(rtwdev, REG_IQKYB, 0x000007ff, tx_y);
		rtw_write32_mask(rtwdev, REG_IQKXB, 0x000007ff, tx_x);
		(void)0;
		(void)0;
		break;
	default:
		break;
	}
}

static void r12_chip12_iqk(struct rtw_r12au_phy *rtwdev)
{
	int tx_x0_temp[10], tx_y0_temp[10], tx_x1_temp[10], tx_y1_temp[10];
	int rx_x0_temp[10], rx_y0_temp[10], rx_x1_temp[10], rx_y1_temp[10];
	const bool has_path_b = rtwdev->hal.rf_phy_num > 1;
	bool iqk0_ready = false, tx0_finish = false, rx0_finish = false;
	bool iqk1_ready = false, tx1_finish = false, rx1_finish = false;
	u8 tx0_avg = 0, tx1_avg = 0, rx0_avg = 0, rx1_avg = 0;
	int tx_x0 = 0, tx_y0 = 0, tx_x1 = 0, tx_y1 = 0;
	int rx_x0 = 0, rx_y0 = 0, rx_x1 = 0, rx_y1 = 0;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	bool tx0_fail = true, rx0_fail = true;
	bool tx1_fail = true, rx1_fail = true;
	u8 cal0_retry, cal1_retry;
	u8 delay_count;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* ========path-A AFE all on======== */
	/* Port 0 DAC/ADC on */
	rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x77777777);
	rtw_write32(rtwdev, REG_AFE_PWR2_A, 0x77777777);

	/* Port 1 DAC/ADC on */
	rtw_write32(rtwdev, REG_AFE_PWR1_B, 0x77777777);
	rtw_write32(rtwdev, REG_AFE_PWR2_B, 0x77777777);

	rtw_write32(rtwdev, REG_RX_WAIT_CCA_TX_CCK_RFON_A, 0x19791979);
	rtw_write32(rtwdev, REG_RX_WAIT_CCA_TX_CCK_RFON_B, 0x19791979);

	/* hardware 3-wire off */
	rtw_write32_mask(rtwdev, REG_3WIRE_SWA, 0xf, 0x4);
	rtw_write32_mask(rtwdev, REG_3WIRE_SWB, 0xf, 0x4);

	/* DAC/ADC sampling rate (160 MHz) */
	rtw_write32_mask(rtwdev, REG_CK_MONHA, GENMASK(26, 24), 0x7);
	rtw_write32_mask(rtwdev, REG_CK_MONHB, GENMASK(26, 24), 0x7);

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
	/* ====== path A TX IQK RF setting ====== */
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80002);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x20000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x3fffd);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xfe83f);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_TXA_PREPAD, RFREG_MASK, 0x931d5);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_RXBB2, RFREG_MASK, 0x8a001);

	/* ====== path B TX IQK RF setting ====== */
	rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWE, RFREG_MASK, 0x80002);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_MODE_TABLE_ADDR, RFREG_MASK, 0x20000);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_MODE_TABLE_DATA0, RFREG_MASK, 0x3fffd);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_MODE_TABLE_DATA1, RFREG_MASK, 0xfe83f);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_TXA_PREPAD, RFREG_MASK, 0x931d5);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_RXBB2, RFREG_MASK, 0x8a001);

	rtw_write32(rtwdev, REG_DAC_RSTB, 0x00008000);
	rtw_write32_mask(rtwdev, REG_TXAGCIDX, BIT(0), 0x1);
	rtw_write32_mask(rtwdev, REG_INIDLYB, BIT(0), 0x1);
	rtw_write32(rtwdev, REG_IQK_COM00, 0x29002000); /* TX (X,Y) */
	rtw_write32(rtwdev, REG_IQK_COM32, 0xa9002000); /* RX (X,Y) */
	rtw_write32(rtwdev, REG_IQK_COM96, 0x00462910); /* [0]:AGC_en, [15]:idac_K_Mask */
	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	if (efuse->ext_pa_5g) {
		if (efuse->rfe_option == 1) {
			rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x821403e3);
			rtw_write32(rtwdev, REG_TXPITMB, 0x821403e3);
		} else {
			rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x821403f7);
			rtw_write32(rtwdev, REG_TXPITMB, 0x821403f7);
		}
	} else {
		rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x821403f1);
		rtw_write32(rtwdev, REG_TXPITMB, 0x821403f1);
	}

	if (rtwdev->hal.current_band_type == RTW_BAND_5G) {
		rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x68163e96);
		rtw_write32(rtwdev, REG_RXPITMB, 0x68163e96);
	} else {
		rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x28163e96);
		rtw_write32(rtwdev, REG_RXPITMB, 0x28163e96);

		if (efuse->rfe_option == 3) {
			if (efuse->ext_pa_2g)
				rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE,
					    0x821403e3);
			else
				rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE,
					    0x821403f7);
		}
	}

	/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
	rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x18008c10);
	/* RX_Tone_idx[9:0], RxK_Mask[29] */
	rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x38008c10);
	rtw_write32(rtwdev, REG_INTPO_SETA, 0x00000000);
	/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
	rtw_write32(rtwdev, REG_TXTONEB, 0x18008c10);
	/* RX_Tone_idx[9:0], RxK_Mask[29] */
	rtw_write32(rtwdev, REG_RXTONEB, 0x38008c10);
	rtw_write32(rtwdev, REG_INTPO_SETB, 0x00000000);

	cal0_retry = 0;
	cal1_retry = 0;
	while (1) {
		/* one shot */
		rtw_write32(rtwdev, REG_RFECTL_A, 0x00100000);
		rtw_write32(rtwdev, REG_RFECTL_B, 0x00100000);
		rtw_write32(rtwdev, REG_IQK_COM64, 0xfa000000);
		rtw_write32(rtwdev, REG_IQK_COM64, 0xf8000000);

		mdelay(10);

		rtw_write32(rtwdev, REG_RFECTL_A, 0x00000000);
		rtw_write32(rtwdev, REG_RFECTL_B, 0x00000000);

		for (delay_count = 0; delay_count < 20; delay_count++) {
			if (!tx0_finish)
				iqk0_ready = rtw_read32_mask(rtwdev,
							     REG_IQKA_END,
							     BIT(10));
			if (has_path_b && !tx1_finish)
				iqk1_ready = rtw_read32_mask(rtwdev,
							     REG_IQKB_END,
							     BIT(10));
			if (iqk0_ready && (!has_path_b || iqk1_ready))
				break;

			mdelay(1);
		}

		(void)0;

		if (delay_count < 20) { /* If 20ms No Result, then cal_retry++ */
			/* ============TXIQK Check============== */
			tx0_fail = rtw_read32_mask(rtwdev, REG_IQKA_END, BIT(12));
			if (has_path_b) tx1_fail = rtw_read32_mask(rtwdev, REG_IQKB_END, BIT(12));

			if (!(tx0_fail || tx0_finish)) {
				rtw_write32(rtwdev, REG_RFECTL_A, 0x02000000);
				tx_x0_temp[tx0_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKA_END,
								      0x07ff0000);
				rtw_write32(rtwdev, REG_RFECTL_A, 0x04000000);
				tx_y0_temp[tx0_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKA_END,
								      0x07ff0000);

				(void)0;

				tx_x0_temp[tx0_avg] = (int32_t)((uint32_t)tx_x0_temp[tx0_avg] << 21);
				tx_y0_temp[tx0_avg] = (int32_t)((uint32_t)tx_y0_temp[tx0_avg] << 21);

				tx0_avg++;
			} else {
				cal0_retry++;
				if (cal0_retry == 10)
					break;
			}

			if (has_path_b && !(tx1_fail || tx1_finish)) {
				rtw_write32(rtwdev, REG_RFECTL_B, 0x02000000);
				tx_x1_temp[tx1_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKB_END,
								      0x07ff0000);
				rtw_write32(rtwdev, REG_RFECTL_B, 0x04000000);
				tx_y1_temp[tx1_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKB_END,
								      0x07ff0000);

				(void)0;

				tx_x1_temp[tx1_avg] = (int32_t)((uint32_t)tx_x1_temp[tx1_avg] << 21);
				tx_y1_temp[tx1_avg] = (int32_t)((uint32_t)tx_y1_temp[tx1_avg] << 21);

				tx1_avg++;
			} else {
				cal1_retry++;
				if (cal1_retry == 10)
					break;
			}
		} else {
			cal0_retry++;
			cal1_retry++;

			(void)0;

			if (cal0_retry == 10)
				break;
		}

		if (tx0_avg >= 2)
			tx0_finish = r12_iqk_finish(tx0_avg, 4,
							 tx_x0_temp, tx_y0_temp, &tx_x0, &tx_y0,
							 false, false);

		if (tx1_avg >= 2)
			tx1_finish = r12_iqk_finish(tx1_avg, 4,
							 tx_x1_temp, tx_y1_temp, &tx_x1, &tx_y1,
							 false, false);

		(void)0;
		(void)0;

		if (tx0_finish && (!has_path_b || tx1_finish))
			break;

		if ((cal0_retry + tx0_avg) >= 10 ||
		    (cal1_retry + tx1_avg) >= 10)
			break;
	}

	(void)0;
	(void)0;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
	/* Load LOK */
	rtw_write_rf(rtwdev, RF_PATH_A, RF_TXMOD, 0x7fe00,
		     rtw_read_rf(rtwdev, RF_PATH_A, RF_DTXLOK, 0xffc00));
	rtw_write_rf(rtwdev, RF_PATH_B, RF_TXMOD, 0x7fe00,
		     rtw_read_rf(rtwdev, RF_PATH_B, RF_DTXLOK, 0xffc00));
	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
	if (tx0_finish) {
		/* ====== path A RX IQK RF setting====== */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK,
			     0x30000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK,
			     0x3f7ff);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK,
			     0xfe7bf);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RXBB2, RFREG_MASK, 0x88001);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_TXA_PREPAD, RFREG_MASK, 0x931d1);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x00000);
	}
	if (tx1_finish) {
		/* ====== path B RX IQK RF setting====== */
		rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_MODE_TABLE_ADDR, RFREG_MASK,
			     0x30000);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_MODE_TABLE_DATA0, RFREG_MASK,
			     0x3f7ff);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_MODE_TABLE_DATA1, RFREG_MASK,
			     0xfe7bf);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RXBB2, RFREG_MASK, 0x88001);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_TXA_PREPAD, RFREG_MASK, 0x931d1);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWE, RFREG_MASK, 0x00000);
	}

	rtw_write32_mask(rtwdev, REG_IQK_COM00, BIT(31), 0x1);
	rtw_write32_mask(rtwdev, REG_IQK_COM00, BIT(31), 0x0);
	rtw_write32(rtwdev, REG_DAC_RSTB, 0x00008000);

	if (rtwdev->hci.type == RTW_HCI_TYPE_PCIE)
		rtw_write32(rtwdev, REG_IQK_COM96, 0x0046a911);
	else
		rtw_write32(rtwdev, REG_IQK_COM96, 0x0046a890);

	if (efuse->rfe_option == 1) {
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777717);
		rtw_write32(rtwdev, REG_RFE_INV_A, 0x00000077);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777717);
		rtw_write32(rtwdev, REG_RFE_INV_B, 0x00000077);
	} else {
		rtw_write32(rtwdev, REG_RFE_PINMUX_A, 0x77777717);
		rtw_write32(rtwdev, REG_RFE_INV_A, 0x02000077);
		rtw_write32(rtwdev, REG_RFE_PINMUX_B, 0x77777717);
		rtw_write32(rtwdev, REG_RFE_INV_B, 0x02000077);
	}

	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	if (tx0_finish) {
		/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
		rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x38008c10);
		/* RX_Tone_idx[9:0], RxK_Mask[29] */
		rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x18008c10);
		rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x82140119);
	}
	if (tx1_finish) {
		/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
		rtw_write32(rtwdev, REG_TXTONEB, 0x38008c10);
		/* RX_Tone_idx[9:0], RxK_Mask[29] */
		rtw_write32(rtwdev, REG_RXTONEB, 0x18008c10);
		rtw_write32(rtwdev, REG_TXPITMB, 0x82140119);
	}

	cal0_retry = 0;
	cal1_retry = 0;
	while (1) {
		/* one shot */
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

		if (tx0_finish) {
			rtw_write32_mask(rtwdev, REG_IQK_COM00, 0x03FF8000,
					 tx_x0 & 0x000007ff);
			rtw_write32_mask(rtwdev, REG_IQK_COM00, 0x000007FF,
					 tx_y0 & 0x000007ff);
			/* [31] = 1 --> Page C1 */
			rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);
			if (efuse->rfe_option == 1)
				rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x28161500);
			else
				rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x28160cc0);
			rtw_write32(rtwdev, REG_RFECTL_A, 0x00300000);
			rtw_write32(rtwdev, REG_RFECTL_A, 0x00100000);
			mdelay(5);
			rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x3c000000);
			rtw_write32(rtwdev, REG_RFECTL_A, 0x00000000);
		}

		if (tx1_finish) {
			/* [31] = 0 --> Page C */
			rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
			rtw_write32_mask(rtwdev, REG_IQK_COM00, 0x03FF8000,
					 tx_x1 & 0x000007ff);
			rtw_write32_mask(rtwdev, REG_IQK_COM00, 0x000007FF,
					 tx_y1 & 0x000007ff);
			/* [31] = 1 --> Page C1 */
			rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);
			if (efuse->rfe_option == 1)
				rtw_write32(rtwdev, REG_RXPITMB, 0x28161500);
			else
				rtw_write32(rtwdev, REG_RXPITMB, 0x28160ca0);
			rtw_write32(rtwdev, REG_RFECTL_B, 0x00300000);
			rtw_write32(rtwdev, REG_RFECTL_B, 0x00100000);
			mdelay(5);
			rtw_write32(rtwdev, REG_RXPITMB, 0x3c000000);
			rtw_write32(rtwdev, REG_RFECTL_B, 0x00000000);
		}

		for (delay_count = 0; delay_count < 20; delay_count++) {
			if (!rx0_finish && tx0_finish)
				iqk0_ready = rtw_read32_mask(rtwdev,
							     REG_IQKA_END,
							     BIT(10));
			if (has_path_b && !rx1_finish && tx1_finish)
				iqk1_ready = rtw_read32_mask(rtwdev,
							     REG_IQKB_END,
							     BIT(10));
			if (iqk0_ready && (!has_path_b || iqk1_ready))
				break;

			mdelay(1);
		}

		(void)0;

		if (delay_count < 20) { /* If 20ms No Result, then cal_retry++ */
			/* ============RXIQK Check============== */
			rx0_fail = rtw_read32_mask(rtwdev, REG_IQKA_END, BIT(11));
			if (has_path_b) rx1_fail = rtw_read32_mask(rtwdev, REG_IQKB_END, BIT(11));

			if (!(rx0_fail || rx0_finish) && tx0_finish) {
				rtw_write32(rtwdev, REG_RFECTL_A, 0x06000000);
				rx_x0_temp[rx0_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKA_END,
								      0x07ff0000);
				rtw_write32(rtwdev, REG_RFECTL_A, 0x08000000);
				rx_y0_temp[rx0_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKA_END,
								      0x07ff0000);

				(void)0;

				rx_x0_temp[rx0_avg] = (int32_t)((uint32_t)rx_x0_temp[rx0_avg] << 21);
				rx_y0_temp[rx0_avg] = (int32_t)((uint32_t)rx_y0_temp[rx0_avg] << 21);

				rx0_avg++;
			} else {
				(void)0;

				cal0_retry++;
				if (cal0_retry == 10)
					break;
			}

			if (has_path_b && !(rx1_fail || rx1_finish) && tx1_finish) {
				rtw_write32(rtwdev, REG_RFECTL_B, 0x06000000);
				rx_x1_temp[rx1_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKB_END,
								      0x07ff0000);
				rtw_write32(rtwdev, REG_RFECTL_B, 0x08000000);
				rx_y1_temp[rx1_avg] = (int)rtw_read32_mask(rtwdev,
								      REG_IQKB_END,
								      0x07ff0000);

				(void)0;

				rx_x1_temp[rx1_avg] = (int32_t)((uint32_t)rx_x1_temp[rx1_avg] << 21);
				rx_y1_temp[rx1_avg] = (int32_t)((uint32_t)rx_y1_temp[rx1_avg] << 21);

				rx1_avg++;
			} else {
				cal1_retry++;
				if (cal1_retry == 10)
					break;
			}
		} else {
			(void)0;

			cal0_retry++;
			cal1_retry++;

			(void)0;

			if (cal0_retry == 10)
				break;
		}

		(void)0;

		if (rx0_avg >= 2)
			rx0_finish = r12_iqk_finish(rx0_avg, 4,
							 rx_x0_temp, rx_y0_temp,
							 &rx_x0, &rx_y0,
							 true, false);

		if (rx1_avg >= 2)
			rx1_finish = r12_iqk_finish(rx1_avg, 4,
							 rx_x1_temp, rx_y1_temp,
							 &rx_x1, &rx_y1,
							 true, false);

		(void)0;
		(void)0;

		if ((rx0_finish || !tx0_finish) && (rx1_finish || !tx1_finish))
			break;

		if ((cal0_retry + rx0_avg) >= 10 ||
		    (cal1_retry + rx1_avg) >= 10 ||
		    rx0_avg == 3 || rx1_avg == 3)
			break;
	}

	(void)0;
	(void)0;

	rtwdev->tx_iqk_mask = (tx0_finish ? 1U : 0U) | (tx1_finish ? 2U : 0U);
	rtwdev->rx_iqk_mask = (rx0_finish ? 1U : 0U) | (rx1_finish ? 2U : 0U);
	/* FillIQK Result */
	(void)0;

	if (tx0_finish)
		r12_chip12_iqk_tx_fill(rtwdev, RF_PATH_A, (u32)tx_x0, (u32)tx_y0);
	else
		r12_chip12_iqk_tx_fill(rtwdev, RF_PATH_A, 0x200, 0x0);

	if (rx0_finish)
		r12_chip12_iqk_rx_fill(rtwdev, RF_PATH_A, (u32)rx_x0, (u32)rx_y0);
	else
		r12_chip12_iqk_rx_fill(rtwdev, RF_PATH_A, 0x200, 0x0);

	(void)0;

	if (tx1_finish)
		r12_chip12_iqk_tx_fill(rtwdev, RF_PATH_B, (u32)tx_x1, (u32)tx_y1);
	else
		r12_chip12_iqk_tx_fill(rtwdev, RF_PATH_B, 0x200, 0x0);

	if (rx1_finish)
		r12_chip12_iqk_rx_fill(rtwdev, RF_PATH_B, (u32)rx_x1, (u32)rx_y1);
	else
		r12_chip12_iqk_rx_fill(rtwdev, RF_PATH_B, 0x200, 0x0);
}

static void r12_chip12_do_iqk(struct rtw_r12au_phy *rtwdev)
{
	static const u32 backup_macbb_reg[MACBB_REG_NUM_8812A] = {
		0x520, 0x550, 0x808, 0xa04, 0x90c, 0xc00, 0xe00, 0x838, 0x82c
	};
	static const u32 backup_afe_reg[AFE_REG_NUM_8812A] = {
		0xc5c, 0xc60, 0xc64, 0xc68, 0xcb0, 0xcb4,
		0xe5c, 0xe60, 0xe64, 0xe68, 0xeb0, 0xeb4
	};
	static const u32 backup_rf_reg[RF_REG_NUM_8812A] = {
		0x65, 0x8f, 0x0
	};
	u32 macbb_backup[MACBB_REG_NUM_8812A] = {0};
	u32 afe_backup[AFE_REG_NUM_8812A] = {0};
	u32 rfa_backup[RF_REG_NUM_8812A] = {0};
	u32 rfb_backup[RF_REG_NUM_8812A] = {0};
	u32 reg_cb8, reg_eb8;

	r12_iqk_backup_mac_bb(rtwdev, macbb_backup,
				   backup_macbb_reg, MACBB_REG_NUM_8812A);

	rtw_write32_set(rtwdev, REG_CCASEL, BIT(31));
	reg_cb8 = rtw_read32(rtwdev, REG_RFECTL_A);
	reg_eb8 = rtw_read32(rtwdev, REG_RFECTL_B);
	rtw_write32_clr(rtwdev, REG_CCASEL, BIT(31));

	r12_iqk_backup_afe(rtwdev, afe_backup,
				backup_afe_reg, AFE_REG_NUM_8812A);
	r12_chip12_iqk_backup_rf(rtwdev, rfa_backup, rfb_backup,
			       backup_rf_reg, RF_REG_NUM_8812A);

	if (rtwdev->io_error) return;
	r12_iqk_configure_mac(rtwdev);

	r12_chip12_iqk(rtwdev);
	rtwdev->cleanup=true;

	r12_chip12_iqk_restore_rf(rtwdev, RF_PATH_A, backup_rf_reg,
				rfa_backup, RF_REG_NUM_8812A);
	r12_chip12_iqk_restore_rf(rtwdev, RF_PATH_B, backup_rf_reg,
				rfb_backup, RF_REG_NUM_8812A);

	r12_chip12_iqk_restore_afe(rtwdev, afe_backup,
				 backup_afe_reg, AFE_REG_NUM_8812A);

	rtw_write32_set(rtwdev, REG_CCASEL, BIT(31));
	rtw_write32(rtwdev, REG_RFECTL_A, reg_cb8);
	rtw_write32(rtwdev, REG_RFECTL_B, reg_eb8);
	rtw_write32_clr(rtwdev, REG_CCASEL, BIT(31));

	r12_iqk_restore_mac_bb(rtwdev, macbb_backup,
				    backup_macbb_reg, MACBB_REG_NUM_8812A);
	rtwdev->cleanup=false;
}
static void r12_chip21_iqk_backup_rf(struct rtw_r12au_phy *rtwdev, u32 *rfa_backup,
				   const u32 *backup_rf_reg, u32 rf_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Save RF Parameters */
	for (i = 0; i < rf_num; i++)
		rfa_backup[i] = rtw_read_rf(rtwdev, RF_PATH_A,
					    backup_rf_reg[i], MASKDWORD);
}

static void r12_chip21_iqk_restore_rf(struct rtw_r12au_phy *rtwdev,
				    const u32 *backup_rf_reg,
				    u32 *RF_backup, u32 rf_reg_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	for (i = 0; i < rf_reg_num; i++)
		rtw_write_rf(rtwdev, RF_PATH_A, backup_rf_reg[i],
			     RFREG_MASK, RF_backup[i]);
}

static void r12_chip21_iqk_restore_afe(struct rtw_r12au_phy *rtwdev, u32 *afe_backup,
				     const u32 *backup_afe_reg, u32 afe_num)
{
	u32 i;

	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	/* Reload AFE Parameters */
	for (i = 0; i < afe_num; i++)
		rtw_write32(rtwdev, backup_afe_reg[i], afe_backup[i]);

	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x0);
	rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x0);
	rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x0);
	rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x3c000000);
	rtw_write32(rtwdev, REG_LSSI_WRITE_A, 0x00000080);
	rtw_write32(rtwdev, REG_TXAGCIDX, 0x00000000);
	rtw_write32(rtwdev, REG_IQK_DPD_CFG, 0x20040000);
	rtw_write32(rtwdev, REG_CFG_PMPD, 0x20000000);
	rtw_write32(rtwdev, REG_RFECTL_A, 0x0);
}

static void r12_chip21_iqk_rx_fill(struct rtw_r12au_phy *rtwdev,
				 unsigned int rx_x, unsigned int rx_y)
{
	/* [31] = 0 --> Page C */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

	rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
			 0x000003ff, rx_x >> 1);
	rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
			 0x03ff0000, (rx_y >> 1) & 0x3ff);
}

static void r12_chip21_iqk_tx_fill(struct rtw_r12au_phy *rtwdev,
				 unsigned int tx_x, unsigned int tx_y)
{
	/* [31] = 1 --> Page C1 */
	rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

	rtw_write32(rtwdev, REG_LSSI_WRITE_A, 0x00000080);
	rtw_write32(rtwdev, REG_IQK_DPD_CFG, 0x20040000);
	rtw_write32(rtwdev, REG_CFG_PMPD, 0x20000000);
	rtw_write32_mask(rtwdev, REG_IQC_Y, 0x000007ff, tx_y);
	rtw_write32_mask(rtwdev, REG_IQC_X, 0x000007ff, tx_x);
}

static void r12_chip21_iqk_tx_vdf_true(struct rtw_r12au_phy *rtwdev, u32 cal,
				     bool *tx0iqkok,
				     int tx_x0[CAL_NUM_8821A],
				     int tx_y0[CAL_NUM_8821A])
{
	u32 cal_retry, delay_count, iqk_ready, tx_fail;
	int tx_dt[3], vdf_y[3], vdf_x[3];
	int k;

	for (k = 0; k < 3; k++) {
		switch (k) {
		case 0:
			/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
			rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE,
				    0x18008c38);
			/* RX_Tone_idx[9:0], RxK_Mask[29] */
			rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x38008c38);
			rtw_write32_mask(rtwdev, REG_INTPO_SETA, BIT(31), 0x0);
			break;
		case 1:
			rtw_write32_mask(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE,
					 BIT(28), 0x0);
			rtw_write32_mask(rtwdev, REG_OFDM0_A_TX_AFE,
					 BIT(28), 0x0);
			rtw_write32_mask(rtwdev, REG_INTPO_SETA, BIT(31), 0x0);
			break;
		case 2:
			(void)0;

			(void)0;

			tx_dt[cal] = (vdf_y[1] >> 20) - (vdf_y[0] >> 20);
			tx_dt[cal] = (16 * tx_dt[cal]) * 10000 / 15708;
			tx_dt[cal] = (tx_dt[cal] >> 1) + (tx_dt[cal] & 1);

			/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
			rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE,
				    0x18008c20);
			/* RX_Tone_idx[9:0], RxK_Mask[29] */
			rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x38008c20);
			rtw_write32_mask(rtwdev, REG_INTPO_SETA, BIT(31), 0x1);
			rtw_write32_mask(rtwdev, REG_INTPO_SETA, 0x3fff0000,
					 tx_dt[cal] & 0x00003fff);
			break;
		}

		rtw_write32(rtwdev, REG_RFECTL_A, 0x00100000);

		for (cal_retry = 0; cal_retry < 10; cal_retry++) {
			/* one shot */
			rtw_write32(rtwdev, REG_IQK_COM64, 0xfa000000);
			rtw_write32(rtwdev, REG_IQK_COM64, 0xf8000000);

			mdelay(10);

			rtw_write32(rtwdev, REG_RFECTL_A, 0x00000000);

			for (delay_count = 0; delay_count < 20; delay_count++) {
				iqk_ready = rtw_read32_mask(rtwdev,
							    REG_IQKA_END,
							    BIT(10));

				/* Originally: if (~iqk_ready || delay_count > 20)
				 * that looks like a typo so make it more explicit
				 */
				/* Measured IQKready bit is required. */

				if (iqk_ready)
					break;

				mdelay(1);
			}

			if (delay_count < 20) {
				/* ============TXIQK Check============== */
				tx_fail = rtw_read32_mask(rtwdev,
							  REG_IQKA_END,
							  BIT(12));

				/* Originally: if (~tx_fail) {
				 * It looks like a typo, so make it more explicit.
				 */
				/* Measured TXfailure bit is authoritative. */

				if (!tx_fail) {
					rtw_write32(rtwdev, REG_RFECTL_A,
						    0x02000000);
					vdf_x[k] = (int)rtw_read32_mask(rtwdev,
								   REG_IQKA_END,
								   0x07ff0000);
					vdf_x[k] = (int32_t)((uint32_t)vdf_x[k] << 21);

					rtw_write32(rtwdev, REG_RFECTL_A,
						    0x04000000);
					vdf_y[k] = (int)rtw_read32_mask(rtwdev,
								   REG_IQKA_END,
								   0x07ff0000);
					vdf_y[k] = (int32_t)((uint32_t)vdf_y[k] << 21);

					*tx0iqkok = true;
					break;
				}

				rtw_write32_mask(rtwdev, REG_IQC_Y,
						 0x000007ff, 0x0);
				rtw_write32_mask(rtwdev, REG_IQC_X,
						 0x000007ff, 0x200);
			}

			*tx0iqkok = false;
		}
	}

	if (k == 3) {
		tx_x0[cal] = vdf_x[k - 1];
		tx_y0[cal] = vdf_y[k - 1];
	}
}

static void r12_chip21_iqk_tx_vdf_false(struct rtw_r12au_phy *rtwdev, u32 cal,
				      bool *tx0iqkok,
				      int tx_x0[CAL_NUM_8821A],
				      int tx_y0[CAL_NUM_8821A])
{
	u32 cal_retry, delay_count, iqk_ready, tx_fail;

	/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
	rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x18008c10);
	/* RX_Tone_idx[9:0], RxK_Mask[29] */
	rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x38008c10);
	rtw_write32(rtwdev, REG_RFECTL_A, 0x00100000);

	for (cal_retry = 0; cal_retry < 10; cal_retry++) {
		/* one shot */
		rtw_write32(rtwdev, REG_IQK_COM64, 0xfa000000);
		rtw_write32(rtwdev, REG_IQK_COM64, 0xf8000000);

		mdelay(10);
		rtw_write32(rtwdev, REG_RFECTL_A, 0x00000000);

		for (delay_count = 0; delay_count < 20; delay_count++) {
			iqk_ready = rtw_read32_mask(rtwdev, REG_IQKA_END, BIT(10));

			/* Originally: if (~iqk_ready || delay_count > 20)
			 * that looks like a typo so make it more explicit
			 */
			/* Measured IQKready bit is required. */

			if (iqk_ready)
				break;

			mdelay(1);
		}

		if (delay_count < 20) {
			/* ============TXIQK Check============== */
			tx_fail = rtw_read32_mask(rtwdev, REG_IQKA_END, BIT(12));

			/* Originally: if (~tx_fail) {
			 * It looks like a typo, so make it more explicit.
			 */
			/* Measured TXfailure bit is authoritative. */

			if (!tx_fail) {
				rtw_write32(rtwdev, REG_RFECTL_A, 0x02000000);
				tx_x0[cal] = (int)rtw_read32_mask(rtwdev, REG_IQKA_END,
							     0x07ff0000);
				tx_x0[cal] = (int32_t)((uint32_t)tx_x0[cal] << 21);

				rtw_write32(rtwdev, REG_RFECTL_A, 0x04000000);
				tx_y0[cal] = (int)rtw_read32_mask(rtwdev, REG_IQKA_END,
							     0x07ff0000);
				tx_y0[cal] = (int32_t)((uint32_t)tx_y0[cal] << 21);

				*tx0iqkok = true;
				break;
			}

			rtw_write32_mask(rtwdev, REG_IQC_Y, 0x000007ff, 0x0);
			rtw_write32_mask(rtwdev, REG_IQC_X, 0x000007ff, 0x200);
		}

		*tx0iqkok = false;
	}
}

static void r12_chip21_iqk_rx(struct rtw_r12au_phy *rtwdev, u32 cal, bool *rx0iqkok,
			    int rx_x0[CAL_NUM_8821A],
			    int rx_y0[CAL_NUM_8821A])
{
	u32 cal_retry, delay_count, iqk_ready, rx_fail;

	rtw_write32(rtwdev, REG_RFECTL_A, 0x00100000);

	for (cal_retry = 0; cal_retry < 10; cal_retry++) {
		/* one shot */
		rtw_write32(rtwdev, REG_IQK_COM64, 0xfa000000);
		rtw_write32(rtwdev, REG_IQK_COM64, 0xf8000000);

		mdelay(10);

		rtw_write32(rtwdev, REG_RFECTL_A, 0x00000000);

		for (delay_count = 0; delay_count < 20; delay_count++) {
			iqk_ready = rtw_read32_mask(rtwdev, REG_IQKA_END, BIT(10));

			/* Originally: if (~iqk_ready || delay_count > 20)
			 * that looks like a typo so make it more explicit
			 */
			/* Measured IQKready bit is required. */

			if (iqk_ready)
				break;

			mdelay(1);
		}

		if (delay_count < 20) {
			/* ============RXIQK Check============== */
			rx_fail = rtw_read32_mask(rtwdev, REG_IQKA_END, BIT(11));
			if (!rx_fail) {
				rtw_write32(rtwdev, REG_RFECTL_A, 0x06000000);
				rx_x0[cal] = (int)rtw_read32_mask(rtwdev, REG_IQKA_END,
							     0x07ff0000);
				rx_x0[cal] = (int32_t)((uint32_t)rx_x0[cal] << 21);

				rtw_write32(rtwdev, REG_RFECTL_A, 0x08000000);
				rx_y0[cal] = (int)rtw_read32_mask(rtwdev, REG_IQKA_END,
							     0x07ff0000);
				rx_y0[cal] = (int32_t)((uint32_t)rx_y0[cal] << 21);

				*rx0iqkok = true;
				break;
			}

			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
					 0x000003ff, 0x200 >> 1);
			rtw_write32_mask(rtwdev, REG_RX_IQC_AB_A,
					 0x03ff0000, 0x0 >> 1);
		}

		*rx0iqkok = false;
	}
}

static void r12_chip21_iqk(struct rtw_r12au_phy *rtwdev)
{
	int tx_average = 0, rx_average = 0, rx_iqk_loop = 0;
	const struct rtw_efuse *efuse = &rtwdev->efuse;
	int tx_x = 0, tx_y = 0, rx_x = 0, rx_y = 0;
	const struct rtw_hal *hal = &rtwdev->hal;
	bool tx0iqkok = false, rx0iqkok = false;
	int rx_x_temp = 0, rx_y_temp = 0;
	int rx_x0[2][CAL_NUM_8821A];
	int rx_y0[2][CAL_NUM_8821A];
	int tx_x0[CAL_NUM_8821A];
	int tx_y0[CAL_NUM_8821A];
	bool rx_finish1 = false;
	bool rx_finish2 = false;
	bool vdf_enable;
	u32 cal;
	int i;

	(void)0;

	vdf_enable = hal->current_band_width == RTW_CHANNEL_WIDTH_80;

	for (cal = 0; cal < CAL_NUM_8821A; cal++) {
		/* path-A LOK */

		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);

		/* ========path-A AFE all on======== */
		/* Port 0 DAC/ADC on */
		rtw_write32(rtwdev, REG_AFE_PWR1_A, 0x77777777);
		rtw_write32(rtwdev, REG_AFE_PWR2_A, 0x77777777);

		rtw_write32(rtwdev, REG_RX_WAIT_CCA_TX_CCK_RFON_A, 0x19791979);

		/* hardware 3-wire off */
		rtw_write32_mask(rtwdev, REG_3WIRE_SWA, 0xf, 0x4);

		/* LOK setting */

		/* 1. DAC/ADC sampling rate (160 MHz) */
		rtw_write32_mask(rtwdev, REG_CK_MONHA, GENMASK(26, 24), 0x7);

		/* 2. LoK RF setting (at BW = 20M) */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80002);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, 0x00c00, 0x3);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK,
			     0x20000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK,
			     0x0003f);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK,
			     0xf3fc3);

		rtw_write_rf(rtwdev, RF_PATH_A, RF_TXA_PREPAD, RFREG_MASK,
			     0x931d5);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RXBB2, RFREG_MASK, 0x8a001);
		rtw_write32(rtwdev, REG_DAC_RSTB, 0x00008000);
		rtw_write32_mask(rtwdev, REG_TXAGCIDX, BIT(0), 0x1);
		/* TX (X,Y) */
		rtw_write32(rtwdev, REG_IQK_COM00, 0x29002000);
		/* RX (X,Y) */
		rtw_write32(rtwdev, REG_IQK_COM32, 0xa9002000);
		/* [0]:AGC_en, [15]:idac_K_Mask */
		rtw_write32(rtwdev, REG_IQK_COM96, 0x00462910);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		if (efuse->ext_pa_5g)
			rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE,
				    0x821403f7);
		else
			rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE,
				    0x821403f4);

		if (hal->current_band_type == RTW_BAND_5G)
			rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x68163e96);
		else
			rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x28163e96);

		/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
		rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x18008c10);
		/* RX_Tone_idx[9:0], RxK_Mask[29] */
		rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x38008c10);
		rtw_write32(rtwdev, REG_RFECTL_A, 0x00100000);
		rtw_write32(rtwdev, REG_IQK_COM64, 0xfa000000);
		rtw_write32(rtwdev, REG_IQK_COM64, 0xf8000000);

		mdelay(10);
		rtw_write32(rtwdev, REG_RFECTL_A, 0x00000000);

		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_TXMOD, 0x7fe00,
			     rtw_read_rf(rtwdev, RF_PATH_A, RF_DTXLOK, 0xffc00));

		if (hal->current_band_width == RTW_CHANNEL_WIDTH_40)
			rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH,
				     RF18_BW_MASK, 0x1);
		else if (hal->current_band_width == RTW_CHANNEL_WIDTH_80)
			rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH,
				     RF18_BW_MASK, 0x0);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		/* 3. TX RF setting */
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK,
			     0x20000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK,
			     0x0003f);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK,
			     0xf3fc3);

		rtw_write_rf(rtwdev, RF_PATH_A, RF_TXA_PREPAD, RFREG_MASK, 0x931d5);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RXBB2, RFREG_MASK, 0x8a001);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x00000);
		rtw_write32(rtwdev, REG_DAC_RSTB, 0x00008000);
		rtw_write32_mask(rtwdev, REG_TXAGCIDX, BIT(0), 0x1);
		/* TX (X,Y) */
		rtw_write32(rtwdev, REG_IQK_COM00, 0x29002000);
		/* RX (X,Y) */
		rtw_write32(rtwdev, REG_IQK_COM32, 0xa9002000);
		/* [0]:AGC_en, [15]:idac_K_Mask */
		rtw_write32(rtwdev, REG_IQK_COM96, 0x0046a910);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		if (efuse->ext_pa_5g)
			rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE,
				    0x821403f7);
		else
			rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE,
				    0x821403e3);

		if (hal->current_band_type == RTW_BAND_5G)
			rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x40163e96);
		else
			rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x00163e96);

		if (vdf_enable)
			r12_chip21_iqk_tx_vdf_true(rtwdev, cal, &tx0iqkok,
						 tx_x0, tx_y0);
		else
			r12_chip21_iqk_tx_vdf_false(rtwdev, cal, &tx0iqkok,
						  tx_x0, tx_y0);

		if (!tx0iqkok)
			break; /* TXK fail, Don't do RXK */

		/* ====== RX IQK ====== */
		/* [31] = 0 --> Page C */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x0);
		/* 1. RX RF setting */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_ADDR, RFREG_MASK,
			     0x30000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA0, RFREG_MASK,
			     0x0002f);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_MODE_TABLE_DATA1, RFREG_MASK,
			     0xfffbb);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RXBB2, RFREG_MASK, 0x88001);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_TXA_PREPAD, RFREG_MASK, 0x931d8);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x00000);

		rtw_write32_mask(rtwdev, REG_IQK_COM00, 0x03FF8000,
				 (tx_x0[cal] >> 21) & 0x000007ff);
		rtw_write32_mask(rtwdev, REG_IQK_COM00, 0x000007FF,
				 (tx_y0[cal] >> 21) & 0x000007ff);
		rtw_write32_mask(rtwdev, REG_IQK_COM00, BIT(31), 0x1);
		rtw_write32_mask(rtwdev, REG_IQK_COM00, BIT(31), 0x0);
		rtw_write32(rtwdev, REG_DAC_RSTB, 0x00008000);
		rtw_write32(rtwdev, REG_IQK_COM96, 0x0046a911);

		/* [31] = 1 --> Page C1 */
		rtw_write32_mask(rtwdev, REG_CCASEL, BIT(31), 0x1);

		/* TX_Tone_idx[9:0], TxK_Mask[29] TX_Tone = 16 */
		rtw_write32(rtwdev, REG_OFDM0_XA_TX_IQ_IMBALANCE, 0x38008c10);
		/* RX_Tone_idx[9:0], RxK_Mask[29] */
		rtw_write32(rtwdev, REG_OFDM0_A_TX_AFE, 0x18008c10);
		rtw_write32(rtwdev, REG_OFDM0_XB_TX_IQ_IMBALANCE, 0x02140119);

		if (rtw_hci_type(rtwdev) == RTW_HCI_TYPE_PCIE)
			rx_iqk_loop = 2; /* for 2% fail; */
		else
			rx_iqk_loop = 1;

		for (i = 0; i < rx_iqk_loop; i++) {
			if (rtw_hci_type(rtwdev) == RTW_HCI_TYPE_PCIE && i == 0)
				rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x28161100); /* Good */
			else
				rtw_write32(rtwdev, REG_TSSI_TRK_SW, 0x28160d00);

			r12_chip21_iqk_rx(rtwdev, cal, &rx0iqkok,
					rx_x0[i], rx_y0[i]);
		}

		if (tx0iqkok)
			tx_average++;
		if (rx0iqkok)
			rx_average++;
	}

	/* FillIQK Result */

	if (tx_average == 0)
		return;

	for (i = 0; i < tx_average; i++)
		(void)0;

	if (r12_iqk_finish(tx_average, 3, tx_x0, tx_y0,
				&tx_x, &tx_y, true, true))
		{rtwdev->tx_iqk_mask=1;r12_chip21_iqk_tx_fill(rtwdev, (u32)tx_x, (u32)tx_y);}
	else
		r12_chip21_iqk_tx_fill(rtwdev, 0x200, 0x0);

	if (rx_average == 0)
		return;

	for (i = 0; i < rx_average; i++) {
		(void)0;

		if (rx_iqk_loop == 2)
			(void)0;
	}

	rx_finish1 = r12_iqk_finish(rx_average, 4, rx_x0[0], rx_y0[0],
					 &rx_x_temp, &rx_y_temp, true, true);

	if (rx_finish1) {
		rx_x = rx_x_temp;
		rx_y = rx_y_temp;
	}

	if (rx_iqk_loop == 2) {
		rx_finish2 = r12_iqk_finish(rx_average, 4,
						 rx_x0[1], rx_y0[1],
						 &rx_x, &rx_y, true, true);

		if (rx_finish1 && rx_finish2) {
			rx_x = (rx_x + rx_x_temp) / 2;
			rx_y = (rx_y + rx_y_temp) / 2;
		}
	}

	if (rx_finish1 || rx_finish2)
		{rtwdev->rx_iqk_mask=1;r12_chip21_iqk_rx_fill(rtwdev, (u32)rx_x, (u32)rx_y);}
	else
		r12_chip21_iqk_rx_fill(rtwdev, 0x200, 0x0);
}

static void r12_chip21_do_iqk(struct rtw_r12au_phy *rtwdev)
{
	static const u32 backup_macbb_reg[MACBB_REG_NUM_8821A] = {
		0x520, 0x550, 0x808, 0xa04, 0x90c, 0xc00, 0x838, 0x82c
	};
	static const u32 backup_afe_reg[AFE_REG_NUM_8821A] = {
		0xc5c, 0xc60, 0xc64, 0xc68
	};
	static const u32 backup_rf_reg[RF_REG_NUM_8821A] = {
		0x65, 0x8f, 0x0
	};
	u32 macbb_backup[MACBB_REG_NUM_8821A] = {0};
	u32 afe_backup[AFE_REG_NUM_8821A] = {0};
	u32 rfa_backup[RF_REG_NUM_8821A] = {0};

	r12_iqk_backup_mac_bb(rtwdev, macbb_backup,
				   backup_macbb_reg, MACBB_REG_NUM_8821A);
	r12_iqk_backup_afe(rtwdev, afe_backup,
				backup_afe_reg, AFE_REG_NUM_8821A);
	r12_chip21_iqk_backup_rf(rtwdev, rfa_backup,
			       backup_rf_reg, RF_REG_NUM_8821A);

	if (rtwdev->io_error) return;
	r12_iqk_configure_mac(rtwdev);

	r12_chip21_iqk(rtwdev);
	rtwdev->cleanup=true;

	r12_chip21_iqk_restore_rf(rtwdev, backup_rf_reg,
				rfa_backup, RF_REG_NUM_8821A);
	r12_chip21_iqk_restore_afe(rtwdev, afe_backup,
				 backup_afe_reg, AFE_REG_NUM_8821A);
	r12_iqk_restore_mac_bb(rtwdev, macbb_backup,
				    backup_macbb_reg, MACBB_REG_NUM_8821A);
	rtwdev->cleanup=false;
}
