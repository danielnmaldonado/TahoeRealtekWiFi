/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

/* Copyright(c)2018-2019 Realtek Corporation. Pinned function bodies; checked adapter in RTW88PHYOperating.c. */

#define RA_FLOOR_TABLE_SIZE	7
#define RA_FLOOR_UP_GAP		3
#define DIG_PERF_FA_TH_LOW			250
#define DIG_PERF_FA_TH_HIGH			500
#define DIG_PERF_FA_TH_EXTRA_HIGH		750
#define DIG_PERF_MAX				0x5a
#define DIG_PERF_MID				0x40
#define DIG_CVRG_FA_TH_LOW			2000
#define DIG_CVRG_FA_TH_HIGH			4000
#define DIG_CVRG_FA_TH_EXTRA_HIGH		5000
#define DIG_CVRG_MAX				0x2a
#define DIG_CVRG_MID				0x26
#define DIG_CVRG_MIN				0x1c
#define DIG_RSSI_GAIN_OFFSET			15
#define CCK_PD_FA_LV1_MIN	1000
#define CCK_PD_FA_LV0_MAX	500
#define CCK_PD_IGI_LV4_VAL 0x38
#define CCK_PD_IGI_LV3_VAL 0x2a
#define CCK_PD_IGI_LV2_VAL 0x24
#define CCK_PD_RSSI_LV4_VAL 32
#define CCK_PD_RSSI_LV3_VAL 32
#define CCK_PD_RSSI_LV2_VAL 24
#define FRAC_BITS 3
#define bcd_to_dec_pwr_by_rate(val, i) bcd2bin(val >> (i * 8))

static const u8 rtw_cck_rates[];

static const u8 rtw_ofdm_rates[];

static const u8 rtw_ht_1s_rates[];

static const u8 rtw_ht_2s_rates[];

static const u8 rtw_vht_1s_rates[];

static const u8 rtw_vht_2s_rates[];

static const u8 rtw_ht_3s_rates[];

static const u8 rtw_ht_4s_rates[];

static const u8 rtw_vht_3s_rates[];

static const u8 rtw_vht_4s_rates[];

static const u8 * const rtw_rate_section[RTW_RATE_SECTION_NUM];

static const u8 rtw_rate_size[RTW_RATE_SECTION_NUM];

static const u8 rtw_channel_idx_5g[RTW_MAX_CHANNEL_NUM_5G];

static const u8 rtw_cck_rates[] = { DESC_RATE1M, DESC_RATE2M, DESC_RATE5_5M, DESC_RATE11M };

static const u8 rtw_ofdm_rates[] = {
	DESC_RATE6M,  DESC_RATE9M,  DESC_RATE12M,
	DESC_RATE18M, DESC_RATE24M, DESC_RATE36M,
	DESC_RATE48M, DESC_RATE54M
};

static const u8 rtw_ht_1s_rates[] = {
	DESC_RATEMCS0, DESC_RATEMCS1, DESC_RATEMCS2,
	DESC_RATEMCS3, DESC_RATEMCS4, DESC_RATEMCS5,
	DESC_RATEMCS6, DESC_RATEMCS7
};

static const u8 rtw_ht_2s_rates[] = {
	DESC_RATEMCS8,  DESC_RATEMCS9,  DESC_RATEMCS10,
	DESC_RATEMCS11, DESC_RATEMCS12, DESC_RATEMCS13,
	DESC_RATEMCS14, DESC_RATEMCS15
};

static const u8 rtw_vht_1s_rates[] = {
	DESC_RATEVHT1SS_MCS0, DESC_RATEVHT1SS_MCS1,
	DESC_RATEVHT1SS_MCS2, DESC_RATEVHT1SS_MCS3,
	DESC_RATEVHT1SS_MCS4, DESC_RATEVHT1SS_MCS5,
	DESC_RATEVHT1SS_MCS6, DESC_RATEVHT1SS_MCS7,
	DESC_RATEVHT1SS_MCS8, DESC_RATEVHT1SS_MCS9
};

static const u8 rtw_vht_2s_rates[] = {
	DESC_RATEVHT2SS_MCS0, DESC_RATEVHT2SS_MCS1,
	DESC_RATEVHT2SS_MCS2, DESC_RATEVHT2SS_MCS3,
	DESC_RATEVHT2SS_MCS4, DESC_RATEVHT2SS_MCS5,
	DESC_RATEVHT2SS_MCS6, DESC_RATEVHT2SS_MCS7,
	DESC_RATEVHT2SS_MCS8, DESC_RATEVHT2SS_MCS9
};

static const u8 rtw_ht_3s_rates[] = {
	DESC_RATEMCS16, DESC_RATEMCS17, DESC_RATEMCS18,
	DESC_RATEMCS19, DESC_RATEMCS20, DESC_RATEMCS21,
	DESC_RATEMCS22, DESC_RATEMCS23
};

static const u8 rtw_ht_4s_rates[] = {
	DESC_RATEMCS24, DESC_RATEMCS25, DESC_RATEMCS26,
	DESC_RATEMCS27, DESC_RATEMCS28, DESC_RATEMCS29,
	DESC_RATEMCS30, DESC_RATEMCS31
};

static const u8 rtw_vht_3s_rates[] = {
	DESC_RATEVHT3SS_MCS0, DESC_RATEVHT3SS_MCS1,
	DESC_RATEVHT3SS_MCS2, DESC_RATEVHT3SS_MCS3,
	DESC_RATEVHT3SS_MCS4, DESC_RATEVHT3SS_MCS5,
	DESC_RATEVHT3SS_MCS6, DESC_RATEVHT3SS_MCS7,
	DESC_RATEVHT3SS_MCS8, DESC_RATEVHT3SS_MCS9
};

static const u8 rtw_vht_4s_rates[] = {
	DESC_RATEVHT4SS_MCS0, DESC_RATEVHT4SS_MCS1,
	DESC_RATEVHT4SS_MCS2, DESC_RATEVHT4SS_MCS3,
	DESC_RATEVHT4SS_MCS4, DESC_RATEVHT4SS_MCS5,
	DESC_RATEVHT4SS_MCS6, DESC_RATEVHT4SS_MCS7,
	DESC_RATEVHT4SS_MCS8, DESC_RATEVHT4SS_MCS9
};

static const u8 * const rtw_rate_section[RTW_RATE_SECTION_NUM] = {
	rtw_cck_rates, rtw_ofdm_rates,
	rtw_ht_1s_rates, rtw_ht_2s_rates,
	rtw_vht_1s_rates, rtw_vht_2s_rates,
	rtw_ht_3s_rates, rtw_ht_4s_rates,
	rtw_vht_3s_rates, rtw_vht_4s_rates
};

static const u8 rtw_rate_size[RTW_RATE_SECTION_NUM] = {
	ARRAY_SIZE(rtw_cck_rates),
	ARRAY_SIZE(rtw_ofdm_rates),
	ARRAY_SIZE(rtw_ht_1s_rates),
	ARRAY_SIZE(rtw_ht_2s_rates),
	ARRAY_SIZE(rtw_vht_1s_rates),
	ARRAY_SIZE(rtw_vht_2s_rates),
	ARRAY_SIZE(rtw_ht_3s_rates),
	ARRAY_SIZE(rtw_ht_4s_rates),
	ARRAY_SIZE(rtw_vht_3s_rates),
	ARRAY_SIZE(rtw_vht_4s_rates)
};

