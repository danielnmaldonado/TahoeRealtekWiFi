/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation. Generated from pinned Linux.
 * This port selects BSD-3-Clause; see RTW88Coex-Upstream.json. */
static void rtw8821c_coex_cfg_init(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	/* enable TBTT nterrupt */
	rtw_write8_set(rtwdev, REG_BCN_CTRL, BIT_EN_BCN_FUNCTION);

	/* BT report packet sample rate */
	rtw_write8_mask(rtwdev, REG_BT_TDMA_TIME, BIT_MASK_SAMPLE_RATE, 0x5);

	/* enable BT counter statistics */
	rtw_write8(rtwdev, REG_BT_STAT_CTRL, BT_CNT_ENABLE);

	/* enable PTA (3-wire function form BT side) */
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_BT_PTA_EN);
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_PO_BT_PTA_PINS);

	/* enable PTA (tx/rx signal form WiFi side) */
	rtw_write8_set(rtwdev, REG_QUEUE_CTRL, BIT_PTA_WL_TX_EN);
	/* wl tx signal to PTA not case EDCCA */
	rtw_write8_clr(rtwdev, REG_QUEUE_CTRL, BIT_PTA_EDCCA_EN);
	/* GNT_BT=1 while select both */
	rtw_write16_set(rtwdev, REG_BT_COEX_V2, BIT_GNT_BT_POLARITY);

	/* beacon queue always hi-pri  */
	rtw_write8_mask(rtwdev, REG_BT_COEX_TABLE_H + 3, BIT_BCN_QUEUE,
			BCN_PRI_EN);
}


static void rtw8821c_coex_cfg_ant_switch(struct rtw_dev *rtwdev, u8 ctrl_type,
					 u8 pos_type)
{
	(void)rtwdev;
	(void)ctrl_type;
	(void)pos_type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	u32 switch_status = FIELD_PREP(CTRL_TYPE_MASK, ctrl_type) | pos_type;
	bool polarity_inverse;
	u8 regval = 0;

	if (switch_status == coex_dm->cur_switch_status)
		return;

	if (coex_rfe->wlg_at_btg) {
		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;

		if (coex_rfe->ant_switch_polarity)
			pos_type = COEX_SWITCH_TO_WLA;
		else
			pos_type = COEX_SWITCH_TO_WLG_BT;
	}

	coex_dm->cur_switch_status = switch_status;

	if (coex_rfe->ant_switch_diversity &&
	    ctrl_type == COEX_SWITCH_CTRL_BY_BBSW)
		ctrl_type = COEX_SWITCH_CTRL_BY_ANTDIV;

	polarity_inverse = (coex_rfe->ant_switch_polarity == 1);

	switch (ctrl_type) {
	default:
	case COEX_SWITCH_CTRL_BY_BBSW:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		/* BB SW, DPDT use RFE_ctrl8 and RFE_ctrl9 as ctrl pin */
		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89,
				DPDT_CTRL_PIN);

		if (pos_type == COEX_SWITCH_TO_WLG_BT) {
			if (coex_rfe->rfe_module_type != 0x4 &&
			    coex_rfe->rfe_module_type != 0x2)
				regval = 0x3;
			else
				regval = (!polarity_inverse ? 0x2 : 0x1);
		} else if (pos_type == COEX_SWITCH_TO_WLG) {
			regval = (!polarity_inverse ? 0x2 : 0x1);
		} else {
			regval = (!polarity_inverse ? 0x1 : 0x2);
		}

		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_R_RFE_SEL_15,
				 regval);
		break;
	case COEX_SWITCH_CTRL_BY_PTA:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		/* PTA,  DPDT use RFE_ctrl8 and RFE_ctrl9 as ctrl pin */
		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89,
				PTA_CTRL_PIN);

		regval = (!polarity_inverse ? 0x2 : 0x1);
		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_R_RFE_SEL_15,
				 regval);
		break;
	case COEX_SWITCH_CTRL_BY_ANTDIV:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89,
				ANTDIC_CTRL_PIN);
		break;
	case COEX_SWITCH_CTRL_BY_MAC:
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);

		regval = (!polarity_inverse ? 0x0 : 0x1);
		rtw_write8_mask(rtwdev, REG_PAD_CTRL1, BIT_SW_DPDT_SEL_DATA,
				regval);
		break;
	case COEX_SWITCH_CTRL_BY_FW:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		break;
	case COEX_SWITCH_CTRL_BY_BT:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		break;
	}

	if (ctrl_type == COEX_SWITCH_CTRL_BY_BT) {
		rtw_write8_clr(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE1);
		rtw_write8_clr(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE2);
	} else {
		rtw_write8_set(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE1);
		rtw_write8_set(rtwdev, REG_CTRL_TYPE, BIT_CTRL_TYPE2);
	}
}


static void rtw8821c_coex_cfg_gnt_fix(struct rtw_dev *rtwdev)
{
	(void)rtwdev;}


