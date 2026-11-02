/* SPDX-License-Identifier: GPL-2.0-only */
/* Derived from Linux rtl8xxxu at 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Copyright(c)2023 Bitterblue Smith,2014-2017 Jes Sorensen,
 * 2007-2011 Realtek Corporation. Retain RTL8710BU-LICENSE.txt and manifest. */
static int rtl8xxxu_flush_fifo(struct rtw_r10bu *priv);
static u32 rtl8xxxu_read_rfreg(struct rtw_r10bu *priv,
			enum rtl8xxxu_rfpath path, u8 reg);
static int rtl8xxxu_write_rfreg(struct rtw_r10bu *priv,
			 enum rtl8xxxu_rfpath path, u8 reg, u32 data);
static int rtl8xxxu_init_phy_regs(struct rtw_r10bu *priv,
			   const struct rtl8xxxu_reg32val *array);
static int rtl8xxxu_init_rf_regs(struct rtw_r10bu *priv,
				 const struct rtl8xxxu_rfregval *array,
				 enum rtl8xxxu_rfpath path);
static int rtl8xxxu_init_phy_rf(struct rtw_r10bu *priv,
			 const struct rtl8xxxu_rfregval *table,
			 enum rtl8xxxu_rfpath path);
static void rtl8xxxu_save_regs(struct rtw_r10bu *priv, const u32 *regs,
			u32 *backup, int count);
static void rtl8xxxu_restore_regs(struct rtw_r10bu *priv, const u32 *regs,
			   u32 *backup, int count);
static void
rtl8xxxu_save_mac_regs(struct rtw_r10bu *priv, const u32 *reg, u32 *backup);
static void rtl8xxxu_restore_mac_regs(struct rtw_r10bu *priv,
			       const u32 *reg, u32 *backup);
static void rtl8xxxu_path_adda_on(struct rtw_r10bu *priv, const u32 *regs,
			   bool path_a_on);
static void rtl8xxxu_fill_iqk_matrix_a(struct rtw_r10bu *priv, bool iqk_ok,
				int result[][8], int candidate, bool tx_only);
static bool rtl8xxxu_gen2_simularity_compare(struct rtw_r10bu *priv,
				      int result[][8], int c1, int c2);
static int rtl8xxxu_auto_llt_table(struct rtw_r10bu *priv);
static void rtl8xxxu_init_queue_reserved_page(struct rtw_r10bu *priv);
static int rtl8xxxu_init_queue_priority(struct rtw_r10bu *priv);
static void
rtl8xxxu_set_retry(struct rtw_r10bu *priv, u16 short_retry, u16 long_retry);
static void
rtl8xxxu_set_spec_sifs(struct rtw_r10bu *priv, u16 cck, u16 ofdm);
static int
rtl8xxxu_init_mac(struct rtw_r10bu *priv);
static int rtl8xxxu_init_phy_bb(struct rtw_r10bu *priv);
static void rtl8xxxu_gen2_usb_quirks(struct rtw_r10bu *priv);
static void rtl8xxxu_init_burst(struct rtw_r10bu *priv);
static void rtl8xxxu_request_hw_feature(struct rtw_r10bu *priv);
static int rtl8xxxu_dump_hw_feature(struct rtw_r10bu *priv);
static int rtl8xxxu_start_firmware(struct rtw_r10bu *priv);
static int rtl8xxxu_download_firmware(struct rtw_r10bu *priv);
static int rtl8xxxu_init_device(struct ieee80211_hw *hw);
static u32 rtl8710b_indirect_read32(struct rtw_r10bu *priv, u32 addr);
static void rtl8710b_indirect_write32(struct rtw_r10bu *priv, u32 addr, u32 val);
static u32 rtl8710b_read_syson_reg(struct rtw_r10bu *priv, u32 addr);
static void rtl8710b_write_syson_reg(struct rtw_r10bu *priv, u32 addr, u32 val);
static void rtl8710bu_power_off(struct rtw_r10bu *priv);
static void rtl8710b_revise_cck_tx_psf(struct rtw_r10bu *priv, u8 channel);
static void rtl8710bu_config_channel(struct ieee80211_hw *hw);
static void rtl8710bu_init_aggregation(struct rtw_r10bu *priv);
static void rtl8710bu_init_statistics(struct rtw_r10bu *priv);
static void rtl8710bu_init_phy_bb(struct rtw_r10bu *priv);
static int rtl8710bu_init_phy_rf(struct rtw_r10bu *priv);
static int rtl8710bu_iqk_path_a(struct rtw_r10bu *priv, u32 *lok_result);
static int rtl8710bu_rx_iqk_path_a(struct rtw_r10bu *priv, u32 lok_result);
static void rtl8710bu_phy_iqcalibrate(struct rtw_r10bu *priv,
				      int result[][8], int t);
static void rtl8710bu_phy_iq_calibrate(struct rtw_r10bu *priv);
static int rtl8710b_emu_to_active(struct rtw_r10bu *priv);
static int rtl8710bu_active_to_emu(struct rtw_r10bu *priv);
static int rtl8710bu_active_to_lps(struct rtw_r10bu *priv);
static int rtl8710bu_power_on(struct rtw_r10bu *priv);
static void rtl8710b_reset_8051(struct rtw_r10bu *priv);
static void rtl8710b_enable_rf(struct rtw_r10bu *priv);
static void rtl8710b_disable_rf(struct rtw_r10bu *priv);
static void rtl8710b_usb_quirks(struct rtw_r10bu *priv);
static void rtl8710b_set_crystal_cap(struct rtw_r10bu *priv, u8 crystal_cap);
static void
rtl8188f_set_tx_power(struct rtw_r10bu *priv, int channel, bool ht40);
static void rtl8188f_phy_lc_calibrate(struct rtw_r10bu *priv);
static void rtl8188f_channel_to_group(int channel, int *group, int *cck_group);

static int rtl8xxxu_flush_fifo(struct rtw_r10bu *priv)
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