static const u8 rtw_channel_idx_5g[RTW_MAX_CHANNEL_NUM_5G] = {
	36,  38,  40,  42,  44,  46,  48, /* Band 1 */
	52,  54,  56,  58,  60,  62,  64, /* Band 2 */
	100, 102, 104, 106, 108, 110, 112, /* Band 3 */
	116, 118, 120, 122, 124, 126, 128, /* Band 3 */
	132, 134, 136, 138, 140, 142, 144, /* Band 3 */
	149, 151, 153, 155, 157, 159, 161, /* Band 4 */
	165, 167, 169, 171, 173, 175, 177};

static void
__cfg_txpwr_lmt_by_alt(struct rtw_hal *hal, u8 regd, u8 regd_alt, u8 bw, u8 rs)
{
	u8 ch;

	for (ch = 0; ch < RTW_MAX_CHANNEL_NUM_2G; ch++)
		hal->tx_pwr_limit_2g[regd][bw][rs][ch] =
			hal->tx_pwr_limit_2g[regd_alt][bw][rs][ch];

	for (ch = 0; ch < RTW_MAX_CHANNEL_NUM_5G; ch++)
		hal->tx_pwr_limit_5g[regd][bw][rs][ch] =
			hal->tx_pwr_limit_5g[regd_alt][bw][rs][ch];
}

static void
__rtw_phy_tx_power_limit_config(struct rtw_hal *hal, u8 regd, u8 bw, u8 rs)
{
	s8 base;
	u8 ch;

	for (ch = 0; ch < RTW_MAX_CHANNEL_NUM_2G; ch++) {
		base = hal->tx_pwr_by_rate_base_2g[0][rs];
		hal->tx_pwr_limit_2g[regd][bw][rs][ch] -= base;
	}

	for (ch = 0; ch < RTW_MAX_CHANNEL_NUM_5G; ch++) {
		base = hal->tx_pwr_by_rate_base_5g[0][rs];
		hal->tx_pwr_limit_5g[regd][bw][rs][ch] -= base;
	}
}

static void
rtw_cfg_txpwr_lmt_by_alt(struct rtw_dev *rtwdev, u8 regd, u8 regd_alt)
{
	u8 bw, rs;

	for (bw = 0; bw < RTW_CHANNEL_WIDTH_MAX; bw++)
		for (rs = 0; rs < RTW_RATE_SECTION_NUM; rs++)
			__cfg_txpwr_lmt_by_alt(&rtwdev->hal, regd, regd_alt,
					       bw, rs);
}

static int rtw_channel_to_idx(u8 band, u8 channel)
{
	int ch_idx;
	u8 n_channel;

	if (band == PHY_BAND_2G) {
		ch_idx = channel - 1;
		n_channel = RTW_MAX_CHANNEL_NUM_2G;
	} else if (band == PHY_BAND_5G) {
		n_channel = RTW_MAX_CHANNEL_NUM_5G;
		for (ch_idx = 0; ch_idx < n_channel; ch_idx++)
			if (rtw_channel_idx_5g[ch_idx] == channel)
				break;
	} else {
		return -1;
	}

	if (ch_idx >= n_channel)
		return -1;

	return ch_idx;
}

static u8 rtw_get_channel_group(u8 channel, u8 rate)
{
	switch (channel) {
	default:
		NATIVE_WARN_ON(1);
		fallthrough;
	case 1:
	case 2:
	case 36:
	case 38:
	case 40:
	case 42:
		return 0;
	case 3:
	case 4:
	case 5:
	case 44:
	case 46:
	case 48:
	case 50:
		return 1;
	case 6:
	case 7:
	case 8:
	case 52:
	case 54:
	case 56:
	case 58:
		return 2;
	case 9:
	case 10:
	case 11:
	case 60:
	case 62:
	case 64:
		return 3;
	case 12:
	case 13:
	case 100:
	case 102:
	case 104:
	case 106:
		return 4;
	case 14:
		return rate <= DESC_RATE11M ? 5 : 4;
	case 108:
	case 110:
	case 112:
	case 114:
		return 5;
	case 116:
	case 118:
	case 120:
	case 122:
		return 6;
	case 124:
	case 126:
	case 128:
	case 130:
		return 7;
	case 132:
	case 134:
	case 136:
	case 138:
		return 8;
	case 140:
	case 142:
	case 144:
		return 9;
	case 149:
	case 151:
	case 153:
	case 155:
		return 10;
	case 157:
	case 159:
	case 161:
		return 11;
	case 165:
	case 167:
	case 169:
	case 171:
		return 12;
	case 173:
	case 175:
	case 177:
		return 13;
	}
}

static void rtw_get_tx_power_params(struct rtw_dev *rtwdev, u8 path, u8 rate, u8 bw,
			     u8 ch, u8 regd, struct rtw_power_params *pwr_param)
{
	struct rtw_hal *hal = &rtwdev->hal;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_txpwr_idx *pwr_idx;
	u8 group, band;
	s16 *base = &pwr_param->pwr_base;
	s8 *offset = &pwr_param->pwr_offset;
	s8 *limit = &pwr_param->pwr_limit;
	s8 *remnant = &pwr_param->pwr_remnant;
	s32 *sar = &pwr_param->pwr_sar;

	pwr_idx = &rtwdev->efuse.txpwr_idx_table[path];
	group = rtw_get_channel_group(ch, rate);

	/* base power index for 2.4G/5G */
	if (IS_CH_2G_BAND(ch)) {
		band = PHY_BAND_2G;
		*base = rtw_phy_get_2g_tx_power_index(rtwdev,
						      &pwr_idx->pwr_idx_2g,
						      bw, rate, group);
		*offset = hal->tx_pwr_by_rate_offset_2g[path][rate];
	} else {
		band = PHY_BAND_5G;
		*base = rtw_phy_get_5g_tx_power_index(rtwdev,
						      &pwr_idx->pwr_idx_5g,
						      bw, rate, group);
		*offset = hal->tx_pwr_by_rate_offset_5g[path][rate];
	}

	*limit = rtw_phy_get_tx_power_limit(rtwdev, band, bw, path,
					    rate, ch, regd);
	*remnant = rate <= DESC_RATE11M ? dm_info->txagc_remnant_cck :
					  dm_info->txagc_remnant_ofdm[path];
	*sar = rtw_phy_get_tx_power_sar(rtwdev, hal->sar_band, path, rate);
}

static void rtw_parse_tbl_bb_pg(struct rtw_dev *rtwdev, const struct rtw_table *tbl)
{
	const struct rtw_phy_pg_cfg_pair *p = tbl->data;
	const struct rtw_phy_pg_cfg_pair *end = p + tbl->size;

	for (; p < end; p++) {
		if (p->addr == 0xfe || p->addr == 0xffe) {
			msleep(50);
			continue;
		}
		rtw_phy_store_tx_power_by_rate(rtwdev, p->band, p->rf_path,
					       p->tx_num, p->addr, p->bitmask,
					       p->data);
	}
}