static void rtw8821c_coex_cfg_gnt_debug(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	rtw_write32_clr(rtwdev, REG_PAD_CTRL1, BIT_BTGP_SPI_EN);
	rtw_write32_clr(rtwdev, REG_PAD_CTRL1, BIT_BTGP_JTAG_EN);
	rtw_write32_clr(rtwdev, REG_GPIO_MUXCFG, BIT_FSPI_EN);
	rtw_write32_clr(rtwdev, REG_PAD_CTRL1, BIT_LED1DIS);
	rtw_write32_clr(rtwdev, REG_SYS_SDIO_CTRL, BIT_SDIO_INT);
	rtw_write32_clr(rtwdev, REG_SYS_SDIO_CTRL, BIT_DBG_GNT_WL_BT);
}


static void rtw8821c_coex_cfg_rfe_type(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	struct rtw_efuse *efuse = &rtwdev->efuse;

	coex_rfe->rfe_module_type = efuse->rfe_option;
	coex_rfe->ant_switch_polarity = 0;
	coex_rfe->ant_switch_exist = true;
	coex_rfe->wlg_at_btg = false;

	switch (coex_rfe->rfe_module_type) {
	case 0:
	case 8:
	case 1:
	case 9:  /* 1-Ant, Main, WLG */
	default: /* 2-Ant, DPDT, WLG */
		break;
	case 2:
	case 10: /* 1-Ant, Main, BTG */
	case 7:
	case 15: /* 2-Ant, DPDT, BTG */
		coex_rfe->wlg_at_btg = true;
		break;
	case 3:
	case 11: /* 1-Ant, Aux, WLG */
		coex_rfe->ant_switch_polarity = 1;
		break;
	case 4:
	case 12: /* 1-Ant, Aux, BTG */
		coex_rfe->wlg_at_btg = true;
		coex_rfe->ant_switch_polarity = 1;
		break;
	case 5:
	case 13: /* 2-Ant, no switch, WLG */
	case 6:
	case 14: /* 2-Ant, no antenna switch, WLG */
		coex_rfe->ant_switch_exist = false;
		break;
	}
}


static void rtw8821c_coex_cfg_wl_tx_power(struct rtw_dev *rtwdev, u8 wl_pwr)
{
	(void)rtwdev;
	(void)wl_pwr;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	bool share_ant = efuse->share_ant;

	if (share_ant)
		return;

	if (wl_pwr == coex_dm->cur_wl_pwr_lvl)
		return;

	coex_dm->cur_wl_pwr_lvl = wl_pwr;
}


static void rtw8821c_coex_cfg_wl_rx_gain(struct rtw_dev *rtwdev, bool low_gain)
{
	(void)rtwdev;
	(void)low_gain;}


static void rtw8821c_false_alarm_statistics(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 cck_enable;
	u32 cck_fa_cnt;
	u32 ofdm_fa_cnt;
	u32 crc32_cnt;
	u32 cca32_cnt;

	cck_enable = rtw_read32(rtwdev, REG_RXPSEL) & BIT(28);
	cck_fa_cnt = rtw_read16(rtwdev, REG_FA_CCK);
	ofdm_fa_cnt = rtw_read16(rtwdev, REG_FA_OFDM);

	dm_info->cck_fa_cnt = cck_fa_cnt;
	dm_info->ofdm_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt = ofdm_fa_cnt;
	if (cck_enable)
		dm_info->total_fa_cnt += cck_fa_cnt;

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_CCK);
	dm_info->cck_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->cck_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_OFDM);
	dm_info->ofdm_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->ofdm_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_HT);
	dm_info->ht_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->ht_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	crc32_cnt = rtw_read32(rtwdev, REG_CRC_VHT);
	dm_info->vht_ok_cnt = FIELD_GET(GENMASK(15, 0), crc32_cnt);
	dm_info->vht_err_cnt = FIELD_GET(GENMASK(31, 16), crc32_cnt);

	cca32_cnt = rtw_read32(rtwdev, REG_CCA_OFDM);
	dm_info->ofdm_cca_cnt = FIELD_GET(GENMASK(31, 16), cca32_cnt);
	dm_info->total_cca_cnt = dm_info->ofdm_cca_cnt;
	if (cck_enable) {
		cca32_cnt = rtw_read32(rtwdev, REG_CCA_CCK);
		dm_info->cck_cca_cnt = FIELD_GET(GENMASK(15, 0), cca32_cnt);
		dm_info->total_cca_cnt += dm_info->cck_cca_cnt;
	}

	rtw_write32_set(rtwdev, REG_FAS, BIT(17));
	rtw_write32_clr(rtwdev, REG_FAS, BIT(17));
	rtw_write32_clr(rtwdev, REG_RXDESC, BIT(15));
	rtw_write32_set(rtwdev, REG_RXDESC, BIT(15));
	rtw_write32_set(rtwdev, REG_CNTRST, BIT(0));
	rtw_write32_clr(rtwdev, REG_CNTRST, BIT(0));
}

