/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

/* Copyright(c)2018-2019 Realtek Corporation. Pinned function bodies; checked adapter in RTW88PHYOperating.c. */

#define RCR_VHT_ACK		BIT(26)
#define BIT_FEN_PCIEA BIT(6)
#define WLAN_SLOT_TIME		0x09
#define WLAN_PIFS_TIME		0x19
#define WLAN_SIFS_CCK_CONT_TX	0xA
#define WLAN_SIFS_OFDM_CONT_TX	0xE
#define WLAN_SIFS_CCK_TRX	0x10
#define WLAN_SIFS_OFDM_TRX	0x10
#define WLAN_VO_TXOP_LIMIT	0x186
#define WLAN_VI_TXOP_LIMIT	0x3BC
#define WLAN_RDG_NAV		0x05
#define WLAN_TXOP_NAV		0x1B
#define WLAN_CCK_RX_TSF		0x30
#define WLAN_OFDM_RX_TSF	0x30
#define WLAN_TBTT_PROHIBIT	0x04
#define WLAN_TBTT_HOLD_TIME	0x064
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
#define WLAN_PRE_TXCNT_TIME_TH		0x1E4
#define GET_PHY_STAT_P0_PWDB(phy_stat)                                          	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#define GET_PHY_STAT_P0_VGA(phy_stat)                                           	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), GENMASK(12, 8))
#define GET_PHY_STAT_P0_LNA_L(phy_stat)                                         	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), GENMASK(15, 13))
#define GET_PHY_STAT_P0_LNA_H(phy_stat)                                         	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), BIT(23))
#define BIT_LNA_H_MASK BIT(3)
#define BIT_LNA_L_MASK GENMASK(2, 0)
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
#define REG_SYS_CTRL	0x000
#define BIT_FEN_EN	BIT(26)
#define REG_INIRTS_RATE_SEL 0x0480
#define REG_HTSTFWT	0x800
#define REG_RXCCAMSK	0x814
#define REG_L1WT	0x83c
#define REG_L1PKWT	0x840
#define REG_MRC		0x850
#define REG_ADC40	0x8c8
#define REG_CHFIR	0x8f0
#define REG_CDDTXP	0x93c
#define REG_TXPSEL1	0x940
#define REG_ACBB0	0x948
#define REG_ACBBRXFIR	0x94c
#define REG_ACGG2TBL	0x958
#define REG_ADCINI	0xa04
#define REG_PWRTH	0xa08
#define REG_CCA_FLTR	0xa20
#define REG_TXSF2	0xa24
#define REG_TXSF6	0xa28
#define REG_RXDESC	0xa2c
#define REG_ENTXCCK	0xa80
#define BTG_LNA		0xfc84
#define WLG_LNA		0x7532
#define REG_ENRXCCA	0xa84
#define BTG_CCA		0x0e
#define WLG_CCA		0x12
#define REG_PWRTH2	0xaa8
#define REG_CSRATIO	0xaaa
#define REG_TXFILTER	0xaac
#define REG_AGCTR_A	0xc08
#define REG_TXDFIR	0xc20
#define REG_TRSW	0xca0
#define REG_RFESEL0	0xcb0
#define REG_RFESEL8	0xcb4
#define REG_RFECTL	0xcb8
#define B_BTG_SWITCH	BIT(16)
#define B_CTRL_SWITCH	BIT(18)
#define B_WL_SWITCH	(BIT(20) | BIT(22))
#define B_WLG_SWITCH	BIT(21)
#define B_WLA_SWITCH	BIT(23)
#define REG_RFEINV	0xcbc
#define REG_AGCTR_B	0xe08
#define REG_DMEM_CTRL	0x1080
#define BIT_WL_RST	BIT(16)
#define REG_ANTWT	0x1904
#define REG_IQKFAILMSK	0x1bf0
#define BIT_MASK_R_RFE_SEL_15	GENMASK(31, 28)
#define BIT_SDIO_INT BIT(18)
#define BT_CNT_ENABLE	0x1
#define BIT_BCN_QUEUE	BIT(3)
#define BCN_PRI_EN	0x1
#define PTA_CTRL_PIN	0x66
#define DPDT_CTRL_PIN	0x77
#define ANTDIC_CTRL_PIN	0x88
#define REG_CTRL_TYPE	0x67
#define BIT_CTRL_TYPE1	BIT(5)
#define BIT_CTRL_TYPE2	BIT(4)
#define CTRL_TYPE_MASK	GENMASK(15, 8)
#define RF18_BAND_MASK		(BIT(16) | BIT(9) | BIT(8))
#define RF18_BAND_2G		(0)
#define RF18_BAND_5G		(BIT(16) | BIT(8))
#define RF18_CHANNEL_MASK	(MASKBYTE0)
#define RF18_RFSI_MASK		(BIT(18) | BIT(17))
#define RF18_RFSI_GE		(BIT(17))
#define RF18_RFSI_GT		(BIT(18))
#define RF18_BW_MASK		(BIT(11) | BIT(10))
#define RF18_BW_20M		(BIT(11) | BIT(10))
#define RF18_BW_40M		(BIT(11))
#define RF18_BW_80M		(BIT(10))



