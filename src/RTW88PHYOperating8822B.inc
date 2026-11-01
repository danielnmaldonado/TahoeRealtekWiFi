/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

/* Copyright(c)2018-2019 Realtek Corporation. Pinned function bodies; checked adapter in RTW88PHYOperating.c. */

#define RCR_VHT_ACK		BIT(26)
#define GET_PHY_STAT_P0_PWDB(phy_stat)                                          	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#define GET_PHY_STAT_P1_PWDB_A(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#define GET_PHY_STAT_P1_PWDB_B(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(23, 16))
#define GET_PHY_STAT_P1_RF_MODE(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), GENMASK(29, 28))
#define GET_PHY_STAT_P1_L_RXSC(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(11, 8))
#define GET_PHY_STAT_P1_HT_RXSC(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(15, 12))
#define GET_PHY_STAT_P1_RXEVM_A(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(7, 0))
#define GET_PHY_STAT_P1_RXEVM_B(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(15, 8))
#define GET_PHY_STAT_P1_CFO_TAIL_A(phy_stat)                                  	le32_get_bits(*((__le32 *)(phy_stat) + 0x05), GENMASK(7, 0))
#define GET_PHY_STAT_P1_CFO_TAIL_B(phy_stat)                                  	le32_get_bits(*((__le32 *)(phy_stat) + 0x05), GENMASK(15, 8))
#define GET_PHY_STAT_P1_RXSNR_A(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x06), GENMASK(7, 0))
#define GET_PHY_STAT_P1_RXSNR_B(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x06), GENMASK(15, 8))
#define RTW8822B_EDCCA_MAX	0x7f
#define RTW8822B_EDCCA_SRC_DEF	1
#define REG_HTSTFWT	0x800
#define REG_RXCCAMSK	0x814
#define REG_L1WT	0x83c
#define REG_L1PKWT	0x840
#define REG_MRC		0x850
#define REG_EDCCA_POW_MA	0x8a0
#define BIT_MA_LEVEL	GENMASK(1, 0)
#define REG_ADC40	0x8c8
#define REG_EDCCA_DECISION	0x8dc
#define BIT_EDCCA_OPTION	BIT(5)
#define REG_CDDTXP	0x93c
#define REG_TXPSEL1	0x940
#define REG_EDCCA_SOURCE	0x944
#define BIT_SOURCE_OPTION	GENMASK(29, 28)
#define REG_ACBB0	0x948
#define REG_ACBBRXFIR	0x94c
#define REG_ACGG2TBL	0x958
#define REG_ADCINI	0xa04
#define REG_TXSF2	0xa24
#define REG_TXSF6	0xa28
#define REG_RXDESC	0xa2c
#define REG_ENTXCCK	0xa80
#define REG_AGCTR_A	0xc08
#define REG_TXDFIR	0xc20
#define REG_TRSW	0xca0
#define REG_RFESEL0	0xcb0
#define REG_RFESEL8	0xcb4
#define REG_RFECTL	0xcb8
#define REG_RFEINV	0xcbc
#define REG_AGCTR_B	0xe08
#define REG_ANTWT	0x1904
#define REG_IQKFAILMSK	0x1bf0

#define RTW_TXSCALE_SIZE 37
#define WLAN_SLOT_TIME		0x09
#define WLAN_PIFS_TIME		0x19
#define WLAN_SIFS_CCK_CONT_TX	0xA
#define WLAN_SIFS_OFDM_CONT_TX	0xE
#define WLAN_SIFS_CCK_TRX	0x10
#define WLAN_SIFS_OFDM_TRX	0x10
#define WLAN_VO_TXOP_LIMIT	0x186 /* unit : 32us */
#define WLAN_VI_TXOP_LIMIT	0x3BC /* unit : 32us */
#define WLAN_RDG_NAV		0x05
#define WLAN_TXOP_NAV		0x1B
#define WLAN_CCK_RX_TSF		0x30
#define WLAN_OFDM_RX_TSF	0x30
#define WLAN_TBTT_PROHIBIT	0x04 /* unit : 32us */
#define WLAN_TBTT_HOLD_TIME	0x064 /* unit : 32us */
#define WLAN_DRV_EARLY_INT	0x04
#define WLAN_BCN_DMA_TIME	0x02
#define WLAN_RX_FILTER0		0xFFFF
#define WLAN_RX_FILTER1		0x0FFF
#define WLAN_RX_FILTER2		0xFFFF
#define WLAN_RCR_CFG		0xE400220E
#define WLAN_RXPKT_MAX_SZ	12288
#define WLAN_RXPKT_MAX_SZ_512	(WLAN_RXPKT_MAX_SZ >> 9)
#define WLAN_AMPDU_MAX_TIME		0x70
#define WLAN_RTS_LEN_TH			0xFF
#define WLAN_RTS_TX_TIME_TH		0x08
#define WLAN_MAX_AGG_PKT_LIMIT		0x20
#define WLAN_RTS_MAX_AGG_PKT_LIMIT	0x20
#define FAST_EDCA_VO_TH		0x06
#define FAST_EDCA_VI_TH		0x06
#define FAST_EDCA_BE_TH		0x06
#define FAST_EDCA_BK_TH		0x06
#define WLAN_BAR_RETRY_LIMIT		0x01
#define WLAN_RA_TRY_RATE_AGG_LIMIT	0x08
#define WLAN_TX_FUNC_CFG1		0x30
#define WLAN_TX_FUNC_CFG2		0x30
#define WLAN_MAC_OPT_NORM_FUNC1		0x98
#define WLAN_MAC_OPT_LB_FUNC1		0x80
#define WLAN_MAC_OPT_FUNC2		0xb0810041
#define WLAN_SIFS_CFG	(WLAN_SIFS_CCK_CONT_TX |  			(WLAN_SIFS_OFDM_CONT_TX << BIT_SHIFT_SIFS_OFDM_CTX) |  			(WLAN_SIFS_CCK_TRX << BIT_SHIFT_SIFS_CCK_TRX) |  			(WLAN_SIFS_OFDM_TRX << BIT_SHIFT_SIFS_OFDM_TRX))
#define WLAN_TBTT_TIME	(WLAN_TBTT_PROHIBIT | 			(WLAN_TBTT_HOLD_TIME << BIT_SHIFT_TBTT_HOLD_TIME_AP))
#define WLAN_NAV_CFG		(WLAN_RDG_NAV | (WLAN_TXOP_NAV << 16))
#define WLAN_RX_TSF_CFG		(WLAN_CCK_RX_TSF | (WLAN_OFDM_RX_TSF) << 8)
#define I2GE5G_CCUT(set_ch) {						 	.cca_ccut_2g = &cca_ifem_ccut,					 	.cca_ccut_5g = &cca_efem_ccut,					 	.fem = RTW_RFE_IFEM2G_EFEM5G,					 	.ifem_ext = false,						 	.rtw_set_channel_rfe = &rtw8822b_set_channel_rfe_ ## set_ch,	 	}
#define IFEM_EXT_CCUT(set_ch) {						 	.cca_ccut_2g = &cca_ifem_ccut_ext,				 	.cca_ccut_5g = &cca_ifem_ccut_ext,				 	.fem = RTW_RFE_IFEM,						 	.ifem_ext = true,						 	.rtw_set_channel_rfe = &rtw8822b_set_channel_rfe_ ## set_ch,	 	}

