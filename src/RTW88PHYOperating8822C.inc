/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

/* Copyright(c)2018-2019 Realtek Corporation. Pinned function bodies; checked adapter in RTW88PHYOperating.c. */

#define RTW_DECL_TABLE_DPK(name)			 const struct rtw_table name ## _tbl = {			 	.data = name,					 	.size = ARRAY_SIZE(name),			 	.parse = rtw8822c_parse_tbl_dpk,		 }
#define DACK_PATH_8822C		2
#define DACK_REG_8822C		16
#define DACK_RF_8822C		1
#define DACK_SN_8822C		100
#define GET_PHY_STAT_P0_PWDB_A(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#define GET_PHY_STAT_P0_PWDB_B(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(7, 0))
#define GET_PHY_STAT_P0_GAIN_A(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(21, 16))
#define GET_PHY_STAT_P0_CHANNEL(phy_stat)				        	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(23, 16))
#define GET_PHY_STAT_P0_GAIN_B(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(29, 24))
#define GET_PHY_STAT_P1_PWDB_A(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#define GET_PHY_STAT_P1_PWDB_B(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(23, 16))
#define GET_PHY_STAT_P1_L_RXSC(phy_stat)                                        	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(11, 8))
#define GET_PHY_STAT_P1_HT_RXSC(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(15, 12))
#define GET_PHY_STAT_P1_CHANNEL(phy_stat)				        	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(23, 16))
#define GET_PHY_STAT_P1_RXEVM_A(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(7, 0))
#define GET_PHY_STAT_P1_RXEVM_B(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(15, 8))
#define GET_PHY_STAT_P1_CFO_TAIL_A(phy_stat)                                  	le32_get_bits(*((__le32 *)(phy_stat) + 0x05), GENMASK(7, 0))
#define GET_PHY_STAT_P1_CFO_TAIL_B(phy_stat)                                  	le32_get_bits(*((__le32 *)(phy_stat) + 0x05), GENMASK(15, 8))
#define GET_PHY_STAT_P1_RXSNR_A(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x06), GENMASK(7, 0))
#define GET_PHY_STAT_P1_RXSNR_B(phy_stat)                                       	le32_get_bits(*((__le32 *)(phy_stat) + 0x06), GENMASK(15, 8))
#define RTW8822C_EDCCA_MAX	0x7f
#define REG_ANAPARLDO_POW_MAC	0x0029
#define BIT_LDOE25_PON		BIT(0)
#define XCAP_MASK		GENMASK(6, 0)
#define CFO_TRK_ENABLE_TH	20
#define CFO_TRK_STOP_TH		10
#define CFO_TRK_ADJ_TH		10
#define REG_TXDFIR0		0x808
#define REG_DFIRBW		0x810
#define REG_ANTMAP0		0x820
#define BIT_ANT_PATH		GENMASK(1, 0)
#define REG_ANTMAP		0x824
#define REG_EDCCA_DECISION	0x844
#define BIT_EDCCA_OPTION	GENMASK(30, 29)
#define REG_DYMPRITH		0x86c
#define REG_DYMENTH0		0x870
#define REG_DYMENTH		0x874
#define REG_SBD			0x88c
#define BITS_SUBTUNE		GENMASK(15, 12)
#define REG_DYMTHMIN		0x8a4
#define REG_TXBWCTL		0x9b0
#define REG_TXCLK		0x9b4
#define REG_SCOTRK		0xc30
#define REG_MRCM		0xc38
#define REG_AGCSWSH		0xc44
#define REG_ANTWTPD		0xc54
#define REG_PT_CHSMO		0xcbc
#define BIT_PT_OPT		BIT(21)
#define REG_ORITXCODE		0x1800
#define BIT_PATH_EN		BIT(31)
#define REG_3WIRE		0x180c
#define BIT_DIS_SHARERX_TXGAT	BIT(27)
#define BIT_3WIRE_TX_EN		BIT(0)
#define BIT_3WIRE_RX_EN		BIT(1)
#define BIT_3WIRE_EN		GENMASK(1, 0)
#define BIT_3WIRE_PI_ON		BIT(28)
#define REG_ANAPAR_A		0x1830
#define BIT_ANAPAR_UPDATE	BIT(29)
#define REG_RFTXEN_GCK_A	0x1864
#define BIT_RFTXEN_GCK_FORCE_ON	BIT(31)
#define REG_DIS_SHARE_RX_A	0x186c
#define BIT_TX_SCALE_0DB	BIT(7)
#define REG_RXAGCCTL0		0x18ac
#define BITS_RXAGC_CCK		GENMASK(15, 12)
#define BITS_RXAGC_OFDM		GENMASK(8, 4)
#define REG_DCKA_I_0		0x18bc
#define REG_DCKA_I_1		0x18c0
#define REG_DCKA_Q_0		0x18d8
#define REG_DCKA_Q_1		0x18dc
#define REG_CCKSB		0x1a00
#define BIT_BBMODE		GENMASK(2, 1)
#define REG_RXCCKSEL		0x1a04
#define REG_BGCTRL		0x1a14
#define BITS_RX_IQ_WEIGHT	(BIT(8) | BIT(9))
#define REG_TXF0		0x1a20
#define REG_TXF1		0x1a24
#define REG_TXF2		0x1a28
#define REG_CCANRX		0x1a2c
#define BIT_CCK_FA_RST		(BIT(14) | BIT(15))
#define BIT_OFDM_FA_RST		(BIT(12) | BIT(13))
#define REG_CCK_FACNT		0x1a5c
#define REG_CCKTXONLY		0x1a80
#define BIT_BB_CCK_CHECK_EN	BIT(18)
#define REG_TXF3		0x1a98
#define REG_TXF4		0x1a9c
#define REG_TXF5		0x1aa0
#define REG_TXF6		0x1aac
#define REG_TXF7		0x1ab0
#define REG_CCK_SOURCE		0x1abc
#define BIT_NBI_EN		BIT(30)
#define REG_NCTL0		0x1b00
#define BIT_SEL_PATH		GENMASK(2, 1)
#define BIT_SUBPAGE		GENMASK(3, 0)
#define REG_DPD_CTL0_S0		0x1b04
#define BIT_GS_PWSF		GENMASK(27, 0)
#define REG_DPD_CTL1_S0		0x1b08
#define BIT_DPD_EN		BIT(31)
#define BIT_PS_EN		BIT(7)
#define REG_IQKSTAT		0x1b10
#define REG_IQK_CTL1		0x1b20
#define BIT_TX_CFIR		GENMASK(31, 30)
#define BIT_CFIR_EN		GENMASK(26, 24)
#define BIT_BYPASS_DPD		BIT(25)
#define REG_TX_TONE_IDX		0x1b2c
#define REG_DPD_LUT0		0x1b44
#define BIT_GLOSS_DB		GENMASK(14, 12)
#define REG_DPD_CTL0_S1		0x1b5c
#define REG_DPD_CTL1_S1		0x1b60
#define REG_DPD_AGC		0x1b67
#define REG_TABLE_SEL		0x1b98
#define BIT_I_GAIN		GENMASK(19, 16)
#define BIT_GAIN_RST		BIT(15)
#define BIT_Q_GAIN_SEL		GENMASK(14, 12)
#define BIT_Q_GAIN		GENMASK(11, 0)
#define REG_TX_GAIN_SET		0x1b9c
#define BIT_GAPK_RPT_IDX	GENMASK(11, 8)
#define REG_DPD_CTL0		0x1bb4
#define REG_SINGLE_TONE_SW	0x1bb8
#define BIT_IRQ_TEST_MODE	BIT(20)
#define REG_R_CONFIG		0x1bcc
#define BIT_INNER_LB		BIT(21)
#define BIT_IQ_SWITCH		GENMASK(5, 0)
#define BIT_2G_SWING		0x2d
#define BIT_5G_SWING		0x36
#define REG_RXSRAM_CTL		0x1bd4
#define BIT_RPT_EN		BIT(21)
#define BIT_RPT_SEL		GENMASK(20, 16)
#define BIT_DPD_CLK		GENMASK(7, 4)
#define REG_DPD_CTL11		0x1be4
#define REG_DPD_CTL12		0x1be8
#define REG_DPD_CTL15		0x1bf4
#define REG_DPD_CTL16		0x1bf8
#define REG_STAT_RPT		0x1bfc
#define BIT_RPT_DGAIN		GENMASK(27, 16)
#define BIT_GAPK_RPT0		GENMASK(3, 0)
#define BIT_GAPK_RPT1		GENMASK(7, 4)
#define BIT_GAPK_RPT2		GENMASK(11, 8)
#define BIT_GAPK_RPT3		GENMASK(15, 12)
#define BIT_GAPK_RPT4		GENMASK(19, 16)
#define BIT_GAPK_RPT5		GENMASK(23, 20)
#define BIT_GAPK_RPT6		GENMASK(27, 24)
#define BIT_GAPK_RPT7		GENMASK(31, 28)
#define REG_TXANT		0x1c28
#define REG_IQK_CTRL		0x1c38
#define REG_ENCCK		0x1c3c
#define BIT_CCK_BLK_EN		BIT(1)
#define BIT_CCK_OFDM_BLK_EN	(BIT(0) | BIT(1))
#define REG_CCAMSK		0x1c80
#define REG_RSTB		0x1c90
#define BIT_RSTB_3WIRE		BIT(8)
#define REG_CH_DELAY_EXTR2	0x1cd0
#define BIT_TST_IQK2SET_SRC	BIT(31)
#define BIT_EN_IOQ_IQK_DPK	BIT(30)
#define BIT_IQK_DPK_RESET_SRC	BIT(29)
#define BIT_IQK_DPK_CLOCK_SRC	BIT(28)
#define REG_RX_BREAK		0x1d2c
#define BIT_COM_RX_GCK_EN	BIT(31)
#define REG_RXFNCTL		0x1d30
#define REG_CCA_OFF		0x1d58
#define BIT_CCA_ON_BY_PW	GENMASK(11, 3)
#define REG_RXIGI		0x1d70
#define REG_ENFN		0x1e24
#define BIT_IQK_DPK_EN		BIT(17)
#define REG_TXANTSEG		0x1e28
#define BIT_ANTSEG		GENMASK(3, 0)
#define REG_TXLGMAP		0x1e2c
#define REG_CCKPATH		0x1e5c
#define REG_TX_FIFO		0x1e70
#define BIT_STOP_TX		GENMASK(3, 0)
#define REG_CNT_CTRL		0x1eb4
#define BIT_ALL_CNT_RST		BIT(25)
#define REG_OFDM_FACNT		0x2d00
#define REG_OFDM_FACNT1		0x2d04
#define REG_OFDM_FACNT2		0x2d08
#define REG_OFDM_FACNT3		0x2d0c
#define REG_OFDM_FACNT4		0x2d10
#define REG_OFDM_FACNT5		0x2d20
#define REG_RPT_CIP		0x2d9c
#define BIT_RPT_CIP_STATUS	GENMASK(7, 0)
#define REG_OFDM_TXCNT		0x2de0
#define REG_ORITXCODE2		0x4100
#define REG_3WIRE2		0x410c
#define REG_ANAPAR_B		0x4130
#define REG_RFTXEN_GCK_B	0x4164
#define REG_DIS_SHARE_RX_B	0x416c
#define BIT_EXT_TIA_BW		BIT(1)
#define REG_RXAGCCTL		0x41ac
#define REG_DCKB_I_0		0x41bc
#define REG_DCKB_I_1		0x41c0
#define REG_DCKB_Q_0		0x41d8
#define REG_DCKB_Q_1		0x41dc
#define RF_MODE_TRXAGC		0x00
#define BIT_RF_MODE		GENMASK(19, 16)
#define BIT_RXAGC		GENMASK(9, 5)
#define BIT_TXAGC		GENMASK(4, 0)
#define RF_RXAGC_OFFSET		0x19
#define RF_BW_TRXBB		0x1a
#define BIT_TX_CCK_IND		BIT(16)
#define BIT_BW_TXBB		GENMASK(14, 12)
#define BIT_BW_RXBB		GENMASK(11, 10)
#define BIT_DBG_CCK_CCA		BIT(1)
#define RF_TX_GAIN_OFFSET	0x55
#define BIT_BB_GAIN		GENMASK(18, 14)
#define BIT_RF_GAIN		GENMASK(4, 2)
#define RF_TX_GAIN		0x56
#define BIT_GAIN_TXBB		GENMASK(4, 0)
#define RF_IDAC			0x58
#define BIT_TX_MODE		GENMASK(19, 8)
#define RF_TX_RESULT		0x5f
#define BIT_GAIN_TX_PAD_H	GENMASK(11, 8)
#define BIT_GAIN_TX_PAD_L	GENMASK(7, 4)
#define RF_PA			0x60
#define RF_PABIAS_2G_MASK	GENMASK(15, 12)
#define RF_PABIAS_5G_MASK	GENMASK(19, 16)
#define RF_TXA_LB_SW		0x63
#define BIT_TXA_LB_ATT		GENMASK(15, 14)
#define BIT_LB_SW		GENMASK(13, 12)
#define BIT_LB_ATT		GENMASK(4, 2)
#define RF_RXG_GAIN		0x87
#define BIT_RXG_GAIN		BIT(18)
#define RF_RXA_MIX_GAIN		0x8a
#define BIT_RXA_MIX_GAIN	GENMASK(4, 3)
#define RF_EXT_TIA_BW		0x8f
#define BIT_PW_EXT_TIA		BIT(1)
#define RF_DIS_BYPASS_TXBB	0x9e
#define BIT_TXBB		BIT(10)
#define BIT_TIA_BYPASS		BIT(5)
#define RF_DEBUG		0xde
#define BIT_DE_PWR_TRIM		BIT(19)
#define BIT_DE_TX_GAIN		BIT(16)
#define BIT_DE_TRXBW		BIT(2)
#define PPG_THERMAL_B		0x1b0
#define RF_THEMAL_MASK		GENMASK(19, 16)
#define PPG_2GH_TXAB		0x1d2
#define PPG_2G_A_MASK		GENMASK(3, 0)
#define PPG_2G_B_MASK		GENMASK(7, 4)
#define PPG_2GL_TXAB		0x1d4
#define PPG_PABIAS_2GB		0x1d5
#define PPG_PABIAS_2GA		0x1d6
#define PPG_PABIAS_MASK		GENMASK(3, 0)
#define PPG_PABIAS_5GB		0x1d7
#define PPG_PABIAS_5GA		0x1d8
#define PPG_5G_MASK		GENMASK(4, 0)
#define PPG_5GH1_TXB		0x1db
#define PPG_5GH1_TXA		0x1dc
#define PPG_5GM2_TXB		0x1df
#define PPG_5GM2_TXA		0x1e0
#define PPG_5GM1_TXB		0x1e3
#define PPG_5GM1_TXA		0x1e4
#define PPG_5GL2_TXB		0x1e7
#define PPG_5GL2_TXA		0x1e8
#define PPG_5GL1_TXB		0x1eb
#define PPG_5GL1_TXA		0x1ec
#define PPG_2GM_TXAB		0x1ee
#define PPG_THERMAL_A		0x1ef

#define IQK_DONE_8822C 0xaa
#define WLAN_TXQ_RPT_EN		0x1F
#define WLAN_SLOT_TIME		0x09
#define WLAN_PIFS_TIME		0x1C
#define WLAN_SIFS_CCK_CONT_TX	0x0A
#define WLAN_SIFS_OFDM_CONT_TX	0x0E
#define WLAN_SIFS_CCK_TRX	0x0A
#define WLAN_SIFS_OFDM_TRX	0x10
#define WLAN_NAV_MAX		0xC8
#define WLAN_RDG_NAV		0x05
#define WLAN_TXOP_NAV		0x1B
#define WLAN_CCK_RX_TSF		0x30
#define WLAN_OFDM_RX_TSF	0x30
#define WLAN_TBTT_PROHIBIT	0x04 /* unit : 32us */
#define WLAN_TBTT_HOLD_TIME	0x064 /* unit : 32us */
#define WLAN_DRV_EARLY_INT	0x04
#define WLAN_BCN_CTRL_CLT0	0x10
#define WLAN_BCN_DMA_TIME	0x02
#define WLAN_BCN_MAX_ERR	0xFF
#define WLAN_SIFS_CCK_DUR_TUNE	0x0A
#define WLAN_SIFS_OFDM_DUR_TUNE	0x10
#define WLAN_SIFS_CCK_CTX	0x0A
#define WLAN_SIFS_CCK_IRX	0x0A
#define WLAN_SIFS_OFDM_CTX	0x0E
#define WLAN_SIFS_OFDM_IRX	0x0E
#define WLAN_EIFS_DUR_TUNE	0x40
#define WLAN_EDCA_VO_PARAM	0x002FA226
#define WLAN_EDCA_VI_PARAM	0x005EA328
#define WLAN_EDCA_BE_PARAM	0x005EA42B
#define WLAN_EDCA_BK_PARAM	0x0000A44F
#define WLAN_RX_FILTER0		0xFFFF
#define WLAN_RX_FILTER1		0xFFFF
#define WLAN_RX_FILTER2		0xFFFF
#define WLAN_RCR_CFG		0xE400220E
#define WLAN_RXPKT_MAX_SZ	12288
#define WLAN_RXPKT_MAX_SZ_512	(WLAN_RXPKT_MAX_SZ >> 9)
#define WLAN_AMPDU_MAX_TIME		0x70
#define WLAN_RTS_LEN_TH			0xFF
#define WLAN_RTS_TX_TIME_TH		0x08
#define WLAN_MAX_AGG_PKT_LIMIT		0x3f
#define WLAN_RTS_MAX_AGG_PKT_LIMIT	0x3f
#define WLAN_PRE_TXCNT_TIME_TH		0x1E0
#define FAST_EDCA_VO_TH		0x06
#define FAST_EDCA_VI_TH		0x06
#define FAST_EDCA_BE_TH		0x06
#define FAST_EDCA_BK_TH		0x06
#define WLAN_BAR_RETRY_LIMIT		0x01
#define WLAN_BAR_ACK_TYPE		0x05
#define WLAN_RA_TRY_RATE_AGG_LIMIT	0x08
#define WLAN_RESP_TXRATE		0x84
#define WLAN_ACK_TO			0x21
#define WLAN_ACK_TO_CCK			0x6A
#define WLAN_DATA_RATE_FB_CNT_1_4	0x01000000
#define WLAN_DATA_RATE_FB_CNT_5_8	0x08070504
#define WLAN_RTS_RATE_FB_CNT_5_8	0x08070504
#define WLAN_DATA_RATE_FB_RATE0		0xFE01F010
#define WLAN_DATA_RATE_FB_RATE0_H	0x40000000
#define WLAN_RTS_RATE_FB_RATE1		0x003FF010
#define WLAN_RTS_RATE_FB_RATE1_H	0x40000000
#define WLAN_RTS_RATE_FB_RATE4		0x0600F010
#define WLAN_RTS_RATE_FB_RATE4_H	0x400003E0
#define WLAN_RTS_RATE_FB_RATE5		0x0600F015
#define WLAN_RTS_RATE_FB_RATE5_H	0x000000E0
#define WLAN_MULTI_ADDR			0xFFFFFFFF
#define WLAN_TX_FUNC_CFG1		0x30
#define WLAN_TX_FUNC_CFG2		0x30
#define WLAN_MAC_OPT_NORM_FUNC1		0x98
#define WLAN_MAC_OPT_LB_FUNC1		0x80
#define WLAN_MAC_OPT_FUNC2		0xb0810041
#define WLAN_MAC_INT_MIG_CFG		0x33330000
#define WLAN_SIFS_CFG	(WLAN_SIFS_CCK_CONT_TX |  			(WLAN_SIFS_OFDM_CONT_TX << BIT_SHIFT_SIFS_OFDM_CTX) |  			(WLAN_SIFS_CCK_TRX << BIT_SHIFT_SIFS_CCK_TRX) |  			(WLAN_SIFS_OFDM_TRX << BIT_SHIFT_SIFS_OFDM_TRX))
#define WLAN_SIFS_DUR_TUNE	(WLAN_SIFS_CCK_DUR_TUNE |  				(WLAN_SIFS_OFDM_DUR_TUNE << 8))
#define WLAN_TBTT_TIME	(WLAN_TBTT_PROHIBIT | 			(WLAN_TBTT_HOLD_TIME << BIT_SHIFT_TBTT_HOLD_TIME_AP))
#define WLAN_NAV_CFG		(WLAN_RDG_NAV | (WLAN_TXOP_NAV << 16))
#define WLAN_RX_TSF_CFG		(WLAN_CCK_RX_TSF | (WLAN_OFDM_RX_TSF) << 8)
#define MAC_CLK_SPEED	80 /* 80M */
#define EFUSE_PCB_INFO_OFFSET	0xCA
#define FWCD_SIZE_REG_8822C 0x2000
#define FWCD_SIZE_DMEM_8822C 0x10000
#define FWCD_SIZE_IMEM_8822C 0x10000
#define FWCD_SIZE_EMEM_8822C 0x20000
#define FWCD_SIZE_ROM_8822C 0x10000
#define XCAP_EXTEND(val) ({typeof(val) _v = (val); _v | _v << 7; })
#define REPORT_TO_KHZ(val) ({typeof(val) _v = (val); (_v << 1) + (_v >> 1); })
#define RTW_CCK_PD_MAX 255
#define RTW_CCK_CS_MAX 31
#define RTW_CCK_CS_ERR1 27
#define RTW_CCK_CS_ERR2 29
#define PWR_TRACK_MASK 0x7f

enum rtw8822c_dpk_agc_phase {
	RTW_DPK_GAIN_CHECK,
	RTW_DPK_GAIN_LARGE,
	RTW_DPK_GAIN_LESS,
	RTW_DPK_GL_LARGE,
	RTW_DPK_GL_LESS,
	RTW_DPK_LOSS_CHECK,
	RTW_DPK_AGC_OUT,
};

enum rtw8822c_dpk_one_shot_action {
	RTW_DPK_CAL_PWR,
	RTW_DPK_GAIN_LOSS,
	RTW_DPK_DO_DPK,
	RTW_DPK_DPK_ON,
	RTW_DPK_DAGC,
	RTW_DPK_ACTION_MAX
};

static u8 (*dpk_state[])(struct rtw_dev *rtwdev,
			  struct rtw8822c_dpk_data *data) = {
	rtw8822c_gain_check_state, rtw8822c_gain_large_state,
	rtw8822c_gain_less_state, rtw8822c_gl_large_state,
	rtw8822c_gl_less_state, rtw8822c_loss_check_state };

struct rtw8822c_dpk_data {
	u8 txbb;
	u8 pga;
	u8 limited_pga;
	u8 agc_cnt;
	bool loss_only;
	bool gain_only;
	u8 path;
};