static void rtw_parse_tbl_txpwr_lmt(struct rtw_dev *rtwdev,
			     const struct rtw_table *tbl)
{
	const struct rtw_txpwr_lmt_cfg_pair *p = tbl->data;
	const struct rtw_txpwr_lmt_cfg_pair *end = p + tbl->size;
	u32 regd_cfg_flag = 0;
	u8 regd_alt;
	u8 i;

	for (; p < end; p++) {
		regd_cfg_flag |= BIT(p->regd);
		rtw_phy_set_tx_power_limit(rtwdev, p->regd, p->band,
					   p->bw, p->rs, p->ch, p->txpwr_lmt);
	}

	for (i = 0; i < RTW_REGD_MAX; i++) {
		if (i == RTW_REGD_WW)
			continue;

		if (regd_cfg_flag & BIT(i))
			continue;

		rtw_dbg(rtwdev, RTW_DBG_REGD,
			"txpwr regd %d does not be configured\n", i);

		if (rtw_regd_has_alt(i, &regd_alt) &&
		    regd_cfg_flag & BIT(regd_alt)) {
			rtw_dbg(rtwdev, RTW_DBG_REGD,
				"cfg txpwr regd %d by regd %d as alternative\n",
				i, regd_alt);

			rtw_cfg_txpwr_lmt_by_alt(rtwdev, i, regd_alt);
			continue;
		}

		rtw_dbg(rtwdev, RTW_DBG_REGD, "cfg txpwr regd %d by WW\n", i);
		rtw_cfg_txpwr_lmt_by_alt(rtwdev, i, RTW_REGD_WW);
	}

	rtw_xref_txpwr_lmt(rtwdev);
}

static void rtw_phy_config_swing_table(struct rtw_dev *rtwdev,
				struct rtw_swing_table *swing_table)
{
	const struct rtw_rfe_def *rfe_def = rtw_get_rfe_def(rtwdev);
	const struct rtw_pwr_track_tbl *tbl = rfe_def->pwr_track_tbl;
	u8 channel = rtwdev->hal.current_channel;

	if (IS_CH_2G_BAND(channel)) {
		if (rtwdev->dm_info.tx_rate <= DESC_RATE11M) {
			swing_table->p[RF_PATH_A] = tbl->pwrtrk_2g_ccka_p;
			swing_table->n[RF_PATH_A] = tbl->pwrtrk_2g_ccka_n;
			swing_table->p[RF_PATH_B] = tbl->pwrtrk_2g_cckb_p;
			swing_table->n[RF_PATH_B] = tbl->pwrtrk_2g_cckb_n;
			swing_table->p[RF_PATH_C] = tbl->pwrtrk_2g_cckc_p;
			swing_table->n[RF_PATH_C] = tbl->pwrtrk_2g_cckc_n;
			swing_table->p[RF_PATH_D] = tbl->pwrtrk_2g_cckd_p;
			swing_table->n[RF_PATH_D] = tbl->pwrtrk_2g_cckd_n;
		} else {
			swing_table->p[RF_PATH_A] = tbl->pwrtrk_2ga_p;
			swing_table->n[RF_PATH_A] = tbl->pwrtrk_2ga_n;
			swing_table->p[RF_PATH_B] = tbl->pwrtrk_2gb_p;
			swing_table->n[RF_PATH_B] = tbl->pwrtrk_2gb_n;
			swing_table->p[RF_PATH_C] = tbl->pwrtrk_2gc_p;
			swing_table->n[RF_PATH_C] = tbl->pwrtrk_2gc_n;
			swing_table->p[RF_PATH_D] = tbl->pwrtrk_2gd_p;
			swing_table->n[RF_PATH_D] = tbl->pwrtrk_2gd_n;
		}
	} else if (IS_CH_5G_BAND_1(channel) || IS_CH_5G_BAND_2(channel)) {
		swing_table->p[RF_PATH_A] = tbl->pwrtrk_5ga_p[RTW_PWR_TRK_5G_1];
		swing_table->n[RF_PATH_A] = tbl->pwrtrk_5ga_n[RTW_PWR_TRK_5G_1];
		swing_table->p[RF_PATH_B] = tbl->pwrtrk_5gb_p[RTW_PWR_TRK_5G_1];
		swing_table->n[RF_PATH_B] = tbl->pwrtrk_5gb_n[RTW_PWR_TRK_5G_1];
		swing_table->p[RF_PATH_C] = tbl->pwrtrk_5gc_p[RTW_PWR_TRK_5G_1];
		swing_table->n[RF_PATH_C] = tbl->pwrtrk_5gc_n[RTW_PWR_TRK_5G_1];
		swing_table->p[RF_PATH_D] = tbl->pwrtrk_5gd_p[RTW_PWR_TRK_5G_1];
		swing_table->n[RF_PATH_D] = tbl->pwrtrk_5gd_n[RTW_PWR_TRK_5G_1];
	} else if (IS_CH_5G_BAND_3(channel)) {
		swing_table->p[RF_PATH_A] = tbl->pwrtrk_5ga_p[RTW_PWR_TRK_5G_2];
		swing_table->n[RF_PATH_A] = tbl->pwrtrk_5ga_n[RTW_PWR_TRK_5G_2];
		swing_table->p[RF_PATH_B] = tbl->pwrtrk_5gb_p[RTW_PWR_TRK_5G_2];
		swing_table->n[RF_PATH_B] = tbl->pwrtrk_5gb_n[RTW_PWR_TRK_5G_2];
		swing_table->p[RF_PATH_C] = tbl->pwrtrk_5gc_p[RTW_PWR_TRK_5G_2];
		swing_table->n[RF_PATH_C] = tbl->pwrtrk_5gc_n[RTW_PWR_TRK_5G_2];
		swing_table->p[RF_PATH_D] = tbl->pwrtrk_5gd_p[RTW_PWR_TRK_5G_2];
		swing_table->n[RF_PATH_D] = tbl->pwrtrk_5gd_n[RTW_PWR_TRK_5G_2];
	} else if (IS_CH_5G_BAND_4(channel)) {
		swing_table->p[RF_PATH_A] = tbl->pwrtrk_5ga_p[RTW_PWR_TRK_5G_3];
		swing_table->n[RF_PATH_A] = tbl->pwrtrk_5ga_n[RTW_PWR_TRK_5G_3];
		swing_table->p[RF_PATH_B] = tbl->pwrtrk_5gb_p[RTW_PWR_TRK_5G_3];
		swing_table->n[RF_PATH_B] = tbl->pwrtrk_5gb_n[RTW_PWR_TRK_5G_3];
		swing_table->p[RF_PATH_C] = tbl->pwrtrk_5gc_p[RTW_PWR_TRK_5G_3];
		swing_table->n[RF_PATH_C] = tbl->pwrtrk_5gc_n[RTW_PWR_TRK_5G_3];
		swing_table->p[RF_PATH_D] = tbl->pwrtrk_5gd_p[RTW_PWR_TRK_5G_3];
		swing_table->n[RF_PATH_D] = tbl->pwrtrk_5gd_n[RTW_PWR_TRK_5G_3];
	} else {
		swing_table->p[RF_PATH_A] = tbl->pwrtrk_2ga_p;
		swing_table->n[RF_PATH_A] = tbl->pwrtrk_2ga_n;
		swing_table->p[RF_PATH_B] = tbl->pwrtrk_2gb_p;
		swing_table->n[RF_PATH_B] = tbl->pwrtrk_2gb_n;
		swing_table->p[RF_PATH_C] = tbl->pwrtrk_2gc_p;
		swing_table->n[RF_PATH_C] = tbl->pwrtrk_2gc_n;
		swing_table->p[RF_PATH_D] = tbl->pwrtrk_2gd_p;
		swing_table->n[RF_PATH_D] = tbl->pwrtrk_2gd_n;
	}
}