static u32 rtl8xxxu_read_rfreg(struct rtw_r10bu *priv,
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

static int rtl8xxxu_write_rfreg(struct rtw_r10bu *priv,
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

static int rtl8xxxu_init_phy_regs(struct rtw_r10bu *priv,
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

static int rtl8xxxu_init_rf_regs(struct rtw_r10bu *priv,
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

static int rtl8xxxu_init_phy_rf(struct rtw_r10bu *priv,
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

static void rtl8xxxu_save_regs(struct rtw_r10bu *priv, const u32 *regs,
			u32 *backup, int count)
{
	int i;

	for (i = 0; i < count; i++)
		backup[i] = rtl8xxxu_read32(priv, regs[i]);
}

static void rtl8xxxu_restore_regs(struct rtw_r10bu *priv, const u32 *regs,
			   u32 *backup, int count)
{
	int i;

	for (i = 0; i < count; i++)
		rtl8xxxu_write32(priv, regs[i], backup[i]);
}

static void
rtl8xxxu_save_mac_regs(struct rtw_r10bu *priv, const u32 *reg, u32 *backup)
{
	int i;

	for (i = 0; i < (RTL8XXXU_MAC_REGS - 1); i++)
		backup[i] = rtl8xxxu_read8(priv, reg[i]);

	backup[i] = rtl8xxxu_read32(priv, reg[i]);
}

static void rtl8xxxu_restore_mac_regs(struct rtw_r10bu *priv,
			       const u32 *reg, u32 *backup)
{
	int i;

	for (i = 0; i < (RTL8XXXU_MAC_REGS - 1); i++)
		rtl8xxxu_write8(priv, reg[i], backup[i]);

	rtl8xxxu_write32(priv, reg[i], backup[i]);
}

static void rtl8xxxu_path_adda_on(struct rtw_r10bu *priv, const u32 *regs,
			   bool path_a_on)
{
	u32 path_on;
	int i;

	if (priv->tx_paths == 1) {
		path_on = 0x03c00016;
		rtl8xxxu_write32(priv, regs[0], 0x03c00016);
	} else {
		path_on = path_a_on ? 0 :
			0;

		rtl8xxxu_write32(priv, regs[0], path_on);
	}

	for (i = 1 ; i < RTL8XXXU_ADDA_REGS ; i++)
		rtl8xxxu_write32(priv, regs[i], path_on);
}

static void rtl8xxxu_fill_iqk_matrix_a(struct rtw_r10bu *priv, bool iqk_ok,
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

static bool rtl8xxxu_gen2_simularity_compare(struct rtw_r10bu *priv,
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

static int rtl8xxxu_auto_llt_table(struct rtw_r10bu *priv)
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

static void rtl8xxxu_init_queue_reserved_page(struct rtw_r10bu *priv)
{
	
	u32 hq, lq, nq, eq, pubq;
	u32 val32;

	hq = 0;
	lq = 0;
	nq = 0;
	eq = 0;
	pubq = 0;

	if (priv->ep_tx_high_queue)
		hq = TX_PAGE_NUM_HI_PQ_8723B;
	if (priv->ep_tx_low_queue)
		lq = TX_PAGE_NUM_LO_PQ_8723B;
	if (priv->ep_tx_normal_queue)
		nq = TX_PAGE_NUM_NORM_PQ_8723B;

	val32 = (nq << RQPN_NPQ_SHIFT) | (eq << RQPN_EPQ_SHIFT);
	rtl8xxxu_write32(priv, REG_RQPN_NPQ, val32);

	pubq = TX_TOTAL_PAGE_NUM_8723B - hq - lq - nq - 1;

	val32 = RQPN_LOAD;
	val32 |= (hq << RQPN_HI_PQ_SHIFT);
	val32 |= (lq << RQPN_LO_PQ_SHIFT);
	val32 |= (pubq << RQPN_PUB_PQ_SHIFT);

	rtl8xxxu_write32(priv, REG_RQPN, val32);
}

static int rtl8xxxu_init_queue_priority(struct rtw_r10bu *priv)
{
	u16 val16, hi, lo;
	u16 hiq, mgq, bkq, beq, viq, voq;
	int hip, mgp, bkp, bep, vip, vop;
	int ret = 0;


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
			val16 = rtl8xxxu_read16(priv, REG_TRXDMA_CTRL);
			val16 &= 0x7;
			val16 |= (voq << TRXDMA_CTRL_VOQ_SHIFT) |
				 (viq << TRXDMA_CTRL_VIQ_SHIFT) |
				 (beq << TRXDMA_CTRL_BEQ_SHIFT) |
				 (bkq << TRXDMA_CTRL_BKQ_SHIFT) |
				 (mgq << TRXDMA_CTRL_MGQ_SHIFT) |
				 (hiq << TRXDMA_CTRL_HIQ_SHIFT);
			rtl8xxxu_write16(priv, REG_TRXDMA_CTRL, val16);
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
rtl8xxxu_set_retry(struct rtw_r10bu *priv, u16 short_retry, u16 long_retry)
{
	u16 val16;

	val16 = ((short_retry << RETRY_LIMIT_SHORT_SHIFT) &
		 RETRY_LIMIT_SHORT_MASK) |
		((long_retry << RETRY_LIMIT_LONG_SHIFT) &
		 RETRY_LIMIT_LONG_MASK);

	rtl8xxxu_write16(priv, REG_RETRY_LIMIT, val16);
}

static void
rtl8xxxu_set_spec_sifs(struct rtw_r10bu *priv, u16 cck, u16 ofdm)
{
	u16 val16;

	val16 = ((cck << SPEC_SIFS_CCK_SHIFT) & SPEC_SIFS_CCK_MASK) |
		((ofdm << SPEC_SIFS_OFDM_SHIFT) & SPEC_SIFS_OFDM_MASK);

	rtl8xxxu_write16(priv, REG_SPEC_SIFS, val16);
}

static int
rtl8xxxu_init_mac(struct rtw_r10bu *priv)
{
	const struct rtl8xxxu_reg8val *array = rtl8710b_mac_init_table;
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

	switch (RTL8710B) {
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

static int rtl8xxxu_init_phy_bb(struct rtw_r10bu *priv)
{
	u32 val32;

	rtl8710bu_init_phy_bb(priv);

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

	rtl8710b_set_crystal_cap(priv, priv->default_crystal_cap);

	{}

	return 0;
}

static void rtl8xxxu_gen2_usb_quirks(struct rtw_r10bu *priv)
{
	u32 val32;

	val32 = rtl8xxxu_read32(priv, REG_TXDMA_OFFSET_CHK);
	val32 |= TXDMA_OFFSET_DROP_DATA_EN;
	rtl8xxxu_write32(priv, REG_TXDMA_OFFSET_CHK, val32);
}

static void rtl8xxxu_init_burst(struct rtw_r10bu *priv)
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

	rtl8xxxu_write16(priv, REG_MAX_AGGR_NUM, 0x0c14);
	rtl8xxxu_write8(priv, REG_AMPDU_MAX_TIME_8723B,
			0x5e);
	rtl8xxxu_write32(priv, REG_AGGLEN_LMT, 0xffffffff);
	rtl8xxxu_write8(priv, REG_RX_PKT_LIMIT, 0x18);
	rtl8xxxu_write8(priv, REG_PIFS, 0x00);
	{
		rtl8xxxu_write8(priv, REG_FWHW_TXQ_CTRL, FWHW_TXQ_CTRL_AMPDU_RETRY);
		rtl8xxxu_write32(priv, REG_FAST_EDCA_CTRL, 0x03086666);
	}
	rtl8xxxu_write8(priv, REG_USTIME_TSF_8723B, 0x28);
	rtl8xxxu_write8(priv, REG_USTIME_EDCA, 0x28);

	
	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL);
	val8 |= RSV_CTRL_WLOCK_1C | RSV_CTRL_DIS_PRST;
	rtl8xxxu_write8(priv, REG_RSV_CTRL, val8);
}

static void rtl8xxxu_request_hw_feature(struct rtw_r10bu *priv)
{
	return;

	rtl8xxxu_write8(priv, REG_C2HEVT_MSG_NORMAL, C2H_HW_FEATURE_DUMP);
}

static int rtl8xxxu_dump_hw_feature(struct rtw_r10bu *priv)
{
 struct rtl8xxxu_hw_feature *hw_feature=&priv->hw_feature;
		hw_feature->max_bw = 40;
		return 0;
	
}
static int rtl8xxxu_start_firmware(struct rtw_r10bu *priv)
{
	
	u16 reg_mcu_fw_dl;
	int ret = 0, i;
	u32 val32;

	reg_mcu_fw_dl = REG_8051FW_CTRL_V1_8710B;

	
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

	
	rtl8710b_reset_8051(priv);

	
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

static int rtl8xxxu_download_firmware(struct rtw_r10bu *priv)
{
	int pages, remainder, i, ret;
	u16 reg_fw_start_address;
	u16 reg_mcu_fw_dl;
	u8 val8;
	u16 val16;
	u32 val32;
	const u8 *fwptr;

	reg_fw_start_address = REG_FW_START_ADDRESS;

	{
		reg_mcu_fw_dl = REG_8051FW_CTRL_V1_8710B;
	}

	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	if (val8 & MCU_FW_RAM_SEL) {
		(void)0;
		rtl8xxxu_write8(priv, reg_mcu_fw_dl, 0x00);
		rtl8710b_reset_8051(priv);
	}

	
	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	val8 |= MCU_FW_DL_ENABLE;
	rtl8xxxu_write8(priv, reg_mcu_fw_dl, val8);

	
	val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
	val32 &= ~BIT(19);
	rtl8xxxu_write32(priv, reg_mcu_fw_dl, val32);

	{
		
		val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl + 1);
		val8 |= BIT(0);
		rtl8xxxu_write8(priv, reg_mcu_fw_dl + 1, val8);
	}

	
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
	struct rtw_r10bu *priv = hw->priv;
	
	
	
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

	ret = rtl8710bu_power_on(priv);
	if (ret < 0) {
		(void)0;
		goto exit;
	}

	rtl8xxxu_init_queue_reserved_page(priv);

	ret = rtl8xxxu_init_queue_priority(priv);
	(void)0;
	if (ret)
		goto exit;

	
	rtl8xxxu_write16(priv, REG_TRXFF_BNDY + 2, 0x3f7f);

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

	ret = rtl8710bu_init_phy_rf(priv);
	if (ret)
		goto exit;

	
	{}

	
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
		
		val8 = TX_TOTAL_PAGE_NUM_8723B + 1;

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

		
		rtl8710b_usb_quirks(priv);

		
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

	{
		rtl8xxxu_write32(priv, REG_HIMR0_8710B, 0);
	}

	
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

	rtl8710bu_init_aggregation(priv);

	{
		rtl8xxxu_write16(priv, REG_PKT_VO_VI_LIFE_TIME, 0x0400); 
		rtl8xxxu_write16(priv, REG_PKT_BE_BK_LIFE_TIME, 0x0400); 
	}

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
	val32 |= (FPGA_RF_MODE_CCK | FPGA_RF_MODE_OFDM);
	rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

	
	rtl8xxxu_write32(priv, REG_CAM_CMD, CAM_CMD_POLLING | BIT(30));

	
	rtl8188f_set_tx_power(priv, 1, false);

	
	{}

	rtl8xxxu_write8(priv, REG_HWSEQ_CTRL, 0xff);

	
	rtl8xxxu_write32(priv, REG_BAR_MODE_CTRL, 0x0201ffff);

	{}

	rtl8710bu_init_statistics(priv);

	{}

	{}

	rtl8188f_phy_lc_calibrate(priv);

	rtl8710bu_phy_iq_calibrate(priv);

	
	{
		{
			val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_T_METER_8723B);
			val32 |= 0x30000;
			rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_T_METER_8723B, val32);
		}
	}

	
	val8 = ((30000 + NAV_UPPER_UNIT - 1) / NAV_UPPER_UNIT);
	rtl8xxxu_write8(priv, REG_NAV_UPPER, val8);

	{}

	val32 = rtl8xxxu_read32(priv, REG_FWHW_TXQ_CTRL);
	val32 |= FWHW_TXQ_CTRL_XMIT_MGMT_ACK;
	
	rtl8xxxu_write32(priv, REG_FWHW_TXQ_CTRL, val32);

	{}

	{
		
		val8 = rtl8xxxu_read8(priv, REG_PORT_CONTROL_8710B);
		val8 |= BIT(5) | BIT(4);
		rtl8xxxu_write8(priv, REG_PORT_CONTROL_8710B, val8);

		
		val32 = rtl8xxxu_read32(priv, REG_WL_RF_PSS_8710B);
		val32 |= 0x107;
		rtl8xxxu_write32(priv, REG_WL_RF_PSS_8710B, val32);
	}

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

static u32 rtl8710b_indirect_read32(struct rtw_r10bu *priv, u32 addr)
{
	
	u32 val32, value = 0xffffffff;
	u8 polling_count = 0xff;

	if (!IS_ALIGNED(addr, 4)) {
		(void)0;
		return value;
	}

	

	rtl8xxxu_write32(priv, REG_USB_HOST_INDIRECT_ADDR_8710B, addr);
	rtl8xxxu_write32(priv, REG_EFUSE_INDIRECT_CTRL_8710B, NORMAL_REG_READ_OFFSET);

	do
		val32 = rtl8xxxu_read32(priv, REG_EFUSE_INDIRECT_CTRL_8710B);
	while ((val32 & BIT(31)) && (--polling_count > 0));

	if (polling_count == 0 && !priv->error) priv->error=ETIMEDOUT;
	else
		value = rtl8xxxu_read32(priv, REG_USB_HOST_INDIRECT_DATA_8710B);

	

	

	return value;
}

static void rtl8710b_indirect_write32(struct rtw_r10bu *priv, u32 addr, u32 val)
{
	
	u8 polling_count = 0xff;
	u32 val32;

	if (!IS_ALIGNED(addr, 4)) {
		(void)0;
		return;
	}

	

	rtl8xxxu_write32(priv, REG_USB_HOST_INDIRECT_ADDR_8710B, addr);
	rtl8xxxu_write32(priv, REG_USB_HOST_INDIRECT_DATA_8710B, val);
	rtl8xxxu_write32(priv, REG_EFUSE_INDIRECT_CTRL_8710B, NORMAL_REG_WRITE_OFFSET);

	do
		val32 = rtl8xxxu_read32(priv, REG_EFUSE_INDIRECT_CTRL_8710B);
	while ((val32 & BIT(31)) && (--polling_count > 0));

	if (polling_count == 0 && !priv->error) priv->error=ETIMEDOUT;

	

	
}

static u32 rtl8710b_read_syson_reg(struct rtw_r10bu *priv, u32 addr)
{
	return rtl8710b_indirect_read32(priv, addr | SYSON_REG_BASE_ADDR_8710B);
}

static void rtl8710b_write_syson_reg(struct rtw_r10bu *priv, u32 addr, u32 val)
{
	rtl8710b_indirect_write32(priv, addr | SYSON_REG_BASE_ADDR_8710B, val);
}

static void rtl8710bu_power_off(struct rtw_r10bu *priv)
{
	u32 val32;
	u8 val8;

	shutdown_result(priv,rtl8xxxu_flush_fifo(priv));

	rtl8xxxu_write32(priv, REG_HISR0_8710B, 0xffffffff);
	rtl8xxxu_write32(priv, REG_HIMR0_8710B, 0x0);

	
	val32 = rtl8710b_read_syson_reg(priv, 0x138);
	val32 |= BIT(5);
	rtl8710b_write_syson_reg(priv, 0x138, val32);

	
	rtl8xxxu_write8(priv, REG_CR, 0x00);

	shutdown_result(priv,rtl8710bu_active_to_lps(priv));

	
	val8 = rtl8xxxu_read8(priv, REG_8051FW_CTRL_V1_8710B + 3);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, REG_8051FW_CTRL_V1_8710B + 3, val8);

	
	rtl8xxxu_write8(priv, REG_8051FW_CTRL_V1_8710B, 0x00);

	shutdown_result(priv,rtl8710bu_active_to_emu(priv));
}

static void rtl8710b_revise_cck_tx_psf(struct rtw_r10bu *priv, u8 channel)
{
	if (channel == 13) {
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER2, 0x64B80C1C);
		rtl8xxxu_write32(priv, REG_CCK0_DEBUG_PORT, 0x00008810);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER3, 0x01235667);
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER1, 0xd1d80001);
	} else if (channel == 14) {
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER2, 0x0000B81C);
		rtl8xxxu_write32(priv, REG_CCK0_DEBUG_PORT, 0x00000000);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER3, 0x00003667);
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER1, 0xE82C0001);
	} else {
		
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER2, 0x64B80C1C);
		rtl8xxxu_write32(priv, REG_CCK0_DEBUG_PORT, 0x00008810);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER3, 0x01235667);
		rtl8xxxu_write32(priv, REG_CCK0_TX_FILTER1, 0xE82C0001);
	}
}

