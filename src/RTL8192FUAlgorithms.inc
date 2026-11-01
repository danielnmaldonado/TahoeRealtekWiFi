/* SPDX-License-Identifier: GPL-2.0-only */
/* Derived from Linux rtl8xxxu at f47d223627f80cb7a1eb14c585fe6cb35de5e984.
 * Copyright(c)2023 Bitterblue Smith,2014-2017 Jes Sorensen,
 * 2007-2011 Realtek Corporation. Retain RTL8192FU-LICENSE.txt and manifest. */
static int rtl8xxxu_flush_fifo(struct rtw_r92fu *priv);
static void rtl8xxxu_firmware_self_reset(struct rtw_r92fu *priv);
static u32 rtl8xxxu_read_rfreg(struct rtw_r92fu *priv,
			enum rtl8xxxu_rfpath path, u8 reg);
static int rtl8xxxu_write_rfreg(struct rtw_r92fu *priv,
			 enum rtl8xxxu_rfpath path, u8 reg, u32 data);
static int rtl8xxxu_write_rfreg_mask(struct rtw_r92fu *priv,
			      enum rtl8xxxu_rfpath path, u8 reg,
			      u32 mask, u32 val);
static int rtl8xxxu_init_phy_regs(struct rtw_r92fu *priv,
			   const struct rtl8xxxu_reg32val *array);
static int rtl8xxxu_init_rf_regs(struct rtw_r92fu *priv,
				 const struct rtl8xxxu_rfregval *array,
				 enum rtl8xxxu_rfpath path);
static int rtl8xxxu_init_phy_rf(struct rtw_r92fu *priv,
			 const struct rtl8xxxu_rfregval *table,
			 enum rtl8xxxu_rfpath path);
static void rtl8xxxu_save_regs(struct rtw_r92fu *priv, const u32 *regs,
			u32 *backup, int count);
static void rtl8xxxu_restore_regs(struct rtw_r92fu *priv, const u32 *regs,
			   u32 *backup, int count);
static void
rtl8xxxu_save_mac_regs(struct rtw_r92fu *priv, const u32 *reg, u32 *backup);
static void rtl8xxxu_restore_mac_regs(struct rtw_r92fu *priv,
			       const u32 *reg, u32 *backup);
static void rtl8xxxu_fill_iqk_matrix_a(struct rtw_r92fu *priv, bool iqk_ok,
				int result[][8], int candidate, bool tx_only);
static void rtl8xxxu_fill_iqk_matrix_b(struct rtw_r92fu *priv, bool iqk_ok,
				int result[][8], int candidate, bool tx_only);
static bool rtl8xxxu_gen2_simularity_compare(struct rtw_r92fu *priv,
				      int result[][8], int c1, int c2);
static int rtl8xxxu_auto_llt_table(struct rtw_r92fu *priv);
static void rtl8xxxu_init_queue_reserved_page(struct rtw_r92fu *priv);
static int rtl8xxxu_init_queue_priority(struct rtw_r92fu *priv);
static void
rtl8xxxu_set_retry(struct rtw_r92fu *priv, u16 short_retry, u16 long_retry);
static void
rtl8xxxu_set_spec_sifs(struct rtw_r92fu *priv, u16 cck, u16 ofdm);
static int
rtl8xxxu_init_mac(struct rtw_r92fu *priv);
static int rtl8xxxu_init_phy_bb(struct rtw_r92fu *priv);
static void rtl8xxxu_gen2_usb_quirks(struct rtw_r92fu *priv);
static void rtl8xxxu_init_burst(struct rtw_r92fu *priv);
static void rtl8xxxu_request_hw_feature(struct rtw_r92fu *priv);
static int rtl8xxxu_dump_hw_feature(struct rtw_r92fu *priv);
static int rtl8xxxu_start_firmware(struct rtw_r92fu *priv);
static int rtl8xxxu_download_firmware(struct rtw_r92fu *priv);
static int rtl8xxxu_init_device(struct ieee80211_hw *hw);
static void
rtl8192f_set_tx_power(struct rtw_r92fu *priv, int channel, bool ht40);
static void rtl8192fu_power_off(struct rtw_r92fu *priv);
static void rtl8192f_revise_cck_tx_psf(struct rtw_r92fu *priv, u8 channel);
static void rtl8192fu_config_kfree(struct rtw_r92fu *priv, u8 channel);
static void rtl8192fu_config_channel(struct ieee80211_hw *hw);
static void rtl8192fu_init_aggregation(struct rtw_r92fu *priv);
static void rtl8192fu_init_phy_bb(struct rtw_r92fu *priv);
static int rtl8192fu_init_phy_rf(struct rtw_r92fu *priv);
static void rtl8192f_phy_lc_calibrate(struct rtw_r92fu *priv);
static int rtl8192fu_iqk_path_a(struct rtw_r92fu *priv);
static int rtl8192fu_rx_iqk_path_a(struct rtw_r92fu *priv);
static int rtl8192fu_iqk_path_b(struct rtw_r92fu *priv);
static int rtl8192fu_rx_iqk_path_b(struct rtw_r92fu *priv);
static void rtl8192fu_phy_iqcalibrate(struct rtw_r92fu *priv,
				      int result[][8], int t);
static void rtl8192fu_phy_iq_calibrate(struct rtw_r92fu *priv);
static void rtl8192fu_disabled_to_emu(struct rtw_r92fu *priv);
static int rtl8192fu_emu_to_active(struct rtw_r92fu *priv);
static int rtl8192fu_active_to_emu(struct rtw_r92fu *priv);
static int rtl8192fu_emu_to_disabled(struct rtw_r92fu *priv);
static int rtl8192fu_active_to_lps(struct rtw_r92fu *priv);
static int rtl8192fu_power_on(struct rtw_r92fu *priv);
static void rtl8192f_reset_8051(struct rtw_r92fu *priv);
static void rtl8192f_enable_rf(struct rtw_r92fu *priv);
static void rtl8192f_disable_rf(struct rtw_r92fu *priv);
static void rtl8192f_usb_quirks(struct rtw_r92fu *priv);
static void rtl8192f_set_crystal_cap(struct rtw_r92fu *priv, u8 crystal_cap);
static void rtl8188f_phy_lc_calibrate(struct rtw_r92fu *priv);
static void rtl8188f_channel_to_group(int channel, int *group, int *cck_group);

static int rtl8xxxu_flush_fifo(struct rtw_r92fu *priv)
{
	
	u32 val32;
	int retry, retval;

	rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);

	val32 = rtl8xxxu_read32(priv, REG_RXPKT_NUM);
	val32 |= RXPKT_NUM_RW_RELEASE_EN;
	rtl8xxxu_write32(priv, REG_RXPKT_NUM, val32);

	retry = 100;
	retval = -EBUSY;

	do {
		val32 = rtl8xxxu_read32(priv, REG_RXPKT_NUM);
		if (val32 & RXPKT_NUM_RXDMA_IDLE) {
			retval = 0;
			break;
		}
	} while (retry--);

	rtl8xxxu_write16(priv, REG_RQPN_NPQ, 0);
	rtl8xxxu_write32(priv, REG_RQPN, 0x80000000);
	mdelay(2);

	if (!retry)
		(void)0;

	return retval;
}

static void rtl8xxxu_firmware_self_reset(struct rtw_r92fu *priv)
{
	u16 val16;
	int i = 100;

	
	rtl8xxxu_write8(priv, REG_HMTFR + 3, 0x20);

	for (i = 100; i > 0; i--) {
		val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);

		if (!(val16 & SYS_FUNC_CPU_ENABLE)) {
			(void)0;
			break;
		}
		udelay(50);
	}

	if (!i) {
		
		val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
		val16 &= ~SYS_FUNC_CPU_ENABLE;
		rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);
	}
}

static u32 rtl8xxxu_read_rfreg(struct rtw_r92fu *priv,
			enum rtl8xxxu_rfpath path, u8 reg)
{
	u32 hssia, val32, retval;

	hssia = rtl8xxxu_read32(priv, REG_FPGA0_XA_HSSI_PARM2);
	if (path != RF_A)
		val32 = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].hssiparm2);
	else
		val32 = hssia;

	val32 &= ~FPGA0_HSSI_PARM2_ADDR_MASK;
	val32 |= (reg << FPGA0_HSSI_PARM2_ADDR_SHIFT);
	val32 |= FPGA0_HSSI_PARM2_EDGE_READ;
	hssia &= ~FPGA0_HSSI_PARM2_EDGE_READ;
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM2, hssia);

	udelay(10);

	rtl8xxxu_write32(priv, rtl8xxxu_rfregs[path].hssiparm2, val32);
	udelay(100);

	hssia |= FPGA0_HSSI_PARM2_EDGE_READ;
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM2, hssia);
	udelay(10);

	val32 = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].hssiparm1);
	if (val32 & FPGA0_HSSI_PARM1_PI)
		retval = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].hspiread);
	else
		retval = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].lssiread);

	retval &= 0xfffff;

	
	return retval;
}

static int rtl8xxxu_write_rfreg(struct rtw_r92fu *priv,
			 enum rtl8xxxu_rfpath path, u8 reg, u32 data)
{
	int ret, retval;
	u32 dataaddr;

	

	data &= FPGA0_LSSI_PARM_DATA_MASK;
	dataaddr = (reg << FPGA0_LSSI_PARM_ADDR_SHIFT) | data;

	{}

	
	ret = rtl8xxxu_write32(priv, rtl8xxxu_rfregs[path].lssiparm, dataaddr);
	if (ret != sizeof(dataaddr))
		retval = -EIO;
	else
		retval = 0;

	udelay(1);

	{}

	return retval;
}

static int rtl8xxxu_write_rfreg_mask(struct rtw_r92fu *priv,
			      enum rtl8xxxu_rfpath path, u8 reg,
			      u32 mask, u32 val)
{
	u32 orig, new, shift;

	shift = __ffs(mask);

	orig = rtl8xxxu_read_rfreg(priv, path, reg);
	new = (orig & ~mask) | ((val << shift) & mask);
	return rtl8xxxu_write_rfreg(priv, path, reg, new);
}

static int rtl8xxxu_init_phy_regs(struct rtw_r92fu *priv,
			   const struct rtl8xxxu_reg32val *array)
{
	int i, ret;
	u16 reg;
	u32 val;

	for (i = 0; ; i++) {
		reg = array[i].reg;
		val = array[i].val;

		if (reg == 0xffff && val == 0xffffffff)
			break;

		ret = rtl8xxxu_write32(priv, reg, val);
		if (ret != sizeof(val)) {
			(void)0;
			return -EAGAIN;
		}
		udelay(1);
	}

	return 0;
}

static int rtl8xxxu_init_rf_regs(struct rtw_r92fu *priv,
				 const struct rtl8xxxu_rfregval *array,
				 enum rtl8xxxu_rfpath path)
{
	int i, ret;
	u8 reg;
	u32 val;

	for (i = 0; ; i++) {
		reg = array[i].reg;
		val = array[i].val;

		if (reg == 0xff && val == 0xffffffff)
			break;

		switch (reg) {
		case 0xfe:
			msleep(50);
			continue;
		case 0xfd:
			mdelay(5);
			continue;
		case 0xfc:
			mdelay(1);
			continue;
		case 0xfb:
			udelay(50);
			continue;
		case 0xfa:
			udelay(5);
			continue;
		case 0xf9:
			udelay(1);
			continue;
		}

		ret = rtl8xxxu_write_rfreg(priv, path, reg, val);
		if (ret) {
			(void)0;
			return -EAGAIN;
		}
		udelay(1);
	}

	return 0;
}

static int rtl8xxxu_init_phy_rf(struct rtw_r92fu *priv,
			 const struct rtl8xxxu_rfregval *table,
			 enum rtl8xxxu_rfpath path)
{
	u32 val32;
	u16 val16, rfsi_rfenv;
	u16 reg_sw_ctrl, reg_int_oe, reg_hssi_parm2;

	switch (path) {
	case RF_A:
		reg_sw_ctrl = REG_FPGA0_XA_RF_SW_CTRL;
		reg_int_oe = REG_FPGA0_XA_RF_INT_OE;
		reg_hssi_parm2 = REG_FPGA0_XA_HSSI_PARM2;
		break;
	case RF_B:
		reg_sw_ctrl = REG_FPGA0_XB_RF_SW_CTRL;
		reg_int_oe = REG_FPGA0_XB_RF_INT_OE;
		reg_hssi_parm2 = REG_FPGA0_XB_HSSI_PARM2;
		break;
	default:
		(void)0;
		return -EINVAL;
	}
	