static s16 rtw_phy_get_2g_tx_power_index(struct rtw_dev *rtwdev,
					struct rtw_2g_txpwr_idx *pwr_idx_2g,
					enum rtw_bandwidth bandwidth,
					u8 rate, u8 group)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	bool above_2ss, above_3ss, above_4ss;
	u8 factor = chip->txgi_factor;
	bool mcs_rate;
	int tx_power;

	if (rate <= DESC_RATE11M)
		tx_power = pwr_idx_2g->cck_base[group];
	else
		tx_power = pwr_idx_2g->bw40_base[group];

	if (rate >= DESC_RATE6M && rate <= DESC_RATE54M)
		tx_power += pwr_idx_2g->ht_1s_diff.ofdm * factor;

	mcs_rate = (rate >= DESC_RATEMCS0 && rate <= DESC_RATEMCS31) ||
		   (rate >= DESC_RATEVHT1SS_MCS0 &&
		    rate <= DESC_RATEVHT4SS_MCS9);
	above_2ss = (rate >= DESC_RATEMCS8 && rate <= DESC_RATEMCS31) ||
		    (rate >= DESC_RATEVHT2SS_MCS0);
	above_3ss = (rate >= DESC_RATEMCS16 && rate <= DESC_RATEMCS31) ||
		    (rate >= DESC_RATEVHT3SS_MCS0);
	above_4ss = (rate >= DESC_RATEMCS24 && rate <= DESC_RATEMCS31) ||
		    (rate >= DESC_RATEVHT4SS_MCS0);

	if (!mcs_rate)
		return (s16)tx_power;

	switch (bandwidth) {
	default:
		WARN_ON(1);
		fallthrough;
	case RTW_CHANNEL_WIDTH_20:
		tx_power += pwr_idx_2g->ht_1s_diff.bw20 * factor;
		if (above_2ss)
			tx_power += pwr_idx_2g->ht_2s_diff.bw20 * factor;
		if (above_3ss)
			tx_power += pwr_idx_2g->ht_3s_diff.bw20 * factor;
		if (above_4ss)
			tx_power += pwr_idx_2g->ht_4s_diff.bw20 * factor;
		break;
	case RTW_CHANNEL_WIDTH_40:
		/* bw40 is the base power */
		if (above_2ss)
			tx_power += pwr_idx_2g->ht_2s_diff.bw40 * factor;
		if (above_3ss)
			tx_power += pwr_idx_2g->ht_3s_diff.bw40 * factor;
		if (above_4ss)
			tx_power += pwr_idx_2g->ht_4s_diff.bw40 * factor;
		break;
	}

	return (s16)tx_power;
}

static s16 rtw_phy_get_5g_tx_power_index(struct rtw_dev *rtwdev,
					struct rtw_5g_txpwr_idx *pwr_idx_5g,
					enum rtw_bandwidth bandwidth,
					u8 rate, u8 group)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	bool above_2ss, above_3ss, above_4ss;
	u8 factor = chip->txgi_factor;
	u8 upper, lower;
	bool mcs_rate;
	int tx_power;

	tx_power = pwr_idx_5g->bw40_base[group];

	mcs_rate = (rate >= DESC_RATEMCS0 && rate <= DESC_RATEMCS31) ||
		   (rate >= DESC_RATEVHT1SS_MCS0 &&
		    rate <= DESC_RATEVHT4SS_MCS9);
	above_2ss = (rate >= DESC_RATEMCS8 && rate <= DESC_RATEMCS31) ||
		    (rate >= DESC_RATEVHT2SS_MCS0);
	above_3ss = (rate >= DESC_RATEMCS16 && rate <= DESC_RATEMCS31) ||
		    (rate >= DESC_RATEVHT3SS_MCS0);
	above_4ss = (rate >= DESC_RATEMCS24 && rate <= DESC_RATEMCS31) ||
		    (rate >= DESC_RATEVHT4SS_MCS0);

	if (!mcs_rate) {
		tx_power += pwr_idx_5g->ht_1s_diff.ofdm * factor;
		return (s16)tx_power;
	}

	switch (bandwidth) {
	default:
		WARN_ON(1);
		fallthrough;
	case RTW_CHANNEL_WIDTH_20:
		tx_power += pwr_idx_5g->ht_1s_diff.bw20 * factor;
		if (above_2ss)
			tx_power += pwr_idx_5g->ht_2s_diff.bw20 * factor;
		if (above_3ss)
			tx_power += pwr_idx_5g->ht_3s_diff.bw20 * factor;
		if (above_4ss)
			tx_power += pwr_idx_5g->ht_4s_diff.bw20 * factor;
		break;
	case RTW_CHANNEL_WIDTH_40:
		/* bw40 is the base power */
		if (above_2ss)
			tx_power += pwr_idx_5g->ht_2s_diff.bw40 * factor;
		if (above_3ss)
			tx_power += pwr_idx_5g->ht_3s_diff.bw40 * factor;
		if (above_4ss)
			tx_power += pwr_idx_5g->ht_4s_diff.bw40 * factor;
		break;
	case RTW_CHANNEL_WIDTH_80:
		/* the base idx of bw80 is the average of bw40+/bw40- */
		lower = pwr_idx_5g->bw40_base[group];
		upper = pwr_idx_5g->bw40_base[group + 1];

		tx_power = (lower + upper) / 2;
		tx_power += pwr_idx_5g->vht_1s_diff.bw80 * factor;
		if (above_2ss)
			tx_power += pwr_idx_5g->vht_2s_diff.bw80 * factor;
		if (above_3ss)
			tx_power += pwr_idx_5g->vht_3s_diff.bw80 * factor;
		if (above_4ss)
			tx_power += pwr_idx_5g->vht_4s_diff.bw80 * factor;
		break;
	}

	return (s16)tx_power;
}