enum rtw8821ce_rf_set {
	SWITCH_TO_BTG,
	SWITCH_TO_WLG,
	SWITCH_TO_WLA,
	SWITCH_TO_BT,
};

static const u32 rtw8821c_txscale_tbl[];

static const u8 rtw8821c_pwrtrk_5gb_n[][RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8821c_pwrtrk_5gb_p[][RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8821c_pwrtrk_5ga_n[][RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8821c_pwrtrk_5ga_p[][RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8821c_pwrtrk_2gb_n[];

static const u8 rtw8821c_pwrtrk_2gb_p[];

static const u8 rtw8821c_pwrtrk_2ga_n[];

static const u8 rtw8821c_pwrtrk_2ga_p[];

static const u8 rtw8821c_pwrtrk_2g_cck_b_n[];

static const u8 rtw8821c_pwrtrk_2g_cck_b_p[];

static const u8 rtw8821c_pwrtrk_2g_cck_a_n[];

static const u8 rtw8821c_pwrtrk_2g_cck_a_p[];

static const struct rtw_pwr_track_tbl rtw8821c_pwr_track_type0_tbl;

static const struct rtw_rfe_def rtw8821c_rfe_defs[];

static const u32 rtw8821c_txscale_tbl[] = {
	0x081, 0x088, 0x090, 0x099, 0x0a2, 0x0ac, 0x0b6, 0x0c0, 0x0cc, 0x0d8,
	0x0e5, 0x0f2, 0x101, 0x110, 0x120, 0x131, 0x143, 0x156, 0x16a, 0x180,
	0x197, 0x1af, 0x1c8, 0x1e3, 0x200, 0x21e, 0x23e, 0x261, 0x285, 0x2ab,
	0x2d3, 0x2fe, 0x32b, 0x35c, 0x38e, 0x3c4, 0x3fe
};

static const u8 rtw8821c_pwrtrk_5gb_n[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 8, 8, 8, 9, 9, 9, 10, 10,
	 11, 11, 12, 12, 12, 12, 12},
	{0, 1, 1, 1, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 10, 10, 11,
	 11, 12, 12, 12, 12, 12, 12, 12},
	{0, 1, 2, 2, 3, 4, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9, 10, 10, 11,
	 11, 12, 12, 12, 12, 12, 12},
};

static const u8 rtw8821c_pwrtrk_5gb_p[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 11,
	 12, 12, 12, 12, 12, 12, 12},
	{0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 7, 7, 8, 8, 9, 10, 10, 11, 11,
	 12, 12, 12, 12, 12, 12, 12, 12},
	{0, 1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 7, 8, 8, 9, 10, 10, 11,
	 11, 12, 12, 12, 12, 12, 12, 12},
};

static const u8 rtw8821c_pwrtrk_5ga_n[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 3, 3, 3, 4, 4, 5, 5, 6, 6, 6, 7, 8, 8, 8, 9, 9, 9, 10, 10,
	 11, 11, 12, 12, 12, 12, 12},
	{0, 1, 1, 1, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 8, 8, 9, 9, 10, 10, 11,
	 11, 12, 12, 12, 12, 12, 12, 12},
	{0, 1, 2, 2, 3, 4, 4, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9, 10, 10, 11,
	 11, 12, 12, 12, 12, 12, 12},
};

static const u8 rtw8821c_pwrtrk_5ga_p[][RTW_PWR_TRK_TBL_SZ] = {
	{0, 1, 1, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 11,
	 12, 12, 12, 12, 12, 12, 12},
	{0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 7, 7, 8, 8, 9, 10, 10, 11, 11,
	 12, 12, 12, 12, 12, 12, 12, 12},
	{0, 1, 1, 1, 2, 3, 3, 3, 4, 4, 4, 5, 6, 6, 7, 7, 8, 8, 9, 10, 10, 11,
	 11, 12, 12, 12, 12, 12, 12, 12},
};

static const u8 rtw8821c_pwrtrk_2gb_n[] = {
	0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,
	4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9
};

static const u8 rtw8821c_pwrtrk_2gb_p[] = {
	0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5,
	5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 9
};

static const u8 rtw8821c_pwrtrk_2ga_n[] = {
	0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4,
	4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 8, 8, 9
};

static const u8 rtw8821c_pwrtrk_2ga_p[] = {
	0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5,
	5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 9
};

static const u8 rtw8821c_pwrtrk_2g_cck_b_n[] = {
	0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
	4, 5, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9
};

static const u8 rtw8821c_pwrtrk_2g_cck_b_p[] = {
	0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5,
	5, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 9, 9, 9
};

static const u8 rtw8821c_pwrtrk_2g_cck_a_n[] = {
	0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4,
	4, 5, 5, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 9
};

static const u8 rtw8821c_pwrtrk_2g_cck_a_p[] = {
	0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5,
	5, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 9, 9, 9
};

static const struct rtw_pwr_track_tbl rtw8821c_pwr_track_type0_tbl = {
	.pwrtrk_5gb_n[0] = rtw8821c_pwrtrk_5gb_n[0],
	.pwrtrk_5gb_n[1] = rtw8821c_pwrtrk_5gb_n[1],
	.pwrtrk_5gb_n[2] = rtw8821c_pwrtrk_5gb_n[2],
	.pwrtrk_5gb_p[0] = rtw8821c_pwrtrk_5gb_p[0],
	.pwrtrk_5gb_p[1] = rtw8821c_pwrtrk_5gb_p[1],
	.pwrtrk_5gb_p[2] = rtw8821c_pwrtrk_5gb_p[2],
	.pwrtrk_5ga_n[0] = rtw8821c_pwrtrk_5ga_n[0],
	.pwrtrk_5ga_n[1] = rtw8821c_pwrtrk_5ga_n[1],
	.pwrtrk_5ga_n[2] = rtw8821c_pwrtrk_5ga_n[2],
	.pwrtrk_5ga_p[0] = rtw8821c_pwrtrk_5ga_p[0],
	.pwrtrk_5ga_p[1] = rtw8821c_pwrtrk_5ga_p[1],
	.pwrtrk_5ga_p[2] = rtw8821c_pwrtrk_5ga_p[2],
	.pwrtrk_2gb_n = rtw8821c_pwrtrk_2gb_n,
	.pwrtrk_2gb_p = rtw8821c_pwrtrk_2gb_p,
	.pwrtrk_2ga_n = rtw8821c_pwrtrk_2ga_n,
	.pwrtrk_2ga_p = rtw8821c_pwrtrk_2ga_p,
	.pwrtrk_2g_cckb_n = rtw8821c_pwrtrk_2g_cck_b_n,
	.pwrtrk_2g_cckb_p = rtw8821c_pwrtrk_2g_cck_b_p,
	.pwrtrk_2g_ccka_n = rtw8821c_pwrtrk_2g_cck_a_n,
	.pwrtrk_2g_ccka_p = rtw8821c_pwrtrk_2g_cck_a_p,
};

static const struct rtw_rfe_def rtw8821c_rfe_defs[] = {
	[0] = RTW_DEF_RFE(8821c, 0, 0, 0),
	[2] = RTW_DEF_RFE_EXT(8821c, 0, 0, 0, 2),
	[4] = RTW_DEF_RFE_EXT(8821c, 0, 0, 0, 2),
	[6] = RTW_DEF_RFE(8821c, 0, 0, 0),
};

static void rtw8821c_cck_tx_filter_srrc(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
	struct rtw_hal *hal = &rtwdev->hal;

	if (channel == 14) {
		rtw_write32_mask(rtwdev, REG_CCA_FLTR, MASKHWORD, 0xe82c);
		rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD, 0x0000b81c);
		rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD, 0x0000);
		rtw_write32_mask(rtwdev, REG_TXFILTER, MASKDWORD, 0x00003667);

		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, RFREG_MASK, 0x00002);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0001e);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0001c);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0000e);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0000c);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, RFREG_MASK, 0x00000);
	} else if (channel == 13 ||
		   (channel == 11 && bw == RTW_CHANNEL_WIDTH_40)) {
		rtw_write32_mask(rtwdev, REG_CCA_FLTR, MASKHWORD, 0xf8fe);
		rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD, 0x64b80c1c);
		rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD, 0x8810);
		rtw_write32_mask(rtwdev, REG_TXFILTER, MASKDWORD, 0x01235667);

		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, RFREG_MASK, 0x00002);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0001e);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00027);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0001c);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00027);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0000e);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00029);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0000c);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00026);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, RFREG_MASK, 0x00000);
	} else {
		rtw_write32_mask(rtwdev, REG_CCA_FLTR, MASKHWORD, 0xe82c);
		rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD,
				 hal->ch_param[0]);
		rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD,
				 hal->ch_param[1] & MASKLWORD);
		rtw_write32_mask(rtwdev, REG_TXFILTER, MASKDWORD,
				 hal->ch_param[2]);

		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, RFREG_MASK, 0x00002);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0001e);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0001c);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0000e);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, RFREG_MASK, 0x0000c);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, RFREG_MASK, 0x00000);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, RFREG_MASK, 0x00000);
	}
}

