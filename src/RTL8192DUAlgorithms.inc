/* SPDX-License-Identifier: GPL-2.0-only */
/* Generated from Linux 0e135910deec43ffe97eca0b6af1f8f847285b59; do not edit by hand. */
// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2024  Realtek Corporation.*/


#define MAX_RF_IMR_INDEX			12
#define MAX_RF_IMR_INDEX_NORMAL			13
#define RF_REG_NUM_FOR_C_CUT_5G			6
#define RF_REG_NUM_FOR_C_CUT_5G_INTERNALPA	7
#define RF_REG_NUM_FOR_C_CUT_2G			5
#define RF_CHNL_NUM_5G				19
#define RF_CHNL_NUM_5G_40M			17
#define CV_CURVE_CNT				64

static const u32 rf_reg_for_5g_swchnl_normal[MAX_RF_IMR_INDEX_NORMAL] = {
	0, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x0
};

static const u8 rf_reg_for_c_cut_5g[RF_REG_NUM_FOR_C_CUT_5G] = {
	RF_SYN_G1, RF_SYN_G2, RF_SYN_G3, RF_SYN_G4, RF_SYN_G5, RF_SYN_G6
};

static const u8 rf_reg_for_c_cut_2g[RF_REG_NUM_FOR_C_CUT_2G] = {
	RF_SYN_G1, RF_SYN_G2, RF_SYN_G3, RF_SYN_G7, RF_SYN_G8
};

static const u8 rf_for_c_cut_5g_internal_pa[RF_REG_NUM_FOR_C_CUT_5G_INTERNALPA] = {
	0x0B, 0x48, 0x49, 0x4B, 0x03, 0x04, 0x0E
};

static const u32 rf_reg_mask_for_c_cut_2g[RF_REG_NUM_FOR_C_CUT_2G] = {
	BIT(19) | BIT(18) | BIT(17) | BIT(14) | BIT(1),
	BIT(10) | BIT(9),
	BIT(18) | BIT(17) | BIT(16) | BIT(1),
	BIT(2) | BIT(1),
	BIT(15) | BIT(14) | BIT(13) | BIT(12) | BIT(11)
};

static const u8 rf_chnl_5g[RF_CHNL_NUM_5G] = {
	36, 40, 44, 48, 52, 56, 60, 64, 100, 104, 108,
	112, 116, 120, 124, 128, 132, 136, 140
};

static const u8 rf_chnl_5g_40m[RF_CHNL_NUM_5G_40M] = {
	38, 42, 46, 50, 54, 58, 62, 102, 106, 110, 114,
	118, 122, 126, 130, 134, 138
};

static const u32 rf_reg_pram_c_5g[5][RF_REG_NUM_FOR_C_CUT_5G] = {
	{0xE43BE, 0xFC638, 0x77C0A, 0xDE471, 0xd7110, 0x8EB04},
	{0xE43BE, 0xFC078, 0xF7C1A, 0xE0C71, 0xD7550, 0xAEB04},
	{0xE43BF, 0xFF038, 0xF7C0A, 0xDE471, 0xE5550, 0xAEB04},
	{0xE43BF, 0xFF079, 0xF7C1A, 0xDE471, 0xE5550, 0xAEB04},
	{0xE43BF, 0xFF038, 0xF7C1A, 0xDE471, 0xd7550, 0xAEB04}
};

static const u32 rf_reg_param_for_c_cut_2g[3][RF_REG_NUM_FOR_C_CUT_2G] = {
	{0x643BC, 0xFC038, 0x77C1A, 0x41289, 0x01840},
	{0x643BC, 0xFC038, 0x07C1A, 0x41289, 0x01840},
	{0x243BC, 0xFC438, 0x07C1A, 0x4128B, 0x0FC41}
};

static const u32 rf_syn_g4_for_c_cut_2g = 0xD1C31 & 0x7FF;

static const u32 rf_pram_c_5g_int_pa[3][RF_REG_NUM_FOR_C_CUT_5G_INTERNALPA] = {
	{0x01a00, 0x40443, 0x00eb5, 0x89bec, 0x94a12, 0x94a12, 0x94a12},
	{0x01800, 0xc0443, 0x00730, 0x896ee, 0x94a52, 0x94a52, 0x94a52},
	{0x01800, 0xc0443, 0x00730, 0x896ee, 0x94a12, 0x94a12, 0x94a12}
};

/* [patha+b][reg] */
static const u32 rf_imr_param_normal[3][MAX_RF_IMR_INDEX_NORMAL] = {
	/* channels 1-14. */
	{
		0x70000, 0x00ff0, 0x4400f, 0x00ff0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x64888, 0xe266c, 0x00090, 0x22fff
	},
	/* channels 36-64 */
	{
		0x70000, 0x22880, 0x4470f, 0x55880, 0x00070, 0x88000,
		0x0, 0x88080, 0x70000, 0x64a82, 0xe466c, 0x00090,
		0x32c9a
	},
	/* channels 100-165 */
	{
		0x70000, 0x44880, 0x4477f, 0x77880, 0x00070, 0x88000,
		0x0, 0x880b0, 0x0, 0x64b82, 0xe466c, 0x00090, 0x32c9a
	}
};

static const u32 targetchnl_5g[TARGET_CHNL_NUM_5G] = {
	25141, 25116, 25091, 25066, 25041,
	25016, 24991, 24966, 24941, 24917,
	24892, 24867, 24843, 24818, 24794,
	24770, 24765, 24721, 24697, 24672,
	24648, 24624, 24600, 24576, 24552,
	24528, 24504, 24480, 24457, 24433,
	24409, 24385, 24362, 24338, 24315,
	24291, 24268, 24245, 24221, 24198,
	24175, 24151, 24128, 24105, 24082,
	24059, 24036, 24013, 23990, 23967,
	23945, 23922, 23899, 23876, 23854,
	23831, 23809, 23786, 23764, 23741,
	23719, 23697, 23674, 23652, 23630,
	23608, 23586, 23564, 23541, 23519,
	23498, 23476, 23454, 23432, 23410,
	23388, 23367, 23345, 23323, 23302,
	23280, 23259, 23237, 23216, 23194,
	23173, 23152, 23130, 23109, 23088,
	23067, 23046, 23025, 23003, 22982,
	22962, 22941, 22920, 22899, 22878,
	22857, 22837, 22816, 22795, 22775,
	22754, 22733, 22713, 22692, 22672,
	22652, 22631, 22611, 22591, 22570,
	22550, 22530, 22510, 22490, 22469,
	22449, 22429, 22409, 22390, 22370,
	22350, 22336, 22310, 22290, 22271,
	22251, 22231, 22212, 22192, 22173,
	22153, 22134, 22114, 22095, 22075,
	22056, 22037, 22017, 21998, 21979,
	21960, 21941, 21921, 21902, 21883,
	21864, 21845, 21826, 21807, 21789,
	21770, 21751, 21732, 21713, 21695,
	21676, 21657, 21639, 21620, 21602,
	21583, 21565, 21546, 21528, 21509,
	21491, 21473, 21454, 21436, 21418,
	21400, 21381, 21363, 21345, 21327,
	21309, 21291, 21273, 21255, 21237,
	21219, 21201, 21183, 21166, 21148,
	21130, 21112, 21095, 21077, 21059,
	21042, 21024, 21007, 20989, 20972,
	25679, 25653, 25627, 25601, 25575,
	25549, 25523, 25497, 25471, 25446,
	25420, 25394, 25369, 25343, 25318,
	25292, 25267, 25242, 25216, 25191,
	25166
};

/* channel 1~14 */
static const u32 targetchnl_2g[TARGET_CHNL_NUM_2G] = {
	26084, 26030, 25976, 25923, 25869, 25816, 25764,
	25711, 25658, 25606, 25554, 25502, 25451, 25328
};


// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2009-2012  Realtek Corporation.*/


static const u8 channel_all[59] = {
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
	36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58,
	60, 62, 64, 100, 102, 104, 106, 108, 110, 112,
	114, 116, 118, 120, 122, 124, 126, 128,	130,
	132, 134, 136, 138, 140, 149, 151, 153, 155,
	157, 159, 161, 163, 165
};


// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2009-2012  Realtek Corporation.*/


static const u32 ofdmswing_table[OFDM_TABLE_SIZE_92D] = {
	0x7f8001fe,		/* 0, +6.0dB */
	0x788001e2,		/* 1, +5.5dB */
	0x71c001c7,		/* 2, +5.0dB */
	0x6b8001ae,		/* 3, +4.5dB */
	0x65400195,		/* 4, +4.0dB */
	0x5fc0017f,		/* 5, +3.5dB */
	0x5a400169,		/* 6, +3.0dB */
	0x55400155,		/* 7, +2.5dB */
	0x50800142,		/* 8, +2.0dB */
	0x4c000130,		/* 9, +1.5dB */
	0x47c0011f,		/* 10, +1.0dB */
	0x43c0010f,		/* 11, +0.5dB */
	0x40000100,		/* 12, +0dB */
	0x3c8000f2,		/* 13, -0.5dB */
	0x390000e4,		/* 14, -1.0dB */
	0x35c000d7,		/* 15, -1.5dB */
	0x32c000cb,		/* 16, -2.0dB */
	0x300000c0,		/* 17, -2.5dB */
	0x2d4000b5,		/* 18, -3.0dB */
	0x2ac000ab,		/* 19, -3.5dB */
	0x288000a2,		/* 20, -4.0dB */
	0x26000098,		/* 21, -4.5dB */
	0x24000090,		/* 22, -5.0dB */
	0x22000088,		/* 23, -5.5dB */
	0x20000080,		/* 24, -6.0dB */
	0x1e400079,		/* 25, -6.5dB */
	0x1c800072,		/* 26, -7.0dB */
	0x1b00006c,		/* 27. -7.5dB */
	0x19800066,		/* 28, -8.0dB */
	0x18000060,		/* 29, -8.5dB */
	0x16c0005b,		/* 30, -9.0dB */
	0x15800056,		/* 31, -9.5dB */
	0x14400051,		/* 32, -10.0dB */
	0x1300004c,		/* 33, -10.5dB */
	0x12000048,		/* 34, -11.0dB */
	0x11000044,		/* 35, -11.5dB */
	0x10000040,		/* 36, -12.0dB */
	0x0f00003c,		/* 37, -12.5dB */
	0x0e400039,		/* 38, -13.0dB */
	0x0d800036,		/* 39, -13.5dB */
	0x0cc00033,		/* 40, -14.0dB */
	0x0c000030,		/* 41, -14.5dB */
	0x0b40002d,		/* 42, -15.0dB */
};

static const u8 cckswing_table_ch1ch13[CCK_TABLE_SIZE][8] = {
	{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04},    /* 0, +0dB */
	{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},    /* 1, -0.5dB */
	{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},    /* 2, -1.0dB */
	{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},    /* 3, -1.5dB */
	{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},    /* 4, -2.0dB */
	{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},    /* 5, -2.5dB */
	{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},    /* 6, -3.0dB */
	{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},    /* 7, -3.5dB */
	{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},    /* 8, -4.0dB */
	{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},    /* 9, -4.5dB */
	{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},    /* 10, -5.0dB */
	{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},    /* 11, -5.5dB */
	{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},    /* 12, -6.0dB */
	{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},    /* 13, -6.5dB */
	{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},    /* 14, -7.0dB */
	{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},    /* 15, -7.5dB */
	{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},    /* 16, -8.0dB */
	{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},    /* 17, -8.5dB */
	{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},    /* 18, -9.0dB */
	{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},    /* 19, -9.5dB */
	{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},    /* 20, -10.0dB */
	{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},    /* 21, -10.5dB */
	{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01},    /* 22, -11.0dB */
	{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01},    /* 23, -11.5dB */
	{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01},    /* 24, -12.0dB */
	{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01},    /* 25, -12.5dB */
	{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01},    /* 26, -13.0dB */
	{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01},    /* 27, -13.5dB */
	{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01},    /* 28, -14.0dB */
	{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01},    /* 29, -14.5dB */
	{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01},    /* 30, -15.0dB */
	{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01},    /* 31, -15.5dB */
	{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01}     /* 32, -16.0dB */
};

static const u8 cckswing_table_ch14[CCK_TABLE_SIZE][8] = {
	{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00},    /* 0, +0dB */
	{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},    /* 1, -0.5dB */
	{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},    /* 2, -1.0dB */
	{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00},    /* 3, -1.5dB */
	{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},    /* 4, -2.0dB */
	{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00},    /* 5, -2.5dB */
	{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},    /* 6, -3.0dB */
	{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},    /* 7, -3.5dB */
	{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},    /* 8, -4.0dB */
	{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},    /* 9, -4.5dB */
	{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},    /* 10, -5.0dB */
	{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},    /* 11, -5.5dB */
	{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},    /* 12, -6.0dB */
	{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},    /* 13, -6.5dB */
	{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},    /* 14, -7.0dB */
	{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},    /* 15, -7.5dB */
	{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},    /* 16, -8.0dB */
	{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},    /* 17, -8.5dB */
	{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},    /* 18, -9.0dB */
	{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},    /* 19, -9.5dB */
	{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},    /* 20, -10.0dB */
	{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},    /* 21, -10.5dB */
	{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00},    /* 22, -11.0dB */
	{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},    /* 23, -11.5dB */
	{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},    /* 24, -12.0dB */
	{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00},    /* 25, -12.5dB */
	{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},    /* 26, -13.0dB */
	{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},    /* 27, -13.5dB */
	{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},    /* 28, -14.0dB */
	{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},    /* 29, -14.5dB */
	{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},    /* 30, -15.0dB */
	{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},    /* 31, -15.5dB */
	{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00}     /* 32, -16.0dB */
};


static u8 _rtl92c_phy_get_rightchnlplace(u8 chnl);

static void _rtl92d_ccxpower_index_check(struct ieee80211_hw *hw,
					 u8 channel, u8 *cckpowerlevel,
					 u8 *ofdmpowerlevel);

static void _rtl92d_get_pwr_diff_limit(struct ieee80211_hw *hw, u8 channel,
				       u8 index, u8 rf, u8 pwr_diff_limit[4]);

static void _rtl92d_get_txpower_index(struct ieee80211_hw *hw, u8 channel,
				      u8 *cckpowerlevel, u8 *ofdmpowerlevel);

static void _rtl92d_get_txpower_writeval_by_regulatory(struct ieee80211_hw *hw,
						       u8 channel, u8 index,
						       u32 *powerbase0,
						       u32 *powerbase1,
						       u32 *p_outwriteval);

static bool _rtl92d_is_legal_5g_channel(struct ieee80211_hw *hw, u8 channel);

static u32 _rtl92d_phy_get_abs(u32 val1, u32 val2);

static void _rtl92d_phy_get_power_base(struct ieee80211_hw *hw,
				       u8 *ppowerlevel, u8 channel,
				       u32 *ofdmbase, u32 *mcsbase);

static u32 _rtl92d_phy_rf_serial_read(struct ieee80211_hw *hw,
				      enum radio_path rfpath, u32 offset);

static void _rtl92d_phy_rf_serial_write(struct ieee80211_hw *hw,
					enum radio_path rfpath,
					u32 offset, u32 data);

static void _rtl92d_read_txpower_info(struct ieee80211_hw *hw,
				      bool autoload_fail, u8 *hwinfo);

static void _rtl92d_readpowervalue_fromprom(struct txpower_info *pwrinfo,
					    u8 *efuse, bool autoloadfail);

static void _rtl92d_write_ofdm_power_reg(struct ieee80211_hw *hw,
					 u8 index, u32 *pvalue);

static bool _rtl92du_phy_bb_config(struct ieee80211_hw *hw);

static bool _rtl92du_phy_config_bb(struct ieee80211_hw *hw, u8 configtype);

static bool _rtl92du_phy_config_bb_pg(struct ieee80211_hw *hw, u8 configtype);

static void _rtl92du_phy_iq_calibrate(struct ieee80211_hw *hw, long result[][8],
				      u8 t, bool is2t);

static void _rtl92du_phy_iq_calibrate_5g_normal(struct ieee80211_hw *hw,
						long result[][8], u8 t);

static void _rtl92du_phy_lc_calibrate_sw(struct ieee80211_hw *hw, bool is2t);

static void _rtl92du_phy_patha_fill_iqk_matrix(struct ieee80211_hw *hw,
					       bool iqk_ok, long result[][8],
					       u8 final_candidate, bool txonly);

static void _rtl92du_phy_patha_fill_iqk_matrix_5g_normal(struct ieee80211_hw *hw,
							 bool iqk_ok,
							 long result[][8],
							 u8 final_candidate,
							 bool txonly);

static u8 _rtl92du_phy_patha_iqk(struct ieee80211_hw *hw, bool configpathb);

static u8 _rtl92du_phy_patha_iqk_5g_normal(struct ieee80211_hw *hw,
					   bool configpathb);

static void _rtl92du_phy_patha_standby(struct ieee80211_hw *hw);

static void _rtl92du_phy_pathb_fill_iqk_matrix(struct ieee80211_hw *hw,
					       bool iqk_ok, long result[][8],
					       u8 final_candidate, bool txonly);

static void _rtl92du_phy_pathb_fill_iqk_matrix_5g_normal(struct ieee80211_hw *hw,
							 bool iqk_ok,
							 long result[][8],
							 u8 final_candidate,
							 bool txonly);

static u8 _rtl92du_phy_pathb_iqk(struct ieee80211_hw *hw);

static u8 _rtl92du_phy_pathb_iqk_5g_normal(struct ieee80211_hw *hw);

static void _rtl92du_phy_pimode_switch(struct ieee80211_hw *hw, bool pi_mode);

static void _rtl92du_phy_reload_adda_registers(struct ieee80211_hw *hw,
					       const u32 *adda_reg,
					       u32 *adda_backup, u32 regnum);

static void _rtl92du_phy_reload_imr_setting(struct ieee80211_hw *hw,
					    u8 channel, u8 rfpath);

static void _rtl92du_phy_reload_lck_setting(struct ieee80211_hw *hw, u8 channel);

static void _rtl92du_phy_reload_mac_registers(struct ieee80211_hw *hw,
					      const u32 *macreg, u32 *macbackup);

static void _rtl92du_phy_set_rfsleep(struct ieee80211_hw *hw);

static bool _rtl92du_phy_simularity_compare(struct ieee80211_hw *hw,
					    long result[][8], u8 c1, u8 c2);

static void _rtl92du_phy_stop_trx_before_changeband(struct ieee80211_hw *hw);

static void _rtl92du_phy_switch_rf_setting(struct ieee80211_hw *hw, u8 channel);

static void rtl92d_bandtype_2_4G(struct ieee80211_hw *hw, long *temp_cckg,
				 s8 *cck_index_old);

static void rtl92d_bandtype_5G(struct rtl_hal *rtlhal, int *ofdm_index,
			       bool *internal_pa, u8 thermalvalue, u8 delta,
			       u8 rf, struct rtl_efuse *rtlefuse,
			       struct rtl_priv *rtlpriv, struct rtl_phy *rtlphy,
			       const u8 index_mapping[5][INDEX_MAPPING_NUM],
			       const u8 index_mapping_pa[8][INDEX_MAPPING_NUM]);

static void rtl92d_dm_check_txpower_tracking_thermal_meter(struct ieee80211_hw *hw);

static void rtl92d_dm_initialize_txpower_tracking(struct ieee80211_hw *hw);

static void rtl92d_dm_rxgain_tracking_thermalmeter(struct ieee80211_hw *hw);

static void
rtl92d_dm_txpower_tracking_callback_thermalmeter(struct ieee80211_hw *hw);

static u8 rtl92d_get_chnlgroup_fromarray(u8 chnl);

static u8 rtl92d_get_rightchnlplace_for_iqk(u8 chnl);

static void rtl92d_phy_calc_curvindex(struct ieee80211_hw *hw,
			       const u32 *targetchnl, u32 *curvecount_val,
			       bool is5g, u32 *curveindex);

static void rtl92d_phy_enable_rf_env(struct ieee80211_hw *hw, u8 rfpath,
			      u32 *pu4_regval);

static u8 rtl92d_phy_get_chnlgroup_bypg(u8 chnlindex);

static void rtl92d_phy_get_hw_reg_originalvalue(struct ieee80211_hw *hw);

static void rtl92d_phy_init_bb_rf_register_definition(struct ieee80211_hw *hw);

static void rtl92d_phy_mac_setting_calibration(struct ieee80211_hw *hw,
					const u32 *macreg, u32 *macbackup);

static void rtl92d_phy_path_adda_on(struct ieee80211_hw *hw,
			     const u32 *adda_reg, bool patha_on, bool is2t);

static u32 rtl92d_phy_query_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			    u32 regaddr, u32 bitmask);

static void rtl92d_phy_reset_iqk_result(struct ieee80211_hw *hw);

static void rtl92d_phy_restore_rf_env(struct ieee80211_hw *hw, u8 rfpath,
			       u32 *pu4_regval);

static void rtl92d_phy_rf6052_set_bandwidth(struct ieee80211_hw *hw, u8 bandwidth);

static void rtl92d_phy_rf6052_set_cck_txpower(struct ieee80211_hw *hw,
				       u8 *ppowerlevel);

static void rtl92d_phy_rf6052_set_ofdm_txpower(struct ieee80211_hw *hw,
					u8 *ppowerlevel, u8 channel);

static void rtl92d_phy_save_adda_registers(struct ieee80211_hw *hw, const u32 *adda_reg,
				    u32 *adda_backup, u32 regnum);

static void rtl92d_phy_save_mac_registers(struct ieee80211_hw *hw,
				   const u32 *macreg, u32 *macbackup);

static void rtl92d_phy_set_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			   u32 regaddr, u32 bitmask, u32 data);

static void rtl92d_phy_set_txpower_level(struct ieee80211_hw *hw, u8 channel);

static void rtl92d_store_pwrindex_diffrate_offset(struct ieee80211_hw *hw,
					   u32 regaddr, u32 bitmask, u32 data);

static bool rtl92du_phy_bb_config(struct ieee80211_hw *hw);

static bool rtl92du_phy_config_rf_with_headerfile(struct ieee80211_hw *hw,
					   enum rf_content content,
					   enum radio_path rfpath);

static bool rtl92du_phy_enable_anotherphy(struct ieee80211_hw *hw, bool bmac0);

static void rtl92du_phy_init_pa_bias(struct ieee80211_hw *hw);

static void rtl92du_phy_iq_calibrate(struct ieee80211_hw *hw);

static void rtl92du_phy_lc_calibrate(struct ieee80211_hw *hw, bool is2t);

static bool rtl92du_phy_mac_config(struct ieee80211_hw *hw);

static void rtl92du_phy_powerdown_anotherphy(struct ieee80211_hw *hw, bool bmac0);

static u32 rtl92du_phy_query_bb_reg(struct ieee80211_hw *hw, u32 regaddr, u32 bitmask);

static void rtl92du_phy_reload_iqk_setting(struct ieee80211_hw *hw, u8 channel);

static bool rtl92du_phy_rf6052_config(struct ieee80211_hw *hw);

static bool rtl92du_phy_rf_config(struct ieee80211_hw *hw);

static void rtl92du_phy_set_bb_reg(struct ieee80211_hw *hw,
			    u32 regaddr, u32 bitmask, u32 data);

static void rtl92du_phy_set_bb_reg_1byte(struct ieee80211_hw *hw,
					 u32 regaddr, u32 bitmask, u32 data);

static void rtl92du_phy_set_bw_mode(struct ieee80211_hw *hw,
			     enum nl80211_channel_type ch_type);

static u8 rtl92du_phy_sw_chnl(struct ieee80211_hw *hw);

static void rtl92du_phy_switch_wirelessband(struct ieee80211_hw *hw, u8 band);

static void rtl92du_update_bbrf_configuration(struct ieee80211_hw *hw);


/* rtl8192d/phy_common.c: _rtl92c_phy_get_rightchnlplace */
static u8 _rtl92c_phy_get_rightchnlplace(u8 chnl)
{
	u8 place = chnl;

	if (chnl > 14) {
		for (place = 14; place < ARRAY_SIZE(channel_all); place++) {
			if (channel_all[place] == chnl) {
				place++;
				break;
			}
		}
	}
	return place;
}


/* rtl8192d/phy_common.c: _rtl92d_ccxpower_index_check */
static void _rtl92d_ccxpower_index_check(struct ieee80211_hw *hw,
					 u8 channel, u8 *cckpowerlevel,
					 u8 *ofdmpowerlevel)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;

	rtlphy->cur_cck_txpwridx = cckpowerlevel[0];
	rtlphy->cur_ofdm24g_txpwridx = ofdmpowerlevel[0];
}


/* rtl8192d/rf_common.c: _rtl92d_get_pwr_diff_limit */
static void _rtl92d_get_pwr_diff_limit(struct ieee80211_hw *hw, u8 channel,
				       u8 index, u8 rf, u8 pwr_diff_limit[4])
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 mcs_offset;
	u8 limit;
	int i;

	mcs_offset = rtlphy->mcs_offset[0][index + (rf ? 8 : 0)];

	for (i = 0; i < 4; i++) {
		pwr_diff_limit[i] = (mcs_offset >> (i * 8)) & 0x7f;

		if (rtlphy->current_chan_bw == HT_CHANNEL_WIDTH_20_40)
			limit = rtlefuse->pwrgroup_ht40[rf][channel - 1];
		else
			limit = rtlefuse->pwrgroup_ht20[rf][channel - 1];

		if (pwr_diff_limit[i] > limit)
			pwr_diff_limit[i] = limit;
	}
}


/* rtl8192d/phy_common.c: _rtl92d_get_txpower_index */
static void _rtl92d_get_txpower_index(struct ieee80211_hw *hw, u8 channel,
				      u8 *cckpowerlevel, u8 *ofdmpowerlevel)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	u8 index = channel - 1;

	/* 1. CCK */
	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		/* RF-A */
		cckpowerlevel[RF90_PATH_A] =
				 rtlefuse->txpwrlevel_cck[RF90_PATH_A][index];
		/* RF-B */
		cckpowerlevel[RF90_PATH_B] =
				 rtlefuse->txpwrlevel_cck[RF90_PATH_B][index];
	} else {
		cckpowerlevel[RF90_PATH_A] = 0;
		cckpowerlevel[RF90_PATH_B] = 0;
	}
	/* 2. OFDM for 1S or 2S */
	if (rtlphy->rf_type == RF_1T2R || rtlphy->rf_type == RF_1T1R) {
		/*  Read HT 40 OFDM TX power */
		ofdmpowerlevel[RF90_PATH_A] =
		    rtlefuse->txpwrlevel_ht40_1s[RF90_PATH_A][index];
		ofdmpowerlevel[RF90_PATH_B] =
		    rtlefuse->txpwrlevel_ht40_1s[RF90_PATH_B][index];
	} else if (rtlphy->rf_type == RF_2T2R) {
		/* Read HT 40 OFDM TX power */
		ofdmpowerlevel[RF90_PATH_A] =
		    rtlefuse->txpwrlevel_ht40_2s[RF90_PATH_A][index];
		ofdmpowerlevel[RF90_PATH_B] =
		    rtlefuse->txpwrlevel_ht40_2s[RF90_PATH_B][index];
	}
	else {
		ofdmpowerlevel[0] = ofdmpowerlevel[1] = 0;
		du_algorithm_failure(hw, RTW_92DU_BAD_ARGUMENT);
	}
}