static s8 rtw_phy_get_dis_dpd_by_rate_diff(struct rtw_dev *rtwdev, u16 rate)
{
	const struct rtw_chip_info *chip = rtwdev->chip;
	s8 dpd_diff = 0;

	if (!chip->en_dis_dpd)
		return 0;

#define RTW_DPD_RATE_CHECK(_rate)					\
	case DESC_RATE ## _rate:					\
	if (DIS_DPD_RATE ## _rate & chip->dpd_ratemask)			\
		dpd_diff = -6 * chip->txgi_factor;			\
	break

	switch (rate) {
	RTW_DPD_RATE_CHECK(6M);
	RTW_DPD_RATE_CHECK(9M);
	RTW_DPD_RATE_CHECK(MCS0);
	RTW_DPD_RATE_CHECK(MCS1);
	RTW_DPD_RATE_CHECK(MCS8);
	RTW_DPD_RATE_CHECK(MCS9);
	RTW_DPD_RATE_CHECK(VHT1SS_MCS0);
	RTW_DPD_RATE_CHECK(VHT1SS_MCS1);
	RTW_DPD_RATE_CHECK(VHT2SS_MCS0);
	RTW_DPD_RATE_CHECK(VHT2SS_MCS1);
	}
#undef RTW_DPD_RATE_CHECK

	return dpd_diff;
}

static void
rtw_phy_get_rate_values_of_txpwr_by_rate(struct rtw_dev *rtwdev,
					 u32 addr, u32 mask, u32 val, u8 *rate,
					 u8 *pwr_by_rate, u8 *rate_num)
{
	int i;

	switch (addr) {
	case 0xE00:
	case 0x830:
		rate[0] = DESC_RATE6M;
		rate[1] = DESC_RATE9M;
		rate[2] = DESC_RATE12M;
		rate[3] = DESC_RATE18M;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xE04:
	case 0x834:
		rate[0] = DESC_RATE24M;
		rate[1] = DESC_RATE36M;
		rate[2] = DESC_RATE48M;
		rate[3] = DESC_RATE54M;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xE08:
		rate[0] = DESC_RATE1M;
		pwr_by_rate[0] = bcd_to_dec_pwr_by_rate(val, 1);
		*rate_num = 1;
		break;
	case 0x86C:
		if (mask == 0xffffff00) {
			rate[0] = DESC_RATE2M;
			rate[1] = DESC_RATE5_5M;
			rate[2] = DESC_RATE11M;
			for (i = 1; i < 4; ++i)
				pwr_by_rate[i - 1] =
					tbl_to_dec_pwr_by_rate(rtwdev, val, i);
			*rate_num = 3;
		} else if (mask == 0x000000ff) {
			rate[0] = DESC_RATE11M;
			pwr_by_rate[0] = bcd_to_dec_pwr_by_rate(val, 0);
			*rate_num = 1;
		}
		break;
	case 0xE10:
	case 0x83C:
		rate[0] = DESC_RATEMCS0;
		rate[1] = DESC_RATEMCS1;
		rate[2] = DESC_RATEMCS2;
		rate[3] = DESC_RATEMCS3;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xE14:
	case 0x848:
		rate[0] = DESC_RATEMCS4;
		rate[1] = DESC_RATEMCS5;
		rate[2] = DESC_RATEMCS6;
		rate[3] = DESC_RATEMCS7;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xE18:
	case 0x84C:
		rate[0] = DESC_RATEMCS8;
		rate[1] = DESC_RATEMCS9;
		rate[2] = DESC_RATEMCS10;
		rate[3] = DESC_RATEMCS11;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xE1C:
	case 0x868:
		rate[0] = DESC_RATEMCS12;
		rate[1] = DESC_RATEMCS13;
		rate[2] = DESC_RATEMCS14;
		rate[3] = DESC_RATEMCS15;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0x838:
		rate[0] = DESC_RATE1M;
		rate[1] = DESC_RATE2M;
		rate[2] = DESC_RATE5_5M;
		for (i = 1; i < 4; ++i)
			pwr_by_rate[i - 1] = tbl_to_dec_pwr_by_rate(rtwdev,
								    val, i);
		*rate_num = 3;
		break;
	case 0xC20:
	case 0xE20:
	case 0x1820:
	case 0x1A20:
		rate[0] = DESC_RATE1M;
		rate[1] = DESC_RATE2M;
		rate[2] = DESC_RATE5_5M;
		rate[3] = DESC_RATE11M;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC24:
	case 0xE24:
	case 0x1824:
	case 0x1A24:
		rate[0] = DESC_RATE6M;
		rate[1] = DESC_RATE9M;
		rate[2] = DESC_RATE12M;
		rate[3] = DESC_RATE18M;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC28:
	case 0xE28:
	case 0x1828:
	case 0x1A28:
		rate[0] = DESC_RATE24M;
		rate[1] = DESC_RATE36M;
		rate[2] = DESC_RATE48M;
		rate[3] = DESC_RATE54M;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC2C:
	case 0xE2C:
	case 0x182C:
	case 0x1A2C:
		rate[0] = DESC_RATEMCS0;
		rate[1] = DESC_RATEMCS1;
		rate[2] = DESC_RATEMCS2;
		rate[3] = DESC_RATEMCS3;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC30:
	case 0xE30:
	case 0x1830:
	case 0x1A30:
		rate[0] = DESC_RATEMCS4;
		rate[1] = DESC_RATEMCS5;
		rate[2] = DESC_RATEMCS6;
		rate[3] = DESC_RATEMCS7;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC34:
	case 0xE34:
	case 0x1834:
	case 0x1A34:
		rate[0] = DESC_RATEMCS8;
		rate[1] = DESC_RATEMCS9;
		rate[2] = DESC_RATEMCS10;
		rate[3] = DESC_RATEMCS11;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC38:
	case 0xE38:
	case 0x1838:
	case 0x1A38:
		rate[0] = DESC_RATEMCS12;
		rate[1] = DESC_RATEMCS13;
		rate[2] = DESC_RATEMCS14;
		rate[3] = DESC_RATEMCS15;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC3C:
	case 0xE3C:
	case 0x183C:
	case 0x1A3C:
		rate[0] = DESC_RATEVHT1SS_MCS0;
		rate[1] = DESC_RATEVHT1SS_MCS1;
		rate[2] = DESC_RATEVHT1SS_MCS2;
		rate[3] = DESC_RATEVHT1SS_MCS3;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC40:
	case 0xE40:
	case 0x1840:
	case 0x1A40:
		rate[0] = DESC_RATEVHT1SS_MCS4;
		rate[1] = DESC_RATEVHT1SS_MCS5;
		rate[2] = DESC_RATEVHT1SS_MCS6;
		rate[3] = DESC_RATEVHT1SS_MCS7;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC44:
	case 0xE44:
	case 0x1844:
	case 0x1A44:
		rate[0] = DESC_RATEVHT1SS_MCS8;
		rate[1] = DESC_RATEVHT1SS_MCS9;
		rate[2] = DESC_RATEVHT2SS_MCS0;
		rate[3] = DESC_RATEVHT2SS_MCS1;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC48:
	case 0xE48:
	case 0x1848:
	case 0x1A48:
		rate[0] = DESC_RATEVHT2SS_MCS2;
		rate[1] = DESC_RATEVHT2SS_MCS3;
		rate[2] = DESC_RATEVHT2SS_MCS4;
		rate[3] = DESC_RATEVHT2SS_MCS5;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xC4C:
	case 0xE4C:
	case 0x184C:
	case 0x1A4C:
		rate[0] = DESC_RATEVHT2SS_MCS6;
		rate[1] = DESC_RATEVHT2SS_MCS7;
		rate[2] = DESC_RATEVHT2SS_MCS8;
		rate[3] = DESC_RATEVHT2SS_MCS9;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xCD8:
	case 0xED8:
	case 0x18D8:
	case 0x1AD8:
		rate[0] = DESC_RATEMCS16;
		rate[1] = DESC_RATEMCS17;
		rate[2] = DESC_RATEMCS18;
		rate[3] = DESC_RATEMCS19;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xCDC:
	case 0xEDC:
	case 0x18DC:
	case 0x1ADC:
		rate[0] = DESC_RATEMCS20;
		rate[1] = DESC_RATEMCS21;
		rate[2] = DESC_RATEMCS22;
		rate[3] = DESC_RATEMCS23;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xCE0:
	case 0xEE0:
	case 0x18E0:
	case 0x1AE0:
		rate[0] = DESC_RATEVHT3SS_MCS0;
		rate[1] = DESC_RATEVHT3SS_MCS1;
		rate[2] = DESC_RATEVHT3SS_MCS2;
		rate[3] = DESC_RATEVHT3SS_MCS3;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xCE4:
	case 0xEE4:
	case 0x18E4:
	case 0x1AE4:
		rate[0] = DESC_RATEVHT3SS_MCS4;
		rate[1] = DESC_RATEVHT3SS_MCS5;
		rate[2] = DESC_RATEVHT3SS_MCS6;
		rate[3] = DESC_RATEVHT3SS_MCS7;
		for (i = 0; i < 4; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 4;
		break;
	case 0xCE8:
	case 0xEE8:
	case 0x18E8:
	case 0x1AE8:
		rate[0] = DESC_RATEVHT3SS_MCS8;
		rate[1] = DESC_RATEVHT3SS_MCS9;
		for (i = 0; i < 2; ++i)
			pwr_by_rate[i] = tbl_to_dec_pwr_by_rate(rtwdev, val, i);
		*rate_num = 2;
		break;
	default:
		rtw_warn(rtwdev, "invalid tx power index addr 0x%08x\n", addr);
		break;
	}
}

static u8
rtw_phy_get_tx_power_index(struct rtw_dev *rtwdev, u8 rf_path, u8 rate,
			   enum rtw_bandwidth bandwidth, u8 channel, u8 regd)
{
	struct rtw_power_params pwr_param = {0};
	int tx_power;
	s32 offset;

	rtw_get_tx_power_params(rtwdev, rf_path, rate, bandwidth,
				channel, regd, &pwr_param);

	tx_power = pwr_param.pwr_base;
	offset = min3(pwr_param.pwr_offset,
		      pwr_param.pwr_limit,
		      pwr_param.pwr_sar);

	if (rtwdev->chip->en_dis_dpd)
		offset += rtw_phy_get_dis_dpd_by_rate_diff(rtwdev, rate);

	tx_power += offset + pwr_param.pwr_remnant;

	if (tx_power > rtwdev->chip->max_power_index)
		tx_power = rtwdev->chip->max_power_index;

	return native_power_index(rtwdev,(u8)clamp_t(int,tx_power,0,rtwdev->chip->max_power_index),rate,&pwr_param);
}

static s8 rtw_phy_get_tx_power_limit(struct rtw_dev *rtwdev, u8 band,
				     enum rtw_bandwidth bw, u8 rf_path,
				     u8 rate, u8 channel, u8 regd)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 *cch_by_bw = hal->cch_by_bw;
	s8 power_limit = (s8)rtwdev->chip->max_power_index;
	u8 rs = rtw_phy_rate_to_rate_section(rate);
	int ch_idx;
	u8 cur_bw, cur_ch;
	s8 cur_lmt;

	if (regd > RTW_REGD_WW)
		return power_limit;

	if (rs == RTW_RATE_SECTION_NUM)
		goto err;

	/* only 20M BW with cck and ofdm */
	if (rs == RTW_RATE_SECTION_CCK || rs == RTW_RATE_SECTION_OFDM)
		bw = RTW_CHANNEL_WIDTH_20;

	/* only 20/40M BW with ht */
	if (rate >= DESC_RATEMCS0 && rate <= DESC_RATEMCS31)
		bw = min_t(u8, bw, RTW_CHANNEL_WIDTH_40);

	/* select min power limit among [20M BW ~ current BW] */
	for (cur_bw = RTW_CHANNEL_WIDTH_20; cur_bw <= bw; cur_bw++) {
		cur_ch = cch_by_bw[cur_bw];

		ch_idx = rtw_channel_to_idx(band, cur_ch);
		if (ch_idx < 0)
			goto err;

		cur_lmt = cur_ch <= RTW_MAX_CHANNEL_NUM_2G ?
			hal->tx_pwr_limit_2g[regd][cur_bw][rs][ch_idx] :
			hal->tx_pwr_limit_5g[regd][cur_bw][rs][ch_idx];

		power_limit = min_t(s8, cur_lmt, power_limit);
	}

	return power_limit;

err:
	WARN(1, "invalid arguments, band=%d, bw=%d, path=%d, rate=%d, ch=%d\n",
	     band, bw, rf_path, rate, channel);
	return (s8)rtwdev->chip->max_power_index;
}

static s32 rtw_phy_get_tx_power_sar(struct rtw_dev *rtwdev, u8 sar_band,
				   u8 rf_path, u8 rate)
{
	u8 rs = rtw_phy_rate_to_rate_section(rate);
	struct rtw_sar_arg arg = {
		.sar_band = sar_band,
		.path = rf_path,
		.rs = rs,
	};

	if (rs == RTW_RATE_SECTION_NUM)
		goto err;

	return rtw_query_sar(rtwdev, &arg);

err:
	WARN(1, "invalid arguments, sar_band=%d, path=%d, rate=%d\n",
	     sar_band, rf_path, rate);
	return (s8)rtwdev->chip->max_power_index;
}

static void rtw_phy_init_tx_power(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 regd, path, rate, rs, bw;

	/* init tx power by rate offset */
	for (path = 0; path < RTW_RF_PATH_MAX; path++) {
		for (rate = 0; rate < DESC_RATE_MAX; rate++) {
			hal->tx_pwr_by_rate_offset_2g[path][rate] = 0;
			hal->tx_pwr_by_rate_offset_5g[path][rate] = 0;
		}
	}

	/* init tx power limit */
	for (regd = 0; regd < RTW_REGD_MAX; regd++)
		for (bw = 0; bw < RTW_CHANNEL_WIDTH_MAX; bw++)
			for (rs = 0; rs < RTW_RATE_SECTION_NUM; rs++)
				rtw_phy_init_tx_power_limit(rtwdev, regd, bw,
							    rs);
}

static void rtw_phy_init_tx_power_limit(struct rtw_dev *rtwdev,
					u8 regd, u8 bw, u8 rs)
{
	struct rtw_hal *hal = &rtwdev->hal;
	s8 max_power_index = (s8)rtwdev->chip->max_power_index;
	u8 ch;

	/* 2.4G channels */
	for (ch = 0; ch < RTW_MAX_CHANNEL_NUM_2G; ch++)
		hal->tx_pwr_limit_2g[regd][bw][rs][ch] = max_power_index;

	/* 5G channels */
	for (ch = 0; ch < RTW_MAX_CHANNEL_NUM_5G; ch++)
		hal->tx_pwr_limit_5g[regd][bw][rs][ch] = max_power_index;
}

static void rtw_phy_pwrtrack_avg(struct rtw_dev *rtwdev, u8 thermal, u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	ewma_thermal_add(&dm_info->avg_thermal[path], thermal);
	dm_info->thermal_avg[path] =
		ewma_thermal_read(&dm_info->avg_thermal[path]);
}

static u8 rtw_phy_pwrtrack_get_delta(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 therm_avg, therm_efuse, therm_delta;

	therm_avg = dm_info->thermal_avg[path];
	therm_efuse = rtwdev->efuse.thermal_meter[path];
	therm_delta = abs(therm_avg - therm_efuse);

	return min_t(u8, therm_delta, RTW_PWR_TRK_TBL_SZ - 1);
}

static s8 rtw_phy_pwrtrack_get_pwridx(struct rtw_dev *rtwdev,
			       struct rtw_swing_table *swing_table,
			       u8 tbl_path, u8 therm_path, u8 delta)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	const u8 *delta_swing_table_idx_pos;
	const u8 *delta_swing_table_idx_neg;

	if (delta >= RTW_PWR_TRK_TBL_SZ) {
		rtw_warn(rtwdev, "power track table overflow\n");
		return 0;
	}

	if (!swing_table) {
		rtw_warn(rtwdev, "swing table not configured\n");
		return 0;
	}

	delta_swing_table_idx_pos = swing_table->p[tbl_path];
	delta_swing_table_idx_neg = swing_table->n[tbl_path];

	if (!delta_swing_table_idx_pos || !delta_swing_table_idx_neg) {
		rtw_warn(rtwdev, "invalid swing table index\n");
		return 0;
	}

	if (dm_info->thermal_avg[therm_path] >
	    rtwdev->efuse.thermal_meter[therm_path])
		return delta_swing_table_idx_pos[delta];
	else
		return -delta_swing_table_idx_neg[delta];
}