static void rtw8821c_do_iqk(struct rtw_dev *rtwdev)
{
	static int do_iqk_cnt;
	struct rtw_iqk_para para = {.clear = 0, .segment_iqk = 0};
	u32 rf_reg, iqk_fail_mask;
	int counter;
	bool reload;

	if (rtw_is_assoc(rtwdev))
		para.segment_iqk = 1;

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

static u32 rtw8821c_get_bb_swing(struct rtw_dev *rtwdev, u8 channel)
{
	struct rtw_efuse efuse = rtwdev->efuse;
	u8 tx_bb_swing;
	u32 swing2setting[4] = {0x200, 0x16a, 0x101, 0x0b6};

	tx_bb_swing = channel <= 14 ? efuse.tx_bb_swing_setting_2g :
				      efuse.tx_bb_swing_setting_5g;
	if (tx_bb_swing > 9)
		tx_bb_swing = 0;

	return swing2setting[(tx_bb_swing / 3)];
}

static u8 rtw8821c_get_swing_index(struct rtw_dev *rtwdev)
{
	u8 i = 0;
	u32 swing, table_value;

	swing = rtw_read32_mask(rtwdev, REG_TXSCALE_A, 0xffe00000);
	for (i = 0; i < ARRAY_SIZE(rtw8821c_txscale_tbl); i++) {
		table_value = rtw8821c_txscale_tbl[i];
		if (swing == table_value)
			break;
	}

	return i;
}

static void rtw8821c_phy_calibration(struct rtw_dev *rtwdev)
{
	rtw8821c_do_iqk(rtwdev);
}

static void rtw8821c_phy_pwrtrack(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_swing_table swing_table;
	u8 thermal_value, delta;

	rtw_phy_config_swing_table(rtwdev, &swing_table);

	if (rtwdev->efuse.thermal_meter[0] == 0xff)
		return;

	thermal_value = rtw_read_rf(rtwdev, RF_PATH_A, RF_T_METER, 0xfc00);

	rtw_phy_pwrtrack_avg(rtwdev, thermal_value, RF_PATH_A);

	if (dm_info->pwr_trk_init_trigger)
		dm_info->pwr_trk_init_trigger = false;
	else if (!rtw_phy_pwrtrack_thermal_changed(rtwdev, thermal_value,
						   RF_PATH_A))
		goto iqk;

	delta = rtw_phy_pwrtrack_get_delta(rtwdev, RF_PATH_A);

	delta = min_t(u8, delta, RTW_PWR_TRK_TBL_SZ - 1);

	dm_info->delta_power_index[RF_PATH_A] =
		rtw_phy_pwrtrack_get_pwridx(rtwdev, &swing_table, RF_PATH_A,
					    RF_PATH_A, delta);
	if (dm_info->delta_power_index[RF_PATH_A] ==
			dm_info->delta_power_index_last[RF_PATH_A])
		goto iqk;
	else
		dm_info->delta_power_index_last[RF_PATH_A] =
			dm_info->delta_power_index[RF_PATH_A];
	rtw8821c_pwrtrack_set(rtwdev);

iqk:
	if (rtw_phy_pwrtrack_need_iqk(rtwdev))
		rtw8821c_do_iqk(rtwdev);
}

static void rtw8821c_pwr_track(struct rtw_dev *rtwdev)
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

	rtw8821c_phy_pwrtrack(rtwdev);
	dm_info->pwr_trk_triggered = false;
}