struct dpk_cfg_pair {
	u32 addr;
	u32 bitmask;
	u32 data;
};

static const u32 rtw8822c_dpk_get_coef_tbl[];

static const u8
rtw8822c_pwrtrk_5gb_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8
rtw8822c_pwrtrk_5gb_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8
rtw8822c_pwrtrk_5ga_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8
rtw8822c_pwrtrk_5ga_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2gb_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2gb_p[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2ga_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2ga_p[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2g_cck_b_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2g_cck_b_p[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2g_cck_a_n[RTW_PWR_TRK_TBL_SZ];

static const u8 rtw8822c_pwrtrk_2g_cck_a_p[RTW_PWR_TRK_TBL_SZ];

static const struct rtw_pwr_track_tbl rtw8822c_pwr_track_type0_tbl;

static const struct rtw_rfe_def rtw8822c_rfe_defs[];

static const u32 rtw8822c_dpk_get_coef_tbl[] = {
	0x000400f0, 0x040400f0, 0x080400f0, 0x010400f0, 0x050400f0,
	0x090400f0, 0x020400f0, 0x060400f0, 0x0a0400f0, 0x030400f0,
	0x070400f0, 0x0b0400f0, 0x0c0400f0, 0x100400f0, 0x0d0400f0,
	0x110400f0, 0x0e0400f0, 0x120400f0, 0x0f0400f0, 0x130400f0,
};

static const u8
rtw8822c_pwrtrk_5gb_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  3,  5,  6,  7,  8,  9, 10,
	 11, 12, 13, 14, 15, 16, 18, 19, 20, 21,
	 22, 23, 24, 25, 26, 27, 28, 29, 30, 32 },
	{ 0,  1,  2,  3,  5,  6,  7,  8,  9, 10,
	 11, 12, 13, 14, 15, 16, 18, 19, 20, 21,
	 22, 23, 24, 25, 26, 27, 28, 29, 30, 32 },
	{ 0,  1,  2,  3,  5,  6,  7,  8,  9, 10,
	 11, 12, 13, 14, 15, 16, 18, 19, 20, 21,
	 22, 23, 24, 25, 26, 27, 28, 29, 30, 32 },
};

static const u8
rtw8822c_pwrtrk_5gb_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	 10, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	 19, 20, 21, 22, 22, 23, 24, 25, 26, 27 },
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	 10, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	 19, 20, 21, 22, 22, 23, 24, 25, 26, 27 },
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	 10, 10, 11, 12, 13, 14, 15, 16, 17, 18,
	 19, 20, 21, 22, 22, 23, 24, 25, 26, 27 },
};

static const u8
rtw8822c_pwrtrk_5ga_n[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  4,  5,  6,  7,  8,  9, 10,
	 11, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	 23, 24, 25, 26, 27, 28, 29, 30, 31, 33 },
	{ 0,  1,  2,  4,  5,  6,  7,  8,  9, 10,
	 11, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	 23, 24, 25, 26, 27, 28, 29, 30, 31, 33 },
	{ 0,  1,  2,  4,  5,  6,  7,  8,  9, 10,
	 11, 13, 14, 15, 16, 17, 18, 19, 20, 21,
	 23, 24, 25, 26, 27, 28, 29, 30, 31, 33 },
};

static const u8
rtw8822c_pwrtrk_5ga_p[RTW_PWR_TRK_5G_NUM][RTW_PWR_TRK_TBL_SZ] = {
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 20,
	 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 },
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 20,
	 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 },
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	 10, 11, 12, 13, 14, 15, 16, 17, 18, 20,
	 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 },
};

static const u8 rtw8822c_pwrtrk_2gb_n[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  4,  4,  5,  6,  7,  8,
	 9,  9, 10, 11, 12, 13, 14, 15, 15, 16,
	17, 18, 19, 20, 20, 21, 22, 23, 24, 25
};

static const u8 rtw8822c_pwrtrk_2gb_p[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24, 25, 26, 27, 28
};

static const u8 rtw8822c_pwrtrk_2ga_n[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  2,  3,  4,  4,  5,  6,  6,
	 7,  8,  8,  9,  9, 10, 11, 11, 12, 13,
	13, 14, 15, 15, 16, 17, 17, 18, 19, 19
};

static const u8 rtw8822c_pwrtrk_2ga_p[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 11, 12, 13, 14, 15, 16, 17, 18,
	19, 20, 21, 22, 23, 24, 25, 25, 26, 27
};

static const u8 rtw8822c_pwrtrk_2g_cck_b_n[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  4,  5,  5,  6,  7,  8,
	 9, 10, 11, 11, 12, 13, 14, 15, 16, 17,
	17, 18, 19, 20, 21, 22, 23, 23, 24, 25
};

static const u8 rtw8822c_pwrtrk_2g_cck_b_p[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 26, 27, 28, 29
};

static const u8 rtw8822c_pwrtrk_2g_cck_a_n[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  3,  4,  5,  6,  6,  7,
	 8,  9,  9, 10, 11, 12, 12, 13, 14, 15,
	15, 16, 17, 18, 18, 19, 20, 21, 21, 22
};

static const u8 rtw8822c_pwrtrk_2g_cck_a_p[RTW_PWR_TRK_TBL_SZ] = {
	 0,  1,  2,  3,  4,  5,  5,  6,  7,  8,
	 9, 10, 11, 11, 12, 13, 14, 15, 16, 17,
	18, 18, 19, 20, 21, 22, 23, 24, 24, 25
};

static const struct rtw_pwr_track_tbl rtw8822c_pwr_track_type0_tbl = {
	.pwrtrk_5gb_n[RTW_PWR_TRK_5G_1] = rtw8822c_pwrtrk_5gb_n[RTW_PWR_TRK_5G_1],
	.pwrtrk_5gb_n[RTW_PWR_TRK_5G_2] = rtw8822c_pwrtrk_5gb_n[RTW_PWR_TRK_5G_2],
	.pwrtrk_5gb_n[RTW_PWR_TRK_5G_3] = rtw8822c_pwrtrk_5gb_n[RTW_PWR_TRK_5G_3],
	.pwrtrk_5gb_p[RTW_PWR_TRK_5G_1] = rtw8822c_pwrtrk_5gb_p[RTW_PWR_TRK_5G_1],
	.pwrtrk_5gb_p[RTW_PWR_TRK_5G_2] = rtw8822c_pwrtrk_5gb_p[RTW_PWR_TRK_5G_2],
	.pwrtrk_5gb_p[RTW_PWR_TRK_5G_3] = rtw8822c_pwrtrk_5gb_p[RTW_PWR_TRK_5G_3],
	.pwrtrk_5ga_n[RTW_PWR_TRK_5G_1] = rtw8822c_pwrtrk_5ga_n[RTW_PWR_TRK_5G_1],
	.pwrtrk_5ga_n[RTW_PWR_TRK_5G_2] = rtw8822c_pwrtrk_5ga_n[RTW_PWR_TRK_5G_2],
	.pwrtrk_5ga_n[RTW_PWR_TRK_5G_3] = rtw8822c_pwrtrk_5ga_n[RTW_PWR_TRK_5G_3],
	.pwrtrk_5ga_p[RTW_PWR_TRK_5G_1] = rtw8822c_pwrtrk_5ga_p[RTW_PWR_TRK_5G_1],
	.pwrtrk_5ga_p[RTW_PWR_TRK_5G_2] = rtw8822c_pwrtrk_5ga_p[RTW_PWR_TRK_5G_2],
	.pwrtrk_5ga_p[RTW_PWR_TRK_5G_3] = rtw8822c_pwrtrk_5ga_p[RTW_PWR_TRK_5G_3],
	.pwrtrk_2gb_n = rtw8822c_pwrtrk_2gb_n,
	.pwrtrk_2gb_p = rtw8822c_pwrtrk_2gb_p,
	.pwrtrk_2ga_n = rtw8822c_pwrtrk_2ga_n,
	.pwrtrk_2ga_p = rtw8822c_pwrtrk_2ga_p,
	.pwrtrk_2g_cckb_n = rtw8822c_pwrtrk_2g_cck_b_n,
	.pwrtrk_2g_cckb_p = rtw8822c_pwrtrk_2g_cck_b_p,
	.pwrtrk_2g_ccka_n = rtw8822c_pwrtrk_2g_cck_a_n,
	.pwrtrk_2g_ccka_p = rtw8822c_pwrtrk_2g_cck_a_p,
};

static const struct rtw_rfe_def rtw8822c_rfe_defs[] = {
	[0] = RTW_DEF_RFE(8822c, 0, 0, 0),
	[1] = RTW_DEF_RFE(8822c, 0, 0, 0),
	[2] = RTW_DEF_RFE(8822c, 0, 0, 0),
	[3] = RTW_DEF_RFE(8822c, 0, 0, 0),
	[4] = RTW_DEF_RFE(8822c, 0, 0, 0),
	[5] = RTW_DEF_RFE(8822c, 0, 5, 0),
	[6] = RTW_DEF_RFE(8822c, 0, 0, 0),
};

static bool __rtw8822c_dac_cal_restore_restore_impl(struct rtw_dev *rtwdev)
{
	if (!rtw8822c_dac_cal_restore_path(rtwdev, RF_PATH_A))
		return false;

	if (!rtw8822c_dac_cal_restore_path(rtwdev, RF_PATH_B))
		return false;

	return true;
}
static bool __rtw8822c_dac_cal_restore(struct rtw_dev *rtwdev)
{++rtwdev->restore_depth;__typeof__(__rtw8822c_dac_cal_restore_restore_impl(rtwdev)) native_result=__rtw8822c_dac_cal_restore_restore_impl(rtwdev);--rtwdev->restore_depth;return native_result;}

static void __rtw8822c_dac_iq_sort(struct rtw_dev *rtwdev, u32 *v1, u32 *v2)
{
	if (*v1 >= 0x200 && *v2 >= 0x200) {
		if (*v1 > *v2)
			swap(*v1, *v2);
	} else if (*v1 < 0x200 && *v2 < 0x200) {
		if (*v1 > *v2)
			swap(*v1, *v2);
	} else if (*v1 < 0x200 && *v2 >= 0x200) {
		swap(*v1, *v2);
	}
}

static void __rtw8822c_pwr_track(struct rtw_dev *rtwdev)
{
	struct rtw_swing_table swing_table;
	u8 i;

	rtw_phy_config_swing_table(rtwdev, &swing_table);

	for (i = 0; i < rtwdev->hal.rf_path_num; i++)
		rtw8822c_pwr_track_stats(rtwdev, i);
	if (rtw_phy_pwrtrack_need_lck(rtwdev))
		rtw8822c_do_lck(rtwdev);
	for (i = 0; i < rtwdev->hal.rf_path_num; i++)
		rtw8822c_pwr_track_path(rtwdev, &swing_table, i);
}

static bool _rtw8822c_txgapk_gain_valid(struct rtw_dev *rtwdev, u32 gain)
{
	if ((FIELD_GET(BIT_GAIN_TX_PAD_H, gain) >= 0xc) &&
	    (FIELD_GET(BIT_GAIN_TX_PAD_L, gain) >= 0xe))
		return true;

	return false;
}

static void _rtw8822c_txgapk_write_gain_bb_table(struct rtw_dev *rtwdev,
						 u8 band, u8 path)
{
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	u32 v, tmp_3f = 0;
	u8 gain, check_txgain;

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, path);

	switch (band) {
	case RF_BAND_2G_OFDM:
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x0);
		break;
	case RF_BAND_5G_L:
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x2);
		break;
	case RF_BAND_5G_M:
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x3);
		break;
	case RF_BAND_5G_H:
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x4);
		break;
	default:
		break;
	}

	rtw_write32_mask(rtwdev, REG_TX_GAIN_SET, MASKBYTE0, 0x88);

	check_txgain = 0;
	for (gain = 0; gain < RF_GAIN_NUM; gain++) {
		v = txgapk->rf3f_bp[band][gain][path];
		if (_rtw8822c_txgapk_gain_valid(rtwdev, v)) {
			if (!check_txgain) {
				tmp_3f = txgapk->rf3f_bp[band][gain][path];
				check_txgain = 1;
			}
			rtw_dbg(rtwdev, RTW_DBG_RFK,
				"[TXGAPK] tx_gain=0x%03X >= 0xCEX\n",
				txgapk->rf3f_bp[band][gain][path]);
		} else {
			tmp_3f = txgapk->rf3f_bp[band][gain][path];
		}

		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_Q_GAIN, tmp_3f);
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_I_GAIN, gain);
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_GAIN_RST, 0x1);
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_GAIN_RST, 0x0);

		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[TXGAPK] Band=%d 0x1b98[11:0]=0x%03X path=%d\n",
			band, tmp_3f, path);
	}
}

static bool check_rf_status(struct rtw_dev *rtwdev, u8 status)
{
	u8 reg_rf0_a, reg_rf0_b;

	reg_rf0_a = (u8)rtw_read_rf(rtwdev, RF_PATH_A,
				    RF_MODE_TRXAGC, BIT_RF_MODE);
	reg_rf0_b = (u8)rtw_read_rf(rtwdev, RF_PATH_B,
				    RF_MODE_TRXAGC, BIT_RF_MODE);

	if (reg_rf0_a == status || reg_rf0_b == status)
		return false;

	return true;
}

static void rtw8822c_bb_reset(struct rtw_dev *rtwdev)
{
	rtw_write16_set(rtwdev, REG_SYS_FUNC_EN, BIT_FEN_BB_RSTB);
	rtw_write16_clr(rtwdev, REG_SYS_FUNC_EN, BIT_FEN_BB_RSTB);
	rtw_write16_set(rtwdev, REG_SYS_FUNC_EN, BIT_FEN_BB_RSTB);
}

static void rtw8822c_config_cck_rx_path(struct rtw_dev *rtwdev, u8 rx_path)
{
	if (rx_path == BB_PATH_A || rx_path == BB_PATH_B) {
		rtw_write32_mask(rtwdev, REG_CCANRX, 0x00060000, 0x0);
		rtw_write32_mask(rtwdev, REG_CCANRX, 0x00600000, 0x0);
	} else if (rx_path == BB_PATH_AB) {
		rtw_write32_mask(rtwdev, REG_CCANRX, 0x00600000, 0x1);
		rtw_write32_mask(rtwdev, REG_CCANRX, 0x00060000, 0x1);
	}

	if (rx_path == BB_PATH_A)
		rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0x0f000000, 0x0);
	else if (rx_path == BB_PATH_B)
		rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0x0f000000, 0x5);
	else if (rx_path == BB_PATH_AB)
		rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0x0f000000, 0x1);
}

static void rtw8822c_config_cck_tx_path(struct rtw_dev *rtwdev, u8 tx_path,
					bool is_tx2_path)
{
	if (tx_path == BB_PATH_A) {
		rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0xf0000000, 0x8);
	} else if (tx_path == BB_PATH_B) {
		rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0xf0000000, 0x4);
	} else {
		if (is_tx2_path)
			rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0xf0000000, 0xc);
		else
			rtw_write32_mask(rtwdev, REG_RXCCKSEL, 0xf0000000, 0x8);
	}
	rtw8822c_bb_reset(rtwdev);
}

static void rtw8822c_config_ofdm_rx_path(struct rtw_dev *rtwdev, u8 rx_path)
{
	if (rx_path == BB_PATH_A || rx_path == BB_PATH_B) {
		rtw_write32_mask(rtwdev, REG_RXFNCTL, 0x300, 0x0);
		rtw_write32_mask(rtwdev, REG_RXFNCTL, 0x600000, 0x0);
		rtw_write32_mask(rtwdev, REG_AGCSWSH, BIT(17), 0x0);
		rtw_write32_mask(rtwdev, REG_ANTWTPD, BIT(20), 0x0);
		rtw_write32_mask(rtwdev, REG_MRCM, BIT(24), 0x0);
	} else if (rx_path == BB_PATH_AB) {
		rtw_write32_mask(rtwdev, REG_RXFNCTL, 0x300, 0x1);
		rtw_write32_mask(rtwdev, REG_RXFNCTL, 0x600000, 0x1);
		rtw_write32_mask(rtwdev, REG_AGCSWSH, BIT(17), 0x1);
		rtw_write32_mask(rtwdev, REG_ANTWTPD, BIT(20), 0x1);
		rtw_write32_mask(rtwdev, REG_MRCM, BIT(24), 0x1);
	}

	rtw_write32_mask(rtwdev, 0x824, 0x0f000000, rx_path);
	rtw_write32_mask(rtwdev, 0x824, 0x000f0000, rx_path);
}

static void rtw8822c_config_ofdm_tx_path(struct rtw_dev *rtwdev, u8 tx_path,
					 enum rtw_bb_path tx_path_sel_1ss)
{
	if (tx_path == BB_PATH_A) {
		rtw_write32_mask(rtwdev, REG_ANTMAP0, 0xff, 0x11);
		rtw_write32_mask(rtwdev, REG_TXLGMAP, 0xff, 0x0);
	} else if (tx_path == BB_PATH_B) {
		rtw_write32_mask(rtwdev, REG_ANTMAP0, 0xff, 0x12);
		rtw_write32_mask(rtwdev, REG_TXLGMAP, 0xff, 0x0);
	} else {
		if (tx_path_sel_1ss == BB_PATH_AB) {
			rtw_write32_mask(rtwdev, REG_ANTMAP0, 0xff, 0x33);
			rtw_write32_mask(rtwdev, REG_TXLGMAP, 0xffff, 0x0404);
		} else if (tx_path_sel_1ss == BB_PATH_B) {
			rtw_write32_mask(rtwdev, REG_ANTMAP0, 0xff, 0x32);
			rtw_write32_mask(rtwdev, REG_TXLGMAP, 0xffff, 0x0400);
		} else if (tx_path_sel_1ss == BB_PATH_A) {
			rtw_write32_mask(rtwdev, REG_ANTMAP0, 0xff, 0x31);
			rtw_write32_mask(rtwdev, REG_TXLGMAP, 0xffff, 0x0400);
		}
	}
	rtw8822c_bb_reset(rtwdev);
}

static void rtw8822c_config_rx_path(struct rtw_dev *rtwdev, u8 rx_path)
{
	rtw8822c_config_cck_rx_path(rtwdev, rx_path);
	rtw8822c_config_ofdm_rx_path(rtwdev, rx_path);
}

static void rtw8822c_config_trx_mode(struct rtw_dev *rtwdev, u8 tx_path,
				     u8 rx_path, bool is_tx2_path)
{
	if ((tx_path | rx_path) & BB_PATH_A)
		rtw_write32_mask(rtwdev, REG_ORITXCODE, MASK20BITS, 0x33312);
	else
		rtw_write32_mask(rtwdev, REG_ORITXCODE, MASK20BITS, 0x11111);
	if ((tx_path | rx_path) & BB_PATH_B)
		rtw_write32_mask(rtwdev, REG_ORITXCODE2, MASK20BITS, 0x33312);
	else
		rtw_write32_mask(rtwdev, REG_ORITXCODE2, MASK20BITS, 0x11111);

	rtw8822c_config_rx_path(rtwdev, rx_path);
	rtw8822c_config_tx_path(rtwdev, tx_path, BB_PATH_A, BB_PATH_A,
				is_tx2_path);

	rtw8822c_toggle_igi(rtwdev);
}

static void rtw8822c_config_tx_path(struct rtw_dev *rtwdev, u8 tx_path,
				    enum rtw_bb_path tx_path_sel_1ss,
				    enum rtw_bb_path tx_path_cck,
				    bool is_tx2_path)
{
	rtw8822c_config_cck_tx_path(rtwdev, tx_path_cck, is_tx2_path);
	rtw8822c_config_ofdm_tx_path(rtwdev, tx_path, tx_path_sel_1ss);
	rtw8822c_bb_reset(rtwdev);
}

static void rtw8822c_dac_backup_reg(struct rtw_dev *rtwdev,
				    struct rtw_backup_info *backup,
				    struct rtw_backup_info *backup_rf)
{
	u32 path, i;
	u32 val;
	u32 reg;
	u32 rf_addr[DACK_RF_8822C] = {0x8f};
	u32 addrs[DACK_REG_8822C] = {0x180c, 0x1810, 0x410c, 0x4110,
				     0x1c3c, 0x1c24, 0x1d70, 0x9b4,
				     0x1a00, 0x1a14, 0x1d58, 0x1c38,
				     0x1e24, 0x1e28, 0x1860, 0x4160};

	for (i = 0; i < DACK_REG_8822C; i++) {
		backup[i].len = 4;
		backup[i].reg = addrs[i];
		backup[i].val = rtw_read32(rtwdev, addrs[i]);
	}

	for (path = 0; path < DACK_PATH_8822C; path++) {
		for (i = 0; i < DACK_RF_8822C; i++) {
			reg = rf_addr[i];
			val = rtw_read_rf(rtwdev, path, reg, RFREG_MASK);
			backup_rf[path * DACK_RF_8822C + i].reg = reg;
			backup_rf[path * DACK_RF_8822C + i].val = val;
		}
	}
}

static void rtw8822c_dac_bb_setting(struct rtw_dev *rtwdev)
{
	rtw_write32_mask(rtwdev, 0x1d58, 0xff8, 0x1ff);
	rtw_write32_mask(rtwdev, 0x1a00, 0x3, 0x2);
	rtw_write32_mask(rtwdev, 0x1a14, 0x300, 0x3);
	rtw_write32(rtwdev, 0x1d70, 0x7e7e7e7e);
	rtw_write32_mask(rtwdev, 0x180c, 0x3, 0x0);
	rtw_write32_mask(rtwdev, 0x410c, 0x3, 0x0);
	rtw_write32(rtwdev, 0x1b00, 0x00000008);
	rtw_write8(rtwdev, 0x1bcc, 0x3f);
	rtw_write32(rtwdev, 0x1b00, 0x0000000a);
	rtw_write8(rtwdev, 0x1bcc, 0x3f);
	rtw_write32_mask(rtwdev, 0x1e24, BIT(31), 0x0);
	rtw_write32_mask(rtwdev, 0x1e28, 0xf, 0x3);
}