static bool rtw_phy_pwrtrack_need_iqk(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 delta_iqk;

	delta_iqk = abs(dm_info->thermal_avg[0] - dm_info->thermal_meter_k);
	if (delta_iqk >= rtwdev->chip->iqk_threshold) {
		dm_info->thermal_meter_k = dm_info->thermal_avg[0];
		return true;
	}
	return false;
}

static bool rtw_phy_pwrtrack_need_lck(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 delta_lck;

	delta_lck = abs(dm_info->thermal_avg[0] - dm_info->thermal_meter_lck);
	if (delta_lck >= rtwdev->chip->lck_threshold) {
		dm_info->thermal_meter_lck = dm_info->thermal_avg[0];
		return true;
	}
	return false;
}

static bool rtw_phy_pwrtrack_thermal_changed(struct rtw_dev *rtwdev, u8 thermal,
				      u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 avg = ewma_thermal_read(&dm_info->avg_thermal[path]);

	if (avg == thermal)
		return false;

	return true;
}

static u8 rtw_phy_rate_to_rate_section(u8 rate)
{
	if (rate >= DESC_RATE1M && rate <= DESC_RATE11M)
		return RTW_RATE_SECTION_CCK;
	else if (rate >= DESC_RATE6M && rate <= DESC_RATE54M)
		return RTW_RATE_SECTION_OFDM;
	else if (rate >= DESC_RATEMCS0 && rate <= DESC_RATEMCS7)
		return RTW_RATE_SECTION_HT_1S;
	else if (rate >= DESC_RATEMCS8 && rate <= DESC_RATEMCS15)
		return RTW_RATE_SECTION_HT_2S;
	else if (rate >= DESC_RATEMCS16 && rate <= DESC_RATEMCS23)
		return RTW_RATE_SECTION_HT_3S;
	else if (rate >= DESC_RATEMCS24 && rate <= DESC_RATEMCS31)
		return RTW_RATE_SECTION_HT_4S;
	else if (rate >= DESC_RATEVHT1SS_MCS0 && rate <= DESC_RATEVHT1SS_MCS9)
		return RTW_RATE_SECTION_VHT_1S;
	else if (rate >= DESC_RATEVHT2SS_MCS0 && rate <= DESC_RATEVHT2SS_MCS9)
		return RTW_RATE_SECTION_VHT_2S;
	else if (rate >= DESC_RATEVHT3SS_MCS0 && rate <= DESC_RATEVHT3SS_MCS9)
		return RTW_RATE_SECTION_VHT_3S;
	else if (rate >= DESC_RATEVHT4SS_MCS0 && rate <= DESC_RATEVHT4SS_MCS9)
		return RTW_RATE_SECTION_VHT_4S;
	else
		return RTW_RATE_SECTION_NUM;
}