enum {
	CCUT_IDX_1R_2G,
	CCUT_IDX_2R_2G,
	CCUT_IDX_1R_5G,
	CCUT_IDX_2R_5G,
	CCUT_IDX_NR,
};

struct cca_ccut {
	u32 reg82c[CCUT_IDX_NR];
	u32 reg830[CCUT_IDX_NR];
	u32 reg838[CCUT_IDX_NR];
};

static const struct cca_ccut cca_ifem_ccut = {
	{0x75C97010, 0x75C97010, 0x75C97010, 0x75C97010}, /*Reg82C*/
	{0x79a0eaaa, 0x79A0EAAC, 0x79a0eaaa, 0x79a0eaaa}, /*Reg830*/
	{0x87765541, 0x87746341, 0x87765541, 0x87746341}, /*Reg838*/
};

static const struct cca_ccut cca_efem_ccut = {
	{0x75B86010, 0x75B76010, 0x75B86010, 0x75B76010}, /*Reg82C*/
	{0x79A0EAA8, 0x79A0EAAC, 0x79A0EAA8, 0x79a0eaaa}, /*Reg830*/
	{0x87766451, 0x87766431, 0x87766451, 0x87766431}, /*Reg838*/
};

static const struct cca_ccut cca_ifem_ccut_ext = {
	{0x75da8010, 0x75da8010, 0x75da8010, 0x75da8010}, /*Reg82C*/
	{0x79a0eaaa, 0x97A0EAAC, 0x79a0eaaa, 0x79a0eaaa}, /*Reg830*/
	{0x87765541, 0x86666341, 0x87765561, 0x86666361}, /*Reg838*/
};

static void rtw8822b_get_cca_val(const struct cca_ccut *cca_ccut, u8 col,
				 u32 *reg82c, u32 *reg830, u32 *reg838)
{
	*reg82c = cca_ccut->reg82c[col];
	*reg830 = cca_ccut->reg830[col];
	*reg838 = cca_ccut->reg838[col];
}

struct rtw8822b_rfe_info {
	const struct cca_ccut *cca_ccut_2g;
	const struct cca_ccut *cca_ccut_5g;
	enum rtw_rfe_fem fem;
	bool ifem_ext;
	void (*rtw_set_channel_rfe)(struct rtw_dev *rtwdev, u8 channel);
};

#define I2GE5G_CCUT(set_ch) {						\
	.cca_ccut_2g = &cca_ifem_ccut,					\
	.cca_ccut_5g = &cca_efem_ccut,					\
	.fem = RTW_RFE_IFEM2G_EFEM5G,					\
	.ifem_ext = false,						\
	.rtw_set_channel_rfe = &rtw8822b_set_channel_rfe_ ## set_ch,	\
	}
#define IFEM_EXT_CCUT(set_ch) {						\
	.cca_ccut_2g = &cca_ifem_ccut_ext,				\
	.cca_ccut_5g = &cca_ifem_ccut_ext,				\
	.fem = RTW_RFE_IFEM,						\
	.ifem_ext = true,						\
	.rtw_set_channel_rfe = &rtw8822b_set_channel_rfe_ ## set_ch,	\
	}

static const struct rtw8822b_rfe_info rtw8822b_rfe_info[] = {
	[2] = I2GE5G_CCUT(efem),
	[3] = IFEM_EXT_CCUT(ifem),
	[5] = IFEM_EXT_CCUT(ifem),
};



static const u32 rtw8822b_txscale_tbl[RTW_TXSCALE_SIZE];

static const u8 low_band[15];

static const u8 middle_band[23];

static const u8 high_band[15];

static const u8
rtw8822b_pwrtrk_5gb_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8
rtw8822b_pwrtrk_5gb_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8
rtw8822b_pwrtrk_5ga_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8
rtw8822b_pwrtrk_5ga_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2gb_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2gb_p[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2ga_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2ga_p[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2g_cck_b_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2g_cck_b_p[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2g_cck_a_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822b_pwrtrk_2g_cck_a_p[RTW_PWR_TRK_TBL_SZ];

static const struct rtw_pwr_track_tbl rtw8822b_pwr_track_type0_tbl;

static const struct rtw_rfe_def rtw8822b_rfe_defs[];

static const u32 rtw8822b_txscale_tbl[RTW_TXSCALE_SIZE] = {
	0x081, 0x088, 0x090, 0x099, 0x0a2, 0x0ac, 0x0b6, 0x0c0, 0x0cc, 0x0d8,
	0x0e5, 0x0f2, 0x101, 0x110, 0x120, 0x131, 0x143, 0x156, 0x16a, 0x180,
	0x197, 0x1af, 0x1c8, 0x1e3, 0x200, 0x21e, 0x23e, 0x261, 0x285, 0x2ab,
	0x2d3, 0x2fe, 0x32b, 0x35c, 0x38e, 0x3c4, 0x3fe
};

static const u8 low_band[15] = {0x7, 0x6, 0x6, 0x5, 0x0, 0x0, 0x7, 0xff, 0x6,
				0x5, 0x0, 0x0, 0x7, 0x6, 0x6};

static const u8 middle_band[23] = {0x6, 0x5, 0x0, 0x0, 0x7, 0x6, 0x6, 0xff, 0x0,
				   0x0, 0x7, 0x6, 0x6, 0x5, 0x0, 0xff, 0x7, 0x6,
				   0x6, 0x5, 0x0, 0x0, 0x7};

static const u8 high_band[15] = {0x5, 0x5, 0x0, 0x7, 0x7, 0x6, 0x5, 0xff, 0x0,
				 0x7, 0x7, 0x6, 0x5, 0x5, 0x0};

static const u8
rtw8822b_pwrtrk_5gb_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  8,  9, 10, 11, 11, 12, 13, 14, 14,
	 15, 16, 17, 17, 18, 19, 20, 20, 21, 22 },
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  8,  9, 10, 11, 11, 12, 13, 14, 14,
	 15, 16, 17, 17, 18, 19, 20, 20, 21, 22 },
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  8,  9, 10, 11, 11, 12, 13, 14, 14,
	 15, 16, 17, 17, 18, 19, 20, 20, 21, 22 },
};