static void rtw8822c_dac_cal_adc(struct rtw_dev *rtwdev,
				 u8 path, u32 *adc_ic, u32 *adc_qc)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 ic = 0, qc = 0, temp = 0;
	u32 base_addr;
	u32 path_sel;
	int i;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] ADCK path(%d)\n", path);

	base_addr = rtw8822c_get_path_write_addr(path);
	switch (path) {
	case RF_PATH_A:
		path_sel = 0xa0000;
		break;
	case RF_PATH_B:
		path_sel = 0x80000;
		break;
	default:
		WARN_ON(1);
		return;
	}

	/* ADCK step1 */
	rtw_write32_mask(rtwdev, base_addr + 0x30, BIT(30), 0x0);
	if (path == RF_PATH_B)
		rtw_write32(rtwdev, base_addr + 0x30, 0x30db8041);
	rtw_write32(rtwdev, base_addr + 0x60, 0xf0040ff0);
	rtw_write32(rtwdev, base_addr + 0x0c, 0xdff00220);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02dd08c4);
	rtw_write32(rtwdev, base_addr + 0x0c, 0x10000260);
	rtw_write_rf(rtwdev, RF_PATH_A, 0x0, RFREG_MASK, 0x10000);
	rtw_write_rf(rtwdev, RF_PATH_B, 0x0, RFREG_MASK, 0x10000);
	for (i = 0; i < 10; i++) {
		rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] ADCK count=%d\n", i);
		rtw_write32(rtwdev, 0x1c3c, path_sel + 0x8003);
		rtw_write32(rtwdev, 0x1c24, 0x00010002);
		rtw8822c_dac_cal_rf_mode(rtwdev, &ic, &qc);
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[DACK] before: i=0x%x, q=0x%x\n", ic, qc);

		/* compensation value */
		if (ic != 0x0) {
			ic = 0x400 - ic;
			*adc_ic = ic;
		}
		if (qc != 0x0) {
			qc = 0x400 - qc;
			*adc_qc = qc;
		}
		temp = (ic & 0x3ff) | ((qc & 0x3ff) << 10);
		rtw_write32(rtwdev, base_addr + 0x68, temp);
		dm_info->dack_adck[path] = temp;
		rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] ADCK 0x%08x=0x08%x\n",
			base_addr + 0x68, temp);
		/* check ADC DC offset */
		rtw_write32(rtwdev, 0x1c3c, path_sel + 0x8103);
		rtw8822c_dac_cal_rf_mode(rtwdev, &ic, &qc);
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[DACK] after:  i=0x%08x, q=0x%08x\n", ic, qc);
		if (ic >= 0x200)
			ic = 0x400 - ic;
		if (qc >= 0x200)
			qc = 0x400 - qc;
		if (ic < 5 && qc < 5)
			break;
	}

	if(ic>=5||qc>=5)native_fail(rtwdev,EIO);
	/* ADCK step2 */
	rtw_write32(rtwdev, 0x1c3c, 0x00000003);
	rtw_write32(rtwdev, base_addr + 0x0c, 0x10000260);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c4);

	/* release pull low switch on IQ path */
	rtw_write_rf(rtwdev, path, 0x8f, BIT(13), 0x1);
}

static void rtw8822c_dac_cal_backup(struct rtw_dev *rtwdev)
{
	u32 temp[3];

	temp[0] = rtw_read32(rtwdev, 0x1860);
	temp[1] = rtw_read32(rtwdev, 0x4160);
	temp[2] = rtw_read32(rtwdev, 0x9b4);

	/* set clock */
	rtw_write32(rtwdev, 0x9b4, 0xdb66db00);

	/* backup path-A I/Q */
	rtw_write32_clr(rtwdev, 0x1830, BIT(30));
	rtw_write32_mask(rtwdev, 0x1860, 0xfc000000, 0x3c);
	rtw8822c_dac_cal_backup_path(rtwdev, RF_PATH_A);

	/* backup path-B I/Q */
	rtw_write32_clr(rtwdev, 0x4130, BIT(30));
	rtw_write32_mask(rtwdev, 0x4160, 0xfc000000, 0x3c);
	rtw8822c_dac_cal_backup_path(rtwdev, RF_PATH_B);

	rtw8822c_dac_cal_backup_dck(rtwdev);
	rtw_write32_set(rtwdev, 0x1830, BIT(30));
	rtw_write32_set(rtwdev, 0x4130, BIT(30));

	rtw_write32(rtwdev, 0x1860, temp[0]);
	rtw_write32(rtwdev, 0x4160, temp[1]);
	rtw_write32(rtwdev, 0x9b4, temp[2]);
}

static void rtw8822c_dac_cal_backup_dck(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 val;

	val = (u8)rtw_read32_mask(rtwdev, REG_DCKA_I_0, 0xf0000000);
	dm_info->dack_dck[RF_PATH_A][0][0] = val;
	val = (u8)rtw_read32_mask(rtwdev, REG_DCKA_I_1, 0xf);
	dm_info->dack_dck[RF_PATH_A][0][1] = val;
	val = (u8)rtw_read32_mask(rtwdev, REG_DCKA_Q_0, 0xf0000000);
	dm_info->dack_dck[RF_PATH_A][1][0] = val;
	val = (u8)rtw_read32_mask(rtwdev, REG_DCKA_Q_1, 0xf);
	dm_info->dack_dck[RF_PATH_A][1][1] = val;

	val = (u8)rtw_read32_mask(rtwdev, REG_DCKB_I_0, 0xf0000000);
	dm_info->dack_dck[RF_PATH_B][0][0] = val;
	val = (u8)rtw_read32_mask(rtwdev, REG_DCKB_I_1, 0xf);
	dm_info->dack_dck[RF_PATH_B][1][0] = val;
	val = (u8)rtw_read32_mask(rtwdev, REG_DCKB_Q_0, 0xf0000000);
	dm_info->dack_dck[RF_PATH_B][0][1] = val;
	val = (u8)rtw_read32_mask(rtwdev, REG_DCKB_Q_1, 0xf);
	dm_info->dack_dck[RF_PATH_B][1][1] = val;
}

static void rtw8822c_dac_cal_backup_path(struct rtw_dev *rtwdev, u8 path)
{
	u32 w_off = 0x1c;
	u32 r_off = 0x2c;
	u32 w_addr, r_addr;

	if (WARN_ON(path >= 2))
		return;

	/* backup I vector */
	w_addr = rtw8822c_get_path_write_addr(path) + 0xb0;
	r_addr = rtw8822c_get_path_read_addr(path) + 0x10;
	rtw8822c_dac_cal_backup_vec(rtwdev, path, 0, w_addr, r_addr);

	/* backup Q vector */
	w_addr = rtw8822c_get_path_write_addr(path) + 0xb0 + w_off;
	r_addr = rtw8822c_get_path_read_addr(path) + 0x10 + r_off;
	rtw8822c_dac_cal_backup_vec(rtwdev, path, 1, w_addr, r_addr);
}

static void rtw8822c_dac_cal_backup_vec(struct rtw_dev *rtwdev,
					u8 path, u8 vec, u32 w_addr, u32 r_addr)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u16 val;
	u32 i;

	if (WARN_ON(vec >= 2))
		return;

	for (i = 0; i < DACK_MSBK_BACKUP_NUM; i++) {
		rtw_write32_mask(rtwdev, w_addr, 0xf0000000, i);
		val = (u16)rtw_read32_mask(rtwdev, r_addr, 0x7fc0000);
		dm_info->dack_msbk[path][vec][i] = val;
	}
}

static void rtw8822c_dac_cal_iq_sample(struct rtw_dev *rtwdev, u32 *iv, u32 *qv)
{
	u32 temp;
	memset(iv,0,DACK_SN_8822C*sizeof(*iv));memset(qv,0,DACK_SN_8822C*sizeof(*qv));
	int i = 0, cnt = 0;

	while (i < DACK_SN_8822C && cnt < 10000) {
		cnt++;
		temp = rtw_read32_mask(rtwdev, 0x2dbc, 0x3fffff);
		iv[i] = (temp & 0x3ff000) >> 12;
		qv[i] = temp & 0x3ff;

		if (rtw8822c_dac_iq_check(rtwdev, iv[i]) &&
		    rtw8822c_dac_iq_check(rtwdev, qv[i]))
			i++;
	}

 if(i<DACK_SN_8822C)native_fail(rtwdev,EIO);
}

static void rtw8822c_dac_cal_iq_search(struct rtw_dev *rtwdev,
				       u32 *iv, u32 *qv,
				       u32 *i_value, u32 *q_value)
{
	u32 i_max = 0, q_max = 0, i_min = 0, q_min = 0;
	u32 i_delta, q_delta;
	u32 temp;
	int i, cnt = 0;

	do {
		i_min = iv[0];
		i_max = iv[0];
		q_min = qv[0];
		q_max = qv[0];
		for (i = 0; i < DACK_SN_8822C; i++) {
			rtw8822c_rf_minmax_cmp(rtwdev, iv[i], &i_min, &i_max);
			rtw8822c_rf_minmax_cmp(rtwdev, qv[i], &q_min, &q_max);
		}

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

		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[DACK] i: min=0x%08x, max=0x%08x, delta=0x%08x\n",
			i_min, i_max, i_delta);
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[DACK] q: min=0x%08x, max=0x%08x, delta=0x%08x\n",
			q_min, q_max, q_delta);

		rtw8822c_dac_iq_sort(rtwdev, iv, qv);

		if (i_delta > 5 || q_delta > 5) {
			temp = rtw_read32_mask(rtwdev, 0x2dbc, 0x3fffff);
			iv[0] = (temp & 0x3ff000) >> 12;
			qv[0] = temp & 0x3ff;
			temp = rtw_read32_mask(rtwdev, 0x2dbc, 0x3fffff);
			iv[DACK_SN_8822C - 1] = (temp & 0x3ff000) >> 12;
			qv[DACK_SN_8822C - 1] = temp & 0x3ff;
		} else {
			break;
		}
	} while (cnt++ < 100);

	if(i_delta>5||q_delta>5)native_fail(rtwdev,EIO);
	rtw8822c_dac_iq_offset(rtwdev, iv, i_value);
	rtw8822c_dac_iq_offset(rtwdev, qv, q_value);
}

static bool rtw8822c_dac_cal_restore_restore_impl(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 temp[3];

	/* sample the first element for both path's IQ vector */
	if (dm_info->dack_msbk[RF_PATH_A][0][0] == 0 &&
	    dm_info->dack_msbk[RF_PATH_A][1][0] == 0 &&
	    dm_info->dack_msbk[RF_PATH_B][0][0] == 0 &&
	    dm_info->dack_msbk[RF_PATH_B][1][0] == 0)
		return false;

	temp[0] = rtw_read32(rtwdev, 0x1860);
	temp[1] = rtw_read32(rtwdev, 0x4160);
	temp[2] = rtw_read32(rtwdev, 0x9b4);

	rtw8822c_dac_cal_restore_prepare(rtwdev);
	if (!check_hw_ready(rtwdev, 0x2808, 0x7fff80, 0xffff) ||
	    !check_hw_ready(rtwdev, 0x2834, 0x7fff80, 0xffff) ||
	    !check_hw_ready(rtwdev, 0x4508, 0x7fff80, 0xffff) ||
	    !check_hw_ready(rtwdev, 0x4534, 0x7fff80, 0xffff))
		return false;

	if (!__rtw8822c_dac_cal_restore(rtwdev)) {
		rtw_err(rtwdev, "failed to restore dack vectors\n");
		return false;
	}

	rtw_write32_mask(rtwdev, 0x1830, BIT(30), 0x1);
	rtw_write32_mask(rtwdev, 0x4130, BIT(30), 0x1);
	rtw_write32(rtwdev, 0x1860, temp[0]);
	rtw_write32(rtwdev, 0x4160, temp[1]);
	rtw_write32_mask(rtwdev, 0x18b0, BIT(27), 0x1);
	rtw_write32_mask(rtwdev, 0x18cc, BIT(27), 0x1);
	rtw_write32_mask(rtwdev, 0x41b0, BIT(27), 0x1);
	rtw_write32_mask(rtwdev, 0x41cc, BIT(27), 0x1);
	rtw_write32(rtwdev, 0x9b4, temp[2]);

	return true;
}
static bool rtw8822c_dac_cal_restore(struct rtw_dev *rtwdev)
{++rtwdev->restore_depth;__typeof__(rtw8822c_dac_cal_restore_restore_impl(rtwdev)) native_result=rtw8822c_dac_cal_restore_restore_impl(rtwdev);--rtwdev->restore_depth;return native_result;}

static void rtw8822c_dac_cal_restore_dck_restore_impl(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 val;

	rtw_write32_set(rtwdev, REG_DCKA_I_0, BIT(19));
	val = dm_info->dack_dck[RF_PATH_A][0][0];
	rtw_write32_mask(rtwdev, REG_DCKA_I_0, 0xf0000000, val);
	val = dm_info->dack_dck[RF_PATH_A][0][1];
	rtw_write32_mask(rtwdev, REG_DCKA_I_1, 0xf, val);

	rtw_write32_set(rtwdev, REG_DCKA_Q_0, BIT(19));
	val = dm_info->dack_dck[RF_PATH_A][1][0];
	rtw_write32_mask(rtwdev, REG_DCKA_Q_0, 0xf0000000, val);
	val = dm_info->dack_dck[RF_PATH_A][1][1];
	rtw_write32_mask(rtwdev, REG_DCKA_Q_1, 0xf, val);

	rtw_write32_set(rtwdev, REG_DCKB_I_0, BIT(19));
	val = dm_info->dack_dck[RF_PATH_B][0][0];
	rtw_write32_mask(rtwdev, REG_DCKB_I_0, 0xf0000000, val);
	val = dm_info->dack_dck[RF_PATH_B][0][1];
	rtw_write32_mask(rtwdev, REG_DCKB_I_1, 0xf, val);

	rtw_write32_set(rtwdev, REG_DCKB_Q_0, BIT(19));
	val = dm_info->dack_dck[RF_PATH_B][1][0];
	rtw_write32_mask(rtwdev, REG_DCKB_Q_0, 0xf0000000, val);
	val = dm_info->dack_dck[RF_PATH_B][1][1];
	rtw_write32_mask(rtwdev, REG_DCKB_Q_1, 0xf, val);
}
static void rtw8822c_dac_cal_restore_dck(struct rtw_dev *rtwdev)
{++rtwdev->restore_depth;rtw8822c_dac_cal_restore_dck_restore_impl(rtwdev);--rtwdev->restore_depth;}

static bool rtw8822c_dac_cal_restore_path_restore_impl(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 w_off = 0x1c;
	u32 r_off = 0x2c;
	u32 w_i, r_i, w_q, r_q;
	u32 value;
	u32 i;

	w_i = rtw8822c_get_path_write_addr(path) + 0xb0;
	r_i = rtw8822c_get_path_read_addr(path) + 0x08;
	w_q = rtw8822c_get_path_write_addr(path) + 0xb0 + w_off;
	r_q = rtw8822c_get_path_read_addr(path) + 0x08 + r_off;

	if (!rtw8822c_dac_cal_restore_wait(rtwdev, r_i, w_i + 0x8))
		return false;

	for (i = 0; i < DACK_MSBK_BACKUP_NUM; i++) {
		rtw_write32_mask(rtwdev, w_i + 0x4, BIT(2), 0x0);
		value = dm_info->dack_msbk[path][0][i];
		rtw_write32_mask(rtwdev, w_i + 0x4, 0xff8, value);
		rtw_write32_mask(rtwdev, w_i, 0xf0000000, i);
		rtw_write32_mask(rtwdev, w_i + 0x4, BIT(2), 0x1);
	}

	rtw_write32_mask(rtwdev, w_i + 0x4, BIT(2), 0x0);

	if (!rtw8822c_dac_cal_restore_wait(rtwdev, r_q, w_q + 0x8))
		return false;

	for (i = 0; i < DACK_MSBK_BACKUP_NUM; i++) {
		rtw_write32_mask(rtwdev, w_q + 0x4, BIT(2), 0x0);
		value = dm_info->dack_msbk[path][1][i];
		rtw_write32_mask(rtwdev, w_q + 0x4, 0xff8, value);
		rtw_write32_mask(rtwdev, w_q, 0xf0000000, i);
		rtw_write32_mask(rtwdev, w_q + 0x4, BIT(2), 0x1);
	}
	rtw_write32_mask(rtwdev, w_q + 0x4, BIT(2), 0x0);

	rtw_write32_mask(rtwdev, w_i + 0x8, BIT(26) | BIT(25), 0x0);
	rtw_write32_mask(rtwdev, w_q + 0x8, BIT(26) | BIT(25), 0x0);
	rtw_write32_mask(rtwdev, w_i + 0x4, BIT(0), 0x0);
	rtw_write32_mask(rtwdev, w_q + 0x4, BIT(0), 0x0);

	return true;
}
static bool rtw8822c_dac_cal_restore_path(struct rtw_dev *rtwdev, u8 path)
{++rtwdev->restore_depth;__typeof__(rtw8822c_dac_cal_restore_path_restore_impl(rtwdev,path)) native_result=rtw8822c_dac_cal_restore_path_restore_impl(rtwdev,path);--rtwdev->restore_depth;return native_result;}

static void rtw8822c_dac_cal_restore_prepare_restore_impl(struct rtw_dev *rtwdev)
{
	rtw_write32(rtwdev, 0x9b4, 0xdb66db00);

	rtw_write32_mask(rtwdev, 0x18b0, BIT(27), 0x0);
	rtw_write32_mask(rtwdev, 0x18cc, BIT(27), 0x0);
	rtw_write32_mask(rtwdev, 0x41b0, BIT(27), 0x0);
	rtw_write32_mask(rtwdev, 0x41cc, BIT(27), 0x0);

	rtw_write32_mask(rtwdev, 0x1830, BIT(30), 0x0);
	rtw_write32_mask(rtwdev, 0x1860, 0xfc000000, 0x3c);
	rtw_write32_mask(rtwdev, 0x18b4, BIT(0), 0x1);
	rtw_write32_mask(rtwdev, 0x18d0, BIT(0), 0x1);

	rtw_write32_mask(rtwdev, 0x4130, BIT(30), 0x0);
	rtw_write32_mask(rtwdev, 0x4160, 0xfc000000, 0x3c);
	rtw_write32_mask(rtwdev, 0x41b4, BIT(0), 0x1);
	rtw_write32_mask(rtwdev, 0x41d0, BIT(0), 0x1);

	rtw_write32_mask(rtwdev, 0x18b0, 0xf00, 0x0);
	rtw_write32_mask(rtwdev, 0x18c0, BIT(14), 0x0);
	rtw_write32_mask(rtwdev, 0x18cc, 0xf00, 0x0);
	rtw_write32_mask(rtwdev, 0x18dc, BIT(14), 0x0);

	rtw_write32_mask(rtwdev, 0x18b0, BIT(0), 0x0);
	rtw_write32_mask(rtwdev, 0x18cc, BIT(0), 0x0);
	rtw_write32_mask(rtwdev, 0x18b0, BIT(0), 0x1);
	rtw_write32_mask(rtwdev, 0x18cc, BIT(0), 0x1);

	rtw8822c_dac_cal_restore_dck(rtwdev);

	rtw_write32_mask(rtwdev, 0x18c0, 0x38000, 0x7);
	rtw_write32_mask(rtwdev, 0x18dc, 0x38000, 0x7);
	rtw_write32_mask(rtwdev, 0x41c0, 0x38000, 0x7);
	rtw_write32_mask(rtwdev, 0x41dc, 0x38000, 0x7);

	rtw_write32_mask(rtwdev, 0x18b8, BIT(26) | BIT(25), 0x1);
	rtw_write32_mask(rtwdev, 0x18d4, BIT(26) | BIT(25), 0x1);

	rtw_write32_mask(rtwdev, 0x41b0, 0xf00, 0x0);
	rtw_write32_mask(rtwdev, 0x41c0, BIT(14), 0x0);
	rtw_write32_mask(rtwdev, 0x41cc, 0xf00, 0x0);
	rtw_write32_mask(rtwdev, 0x41dc, BIT(14), 0x0);

	rtw_write32_mask(rtwdev, 0x41b0, BIT(0), 0x0);
	rtw_write32_mask(rtwdev, 0x41cc, BIT(0), 0x0);
	rtw_write32_mask(rtwdev, 0x41b0, BIT(0), 0x1);
	rtw_write32_mask(rtwdev, 0x41cc, BIT(0), 0x1);

	rtw_write32_mask(rtwdev, 0x41b8, BIT(26) | BIT(25), 0x1);
	rtw_write32_mask(rtwdev, 0x41d4, BIT(26) | BIT(25), 0x1);
}
static void rtw8822c_dac_cal_restore_prepare(struct rtw_dev *rtwdev)
{++rtwdev->restore_depth;rtw8822c_dac_cal_restore_prepare_restore_impl(rtwdev);--rtwdev->restore_depth;}

static bool rtw8822c_dac_cal_restore_wait_restore_impl(struct rtw_dev *rtwdev,
					  u32 target_addr, u32 toggle_addr)
{
	u32 cnt = 0;

	do {
		rtw_write32_mask(rtwdev, toggle_addr, BIT(26) | BIT(25), 0x0);
		rtw_write32_mask(rtwdev, toggle_addr, BIT(26) | BIT(25), 0x2);

		if (rtw_read32_mask(rtwdev, target_addr, 0xf) == 0x6)
			return true;

	} while (cnt++ < 100);

	return false;
}
static bool rtw8822c_dac_cal_restore_wait(struct rtw_dev *rtwdev,
					  u32 target_addr, u32 toggle_addr)
{++rtwdev->restore_depth;__typeof__(rtw8822c_dac_cal_restore_wait_restore_impl(rtwdev,target_addr,toggle_addr)) native_result=rtw8822c_dac_cal_restore_wait_restore_impl(rtwdev,target_addr,toggle_addr);--rtwdev->restore_depth;return native_result;}

static void rtw8822c_dac_cal_rf_mode(struct rtw_dev *rtwdev,
				     u32 *i_value, u32 *q_value)
{
	u32 iv[DACK_SN_8822C], qv[DACK_SN_8822C];
	u32 rf_a, rf_b;

	rf_a = rtw_read_rf(rtwdev, RF_PATH_A, 0x0, RFREG_MASK);
	rf_b = rtw_read_rf(rtwdev, RF_PATH_B, 0x0, RFREG_MASK);

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] RF path-A=0x%05x\n", rf_a);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] RF path-B=0x%05x\n", rf_b);

	rtw8822c_dac_cal_iq_sample(rtwdev, iv, qv);
	rtw8822c_dac_cal_iq_search(rtwdev, iv, qv, i_value, q_value);
}