/* rtl8192d/rf_common.c: _rtl92d_get_txpower_writeval_by_regulatory */
static void _rtl92d_get_txpower_writeval_by_regulatory(struct ieee80211_hw *hw,
						       u8 channel, u8 index,
						       u32 *powerbase0,
						       u32 *powerbase1,
						       u32 *p_outwriteval)
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 writeval = 0, customer_limit, rf;
	u8 chnlgroup = 0, pwr_diff_limit[4];

	for (rf = 0; rf < 2; rf++) {
		switch (rtlefuse->eeprom_regulatory) {
		case 0:
			writeval = rtlphy->mcs_offset[0][index + (rf ? 8 : 0)];

			RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
				"RTK better performance\n");
			break;
		case 1:
			if (rtlphy->pwrgroup_cnt == 1)
				chnlgroup = 0;

			if (rtlphy->pwrgroup_cnt < MAX_PG_GROUP)
				break;

			chnlgroup = rtl92d_phy_get_chnlgroup_bypg(channel - 1);
			if (rtlphy->current_chan_bw == HT_CHANNEL_WIDTH_20)
				chnlgroup++;
			else
				chnlgroup += 4;

			writeval = rtlphy->mcs_offset
					[chnlgroup][index + (rf ? 8 : 0)];

			RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
				"Realtek regulatory, 20MHz\n");
			break;
		case 2:
			writeval = 0;

			RTPRINT(rtlpriv, FPHY, PHY_TXPWR, "Better regulatory\n");
			break;
		case 3:
			if (rtlphy->current_chan_bw == HT_CHANNEL_WIDTH_20_40) {
				RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
					"customer's limit, 40MHz rf(%c) = 0x%x\n",
					rf == 0 ? 'A' : 'B',
					rtlefuse->pwrgroup_ht40[rf][channel - 1]);
			} else {
				RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
					"customer's limit, 20MHz rf(%c) = 0x%x\n",
					rf == 0 ? 'A' : 'B',
					rtlefuse->pwrgroup_ht20[rf][channel - 1]);
			}

			_rtl92d_get_pwr_diff_limit(hw, channel, index, rf,
						   pwr_diff_limit);

			customer_limit = (pwr_diff_limit[3] << 24) |
					 (pwr_diff_limit[2] << 16) |
					 (pwr_diff_limit[1] << 8) |
					 (pwr_diff_limit[0]);

			RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
				"Customer's limit rf(%c) = 0x%x\n",
				rf == 0 ? 'A' : 'B', customer_limit);

			writeval = customer_limit;

			RTPRINT(rtlpriv, FPHY, PHY_TXPWR, "Customer\n");
			break;
		default:
			writeval = rtlphy->mcs_offset[0][index + (rf ? 8 : 0)];

			RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
				"RTK better performance\n");
			break;
		}

		if (index < 2)
			writeval += powerbase0[rf];
		else
			writeval += powerbase1[rf];

		RTPRINT(rtlpriv, FPHY, PHY_TXPWR, "writeval rf(%c)= 0x%x\n",
			rf == 0 ? 'A' : 'B', writeval);

		*(p_outwriteval + rf) = writeval;
	}
}


/* rtl8192d/phy_common.c: _rtl92d_is_legal_5g_channel */
static bool _rtl92d_is_legal_5g_channel(struct ieee80211_hw *hw, u8 channel)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(channel5g); i++)
		if (channel == channel5g[i])
			return true;
	return false;
}


/* rtl8192d/phy_common.c: _rtl92d_phy_get_abs */
static u32 _rtl92d_phy_get_abs(u32 val1, u32 val2)
{
	u32 ret;

	if (val1 >= val2)
		ret = val1 - val2;
	else
		ret = val2 - val1;
	return ret;
}


/* rtl8192d/rf_common.c: _rtl92d_phy_get_power_base */
static void _rtl92d_phy_get_power_base(struct ieee80211_hw *hw,
				       u8 *ppowerlevel, u8 channel,
				       u32 *ofdmbase, u32 *mcsbase)
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 powerbase0, powerbase1;
	u8 legacy_pwrdiff, ht20_pwrdiff;
	u8 i, powerlevel[2];

	for (i = 0; i < 2; i++) {
		powerlevel[i] = ppowerlevel[i];
		legacy_pwrdiff = rtlefuse->txpwr_legacyhtdiff[i][channel - 1];
		powerbase0 = powerlevel[i] + legacy_pwrdiff;
		powerbase0 = (powerbase0 << 24) | (powerbase0 << 16) |
			     (powerbase0 << 8) | powerbase0;
		*(ofdmbase + i) = powerbase0;
		RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
			" [OFDM power base index rf(%c) = 0x%x]\n",
			i == 0 ? 'A' : 'B', *(ofdmbase + i));
	}

	for (i = 0; i < 2; i++) {
		if (rtlphy->current_chan_bw == HT_CHANNEL_WIDTH_20) {
			ht20_pwrdiff = rtlefuse->txpwr_ht20diff[i][channel - 1];
			powerlevel[i] += ht20_pwrdiff;
		}
		powerbase1 = powerlevel[i];
		powerbase1 = (powerbase1 << 24) | (powerbase1 << 16) |
			     (powerbase1 << 8) | powerbase1;
		*(mcsbase + i) = powerbase1;
		RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
			" [MCS power base index rf(%c) = 0x%x]\n",
			i == 0 ? 'A' : 'B', *(mcsbase + i));
	}
}


/* rtl8192d/phy_common.c: _rtl92d_phy_rf_serial_read */
static u32 _rtl92d_phy_rf_serial_read(struct ieee80211_hw *hw,
				      enum radio_path rfpath, u32 offset)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct bb_reg_def *pphyreg = &rtlphy->phyreg_def[rfpath];
	u32 newoffset;
	u32 tmplong, tmplong2;
	u8 rfpi_enable = 0;
	u32 retvalue;

	newoffset = offset;
	tmplong = rtl_get_bbreg(hw, RFPGA0_XA_HSSIPARAMETER2, MASKDWORD);
	if (rfpath == RF90_PATH_A)
		tmplong2 = tmplong;
	else
		tmplong2 = rtl_get_bbreg(hw, pphyreg->rfhssi_para2, MASKDWORD);
	tmplong2 = (tmplong2 & (~BLSSIREADADDRESS)) |
		(newoffset << 23) | BLSSIREADEDGE;
	rtl_set_bbreg(hw, RFPGA0_XA_HSSIPARAMETER2, MASKDWORD,
		      tmplong & (~BLSSIREADEDGE));
	udelay(10);
	rtl_set_bbreg(hw, pphyreg->rfhssi_para2, MASKDWORD, tmplong2);
	udelay(100);
	rtl_set_bbreg(hw, RFPGA0_XA_HSSIPARAMETER2, MASKDWORD,
		      tmplong | BLSSIREADEDGE);
	udelay(10);
	if (rfpath == RF90_PATH_A)
		rfpi_enable = (u8)rtl_get_bbreg(hw, RFPGA0_XA_HSSIPARAMETER1,
						BIT(8));
	else if (rfpath == RF90_PATH_B)
		rfpi_enable = (u8)rtl_get_bbreg(hw, RFPGA0_XB_HSSIPARAMETER1,
						BIT(8));
	if (rfpi_enable)
		retvalue = rtl_get_bbreg(hw, pphyreg->rf_rbpi,
					 BLSSIREADBACKDATA);
	else
		retvalue = rtl_get_bbreg(hw, pphyreg->rf_rb,
					 BLSSIREADBACKDATA);
	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "RFR-%d Addr[0x%x] = 0x%x\n",
		rfpath, pphyreg->rf_rb, retvalue);
	du_rf_sample(hw, rfpath, offset, retvalue);
	return retvalue;
}


/* rtl8192d/phy_common.c: _rtl92d_phy_rf_serial_write */
static void _rtl92d_phy_rf_serial_write(struct ieee80211_hw *hw,
					enum radio_path rfpath,
					u32 offset, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct bb_reg_def *pphyreg = &rtlphy->phyreg_def[rfpath];
	u32 data_and_addr;
	u32 newoffset;

	newoffset = offset;
	/* T65 RF */
	data_and_addr = ((newoffset << 20) | (data & 0x000fffff)) & 0x0fffffff;
	rtl_set_bbreg(hw, pphyreg->rf3wire_offset, MASKDWORD, data_and_addr);
	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "RFW-%d Addr[0x%x]=0x%x\n",
		rfpath, pphyreg->rf3wire_offset, data_and_addr);
}


/* rtl8192d/hw_common.c: _rtl92d_read_txpower_info */
static void _rtl92d_read_txpower_info(struct ieee80211_hw *hw,
				      bool autoload_fail, u8 *hwinfo)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct txpower_info pwrinfo;
	u8 tempval[2], i, pwr, diff;
	u32 ch, rfpath, group;

	_rtl92d_readpowervalue_fromprom(&pwrinfo, hwinfo, autoload_fail);
	if (!autoload_fail) {
		/* bit0~2 */
		rtlefuse->eeprom_regulatory = (hwinfo[EEPROM_RF_OPT1] & 0x7);
		rtlefuse->eeprom_thermalmeter =
			 hwinfo[EEPROM_THERMAL_METER] & 0x1f;
		rtlefuse->crystalcap = hwinfo[EEPROM_XTAL_K];
		tempval[0] = hwinfo[EEPROM_IQK_DELTA] & 0x03;
		tempval[1] = (hwinfo[EEPROM_LCK_DELTA] & 0x0C) >> 2;
		rtlefuse->txpwr_fromeprom = true;
		if (IS_92D_D_CUT(rtlpriv->rtlhal.version) ||
		    IS_92D_E_CUT(rtlpriv->rtlhal.version)) {
			rtlefuse->internal_pa_5g[0] =
				!((hwinfo[EEPROM_TSSI_A_5G] & BIT(6)) >> 6);
			rtlefuse->internal_pa_5g[1] =
				!((hwinfo[EEPROM_TSSI_B_5G] & BIT(6)) >> 6);
			rtl_dbg(rtlpriv, COMP_INIT, DBG_DMESG,
				"Is D cut,Internal PA0 %d Internal PA1 %d\n",
				rtlefuse->internal_pa_5g[0],
				rtlefuse->internal_pa_5g[1]);
		}
		rtlefuse->eeprom_c9 = hwinfo[EEPROM_RF_OPT6];
		rtlefuse->eeprom_cc = hwinfo[EEPROM_RF_OPT7];
	} else {
		rtlefuse->eeprom_regulatory = 0;
		rtlefuse->eeprom_thermalmeter = EEPROM_DEFAULT_THERMALMETER;
		rtlefuse->crystalcap = EEPROM_DEFAULT_CRYSTALCAP;
		tempval[0] = 3;
		tempval[1] = tempval[0];
	}

	/* Use default value to fill parameters if
	 * efuse is not filled on some place.
	 */

	/* ThermalMeter from EEPROM */
	if (rtlefuse->eeprom_thermalmeter < 0x06 ||
	    rtlefuse->eeprom_thermalmeter > 0x1c)
		rtlefuse->eeprom_thermalmeter = 0x12;
	rtlefuse->thermalmeter[0] = rtlefuse->eeprom_thermalmeter;

	/* check XTAL_K */
	if (rtlefuse->crystalcap == 0xFF)
		rtlefuse->crystalcap = 0;
	if (rtlefuse->eeprom_regulatory > 3)
		rtlefuse->eeprom_regulatory = 0;

	for (i = 0; i < 2; i++) {
		switch (tempval[i]) {
		case 0:
			tempval[i] = 5;
			break;
		case 1:
			tempval[i] = 4;
			break;
		case 2:
			tempval[i] = 3;
			break;
		case 3:
		default:
			tempval[i] = 0;
			break;
		}
	}

	rtlefuse->delta_iqk = tempval[0];
	if (tempval[1] > 0)
		rtlefuse->delta_lck = tempval[1] - 1;
	if (rtlefuse->eeprom_c9 == 0xFF)
		rtlefuse->eeprom_c9 = 0x00;
	rtl_dbg(rtlpriv, COMP_INTR, DBG_LOUD,
		"EEPROMRegulatory = 0x%x\n", rtlefuse->eeprom_regulatory);
	rtl_dbg(rtlpriv, COMP_INTR, DBG_LOUD,
		"ThermalMeter = 0x%x\n", rtlefuse->eeprom_thermalmeter);
	rtl_dbg(rtlpriv, COMP_INTR, DBG_LOUD,
		"CrystalCap = 0x%x\n", rtlefuse->crystalcap);
	rtl_dbg(rtlpriv, COMP_INTR, DBG_LOUD,
		"Delta_IQK = 0x%x Delta_LCK = 0x%x\n",
		rtlefuse->delta_iqk, rtlefuse->delta_lck);

	for (rfpath = 0; rfpath < RF6052_MAX_PATH; rfpath++) {
		for (ch = 0; ch < CHANNEL_MAX_NUMBER; ch++) {
			group = rtl92d_get_chnlgroup_fromarray((u8)ch);
			if (ch < CHANNEL_MAX_NUMBER_2G)
				rtlefuse->txpwrlevel_cck[rfpath][ch] =
				    pwrinfo.cck_index[rfpath][group];
			rtlefuse->txpwrlevel_ht40_1s[rfpath][ch] =
				    pwrinfo.ht40_1sindex[rfpath][group];
			rtlefuse->txpwr_ht20diff[rfpath][ch] =
				    pwrinfo.ht20indexdiff[rfpath][group];
			rtlefuse->txpwr_legacyhtdiff[rfpath][ch] =
				    pwrinfo.ofdmindexdiff[rfpath][group];
			rtlefuse->pwrgroup_ht20[rfpath][ch] =
				    pwrinfo.ht20maxoffset[rfpath][group];
			rtlefuse->pwrgroup_ht40[rfpath][ch] =
				    pwrinfo.ht40maxoffset[rfpath][group];
			pwr = pwrinfo.ht40_1sindex[rfpath][group];
			diff = pwrinfo.ht40_2sindexdiff[rfpath][group];
			rtlefuse->txpwrlevel_ht40_2s[rfpath][ch] =
				    (pwr > diff) ? (pwr - diff) : 0;
		}
	}
}


/* rtl8192d/hw_common.c: _rtl92d_readpowervalue_fromprom */
static void _rtl92d_readpowervalue_fromprom(struct txpower_info *pwrinfo,
					    u8 *efuse, bool autoloadfail)
{
	u32 rfpath, eeaddr, group, offset, offset1, offset2;
	u8 i, val8;

	memset(pwrinfo, 0, sizeof(struct txpower_info));
	if (autoloadfail) {
		for (group = 0; group < CHANNEL_GROUP_MAX; group++) {
			for (rfpath = 0; rfpath < RF6052_MAX_PATH; rfpath++) {
				if (group < CHANNEL_GROUP_MAX_2G) {
					pwrinfo->cck_index[rfpath][group] =
					    EEPROM_DEFAULT_TXPOWERLEVEL_2G;
					pwrinfo->ht40_1sindex[rfpath][group] =
					    EEPROM_DEFAULT_TXPOWERLEVEL_2G;
				} else {
					pwrinfo->ht40_1sindex[rfpath][group] =
					    EEPROM_DEFAULT_TXPOWERLEVEL_5G;
				}
				pwrinfo->ht40_2sindexdiff[rfpath][group] =
				    EEPROM_DEFAULT_HT40_2SDIFF;
				pwrinfo->ht20indexdiff[rfpath][group] =
				    EEPROM_DEFAULT_HT20_DIFF;
				pwrinfo->ofdmindexdiff[rfpath][group] =
				    EEPROM_DEFAULT_LEGACYHTTXPOWERDIFF;
				pwrinfo->ht40maxoffset[rfpath][group] =
				    EEPROM_DEFAULT_HT40_PWRMAXOFFSET;
				pwrinfo->ht20maxoffset[rfpath][group] =
				    EEPROM_DEFAULT_HT20_PWRMAXOFFSET;
			}
		}
		for (i = 0; i < 3; i++) {
			pwrinfo->tssi_a[i] = EEPROM_DEFAULT_TSSI;
			pwrinfo->tssi_b[i] = EEPROM_DEFAULT_TSSI;
		}
		return;
	}

	/* Maybe autoload OK,buf the tx power index value is not filled.
	 * If we find it, we set it to default value.
	 */
	for (rfpath = 0; rfpath < RF6052_MAX_PATH; rfpath++) {
		for (group = 0; group < CHANNEL_GROUP_MAX_2G; group++) {
			eeaddr = EEPROM_CCK_TX_PWR_INX_2G + (rfpath * 3) + group;

			pwrinfo->cck_index[rfpath][group] =
				efuse[eeaddr] == 0xFF ?
				(eeaddr > 0x7B ?
				 EEPROM_DEFAULT_TXPOWERLEVEL_5G :
				 EEPROM_DEFAULT_TXPOWERLEVEL_2G) :
				efuse[eeaddr];
		}
	}
	for (rfpath = 0; rfpath < RF6052_MAX_PATH; rfpath++) {
		for (group = 0; group < CHANNEL_GROUP_MAX; group++) {
			offset1 = group / 3;
			offset2 = group % 3;
			eeaddr = EEPROM_HT40_1S_TX_PWR_INX_2G + (rfpath * 3);
			eeaddr += offset2 + offset1 * 21;

			pwrinfo->ht40_1sindex[rfpath][group] =
				efuse[eeaddr] == 0xFF ?
				(eeaddr > 0x7B ?
				 EEPROM_DEFAULT_TXPOWERLEVEL_5G :
				 EEPROM_DEFAULT_TXPOWERLEVEL_2G) :
				efuse[eeaddr];
		}
	}

	/* These just for 92D efuse offset. */
	for (group = 0; group < CHANNEL_GROUP_MAX; group++) {
		for (rfpath = 0; rfpath < RF6052_MAX_PATH; rfpath++) {
			offset1 = group / 3;
			offset2 = group % 3;
			offset = offset2 + offset1 * 21;

			val8 = efuse[EEPROM_HT40_2S_TX_PWR_INX_DIFF_2G + offset];
			if (val8 != 0xFF)
				pwrinfo->ht40_2sindexdiff[rfpath][group] =
				    (val8 >> (rfpath * 4)) & 0xF;
			else
				pwrinfo->ht40_2sindexdiff[rfpath][group] =
				    EEPROM_DEFAULT_HT40_2SDIFF;

			val8 = efuse[EEPROM_HT20_TX_PWR_INX_DIFF_2G + offset];
			if (val8 != 0xFF)
				pwrinfo->ht20indexdiff[rfpath][group] =
				    (val8 >> (rfpath * 4)) & 0xF;
			else
				pwrinfo->ht20indexdiff[rfpath][group] =
				    EEPROM_DEFAULT_HT20_DIFF;

			val8 = efuse[EEPROM_OFDM_TX_PWR_INX_DIFF_2G + offset];
			if (val8 != 0xFF)
				pwrinfo->ofdmindexdiff[rfpath][group] =
				    (val8 >> (rfpath * 4)) & 0xF;
			else
				pwrinfo->ofdmindexdiff[rfpath][group] =
				    EEPROM_DEFAULT_LEGACYHTTXPOWERDIFF;

			val8 = efuse[EEPROM_HT40_MAX_PWR_OFFSET_2G + offset];
			if (val8 != 0xFF)
				pwrinfo->ht40maxoffset[rfpath][group] =
				    (val8 >> (rfpath * 4)) & 0xF;
			else
				pwrinfo->ht40maxoffset[rfpath][group] =
				    EEPROM_DEFAULT_HT40_PWRMAXOFFSET;

			val8 = efuse[EEPROM_HT20_MAX_PWR_OFFSET_2G + offset];
			if (val8 != 0xFF)
				pwrinfo->ht20maxoffset[rfpath][group] =
				    (val8 >> (rfpath * 4)) & 0xF;
			else
				pwrinfo->ht20maxoffset[rfpath][group] =
				    EEPROM_DEFAULT_HT20_PWRMAXOFFSET;
		}
	}

	if (efuse[EEPROM_TSSI_A_5G] != 0xFF) {
		/* 5GL */
		pwrinfo->tssi_a[0] = efuse[EEPROM_TSSI_A_5G] & 0x3F;
		pwrinfo->tssi_b[0] = efuse[EEPROM_TSSI_B_5G] & 0x3F;
		/* 5GM */
		pwrinfo->tssi_a[1] = efuse[EEPROM_TSSI_AB_5G] & 0x3F;
		pwrinfo->tssi_b[1] = (efuse[EEPROM_TSSI_AB_5G] & 0xC0) >> 6 |
				     (efuse[EEPROM_TSSI_AB_5G + 1] & 0x0F) << 2;
		/* 5GH */
		pwrinfo->tssi_a[2] = (efuse[EEPROM_TSSI_AB_5G + 1] & 0xF0) >> 4 |
				     (efuse[EEPROM_TSSI_AB_5G + 2] & 0x03) << 4;
		pwrinfo->tssi_b[2] = (efuse[EEPROM_TSSI_AB_5G + 2] & 0xFC) >> 2;
	} else {
		for (i = 0; i < 3; i++) {
			pwrinfo->tssi_a[i] = EEPROM_DEFAULT_TSSI;
			pwrinfo->tssi_b[i] = EEPROM_DEFAULT_TSSI;
		}
	}
}


/* rtl8192d/rf_common.c: _rtl92d_write_ofdm_power_reg */
static void _rtl92d_write_ofdm_power_reg(struct ieee80211_hw *hw,
					 u8 index, u32 *pvalue)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	static const u16 regoffset_a[6] = {
		RTXAGC_A_RATE18_06, RTXAGC_A_RATE54_24,
		RTXAGC_A_MCS03_MCS00, RTXAGC_A_MCS07_MCS04,
		RTXAGC_A_MCS11_MCS08, RTXAGC_A_MCS15_MCS12
	};
	static const u16 regoffset_b[6] = {
		RTXAGC_B_RATE18_06, RTXAGC_B_RATE54_24,
		RTXAGC_B_MCS03_MCS00, RTXAGC_B_MCS07_MCS04,
		RTXAGC_B_MCS11_MCS08, RTXAGC_B_MCS15_MCS12
	};
	u8 i, rf, pwr_val[4];
	u32 writeval;
	u16 regoffset;

	for (rf = 0; rf < 2; rf++) {
		writeval = pvalue[rf];
		for (i = 0; i < 4; i++) {
			pwr_val[i] = (u8)((writeval & (0x7f <<
				     (i * 8))) >> (i * 8));
			if (pwr_val[i] > RF6052_MAX_TX_PWR)
				pwr_val[i] = RF6052_MAX_TX_PWR;
		}
		writeval = (pwr_val[3] << 24) | (pwr_val[2] << 16) |
			   (pwr_val[1] << 8) | pwr_val[0];
		if (rf == 0)
			regoffset = regoffset_a[index];
		else
			regoffset = regoffset_b[index];
		rtl_set_bbreg(hw, regoffset, MASKDWORD, writeval);
		RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
			"Set 0x%x = %08x\n", regoffset, writeval);
		if (((get_rf_type(rtlphy) == RF_2T2R) &&
		     (regoffset == RTXAGC_A_MCS15_MCS12 ||
		      regoffset == RTXAGC_B_MCS15_MCS12)) ||
		    ((get_rf_type(rtlphy) != RF_2T2R) &&
		     (regoffset == RTXAGC_A_MCS07_MCS04 ||
		      regoffset == RTXAGC_B_MCS07_MCS04))) {
			writeval = pwr_val[3];
			if (regoffset == RTXAGC_A_MCS15_MCS12 ||
			    regoffset == RTXAGC_A_MCS07_MCS04)
				regoffset = 0xc90;
			if (regoffset == RTXAGC_B_MCS15_MCS12 ||
			    regoffset == RTXAGC_B_MCS07_MCS04)
				regoffset = 0xc98;
			for (i = 0; i < 3; i++) {
				if (i != 2)
					writeval = (writeval > 8) ?
						   (writeval - 8) : 0;
				else
					writeval = (writeval > 6) ?
						   (writeval - 6) : 0;
				rtl_write_byte(rtlpriv, (u32)(regoffset + i),
					       (u8)writeval);
			}
		}
	}
}


/* rtl8192du/phy.c: _rtl92du_phy_bb_config */
static bool _rtl92du_phy_bb_config(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtlpriv);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	bool ret;

	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE, "==>\n");
	ret = _rtl92du_phy_config_bb(hw, BASEBAND_CONFIG_PHY_REG);
	if (!ret) {
		pr_err("Write BB Reg Fail!!\n");
		return false;
	}

	if (!rtlefuse->autoload_failflag) {
		rtlphy->pwrgroup_cnt = 0;
		ret = _rtl92du_phy_config_bb_pg(hw, BASEBAND_CONFIG_PHY_REG);
	}
	if (!ret) {
		pr_err("BB_PG Reg Fail!!\n");
		return false;
	}

	ret = _rtl92du_phy_config_bb(hw, BASEBAND_CONFIG_AGC_TAB);
	if (!ret) {
		pr_err("AGC Table Fail\n");
		return false;
	}

	rtlphy->cck_high_power = (bool)rtl_get_bbreg(hw,
						     RFPGA0_XA_HSSIPARAMETER2,
						     0x200);

	return true;
}


/* rtl8192du/phy.c: _rtl92du_phy_config_bb */
static bool _rtl92du_phy_config_bb(struct ieee80211_hw *hw, u8 configtype)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	u16 phy_reg_arraylen, agctab_arraylen = 0;
	const u32 *agctab_array_table = NULL;
	const u32 *phy_regarray_table;
	int i;

	/* Normal chip, Mac0 use AGC_TAB.txt for 2G and 5G band. */
	if (rtlhal->interfaceindex == 0) {
		agctab_arraylen = AGCTAB_ARRAYLENGTH;
		agctab_array_table = rtl8192du_agctab_array;
		rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
			" ===> phy:MAC0, Rtl819XAGCTAB_Array\n");
	} else {
		if (rtlhal->current_bandtype == BAND_ON_2_4G) {
			agctab_arraylen = AGCTAB_2G_ARRAYLENGTH;
			agctab_array_table = rtl8192du_agctab_2garray;
			rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
				" ===> phy:MAC1, Rtl819XAGCTAB_2GArray\n");
		} else {
			agctab_arraylen = AGCTAB_5G_ARRAYLENGTH;
			agctab_array_table = rtl8192du_agctab_5garray;
			rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
				" ===> phy:MAC1, Rtl819XAGCTAB_5GArray\n");
		}
	}
	phy_reg_arraylen = PHY_REG_2T_ARRAYLENGTH;
	phy_regarray_table = rtl8192du_phy_reg_2tarray;
	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
		" ===> phy:Rtl819XPHY_REG_Array_PG\n");

	if (configtype == BASEBAND_CONFIG_PHY_REG) {
		for (i = 0; i < phy_reg_arraylen; i = i + 2) {
			rtl_addr_delay(phy_regarray_table[i]);
			rtl_set_bbreg(hw, phy_regarray_table[i], MASKDWORD,
				      phy_regarray_table[i + 1]);
			udelay(1);
			rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE,
				"The phy_regarray_table[0] is %x Rtl819XPHY_REGArray[1] is %x\n",
				phy_regarray_table[i],
				phy_regarray_table[i + 1]);
		}
	} else if (configtype == BASEBAND_CONFIG_AGC_TAB) {
		for (i = 0; i < agctab_arraylen; i = i + 2) {
			rtl_set_bbreg(hw, agctab_array_table[i],
				      MASKDWORD, agctab_array_table[i + 1]);

			/* Add 1us delay between BB/RF register setting. */
			udelay(1);

			rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE,
				"AGC table %u %u\n",
				agctab_array_table[i],
				agctab_array_table[i + 1]);
		}
		rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
			"Normal Chip, loaded AGC table\n");
	}
	return true;
}


/* rtl8192du/phy.c: _rtl92du_phy_config_bb_pg */
static bool _rtl92du_phy_config_bb_pg(struct ieee80211_hw *hw, u8 configtype)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	const u32 *phy_regarray_table_pg;
	u16 phy_regarray_pg_len;
	int i;

	phy_regarray_pg_len = PHY_REG_ARRAY_PG_LENGTH;
	phy_regarray_table_pg = rtl8192du_phy_reg_array_pg;

	if (configtype == BASEBAND_CONFIG_PHY_REG) {
		for (i = 0; i < phy_regarray_pg_len; i = i + 3) {
			rtl_addr_delay(phy_regarray_table_pg[i]);
			rtl92d_store_pwrindex_diffrate_offset(hw,
				phy_regarray_table_pg[i],
				phy_regarray_table_pg[i + 1],
				phy_regarray_table_pg[i + 2]);
		}
	} else {
		rtl_dbg(rtlpriv, COMP_SEND, DBG_TRACE,
			"configtype != BaseBand_Config_PHY_REG\n");
	}
	return true;
}