static void rtw8821c_pwrtrack_init(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 swing_idx = rtw8821c_get_swing_index(rtwdev);

	if (swing_idx >= ARRAY_SIZE(rtw8821c_txscale_tbl))
		dm_info->default_ofdm_index = 24;
	else
		dm_info->default_ofdm_index = swing_idx;

	ewma_thermal_init(&dm_info->avg_thermal[RF_PATH_A]);
	dm_info->delta_power_index[RF_PATH_A] = 0;
	dm_info->delta_power_index_last[RF_PATH_A] = 0;
	dm_info->pwr_trk_triggered = false;
	dm_info->pwr_trk_init_trigger = true;
	dm_info->thermal_meter_k = rtwdev->efuse.thermal_meter_k;
}

static void rtw8821c_pwrtrack_set(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 pwr_idx_offset, tx_pwr_idx;
	s8 pwr_idx_offset_lower;
	u8 channel = rtwdev->hal.current_channel;
	u8 band_width = rtwdev->hal.current_band_width;
	u8 regd = rtw_regd_get(rtwdev);
	u8 tx_rate = dm_info->tx_rate;
	u8 max_pwr_idx = rtwdev->chip->max_power_index;

	tx_pwr_idx = native_thermal_power_index(rtwdev, RF_PATH_A, tx_rate,
						band_width, channel, regd);

	tx_pwr_idx = min_t(u8, tx_pwr_idx, max_pwr_idx);

	pwr_idx_offset = max_pwr_idx - tx_pwr_idx;
	pwr_idx_offset_lower = 0 - tx_pwr_idx;

	rtw8821c_pwrtrack_set_pwr(rtwdev, pwr_idx_offset, pwr_idx_offset_lower);
}