static void rtw8822c_dac_cal_step1(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 base_addr;
	u32 read_addr;

	base_addr = rtw8822c_get_path_write_addr(path);
	read_addr = rtw8822c_get_path_read_addr(path);

	rtw_write32(rtwdev, base_addr + 0x68, dm_info->dack_adck[path]);
	rtw_write32(rtwdev, base_addr + 0x0c, 0xdff00220);
	if (path == RF_PATH_A) {
		rtw_write32(rtwdev, base_addr + 0x60, 0xf0040ff0);
		rtw_write32(rtwdev, 0x1c38, 0xffffffff);
	}
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c5);
	rtw_write32(rtwdev, 0x9b4, 0xdb66db00);
	rtw_write32(rtwdev, base_addr + 0xb0, 0x0a11fb88);
	rtw_write32(rtwdev, base_addr + 0xbc, 0x0008ff81);
	rtw_write32(rtwdev, base_addr + 0xc0, 0x0003d208);
	rtw_write32(rtwdev, base_addr + 0xcc, 0x0a11fb88);
	rtw_write32(rtwdev, base_addr + 0xd8, 0x0008ff81);
	rtw_write32(rtwdev, base_addr + 0xdc, 0x0003d208);
	rtw_write32(rtwdev, base_addr + 0xb8, 0x60000000);
	mdelay(2);
	rtw_write32(rtwdev, base_addr + 0xbc, 0x000aff8d);
	mdelay(2);
	rtw_write32(rtwdev, base_addr + 0xb0, 0x0a11fb89);
	rtw_write32(rtwdev, base_addr + 0xcc, 0x0a11fb89);
	mdelay(1);
	rtw_write32(rtwdev, base_addr + 0xb8, 0x62000000);
	rtw_write32(rtwdev, base_addr + 0xd4, 0x62000000);
	mdelay(20);
	if (!check_hw_ready(rtwdev, read_addr + 0x08, 0x7fff80, 0xffff) ||
	    !check_hw_ready(rtwdev, read_addr + 0x34, 0x7fff80, 0xffff))
		rtw_err(rtwdev, "failed to wait for dack ready\n");
	rtw_write32(rtwdev, base_addr + 0xb8, 0x02000000);
	mdelay(1);
	rtw_write32(rtwdev, base_addr + 0xbc, 0x0008ff87);
	rtw_write32(rtwdev, 0x9b4, 0xdb6db600);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c5);
	rtw_write32(rtwdev, base_addr + 0xbc, 0x0008ff87);
	rtw_write32(rtwdev, base_addr + 0x60, 0xf0000000);
}

static void rtw8822c_dac_cal_step2(struct rtw_dev *rtwdev,
				   u8 path, u32 *ic_out, u32 *qc_out)
{
	u32 base_addr;
	u32 ic, qc, ic_in, qc_in;

	base_addr = rtw8822c_get_path_write_addr(path);
	rtw_write32_mask(rtwdev, base_addr + 0xbc, 0xf0000000, 0x0);
	rtw_write32_mask(rtwdev, base_addr + 0xc0, 0xf, 0x8);
	rtw_write32_mask(rtwdev, base_addr + 0xd8, 0xf0000000, 0x0);
	rtw_write32_mask(rtwdev, base_addr + 0xdc, 0xf, 0x8);

	rtw_write32(rtwdev, 0x1b00, 0x00000008);
	rtw_write8(rtwdev, 0x1bcc, 0x03f);
	rtw_write32(rtwdev, base_addr + 0x0c, 0xdff00220);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c5);
	rtw_write32(rtwdev, 0x1c3c, 0x00088103);

	rtw8822c_dac_cal_rf_mode(rtwdev, &ic_in, &qc_in);
	ic = ic_in;
	qc = qc_in;

	/* compensation value */
	if (ic != 0x0)
		ic = 0x400 - ic;
	if (qc != 0x0)
		qc = 0x400 - qc;
	if (ic < 0x300) {
		ic = ic * 2 * 6 / 5;
		ic = ic + 0x80;
	} else {
		ic = (0x400 - ic) * 2 * 6 / 5;
		ic = 0x7f - ic;
	}
	if (qc < 0x300) {
		qc = qc * 2 * 6 / 5;
		qc = qc + 0x80;
	} else {
		qc = (0x400 - qc) * 2 * 6 / 5;
		qc = 0x7f - qc;
	}

	*ic_out = ic;
	*qc_out = qc;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] before i=0x%x, q=0x%x\n", ic_in, qc_in);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] after  i=0x%x, q=0x%x\n", ic, qc);
}

static void rtw8822c_dac_cal_step3(struct rtw_dev *rtwdev, u8 path,
				   u32 adc_ic, u32 adc_qc,
				   u32 *ic_in, u32 *qc_in,
				   u32 *i_out, u32 *q_out)
{
	u32 base_addr;
	u32 read_addr;
	u32 ic, qc;
	u32 temp;

	base_addr = rtw8822c_get_path_write_addr(path);
	read_addr = rtw8822c_get_path_read_addr(path);
	ic = *ic_in;
	qc = *qc_in;

	rtw_write32(rtwdev, base_addr + 0x0c, 0xdff00220);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c5);
	rtw_write32(rtwdev, 0x9b4, 0xdb66db00);
	rtw_write32(rtwdev, base_addr + 0xb0, 0x0a11fb88);
	rtw_write32(rtwdev, base_addr + 0xbc, 0xc008ff81);
	rtw_write32(rtwdev, base_addr + 0xc0, 0x0003d208);
	rtw_write32_mask(rtwdev, base_addr + 0xbc, 0xf0000000, ic & 0xf);
	rtw_write32_mask(rtwdev, base_addr + 0xc0, 0xf, (ic & 0xf0) >> 4);
	rtw_write32(rtwdev, base_addr + 0xcc, 0x0a11fb88);
	rtw_write32(rtwdev, base_addr + 0xd8, 0xe008ff81);
	rtw_write32(rtwdev, base_addr + 0xdc, 0x0003d208);
	rtw_write32_mask(rtwdev, base_addr + 0xd8, 0xf0000000, qc & 0xf);
	rtw_write32_mask(rtwdev, base_addr + 0xdc, 0xf, (qc & 0xf0) >> 4);
	rtw_write32(rtwdev, base_addr + 0xb8, 0x60000000);
	mdelay(2);
	rtw_write32_mask(rtwdev, base_addr + 0xbc, 0xe, 0x6);
	mdelay(2);
	rtw_write32(rtwdev, base_addr + 0xb0, 0x0a11fb89);
	rtw_write32(rtwdev, base_addr + 0xcc, 0x0a11fb89);
	mdelay(1);
	rtw_write32(rtwdev, base_addr + 0xb8, 0x62000000);
	rtw_write32(rtwdev, base_addr + 0xd4, 0x62000000);
	mdelay(20);
	if (!check_hw_ready(rtwdev, read_addr + 0x24, 0x07f80000, ic) ||
	    !check_hw_ready(rtwdev, read_addr + 0x50, 0x07f80000, qc))
		rtw_err(rtwdev, "failed to write IQ vector to hardware\n");
	rtw_write32(rtwdev, base_addr + 0xb8, 0x02000000);
	mdelay(1);
	rtw_write32_mask(rtwdev, base_addr + 0xbc, 0xe, 0x3);
	rtw_write32(rtwdev, 0x9b4, 0xdb6db600);

	/* check DAC DC offset */
	temp = ((adc_ic + 0x10) & 0x3ff) | (((adc_qc + 0x10) & 0x3ff) << 10);
	rtw_write32(rtwdev, base_addr + 0x68, temp);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c5);
	rtw_write32(rtwdev, base_addr + 0x60, 0xf0000000);
	rtw8822c_dac_cal_rf_mode(rtwdev, &ic, &qc);
	if (ic >= 0x10)
		ic = ic - 0x10;
	else
		ic = 0x400 - (0x10 - ic);

	if (qc >= 0x10)
		qc = qc - 0x10;
	else
		qc = 0x400 - (0x10 - qc);

	*i_out = ic;
	*q_out = qc;

	if (ic >= 0x200)
		ic = 0x400 - ic;
	if (qc >= 0x200)
		qc = 0x400 - qc;

	*ic_in = ic;
	*qc_in = qc;

	rtw_dbg(rtwdev, RTW_DBG_RFK,
		"[DACK] after  DACK i=0x%x, q=0x%x\n", *i_out, *q_out);
}

static void rtw8822c_dac_cal_step4(struct rtw_dev *rtwdev, u8 path)
{
	u32 base_addr = rtw8822c_get_path_write_addr(path);

	rtw_write32(rtwdev, base_addr + 0x68, 0x0);
	rtw_write32(rtwdev, base_addr + 0x10, 0x02d508c4);
	rtw_write32_mask(rtwdev, base_addr + 0xbc, 0x1, 0x0);
	rtw_write32_mask(rtwdev, base_addr + 0x30, BIT(30), 0x1);
}

static bool rtw8822c_dac_iq_check(struct rtw_dev *rtwdev, u32 value)
{
	bool ret = true;

	if ((value >= 0x200 && (0x400 - value) > 0x64) ||
	    (value < 0x200 && value > 0x64)) {
		ret = false;
		rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] Error overflow\n");
	}

	return ret;
}

static void rtw8822c_dac_iq_offset(struct rtw_dev *rtwdev, u32 *vec, u32 *val)
{
	u32 p, m, t, i;

	m = 0;
	p = 0;
	for (i = 10; i < DACK_SN_8822C - 10; i++) {
		if (vec[i] > 0x200)
			m = (0x400 - vec[i]) + m;
		else
			p = vec[i] + p;
	}

	if (p > m) {
		t = p - m;
		t = t / (DACK_SN_8822C - 20);
	} else {
		t = m - p;
		t = t / (DACK_SN_8822C - 20);
		if (t != 0x0)
			t = 0x400 - t;
	}

	*val = t;
}

static void rtw8822c_dac_iq_sort(struct rtw_dev *rtwdev, u32 *iv, u32 *qv)
{
	u32 i, j;

	for (i = 0; i < DACK_SN_8822C - 1; i++) {
		for (j = 0; j < (DACK_SN_8822C - 1 - i) ; j++) {
			__rtw8822c_dac_iq_sort(rtwdev, &iv[j], &iv[j + 1]);
			__rtw8822c_dac_iq_sort(rtwdev, &qv[j], &qv[j + 1]);
		}
	}
}

static void rtw8822c_dac_restore_reg_restore_impl(struct rtw_dev *rtwdev,
				     struct rtw_backup_info *backup,
				     struct rtw_backup_info *backup_rf)
{
	u32 path, i;
	u32 val;
	u32 reg;

	rtw_restore_reg(rtwdev, backup, DACK_REG_8822C);

	for (path = 0; path < DACK_PATH_8822C; path++) {
		for (i = 0; i < DACK_RF_8822C; i++) {
			val = backup_rf[path * i + i].val;
			reg = backup_rf[path * i + i].reg;
			rtw_write_rf(rtwdev, path, reg, RFREG_MASK, val);
		}
	}
}
static void rtw8822c_dac_restore_reg(struct rtw_dev *rtwdev,
				     struct rtw_backup_info *backup,
				     struct rtw_backup_info *backup_rf)
{++rtwdev->restore_depth;rtw8822c_dac_restore_reg_restore_impl(rtwdev,backup,backup_rf);--rtwdev->restore_depth;}

static void rtw8822c_do_dpk(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	struct rtw_backup_info bckp[DPK_BB_REG_NUM];
	u32 rf_reg_backup[DPK_RF_REG_NUM][DPK_RF_PATH_NUM];
	u32 bb_reg[DPK_BB_REG_NUM] = {
		0x520, 0x820, 0x824, 0x1c3c, 0x1d58, 0x1864,
		0x4164, 0x180c, 0x410c, 0x186c, 0x416c,
		0x1a14, 0x1e70, 0x80c, 0x1d70, 0x1e7c, 0x18a4, 0x41a4};
	u32 rf_reg[DPK_RF_REG_NUM] = {
		0x0, 0x1a, 0x55, 0x63, 0x87, 0x8f, 0xde};
	u8 path;

	if (!dpk_info->is_dpk_pwr_on) {
		rtw_dbg(rtwdev, RTW_DBG_RFK, "[DPK] Skip DPK due to DPD PWR off\n");
		return;
	} else if (rtw8822c_dpk_reload(rtwdev)) {
		return;
	}

	for (path = RF_PATH_A; path < DPK_RF_PATH_NUM; path++)
		ewma_thermal_init(&dpk_info->avg_thermal[path]);

	rtw8822c_dpk_information(rtwdev);

	rtw8822c_dpk_backup_registers(rtwdev, bb_reg, DPK_BB_REG_NUM, bckp);
	rtw8822c_dpk_backup_rf_registers(rtwdev, rf_reg, rf_reg_backup);

	rtw8822c_dpk_mac_bb_setting(rtwdev);
	rtw8822c_dpk_afe_setting(rtwdev, true);
	rtw8822c_dpk_pre_setting(rtwdev);
	rtw8822c_dpk_result_reset(rtwdev);
	rtw8822c_dpk_path_select(rtwdev);
	rtw8822c_dpk_afe_setting(rtwdev, false);
	rtw8822c_dpk_enable_disable(rtwdev);

	rtw8822c_dpk_reload_rf_registers(rtwdev, rf_reg, rf_reg_backup);
	for (path = 0; path < rtwdev->hal.rf_path_num; path++)
		rtw8822c_dpk_rxbb_dc_cal(rtwdev, path);
	rtw8822c_dpk_restore_registers(rtwdev, DPK_BB_REG_NUM, bckp);
}

static void rtw8822c_do_gapk(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm = &rtwdev->dm_info;

	if (dm->dm_flags & BIT(RTW_DM_CAP_TXGAPK)) {
		rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] feature disable!!!\n");
		return;
	}
	rtw8822c_rfk_handshake(rtwdev, true);
	rtw8822c_txgapk(rtwdev);
	rtw8822c_rfk_handshake(rtwdev, false);
}

static void rtw8822c_do_iqk(struct rtw_dev *rtwdev)
{
	struct rtw_iqk_para para = {0};
	u8 iqk_chk;
	int ret;

	para.clear = 1;
	rtw_write8(rtwdev,REG_IQKSTAT,0);
	rtw_fw_do_iqk(rtwdev,&para);

	ret = read_poll_timeout(rtw_read8, iqk_chk, iqk_chk == IQK_DONE_8822C,
				20000, 300000, false, rtwdev, REG_RPT_CIP);
	if (ret)
		rtw_warn(rtwdev, "failed to poll iqk status bit\n");

	if(!ret&&!native_error(rtwdev))rtwdev->iqk_complete=1;
	rtw_write8(rtwdev, REG_IQKSTAT, 0x0);
}

static void rtw8822c_do_lck(struct rtw_dev *rtwdev)
{
	u32 val;

	rtw_write_rf(rtwdev, RF_PATH_A, RF_SYN_CTRL, RFREG_MASK, 0x80010);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_SYN_PFD, RFREG_MASK, 0x1F0FA);
	fsleep(1);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_AAC_CTRL, RFREG_MASK, 0x80000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_SYN_AAC, RFREG_MASK, 0x80001);
	read_poll_timeout(rtw_read_rf, val, val != 0x1, 1000, 100000,
			  true, rtwdev, RF_PATH_A, RF_AAC_CTRL, 0x1000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_SYN_PFD, RFREG_MASK, 0x1F0F8);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_SYN_CTRL, RFREG_MASK, 0x80010);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_FAST_LCK, RFREG_MASK, 0x0f000);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_FAST_LCK, RFREG_MASK, 0x4f000);
	fsleep(1);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_FAST_LCK, RFREG_MASK, 0x0f000);
}

static void rtw8822c_dpk_afe_setting(struct rtw_dev *rtwdev, bool is_do_dpk)
{
	if (is_do_dpk)
		rtw_load_table(rtwdev, &rtw8822c_dpk_afe_is_dpk_tbl);
	else
		rtw_load_table(rtwdev, &rtw8822c_dpk_afe_no_dpk_tbl);
}

static u8 rtw8822c_dpk_agc_gain_chk(struct rtw_dev *rtwdev, u8 path,
				    u8 limited_pga)
{
	u8 result = 0;
	u16 dgain;

	rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_DAGC);
	dgain = rtw8822c_dpk_dgain_read(rtwdev, path);

	if (dgain > 1535 && !limited_pga)
		return RTW_DPK_GAIN_LESS;
	else if (dgain < 768 && !limited_pga)
		return RTW_DPK_GAIN_LARGE;
	else
		return result;
}

static u8 rtw8822c_dpk_agc_loss_chk(struct rtw_dev *rtwdev, u8 path)
{
	u32 loss, loss_db;

	loss = rtw8822c_dpk_pas_read(rtwdev, path);
	if (loss < 0x4000000)
		return RTW_DPK_GL_LESS;
	loss_db = 3 * rtw8822c_psd_log2base(loss >> 13) - 3870;

	if (loss_db > 1000)
		return RTW_DPK_GL_LARGE;
	else if (loss_db < 250)
		return RTW_DPK_GL_LESS;
	else
		return RTW_DPK_AGC_OUT;
}

static void
rtw8822c_dpk_backup_registers(struct rtw_dev *rtwdev, u32 *reg,
			      u32 reg_num, struct rtw_backup_info *bckp)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		bckp[i].len = 4;
		bckp[i].reg = reg[i];
		bckp[i].val = rtw_read32(rtwdev, reg[i]);
	}
}

static void rtw8822c_dpk_backup_rf_registers(struct rtw_dev *rtwdev,
					     u32 *rf_reg,
					     u32 rf_reg_bak[][2])
{
	u32 i;

	for (i = 0; i < DPK_RF_REG_NUM; i++) {
		rf_reg_bak[i][RF_PATH_A] = rtw_read_rf(rtwdev, RF_PATH_A,
						       rf_reg[i], RFREG_MASK);
		rf_reg_bak[i][RF_PATH_B] = rtw_read_rf(rtwdev, RF_PATH_B,
						       rf_reg[i], RFREG_MASK);
	}
}

static u8 rtw8822c_dpk_by_path(struct rtw_dev *rtwdev, u32 tx_agc, u8 path)
{
	u8 result;

	result = rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_DO_DPK);

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x8 | (path << 1));

	result = result | (u8)rtw_read32_mask(rtwdev, REG_DPD_CTL1_S0, BIT(26));

	rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, RFREG_MASK, 0x33e14);

	rtw8822c_dpk_get_coef(rtwdev, path);

	return result;
}

static void rtw8822c_dpk_cal_coef1(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u32 offset[DPK_RF_PATH_NUM] = {0, 0x58};
	u32 i_scaling;
	u8 path;

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x0000000c);
	rtw_write32(rtwdev, REG_RXSRAM_CTL, 0x000000f0);
	rtw_write32(rtwdev, REG_NCTL0, 0x00001148);
	rtw_write32(rtwdev, REG_NCTL0, 0x00001149);

	if (!check_hw_ready(rtwdev, 0x2d9c, MASKBYTE0, 0x55))
		rtw_warn(rtwdev, "DPK stuck, performance may be suboptimal");

	rtw_write8(rtwdev, 0x1b10, 0x0);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x0000000c);

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		if(!dpk_info->dpk_gs[path]||native_error(rtwdev)){native_fail(rtwdev,EIO);return;}
		i_scaling = 0x16c00 / dpk_info->dpk_gs[path];

		rtw_write32_mask(rtwdev, 0x1b18 + offset[path], MASKHWORD,
				 i_scaling);
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0 + offset[path],
				 GENMASK(31, 28), 0x9);
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0 + offset[path],
				 GENMASK(31, 28), 0x1);
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0 + offset[path],
				 GENMASK(31, 28), 0x0);
		rtw_write32_mask(rtwdev, REG_DPD_CTL1_S0 + offset[path],
				 BIT(14), 0x0);
	}
}

static void rtw8822c_dpk_cal_gs(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u32 tmp_gs = 0;

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x8 | (path << 1));
	rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_BYPASS_DPD, 0x0);
	rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x0);
	rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_IQ_SWITCH, 0x9);
	rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_INNER_LB, 0x1);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);
	rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, BIT_DPD_CLK, 0xf);

	if (path == RF_PATH_A) {
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0, BIT_GS_PWSF,
				 0x1066680);
		rtw_write32_mask(rtwdev, REG_DPD_CTL1_S0, BIT_DPD_EN, 0x1);
	} else {
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S1, BIT_GS_PWSF,
				 0x1066680);
		rtw_write32_mask(rtwdev, REG_DPD_CTL1_S1, BIT_DPD_EN, 0x1);
	}

	if (dpk_info->dpk_bw == DPK_CHANNEL_WIDTH_80) {
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x80001310);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x00001310);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x810000db);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x010000db);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x0000b428);
		rtw_write32(rtwdev, REG_DPD_CTL15,
			    0x05020000 | (BIT(path) << 28));
	} else {
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x8200190c);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x0200190c);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x8301ee14);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x0301ee14);
		rtw_write32(rtwdev, REG_DPD_CTL16, 0x0000b428);
		rtw_write32(rtwdev, REG_DPD_CTL15,
			    0x05020008 | (BIT(path) << 28));
	}

	rtw_write32_mask(rtwdev, REG_DPD_CTL0, MASKBYTE3, 0x8 | path);

	rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_CAL_PWR);

	rtw_write32_mask(rtwdev, REG_DPD_CTL15, MASKBYTE3, 0x0);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x8 | (path << 1));
	rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_IQ_SWITCH, 0x0);
	rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_INNER_LB, 0x0);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);

	if (path == RF_PATH_A)
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0, BIT_GS_PWSF, 0x5b);
	else
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S1, BIT_GS_PWSF, 0x5b);

	rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, BIT_RPT_SEL, 0x0);

	tmp_gs = (u16)rtw_read32_mask(rtwdev, REG_STAT_RPT, BIT_RPT_DGAIN);
	tmp_gs = (tmp_gs * 910) >> 10;
	tmp_gs = DIV_ROUND_CLOSEST(tmp_gs, 10);

	if (path == RF_PATH_A)
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0, BIT_GS_PWSF, tmp_gs);
	else
		rtw_write32_mask(rtwdev, REG_DPD_CTL0_S1, BIT_GS_PWSF, tmp_gs);

	dpk_info->dpk_gs[path] = tmp_gs;
}

static void rtw8822c_dpk_calibrate(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u32 dpk_txagc;
	u8 dpk_fail;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DPK] s%d dpk start\n", path);

	dpk_txagc = rtw8822c_dpk_gainloss(rtwdev, path);

	dpk_fail = rtw8822c_dpk_by_path(rtwdev, dpk_txagc, path);

	if (!rtw8822c_dpk_check_pass(rtwdev, dpk_fail, dpk_txagc, path))
		rtw_err(rtwdev, "failed to do dpk calibration\n");

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DPK] s%d dpk finish\n", path);

	if (dpk_info->result[path])
		set_bit(path, dpk_info->dpk_path_ok);
}