static void rtl8710bu_config_channel(struct ieee80211_hw *hw)
{
	struct rtw_r10bu *priv = hw->priv;
	bool ht40 = conf_is_ht40(&hw->conf);
	u8 channel, subchannel = 0;
	bool sec_ch_above = 0;
	u32 val32;
	u16 val16;

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
	u32p_replace_bits(&val32, channel, MODE_AG_CHANNEL_MASK);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, val32);

	rtl8710b_revise_cck_tx_psf(priv, channel);

	
	val16 = rtl8xxxu_read16(priv, REG_WMAC_TRXPTCL_CTL);
	val16 &= ~WMAC_TRXPTCL_CTL_BW_MASK;
	if (ht40)
		val16 |= WMAC_TRXPTCL_CTL_BW_40;
	rtl8xxxu_write16(priv, REG_WMAC_TRXPTCL_CTL, val16);

	rtl8xxxu_write8(priv, REG_DATA_SUBCHANNEL, subchannel);

	val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
	u32p_replace_bits(&val32, ht40, FPGA_RF_MODE);
	rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

	val32 = rtl8xxxu_read32(priv, REG_FPGA1_RF_MODE);
	u32p_replace_bits(&val32, ht40, FPGA_RF_MODE);
	rtl8xxxu_write32(priv, REG_FPGA1_RF_MODE, val32);

	if (ht40) {
		
		val32 = rtl8xxxu_read32(priv, REG_CCK0_SYSTEM);
		u32p_replace_bits(&val32, !sec_ch_above, CCK0_SIDEBAND);
		rtl8xxxu_write32(priv, REG_CCK0_SYSTEM, val32);
	}

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
	val32 |= GENMASK(10, 8);
	rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
	val32 |= BIT(14) | BIT(12);
	val32 &= ~BIT(13);
	rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT);
	val32 &= ~GENMASK(31, 30);
	rtl8xxxu_write32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT);
	val32 &= ~BIT(29);
	val32 |= BIT(28);
	rtl8xxxu_write32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_RX_AFE);
	val32 &= ~BIT(29);
	val32 |= BIT(28);
	rtl8xxxu_write32(priv, REG_OFDM0_XA_RX_AFE, val32);

	val32 = rtl8xxxu_read32(priv, REG_FPGA0_XB_RF_INT_OE);
	val32 &= ~BIT(30);
	val32 |= BIT(29);
	rtl8xxxu_write32(priv, REG_FPGA0_XB_RF_INT_OE, val32);

	if (ht40) {
		val32 = rtl8xxxu_read32(priv, REG_OFDM_RX_DFIR);
		val32 &= ~BIT(19);
		rtl8xxxu_write32(priv, REG_OFDM_RX_DFIR, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM_RX_DFIR);
		val32 &= ~GENMASK(23, 20);
		rtl8xxxu_write32(priv, REG_OFDM_RX_DFIR, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM_RX_DFIR);
		val32 &= ~GENMASK(27, 24);
		rtl8xxxu_write32(priv, REG_OFDM_RX_DFIR, val32);

		
		val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG);
		val32 &= ~MODE_AG_BW_MASK;
		val32 |= MODE_AG_BW_40MHZ_8723B;
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, val32);
	} else {
		val32 = rtl8xxxu_read32(priv, REG_OFDM_RX_DFIR);
		val32 |= BIT(19);
		rtl8xxxu_write32(priv, REG_OFDM_RX_DFIR, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM_RX_DFIR);
		val32 &= ~GENMASK(23, 20);
		val32 |= BIT(23);
		rtl8xxxu_write32(priv, REG_OFDM_RX_DFIR, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM_RX_DFIR);
		val32 &= ~GENMASK(27, 24);
		val32 |= BIT(27) | BIT(25);
		rtl8xxxu_write32(priv, REG_OFDM_RX_DFIR, val32);

		
		val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG);
		val32 &= ~MODE_AG_BW_MASK;
		val32 |= MODE_AG_BW_20MHZ_8723B;
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, val32);
	}
}