	rfsi_rfenv = rtl8xxxu_read16(priv, reg_sw_ctrl);
	rfsi_rfenv &= FPGA0_RF_RFENV;

	
	val32 = rtl8xxxu_read32(priv, reg_int_oe);
	val32 |= BIT(20);	
	rtl8xxxu_write32(priv, reg_int_oe, val32);
	udelay(1);

	val32 = rtl8xxxu_read32(priv, reg_int_oe);
	val32 |= BIT(4);
	rtl8xxxu_write32(priv, reg_int_oe, val32);
	udelay(1);

	
	val32 = rtl8xxxu_read32(priv, reg_hssi_parm2);
	val32 &= ~FPGA0_HSSI_3WIRE_ADDR_LEN;
	rtl8xxxu_write32(priv, reg_hssi_parm2, val32);
	udelay(1);

	val32 = rtl8xxxu_read32(priv, reg_hssi_parm2);
	val32 &= ~FPGA0_HSSI_3WIRE_DATA_LEN;
	rtl8xxxu_write32(priv, reg_hssi_parm2, val32);
	udelay(1);

	rtl8xxxu_init_rf_regs(priv, table, path);

	
	val16 = rtl8xxxu_read16(priv, reg_sw_ctrl);
	val16 &= ~FPGA0_RF_RFENV;
	val16 |= rfsi_rfenv;
	rtl8xxxu_write16(priv, reg_sw_ctrl, val16);

	return 0;
}

static void rtl8xxxu_save_regs(struct rtw_r92fu *priv, const u32 *regs,
			u32 *backup, int count)
{
	int i;

	for (i = 0; i < count; i++)
		backup[i] = rtl8xxxu_read32(priv, regs[i]);
}

static void rtl8xxxu_restore_regs(struct rtw_r92fu *priv, const u32 *regs,
			   u32 *backup, int count)
{
	int i;

	for (i = 0; i < count; i++)
		rtl8xxxu_write32(priv, regs[i], backup[i]);
}

static void
rtl8xxxu_save_mac_regs(struct rtw_r92fu *priv, const u32 *reg, u32 *backup)
{
	int i;

	for (i = 0; i < (RTL8XXXU_MAC_REGS - 1); i++)
		backup[i] = rtl8xxxu_read8(priv, reg[i]);

	backup[i] = rtl8xxxu_read32(priv, reg[i]);
}

static void rtl8xxxu_restore_mac_regs(struct rtw_r92fu *priv,
			       const u32 *reg, u32 *backup)
{
	int i;

	for (i = 0; i < (RTL8XXXU_MAC_REGS - 1); i++)
		rtl8xxxu_write8(priv, reg[i], backup[i]);

	rtl8xxxu_write32(priv, reg[i], backup[i]);
}

static void rtl8xxxu_fill_iqk_matrix_a(struct rtw_r92fu *priv, bool iqk_ok,
				int result[][8], int candidate, bool tx_only)
{
	u32 oldval, x, tx0_a, reg;
	int y, tx0_c;
	u32 val32;

	if (!iqk_ok)
		return;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_TX_IQ_IMBALANCE);
	oldval = val32 >> 22;

	x = result[candidate][0];
	if ((x & 0x00000200) != 0)
		x = x | 0xfffffc00;
	tx0_a = (x * oldval) >> 8;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_TX_IQ_IMBALANCE);
	val32 &= ~0x3ff;
	val32 |= tx0_a;
	rtl8xxxu_write32(priv, REG_OFDM0_XA_TX_IQ_IMBALANCE, val32);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_ENERGY_CCA_THRES);
	val32 &= ~BIT(31);
	if ((x * oldval >> 7) & 0x1)
		val32 |= BIT(31);
	rtl8xxxu_write32(priv, REG_OFDM0_ENERGY_CCA_THRES, val32);

	y = result[candidate][1];
	if ((y & 0x00000200) != 0)
		y = y | 0xfffffc00;
	tx0_c = (y * oldval) >> 8;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XC_TX_AFE);
	val32 &= ~0xf0000000;
	val32 |= ((u32)((tx0_c & 0x3c0) >> 6) << 28);
	rtl8xxxu_write32(priv, REG_OFDM0_XC_TX_AFE, val32);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_TX_IQ_IMBALANCE);
	val32 &= ~0x003f0000;
	val32 |= ((tx0_c & 0x3f) << 16);
	rtl8xxxu_write32(priv, REG_OFDM0_XA_TX_IQ_IMBALANCE, val32);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_ENERGY_CCA_THRES);
	val32 &= ~BIT(29);
	if ((y * oldval >> 7) & 0x1)
		val32 |= BIT(29);
	rtl8xxxu_write32(priv, REG_OFDM0_ENERGY_CCA_THRES, val32);

	if (tx_only) {
		(void)0;
		return;
	}

	reg = result[candidate][2];

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_RX_IQ_IMBALANCE);
	val32 &= ~0x3ff;
	val32 |= (reg & 0x3ff);
	rtl8xxxu_write32(priv, REG_OFDM0_XA_RX_IQ_IMBALANCE, val32);

	reg = result[candidate][3] & 0x3F;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_RX_IQ_IMBALANCE);
	val32 &= ~0xfc00;
	val32 |= ((reg << 10) & 0xfc00);
	rtl8xxxu_write32(priv, REG_OFDM0_XA_RX_IQ_IMBALANCE, val32);

	reg = (result[candidate][3] >> 6) & 0xF;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_RX_IQ_EXT_ANTA);
	val32 &= ~0xf0000000;
	val32 |= (reg << 28);
	rtl8xxxu_write32(priv, REG_OFDM0_RX_IQ_EXT_ANTA, val32);
}

static void rtl8xxxu_fill_iqk_matrix_b(struct rtw_r92fu *priv, bool iqk_ok,
				int result[][8], int candidate, bool tx_only)
{
	u32 oldval, x, tx1_a, reg;
	int y, tx1_c;
	u32 val32;

	if (!iqk_ok)
		return;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XB_TX_IQ_IMBALANCE);
	oldval = val32 >> 22;

	x = result[candidate][4];
	if ((x & 0x00000200) != 0)
		x = x | 0xfffffc00;
	tx1_a = (x * oldval) >> 8;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XB_TX_IQ_IMBALANCE);
	val32 &= ~0x3ff;
	val32 |= tx1_a;
	rtl8xxxu_write32(priv, REG_OFDM0_XB_TX_IQ_IMBALANCE, val32);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_ENERGY_CCA_THRES);
	val32 &= ~BIT(27);
	if ((x * oldval >> 7) & 0x1)
		val32 |= BIT(27);
	rtl8xxxu_write32(priv, REG_OFDM0_ENERGY_CCA_THRES, val32);

	y = result[candidate][5];
	if ((y & 0x00000200) != 0)
		y = y | 0xfffffc00;
	tx1_c = (y * oldval) >> 8;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XD_TX_AFE);
	val32 &= ~0xf0000000;
	val32 |= ((u32)((tx1_c & 0x3c0) >> 6) << 28);
	rtl8xxxu_write32(priv, REG_OFDM0_XD_TX_AFE, val32);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XB_TX_IQ_IMBALANCE);
	val32 &= ~0x003f0000;
	val32 |= ((tx1_c & 0x3f) << 16);
	rtl8xxxu_write32(priv, REG_OFDM0_XB_TX_IQ_IMBALANCE, val32);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_ENERGY_CCA_THRES);
	val32 &= ~BIT(25);
	if ((y * oldval >> 7) & 0x1)
		val32 |= BIT(25);
	rtl8xxxu_write32(priv, REG_OFDM0_ENERGY_CCA_THRES, val32);

	if (tx_only) {
		(void)0;
		return;
	}

	reg = result[candidate][6];

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XB_RX_IQ_IMBALANCE);
	val32 &= ~0x3ff;
	val32 |= (reg & 0x3ff);
	rtl8xxxu_write32(priv, REG_OFDM0_XB_RX_IQ_IMBALANCE, val32);

	reg = result[candidate][7] & 0x3f;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XB_RX_IQ_IMBALANCE);
	val32 &= ~0xfc00;
	val32 |= ((reg << 10) & 0xfc00);
	rtl8xxxu_write32(priv, REG_OFDM0_XB_RX_IQ_IMBALANCE, val32);

	reg = (result[candidate][7] >> 6) & 0xf;

	{
		rtl8xxxu_write32_mask(priv, REG_RXIQB_EXT, 0x000000f0, reg);
	}
}

static bool rtl8xxxu_gen2_simularity_compare(struct rtw_r92fu *priv,
				      int result[][8], int c1, int c2)
{
	u32 i, j, diff, simubitmap, bound = 0;
	int candidate[2] = {-1, -1};	
	int tmp1, tmp2;
	bool retval = true;

	if (priv->tx_paths > 1)
		bound = 8;
	else
		bound = 4;

	simubitmap = 0;

	for (i = 0; i < bound; i++) {
		if (i & 1) {
			if ((result[c1][i] & 0x00000200))
				tmp1 = result[c1][i] | 0xfffffc00;
			else
				tmp1 = result[c1][i];

			if ((result[c2][i]& 0x00000200))
				tmp2 = result[c2][i] | 0xfffffc00;
			else
				tmp2 = result[c2][i];
		} else {
			tmp1 = result[c1][i];
			tmp2 = result[c2][i];
		}

		diff = (tmp1 > tmp2) ? (tmp1 - tmp2) : (tmp2 - tmp1);

		if (diff > MAX_TOLERANCE) {
			if ((i == 2 || i == 6) && !simubitmap) {
				if (result[c1][i] + result[c1][i + 1] == 0)
					candidate[(i / 4)] = c2;
				else if (result[c2][i] + result[c2][i + 1] == 0)
					candidate[(i / 4)] = c1;
				else
					simubitmap = simubitmap | (1 << i);
			} else {
				simubitmap = simubitmap | (1 << i);
			}
		}
	}

	if (simubitmap == 0) {
		for (i = 0; i < (bound / 4); i++) {
			if (candidate[i] >= 0) {
				for (j = i * 4; j < (i + 1) * 4 - 2; j++)
					result[3][j] = result[candidate[i]][j];
				retval = false;
			}
		}
		return retval;
	} else {
		if (!(simubitmap & 0x03)) {
			
			for (i = 0; i < 2; i++)
				result[3][i] = result[c1][i];
		}

		if (!(simubitmap & 0x0c)) {
			
			for (i = 2; i < 4; i++)
				result[3][i] = result[c1][i];
		}

		if (!(simubitmap & 0x30) && priv->tx_paths > 1) {
			
			for (i = 4; i < 6; i++)
				result[3][i] = result[c1][i];
		}

		if (!(simubitmap & 0xc0) && priv->tx_paths > 1) {
			
			for (i = 6; i < 8; i++)
				result[3][i] = result[c1][i];
		}
	}

	return false;
}

static int rtl8xxxu_auto_llt_table(struct rtw_r92fu *priv)
{
	u32 val32;
	int ret = 0;
	int i;

	val32 = rtl8xxxu_read32(priv, REG_AUTO_LLT);
	val32 |= AUTO_LLT_INIT_LLT;
	rtl8xxxu_write32(priv, REG_AUTO_LLT, val32);

	for (i = 500; i; i--) {
		val32 = rtl8xxxu_read32(priv, REG_AUTO_LLT);
		if (!(val32 & AUTO_LLT_INIT_LLT))
			break;
		usleep_range(2, 4);
	}

	if (!i) {
		ret = -EBUSY;
		(void)0;
	}

	return ret;
}

static void rtl8xxxu_init_queue_reserved_page(struct rtw_r92fu *priv)
{
	
	u32 hq, lq, nq, eq, pubq;
	u32 val32;

	hq = 0;
	lq = 0;
	nq = 0;
	eq = 0;
	pubq = 0;

	if (priv->ep_tx_high_queue)
		hq = TX_PAGE_NUM_HI_PQ_8192F;
	if (priv->ep_tx_low_queue)
		lq = TX_PAGE_NUM_LO_PQ_8192F;
	if (priv->ep_tx_normal_queue)
		nq = TX_PAGE_NUM_NORM_PQ_8192F;

	val32 = (nq << RQPN_NPQ_SHIFT) | (eq << RQPN_EPQ_SHIFT);
	rtl8xxxu_write32(priv, REG_RQPN_NPQ, val32);

	pubq = TX_TOTAL_PAGE_NUM_8192F - hq - lq - nq - 1;

	val32 = RQPN_LOAD;
	val32 |= (hq << RQPN_HI_PQ_SHIFT);
	val32 |= (lq << RQPN_LO_PQ_SHIFT);
	val32 |= (pubq << RQPN_PUB_PQ_SHIFT);

	rtl8xxxu_write32(priv, REG_RQPN, val32);
}