static const struct rtw_chip_ops cx_ops_8821c={rtw8821c_coex_cfg_init,rtw8821c_coex_cfg_ant_switch,rtw8821c_coex_cfg_gnt_fix,rtw8821c_coex_cfg_gnt_debug,rtw8821c_coex_cfg_rfe_type,rtw8821c_coex_cfg_wl_tx_power,rtw8821c_coex_cfg_wl_rx_gain};


static void rtw8822b_coex_cfg_init(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	/* enable TBTT nterrupt */
	rtw_write8_set(rtwdev, REG_BCN_CTRL, BIT_EN_BCN_FUNCTION);

	/* BT report packet sample rate */
	/* 0x790[5:0]=0x5 */
	rtw_write8_mask(rtwdev, REG_BT_TDMA_TIME, BIT_MASK_SAMPLE_RATE, 0x5);

	/* enable BT counter statistics */
	rtw_write8(rtwdev, REG_BT_STAT_CTRL, 0x1);

	/* enable PTA (3-wire function form BT side) */
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_BT_PTA_EN);
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_PO_BT_PTA_PINS);

	/* enable PTA (tx/rx signal form WiFi side) */
	rtw_write8_set(rtwdev, REG_QUEUE_CTRL, BIT_PTA_WL_TX_EN);
	/* wl tx signal to PTA not case EDCCA */
	rtw_write8_clr(rtwdev, REG_QUEUE_CTRL, BIT_PTA_EDCCA_EN);
	/* GNT_BT=1 while select both */
	rtw_write16_set(rtwdev, REG_BT_COEX_V2, BIT_GNT_BT_POLARITY);
}


static void rtw8822b_coex_cfg_ant_switch(struct rtw_dev *rtwdev,
					 u8 ctrl_type, u8 pos_type)
{
	(void)rtwdev;
	(void)ctrl_type;
	(void)pos_type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	bool polarity_inverse;
	u8 regval = 0;

	if ((((u32)ctrl_type << 8) + pos_type) == coex_dm->cur_switch_status)
		return;

	coex_dm->cur_switch_status = (ctrl_type << 8) + pos_type;

	if (coex_rfe->ant_switch_diversity &&
	    ctrl_type == COEX_SWITCH_CTRL_BY_BBSW)
		ctrl_type = COEX_SWITCH_CTRL_BY_ANTDIV;

	polarity_inverse = (coex_rfe->ant_switch_polarity == 1);

	switch (ctrl_type) {
	default:
	case COEX_SWITCH_CTRL_BY_BBSW:
		/* 0x4c[23] = 0 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 2, BIT_DPDT_SEL_EN >> 16, 0x0);
		/* 0x4c[24] = 1 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 3, BIT_DPDT_WL_SEL >> 24, 0x1);
		/* BB SW, DPDT use RFE_ctrl8 and RFE_ctrl9 as ctrl pin */
		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89, 0x77);

		if (pos_type == COEX_SWITCH_TO_WLG_BT) {
			if (coex_rfe->rfe_module_type != 0x4 &&
			    coex_rfe->rfe_module_type != 0x2)
				regval = 0x3;
			else
				regval = (!polarity_inverse ? 0x2 : 0x1);
		} else if (pos_type == COEX_SWITCH_TO_WLG) {
			regval = (!polarity_inverse ? 0x2 : 0x1);
		} else {
			regval = (!polarity_inverse ? 0x1 : 0x2);
		}

		rtw_write8_mask(rtwdev, REG_RFE_INV8, BIT_MASK_RFE_INV89, regval);
		break;
	case COEX_SWITCH_CTRL_BY_PTA:
		/* 0x4c[23] = 0 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 2, BIT_DPDT_SEL_EN >> 16, 0x0);
		/* 0x4c[24] = 1 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 3, BIT_DPDT_WL_SEL >> 24, 0x1);
		/* PTA,  DPDT use RFE_ctrl8 and RFE_ctrl9 as ctrl pin */
		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89, 0x66);

		regval = (!polarity_inverse ? 0x2 : 0x1);
		rtw_write8_mask(rtwdev, REG_RFE_INV8, BIT_MASK_RFE_INV89, regval);
		break;
	case COEX_SWITCH_CTRL_BY_ANTDIV:
		/* 0x4c[23] = 0 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 2, BIT_DPDT_SEL_EN >> 16, 0x0);
		/* 0x4c[24] = 1 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 3, BIT_DPDT_WL_SEL >> 24, 0x1);
		rtw_write8_mask(rtwdev, REG_RFE_CTRL8, BIT_MASK_RFE_SEL89, 0x88);
		break;
	case COEX_SWITCH_CTRL_BY_MAC:
		/* 0x4c[23] = 1 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 2, BIT_DPDT_SEL_EN >> 16, 0x1);

		regval = (!polarity_inverse ? 0x0 : 0x1);
		rtw_write8_mask(rtwdev, REG_PAD_CTRL1, BIT_SW_DPDT_SEL_DATA, regval);
		break;
	case COEX_SWITCH_CTRL_BY_FW:
		/* 0x4c[23] = 0 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 2, BIT_DPDT_SEL_EN >> 16, 0x0);
		/* 0x4c[24] = 1 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 3, BIT_DPDT_WL_SEL >> 24, 0x1);
		break;
	case COEX_SWITCH_CTRL_BY_BT:
		/* 0x4c[23] = 0 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 2, BIT_DPDT_SEL_EN >> 16, 0x0);
		/* 0x4c[24] = 0 */
		rtw_write8_mask(rtwdev, REG_LED_CFG + 3, BIT_DPDT_WL_SEL >> 24, 0x0);
		break;
	}
}