static void rtl8710bu_init_aggregation(struct rtw_r10bu *priv)
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

static void rtl8710bu_init_statistics(struct rtw_r10bu *priv)
{
	u32 val32;

	
	rtl8xxxu_write16(priv, REG_NHM_TIMER_8723B + 2, 0xc350);
	rtl8xxxu_write16(priv, REG_NHM_TH9_TH10_8723B + 2, 0xffff);
	rtl8xxxu_write32(priv, REG_NHM_TH3_TO_TH0_8723B, 0xffffff50);
	rtl8xxxu_write32(priv, REG_NHM_TH7_TO_TH4_8723B, 0xffffffff);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 |= 0xff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_NHM_TH9_TH10_8723B);
	val32 &= ~(BIT(8) | BIT(9) | BIT(10));
	val32 |= BIT(8);
	rtl8xxxu_write32(priv, REG_NHM_TH9_TH10_8723B, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_FA_RSTC);
	val32 |= BIT(7);
	rtl8xxxu_write32(priv, REG_OFDM0_FA_RSTC, val32);
}

static void rtl8710bu_init_phy_bb(struct rtw_r10bu *priv)
{
	const struct rtl8xxxu_reg32val *phy_init_table;
	u32 val32;

	
	val32 = rtl8xxxu_read32(priv, REG_SYS_FUNC_8710B);
	val32 |= GENMASK(17, 16) | GENMASK(26, 24);
	rtl8xxxu_write32(priv, REG_SYS_FUNC_8710B, val32);

	if (priv->package_type == PACKAGE_QFN48M_U)
		phy_init_table = rtl8710bu_qfn48m_u_phy_init_table;
	else
		phy_init_table = rtl8710bu_qfn48m_s_phy_init_table;

	rtl8xxxu_init_phy_regs(priv, phy_init_table);

	rtl8xxxu_init_phy_regs(priv, rtl8710b_agc_table);
}