static const u8
rtw8822b_pwrtrk_5gb_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  9,  9, 10, 11, 12, 13, 14, 14, 15,
	 16, 17, 18, 19, 19, 20, 21, 22, 22, 23 },
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  9,  9, 10, 11, 12, 13, 14, 14, 15,
	 16, 17, 18, 19, 19, 20, 21, 22, 22, 23 },
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  9,  9, 10, 11, 12, 13, 14, 14, 15,
	 16, 17, 18, 19, 19, 20, 21, 22, 22, 23 },
};

static const u8
rtw8822b_pwrtrk_5ga_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  8,  9, 10, 11, 11, 12, 13, 14, 14,
	 15, 16, 17, 17, 18, 19, 20, 20, 21, 22 },
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  8,  9, 10, 11, 11, 12, 13, 14, 14,
	 15, 16, 17, 17, 18, 19, 20, 20, 21, 22 },
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  8,  9, 10, 11, 11, 12, 13, 14, 14,
	 15, 16, 17, 17, 18, 19, 20, 20, 21, 22 },
};

static const u8
rtw8822b_pwrtrk_5ga_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  9,  9, 10, 11, 12, 13, 14, 14, 15,
	 16, 17, 18, 19, 19, 20, 21, 22, 22, 23},
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  9,  9, 10, 11, 12, 13, 14, 14, 15,
	 16, 17, 18, 19, 19, 20, 21, 22, 22, 23},
	{ 0,  1,  2,  2,  3,  4,  5,  5,  6,  7,
	  8,  9,  9, 10, 11, 12, 13, 14, 14, 15,
	 16, 17, 18, 19, 19, 20, 21, 22, 22, 23},
};

static const u8 rtw8822b_pwrtrk_2gb_n[RTW_PWR_TRK_TBL_SZ] = {
	0,  1,  1,  1,  2,  2,  3,  3,  3,  4,
	4,  5,  5,  5,  6,  6,  7,  7,  7,  8,
	8,  9,  9,  9, 10, 10, 11, 11, 11, 12
};

static const u8 rtw8822b_pwrtrk_2gb_p[RTW_PWR_TRK_TBL_SZ] = {
	0,  0,  1,  1,  2,  2,  3,  3,  4,  4,
	5,  5,  6,  6,  6,  7,  7,  8,  8,  9,
	9, 10, 10, 11, 11, 12, 12, 12, 13, 13
};

static const u8 rtw8822b_pwrtrk_2ga_n[RTW_PWR_TRK_TBL_SZ] = {
	0,  1,  1,  1,  2,  2,  3,  3,  3,  4,
	4,  5,  5,  5,  6,  6,  7,  7,  7,  8,
	8,  9,  9,  9, 10, 10, 11, 11, 11, 12
};

static const u8 rtw8822b_pwrtrk_2ga_p[RTW_PWR_TRK_TBL_SZ] = {
	0,  1,  1,  2,  2,  3,  3,  4,  4,  5,
	5,  6,  6,  7,  7,  8,  8,  9,  9, 10,
	10, 11, 11, 12, 12, 13, 13, 14, 14, 15
};

static const u8 rtw8822b_pwrtrk_2g_cck_b_n[RTW_PWR_TRK_TBL_SZ] = {
	0,  1,  1,  1,  2,  2,  3,  3,  3,  4,
	4,  5,  5,  5,  6,  6,  7,  7,  7,  8,
	8,  9,  9,  9, 10, 10, 11, 11, 11, 12
};

static const u8 rtw8822b_pwrtrk_2g_cck_b_p[RTW_PWR_TRK_TBL_SZ] = {
	0,  0,  1,  1,  2,  2,  3,  3,  4,  4,
	5,  5,  6,  6,  6,  7,  7,  8,  8,  9,
	9, 10, 10, 11, 11, 12, 12, 12, 13, 13
};

static const u8 rtw8822b_pwrtrk_2g_cck_a_n[RTW_PWR_TRK_TBL_SZ] = {
	0,  1,  1,  1,  2,  2,  3,  3,  3,  4,
	4,  5,  5,  5,  6,  6,  7,  7,  7,  8,
	8,  9,  9,  9, 10, 10, 11, 11, 11, 12
};

static const u8 rtw8822b_pwrtrk_2g_cck_a_p[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  1,  2,  2,  3,  3,  4,  4,  5,
	 5,  6,  6,  7,  7,  8,  8,  9,  9, 10,
	10, 11, 11, 12, 12, 13, 13, 14, 14, 15
};

static const struct rtw_pwr_track_tbl rtw8822b_pwr_track_type0_tbl = {
	.pwrtrk_5gb_n[RTW_PWR_TRK_5G_1] = rtw8822b_pwrtrk_5gb_n[RTW_PWR_TRK_5G_1],
	.pwrtrk_5gb_n[RTW_PWR_TRK_5G_2] = rtw8822b_pwrtrk_5gb_n[RTW_PWR_TRK_5G_2],
	.pwrtrk_5gb_n[RTW_PWR_TRK_5G_3] = rtw8822b_pwrtrk_5gb_n[RTW_PWR_TRK_5G_3],
	.pwrtrk_5gb_p[RTW_PWR_TRK_5G_1] = rtw8822b_pwrtrk_5gb_p[RTW_PWR_TRK_5G_1],
	.pwrtrk_5gb_p[RTW_PWR_TRK_5G_2] = rtw8822b_pwrtrk_5gb_p[RTW_PWR_TRK_5G_2],
	.pwrtrk_5gb_p[RTW_PWR_TRK_5G_3] = rtw8822b_pwrtrk_5gb_p[RTW_PWR_TRK_5G_3],
	.pwrtrk_5ga_n[RTW_PWR_TRK_5G_1] = rtw8822b_pwrtrk_5ga_n[RTW_PWR_TRK_5G_1],
	.pwrtrk_5ga_n[RTW_PWR_TRK_5G_2] = rtw8822b_pwrtrk_5ga_n[RTW_PWR_TRK_5G_2],
	.pwrtrk_5ga_n[RTW_PWR_TRK_5G_3] = rtw8822b_pwrtrk_5ga_n[RTW_PWR_TRK_5G_3],
	.pwrtrk_5ga_p[RTW_PWR_TRK_5G_1] = rtw8822b_pwrtrk_5ga_p[RTW_PWR_TRK_5G_1],
	.pwrtrk_5ga_p[RTW_PWR_TRK_5G_2] = rtw8822b_pwrtrk_5ga_p[RTW_PWR_TRK_5G_2],
	.pwrtrk_5ga_p[RTW_PWR_TRK_5G_3] = rtw8822b_pwrtrk_5ga_p[RTW_PWR_TRK_5G_3],
	.pwrtrk_2gb_n = rtw8822b_pwrtrk_2gb_n,
	.pwrtrk_2gb_p = rtw8822b_pwrtrk_2gb_p,
	.pwrtrk_2ga_n = rtw8822b_pwrtrk_2ga_n,
	.pwrtrk_2ga_p = rtw8822b_pwrtrk_2ga_p,
	.pwrtrk_2g_cckb_n = rtw8822b_pwrtrk_2g_cck_b_n,
	.pwrtrk_2g_cckb_p = rtw8822b_pwrtrk_2g_cck_b_p,
	.pwrtrk_2g_ccka_n = rtw8822b_pwrtrk_2g_cck_a_n,
	.pwrtrk_2g_ccka_p = rtw8822b_pwrtrk_2g_cck_a_p,
};