static void rtw8822b_coex_cfg_gnt_fix(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
}


static void rtw8822b_coex_cfg_gnt_debug(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	rtw_write8_mask(rtwdev, REG_PAD_CTRL1 + 2, BIT_BTGP_SPI_EN >> 16, 0);
	rtw_write8_mask(rtwdev, REG_PAD_CTRL1 + 3, BIT_BTGP_JTAG_EN >> 24, 0);
	rtw_write8_mask(rtwdev, REG_GPIO_MUXCFG + 2, BIT_FSPI_EN >> 16, 0);
	rtw_write8_mask(rtwdev, REG_PAD_CTRL1 + 1, BIT_LED1DIS >> 8, 0);
	rtw_write8_mask(rtwdev, REG_SYS_SDIO_CTRL + 3, BIT_DBG_GNT_WL_BT >> 24, 0);
}


static void rtw8822b_coex_cfg_rfe_type(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	bool is_ext_fem = false;

	coex_rfe->rfe_module_type = rtwdev->efuse.rfe_option;
	coex_rfe->ant_switch_polarity = 0;
	coex_rfe->ant_switch_diversity = false;
	if (coex_rfe->rfe_module_type == 0x12 ||
	    coex_rfe->rfe_module_type == 0x15 ||
	    coex_rfe->rfe_module_type == 0x16)
		coex_rfe->ant_switch_exist = false;
	else
		coex_rfe->ant_switch_exist = true;

	if (coex_rfe->rfe_module_type == 2 ||
	    coex_rfe->rfe_module_type == 4) {
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_EXTFEM, true);
		is_ext_fem = true;
	} else {
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_EXTFEM, false);
	}

	coex_rfe->wlg_at_btg = false;

	if (efuse->share_ant &&
	    coex_rfe->ant_switch_exist && !is_ext_fem)
		coex_rfe->ant_switch_with_bt = true;
	else
		coex_rfe->ant_switch_with_bt = false;

	/* Ext switch buffer mux */
	rtw_write8(rtwdev, REG_RFE_CTRL_E, 0xff);
	rtw_write8_mask(rtwdev, REG_RFESEL_CTRL + 1, 0x3, 0x0);
	rtw_write8_mask(rtwdev, REG_RFE_INV16, BIT_RFE_BUF_EN, 0x0);

	/* Disable LTE Coex Function in WiFi side */
	rtw_coex_write_indirect_reg(rtwdev, LTE_COEX_CTRL, BIT_LTE_COEX_EN, 0);

	/* BTC_CTT_WL_VS_LTE */
	rtw_coex_write_indirect_reg(rtwdev, LTE_WL_TRX_CTRL, MASKLWORD, 0xffff);

	/* BTC_CTT_BT_VS_LTE */
	rtw_coex_write_indirect_reg(rtwdev, LTE_BT_TRX_CTRL, MASKLWORD, 0xffff);
}


static void rtw8822b_coex_cfg_wl_tx_power(struct rtw_dev *rtwdev, u8 wl_pwr)
{
	(void)rtwdev;
	(void)wl_pwr;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	static const u16 reg_addr[] = {0xc58, 0xe58};
	static const u8	wl_tx_power[] = {0xd8, 0xd4, 0xd0, 0xcc, 0xc8};
	u8 i, pwr;

	if (wl_pwr == coex_dm->cur_wl_pwr_lvl)
		return;

	coex_dm->cur_wl_pwr_lvl = wl_pwr;

	if (coex_dm->cur_wl_pwr_lvl >= ARRAY_SIZE(wl_tx_power))
		coex_dm->cur_wl_pwr_lvl = ARRAY_SIZE(wl_tx_power) - 1;

	pwr = wl_tx_power[coex_dm->cur_wl_pwr_lvl];

	for (i = 0; i < ARRAY_SIZE(reg_addr); i++)
		rtw_write8_mask(rtwdev, reg_addr[i], 0xff, pwr);
}