static int rtl8710bu_init_phy_rf(struct rtw_r10bu *priv)
{
	const struct rtl8xxxu_rfregval *radioa_init_table;

	if (priv->package_type == PACKAGE_QFN48M_U)
		radioa_init_table = rtl8710bu_qfn48m_u_radioa_init_table;
	else
		radioa_init_table = rtl8710bu_qfn48m_s_radioa_init_table;

	return rtl8xxxu_init_phy_rf(priv, radioa_init_table, RF_A);
}

static int rtl8710bu_iqk_path_a(struct rtw_r10bu *priv, u32 *lok_result)
{
	u32 reg_eac, reg_e94, reg_e9c, val32, path_sel_bb;
	int result = 0;

	path_sel_bb = rtl8xxxu_read32(priv, REG_S0S1_PATH_SWITCH);

	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x99000000);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x20000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0000f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0x07ff7);

	
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
	val32 |= BIT(11);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, val32);
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_PAD_TXG);
	u32p_replace_bits(&val32, 0x1ed, 0x00fff);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_PAD_TXG, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0x808000, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x821403ff);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28160c06);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x02002911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xfa000000);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8000000);

	mdelay(10);

	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel_bb);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
	val32 &= ~BIT(11);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, val32);

	
	*lok_result = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_TXM_IDAC);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_e94 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
	reg_e9c = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);

	if (!(reg_eac & BIT(28)) &&
	    ((reg_e94 & 0x03ff0000) != 0x01420000) &&
	    ((reg_e9c & 0x03ff0000) != 0x00420000))
		result |= 0x01;

	return result;
}

