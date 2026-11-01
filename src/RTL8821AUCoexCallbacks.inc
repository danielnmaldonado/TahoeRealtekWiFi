/* SPDX-License-Identifier: BSD-3-Clause */
/* Pinned Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation.
 * Dual GPL-2.0 OR BSD-3-Clause; this port selects BSD-3-Clause.
 * See RTL8821AUCoex-Upstream.json and RTL8812AUPHY-LICENSE.txt. */

static void rtw8821a_coex_cfg_init(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	u8 val8;

	/* BT report packet sample rate */
	rtw_write8_mask(rtwdev, REG_BT_TDMA_TIME, BIT_MASK_SAMPLE_RATE, 0x5);

	val8 = BIT_STATIS_BT_EN;
	if (rtwdev->efuse.share_ant)
		val8 |= BIT_R_GRANTALL_WLMASK;
	rtw_write8(rtwdev, REG_BT_COEX_ENH_INTR_CTRL, val8);

	/* enable BT counter statistics */
	rtw_write8(rtwdev, REG_BT_STAT_CTRL, 0x3);

	/* enable PTA */
	rtw_write32_set(rtwdev, REG_GPIO_MUXCFG, BIT_BT_PTA_EN);
}


static void rtw8821a_coex_cfg_ant_switch(struct rtw_dev *rtwdev, u8 ctrl_type,
					 u8 pos_type)
{
	(void)rtwdev;
	(void)ctrl_type;
	(void)pos_type;
	bool share_ant = rtwdev->efuse.share_ant;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	u32 phase = coex_dm->cur_ant_pos_type;

	if (!rtwdev->efuse.btcoex)
		return;

	switch (phase) {
	case COEX_SET_ANT_POWERON:
	case COEX_SET_ANT_INIT:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		rtw_write8_set(rtwdev, REG_GNT_BT, BIT_PTA_SW_CTL);

		rtw_write8(rtwdev, REG_RFE_CTRL8,
			   share_ant ? PTA_CTRL_PIN : DPDT_CTRL_PIN);
		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, 0x30000000, 0x1);
		break;
	case COEX_SET_ANT_WONLY:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		rtw_write8_clr(rtwdev, REG_GNT_BT, BIT_PTA_SW_CTL);

		rtw_write8(rtwdev, REG_RFE_CTRL8, DPDT_CTRL_PIN);
		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, 0x30000000, 0x1);
		break;
	case COEX_SET_ANT_2G:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		rtw_write8_clr(rtwdev, REG_GNT_BT, BIT_PTA_SW_CTL);

		rtw_write8(rtwdev, REG_RFE_CTRL8,
			   share_ant ? PTA_CTRL_PIN : DPDT_CTRL_PIN);
		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, 0x30000000, 0x1);
		break;
	case COEX_SET_ANT_5G:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_set(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		rtw_write8_set(rtwdev, REG_GNT_BT, BIT_PTA_SW_CTL);

		rtw_write8(rtwdev, REG_RFE_CTRL8, DPDT_CTRL_PIN);
		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, 0x30000000,
				 share_ant ? 0x2 : 0x1);
		break;
	case COEX_SET_ANT_WOFF:
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_SEL_EN);
		rtw_write32_clr(rtwdev, REG_LED_CFG, BIT_DPDT_WL_SEL);
		rtw_write8_set(rtwdev, REG_GNT_BT, BIT_PTA_SW_CTL);

		rtw_write8(rtwdev, REG_RFE_CTRL8, DPDT_CTRL_PIN);
		rtw_write32_mask(rtwdev, REG_RFE_CTRL8, 0x30000000,
				 share_ant ? 0x2 : 0x1);
		break;
	default:
		rtw_warn(rtwdev, "%s: not handling phase %d\n",
			 __func__, phase);
		break;
	}
}


static void rtw8821a_coex_cfg_gnt_fix(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
}


static void rtw8821a_coex_cfg_gnt_debug(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
}


static void rtw8821a_coex_cfg_rfe_type(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;

	coex_rfe->ant_switch_exist = true;
}


static void rtw8821a_coex_cfg_wl_tx_power(struct rtw_dev *rtwdev, u8 wl_pwr)
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


static void rtw8821a_coex_cfg_wl_rx_gain(struct rtw_dev *rtwdev, bool low_gain)
{
	(void)rtwdev;
	(void)low_gain;
}