/* rtl8192du/phy.c: _rtl92du_phy_iq_calibrate */
static void _rtl92du_phy_iq_calibrate(struct ieee80211_hw *hw, long result[][8],
				      u8 t, bool is2t)
{
	static const u32 adda_reg[IQK_ADDA_REG_NUM] = {
		RFPGA0_XCD_SWITCHCONTROL, RBLUE_TOOTH, RRX_WAIT_CCA,
		RTX_CCK_RFON, RTX_CCK_BBON, RTX_OFDM_RFON, RTX_OFDM_BBON,
		RTX_TO_RX, RTX_TO_TX, RRX_CCK, RRX_OFDM, RRX_WAIT_RIFS,
		RRX_TO_RX, RSTANDBY, RSLEEP, RPMPD_ANAEN
	};
	static const u32 iqk_mac_reg[IQK_MAC_REG_NUM] = {
		REG_TXPAUSE, REG_BCN_CTRL, REG_BCN_CTRL_1, REG_GPIO_MUXCFG
	};
	static const u32 iqk_bb_reg[IQK_BB_REG_NUM] = {
		RFPGA0_XAB_RFINTERFACESW, RFPGA0_XA_RFINTERFACEOE,
		RFPGA0_XB_RFINTERFACEOE, ROFDM0_TRMUXPAR,
		RFPGA0_XCD_RFINTERFACESW, ROFDM0_TRXPATHENABLE,
		RFPGA0_RFMOD, RFPGA0_ANALOGPARAMETER4,
		ROFDM0_XAAGCCORE1, ROFDM0_XBAGCCORE1
	};
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	const u32 retrycount = 2;
	u8 patha_ok, pathb_ok;
	u32 bbvalue;
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "IQK for 2.4G :Start!!!\n");
	if (t == 0) {
		bbvalue = rtl_get_bbreg(hw, RFPGA0_RFMOD, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "==>0x%08x\n", bbvalue);
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "IQ Calibration for %s\n",
			is2t ? "2T2R" : "1T1R");

		/*  Save ADDA parameters, turn Path A ADDA on */
		rtl92d_phy_save_adda_registers(hw, adda_reg,
					       rtlphy->adda_backup,
					       IQK_ADDA_REG_NUM);
		rtl92d_phy_save_mac_registers(hw, iqk_mac_reg,
					      rtlphy->iqk_mac_backup);
		rtl92d_phy_save_adda_registers(hw, iqk_bb_reg,
					       rtlphy->iqk_bb_backup,
					       IQK_BB_REG_NUM);
	}
	rtl92d_phy_path_adda_on(hw, adda_reg, true, is2t);

	rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0x01017038);

	if (t == 0)
		rtlphy->rfpi_enable = (u8)rtl_get_bbreg(hw,
				RFPGA0_XA_HSSIPARAMETER1, BIT(8));

	/*  Switch BB to PI mode to do IQ Calibration. */
	if (!rtlphy->rfpi_enable)
		_rtl92du_phy_pimode_switch(hw, true);

	rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD, BCCKEN, 0x00);
	rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE, MASKDWORD, 0x03a05600);
	rtl_set_bbreg(hw, ROFDM0_TRMUXPAR, MASKDWORD, 0x000800e4);
	rtl_set_bbreg(hw, RFPGA0_XCD_RFINTERFACESW, MASKDWORD, 0x22204000);
	rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4, 0xf00000, 0x0f);
	if (is2t) {
		rtl_set_bbreg(hw, RFPGA0_XA_LSSIPARAMETER, MASKDWORD,
			      0x00010000);
		rtl_set_bbreg(hw, RFPGA0_XB_LSSIPARAMETER, MASKDWORD,
			      0x00010000);
	}

	/* MAC settings */
	rtl92d_phy_mac_setting_calibration(hw, iqk_mac_reg,
					   rtlphy->iqk_mac_backup);

	/* Page B init */
	rtl_set_bbreg(hw, RCONFIG_ANTA, MASKDWORD, 0x0f600000);
	if (is2t)
		rtl_set_bbreg(hw, RCONFIG_ANTB, MASKDWORD, 0x0f600000);

	/* IQ calibration setting */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "IQK setting!\n");
	rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x808000);
	rtl_set_bbreg(hw, RTX_IQK, MASKDWORD, 0x01007c00);
	rtl_set_bbreg(hw, RRX_IQK, MASKDWORD, 0x01004800);

	for (i = 0; i < retrycount; i++) {
		patha_ok = _rtl92du_phy_patha_iqk(hw, is2t);
		if (patha_ok == 0x03) {
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path A IQK Success!!\n");
			result[t][0] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_A,
						     MASK_IQK_RESULT);
			result[t][1] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_A,
						     MASK_IQK_RESULT);
			result[t][2] = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_A_2,
						     MASK_IQK_RESULT);
			result[t][3] = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2,
						     MASK_IQK_RESULT);
			break;
		} else if (i == (retrycount - 1) && patha_ok == 0x01) {
			/* Tx IQK OK */
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path A IQK Only  Tx Success!!\n");

			result[t][0] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_A,
						     MASK_IQK_RESULT);
			result[t][1] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_A,
						     MASK_IQK_RESULT);
		}
	}
	if (patha_ok == 0x00)
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "Path A IQK failed!!\n");

	if (is2t) {
		_rtl92du_phy_patha_standby(hw);
		/* Turn Path B ADDA on */
		rtl92d_phy_path_adda_on(hw, adda_reg, false, is2t);

		for (i = 0; i < retrycount; i++) {
			pathb_ok = _rtl92du_phy_pathb_iqk(hw);
			if (pathb_ok == 0x03) {
				RTPRINT(rtlpriv, FINIT, INIT_IQK,
					"Path B IQK Success!!\n");
				result[t][4] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_B,
							     MASK_IQK_RESULT);
				result[t][5] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_B,
							     MASK_IQK_RESULT);
				result[t][6] = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_B_2,
							     MASK_IQK_RESULT);
				result[t][7] = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_B_2,
							     MASK_IQK_RESULT);
				break;
			} else if (i == (retrycount - 1) && pathb_ok == 0x01) {
				/* Tx IQK OK */
				RTPRINT(rtlpriv, FINIT, INIT_IQK,
					"Path B Only Tx IQK Success!!\n");
				result[t][4] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_B,
							     MASK_IQK_RESULT);
				result[t][5] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_B,
							     MASK_IQK_RESULT);
			}
		}
		if (pathb_ok == 0x00)
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path B IQK failed!!\n");
	}

	/* Back to BB mode, load original value */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"IQK:Back to BB mode, load original value!\n");

	rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x000000);

	if (t != 0) {
		/* Switch back BB to SI mode after finish IQ Calibration. */
		if (!rtlphy->rfpi_enable)
			_rtl92du_phy_pimode_switch(hw, false);

		/* Reload ADDA power saving parameters */
		_rtl92du_phy_reload_adda_registers(hw, adda_reg,
						   rtlphy->adda_backup,
						   IQK_ADDA_REG_NUM);

		/* Reload MAC parameters */
		_rtl92du_phy_reload_mac_registers(hw, iqk_mac_reg,
						  rtlphy->iqk_mac_backup);

		if (is2t)
			_rtl92du_phy_reload_adda_registers(hw, iqk_bb_reg,
							   rtlphy->iqk_bb_backup,
							   IQK_BB_REG_NUM);
		else
			_rtl92du_phy_reload_adda_registers(hw, iqk_bb_reg,
							   rtlphy->iqk_bb_backup,
							   IQK_BB_REG_NUM - 1);

		/* load 0xe30 IQC default value */
		rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x01008c00);
		rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x01008c00);
	}
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "<==\n");
}


/* rtl8192du/phy.c: _rtl92du_phy_iq_calibrate_5g_normal */
static void _rtl92du_phy_iq_calibrate_5g_normal(struct ieee80211_hw *hw,
						long result[][8], u8 t)
{
	static const u32 adda_reg[IQK_ADDA_REG_NUM] = {
		RFPGA0_XCD_SWITCHCONTROL, RBLUE_TOOTH, RRX_WAIT_CCA,
		RTX_CCK_RFON, RTX_CCK_BBON, RTX_OFDM_RFON, RTX_OFDM_BBON,
		RTX_TO_RX, RTX_TO_TX, RRX_CCK, RRX_OFDM, RRX_WAIT_RIFS,
		RRX_TO_RX, RSTANDBY, RSLEEP, RPMPD_ANAEN
	};
	static const u32 iqk_mac_reg[IQK_MAC_REG_NUM] = {
		REG_TXPAUSE, REG_BCN_CTRL, REG_BCN_CTRL_1, REG_GPIO_MUXCFG
	};
	static const u32 iqk_bb_reg[IQK_BB_REG_NUM] = {
		RFPGA0_XAB_RFINTERFACESW, RFPGA0_XA_RFINTERFACEOE,
		RFPGA0_XB_RFINTERFACEOE, ROFDM0_TRMUXPAR,
		RFPGA0_XCD_RFINTERFACESW, ROFDM0_TRXPATHENABLE,
		RFPGA0_RFMOD, RFPGA0_ANALOGPARAMETER4,
		ROFDM0_XAAGCCORE1, ROFDM0_XBAGCCORE1
	};
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	bool is2t = IS_92D_SINGLEPHY(rtlhal->version);
	u8 patha_ok, pathb_ok;
	bool rf_path_div;
	u32 bbvalue;

	/* Note: IQ calibration must be performed after loading
	 * PHY_REG.txt , and radio_a, radio_b.txt
	 */

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "IQK for 5G NORMAL:Start!!!\n");

	mdelay(IQK_DELAY_TIME * 20);

	if (t == 0) {
		bbvalue = rtl_get_bbreg(hw, RFPGA0_RFMOD, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "==>0x%08x\n", bbvalue);
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "IQ Calibration for %s\n",
			is2t ? "2T2R" : "1T1R");

		/* Save ADDA parameters, turn Path A ADDA on */
		rtl92d_phy_save_adda_registers(hw, adda_reg,
					       rtlphy->adda_backup,
					       IQK_ADDA_REG_NUM);
		rtl92d_phy_save_mac_registers(hw, iqk_mac_reg,
					      rtlphy->iqk_mac_backup);
		if (is2t)
			rtl92d_phy_save_adda_registers(hw, iqk_bb_reg,
						       rtlphy->iqk_bb_backup,
						       IQK_BB_REG_NUM);
		else
			rtl92d_phy_save_adda_registers(hw, iqk_bb_reg,
						       rtlphy->iqk_bb_backup,
						       IQK_BB_REG_NUM - 1);
	}

	rf_path_div = rtl_get_bbreg(hw, 0xb30, BIT(27));
	rtl92d_phy_path_adda_on(hw, adda_reg, !rf_path_div, is2t);

	if (t == 0)
		rtlphy->rfpi_enable = rtl_get_bbreg(hw,
						    RFPGA0_XA_HSSIPARAMETER1,
						    BIT(8));

	/*  Switch BB to PI mode to do IQ Calibration. */
	if (!rtlphy->rfpi_enable)
		_rtl92du_phy_pimode_switch(hw, true);

	/* MAC settings */
	rtl92d_phy_mac_setting_calibration(hw, iqk_mac_reg,
					   rtlphy->iqk_mac_backup);

	rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD, BCCKEN, 0x00);
	rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE, MASKDWORD, 0x03a05600);
	rtl_set_bbreg(hw, ROFDM0_TRMUXPAR, MASKDWORD, 0x000800e4);
	rtl_set_bbreg(hw, RFPGA0_XCD_RFINTERFACESW, MASKDWORD, 0x22208000);
	rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4, 0xf00000, 0x0f);

	/* Page A AP setting for IQK */
	rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0);
	rtl_set_bbreg(hw, RCONFIG_ANTA, MASKDWORD, 0x20000000);
	if (is2t) {
		/* Page B AP setting for IQK */
		rtl_set_bbreg(hw, RPDP_ANTB, MASKDWORD, 0);
		rtl_set_bbreg(hw, RCONFIG_ANTB, MASKDWORD, 0x20000000);
	}

	/* IQ calibration setting  */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "IQK setting!\n");
	rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x808000);
	rtl_set_bbreg(hw, RTX_IQK, MASKDWORD, 0x10007c00);
	rtl_set_bbreg(hw, RRX_IQK, MASKDWORD, 0x01004800);

	patha_ok = _rtl92du_phy_patha_iqk_5g_normal(hw, is2t);
	if (patha_ok == 0x03) {
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path A IQK Success!!\n");
		result[t][0] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_A,
					     MASK_IQK_RESULT);
		result[t][1] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_A,
					     MASK_IQK_RESULT);
		result[t][2] = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_A_2,
					     MASK_IQK_RESULT);
		result[t][3] = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2,
					     MASK_IQK_RESULT);
	} else if (patha_ok == 0x01) {	/* Tx IQK OK */
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"Path A IQK Only  Tx Success!!\n");

		result[t][0] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_A,
					     MASK_IQK_RESULT);
		result[t][1] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_A,
					     MASK_IQK_RESULT);
	} else {
		rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x000000);
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "0xe70 = %#x\n",
			rtl_get_bbreg(hw, RRX_WAIT_CCA, MASKDWORD));
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "RF path A 0x0 = %#x\n",
			rtl_get_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK));
		rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x808000);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path A IQK Fail!!\n");
	}

	if (is2t) {
		/* _rtl92d_phy_patha_standby(hw); */
		/* Turn Path B ADDA on  */
		rtl92d_phy_path_adda_on(hw, adda_reg, false, is2t);

		pathb_ok = _rtl92du_phy_pathb_iqk_5g_normal(hw);
		if (pathb_ok == 0x03) {
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path B IQK Success!!\n");
			result[t][4] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_B,
						     MASK_IQK_RESULT);
			result[t][5] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_B,
						     MASK_IQK_RESULT);
			result[t][6] = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_B_2,
						     MASK_IQK_RESULT);
			result[t][7] = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_B_2,
						     MASK_IQK_RESULT);
		} else if (pathb_ok == 0x01) { /* Tx IQK OK */
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path B Only Tx IQK Success!!\n");
			result[t][4] = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_B,
						     MASK_IQK_RESULT);
			result[t][5] = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_B,
						     MASK_IQK_RESULT);
		} else {
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path B IQK failed!!\n");
		}
	}

	/* Back to BB mode, load original value */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"IQK:Back to BB mode, load original value!\n");
	rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0);

	if (is2t)
		_rtl92du_phy_reload_adda_registers(hw, iqk_bb_reg,
						   rtlphy->iqk_bb_backup,
						   IQK_BB_REG_NUM);
	else
		_rtl92du_phy_reload_adda_registers(hw, iqk_bb_reg,
						   rtlphy->iqk_bb_backup,
						   IQK_BB_REG_NUM - 1);

	/* path A IQ path to DP block */
	rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0x010170b8);
	if (is2t) /* path B IQ path to DP block */
		rtl_set_bbreg(hw, RPDP_ANTB, MASKDWORD, 0x010170b8);

	/* Reload MAC parameters */
	_rtl92du_phy_reload_mac_registers(hw, iqk_mac_reg,
					  rtlphy->iqk_mac_backup);

	/* Switch back BB to SI mode after finish IQ Calibration. */
	if (!rtlphy->rfpi_enable)
		_rtl92du_phy_pimode_switch(hw, false);

	/* Reload ADDA power saving parameters */
	_rtl92du_phy_reload_adda_registers(hw, adda_reg,
					   rtlphy->adda_backup,
					   IQK_ADDA_REG_NUM);

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "<==\n");
}


/* rtl8192du/phy.c: _rtl92du_phy_lc_calibrate_sw */
static void _rtl92du_phy_lc_calibrate_sw(struct ieee80211_hw *hw, bool is2t)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u32 curvecount_val[CV_CURVE_CNT * 2];
	u16 timeout = 800, timecount = 0;
	u32 u4tmp, offset, rf_syn_g4[2];
	u8 tmpreg, index, rf_mode[2];
	u8 path = is2t ? 2 : 1;
	u8 i;

	/* Check continuous TX and Packet TX */
	tmpreg = rtl_read_byte(rtlpriv, 0xd03);
	if ((tmpreg & 0x70) != 0)
		/* if Deal with contisuous TX case, disable all continuous TX */
		rtl_write_byte(rtlpriv, 0xd03, tmpreg & 0x8F);
	else
		/* if Deal with Packet TX case, block all queues */
		rtl_write_byte(rtlpriv, REG_TXPAUSE, 0xFF);

	rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4, 0xF00000, 0x0F);

	for (index = 0; index < path; index++) {
		/* 1. Read original RF mode */
		offset = index == 0 ? ROFDM0_XAAGCCORE1 : ROFDM0_XBAGCCORE1;
		rf_mode[index] = rtl_read_byte(rtlpriv, offset);

		/* 2. Set RF mode = standby mode */
		rtl_set_rfreg(hw, (enum radio_path)index, RF_AC,
			      RFREG_OFFSET_MASK, 0x010000);

		rf_syn_g4[index] = rtl_get_rfreg(hw, index, RF_SYN_G4,
						 RFREG_OFFSET_MASK);
		rtl_set_rfreg(hw, index, RF_SYN_G4, 0x700, 0x7);

		/* switch CV-curve control by LC-calibration */
		rtl_set_rfreg(hw, (enum radio_path)index, RF_SYN_G7,
			      BIT(17), 0x0);

		/* 4. Set LC calibration begin */
		rtl_set_rfreg(hw, (enum radio_path)index, RF_CHNLBW,
			      0x08000, 0x01);
	}

	for (index = 0; index < path; index++) {
		u4tmp = rtl_get_rfreg(hw, (enum radio_path)index, RF_SYN_G6,
				      RFREG_OFFSET_MASK);

		while ((!(u4tmp & BIT(11))) && timecount <= timeout) {
			mdelay(50);
			timecount += 50;
			u4tmp = rtl_get_rfreg(hw, (enum radio_path)index,
					      RF_SYN_G6, RFREG_OFFSET_MASK);
		}
		if (!(u4tmp & BIT(11))) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"PHY_LCK finish delay for %d ms=2\n", timecount);
	}

	if ((tmpreg & 0x70) != 0)
		rtl_write_byte(rtlpriv, 0xd03, tmpreg);
	else /* Deal with Packet TX case */
		rtl_write_byte(rtlpriv, REG_TXPAUSE, 0x00);

	rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4, 0xF00000, 0x00);

	for (index = 0; index < path; index++) {
		rtl_get_rfreg(hw, index, RF_SYN_G4, RFREG_OFFSET_MASK);

		if (index == 0 && rtlhal->interfaceindex == 0) {
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"path-A / 5G LCK\n");
		} else {
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"path-B / 2.4G LCK\n");
		}

		memset(curvecount_val, 0, sizeof(curvecount_val));

		/* Set LC calibration off */
		rtl_set_rfreg(hw, (enum radio_path)index, RF_CHNLBW,
			      0x08000, 0x0);

		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "set RF 0x18[15] = 0\n");

		/* save Curve-counting number */
		for (i = 0; i < CV_CURVE_CNT; i++) {
			u32 readval = 0, readval2 = 0;

			rtl_set_rfreg(hw, (enum radio_path)index, 0x3F,
				      0x7f, i);

			rtl_set_rfreg(hw, (enum radio_path)index, 0x4D,
				      RFREG_OFFSET_MASK, 0x0);

			readval = rtl_get_rfreg(hw, (enum radio_path)index,
						0x4F, RFREG_OFFSET_MASK);
			curvecount_val[2 * i + 1] = (readval & 0xfffe0) >> 5;

			/* reg 0x4f [4:0] */
			/* reg 0x50 [19:10] */
			readval2 = rtl_get_rfreg(hw, (enum radio_path)index,
						 0x50, 0xffc00);
			curvecount_val[2 * i] = (((readval & 0x1F) << 10) |
						 readval2);
		}

		if (index == 0 && rtlhal->interfaceindex == 0)
			rtl92d_phy_calc_curvindex(hw, targetchnl_5g,
						  curvecount_val,
						  true, rtlpriv->curveindex_5g);
		else
			rtl92d_phy_calc_curvindex(hw, targetchnl_2g,
						  curvecount_val,
						  false, rtlpriv->curveindex_2g);

		/* switch CV-curve control mode */
		rtl_set_rfreg(hw, (enum radio_path)index, RF_SYN_G7,
			      BIT(17), 0x1);
	}

	/* Restore original situation  */
	for (index = 0; index < path; index++) {
		rtl_set_rfreg(hw, index, RF_SYN_G4, RFREG_OFFSET_MASK,
			      rf_syn_g4[index]);

		offset = index == 0 ? ROFDM0_XAAGCCORE1 : ROFDM0_XBAGCCORE1;
		rtl_write_byte(rtlpriv, offset, 0x50);
		rtl_write_byte(rtlpriv, offset, rf_mode[index]);
	}

	_rtl92du_phy_reload_lck_setting(hw, rtlpriv->phy.current_channel);
}


/* rtl8192du/phy.c: _rtl92du_phy_patha_fill_iqk_matrix */
static void _rtl92du_phy_patha_fill_iqk_matrix(struct ieee80211_hw *hw,
					       bool iqk_ok, long result[][8],
					       u8 final_candidate, bool txonly)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u32 oldval_0, val_x, tx0_a, reg;
	long val_y, tx0_c;
	bool is2t = IS_92D_SINGLEPHY(rtlhal->version) ||
		    rtlhal->macphymode == DUALMAC_DUALPHY;

	if (rtlhal->current_bandtype == BAND_ON_5G) {
		_rtl92du_phy_patha_fill_iqk_matrix_5g_normal(hw, iqk_ok, result,
							     final_candidate,
							     txonly);
		return;
	}

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Path A IQ Calibration %s !\n", iqk_ok ? "Success" : "Failed");
	if (final_candidate == 0xFF || !iqk_ok)
		return;

	/* OFDM0_D */
	oldval_0 = rtl_get_bbreg(hw, ROFDM0_XATXIQIMBALANCE, 0xffc00000);

	val_x = result[final_candidate][0];
	if ((val_x & 0x00000200) != 0)
		val_x = val_x | 0xFFFFFC00;

	tx0_a = (val_x * oldval_0) >> 8;
	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"X = 0x%x, tx0_a = 0x%x, oldval_0 0x%x\n",
		val_x, tx0_a, oldval_0);
	rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE, 0x3FF, tx0_a);
	rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(24),
		      ((val_x * oldval_0 >> 7) & 0x1));

	val_y = result[final_candidate][1];
	if ((val_y & 0x00000200) != 0)
		val_y = val_y | 0xFFFFFC00;

	/* path B IQK result + 3 */
	if (rtlhal->interfaceindex == 1 &&
	    rtlhal->current_bandtype == BAND_ON_5G)
		val_y += 3;

	tx0_c = (val_y * oldval_0) >> 8;
	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Y = 0x%lx, tx0_c = 0x%lx\n",
		val_y, tx0_c);

	rtl_set_bbreg(hw, ROFDM0_XCTXAFE, 0xF0000000, (tx0_c & 0x3C0) >> 6);
	rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE, 0x003F0000, tx0_c & 0x3F);
	if (is2t)
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(26),
			      (val_y * oldval_0 >> 7) & 0x1);

	RTPRINT(rtlpriv, FINIT, INIT_IQK, "0xC80 = 0x%x\n",
		rtl_get_bbreg(hw, ROFDM0_XATXIQIMBALANCE,
			      MASKDWORD));

	if (txonly) {
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "only Tx OK\n");
		return;
	}

	reg = result[final_candidate][2];
	rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, 0x3FF, reg);
	reg = result[final_candidate][3] & 0x3F;
	rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, 0xFC00, reg);
	reg = (result[final_candidate][3] >> 6) & 0xF;
	rtl_set_bbreg(hw, ROFDM0_RXIQEXTANTA, 0xF0000000, reg);
}


/* rtl8192du/phy.c: _rtl92du_phy_patha_fill_iqk_matrix_5g_normal */
static void _rtl92du_phy_patha_fill_iqk_matrix_5g_normal(struct ieee80211_hw *hw,
							 bool iqk_ok,
							 long result[][8],
							 u8 final_candidate,
							 bool txonly)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u32 val_x, reg;
	int val_y;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Path A IQ Calibration %s !\n", iqk_ok ? "Success" : "Failed");
	if (iqk_ok && final_candidate != 0xFF) {
		val_x = result[final_candidate][0];
		if ((val_x & 0x00000200) != 0)
			val_x = val_x | 0xFFFFFC00;

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "X = 0x%x\n", val_x);
		rtl_set_bbreg(hw, RTX_IQK_TONE_A, 0x3FF0000, val_x);
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(24), 0);

		val_y = result[final_candidate][1];
		if ((val_y & 0x00000200) != 0)
			val_y = val_y | 0xFFFFFC00;

		/* path B IQK result + 3 */
		if (rtlhal->current_bandtype == BAND_ON_5G)
			val_y += 3;

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "Y = 0x%x\n", val_y);

		rtl_set_bbreg(hw, RTX_IQK_TONE_A, 0x3FF, val_y);
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(26), 0);

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "0xe30 = 0x%x\n",
			rtl_get_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD));

		if (txonly) {
			RTPRINT(rtlpriv, FINIT, INIT_IQK, "only Tx OK\n");
			return;
		}

		reg = result[final_candidate][2];
		rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, 0x3FF, reg);
		reg = result[final_candidate][3] & 0x3F;
		rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, 0xFC00, reg);
		reg = (result[final_candidate][3] >> 6) & 0xF;
		rtl_set_bbreg(hw, ROFDM0_RXIQEXTANTA, 0xF0000000, reg);
	} else {
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"%s: Tx/Rx fail restore default value\n", __func__);

		rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x19008c00);
		rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, MASKDWORD, 0x40000100);
		rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x19008c00);
	}
}


/* rtl8192du/phy.c: _rtl92du_phy_patha_iqk */
static u8 _rtl92du_phy_patha_iqk(struct ieee80211_hw *hw, bool configpathb)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u32 regeac, rege94, rege9c, regea4;
	u8 result = 0;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path-A IQK setting!\n");

	if (rtlhal->interfaceindex == 0) {
		rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x10008c1f);
		rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x10008c1f);
	} else {
		rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x10008c22);
		rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x10008c22);
	}
	rtl_set_bbreg(hw, RTX_IQK_PI_A, MASKDWORD, 0x82140102);
	rtl_set_bbreg(hw, RRX_IQK_PI_A, MASKDWORD,
		      configpathb ? 0x28160202 : 0x28160502);
	/* path-B IQK setting */
	if (configpathb) {
		rtl_set_bbreg(hw, RTX_IQK_TONE_B, MASKDWORD, 0x10008c22);
		rtl_set_bbreg(hw, RRX_IQK_TONE_B, MASKDWORD, 0x10008c22);
		rtl_set_bbreg(hw, RTX_IQK_PI_B, MASKDWORD, 0x82140102);
		rtl_set_bbreg(hw, RRX_IQK_PI_B, MASKDWORD, 0x28160206);
	}

	/* LO calibration setting */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "LO calibration setting!\n");
	rtl_set_bbreg(hw, RIQK_AGC_RSP, MASKDWORD, 0x00462911);

	/* One shot, path A LOK & IQK */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "One shot, path A LOK & IQK!\n");
	rtl_set_bbreg(hw, RIQK_AGC_PTS, MASKDWORD, 0xf9000000);
	rtl_set_bbreg(hw, RIQK_AGC_PTS, MASKDWORD, 0xf8000000);

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Delay %d ms for One shot, path A LOK & IQK\n",
		IQK_DELAY_TIME);
	mdelay(IQK_DELAY_TIME);

	/* Check failed */
	regeac = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xeac = 0x%x\n", regeac);
	rege94 = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_A, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xe94 = 0x%x\n", rege94);
	rege9c = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_A, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xe9c = 0x%x\n", rege9c);
	regea4 = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_A_2, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xea4 = 0x%x\n", regea4);

	if (!(regeac & BIT(28)) &&
	    (((rege94 & 0x03FF0000) >> 16) != 0x142) &&
	    (((rege9c & 0x03FF0000) >> 16) != 0x42))
		result |= 0x01;
	else /* if Tx not OK, ignore Rx */
		return result;

	/* if Tx is OK, check whether Rx is OK */
	if (!(regeac & BIT(27)) &&
	    (((regea4 & 0x03FF0000) >> 16) != 0x132) &&
	    (((regeac & 0x03FF0000) >> 16) != 0x36))
		result |= 0x02;
	else
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path A Rx IQK fail!!\n");

	return result;
}