static int rtl8710bu_rx_iqk_path_a(struct rtw_r10bu *priv, u32 lok_result)
{
	u32 reg_ea4, reg_eac, reg_e94, reg_e9c, val32, path_sel_bb, tmp;
	int result = 0;

	path_sel_bb = rtl8xxxu_read32(priv, REG_S0S1_PATH_SWITCH);

	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x99000000);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x30000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0000f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xf1173);

	
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
	val32 |= BIT(11);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, val32);
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_PAD_TXG);
	u32p_replace_bits(&val32, 0xf, 0x003e0);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_PAD_TXG, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0x808000, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x8216129f);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28160c00);

	
	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0046a911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf9000000);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8000000);

	mdelay(10);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_e94 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
	reg_e9c = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);

	if (!(reg_eac & BIT(28)) &&
	    ((reg_e94 & 0x03ff0000) != 0x01420000) &&
	    ((reg_e9c & 0x03ff0000) != 0x00420000)) {
		result |= 0x01;
	} else { 

		
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel_bb);

		
		val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
		u32p_replace_bits(&val32, 0, 0xffffff00);
		rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

		val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
		val32 &= ~BIT(11);
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, val32);

		return result;
	}

	val32 = 0x80007c00 | (reg_e94 & 0x3ff0000) | ((reg_e9c & 0x3ff0000) >> 16);
	rtl8xxxu_write32(priv, REG_TX_IQK, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x30000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0000f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xf7ff2);

	
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
	val32 |= BIT(11);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, val32);
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_PAD_TXG);
	u32p_replace_bits(&val32, 0x2a, 0x00fff);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_PAD_TXG, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0x808000, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x18008c1c);

	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x2816169f);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0046a911);

	
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf9000000);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8000000);

	mdelay(10);

	
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel_bb);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA);
	val32 &= ~BIT(11);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, val32);

	
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXM_IDAC, lok_result);

	
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_ea4 = rtl8xxxu_read32(priv, REG_RX_POWER_BEFORE_IQK_A_2);

	tmp = (reg_eac & 0x03ff0000) >> 16;
	if ((tmp & 0x200) > 0)
		tmp = 0x400 - tmp;

	if (!(reg_eac & BIT(27)) &&
	    ((reg_ea4 & 0x03ff0000) != 0x01320000) &&
	    ((reg_eac & 0x03ff0000) != 0x00360000) &&
	    (((reg_ea4 & 0x03ff0000) >> 16) < 0x11a) &&
	    (((reg_ea4 & 0x03ff0000) >> 16) > 0xe6) &&
	    (tmp < 0x1a))
		result |= 0x02;

	return result;
}

static void rtl8710bu_phy_iqcalibrate(struct rtw_r10bu *priv,
				      int result[][8], int t)
{
	
	u32 i, val32, rx_initial_gain, lok_result;
	u32 path_sel_bb, path_sel_rf;
	int path_a_ok;
	u32 retry = 2;
	static const u32 adda_regs[RTL8XXXU_ADDA_REGS] = {
		REG_FPGA0_XCD_SWITCH_CTRL, REG_BLUETOOTH,
		REG_RX_WAIT_CCA, REG_TX_CCK_RFON,
		REG_TX_CCK_BBON, REG_TX_OFDM_RFON,
		REG_TX_OFDM_BBON, REG_TX_TO_RX,
		REG_TX_TO_TX, REG_RX_CCK,
		REG_RX_OFDM, REG_RX_WAIT_RIFS,
		REG_RX_TO_RX, REG_STANDBY,
		REG_SLEEP, REG_PMPD_ANAEN
	};
	static const u32 iqk_mac_regs[RTL8XXXU_MAC_REGS] = {
		REG_TXPAUSE, REG_BEACON_CTRL,
		REG_BEACON_CTRL_1, REG_GPIO_MUXCFG
	};
	static const u32 iqk_bb_regs[RTL8XXXU_BB_REGS] = {
		REG_OFDM0_TRX_PATH_ENABLE, REG_OFDM0_TR_MUX_PAR,
		REG_FPGA0_XCD_RF_SW_CTRL, REG_CONFIG_ANT_A, REG_CONFIG_ANT_B,
		REG_FPGA0_XAB_RF_SW_CTRL, REG_FPGA0_XA_RF_INT_OE,
		REG_FPGA0_XB_RF_INT_OE, REG_CCK0_AFE_SETTING
	};

	

	rx_initial_gain = rtl8xxxu_read32(priv, REG_OFDM0_XA_AGC_CORE1);

	if (t == 0) {
		
		rtl8xxxu_save_regs(priv, adda_regs, priv->adda_backup,
				   RTL8XXXU_ADDA_REGS);
		rtl8xxxu_save_mac_regs(priv, iqk_mac_regs, priv->mac_backup);
		rtl8xxxu_save_regs(priv, iqk_bb_regs,
				   priv->bb_backup, RTL8XXXU_BB_REGS);
	}

	rtl8xxxu_path_adda_on(priv, adda_regs, true);

	if (t == 0) {
		val32 = rtl8xxxu_read32(priv, REG_FPGA0_XA_HSSI_PARM1);
		priv->pi_enabled = u32_get_bits(val32, FPGA0_HSSI_PARM1_PI);
	}

	if (!priv->pi_enabled) {
		
		rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM1, 0x01000100);
		rtl8xxxu_write32(priv, REG_FPGA0_XB_HSSI_PARM1, 0x01000100);
	}

	
	val32 = rtl8xxxu_read32(priv, REG_TX_PTCL_CTRL);
	val32 |= 0x00ff0000;
	rtl8xxxu_write32(priv, REG_TX_PTCL_CTRL, val32);

	
	path_sel_bb = rtl8xxxu_read32(priv, REG_S0S1_PATH_SWITCH);
	path_sel_rf = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_S0S1);

	
	val32 = rtl8xxxu_read32(priv, REG_CCK0_AFE_SETTING);
	val32 |= 0x0f000000;
	rtl8xxxu_write32(priv, REG_CCK0_AFE_SETTING, val32);
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, 0x03c00010);
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x03a05601);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000800e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x25204000);

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0x808000, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);
	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	for (i = 0; i < retry; i++) {
		path_a_ok = rtl8710bu_iqk_path_a(priv, &lok_result);

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
		path_a_ok = rtl8710bu_rx_iqk_path_a(priv, lok_result);

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

	
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	u32p_replace_bits(&val32, 0, 0xffffff00);
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	if (t == 0)
		return;

	
	rtl8xxxu_restore_regs(priv, adda_regs, priv->adda_backup, RTL8XXXU_ADDA_REGS);

	
	rtl8xxxu_restore_mac_regs(priv, iqk_mac_regs, priv->mac_backup);

	
	rtl8xxxu_restore_regs(priv, iqk_bb_regs, priv->bb_backup, RTL8XXXU_BB_REGS);

	
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel_bb);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_S0S1, path_sel_rf);

	
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_AGC_CORE1);
	u32p_replace_bits(&val32, 0x50, 0x000000ff);
	rtl8xxxu_write32(priv, REG_OFDM0_XA_AGC_CORE1, val32);
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_AGC_CORE1);
	u32p_replace_bits(&val32, rx_initial_gain & 0xff, 0x000000ff);
	rtl8xxxu_write32(priv, REG_OFDM0_XA_AGC_CORE1, val32);

	
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x01008c00);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x01008c00);
}