static void rtw8822b_coex_cfg_wl_rx_gain(struct rtw_dev *rtwdev, bool low_gain)
{
	(void)rtwdev;
	(void)low_gain;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	/* WL Rx Low gain on */
	static const u32 wl_rx_low_gain_on[] = {
		0xff000003, 0xbd120003, 0xbe100003, 0xbf080003, 0xbf060003,
		0xbf050003, 0xbc140003, 0xbb160003, 0xba180003, 0xb91a0003,
		0xb81c0003, 0xb71e0003, 0xb4200003, 0xb5220003, 0xb4240003,
		0xb3260003, 0xb2280003, 0xb12a0003, 0xb02c0003, 0xaf2e0003,
		0xae300003, 0xad320003, 0xac340003, 0xab360003, 0x8d380003,
		0x8c3a0003, 0x8b3c0003, 0x8a3e0003, 0x6e400003, 0x6d420003,
		0x6c440003, 0x6b460003, 0x6a480003, 0x694a0003, 0x684c0003,
		0x674e0003, 0x66500003, 0x65520003, 0x64540003, 0x64560003,
		0x007e0403
	};

	/* WL Rx Low gain off */
	static const u32 wl_rx_low_gain_off[] = {
		0xff000003, 0xf4120003, 0xf5100003, 0xf60e0003, 0xf70c0003,
		0xf80a0003, 0xf3140003, 0xf2160003, 0xf1180003, 0xf01a0003,
		0xef1c0003, 0xee1e0003, 0xed200003, 0xec220003, 0xeb240003,
		0xea260003, 0xe9280003, 0xe82a0003, 0xe72c0003, 0xe62e0003,
		0xe5300003, 0xc8320003, 0xc7340003, 0xc6360003, 0xc5380003,
		0xc43a0003, 0xc33c0003, 0xc23e0003, 0xc1400003, 0xc0420003,
		0xa5440003, 0xa4460003, 0xa3480003, 0xa24a0003, 0xa14c0003,
		0x834e0003, 0x82500003, 0x81520003, 0x80540003, 0x65560003,
		0x007e0403
	};
	u8 i;

	if (low_gain == coex_dm->cur_wl_rx_low_gain_en)
		return;

	coex_dm->cur_wl_rx_low_gain_en = low_gain;

	if (coex_dm->cur_wl_rx_low_gain_en) {
		(void)0;
		for (i = 0; i < ARRAY_SIZE(wl_rx_low_gain_on); i++)
			rtw_write32(rtwdev, REG_RX_GAIN_EN, wl_rx_low_gain_on[i]);

		/* set Rx filter corner RCK offset */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCKD, 0x2, 0x1);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCK, 0x3f, 0x3f);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCKD, 0x2, 0x1);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCK, 0x3f, 0x3f);
	} else {
		(void)0;
		for (i = 0; i < ARRAY_SIZE(wl_rx_low_gain_off); i++)
			rtw_write32(rtwdev, 0x81c, wl_rx_low_gain_off[i]);

		/* set Rx filter corner RCK offset */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCK, 0x3f, 0x4);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCKD, 0x2, 0x0);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCK, 0x3f, 0x4);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCKD, 0x2, 0x0);
	}
}


static void rtw8822b_false_alarm_statistics(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 cck_enable;
	u32 cck_fa_cnt;
	u32 ofdm_fa_cnt;
	u32 crc32_cnt;
	u32 cca32_cnt;

	cck_enable = rtw_read32(rtwdev, 0x808) & BIT(28);
	cck_fa_cnt = rtw_read16(rtwdev, 0xa5c);
	ofdm_fa_cnt = rtw_read16(rtwdev, 0xf48);

	dm_info->cck_fa_cnt = cck_fa_cnt;
	dm_info->ofdm_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt += cck_enable ? cck_fa_cnt : 0;

	crc32_cnt = rtw_read32(rtwdev, 0xf04);
	dm_info->cck_ok_cnt = crc32_cnt & 0xffff;
	dm_info->cck_err_cnt = (crc32_cnt & 0xffff0000) >> 16;
	crc32_cnt = rtw_read32(rtwdev, 0xf14);
	dm_info->ofdm_ok_cnt = crc32_cnt & 0xffff;
	dm_info->ofdm_err_cnt = (crc32_cnt & 0xffff0000) >> 16;
	crc32_cnt = rtw_read32(rtwdev, 0xf10);
	dm_info->ht_ok_cnt = crc32_cnt & 0xffff;
	dm_info->ht_err_cnt = (crc32_cnt & 0xffff0000) >> 16;
	crc32_cnt = rtw_read32(rtwdev, 0xf0c);
	dm_info->vht_ok_cnt = crc32_cnt & 0xffff;
	dm_info->vht_err_cnt = (crc32_cnt & 0xffff0000) >> 16;

	cca32_cnt = rtw_read32(rtwdev, 0xf08);
	dm_info->ofdm_cca_cnt = ((cca32_cnt & 0xffff0000) >> 16);
	dm_info->total_cca_cnt = dm_info->ofdm_cca_cnt;
	if (cck_enable) {
		cca32_cnt = rtw_read32(rtwdev, 0xfcc);
		dm_info->cck_cca_cnt = cca32_cnt & 0xffff;
		dm_info->total_cca_cnt += dm_info->cck_cca_cnt;
	}

	rtw_write32_set(rtwdev, 0x9a4, BIT(17));
	rtw_write32_clr(rtwdev, 0x9a4, BIT(17));
	rtw_write32_clr(rtwdev, 0xa2c, BIT(15));
	rtw_write32_set(rtwdev, 0xa2c, BIT(15));
	rtw_write32_set(rtwdev, 0xb58, BIT(0));
	rtw_write32_clr(rtwdev, 0xb58, BIT(0));
}