static const struct rtw_rfe_def rtw8822b_rfe_defs[] = {
	[2] = RTW_DEF_RFE(8822b, 2, 2, 0),
	[3] = RTW_DEF_RFE(8822b, 3, 0, 0),
	[5] = RTW_DEF_RFE(8822b, 5, 5, 0),
};

static void rtw8822b_config_trx_mode(struct rtw_dev *rtwdev, u8 tx_path,
				     u8 rx_path, bool is_tx2_path)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;
	const struct rtw8822b_rfe_info *rfe_info;
	u8 ch = rtwdev->hal.current_channel;
	u8 tx_path_sel, rx_path_sel;
	int counter;

	if (WARN(efuse->rfe_option >= ARRAY_SIZE(rtw8822b_rfe_info),
		 "rfe_option %d is out of boundary\n", efuse->rfe_option))
		return;

	rfe_info = &rtw8822b_rfe_info[efuse->rfe_option];

	if ((tx_path | rx_path) & BB_PATH_A)
		rtw_write32_mask(rtwdev, REG_AGCTR_A, MASKLWORD, 0x3231);
	else
		rtw_write32_mask(rtwdev, REG_AGCTR_A, MASKLWORD, 0x1111);

	if ((tx_path | rx_path) & BB_PATH_B)
		rtw_write32_mask(rtwdev, REG_AGCTR_B, MASKLWORD, 0x3231);
	else
		rtw_write32_mask(rtwdev, REG_AGCTR_B, MASKLWORD, 0x1111);

	rtw_write32_mask(rtwdev, REG_CDDTXP, (BIT(19) | BIT(18)), 0x3);
	rtw_write32_mask(rtwdev, REG_TXPSEL, (BIT(29) | BIT(28)), 0x1);
	rtw_write32_mask(rtwdev, REG_TXPSEL, BIT(30), 0x1);

	if (tx_path & BB_PATH_A) {
		rtw_write32_mask(rtwdev, REG_CDDTXP, 0xfff00000, 0x001);
		rtw_write32_mask(rtwdev, REG_ADCINI, 0xf0000000, 0x8);
	} else if (tx_path & BB_PATH_B) {
		rtw_write32_mask(rtwdev, REG_CDDTXP, 0xfff00000, 0x002);
		rtw_write32_mask(rtwdev, REG_ADCINI, 0xf0000000, 0x4);
	}

	if (tx_path == BB_PATH_A || tx_path == BB_PATH_B)
		rtw_write32_mask(rtwdev, REG_TXPSEL1, 0xfff0, 0x01);
	else
		rtw_write32_mask(rtwdev, REG_TXPSEL1, 0xfff0, 0x43);

	tx_path_sel = (tx_path << 4) | tx_path;
	rtw_write32_mask(rtwdev, REG_TXPSEL, MASKBYTE0, tx_path_sel);

	if (tx_path != BB_PATH_A && tx_path != BB_PATH_B) {
		if (is_tx2_path || rtwdev->mp_mode) {
			rtw_write32_mask(rtwdev, REG_CDDTXP, 0xfff00000, 0x043);
			rtw_write32_mask(rtwdev, REG_ADCINI, 0xf0000000, 0xc);
		}
	}

	rtw_write32_mask(rtwdev, REG_RXDESC, BIT(22), 0x0);
	rtw_write32_mask(rtwdev, REG_RXDESC, BIT(18), 0x0);

	if (rx_path & BB_PATH_A)
		rtw_write32_mask(rtwdev, REG_ADCINI, 0x0f000000, 0x0);
	else if (rx_path & BB_PATH_B)
		rtw_write32_mask(rtwdev, REG_ADCINI, 0x0f000000, 0x5);

	rx_path_sel = (rx_path << 4) | rx_path;
	rtw_write32_mask(rtwdev, REG_RXPSEL, MASKBYTE0, rx_path_sel);

	if (rx_path == BB_PATH_A || rx_path == BB_PATH_B) {
		rtw_write32_mask(rtwdev, REG_ANTWT, BIT(16), 0x0);
		rtw_write32_mask(rtwdev, REG_HTSTFWT, BIT(28), 0x0);
		rtw_write32_mask(rtwdev, REG_MRC, BIT(23), 0x0);
	} else {
		rtw_write32_mask(rtwdev, REG_ANTWT, BIT(16), 0x1);
		rtw_write32_mask(rtwdev, REG_HTSTFWT, BIT(28), 0x1);
		rtw_write32_mask(rtwdev, REG_MRC, BIT(23), 0x1);
	}

	for (counter = 100; counter > 0; counter--) {
		u32 rf_reg33;

		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x00001);

		udelay(2);
		rf_reg33 = rtw_read_rf(rtwdev, RF_PATH_A, 0x33, RFREG_MASK);

		if (rf_reg33 == 0x00001)
			break;
	}

	if (WARN(counter <= 0, "write RF mode table fail\n"))
		return;

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x80000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x00001);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD1, RFREG_MASK, 0x00034);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x4080c);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x00000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE, RFREG_MASK, 0x00000);

	rtw8822b_toggle_igi(rtwdev);
	rtw8822b_set_channel_cca(rtwdev, 1, RTW_CHANNEL_WIDTH_20, rfe_info);
	(*rfe_info->rtw_set_channel_rfe)(rtwdev, ch);
}