static int rtl8xxxu_init_queue_priority(struct rtw_r92fu *priv)
{
	u16 hi, lo;
	u16 hiq, mgq, bkq, beq, viq, voq;
	int hip, mgp, bkp, bep, vip, vop;
	int ret = 0;
	u32 val32;

	switch (priv->ep_tx_count) {
	case 1:
		if (priv->ep_tx_high_queue) {
			hi = TRXDMA_QUEUE_HIGH;
		} else if (priv->ep_tx_low_queue) {
			hi = TRXDMA_QUEUE_LOW;
		} else if (priv->ep_tx_normal_queue) {
			hi = TRXDMA_QUEUE_NORMAL;
		} else {
			hi = 0;
			ret = -EINVAL;
		}

		hiq = hi;
		mgq = hi;
		bkq = hi;
		beq = hi;
		viq = hi;
		voq = hi;

		hip = 0;
		mgp = 0;
		bkp = 0;
		bep = 0;
		vip = 0;
		vop = 0;
		break;
	case 2:
		if (priv->ep_tx_high_queue && priv->ep_tx_low_queue) {
			hi = TRXDMA_QUEUE_HIGH;
			lo = TRXDMA_QUEUE_LOW;
		} else if (priv->ep_tx_normal_queue && priv->ep_tx_low_queue) {
			hi = TRXDMA_QUEUE_NORMAL;
			lo = TRXDMA_QUEUE_LOW;
		} else if (priv->ep_tx_high_queue && priv->ep_tx_normal_queue) {
			hi = TRXDMA_QUEUE_HIGH;
			lo = TRXDMA_QUEUE_NORMAL;
		} else {
			ret = -EINVAL;
			hi = 0;
			lo = 0;
		}

		hiq = hi;
		mgq = hi;
		bkq = lo;
		beq = lo;
		viq = hi;
		voq = hi;

		hip = 0;
		mgp = 0;
		bkp = 1;
		bep = 1;
		vip = 0;
		vop = 0;
		break;
	case 3:
		beq = TRXDMA_QUEUE_LOW;
		bkq = TRXDMA_QUEUE_LOW;
		viq = TRXDMA_QUEUE_NORMAL;
		voq = TRXDMA_QUEUE_HIGH;
		mgq = TRXDMA_QUEUE_HIGH;
		hiq = TRXDMA_QUEUE_HIGH;

		hip = hiq ^ 3;
		mgp = mgq ^ 3;
		bkp = bkq ^ 3;
		bep = beq ^ 3;
		vip = viq ^ 3;
		vop = viq ^ 3;
		break;
	default:
		ret = -EINVAL;
	}

	
	if (!ret) {
		
		{
			val32 = rtl8xxxu_read32(priv, REG_TRXDMA_CTRL);
			val32 &= 0x7;
			val32 |= (voq << TRXDMA_CTRL_VOQ_SHIFT_8192F) |
				 (viq << TRXDMA_CTRL_VIQ_SHIFT_8192F) |
				 (beq << TRXDMA_CTRL_BEQ_SHIFT_8192F) |
				 (bkq << TRXDMA_CTRL_BKQ_SHIFT_8192F) |
				 (mgq << TRXDMA_CTRL_MGQ_SHIFT_8192F) |
				 (hiq << TRXDMA_CTRL_HIQ_SHIFT_8192F);
			rtl8xxxu_write32(priv, REG_TRXDMA_CTRL, val32);
		}

		priv->pipe_out[TXDESC_QUEUE_VO] =
			priv->out_ep[vop];
		priv->pipe_out[TXDESC_QUEUE_VI] =
			priv->out_ep[vip];
		priv->pipe_out[TXDESC_QUEUE_BE] =
			priv->out_ep[bep];
		priv->pipe_out[TXDESC_QUEUE_BK] =
			priv->out_ep[bkp];
		priv->pipe_out[TXDESC_QUEUE_BEACON] =
			priv->out_ep[0];
		priv->pipe_out[TXDESC_QUEUE_MGNT] =
			priv->out_ep[mgp];
		priv->pipe_out[TXDESC_QUEUE_HIGH] =
			priv->out_ep[hip];
		priv->pipe_out[TXDESC_QUEUE_CMD] =
			priv->out_ep[0];
	}

	return ret;
}

static void
rtl8xxxu_set_retry(struct rtw_r92fu *priv, u16 short_retry, u16 long_retry)
{
	u16 val16;

	val16 = ((short_retry << RETRY_LIMIT_SHORT_SHIFT) &
		 RETRY_LIMIT_SHORT_MASK) |
		((long_retry << RETRY_LIMIT_LONG_SHIFT) &
		 RETRY_LIMIT_LONG_MASK);

	rtl8xxxu_write16(priv, REG_RETRY_LIMIT, val16);
}

static void
rtl8xxxu_set_spec_sifs(struct rtw_r92fu *priv, u16 cck, u16 ofdm)
{
	u16 val16;

	val16 = ((cck << SPEC_SIFS_CCK_SHIFT) & SPEC_SIFS_CCK_MASK) |
		((ofdm << SPEC_SIFS_OFDM_SHIFT) & SPEC_SIFS_OFDM_MASK);

	rtl8xxxu_write16(priv, REG_SPEC_SIFS, val16);
}

static int
rtl8xxxu_init_mac(struct rtw_r92fu *priv)
{
	const struct rtl8xxxu_reg8val *array = rtl8192f_mac_init_table;
	int i, ret;
	u16 reg;
	u8 val;

	for (i = 0; ; i++) {
		reg = array[i].reg;
		val = array[i].val;

		if (reg == 0xffff && val == 0xff)
			break;

		ret = rtl8xxxu_write8(priv, reg, val);
		if (ret != 1) {
			(void)0;
			return -EAGAIN;
		}
	}

	switch (RTL8192F) {
	case RTL8188C:
	case RTL8188R:
	case RTL8191C:
	case RTL8192C:
	case RTL8723A:
		rtl8xxxu_write8(priv, REG_MAX_AGGR_NUM, 0x0a);
		break;
	case RTL8188E:
		rtl8xxxu_write16(priv, REG_MAX_AGGR_NUM, 0x0707);
		break;
	default:
		break;
	}

	return 0;
}

static int rtl8xxxu_init_phy_bb(struct rtw_r92fu *priv)
{
	u32 val32;

	rtl8192fu_init_phy_bb(priv);

	if (priv->tx_paths == 1 && priv->rx_paths == 2) {
		
		val32 = rtl8xxxu_read32(priv, REG_FPGA0_TX_INFO);
		val32 &= ~(BIT(0) | BIT(1));
		val32 |= BIT(1);
		rtl8xxxu_write32(priv, REG_FPGA0_TX_INFO, val32);

		val32 = rtl8xxxu_read32(priv, REG_FPGA1_TX_INFO);
		val32 &= ~0x300033;
		val32 |= 0x200022;
		rtl8xxxu_write32(priv, REG_FPGA1_TX_INFO, val32);

		val32 = rtl8xxxu_read32(priv, REG_CCK0_AFE_SETTING);
		val32 &= ~CCK0_AFE_RX_MASK;
		val32 &= 0x00ffffff;
		val32 |= 0x40000000;
		val32 |= CCK0_AFE_RX_ANT_B;
		rtl8xxxu_write32(priv, REG_CCK0_AFE_SETTING, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM0_TRX_PATH_ENABLE);
		val32 &= ~(OFDM_RF_PATH_RX_MASK | OFDM_RF_PATH_TX_MASK);
		val32 |= (OFDM_RF_PATH_RX_A | OFDM_RF_PATH_RX_B |
			  OFDM_RF_PATH_TX_B);
		rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM0_AGC_PARM1);
		val32 &= ~(BIT(4) | BIT(5));
		val32 |= BIT(4);
		rtl8xxxu_write32(priv, REG_OFDM0_AGC_PARM1, val32);

		val32 = rtl8xxxu_read32(priv, REG_TX_CCK_RFON);
		val32 &= ~(BIT(27) | BIT(26));
		val32 |= BIT(27);
		rtl8xxxu_write32(priv, REG_TX_CCK_RFON, val32);

		val32 = rtl8xxxu_read32(priv, REG_TX_CCK_BBON);
		val32 &= ~(BIT(27) | BIT(26));
		val32 |= BIT(27);
		rtl8xxxu_write32(priv, REG_TX_CCK_BBON, val32);

		val32 = rtl8xxxu_read32(priv, REG_TX_OFDM_RFON);
		val32 &= ~(BIT(27) | BIT(26));
		val32 |= BIT(27);
		rtl8xxxu_write32(priv, REG_TX_OFDM_RFON, val32);

		val32 = rtl8xxxu_read32(priv, REG_TX_OFDM_BBON);
		val32 &= ~(BIT(27) | BIT(26));
		val32 |= BIT(27);
		rtl8xxxu_write32(priv, REG_TX_OFDM_BBON, val32);

		val32 = rtl8xxxu_read32(priv, REG_TX_TO_TX);
		val32 &= ~(BIT(27) | BIT(26));
		val32 |= BIT(27);
		rtl8xxxu_write32(priv, REG_TX_TO_TX, val32);
	}

	rtl8192f_set_crystal_cap(priv, priv->default_crystal_cap);

	{}

	return 0;
}

static void rtl8xxxu_gen2_usb_quirks(struct rtw_r92fu *priv)
{
	u32 val32;

	val32 = rtl8xxxu_read32(priv, REG_TXDMA_OFFSET_CHK);
	val32 |= TXDMA_OFFSET_DROP_DATA_EN;
	rtl8xxxu_write32(priv, REG_TXDMA_OFFSET_CHK, val32);
}

static void rtl8xxxu_init_burst(struct rtw_r92fu *priv)
{
	u8 val8;

	
	val8 = rtl8xxxu_read8(priv, REG_RXDMA_PRO_8723B);
	u8p_replace_bits(&val8, 1, RXDMA_PRO_DMA_BURST_SIZE);
	u8p_replace_bits(&val8, 3, RXDMA_PRO_DMA_BURST_CNT);
	val8 |= RXDMA_PRO_DMA_MODE;
	rtl8xxxu_write8(priv, REG_RXDMA_PRO_8723B, val8);

	
	val8 = rtl8xxxu_read8(priv, REG_HT_SINGLE_AMPDU_8723B);
	val8 |= HT_SINGLE_AMPDU_ENABLE;
	rtl8xxxu_write8(priv, REG_HT_SINGLE_AMPDU_8723B, val8);

	rtl8xxxu_write16(priv, REG_MAX_AGGR_NUM, 0x1f1f);
	rtl8xxxu_write8(priv, REG_AMPDU_MAX_TIME_8723B,
			0x5e);
	rtl8xxxu_write32(priv, REG_AGGLEN_LMT, 0xffffffff);
	rtl8xxxu_write8(priv, REG_RX_PKT_LIMIT, 0x18);
	rtl8xxxu_write8(priv, REG_PIFS, 0x00);
	{
		rtl8xxxu_write8(priv, REG_FWHW_TXQ_CTRL, FWHW_TXQ_CTRL_AMPDU_RETRY);
		rtl8xxxu_write32(priv, REG_FAST_EDCA_CTRL, 0x03086666);
	}
	rtl8xxxu_write8(priv, REG_USTIME_TSF_8723B, 0x50);
	rtl8xxxu_write8(priv, REG_USTIME_EDCA, 0x50);

	
	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL);
	val8 |= RSV_CTRL_WLOCK_1C | RSV_CTRL_DIS_PRST;
	rtl8xxxu_write8(priv, REG_RSV_CTRL, val8);
}

static void rtl8xxxu_request_hw_feature(struct rtw_r92fu *priv)
{
	{}

	rtl8xxxu_write8(priv, REG_C2HEVT_MSG_NORMAL, C2H_HW_FEATURE_DUMP);
}

static int rtl8xxxu_dump_hw_feature(struct rtw_r92fu *priv)
{
	static const u8 bw_map[8] = { 0, 0, 160, 5, 10, 20, 40, 80 };
	struct rtl8xxxu_hw_feature *hw_feature = &priv->hw_feature;
	u8 feature[13];
	int i, ret;
	u8 bw;

	{}

	ret = fu_poll_feature(priv);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(feature); i++)
		feature[i] = rtl8xxxu_read8(priv, REG_C2HEVT_MSG_NORMAL + 2 + i);

	rtl8xxxu_write8(priv, REG_C2HEVT_MSG_NORMAL, 0);

	bw = u8_get_bits(feature[6], GENMASK(2, 0));

	hw_feature->max_bw = bw_map[bw];

	return 0;
}