static const struct rtw_chip_ops cx_ops_8822b={rtw8822b_coex_cfg_init,rtw8822b_coex_cfg_ant_switch,rtw8822b_coex_cfg_gnt_fix,rtw8822b_coex_cfg_gnt_debug,rtw8822b_coex_cfg_rfe_type,rtw8822b_coex_cfg_wl_tx_power,rtw8822b_coex_cfg_wl_rx_gain};

static void rtw8822c_coex_cfg_init(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	/* enable TBTT nterrupt */
	rtw_write8_set(rtwdev, REG_BCN_CTRL, BIT_EN_BCN_FUNCTION);

	/* BT report packet sample rate */
	/* 0x790[5:0]=0x5 */
	rtw_write8_mask(rtwdev, REG_BT_TDMA_TIME, BIT_MASK_SAMPLE_RATE, 0x5);

	/* enable BT counter statistics */
	rtw_write8(rtwdev, REG_BT_STAT_CTRL, 0x1);

	/* enable PTA (3-wire function form BT side) */
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_BT_PTA_EN);
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_PO_BT_PTA_PINS);

	/* enable PTA (tx/rx signal form WiFi side) */
	rtw_write8_set(rtwdev, REG_QUEUE_CTRL, BIT_PTA_WL_TX_EN);
	/* wl tx signal to PTA not case EDCCA */
	rtw_write8_clr(rtwdev, REG_QUEUE_CTRL, BIT_PTA_EDCCA_EN);
	/* GNT_BT=1 while select both */
	rtw_write16_set(rtwdev, REG_BT_COEX_V2, BIT_GNT_BT_POLARITY);
	/* BT_CCA = ~GNT_WL_BB, not or GNT_BT_BB, LTE_Rx */
	rtw_write8_clr(rtwdev, REG_DUMMY_PAGE4_V1, BIT_BTCCA_CTRL);

	/* to avoid RF parameter error */
	rtw_write_rf(rtwdev, RF_PATH_B, RF_MODOPT, 0xfffff, 0x40000);
}


static void rtw8822c_coex_cfg_gnt_fix(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u32 rf_0x1;

	if (coex_stat->gnt_workaround_state == coex_stat->wl_coex_mode)
		return;

	coex_stat->gnt_workaround_state = coex_stat->wl_coex_mode;

	if ((coex_stat->kt_ver == 0 && coex->under_5g) || coex->freerun)
		rf_0x1 = 0x40021;
	else
		rf_0x1 = 0x40000;

	/* BT at S1 for Shared-Ant */
	if (efuse->share_ant)
		rf_0x1 |= BIT(13);

	rtw_write_rf(rtwdev, RF_PATH_B, 0x1, 0xfffff, rf_0x1);

	/* WL-S0 2G RF TRX cannot be masked by GNT_BT
	 * enable "WLS0 BB chage RF mode if GNT_BT = 1" for shared-antenna type
	 * disable:0x1860[3] = 1, enable:0x1860[3] = 0
	 *
	 * enable "DAC off if GNT_WL = 0" for non-shared-antenna
	 * disable 0x1c30[22] = 0,
	 * enable: 0x1c30[22] = 1, 0x1c38[12] = 0, 0x1c38[28] = 1
	 */
	if (coex_stat->wl_coex_mode == COEX_WLINK_2GFREE) {
		rtw_write8_mask(rtwdev, REG_ANAPAR + 2,
				BIT_ANAPAR_BTPS >> 16, 0);
	} else {
		rtw_write8_mask(rtwdev, REG_ANAPAR + 2,
				BIT_ANAPAR_BTPS >> 16, 1);
		rtw_write8_mask(rtwdev, REG_RSTB_SEL + 1,
				BIT_DAC_OFF_ENABLE, 0);
		rtw_write8_mask(rtwdev, REG_RSTB_SEL + 3,
				BIT_DAC_OFF_ENABLE, 1);
	}

	/* disable WL-S1 BB chage RF mode if GNT_BT
	 * since RF TRx mask can do it
	 */
	rtw_write8_mask(rtwdev, REG_IGN_GNTBT4,
			BIT_PI_IGNORE_GNT_BT, 1);

	/* disable WL-S0 BB chage RF mode if wifi is at 5G,
	 * or antenna path is separated
	 */
	if (coex_stat->wl_coex_mode == COEX_WLINK_2GFREE) {
		rtw_write8_mask(rtwdev, REG_IGN_GNT_BT1,
				BIT_PI_IGNORE_GNT_BT, 1);
		rtw_write8_mask(rtwdev, REG_NOMASK_TXBT,
				BIT_NOMASK_TXBT_ENABLE, 1);
	} else if (coex_stat->wl_coex_mode == COEX_WLINK_5G ||
	    coex->under_5g || !efuse->share_ant) {
		if (coex_stat->kt_ver >= 3) {
			rtw_write8_mask(rtwdev, REG_IGN_GNT_BT1,
					BIT_PI_IGNORE_GNT_BT, 0);
			rtw_write8_mask(rtwdev, REG_NOMASK_TXBT,
					BIT_NOMASK_TXBT_ENABLE, 1);
		} else {
			rtw_write8_mask(rtwdev, REG_IGN_GNT_BT1,
					BIT_PI_IGNORE_GNT_BT, 1);
		}
	} else {
		/* shared-antenna */
		rtw_write8_mask(rtwdev, REG_IGN_GNT_BT1,
				BIT_PI_IGNORE_GNT_BT, 0);
		if (coex_stat->kt_ver >= 3) {
			rtw_write8_mask(rtwdev, REG_NOMASK_TXBT,
					BIT_NOMASK_TXBT_ENABLE, 0);
		}
	}
}