static void rtw8822b_do_iqk(struct rtw_dev *rtwdev)
{
	static int do_iqk_cnt;
	struct rtw_iqk_para para = {.clear = 0, .segment_iqk = 0};
	u32 rf_reg, iqk_fail_mask;
	int counter;
	bool reload;

	rtw_write_rf(rtwdev,RF_PATH_A,RF_DTXLOK,RFREG_MASK,0);
	rtw_fw_do_iqk(rtwdev, &para);

	for (counter = 0; counter < 300; counter++) {
		rf_reg = rtw_read_rf(rtwdev, RF_PATH_A, RF_DTXLOK, RFREG_MASK);
		if (rf_reg == 0xabcde)
			break;
		msleep(20);
	}
	if(counter>=300)native_fail(rtwdev,ETIMEDOUT);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_DTXLOK, RFREG_MASK, 0x0);

	reload = !!rtw_read32_mask(rtwdev, REG_IQKFAILMSK, BIT(16));
	iqk_fail_mask = rtw_read32_mask(rtwdev, REG_IQKFAILMSK, GENMASK(7, 0));
	if(iqk_fail_mask)native_fail(rtwdev,EIO);
	if(!native_error(rtwdev))rtwdev->iqk_complete=1;
	rtw_dbg(rtwdev, RTW_DBG_PHY,
		"iqk counter=%d reload=%d do_iqk_cnt=%d n_iqk_fail(mask)=0x%02x\n",
		counter, reload, ++do_iqk_cnt, iqk_fail_mask);
}

static u8 rtw8822b_get_swing_index(struct rtw_dev *rtwdev)
{
	u8 i = 0;
	u32 swing, table_value;

	swing = rtw_read32_mask(rtwdev, 0xc1c, 0xffe00000);
	for (i = 0; i < RTW_TXSCALE_SIZE; i++) {
		table_value = rtw8822b_txscale_tbl[i];
		if (swing == table_value)
			break;
	}

	return i;
}

static void rtw8822b_phy_calibration(struct rtw_dev *rtwdev)
{
	rtw8822b_do_iqk(rtwdev);
}

static void rtw8822b_phy_pwrtrack(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_swing_table swing_table;
	u8 thermal_value, path;

	rtw_phy_config_swing_table(rtwdev, &swing_table);

	if (rtwdev->efuse.thermal_meter[RF_PATH_A] == 0xff)
		return;

	thermal_value = rtw_read_rf(rtwdev, RF_PATH_A, RF_T_METER, 0xfc00);

	rtw_phy_pwrtrack_avg(rtwdev, thermal_value, RF_PATH_A);

	if (dm_info->pwr_trk_init_trigger)
		dm_info->pwr_trk_init_trigger = false;
	else if (!rtw_phy_pwrtrack_thermal_changed(rtwdev, thermal_value,
						   RF_PATH_A))
		goto iqk;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++)
		rtw8822b_phy_pwrtrack_path(rtwdev, &swing_table, path);

iqk:
	if (rtw_phy_pwrtrack_need_iqk(rtwdev))
		rtw8822b_do_iqk(rtwdev);
}

static void rtw8822b_phy_pwrtrack_path(struct rtw_dev *rtwdev,
				       struct rtw_swing_table *swing_table,
				       u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 power_idx_cur, power_idx_last;
	u8 delta;

	/* 8822B only has one thermal meter at PATH A */
	delta = rtw_phy_pwrtrack_get_delta(rtwdev, RF_PATH_A);

	power_idx_last = dm_info->delta_power_index[path];
	power_idx_cur = rtw_phy_pwrtrack_get_pwridx(rtwdev, swing_table,
						    path, RF_PATH_A, delta);

	/* if delta of power indexes are the same, just skip */
	if (power_idx_cur == power_idx_last)
		return;

	dm_info->delta_power_index[path] = power_idx_cur;
	rtw8822b_pwrtrack_set(rtwdev, path);
}

static void rtw8822b_phy_rfe_init(struct rtw_dev *rtwdev)
{
	/* chip top mux */
	rtw_write32_mask(rtwdev, 0x64, BIT(29) | BIT(28), 0x3);
	rtw_write32_mask(rtwdev, 0x4c, BIT(26) | BIT(25), 0x0);
	rtw_write32_mask(rtwdev, 0x40, BIT(2), 0x1);

	/* from s0 or s1 */
	rtw_write32_mask(rtwdev, 0x1990, 0x3f, 0x30);
	rtw_write32_mask(rtwdev, 0x1990, (BIT(11) | BIT(10)), 0x3);

	/* input or output */
	rtw_write32_mask(rtwdev, 0x974, 0x3f, 0x3f);
	rtw_write32_mask(rtwdev, 0x974, (BIT(11) | BIT(10)), 0x3);
}

static void rtw8822b_pwr_track(struct rtw_dev *rtwdev)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	if (efuse->power_track_type != 0)
		return;

	if (!dm_info->pwr_trk_triggered) {
		rtw_write_rf(rtwdev, RF_PATH_A, RF_T_METER,
			     GENMASK(17, 16), 0x03);
		dm_info->pwr_trk_triggered = true;
		return;
	}

	rtw8822b_phy_pwrtrack(rtwdev);
	dm_info->pwr_trk_triggered = false;
}

static void rtw8822b_pwrtrack_init(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 swing_idx = rtw8822b_get_swing_index(rtwdev);
	u8 path;

	if (swing_idx >= RTW_TXSCALE_SIZE)
		dm_info->default_ofdm_index = 24;
	else
		dm_info->default_ofdm_index = swing_idx;

	for (path = RF_PATH_A; path < rtwdev->hal.rf_path_num; path++) {
		ewma_thermal_init(&dm_info->avg_thermal[path]);
		dm_info->delta_power_index[path] = 0;
	}
	dm_info->pwr_trk_triggered = false;
	dm_info->pwr_trk_init_trigger = true;
	dm_info->thermal_meter_k = rtwdev->efuse.thermal_meter_k;
}

static void rtw8822b_pwrtrack_set(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 pwr_idx_offset, tx_pwr_idx;
	u8 channel = rtwdev->hal.current_channel;
	u8 band_width = rtwdev->hal.current_band_width;
	u8 regd = rtw_regd_get(rtwdev);
	u8 tx_rate = dm_info->tx_rate;
	u8 max_pwr_idx = rtwdev->chip->max_power_index;

	tx_pwr_idx = native_thermal_power_index(rtwdev, path, tx_rate,
						band_width, channel, regd);

	tx_pwr_idx = min_t(u8, tx_pwr_idx, max_pwr_idx);

	pwr_idx_offset = max_pwr_idx - tx_pwr_idx;

	rtw8822b_pwrtrack_set_pwr(rtwdev, path, pwr_idx_offset);
}