static int rtl8xxxu_start_firmware(struct rtw_r92fu *priv)
{
	
	u16 reg_mcu_fw_dl;
	int ret = 0, i;
	u32 val32;

	reg_mcu_fw_dl = REG_MCU_FW_DL;

	
	for (i = 0; i < RTL8XXXU_FIRMWARE_POLL_MAX; i++) {
		val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
		if (val32 & MCU_FW_DL_CSUM_REPORT)
			break;
	}

	if (i == RTL8XXXU_FIRMWARE_POLL_MAX) {
		(void)0;
		ret = -EAGAIN;
		goto exit;
	}

	val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
	val32 |= MCU_FW_DL_READY;
	val32 &= ~MCU_WINT_INIT_READY;
	rtl8xxxu_write32(priv, reg_mcu_fw_dl, val32);

	
	rtl8192f_reset_8051(priv);

	
	for (i = 0; i < RTL8XXXU_FIRMWARE_POLL_MAX; i++) {
		val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
		if (val32 & MCU_WINT_INIT_READY)
			break;

		udelay(100);
	}

	if (i == RTL8XXXU_FIRMWARE_POLL_MAX) {
		(void)0;
		ret = -EAGAIN;
		goto exit;
	}

	
	rtl8xxxu_write8(priv, REG_HMTFR, 0x0f);
exit:
	return ret;
}

static int rtl8xxxu_download_firmware(struct rtw_r92fu *priv)
{
	int pages, remainder, i, ret;
	u16 reg_fw_start_address;
	u16 reg_mcu_fw_dl;
	u8 val8;
	u16 val16;
	u32 val32;
	const u8 *fwptr;

	reg_fw_start_address = REG_FW_START_ADDRESS_8192F;

	{
		reg_mcu_fw_dl = REG_MCU_FW_DL;

		val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC + 1);
		val8 |= 4;
		rtl8xxxu_write8(priv, REG_SYS_FUNC + 1, val8);

		
		val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
		val16 |= SYS_FUNC_CPU_ENABLE;
		rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);
	}

	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	if (val8 & MCU_FW_RAM_SEL) {
		(void)0;
		rtl8xxxu_write8(priv, reg_mcu_fw_dl, 0x00);
		rtl8192f_reset_8051(priv);
	}

	
	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	val8 |= MCU_FW_DL_ENABLE;
	rtl8xxxu_write8(priv, reg_mcu_fw_dl, val8);

	
	val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
	val32 &= ~BIT(19);
	rtl8xxxu_write32(priv, reg_mcu_fw_dl, val32);

	{}

	
	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	val8 |= MCU_FW_DL_CSUM_REPORT;
	rtl8xxxu_write8(priv, reg_mcu_fw_dl, val8);

	pages = priv->fw_size / RTL_FW_PAGE_SIZE;
	remainder = priv->fw_size % RTL_FW_PAGE_SIZE;

	fwptr = priv->fw_data->data;

	for (i = 0; i < pages; i++) {
		val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl + 2) & 0xF8;
		val8 |= i;
		rtl8xxxu_write8(priv, reg_mcu_fw_dl + 2, val8);

		ret = rtl8xxxu_writeN(priv, reg_fw_start_address,
				      fwptr, RTL_FW_PAGE_SIZE);
		if (ret != RTL_FW_PAGE_SIZE) {
			ret = -EAGAIN;
			goto fw_abort;
		}

		fwptr += RTL_FW_PAGE_SIZE;
	}

	if (remainder) {
		val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl + 2) & 0xF8;
		val8 |= i;
		rtl8xxxu_write8(priv, reg_mcu_fw_dl + 2, val8);
		ret = rtl8xxxu_writeN(priv, reg_fw_start_address,
				      fwptr, remainder);
		if (ret != remainder) {
			ret = -EAGAIN;
			goto fw_abort;
		}
	}

	ret = 0;
fw_abort:
	
	val16 = rtl8xxxu_read16(priv, reg_mcu_fw_dl);
	val16 &= ~MCU_FW_DL_ENABLE;
	rtl8xxxu_write16(priv, reg_mcu_fw_dl, val16);

	return ret;
}

static int rtl8xxxu_init_device(struct ieee80211_hw *hw)
{
	struct rtw_r92fu *priv = hw->priv;
	
	
	
	int ret;
	u8 val8;
	u16 val16;
	u32 val32;

	
	val8 = rtl8xxxu_read8(priv, REG_CR);
	val16 = rtl8xxxu_read16(priv, REG_SYS_CLKR);

	
	if (val8 == 0xea || !(val16 & SYS_CLK_MAC_CLK_ENABLE))
		(void)0;
	else
		(void)0;

	(void)0;

	ret = rtl8192fu_power_on(priv);
	if (ret < 0) {
		(void)0;
		goto exit;
	}

	rtl8xxxu_init_queue_reserved_page(priv);

	ret = rtl8xxxu_init_queue_priority(priv);
	(void)0;
	if (ret)
		goto exit;

	
	rtl8xxxu_write16(priv, REG_TRXFF_BNDY + 2, 0x3f3f);

	rtl8xxxu_request_hw_feature(priv);

	for (int retry = 5; retry >= 0 ; retry--) {
		ret = rtl8xxxu_download_firmware(priv);
		(void)0;
		if (ret != -EAGAIN)
			break;
		if (retry)
			(void)0;
	}
	if (ret)
		goto exit;
	ret = rtl8xxxu_start_firmware(priv);
	(void)0;
	if (ret)
		goto exit;

	ret = rtl8xxxu_dump_hw_feature(priv);
	if (ret) {
		(void)0;
		goto exit;
	}

	{}

	ret = rtl8xxxu_init_mac(priv);

	(void)0;
	if (ret)
		goto exit;

	ret = rtl8xxxu_init_phy_bb(priv);
	(void)0;
	if (ret)
		goto exit;

	ret = rtl8192fu_init_phy_rf(priv);
	if (ret)
		goto exit;

	
	rtl8xxxu_write16_set(priv, REG_AFE_CTRL4, BIT(4) | BIT(15));

	
	{}

	val32 = FPGA0_RF_TRSW | FPGA0_RF_TRSWB | FPGA0_RF_ANTSW |
		FPGA0_RF_ANTSWB |
		((FPGA0_RF_ANTSW | FPGA0_RF_ANTSWB) << FPGA0_RF_BD_CTRL_SHIFT);
	if (!priv->no_pape) {
		val32 |= (FPGA0_RF_PAPE |
			  (FPGA0_RF_PAPE << FPGA0_RF_BD_CTRL_SHIFT));
	}
	rtl8xxxu_write32(priv, REG_FPGA0_XAB_RF_SW_CTRL, val32);

	
	{}

	{
		
		val8 = TX_TOTAL_PAGE_NUM_8192F + 1;

		rtl8xxxu_write8(priv, REG_TXPKTBUF_BCNQ_BDNY, val8);
		rtl8xxxu_write8(priv, REG_TXPKTBUF_MGQ_BDNY, val8);
		rtl8xxxu_write8(priv, REG_TXPKTBUF_WMAC_LBK_BF_HD, val8);
		rtl8xxxu_write8(priv, REG_TRXFF_BNDY, val8);
		rtl8xxxu_write8(priv, REG_TDECTRL + 1, val8);
	}

	
	val8 = (PBP_PAGE_SIZE_256 << PBP_PAGE_SIZE_RX_SHIFT) |
		(PBP_PAGE_SIZE_256 << PBP_PAGE_SIZE_TX_SHIFT);
	rtl8xxxu_write8(priv, REG_PBP, val8);

	(void)0;
	{
		ret = rtl8xxxu_auto_llt_table(priv);
		if (ret) {
			(void)0;
			goto exit;
		}

		
		rtl8192f_usb_quirks(priv);

		
		{
			
			val8 = rtl8xxxu_read8(priv, REG_TX_REPORT_CTRL);
			{}
			val8 |= TX_REPORT_CTRL_TIMER_ENABLE;
			rtl8xxxu_write8(priv, REG_TX_REPORT_CTRL, val8);
			
			rtl8xxxu_write8(priv, REG_TX_REPORT_CTRL + 1, 0x02);
			
			rtl8xxxu_write16(priv, REG_TX_REPORT_TIME, 0xcdf0);

			
			val8 = rtl8xxxu_read8(priv, 0xa3);
			val8 &= 0xf8;
			rtl8xxxu_write8(priv, 0xa3, val8);
		}

		rtl8xxxu_write8(priv, REG_EARLY_MODE_CONTROL_8710B, 0);
	}

	
	rtl8xxxu_write8(priv, REG_RX_DRVINFO_SZ, 4);

	{}

	
	val32 = RCR_ACCEPT_PHYS_MATCH | RCR_ACCEPT_MCAST | RCR_ACCEPT_BCAST |
		RCR_ACCEPT_MGMT_FRAME | RCR_HTC_LOC_CTRL |
		RCR_APPEND_PHYSTAT | RCR_APPEND_ICV | RCR_APPEND_MIC;
	rtl8xxxu_write32(priv, REG_RCR, val32);
	priv->regrcr = val32;

	{
		
		rtl8xxxu_write16(priv, REG_RXFLTMAP2, 0xffff);

		
		rtl8xxxu_write16(priv, REG_RXFLTMAP1, 0x400);

		
		rtl8xxxu_write16(priv, REG_RXFLTMAP0, 0xffff);
	}

	
	val32 = rtl8xxxu_read32(priv, REG_RESPONSE_RATE_SET);
	val32 &= ~RESPONSE_RATE_BITMAP_ALL;
	val32 |= RESPONSE_RATE_RRSR_CCK_ONLY_1M;
	rtl8xxxu_write32(priv, REG_RESPONSE_RATE_SET, val32);

	
	rtl8xxxu_set_spec_sifs(priv, 0x10, 0x10);
	rtl8xxxu_set_retry(priv, 0x30, 0x30);
	rtl8xxxu_set_spec_sifs(priv, 0x0a, 0x10);

	
	rtl8xxxu_write16(priv, REG_MAC_SPEC_SIFS, 0x100a);

	
	rtl8xxxu_write16(priv, REG_SIFS_CCK, 0x100a);

	
	rtl8xxxu_write16(priv, REG_SIFS_OFDM, 0x100a);

	
	rtl8xxxu_write32(priv, REG_EDCA_BE_PARAM, 0x005ea42b);
	rtl8xxxu_write32(priv, REG_EDCA_BK_PARAM, 0x0000a44f);
	rtl8xxxu_write32(priv, REG_EDCA_VI_PARAM, 0x005ea324);
	rtl8xxxu_write32(priv, REG_EDCA_VO_PARAM, 0x002fa226);

	
	rtl8xxxu_write32(priv, REG_DARFRC, 0x00000000);
	rtl8xxxu_write32(priv, REG_DARFRC + 4, 0x10080404);
	rtl8xxxu_write32(priv, REG_RARFRC, 0x04030201);
	rtl8xxxu_write32(priv, REG_RARFRC + 4, 0x08070605);

	val8 = rtl8xxxu_read8(priv, REG_FWHW_TXQ_CTRL);
	val8 |= FWHW_TXQ_CTRL_AMPDU_RETRY;
	rtl8xxxu_write8(priv, REG_FWHW_TXQ_CTRL, val8);

	
	rtl8xxxu_write8(priv, REG_ACKTO, 0x40);

	
	val16 = BEACON_DISABLE_TSF_UPDATE | (BEACON_DISABLE_TSF_UPDATE << 8);
	rtl8xxxu_write16(priv, REG_BEACON_CTRL, val16);
	rtl8xxxu_write16(priv, REG_TBTT_PROHIBIT, 0x6404);
	{}
	rtl8xxxu_write8(priv, REG_BEACON_DMA_TIME, BEACON_DMA_ATIME_INT_TIME);
	rtl8xxxu_write16(priv, REG_BEACON_TCFG, 0x660F);

	
	rtl8xxxu_init_burst(priv);

	rtl8192fu_init_aggregation(priv);

	{
		rtl8xxxu_write16(priv, REG_PKT_VO_VI_LIFE_TIME, 0x0400); 
		rtl8xxxu_write16(priv, REG_PKT_BE_BK_LIFE_TIME, 0x0400); 
	}

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
	val32 |= (FPGA_RF_MODE_CCK | FPGA_RF_MODE_OFDM);
	rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

	
	rtl8xxxu_write32(priv, REG_CAM_CMD, CAM_CMD_POLLING | BIT(30));

	
	rtl8192f_set_tx_power(priv, 1, false);

	
	{
		val8 = rtl8xxxu_read8(priv, REG_LEDCFG2);
		val8 |= LEDCFG2_DPDT_SELECT;
		rtl8xxxu_write8(priv, REG_LEDCFG2, val8);
	}

	rtl8xxxu_write8(priv, REG_HWSEQ_CTRL, 0xff);

	
	rtl8xxxu_write32(priv, REG_BAR_MODE_CTRL, 0x0201ffff);

	{}

	{}

	{
		
		val8 = rtl8xxxu_read8(priv, REG_GPIO_MUXCFG);
		val8 &= ~GPIO_MUXCFG_IO_SEL_ENBT;
		rtl8xxxu_write8(priv, REG_GPIO_MUXCFG, val8);
	}

	{}

	rtl8192f_phy_lc_calibrate(priv);

	rtl8192fu_phy_iq_calibrate(priv);

	
	{
		{
			rtl8xxxu_write_rfreg(priv,
					     RF_A, RF6052_REG_T_METER_8723B, 0x37cf8);
		}
	}

	
	val8 = ((30000 + NAV_UPPER_UNIT - 1) / NAV_UPPER_UNIT);
	rtl8xxxu_write8(priv, REG_NAV_UPPER, val8);

	{}

	val32 = rtl8xxxu_read32(priv, REG_FWHW_TXQ_CTRL);
	val32 |= FWHW_TXQ_CTRL_XMIT_MGMT_ACK;
	
	rtl8xxxu_write32(priv, REG_FWHW_TXQ_CTRL, val32);

	{}

	{}

	val32 = rtl8xxxu_read32(priv, 0xa9c);
	priv->cck_new_agc = u32_get_bits(val32, BIT(17));

	
	{
		val32 = rtl8xxxu_read32(priv, REG_OFDM1_CFO_TRACKING);
		priv->cfo_tracking.atc_status = val32 & CFO_TRACKING_ATC_STATUS;
		priv->cfo_tracking.adjust = true;
		priv->cfo_tracking.crystal_cap = priv->default_crystal_cap;
	}

	{}

	
	

exit:
	return ret;
}