static void rtw8821c_pwrtrack_set_pwr(struct rtw_dev *rtwdev, u8 pwr_idx_offset,
				      s8 pwr_idx_offset_lower)
{
	s8 txagc_idx;
	u8 swing_idx;

	rtw8821c_txagc_swing_offset(rtwdev, pwr_idx_offset, pwr_idx_offset_lower,
				    &txagc_idx, &swing_idx);
	rtw_write32_mask(rtwdev, REG_TXAGCIDX, GENMASK(6, 1), txagc_idx);
	rtw_write32_mask(rtwdev, REG_TXSCALE_A, GENMASK(31, 21),
			 rtw8821c_txscale_tbl[swing_idx]);
}

static void rtw8821c_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx)
{
	rtw8821c_set_channel_bb(rtwdev, channel, bw, primary_chan_idx);
	rtw8821c_set_channel_bb_swing(rtwdev, channel, bw, primary_chan_idx);
	rtw_set_channel_mac(rtwdev, channel, bw, primary_chan_idx);
	rtw8821c_set_channel_rf(rtwdev, channel, bw);
	rtw8821c_set_channel_rxdfir(rtwdev, bw);
}

static void rtw8821c_set_channel_bb(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				    u8 primary_ch_idx)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 val32;

	if (channel <= 14) {
		rtw_write32_mask(rtwdev, REG_RXPSEL, BIT(28), 0x1);
		rtw_write32_mask(rtwdev, REG_CCK_CHECK, BIT(7), 0x0);
		rtw_write32_mask(rtwdev, REG_ENTXCCK, BIT(18), 0x0);
		rtw_write32_mask(rtwdev, REG_RXCCAMSK, 0x0000FC00, 15);

		rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 0x0);
		rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x96a);

		if (rtw_regd_srrc(rtwdev)) {
			rtw8821c_cck_tx_filter_srrc(rtwdev, channel, bw);
			goto set_bw;
		}

		/* CCK TX filter parameters for default case */
		if (channel == 14) {
			rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD, 0x0000b81c);
			rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD, 0x0000);
			rtw_write32_mask(rtwdev, REG_TXFILTER, MASKDWORD, 0x00003667);
		} else {
			rtw_write32_mask(rtwdev, REG_TXSF2, MASKDWORD,
					 hal->ch_param[0]);
			rtw_write32_mask(rtwdev, REG_TXSF6, MASKLWORD,
					 hal->ch_param[1] & MASKLWORD);
			rtw_write32_mask(rtwdev, REG_TXFILTER, MASKDWORD,
					 hal->ch_param[2]);
		}
	} else if (channel > 35) {
		rtw_write32_mask(rtwdev, REG_ENTXCCK, BIT(18), 0x1);
		rtw_write32_mask(rtwdev, REG_CCK_CHECK, BIT(7), 0x1);
		rtw_write32_mask(rtwdev, REG_RXPSEL, BIT(28), 0x0);
		rtw_write32_mask(rtwdev, REG_RXCCAMSK, 0x0000FC00, 15);

		if (channel >= 36 && channel <= 64)
			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 0x1);
		else if (channel >= 100 && channel <= 144)
			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 0x2);
		else if (channel >= 149)
			rtw_write32_mask(rtwdev, REG_TXSCALE_A, 0xf00, 0x3);

		if (channel >= 36 && channel <= 48)
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x494);
		else if (channel >= 52 && channel <= 64)
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x453);
		else if (channel >= 100 && channel <= 116)
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x452);
		else if (channel >= 118 && channel <= 177)
			rtw_write32_mask(rtwdev, REG_CLKTRK, 0x1ffe0000, 0x412);
	}

