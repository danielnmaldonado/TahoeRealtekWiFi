/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

/* Copyright(c)2018-2019 Realtek Corporation. Pinned phy.c RSSI math. */

typedef int8_t s8;typedef uint8_t u8;typedef uint32_t u32;typedef uint64_t u64;

#define FRAC_BITS 3

static const u32 db_invert_table[12][8] = {
	{10,		13,		16,		20,
	 25,		32,		40,		50},
	{64,		80,		101,		128,
	 160,		201,		256,		318},
	{401,		505,		635,		800,
	 1007,		1268,		1596,		2010},
	{316,		398,		501,		631,
	 794,		1000,		1259,		1585},
	{1995,		2512,		3162,		3981,
	 5012,		6310,		7943,		10000},
	{12589,		15849,		19953,		25119,
	 31623,		39811,		50119,		63098},
	{79433,		100000,		125893,		158489,
	 199526,	251189,		316228,		398107},
	{501187,	630957,		794328,		1000000,
	 1258925,	1584893,	1995262,	2511886},
	{3162278,	3981072,	5011872,	6309573,
	 7943282,	1000000,	12589254,	15848932},
	{19952623,	25118864,	31622777,	39810717,
	 50118723,	63095734,	79432823,	100000000},
	{125892541,	158489319,	199526232,	251188643,
	 316227766,	398107171,	501187234,	630957345},
	{794328235,	1000000000,	1258925412,	1584893192,
	 1995262315,	2511886432U,	3162277660U,	3981071706U}
};

static u8 rtw_phy_power_2_db(s8 power)
{
	if (power <= -100 || power >= 20)
		return 0;
	else if (power >= 0)
		return 100;
	else
		return 100 + power;
}

static u64 rtw_phy_db_2_linear(u8 power_db)
{
	u8 i, j;
	u64 linear;

	if (power_db > 96)
		power_db = 96;
	else if (power_db < 1)
		return 1;

	/* 1dB ~ 96dB */
	i = (power_db - 1) >> 3;
	j = (power_db - 1) - (i << 3);

	linear = db_invert_table[i][j];
	linear = i > 2 ? linear << FRAC_BITS : linear;

	return linear;
}

static u8 rtw_phy_linear_2_db(u64 linear)
{
	u8 i;
	u8 j;
	u32 dB;

	for (i = 0; i < 12; i++) {
		for (j = 0; j < 8; j++) {
			if (i <= 2 && (linear << FRAC_BITS) <= db_invert_table[i][j])
				goto cnt;
			else if (i > 2 && linear <= db_invert_table[i][j])
				goto cnt;
		}
	}

	return 96; /* maximum 96 dB */

cnt:
	if (j == 0 && i == 0)
		goto end;

	if (j == 0) {
		if (i != 3) {
			if (db_invert_table[i][0] - linear >
			    linear - db_invert_table[i - 1][7]) {
				i = i - 1;
				j = 7;
			}
		} else {
			if (db_invert_table[3][0] - linear >
			    linear - db_invert_table[2][7]) {
				i = 2;
				j = 7;
			}
		}
	} else {
		if (db_invert_table[i][j] - linear >
		    linear - db_invert_table[i][j - 1]) {
			j = j - 1;
		}
	}
end:
	dB = (i << 3) + j + 1;

	return dB;
}

static u8 rtw_phy_rf_power_2_rssi(s8 *rf_power, u8 path_num)
{
	s8 power;
	u8 power_db;
	u64 linear;
	u64 sum = 0;
	u8 path;

	for (path = 0; path < path_num; path++) {
		power = rf_power[path];
		power_db = rtw_phy_power_2_db(power);
		linear = rtw_phy_db_2_linear(power_db);
		sum += linear;
	}

	sum = (sum + (1 << (FRAC_BITS - 1))) >> FRAC_BITS;
	switch (path_num) {
	case 2:
		sum >>= 1;
		break;
	case 3:
		sum = ((sum) + ((sum) << 1) + ((sum) << 3)) >> 5;
		break;
	case 4:
		sum >>= 2;
		break;
	default:
		break;
	}

	return rtw_phy_linear_2_db(sum);
}