static void
rtl8192f_set_tx_power(struct rtw_r92fu *priv, int channel, bool ht40)
{
	u8 cck, ofdmbase, mcsbase;
	u32 val32, ofdm, mcs;
	int group, cck_group;

	rtl8188f_channel_to_group(channel, &group, &cck_group);

	cck = priv->cck_tx_power_index_A[cck_group];

	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_CCK1_MCS32, 0x00007f00, cck);

	val32 = (cck << 16) | (cck << 8) | cck;
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_CCK11_A_CCK2_11,
			      0x7f7f7f00, val32);

	ofdmbase = priv->ht40_1s_tx_power_index_A[group];
	ofdmbase += priv->ofdm_tx_power_diff[RF_A].a;
	ofdm = ofdmbase | ofdmbase << 8 | ofdmbase << 16 | ofdmbase << 24;

	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_RATE18_06, 0x7f7f7f7f, ofdm);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_RATE54_24, 0x7f7f7f7f, ofdm);

	mcsbase = priv->ht40_1s_tx_power_index_A[group];
	if (ht40)
		mcsbase += priv->ht40_tx_power_diff[RF_A].a;
	else
		mcsbase += priv->ht20_tx_power_diff[RF_A].a;
	mcs = mcsbase | mcsbase << 8 | mcsbase << 16 | mcsbase << 24;

	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_MCS03_MCS00, 0x7f7f7f7f, mcs);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_MCS07_MCS04, 0x7f7f7f7f, mcs);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_MCS11_MCS08, 0x7f7f7f7f, mcs);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_A_MCS15_MCS12, 0x7f7f7f7f, mcs);

	if (priv->tx_paths == 1)
		return;

	cck = priv->cck_tx_power_index_B[cck_group];

	val32 = (cck << 16) | (cck << 8) | cck;
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_CCK1_55_MCS32,
			      0x7f7f7f00, val32);

	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_CCK11_A_CCK2_11,
			      0x0000007f, cck);

	ofdmbase = priv->ht40_1s_tx_power_index_B[group];
	ofdmbase += priv->ofdm_tx_power_diff[RF_B].b;
	ofdm = ofdmbase | ofdmbase << 8 | ofdmbase << 16 | ofdmbase << 24;

	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_RATE18_06, 0x7f7f7f7f, ofdm);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_RATE54_24, 0x7f7f7f7f, ofdm);

	mcsbase = priv->ht40_1s_tx_power_index_B[group];
	if (ht40)
		mcsbase += priv->ht40_tx_power_diff[RF_B].b;
	else
		mcsbase += priv->ht20_tx_power_diff[RF_B].b;
	mcs = mcsbase | mcsbase << 8 | mcsbase << 16 | mcsbase << 24;

	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_MCS03_MCS00, 0x7f7f7f7f, mcs);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_MCS07_MCS04, 0x7f7f7f7f, mcs);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_MCS11_MCS08, 0x7f7f7f7f, mcs);
	rtl8xxxu_write32_mask(priv, REG_TX_AGC_B_MCS15_MCS12, 0x7f7f7f7f, mcs);
}

static void rtl8192fu_power_off(struct rtw_r92fu *priv)
{
	rtl8xxxu_flush_fifo(priv);

	
	rtl8xxxu_write8_clear(priv, REG_TX_REPORT_CTRL,
			      TX_REPORT_CTRL_TIMER_ENABLE);

	
	rtl8xxxu_write8(priv, REG_CR, 0x00);

	rtl8192fu_active_to_lps(priv);

	
	if (rtl8xxxu_read8(priv, REG_MCU_FW_DL) & MCU_FW_RAM_SEL)
		rtl8xxxu_firmware_self_reset(priv);

	
	rtl8xxxu_write16_clear(priv, REG_SYS_FUNC, SYS_FUNC_CPU_ENABLE);

	
	rtl8xxxu_write8(priv, REG_MCU_FW_DL, 0x00);

	rtl8192fu_active_to_emu(priv);
	rtl8192fu_emu_to_disabled(priv);
}

static void rtl8192f_revise_cck_tx_psf(struct rtw_r92fu *priv, u8 channel)
{
	if (channel == 13) {
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER1, 0xf8fe0001);
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER2, 0x64B80C1C);
		rtl8xxxu_write16(priv, REG_CCK0_DEBUG_PORT, 0x8810);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER3, 0x01235667);
	} else if (channel == 14) {
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER1, 0xE82C0001);
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER2, 0x0000B81C);
		rtl8xxxu_write16(priv, REG_CCK0_DEBUG_PORT, 0x0000);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER3, 0x00003667);
	} else {
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER1, 0xE82C0001);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER2, 0x64B80C1C);
		rtl8xxxu_write16(priv, REG_CCK0_DEBUG_PORT, 0x8810);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER3, 0x01235667);
	}
}

static void rtl8192fu_config_kfree(struct rtw_r92fu *priv, u8 channel)
{
	u8 bb_gain[3] = { EFUSE_UNDEFINED, EFUSE_UNDEFINED, EFUSE_UNDEFINED };
	u8 bb_gain_path_mask[2] = { 0x0f, 0xf0 };
	enum rtl8xxxu_rfpath rfpath;
	u8 bb_gain_for_path;
	u8 channel_idx = 0;

	if (channel >= 1 && channel <= 3)
		channel_idx = 0;
	if (channel >= 4 && channel <= 9)
		channel_idx = 1;
	if (channel >= 10 && channel <= 14)
		channel_idx = 2;

	rtl8xxxu_read_efuse8(priv, 0x1ee, &bb_gain[1]);
	rtl8xxxu_read_efuse8(priv, 0x1ec, &bb_gain[0]);
	rtl8xxxu_read_efuse8(priv, 0x1ea, &bb_gain[2]);

	if (bb_gain[1] == EFUSE_UNDEFINED)
		return;

	if (bb_gain[0] == EFUSE_UNDEFINED)
		bb_gain[0] = bb_gain[1];

	if (bb_gain[2] == EFUSE_UNDEFINED)
		bb_gain[2] = bb_gain[1];

	for (rfpath = RF_A; rfpath < priv->rf_paths; rfpath++) {
		
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_UNKNOWN_55,
					  BIT(5), 1);

		
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_GAIN_CTRL,
					  BIT(18), 1);

		
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_GAIN_CCA,
					  BIT(7), 1);

		
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_WE_LUT, BIT(7), 1);

		bb_gain_for_path = (bb_gain[channel_idx] & bb_gain_path_mask[rfpath]);
		bb_gain_for_path >>= __ffs(bb_gain_path_mask[rfpath]);

		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_TXPA_G3,
					  0x70000, channel_idx * 2);
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_TXPA_G3,
					  0x3f, bb_gain_for_path);

		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_TXPA_G3,
					  0x70000, channel_idx * 2 + 1);
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_TXPA_G3,
					  0x3f, bb_gain_for_path);

		
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_GAIN_CCA,
					  BIT(7), 0);

		
		rtl8xxxu_write_rfreg_mask(priv, rfpath, RF6052_REG_WE_LUT, BIT(7), 0);
	}
}

static void rtl8192fu_config_channel(struct ieee80211_hw *hw)
{
	struct rtw_r92fu *priv = hw->priv;
	bool ht40 = conf_is_ht40(&hw->conf);
	u8 channel, subchannel = 0;
	bool sec_ch_above = 0;
	u32 val32;

	channel = (u8)hw->conf.chandef.chan->hw_value;

	if (conf_is_ht40_plus(&hw->conf)) {
		sec_ch_above = 1;
		channel += 2;
		subchannel = 2;
	} else if (conf_is_ht40_minus(&hw->conf)) {
		sec_ch_above = 0;
		channel -= 2;
		subchannel = 1;
	}

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG);

	rtl8192f_revise_cck_tx_psf(priv, channel);

	
	val32 &= ~(BIT(18) | BIT(17)); 
	u32p_replace_bits(&val32, channel, 0xff);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, val32);
	if (priv->rf_paths > 1)
		rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_MODE_AG, val32);

	rtl8192fu_config_kfree(priv, channel);

	rtl8xxxu_write8(priv, REG_DATA_SUBCHANNEL, subchannel);

	
	rtl8xxxu_write32_clear(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT, GENMASK(31, 30));

	rtl8xxxu_write32_mask(priv, REG_FPGA0_RF_MODE, FPGA_RF_MODE, ht40);
	rtl8xxxu_write32_mask(priv, REG_FPGA1_RF_MODE, FPGA_RF_MODE, ht40);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_RF_MODE, GENMASK(10, 8), 4);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_RF_MODE, BIT(13) | BIT(12), 2);

	
	rtl8xxxu_write32_mask(priv, REG_ANTDIV_PARA1, BIT(27) | BIT(26), 2);

	if (ht40)
		
		rtl8xxxu_write32_mask(priv, REG_CCK0_SYSTEM,
				      CCK0_SIDEBAND, !sec_ch_above);

	
	rtl8xxxu_write32_set(priv, REG_FPGA0_RF_MODE, FPGA_RF_MODE_CCK);

	
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG);
	val32 &= ~MODE_AG_BW_MASK;
	if (ht40)
		val32 |= MODE_AG_BW_40MHZ_8723B;
	else
		val32 |= MODE_AG_BW_20MHZ_8723B;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, val32);
	if (priv->rf_paths > 1)
		rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_MODE_AG, val32);

	
	rtl8xxxu_write32_mask(priv, REG_TAP_UPD_97F, BIT(21) | BIT(20), 2);

	rtl8xxxu_write32_mask(priv, REG_DOWNSAM_FACTOR, BIT(29) | BIT(28), 2);

	if (ht40)
		val32 = 0x3;
	else
		val32 = 0x1a3;
	rtl8xxxu_write32_mask(priv, REG_RX_DFIR_MOD_97F, 0x1ff, val32);
}

static void rtl8192fu_init_aggregation(struct rtw_r92fu *priv)
{
	u32 agg_rx;
	u8 agg_ctrl;

	
	agg_ctrl = rtl8xxxu_read8(priv, REG_TRXDMA_CTRL);
	agg_ctrl &= ~TRXDMA_CTRL_RXDMA_AGG_EN;

	agg_rx = rtl8xxxu_read32(priv, REG_RXDMA_AGG_PG_TH);
	agg_rx &= ~RXDMA_USB_AGG_ENABLE;
	agg_rx &= ~0xFF0F; 

	rtl8xxxu_write8(priv, REG_TRXDMA_CTRL, agg_ctrl);
	rtl8xxxu_write32(priv, REG_RXDMA_AGG_PG_TH, agg_rx);
}