/* rtl8192du/phy.c: _rtl92du_phy_patha_iqk_5g_normal */
static u8 _rtl92du_phy_patha_iqk_5g_normal(struct ieee80211_hw *hw,
					   bool configpathb)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 TXOKBIT = BIT(28), RXOKBIT = BIT(27);
	u32 regeac, rege94, rege9c, regea4;
	u8 timeout = 20, timecount = 0;
	u8 retrycount = 2;
	u8 result = 0;
	u8 i;

	if (rtlhal->interfaceindex == 1) { /* PHY1 */
		TXOKBIT = BIT(31);
		RXOKBIT = BIT(30);
	}

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path-A IQK setting!\n");
	rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x18008c1f);
	rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x18008c1f);
	rtl_set_bbreg(hw, RTX_IQK_PI_A, MASKDWORD, 0x82140307);
	rtl_set_bbreg(hw, RRX_IQK_PI_A, MASKDWORD, 0x68160960);
	/* path-B IQK setting */
	if (configpathb) {
		rtl_set_bbreg(hw, RTX_IQK_TONE_B, MASKDWORD, 0x18008c2f);
		rtl_set_bbreg(hw, RRX_IQK_TONE_B, MASKDWORD, 0x18008c2f);
		rtl_set_bbreg(hw, RTX_IQK_PI_B, MASKDWORD, 0x82110000);
		rtl_set_bbreg(hw, RRX_IQK_PI_B, MASKDWORD, 0x68110000);
	}

	/* LO calibration setting */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "LO calibration setting!\n");
	rtl_set_bbreg(hw, RIQK_AGC_RSP, MASKDWORD, 0x00462911);

	/* path-A PA on */
	rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW, MASKDWORD, 0x07000f60);
	rtl_set_bbreg(hw, RFPGA0_XA_RFINTERFACEOE, MASKDWORD, 0x66e60e30);

	for (i = 0; i < retrycount; i++) {
		/* One shot, path A LOK & IQK */
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"One shot, path A LOK & IQK!\n");
		rtl_set_bbreg(hw, RIQK_AGC_PTS, MASKDWORD, 0xf9000000);
		rtl_set_bbreg(hw, RIQK_AGC_PTS, MASKDWORD, 0xf8000000);

		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"Delay %d ms for One shot, path A LOK & IQK.\n",
			IQK_DELAY_TIME);
		mdelay(IQK_DELAY_TIME * 10);

		while (timecount < timeout &&
		       rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2, BIT(26)) == 0) {
			udelay(IQK_DELAY_TIME * 1000 * 2);
			timecount++;
		}
		if (timecount >= timeout) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);

		timecount = 0;
		while (timecount < timeout &&
		       rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_A_2, MASK_IQK_RESULT) == 0) {
			udelay(IQK_DELAY_TIME * 1000 * 2);
			timecount++;
		}
		if (timecount >= timeout) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);

		/* Check failed */
		regeac = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xeac = 0x%x\n", regeac);
		rege94 = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_A, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xe94 = 0x%x\n", rege94);
		rege9c = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_A, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xe9c = 0x%x\n", rege9c);
		regea4 = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_A_2, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xea4 = 0x%x\n", regea4);

		if (!(regeac & TXOKBIT) &&
		    (((rege94 & 0x03FF0000) >> 16) != 0x142)) {
			result |= 0x01;
		} else { /* if Tx not OK, ignore Rx */
			RTPRINT(rtlpriv, FINIT, INIT_IQK,
				"Path A Tx IQK fail!!\n");
			continue;
		}

		/* if Tx is OK, check whether Rx is OK */
		if (!(regeac & RXOKBIT) &&
		    (((regea4 & 0x03FF0000) >> 16) != 0x132)) {
			result |= 0x02;
			break;
		}
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "Path A Rx IQK fail!!\n");
	}

	/* path A PA off */
	rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW, MASKDWORD,
		      rtlphy->iqk_bb_backup[0]);
	rtl_set_bbreg(hw, RFPGA0_XA_RFINTERFACEOE, MASKDWORD,
		      rtlphy->iqk_bb_backup[1]);

	if (!(result & 0x01)) /* Tx IQK fail */
		rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x19008c00);

	if (!(result & 0x02)) { /* Rx IQK fail */
		rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, MASKDWORD, 0x40000100);
		rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x19008c00);

		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"Path A Rx IQK fail!! 0xe34 = %#x\n",
			rtl_get_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD));
	}

	return result;
}


/* rtl8192du/phy.c: _rtl92du_phy_patha_standby */
static void _rtl92du_phy_patha_standby(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	RTPRINT(rtlpriv, FINIT, INIT_IQK, "Path-A standby mode!\n");

	rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x0);
	rtl_set_bbreg(hw, RFPGA0_XA_LSSIPARAMETER, MASKDWORD, 0x00010000);
	rtl_set_bbreg(hw, RFPGA0_IQK, MASKH3BYTES, 0x808000);
}


/* rtl8192du/phy.c: _rtl92du_phy_pathb_fill_iqk_matrix */
static void _rtl92du_phy_pathb_fill_iqk_matrix(struct ieee80211_hw *hw,
					       bool iqk_ok, long result[][8],
					       u8 final_candidate, bool txonly)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u32 oldval_1, val_x, tx1_a, reg;
	long val_y, tx1_c;

	if (rtlhal->current_bandtype == BAND_ON_5G) {
		_rtl92du_phy_pathb_fill_iqk_matrix_5g_normal(hw, iqk_ok, result,
							     final_candidate,
							     txonly);
		return;
	}

	RTPRINT(rtlpriv, FINIT, INIT_IQK, "Path B IQ Calibration %s !\n",
		iqk_ok ? "Success" : "Failed");

	if (final_candidate == 0xFF || !iqk_ok)
		return;

	oldval_1 = rtl_get_bbreg(hw, ROFDM0_XBTXIQIMBALANCE, 0xffc00000);

	val_x = result[final_candidate][4];
	if ((val_x & 0x00000200) != 0)
		val_x = val_x | 0xFFFFFC00;

	tx1_a = (val_x * oldval_1) >> 8;
	RTPRINT(rtlpriv, FINIT, INIT_IQK, "X = 0x%x, tx1_a = 0x%x\n",
		val_x, tx1_a);
	rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE, 0x3FF, tx1_a);
	rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(28),
		      (val_x * oldval_1 >> 7) & 0x1);

	val_y = result[final_candidate][5];
	if ((val_y & 0x00000200) != 0)
		val_y = val_y | 0xFFFFFC00;

	if (rtlhal->current_bandtype == BAND_ON_5G)
		val_y += 3;

	tx1_c = (val_y * oldval_1) >> 8;
	RTPRINT(rtlpriv, FINIT, INIT_IQK, "Y = 0x%lx, tx1_c = 0x%lx\n",
		val_y, tx1_c);

	rtl_set_bbreg(hw, ROFDM0_XDTXAFE, 0xF0000000, (tx1_c & 0x3C0) >> 6);
	rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE, 0x003F0000, tx1_c & 0x3F);
	rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(30),
		      (val_y * oldval_1 >> 7) & 0x1);

	if (txonly)
		return;

	reg = result[final_candidate][6];
	rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, 0x3FF, reg);
	reg = result[final_candidate][7] & 0x3F;
	rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, 0xFC00, reg);
	reg = (result[final_candidate][7] >> 6) & 0xF;
	rtl_set_bbreg(hw, ROFDM0_AGCRSSITABLE, 0x0000F000, reg);
}


/* rtl8192du/phy.c: _rtl92du_phy_pathb_fill_iqk_matrix_5g_normal */
static void _rtl92du_phy_pathb_fill_iqk_matrix_5g_normal(struct ieee80211_hw *hw,
							 bool iqk_ok,
							 long result[][8],
							 u8 final_candidate,
							 bool txonly)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u32 val_x, reg;
	int val_y;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Path B IQ Calibration %s !\n", iqk_ok ? "Success" : "Failed");
	if (iqk_ok && final_candidate != 0xFF) {
		val_x = result[final_candidate][4];
		if ((val_x & 0x00000200) != 0)
			val_x = val_x | 0xFFFFFC00;

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "X = 0x%x\n", val_x);
		rtl_set_bbreg(hw, RTX_IQK_TONE_B, 0x3FF0000, val_x);
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(28), 0);

		val_y = result[final_candidate][5];
		if ((val_y & 0x00000200) != 0)
			val_y = val_y | 0xFFFFFC00;

		/* path B IQK result + 3 */
		if (rtlhal->current_bandtype == BAND_ON_5G)
			val_y += 3;

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "Y = 0x%x\n", val_y);

		rtl_set_bbreg(hw, RTX_IQK_TONE_B, 0x3FF, val_y);
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(30), 0);

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "0xe50 = 0x%x\n",
			rtl_get_bbreg(hw, RTX_IQK_TONE_B, MASKDWORD));

		if (txonly) {
			RTPRINT(rtlpriv, FINIT, INIT_IQK, "only Tx OK\n");
			return;
		}

		reg = result[final_candidate][6];
		rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, 0x3FF, reg);
		reg = result[final_candidate][7] & 0x3F;
		rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, 0xFC00, reg);
		reg = (result[final_candidate][7] >> 6) & 0xF;
		rtl_set_bbreg(hw, ROFDM0_AGCRSSITABLE, 0x0000F000, reg);
	} else {
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"%s: Tx/Rx fail restore default value\n", __func__);

		rtl_set_bbreg(hw, RTX_IQK_TONE_B, MASKDWORD, 0x19008c00);
		rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, MASKDWORD, 0x40000100);
		rtl_set_bbreg(hw, RRX_IQK_TONE_B, MASKDWORD, 0x19008c00);
	}
}


/* rtl8192du/phy.c: _rtl92du_phy_pathb_iqk */
static u8 _rtl92du_phy_pathb_iqk(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 regeac, regeb4, regebc, regec4, regecc;
	u8 result = 0;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "One shot, path B LOK & IQK!\n");
	rtl_set_bbreg(hw, RIQK_AGC_CONT, MASKDWORD, 0x00000002);
	rtl_set_bbreg(hw, RIQK_AGC_CONT, MASKDWORD, 0x00000000);

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Delay %d ms for One shot, path B LOK & IQK\n", IQK_DELAY_TIME);
	mdelay(IQK_DELAY_TIME);

	/* Check failed */
	regeac = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xeac = 0x%x\n", regeac);
	regeb4 = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_B, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xeb4 = 0x%x\n", regeb4);
	regebc = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_B, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xebc = 0x%x\n", regebc);
	regec4 = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_B_2, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xec4 = 0x%x\n", regec4);
	regecc = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_B_2, MASKDWORD);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xecc = 0x%x\n", regecc);

	if (!(regeac & BIT(31)) &&
	    (((regeb4 & 0x03FF0000) >> 16) != 0x142) &&
	    (((regebc & 0x03FF0000) >> 16) != 0x42))
		result |= 0x01;
	else
		return result;

	if (!(regeac & BIT(30)) &&
	    (((regec4 & 0x03FF0000) >> 16) != 0x132) &&
	    (((regecc & 0x03FF0000) >> 16) != 0x36))
		result |= 0x02;
	else
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path B Rx IQK fail!!\n");

	return result;
}


/* rtl8192du/phy.c: _rtl92du_phy_pathb_iqk_5g_normal */
static u8 _rtl92du_phy_pathb_iqk_5g_normal(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 regeac, regeb4, regebc, regec4, regecc;
	u8 timeout = 20, timecount = 0;
	u8 retrycount = 2;
	u8 result = 0;
	u8 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Path-B IQK setting!\n");
	rtl_set_bbreg(hw, RTX_IQK_TONE_A, MASKDWORD, 0x18008c1f);
	rtl_set_bbreg(hw, RRX_IQK_TONE_A, MASKDWORD, 0x18008c1f);
	rtl_set_bbreg(hw, RTX_IQK_PI_A, MASKDWORD, 0x82110000);
	rtl_set_bbreg(hw, RRX_IQK_PI_A, MASKDWORD, 0x68110000);

	/* path-B IQK setting */
	rtl_set_bbreg(hw, RTX_IQK_TONE_B, MASKDWORD, 0x18008c2f);
	rtl_set_bbreg(hw, RRX_IQK_TONE_B, MASKDWORD, 0x18008c2f);
	rtl_set_bbreg(hw, RTX_IQK_PI_B, MASKDWORD, 0x82140307);
	rtl_set_bbreg(hw, RRX_IQK_PI_B, MASKDWORD, 0x68160960);

	/* LO calibration setting */
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "LO calibration setting!\n");
	rtl_set_bbreg(hw, RIQK_AGC_RSP, MASKDWORD, 0x00462911);

	/* path-B PA on */
	rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW, MASKDWORD, 0x0f600700);
	rtl_set_bbreg(hw, RFPGA0_XB_RFINTERFACEOE, MASKDWORD, 0x061f0d30);

	for (i = 0; i < retrycount; i++) {
		/* One shot, path B LOK & IQK */
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"One shot, path A LOK & IQK!\n");
		rtl_set_bbreg(hw, RIQK_AGC_PTS, MASKDWORD, 0xfa000000);
		rtl_set_bbreg(hw, RIQK_AGC_PTS, MASKDWORD, 0xf8000000);

		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"Delay %d ms for One shot, path B LOK & IQK.\n", 10);
		mdelay(IQK_DELAY_TIME * 10);

		while (timecount < timeout &&
		       rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2, BIT(29)) == 0) {
			udelay(IQK_DELAY_TIME * 1000 * 2);
			timecount++;
		}
		if (timecount >= timeout) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);

		timecount = 0;
		while (timecount < timeout &&
		       rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_B_2, MASK_IQK_RESULT) == 0) {
			udelay(IQK_DELAY_TIME * 1000 * 2);
			timecount++;
		}
		if (timecount >= timeout) du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);

		/* Check failed */
		regeac = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_A_2, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xeac = 0x%x\n", regeac);
		regeb4 = rtl_get_bbreg(hw, RTX_POWER_BEFORE_IQK_B, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xeb4 = 0x%x\n", regeb4);
		regebc = rtl_get_bbreg(hw, RTX_POWER_AFTER_IQK_B, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xebc = 0x%x\n", regebc);
		regec4 = rtl_get_bbreg(hw, RRX_POWER_BEFORE_IQK_B_2, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xec4 = 0x%x\n", regec4);
		regecc = rtl_get_bbreg(hw, RRX_POWER_AFTER_IQK_B_2, MASKDWORD);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,  "0xecc = 0x%x\n", regecc);

		if (!(regeac & BIT(31)) &&
		    (((regeb4 & 0x03FF0000) >> 16) != 0x142))
			result |= 0x01;
		else
			continue;

		if (!(regeac & BIT(30)) &&
		    (((regec4 & 0x03FF0000) >> 16) != 0x132)) {
			result |= 0x02;
			break;
		}

		RTPRINT(rtlpriv, FINIT, INIT_IQK, "Path B Rx IQK fail!!\n");
	}

	/* path B PA off */
	rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW, MASKDWORD,
		      rtlphy->iqk_bb_backup[0]);
	rtl_set_bbreg(hw, RFPGA0_XB_RFINTERFACEOE, MASKDWORD,
		      rtlphy->iqk_bb_backup[2]);

	if (!(result & 0x01))
		rtl_set_bbreg(hw, RTX_IQK_TONE_B, MASKDWORD, 0x19008c00);

	if (!(result & 0x02)) {
		rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, MASKDWORD, 0x40000100);
		rtl_set_bbreg(hw, RRX_IQK_TONE_B, MASKDWORD, 0x19008c00);

		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"Path B Rx IQK fail!! 0xe54 = %#x\n",
			rtl_get_bbreg(hw, RRX_IQK_TONE_B, MASKDWORD));
	}

	return result;
}


/* rtl8192du/phy.c: _rtl92du_phy_pimode_switch */
static void _rtl92du_phy_pimode_switch(struct ieee80211_hw *hw, bool pi_mode)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 mode;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"BB Switch to %s mode!\n", pi_mode ? "PI" : "SI");
	mode = pi_mode ? 0x01000100 : 0x01000000;
	rtl_set_bbreg(hw, RFPGA0_XA_HSSIPARAMETER1, MASKDWORD, mode);
	rtl_set_bbreg(hw, RFPGA0_XB_HSSIPARAMETER1, MASKDWORD, mode);
}


/* rtl8192du/phy.c: _rtl92du_phy_reload_adda_registers */
static void _rtl92du_phy_reload_adda_registers(struct ieee80211_hw *hw,
					       const u32 *adda_reg,
					       u32 *adda_backup, u32 regnum)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"Reload ADDA power saving parameters !\n");
	for (i = 0; i < regnum; i++) {
		/* path-A/B BB to initial gain */
		if (adda_reg[i] == ROFDM0_XAAGCCORE1 ||
		    adda_reg[i] == ROFDM0_XBAGCCORE1)
			rtl_set_bbreg(hw, adda_reg[i], MASKDWORD, 0x50);

		rtl_set_bbreg(hw, adda_reg[i], MASKDWORD, adda_backup[i]);
	}
}


/* rtl8192du/phy.c: _rtl92du_phy_reload_imr_setting */
static void _rtl92du_phy_reload_imr_setting(struct ieee80211_hw *hw,
					    u8 channel, u8 rfpath)
{
	struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 group, i;

	if (rtlusb->udev->speed != USB_SPEED_HIGH)
		return;

	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "====>path %d\n", rfpath);
	if (rtlpriv->rtlhal.current_bandtype == BAND_ON_5G) {
		rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "====>5G\n");
		rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD,
					     BOFDMEN | BCCKEN, 0);
		rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4, 0x00f00000, 0xf);

		/* fc area 0xd2c */
		if (channel >= 149)
			rtl_set_bbreg(hw, ROFDM1_CFOTRACKING, BIT(13) |
				      BIT(14), 2);
		else
			rtl_set_bbreg(hw, ROFDM1_CFOTRACKING, BIT(13) |
				      BIT(14), 1);

		/* leave 0 for channel1-14. */
		group = channel <= 64 ? 1 : 2;
		for (i = 0; i < MAX_RF_IMR_INDEX_NORMAL; i++)
			rtl_set_rfreg(hw, (enum radio_path)rfpath,
				      rf_reg_for_5g_swchnl_normal[i],
				      RFREG_OFFSET_MASK,
				      rf_imr_param_normal[group][i]);

		rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4, 0x00f00000, 0);
		rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD,
					     BOFDMEN | BCCKEN, 3);
	} else {
		/* G band. */
		rtl_dbg(rtlpriv, COMP_SCAN, DBG_LOUD,
			"Load RF IMR parameters for G band. IMR already setting %d\n",
			rtlpriv->rtlhal.load_imrandiqk_setting_for2g);
		rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "====>2.4G\n");

		if (!rtlpriv->rtlhal.load_imrandiqk_setting_for2g) {
			rtl_dbg(rtlpriv, COMP_SCAN, DBG_LOUD,
				"Load RF IMR parameters for G band. %d\n",
				rfpath);
			rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD,
						     BOFDMEN | BCCKEN, 0);
			rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4,
				      0x00f00000, 0xf);

			for (i = 0; i < MAX_RF_IMR_INDEX_NORMAL; i++) {
				rtl_set_rfreg(hw, (enum radio_path)rfpath,
					      rf_reg_for_5g_swchnl_normal[i],
					      RFREG_OFFSET_MASK,
					      rf_imr_param_normal[0][i]);
			}

			rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER4,
				      0x00f00000, 0);
			rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD,
						     BOFDMEN | BCCKEN, 3);
		}
	}
	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "<====\n");
}


/* rtl8192du/phy.c: _rtl92du_phy_reload_lck_setting */
static void _rtl92du_phy_reload_lck_setting(struct ieee80211_hw *hw, u8 channel)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u8 erfpath = rtlhal->current_bandtype == BAND_ON_5G ? RF90_PATH_A :
		IS_92D_SINGLEPHY(rtlhal->version) ? RF90_PATH_B : RF90_PATH_A;
	bool bneed_powerdown_radio = false;
	u32 u4tmp, u4regvalue;

	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "path %d\n", erfpath);
	RTPRINT(rtlpriv, FINIT, INIT_IQK, "band type = %d\n",
		rtlpriv->rtlhal.current_bandtype);
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "channel = %d\n", channel);

	if (rtlpriv->rtlhal.current_bandtype == BAND_ON_5G) {/* Path-A for 5G */
		u4tmp = rtlpriv->curveindex_5g[channel - 1];
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"ver 1 set RF-A, 5G,	0x28 = 0x%x !!\n", u4tmp);

		if (rtlpriv->rtlhal.macphymode == DUALMAC_DUALPHY &&
		    rtlpriv->rtlhal.interfaceindex == 1) {
			bneed_powerdown_radio =
				rtl92du_phy_enable_anotherphy(hw, false);
			rtlpriv->rtlhal.during_mac1init_radioa = true;
			/* asume no this case */
			if (bneed_powerdown_radio)
				rtl92d_phy_enable_rf_env(hw, erfpath,
							 &u4regvalue);
		}

		rtl_set_rfreg(hw, erfpath, RF_SYN_G4, 0x3f800, u4tmp);

		if (bneed_powerdown_radio) {
			rtl92d_phy_restore_rf_env(hw, erfpath, &u4regvalue);
			rtl92du_phy_powerdown_anotherphy(hw, false);
		}
	} else if (rtlpriv->rtlhal.current_bandtype == BAND_ON_2_4G) {
		u4tmp = rtlpriv->curveindex_2g[channel - 1];
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"ver 3 set RF-B, 2G, 0x28 = 0x%x !!\n", u4tmp);

		if (rtlpriv->rtlhal.macphymode == DUALMAC_DUALPHY &&
		    rtlpriv->rtlhal.interfaceindex == 0) {
			bneed_powerdown_radio =
				rtl92du_phy_enable_anotherphy(hw, true);
			rtlpriv->rtlhal.during_mac0init_radiob = true;
			if (bneed_powerdown_radio)
				rtl92d_phy_enable_rf_env(hw, erfpath,
							 &u4regvalue);
		}

		rtl_set_rfreg(hw, erfpath, RF_SYN_G4, 0x3f800, u4tmp);

		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"ver 3 set RF-B, 2G, 0x28 = 0x%x !!\n",
			rtl_get_rfreg(hw,  erfpath, RF_SYN_G4, 0x3f800));

		if (bneed_powerdown_radio) {
			rtl92d_phy_restore_rf_env(hw, erfpath, &u4regvalue);
			rtl92du_phy_powerdown_anotherphy(hw, true);
		}
	}
	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "<====\n");
}


/* rtl8192du/phy.c: _rtl92du_phy_reload_mac_registers */
static void _rtl92du_phy_reload_mac_registers(struct ieee80211_hw *hw,
					      const u32 *macreg, u32 *macbackup)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK, "Reload MAC parameters !\n");
	for (i = 0; i < (IQK_MAC_REG_NUM - 1); i++)
		rtl_write_byte(rtlpriv, macreg[i], (u8)macbackup[i]);
	rtl_write_dword(rtlpriv, macreg[i], macbackup[i]);
}


/* rtl8192du/phy.c: _rtl92du_phy_set_rfsleep */
static void _rtl92du_phy_set_rfsleep(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 u4btmp;
	u8 retry = 5;

	/* a.   TXPAUSE 0x522[7:0] = 0xFF  Pause MAC TX queue  */
	rtl_write_byte(rtlpriv, REG_TXPAUSE, 0xFF);

	/* b.   RF path 0 offset 0x00 = 0x00  disable RF  */
	rtl_set_rfreg(hw, RF90_PATH_A, 0x00, RFREG_OFFSET_MASK, 0x00);

	/* c.   APSD_CTRL 0x600[7:0] = 0x40 */
	rtl_write_byte(rtlpriv, REG_APSD_CTRL, 0x40);

	/* d. APSD_CTRL 0x600[7:0] = 0x00
	 * APSD_CTRL 0x600[7:0] = 0x00
	 * RF path 0 offset 0x00 = 0x00
	 * APSD_CTRL 0x600[7:0] = 0x40
	 */
	u4btmp = rtl_get_rfreg(hw, RF90_PATH_A, 0, RFREG_OFFSET_MASK);
	while (u4btmp != 0 && retry > 0) {
		rtl_write_byte(rtlpriv, REG_APSD_CTRL, 0x0);
		rtl_set_rfreg(hw, RF90_PATH_A, 0x00, RFREG_OFFSET_MASK, 0x00);
		rtl_write_byte(rtlpriv, REG_APSD_CTRL, 0x40);
		u4btmp = rtl_get_rfreg(hw, RF90_PATH_A, 0, RFREG_OFFSET_MASK);
		retry--;
	}
	if (u4btmp != 0) {
		du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);
		return;
		/* Jump out the LPS turn off sequence to RF_ON_EXCEP */
		rtl_write_byte(rtlpriv, REG_APSD_CTRL, 0x00);

		rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN, 0xE2);
		rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN, 0xE3);
		rtl_write_byte(rtlpriv, REG_TXPAUSE, 0x00);
		rtl_dbg(rtlpriv, COMP_POWER, DBG_LOUD,
			"Fail !!! Switch RF timeout\n");
		return;
	}

	/* e.   For PCIE: SYS_FUNC_EN 0x02[7:0] = 0xE2 reset BB TRX function */
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN, 0xE2);

	/* f.   SPS_CTRL 0x11[7:0] = 0x22 */
	if (rtlpriv->rtlhal.macphymode == SINGLEMAC_SINGLEPHY)
		rtl_write_byte(rtlpriv, REG_SPS0_CTRL, 0x22);
}


/* rtl8192du/phy.c: _rtl92du_phy_simularity_compare */
static bool _rtl92du_phy_simularity_compare(struct ieee80211_hw *hw,
					    long result[][8], u8 c1, u8 c2)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u32 i, j, diff, sim_bitmap, bound, u4temp = 0;
	u8 final_candidate[2] = {0xFF, 0xFF};	/* for path A and path B */
	bool is2t = IS_92D_SINGLEPHY(rtlhal->version);
	bool bresult = true;

	if (is2t)
		bound = 8;
	else
		bound = 4;

	sim_bitmap = 0;

	for (i = 0; i < bound; i++) {
		diff = abs_diff(result[c1][i], result[c2][i]);

		if (diff > MAX_TOLERANCE_92D) {
			if ((i == 2 || i == 6) && !sim_bitmap) {
				if (result[c1][i] + result[c1][i + 1] == 0)
					final_candidate[(i / 4)] = c2;
				else if (result[c2][i] + result[c2][i + 1] == 0)
					final_candidate[(i / 4)] = c1;
				else
					sim_bitmap = sim_bitmap | (1 << i);
			} else {
				sim_bitmap = sim_bitmap | (1 << i);
			}
		}
	}

	if (sim_bitmap == 0) {
		for (i = 0; i < (bound / 4); i++) {
			if (final_candidate[i] != 0xFF) {
				for (j = i * 4; j < (i + 1) * 4 - 2; j++)
					result[3][j] =
						 result[final_candidate[i]][j];
				bresult = false;
			}
		}

		for (i = 0; i < bound; i++)
			u4temp += result[c1][i] + result[c2][i];

		if (u4temp == 0) /* IQK fail for c1 & c2 */
			bresult = false;

		return bresult;
	}

	if (!(sim_bitmap & 0x0F)) { /* path A OK */
		for (i = 0; i < 4; i++)
			result[3][i] = result[c1][i];
	} else if (!(sim_bitmap & 0x03)) { /* path A, Tx OK */
		for (i = 0; i < 2; i++)
			result[3][i] = result[c1][i];
	}

	if (!(sim_bitmap & 0xF0) && is2t) { /* path B OK */
		for (i = 4; i < 8; i++)
			result[3][i] = result[c1][i];
	} else if (!(sim_bitmap & 0x30)) { /* path B, Tx OK */
		for (i = 4; i < 6; i++)
			result[3][i] = result[c1][i];
	}

	return false;
}


/* rtl8192du/phy.c: _rtl92du_phy_stop_trx_before_changeband */
static void _rtl92du_phy_stop_trx_before_changeband(struct ieee80211_hw *hw)
{
	rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD, BCCKEN | BOFDMEN, 0);
	rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE, MASKBYTE0, 0x00);
	rtl_set_bbreg(hw, ROFDM1_TRXPATHENABLE, BDWORD, 0x0);
}