set_bw:
	switch (bw) {
	case RTW_CHANNEL_WIDTH_20:
	default:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xffcffc00;
		val32 |= 0x10010000;
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_40:
		if (primary_ch_idx == 1)
			rtw_write32_set(rtwdev, REG_RXSB, BIT(4));
		else
			rtw_write32_clr(rtwdev, REG_RXSB, BIT(4));

		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xff3ff300;
		val32 |= 0x20020000 | ((primary_ch_idx & 0xf) << 2) |
			 RTW_CHANNEL_WIDTH_40;
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_80:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xfcffcf00;
		val32 |= 0x40040000 | ((primary_ch_idx & 0xf) << 2) |
			 RTW_CHANNEL_WIDTH_80;
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_5:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xefcefc00;
		val32 |= 0x200240;
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x0);
		rtw_write32_mask(rtwdev, REG_ADC40, BIT(31), 0x1);
		break;
	case RTW_CHANNEL_WIDTH_10:
		val32 = rtw_read32_mask(rtwdev, REG_ADCCLK, MASKDWORD);
		val32 &= 0xefcefc00;
		val32 |= 0x300380;
		rtw_write32_mask(rtwdev, REG_ADCCLK, MASKDWORD, val32);

		rtw_write32_mask(rtwdev, REG_ADC160, BIT(30), 0x0);
		rtw_write32_mask(rtwdev, REG_ADC40, BIT(31), 0x1);
		break;
	}
}