static void rtl8192fu_init_phy_bb(struct rtw_r92fu *priv)
{
	
	rtl8xxxu_write16_set(priv, REG_SYS_FUNC,
			     SYS_FUNC_BBRSTB | SYS_FUNC_BB_GLB_RSTN);

	rtl8xxxu_write8(priv, REG_RF_CTRL, RF_ENABLE | RF_RSTB | RF_SDMRSTB);

	
	rtl8xxxu_write8(priv, REG_LDOHCI12_CTRL, 0xf);
	rtl8xxxu_write8(priv, REG_SYS_SWR_CTRL2 + 1, 0xe9);

	rtl8xxxu_init_phy_regs(priv, rtl8192fu_phy_init_table);

	rtl8xxxu_init_phy_regs(priv, rtl8192f_agc_table);
}

static int rtl8192fu_init_phy_rf(struct rtw_r92fu *priv)
{
	int ret;

	ret = rtl8xxxu_init_phy_rf(priv, rtl8192fu_radioa_init_table, RF_A);
	if (ret)
		return ret;

	return rtl8xxxu_init_phy_rf(priv, rtl8192fu_radiob_init_table, RF_B);
}

static void rtl8192f_phy_lc_calibrate(struct rtw_r92fu *priv)
{
	u32 backup_mask = BIT(31) | BIT(30);
	u32 backup;
	u32 val32;

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT);
	backup = u32_get_bits(val32, backup_mask);

	u32p_replace_bits(&val32, 0, backup_mask);
	rtl8xxxu_write32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT, val32);

	rtl8188f_phy_lc_calibrate(priv);

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT);
	u32p_replace_bits(&val32, backup, backup_mask);
	rtl8xxxu_write32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT, val32);

	
	rtl8xxxu_write32_clear(priv, REG_FPGA0_RF_MODE, FPGA_RF_MODE_OFDM);
	rtl8xxxu_write32_set(priv, REG_FPGA0_RF_MODE, FPGA_RF_MODE_OFDM);
}

static int rtl8192fu_iqk_path_a(struct rtw_r92fu *priv)
{
 bool measured_ready=true;
	u32 reg_eac, reg_e94, reg_e9c, val32;
	u32 rf_0x58_i, rf_0x58_q;
	u8 rfe = priv->rfe_type;
	int result = 0;
	int ktime, i;

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write32(priv, REG_FPGA0_ANALOG4, 0xccf000c0);
	rtl8xxxu_write32(priv, REG_ANAPWR1, 0x44ffbb44);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x00400040);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x6f005403);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000804e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x04203400);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000100);

	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(4), 1);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(11), 1);
	if (rfe == 7 || rfe == 8 || rfe == 9 || rfe == 12)
		val32 = 0x30;
	else
		val32 = 0xe9;
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_PAD_TXG, 0x003ff, val32);

	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x8214000f);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28140000);

	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x00e62911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa005800);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8005800);

	mdelay(15);

	ktime = 0;
	while (rtl8xxxu_read32(priv, REG_IQK_RPT_TXA) == 0 && ktime < 21) {
		mdelay(5);
		ktime += 5;
	}
 measured_ready=measured_ready&&(rtl8xxxu_read32(priv,REG_IQK_RPT_TXA)!=0);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_e94 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
	reg_e9c = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_WE_LUT, BIT(4), 1);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_TXMOD);
	rf_0x58_i = u32_get_bits(val32, 0xfc000);
	rf_0x58_q = u32_get_bits(val32, 0x003f0);

	for (i = 0; i < 8; i++) {
		rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_TXPA_G3,
					  0x1c000, i);
		rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_TXPA_G3,
					  0x00fc0, rf_0x58_i);
		rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_TXPA_G3,
					  0x0003f, rf_0x58_q);
	}

	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_AC, BIT(14), 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_WE_LUT, BIT(4), 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, 0x00810, 0);

	if (!(reg_eac & BIT(28)) &&
	    ((reg_e94 & 0x03ff0000) != 0x01420000) &&
	    ((reg_e9c & 0x03ff0000) != 0x00420000))
		result |= 0x01;

	return measured_ready?result:0;
}

static int rtl8192fu_rx_iqk_path_a(struct rtw_r92fu *priv)
{
 bool measured_ready=true;
	u32 reg_ea4, reg_eac, reg_e94, reg_e9c, val32;
	int result = 0;
	int ktime;

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(1), 1);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_P1, 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(11), 1);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_PAD_TXG, 0x003ff, 0x27);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x82160027);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28160000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0086a911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa005800);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8005800);

	mdelay(15);

	ktime = 0;
	while (rtl8xxxu_read32(priv, REG_IQK_RPT_TXA) == 0 && ktime < 21) {
		mdelay(5);
		ktime += 5;
	}
 measured_ready=measured_ready&&(rtl8xxxu_read32(priv,REG_IQK_RPT_TXA)!=0);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_e94 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
	reg_e9c = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);

	if (!(reg_eac & BIT(28)) &&
	    ((reg_e94 & 0x03ff0000) != 0x01420000) &&
	    ((reg_e9c & 0x03ff0000) != 0x00420000)) {
		result |= 0x01;
	} else { 
		
		rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

		rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA,
					  BIT(11), 0);

		return measured_ready?result:0;
	}

	val32 = 0x80007c00 | (reg_e94 & 0x3ff0000) | ((reg_e9c & 0x3ff0000) >> 16);
	rtl8xxxu_write32(priv, REG_TX_IQK, val32);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(1), 1);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_P1, 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(11), 1);
	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_PAD_TXG, 0x003ff, 0x1e0);

	rtl8xxxu_write32(priv, REG_FPGA0_ANALOG4, 0xccf000c0);
	rtl8xxxu_write32(priv, REG_ANAPWR1, 0x44ffbb44);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x00400040);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x6f005403);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000804e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x04203400);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000100);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x82170000);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28170000);

	
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0046a8d1);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa005800);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8005800);

	mdelay(15);

	ktime = 0;
	while (rtl8xxxu_read32(priv, REG_IQK_RPT_RXA) == 0 && ktime < 21) {
		mdelay(5);
		ktime += 5;
	}
 measured_ready=measured_ready&&(rtl8xxxu_read32(priv,REG_IQK_RPT_RXA)!=0);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_ea4 = rtl8xxxu_read32(priv, REG_RX_POWER_BEFORE_IQK_A_2);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write_rfreg_mask(priv, RF_A, RF6052_REG_GAIN_CCA, BIT(11), 0);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_P1, 0x02000);

	if (!(reg_eac & BIT(27)) &&
	    ((reg_ea4 & 0x03ff0000) != 0x01320000) &&
	    ((reg_eac & 0x03ff0000) != 0x00360000))
		result |= 0x02;

	return measured_ready?result:0;
}

static int rtl8192fu_iqk_path_b(struct rtw_r92fu *priv)
{
 bool measured_ready=true;
	u32 reg_eac, reg_eb4, reg_ebc, val32;
	u32 rf_0x58_i, rf_0x58_q;
	u8 rfe = priv->rfe_type;
	int result = 0;
	int ktime, i;

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write32(priv, REG_FPGA0_ANALOG4, 0xccf000c0);
	rtl8xxxu_write32(priv, REG_ANAPWR1, 0x44ffbb44);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x00400040);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x6f005403);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000804e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x04203400);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000000);

	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(4), 1);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(11), 1);
	if (rfe == 7 || rfe == 8 || rfe == 9 || rfe == 12)
		rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_PAD_TXG,
					  0x003ff, 0x30);
	else
		rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_PAD_TXG,
					  0x00fff, 0xe9);

	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_B, 0x8214000F);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_B, 0x28140000);

	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x00e62911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa005800);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8005800);

	mdelay(15);

	ktime = 0;
	while (rtl8xxxu_read32(priv, REG_IQK_RPT_TXB) == 0 && ktime < 21) {
		mdelay(5);
		ktime += 5;
	}
 measured_ready=measured_ready&&(rtl8xxxu_read32(priv,REG_IQK_RPT_TXB)!=0);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_eb4 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_B);
	reg_ebc = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_B);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_WE_LUT, BIT(4), 1);

	val32 = rtl8xxxu_read_rfreg(priv, RF_B, RF6052_REG_TXMOD);
	rf_0x58_i = u32_get_bits(val32, 0xfc000);
	rf_0x58_q = u32_get_bits(val32, 0x003f0);

	for (i = 0; i < 8; i++) {
		rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_TXPA_G3,
					  0x1c000, i);
		rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_TXPA_G3,
					  0x00fc0, rf_0x58_i);
		rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_TXPA_G3,
					  0x0003f, rf_0x58_q);
	}

	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_AC, BIT(14), 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_WE_LUT, BIT(4), 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, 0x00810, 0);

	if (!(reg_eac & BIT(31)) &&
	    ((reg_eb4 & 0x03ff0000) != 0x01420000) &&
	    ((reg_ebc & 0x03ff0000) != 0x00420000))
		result |= 0x01;
	else
		(void)0;

	return measured_ready?result:0;
}

static int rtl8192fu_rx_iqk_path_b(struct rtw_r92fu *priv)
{
 bool measured_ready=true;
	u32 reg_eac, reg_eb4, reg_ebc, reg_ec4, reg_ecc, val32;
	int result = 0;
	int ktime;

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(1), 1);
	rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_GAIN_P1, 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(11), 1);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_PAD_TXG, 0x003ff, 0x67);

	rtl8xxxu_write32(priv, REG_FPGA0_ANALOG4, 0xccf000c0);
	rtl8xxxu_write32(priv, REG_ANAPWR1, 0x44ffbb44);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x00400040);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x6f005403);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000804e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x04203400);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000000);

	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_B, 0x82160027);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_B, 0x28160000);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0086a911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa005800);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8005800);

	mdelay(15);

	ktime = 0;
	while (rtl8xxxu_read32(priv, REG_IQK_RPT_TXB) == 0 && ktime < 21) {
		mdelay(5);
		ktime += 5;
	}
 measured_ready=measured_ready&&(rtl8xxxu_read32(priv,REG_IQK_RPT_TXB)!=0);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_eb4 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_B);
	reg_ebc = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_B);

	if (!(reg_eac & BIT(31)) &&
	    ((reg_eb4 & 0x03ff0000) != 0x01420000) &&
	    ((reg_ebc & 0x03ff0000) != 0x00420000)) {
		result |= 0x01;
	} else {
		
		rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

		rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA,
					  BIT(11), 0);

		return measured_ready?result:0;
	}

	val32 = 0x80007c00 | (reg_eb4 & 0x03ff0000) | ((reg_ebc >> 16) & 0x03ff);
	rtl8xxxu_write32(priv, REG_TX_IQK, val32);

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(1), 1);
	rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_GAIN_P1, 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(11), 1);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_PAD_TXG, 0x003ff, 0x1e0);

	rtl8xxxu_write32(priv, REG_FPGA0_ANALOG4, 0xccf000c0);
	rtl8xxxu_write32(priv, REG_ANAPWR1, 0x44ffbb44);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x00400040);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x6f005403);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000804e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x04203400);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000000);

	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x18008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_B, 0x82170000);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_B, 0x28170000);

	
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0046a911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa005800);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8005800);

	mdelay(15);

	ktime = 0;
	while (rtl8xxxu_read32(priv, REG_IQK_RPT_RXB) == 0 && ktime < 21) {
		mdelay(5);
		ktime += 5;
	}
 measured_ready=measured_ready&&(rtl8xxxu_read32(priv,REG_IQK_RPT_RXB)!=0);

	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_ec4 = rtl8xxxu_read32(priv, REG_RX_POWER_BEFORE_IQK_B_2);
	reg_ecc = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_B_2);

	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000100);

	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(11), 0);
	rtl8xxxu_write_rfreg_mask(priv, RF_B, RF6052_REG_GAIN_CCA, BIT(1), 0);
	rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_GAIN_P1, 0x02000);

	if (!(reg_eac & BIT(30)) &&
	    ((reg_ec4 & 0x03ff0000) != 0x01320000) &&
	    ((reg_ecc & 0x03ff0000) != 0x00360000))
		result |= 0x02;
	else
		(void)0;

	return measured_ready?result:0;
}