/* rtl8192du/phy.c: _rtl92du_phy_switch_rf_setting */
static void _rtl92du_phy_switch_rf_setting(struct ieee80211_hw *hw, u8 channel)
{
	struct rtl_usb *rtlusb = rtl_usbdev(rtl_usbpriv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u8 path = rtlhal->current_bandtype == BAND_ON_5G ? RF90_PATH_A
							 : RF90_PATH_B;
	u32 u4regvalue, mask = 0x1C000, value = 0, u4tmp, u4tmp2;
	bool need_pwr_down = false, internal_pa = false;
	u32 regb30 = rtl_get_bbreg(hw, 0xb30, BIT(27));
	u8 index = 0, i, rfpath;

	if (rtlusb->udev->speed != USB_SPEED_HIGH)
		return;

	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "====>\n");
	/* config path A for 5G */
	if (rtlhal->current_bandtype == BAND_ON_5G) {
		rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "====>5G\n");
		u4tmp = rtlpriv->curveindex_5g[channel - 1];
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"ver 1 set RF-A, 5G, 0x28 = 0x%x !!\n", u4tmp);

		for (i = 0; i < RF_CHNL_NUM_5G; i++) {
			if (channel == rf_chnl_5g[i] && channel <= 140)
				index = 0;
		}
		for (i = 0; i < RF_CHNL_NUM_5G_40M; i++) {
			if (channel == rf_chnl_5g_40m[i] && channel <= 140)
				index = 1;
		}
		if (channel == 149 || channel == 155 || channel == 161)
			index = 2;
		else if (channel == 151 || channel == 153 || channel == 163 ||
			 channel == 165)
			index = 3;
		else if (channel == 157 || channel == 159)
			index = 4;

		if (rtlhal->macphymode == DUALMAC_DUALPHY &&
		    rtlhal->interfaceindex == 1) {
			need_pwr_down = rtl92du_phy_enable_anotherphy(hw, false);
			rtlhal->during_mac1init_radioa = true;
			/* asume no this case */
			if (need_pwr_down)
				rtl92d_phy_enable_rf_env(hw, path,
							 &u4regvalue);
		}

		/* DMDP, if band = 5G, Mac0 need to set PHY1 when regB30[27]=1 */
		if (regb30 && rtlhal->interfaceindex == 0) {
			need_pwr_down = rtl92du_phy_enable_anotherphy(hw, true);
			rtlhal->during_mac0init_radiob = true;
			if (need_pwr_down)
				rtl92d_phy_enable_rf_env(hw, path,
							 &u4regvalue);
		}

		for (i = 0; i < RF_REG_NUM_FOR_C_CUT_5G; i++) {
			if (i == 0 && rtlhal->macphymode == DUALMAC_DUALPHY) {
				rtl_set_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_5g[i],
					      RFREG_OFFSET_MASK, 0xE439D);
			} else if (rf_reg_for_c_cut_5g[i] == RF_SYN_G4) {
				u4tmp2 = (rf_reg_pram_c_5g[index][i] &
				     0x7FF) | (u4tmp << 11);
				if (channel == 36)
					u4tmp2 &= ~(BIT(7) | BIT(6));
				rtl_set_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_5g[i],
					      RFREG_OFFSET_MASK, u4tmp2);
			} else {
				rtl_set_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_5g[i],
					      RFREG_OFFSET_MASK,
					      rf_reg_pram_c_5g[index][i]);
			}
			rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
				"offset 0x%x value 0x%x path %d index %d readback 0x%x\n",
				rf_reg_for_c_cut_5g[i],
				rf_reg_pram_c_5g[index][i],
				path, index,
				rtl_get_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_5g[i],
					      RFREG_OFFSET_MASK));
		}
		if (rtlhal->macphymode == DUALMAC_DUALPHY &&
		    rtlhal->interfaceindex == 1) {
			if (need_pwr_down)
				rtl92d_phy_restore_rf_env(hw, path, &u4regvalue);

			rtl92du_phy_powerdown_anotherphy(hw, false);
		}

		if (regb30 && rtlhal->interfaceindex == 0) {
			if (need_pwr_down)
				rtl92d_phy_restore_rf_env(hw, path, &u4regvalue);

			rtl92du_phy_powerdown_anotherphy(hw, true);
		}

		if (channel < 149)
			value = 0x07;
		else if (channel >= 149)
			value = 0x02;
		if (channel >= 36 && channel <= 64)
			index = 0;
		else if (channel >= 100 && channel <= 140)
			index = 1;
		else
			index = 2;

		for (rfpath = RF90_PATH_A; rfpath < rtlphy->num_total_rfpath;
			rfpath++) {
			if (rtlhal->macphymode == DUALMAC_DUALPHY &&
			    rtlhal->interfaceindex == 1) /* MAC 1 5G */
				internal_pa = rtlpriv->efuse.internal_pa_5g[1];
			else
				internal_pa =
					 rtlpriv->efuse.internal_pa_5g[rfpath];

			if (internal_pa) {
				for (i = 0;
				     i < RF_REG_NUM_FOR_C_CUT_5G_INTERNALPA;
				     i++) {
					if (rf_for_c_cut_5g_internal_pa[i] == 0x03 &&
					    channel >= 36 && channel <= 64)
						rtl_set_rfreg(hw, rfpath,
							rf_for_c_cut_5g_internal_pa[i],
							RFREG_OFFSET_MASK,
							0x7bdef);
					else
						rtl_set_rfreg(hw, rfpath,
							rf_for_c_cut_5g_internal_pa[i],
							RFREG_OFFSET_MASK,
							rf_pram_c_5g_int_pa[index][i]);
					rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD,
						"offset 0x%x value 0x%x path %d index %d\n",
						rf_for_c_cut_5g_internal_pa[i],
						rf_pram_c_5g_int_pa[index][i],
						rfpath, index);
				}
			} else {
				rtl_set_rfreg(hw, (enum radio_path)rfpath, RF_TXPA_AG,
					      mask, value);
			}
		}
	} else if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "====>2.4G\n");
		u4tmp = rtlpriv->curveindex_2g[channel - 1];
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"ver 3 set RF-B, 2G, 0x28 = 0x%x !!\n", u4tmp);

		if (channel == 1 || channel == 2 || channel == 4 ||
		    channel == 9 || channel == 10 || channel == 11 ||
		    channel == 12)
			index = 0;
		else if (channel == 3 || channel == 13 || channel == 14)
			index = 1;
		else if (channel >= 5 && channel <= 8)
			index = 2;

		if (rtlhal->macphymode == DUALMAC_DUALPHY) {
			path = RF90_PATH_A;
			if (rtlhal->interfaceindex == 0) {
				need_pwr_down =
					 rtl92du_phy_enable_anotherphy(hw, true);
				rtlhal->during_mac0init_radiob = true;

				if (need_pwr_down)
					rtl92d_phy_enable_rf_env(hw, path,
								 &u4regvalue);
			}

			/* DMDP, if band = 2G, MAC1 need to set PHY0 when regB30[27]=1 */
			if (regb30 && rtlhal->interfaceindex == 1) {
				need_pwr_down =
					 rtl92du_phy_enable_anotherphy(hw, false);
				rtlhal->during_mac1init_radioa = true;

				if (need_pwr_down)
					rtl92d_phy_enable_rf_env(hw, path,
								 &u4regvalue);
			}
		}

		for (i = 0; i < RF_REG_NUM_FOR_C_CUT_2G; i++) {
			if (rf_reg_for_c_cut_2g[i] == RF_SYN_G7)
				rtl_set_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_2g[i],
					      RFREG_OFFSET_MASK,
					      rf_reg_param_for_c_cut_2g[index][i] |
					      BIT(17));
			else
				rtl_set_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_2g[i],
					      RFREG_OFFSET_MASK,
					      rf_reg_param_for_c_cut_2g
					      [index][i]);

			rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
				"offset 0x%x value 0x%x mak 0x%x path %d index %d readback 0x%x\n",
				rf_reg_for_c_cut_2g[i],
				rf_reg_param_for_c_cut_2g[index][i],
				rf_reg_mask_for_c_cut_2g[i], path, index,
				rtl_get_rfreg(hw, (enum radio_path)path,
					      rf_reg_for_c_cut_2g[i],
					      RFREG_OFFSET_MASK));
		}
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"cosa ver 3 set RF-B, 2G, 0x28 = 0x%x !!\n",
			rf_syn_g4_for_c_cut_2g | (u4tmp << 11));

		rtl_set_rfreg(hw, (enum radio_path)path, RF_SYN_G4,
			      RFREG_OFFSET_MASK,
			      rf_syn_g4_for_c_cut_2g | (u4tmp << 11));

		if (rtlhal->macphymode == DUALMAC_DUALPHY &&
		    rtlhal->interfaceindex == 0) {
			if (need_pwr_down)
				rtl92d_phy_restore_rf_env(hw, path, &u4regvalue);

			rtl92du_phy_powerdown_anotherphy(hw, true);
		}

		if (regb30 && rtlhal->interfaceindex == 1) {
			if (need_pwr_down)
				rtl92d_phy_restore_rf_env(hw, path, &u4regvalue);

			rtl92du_phy_powerdown_anotherphy(hw, false);
		}
	}
	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "<====\n");
}


/* rtl8192d/dm_common.c: rtl92d_bandtype_2_4G */
static void rtl92d_bandtype_2_4G(struct ieee80211_hw *hw, long *temp_cckg,
				 s8 *cck_index_old)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	unsigned long flag = 0;
	const u8 *cckswing;
	long temp_cck;
	int i;

	/* Query CCK default setting From 0xa24 */
	rtl92d_acquire_cckandrw_pagea_ctl(hw, &flag);
	temp_cck = rtl_get_bbreg(hw, RCCK0_TXFILTER2,
				 MASKDWORD) & MASKCCK;
	rtl92d_release_cckandrw_pagea_ctl(hw, &flag);

	for (i = 0; i < CCK_TABLE_LENGTH; i++) {
		if (rtlpriv->dm.cck_inch14)
			cckswing = &cckswing_table_ch14[i][2];
		else
			cckswing = &cckswing_table_ch1ch13[i][2];

		if (temp_cck == du_get_le32(cckswing)) {
			*cck_index_old = (u8)i;
			rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
				"Initial reg0x%x = 0x%lx, cck_index = 0x%x, ch14 %d\n",
				RCCK0_TXFILTER2, temp_cck,
				*cck_index_old,
				rtlpriv->dm.cck_inch14);
			break;
		}
	}
	*temp_cckg = temp_cck;
}


/* rtl8192d/dm_common.c: rtl92d_bandtype_5G */
static void rtl92d_bandtype_5G(struct rtl_hal *rtlhal, int *ofdm_index,
			       bool *internal_pa, u8 thermalvalue, u8 delta,
			       u8 rf, struct rtl_efuse *rtlefuse,
			       struct rtl_priv *rtlpriv, struct rtl_phy *rtlphy,
			       const u8 index_mapping[5][INDEX_MAPPING_NUM],
			       const u8 index_mapping_pa[8][INDEX_MAPPING_NUM])
{
	u8 offset = 0;
	u8 index;
	int i;

	for (i = 0; i < rf; i++) {
		if (rtlhal->macphymode == DUALMAC_DUALPHY &&
		    rtlhal->interfaceindex == 1) /* MAC 1 5G */
			*internal_pa = rtlefuse->internal_pa_5g[1];
		else
			*internal_pa = rtlefuse->internal_pa_5g[i];

		if (*internal_pa) {
			if (rtlhal->interfaceindex == 1 || i == rf)
				offset = 4;
			else
				offset = 0;
			if (rtlphy->current_channel >= 100 &&
			    rtlphy->current_channel <= 165)
				offset += 2;
		} else {
			if (rtlhal->interfaceindex == 1 || i == rf)
				offset = 2;
			else
				offset = 0;
		}

		if (thermalvalue > rtlefuse->eeprom_thermalmeter)
			offset++;

		if (*internal_pa) {
			if (delta > INDEX_MAPPING_NUM - 1)
				index = index_mapping_pa[offset]
						    [INDEX_MAPPING_NUM - 1];
			else
				index =
				     index_mapping_pa[offset][delta];
		} else {
			if (delta > INDEX_MAPPING_NUM - 1)
				index =
				   index_mapping[offset][INDEX_MAPPING_NUM - 1];
			else
				index = index_mapping[offset][delta];
		}

		if (thermalvalue > rtlefuse->eeprom_thermalmeter) {
			if (*internal_pa && thermalvalue > 0x12) {
				ofdm_index[i] = rtlpriv->dm.ofdm_index[i] -
						((delta / 2) * 3 + (delta % 2));
			} else {
				ofdm_index[i] -= index;
			}
		} else {
			ofdm_index[i] += index;
		}
	}
}


/* rtl8192d/dm_common.c: rtl92d_dm_check_txpower_tracking_thermal_meter */
static void rtl92d_dm_check_txpower_tracking_thermal_meter(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	if (!rtlpriv->dm.txpower_tracking)
		return;

	if (!rtlpriv->dm.tm_trigger) {
		rtl_set_rfreg(hw, RF90_PATH_A, RF_T_METER, BIT(17) |
			      BIT(16), 0x03);
		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"Trigger 92S Thermal Meter!!\n");
		rtlpriv->dm.tm_trigger = 1;
	} else {
		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"Schedule TxPowerTracking direct call!!\n");
		rtl92d_dm_txpower_tracking_callback_thermalmeter(hw);
		rtlpriv->dm.tm_trigger = 0;
	}
}


/* rtl8192d/dm_common.c: rtl92d_dm_initialize_txpower_tracking */
static void rtl92d_dm_initialize_txpower_tracking(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	rtlpriv->dm.txpower_tracking = true;
	rtlpriv->dm.txpower_trackinginit = false;
	rtlpriv->dm.txpower_track_control = true;
	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
		"pMgntInfo->txpower_tracking = %d\n",
		rtlpriv->dm.txpower_tracking);
}


/* rtl8192d/dm_common.c: rtl92d_dm_rxgain_tracking_thermalmeter */
static void rtl92d_dm_rxgain_tracking_thermalmeter(struct ieee80211_hw *hw)
{
	static const u8 index_mapping[RX_INDEX_MAPPING_NUM] = {
		0x0f, 0x0f, 0x0d, 0x0c, 0x0b,
		0x0a, 0x09, 0x08, 0x07, 0x06,
		0x05, 0x04, 0x04, 0x03, 0x02
	};
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	int i, idx;
	u32 u4tmp;

	idx = rtlpriv->efuse.eeprom_thermalmeter - rtlpriv->dm.thermalvalue_rxgain;
	if (idx < 0) idx = 0;
	if (idx >= RX_INDEX_MAPPING_NUM) idx = RX_INDEX_MAPPING_NUM - 1;
	u4tmp = index_mapping[idx] << 12;

	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
		"===> Rx Gain %x\n", u4tmp);

	for (i = RF90_PATH_A; i < rtlpriv->phy.num_total_rfpath; i++)
		rtl_set_rfreg(hw, i, 0x3C, RFREG_OFFSET_MASK,
			      (rtlpriv->phy.reg_rf3c[i] & ~0xF000) | u4tmp);
}


/* rtl8192d/dm_common.c: rtl92d_dm_txpower_tracking_callback_thermalmeter */
static void
rtl92d_dm_txpower_tracking_callback_thermalmeter(struct ieee80211_hw *hw)
{
	static const u8 index_mapping[5][INDEX_MAPPING_NUM] = {
		/* 5G, path A/MAC 0, decrease power  */
		{0, 1, 3, 6, 8, 9, 11, 13, 14, 16, 17, 18, 18},
		/* 5G, path A/MAC 0, increase power  */
		{0, 2, 4, 5, 7, 10, 12, 14, 16, 18, 18, 18, 18},
		/* 5G, path B/MAC 1, decrease power */
		{0, 2, 3, 6, 8, 9, 11, 13, 14, 16, 17, 18, 18},
		/* 5G, path B/MAC 1, increase power */
		{0, 2, 4, 5, 7, 10, 13, 16, 16, 18, 18, 18, 18},
		/* 2.4G, for decreas power */
		{0, 1, 2, 3, 4, 5, 6, 7, 7, 8, 9, 10, 10},
	};
	static const u8 index_mapping_internal_pa[8][INDEX_MAPPING_NUM] = {
		/* 5G, path A/MAC 0, ch36-64, decrease power  */
		{0, 1, 2, 4, 6, 7, 9, 11, 12, 14, 15, 16, 16},
		/* 5G, path A/MAC 0, ch36-64, increase power  */
		{0, 2, 4, 5, 7, 10, 12, 14, 16, 18, 18, 18, 18},
		/* 5G, path A/MAC 0, ch100-165, decrease power  */
		{0, 1, 2, 3, 5, 6, 8, 10, 11, 13, 14, 15, 15},
		/* 5G, path A/MAC 0, ch100-165, increase power  */
		{0, 2, 4, 5, 7, 10, 12, 14, 16, 18, 18, 18, 18},
		/* 5G, path B/MAC 1, ch36-64, decrease power */
		{0, 1, 2, 4, 6, 7, 9, 11, 12, 14, 15, 16, 16},
		/* 5G, path B/MAC 1, ch36-64, increase power */
		{0, 2, 4, 5, 7, 10, 13, 16, 16, 18, 18, 18, 18},
		/* 5G, path B/MAC 1, ch100-165, decrease power */
		{0, 1, 2, 3, 5, 6, 8, 9, 10, 12, 13, 14, 14},
		/* 5G, path B/MAC 1, ch100-165, increase power */
		{0, 2, 4, 5, 7, 10, 13, 16, 16, 18, 18, 18, 18},
	};
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtlpriv);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct rtl_dm *dm = &rtlpriv->dm;
	u8 thermalvalue, delta, delta_lck, delta_iqk, delta_rxgain;
	u8 ofdm_min_index = 6, ofdm_min_index_internal_pa = 3, rf;
	long ele_a = 0, ele_d, temp_cck, val_x, value32;
	bool is2t = IS_92D_SINGLEPHY(rtlhal->version);
	u8 offset, thermalvalue_avg_count = 0;
	u8 ofdm_index_old[2] = {0, 0};
	u32 thermalvalue_avg = 0;
	bool internal_pa = false;
	long val_y, ele_c = 0;
	s8 cck_index_old = 0;
	u8 indexforchannel;
	int ofdm_index[2];
	s8 cck_index = 0;
	u8 index, swing;
	int i;

	indexforchannel = rtl92d_get_rightchnlplace_for_iqk(rtlphy->current_channel);

	dm->txpower_trackinginit = true;

	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD, "\n");

	thermalvalue = (u8)rtl_get_rfreg(hw, RF90_PATH_A, RF_T_METER, 0xf800);

	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
		"Readback Thermal Meter = 0x%x pre thermal meter 0x%x eeprom_thermalmeter 0x%x\n",
		thermalvalue,
		dm->thermalvalue, rtlefuse->eeprom_thermalmeter);

	if (!thermalvalue)
		goto exit;

	if (is2t)
		rf = 2;
	else
		rf = 1;

	if (dm->thermalvalue && !rtlhal->reloadtxpowerindex)
		goto old_index_done;

	ele_d = rtl_get_bbreg(hw, ROFDM0_XATXIQIMBALANCE, MASKDWORD) & MASKOFDM_D;

	for (i = 0; i < OFDM_TABLE_SIZE_92D; i++) {
		if (ele_d == (ofdmswing_table[i] & MASKOFDM_D)) {
			ofdm_index_old[0] = (u8)i;

			rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
				"Initial pathA ele_d reg0x%x = 0x%lx, ofdm_index=0x%x\n",
				ROFDM0_XATXIQIMBALANCE,
				ele_d, ofdm_index_old[0]);
			break;
		}
	}

	if (is2t) {
		ele_d = rtl_get_bbreg(hw, ROFDM0_XBTXIQIMBALANCE, MASKDWORD);
		ele_d &= MASKOFDM_D;

		for (i = 0; i < OFDM_TABLE_SIZE_92D; i++) {
			if (ele_d == (ofdmswing_table[i] & MASKOFDM_D)) {
				ofdm_index_old[1] = (u8)i;

				rtl_dbg(rtlpriv, COMP_POWER_TRACKING,
					DBG_LOUD,
					"Initial pathB ele_d reg 0x%x = 0x%lx, ofdm_index = 0x%x\n",
					ROFDM0_XBTXIQIMBALANCE, ele_d,
					ofdm_index_old[1]);
				break;
			}
		}
	}

	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		rtl92d_bandtype_2_4G(hw, &temp_cck, &cck_index_old);
	} else {
		temp_cck = 0x090e1317;
		cck_index_old = 12;
	}

	if (!dm->thermalvalue) {
		dm->thermalvalue = rtlefuse->eeprom_thermalmeter;
		dm->thermalvalue_lck = thermalvalue;
		dm->thermalvalue_iqk = thermalvalue;
		dm->thermalvalue_rxgain = rtlefuse->eeprom_thermalmeter;

		for (i = 0; i < rf; i++)
			dm->ofdm_index[i] = ofdm_index_old[i];

		dm->cck_index = cck_index_old;
	}

	if (rtlhal->reloadtxpowerindex) {
		for (i = 0; i < rf; i++)
			dm->ofdm_index[i] = ofdm_index_old[i];

		dm->cck_index = cck_index_old;

		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"reload ofdm index for band switch\n");
	}

old_index_done:
	for (i = 0; i < rf; i++)
		ofdm_index[i] = dm->ofdm_index[i];

	dm->thermalvalue_avg[dm->thermalvalue_avg_index] = thermalvalue;
	dm->thermalvalue_avg_index++;

	if (dm->thermalvalue_avg_index == AVG_THERMAL_NUM)
		dm->thermalvalue_avg_index = 0;

	for (i = 0; i < AVG_THERMAL_NUM; i++) {
		if (dm->thermalvalue_avg[i]) {
			thermalvalue_avg += dm->thermalvalue_avg[i];
			thermalvalue_avg_count++;
		}
	}

	if (thermalvalue_avg_count)
		thermalvalue = (u8)(thermalvalue_avg / thermalvalue_avg_count);

	if (rtlhal->reloadtxpowerindex) {
		delta = abs_diff(thermalvalue, rtlefuse->eeprom_thermalmeter);
		rtlhal->reloadtxpowerindex = false;
		dm->done_txpower = false;
	} else if (dm->done_txpower) {
		delta = abs_diff(thermalvalue, dm->thermalvalue);
	} else {
		delta = abs_diff(thermalvalue, rtlefuse->eeprom_thermalmeter);
	}

	delta_lck = abs_diff(thermalvalue, dm->thermalvalue_lck);
	delta_iqk = abs_diff(thermalvalue, dm->thermalvalue_iqk);
	delta_rxgain = abs_diff(thermalvalue, dm->thermalvalue_rxgain);

	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
		"Readback Thermal Meter = 0x%x pre thermal meter 0x%x eeprom_thermalmeter 0x%x delta 0x%x delta_lck 0x%x delta_iqk 0x%x\n",
		thermalvalue, dm->thermalvalue, rtlefuse->eeprom_thermalmeter,
		delta, delta_lck, delta_iqk);

	if (delta_lck > rtlefuse->delta_lck && rtlefuse->delta_lck != 0) {
		dm->thermalvalue_lck = thermalvalue;
		rtlpriv->cfg->ops->phy_lc_calibrate(hw, is2t);
	}

	if (delta == 0 || !dm->txpower_track_control)
		goto check_delta;

	dm->done_txpower = true;
	delta = abs_diff(thermalvalue, rtlefuse->eeprom_thermalmeter);

	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		offset = 4;
		if (delta > INDEX_MAPPING_NUM - 1)
			index = index_mapping[offset][INDEX_MAPPING_NUM - 1];
		else
			index = index_mapping[offset][delta];

		if (thermalvalue > dm->thermalvalue) {
			for (i = 0; i < rf; i++)
				ofdm_index[i] -= delta;

			cck_index -= delta;
		} else {
			for (i = 0; i < rf; i++)
				ofdm_index[i] += index;

			cck_index += index;
		}
	} else if (rtlhal->current_bandtype == BAND_ON_5G) {
		rtl92d_bandtype_5G(rtlhal, ofdm_index, &internal_pa,
				   thermalvalue, delta, rf, rtlefuse, rtlpriv,
				   rtlphy, index_mapping,
				   index_mapping_internal_pa);
	}

	if (is2t) {
		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"temp OFDM_A_index=0x%x, OFDM_B_index = 0x%x, cck_index=0x%x\n",
			dm->ofdm_index[0], dm->ofdm_index[1], dm->cck_index);
	} else {
		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"temp OFDM_A_index=0x%x, cck_index = 0x%x\n",
			dm->ofdm_index[0], dm->cck_index);
	}

	for (i = 0; i < rf; i++) {
		if (ofdm_index[i] > OFDM_TABLE_SIZE_92D - 1) {
			ofdm_index[i] = OFDM_TABLE_SIZE_92D - 1;
		} else if (internal_pa ||
			   rtlhal->current_bandtype == BAND_ON_2_4G) {
			if (ofdm_index[i] < ofdm_min_index_internal_pa)
				ofdm_index[i] = ofdm_min_index_internal_pa;
		} else if (ofdm_index[i] < ofdm_min_index) {
			ofdm_index[i] = ofdm_min_index;
		}
	}

	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		if (cck_index > CCK_TABLE_SIZE - 1)
			cck_index = CCK_TABLE_SIZE - 1;
		else if (cck_index < 0)
			cck_index = 0;
	}

	if (is2t) {
		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"new OFDM_A_index=0x%x, OFDM_B_index = 0x%x, cck_index=0x%x\n",
			ofdm_index[0], ofdm_index[1], cck_index);
	} else {
		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"new OFDM_A_index=0x%x, cck_index = 0x%x\n",
			ofdm_index[0], cck_index);
	}

	ele_d = (ofdmswing_table[ofdm_index[0]] & 0xFFC00000) >> 22;
	val_x = rtlphy->iqk_matrix[indexforchannel].value[0][0];
	val_y = rtlphy->iqk_matrix[indexforchannel].value[0][1];

	if (val_x != 0) {
		if ((val_x & 0x00000200) != 0)
			val_x = val_x | 0xFFFFFC00;
		ele_a = ((val_x * ele_d) >> 8) & 0x000003FF;

		/* new element C = element D x Y */
		if ((val_y & 0x00000200) != 0)
			val_y = val_y | 0xFFFFFC00;
		ele_c = ((val_y * ele_d) >> 8) & 0x000003FF;

		/* write new elements A, C, D to regC80 and
		 * regC94, element B is always 0
		 */
		value32 = (ele_d << 22) | ((ele_c & 0x3F) << 16) | ele_a;
		rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE,
			      MASKDWORD, value32);

		value32 = (ele_c & 0x000003C0) >> 6;
		rtl_set_bbreg(hw, ROFDM0_XCTXAFE, MASKH4BITS, value32);

		value32 = ((val_x * ele_d) >> 7) & 0x01;
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(24), value32);

	} else {
		rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE, MASKDWORD,
			      ofdmswing_table[ofdm_index[0]]);
		rtl_set_bbreg(hw, ROFDM0_XCTXAFE, MASKH4BITS, 0x00);
		rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(24), 0x00);
	}

	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
		"TxPwrTracking for interface %d path A: X = 0x%lx, Y = 0x%lx ele_A = 0x%lx ele_C = 0x%lx ele_D = 0x%lx 0xe94 = 0x%lx 0xe9c = 0x%lx\n",
		rtlhal->interfaceindex,
		val_x, val_y, ele_a, ele_c, ele_d,
		val_x, val_y);

	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		/* Adjust CCK according to IQK result */
		for (i = 0; i < 8; i++) {
			if (dm->cck_inch14)
				swing = cckswing_table_ch14[cck_index][i];
			else
				swing = cckswing_table_ch1ch13[cck_index][i];

			rtl_write_byte(rtlpriv, 0xa22 + i, swing);
		}
	}

	if (is2t) {
		ele_d = (ofdmswing_table[ofdm_index[1]] & 0xFFC00000) >> 22;
		val_x = rtlphy->iqk_matrix[indexforchannel].value[0][4];
		val_y = rtlphy->iqk_matrix[indexforchannel].value[0][5];

		if (val_x != 0) {
			if ((val_x & 0x00000200) != 0)
				/* consider minus */
				val_x = val_x | 0xFFFFFC00;
			ele_a = ((val_x * ele_d) >> 8) & 0x000003FF;

			/* new element C = element D x Y */
			if ((val_y & 0x00000200) != 0)
				val_y = val_y | 0xFFFFFC00;
			ele_c = ((val_y * ele_d) >> 8) & 0x00003FF;

			/* write new elements A, C, D to regC88
			 * and regC9C, element B is always 0
			 */
			value32 = (ele_d << 22) | ((ele_c & 0x3F) << 16) | ele_a;
			rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE,
				      MASKDWORD, value32);

			value32 = (ele_c & 0x000003C0) >> 6;
			rtl_set_bbreg(hw, ROFDM0_XDTXAFE, MASKH4BITS, value32);

			value32 = ((val_x * ele_d) >> 7) & 0x01;
			rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(28), value32);
		} else {
			rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE,
				      MASKDWORD, ofdmswing_table[ofdm_index[1]]);
			rtl_set_bbreg(hw, ROFDM0_XDTXAFE, MASKH4BITS, 0x00);
			rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(28), 0x00);
		}

		rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
			"TxPwrTracking path B: X = 0x%lx, Y = 0x%lx ele_A = 0x%lx ele_C = 0x%lx ele_D = 0x%lx 0xeb4 = 0x%lx 0xebc = 0x%lx\n",
			val_x, val_y, ele_a, ele_c, ele_d, val_x, val_y);
	}

	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD,
		"TxPwrTracking 0xc80 = 0x%x, 0xc94 = 0x%x RF 0x24 = 0x%x\n",
		rtl_get_bbreg(hw, 0xc80, MASKDWORD),
		rtl_get_bbreg(hw, 0xc94, MASKDWORD),
		rtl_get_rfreg(hw, RF90_PATH_A, 0x24, RFREG_OFFSET_MASK));