static void rtw8822b_pwrtrack_set_pwr(struct rtw_dev *rtwdev, u8 path,
				      u8 pwr_idx_offset)
{
	s8 txagc_idx;
	u8 swing_idx;
	u32 reg1, reg2;

	if (path == RF_PATH_A) {
		reg1 = 0xc94;
		reg2 = 0xc1c;
	} else if (path == RF_PATH_B) {
		reg1 = 0xe94;
		reg2 = 0xe1c;
	} else {
		return;
	}

	rtw8822b_txagc_swing_offset(rtwdev, path, pwr_idx_offset,
				    &txagc_idx, &swing_idx);
	rtw_write32_mask(rtwdev, reg1, GENMASK(29, 25), txagc_idx);
	rtw_write32_mask(rtwdev, reg2, GENMASK(31, 21),
			 rtw8822b_txscale_tbl[swing_idx]);
}

static void rtw8822b_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;
	const struct rtw8822b_rfe_info *rfe_info;

	if (WARN(efuse->rfe_option >= ARRAY_SIZE(rtw8822b_rfe_info),
		 "rfe_option %d is out of boundary\n", efuse->rfe_option))
		return;

	rfe_info = &rtw8822b_rfe_info[efuse->rfe_option];

	rtw8822b_set_channel_bb(rtwdev, channel, bw, primary_chan_idx);
	rtw_set_channel_mac(rtwdev, channel, bw, primary_chan_idx);
	rtw8822b_set_channel_rf(rtwdev, channel, bw);
	rtw8822b_set_channel_rxdfir(rtwdev, bw);
	rtw8822b_toggle_igi(rtwdev);
	rtw8822b_set_channel_cca(rtwdev, channel, bw, rfe_info);
	(*rfe_info->rtw_set_channel_rfe)(rtwdev, channel);
}

static void rtw8822b_set_channel_bb(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				    u8 primary_ch_idx)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 rfe_option = efuse->rfe_option;
	u32 val32;

	if (IS_CH_2G_BAND(channel)) {
		rtw_write32_mask(rtwdev, REG_RXPSEL, BIT(28), 0x1);
		rtw_write32_mask(rtwdev, REG_CCK_CHECK, BIT(7), 0x0);
		rtw_write32_mask(rtwdev, REG_ENTXCCK, BIT(18), 0x0);
		rtw_write32_mask(rtwdev, REG_RXCCAMSK, 0x0000FC00, 15);

		rtw_write32_mask(rtwdev, REG_ACGG2TBL, 0x1f, 0x0);
		rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x96a);
		if (channel == 14) {
			rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD, 0x00006577);
			rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD, 0x0000);
		} else {
			rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD, 0x384f6577);
			rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD, 0x1525);
		}

		rtw_write32_mask(rtwdev, REG_RFEINV, 0x300, 0x2);
	} else if (IS_CH_5G_BAND(channel)) {
		rtw_write32_mask(rtwdev, REG_ENTXCCK, BIT(18), 0x1);
		rtw_write32_mask(rtwdev, REG_CCK_CHECK, BIT(7), 0x1);
		rtw_write32_mask(rtwdev, REG_RXPSEL, BIT(28), 0x0);
		rtw_write32_mask(rtwdev, REG_RXCCAMSK, 0x0000FC00, 34);

		if (IS_CH_5G_BAND_1(channel) || IS_CH_5G_BAND_2(channel))
			rtw_write32_mask(rtwdev, REG_ACGG2TBL, 0x1f, 0x1);
		else if (IS_CH_5G_BAND_3(channel))
			rtw_write32_mask(rtwdev, REG_ACGG2TBL, 0x1f, 0x2);
		else if (IS_CH_5G_BAND_4(channel))
			rtw_write32_mask(rtwdev, REG_ACGG2TBL, 0x1f, 0x3);

		if (IS_CH_5G_BAND_1(channel))
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x494);
		else if (IS_CH_5G_BAND_2(channel))
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x453);
		else if (channel >= 100 && channel <= 116)
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x452);
		else if (channel >= 118 && channel <= 177)
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x412);

		rtw_write32_mask(rtwdev, 0xcbc, 0x300, 0x1);
	}

	switch (bw) {
	case RTW_CHANNEL_WIDTH_20:
	default:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xFFCFFC00;
		val32 |= (RTW_CHANNEL_WIDTH_20);
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_40:
		if (primary_ch_idx == RTW_SC_20_UPPER)
			rtw_write32_set(rtwdev, REG_RXSB, BIT(4));
		else
			rtw_write32_clr(rtwdev, REG_RXSB, BIT(4));

		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xFF3FF300;
		val32 |= (((primary_ch_idx & 0xf) << 2) | RTW_CHANNEL_WIDTH_40);
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_80:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xFCEFCF00;
		val32 |= (((primary_ch_idx & 0xf) << 2) | RTW_CHANNEL_WIDTH_80);
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x1);

		if (rfe_option == 2 || rfe_option == 3) {
			rtw_write32_mask(rtwdev, REG_L1PKWT, 0x0000f000, 0x6);
			rtw_write32_mask(rtwdev, REG_ADC40, BIT(10), 0x1);
		}
		break;
	case RTW_CHANNEL_WIDTH_5:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xEFEEFE00;
		val32 |= ((BIT(6) | RTW_CHANNEL_WIDTH_20));
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x0);
		rtw_write32_mask(rtwdev, REG_ADC40, BIT(31), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_10:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xEFFEFF00;
		val32 |= ((BIT(7) | RTW_CHANNEL_WIDTH_20));
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x0);
		rtw_write32_mask(rtwdev, REG_ADC40, BIT(31), 0x1);
		break;
	}
}