static void rtl8192fu_phy_iqcalibrate(struct rtw_r92fu *priv,
				      int result[][8], int t)
{
	static const u32 adda_regs[2] = {
		REG_ANAPWR1, REG_RX_WAIT_CCA
	};
	static const u32 iqk_mac_regs[RTL8XXXU_MAC_REGS] = {
		REG_TXPAUSE, REG_BEACON_CTRL,
		REG_BEACON_CTRL_1, REG_GPIO_MUXCFG
	};
	static const u32 iqk_bb_regs[RTL8XXXU_BB_REGS] = {
		REG_OFDM0_TRX_PATH_ENABLE, REG_OFDM0_TR_MUX_PAR,
		REG_FPGA0_XCD_RF_SW_CTRL, REG_CONFIG_ANT_A, REG_CONFIG_ANT_B,
		REG_DPDT_CTRL, REG_RFE_CTRL_ANTA_SRC,
		REG_RFE_CTRL_ANT_SRC2, REG_CCK0_AFE_SETTING
	};
	u32 rx_initial_gain_a, rx_initial_gain_b;
	
	int path_a_ok, path_b_ok;
	u8 rfe = priv->rfe_type;
	u32 retry = 2;
	u32 i, val32;

	

	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rx_initial_gain_a = rtl8xxxu_read32(priv, REG_OFDM0_XA_AGC_CORE1);
	rx_initial_gain_b = rtl8xxxu_read32(priv, REG_OFDM0_XB_AGC_CORE1);

	if (t == 0) {
		
		rtl8xxxu_save_regs(priv, adda_regs, priv->adda_backup,
				   ARRAY_SIZE(adda_regs));
		rtl8xxxu_save_mac_regs(priv, iqk_mac_regs, priv->mac_backup);
		rtl8xxxu_save_regs(priv, iqk_bb_regs,
				   priv->bb_backup, RTL8XXXU_BB_REGS);
	}

	
	rtl8xxxu_write32_set(priv, REG_FPGA0_XCD_RF_PARM, BIT(31));

	
	rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);
	rtl8xxxu_write8_clear(priv, REG_GPIO_MUXCFG, GPIO_MUXCFG_IO_SEL_ENBT);

	if (rfe == 7 || rfe == 8 || rfe == 9 || rfe == 12) {
		
		
		rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANTA_SRC, 0xF, 0x7);
		rtl8xxxu_write32_mask(priv, REG_DPDT_CTRL, 0x1, 0x0);

		rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANTA_SRC, 0xF00, 0x7);
		rtl8xxxu_write32_mask(priv, REG_DPDT_CTRL, 0x4, 0x0);

		rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANTA_SRC, 0xF000, 0x7);
		rtl8xxxu_write32_mask(priv, REG_DPDT_CTRL, 0x8, 0x0);

		
		rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC2, 0xF0, 0x7);
		rtl8xxxu_write32_mask(priv, REG_DPDT_CTRL, 0x20000, 0x0);

		rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC2, 0xF0000, 0x7);
		rtl8xxxu_write32_mask(priv, REG_DPDT_CTRL, 0x100000, 0x0);

		rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC3, 0xF000, 0x7);
		rtl8xxxu_write32_mask(priv, REG_DPDT_CTRL, 0x8000000, 0x0);
	}

	if (priv->rf_paths > 1) {
		
		rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x000000);
		rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_AC, 0x10000);
		rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0x808000);
	}

	for (i = 0; i < retry; i++) {
		path_a_ok = rtl8192fu_iqk_path_a(priv);

		if (path_a_ok == 0x01) {
			val32 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
			result[t][0] = (val32 >> 16) & 0x3ff;if(result[t][0])priv->measured_tx[t]|=1;

			val32 = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);
			result[t][1] = (val32 >> 16) & 0x3ff;
			break;
		} else {
			result[t][0] = 0x100;
			result[t][1] = 0x0;
		}
	}

	for (i = 0; i < retry; i++) {
		path_a_ok = rtl8192fu_rx_iqk_path_a(priv);

		if (path_a_ok == 0x03) {
			val32 = rtl8xxxu_read32(priv, REG_RX_POWER_BEFORE_IQK_A_2);
			result[t][2] = (val32 >> 16) & 0x3ff;if(result[t][2])priv->measured_rx[t]|=1;

			val32 = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
			result[t][3] = (val32 >> 16) & 0x3ff;
			break;
		} else {
			result[t][2] = 0x100;
			result[t][3] = 0x0;
		}
	}

	if (!path_a_ok)
		(void)0;

	if (priv->rf_paths > 1) {
		for (i = 0; i < retry; i++) {
			path_b_ok = rtl8192fu_iqk_path_b(priv);

			if (path_b_ok == 0x01) {
				val32 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_B);
				result[t][4] = (val32 >> 16) & 0x3ff;if(result[t][4])priv->measured_tx[t]|=2;

				val32 = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_B);
				result[t][5] = (val32 >> 16) & 0x3ff;
				break;
			} else {
				result[t][4] = 0x100;
				result[t][5] = 0x0;
			}
		}

		for (i = 0; i < retry; i++) {
			path_b_ok = rtl8192fu_rx_iqk_path_b(priv);

			if (path_b_ok == 0x03) {
				val32 = rtl8xxxu_read32(priv, REG_RX_POWER_BEFORE_IQK_B_2);
				result[t][6] = (val32 >> 16) & 0x3ff;if(result[t][6])priv->measured_rx[t]|=2;

				val32 = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_B_2);
				result[t][7] = (val32 >> 16) & 0x3ff;
				break;
			} else {
				result[t][6] = 0x100;
				result[t][7] = 0x0;
			}
		}

		if (!path_b_ok)
			(void)0;
	}

	
	rtl8xxxu_write32_mask(priv, REG_FPGA0_IQK, 0xffffff00, 0);

	rtl8xxxu_write32(priv, REG_FPGA0_ANALOG4, 0xcc0000c0);

	rtl8xxxu_write32(priv, REG_ANAPWR1, 0x44bbbb44);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x80408040);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x6f005433);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000004e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x04003400);
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000100);

	
	rtl8xxxu_restore_regs(priv, adda_regs, priv->adda_backup,
			      ARRAY_SIZE(adda_regs));

	
	rtl8xxxu_restore_mac_regs(priv, iqk_mac_regs, priv->mac_backup);

	
	rtl8xxxu_restore_regs(priv, iqk_bb_regs, priv->bb_backup, RTL8XXXU_BB_REGS);

	rtl8xxxu_write32_clear(priv, REG_FPGA0_XCD_RF_PARM, BIT(31));

	
	rtl8xxxu_write32_mask(priv, REG_OFDM0_XA_AGC_CORE1, 0xff, 0x50);
	rtl8xxxu_write32_mask(priv, REG_OFDM0_XA_AGC_CORE1, 0xff,
			      rx_initial_gain_a & 0xff);
	if (priv->rf_paths > 1) {
		rtl8xxxu_write32_mask(priv, REG_OFDM0_XB_AGC_CORE1, 0xff, 0x50);
		rtl8xxxu_write32_mask(priv, REG_OFDM0_XB_AGC_CORE1, 0xff,
				      rx_initial_gain_b & 0xff);
	}
}

static void rtl8192fu_phy_iq_calibrate(struct rtw_r92fu *priv)
{
	s32 reg_e94, reg_ea4;
	s32 reg_eb4, reg_ec4;
	
	u32 path_a_0xdf, path_a_0x35;
	u32 path_b_0xdf, path_b_0x35;
	bool path_a_ok, path_b_ok;
	u8 rfe = priv->rfe_type;
	u32 rfe_path_select;
	int result[4][8]; 
	int i, candidate;
	s32 reg_tmp = 0;
	bool simu;
	u32 val32;

	rfe_path_select = rtl8xxxu_read32(priv, REG_RFE_PATH_SELECT);

	path_a_0xdf = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
	path_a_0x35 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_P1);
	path_b_0xdf = rtl8xxxu_read_rfreg(priv, RF_B, RF6052_REG_GAIN_CCA);
	path_b_0x35 = rtl8xxxu_read_rfreg(priv, RF_B, RF6052_REG_GAIN_P1);

	memset(result, 0, sizeof(result));
	candidate = -1;
 memset(priv->measured_tx,0,sizeof(priv->measured_tx));memset(priv->measured_rx,0,sizeof(priv->measured_rx));

	path_a_ok = false;
	path_b_ok = false;

	for (i = 0; i < 3; i++) {
		rtl8192fu_phy_iqcalibrate(priv, result, i);

		if (i == 1) {
			simu = rtl8xxxu_gen2_simularity_compare(priv, result, 0, 1);
			if (simu) {
				candidate = 0;
				break;
			}
		}

		if (i == 2) {
			simu = rtl8xxxu_gen2_simularity_compare(priv, result, 0, 2);
			if (simu) {
				candidate = 0;
				break;
			}

			simu = rtl8xxxu_gen2_simularity_compare(priv, result, 1, 2);
			if (simu) {
				candidate = 1;
			} else {
				for (i = 0; i < 8; i++)
					reg_tmp += result[3][i];

				if (reg_tmp)
					candidate = 3;
				else
					candidate = -1;
			}
		}
	}

	if (candidate >= 0) {
		reg_e94 = result[candidate][0];
		
		reg_ea4 = result[candidate][2];
		
		reg_eb4 = result[candidate][4];
		
		reg_ec4 = result[candidate][6];
		

		(void)0;
		(void)0;

		path_a_ok = true;
		path_b_ok = true;
	}

	if(candidate>=0&&candidate<3){priv->tx_iqk_mask=priv->measured_tx[candidate];priv->rx_iqk_mask=priv->measured_rx[candidate];}
 else if(candidate==3){priv->tx_iqk_mask=priv->measured_tx[0]&priv->measured_tx[1]&priv->measured_tx[2];priv->rx_iqk_mask=priv->measured_rx[0]&priv->measured_rx[1]&priv->measured_rx[2];}
rtl8xxxu_write32_mask(priv, REG_TX_IQK_TONE_A, 0x3ff00000, 0x100);
	rtl8xxxu_write32_mask(priv, REG_NP_ANTA, 0x3ff, 0);
	rtl8xxxu_write32_mask(priv, REG_TX_IQK_TONE_B, 0x3ff00000, 0x100);
	rtl8xxxu_write32_mask(priv, REG_TAP_UPD_97F, 0x3ff, 0);

	if (candidate >= 0) {
		if (reg_e94)
			rtl8xxxu_fill_iqk_matrix_a(priv, path_a_ok, result,
						   candidate, (reg_ea4 == 0));

		if (reg_eb4)
			rtl8xxxu_fill_iqk_matrix_b(priv, path_b_ok, result,
						   candidate, (reg_ec4 == 0));
	}

	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, path_a_0xdf);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_P1, path_a_0x35);
	rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_GAIN_CCA, path_b_0xdf);
	rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_GAIN_P1, path_b_0x35);

	if (rfe == 7 || rfe == 8 || rfe == 9 || rfe == 12) {
		rtl8xxxu_write32_set(priv, REG_SW_GPIO_SHARE_CTRL_1, 0x70000);
		rtl8xxxu_write32_clear(priv, REG_LEDCFG0, 0x6c00000);
		rtl8xxxu_write32_set(priv, REG_PAD_CTRL1, BIT(29) | BIT(28));
		rtl8xxxu_write32_clear(priv, REG_SW_GPIO_SHARE_CTRL_0,
				       0x600000 | BIT(4));

		
		val32 = rtl8xxxu_read32(priv, REG_RFE_BUFFER);
		val32 &= ~BIT(11);
		val32 |= 0x1f;
		rtl8xxxu_write32(priv, REG_RFE_BUFFER, val32);

		if (rfe == 7) {
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANTA_SRC,
					      0xfffff, 0x23200);
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC2,
					      0xfffff, 0x23200);
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC1,
					      0xf000, 0x3);
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC3,
					      0xf000, 0x3);
		} else {
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANTA_SRC,
					      0xfffff, 0x22200);
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC2,
					      0xfffff, 0x22200);
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC1,
					      0xf000, 0x2);
			rtl8xxxu_write32_mask(priv, REG_RFE_CTRL_ANT_SRC3,
					      0xf000, 0x2);
		}

		rtl8xxxu_write32_clear(priv, REG_RFE_OPT62, BIT(2));

		if (rfe == 7)
			rtl8xxxu_write32(priv, REG_RFE_OPT, 0x03000003);

		rtl8xxxu_write32(priv, REG_RFE_PATH_SELECT, rfe_path_select);
	}
}

static void rtl8192fu_disabled_to_emu(struct rtw_r92fu *priv)
{
	rtl8xxxu_write16_clear(priv, REG_APS_FSMCO,
			       APS_FSMCO_HW_POWERDOWN | APS_FSMCO_HW_SUSPEND);

	rtl8xxxu_write32_clear(priv, REG_GPIO_INTM, BIT(16));

	rtl8xxxu_write16_clear(priv, REG_APS_FSMCO,
			       APS_FSMCO_PCIE | APS_FSMCO_HW_SUSPEND);
}