static void rtw_phy_set_tx_power_limit(struct rtw_dev *rtwdev, u8 regd, u8 band,
				       u8 bw, u8 rs, u8 ch, s8 pwr_limit)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 max_power_index = rtwdev->chip->max_power_index;
	s8 ww;
	int ch_idx;

	pwr_limit = clamp_t(s8, pwr_limit,
			    -max_power_index, max_power_index);
	ch_idx = rtw_channel_to_idx(band, ch);

	if (regd >= RTW_REGD_MAX || bw >= RTW_CHANNEL_WIDTH_MAX ||
	    rs >= RTW_RATE_SECTION_NUM || ch_idx < 0) {
		WARN(1,
		     "wrong txpwr_lmt regd=%u, band=%u bw=%u, rs=%u, ch_idx=%u, pwr_limit=%d\n",
		     regd, band, bw, rs, ch_idx, pwr_limit);
		return;
	}

	if (band == PHY_BAND_2G) {
		hal->tx_pwr_limit_2g[regd][bw][rs][ch_idx] = pwr_limit;
		ww = hal->tx_pwr_limit_2g[RTW_REGD_WW][bw][rs][ch_idx];
		ww = min_t(s8, ww, pwr_limit);
		hal->tx_pwr_limit_2g[RTW_REGD_WW][bw][rs][ch_idx] = ww;
	} else if (band == PHY_BAND_5G) {
		hal->tx_pwr_limit_5g[regd][bw][rs][ch_idx] = pwr_limit;
		ww = hal->tx_pwr_limit_5g[RTW_REGD_WW][bw][rs][ch_idx];
		ww = min_t(s8, ww, pwr_limit);
		hal->tx_pwr_limit_5g[RTW_REGD_WW][bw][rs][ch_idx] = ww;
	}
}

static void rtw_phy_store_tx_power_by_rate(struct rtw_dev *rtwdev,
					   u32 band, u32 rfpath, u32 txnum,
					   u32 regaddr, u32 bitmask, u32 data)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 rate_num = 0;
	u8 rate;
	u8 rates[RTW_RF_PATH_MAX] = {0};
	s8 offset;
	s8 pwr_by_rate[RTW_RF_PATH_MAX] = {0};
	int i;

	rtw_phy_get_rate_values_of_txpwr_by_rate(rtwdev, regaddr, bitmask, data,
						 rates, pwr_by_rate, &rate_num);

	if (WARN_ON(rfpath >= RTW_RF_PATH_MAX ||
		    (band != PHY_BAND_2G && band != PHY_BAND_5G) ||
		    rate_num > RTW_RF_PATH_MAX))
		return;

	for (i = 0; i < rate_num; i++) {
		offset = pwr_by_rate[i];
		rate = rates[i];
		if (band == PHY_BAND_2G)
			hal->tx_pwr_by_rate_offset_2g[rfpath][rate] = offset;
		else
			hal->tx_pwr_by_rate_offset_5g[rfpath][rate] = offset;
	}
}

static void rtw_phy_tx_power_by_rate_config(struct rtw_hal *hal)
{
	u8 path, rs;

	for (path = 0; path < RTW_RF_PATH_MAX; path++)
		for (rs = 0; rs < RTW_RATE_SECTION_NUM; rs++)
			rtw_phy_tx_power_by_rate_config_by_path(hal, path, rs,
				rtw_rate_size[rs], rtw_rate_section[rs]);
}