static void rtw8822b_set_channel_cca(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				     const struct rtw8822b_rfe_info *rfe_info)
{
	struct rtw_hal *hal = &rtwdev->hal;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	const struct cca_ccut *cca_ccut;
	u8 col;
	u32 reg82c, reg830, reg838;
	bool is_efem_cca = false, is_ifem_cca = false, is_rfe_type = false;

	if (IS_CH_2G_BAND(channel)) {
		cca_ccut = rfe_info->cca_ccut_2g;

		if (hal->antenna_rx == BB_PATH_A ||
		    hal->antenna_rx == BB_PATH_B)
			col = CCUT_IDX_1R_2G;
		else
			col = CCUT_IDX_2R_2G;
	} else {
		cca_ccut = rfe_info->cca_ccut_5g;

		if (hal->antenna_rx == BB_PATH_A ||
		    hal->antenna_rx == BB_PATH_B)
			col = CCUT_IDX_1R_5G;
		else
			col = CCUT_IDX_2R_5G;
	}

	rtw8822b_get_cca_val(cca_ccut, col, &reg82c, &reg830, &reg838);

	switch (rfe_info->fem) {
	case RTW_RFE_IFEM:
	default:
		is_ifem_cca = true;
		if (rfe_info->ifem_ext)
			is_rfe_type = true;
		break;
	case RTW_RFE_EFEM:
		is_efem_cca = true;
		break;
	case RTW_RFE_IFEM2G_EFEM5G:
		if (IS_CH_2G_BAND(channel))
			is_ifem_cca = true;
		else
			is_efem_cca = true;
		break;
	}

	if (is_ifem_cca) {
		if ((hal->cut_version == RTW_CHIP_VER_CUT_B &&
		     (col == CCUT_IDX_2R_2G || col == CCUT_IDX_2R_5G) &&
		     bw == RTW_CHANNEL_WIDTH_40) ||
		    (!is_rfe_type && col == CCUT_IDX_2R_5G &&
		     bw == RTW_CHANNEL_WIDTH_40) ||
		    (efuse->rfe_option == 5 && col == CCUT_IDX_2R_5G))
			reg830 = 0x79a0ea28;
	}

	rtw_write32_mask(rtwdev, REG_CCASEL, MASKDWORD, reg82c);
	rtw_write32_mask(rtwdev, REG_PDMFTH, MASKDWORD, reg830);
	rtw_write32_mask(rtwdev, REG_CCA2ND, MASKDWORD, reg838);

	if (is_efem_cca && !(hal->cut_version == RTW_CHIP_VER_CUT_B))
		rtw_write32_mask(rtwdev, REG_L1WT, MASKDWORD, 0x9194b2b9);

	if (bw == RTW_CHANNEL_WIDTH_20 && IS_CH_5G_BAND_MID(channel))
		rtw_write32_mask(rtwdev, REG_CCA2ND, 0xf0, 0x4);
}

static void rtw8822b_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
#define RF18_BAND_MASK		(BIT(16) | BIT(9) | BIT(8))
#define RF18_BAND_2G		(0)
#define RF18_BAND_5G		(BIT(16) | BIT(8))
#define RF18_CHANNEL_MASK	(MASKBYTE0)
#define RF18_RFSI_MASK		(BIT(18) | BIT(17))
#define RF18_RFSI_GE_CH80	(BIT(17))
#define RF18_RFSI_GT_CH144	(BIT(18))
#define RF18_BW_MASK		(BIT(11) | BIT(10))
#define RF18_BW_20M		(BIT(11) | BIT(10))
#define RF18_BW_40M		(BIT(11))
#define RF18_BW_80M		(BIT(10))
#define RFBE_MASK		(BIT(17) | BIT(16) | BIT(15))

	struct rtw_hal *hal = &rtwdev->hal;
	u32 rf_reg18, rf_reg_be;

	rf_reg18 = rtw_read_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK);

	rf_reg18 &= ~(RF18_BAND_MASK | RF18_CHANNEL_MASK | RF18_RFSI_MASK |
		      RF18_BW_MASK);

	rf_reg18 |= (IS_CH_2G_BAND(channel) ? RF18_BAND_2G : RF18_BAND_5G);
	rf_reg18 |= (channel & RF18_CHANNEL_MASK);
	if (channel > 144)
		rf_reg18 |= RF18_RFSI_GT_CH144;
	else if (channel >= 80)
		rf_reg18 |= RF18_RFSI_GE_CH80;

	switch (bw) {
	case RTW_CHANNEL_WIDTH_5:
	case RTW_CHANNEL_WIDTH_10:
	case RTW_CHANNEL_WIDTH_20:
	default:
		rf_reg18 |= RF18_BW_20M;
		break;
	case RTW_CHANNEL_WIDTH_40:
		rf_reg18 |= RF18_BW_40M;
		break;
	case RTW_CHANNEL_WIDTH_80:
		rf_reg18 |= RF18_BW_80M;
		break;
	}

	if (IS_CH_2G_BAND(channel))
		rf_reg_be = 0x0;
	else if (IS_CH_5G_BAND_1(channel) || IS_CH_5G_BAND_2(channel))
		rf_reg_be = low_band[(channel - 36) >> 1];
	else if (IS_CH_5G_BAND_3(channel))
		rf_reg_be = middle_band[(channel - 100) >> 1];
	else if (IS_CH_5G_BAND_4(channel))
		rf_reg_be = high_band[(channel - 149) >> 1];
	else
		goto err;

	rtw_write_rf(rtwdev, RF_PATH_A, RF_MALSEL, RFBE_MASK, rf_reg_be);

	/* need to set 0xdf[18]=1 before writing RF18 when channel 144 */
	if (channel == 144)
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTDBG, BIT(18), 0x1);
	else
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTDBG, BIT(18), 0x0);

	rtw_write_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK, rf_reg18);
	if (hal->rf_type > RF_1T1R)
		rtw_write_rf(rtwdev, RF_PATH_B, 0x18, RFREG_MASK, rf_reg18);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_XTALX2, BIT(19), 0);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_XTALX2, BIT(19), 1);

	return;

err:
	WARN_ON(1);
}

static void rtw8822b_set_channel_rfe_efem(struct rtw_dev *rtwdev, u8 channel)
{
	struct rtw_hal *hal = &rtwdev->hal;

	if (IS_CH_2G_BAND(channel)) {
		rtw_write32s_mask(rtwdev, REG_RFESEL0, 0xffffff, 0x705770);
		rtw_write32s_mask(rtwdev, REG_RFESEL8, MASKBYTE1, 0x57);
		rtw_write32s_mask(rtwdev, REG_RFECTL, BIT(4), 0);
	} else {
		rtw_write32s_mask(rtwdev, REG_RFESEL0, 0xffffff, 0x177517);
		rtw_write32s_mask(rtwdev, REG_RFESEL8, MASKBYTE1, 0x75);
		rtw_write32s_mask(rtwdev, REG_RFECTL, BIT(5), 0);
	}

	rtw_write32s_mask(rtwdev, REG_RFEINV, BIT(11) | BIT(10) | 0x3f, 0x0);

	if (hal->antenna_rx == BB_PATH_AB ||
	    hal->antenna_tx == BB_PATH_AB) {
		/* 2TX or 2RX */
		rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa501);
	} else if (hal->antenna_rx == hal->antenna_tx) {
		/* TXA+RXA or TXB+RXB */
		rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa500);
	} else {
		/* TXB+RXA or TXA+RXB */
		rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa005);
	}
}