static int rtl8192fu_emu_to_active(struct rtw_r92fu *priv)
{
	u32 val32;
	u16 val16;
	int count;

	
	rtl8xxxu_write8_set(priv, REG_LDOA15_CTRL, LDOA15_ENABLE);

	
	rtl8xxxu_write32_clear(priv, REG_PAD_CTRL1, BIT(28));

	mdelay(1);

	
	rtl8xxxu_write8_clear(priv, REG_SYS_ISO_CTRL, SYS_ISO_ANALOG_IPS);

	val16 = APS_FSMCO_PCIE | APS_FSMCO_HW_SUSPEND | APS_FSMCO_SW_LPS;
	rtl8xxxu_write16_clear(priv, REG_APS_FSMCO, val16);

	
	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
		if (val32 & BIT(17))
			break;

		udelay(10);
	}

	if (!count)
		return -EBUSY;

	rtl8xxxu_write32_set(priv, REG_APS_FSMCO, APS_FSMCO_WLON_RESET);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
		if ((val32 & (APS_FSMCO_MAC_ENABLE | APS_FSMCO_MAC_OFF)) == 0)
			break;

		udelay(10);
	}

	if (!count)
		return -EBUSY;

	
	rtl8xxxu_write32_set(priv, REG_AFE_MISC, BIT(18));

	rtl8xxxu_write16_clear(priv, REG_APS_FSMCO, APS_FSMCO_HW_POWERDOWN);

	rtl8xxxu_write16_clear(priv, REG_APS_FSMCO,
			       APS_FSMCO_PCIE | APS_FSMCO_HW_SUSPEND);

	
	rtl8xxxu_write32_set(priv, REG_LDO_SW_CTRL, BIT(31));

	rtl8xxxu_write16_set(priv, REG_APS_FSMCO, APS_FSMCO_MAC_ENABLE);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
		if ((val32 & APS_FSMCO_MAC_ENABLE) == 0)
			break;

		udelay(10);
	}

	if (!count)
		return -EBUSY;

	
	rtl8xxxu_write8_set(priv, REG_AFE_MISC, AFE_MISC_WL_XTAL_CTRL);

	
	rtl8xxxu_write16_set(priv, REG_GPIO_INTM, GPIO_INTM_EDGE_TRIG_IRQ);

	
	rtl8xxxu_write16_clear(priv, REG_GPIO_IO_SEL_2, GPIO_IO_SEL_2_GPIO09_IRQ);

	
	rtl8xxxu_write16_clear(priv, REG_GPIO_IO_SEL_2, GPIO_IO_SEL_2_GPIO09_INPUT);

	
	rtl8xxxu_write8_set(priv, REG_HSIMR, BIT(0));

	
	rtl8xxxu_write8_clear(priv, REG_MULTI_FUNC_CTRL, MULTI_WIFI_HW_ROF_EN);

	
	rtl8xxxu_write8_set(priv, REG_RSV_CTRL, BIT(7));

	
	rtl8xxxu_write16_set(priv, REG_MULTI_FUNC_CTRL, BIT(14));

	
	rtl8xxxu_write8(priv, REG_RF_CTRL, 0);

	
	rtl8xxxu_write8(priv, REG_AFE_CTRL4 + 3, 0);

	
	rtl8xxxu_write8(priv, REG_RF_CTRL, RF_SDMRSTB | RF_RSTB | RF_ENABLE);

	
	rtl8xxxu_write8(priv, REG_AFE_CTRL4 + 3, RF_SDMRSTB | RF_RSTB | RF_ENABLE);

	
	rtl8xxxu_write8_set(priv, REG_RSVD_1, BIT(5));

	
	rtl8xxxu_write8(priv, REG_RSVD_4, 0xcc);

	
	rtl8xxxu_write8_clear(priv, REG_AFE_XTAL_CTRL, BIT(4) | BIT(3));

	
	rtl8xxxu_write32(priv, REG_GPIO_A0, 0xffffffff);

	
	rtl8xxxu_write8(priv, REG_GPIO_B0, 0xff);

	
	rtl8xxxu_write8_clear(priv, REG_RSV_CTRL, BIT(7));

	return 0;
}

static int rtl8192fu_active_to_emu(struct rtw_r92fu *priv)
{
	u32 val32;
	int count;

	
	rtl8xxxu_write8_clear(priv, REG_SYS_FUNC, SYS_FUNC_BBRSTB);

	
	rtl8xxxu_write16_clear(priv, REG_GPIO_INTM, GPIO_INTM_EDGE_TRIG_IRQ);

	
	rtl8xxxu_write32_set(priv, REG_APS_FSMCO, APS_FSMCO_WLON_RESET);

	
	rtl8xxxu_write16_set(priv, REG_APS_FSMCO, APS_FSMCO_MAC_OFF);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
		if ((val32 & APS_FSMCO_MAC_OFF) == 0)
			break;

		udelay(10);
	}

	if (!count)
		return -EBUSY;

	
	rtl8xxxu_write8_set(priv, REG_SYS_ISO_CTRL, SYS_ISO_ANALOG_IPS);

	
	rtl8xxxu_write8_clear(priv, REG_LDOA15_CTRL, LDOA15_ENABLE);

	return 0;
}

static int rtl8192fu_emu_to_disabled(struct rtw_r92fu *priv)
{
	u16 val16;

	
	rtl8xxxu_write8(priv, REG_APS_FSMCO + 3, 0x20);

	
	val16 = rtl8xxxu_read16(priv, REG_APS_FSMCO);
	val16 &= ~APS_FSMCO_PCIE;
	val16 |= APS_FSMCO_HW_SUSPEND;
	rtl8xxxu_write16(priv, REG_APS_FSMCO, val16);

	
	rtl8xxxu_write32_set(priv, REG_GPIO_INTM, BIT(16));

	return 0;
}

static int rtl8192fu_active_to_lps(struct rtw_r92fu *priv)
{
	
	u16 val16;
	u32 val32;
	int retry;

	
	rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);

	retry = 100;

	
	do {
		val32 = rtl8xxxu_read32(priv, REG_SCH_TX_CMD);
		if (!val32)
			break;

		udelay(10);
	} while (retry--);

	if (!retry) {
		(void)0;
		return -EBUSY;
	}

	
	rtl8xxxu_write8_clear(priv, REG_SYS_FUNC, SYS_FUNC_BBRSTB);

	udelay(2);

	
	rtl8xxxu_write8_clear(priv, REG_SYS_FUNC, SYS_FUNC_BB_GLB_RSTN);

	
	val16 = rtl8xxxu_read16(priv, REG_CR);
	val16 &= 0xff00;
	val16 |= CR_HCI_RXDMA_ENABLE | CR_HCI_TXDMA_ENABLE;
	val16 &= ~CR_SECURITY_ENABLE;
	rtl8xxxu_write16(priv, REG_CR, val16);

	
	rtl8xxxu_write8_set(priv, REG_DUAL_TSF_RST, DUAL_TSF_TX_OK);

	return 0;
}

static int rtl8192fu_power_on(struct rtw_r92fu *priv)
{
	u16 val16;
	int ret;

	rtl8xxxu_write8(priv, REG_USB_ACCESS_TIMEOUT, 0x80);

	rtl8192fu_disabled_to_emu(priv);

	ret = rtl8192fu_emu_to_active(priv);
	if (ret)
		return ret;

	rtl8xxxu_write16(priv, REG_CR, 0);

	val16 = rtl8xxxu_read16(priv, REG_CR);

	val16 |= CR_HCI_TXDMA_ENABLE | CR_HCI_RXDMA_ENABLE |
		 CR_TXDMA_ENABLE | CR_RXDMA_ENABLE |
		 CR_PROTOCOL_ENABLE | CR_SCHEDULE_ENABLE |
		 CR_SECURITY_ENABLE | CR_CALTIMER_ENABLE;
	rtl8xxxu_write16(priv, REG_CR, val16);

	rtl8xxxu_write8(priv,REG_TXPAUSE,0xff);return 0;
}

static void rtl8192f_reset_8051(struct rtw_r92fu *priv)
{
	rtl8xxxu_write8_clear(priv, REG_RSV_CTRL, BIT(1));

	rtl8xxxu_write8_clear(priv, REG_RSV_CTRL + 1, BIT(0));

	rtl8xxxu_write16_clear(priv, REG_SYS_FUNC, SYS_FUNC_CPU_ENABLE);

	rtl8xxxu_write8_clear(priv, REG_RSV_CTRL, BIT(1));

	rtl8xxxu_write8_set(priv, REG_RSV_CTRL + 1, BIT(0));

	rtl8xxxu_write16_set(priv, REG_SYS_FUNC, SYS_FUNC_CPU_ENABLE);
}

static void rtl8192f_enable_rf(struct rtw_r92fu *priv)
{
	u32 val32;

	rtl8xxxu_write8(priv, REG_RF_CTRL, RF_ENABLE | RF_RSTB | RF_SDMRSTB);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TRX_PATH_ENABLE);
	val32 &= ~(OFDM_RF_PATH_RX_MASK | OFDM_RF_PATH_TX_MASK);
	val32 |= OFDM_RF_PATH_RX_A | OFDM_RF_PATH_RX_B |
		 OFDM_RF_PATH_TX_A | OFDM_RF_PATH_TX_B;
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, val32);

	rtl8xxxu_write8(priv, REG_TXPAUSE, 0x00);
}

static void rtl8192f_disable_rf(struct rtw_r92fu *priv)
{
	u32 val32;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TRX_PATH_ENABLE);
	val32 &= ~OFDM_RF_PATH_TX_MASK;
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, val32);

	
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_AC, 0);
}

static void rtl8192f_usb_quirks(struct rtw_r92fu *priv)
{
	u16 val16;

	rtl8xxxu_gen2_usb_quirks(priv);

	val16 = rtl8xxxu_read16(priv, REG_CR);
	val16 |= (CR_MAC_TX_ENABLE | CR_MAC_RX_ENABLE);
	rtl8xxxu_write16(priv, REG_CR, val16);
}

static void rtl8192f_set_crystal_cap(struct rtw_r92fu *priv, u8 crystal_cap)
{
	struct rtl8xxxu_cfo_tracking *cfo = &priv->cfo_tracking;
	u32 xtal1, xtal0;

	if (crystal_cap == cfo->crystal_cap)
		return;

	xtal1 = rtl8xxxu_read32(priv, REG_AFE_PLL_CTRL);
	xtal0 = rtl8xxxu_read32(priv, REG_AFE_XTAL_CTRL);

	(void)0;

	u32p_replace_bits(&xtal1, crystal_cap, XTAL1);
	u32p_replace_bits(&xtal0, crystal_cap, XTAL0);
	rtl8xxxu_write32(priv, REG_AFE_PLL_CTRL, xtal1);
	rtl8xxxu_write32(priv, REG_AFE_XTAL_CTRL, xtal0);

	cfo->crystal_cap = crystal_cap;
}

static void rtl8188f_phy_lc_calibrate(struct rtw_r92fu *priv)
{
	u32 val32;
 u8 saved_pause=rtl8xxxu_read8(priv,REG_TXPAUSE);
	u32 rf_amode, lstf;
	int i;

	
	lstf = rtl8xxxu_read32(priv, REG_OFDM1_LSTF);

	if (lstf & OFDM_LSTF_MASK) {
		
		val32 = lstf & ~OFDM_LSTF_MASK;
		rtl8xxxu_write32(priv, REG_OFDM1_LSTF, val32);
	} else {
		
		
		rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);
	}

	
	rf_amode = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG);

	
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, rf_amode | 0x08000);

	for (i = 0; i < 100; i++) {
		if ((rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG) & 0x08000) == 0)
			break;
		msleep(10);
	}

	if (i == 100) priv->cal_timeout=true;

	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, rf_amode);

	
	if (lstf & OFDM_LSTF_MASK)
		rtl8xxxu_write32(priv, REG_OFDM1_LSTF, lstf);
	else 
		rtl8xxxu_write8(priv, REG_TXPAUSE, saved_pause);
}

static void rtl8188f_channel_to_group(int channel, int *group, int *cck_group)
{
	if (channel < 3)
		*group = 0;
	else if (channel < 6)
		*group = 1;
	else if (channel < 9)
		*group = 2;
	else if (channel < 12)
		*group = 3;
	else
		*group = 4;

	if (channel == 14)
		*cck_group = 5;
	else
		*cck_group = *group;
}