static bool rtw8822c_dpk_check_pass(struct rtw_dev *rtwdev, bool is_fail,
				    u32 dpk_txagc, u8 path)
{
	bool result;

	if (!is_fail) {
		if (rtw8822c_dpk_coef_read(rtwdev, path))
			result = true;
		else
			result = false;
	} else {
		result = false;
	}

	rtw8822c_dpk_fill_result(rtwdev, dpk_txagc, path, result);

	return result;
}

static bool rtw8822c_dpk_coef_iq_check(struct rtw_dev *rtwdev,
				       u16 coef_i, u16 coef_q)
{
	if (coef_i == 0x1000 || coef_i == 0x0fff ||
	    coef_q == 0x1000 || coef_q == 0x0fff)
		return true;

	return false;
}

static u8 rtw8822c_dpk_coef_read(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u8 addr, result = 1;
	u16 coef_i, coef_q;

	for (addr = 0; addr < 20; addr++) {
		coef_i = FIELD_GET(0x1fff0000, dpk_info->coef[path][addr]);
		coef_q = FIELD_GET(0x1fff, dpk_info->coef[path][addr]);

		if (rtw8822c_dpk_coef_iq_check(rtwdev, coef_i, coef_q)) {
			result = 0;
			break;
		}
	}
	return result;
}

static void rtw8822c_dpk_coef_tbl_apply(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	int i;

	for (i = 0; i < 20; i++) {
		rtw_write32(rtwdev, REG_RXSRAM_CTL,
			    rtw8822c_dpk_get_coef_tbl[i]);
		dpk_info->coef[path][i] = rtw8822c_dpk_coef_transfer(rtwdev);
	}
}

static u32 rtw8822c_dpk_coef_transfer(struct rtw_dev *rtwdev)
{
	u32 reg = 0;
	u16 coef_i = 0, coef_q = 0;

	reg = rtw_read32(rtwdev, REG_STAT_RPT);

	coef_i = (u16)rtw_read32_mask(rtwdev, REG_STAT_RPT, MASKHWORD) & 0x1fff;
	coef_q = (u16)rtw_read32_mask(rtwdev, REG_STAT_RPT, MASKLWORD) & 0x1fff;

	coef_q = ((0x2000 - coef_q) & 0x1fff) - 1;

	reg = (coef_i << 16) | coef_q;

	return reg;
}

static void rtw8822c_dpk_coef_write(struct rtw_dev *rtwdev, u8 path, u8 result)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u16 reg[DPK_RF_PATH_NUM] = {0x1b0c, 0x1b64};
	u32 coef;
	u8 addr;

	rtw_write32(rtwdev, REG_NCTL0, 0x0000000c);
	rtw_write32(rtwdev, REG_RXSRAM_CTL, 0x000000f0);

	for (addr = 0; addr < 20; addr++) {
		if (result == 0) {
			if (addr == 3)
				coef = 0x04001fff;
			else
				coef = 0x00001fff;
		} else {
			coef = dpk_info->coef[path][addr];
		}
		rtw_write32(rtwdev, reg[path] + addr * 4, coef);
	}
}

static u8 rtw8822c_dpk_dc_corr_check(struct rtw_dev *rtwdev, u8 path)
{
	u16 dc_i, dc_q;
	u8 corr_idx;

	rtw_write32(rtwdev, REG_RXSRAM_CTL, 0x000900f0);
	dc_i = (u16)rtw_read32_mask(rtwdev, REG_STAT_RPT, GENMASK(27, 16));
	dc_q = (u16)rtw_read32_mask(rtwdev, REG_STAT_RPT, GENMASK(11, 0));

	if (dc_i & BIT(11))
		dc_i = 0x1000 - dc_i;
	if (dc_q & BIT(11))
		dc_q = 0x1000 - dc_q;

	rtw_write32(rtwdev, REG_RXSRAM_CTL, 0x000000f0);
	corr_idx = (u8)rtw_read32_mask(rtwdev, REG_STAT_RPT, GENMASK(7, 0));
	rtw_read32_mask(rtwdev, REG_STAT_RPT, GENMASK(15, 8));

	if (dc_i > 200 || dc_q > 200 || corr_idx < 40 || corr_idx > 65)
		return 1;
	else
		return 0;

}

static u16 rtw8822c_dpk_dgain_read(struct rtw_dev *rtwdev, u8 path)
{
	u16 dgain;

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);
	rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, 0x00ff0000, 0x0);

	dgain = (u16)rtw_read32_mask(rtwdev, REG_STAT_RPT, GENMASK(27, 16));

	return dgain;
}

static void rtw8822c_dpk_enable_disable(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u32 mask = BIT(15) | BIT(14);

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);

	rtw_write32_mask(rtwdev, REG_DPD_CTL1_S0, BIT_DPD_EN,
			 dpk_info->is_dpk_pwr_on);
	rtw_write32_mask(rtwdev, REG_DPD_CTL1_S1, BIT_DPD_EN,
			 dpk_info->is_dpk_pwr_on);

	if (test_bit(RF_PATH_A, dpk_info->dpk_path_ok)) {
		rtw_write32_mask(rtwdev, REG_DPD_CTL1_S0, mask, 0x0);
		rtw_write8(rtwdev, REG_DPD_CTL0_S0, dpk_info->dpk_gs[RF_PATH_A]);
	}
	if (test_bit(RF_PATH_B, dpk_info->dpk_path_ok)) {
		rtw_write32_mask(rtwdev, REG_DPD_CTL1_S1, mask, 0x0);
		rtw_write8(rtwdev, REG_DPD_CTL0_S1, dpk_info->dpk_gs[RF_PATH_B]);
	}
}

static void rtw8822c_dpk_fill_result(struct rtw_dev *rtwdev, u32 dpk_txagc,
				     u8 path, u8 result)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x8 | (path << 1));

	if (result)
		rtw_write8(rtwdev, REG_DPD_AGC, (u8)(dpk_txagc - 6));
	else
		rtw_write8(rtwdev, REG_DPD_AGC, 0x00);

	dpk_info->result[path] = result;
	dpk_info->dpk_txagc[path] = rtw_read8(rtwdev, REG_DPD_AGC);

	rtw8822c_dpk_coef_write(rtwdev, path, result);
}

static u32 rtw8822c_dpk_gainloss(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u8 tx_agc, tx_bb, ori_txbb, ori_txagc, tx_agc_search, t1, t2;

	ori_txbb = rtw8822c_dpk_rf_setting(rtwdev, path);
	ori_txagc = (u8)rtw_read_rf(rtwdev, path, RF_MODE_TRXAGC, BIT_TXAGC);

	rtw8822c_dpk_rxbb_dc_cal(rtwdev, path);
	rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_DAGC);
	rtw8822c_dpk_dgain_read(rtwdev, path);

	if (rtw8822c_dpk_dc_corr_check(rtwdev, path)) {
		rtw8822c_dpk_rxbb_dc_cal(rtwdev, path);
		rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_DAGC);
		if(rtw8822c_dpk_dc_corr_check(rtwdev,path))native_fail(rtwdev,EIO);
	}

	t1 = rtw8822c_dpk_thermal_read(rtwdev, path);
	tx_bb = rtw8822c_dpk_pas_agc(rtwdev, path, false, true);
	tx_agc_search = rtw8822c_dpk_gainloss_result(rtwdev, path);

	if (tx_bb < tx_agc_search)
		tx_bb = 0;
	else
		tx_bb = tx_bb - tx_agc_search;

	rtw_write_rf(rtwdev, path, RF_TX_GAIN, BIT_GAIN_TXBB, tx_bb);

	tx_agc = ori_txagc - (ori_txbb - tx_bb);

	t2 = rtw8822c_dpk_thermal_read(rtwdev, path);

	dpk_info->thermal_dpk_delta[path] = abs(t2 - t1);

	return tx_agc;
}

static u8 rtw8822c_dpk_gainloss_result(struct rtw_dev *rtwdev, u8 path)
{
	u8 result;

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x8 | (path << 1));
	rtw_write32_mask(rtwdev, 0x1b48, BIT(14), 0x1);
	rtw_write32(rtwdev, REG_RXSRAM_CTL, 0x00060000);

	result = (u8)rtw_read32_mask(rtwdev, REG_STAT_RPT, 0x000000f0);

	rtw_write32_mask(rtwdev, 0x1b48, BIT(14), 0x0);

	return result;
}

static u16 rtw8822c_dpk_get_cmd(struct rtw_dev *rtwdev, u8 action, u8 path)
{
	u16 cmd;
	u8 bw = rtwdev->dm_info.dpk_info.dpk_bw == DPK_CHANNEL_WIDTH_80 ? 2 : 0;

	switch (action) {
	case RTW_DPK_GAIN_LOSS:
		cmd = 0x14 + path;
		break;
	case RTW_DPK_DO_DPK:
		cmd = 0x16 + path + bw;
		break;
	case RTW_DPK_DPK_ON:
		cmd = 0x1a + path;
		break;
	case RTW_DPK_DAGC:
		cmd = 0x1c + path + bw;
		break;
	default:
		return 0;
	}

	return (cmd << 8) | 0x48;
}

static void rtw8822c_dpk_get_coef(struct rtw_dev *rtwdev, u8 path)
{
	rtw_write32(rtwdev, REG_NCTL0, 0x0000000c);

	if (path == RF_PATH_A) {
		rtw_write32_mask(rtwdev, REG_DPD_CTL0, BIT(24), 0x0);
		rtw_write32(rtwdev, REG_DPD_CTL0_S0, 0x30000080);
	} else if (path == RF_PATH_B) {
		rtw_write32_mask(rtwdev, REG_DPD_CTL0, BIT(24), 0x1);
		rtw_write32(rtwdev, REG_DPD_CTL0_S1, 0x30000080);
	}

	rtw8822c_dpk_coef_tbl_apply(rtwdev, path);
}

static void rtw8822c_dpk_information(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u32  reg;
	u8 band_shift;

	reg = rtw_read_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK);

	band_shift = FIELD_GET(BIT(16), reg);
	dpk_info->dpk_band = 1 << band_shift;
	dpk_info->dpk_ch = FIELD_GET(0xff, reg);
	dpk_info->dpk_bw = FIELD_GET(0x3000, reg);
}

static void rtw8822c_dpk_mac_bb_setting(struct rtw_dev *rtwdev)
{
	rtw8822c_dpk_tx_pause(rtwdev);
	rtw_load_table(rtwdev, &rtw8822c_dpk_mac_bb_tbl);
}

static void rtw8822c_dpk_on(struct rtw_dev *rtwdev, u8 path)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;

	rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_DPK_ON);

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0x8 | (path << 1));
	rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x0);

	if (test_bit(path, dpk_info->dpk_path_ok))
		rtw8822c_dpk_cal_gs(rtwdev, path);
}

static u8 rtw8822c_dpk_one_shot(struct rtw_dev *rtwdev, u8 path, u8 action)
{
	u16 dpk_cmd;
	u8 result = 0;

	rtw8822c_dpk_set_gnt_wl(rtwdev, true);

	if (action == RTW_DPK_CAL_PWR) {
		rtw_write32_mask(rtwdev, REG_DPD_CTL0, BIT(12), 0x1);
		rtw_write32_mask(rtwdev, REG_DPD_CTL0, BIT(12), 0x0);
		rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, BIT_RPT_SEL, 0x0);
		fsleep(10000);
		if (!check_hw_ready(rtwdev, REG_STAT_RPT, BIT(31), 0x1)) {
			result = 1;
			rtw_dbg(rtwdev, RTW_DBG_RFK, "[DPK] one-shot over 20ms\n");
		}
	} else {
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE,
				 0x8 | (path << 1));
		rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_IQ_SWITCH, 0x9);

		dpk_cmd = rtw8822c_dpk_get_cmd(rtwdev, action, path);
		rtw_write32(rtwdev, REG_NCTL0, dpk_cmd);
		rtw_write32(rtwdev, REG_NCTL0, dpk_cmd + 1);
		fsleep(10000);
		if (!check_hw_ready(rtwdev, 0x2d9c, 0xff, 0x55)) {
			result = 1;
			rtw_dbg(rtwdev, RTW_DBG_RFK, "[DPK] one-shot over 20ms\n");
		}
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE,
				 0x8 | (path << 1));
		rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_IQ_SWITCH, 0x0);
	}

	rtw8822c_dpk_set_gnt_wl(rtwdev, false);

	rtw_write8(rtwdev, 0x1b10, 0x0);

	return result;
}

static u8 rtw8822c_dpk_pas_agc(struct rtw_dev *rtwdev, u8 path,
			       bool gain_only, bool loss_only)
{
	struct rtw8822c_dpk_data data = {0};
	u8 (*func)(struct rtw_dev *rtwdev, struct rtw8822c_dpk_data *data);
	u8 state = RTW_DPK_GAIN_CHECK;

	data.loss_only = loss_only;
	data.gain_only = gain_only;
	data.path = path;

	for (unsigned native_state_steps=0; ;native_state_steps++) {
		if(native_state_steps>=256||state>=ARRAY_SIZE(dpk_state)||native_error(rtwdev)){native_fail(rtwdev,ETIMEDOUT);break;}
		func = dpk_state[state];
		state = func(rtwdev, &data);
		if (state == RTW_DPK_AGC_OUT)
			break;
	}

	return data.txbb;
}

static u32 rtw8822c_dpk_pas_read(struct rtw_dev *rtwdev, u8 path)
{
	u32 i_val, q_val;

	rtw_write32(rtwdev, REG_NCTL0, 0x8 | (path << 1));
	rtw_write32_mask(rtwdev, 0x1b48, BIT(14), 0x0);
	rtw_write32(rtwdev, REG_RXSRAM_CTL, 0x00060001);
	rtw_write32(rtwdev, 0x1b4c, 0x00000000);
	rtw_write32(rtwdev, 0x1b4c, 0x00080000);

	q_val = rtw_read32_mask(rtwdev, REG_STAT_RPT, MASKHWORD);
	i_val = rtw_read32_mask(rtwdev, REG_STAT_RPT, MASKLWORD);

	if (i_val & BIT(15))
		i_val = 0x10000 - i_val;
	if (q_val & BIT(15))
		q_val = 0x10000 - q_val;

	rtw_write32(rtwdev, 0x1b4c, 0x00000000);

	return i_val * i_val + q_val * q_val;
}

static void rtw8822c_dpk_path_select(struct rtw_dev *rtwdev)
{
	rtw8822c_dpk_calibrate(rtwdev, RF_PATH_A);
	rtw8822c_dpk_calibrate(rtwdev, RF_PATH_B);
	rtw8822c_dpk_on(rtwdev, RF_PATH_A);
	rtw8822c_dpk_on(rtwdev, RF_PATH_B);
	rtw8822c_dpk_cal_coef1(rtwdev);
}

static void rtw8822c_dpk_pre_setting(struct rtw_dev *rtwdev)
{
	u8 path;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_write_rf(rtwdev, path, RF_RXAGC_OFFSET, RFREG_MASK, 0x0);
		rtw_write32(rtwdev, REG_NCTL0, 0x8 | (path << 1));
		if (rtwdev->dm_info.dpk_info.dpk_band == RTW_BAND_2G)
			rtw_write32(rtwdev, REG_DPD_CTL1_S1, 0x1f100000);
		else
			rtw_write32(rtwdev, REG_DPD_CTL1_S1, 0x1f0d0000);
		rtw_write32_mask(rtwdev, REG_DPD_LUT0, BIT_GLOSS_DB, 0x4);
		rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x3);
	}
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);
	rtw_write32(rtwdev, REG_DPD_CTL11, 0x3b23170b);
	rtw_write32(rtwdev, REG_DPD_CTL12, 0x775f5347);
}

static bool rtw8822c_dpk_reload_restore_impl(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u8 channel;

	dpk_info->is_reload = false;

	channel = (u8)(rtw_read_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK) & 0xff);

	if (channel == dpk_info->dpk_ch) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[DPK] DPK reload for CH%d!!\n", dpk_info->dpk_ch);
		rtw8822c_dpk_reload_data(rtwdev);
		dpk_info->is_reload = true;
	}

	return dpk_info->is_reload;
}
static bool rtw8822c_dpk_reload(struct rtw_dev *rtwdev)
{++rtwdev->restore_depth;__typeof__(rtw8822c_dpk_reload_restore_impl(rtwdev)) native_result=rtw8822c_dpk_reload_restore_impl(rtwdev);--rtwdev->restore_depth;return native_result;}

static void rtw8822c_dpk_reload_data_restore_impl(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u8 path;

	if (!test_bit(RF_PATH_A, dpk_info->dpk_path_ok) &&
	    !test_bit(RF_PATH_B, dpk_info->dpk_path_ok) &&
	    dpk_info->dpk_ch == 0)
		return;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE,
				 0x8 | (path << 1));
		if (dpk_info->dpk_band == RTW_BAND_2G)
			rtw_write32(rtwdev, REG_DPD_CTL1_S1, 0x1f100000);
		else
			rtw_write32(rtwdev, REG_DPD_CTL1_S1, 0x1f0d0000);

		rtw_write8(rtwdev, REG_DPD_AGC, dpk_info->dpk_txagc[path]);

		rtw8822c_dpk_coef_write(rtwdev, path,
					test_bit(path, dpk_info->dpk_path_ok));

		rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_DPK_ON);

		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);

		if (path == RF_PATH_A)
			rtw_write32_mask(rtwdev, REG_DPD_CTL0_S0, BIT_GS_PWSF,
					 dpk_info->dpk_gs[path]);
		else
			rtw_write32_mask(rtwdev, REG_DPD_CTL0_S1, BIT_GS_PWSF,
					 dpk_info->dpk_gs[path]);
	}
	rtw8822c_dpk_cal_coef1(rtwdev);
}
static void rtw8822c_dpk_reload_data(struct rtw_dev *rtwdev)
{++rtwdev->restore_depth;rtw8822c_dpk_reload_data_restore_impl(rtwdev);--rtwdev->restore_depth;}

static void rtw8822c_dpk_reload_rf_registers_restore_impl(struct rtw_dev *rtwdev,
					     u32 *rf_reg,
					     u32 rf_reg_bak[][2])
{
	u32 i;

	for (i = 0; i < DPK_RF_REG_NUM; i++) {
		rtw_write_rf(rtwdev, RF_PATH_A, rf_reg[i], RFREG_MASK,
			     rf_reg_bak[i][RF_PATH_A]);
		rtw_write_rf(rtwdev, RF_PATH_B, rf_reg[i], RFREG_MASK,
			     rf_reg_bak[i][RF_PATH_B]);
	}
}
static void rtw8822c_dpk_reload_rf_registers(struct rtw_dev *rtwdev,
					     u32 *rf_reg,
					     u32 rf_reg_bak[][2])
{++rtwdev->restore_depth;rtw8822c_dpk_reload_rf_registers_restore_impl(rtwdev,rf_reg,rf_reg_bak);--rtwdev->restore_depth;}

static void
rtw8822c_dpk_restore_registers_restore_impl(struct rtw_dev *rtwdev, u32 reg_num,
			       struct rtw_backup_info *bckp)
{
	rtw_restore_reg(rtwdev, bckp, reg_num);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE, 0xc);
	rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, BIT_DPD_CLK, 0x4);
}
static void
rtw8822c_dpk_restore_registers(struct rtw_dev *rtwdev, u32 reg_num,
			       struct rtw_backup_info *bckp)
{++rtwdev->restore_depth;rtw8822c_dpk_restore_registers_restore_impl(rtwdev,reg_num,bckp);--rtwdev->restore_depth;}

static void rtw8822c_dpk_result_reset(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u8 path;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		clear_bit(path, dpk_info->dpk_path_ok);
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE,
				 0x8 | (path << 1));
		rtw_write32_mask(rtwdev, 0x1b58, 0x0000007f, 0x0);

		dpk_info->dpk_txagc[path] = 0;
		dpk_info->result[path] = 0;
		dpk_info->dpk_gs[path] = 0x5b;
		dpk_info->pre_pwsf[path] = 0;
		dpk_info->thermal_dpk[path] = rtw8822c_dpk_thermal_read(rtwdev,
									path);
	}
}

static u32 rtw8822c_dpk_rf_setting(struct rtw_dev *rtwdev, u8 path)
{
	u32 ori_txbb;

	rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, RFREG_MASK, 0x50017);
	ori_txbb = rtw_read_rf(rtwdev, path, RF_TX_GAIN, RFREG_MASK);

	rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TX_GAIN, 0x1);
	rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_PWR_TRIM, 0x1);
	rtw_write_rf(rtwdev, path, RF_TX_GAIN_OFFSET, BIT_BB_GAIN, 0x0);
	rtw_write_rf(rtwdev, path, RF_TX_GAIN, RFREG_MASK, ori_txbb);

	if (rtwdev->dm_info.dpk_info.dpk_band == RTW_BAND_2G) {
		rtw_write_rf(rtwdev, path, RF_TX_GAIN_OFFSET, BIT_RF_GAIN, 0x1);
		rtw_write_rf(rtwdev, path, RF_RXG_GAIN, BIT_RXG_GAIN, 0x0);
	} else {
		rtw_write_rf(rtwdev, path, RF_TXA_LB_SW, BIT_TXA_LB_ATT, 0x0);
		rtw_write_rf(rtwdev, path, RF_TXA_LB_SW, BIT_LB_ATT, 0x6);
		rtw_write_rf(rtwdev, path, RF_TXA_LB_SW, BIT_LB_SW, 0x1);
		rtw_write_rf(rtwdev, path, RF_RXA_MIX_GAIN, BIT_RXA_MIX_GAIN, 0);
	}

	rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, BIT_RXAGC, 0xf);
	rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TRXBW, 0x1);
	rtw_write_rf(rtwdev, path, RF_BW_TRXBB, BIT_BW_RXBB, 0x0);

	if (rtwdev->dm_info.dpk_info.dpk_bw == DPK_CHANNEL_WIDTH_80)
		rtw_write_rf(rtwdev, path, RF_BW_TRXBB, BIT_BW_TXBB, 0x2);
	else
		rtw_write_rf(rtwdev, path, RF_BW_TRXBB, BIT_BW_TXBB, 0x1);

	rtw_write_rf(rtwdev, path, RF_EXT_TIA_BW, BIT(1), 0x1);

	usleep_range(100, 110);

	return ori_txbb & 0x1f;
}

static void rtw8822c_dpk_rxbb_dc_cal(struct rtw_dev *rtwdev, u8 path)
{
	rtw_write_rf(rtwdev, path, 0x92, RFREG_MASK, 0x84800);
	udelay(5);
	rtw_write_rf(rtwdev, path, 0x92, RFREG_MASK, 0x84801);
	usleep_range(600, 610);
	rtw_write_rf(rtwdev, path, 0x92, RFREG_MASK, 0x84800);
}