check_delta:
	if (delta_iqk > rtlefuse->delta_iqk && rtlefuse->delta_iqk != 0) {
		rtl92d_phy_reset_iqk_result(hw);
		dm->thermalvalue_iqk = thermalvalue;
		rtlpriv->cfg->ops->phy_iq_calibrate(hw);
	}

	if (delta_rxgain > 0 && rtlhal->current_bandtype == BAND_ON_5G &&
	    thermalvalue <= rtlefuse->eeprom_thermalmeter) {
		dm->thermalvalue_rxgain = thermalvalue;
		rtl92d_dm_rxgain_tracking_thermalmeter(hw);
	}

	if (dm->txpower_track_control)
		dm->thermalvalue = thermalvalue;

exit:
	rtl_dbg(rtlpriv, COMP_POWER_TRACKING, DBG_LOUD, "<===\n");
}


/* rtl8192d/phy_common.c: rtl92d_get_chnlgroup_fromarray */
static u8 rtl92d_get_chnlgroup_fromarray(u8 chnl)
{
	u8 group;

	if (channel_all[chnl] <= 3)
		group = 0;
	else if (channel_all[chnl] <= 9)
		group = 1;
	else if (channel_all[chnl] <= 14)
		group = 2;
	else if (channel_all[chnl] <= 44)
		group = 3;
	else if (channel_all[chnl] <= 54)
		group = 4;
	else if (channel_all[chnl] <= 64)
		group = 5;
	else if (channel_all[chnl] <= 112)
		group = 6;
	else if (channel_all[chnl] <= 126)
		group = 7;
	else if (channel_all[chnl] <= 140)
		group = 8;
	else if (channel_all[chnl] <= 153)
		group = 9;
	else if (channel_all[chnl] <= 159)
		group = 10;
	else
		group = 11;
	return group;
}


/* rtl8192d/phy_common.c: rtl92d_get_rightchnlplace_for_iqk */
static u8 rtl92d_get_rightchnlplace_for_iqk(u8 chnl)
{
	u8 place;

	if (chnl > 14) {
		for (place = 14; place < ARRAY_SIZE(channel_all); place++) {
			if (channel_all[place] == chnl)
				return place - 13;
		}
	}

	return 0;
}


/* rtl8192d/phy_common.c: rtl92d_phy_calc_curvindex */
static void rtl92d_phy_calc_curvindex(struct ieee80211_hw *hw,
			       const u32 *targetchnl, u32 *curvecount_val,
			       bool is5g, u32 *curveindex)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 smallest_abs_val = 0xffffffff, u4tmp;
	u8 i, j;
	u8 chnl_num = is5g ? TARGET_CHNL_NUM_5G : TARGET_CHNL_NUM_2G;

	for (i = 0; i < chnl_num; i++) {
		if (is5g && !_rtl92d_is_legal_5g_channel(hw, i + 1))
			continue;
		curveindex[i] = 0;
		for (j = 0; j < (CV_CURVE_CNT * 2); j++) {
			u4tmp = _rtl92d_phy_get_abs(targetchnl[i],
						    curvecount_val[j]);

			if (u4tmp < smallest_abs_val) {
				curveindex[i] = j;
				smallest_abs_val = u4tmp;
			}
		}
		smallest_abs_val = 0xffffffff;
		RTPRINT(rtlpriv, FINIT, INIT_IQK, "curveindex[%d] = %x\n",
			i, curveindex[i]);
	}
}


/* rtl8192d/phy_common.c: rtl92d_phy_enable_rf_env */
static void rtl92d_phy_enable_rf_env(struct ieee80211_hw *hw, u8 rfpath,
			      u32 *pu4_regval)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct bb_reg_def *pphyreg = &rtlphy->phyreg_def[rfpath];

	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "====>\n");
	/*----Store original RFENV control type----*/
	switch (rfpath) {
	case RF90_PATH_A:
	case RF90_PATH_C:
		*pu4_regval = rtl_get_bbreg(hw, pphyreg->rfintfs, BRFSI_RFENV);
		break;
	case RF90_PATH_B:
	case RF90_PATH_D:
		*pu4_regval =
		    rtl_get_bbreg(hw, pphyreg->rfintfs, BRFSI_RFENV << 16);
		break;
	}
	/*----Set RF_ENV enable----*/
	rtl_set_bbreg(hw, pphyreg->rfintfe, BRFSI_RFENV << 16, 0x1);
	udelay(1);
	/*----Set RF_ENV output high----*/
	rtl_set_bbreg(hw, pphyreg->rfintfo, BRFSI_RFENV, 0x1);
	udelay(1);
	/* Set bit number of Address and Data for RF register */
	/* Set 1 to 4 bits for 8255 */
	rtl_set_bbreg(hw, pphyreg->rfhssi_para2, B3WIREADDRESSLENGTH, 0x0);
	udelay(1);
	/*Set 0 to 12 bits for 8255 */
	rtl_set_bbreg(hw, pphyreg->rfhssi_para2, B3WIREDATALENGTH, 0x0);
	udelay(1);
	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "<====\n");
}


/* rtl8192d/phy_common.c: rtl92d_phy_get_chnlgroup_bypg */
static u8 rtl92d_phy_get_chnlgroup_bypg(u8 chnlindex)
{
	u8 group;

	if (channel_all[chnlindex] <= 3)	/* Chanel 1-3 */
		group = 0;
	else if (channel_all[chnlindex] <= 9)	/* Channel 4-9 */
		group = 1;
	else if (channel_all[chnlindex] <= 14)	/* Channel 10-14 */
		group = 2;
	else if (channel_all[chnlindex] <= 64)
		group = 6;
	else if (channel_all[chnlindex] <= 140)
		group = 7;
	else
		group = 8;
	return group;
}


/* rtl8192d/phy_common.c: rtl92d_phy_get_hw_reg_originalvalue */
static void rtl92d_phy_get_hw_reg_originalvalue(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;

	rtlphy->default_initialgain[0] =
	    rtl_get_bbreg(hw, ROFDM0_XAAGCCORE1, MASKBYTE0);
	rtlphy->default_initialgain[1] =
	    rtl_get_bbreg(hw, ROFDM0_XBAGCCORE1, MASKBYTE0);
	rtlphy->default_initialgain[2] =
	    rtl_get_bbreg(hw, ROFDM0_XCAGCCORE1, MASKBYTE0);
	rtlphy->default_initialgain[3] =
	    rtl_get_bbreg(hw, ROFDM0_XDAGCCORE1, MASKBYTE0);
	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE,
		"Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x\n",
		rtlphy->default_initialgain[0],
		rtlphy->default_initialgain[1],
		rtlphy->default_initialgain[2],
		rtlphy->default_initialgain[3]);
	rtlphy->framesync = rtl_get_bbreg(hw, ROFDM0_RXDETECTOR3, MASKBYTE0);
	rtlphy->framesync_c34 = rtl_get_bbreg(hw, ROFDM0_RXDETECTOR2, MASKDWORD);
	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE,
		"Default framesync (0x%x) = 0x%x\n",
		ROFDM0_RXDETECTOR3, rtlphy->framesync);
}


/* rtl8192d/phy_common.c: rtl92d_phy_init_bb_rf_register_definition */
static void rtl92d_phy_init_bb_rf_register_definition(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;

	/* RF Interface Sowrtware Control */
	/* 16 LSBs if read 32-bit from 0x870 */
	rtlphy->phyreg_def[RF90_PATH_A].rfintfs = RFPGA0_XAB_RFINTERFACESW;
	/* 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872) */
	rtlphy->phyreg_def[RF90_PATH_B].rfintfs = RFPGA0_XAB_RFINTERFACESW;
	/* 16 LSBs if read 32-bit from 0x874 */
	rtlphy->phyreg_def[RF90_PATH_C].rfintfs = RFPGA0_XCD_RFINTERFACESW;
	/* 16 MSBs if read 32-bit from 0x874 (16-bit for 0x876) */

	rtlphy->phyreg_def[RF90_PATH_D].rfintfs = RFPGA0_XCD_RFINTERFACESW;
	/* RF Interface Readback Value */
	/* 16 LSBs if read 32-bit from 0x8E0 */
	rtlphy->phyreg_def[RF90_PATH_A].rfintfi = RFPGA0_XAB_RFINTERFACERB;
	/* 16 MSBs if read 32-bit from 0x8E0 (16-bit for 0x8E2) */
	rtlphy->phyreg_def[RF90_PATH_B].rfintfi = RFPGA0_XAB_RFINTERFACERB;
	/* 16 LSBs if read 32-bit from 0x8E4 */
	rtlphy->phyreg_def[RF90_PATH_C].rfintfi = RFPGA0_XCD_RFINTERFACERB;
	/* 16 MSBs if read 32-bit from 0x8E4 (16-bit for 0x8E6) */
	rtlphy->phyreg_def[RF90_PATH_D].rfintfi = RFPGA0_XCD_RFINTERFACERB;

	/* RF Interface Output (and Enable) */
	/* 16 LSBs if read 32-bit from 0x860 */
	rtlphy->phyreg_def[RF90_PATH_A].rfintfo = RFPGA0_XA_RFINTERFACEOE;
	/* 16 LSBs if read 32-bit from 0x864 */
	rtlphy->phyreg_def[RF90_PATH_B].rfintfo = RFPGA0_XB_RFINTERFACEOE;

	/* RF Interface (Output and)  Enable */
	/* 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862) */
	rtlphy->phyreg_def[RF90_PATH_A].rfintfe = RFPGA0_XA_RFINTERFACEOE;
	/* 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866) */
	rtlphy->phyreg_def[RF90_PATH_B].rfintfe = RFPGA0_XB_RFINTERFACEOE;

	/* Addr of LSSI. Write RF register by driver */
	/* LSSI Parameter */
	rtlphy->phyreg_def[RF90_PATH_A].rf3wire_offset =
				 RFPGA0_XA_LSSIPARAMETER;
	rtlphy->phyreg_def[RF90_PATH_B].rf3wire_offset =
				 RFPGA0_XB_LSSIPARAMETER;

	/* RF parameter */
	/* BB Band Select */
	rtlphy->phyreg_def[RF90_PATH_A].rflssi_select = RFPGA0_XAB_RFPARAMETER;
	rtlphy->phyreg_def[RF90_PATH_B].rflssi_select = RFPGA0_XAB_RFPARAMETER;
	rtlphy->phyreg_def[RF90_PATH_C].rflssi_select = RFPGA0_XCD_RFPARAMETER;
	rtlphy->phyreg_def[RF90_PATH_D].rflssi_select = RFPGA0_XCD_RFPARAMETER;

	/* Tx AGC Gain Stage (same for all path. Should we remove this?) */
	/* Tx gain stage */
	rtlphy->phyreg_def[RF90_PATH_A].rftxgain_stage = RFPGA0_TXGAINSTAGE;
	/* Tx gain stage */
	rtlphy->phyreg_def[RF90_PATH_B].rftxgain_stage = RFPGA0_TXGAINSTAGE;
	/* Tx gain stage */
	rtlphy->phyreg_def[RF90_PATH_C].rftxgain_stage = RFPGA0_TXGAINSTAGE;
	/* Tx gain stage */
	rtlphy->phyreg_def[RF90_PATH_D].rftxgain_stage = RFPGA0_TXGAINSTAGE;

	/* Transceiver A~D HSSI Parameter-1 */
	/* wire control parameter1 */
	rtlphy->phyreg_def[RF90_PATH_A].rfhssi_para1 = RFPGA0_XA_HSSIPARAMETER1;
	/* wire control parameter1 */
	rtlphy->phyreg_def[RF90_PATH_B].rfhssi_para1 = RFPGA0_XB_HSSIPARAMETER1;

	/* Transceiver A~D HSSI Parameter-2 */
	/* wire control parameter2 */
	rtlphy->phyreg_def[RF90_PATH_A].rfhssi_para2 = RFPGA0_XA_HSSIPARAMETER2;
	/* wire control parameter2 */
	rtlphy->phyreg_def[RF90_PATH_B].rfhssi_para2 = RFPGA0_XB_HSSIPARAMETER2;

	/* RF switch Control */
	/* TR/Ant switch control */
	rtlphy->phyreg_def[RF90_PATH_A].rfsw_ctrl = RFPGA0_XAB_SWITCHCONTROL;
	rtlphy->phyreg_def[RF90_PATH_B].rfsw_ctrl = RFPGA0_XAB_SWITCHCONTROL;
	rtlphy->phyreg_def[RF90_PATH_C].rfsw_ctrl = RFPGA0_XCD_SWITCHCONTROL;
	rtlphy->phyreg_def[RF90_PATH_D].rfsw_ctrl = RFPGA0_XCD_SWITCHCONTROL;

	/* AGC control 1 */
	rtlphy->phyreg_def[RF90_PATH_A].rfagc_control1 = ROFDM0_XAAGCCORE1;
	rtlphy->phyreg_def[RF90_PATH_B].rfagc_control1 = ROFDM0_XBAGCCORE1;
	rtlphy->phyreg_def[RF90_PATH_C].rfagc_control1 = ROFDM0_XCAGCCORE1;
	rtlphy->phyreg_def[RF90_PATH_D].rfagc_control1 = ROFDM0_XDAGCCORE1;

	/* AGC control 2  */
	rtlphy->phyreg_def[RF90_PATH_A].rfagc_control2 = ROFDM0_XAAGCCORE2;
	rtlphy->phyreg_def[RF90_PATH_B].rfagc_control2 = ROFDM0_XBAGCCORE2;
	rtlphy->phyreg_def[RF90_PATH_C].rfagc_control2 = ROFDM0_XCAGCCORE2;
	rtlphy->phyreg_def[RF90_PATH_D].rfagc_control2 = ROFDM0_XDAGCCORE2;

	/* RX AFE control 1 */
	rtlphy->phyreg_def[RF90_PATH_A].rfrxiq_imbal = ROFDM0_XARXIQIMBALANCE;
	rtlphy->phyreg_def[RF90_PATH_B].rfrxiq_imbal = ROFDM0_XBRXIQIMBALANCE;
	rtlphy->phyreg_def[RF90_PATH_C].rfrxiq_imbal = ROFDM0_XCRXIQIMBALANCE;
	rtlphy->phyreg_def[RF90_PATH_D].rfrxiq_imbal = ROFDM0_XDRXIQIMBALANCE;

	/*RX AFE control 1 */
	rtlphy->phyreg_def[RF90_PATH_A].rfrx_afe = ROFDM0_XARXAFE;
	rtlphy->phyreg_def[RF90_PATH_B].rfrx_afe = ROFDM0_XBRXAFE;
	rtlphy->phyreg_def[RF90_PATH_C].rfrx_afe = ROFDM0_XCRXAFE;
	rtlphy->phyreg_def[RF90_PATH_D].rfrx_afe = ROFDM0_XDRXAFE;

	/* Tx AFE control 1 */
	rtlphy->phyreg_def[RF90_PATH_A].rftxiq_imbal = ROFDM0_XATXIQIMBALANCE;
	rtlphy->phyreg_def[RF90_PATH_B].rftxiq_imbal = ROFDM0_XBTXIQIMBALANCE;
	rtlphy->phyreg_def[RF90_PATH_C].rftxiq_imbal = ROFDM0_XCTXIQIMBALANCE;
	rtlphy->phyreg_def[RF90_PATH_D].rftxiq_imbal = ROFDM0_XDTXIQIMBALANCE;

	/* Tx AFE control 2 */
	rtlphy->phyreg_def[RF90_PATH_A].rftx_afe = ROFDM0_XATXAFE;
	rtlphy->phyreg_def[RF90_PATH_B].rftx_afe = ROFDM0_XBTXAFE;
	rtlphy->phyreg_def[RF90_PATH_C].rftx_afe = ROFDM0_XCTXAFE;
	rtlphy->phyreg_def[RF90_PATH_D].rftx_afe = ROFDM0_XDTXAFE;

	/* Transceiver LSSI Readback SI mode */
	rtlphy->phyreg_def[RF90_PATH_A].rf_rb = RFPGA0_XA_LSSIREADBACK;
	rtlphy->phyreg_def[RF90_PATH_B].rf_rb = RFPGA0_XB_LSSIREADBACK;
	rtlphy->phyreg_def[RF90_PATH_C].rf_rb = RFPGA0_XC_LSSIREADBACK;
	rtlphy->phyreg_def[RF90_PATH_D].rf_rb = RFPGA0_XD_LSSIREADBACK;

	/* Transceiver LSSI Readback PI mode */
	rtlphy->phyreg_def[RF90_PATH_A].rf_rbpi = TRANSCEIVERA_HSPI_READBACK;
	rtlphy->phyreg_def[RF90_PATH_B].rf_rbpi = TRANSCEIVERB_HSPI_READBACK;
}


/* rtl8192d/phy_common.c: rtl92d_phy_mac_setting_calibration */
static void rtl92d_phy_mac_setting_calibration(struct ieee80211_hw *hw,
					const u32 *macreg, u32 *macbackup)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK, "MAC settings for Calibration.\n");
	rtl_write_byte(rtlpriv, macreg[0], 0x3F);

	for (i = 1; i < (IQK_MAC_REG_NUM - 1); i++)
		rtl_write_byte(rtlpriv, macreg[i], (u8)(macbackup[i] &
			       (~BIT(3))));
	rtl_write_byte(rtlpriv, macreg[i], (u8)(macbackup[i] & (~BIT(5))));
}


/* rtl8192d/phy_common.c: rtl92d_phy_path_adda_on */
static void rtl92d_phy_path_adda_on(struct ieee80211_hw *hw,
			     const u32 *adda_reg, bool patha_on, bool is2t)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 pathon;
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "ADDA ON.\n");
	pathon = patha_on ? 0x04db25a4 : 0x0b1b25a4;
	if (patha_on)
		pathon = rtlpriv->rtlhal.interfaceindex == 0 ?
		    0x04db25a4 : 0x0b1b25a4;
	for (i = 0; i < IQK_ADDA_REG_NUM; i++)
		rtl_set_bbreg(hw, adda_reg[i], MASKDWORD, pathon);
}


/* rtl8192d/phy_common.c: rtl92d_phy_query_rf_reg */
static u32 rtl92d_phy_query_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			    u32 regaddr, u32 bitmask)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 original_value, readback_value, bitshift;

	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"regaddr(%#x), rfpath(%#x), bitmask(%#x)\n",
		regaddr, rfpath, bitmask);
	rtl92d_pci_lock(rtlpriv);
	original_value = _rtl92d_phy_rf_serial_read(hw, rfpath, regaddr);
	bitshift = calculate_bit_shift(bitmask);
	readback_value = (original_value & bitmask) >> bitshift;
	rtl92d_pci_unlock(rtlpriv);
	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"regaddr(%#x), rfpath(%#x), bitmask(%#x), original_value(%#x)\n",
		regaddr, rfpath, bitmask, original_value);
	return readback_value;
}


/* rtl8192d/phy_common.c: rtl92d_phy_reset_iqk_result */
static void rtl92d_phy_reset_iqk_result(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u8 i;

	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
		"settings regs %zu default regs %d\n",
		ARRAY_SIZE(rtlphy->iqk_matrix),
		IQK_MATRIX_REG_NUM);
	/* 0xe94, 0xe9c, 0xea4, 0xeac, 0xeb4, 0xebc, 0xec4, 0xecc */
	for (i = 0; i < IQK_MATRIX_SETTINGS_NUM; i++) {
		rtlphy->iqk_matrix[i].value[0][0] = 0x100;
		rtlphy->iqk_matrix[i].value[0][2] = 0x100;
		rtlphy->iqk_matrix[i].value[0][4] = 0x100;
		rtlphy->iqk_matrix[i].value[0][6] = 0x100;
		rtlphy->iqk_matrix[i].value[0][1] = 0x0;
		rtlphy->iqk_matrix[i].value[0][3] = 0x0;
		rtlphy->iqk_matrix[i].value[0][5] = 0x0;
		rtlphy->iqk_matrix[i].value[0][7] = 0x0;
		rtlphy->iqk_matrix[i].iqk_done = false;
	}
}


/* rtl8192d/phy_common.c: rtl92d_phy_restore_rf_env */
static void rtl92d_phy_restore_rf_env(struct ieee80211_hw *hw, u8 rfpath,
			       u32 *pu4_regval)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct bb_reg_def *pphyreg = &rtlphy->phyreg_def[rfpath];

	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "=====>\n");
	/*----Restore RFENV control type----*/
	switch (rfpath) {
	case RF90_PATH_A:
	case RF90_PATH_C:
		rtl_set_bbreg(hw, pphyreg->rfintfs, BRFSI_RFENV, *pu4_regval);
		break;
	case RF90_PATH_B:
	case RF90_PATH_D:
		rtl_set_bbreg(hw, pphyreg->rfintfs, BRFSI_RFENV << 16,
			      *pu4_regval);
		break;
	}
	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "<=====\n");
}


/* rtl8192d/rf_common.c: rtl92d_phy_rf6052_set_bandwidth */
static void rtl92d_phy_rf6052_set_bandwidth(struct ieee80211_hw *hw, u8 bandwidth)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u8 rfpath;

	switch (bandwidth) {
	case HT_CHANNEL_WIDTH_20:
		for (rfpath = 0; rfpath < rtlphy->num_total_rfpath; rfpath++) {
			rtlphy->rfreg_chnlval[rfpath] &= 0xfffff3ff;
			rtlphy->rfreg_chnlval[rfpath] |= 0x0400;

			rtl_set_rfreg(hw, rfpath, RF_CHNLBW,
				      BIT(10) | BIT(11), 0x01);

			rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD,
				"20M RF 0x18 = 0x%x\n",
				rtlphy->rfreg_chnlval[rfpath]);
		}

		break;
	case HT_CHANNEL_WIDTH_20_40:
		for (rfpath = 0; rfpath < rtlphy->num_total_rfpath; rfpath++) {
			rtlphy->rfreg_chnlval[rfpath] &= 0xfffff3ff;

			rtl_set_rfreg(hw, rfpath, RF_CHNLBW,
				      BIT(10) | BIT(11), 0x00);

			rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD,
				"40M RF 0x18 = 0x%x\n",
				rtlphy->rfreg_chnlval[rfpath]);
		}
		break;
	default:
		pr_err("unknown bandwidth: %#X\n", bandwidth);
		break;
	}
}


/* rtl8192d/rf_common.c: rtl92d_phy_rf6052_set_cck_txpower */
static void rtl92d_phy_rf6052_set_cck_txpower(struct ieee80211_hw *hw,
				       u8 *ppowerlevel)
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_mac *mac = rtl_mac(rtl_priv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 tx_agc[2] = {0, 0}, tmpval;
	bool turbo_scanoff = false;
	u8 idx1, idx2;

	if (rtlefuse->eeprom_regulatory != 0)
		turbo_scanoff = true;
	if (mac->act_scanning) {
		tx_agc[RF90_PATH_A] = 0x3f3f3f3f;
		tx_agc[RF90_PATH_B] = 0x3f3f3f3f;
		if (turbo_scanoff) {
			for (idx1 = RF90_PATH_A; idx1 <= RF90_PATH_B; idx1++) {
				tx_agc[idx1] = ppowerlevel[idx1] |
				    (ppowerlevel[idx1] << 8) |
				    (ppowerlevel[idx1] << 16) |
				    (ppowerlevel[idx1] << 24);
			}
		}
	} else {
		for (idx1 = RF90_PATH_A; idx1 <= RF90_PATH_B; idx1++) {
			tx_agc[idx1] = ppowerlevel[idx1] |
			    (ppowerlevel[idx1] << 8) |
			    (ppowerlevel[idx1] << 16) |
			    (ppowerlevel[idx1] << 24);
		}
		if (rtlefuse->eeprom_regulatory == 0) {
			tmpval = (rtlphy->mcs_offset[0][6]) +
			    (rtlphy->mcs_offset[0][7] << 8);
			tx_agc[RF90_PATH_A] += tmpval;
			tmpval = (rtlphy->mcs_offset[0][14]) +
			    (rtlphy->mcs_offset[0][15] << 24);
			tx_agc[RF90_PATH_B] += tmpval;
		}
	}

	for (idx1 = RF90_PATH_A; idx1 <= RF90_PATH_B; idx1++) {
		u32 clamped = 0;
		for (idx2 = 0; idx2 < 4; idx2++) {
			u32 byte = (tx_agc[idx1] >> (idx2 * 8)) & 0xff;
			if (byte > RF6052_MAX_TX_PWR) byte = RF6052_MAX_TX_PWR;
			clamped |= byte << (idx2 * 8);
		}
		tx_agc[idx1] = clamped;
	}

	tmpval = tx_agc[RF90_PATH_A] & 0xff;
	rtl_set_bbreg(hw, RTXAGC_A_CCK1_MCS32, MASKBYTE1, tmpval);
	RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
		"CCK PWR 1M (rf-A) = 0x%x (reg 0x%x)\n",
		tmpval, RTXAGC_A_CCK1_MCS32);
	tmpval = tx_agc[RF90_PATH_A] >> 8;
	rtl_set_bbreg(hw, RTXAGC_B_CCK11_A_CCK2_11, 0xffffff00, tmpval);
	RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
		"CCK PWR 2~11M (rf-A) = 0x%x (reg 0x%x)\n",
		tmpval, RTXAGC_B_CCK11_A_CCK2_11);
	tmpval = tx_agc[RF90_PATH_B] >> 24;
	rtl_set_bbreg(hw, RTXAGC_B_CCK11_A_CCK2_11, MASKBYTE0, tmpval);
	RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
		"CCK PWR 11M (rf-B) = 0x%x (reg 0x%x)\n",
		tmpval, RTXAGC_B_CCK11_A_CCK2_11);
	tmpval = tx_agc[RF90_PATH_B] & 0x00ffffff;
	rtl_set_bbreg(hw, RTXAGC_B_CCK1_55_MCS32, 0xffffff00, tmpval);
	RTPRINT(rtlpriv, FPHY, PHY_TXPWR,
		"CCK PWR 1~5.5M (rf-B) = 0x%x (reg 0x%x)\n",
		tmpval, RTXAGC_B_CCK1_55_MCS32);
}


/* rtl8192d/rf_common.c: rtl92d_phy_rf6052_set_ofdm_txpower */
static void rtl92d_phy_rf6052_set_ofdm_txpower(struct ieee80211_hw *hw,
					u8 *ppowerlevel, u8 channel)
{
	u32 writeval[2], powerbase0[2], powerbase1[2];
	u8 index;

	_rtl92d_phy_get_power_base(hw, ppowerlevel, channel,
				   &powerbase0[0], &powerbase1[0]);
	for (index = 0; index < 6; index++) {
		_rtl92d_get_txpower_writeval_by_regulatory(hw, channel, index,
							   &powerbase0[0],
							   &powerbase1[0],
							   &writeval[0]);
		_rtl92d_write_ofdm_power_reg(hw, index, &writeval[0]);
	}
}


/* rtl8192d/phy_common.c: rtl92d_phy_save_adda_registers */
static void rtl92d_phy_save_adda_registers(struct ieee80211_hw *hw, const u32 *adda_reg,
				    u32 *adda_backup, u32 regnum)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Save ADDA parameters.\n");
	for (i = 0; i < regnum; i++)
		adda_backup[i] = rtl_get_bbreg(hw, adda_reg[i], MASKDWORD);
}


/* rtl8192d/phy_common.c: rtl92d_phy_save_mac_registers */
static void rtl92d_phy_save_mac_registers(struct ieee80211_hw *hw,
				   const u32 *macreg, u32 *macbackup)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 i;

	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "Save MAC parameters.\n");
	for (i = 0; i < (IQK_MAC_REG_NUM - 1); i++)
		macbackup[i] = rtl_read_byte(rtlpriv, macreg[i]);
	macbackup[i] = rtl_read_dword(rtlpriv, macreg[i]);
}