static void rtw8822c_coex_cfg_gnt_debug(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	rtw_write8_mask(rtwdev, REG_PAD_CTRL1 + 2, BIT_BTGP_SPI_EN >> 16, 0);
	rtw_write8_mask(rtwdev, REG_PAD_CTRL1 + 3, BIT_BTGP_JTAG_EN >> 24, 0);
	rtw_write8_mask(rtwdev, REG_GPIO_MUXCFG + 2, BIT_FSPI_EN >> 16, 0);
	rtw_write8_mask(rtwdev, REG_PAD_CTRL1 + 1, BIT_LED1DIS >> 8, 0);
	rtw_write8_mask(rtwdev, REG_SYS_SDIO_CTRL + 3, BIT_DBG_GNT_WL_BT >> 24, 0);
}


static void rtw8822c_coex_cfg_rfe_type(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	struct rtw_efuse *efuse = &rtwdev->efuse;

	coex_rfe->rfe_module_type = rtwdev->efuse.rfe_option;
	coex_rfe->ant_switch_polarity = 0;
	coex_rfe->ant_switch_exist = false;
	coex_rfe->ant_switch_with_bt = false;
	coex_rfe->ant_switch_diversity = false;

	if (efuse->share_ant)
		coex_rfe->wlg_at_btg = true;
	else
		coex_rfe->wlg_at_btg = false;

	/* disable LTE coex in wifi side */
	rtw_coex_write_indirect_reg(rtwdev, LTE_COEX_CTRL, BIT_LTE_COEX_EN, 0x0);
	rtw_coex_write_indirect_reg(rtwdev, LTE_WL_TRX_CTRL, MASKLWORD, 0xffff);
	rtw_coex_write_indirect_reg(rtwdev, LTE_BT_TRX_CTRL, MASKLWORD, 0xffff);
}


static void rtw8822c_coex_cfg_wl_tx_power(struct rtw_dev *rtwdev, u8 wl_pwr)
{
	(void)rtwdev;
	(void)wl_pwr;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;

	if (wl_pwr == coex_dm->cur_wl_pwr_lvl)
		return;

	coex_dm->cur_wl_pwr_lvl = wl_pwr;
}


static void rtw8822c_coex_cfg_wl_rx_gain(struct rtw_dev *rtwdev, bool low_gain)
{
	(void)rtwdev;
	(void)low_gain;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;

	if (low_gain == coex_dm->cur_wl_rx_low_gain_en)
		return;

	coex_dm->cur_wl_rx_low_gain_en = low_gain;

	if (coex_dm->cur_wl_rx_low_gain_en) {
		(void)0;

		/* set Rx filter corner RCK offset */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCKD, RFREG_MASK, 0x22);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCK, RFREG_MASK, 0x36);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCKD, RFREG_MASK, 0x22);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCK, RFREG_MASK, 0x36);

	} else {
		(void)0;

		/* set Rx filter corner RCK offset */
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCKD, RFREG_MASK, 0x20);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_RCK, RFREG_MASK, 0x0);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCKD, RFREG_MASK, 0x20);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_RCK, RFREG_MASK, 0x0);
	}
}