static void rtw8822c_dpk_set_gnt_wl(struct rtw_dev *rtwdev, bool is_before_k)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;

	if (is_before_k) {
		dpk_info->gnt_control = rtw_read32(rtwdev, 0x70);
		dpk_info->gnt_value = rtw_coex_read_indirect_reg(rtwdev, 0x38);
		if(native_error(rtwdev))return;rtwdev->gnt_saved=1;
		rtw_write32_mask(rtwdev, 0x70, BIT(26), 0x1);
		rtw_coex_write_indirect_reg(rtwdev, 0x38, MASKBYTE1, 0x77);
	} else {
		if(!rtwdev->gnt_saved)return;++rtwdev->restore_depth;
		rtw_coex_write_indirect_reg(rtwdev, 0x38, MASKDWORD,
					    dpk_info->gnt_value);
		rtw_write32(rtwdev, 0x70, dpk_info->gnt_control);
		rtwdev->gnt_saved=0;--rtwdev->restore_depth;
	}
}

static u8 rtw8822c_dpk_thermal_read(struct rtw_dev *rtwdev, u8 path)
{
	rtw_write_rf(rtwdev, path, RF_T_METER, BIT(19), 0x1);
	rtw_write_rf(rtwdev, path, RF_T_METER, BIT(19), 0x0);
	rtw_write_rf(rtwdev, path, RF_T_METER, BIT(19), 0x1);
	udelay(15);

	return (u8)rtw_read_rf(rtwdev, path, RF_T_METER, 0x0007e);
}

static void rtw8822c_dpk_track(struct rtw_dev *rtwdev)
{
	struct rtw_dpk_info *dpk_info = &rtwdev->dm_info.dpk_info;
	u8 path;
	u8 thermal_value[DPK_RF_PATH_NUM] = {0};
	s8 offset[DPK_RF_PATH_NUM], delta_dpk[DPK_RF_PATH_NUM];

	if (dpk_info->thermal_dpk[0] == 0 && dpk_info->thermal_dpk[1] == 0)
		return;

	for (path = 0; path < DPK_RF_PATH_NUM; path++) {
		thermal_value[path] = rtw8822c_dpk_thermal_read(rtwdev, path);
		ewma_thermal_add(&dpk_info->avg_thermal[path],
				 thermal_value[path]);
		thermal_value[path] =
			ewma_thermal_read(&dpk_info->avg_thermal[path]);
		delta_dpk[path] = dpk_info->thermal_dpk[path] -
				  thermal_value[path];
		offset[path] = delta_dpk[path] -
			       dpk_info->thermal_dpk_delta[path];
		offset[path] &= 0x7f;

		if (offset[path] != dpk_info->pre_pwsf[path]) {
			rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SUBPAGE,
					 0x8 | (path << 1));
			rtw_write32_mask(rtwdev, 0x1b58, GENMASK(6, 0),
					 offset[path]);
			dpk_info->pre_pwsf[path] = offset[path];
		}
	}
}

static void rtw8822c_dpk_tx_pause(struct rtw_dev *rtwdev)
{
	u8 reg_a, reg_b;
	u16 count = 0;

	rtw_write8(rtwdev, 0x522, 0xff);
	rtw_write32_mask(rtwdev, 0x1e70, 0xf, 0x2);

	do {
		reg_a = (u8)rtw_read_rf(rtwdev, RF_PATH_A, 0x00, 0xf0000);
		reg_b = (u8)rtw_read_rf(rtwdev, RF_PATH_B, 0x00, 0xf0000);
		udelay(2);
		count++;
	} while ((reg_a == 2 || reg_b == 2) && count < 2500);
}

static u8 rtw8822c_gain_check_state(struct rtw_dev *rtwdev,
				    struct rtw8822c_dpk_data *data)
{
	u8 state;

	data->txbb = (u8)rtw_read_rf(rtwdev, data->path, RF_TX_GAIN,
				     BIT_GAIN_TXBB);
	data->pga = (u8)rtw_read_rf(rtwdev, data->path, RF_MODE_TRXAGC,
				    BIT_RXAGC);

	if (data->loss_only) {
		state = RTW_DPK_LOSS_CHECK;
		goto check_end;
	}

	state = rtw8822c_dpk_agc_gain_chk(rtwdev, data->path,
					  data->limited_pga);
	if (state == RTW_DPK_GAIN_CHECK && data->gain_only)
		state = RTW_DPK_AGC_OUT;
	else if (state == RTW_DPK_GAIN_CHECK)
		state = RTW_DPK_LOSS_CHECK;

check_end:
	data->agc_cnt++;
	if (data->agc_cnt >= 6)
		state = RTW_DPK_AGC_OUT;

	return state;
}

static u8 rtw8822c_gain_large_state(struct rtw_dev *rtwdev,
				    struct rtw8822c_dpk_data *data)
{
	u8 pga = data->pga;

	if (pga > 0xe)
		rtw_write_rf(rtwdev, data->path, RF_MODE_TRXAGC, BIT_RXAGC, 0xc);
	else if (pga > 0xb && pga < 0xf)
		rtw_write_rf(rtwdev, data->path, RF_MODE_TRXAGC, BIT_RXAGC, 0x0);
	else if (pga < 0xc)
		data->limited_pga = 1;

	return RTW_DPK_GAIN_CHECK;
}

static u8 rtw8822c_gain_less_state(struct rtw_dev *rtwdev,
				   struct rtw8822c_dpk_data *data)
{
	u8 pga = data->pga;

	if (pga < 0xc)
		rtw_write_rf(rtwdev, data->path, RF_MODE_TRXAGC, BIT_RXAGC, 0xc);
	else if (pga > 0xb && pga < 0xf)
		rtw_write_rf(rtwdev, data->path, RF_MODE_TRXAGC, BIT_RXAGC, 0xf);
	else if (pga > 0xe)
		data->limited_pga = 1;

	return RTW_DPK_GAIN_CHECK;
}

static u32 rtw8822c_get_path_read_addr(u8 path)
{
	u32 base_addr;

	switch (path) {
	case RF_PATH_A:
		base_addr = 0x2800;
		break;
	case RF_PATH_B:
		base_addr = 0x4500;
		break;
	default:
		NATIVE_WARN_ON(1);
		return -1;
	}

	return base_addr;
}

static u32 rtw8822c_get_path_write_addr(u8 path)
{
	u32 base_addr;

	switch (path) {
	case RF_PATH_A:
		base_addr = 0x1800;
		break;
	case RF_PATH_B:
		base_addr = 0x4100;
		break;
	default:
		NATIVE_WARN_ON(1);
		return -1;
	}

	return base_addr;
}

static u8 rtw8822c_gl_large_state(struct rtw_dev *rtwdev,
				  struct rtw8822c_dpk_data *data)
{
	return rtw8822c_gl_state(rtwdev, data, 1);
}

static u8 rtw8822c_gl_less_state(struct rtw_dev *rtwdev,
				 struct rtw8822c_dpk_data *data)
{
	return rtw8822c_gl_state(rtwdev, data, 0);
}

static u8 rtw8822c_gl_state(struct rtw_dev *rtwdev,
			    struct rtw8822c_dpk_data *data, u8 is_large)
{
	u8 txbb_bound[] = {0x1f, 0};

	if (data->txbb == txbb_bound[is_large])
		return RTW_DPK_AGC_OUT;

	if (is_large == 1)
		data->txbb -= 2;
	else
		data->txbb += 3;

	rtw_write_rf(rtwdev, data->path, RF_TX_GAIN, BIT_GAIN_TXBB, data->txbb);
	data->limited_pga = 0;

	return RTW_DPK_GAIN_CHECK;
}

static u8 rtw8822c_loss_check_state(struct rtw_dev *rtwdev,
				    struct rtw8822c_dpk_data *data)
{
	u8 path = data->path;
	u8 state;

	rtw8822c_dpk_one_shot(rtwdev, path, RTW_DPK_GAIN_LOSS);
	state = rtw8822c_dpk_agc_loss_chk(rtwdev, path);

	return state;
}

static void rtw8822c_pa_bias(struct rtw_dev *rtwdev)
{
	u16 rf_efuse_2g[2] = {PPG_PABIAS_2GA, PPG_PABIAS_2GB};
	u16 rf_efuse_5g[2] = {PPG_PABIAS_5GA, PPG_PABIAS_5GB};
	u8 pg_pa_bias = 0xff, path;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_read8_physical_efuse(rtwdev, rf_efuse_2g[path],
					 &pg_pa_bias);
		if (pg_pa_bias == EFUSE_READ_FAIL)
			return;
		pg_pa_bias = FIELD_GET(PPG_PABIAS_MASK, pg_pa_bias);
		rtw_write_rf(rtwdev, path, RF_PA, RF_PABIAS_2G_MASK, pg_pa_bias);
	}
	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_read8_physical_efuse(rtwdev, rf_efuse_5g[path],
					 &pg_pa_bias);
		pg_pa_bias = FIELD_GET(PPG_PABIAS_MASK, pg_pa_bias);
		rtw_write_rf(rtwdev, path, RF_PA, RF_PABIAS_5G_MASK, pg_pa_bias);
	}
}

static void rtw8822c_parse_tbl_dpk(struct rtw_dev *rtwdev,
			    const struct rtw_table *tbl)
{
	const struct dpk_cfg_pair *p = tbl->data;
	const struct dpk_cfg_pair *end = p + tbl->size / 3;

	BUILD_BUG_ON(sizeof(struct dpk_cfg_pair) != sizeof(u32) * 3);

	for (; p < end; p++)
		rtw_write32_mask(rtwdev, p->addr, p->bitmask, p->data);
}

static void rtw8822c_phy_calibration(struct rtw_dev *rtwdev)
{
	rtw8822c_rfk_power_save(rtwdev, false);
	rtw8822c_do_gapk(rtwdev);
	rtw8822c_do_iqk(rtwdev);
	rtw8822c_do_dpk(rtwdev);
	rtw8822c_rfk_power_save(rtwdev, true);
}

static void rtw8822c_power_trim(struct rtw_dev *rtwdev)
{
	u8 pg_pwr = 0xff, i, path, idx;
	s8 bb_gain[2][8] = {};
	u16 rf_efuse_2g[3] = {PPG_2GL_TXAB, PPG_2GM_TXAB, PPG_2GH_TXAB};
	u16 rf_efuse_5g[2][5] = {{PPG_5GL1_TXA, PPG_5GL2_TXA, PPG_5GM1_TXA,
				  PPG_5GM2_TXA, PPG_5GH1_TXA},
				 {PPG_5GL1_TXB, PPG_5GL2_TXB, PPG_5GM1_TXB,
				  PPG_5GM2_TXB, PPG_5GH1_TXB} };
	bool set = false;

	for (i = 0; i < ARRAY_SIZE(rf_efuse_2g); i++) {
		rtw_read8_physical_efuse(rtwdev, rf_efuse_2g[i], &pg_pwr);
		if (pg_pwr == EFUSE_READ_FAIL)
			continue;
		set = true;
		bb_gain[RF_PATH_A][i] = FIELD_GET(PPG_2G_A_MASK, pg_pwr);
		bb_gain[RF_PATH_B][i] = FIELD_GET(PPG_2G_B_MASK, pg_pwr);
	}

	for (i = 0; i < ARRAY_SIZE(rf_efuse_5g[0]); i++) {
		for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
			rtw_read8_physical_efuse(rtwdev, rf_efuse_5g[path][i],
						 &pg_pwr);
			if (pg_pwr == EFUSE_READ_FAIL)
				continue;
			set = true;
			idx = i + ARRAY_SIZE(rf_efuse_2g);
			bb_gain[path][idx] = FIELD_GET(PPG_5G_MASK, pg_pwr);
		}
	}
	if (set)
		rtw8822c_set_power_trim(rtwdev, bb_gain);

	rtw_write32_mask(rtwdev, REG_DIS_DPD, DIS_DPD_MASK, DIS_DPD_RATEALL);
}

static u32 rtw8822c_psd_log2base(u32 val)
{
	u32 tmp, val_integerd_b, tindex;
	u32 result, val_fractiond_b;
	u32 table_fraction[21] = {0, 432, 332, 274, 232, 200, 174,
				  151, 132, 115, 100, 86, 74, 62, 51,
				  42, 32, 23, 15, 7, 0};

	if (val == 0)
		return 0;

	val_integerd_b = __fls(val) + 1;

	tmp = ((u64)val * 100U) / (UINT64_C(1) << val_integerd_b);
	tindex = tmp / 5;

	if (tindex >= ARRAY_SIZE(table_fraction))
		tindex = ARRAY_SIZE(table_fraction) - 1;

	val_fractiond_b = table_fraction[tindex];

	result = val_integerd_b * 100 - val_fractiond_b;

	return result;
}

static void rtw8822c_pwr_track(struct rtw_dev *rtwdev)
{
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	if (efuse->power_track_type != 0)
		return;

	if (!dm_info->pwr_trk_triggered) {
		rtw_write_rf(rtwdev, RF_PATH_A, RF_T_METER, BIT(19), 0x01);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_T_METER, BIT(19), 0x00);
		rtw_write_rf(rtwdev, RF_PATH_A, RF_T_METER, BIT(19), 0x01);

		rtw_write_rf(rtwdev, RF_PATH_B, RF_T_METER, BIT(19), 0x01);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_T_METER, BIT(19), 0x00);
		rtw_write_rf(rtwdev, RF_PATH_B, RF_T_METER, BIT(19), 0x01);

		dm_info->pwr_trk_triggered = true;
		return;
	}

	__rtw8822c_pwr_track(rtwdev);
	dm_info->pwr_trk_triggered = false;
}

static void rtw8822c_pwr_track_path(struct rtw_dev *rtwdev,
				    struct rtw_swing_table *swing_table,
				    u8 path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 delta;

	delta = rtw_phy_pwrtrack_get_delta(rtwdev, path);
	dm_info->delta_power_index[path] =
		rtw_phy_pwrtrack_get_pwridx(rtwdev, swing_table, path, path,
					    delta);
	rtw8822c_pwrtrack_set(rtwdev, path);
}

static void rtw8822c_pwr_track_stats(struct rtw_dev *rtwdev, u8 path)
{
	u8 thermal_value;

	if (rtwdev->efuse.thermal_meter[path] == 0xff)
		return;

	thermal_value = rtw_read_rf(rtwdev, path, RF_T_METER, 0x7e);
	rtw_phy_pwrtrack_avg(rtwdev, thermal_value, path);
}

static void rtw8822c_pwrtrack_init(struct rtw_dev *rtwdev)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 path;

	for (path = RF_PATH_A; path < RTW_RF_PATH_MAX; path++) {
		dm_info->delta_power_index[path] = 0;
		ewma_thermal_init(&dm_info->avg_thermal[path]);
		dm_info->thermal_avg[path] = 0xff;
	}

	dm_info->pwr_trk_triggered = false;
	dm_info->thermal_meter_k = rtwdev->efuse.thermal_meter_k;
	dm_info->thermal_meter_lck = rtwdev->efuse.thermal_meter_k;
}

static void rtw8822c_pwrtrack_set(struct rtw_dev *rtwdev, u8 rf_path)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	switch (rf_path) {
	case RF_PATH_A:
		rtw_write32_mask(rtwdev, 0x18a0, PWR_TRACK_MASK,
				 dm_info->delta_power_index[rf_path]);
		break;
	case RF_PATH_B:
		rtw_write32_mask(rtwdev, 0x41a0, PWR_TRACK_MASK,
				 dm_info->delta_power_index[rf_path]);
		break;
	default:
		break;
	}
}

static void rtw8822c_rf_dac_cal(struct rtw_dev *rtwdev)
{
	struct rtw_backup_info backup_rf[DACK_RF_8822C * DACK_PATH_8822C];
	struct rtw_backup_info backup[DACK_REG_8822C];
	u32 ic = 0, qc = 0, i;
	u32 i_a = 0x0, q_a = 0x0, i_b = 0x0, q_b = 0x0;
	u32 ic_a = 0x0, qc_a = 0x0, ic_b = 0x0, qc_b = 0x0;
	u32 adc_ic_a = 0x0, adc_qc_a = 0x0, adc_ic_b = 0x0, adc_qc_b = 0x0;

	if (rtw8822c_dac_cal_restore(rtwdev)){rtwdev->dac_complete=1;return;}

	/* not able to restore, do it */

	rtw8822c_dac_backup_reg(rtwdev, backup, backup_rf);

	rtw8822c_dac_bb_setting(rtwdev);

	/* path-A */
	rtw8822c_dac_cal_adc(rtwdev, RF_PATH_A, &adc_ic_a, &adc_qc_a);
	for (i = 0; i < 10; i++) {
		rtw8822c_dac_cal_step1(rtwdev, RF_PATH_A);
		rtw8822c_dac_cal_step2(rtwdev, RF_PATH_A, &ic, &qc);
		ic_a = ic;
		qc_a = qc;

		rtw8822c_dac_cal_step3(rtwdev, RF_PATH_A, adc_ic_a, adc_qc_a,
				       &ic, &qc, &i_a, &q_a);

		if (ic < 5 && qc < 5)
			break;
	}
	rtwdev->dac_a_complete=(ic<5&&qc<5);
	rtw8822c_dac_cal_step4(rtwdev, RF_PATH_A);

	/* path-B */
	rtw8822c_dac_cal_adc(rtwdev, RF_PATH_B, &adc_ic_b, &adc_qc_b);
	for (i = 0; i < 10; i++) {
		rtw8822c_dac_cal_step1(rtwdev, RF_PATH_B);
		rtw8822c_dac_cal_step2(rtwdev, RF_PATH_B, &ic, &qc);
		ic_b = ic;
		qc_b = qc;

		rtw8822c_dac_cal_step3(rtwdev, RF_PATH_B, adc_ic_b, adc_qc_b,
				       &ic, &qc, &i_b, &q_b);

		if (ic < 5 && qc < 5)
			break;
	}
	rtwdev->dac_complete=(rtwdev->dac_a_complete&&ic<5&&qc<5&&!native_error(rtwdev));
	rtw8822c_dac_cal_step4(rtwdev, RF_PATH_B);

	rtw_write32(rtwdev, 0x1b00, 0x00000008);
	rtw_write32_mask(rtwdev, 0x4130, BIT(30), 0x1);
	rtw_write8(rtwdev, 0x1bcc, 0x0);
	rtw_write32(rtwdev, 0x1b00, 0x0000000a);
	rtw_write8(rtwdev, 0x1bcc, 0x0);

	rtw8822c_dac_restore_reg(rtwdev, backup, backup_rf);

	/* backup results to restore, saving a lot of time */
	if(rtwdev->dac_complete&&!native_error(rtwdev))rtw8822c_dac_cal_backup(rtwdev);else native_fail(rtwdev,EIO);

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] path A: ic=0x%x, qc=0x%x\n", ic_a, qc_a);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] path B: ic=0x%x, qc=0x%x\n", ic_b, qc_b);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] path A: i=0x%x, q=0x%x\n", i_a, q_a);
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[DACK] path B: i=0x%x, q=0x%x\n", i_b, q_b);
}

static void rtw8822c_rf_init(struct rtw_dev *rtwdev)
{
	rtw8822c_rf_dac_cal(rtwdev);
	rtw8822c_rf_x2_check(rtwdev);
	rtw8822c_thermal_trim(rtwdev);
	rtw8822c_power_trim(rtwdev);
	rtw8822c_pa_bias(rtwdev);
}

static void rtw8822c_rf_minmax_cmp(struct rtw_dev *rtwdev, u32 value,
				   u32 *min, u32 *max)
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

		if (*max  >= 0x200) {
			*max = value;
		} else {
			if (*max < value)
				*max = value;
		}
	}
}

static void rtw8822c_rf_x2_check(struct rtw_dev *rtwdev)
{
	u8 x2k_busy;

	mdelay(1);
	x2k_busy = rtw_read_rf(rtwdev, RF_PATH_A, 0xb8, BIT(15));
	if (x2k_busy == 1) {
		rtw_write_rf(rtwdev, RF_PATH_A, 0xb8, RFREG_MASK, 0xC4440);
		rtw_write_rf(rtwdev, RF_PATH_A, 0xba, RFREG_MASK, 0x6840D);
		rtw_write_rf(rtwdev, RF_PATH_A, 0xb8, RFREG_MASK, 0x80440);
		mdelay(1);
	}
}

static void rtw8822c_rfk_handshake(struct rtw_dev *rtwdev, bool is_before_k)
{
	struct rtw_dm_info *dm = &rtwdev->dm_info;
	u8 u1b_tmp;
	u8 u4b_tmp;
	int ret;

	if (is_before_k) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[RFK] WiFi / BT RFK handshake start!!\n");

		if (!dm->is_bt_iqk_timeout) {
			ret = read_poll_timeout(rtw_read32_mask, u4b_tmp,
						u4b_tmp == 0, 20, 600000, false,
						rtwdev, REG_PMC_DBG_CTRL1,
						BITS_PMC_BT_IQK_STS);
			if (ret) {
				rtw_dbg(rtwdev, RTW_DBG_RFK,
					"[RFK] Wait BT IQK finish timeout!!\n");
				dm->is_bt_iqk_timeout = true;
			}
		}

		rtw_fw_inform_rfk_status(rtwdev, true);

		ret = read_poll_timeout(rtw_read8_mask, u1b_tmp,
					u1b_tmp == 1, 20, 100000, false,
					rtwdev, REG_ARFR4, BIT_WL_RFK);
		if (ret)
			rtw_dbg(rtwdev, RTW_DBG_RFK,
				"[RFK] Send WiFi RFK start H2C cmd FAIL!!\n");
	} else {
		++rtwdev->restore_depth;
		rtw_fw_inform_rfk_status(rtwdev, false);
		ret = read_poll_timeout(rtw_read8_mask, u1b_tmp,
					u1b_tmp == 1, 20, 100000, false,
					rtwdev, REG_ARFR4,
					BIT_WL_RFK);
		if (ret)
			rtw_dbg(rtwdev, RTW_DBG_RFK,
				"[RFK] Send WiFi RFK finish H2C cmd FAIL!!\n");

		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[RFK] WiFi / BT RFK handshake finish!!\n");
		--rtwdev->restore_depth;
	}
}

static void rtw8822c_rfk_power_save(struct rtw_dev *rtwdev,
				    bool is_power_save)
{
	u8 path;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, path);
		rtw_write32_mask(rtwdev, REG_DPD_CTL1_S0, BIT_PS_EN,
				 is_power_save ? 0 : 1);
	}
}