static void rtw8821c_set_channel_bb_swing(struct rtw_dev *rtwdev, u8 channel,
					  u8 bw, u8 primary_ch_idx)
{
	rtw_write32_mask(rtwdev, REG_TXSCALE_A, GENMASK(31, 21),
			 rtw8821c_get_bb_swing(rtwdev, channel));
	rtw8821c_pwrtrack_init(rtwdev);
}

static void rtw8821c_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 rf_reg18;

	rf_reg18 = rtw_read_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK);

	rf_reg18 &= ~(RF18_BAND_MASK | RF18_CHANNEL_MASK | RF18_RFSI_MASK |
		      RF18_BW_MASK);

	rf_reg18 |= (channel <= 14 ? RF18_BAND_2G : RF18_BAND_5G);
	rf_reg18 |= (channel & RF18_CHANNEL_MASK);

	if (channel >= 100 && channel <= 140)
		rf_reg18 |= RF18_RFSI_GE;
	else if (channel > 140)
		rf_reg18 |= RF18_RFSI_GT;

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

	if (channel <= 14) {
		if (hal->rfe_btg)
			rtw8821c_switch_rf_set(rtwdev, SWITCH_TO_BTG);
		else
			rtw8821c_switch_rf_set(rtwdev, SWITCH_TO_WLG);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTDBG, BIT(6), 0x1);
		rtw_write_rf(rtwdev, RF_PATH_A, 0x64, 0xf, 0xf);
	} else {
		rtw8821c_switch_rf_set(rtwdev, SWITCH_TO_WLA);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTDBG, BIT(6), 0x0);
	}

	rtw_write_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK, rf_reg18);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_XTALX2, BIT(19), 0);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_XTALX2, BIT(19), 1);
}

static void rtw8821c_set_channel_rxdfir(struct rtw_dev *rtwdev, u8 bw)
{
	if (bw == RTW_CHANNEL_WIDTH_40) {
		/* RX DFIR for BW40 */
		rtw_write32_mask(rtwdev, REG_ACBB0, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_ACBBRXFIR, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_TXDFIR, BIT(31), 0x0);
		rtw_write32_mask(rtwdev, REG_CHFIR, BIT(31), 0x0);
	} else if (bw == RTW_CHANNEL_WIDTH_80) {
		/* RX DFIR for BW80 */
		rtw_write32_mask(rtwdev, REG_ACBB0, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_ACBBRXFIR, BIT(29) | BIT(28), 0x1);
		rtw_write32_mask(rtwdev, REG_TXDFIR, BIT(31), 0x0);
		rtw_write32_mask(rtwdev, REG_CHFIR, BIT(31), 0x1);
	} else {
		/* RX DFIR for BW20, BW10 and BW5 */
		rtw_write32_mask(rtwdev, REG_ACBB0, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_ACBBRXFIR, BIT(29) | BIT(28), 0x2);
		rtw_write32_mask(rtwdev, REG_TXDFIR, BIT(31), 0x1);
		rtw_write32_mask(rtwdev, REG_CHFIR, BIT(31), 0x0);
	}
}

static void rtw8821c_set_tx_power_index(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 phy_pwr_idx = 0;
	int rs, path;

	for (path = 0; path < hal->rf_path_num; path++) {
		for (rs = 0; rs <= __RTW_RATE_SECTION_2SS_MAX; rs++) {
			if (rs == RTW_RATE_SECTION_HT_2S ||
			    rs == RTW_RATE_SECTION_VHT_2S)
				continue;
			rtw8821c_set_tx_power_index_by_rate(rtwdev, path, rs,
							    &phy_pwr_idx);
		}
	}
}

static void
rtw8821c_set_tx_power_index_by_rate(struct rtw_dev *rtwdev, u8 path,
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
		if (shift == 0x3 || rate == DESC_RATEVHT1SS_MCS9) {
			rate_idx = rate & 0xfc;
			rtw_write32(rtwdev, offset_txagc[path] + rate_idx,
				    *phy_pwr_idx);
			*phy_pwr_idx = 0;
		}
	}
}