static void rtw8822c_false_alarm_statistics(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 cck_enable;
	u32 cck_fa_cnt;
	u32 crc32_cnt;
	u32 cca32_cnt;
	u32 ofdm_fa_cnt;
	u32 ofdm_fa_cnt1, ofdm_fa_cnt2, ofdm_fa_cnt3, ofdm_fa_cnt4, ofdm_fa_cnt5;
	u16 parity_fail, rate_illegal, crc8_fail, mcs_fail, sb_search_fail,
	    fast_fsync, crc8_fail_vhta, mcs_fail_vht;

	cck_enable = rtw_read32(rtwdev, REG_ENCCK) & BIT_CCK_BLK_EN;
	cck_fa_cnt = rtw_read16(rtwdev, REG_CCK_FACNT);

	ofdm_fa_cnt1 = rtw_read32(rtwdev, REG_OFDM_FACNT1);
	ofdm_fa_cnt2 = rtw_read32(rtwdev, REG_OFDM_FACNT2);
	ofdm_fa_cnt3 = rtw_read32(rtwdev, REG_OFDM_FACNT3);
	ofdm_fa_cnt4 = rtw_read32(rtwdev, REG_OFDM_FACNT4);
	ofdm_fa_cnt5 = rtw_read32(rtwdev, REG_OFDM_FACNT5);

	parity_fail	= FIELD_GET(GENMASK(31, 16), ofdm_fa_cnt1);
	rate_illegal	= FIELD_GET(GENMASK(15, 0), ofdm_fa_cnt2);
	crc8_fail	= FIELD_GET(GENMASK(31, 16), ofdm_fa_cnt2);
	crc8_fail_vhta	= FIELD_GET(GENMASK(15, 0), ofdm_fa_cnt3);
	mcs_fail	= FIELD_GET(GENMASK(15, 0), ofdm_fa_cnt4);
	mcs_fail_vht	= FIELD_GET(GENMASK(31, 16), ofdm_fa_cnt4);
	fast_fsync	= FIELD_GET(GENMASK(15, 0), ofdm_fa_cnt5);
	sb_search_fail	= FIELD_GET(GENMASK(31, 16), ofdm_fa_cnt5);

	ofdm_fa_cnt = parity_fail + rate_illegal + crc8_fail + crc8_fail_vhta +
		      mcs_fail + mcs_fail_vht + fast_fsync + sb_search_fail;

	dm_info->cck_fa_cnt = cck_fa_cnt;
	dm_info->ofdm_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt = ofdm_fa_cnt;
	dm_info->total_fa_cnt += cck_enable ? cck_fa_cnt : 0;

	crc32_cnt = rtw_read32(rtwdev, 0x2c04);
	dm_info->cck_ok_cnt = crc32_cnt & 0xffff;
	dm_info->cck_err_cnt = (crc32_cnt & 0xffff0000) >> 16;
	crc32_cnt = rtw_read32(rtwdev, 0x2c14);
	dm_info->ofdm_ok_cnt = crc32_cnt & 0xffff;
	dm_info->ofdm_err_cnt = (crc32_cnt & 0xffff0000) >> 16;
	crc32_cnt = rtw_read32(rtwdev, 0x2c10);
	dm_info->ht_ok_cnt = crc32_cnt & 0xffff;
	dm_info->ht_err_cnt = (crc32_cnt & 0xffff0000) >> 16;
	crc32_cnt = rtw_read32(rtwdev, 0x2c0c);
	dm_info->vht_ok_cnt = crc32_cnt & 0xffff;
	dm_info->vht_err_cnt = (crc32_cnt & 0xffff0000) >> 16;

	cca32_cnt = rtw_read32(rtwdev, 0x2c08);
	dm_info->ofdm_cca_cnt = ((cca32_cnt & 0xffff0000) >> 16);
	dm_info->cck_cca_cnt = cca32_cnt & 0xffff;
	dm_info->total_cca_cnt = dm_info->ofdm_cca_cnt;
	if (cck_enable)
		dm_info->total_cca_cnt += dm_info->cck_cca_cnt;

	rtw_write32_mask(rtwdev, REG_CCANRX, BIT_CCK_FA_RST, 0);
	rtw_write32_mask(rtwdev, REG_CCANRX, BIT_CCK_FA_RST, 2);
	rtw_write32_mask(rtwdev, REG_CCANRX, BIT_OFDM_FA_RST, 0);
	rtw_write32_mask(rtwdev, REG_CCANRX, BIT_OFDM_FA_RST, 2);

	/* disable rx clk gating to reset counters */
	rtw_write32_clr(rtwdev, REG_RX_BREAK, BIT_COM_RX_GCK_EN);
	rtw_write32_set(rtwdev, REG_CNT_CTRL, BIT_ALL_CNT_RST);
	rtw_write32_clr(rtwdev, REG_CNT_CTRL, BIT_ALL_CNT_RST);
	rtw_write32_set(rtwdev, REG_RX_BREAK, BIT_COM_RX_GCK_EN);
}

static const struct rtw_chip_ops cx_ops_8822c={rtw8822c_coex_cfg_init,NULL,rtw8822c_coex_cfg_gnt_fix,rtw8822c_coex_cfg_gnt_debug,rtw8822c_coex_cfg_rfe_type,rtw8822c_coex_cfg_wl_tx_power,rtw8822c_coex_cfg_wl_rx_gain};