static void rtw8822c_rstb_3wire(struct rtw_dev *rtwdev, bool enable)
{
	if (enable) {
		rtw_write32_mask(rtwdev, REG_RSTB, BIT_RSTB_3WIRE, 0x1);
		rtw_write32_mask(rtwdev, REG_ANAPAR_A, BIT_ANAPAR_UPDATE, 0x1);
		rtw_write32_mask(rtwdev, REG_ANAPAR_B, BIT_ANAPAR_UPDATE, 0x1);
	} else {
		rtw_write32_mask(rtwdev, REG_RSTB, BIT_RSTB_3WIRE, 0x0);
	}
}

static void rtw8822c_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx)
{
	rtw8822c_set_channel_bb(rtwdev, channel, bw, primary_chan_idx);
	rtw_set_channel_mac(rtwdev, channel, bw, primary_chan_idx);
	rtw8822c_set_channel_rf(rtwdev, channel, bw);
	rtw8822c_toggle_igi(rtwdev);
}

static void rtw8822c_set_channel_bb(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				    u8 primary_ch_idx)
{
	if (IS_CH_2G_BAND(channel)) {
		rtw_write32_clr(rtwdev, REG_BGCTRL, BITS_RX_IQ_WEIGHT);
		rtw_write32_set(rtwdev, REG_TXF4, BIT(20));
		rtw_write32_clr(rtwdev, REG_CCK_CHECK, BIT_CHECK_CCK_EN);
		rtw_write32_clr(rtwdev, REG_CCKTXONLY, BIT_BB_CCK_CHECK_EN);
		rtw_write32_mask(rtwdev, REG_CCAMSK, 0x3F000000, 0xF);

		switch (bw) {
		case RTW_CHANNEL_WIDTH_20:
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_CCK,
					 0x5);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_CCK,
					 0x5);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_OFDM,
					 0x6);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_OFDM,
					 0x6);
			break;
		case RTW_CHANNEL_WIDTH_40:
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_CCK,
					 0x4);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_CCK,
					 0x4);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_OFDM,
					 0x0);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_OFDM,
					 0x0);
			break;
		}
		if (channel == 13 || channel == 14)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x969);
		else if (channel == 11 || channel == 12)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x96a);
		else
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x9aa);
		if (channel == 14) {
			rtw_write32_mask(rtwdev, REG_TXF0, MASKHWORD, 0x3da0);
			rtw_write32_mask(rtwdev, REG_TXF1, MASKDWORD,
					 0x4962c931);
			rtw_write32_mask(rtwdev, REG_TXF2, MASKLWORD, 0x6aa3);
			rtw_write32_mask(rtwdev, REG_TXF3, MASKHWORD, 0xaa7b);
			rtw_write32_mask(rtwdev, REG_TXF4, MASKLWORD, 0xf3d7);
			rtw_write32_mask(rtwdev, REG_TXF5, MASKDWORD, 0x0);
			rtw_write32_mask(rtwdev, REG_TXF6, MASKDWORD,
					 0xff012455);
			rtw_write32_mask(rtwdev, REG_TXF7, MASKDWORD, 0xffff);
		} else {
			rtw_write32_mask(rtwdev, REG_TXF0, MASKHWORD, 0x5284);
			rtw_write32_mask(rtwdev, REG_TXF1, MASKDWORD,
					 0x3e18fec8);
			rtw_write32_mask(rtwdev, REG_TXF2, MASKLWORD, 0x0a88);
			rtw_write32_mask(rtwdev, REG_TXF3, MASKHWORD, 0xacc4);
			rtw_write32_mask(rtwdev, REG_TXF4, MASKLWORD, 0xc8b2);
			rtw_write32_mask(rtwdev, REG_TXF5, MASKDWORD,
					 0x00faf0de);
			rtw_write32_mask(rtwdev, REG_TXF6, MASKDWORD,
					 0x00122344);
			rtw_write32_mask(rtwdev, REG_TXF7, MASKDWORD,
					 0x0fffffff);
		}
		if (channel == 13)
			rtw_write32_mask(rtwdev, REG_TXDFIR0, 0x70, 0x3);
		else
			rtw_write32_mask(rtwdev, REG_TXDFIR0, 0x70, 0x1);
	} else if (IS_CH_5G_BAND(channel)) {
		rtw_write32_set(rtwdev, REG_CCKTXONLY, BIT_BB_CCK_CHECK_EN);
		rtw_write32_set(rtwdev, REG_CCK_CHECK, BIT_CHECK_CCK_EN);
		rtw_write32_set(rtwdev, REG_BGCTRL, BITS_RX_IQ_WEIGHT);
		rtw_write32_clr(rtwdev, REG_TXF4, BIT(20));
		rtw_write32_mask(rtwdev, REG_CCAMSK, 0x3F000000, 0x22);
		rtw_write32_mask(rtwdev, REG_TXDFIR0, 0x70, 0x3);
		if (IS_CH_5G_BAND_1(channel) || IS_CH_5G_BAND_2(channel)) {
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_OFDM,
					 0x1);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_OFDM,
					 0x1);
		} else if (IS_CH_5G_BAND_3(channel)) {
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_OFDM,
					 0x2);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_OFDM,
					 0x2);
		} else if (IS_CH_5G_BAND_4(channel)) {
			rtw_write32_mask(rtwdev, REG_RXAGCCTL0, BITS_RXAGC_OFDM,
					 0x3);
			rtw_write32_mask(rtwdev, REG_RXAGCCTL, BITS_RXAGC_OFDM,
					 0x3);
		}

		if (channel >= 36 && channel <= 51)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x494);
		else if (channel >= 52 && channel <= 55)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x493);
		else if (channel >= 56 && channel <= 111)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x453);
		else if (channel >= 112 && channel <= 119)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x452);
		else if (channel >= 120 && channel <= 172)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x412);
		else if (channel >= 173 && channel <= 177)
			rtw_write32_mask(rtwdev, REG_SCOTRK, 0xfff, 0x411);
	}

	switch (bw) {
	case RTW_CHANNEL_WIDTH_20:
		rtw_write32_mask(rtwdev, REG_DFIRBW, 0x3FF0, 0x19B);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xf, 0x0);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xffc0, 0x0);
		rtw_write32_mask(rtwdev, REG_TXCLK, 0x700, 0x7);
		rtw_write32_mask(rtwdev, REG_TXCLK, 0x700000, 0x6);
		rtw_write32_mask(rtwdev, REG_CCK_SOURCE, BIT_NBI_EN, 0x0);
		rtw_write32_mask(rtwdev, REG_SBD, BITS_SUBTUNE, 0x1);
		rtw_write32_mask(rtwdev, REG_PT_CHSMO, BIT_PT_OPT, 0x0);
		break;
	case RTW_CHANNEL_WIDTH_40:
		rtw_write32_mask(rtwdev, REG_CCKSB, BIT(4),
				 (primary_ch_idx == RTW_SC_20_UPPER ? 1 : 0));
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xf, 0x5);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xc0, 0x0);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xff00,
				 (primary_ch_idx | (primary_ch_idx << 4)));
		rtw_write32_mask(rtwdev, REG_CCK_SOURCE, BIT_NBI_EN, 0x1);
		rtw_write32_mask(rtwdev, REG_SBD, BITS_SUBTUNE, 0x1);
		rtw_write32_mask(rtwdev, REG_PT_CHSMO, BIT_PT_OPT, 0x1);
		break;
	case RTW_CHANNEL_WIDTH_80:
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xf, 0xa);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xc0, 0x0);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xff00,
				 (primary_ch_idx | (primary_ch_idx << 4)));
		rtw_write32_mask(rtwdev, REG_SBD, BITS_SUBTUNE, 0x6);
		rtw_write32_mask(rtwdev, REG_PT_CHSMO, BIT_PT_OPT, 0x1);
		break;
	case RTW_CHANNEL_WIDTH_5:
		rtw_write32_mask(rtwdev, REG_DFIRBW, 0x3FF0, 0x2AB);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xf, 0x0);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xffc0, 0x1);
		rtw_write32_mask(rtwdev, REG_TXCLK, 0x700, 0x4);
		rtw_write32_mask(rtwdev, REG_TXCLK, 0x700000, 0x4);
		rtw_write32_mask(rtwdev, REG_CCK_SOURCE, BIT_NBI_EN, 0x0);
		rtw_write32_mask(rtwdev, REG_SBD, BITS_SUBTUNE, 0x1);
		rtw_write32_mask(rtwdev, REG_PT_CHSMO, BIT_PT_OPT, 0x0);
		break;
	case RTW_CHANNEL_WIDTH_10:
		rtw_write32_mask(rtwdev, REG_DFIRBW, 0x3FF0, 0x2AB);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xf, 0x0);
		rtw_write32_mask(rtwdev, REG_TXBWCTL, 0xffc0, 0x2);
		rtw_write32_mask(rtwdev, REG_TXCLK, 0x700, 0x6);
		rtw_write32_mask(rtwdev, REG_TXCLK, 0x700000, 0x5);
		rtw_write32_mask(rtwdev, REG_CCK_SOURCE, BIT_NBI_EN, 0x0);
		rtw_write32_mask(rtwdev, REG_SBD, BITS_SUBTUNE, 0x1);
		rtw_write32_mask(rtwdev, REG_PT_CHSMO, BIT_PT_OPT, 0x0);
		break;
	}
}

static void rtw8822c_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw)
{
#define RF18_BAND_MASK		(BIT(16) | BIT(9) | BIT(8))
#define RF18_BAND_2G		(0)
#define RF18_BAND_5G		(BIT(16) | BIT(8))
#define RF18_CHANNEL_MASK	(MASKBYTE0)
#define RF18_RFSI_MASK		(BIT(18) | BIT(17))
#define RF18_RFSI_GE_CH80	(BIT(17))
#define RF18_RFSI_GT_CH140	(BIT(18))
#define RF18_BW_MASK		(BIT(13) | BIT(12))
#define RF18_BW_20M		(BIT(13) | BIT(12))
#define RF18_BW_40M		(BIT(13))
#define RF18_BW_80M		(BIT(12))

	u32 rf_reg18 = 0;
	u32 rf_rxbb = 0;

	rf_reg18 = rtw_read_rf(rtwdev, RF_PATH_A, 0x18, RFREG_MASK);

	rf_reg18 &= ~(RF18_BAND_MASK | RF18_CHANNEL_MASK | RF18_RFSI_MASK |
		      RF18_BW_MASK);

	rf_reg18 |= (IS_CH_2G_BAND(channel) ? RF18_BAND_2G : RF18_BAND_5G);
	rf_reg18 |= (channel & RF18_CHANNEL_MASK);
	if (IS_CH_5G_BAND_4(channel))
		rf_reg18 |= RF18_RFSI_GT_CH140;
	else if (IS_CH_5G_BAND_3(channel))
		rf_reg18 |= RF18_RFSI_GE_CH80;

	switch (bw) {
	case RTW_CHANNEL_WIDTH_5:
	case RTW_CHANNEL_WIDTH_10:
	case RTW_CHANNEL_WIDTH_20:
	default:
		rf_reg18 |= RF18_BW_20M;
		rf_rxbb = 0x18;
		break;
	case RTW_CHANNEL_WIDTH_40:
		/* RF bandwidth */
		rf_reg18 |= RF18_BW_40M;
		rf_rxbb = 0x10;
		break;
	case RTW_CHANNEL_WIDTH_80:
		rf_reg18 |= RF18_BW_80M;
		rf_rxbb = 0x8;
		break;
	}

	rtw8822c_rstb_3wire(rtwdev, false);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, 0x04, 0x01);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWA, 0x1f, 0x12);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWD0, 0xfffff, rf_rxbb);
	rtw_write_rf(rtwdev, RF_PATH_A, RF_LUTWE2, 0x04, 0x00);

	rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWE2, 0x04, 0x01);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWA, 0x1f, 0x12);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWD0, 0xfffff, rf_rxbb);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_LUTWE2, 0x04, 0x00);

	rtw_write_rf(rtwdev, RF_PATH_A, RF_CFGCH, RFREG_MASK, rf_reg18);
	rtw_write_rf(rtwdev, RF_PATH_B, RF_CFGCH, RFREG_MASK, rf_reg18);

	rtw8822c_rstb_3wire(rtwdev, true);
}

static void rtw8822c_set_power_trim(struct rtw_dev *rtwdev, s8 bb_gain[2][8])
{
#define RF_SET_POWER_TRIM(_path, _seq, _idx)					\
		do {								\
			rtw_write_rf(rtwdev, _path, 0x33, RFREG_MASK, _seq);	\
			rtw_write_rf(rtwdev, _path, 0x3f, RFREG_MASK,		\
				     bb_gain[_path][_idx]);			\
		} while (0)
	u8 path;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_write_rf(rtwdev, path, 0xee, BIT(19), 1);
		RF_SET_POWER_TRIM(path, 0x0, 0);
		RF_SET_POWER_TRIM(path, 0x1, 1);
		RF_SET_POWER_TRIM(path, 0x2, 2);
		RF_SET_POWER_TRIM(path, 0x3, 2);
		RF_SET_POWER_TRIM(path, 0x4, 3);
		RF_SET_POWER_TRIM(path, 0x5, 4);
		RF_SET_POWER_TRIM(path, 0x6, 5);
		RF_SET_POWER_TRIM(path, 0x7, 6);
		RF_SET_POWER_TRIM(path, 0x8, 7);
		RF_SET_POWER_TRIM(path, 0x9, 3);
		RF_SET_POWER_TRIM(path, 0xa, 4);
		RF_SET_POWER_TRIM(path, 0xb, 5);
		RF_SET_POWER_TRIM(path, 0xc, 6);
		RF_SET_POWER_TRIM(path, 0xd, 7);
		RF_SET_POWER_TRIM(path, 0xe, 7);
		rtw_write_rf(rtwdev, path, 0xee, BIT(19), 0);
	}
#undef RF_SET_POWER_TRIM
}

static void rtw8822c_set_tx_power_diff(struct rtw_dev *rtwdev, u8 rate,
				       s8 *diff_idx)
{
	u32 offset_txagc = 0x3a00;
	u8 rate_idx = rate & 0xfc;
	u8 pwr_idx[4];
	u32 phy_pwr_idx;
	int i;

	for (i = 0; i < 4; i++)
		pwr_idx[i] = diff_idx[i] & 0x7f;

	phy_pwr_idx = pwr_idx[0] |
		      (pwr_idx[1] << 8) |
		      (pwr_idx[2] << 16) |
		      (pwr_idx[3] << 24);

	rtw_write32_mask(rtwdev, 0x1c90, BIT(15), 0x0);
	rtw_write32_mask(rtwdev, offset_txagc + rate_idx, MASKDWORD,
			 phy_pwr_idx);
}

static void rtw8822c_set_tx_power_index(struct rtw_dev *rtwdev)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u8 rs, rate, j;
	u8 pwr_ref_cck[2] = {hal->tx_pwr_tbl[RF_PATH_A][DESC_RATE11M],
			     hal->tx_pwr_tbl[RF_PATH_B][DESC_RATE11M]};
	u8 pwr_ref_ofdm[2] = {hal->tx_pwr_tbl[RF_PATH_A][DESC_RATEMCS7],
			      hal->tx_pwr_tbl[RF_PATH_B][DESC_RATEMCS7]};
	s8 diff_a, diff_b;
	u8 pwr_a, pwr_b;
	s8 diff_idx[4];

	rtw8822c_set_write_tx_power_ref(rtwdev, pwr_ref_cck, pwr_ref_ofdm);
	for (rs = 0; rs <= __RTW_RATE_SECTION_2SS_MAX; rs++) {
		for (j = 0; j < rtw_rate_size[rs]; j++) {
			rate = rtw_rate_section[rs][j];
			pwr_a = hal->tx_pwr_tbl[RF_PATH_A][rate];
			pwr_b = hal->tx_pwr_tbl[RF_PATH_B][rate];
			if (rs == 0) {
				diff_a = (s8)pwr_a - (s8)pwr_ref_cck[0];
				diff_b = (s8)pwr_b - (s8)pwr_ref_cck[1];
			} else {
				diff_a = (s8)pwr_a - (s8)pwr_ref_ofdm[0];
				diff_b = (s8)pwr_b - (s8)pwr_ref_ofdm[1];
			}
			diff_idx[rate % 4] = min(diff_a, diff_b);
			if (rate % 4 == 3)
				rtw8822c_set_tx_power_diff(rtwdev, rate - 3,
							   diff_idx);
		}
	}
}

static void
rtw8822c_set_write_tx_power_ref(struct rtw_dev *rtwdev, u8 *tx_pwr_ref_cck,
				u8 *tx_pwr_ref_ofdm)
{
	struct rtw_hal *hal = &rtwdev->hal;
	u32 txref_cck[2] = {0x18a0, 0x41a0};
	u32 txref_ofdm[2] = {0x18e8, 0x41e8};
	u8 path;

	for (path = 0; path < hal->rf_path_num; path++) {
		rtw_write32_mask(rtwdev, 0x1c90, BIT(15), 0);
		rtw_write32_mask(rtwdev, txref_cck[path], 0x7f0000,
				 tx_pwr_ref_cck[path]);
	}
	for (path = 0; path < hal->rf_path_num; path++) {
		rtw_write32_mask(rtwdev, 0x1c90, BIT(15), 0);
		rtw_write32_mask(rtwdev, txref_ofdm[path], 0x1fc00,
				 tx_pwr_ref_ofdm[path]);
	}
}

static void rtw8822c_thermal_trim(struct rtw_dev *rtwdev)
{
	u16 rf_efuse[2] = {PPG_THERMAL_A, PPG_THERMAL_B};
	u8 pg_therm = 0xff, thermal[2] = {0}, path;

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		rtw_read8_physical_efuse(rtwdev, rf_efuse[path], &pg_therm);
		if (pg_therm == EFUSE_READ_FAIL)
			return;
		/* Efuse value of BIT(0) shall be move to BIT(3), and the value
		 * of BIT(1) to BIT(3) should be right shifted 1 bit.
		 */
		thermal[path] = FIELD_GET(GENMASK(3, 1), pg_therm);
		thermal[path] |= FIELD_PREP(BIT(3), pg_therm & BIT(0));
		rtw_write_rf(rtwdev, path, 0x43, RF_THEMAL_MASK, thermal[path]);
	}
}

static void rtw8822c_toggle_igi(struct rtw_dev *rtwdev)
{
	u32 igi;

	igi = rtw_read32_mask(rtwdev, REG_RXIGI, 0x7f);
	rtw_write32_mask(rtwdev, REG_RXIGI, 0x7f, igi - 2);
	rtw_write32_mask(rtwdev, REG_RXIGI, 0x7f00, igi - 2);
	rtw_write32_mask(rtwdev, REG_RXIGI, 0x7f, igi);
	rtw_write32_mask(rtwdev, REG_RXIGI, 0x7f00, igi);
}

static void rtw8822c_txgapk(struct rtw_dev *rtwdev)
{
	static const u32 bb_reg[2] = {REG_TX_PTCL_CTRL, REG_TX_FIFO};
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	u32 bb_reg_backup[2];
	u8 path;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	rtw8822c_txgapk_save_all_tx_gain_table(rtwdev);

	if (txgapk->read_txgain == 0) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[TXGAPK] txgapk->read_txgain == 0 return!!!\n");
		return;
	}

	if (rtwdev->efuse.power_track_type >= 4 &&
	    rtwdev->efuse.power_track_type <= 7) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[TXGAPK] Normal Mode in TSSI mode. return!!!\n");
		return;
	}

	rtw8822c_txgapk_backup_bb_reg(rtwdev, bb_reg,
				      bb_reg_backup, ARRAY_SIZE(bb_reg));
	rtw8822c_txgapk_tx_pause(rtwdev);
	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		txgapk->channel = rtw_read_rf(rtwdev, path,
					      RF_CFGCH, RFREG_MASK) & MASKBYTE0;
		rtw8822c_txgapk_bb_dpk(rtwdev, path);
		rtw8822c_txgapk_afe_dpk(rtwdev, path);
		rtw8822c_txgapk_calculate_offset(rtwdev, path);
		rtw8822c_txgapk_rf_restore(rtwdev, path);
		rtw8822c_txgapk_afe_dpk_restore(rtwdev, path);
		rtw8822c_txgapk_bb_dpk_restore(rtwdev, path);
	}
	rtw8822c_txgapk_write_tx_gain(rtwdev);
	rtw8822c_txgapk_reload_bb_reg(rtwdev, bb_reg,
				      bb_reg_backup, ARRAY_SIZE(bb_reg));
}

static void rtw8822c_txgapk_afe_dpk(struct rtw_dev *rtwdev, u8 path)
{
	u32 reg;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		reg = REG_ANAPAR_A;
	} else if (path == RF_PATH_B) {
		reg = REG_ANAPAR_B;
	} else {
		rtw_err(rtwdev, "[TXGAPK] unknown path %d!!\n", path);
		return;
	}

	rtw_write32_mask(rtwdev, REG_IQK_CTRL, MASKDWORD, MASKDWORD);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x700f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x700f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x701f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x702f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x703f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x704f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x705f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x706f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x707f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x708f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x709f0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70af0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70bf0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70cf0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70df0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70ef0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70ff0001);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70ff0001);
}

static void rtw8822c_txgapk_afe_dpk_restore_restore_impl(struct rtw_dev *rtwdev, u8 path)
{
	u32 reg;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		reg = REG_ANAPAR_A;
	} else if (path == RF_PATH_B) {
		reg = REG_ANAPAR_B;
	} else {
		rtw_err(rtwdev, "[TXGAPK] unknown path %d!!\n", path);
		return;
	}
	rtw_write32_mask(rtwdev, REG_IQK_CTRL, MASKDWORD, 0xffa1005e);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x700b8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70144041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70244041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70344041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70444041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x705b8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70644041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x707b8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x708b8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x709b8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70ab8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70bb8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70cb8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70db8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70eb8041);
	rtw_write32_mask(rtwdev, reg, MASKDWORD, 0x70fb8041);
}
static void rtw8822c_txgapk_afe_dpk_restore(struct rtw_dev *rtwdev, u8 path)
{++rtwdev->restore_depth;rtw8822c_txgapk_afe_dpk_restore_restore_impl(rtwdev,path);--rtwdev->restore_depth;}

static void rtw8822c_txgapk_backup_bb_reg(struct rtw_dev *rtwdev, const u32 reg[],
					  u32 reg_backup[], u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = rtw_read32(rtwdev, reg[i]);

		rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] Backup BB 0x%x = 0x%x\n",
			reg[i], reg_backup[i]);
	}
}