static void rtl8710bu_phy_iq_calibrate(struct rtw_r10bu *priv)
{
	
	int result[4][8]; 
	int i, candidate;
	bool path_a_ok;
	s32 reg_e94, reg_ea4;
	s32 reg_tmp = 0;
	bool simu;
	u32 path_sel_bb;

	
	path_sel_bb = rtl8xxxu_read32(priv, REG_S0S1_PATH_SWITCH);

	memset(result, 0, sizeof(result));
	candidate = -1;
 memset(priv->measured_tx,0,sizeof(priv->measured_tx));memset(priv->measured_rx,0,sizeof(priv->measured_rx));

	path_a_ok = false;

	for (i = 0; i < 3; i++) {
		rtl8710bu_phy_iqcalibrate(priv, result, i);

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
		

		(void)0;
		(void)0;

		path_a_ok = true;

		if (reg_e94)
			rtl8xxxu_fill_iqk_matrix_a(priv, path_a_ok, result,
						   candidate, (reg_ea4 == 0));
	}

	if(candidate>=0&&candidate<3){priv->tx_iqk_mask=priv->measured_tx[candidate];priv->rx_iqk_mask=priv->measured_rx[candidate];}else if(candidate==3){priv->tx_iqk_mask=priv->measured_tx[0]&priv->measured_tx[1]&priv->measured_tx[2];priv->rx_iqk_mask=priv->measured_rx[0]&priv->measured_rx[1]&priv->measured_rx[2];}
rtl8xxxu_save_regs(priv, rtl8xxxu_iqk_phy_iq_bb_reg,
			   priv->bb_recovery_backup, RTL8XXXU_BB_REGS);

	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel_bb);
}

static int rtl8710b_emu_to_active(struct rtw_r10bu *priv)
{
	u8 val8;
	int count, ret = 0;

	
	val8 = rtl8xxxu_read8(priv, 0x5d);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, 0x5d, val8);

	val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC_8710B);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, REG_SYS_FUNC_8710B, val8);

	rtl8xxxu_write8(priv, 0x56, 0x0e);

	val8 = rtl8xxxu_read8(priv, 0x20);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, 0x20, val8);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val8 = rtl8xxxu_read8(priv, 0x20);
		if (!(val8 & BIT(0)))
			break;

		udelay(10);
	}

	if (!count)
		ret = -EBUSY;

	return ret;
}

static int rtl8710bu_active_to_emu(struct rtw_r10bu *priv)
{
	u8 val8;
	u32 val32;
	int count, ret = 0;

	
	val32 = rtl8xxxu_read32(priv, REG_SYS_FUNC_8710B);
	val32 &= ~GENMASK(26, 24);
	rtl8xxxu_write32(priv, REG_SYS_FUNC_8710B, val32);

	
	val32 = rtl8xxxu_read32(priv, REG_SYS_FUNC_8710B);
	val32 &= ~GENMASK(17, 16);
	rtl8xxxu_write32(priv, REG_SYS_FUNC_8710B, val32);

	
	val8 = rtl8xxxu_read8(priv, 0x20);
	val8 |= BIT(1);
	rtl8xxxu_write8(priv, 0x20, val8);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val8 = rtl8xxxu_read8(priv, 0x20);
		if ((val8 & BIT(1)) == 0) {
			ret = 0;
			break;
		}
		udelay(10);
	}

	if (!count)
		ret = -EBUSY;

	return ret;
}

static int rtl8710bu_active_to_lps(struct rtw_r10bu *priv)
{
	
	u8 val8;
	u16 val16;
	u32 val32;
	int retry, retval;

	
	rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);

	retry = 100;
	retval = -EBUSY;
	
	do {
		val32 = rtl8xxxu_read32(priv, REG_SCH_TX_CMD);
		if (!val32) {
			retval = 0;
			break;
		}
		udelay(10);
	} while (retry--);

	if (val32 != 0) {
		(void)0;
		retval = -EBUSY;
		return retval;
	}

	
	val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC);
	val8 &= ~SYS_FUNC_BBRSTB;
	rtl8xxxu_write8(priv, REG_SYS_FUNC, val8);

	udelay(2);

	
	val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC);
	val8 &= ~SYS_FUNC_BB_GLB_RSTN;
	rtl8xxxu_write8(priv, REG_SYS_FUNC, val8);

	
	val16 = rtl8xxxu_read16(priv, REG_CR);
	val16 &= 0xff00;
	val16 |= CR_HCI_RXDMA_ENABLE | CR_HCI_TXDMA_ENABLE;
	val16 &= ~CR_SECURITY_ENABLE;
	rtl8xxxu_write16(priv, REG_CR, val16);

	
	val8 = rtl8xxxu_read8(priv, REG_DUAL_TSF_RST);
	val8 |= DUAL_TSF_TX_OK;
	rtl8xxxu_write8(priv, REG_DUAL_TSF_RST, val8);

	return retval;
}