static void rtw8821c_switch_rf_set(struct rtw_dev *rtwdev, u8 rf_set)
{
	u32 reg;

	rtw_write32_set(rtwdev, REG_DMEM_CTRL, BIT_WL_RST);
	rtw_write32_set(rtwdev, REG_SYS_CTRL, BIT_FEN_EN);

	reg = rtw_read32(rtwdev, REG_RFECTL);
	switch (rf_set) {
	case SWITCH_TO_BTG:
		reg |= B_BTG_SWITCH;
		reg &= ~(B_CTRL_SWITCH | B_WL_SWITCH | B_WLG_SWITCH |
			 B_WLA_SWITCH);
		rtw_write32_mask(rtwdev, REG_ENRXCCA, MASKBYTE2, BTG_CCA);
		rtw_write32_mask(rtwdev, REG_ENTXCCK, MASKLWORD, BTG_LNA);
		break;
	case SWITCH_TO_WLG:
		reg |= B_WL_SWITCH | B_WLG_SWITCH;
		reg &= ~(B_BTG_SWITCH | B_CTRL_SWITCH | B_WLA_SWITCH);
		rtw_write32_mask(rtwdev, REG_ENRXCCA, MASKBYTE2, WLG_CCA);
		rtw_write32_mask(rtwdev, REG_ENTXCCK, MASKLWORD, WLG_LNA);
		break;
	case SWITCH_TO_WLA:
		reg |= B_WL_SWITCH | B_WLA_SWITCH;
		reg &= ~(B_BTG_SWITCH | B_CTRL_SWITCH | B_WLG_SWITCH);
		break;
	case SWITCH_TO_BT:
	default:
		break;
	}

	rtw_write32(rtwdev, REG_RFECTL, reg);
}

static void
rtw8821c_txagc_swing_offset(struct rtw_dev *rtwdev, u8 pwr_idx_offset,
			    s8 pwr_idx_offset_lower,
			    s8 *txagc_idx, u8 *swing_idx)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	s8 delta_pwr_idx = dm_info->delta_power_index[RF_PATH_A];
	u8 swing_upper_bound = dm_info->default_ofdm_index + 10;
	u8 swing_lower_bound = 0;
	u8 max_pwr_idx_offset = 0xf;
	s8 agc_index = 0;
	u8 swing_index = dm_info->default_ofdm_index;

	pwr_idx_offset = min_t(u8, pwr_idx_offset, max_pwr_idx_offset);
	pwr_idx_offset_lower = max_t(s8, pwr_idx_offset_lower, -15);

	if (delta_pwr_idx >= 0) {
		if (delta_pwr_idx <= pwr_idx_offset) {
			agc_index = delta_pwr_idx;
			swing_index = dm_info->default_ofdm_index;
		} else if (delta_pwr_idx > pwr_idx_offset) {
			agc_index = pwr_idx_offset;
			swing_index = dm_info->default_ofdm_index +
					delta_pwr_idx - pwr_idx_offset;
			swing_index = min_t(u8, swing_index, swing_upper_bound);
		}
	} else if (delta_pwr_idx < 0) {
		if (delta_pwr_idx >= pwr_idx_offset_lower) {
			agc_index = delta_pwr_idx;
			swing_index = dm_info->default_ofdm_index;
		} else if (delta_pwr_idx < pwr_idx_offset_lower) {
			if (dm_info->default_ofdm_index >
				(pwr_idx_offset_lower - delta_pwr_idx))
				swing_index = dm_info->default_ofdm_index +
					delta_pwr_idx - pwr_idx_offset_lower;
			else
				swing_index = swing_lower_bound;

			agc_index = pwr_idx_offset_lower;
		}
	}

	if (swing_index >= ARRAY_SIZE(rtw8821c_txscale_tbl)) {
		rtw_warn(rtwdev, "swing index overflow\n");
		swing_index = ARRAY_SIZE(rtw8821c_txscale_tbl) - 1;
	}

	*txagc_idx = agc_index;
	*swing_idx = swing_index;
}