static void rtw8822c_txgapk_bb_dpk(struct rtw_dev *rtwdev, u8 path)
{
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	rtw_write32_mask(rtwdev, REG_ENFN, BIT_IQK_DPK_EN, 0x1);
	rtw_write32_mask(rtwdev, REG_CH_DELAY_EXTR2,
			 BIT_IQK_DPK_CLOCK_SRC, 0x1);
	rtw_write32_mask(rtwdev, REG_CH_DELAY_EXTR2,
			 BIT_IQK_DPK_RESET_SRC, 0x1);
	rtw_write32_mask(rtwdev, REG_CH_DELAY_EXTR2, BIT_EN_IOQ_IQK_DPK, 0x1);
	rtw_write32_mask(rtwdev, REG_CH_DELAY_EXTR2, BIT_TST_IQK2SET_SRC, 0x0);
	rtw_write32_mask(rtwdev, REG_CCA_OFF, BIT_CCA_ON_BY_PW, 0x1ff);

	if (path == RF_PATH_A) {
		rtw_write32_mask(rtwdev, REG_RFTXEN_GCK_A,
				 BIT_RFTXEN_GCK_FORCE_ON, 0x1);
		rtw_write32_mask(rtwdev, REG_3WIRE, BIT_DIS_SHARERX_TXGAT, 0x1);
		rtw_write32_mask(rtwdev, REG_DIS_SHARE_RX_A,
				 BIT_TX_SCALE_0DB, 0x1);
		rtw_write32_mask(rtwdev, REG_3WIRE, BIT_3WIRE_EN, 0x0);
	} else if (path == RF_PATH_B) {
		rtw_write32_mask(rtwdev, REG_RFTXEN_GCK_B,
				 BIT_RFTXEN_GCK_FORCE_ON, 0x1);
		rtw_write32_mask(rtwdev, REG_3WIRE2,
				 BIT_DIS_SHARERX_TXGAT, 0x1);
		rtw_write32_mask(rtwdev, REG_DIS_SHARE_RX_B,
				 BIT_TX_SCALE_0DB, 0x1);
		rtw_write32_mask(rtwdev, REG_3WIRE2, BIT_3WIRE_EN, 0x0);
	}
	rtw_write32_mask(rtwdev, REG_CCKSB, BIT_BBMODE, 0x2);
}

static void rtw8822c_txgapk_bb_dpk_restore_restore_impl(struct rtw_dev *rtwdev, u8 path)
{
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TX_GAIN, 0x0);
	rtw_write_rf(rtwdev, path, RF_DIS_BYPASS_TXBB, BIT_TIA_BYPASS, 0x0);
	rtw_write_rf(rtwdev, path, RF_DIS_BYPASS_TXBB, BIT_TXBB, 0x0);

	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, 0x0);
	rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x0);
	rtw_write32_mask(rtwdev, REG_SINGLE_TONE_SW, BIT_IRQ_TEST_MODE, 0x0);
	rtw_write32_mask(rtwdev, REG_R_CONFIG, MASKBYTE0, 0x00);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, 0x1);
	rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x0);
	rtw_write32_mask(rtwdev, REG_SINGLE_TONE_SW, BIT_IRQ_TEST_MODE, 0x0);
	rtw_write32_mask(rtwdev, REG_R_CONFIG, MASKBYTE0, 0x00);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, 0x0);
	rtw_write32_mask(rtwdev, REG_CCA_OFF, BIT_CCA_ON_BY_PW, 0x0);

	if (path == RF_PATH_A) {
		rtw_write32_mask(rtwdev, REG_RFTXEN_GCK_A,
				 BIT_RFTXEN_GCK_FORCE_ON, 0x0);
		rtw_write32_mask(rtwdev, REG_3WIRE, BIT_DIS_SHARERX_TXGAT, 0x0);
		rtw_write32_mask(rtwdev, REG_DIS_SHARE_RX_A,
				 BIT_TX_SCALE_0DB, 0x0);
		rtw_write32_mask(rtwdev, REG_3WIRE, BIT_3WIRE_EN, 0x3);
	} else if (path == RF_PATH_B) {
		rtw_write32_mask(rtwdev, REG_RFTXEN_GCK_B,
				 BIT_RFTXEN_GCK_FORCE_ON, 0x0);
		rtw_write32_mask(rtwdev, REG_3WIRE2,
				 BIT_DIS_SHARERX_TXGAT, 0x0);
		rtw_write32_mask(rtwdev, REG_DIS_SHARE_RX_B,
				 BIT_TX_SCALE_0DB, 0x0);
		rtw_write32_mask(rtwdev, REG_3WIRE2, BIT_3WIRE_EN, 0x3);
	}

	rtw_write32_mask(rtwdev, REG_CCKSB, BIT_BBMODE, 0x0);
	rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_CFIR_EN, 0x5);
}
static void rtw8822c_txgapk_bb_dpk_restore(struct rtw_dev *rtwdev, u8 path)
{++rtwdev->restore_depth;rtw8822c_txgapk_bb_dpk_restore_restore_impl(rtwdev,path);--rtwdev->restore_depth;}

static u32 rtw8822c_txgapk_cal_gain(struct rtw_dev *rtwdev, u32 gain, s8 offset)
{
	u32 gain_x2, new_gain;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	if (_rtw8822c_txgapk_gain_valid(rtwdev, gain)) {
		new_gain = gain;
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[TXGAPK] gain=0x%03X(>=0xCEX) offset=%d new_gain=0x%03X\n",
			gain, offset, new_gain);
		return new_gain;
	}

	gain_x2 = (gain << 1) + offset;
	new_gain = (gain_x2 >> 1) | (gain_x2 & BIT(0) ? BIT_GAIN_EXT : 0);

	rtw_dbg(rtwdev, RTW_DBG_RFK,
		"[TXGAPK] gain=0x%X offset=%d new_gain=0x%X\n",
		gain, offset, new_gain);

	return new_gain;
}

static void rtw8822c_txgapk_calculate_offset(struct rtw_dev *rtwdev, u8 path)
{
	static const u32 bb_reg[] = {REG_ANTMAP0, REG_TXLGMAP, REG_TXANTSEG,
				     REG_ORITXCODE, REG_ORITXCODE2};
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	u8 channel = txgapk->channel;
	u32 reg_backup[ARRAY_SIZE(bb_reg)] = {0};

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);

	rtw8822c_txgapk_backup_bb_reg(rtwdev, bb_reg,
				      reg_backup, ARRAY_SIZE(bb_reg));

	if (channel >= 1 && channel <= 14) {
		rtw_write32_mask(rtwdev,
				 REG_SINGLE_TONE_SW, BIT_IRQ_TEST_MODE, 0x0);
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, path);
		rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_IQ_SWITCH, 0x3f);
		rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x0);
		rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TX_GAIN, 0x1);
		rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, RFREG_MASK, 0x5000f);
		rtw_write_rf(rtwdev, path, RF_TX_GAIN_OFFSET, BIT_RF_GAIN, 0x0);
		rtw_write_rf(rtwdev, path, RF_RXG_GAIN, BIT_RXG_GAIN, 0x1);
		rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, BIT_RXAGC, 0x0f);
		rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TRXBW, 0x1);
		rtw_write_rf(rtwdev, path, RF_BW_TRXBB, BIT_BW_TXBB, 0x1);
		rtw_write_rf(rtwdev, path, RF_BW_TRXBB, BIT_BW_RXBB, 0x0);
		rtw_write_rf(rtwdev, path, RF_EXT_TIA_BW, BIT_PW_EXT_TIA, 0x1);

		rtw_write32_mask(rtwdev, REG_IQKSTAT, MASKBYTE0, 0x00);
		rtw_write32_mask(rtwdev, REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x0);

		rtw8822c_txgapk_read_offset(rtwdev, path);
		rtw_dbg(rtwdev, RTW_DBG_RFK, "=============================\n");

	} else {
		rtw_write32_mask(rtwdev,
				 REG_SINGLE_TONE_SW, BIT_IRQ_TEST_MODE, 0x0);
		rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, path);
		rtw_write32_mask(rtwdev, REG_R_CONFIG, BIT_IQ_SWITCH, 0x3f);
		rtw_write32_mask(rtwdev, REG_IQK_CTL1, BIT_TX_CFIR, 0x0);
		rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TX_GAIN, 0x1);
		rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, RFREG_MASK, 0x50011);
		rtw_write_rf(rtwdev, path, RF_TXA_LB_SW, BIT_TXA_LB_ATT, 0x3);
		rtw_write_rf(rtwdev, path, RF_TXA_LB_SW, BIT_LB_ATT, 0x3);
		rtw_write_rf(rtwdev, path, RF_TXA_LB_SW, BIT_LB_SW, 0x1);
		rtw_write_rf(rtwdev, path,
			     RF_RXA_MIX_GAIN, BIT_RXA_MIX_GAIN, 0x2);
		rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, BIT_RXAGC, 0x12);
		rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TRXBW, 0x1);
		rtw_write_rf(rtwdev, path, RF_BW_TRXBB, BIT_BW_RXBB, 0x0);
		rtw_write_rf(rtwdev, path, RF_EXT_TIA_BW, BIT_PW_EXT_TIA, 0x1);
		rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, BIT_RF_MODE, 0x5);

		rtw_write32_mask(rtwdev, REG_IQKSTAT, MASKBYTE0, 0x0);

		if (channel >= 36 && channel <= 64)
			rtw_write32_mask(rtwdev,
					 REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x2);
		else if (channel >= 100 && channel <= 144)
			rtw_write32_mask(rtwdev,
					 REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x3);
		else if (channel >= 149 && channel <= 177)
			rtw_write32_mask(rtwdev,
					 REG_TABLE_SEL, BIT_Q_GAIN_SEL, 0x4);

		rtw8822c_txgapk_read_offset(rtwdev, path);
		rtw_dbg(rtwdev, RTW_DBG_RFK, "=============================\n");
	}
	rtw8822c_txgapk_reload_bb_reg(rtwdev, bb_reg,
				      reg_backup, ARRAY_SIZE(bb_reg));
}

static void rtw8822c_txgapk_read_offset(struct rtw_dev *rtwdev, u8 path)
{
	static const u32 cfg1_1b00[2] = {0x00000d18, 0x00000d2a};
	static const u32 cfg2_1b00[2] = {0x00000d19, 0x00000d2b};
	static const u32 set_pi[2] = {REG_RSV_CTRL, REG_WLRF1};
	static const u32 path_setting[2] = {REG_ORITXCODE, REG_ORITXCODE2};
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	u8 channel = txgapk->channel;
	u32 val;
	int i;

	if (path >= ARRAY_SIZE(cfg1_1b00) ||
	    path >= ARRAY_SIZE(cfg2_1b00) ||
	    path >= ARRAY_SIZE(set_pi) ||
	    path >= ARRAY_SIZE(path_setting)) {
		rtw_warn(rtwdev, "[TXGAPK] wrong path %d\n", path);
		return;
	}

	rtw_write32_mask(rtwdev, REG_ANTMAP0, BIT_ANT_PATH, path + 1);
	rtw_write32_mask(rtwdev, REG_TXLGMAP, MASKDWORD, 0xe4e40000);
	rtw_write32_mask(rtwdev, REG_TXANTSEG, BIT_ANTSEG, 0x3);
	rtw_write32_mask(rtwdev, path_setting[path], MASK20BITS, 0x33312);
	rtw_write32_mask(rtwdev, path_setting[path], BIT_PATH_EN, 0x1);
	rtw_write32_mask(rtwdev, set_pi[path], BITS_RFC_DIRECT, 0x0);
	rtw_write_rf(rtwdev, path, RF_LUTDBG, BIT_TXA_TANK, 0x1);
	rtw_write_rf(rtwdev, path, RF_IDAC, BIT_TX_MODE, 0x820);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, path);
	rtw_write32_mask(rtwdev, REG_IQKSTAT, MASKBYTE0, 0x0);

	rtw_write32_mask(rtwdev, REG_TX_TONE_IDX, MASKBYTE0, 0x018);
	fsleep(1000);
	if (channel >= 1 && channel <= 14)
		rtw_write32_mask(rtwdev, REG_R_CONFIG, MASKBYTE0, BIT_2G_SWING);
	else
		rtw_write32_mask(rtwdev, REG_R_CONFIG, MASKBYTE0, BIT_5G_SWING);
	fsleep(1000);

	rtw_write32_mask(rtwdev, REG_NCTL0, MASKDWORD, cfg1_1b00[path]);
	rtw_write32_mask(rtwdev, REG_NCTL0, MASKDWORD, cfg2_1b00[path]);

	read_poll_timeout(rtw_read32_mask, val,
			  val == 0x55, 1000, 100000, false,
			  rtwdev, REG_RPT_CIP, BIT_RPT_CIP_STATUS);

	rtw_write32_mask(rtwdev, set_pi[path], BITS_RFC_DIRECT, 0x2);
	rtw_write32_mask(rtwdev, REG_NCTL0, BIT_SEL_PATH, path);
	rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, BIT_RPT_EN, 0x1);
	rtw_write32_mask(rtwdev, REG_RXSRAM_CTL, BIT_RPT_SEL, 0x12);
	rtw_write32_mask(rtwdev, REG_TX_GAIN_SET, BIT_GAPK_RPT_IDX, 0x3);
	val = rtw_read32(rtwdev, REG_STAT_RPT);

	txgapk->offset[0][path] = (s8)FIELD_GET(BIT_GAPK_RPT0, val);
	txgapk->offset[1][path] = (s8)FIELD_GET(BIT_GAPK_RPT1, val);
	txgapk->offset[2][path] = (s8)FIELD_GET(BIT_GAPK_RPT2, val);
	txgapk->offset[3][path] = (s8)FIELD_GET(BIT_GAPK_RPT3, val);
	txgapk->offset[4][path] = (s8)FIELD_GET(BIT_GAPK_RPT4, val);
	txgapk->offset[5][path] = (s8)FIELD_GET(BIT_GAPK_RPT5, val);
	txgapk->offset[6][path] = (s8)FIELD_GET(BIT_GAPK_RPT6, val);
	txgapk->offset[7][path] = (s8)FIELD_GET(BIT_GAPK_RPT7, val);

	rtw_write32_mask(rtwdev, REG_TX_GAIN_SET, BIT_GAPK_RPT_IDX, 0x4);
	val = rtw_read32(rtwdev, REG_STAT_RPT);

	txgapk->offset[8][path] = (s8)FIELD_GET(BIT_GAPK_RPT0, val);
	txgapk->offset[9][path] = (s8)FIELD_GET(BIT_GAPK_RPT1, val);

	for (i = 0; i < RF_HW_OFFSET_NUM; i++)
		if (txgapk->offset[i][path] & BIT(3))
			txgapk->offset[i][path] = txgapk->offset[i][path] |
						  0xf0;
	for (i = 0; i < RF_HW_OFFSET_NUM; i++)
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[TXGAPK] offset %d %d path=%d\n",
			txgapk->offset[i][path], i, path);
}

static void rtw8822c_txgapk_reload_bb_reg_restore_impl(struct rtw_dev *rtwdev,
					  const u32 reg[], u32 reg_backup[],
					  u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		rtw_write32(rtwdev, reg[i], reg_backup[i]);
		rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] Reload BB 0x%x = 0x%x\n",
			reg[i], reg_backup[i]);
	}
}
static void rtw8822c_txgapk_reload_bb_reg(struct rtw_dev *rtwdev,
					  const u32 reg[], u32 reg_backup[],
					  u32 reg_num)
{++rtwdev->restore_depth;rtw8822c_txgapk_reload_bb_reg_restore_impl(rtwdev,reg,reg_backup,reg_num);--rtwdev->restore_depth;}

static void rtw8822c_txgapk_rf_restore_restore_impl(struct rtw_dev *rtwdev, u8 path)
{
	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	if (path >= rtwdev->hal.rf_path_num)
		return;

	rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC, BIT_RF_MODE, 0x3);
	rtw_write_rf(rtwdev, path, RF_DEBUG, BIT_DE_TRXBW, 0x0);
	rtw_write_rf(rtwdev, path, RF_EXT_TIA_BW, BIT_PW_EXT_TIA, 0x0);
}
static void rtw8822c_txgapk_rf_restore(struct rtw_dev *rtwdev, u8 path)
{++rtwdev->restore_depth;rtw8822c_txgapk_rf_restore_restore_impl(rtwdev,path);--rtwdev->restore_depth;}

static void rtw8822c_txgapk_save_all_tx_gain_table(struct rtw_dev *rtwdev)
{
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	static const u32 three_wire[2] = {REG_3WIRE, REG_3WIRE2};
	static const u8 ch_num[RF_BAND_MAX] = {1, 1, 36, 100, 149};
	static const u8 band_num[RF_BAND_MAX] = {0x0, 0x0, 0x1, 0x3, 0x5};
	static const u8 cck[RF_BAND_MAX] = {0x1, 0x0, 0x0, 0x0, 0x0};
	u8 path, band, gain, rf0_idx;
	u32 rf18, v;

	if (rtwdev->dm_info.dm_flags & BIT(RTW_DM_CAP_TXGAPK))
		return;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	if (txgapk->read_txgain == 1) {
		rtw_dbg(rtwdev, RTW_DBG_RFK,
			"[TXGAPK] Already Read txgapk->read_txgain return!!!\n");
		rtw8822c_txgapk_write_gain_bb_table(rtwdev);
		return;
	}

	for (band = 0; band < RF_BAND_MAX; band++) {
		for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
			rf18 = rtw_read_rf(rtwdev, path, RF_CFGCH, RFREG_MASK);

			rtw_write32_mask(rtwdev,
					 three_wire[path], BIT_3WIRE_EN, 0x0);
			rtw_write_rf(rtwdev, path,
				     RF_CFGCH, MASKBYTE0, ch_num[band]);
			rtw_write_rf(rtwdev, path,
				     RF_CFGCH, BIT_BAND, band_num[band]);
			rtw_write_rf(rtwdev, path,
				     RF_BW_TRXBB, BIT_DBG_CCK_CCA, cck[band]);
			rtw_write_rf(rtwdev, path,
				     RF_BW_TRXBB, BIT_TX_CCK_IND, cck[band]);
			gain = 0;
			for (rf0_idx = 1; rf0_idx < 32; rf0_idx += 3) {
				rtw_write_rf(rtwdev, path, RF_MODE_TRXAGC,
					     MASKBYTE0, rf0_idx);
				v = rtw_read_rf(rtwdev, path,
						RF_TX_RESULT, RFREG_MASK);
				txgapk->rf3f_bp[band][gain][path] = v & BIT_DATA_L;

				rtw_dbg(rtwdev, RTW_DBG_RFK,
					"[TXGAPK] 0x5f=0x%03X band=%d path=%d\n",
					txgapk->rf3f_bp[band][gain][path],
					band, path);
				gain++;
			}
			rtw_write_rf(rtwdev, path, RF_CFGCH, RFREG_MASK, rf18);
			rtw_write32_mask(rtwdev,
					 three_wire[path], BIT_3WIRE_EN, 0x3);
		}
	}
	rtw8822c_txgapk_write_gain_bb_table(rtwdev);
	txgapk->read_txgain = 1;
}

static void rtw8822c_txgapk_tx_pause(struct rtw_dev *rtwdev)
{
	bool status;
	int ret;

	rtw_write8(rtwdev, REG_TXPAUSE, BIT_AC_QUEUE);
	rtw_write32_mask(rtwdev, REG_TX_FIFO, BIT_STOP_TX, 0x2);

	ret = read_poll_timeout_atomic(check_rf_status, status, status,
				       2, 5000, false, rtwdev, 2);
	if (ret)
		rtw_warn(rtwdev, "failed to pause TX\n");

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] Tx pause!!\n");
}

static void rtw8822c_txgapk_write_gain_bb_table(struct rtw_dev *rtwdev)
{
	u8 path, band;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, rtwdev->dm_info.gapk.channel);

	for (band = 0; band < RF_BAND_MAX; band++) {
		for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
			_rtw8822c_txgapk_write_gain_bb_table(rtwdev,
							     band, path);
		}
	}
}

static void rtw8822c_txgapk_write_tx_gain(struct rtw_dev *rtwdev)
{
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	u32 i, j, tmp = 0x20, tmp_3f, v;
	s8 offset_tmp[RF_GAIN_NUM] = {0};
	u8 path, band = RF_BAND_2G_OFDM, channel = txgapk->channel;

	rtw_dbg(rtwdev, RTW_DBG_RFK, "[TXGAPK] ======>%s\n", __func__);

	if (channel >= 1 && channel <= 14) {
		tmp = 0x20;
		band = RF_BAND_2G_OFDM;
	} else if (channel >= 36 && channel <= 64) {
		tmp = 0x200;
		band = RF_BAND_5G_L;
	} else if (channel >= 100 && channel <= 144) {
		tmp = 0x280;
		band = RF_BAND_5G_M;
	} else if (channel >= 149 && channel <= 177) {
		tmp = 0x300;
		band = RF_BAND_5G_H;
	} else {
		rtw_err(rtwdev, "[TXGAPK] unknown channel %d!!\n", channel);
		return;
	}

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		for (i = 0; i < RF_GAIN_NUM; i++) {
			offset_tmp[i] = 0;
			for (j = i; j < RF_GAIN_NUM; j++) {
				v = txgapk->rf3f_bp[band][j][path];
				if (_rtw8822c_txgapk_gain_valid(rtwdev, v))
					continue;

				offset_tmp[i] += txgapk->offset[j][path];
				txgapk->fianl_offset[i][path] = offset_tmp[i];
			}

			v = txgapk->rf3f_bp[band][i][path];
			if (_rtw8822c_txgapk_gain_valid(rtwdev, v)) {
				rtw_dbg(rtwdev, RTW_DBG_RFK,
					"[TXGAPK] tx_gain=0x%03X >= 0xCEX\n",
					txgapk->rf3f_bp[band][i][path]);
			} else {
				txgapk->rf3f_fs[path][i] = offset_tmp[i];
				rtw_dbg(rtwdev, RTW_DBG_RFK,
					"[TXGAPK] offset %d %d\n",
					offset_tmp[i], i);
			}
		}

		rtw_write_rf(rtwdev, path, RF_LUTWE2, RFREG_MASK, 0x10000);
		for (i = 0; i < RF_GAIN_NUM; i++) {
			rtw_write_rf(rtwdev, path,
				     RF_LUTWA, RFREG_MASK, tmp + i);

			tmp_3f = rtw8822c_txgapk_cal_gain(rtwdev,
							  txgapk->rf3f_bp[band][i][path],
							  offset_tmp[i]);
			rtw_write_rf(rtwdev, path, RF_LUTWD0,
				     BIT_GAIN_EXT | BIT_DATA_L, tmp_3f);

			rtw_dbg(rtwdev, RTW_DBG_RFK,
				"[TXGAPK] 0x33=0x%05X 0x3f=0x%04X\n",
				tmp + i, tmp_3f);
		}
		rtw_write_rf(rtwdev, path, RF_LUTWE2, RFREG_MASK, 0x0);
	}
}