static void
rtw_phy_tx_power_by_rate_config_by_path(struct rtw_hal *hal, u8 path,
					u8 rs, u8 size, const u8 *rates)
{
	u8 rate;
	u8 base_idx, rate_idx;
	s8 base_2g, base_5g;

	if (size == 10) /* VHT rates */
		base_idx = rates[size - 3];
	else
		base_idx = rates[size - 1];
	base_2g = hal->tx_pwr_by_rate_offset_2g[path][base_idx];
	base_5g = hal->tx_pwr_by_rate_offset_5g[path][base_idx];
	hal->tx_pwr_by_rate_base_2g[path][rs] = base_2g;
	hal->tx_pwr_by_rate_base_5g[path][rs] = base_5g;
	for (rate = 0; rate < size; rate++) {
		rate_idx = rates[rate];
		hal->tx_pwr_by_rate_offset_2g[path][rate_idx] -= base_2g;
		hal->tx_pwr_by_rate_offset_5g[path][rate_idx] -= base_5g;
	}
}

static void rtw_phy_tx_power_limit_config(struct rtw_hal *hal)
{
	u8 regd, bw, rs;

	/* default at channel 1 */
	hal->cch_by_bw[RTW_CHANNEL_WIDTH_20] = 1;

	for (regd = 0; regd < RTW_REGD_MAX; regd++)
		for (bw = 0; bw < RTW_CHANNEL_WIDTH_MAX; bw++)
			for (rs = 0; rs < RTW_RATE_SECTION_NUM; rs++)
				__rtw_phy_tx_power_limit_config(hal, regd, bw, rs);
}

static void rtw_set_channel_mac(struct rtw_dev *rtwdev, u8 channel, u8 bw,
			 u8 primary_ch_idx)
{
	u8 txsc40 = 0, txsc20 = 0;
	u32 value32;
	u8 value8;

	txsc20 = primary_ch_idx;
	if (bw == RTW_CHANNEL_WIDTH_80) {
		if (txsc20 == RTW_SC_20_UPPER || txsc20 == RTW_SC_20_UPMOST)
			txsc40 = RTW_SC_40_UPPER;
		else
			txsc40 = RTW_SC_40_LOWER;
	}
	rtw_write8(rtwdev, REG_DATA_SC,
		   BIT_TXSC_20M(txsc20) | BIT_TXSC_40M(txsc40));

	value32 = rtw_read32(rtwdev, REG_WMAC_TRXPTCL_CTL);
	value32 &= ~BIT_RFMOD;
	switch (bw) {
	case RTW_CHANNEL_WIDTH_80:
		value32 |= BIT_RFMOD_80M;
		break;
	case RTW_CHANNEL_WIDTH_40:
		value32 |= BIT_RFMOD_40M;
		break;
	case RTW_CHANNEL_WIDTH_20:
	default:
		break;
	}
	rtw_write32(rtwdev, REG_WMAC_TRXPTCL_CTL, value32);

	if (rtw_chip_wcpu_8051(rtwdev))
		return;

	value32 = rtw_read32(rtwdev, REG_AFE_CTRL1) & ~(BIT_MAC_CLK_SEL);
	value32 |= (MAC_CLK_HW_DEF_80M << BIT_SHIFT_MAC_CLK_SEL);
	rtw_write32(rtwdev, REG_AFE_CTRL1, value32);

	rtw_write8(rtwdev, REG_USTIME_TSF, MAC_CLK_SPEED);
	rtw_write8(rtwdev, REG_USTIME_EDCA, MAC_CLK_SPEED);

	value8 = rtw_read8(rtwdev, REG_CCK_CHECK);
	value8 = value8 & ~BIT_CHECK_CCK_EN;
	if (IS_CH_5G_BAND(channel))
		value8 |= BIT_CHECK_CCK_EN;
	rtw_write8(rtwdev, REG_CCK_CHECK, value8);
}

static void
rtw_xref_5g_txpwr_lmt(struct rtw_dev *rtwdev, u8 regd,
		      u8 bw, u8 ch_idx, u8 rs_ht, u8 rs_vht)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 max_power_index = rtwdev->chip->max_power_index;
	s8 lmt_ht = hal->tx_pwr_limit_5g[regd][bw][rs_ht][ch_idx];
	s8 lmt_vht = hal->tx_pwr_limit_5g[regd][bw][rs_vht][ch_idx];

	if (lmt_ht == lmt_vht)
		return;

	if (lmt_ht == max_power_index)
		hal->tx_pwr_limit_5g[regd][bw][rs_ht][ch_idx] = lmt_vht;

	else if (lmt_vht == max_power_index)
		hal->tx_pwr_limit_5g[regd][bw][rs_vht][ch_idx] = lmt_ht;
}

static void
rtw_xref_5g_txpwr_lmt_by_ch(struct rtw_dev *rtwdev, u8 regd, u8 bw)
{
	u8 ch_idx;

	for (ch_idx = 0; ch_idx < RTW_MAX_CHANNEL_NUM_5G; ch_idx++)
		rtw_xref_txpwr_lmt_by_rs(rtwdev, regd, bw, ch_idx);
}

static void rtw_xref_txpwr_lmt(struct rtw_dev *rtwdev)
{
	u8 regd;

	for (regd = 0; regd < RTW_REGD_MAX; regd++)
		rtw_xref_txpwr_lmt_by_bw(rtwdev, regd);
}

static void
rtw_xref_txpwr_lmt_by_bw(struct rtw_dev *rtwdev, u8 regd)
{
	u8 bw;

	for (bw = RTW_CHANNEL_WIDTH_20; bw <= RTW_CHANNEL_WIDTH_40; bw++)
		rtw_xref_5g_txpwr_lmt_by_ch(rtwdev, regd, bw);
}

static void
rtw_xref_txpwr_lmt_by_rs(struct rtw_dev *rtwdev, u8 regd, u8 bw, u8 ch_idx)
{
	static const u8 rs_cmp[4][2] = {
		{RTW_RATE_SECTION_HT_1S, RTW_RATE_SECTION_VHT_1S},
		{RTW_RATE_SECTION_HT_2S, RTW_RATE_SECTION_VHT_2S},
		{RTW_RATE_SECTION_HT_3S, RTW_RATE_SECTION_VHT_3S},
		{RTW_RATE_SECTION_HT_4S, RTW_RATE_SECTION_VHT_4S}
	};
	u8 rs_idx, rs_ht, rs_vht;

	for (rs_idx = 0; rs_idx < 4; rs_idx++) {
		rs_ht = rs_cmp[rs_idx][0];
		rs_vht = rs_cmp[rs_idx][1];

		rtw_xref_5g_txpwr_lmt(rtwdev, regd, bw, ch_idx, rs_ht, rs_vht);
	}
}

static u8 tbl_to_dec_pwr_by_rate(struct rtw_dev *rtwdev, u32 hex, u8 i)
{
	if (rtwdev->chip->is_pwr_by_rate_dec)
		return bcd_to_dec_pwr_by_rate(hex, i);

	return (hex >> (i * 8)) & 0xFF;
}