static int rtl8710bu_power_on(struct rtw_r10bu *priv)
{
	u32 val32;
	u16 val16;
	u8 val8;
	int ret;

	rtl8xxxu_write8(priv, REG_USB_ACCESS_TIMEOUT, 0x80);

	val8 = rtl8xxxu_read8(priv, REG_SYS_ISO_CTRL);
	val8 &= ~BIT(5);
	rtl8xxxu_write8(priv, REG_SYS_ISO_CTRL, val8);

	val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC_8710B);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, REG_SYS_FUNC_8710B, val8);

	val8 = rtl8xxxu_read8(priv, 0x20);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, 0x20, val8);

	rtl8xxxu_write8(priv, REG_AFE_CTRL_8710B, 0);

	val8 = rtl8xxxu_read8(priv, REG_WL_STATUS_8710B);
	val8 |= BIT(1);
	rtl8xxxu_write8(priv, REG_WL_STATUS_8710B, val8);

	ret = rtl8710b_emu_to_active(priv);
	if (ret)
		return ret;

	rtl8xxxu_write16(priv, REG_CR, 0);

	val16 = rtl8xxxu_read16(priv, REG_CR);

	val16 |= CR_HCI_TXDMA_ENABLE | CR_HCI_RXDMA_ENABLE |
		 CR_TXDMA_ENABLE | CR_RXDMA_ENABLE |
		 CR_PROTOCOL_ENABLE | CR_SCHEDULE_ENABLE |
		 CR_SECURITY_ENABLE | CR_CALTIMER_ENABLE;
	rtl8xxxu_write16(priv, REG_CR, val16);

	
	val8 = rtl8xxxu_read8(priv, REG_HWSEQ_CTRL);
	val8 |= 0x7f;
	rtl8xxxu_write8(priv, REG_HWSEQ_CTRL, val8);

	udelay(2);

	

	val8 = rtl8xxxu_read8(priv, 0xfef9);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, 0xfef9, val8);

	
	val32 = rtl8710b_read_syson_reg(priv, 0x138);
	val32 &= ~BIT(5);
	rtl8710b_write_syson_reg(priv, 0x138, val32);

	rtl8xxxu_write8(priv,REG_TXPAUSE,0xff);return ret;
}

static void rtl8710b_reset_8051(struct rtw_r10bu *priv)
{
	u8 val8;

	val8 = rtl8xxxu_read8(priv, REG_8051FW_CTRL_V1_8710B + 3);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, REG_8051FW_CTRL_V1_8710B + 3, val8);

	udelay(50);

	val8 = rtl8xxxu_read8(priv, REG_8051FW_CTRL_V1_8710B + 3);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, REG_8051FW_CTRL_V1_8710B + 3, val8);
}

static void rtl8710b_enable_rf(struct rtw_r10bu *priv)
{
	u32 val32;

	rtl8xxxu_write8(priv, REG_RF_CTRL, RF_ENABLE | RF_RSTB | RF_SDMRSTB);

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TRX_PATH_ENABLE);
	val32 &= ~(OFDM_RF_PATH_RX_MASK | OFDM_RF_PATH_TX_MASK);
	val32 |= OFDM_RF_PATH_RX_A | OFDM_RF_PATH_TX_A;
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, val32);

	rtl8xxxu_write8(priv, REG_TXPAUSE, 0x00);
}

static void rtl8710b_disable_rf(struct rtw_r10bu *priv)
{
	u32 val32;

	val32 = rtl8xxxu_read32(priv, REG_OFDM0_TRX_PATH_ENABLE);
	val32 &= ~OFDM_RF_PATH_TX_MASK;
	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, val32);

	
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_AC, 0);
}

static void rtl8710b_usb_quirks(struct rtw_r10bu *priv)
{
	u16 val16;

	rtl8xxxu_gen2_usb_quirks(priv);

	val16 = rtl8xxxu_read16(priv, REG_CR);
	val16 |= (CR_MAC_TX_ENABLE | CR_MAC_RX_ENABLE);
	rtl8xxxu_write16(priv, REG_CR, val16);
}

static void rtl8710b_set_crystal_cap(struct rtw_r10bu *priv, u8 crystal_cap)
{
	struct rtl8xxxu_cfo_tracking *cfo = &priv->cfo_tracking;
	u32 val32;

	if (crystal_cap == cfo->crystal_cap)
		return;

	val32 = rtl8710b_read_syson_reg(priv, REG_SYS_XTAL_CTRL0_8710B);

	(void)0;

	u32p_replace_bits(&val32, crystal_cap, XTAL1);
	u32p_replace_bits(&val32, crystal_cap, XTAL0);
	rtl8710b_write_syson_reg(priv, REG_SYS_XTAL_CTRL0_8710B, val32);

	cfo->crystal_cap = crystal_cap;
}

static void
rtl8188f_set_tx_power(struct rtw_r10bu *priv, int channel, bool ht40)
{
	u32 val32, ofdm, mcs;
	u8 cck, ofdmbase, mcsbase;
	int group, cck_group;

	rtl8188f_channel_to_group(channel, &group, &cck_group);

	cck = priv->cck_tx_power_index_A[cck_group];

	val32 = rtl8xxxu_read32(priv, REG_TX_AGC_A_CCK1_MCS32);
	val32 &= 0xffff00ff;
	val32 |= (cck << 8);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_CCK1_MCS32, val32);

	val32 = rtl8xxxu_read32(priv, REG_TX_AGC_B_CCK11_A_CCK2_11);
	val32 &= 0xff;
	val32 |= ((cck << 8) | (cck << 16) | (cck << 24));
	rtl8xxxu_write32(priv, REG_TX_AGC_B_CCK11_A_CCK2_11, val32);

	ofdmbase = priv->ht40_1s_tx_power_index_A[group];
	ofdmbase += priv->ofdm_tx_power_diff[0].a;
	ofdm = ofdmbase | ofdmbase << 8 | ofdmbase << 16 | ofdmbase << 24;

	rtl8xxxu_write32(priv, REG_TX_AGC_A_RATE18_06, ofdm);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_RATE54_24, ofdm);

	mcsbase = priv->ht40_1s_tx_power_index_A[group];
	if (ht40)
		
		mcsbase += priv->ht40_tx_power_diff[0].a;
	else
		mcsbase += priv->ht20_tx_power_diff[0].a;
	mcs = mcsbase | mcsbase << 8 | mcsbase << 16 | mcsbase << 24;

	rtl8xxxu_write32(priv, REG_TX_AGC_A_MCS03_MCS00, mcs);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_MCS07_MCS04, mcs);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_MCS11_MCS08, mcs);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_MCS15_MCS12, mcs);
}

static void rtl8188f_phy_lc_calibrate(struct rtw_r10bu *priv)
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