static void rtw8822b_set_channel_rfe_ifem(struct rtw_dev *rtwdev, u8 channel)
{
	struct rtw_hal *hal = &rtwdev->hal;

	if (IS_CH_2G_BAND(channel)) {
		/* signal source */
		rtw_write32s_mask(rtwdev, REG_RFESEL0, 0xffffff, 0x745774);
		rtw_write32s_mask(rtwdev, REG_RFESEL8, MASKBYTE1, 0x57);
	} else {
		/* signal source */
		rtw_write32s_mask(rtwdev, REG_RFESEL0, 0xffffff, 0x477547);
		rtw_write32s_mask(rtwdev, REG_RFESEL8, MASKBYTE1, 0x75);
	}

	rtw_write32s_mask(rtwdev, REG_RFEINV, BIT(11) | BIT(10) | 0x3f, 0x0);

	if (IS_CH_2G_BAND(channel)) {
		if (hal->antenna_rx == BB_PATH_AB ||
		    hal->antenna_tx == BB_PATH_AB) {
			/* 2TX or 2RX */
			rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa501);
		} else if (hal->antenna_rx == hal->antenna_tx) {
			/* TXA+RXA or TXB+RXB */
			rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa500);
		} else {
			/* TXB+RXA or TXA+RXB */
			rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa005);
		}
	} else {
		rtw_write32s_mask(rtwdev, REG_TRSW, MASKLWORD, 0xa5a5);
	}
}

static void rtw8822b_set_channel_rxdfir(struct rtw_dev *rtwdev, u8 bw)
{
	if (bw == RTW_CHANNEL_WIDTH_40) {
		/* RX DFIR for BW40 */
		rtw_write32_mask(rtwdev, REG_ACBB0, BIT(29) | BIT(28), 0x1);
		rtw_write32_mask(rtwdev, REG_ACBBRXFIR, BIT(29) | BIT(28), 0x0);
		rtw_write32s_mask(rtwdev, REG_TXDFIR, BIT(31), 0x0);
	} else if (bw == RTW_CHANNEL_WIDTH_80) {
		/* RX DFIR for BW80 */
		rtw_write32_mask(rtwdev, REG_ACBB0, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_ACBBRXFIR, BIT(29) | BIT(28), 0x1);
		rtw_write32s_mask(rtwdev, REG_TXDFIR, BIT(31), 0x0);
	} else {
		/* RX DFIR for BW20, BW10 and BW5*/
		rtw_write32_mask(rtwdev, REG_ACBB0, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_ACBBRXFIR, BIT(29) | BIT(28), 0x2);
		rtw_write32s_mask(rtwdev, REG_TXDFIR, BIT(31), 0x1);
	}
}

static void rtw8822b_set_tx_power_index(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 phy_pwr_idx = 0;
	int rs, path;

	for (path = 0; path < hal->rf_path_num; path++) {
		for (rs = 0; rs <= __RTW_RATE_SECTION_2SS_MAX; rs++)
			rtw8822b_set_tx_power_index_by_rate(rtwdev, path, rs,
							    &phy_pwr_idx);
	}
}

static void
rtw8822b_set_tx_power_index_by_rate(struct rtw_dev *rtwdev, u8 path,
				    u8 rs, u32 *phy_pwr_idx)
{
	struct rtw_hal *hal = &rtwdev->hal;
	static const u32 offset_txagc[2] = {0x1d00, 0x1d80};
	u8 rate, rate_idx, pwr_index, shift;
	int j;

	for (j = 0; j < rtw_rate_size[rs]; j++) {
		rate = rtw_rate_section[rs][j];
		pwr_index = hal->tx_pwr_tbl[path][rate];
		shift = rate & 0x3;
		*phy_pwr_idx |= ((u32)pwr_index << (shift * 8));
		if (shift == 0x3) {
			rate_idx = rate & 0xfc;
			rtw_write32(rtwdev, offset_txagc[path] + rate_idx,
				    *phy_pwr_idx);
			*phy_pwr_idx = 0;
		}
	}
}

static void rtw8822b_toggle_igi(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 igi;

	igi = rtw_read32_mask(rtwdev, REG_RXIGI_A, 0x7f);
	rtw_write32_mask(rtwdev, REG_RXIGI_A, 0x7f, igi - 2);
	rtw_write32_mask(rtwdev, REG_RXIGI_A, 0x7f, igi);
	rtw_write32_mask(rtwdev, REG_RXIGI_B, 0x7f, igi - 2);
	rtw_write32_mask(rtwdev, REG_RXIGI_B, 0x7f, igi);

	rtw_write32_mask(rtwdev, REG_RXPSEL, MASKBYTE0, 0x0);
	rtw_write32_mask(rtwdev, REG_RXPSEL, MASKBYTE0,
			 hal->antenna_rx | (hal->antenna_rx << 4));
}

static void rtw8822b_txagc_swing_offset(struct rtw_dev *rtwdev, u8 path,
					u8 tx_pwr_idx_offset,
					s8 *txagc_idx, u8 *swing_idx)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	s8 delta_pwr_idx = dm_info->delta_power_index[path];
	u8 swing_upper_bound = dm_info->default_ofdm_index + 10;
	u8 swing_lower_bound = 0;
	u8 max_tx_pwr_idx_offset = 0xf;
	s8 agc_index = 0;
	u8 swing_index = dm_info->default_ofdm_index;

	tx_pwr_idx_offset = min_t(u8, tx_pwr_idx_offset, max_tx_pwr_idx_offset);

	if (delta_pwr_idx >= 0) {
		if (delta_pwr_idx <= tx_pwr_idx_offset) {
			agc_index = delta_pwr_idx;
			swing_index = dm_info->default_ofdm_index;
		} else if (delta_pwr_idx > tx_pwr_idx_offset) {
			agc_index = tx_pwr_idx_offset;
			swing_index = dm_info->default_ofdm_index +
					delta_pwr_idx - tx_pwr_idx_offset;
			swing_index = min_t(u8, swing_index, swing_upper_bound);
		}
	} else {
		if (dm_info->default_ofdm_index > abs(delta_pwr_idx))
			swing_index =
				dm_info->default_ofdm_index + delta_pwr_idx;
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