/* rtl8192d/phy_common.c: rtl92d_phy_set_rf_reg */
static void rtl92d_phy_set_rf_reg(struct ieee80211_hw *hw, enum radio_path rfpath,
			   u32 regaddr, u32 bitmask, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 original_value, bitshift;

	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"regaddr(%#x), bitmask(%#x), data(%#x), rfpath(%#x)\n",
		regaddr, bitmask, data, rfpath);
	if (bitmask == 0)
		return;
	rtl92d_pci_lock(rtlpriv);
	if (rtlphy->rf_mode != RF_OP_BY_FW) {
		if (bitmask != RFREG_OFFSET_MASK) {
			original_value = _rtl92d_phy_rf_serial_read(hw,
								    rfpath,
								    regaddr);
			bitshift = calculate_bit_shift(bitmask);
			data = ((original_value & (~bitmask)) |
				(data << bitshift));
		}
		_rtl92d_phy_rf_serial_write(hw, rfpath, regaddr, data);
	}
	rtl92d_pci_unlock(rtlpriv);
	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"regaddr(%#x), bitmask(%#x), data(%#x), rfpath(%#x)\n",
		regaddr, bitmask, data, rfpath);
}


/* rtl8192d/phy_common.c: rtl92d_phy_set_txpower_level */
static void rtl92d_phy_set_txpower_level(struct ieee80211_hw *hw, u8 channel)
{
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 cckpowerlevel[2], ofdmpowerlevel[2];

	if (!rtlefuse->txpwr_fromeprom)
		return;
	channel = _rtl92c_phy_get_rightchnlplace(channel);
	_rtl92d_get_txpower_index(hw, channel, &cckpowerlevel[0],
				  &ofdmpowerlevel[0]);
	if (rtlpriv->rtlhal.current_bandtype == BAND_ON_2_4G)
		_rtl92d_ccxpower_index_check(hw, channel, &cckpowerlevel[0],
					     &ofdmpowerlevel[0]);
	if (rtlpriv->rtlhal.current_bandtype == BAND_ON_2_4G)
		rtl92d_phy_rf6052_set_cck_txpower(hw, &cckpowerlevel[0]);
	rtl92d_phy_rf6052_set_ofdm_txpower(hw, &ofdmpowerlevel[0], channel);
}


/* rtl8192d/phy_common.c: rtl92d_store_pwrindex_diffrate_offset */
static void rtl92d_store_pwrindex_diffrate_offset(struct ieee80211_hw *hw,
					   u32 regaddr, u32 bitmask, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	int index;

	if (regaddr == RTXAGC_A_RATE18_06)
		index = 0;
	else if (regaddr == RTXAGC_A_RATE54_24)
		index = 1;
	else if (regaddr == RTXAGC_A_CCK1_MCS32)
		index = 6;
	else if (regaddr == RTXAGC_B_CCK11_A_CCK2_11 && bitmask == 0xffffff00)
		index = 7;
	else if (regaddr == RTXAGC_A_MCS03_MCS00)
		index = 2;
	else if (regaddr == RTXAGC_A_MCS07_MCS04)
		index = 3;
	else if (regaddr == RTXAGC_A_MCS11_MCS08)
		index = 4;
	else if (regaddr == RTXAGC_A_MCS15_MCS12)
		index = 5;
	else if (regaddr == RTXAGC_B_RATE18_06)
		index = 8;
	else if (regaddr == RTXAGC_B_RATE54_24)
		index = 9;
	else if (regaddr == RTXAGC_B_CCK1_55_MCS32)
		index = 14;
	else if (regaddr == RTXAGC_B_CCK11_A_CCK2_11 && bitmask == 0x000000ff)
		index = 15;
	else if (regaddr == RTXAGC_B_MCS03_MCS00)
		index = 10;
	else if (regaddr == RTXAGC_B_MCS07_MCS04)
		index = 11;
	else if (regaddr == RTXAGC_B_MCS11_MCS08)
		index = 12;
	else if (regaddr == RTXAGC_B_MCS15_MCS12)
		index = 13;
	else
		return;

	rtlphy->mcs_offset[rtlphy->pwrgroup_cnt][index] = data;
	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE,
		"MCSTxPowerLevelOriginalOffset[%d][%d] = 0x%x\n",
		rtlphy->pwrgroup_cnt, index,
		rtlphy->mcs_offset[rtlphy->pwrgroup_cnt][index]);
	if (index == 13)
		rtlphy->pwrgroup_cnt++;
}


/* rtl8192du/phy.c: rtl92du_phy_bb_config */
static bool rtl92du_phy_bb_config(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	bool rtstatus;
	u32 regvaldw;
	u16 regval;
	u8 value;

	rtl92d_phy_init_bb_rf_register_definition(hw);

	regval = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN);
	rtl_write_word(rtlpriv, REG_SYS_FUNC_EN,
		       regval | BIT(13) | BIT(0) | BIT(1));

	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL, 0x83);
	rtl_write_byte(rtlpriv, REG_AFE_PLL_CTRL + 1, 0xdb);

	/* 0x1f bit7 bit6 represent for mac0/mac1 driver ready */
	value = rtl_read_byte(rtlpriv, REG_RF_CTRL);
	rtl_write_byte(rtlpriv, REG_RF_CTRL, value | RF_EN | RF_RSTB |
		RF_SDMRSTB);

	value = FEN_BB_GLB_RSTN | FEN_BBRSTB;
	if (rtlhal->interface == INTF_PCI)
		value |= FEN_PPLL | FEN_PCIEA | FEN_DIO_PCIE;
	else if (rtlhal->interface == INTF_USB)
		value |= FEN_USBA | FEN_USBD;
	rtl_write_byte(rtlpriv, REG_SYS_FUNC_EN, value);

	regvaldw = rtl_read_dword(rtlpriv, RFPGA0_XCD_RFPARAMETER);
	regvaldw &= ~BIT(31);
	rtl_write_dword(rtlpriv, RFPGA0_XCD_RFPARAMETER, regvaldw);

	/* To Fix MAC loopback mode fail. */
	rtl_write_byte(rtlpriv, REG_LDOHCI12_CTRL, 0x0f);
	rtl_write_byte(rtlpriv, 0x15, 0xe9);

	rtl_write_byte(rtlpriv, REG_AFE_XTAL_CTRL + 1, 0x80);
	if (!(IS_92D_SINGLEPHY(rtlpriv->rtlhal.version)) &&
	    rtlhal->interface == INTF_PCI) {
		regvaldw = rtl_read_dword(rtlpriv, REG_LEDCFG0);
		rtl_write_dword(rtlpriv, REG_LEDCFG0, regvaldw | BIT(23));
	}

	rtstatus = _rtl92du_phy_bb_config(hw);

	/* Crystal calibration */
	rtl_set_bbreg(hw, REG_AFE_XTAL_CTRL, 0xf0,
		      rtlpriv->efuse.crystalcap & 0x0f);
	rtl_set_bbreg(hw, REG_AFE_PLL_CTRL, 0xf0000000,
		      (rtlpriv->efuse.crystalcap & 0xf0) >> 4);

	return rtstatus;
}


/* rtl8192du/phy.c: rtl92du_phy_config_rf_with_headerfile */
static bool rtl92du_phy_config_rf_with_headerfile(struct ieee80211_hw *hw,
					   enum rf_content content,
					   enum radio_path rfpath)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u16 radioa_arraylen, radiob_arraylen;
	const u32 *radioa_array_table;
	const u32 *radiob_array_table;
	int i;

	radioa_arraylen = RADIOA_2T_ARRAYLENGTH;
	radioa_array_table = rtl8192du_radioa_2tarray;
	radiob_arraylen = RADIOB_2T_ARRAYLENGTH;
	radiob_array_table = rtl8192du_radiob_2tarray;
	if (rtlpriv->efuse.internal_pa_5g[0]) {
		radioa_arraylen = RADIOA_2T_INT_PA_ARRAYLENGTH;
		radioa_array_table = rtl8192du_radioa_2t_int_paarray;
	}
	if (rtlpriv->efuse.internal_pa_5g[1]) {
		radiob_arraylen = RADIOB_2T_INT_PA_ARRAYLENGTH;
		radiob_array_table = rtl8192du_radiob_2t_int_paarray;
	}
	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
		"PHY_ConfigRFWithHeaderFile() Radio_A:Rtl819XRadioA_1TArray\n");
	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
		"PHY_ConfigRFWithHeaderFile() Radio_B:Rtl819XRadioB_1TArray\n");
	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE, "Radio No %x\n", rfpath);

	/* this only happens when DMDP, mac0 start on 2.4G,
	 * mac1 start on 5G, mac 0 has to set phy0 & phy1
	 * pathA or mac1 has to set phy0 & phy1 pathA
	 */
	if (content == radiob_txt && rfpath == RF90_PATH_A) {
		rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD,
			" ===> althougth Path A, we load radiob.txt\n");
		radioa_arraylen = radiob_arraylen;
		radioa_array_table = radiob_array_table;
	}

	switch (rfpath) {
	case RF90_PATH_A:
		for (i = 0; i < radioa_arraylen; i = i + 2) {
			rtl_rfreg_delay(hw, rfpath, radioa_array_table[i],
					RFREG_OFFSET_MASK,
					radioa_array_table[i + 1]);
		}
		break;
	case RF90_PATH_B:
		for (i = 0; i < radiob_arraylen; i = i + 2) {
			rtl_rfreg_delay(hw, rfpath, radiob_array_table[i],
					RFREG_OFFSET_MASK,
					radiob_array_table[i + 1]);
		}
		break;
	case RF90_PATH_C:
	case RF90_PATH_D:
		pr_err("switch case %#x not processed\n", rfpath);
		break;
	}

	return true;
}


/* rtl8192du/rf.c: rtl92du_phy_enable_anotherphy */
static bool rtl92du_phy_enable_anotherphy(struct ieee80211_hw *hw, bool bmac0)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u8 mac_on_bit = bmac0 ? MAC1_ON : MAC0_ON;
	u8 mac_reg = bmac0 ? REG_MAC1 : REG_MAC0;
	bool bresult = true; /* true: need to enable BB/RF power */
	u32 maskforphyset = 0;
	u16 val16;
	u8 u1btmp;

	rtlhal->during_mac0init_radiob = false;
	rtlhal->during_mac1init_radioa = false;
	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "===>\n");

	/* MAC0 Need PHY1 load radio_b.txt . Driver use DBI to write. */
	u1btmp = rtl_read_byte(rtlpriv, mac_reg);
	if (!(u1btmp & mac_on_bit)) {
		rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD, "enable BB & RF\n");
		/* Enable BB and RF power */

		maskforphyset = bmac0 ? MAC0_ACCESS_PHY1 : MAC1_ACCESS_PHY0;

		val16 = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN | maskforphyset);
		val16 &= 0xfffc;
		rtl_write_word(rtlpriv, REG_SYS_FUNC_EN | maskforphyset, val16);

		val16 = rtl_read_word(rtlpriv, REG_SYS_FUNC_EN | maskforphyset);
		val16 |= BIT(13) | BIT(0) | BIT(1);
		rtl_write_word(rtlpriv, REG_SYS_FUNC_EN | maskforphyset, val16);
	} else {
		/* We think if MAC1 is ON,then radio_a.txt
		 * and radio_b.txt has been load.
		 */
		bresult = false;
	}
	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "<===\n");
	return bresult;
}


/* rtl8192du/phy.c: rtl92du_phy_init_pa_bias */
static void rtl92du_phy_init_pa_bias(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	bool is_single_mac = rtlhal->macphymode == SINGLEMAC_SINGLEPHY;
	enum radio_path rf_path;
	u8 val8;

	read_efuse_byte(hw, 0x3FA, &val8);

	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "%s: 0x3FA %#x\n",
		__func__, val8);

	if (!(val8 & BIT(0)) && (is_single_mac || rtlhal->interfaceindex == 0)) {
		rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW, RFREG_OFFSET_MASK, 0x07401);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x0F425);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x4F425);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x8F425);

		/* Back to RX Mode */
		rtl_set_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK, 0x30000);

		rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "2G PA BIAS path A\n");
	}

	if (!(val8 & BIT(1)) && (is_single_mac || rtlhal->interfaceindex == 1)) {
		rf_path = rtlhal->interfaceindex == 1 ? RF90_PATH_A : RF90_PATH_B;

		rtl_set_rfreg(hw, rf_path, RF_CHNLBW, RFREG_OFFSET_MASK, 0x07401);
		rtl_set_rfreg(hw, rf_path, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x0F425);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x4F425);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x8F425);

		/* Back to RX Mode */
		rtl_set_rfreg(hw, rf_path, RF_AC, RFREG_OFFSET_MASK, 0x30000);

		rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "2G PA BIAS path B\n");
	}

	if (!(val8 & BIT(2)) && (is_single_mac || rtlhal->interfaceindex == 0)) {
		/* 5GL_channel */
		rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW, RFREG_OFFSET_MASK, 0x17524);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x0F496);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x4F496);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x8F496);

		/* 5GM_channel */
		rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW, RFREG_OFFSET_MASK, 0x37564);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x0F496);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x4F496);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x8F496);

		/* 5GH_channel */
		rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW, RFREG_OFFSET_MASK, 0x57595);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x0F496);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x4F496);
		rtl_set_rfreg(hw, RF90_PATH_A, RF_IPA, RFREG_OFFSET_MASK, 0x8F496);

		/* Back to RX Mode */
		rtl_set_rfreg(hw, RF90_PATH_A, RF_AC, RFREG_OFFSET_MASK, 0x30000);

		rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "5G PA BIAS path A\n");
	}

	if (!(val8 & BIT(3)) && (is_single_mac || rtlhal->interfaceindex == 1)) {
		rf_path = rtlhal->interfaceindex == 1 ? RF90_PATH_A : RF90_PATH_B;

		/* 5GL_channel */
		rtl_set_rfreg(hw, rf_path, RF_CHNLBW, RFREG_OFFSET_MASK, 0x17524);
		rtl_set_rfreg(hw, rf_path, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x0F496);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x4F496);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x8F496);

		/* 5GM_channel */
		rtl_set_rfreg(hw, rf_path, RF_CHNLBW, RFREG_OFFSET_MASK, 0x37564);
		rtl_set_rfreg(hw, rf_path, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x0F496);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x4F496);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x8F496);

		/* 5GH_channel */
		rtl_set_rfreg(hw, rf_path, RF_CHNLBW, RFREG_OFFSET_MASK, 0x57595);
		rtl_set_rfreg(hw, rf_path, RF_AC, RFREG_OFFSET_MASK, 0x70000);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x0F496);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x4F496);
		rtl_set_rfreg(hw, rf_path, RF_IPA, RFREG_OFFSET_MASK, 0x8F496);

		/* Back to RX Mode */
		rtl_set_rfreg(hw, rf_path, RF_AC, RFREG_OFFSET_MASK, 0x30000);

		rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "5G PA BIAS path B\n");
	}
}


/* rtl8192du/phy.c: rtl92du_phy_iq_calibrate */
static void rtl92du_phy_iq_calibrate(struct ieee80211_hw *hw)
{
	long rege94, rege9c, regea4, regeac, regeb4;
	bool is12simular, is13simular, is23simular;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	long regebc, regec4, regecc, regtmp = 0;
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u8 i, final_candidate, indexforchannel;
	bool patha_ok, pathb_ok;
	long result[4][8] = {0};

	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"IQK:Start!!!channel %d\n", rtlphy->current_channel);

	final_candidate = 0xff;
	patha_ok = false;
	pathb_ok = false;
	is12simular = false;
	is23simular = false;
	is13simular = false;
	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"IQK !!!currentband %d\n", rtlhal->current_bandtype);

	for (i = 0; i < 3; i++) {
		if (rtlhal->current_bandtype == BAND_ON_5G) {
			_rtl92du_phy_iq_calibrate_5g_normal(hw, result, i);
		} else if (rtlhal->current_bandtype == BAND_ON_2_4G) {
			if (IS_92D_SINGLEPHY(rtlhal->version))
				_rtl92du_phy_iq_calibrate(hw, result, i, true);
			else
				_rtl92du_phy_iq_calibrate(hw, result, i, false);
		}

		if (i == 1) {
			is12simular = _rtl92du_phy_simularity_compare(hw, result,
								      0, 1);
			if (is12simular) {
				final_candidate = 0;
				break;
			}
		}

		if (i == 2) {
			is13simular = _rtl92du_phy_simularity_compare(hw, result,
								      0, 2);
			if (is13simular) {
				final_candidate = 0;
				break;
			}

			is23simular = _rtl92du_phy_simularity_compare(hw, result,
								      1, 2);
			if (is23simular) {
				final_candidate = 1;
			} else {
				for (i = 0; i < 8; i++)
					regtmp += result[3][i];

				if (regtmp != 0)
					final_candidate = 3;
				else
					final_candidate = 0xFF;
			}
		}
	}

	for (i = 0; i < 4; i++) {
		rege94 = result[i][0];
		rege9c = result[i][1];
		regea4 = result[i][2];
		regeac = result[i][3];
		regeb4 = result[i][4];
		regebc = result[i][5];
		regec4 = result[i][6];
		regecc = result[i][7];
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"IQK: rege94=%lx rege9c=%lx regea4=%lx regeac=%lx regeb4=%lx regebc=%lx regec4=%lx regecc=%lx\n",
			rege94, rege9c, regea4, regeac, regeb4, regebc, regec4,
			regecc);
	}

	if (final_candidate != 0xff) {
		rege94 = result[final_candidate][0];
		rtlphy->reg_e94 = rege94;
		rege9c = result[final_candidate][1];
		rtlphy->reg_e9c = rege9c;
		regea4 = result[final_candidate][2];
		regeac = result[final_candidate][3];
		regeb4 = result[final_candidate][4];
		rtlphy->reg_eb4 = regeb4;
		regebc = result[final_candidate][5];
		rtlphy->reg_ebc = regebc;
		regec4 = result[final_candidate][6];
		regecc = result[final_candidate][7];

		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"IQK: final_candidate is %x\n", final_candidate);
		RTPRINT(rtlpriv, FINIT, INIT_IQK,
			"IQK: rege94=%lx rege9c=%lx regea4=%lx regeac=%lx regeb4=%lx regebc=%lx regec4=%lx regecc=%lx\n",
			rege94, rege9c, regea4, regeac, regeb4, regebc, regec4,
			regecc);

		patha_ok = true;
		pathb_ok = true;
		if (!rege94 || (IS_92D_SINGLEPHY(rtlhal->version) && !regeb4))
			du_algorithm_failure(hw, RTW_92DU_POLL_TIMEOUT);
	} else {
		rtlphy->reg_e94 = 0x100;
		rtlphy->reg_eb4 = 0x100; /* X default value */
		rtlphy->reg_e9c = 0x0;
		rtlphy->reg_ebc = 0x0;   /* Y default value */
	}
	if (rege94 != 0 /*&& regea4 != 0*/)
		_rtl92du_phy_patha_fill_iqk_matrix(hw, patha_ok, result,
						   final_candidate,
						   regea4 == 0);
	if (IS_92D_SINGLEPHY(rtlhal->version) &&
	    regeb4 != 0 /*&& regec4 != 0*/)
		_rtl92du_phy_pathb_fill_iqk_matrix(hw, pathb_ok, result,
						   final_candidate,
						   regec4 == 0);

	if (final_candidate != 0xFF) {
		indexforchannel =
			rtl92d_get_rightchnlplace_for_iqk(rtlphy->current_channel);

		for (i = 0; i < IQK_MATRIX_REG_NUM; i++)
			rtlphy->iqk_matrix[indexforchannel].value[0][i] =
				result[final_candidate][i];

		rtlphy->iqk_matrix[indexforchannel].iqk_done = true;

		rtl_dbg(rtlpriv, COMP_SCAN | COMP_MLME, DBG_LOUD,
			"IQK OK indexforchannel %d\n", indexforchannel);
	}
}


/* rtl8192du/phy.c: rtl92du_phy_lc_calibrate */
static void rtl92du_phy_lc_calibrate(struct ieee80211_hw *hw, bool is2t)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u32 timeout = 2000, timecount = 0;

	while (rtlpriv->mac80211.act_scanning && timecount < timeout) {
		udelay(50);
		timecount += 50;
	}

	rtlphy->lck_inprogress = true;
	RTPRINT(rtlpriv, FINIT, INIT_IQK,
		"LCK:Start!!! currentband %x delay %d ms\n",
		rtlhal->current_bandtype, timecount);

	_rtl92du_phy_lc_calibrate_sw(hw, is2t);

	rtlphy->lck_inprogress = false;
	RTPRINT(rtlpriv, FINIT, INIT_IQK,  "LCK:Finish!!!\n");
}


/* rtl8192du/phy.c: rtl92du_phy_mac_config */
static bool rtl92du_phy_mac_config(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 arraylength;
	const u32 *ptrarray;
	u32 i;

	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE, "Read Rtl819XMACPHY_Array\n");

	arraylength = MAC_2T_ARRAYLENGTH;
	ptrarray = rtl8192du_mac_2tarray;

	for (i = 0; i < arraylength; i = i + 2)
		rtl_write_byte(rtlpriv, ptrarray[i], (u8)ptrarray[i + 1]);

	if (rtlpriv->rtlhal.macphymode == SINGLEMAC_SINGLEPHY) {
		/* improve 2-stream TX EVM */
		/* rtl_write_byte(rtlpriv, 0x14,0x71); */
		/* AMPDU aggregation number 9 */
		/* rtl_write_word(rtlpriv, REG_MAX_AGGR_NUM, MAX_AGGR_NUM); */
		rtl_write_byte(rtlpriv, REG_MAX_AGGR_NUM, 0x0B);
	} else {
		/* 92D need to test to decide the num. */
		rtl_write_byte(rtlpriv, REG_MAX_AGGR_NUM, 0x07);
	}

	return true;
}


/* rtl8192du/rf.c: rtl92du_phy_powerdown_anotherphy */
static void rtl92du_phy_powerdown_anotherphy(struct ieee80211_hw *hw, bool bmac0)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	u8 mac_on_bit = bmac0 ? MAC1_ON : MAC0_ON;
	u8 mac_reg = bmac0 ? REG_MAC1 : REG_MAC0;
	u32 maskforphyset = 0;
	u8 u1btmp;

	rtlhal->during_mac0init_radiob = false;
	rtlhal->during_mac1init_radioa = false;
	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "====>\n");

	/* check MAC0 enable or not again now, if
	 * enabled, not power down radio A.
	 */
	u1btmp = rtl_read_byte(rtlpriv, mac_reg);
	if (!(u1btmp & mac_on_bit)) {
		rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD, "power down\n");
		/* power down RF radio A according to YuNan's advice. */
		maskforphyset = bmac0 ? MAC0_ACCESS_PHY1 : MAC1_ACCESS_PHY0;
		rtl_write_dword(rtlpriv, RFPGA0_XA_LSSIPARAMETER | maskforphyset,
				0x00000000);
	}
	rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "<====\n");
}


/* rtl8192du/phy.c: rtl92du_phy_query_bb_reg */
static u32 rtl92du_phy_query_bb_reg(struct ieee80211_hw *hw, u32 regaddr, u32 bitmask)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	u32 returnvalue, originalvalue, bitshift;

	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE, "regaddr(%#x), bitmask(%#x)\n",
		regaddr, bitmask);

	if (rtlhal->during_mac1init_radioa)
		regaddr |= MAC1_ACCESS_PHY0;
	else if (rtlhal->during_mac0init_radiob)
		regaddr |= MAC0_ACCESS_PHY1;

	originalvalue = rtl_read_dword(rtlpriv, regaddr);
	bitshift = calculate_bit_shift(bitmask);
	returnvalue = (originalvalue & bitmask) >> bitshift;
	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"BBR MASK=0x%x Addr[0x%x]=0x%x\n",
		bitmask, regaddr, originalvalue);
	return returnvalue;
}


/* rtl8192du/phy.c: rtl92du_phy_reload_iqk_setting */
static void rtl92du_phy_reload_iqk_setting(struct ieee80211_hw *hw, u8 channel)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct rtl_mac *mac = rtl_mac(rtlpriv);
	u8 indexforchannel;
	bool need_iqk;

	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "channel %d\n", channel);
	/*------Do IQK for normal chip and test chip 5G band------- */

	indexforchannel = rtl92d_get_rightchnlplace_for_iqk(channel);
	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "indexforchannel %d done %d\n",
		indexforchannel,
		rtlphy->iqk_matrix[indexforchannel].iqk_done);

	/* We need to do IQK if we're about to connect to a network on 5 GHz.
	 * On 5 GHz a channel switch outside of scanning happens only before
	 * connecting.
	 */
	need_iqk = !mac->act_scanning;

	if (!rtlphy->iqk_matrix[indexforchannel].iqk_done && need_iqk) {
		rtl_dbg(rtlpriv, COMP_SCAN | COMP_INIT, DBG_LOUD,
			"Do IQK Matrix reg for channel:%d....\n", channel);
		rtl92du_phy_iq_calibrate(hw);
		return;
	}

	/* Just load the value. */
	/* 2G band just load once. */
	if ((!rtlhal->load_imrandiqk_setting_for2g && indexforchannel == 0) ||
	    indexforchannel > 0) {
		rtl_dbg(rtlpriv, COMP_SCAN, DBG_LOUD,
			"Just Read IQK Matrix reg for channel:%d....\n",
			channel);

		if (rtlphy->iqk_matrix[indexforchannel].value[0][0] != 0)
			_rtl92du_phy_patha_fill_iqk_matrix(hw, true,
				rtlphy->iqk_matrix[indexforchannel].value, 0,
				rtlphy->iqk_matrix[indexforchannel].value[0][2] == 0);

		if (IS_92D_SINGLEPHY(rtlhal->version) &&
		    rtlphy->iqk_matrix[indexforchannel].value[0][4] != 0)
			_rtl92du_phy_pathb_fill_iqk_matrix(hw, true,
				rtlphy->iqk_matrix[indexforchannel].value, 0,
				rtlphy->iqk_matrix[indexforchannel].value[0][6] == 0);
	}
	rtl_dbg(rtlpriv, COMP_CMD, DBG_LOUD, "<====\n");
}


/* rtl8192du/rf.c: rtl92du_phy_rf6052_config */
static bool rtl92du_phy_rf6052_config(struct ieee80211_hw *hw)
{
	bool mac1_initradioa_first = false, mac0_initradiob_first = false;
	bool need_pwrdown_radioa = false, need_pwrdown_radiob = false;
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = &rtlpriv->rtlhal;
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct bb_reg_def *pphyreg;
	bool true_bpath = false;
	bool rtstatus = true;
	u32 u4_regvalue = 0;
	u8 rfpath;

	if (rtlphy->rf_type == RF_1T1R)
		rtlphy->num_total_rfpath = 1;
	else
		rtlphy->num_total_rfpath = 2;

	/* Single phy mode: use radio_a radio_b config path_A path_B
	 * separately by MAC0, and MAC1 needn't configure RF;
	 * Dual PHY mode: MAC0 use radio_a config 1st phy path_A,
	 * MAC1 use radio_b config 2nd PHY path_A.
	 * DMDP, MAC0 on G band, MAC1 on A band.
	 */
	if (rtlhal->macphymode == DUALMAC_DUALPHY) {
		if (rtlhal->current_bandtype == BAND_ON_2_4G &&
		    rtlhal->interfaceindex == 0) {
			/* MAC0 needs PHY1 load radio_b.txt. */
			if (rtl92du_phy_enable_anotherphy(hw, true)) {
				rtlphy->num_total_rfpath = 2;
				mac0_initradiob_first = true;
			} else {
				/* We think if MAC1 is ON,then radio_a.txt and
				 * radio_b.txt has been load.
				 */
				return rtstatus;
			}
		} else if (rtlhal->current_bandtype == BAND_ON_5G &&
			   rtlhal->interfaceindex == 1) {
			/* MAC1 needs PHY0 load radio_a.txt. */
			if (rtl92du_phy_enable_anotherphy(hw, false)) {
				rtlphy->num_total_rfpath = 2;
				mac1_initradioa_first = true;
			} else {
				/* We think if MAC0 is ON, then radio_a.txt and
				 * radio_b.txt has been load.
				 */
				return rtstatus;
			}
		} else if (rtlhal->interfaceindex == 1) {
			/* MAC0 enabled, only init radia B.   */
			true_bpath = true;
		}
	}

	for (rfpath = 0; rfpath < rtlphy->num_total_rfpath; rfpath++) {
		/* Mac1 use PHY0 write */
		if (mac1_initradioa_first) {
			if (rfpath == RF90_PATH_A) {
				rtlhal->during_mac1init_radioa = true;
				need_pwrdown_radioa = true;
			} else if (rfpath == RF90_PATH_B) {
				rtlhal->during_mac1init_radioa = false;
				mac1_initradioa_first = false;
				rfpath = RF90_PATH_A;
				true_bpath = true;
				rtlphy->num_total_rfpath = 1;
			}
		} else if (mac0_initradiob_first) {
			/* Mac0 use PHY1 write */
			if (rfpath == RF90_PATH_A)
				rtlhal->during_mac0init_radiob = false;
			if (rfpath == RF90_PATH_B) {
				rtlhal->during_mac0init_radiob = true;
				mac0_initradiob_first = false;
				need_pwrdown_radiob = true;
				rfpath = RF90_PATH_A;
				true_bpath = true;
				rtlphy->num_total_rfpath = 1;
			}
		}

		pphyreg = &rtlphy->phyreg_def[rfpath];

		switch (rfpath) {
		case RF90_PATH_A:
		case RF90_PATH_C:
			u4_regvalue = rtl_get_bbreg(hw, pphyreg->rfintfs,
						    BRFSI_RFENV);
			break;
		case RF90_PATH_B:
		case RF90_PATH_D:
			u4_regvalue = rtl_get_bbreg(hw, pphyreg->rfintfs,
						    BRFSI_RFENV << 16);
			break;
		}

		rtl_set_bbreg(hw, pphyreg->rfintfe, BRFSI_RFENV << 16, 0x1);
		udelay(1);
		rtl_set_bbreg(hw, pphyreg->rfintfo, BRFSI_RFENV, 0x1);
		udelay(1);

		/* Set bit number of Address and Data for RF register */
		rtl_set_bbreg(hw, pphyreg->rfhssi_para2,
			      B3WIREADDRESSLENGTH, 0x0);
		udelay(1);
		rtl_set_bbreg(hw, pphyreg->rfhssi_para2, B3WIREDATALENGTH, 0x0);
		udelay(1);

		switch (rfpath) {
		case RF90_PATH_A:
			if (true_bpath)
				rtstatus = rtl92du_phy_config_rf_with_headerfile(
						hw, radiob_txt,
						(enum radio_path)rfpath);
			else
				rtstatus = rtl92du_phy_config_rf_with_headerfile(
						hw, radioa_txt,
						(enum radio_path)rfpath);
			break;
		case RF90_PATH_B:
			rtstatus =
			    rtl92du_phy_config_rf_with_headerfile(hw, radiob_txt,
						(enum radio_path)rfpath);
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
		}

		switch (rfpath) {
		case RF90_PATH_A:
		case RF90_PATH_C:
			rtl_set_bbreg(hw, pphyreg->rfintfs, BRFSI_RFENV,
				      u4_regvalue);
			break;
		case RF90_PATH_B:
		case RF90_PATH_D:
			rtl_set_bbreg(hw, pphyreg->rfintfs, BRFSI_RFENV << 16,
				      u4_regvalue);
			break;
		}

		if (!rtstatus) {
			rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE,
				"Radio[%d] Fail!!\n", rfpath);
			return rtstatus;
		}
	}

	/* check MAC0 enable or not again, if enabled,
	 * not power down radio A.
	 * check MAC1 enable or not again, if enabled,
	 * not power down radio B.
	 */
	if (need_pwrdown_radioa)
		rtl92du_phy_powerdown_anotherphy(hw, false);
	else if (need_pwrdown_radiob)
		rtl92du_phy_powerdown_anotherphy(hw, true);
	rtl_dbg(rtlpriv, COMP_INIT, DBG_TRACE, "<---\n");

	return rtstatus;
}


/* rtl8192du/phy.c: rtl92du_phy_rf_config */
static bool rtl92du_phy_rf_config(struct ieee80211_hw *hw)
{
	return rtl92du_phy_rf6052_config(hw);
}


/* rtl8192du/phy.c: rtl92du_phy_set_bb_reg */
static void rtl92du_phy_set_bb_reg(struct ieee80211_hw *hw,
			    u32 regaddr, u32 bitmask, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	u32 originalvalue, bitshift;

	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"regaddr(%#x), bitmask(%#x), data(%#x)\n",
		regaddr, bitmask, data);

	if (rtlhal->during_mac1init_radioa)
		regaddr |= MAC1_ACCESS_PHY0;
	else if (rtlhal->during_mac0init_radiob)
		regaddr |= MAC0_ACCESS_PHY1;

	if (bitmask != MASKDWORD) {
		originalvalue = rtl_read_dword(rtlpriv, regaddr);
		bitshift = calculate_bit_shift(bitmask);
		data = (originalvalue & (~bitmask)) |
			((data << bitshift) & bitmask);
	}

	rtl_write_dword(rtlpriv, regaddr, data);
	rtl_dbg(rtlpriv, COMP_RF, DBG_TRACE,
		"regaddr(%#x), bitmask(%#x), data(%#x)\n",
		regaddr, bitmask, data);
}


/* rtl8192du/phy.c: rtl92du_phy_set_bb_reg_1byte */
static void rtl92du_phy_set_bb_reg_1byte(struct ieee80211_hw *hw,
					 u32 regaddr, u32 bitmask, u32 data)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u32 originalvalue, bitshift, offset;
	u8 value;

	/* BitMask only support bit0~bit7 or bit8~bit15, bit16~bit23,
	 * bit24~bit31, should be in 1 byte scale;
	 */
	bitshift = calculate_bit_shift(bitmask);
	offset = bitshift / 8;

	originalvalue = rtl_read_dword(rtlpriv, regaddr);
	data = (originalvalue & (~bitmask)) | ((data << bitshift) & bitmask);

	value = data >> (8 * offset);

	rtl_write_byte(rtlpriv, regaddr + offset, value);
}


/* rtl8192du/phy.c: rtl92du_phy_set_bw_mode */
static void rtl92du_phy_set_bw_mode(struct ieee80211_hw *hw,
			     enum nl80211_channel_type ch_type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	struct rtl_mac *mac = rtl_mac(rtlpriv);
	u8 reg_bw_opmode;
	u8 reg_prsr_rsc;

	if (rtlphy->set_bwmode_inprogress)
		return;

	if ((is_hal_stop(rtlhal)) || (RT_CANNOT_IO(hw))) {
		rtl_dbg(rtlpriv, COMP_ERR, DBG_WARNING,
			"FALSE driver sleep or unload\n");
		return;
	}

	rtlphy->set_bwmode_inprogress = true;

	rtl_dbg(rtlpriv, COMP_SCAN, DBG_TRACE, "Switch to %s bandwidth\n",
		rtlphy->current_chan_bw == HT_CHANNEL_WIDTH_20 ?
		"20MHz" : "40MHz");

	reg_bw_opmode = rtl_read_byte(rtlpriv, REG_BWOPMODE);
	reg_prsr_rsc = rtl_read_byte(rtlpriv, REG_RRSR + 2);

	switch (rtlphy->current_chan_bw) {
	case HT_CHANNEL_WIDTH_20:
		reg_bw_opmode |= BW_OPMODE_20MHZ;
		rtl_write_byte(rtlpriv, REG_BWOPMODE, reg_bw_opmode);
		break;
	case HT_CHANNEL_WIDTH_20_40:
		reg_bw_opmode &= ~BW_OPMODE_20MHZ;
		rtl_write_byte(rtlpriv, REG_BWOPMODE, reg_bw_opmode);

		reg_prsr_rsc = (reg_prsr_rsc & 0x90) |
			       (mac->cur_40_prime_sc << 5);
		rtl_write_byte(rtlpriv, REG_RRSR + 2, reg_prsr_rsc);
		break;
	default:
		pr_err("unknown bandwidth: %#X\n",
		       rtlphy->current_chan_bw);
		break;
	}

	switch (rtlphy->current_chan_bw) {
	case HT_CHANNEL_WIDTH_20:
		rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD, BRFMOD, 0x0);
		rtl_set_bbreg(hw, RFPGA1_RFMOD, BRFMOD, 0x0);
		/* SET BIT10 BIT11  for receive cck */
		rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER2, BIT(10) | BIT(11), 3);
		break;
	case HT_CHANNEL_WIDTH_20_40:
		rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD, BRFMOD, 0x1);
		rtl_set_bbreg(hw, RFPGA1_RFMOD, BRFMOD, 0x1);
		/* Set Control channel to upper or lower.
		 * These settings are required only for 40MHz
		 */
		if (rtlhal->current_bandtype == BAND_ON_2_4G)
			rtl_set_bbreg(hw, RCCK0_SYSTEM, BCCKSIDEBAND,
				      mac->cur_40_prime_sc >> 1);
		rtl_set_bbreg(hw, ROFDM1_LSTF, 0xC00, mac->cur_40_prime_sc);
		/* SET BIT10 BIT11  for receive cck */
		rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER2,
			      BIT(10) | BIT(11), 0);
		rtl_set_bbreg(hw, 0x818, BIT(26) | BIT(27),
			      mac->cur_40_prime_sc ==
			      HAL_PRIME_CHNL_OFFSET_LOWER ? 2 : 1);
		break;
	default:
		pr_err("unknown bandwidth: %#X\n",
		       rtlphy->current_chan_bw);
		break;
	}

	rtl92d_phy_rf6052_set_bandwidth(hw, rtlphy->current_chan_bw);

	rtlphy->set_bwmode_inprogress = false;
	rtl_dbg(rtlpriv, COMP_SCAN, DBG_TRACE, "<==\n");
}


/* rtl8192du/phy.c: rtl92du_phy_sw_chnl */
static u8 rtl92du_phy_sw_chnl(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u8 num_total_rfpath = rtlphy->num_total_rfpath;
	u8 channel = rtlphy->current_channel;
	u32 timeout = 1000, timecount = 0;
	u32 ret_value;
	u8 rfpath;

	if (rtlphy->sw_chnl_inprogress)
		return 0;
	if (rtlphy->set_bwmode_inprogress)
		return 0;

	if ((is_hal_stop(rtlhal)) || (RT_CANNOT_IO(hw))) {
		rtl_dbg(rtlpriv, COMP_CHAN, DBG_LOUD,
			"sw_chnl_inprogress false driver sleep or unload\n");
		return 0;
	}

	while (rtlphy->lck_inprogress && timecount < timeout) {
		mdelay(50);
		timecount += 50;
	}

	if (rtlhal->macphymode == SINGLEMAC_SINGLEPHY &&
	    rtlhal->bandset == BAND_ON_BOTH) {
		ret_value = rtl_get_bbreg(hw, RFPGA0_XAB_RFPARAMETER,
					  MASKDWORD);
		if (rtlphy->current_channel > 14 && !(ret_value & BIT(0)))
			rtl92du_phy_switch_wirelessband(hw, BAND_ON_5G);
		else if (rtlphy->current_channel <= 14 && (ret_value & BIT(0)))
			rtl92du_phy_switch_wirelessband(hw, BAND_ON_2_4G);
	}

	switch (rtlhal->current_bandtype) {
	case BAND_ON_5G:
		/* Get first channel error when change between
		 * 5G and 2.4G band.
		 */
		if (WARN_ONCE(channel <= 14, "rtl8192du: 5G but channel<=14\n"))
			return 0;
		break;
	case BAND_ON_2_4G:
		/* Get first channel error when change between
		 * 5G and 2.4G band.
		 */
		if (WARN_ONCE(channel > 14, "rtl8192du: 2G but channel>14\n"))
			return 0;
		break;
	default:
		WARN_ONCE(true, "rtl8192du: Invalid WirelessMode(%#x)!!\n",
			  rtlpriv->mac80211.mode);
		break;
	}

	rtlphy->sw_chnl_inprogress = true;

	rtl_dbg(rtlpriv, COMP_SCAN, DBG_TRACE,
		"switch to channel%d\n", rtlphy->current_channel);

	rtl92d_phy_set_txpower_level(hw, channel);

	for (rfpath = 0; rfpath < num_total_rfpath; rfpath++) {
		u32p_replace_bits(&rtlphy->rfreg_chnlval[rfpath],
				  channel, 0xff);

		if (rtlpriv->rtlhal.current_bandtype == BAND_ON_5G) {
			if (channel > 99)
				rtlphy->rfreg_chnlval[rfpath] |= (BIT(18));
			else
				rtlphy->rfreg_chnlval[rfpath] &= ~BIT(18);
			rtlphy->rfreg_chnlval[rfpath] |= (BIT(16) | BIT(8));
		} else {
			rtlphy->rfreg_chnlval[rfpath] &=
				~(BIT(8) | BIT(16) | BIT(18));
		}
		rtl_set_rfreg(hw, rfpath, RF_CHNLBW, RFREG_OFFSET_MASK,
			      rtlphy->rfreg_chnlval[rfpath]);

		_rtl92du_phy_reload_imr_setting(hw, channel, rfpath);
	}

	_rtl92du_phy_switch_rf_setting(hw, channel);

	/* do IQK when all parameters are ready */
	rtl92du_phy_reload_iqk_setting(hw, channel);

	rtl_dbg(rtlpriv, COMP_SCAN, DBG_TRACE, "<==\n");
	rtlphy->sw_chnl_inprogress = false;
	return 1;
}


/* rtl8192du/phy.c: rtl92du_phy_switch_wirelessband */
static void rtl92du_phy_switch_wirelessband(struct ieee80211_hw *hw, u8 band)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	u16 basic_rates;
	u32 reg_mac;
	u8 value8;

	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD, "==>\n");
	rtlhal->bandset = band;
	rtlhal->current_bandtype = band;
	if (IS_92D_SINGLEPHY(rtlhal->version))
		rtlhal->bandset = BAND_ON_BOTH;

	/* stop RX/Tx */
	_rtl92du_phy_stop_trx_before_changeband(hw);

	/* reconfig BB/RF according to wireless mode */
	if (rtlhal->current_bandtype == BAND_ON_2_4G)
		/* BB & RF Config */
		rtl_dbg(rtlpriv, COMP_CMD, DBG_DMESG, "====>2.4G\n");
	else
		/* 5G band */
		rtl_dbg(rtlpriv, COMP_CMD, DBG_DMESG, "====>5G\n");

	if (rtlhal->interfaceindex == 1)
		_rtl92du_phy_config_bb(hw, BASEBAND_CONFIG_AGC_TAB);

	rtl92du_update_bbrf_configuration(hw);

	basic_rates = RRSR_6M | RRSR_12M | RRSR_24M;
	if (rtlhal->current_bandtype == BAND_ON_2_4G)
		basic_rates |= RRSR_1M | RRSR_2M | RRSR_5_5M | RRSR_11M;
	rtlpriv->cfg->ops->set_hw_reg(hw, HW_VAR_BASIC_RATE,
				      (u8 *)&basic_rates);

	rtl92du_phy_set_bb_reg_1byte(hw, RFPGA0_RFMOD, BCCKEN | BOFDMEN, 0x3);

	/* 20M BW. */
	/* rtl_set_bbreg(hw, RFPGA0_ANALOGPARAMETER2, BIT(10), 1); */
	rtlhal->reloadtxpowerindex = true;

	reg_mac = rtlhal->interfaceindex == 0 ? REG_MAC0 : REG_MAC1;

	/* notice fw know band status  0x81[1]/0x53[1] = 0: 5G, 1: 2G */
	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		value8 = rtl_read_byte(rtlpriv,	reg_mac);
		value8 |= BIT(1);
		rtl_write_byte(rtlpriv, reg_mac, value8);
	} else {
		value8 = rtl_read_byte(rtlpriv, reg_mac);
		value8 &= ~BIT(1);
		rtl_write_byte(rtlpriv, reg_mac, value8);
	}
	mdelay(1);
	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD, "<==Switch Band OK\n");
}


/* rtl8192du/phy.c: rtl92du_update_bbrf_configuration */
static void rtl92du_update_bbrf_configuration(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtlpriv);
	struct rtl_hal *rtlhal = rtl_hal(rtlpriv);
	struct rtl_phy *rtlphy = &rtlpriv->phy;
	u8 rfpath, i;

	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD, "==>\n");
	/* r_select_5G for path_A/B 0 for 2.4G, 1 for 5G */
	if (rtlhal->current_bandtype == BAND_ON_2_4G) {
		/* r_select_5G for path_A/B, 0x878 */
		rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(0), 0x0);
		rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(15), 0x0);
		if (rtlhal->macphymode != DUALMAC_DUALPHY) {
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(16), 0x0);
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(31), 0x0);
		}

		/* rssi_table_select: index 0 for 2.4G. 1~3 for 5G, 0xc78 */
		rtl_set_bbreg(hw, ROFDM0_AGCRSSITABLE, BIT(6) | BIT(7), 0x0);

		/* fc_area  0xd2c */
		rtl_set_bbreg(hw, ROFDM1_CFOTRACKING, BIT(14) | BIT(13), 0x0);

		/* 5G LAN ON */
		rtl_set_bbreg(hw, 0xB30, 0x00F00000, 0xa);

		/* TX BB gain shift*1, Just for testchip, 0xc80, 0xc88 */
		rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE, MASKDWORD, 0x40000100);
		rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE, MASKDWORD, 0x40000100);
		if (rtlhal->macphymode == DUALMAC_DUALPHY) {
			rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW,
				      BIT(10) | BIT(6) | BIT(5),
				      ((rtlefuse->eeprom_c9 & BIT(3)) >> 3) |
				      (rtlefuse->eeprom_c9 & BIT(1)) |
				      ((rtlefuse->eeprom_cc & BIT(1)) << 4));
			rtl_set_bbreg(hw, RFPGA0_XA_RFINTERFACEOE,
				      BIT(10) | BIT(6) | BIT(5),
				      ((rtlefuse->eeprom_c9 & BIT(2)) >> 2) |
				      ((rtlefuse->eeprom_c9 & BIT(0)) << 1) |
				      ((rtlefuse->eeprom_cc & BIT(0)) << 5));
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(15), 0);

			rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0x01017038);
			rtl_set_bbreg(hw, RCONFIG_ANTA, MASKDWORD, 0x0f600000);
		} else {
			rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW,
				      BIT(26) | BIT(22) | BIT(21) | BIT(10) |
				      BIT(6) | BIT(5),
				      ((rtlefuse->eeprom_c9 & BIT(3)) >> 3) |
				      (rtlefuse->eeprom_c9 & BIT(1)) |
				      ((rtlefuse->eeprom_cc & BIT(1)) << 4) |
				      ((rtlefuse->eeprom_c9 & BIT(7)) << 9) |
				      ((rtlefuse->eeprom_c9 & BIT(5)) << 12) |
				      ((rtlefuse->eeprom_cc & BIT(3)) << 18));
			rtl_set_bbreg(hw, RFPGA0_XA_RFINTERFACEOE,
				      BIT(10) | BIT(6) | BIT(5),
				      ((rtlefuse->eeprom_c9 & BIT(2)) >> 2) |
				      ((rtlefuse->eeprom_c9 & BIT(0)) << 1) |
				      ((rtlefuse->eeprom_cc & BIT(0)) << 5));
			rtl_set_bbreg(hw, RFPGA0_XB_RFINTERFACEOE,
				      BIT(10) | BIT(6) | BIT(5),
				      ((rtlefuse->eeprom_c9 & BIT(6)) >> 6) |
				      ((rtlefuse->eeprom_c9 & BIT(4)) >> 3) |
				      ((rtlefuse->eeprom_cc & BIT(2)) << 3));
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER,
				      BIT(31) | BIT(15), 0);

			rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0x01017038);
			rtl_set_bbreg(hw, RPDP_ANTB, MASKDWORD, 0x01017038);
			rtl_set_bbreg(hw, RCONFIG_ANTA, MASKDWORD, 0x0f600000);
			rtl_set_bbreg(hw, RCONFIG_ANTB, MASKDWORD, 0x0f600000);
		}
		/* 1.5V_LDO */
	} else {
		/* r_select_5G for path_A/B */
		rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(0), 0x1);
		rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(15), 0x1);
		if (rtlhal->macphymode != DUALMAC_DUALPHY) {
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(16), 0x1);
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(31), 0x1);
		}

		/* rssi_table_select: index 0 for 2.4G. 1~3 for 5G */
		rtl_set_bbreg(hw, ROFDM0_AGCRSSITABLE, BIT(6) | BIT(7), 0x1);

		/* fc_area */
		rtl_set_bbreg(hw, ROFDM1_CFOTRACKING, BIT(14) | BIT(13), 0x1);

		/* 5G LAN ON */
		rtl_set_bbreg(hw, 0xB30, 0x00F00000, 0x0);

		/* TX BB gain shift, Just for testchip, 0xc80, 0xc88 */
		if (rtlefuse->internal_pa_5g[rtlhal->interfaceindex])
			rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE, MASKDWORD,
				      0x2d4000b5);
		else
			rtl_set_bbreg(hw, ROFDM0_XATXIQIMBALANCE, MASKDWORD,
				      0x20000080);

		if (rtlhal->macphymode != DUALMAC_DUALPHY) {
			if (rtlefuse->internal_pa_5g[1])
				rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE,
					      MASKDWORD, 0x2d4000b5);
			else
				rtl_set_bbreg(hw, ROFDM0_XBTXIQIMBALANCE,
					      MASKDWORD, 0x20000080);
		}

		rtl_set_bbreg(hw, 0xB30, BIT(27), 0);

		if (rtlhal->macphymode == DUALMAC_DUALPHY) {
			rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW,
				      BIT(10) | BIT(6) | BIT(5),
				      (rtlefuse->eeprom_cc & BIT(5)));
			rtl_set_bbreg(hw, RFPGA0_XA_RFINTERFACEOE, BIT(10),
				      ((rtlefuse->eeprom_cc & BIT(4)) >> 4));
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER, BIT(15),
				      (rtlefuse->eeprom_cc & BIT(4)) >> 4);

			rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0x01017098);
			rtl_set_bbreg(hw, RCONFIG_ANTA, MASKDWORD, 0x20000000);
		} else {
			rtl_set_bbreg(hw, RFPGA0_XAB_RFINTERFACESW,
				      BIT(26) | BIT(22) | BIT(21) | BIT(10) |
				      BIT(6) | BIT(5),
				      (rtlefuse->eeprom_cc & BIT(5)) |
				      ((rtlefuse->eeprom_cc & BIT(7)) << 14));
			rtl_set_bbreg(hw, RFPGA0_XA_RFINTERFACEOE, BIT(10),
				      ((rtlefuse->eeprom_cc & BIT(4)) >> 4));
			rtl_set_bbreg(hw, RFPGA0_XB_RFINTERFACEOE, BIT(10),
				      ((rtlefuse->eeprom_cc & BIT(6)) >> 6));
			rtl_set_bbreg(hw, RFPGA0_XAB_RFPARAMETER,
				      BIT(31) | BIT(15),
				      ((rtlefuse->eeprom_cc & BIT(4)) >> 4) |
				      ((rtlefuse->eeprom_cc & BIT(6)) << 10));

			rtl_set_bbreg(hw, RPDP_ANTA, MASKDWORD, 0x01017098);
			rtl_set_bbreg(hw, RPDP_ANTB, MASKDWORD, 0x01017098);
			rtl_set_bbreg(hw, RCONFIG_ANTA, MASKDWORD, 0x20000000);
			rtl_set_bbreg(hw, RCONFIG_ANTB, MASKDWORD, 0x20000000);
		}
	}

	/* update IQK related settings */
	rtl_set_bbreg(hw, ROFDM0_XARXIQIMBALANCE, MASKDWORD, 0x40000100);
	rtl_set_bbreg(hw, ROFDM0_XBRXIQIMBALANCE, MASKDWORD, 0x40000100);
	rtl_set_bbreg(hw, ROFDM0_XCTXAFE, 0xF0000000, 0x00);
	rtl_set_bbreg(hw, ROFDM0_ECCATHRESHOLD, BIT(30) | BIT(28) |
		      BIT(26) | BIT(24), 0x00);
	rtl_set_bbreg(hw, ROFDM0_XDTXAFE, 0xF0000000, 0x00);
	rtl_set_bbreg(hw, ROFDM0_RXIQEXTANTA, 0xF0000000, 0x00);
	rtl_set_bbreg(hw, ROFDM0_AGCRSSITABLE, 0x0000F000, 0x00);

	/* Update RF */
	for (rfpath = RF90_PATH_A; rfpath < rtlphy->num_total_rfpath;
	     rfpath++) {
		if (rtlhal->current_bandtype == BAND_ON_2_4G) {
			/* MOD_AG for RF path_A 0x18 BIT8,BIT16 */
			rtl_set_rfreg(hw, rfpath, RF_CHNLBW, BIT(8) | BIT(16) |
				      BIT(18) | 0xff, 1);

			/* RF0x0b[16:14] =3b'111 */
			rtl_set_rfreg(hw, (enum radio_path)rfpath, 0x0B,
				      0x1c000, 0x07);
		} else {
			/* MOD_AG for RF path_A 0x18 BIT8,BIT16 */
			rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW, RFREG_OFFSET_MASK,
				      0x97524);
		}

		/* Set right channel on RF reg0x18 for another mac. */
		if (rtlhal->interfaceindex == 0 && rtlhal->bandset == BAND_ON_2_4G) {
			/* Set MAC1 default channel if MAC1 not up. */
			if (!(rtl_read_byte(rtlpriv, REG_MAC1) & MAC1_ON)) {
				rtl92du_phy_enable_anotherphy(hw, true);
				rtlhal->during_mac0init_radiob = true;
				rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW,
					      RFREG_OFFSET_MASK, 0x97524);
				rtl92du_phy_powerdown_anotherphy(hw, true);
			}
		} else if (rtlhal->interfaceindex == 1 && rtlhal->bandset == BAND_ON_5G) {
			/* Set MAC0 default channel */
			if (!(rtl_read_byte(rtlpriv, REG_MAC0) & MAC0_ON)) {
				rtl92du_phy_enable_anotherphy(hw, false);
				rtlhal->during_mac1init_radioa = true;
				rtl_set_rfreg(hw, RF90_PATH_A, RF_CHNLBW,
					      RFREG_OFFSET_MASK, 0x87401);
				rtl92du_phy_powerdown_anotherphy(hw, false);
			}
		}
	}

	/* Update for all band. */
	/* DMDP */
	if (rtlphy->rf_type == RF_1T1R) {
		/* Use antenna 0, 0xc04, 0xd04 */
		rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE, MASKBYTE0, 0x11);
		rtl_set_bbreg(hw, ROFDM1_TRXPATHENABLE, BDWORD, 0x1);

		/* enable ad/da clock1 for dual-phy reg0x888 */
		if (rtlhal->interfaceindex == 0) {
			rtl_set_bbreg(hw, RFPGA0_ADDALLOCKEN, BIT(12) |
				      BIT(13), 0x3);
		} else if (rtl92du_phy_enable_anotherphy(hw, false)) {
			rtlhal->during_mac1init_radioa = true;
			rtl_set_bbreg(hw, RFPGA0_ADDALLOCKEN,
				      BIT(12) | BIT(13), 0x3);
			rtl92du_phy_powerdown_anotherphy(hw, false);
		}

		rtl_set_bbreg(hw, ROFDM1_LSTF, BIT(19) | BIT(20), 0x0);
	} else {
		/* Single PHY */
		/* Use antenna 0 & 1, 0xc04, 0xd04 */
		rtl_set_bbreg(hw, ROFDM0_TRXPATHENABLE, MASKBYTE0, 0x33);
		rtl_set_bbreg(hw, ROFDM1_TRXPATHENABLE, BDWORD, 0x3);
		/* disable ad/da clock1,0x888 */
		rtl_set_bbreg(hw, RFPGA0_ADDALLOCKEN, BIT(12) | BIT(13), 0);

		rtl_set_bbreg(hw, ROFDM1_LSTF, BIT(19) | BIT(20), 0x1);
	}

	for (rfpath = RF90_PATH_A; rfpath < rtlphy->num_total_rfpath;
	     rfpath++) {
		rtlphy->rfreg_chnlval[rfpath] = rtl_get_rfreg(hw, rfpath,
							      RF_CHNLBW,
							      RFREG_OFFSET_MASK);
		rtlphy->reg_rf3c[rfpath] = rtl_get_rfreg(hw, rfpath, 0x3C,
							 RFREG_OFFSET_MASK);
	}

	for (i = 0; i < 2; i++)
		rtl_dbg(rtlpriv, COMP_RF, DBG_LOUD, "RF 0x18 = 0x%x\n",
			rtlphy->rfreg_chnlval[i]);

	rtl_dbg(rtlpriv, COMP_INIT, DBG_LOUD, "<==\n");
}
