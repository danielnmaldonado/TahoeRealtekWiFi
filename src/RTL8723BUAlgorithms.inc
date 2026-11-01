/* SPDX-License-Identifier: GPL-2.0-only */
/* Linux rtl8xxxu 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Copyright(c)2014-2017 Jes Sorensen <Jes.Sorensen@gmail.com>.
 * Copyright(c)Realtek Corporation. Retain the accompanying provenance/license.
 * Generated register/table/algorithm subset, checked I/O supplied by the port. */
static __attribute__((unused)) void rtl8723bu_reset_8051(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u16 sys_func;

	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL);
	val8 &= ~BIT(1);
	rtl8xxxu_write8(priv, REG_RSV_CTRL, val8);

	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL + 1);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, REG_RSV_CTRL + 1, val8);

	sys_func = rtl8xxxu_read16(priv, REG_SYS_FUNC);
	sys_func &= ~SYS_FUNC_CPU_ENABLE;
	rtl8xxxu_write16(priv, REG_SYS_FUNC, sys_func);

	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL);
	val8 &= ~BIT(1);
	rtl8xxxu_write8(priv, REG_RSV_CTRL, val8);

	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL + 1);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, REG_RSV_CTRL + 1, val8);

	sys_func |= SYS_FUNC_CPU_ENABLE;
	rtl8xxxu_write16(priv, REG_SYS_FUNC, sys_func);
}

static __attribute__((unused)) void rtl8xxxu_disabled_to_emu(struct rtw_8723bu_private *priv)
{
	u8 val8;

	/* Clear suspend enable and power down enable*/
	val8 = rtl8xxxu_read8(priv, REG_APS_FSMCO + 1);
	val8 &= ~(BIT(3) | BIT(7));
	rtl8xxxu_write8(priv, REG_APS_FSMCO + 1, val8);

	/* 0x48[16] = 0 to disable GPIO9 as EXT WAKEUP*/
	val8 = rtl8xxxu_read8(priv, REG_GPIO_INTM + 2);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, REG_GPIO_INTM + 2, val8);

	/* 0x04[12:11] = 11 enable WL suspend*/
	val8 = rtl8xxxu_read8(priv, REG_APS_FSMCO + 1);
	val8 &= ~(BIT(3) | BIT(4));
	rtl8xxxu_write8(priv, REG_APS_FSMCO + 1, val8);
}

static __attribute__((unused)) int rtl8723b_emu_to_active(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u32 val32;
	int count, ret = 0;

	/* 0x20[0] = 1 enable LDOA12 MACRO block for all interface */
	val8 = rtl8xxxu_read8(priv, REG_LDOA15_CTRL);
	val8 |= LDOA15_ENABLE;
	rtl8xxxu_write8(priv, REG_LDOA15_CTRL, val8);

	/* 0x67[0] = 0 to disable BT_GPS_SEL pins*/
	val8 = rtl8xxxu_read8(priv, 0x0067);
	val8 &= ~BIT(4);
	rtl8xxxu_write8(priv, 0x0067, val8);

	r23_delay(priv,1000U*(1));

	/* 0x00[5] = 0 release analog Ips to digital, 1:isolation */
	val8 = rtl8xxxu_read8(priv, REG_SYS_ISO_CTRL);
	val8 &= ~SYS_ISO_ANALOG_IPS;
	rtl8xxxu_write8(priv, REG_SYS_ISO_CTRL, val8);

	/* Disable SW LPS 0x04[10]= 0 */
	val32 = rtl8xxxu_read8(priv, REG_APS_FSMCO);
	val32 &= ~APS_FSMCO_SW_LPS;
	rtl8xxxu_write32(priv, REG_APS_FSMCO, val32);

	/* Wait until 0x04[17] = 1 power ready */
	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
		if (val32 & BIT(17))
			break;

		r23_delay(priv,10);
	}

	if (!count) {
		ret = -EBUSY;
		goto exit;
	}

	/* We should be able to optimize the following three entries into one */

	/* Release WLON reset 0x04[16]= 1*/
	val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
	val32 |= APS_FSMCO_WLON_RESET;
	rtl8xxxu_write32(priv, REG_APS_FSMCO, val32);

	/* Disable HWPDN 0x04[15]= 0*/
	val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
	val32 &= ~APS_FSMCO_HW_POWERDOWN;
	rtl8xxxu_write32(priv, REG_APS_FSMCO, val32);

	/* Disable WL suspend*/
	val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
	val32 &= ~(APS_FSMCO_HW_SUSPEND | APS_FSMCO_PCIE);
	rtl8xxxu_write32(priv, REG_APS_FSMCO, val32);

	/* Set, then poll until 0 */
	val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
	val32 |= APS_FSMCO_MAC_ENABLE;
	rtl8xxxu_write32(priv, REG_APS_FSMCO, val32);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
		if ((val32 & APS_FSMCO_MAC_ENABLE) == 0) {
			ret = 0;
			break;
		}
		r23_delay(priv,10);
	}

	if (!count) {
		ret = -EBUSY;
		goto exit;
	}

	/* Enable WL control XTAL setting */
	val8 = rtl8xxxu_read8(priv, REG_AFE_MISC);
	val8 |= AFE_MISC_WL_XTAL_CTRL;
	rtl8xxxu_write8(priv, REG_AFE_MISC, val8);

	/* Enable falling edge triggering interrupt */
	val8 = rtl8xxxu_read8(priv, REG_GPIO_INTM + 1);
	val8 |= BIT(1);
	rtl8xxxu_write8(priv, REG_GPIO_INTM + 1, val8);

	/* Enable GPIO9 interrupt mode */
	val8 = rtl8xxxu_read8(priv, REG_GPIO_IO_SEL_2 + 1);
	val8 |= BIT(1);
	rtl8xxxu_write8(priv, REG_GPIO_IO_SEL_2 + 1, val8);

	/* Enable GPIO9 input mode */
	val8 = rtl8xxxu_read8(priv, REG_GPIO_IO_SEL_2);
	val8 &= ~BIT(1);
	rtl8xxxu_write8(priv, REG_GPIO_IO_SEL_2, val8);

	/* Enable HSISR GPIO[C:0] interrupt */
	val8 = rtl8xxxu_read8(priv, REG_HSIMR);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, REG_HSIMR, val8);

	/* Enable HSISR GPIO9 interrupt */
	val8 = rtl8xxxu_read8(priv, REG_HSIMR + 2);
	val8 |= BIT(1);
	rtl8xxxu_write8(priv, REG_HSIMR + 2, val8);

	val8 = rtl8xxxu_read8(priv, REG_MULTI_FUNC_CTRL);
	val8 |= MULTI_WIFI_HW_ROF_EN;
	rtl8xxxu_write8(priv, REG_MULTI_FUNC_CTRL, val8);

	/* For GPIO9 internal pull high setting BIT(14) */
	val8 = rtl8xxxu_read8(priv, REG_MULTI_FUNC_CTRL + 1);
	val8 |= BIT(6);
	rtl8xxxu_write8(priv, REG_MULTI_FUNC_CTRL + 1, val8);

exit:
	return ret;
}

static __attribute__((unused)) int rtl8723bu_power_on(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u16 val16;
	u32 val32;
	int ret;

	rtl8xxxu_disabled_to_emu(priv);

	ret = rtl8723b_emu_to_active(priv);
	if (ret)
		goto exit;

	/*
	 * Enable MAC DMA/WMAC/SCHEDULE/SEC block
	 * Set CR bit10 to enable 32k calibration.
	 */
	val16 = rtl8xxxu_read16(priv, REG_CR);
	val16 |= (CR_HCI_TXDMA_ENABLE | CR_HCI_RXDMA_ENABLE |
		  CR_TXDMA_ENABLE | CR_RXDMA_ENABLE |
		  CR_PROTOCOL_ENABLE | CR_SCHEDULE_ENABLE |
		  CR_MAC_TX_ENABLE | CR_MAC_RX_ENABLE |
		  CR_SECURITY_ENABLE | CR_CALTIMER_ENABLE);
	rtl8xxxu_write16(priv, REG_CR, val16);

	/*
	 * BT coexist power on settings. This is identical for 1 and 2
	 * antenna parts.
	 */
	rtl8xxxu_write8(priv, REG_PAD_CTRL1 + 3, 0x20);

	val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
	val16 |= SYS_FUNC_BBRSTB | SYS_FUNC_BB_GLB_RSTN;
	rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);

	rtl8xxxu_write8(priv, REG_BT_CONTROL_8723BU + 1, 0x18);
	rtl8xxxu_write8(priv, REG_WLAN_ACT_CONTROL_8723B, 0x04);
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00);
	/* Antenna inverse */
	rtl8xxxu_write8(priv, 0xfe08, 0x01);

	val16 = rtl8xxxu_read16(priv, REG_PWR_DATA);
	val16 |= PWR_DATA_EEPRPAD_RFE_CTRL_EN;
	rtl8xxxu_write16(priv, REG_PWR_DATA, val16);

	val32 = rtl8xxxu_read32(priv, REG_LEDCFG0);
	val32 |= LEDCFG0_DPDT_SELECT;
	rtl8xxxu_write32(priv, REG_LEDCFG0, val32);

	val8 = rtl8xxxu_read8(priv, REG_PAD_CTRL1);
	val8 &= ~PAD_CTRL1_SW_DPDT_SEL_DATA;
	rtl8xxxu_write8(priv, REG_PAD_CTRL1, val8);
exit:
	return ret;
}

static __attribute__((unused)) int rtl8xxxu_flush_fifo(struct rtw_8723bu_private *priv)
{
	void *dev=priv;
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
	r23_delay(priv,1000U*(2));

	if (!retry)
		dev_warn(dev, "Failed to flush FIFO\n");

	return retval;
}

static __attribute__((unused)) int rtl8xxxu_active_to_lps(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u8 val32;
	int count, ret = 0;

	rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);

	/*
	 * Poll - wait for RX packet to complete
	 */
	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val32 = rtl8xxxu_read32(priv, 0x5f8);
		if (!val32)
			break;
		r23_delay(priv,10);
	}

	if (!count) {
		dev_warn(priv,
			 "%s: RX poll timed out (0x05f8)\n", __func__);
		ret = -EBUSY;
		goto exit;
	}

	/* Disable CCK and OFDM, clock gated */
	val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC);
	val8 &= ~SYS_FUNC_BBRSTB;
	rtl8xxxu_write8(priv, REG_SYS_FUNC, val8);

	r23_delay(priv,2);

	/* Reset baseband */
	val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC);
	val8 &= ~SYS_FUNC_BB_GLB_RSTN;
	rtl8xxxu_write8(priv, REG_SYS_FUNC, val8);

	/* Reset MAC TRX */
	val8 = rtl8xxxu_read8(priv, REG_CR);
	val8 = CR_HCI_TXDMA_ENABLE | CR_HCI_RXDMA_ENABLE;
	rtl8xxxu_write8(priv, REG_CR, val8);

	/* Reset MAC TRX */
	val8 = rtl8xxxu_read8(priv, REG_CR + 1);
	val8 &= ~BIT(1); /* CR_SECURITY_ENABLE */
	rtl8xxxu_write8(priv, REG_CR + 1, val8);

	/* Respond TX OK to scheduler */
	val8 = rtl8xxxu_read8(priv, REG_DUAL_TSF_RST);
	val8 |= DUAL_TSF_TX_OK;
	rtl8xxxu_write8(priv, REG_DUAL_TSF_RST, val8);

exit:
	return ret;
}

static __attribute__((unused)) void rtl8xxxu_firmware_self_reset(struct rtw_8723bu_private *priv)
{
	u16 val16;
	int i = 100;

	/* Inform 8051 to perform reset */
	rtl8xxxu_write8(priv, REG_HMTFR + 3, 0x20);

	for (i = 100; i > 0; i--) {
		val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);

		if (!(val16 & SYS_FUNC_CPU_ENABLE)) {
			dev_dbg(priv,
				"%s: Firmware self reset success!\n", __func__);
			break;
		}
		r23_delay(priv,50);
	}

	if (!i) {
		/* Force firmware reset */
		val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
		val16 &= ~SYS_FUNC_CPU_ENABLE;
		rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);
	}
}

static __attribute__((unused)) int rtl8723bu_active_to_emu(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u16 val16;
	u32 val32;
	int count, ret = 0;

	/* Turn off RF */
	rtl8xxxu_write8(priv, REG_RF_CTRL, 0);

	/* Enable rising edge triggering interrupt */
	val16 = rtl8xxxu_read16(priv, REG_GPIO_INTM);
	val16 &= ~GPIO_INTM_EDGE_TRIG_IRQ;
	rtl8xxxu_write16(priv, REG_GPIO_INTM, val16);

	/* Release WLON reset 0x04[16]= 1*/
	val32 = rtl8xxxu_read32(priv, REG_APS_FSMCO);
	val32 |= APS_FSMCO_WLON_RESET;
	rtl8xxxu_write32(priv, REG_APS_FSMCO, val32);

	/* 0x0005[1] = 1 turn off MAC by HW state machine*/
	val8 = rtl8xxxu_read8(priv, REG_APS_FSMCO + 1);
	val8 |= BIT(1);
	rtl8xxxu_write8(priv, REG_APS_FSMCO + 1, val8);

	for (count = RTL8XXXU_MAX_REG_POLL; count; count--) {
		val8 = rtl8xxxu_read8(priv, REG_APS_FSMCO + 1);
		if ((val8 & BIT(1)) == 0)
			break;
		r23_delay(priv,10);
	}

	if (!count) {
		dev_warn(priv, "%s: Disabling MAC timed out\n",
			 __func__);
		ret = -EBUSY;
		goto exit;
	}

	/* Enable BT control XTAL setting */
	val8 = rtl8xxxu_read8(priv, REG_AFE_MISC);
	val8 &= ~AFE_MISC_WL_XTAL_CTRL;
	rtl8xxxu_write8(priv, REG_AFE_MISC, val8);

	/* 0x0000[5] = 1 analog Ips to digital, 1:isolation */
	val8 = rtl8xxxu_read8(priv, REG_SYS_ISO_CTRL);
	val8 |= SYS_ISO_ANALOG_IPS;
	rtl8xxxu_write8(priv, REG_SYS_ISO_CTRL, val8);

	/* 0x0020[0] = 0 disable LDOA12 MACRO block*/
	val8 = rtl8xxxu_read8(priv, REG_LDOA15_CTRL);
	val8 &= ~LDOA15_ENABLE;
	rtl8xxxu_write8(priv, REG_LDOA15_CTRL, val8);

exit:
	return ret;
}

static __attribute__((unused)) void rtl8723bu_power_off(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u16 val16;

	rtl8xxxu_flush_fifo(priv);

	/*
	 * Disable TX report timer
	 */
	val8 = rtl8xxxu_read8(priv, REG_TX_REPORT_CTRL);
	val8 &= ~TX_REPORT_CTRL_TIMER_ENABLE;
	rtl8xxxu_write8(priv, REG_TX_REPORT_CTRL, val8);

	rtl8xxxu_write8(priv, REG_CR, 0x0000);

	rtl8xxxu_active_to_lps(priv);

	/* Reset Firmware if running in RAM */
	if (rtl8xxxu_read8(priv, REG_MCU_FW_DL) & MCU_FW_RAM_SEL)
		rtl8xxxu_firmware_self_reset(priv);

	/* Reset MCU */
	val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
	val16 &= ~SYS_FUNC_CPU_ENABLE;
	rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);

	/* Reset MCU ready status */
	rtl8xxxu_write8(priv, REG_MCU_FW_DL, 0x00);

	rtl8723bu_active_to_emu(priv);

	val8 = rtl8xxxu_read8(priv, REG_APS_FSMCO + 1);
	val8 |= BIT(3); /* APS_FSMCO_HW_SUSPEND */
	rtl8xxxu_write8(priv, REG_APS_FSMCO + 1, val8);

	/* 0x48[16] = 1 to enable GPIO9 as EXT wakeup */
	val8 = rtl8xxxu_read8(priv, REG_GPIO_INTM + 2);
	val8 |= BIT(0);
	rtl8xxxu_write8(priv, REG_GPIO_INTM + 2, val8);
}

static __attribute__((unused)) int rtl8xxxu_gen2_channel_to_group(int channel)
{
	int group;

	if (channel < 3)
		group = 0;
	else if (channel < 6)
		group = 1;
	else if (channel < 9)
		group = 2;
	else if (channel < 12)
		group = 3;
	else
		group = 4;

	return group;
}

static __attribute__((unused)) void
rtl8723b_set_tx_power(struct rtw_8723bu_private *priv, int channel, bool ht40)
{
	u32 val32, ofdm, mcs;
	u8 cck, ofdmbase, mcsbase;
	int group, tx_idx;

	tx_idx = 0;
	group = rtl8xxxu_gen2_channel_to_group(channel);

	cck = priv->cck_tx_power_index_B[group];
	val32 = rtl8xxxu_read32(priv, REG_TX_AGC_A_CCK1_MCS32);
	val32 &= 0xffff00ff;
	val32 |= (cck << 8);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_CCK1_MCS32, val32);

	val32 = rtl8xxxu_read32(priv, REG_TX_AGC_B_CCK11_A_CCK2_11);
	val32 &= 0xff;
	val32 |= ((cck << 8) | (cck << 16) | (cck << 24));
	rtl8xxxu_write32(priv, REG_TX_AGC_B_CCK11_A_CCK2_11, val32);

	ofdmbase = priv->ht40_1s_tx_power_index_B[group];
	ofdmbase=(u8)r23_power_clamp((int)ofdmbase+(priv->ofdm_tx_power_diff[tx_idx].b));
	ofdm = ofdmbase | ofdmbase << 8 | ofdmbase << 16 | ofdmbase << 24;

	rtl8xxxu_write32(priv, REG_TX_AGC_A_RATE18_06, ofdm);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_RATE54_24, ofdm);

	mcsbase = priv->ht40_1s_tx_power_index_B[group];
	if (ht40)
		mcsbase=(u8)r23_power_clamp((int)mcsbase+(priv->ht40_tx_power_diff[tx_idx++].b));
	else
		mcsbase=(u8)r23_power_clamp((int)mcsbase+(priv->ht20_tx_power_diff[tx_idx++].b));
	mcs = mcsbase | mcsbase << 8 | mcsbase << 16 | mcsbase << 24;

	rtl8xxxu_write32(priv, REG_TX_AGC_A_MCS03_MCS00, mcs);
	rtl8xxxu_write32(priv, REG_TX_AGC_A_MCS07_MCS04, mcs);
}

static __attribute__((unused)) int rtl8xxxu_init_phy_regs(struct rtw_8723bu_private *priv,
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
			dev_warn(priv,
				 "Failed to initialize PHY\n");
			return -EAGAIN;
		}
		r23_delay(priv,1);
	}

	return 0;
}

static __attribute__((unused)) void rtl8723bu_init_phy_bb(struct rtw_8723bu_private *priv)
{
	u8 val8;
	u16 val16;

	val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
	val16 |= SYS_FUNC_BB_GLB_RSTN | SYS_FUNC_BBRSTB | SYS_FUNC_DIO_RF;
	rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);

	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00);

	/* 6. 0x1f[7:0] = 0x07 */
	val8 = RF_ENABLE | RF_RSTB | RF_SDMRSTB;
	rtl8xxxu_write8(priv, REG_RF_CTRL, val8);

	/* Why? */
	rtl8xxxu_write8(priv, REG_SYS_FUNC, 0xe3);
	rtl8xxxu_write8(priv, REG_AFE_XTAL_CTRL + 1, 0x80);
	rtl8xxxu_init_phy_regs(priv, rtl8723b_phy_1t_init_table);

	rtl8xxxu_init_phy_regs(priv, rtl8xxx_agc_8723bu_table);
}

static __attribute__((unused)) int rtl8xxxu_write_rfreg(struct rtw_8723bu_private *priv,
			 enum rtl8xxxu_rfpath path, u8 reg, u32 data)
{
	int ret, retval;
	u32 dataaddr, val32;

	if (rtl8xxxu_debug & RTL8XXXU_DEBUG_RFREG_WRITE)
		dev_info(priv, "%s(%02x) = 0x%06x\n",
			 __func__, reg, data);

	data &= FPGA0_LSSI_PARM_DATA_MASK;
	dataaddr = (reg << FPGA0_LSSI_PARM_ADDR_SHIFT) | data;

	if (priv->rtl_chip == RTL8192E) {
		val32 = rtl8xxxu_read32(priv, REG_FPGA0_POWER_SAVE);
		val32 &= ~0x20000;
		rtl8xxxu_write32(priv, REG_FPGA0_POWER_SAVE, val32);
	}

	/* Use XB for path B */
	ret = rtl8xxxu_write32(priv, rtl8xxxu_rfregs[path].lssiparm, dataaddr);
	if (ret != sizeof(dataaddr))
		retval = -EIO;
	else
		retval = 0;

	r23_delay(priv,1);

	if (priv->rtl_chip == RTL8192E) {
		val32 = rtl8xxxu_read32(priv, REG_FPGA0_POWER_SAVE);
		val32 |= 0x20000;
		rtl8xxxu_write32(priv, REG_FPGA0_POWER_SAVE, val32);
	}

	return retval;
}

static __attribute__((unused)) int rtl8xxxu_init_rf_regs(struct rtw_8723bu_private *priv,
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
			r23_delay(priv,1000U*(50));
			continue;
		case 0xfd:
			r23_delay(priv,1000U*(5));
			continue;
		case 0xfc:
			r23_delay(priv,1000U*(1));
			continue;
		case 0xfb:
			r23_delay(priv,50);
			continue;
		case 0xfa:
			r23_delay(priv,5);
			continue;
		case 0xf9:
			r23_delay(priv,1);
			continue;
		}

		ret = rtl8xxxu_write_rfreg(priv, path, reg, val);
		if (ret) {
			dev_warn(priv,
				 "Failed to initialize RF\n");
			return -EAGAIN;
		}
		r23_delay(priv,1);
	}

	return 0;
}

static __attribute__((unused)) int rtl8xxxu_init_phy_rf(struct rtw_8723bu_private *priv,
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
		dev_err(priv, "%s:Unsupported RF path %c\n",
			__func__, path + 'A');
		return -EINVAL;
	}
	/* For path B, use XB */
	rfsi_rfenv = rtl8xxxu_read16(priv, reg_sw_ctrl);
	rfsi_rfenv &= FPGA0_RF_RFENV;

	/*
	 * These two we might be able to optimize into one
	 */
	val32 = rtl8xxxu_read32(priv, reg_int_oe);
	val32 |= BIT(20);	/* 0x10 << 16 */
	rtl8xxxu_write32(priv, reg_int_oe, val32);
	r23_delay(priv,1);

	val32 = rtl8xxxu_read32(priv, reg_int_oe);
	val32 |= BIT(4);
	rtl8xxxu_write32(priv, reg_int_oe, val32);
	r23_delay(priv,1);

	/*
	 * These two we might be able to optimize into one
	 */
	val32 = rtl8xxxu_read32(priv, reg_hssi_parm2);
	val32 &= ~FPGA0_HSSI_3WIRE_ADDR_LEN;
	rtl8xxxu_write32(priv, reg_hssi_parm2, val32);
	r23_delay(priv,1);

	val32 = rtl8xxxu_read32(priv, reg_hssi_parm2);
	val32 &= ~FPGA0_HSSI_3WIRE_DATA_LEN;
	rtl8xxxu_write32(priv, reg_hssi_parm2, val32);
	r23_delay(priv,1);

	rtl8xxxu_init_rf_regs(priv, table, path);

	/* For path B, use XB */
	val16 = rtl8xxxu_read16(priv, reg_sw_ctrl);
	val16 &= ~FPGA0_RF_RFENV;
	val16 |= rfsi_rfenv;
	rtl8xxxu_write16(priv, reg_sw_ctrl, val16);

	return 0;
}

static __attribute__((unused)) int rtl8723bu_init_phy_rf(struct rtw_8723bu_private *priv)
{
	int ret;

	ret = rtl8xxxu_init_phy_rf(priv, rtl8723bu_radioa_1t_init_table, RF_A);
	/*
	 * PHY LCK
	 */
	rtl8xxxu_write_rfreg(priv, RF_A, 0xb0, 0xdfbe0);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, 0x8c01);
	r23_delay(priv,1000U*(200));
	rtl8xxxu_write_rfreg(priv, RF_A, 0xb0, 0xdffe0);

	return ret;
}

static __attribute__((unused)) void rtl8723bu_phy_init_antenna_selection(struct rtw_8723bu_private *priv)
{
	u32 val32;

	val32 = rtl8xxxu_read32(priv, REG_PAD_CTRL1);
	val32 &= ~(BIT(20) | BIT(24));
	rtl8xxxu_write32(priv, REG_PAD_CTRL1, val32);

	val32 = rtl8xxxu_read32(priv, REG_GPIO_MUXCFG);
	val32 &= ~BIT(4);
	rtl8xxxu_write32(priv, REG_GPIO_MUXCFG, val32);

	val32 = rtl8xxxu_read32(priv, REG_GPIO_MUXCFG);
	val32 |= BIT(3);
	rtl8xxxu_write32(priv, REG_GPIO_MUXCFG, val32);

	val32 = rtl8xxxu_read32(priv, REG_LEDCFG0);
	val32 |= BIT(24);
	rtl8xxxu_write32(priv, REG_LEDCFG0, val32);

	val32 = rtl8xxxu_read32(priv, REG_LEDCFG0);
	val32 &= ~BIT(23);
	rtl8xxxu_write32(priv, REG_LEDCFG0, val32);

	val32 = rtl8xxxu_read32(priv, REG_RFE_BUFFER);
	val32 |= (BIT(0) | BIT(1));
	rtl8xxxu_write32(priv, REG_RFE_BUFFER, val32);

	val32 = rtl8xxxu_read32(priv, REG_RFE_CTRL_ANTA_SRC);
	val32 &= 0xffffff00;
	val32 |= 0x77;
	rtl8xxxu_write32(priv, REG_RFE_CTRL_ANTA_SRC, val32);

	val32 = rtl8xxxu_read32(priv, REG_PWR_DATA);
	val32 |= PWR_DATA_EEPRPAD_RFE_CTRL_EN;
	rtl8xxxu_write32(priv, REG_PWR_DATA, val32);
}

static __attribute__((unused)) void rtl8xxxu_gen2_prepare_calibrate(struct rtw_8723bu_private *priv, u8 start)
{
	struct h2c_cmd h2c;

	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.bt_wlan_calibration.cmd = H2C_8723B_BT_WLAN_CALIBRATION;
	h2c.bt_wlan_calibration.data = start;

	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.bt_wlan_calibration));
}

static __attribute__((unused)) void rtl8xxxu_save_regs(struct rtw_8723bu_private *priv, const u32 *regs,
			u32 *backup, int count)
{
	int i;

	for (i = 0; i < count; i++)
		backup[i] = rtl8xxxu_read32(priv, regs[i]);
}

static __attribute__((unused)) void
rtl8xxxu_save_mac_regs(struct rtw_8723bu_private *priv, const u32 *reg, u32 *backup)
{
	int i;

	for (i = 0; i < (RTL8XXXU_MAC_REGS - 1); i++)
		backup[i] = rtl8xxxu_read8(priv, reg[i]);

	backup[i] = rtl8xxxu_read32(priv, reg[i]);
}

static __attribute__((unused)) void rtl8xxxu_path_adda_on(struct rtw_8723bu_private *priv, const u32 *regs,
			   bool path_a_on)
{
	u32 path_on;
	int i;

	if (priv->tx_paths == 1) {
		path_on = priv->fops->adda_1t_path_on;
		rtl8xxxu_write32(priv, regs[0], priv->fops->adda_1t_init);
	} else {
		path_on = path_a_on ? priv->fops->adda_2t_path_on_a :
			priv->fops->adda_2t_path_on_b;

		rtl8xxxu_write32(priv, regs[0], path_on);
	}

	for (i = 1 ; i < RTL8XXXU_ADDA_REGS ; i++)
		rtl8xxxu_write32(priv, regs[i], path_on);
}

static __attribute__((unused)) void rtl8xxxu_mac_calibration(struct rtw_8723bu_private *priv,
			      const u32 *regs, u32 *backup)
{
	int i = 0;

	rtl8xxxu_write8(priv, regs[i], 0x3f);

	for (i = 1 ; i < (RTL8XXXU_MAC_REGS - 1); i++)
		rtl8xxxu_write8(priv, regs[i], (u8)(backup[i] & ~BIT(3)));

	rtl8xxxu_write8(priv, regs[i], (u8)(backup[i] & ~BIT(5)));
}

static __attribute__((unused)) u32 rtl8xxxu_read_rfreg(struct rtw_8723bu_private *priv,
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

	r23_delay(priv,10);

	rtl8xxxu_write32(priv, rtl8xxxu_rfregs[path].hssiparm2, val32);
	r23_delay(priv,100);

	hssia |= FPGA0_HSSI_PARM2_EDGE_READ;
	rtl8xxxu_write32(priv, REG_FPGA0_XA_HSSI_PARM2, hssia);
	r23_delay(priv,10);

	val32 = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].hssiparm1);
	if (val32 & FPGA0_HSSI_PARM1_PI)
		retval = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].hspiread);
	else
		retval = rtl8xxxu_read32(priv, rtl8xxxu_rfregs[path].lssiread);

	retval &= 0xfffff;

	if (rtl8xxxu_debug & RTL8XXXU_DEBUG_RFREG_READ)
		dev_info(priv, "%s(%02x) = 0x%06x\n",
			 __func__, reg, retval);
	return retval;
}

static __attribute__((unused)) int rtl8723bu_iqk_path_a(struct rtw_8723bu_private *priv)
{
	u32 reg_eac, reg_e94, reg_e9c, path_sel, val32;
	int result = 0;

	path_sel = rtl8xxxu_read32(priv, REG_S0S1_PATH_SWITCH);

	/*
	 * Leave IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/*
	 * Enable path A PA in TX IQK mode
	 */
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x20000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0003f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xc7f87);

	/*
	 * Tx IQK setting
	 */
	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	/* path-A IQK setting */
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x821403ea);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28110000);
	rtl8xxxu_write32(priv, REG_TX_IQK_PI_B, 0x82110000);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_B, 0x28110000);

	/* LO calibration setting */
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x00462911);

	/*
	 * Enter IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	val32 |= 0x80800000;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/*
	 * The vendor driver indicates the USB module is always using
	 * S0S1 path 1 for the 8723bu. This may be different for 8192eu
	 */
	if (priv->rf_paths > 1)
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00000000);
	else
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00000280);

	/*
	 * Bit 12 seems to be BT_GRANT, and is only found in the 8723bu.
	 * No trace of this in the 8192eu or 8188eu vendor drivers.
	 */
	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, 0x00000800);

	/* One shot, path A LOK & IQK */
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf9000000);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8000000);

	r23_delay(priv,1000U*(1));

	/* Restore Ant Path */
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel);
#ifdef RTL8723BU_BT
	/* GNT_BT = 1 */
	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, 0x00001800);
#endif

	/*
	 * Leave IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/* Check failed */
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_e94 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
	reg_e9c = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);

	val32 = (reg_e9c >> 16) & 0x3ff;
	if (val32 & 0x200)
		val32 = 0x400 - val32;

	if (!(reg_eac & BIT(28)) &&
	    ((reg_e94 & 0x03ff0000) != 0x01420000) &&
	    ((reg_e9c & 0x03ff0000) != 0x00420000) &&
	    ((reg_e94 & 0x03ff0000)  < 0x01100000) &&
	    ((reg_e94 & 0x03ff0000)  > 0x00f00000) &&
	    val32 < 0xf)
		result |= 0x01;
	else	/* If TX not OK, ignore RX */
		goto out;

out:
	return result;
}

static __attribute__((unused)) int rtl8723bu_rx_iqk_path_a(struct rtw_8723bu_private *priv)
{
	u32 reg_ea4, reg_eac, reg_e94, reg_e9c, path_sel, val32;
	int result = 0;

	path_sel = rtl8xxxu_read32(priv, REG_S0S1_PATH_SWITCH);

	/*
	 * Leave IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/*
	 * Enable path A PA in TX IQK mode
	 */
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x30000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0001f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xf7fb7);

	/*
	 * Tx IQK setting
	 */
	rtl8xxxu_write32(priv, REG_TX_IQK, 0x01007c00);
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	/* path-A IQK setting */
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x82160ff0);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x28110000);
	rtl8xxxu_write32(priv, REG_TX_IQK_PI_B, 0x82110000);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_B, 0x28110000);

	/* LO calibration setting */
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0046a911);

	/*
	 * Enter IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	val32 |= 0x80800000;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/*
	 * The vendor driver indicates the USB module is always using
	 * S0S1 path 1 for the 8723bu. This may be different for 8192eu
	 */
	if (priv->rf_paths > 1)
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00000000);
	else
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00000280);

	/*
	 * Bit 12 seems to be BT_GRANT, and is only found in the 8723bu.
	 * No trace of this in the 8192eu or 8188eu vendor drivers.
	 */
	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, 0x00000800);

	/* One shot, path A LOK & IQK */
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf9000000);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8000000);

	r23_delay(priv,1000U*(1));

	/* Restore Ant Path */
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel);
#ifdef RTL8723BU_BT
	/* GNT_BT = 1 */
	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, 0x00001800);
#endif

	/*
	 * Leave IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/* Check failed */
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_e94 = rtl8xxxu_read32(priv, REG_TX_POWER_BEFORE_IQK_A);
	reg_e9c = rtl8xxxu_read32(priv, REG_TX_POWER_AFTER_IQK_A);

	val32 = (reg_e9c >> 16) & 0x3ff;
	if (val32 & 0x200)
		val32 = 0x400 - val32;

	if (!(reg_eac & BIT(28)) &&
	    ((reg_e94 & 0x03ff0000) != 0x01420000) &&
	    ((reg_e9c & 0x03ff0000) != 0x00420000) &&
	    ((reg_e94 & 0x03ff0000)  < 0x01100000) &&
	    ((reg_e94 & 0x03ff0000)  > 0x00f00000) &&
	    val32 < 0xf)
		result |= 0x01;
	else	/* If TX not OK, ignore RX */
		goto out;

	val32 = 0x80007c00 | (reg_e94 &0x3ff0000) |
		((reg_e9c & 0x3ff0000) >> 16);
	rtl8xxxu_write32(priv, REG_TX_IQK, val32);

	/*
	 * Modify RX IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x30000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0001f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xf7d77);

	/*
	 * PA, PAD setting
	 */
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, 0xf80);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_UNKNOWN_55, 0x4021f);

	/*
	 * RX IQK setting
	 */
	rtl8xxxu_write32(priv, REG_RX_IQK, 0x01004800);

	/* path-A IQK setting */
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x18008c1c);
	rtl8xxxu_write32(priv, REG_TX_IQK_TONE_B, 0x38008c1c);
	rtl8xxxu_write32(priv, REG_RX_IQK_TONE_B, 0x38008c1c);

	rtl8xxxu_write32(priv, REG_TX_IQK_PI_A, 0x82110000);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_A, 0x2816001f);
	rtl8xxxu_write32(priv, REG_TX_IQK_PI_B, 0x82110000);
	rtl8xxxu_write32(priv, REG_RX_IQK_PI_B, 0x28110000);

	/* LO calibration setting */
	rtl8xxxu_write32(priv, REG_IQK_AGC_RSP, 0x0046a8d1);

	/*
	 * Enter IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	val32 |= 0x80800000;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	if (priv->rf_paths > 1)
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00000000);
	else
		rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x00000280);

	/*
	 * Disable BT
	 */
	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, 0x00000800);

	/* One shot, path A LOK & IQK */
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf9000000);
	rtl8xxxu_write32(priv, REG_IQK_AGC_PTS, 0xf8000000);

	r23_delay(priv,1000U*(1));

	/* Restore Ant Path */
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, path_sel);
#ifdef RTL8723BU_BT
	/* GNT_BT = 1 */
	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, 0x00001800);
#endif

	/*
	 * Leave IQK mode
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	/* Check failed */
	reg_eac = rtl8xxxu_read32(priv, REG_RX_POWER_AFTER_IQK_A_2);
	reg_ea4 = rtl8xxxu_read32(priv, REG_RX_POWER_BEFORE_IQK_A_2);

	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_GAIN_CCA, 0x780);

	val32 = (reg_eac >> 16) & 0x3ff;
	if (val32 & 0x200)
		val32 = 0x400 - val32;

	if (!(reg_eac & BIT(27)) &&
	    ((reg_ea4 & 0x03ff0000) != 0x01320000) &&
	    ((reg_eac & 0x03ff0000) != 0x00360000) &&
	    ((reg_ea4 & 0x03ff0000)  < 0x01100000) &&
	    ((reg_ea4 & 0x03ff0000)  > 0x00f00000) &&
	    val32 < 0xf)
		result |= 0x02;
	else	/* If TX not OK, ignore RX */
		goto out;
out:
	return result;
}

static __attribute__((unused)) void rtl8xxxu_restore_regs(struct rtw_8723bu_private *priv, const u32 *regs,
			   u32 *backup, int count)
{
	int i;

	for (i = 0; i < count; i++)
		rtl8xxxu_write32(priv, regs[i], backup[i]);
}

static __attribute__((unused)) void rtl8xxxu_restore_mac_regs(struct rtw_8723bu_private *priv,
			       const u32 *reg, u32 *backup)
{
	int i;

	for (i = 0; i < (RTL8XXXU_MAC_REGS - 1); i++)
		rtl8xxxu_write8(priv, reg[i], backup[i]);

	rtl8xxxu_write32(priv, reg[i], backup[i]);
}

static __attribute__((unused)) void rtl8723bu_phy_iqcalibrate(struct rtw_8723bu_private *priv,
				      int result[][8], int t)
{
	void *dev=priv;
	u32 i, val32;
	int path_a_ok /*, path_b_ok */;
	int retry = 2;
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
		REG_FPGA0_XB_RF_INT_OE, REG_FPGA0_RF_MODE
	};
	u8 xa_agc = rtl8xxxu_read32(priv, REG_OFDM0_XA_AGC_CORE1) & 0xff;
	u8 xb_agc = rtl8xxxu_read32(priv, REG_OFDM0_XB_AGC_CORE1) & 0xff;

	/*
	 * Note: IQ calibration must be performed after loading
	 *       PHY_REG.txt , and radio_a, radio_b.txt
	 */

	if (t == 0) {
		/* Save ADDA parameters, turn Path A ADDA on */
		rtl8xxxu_save_regs(priv, adda_regs, priv->adda_backup,
				   RTL8XXXU_ADDA_REGS);
		rtl8xxxu_save_mac_regs(priv, iqk_mac_regs, priv->mac_backup);
		rtl8xxxu_save_regs(priv, iqk_bb_regs,
				   priv->bb_backup, RTL8XXXU_BB_REGS);
        if(priv->owner->last_error)return;
	}

	rtl8xxxu_path_adda_on(priv, adda_regs, true);

	/* MAC settings */
	rtl8xxxu_mac_calibration(priv, iqk_mac_regs, priv->mac_backup);

	val32 = rtl8xxxu_read32(priv, REG_CCK0_AFE_SETTING);
	val32 |= 0x0f000000;
	rtl8xxxu_write32(priv, REG_CCK0_AFE_SETTING, val32);

	rtl8xxxu_write32(priv, REG_OFDM0_TRX_PATH_ENABLE, 0x03a05600);
	rtl8xxxu_write32(priv, REG_OFDM0_TR_MUX_PAR, 0x000800e4);
	rtl8xxxu_write32(priv, REG_FPGA0_XCD_RF_SW_CTRL, 0x22204000);

	/*
	 * RX IQ calibration setting for 8723B D cut large current issue
	 * when leaving IPS
	 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);

	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x30000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0001f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xf7fb7);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_UNKNOWN_ED);
	val32 |= 0x20;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_UNKNOWN_ED, val32);

	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_UNKNOWN_43, 0x60fbd);

	for (i = 0; i < (u32)retry; i++) {
		path_a_ok = rtl8723bu_iqk_path_a(priv);
		if (path_a_ok == 0x01) {
			val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
			val32 &= 0x000000ff;
			rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

			val32 = rtl8xxxu_read32(priv,
						REG_TX_POWER_BEFORE_IQK_A);
			result[t][0] = (val32 >> 16) & 0x3ff;
			val32 = rtl8xxxu_read32(priv,
						REG_TX_POWER_AFTER_IQK_A);
			result[t][1] = (val32 >> 16) & 0x3ff;

			break;
		}
	}

	if (!path_a_ok)
		dev_dbg(dev, "%s: Path A TX IQK failed!\n", __func__);

	for (i = 0; i < (u32)retry; i++) {
		path_a_ok = rtl8723bu_rx_iqk_path_a(priv);
		if (path_a_ok == 0x03) {
			val32 = rtl8xxxu_read32(priv,
						REG_RX_POWER_BEFORE_IQK_A_2);
			result[t][2] = (val32 >> 16) & 0x3ff;
			val32 = rtl8xxxu_read32(priv,
						REG_RX_POWER_AFTER_IQK_A_2);
			result[t][3] = (val32 >> 16) & 0x3ff;

			break;
		}
	}

	if (!path_a_ok)
		dev_dbg(dev, "%s: Path A RX IQK failed!\n", __func__);

	if (priv->tx_paths > 1) {
		dev_warn(dev, "%s: Path B not supported\n", __func__);

	}

	/* Back to BB mode, load original value */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 &= 0x000000ff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);

	if (t) {
		/* Reload ADDA power saving parameters */
		rtl8xxxu_restore_regs(priv, adda_regs, priv->adda_backup,
				      RTL8XXXU_ADDA_REGS);

		/* Reload MAC parameters */
		rtl8xxxu_restore_mac_regs(priv, iqk_mac_regs, priv->mac_backup);

		/* Reload BB parameters */
		rtl8xxxu_restore_regs(priv, iqk_bb_regs,
				      priv->bb_backup, RTL8XXXU_BB_REGS);

		/* Restore RX initial gain */
		val32 = rtl8xxxu_read32(priv, REG_OFDM0_XA_AGC_CORE1);
		val32 &= 0xffffff00;
		rtl8xxxu_write32(priv, REG_OFDM0_XA_AGC_CORE1, val32 | 0x50);
		rtl8xxxu_write32(priv, REG_OFDM0_XA_AGC_CORE1, val32 | xa_agc);

		if (priv->tx_paths > 1) {
			val32 = rtl8xxxu_read32(priv, REG_OFDM0_XB_AGC_CORE1);
			val32 &= 0xffffff00;
			rtl8xxxu_write32(priv, REG_OFDM0_XB_AGC_CORE1,
					 val32 | 0x50);
			rtl8xxxu_write32(priv, REG_OFDM0_XB_AGC_CORE1,
					 val32 | xb_agc);
		}

		/* Load 0xe30 IQC default value */
		rtl8xxxu_write32(priv, REG_TX_IQK_TONE_A, 0x01008c00);
		rtl8xxxu_write32(priv, REG_RX_IQK_TONE_A, 0x01008c00);
	}
}

static __attribute__((unused)) bool rtl8xxxu_gen2_simularity_compare(struct rtw_8723bu_private *priv,
				      int result[][8], int c1, int c2)
{
	u32 i, j, diff, simubitmap, bound = 0;
	int candidate[2] = {-1, -1};	/* for path A and path B */
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
			/* path A TX OK */
			for (i = 0; i < 2; i++)
				result[3][i] = result[c1][i];
		}

		if (!(simubitmap & 0x0c)) {
			/* path A RX OK */
			for (i = 2; i < 4; i++)
				result[3][i] = result[c1][i];
		}

		if (!(simubitmap & 0x30) && priv->tx_paths > 1) {
			/* path B TX OK */
			for (i = 4; i < 6; i++)
				result[3][i] = result[c1][i];
		}

		if (!(simubitmap & 0xc0) && priv->tx_paths > 1) {
			/* path B RX OK */
			for (i = 6; i < 8; i++)
				result[3][i] = result[c1][i];
		}
	}

	return false;
}

static __attribute__((unused)) void rtl8xxxu_fill_iqk_matrix_a(struct rtw_8723bu_private *priv, bool iqk_ok,
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
	val32 |= (((tx0_c & 0x3c0) >> 6) << 28);
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
		dev_dbg(priv, "%s: only TX\n", __func__);
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

static __attribute__((unused)) int rtl8xxxu_write32_mask(struct rtw_8723bu_private *priv, u16 addr,
			  u32 mask, u32 val)
{
	u32 orig, new, shift;

	shift = __ffs(mask);

	orig = rtl8xxxu_read32(priv, addr);
	new = (orig & ~mask) | ((val << shift) & mask);
	return rtl8xxxu_write32(priv, addr, new);
}

static __attribute__((unused)) void rtl8xxxu_fill_iqk_matrix_b(struct rtw_8723bu_private *priv, bool iqk_ok,
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
	val32 |= (((tx1_c & 0x3c0) >> 6) << 28);
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
		dev_dbg(priv, "%s: only TX\n", __func__);
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

	if (priv->rtl_chip == RTL8192F) {
		rtl8xxxu_write32_mask(priv, REG_RXIQB_EXT, 0x000000f0, reg);
	} else {
		val32 = rtl8xxxu_read32(priv, REG_OFDM0_AGC_RSSI_TABLE);
		val32 &= ~0x0000f000;
		val32 |= (reg << 12);
		rtl8xxxu_write32(priv, REG_OFDM0_AGC_RSSI_TABLE, val32);
	}
}

static __attribute__((unused)) void rtl8723bu_phy_iq_calibrate(struct rtw_8723bu_private *priv)
{
	void *dev=priv;
	int result[4][8];	/* last is final result */
	int i, candidate;
	bool path_a_ok, path_b_ok;
	u32 reg_e94, reg_e9c, reg_ea4, reg_eac;
	u32 reg_eb4, reg_ebc, reg_ec4, reg_ecc;
	u32 val32, bt_control;
	s32 reg_tmp = 0;
	bool simu;

	rtl8xxxu_gen2_prepare_calibrate(priv, 1);

	memset(result, 0, sizeof(result));
	candidate = -1;

	path_a_ok = false;
	path_b_ok = false;

	bt_control = rtl8xxxu_read32(priv, REG_BT_CONTROL_8723BU);

	for (i = 0; i < 3; i++) {
		rtl8723bu_phy_iqcalibrate(priv, result, i);

		if (i == 1) {
			simu = rtl8xxxu_gen2_simularity_compare(priv,
								result, 0, 1);
			if (simu) {
				candidate = 0;
				break;
			}
		}

		if (i == 2) {
			simu = rtl8xxxu_gen2_simularity_compare(priv,
								result, 0, 2);
			if (simu) {
				candidate = 0;
				break;
			}

			simu = rtl8xxxu_gen2_simularity_compare(priv,
								result, 1, 2);
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

	for (i = 0; i < 4; i++) {
		reg_e94 = result[i][0];
		reg_e9c = result[i][1];
		reg_ea4 = result[i][2];
		reg_eac = result[i][3];
		reg_eb4 = result[i][4];
		reg_ebc = result[i][5];
		reg_ec4 = result[i][6];
		reg_ecc = result[i][7];
	}

	if (candidate >= 0) {
		reg_e94 = result[candidate][0];
		priv->rege94 =  reg_e94;
		reg_e9c = result[candidate][1];
		priv->rege9c = reg_e9c;
		reg_ea4 = result[candidate][2];
		reg_eac = result[candidate][3];
		reg_eb4 = result[candidate][4];
		priv->regeb4 = reg_eb4;
		reg_ebc = result[candidate][5];
		priv->regebc = reg_ebc;
		reg_ec4 = result[candidate][6];
		reg_ecc = result[candidate][7];
		dev_dbg(dev, "%s: candidate is %x\n", __func__, candidate);
		dev_dbg(dev,
			"%s: e94 =%x e9c=%x ea4=%x eac=%x eb4=%x ebc=%x ec4=%x ecc=%x\n",
			__func__, reg_e94, reg_e9c,
			reg_ea4, reg_eac, reg_eb4, reg_ebc, reg_ec4, reg_ecc);
		path_a_ok = true;
		path_b_ok = true;
	} else {
		reg_e94 = reg_eb4 = priv->rege94 = priv->regeb4 = 0x100;
		reg_e9c = reg_ebc = priv->rege9c = priv->regebc = 0x0;
	}

	if (reg_e94 && candidate >= 0)
		rtl8xxxu_fill_iqk_matrix_a(priv, path_a_ok, result,
					   candidate, (reg_ea4 == 0));

	if (priv->tx_paths > 1 && reg_eb4)
		rtl8xxxu_fill_iqk_matrix_b(priv, path_b_ok, result,
					   candidate, (reg_ec4 == 0));

	rtl8xxxu_save_regs(priv, rtl8xxxu_iqk_phy_iq_bb_reg,
			   priv->bb_recovery_backup, RTL8XXXU_BB_REGS);

	rtl8xxxu_write32(priv, REG_BT_CONTROL_8723BU, bt_control);

	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_WE_LUT);
	val32 |= 0x80000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_WE_LUT, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_RCK_OS, 0x18000);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G1, 0x0001f);
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_TXPA_G2, 0xe6177);
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_UNKNOWN_ED);
	val32 |= 0x20;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_UNKNOWN_ED, val32);
	rtl8xxxu_write_rfreg(priv, RF_A, 0x43, 0x300bd);

	if (priv->rf_paths > 1)
		dev_dbg(dev, "%s: 8723BU 2T not supported\n", __func__);

	rtl8xxxu_gen2_prepare_calibrate(priv, 0);
 priv->iqk_ok=candidate>=0&&reg_e94!=0&&reg_ea4!=0;
 (void)reg_eac;(void)reg_ecc;
}

static __attribute__((unused)) void rtl8723bu_write_btreg(struct rtw_8723bu_private *priv, u8 reg, u8 data)
{
	struct h2c_cmd h2c;
	int reqnum = 0;

	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.bt_mp_oper.cmd = H2C_8723B_BT_MP_OPER;
	h2c.bt_mp_oper.operreq = 0 | (reqnum << 4);
	h2c.bt_mp_oper.opcode = BT_MP_OP_WRITE_REG_VALUE;
	h2c.bt_mp_oper.data = data;
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.bt_mp_oper));

	reqnum++;
	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.bt_mp_oper.cmd = H2C_8723B_BT_MP_OPER;
	h2c.bt_mp_oper.operreq = 0 | (reqnum << 4);
	h2c.bt_mp_oper.opcode = BT_MP_OP_WRITE_REG_VALUE;
	h2c.bt_mp_oper.addr = reg;
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.bt_mp_oper));
}

static __attribute__((unused)) void rtl8723bu_set_ps_tdma(struct rtw_8723bu_private *priv,
			   u8 arg1, u8 arg2, u8 arg3, u8 arg4, u8 arg5)
{
	struct h2c_cmd h2c;

	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.b_type_dma.cmd = H2C_8723B_B_TYPE_TDMA;
	h2c.b_type_dma.data1 = arg1;
	h2c.b_type_dma.data2 = arg2;
	h2c.b_type_dma.data3 = arg3;
	h2c.b_type_dma.data4 = arg4;
	h2c.b_type_dma.data5 = arg5;
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.b_type_dma));
}

static __attribute__((unused)) void rtl8723b_enable_rf(struct rtw_8723bu_private *priv)
{
	struct h2c_cmd h2c;
	u32 val32;
	u8 val8;

	val32 = rtl8xxxu_read32(priv, REG_RX_WAIT_CCA);
	val32 |= (BIT(22) | BIT(23));
	rtl8xxxu_write32(priv, REG_RX_WAIT_CCA, val32);

	/*
	 * No indication anywhere as to what 0x0790 does. The 2 antenna
	 * vendor code preserves bits 6-7 here.
	 */
	rtl8xxxu_write8(priv, 0x0790, 0x05);
	/*
	 * 0x0778 seems to be related to enabling the number of antennas
	 * In the vendor driver halbtc8723b2ant_InitHwConfig() sets it
	 * to 0x03, while halbtc8723b1ant_InitHwConfig() sets it to 0x01
	 */
	rtl8xxxu_write8(priv, 0x0778, 0x01);

	val8 = rtl8xxxu_read8(priv, REG_GPIO_MUXCFG);
	val8 |= BIT(5);
	rtl8xxxu_write8(priv, REG_GPIO_MUXCFG, val8);

	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_IQADJ_G1, 0x780);

	rtl8723bu_write_btreg(priv, 0x3c, 0x15); /* BT TRx Mask on */

	/*
	 * Set BT grant to low
	 */
	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.bt_grant.cmd = H2C_8723B_BT_GRANT;
	h2c.bt_grant.data = 0;
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.bt_grant));

	/*
	 * WLAN action by PTA
	 */
	rtl8xxxu_write8(priv, REG_WLAN_ACT_CONTROL_8723B, 0x0c);

	/*
	 * BT select S0/S1 controlled by WiFi
	 */
	val8 = rtl8xxxu_read8(priv, 0x0067);
	val8 |= BIT(5);
	rtl8xxxu_write8(priv, 0x0067, val8);

	val32 = rtl8xxxu_read32(priv, REG_PWR_DATA);
	val32 |= PWR_DATA_EEPRPAD_RFE_CTRL_EN;
	rtl8xxxu_write32(priv, REG_PWR_DATA, val32);

	/*
	 * Bits 6/7 are marked in/out ... but for what?
	 */
	rtl8xxxu_write8(priv, 0x0974, 0xff);

	val32 = rtl8xxxu_read32(priv, REG_RFE_BUFFER);
	val32 |= (BIT(0) | BIT(1));
	rtl8xxxu_write32(priv, REG_RFE_BUFFER, val32);

	rtl8xxxu_write8(priv, REG_RFE_CTRL_ANTA_SRC, 0x77);

	val32 = rtl8xxxu_read32(priv, REG_LEDCFG0);
	val32 &= ~BIT(24);
	val32 |= BIT(23);
	rtl8xxxu_write32(priv, REG_LEDCFG0, val32);

	/*
	 * Fix external switch Main->S1, Aux->S0
	 */
	val8 = rtl8xxxu_read8(priv, REG_PAD_CTRL1);
	val8 &= ~BIT(0);
	rtl8xxxu_write8(priv, REG_PAD_CTRL1, val8);

	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.ant_sel_rsv.cmd = H2C_8723B_ANT_SEL_RSV;
	h2c.ant_sel_rsv.ant_inverse = 1;
	h2c.ant_sel_rsv.int_switch_type = 0;
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.ant_sel_rsv));

	/*
	 * Different settings per different antenna position.
	 *      Antenna Position:   | Normal   Inverse
	 * --------------------------------------------------
	 * Antenna switch to BT:    |  0x280,   0x00
	 * Antenna switch to WiFi:  |  0x0,     0x280
	 * Antenna switch to PTA:   |  0x200,   0x80
	 */
	rtl8xxxu_write32(priv, REG_S0S1_PATH_SWITCH, 0x80);

	/*
	 * Software control, antenna at WiFi side
	 */
	rtl8723bu_set_ps_tdma(priv, 0x08, 0x00, 0x00, 0x00, 0x00);

	rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x55555555);
	rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0x55555555);
	rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
	rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);

	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.bt_info.cmd = H2C_8723B_BT_INFO;
	h2c.bt_info.data = BIT(0);
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.bt_info));

	memset(&h2c, 0, sizeof(struct h2c_cmd));
	h2c.ignore_wlan.cmd = H2C_8723B_BT_IGNORE_WLANACT;
	h2c.ignore_wlan.data = 0;
	rtl8xxxu_gen2_h2c_cmd(priv, &h2c, sizeof(h2c.ignore_wlan));
}

static __attribute__((unused)) void rtl8723bu_init_aggregation(struct rtw_8723bu_private *priv)
{
	u32 agg_rx;
	u8 agg_ctrl;

	/*
	 * For now simply disable RX aggregation
	 */
	agg_ctrl = rtl8xxxu_read8(priv, REG_TRXDMA_CTRL);
	agg_ctrl &= ~TRXDMA_CTRL_RXDMA_AGG_EN;

	agg_rx = rtl8xxxu_read32(priv, REG_RXDMA_AGG_PG_TH);
	agg_rx &= ~RXDMA_USB_AGG_ENABLE;
	agg_rx &= ~0xff0f;

	rtl8xxxu_write8(priv, REG_TRXDMA_CTRL, agg_ctrl);
	rtl8xxxu_write32(priv, REG_RXDMA_AGG_PG_TH, agg_rx);
}

static __attribute__((unused)) void rtl8723bu_init_statistics(struct rtw_8723bu_private *priv)
{
	u32 val32;

	/* Time duration for NHM unit: 4us, 0x2710=40ms */
	rtl8xxxu_write16(priv, REG_NHM_TIMER_8723B + 2, 0x2710);
	rtl8xxxu_write16(priv, REG_NHM_TH9_TH10_8723B + 2, 0xffff);
	rtl8xxxu_write32(priv, REG_NHM_TH3_TO_TH0_8723B, 0xffffff52);
	rtl8xxxu_write32(priv, REG_NHM_TH7_TO_TH4_8723B, 0xffffffff);
	/* TH8 */
	val32 = rtl8xxxu_read32(priv, REG_FPGA0_IQK);
	val32 |= 0xff;
	rtl8xxxu_write32(priv, REG_FPGA0_IQK, val32);
	/* Enable CCK */
	val32 = rtl8xxxu_read32(priv, REG_NHM_TH9_TH10_8723B);
	val32 |= BIT(8) | BIT(9) | BIT(10);
	rtl8xxxu_write32(priv, REG_NHM_TH9_TH10_8723B, val32);
	/* Max power amongst all RX antennas */
	val32 = rtl8xxxu_read32(priv, REG_OFDM0_FA_RSTC);
	val32 |= BIT(7);
	rtl8xxxu_write32(priv, REG_OFDM0_FA_RSTC, val32);
}

static __attribute__((unused)) void rtl8723a_phy_lc_calibrate(struct rtw_8723bu_private *priv)
{
	u32 val32;
	u32 rf_amode, rf_bmode = 0, lstf;

	/* Check continuous TX and Packet TX */
	lstf = rtl8xxxu_read32(priv, REG_OFDM1_LSTF);

	if (lstf & OFDM_LSTF_MASK) {
		/* Disable all continuous TX */
		val32 = lstf & ~OFDM_LSTF_MASK;
		rtl8xxxu_write32(priv, REG_OFDM1_LSTF, val32);

		/* Read original RF mode Path A */
		rf_amode = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_AC);

		/* Set RF mode to standby Path A */
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_AC,
				     (rf_amode & 0x8ffff) | 0x10000);

		/* Path-B */
		if (priv->tx_paths > 1) {
			rf_bmode = rtl8xxxu_read_rfreg(priv, RF_B,
						       RF6052_REG_AC);

			rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_AC,
					     (rf_bmode & 0x8ffff) | 0x10000);
		}
	} else {
		/*  Deal with Packet TX case */
		/*  block all queues */
		rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);
	}

	/* Start LC calibration */
	if (priv->fops->has_s0s1)
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_S0S1, 0xdfbe0);
	val32 = rtl8xxxu_read_rfreg(priv, RF_A, RF6052_REG_MODE_AG);
	val32 |= 0x08000;
	rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_MODE_AG, val32);

	r23_delay(priv,1000U*(100));

	if (priv->fops->has_s0s1)
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_S0S1, 0xdffe0);

	/* Restore original parameters */
	if (lstf & OFDM_LSTF_MASK) {
		/* Path-A */
		rtl8xxxu_write32(priv, REG_OFDM1_LSTF, lstf);
		rtl8xxxu_write_rfreg(priv, RF_A, RF6052_REG_AC, rf_amode);

		/* Path-B */
		if (priv->tx_paths > 1)
			rtl8xxxu_write_rfreg(priv, RF_B, RF6052_REG_AC,
					     rf_bmode);
	} else /*  Deal with Packet TX case */
		rtl8xxxu_write8(priv, REG_TXPAUSE, 0xff);
}

static __attribute__((unused)) void rtl8723a_set_crystal_cap(struct rtw_8723bu_private *priv, u8 crystal_cap)
{
	struct rtl8xxxu_cfo_tracking *cfo = &priv->cfo_tracking;
	u32 val32;

	if (crystal_cap == cfo->crystal_cap)
		return;

	val32 = rtl8xxxu_read32(priv, REG_MAC_PHY_CTRL);

	dev_dbg(priv,
	        "%s: Adjusting crystal cap from 0x%x (actually 0x%lx 0x%lx) to 0x%x\n",
	        __func__,
	        cfo->crystal_cap,
	        FIELD_GET(XTAL1, val32),
	        FIELD_GET(XTAL0, val32),
	        crystal_cap);

	val32 &= ~(XTAL1 | XTAL0);
	val32 |= FIELD_PREP(XTAL1, crystal_cap) |
		 FIELD_PREP(XTAL0, crystal_cap);
	rtl8xxxu_write32(priv, REG_MAC_PHY_CTRL, val32);

	cfo->crystal_cap = crystal_cap;
}

static __attribute__((unused)) int rtl8xxxu_download_firmware(struct rtw_8723bu_private *priv)
{
	int pages, remainder, i, ret;
	u16 reg_fw_start_address;
	u16 reg_mcu_fw_dl;
	u8 val8;
	u16 val16;
	u32 val32;
	const u8 *fwptr;

	if (priv->rtl_chip == RTL8192F)
		reg_fw_start_address = REG_FW_START_ADDRESS_8192F;
	else
		reg_fw_start_address = REG_FW_START_ADDRESS;

	if (priv->rtl_chip == RTL8710B) {
		reg_mcu_fw_dl = REG_8051FW_CTRL_V1_8710B;
	} else {
		reg_mcu_fw_dl = REG_MCU_FW_DL;

		val8 = rtl8xxxu_read8(priv, REG_SYS_FUNC + 1);
		val8 |= 4;
		rtl8xxxu_write8(priv, REG_SYS_FUNC + 1, val8);

		/* 8051 enable */
		val16 = rtl8xxxu_read16(priv, REG_SYS_FUNC);
		val16 |= SYS_FUNC_CPU_ENABLE;
		rtl8xxxu_write16(priv, REG_SYS_FUNC, val16);
	}

	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	if (val8 & MCU_FW_RAM_SEL) {
		dev_info(priv,
			 "Firmware is already running, resetting the MCU.\n");
		rtl8xxxu_write8(priv, reg_mcu_fw_dl, 0x00);
		priv->fops->reset_8051(priv);
	}

	/* MCU firmware download enable */
	val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl);
	val8 |= MCU_FW_DL_ENABLE;
	rtl8xxxu_write8(priv, reg_mcu_fw_dl, val8);

	/* 8051 reset */
	val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
	val32 &= ~BIT(19);
	rtl8xxxu_write32(priv, reg_mcu_fw_dl, val32);

	if (priv->rtl_chip == RTL8710B) {
		/* We must set 0x8090[8]=1 before download FW. */
		val8 = rtl8xxxu_read8(priv, reg_mcu_fw_dl + 1);
		val8 |= BIT(0);
		rtl8xxxu_write8(priv, reg_mcu_fw_dl + 1, val8);
	}

	/* Reset firmware download checksum */
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
 priv->cleanup=true;
	/* MCU firmware download disable */
	val16 = rtl8xxxu_read16(priv, reg_mcu_fw_dl);
	val16 &= ~MCU_FW_DL_ENABLE;
	rtl8xxxu_write16(priv, reg_mcu_fw_dl, val16);

 priv->cleanup=false;
	return ret;
}

static __attribute__((unused)) int rtl8xxxu_start_firmware(struct rtw_8723bu_private *priv)
{
	void *dev=priv;
	u16 reg_mcu_fw_dl;
	int ret = 0, i;
	u32 val32;

	if (priv->rtl_chip == RTL8710B)
		reg_mcu_fw_dl = REG_8051FW_CTRL_V1_8710B;
	else
		reg_mcu_fw_dl = REG_MCU_FW_DL;

	/* Poll checksum report */
	for (i = 0; i < RTL8XXXU_FIRMWARE_POLL_MAX; i++) {
		val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
		if (val32 & MCU_FW_DL_CSUM_REPORT)
			break;
	}

	if (i == RTL8XXXU_FIRMWARE_POLL_MAX) {
		dev_warn(dev, "Firmware checksum poll timed out\n");
		ret = -EAGAIN;
		goto exit;
	}

	val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
	val32 |= MCU_FW_DL_READY;
	val32 &= ~MCU_WINT_INIT_READY;
	rtl8xxxu_write32(priv, reg_mcu_fw_dl, val32);

	/*
	 * Reset the 8051 in order for the firmware to start running,
	 * otherwise it won't come up on the 8192eu
	 */
	priv->fops->reset_8051(priv);

	/* Wait for firmware to become ready */
	for (i = 0; i < RTL8XXXU_FIRMWARE_POLL_MAX; i++) {
		val32 = rtl8xxxu_read32(priv, reg_mcu_fw_dl);
		if (val32 & MCU_WINT_INIT_READY)
			break;

		r23_delay(priv,100);
	}

	if (i == RTL8XXXU_FIRMWARE_POLL_MAX) {
		dev_warn(dev, "Firmware failed to start\n");
		ret = -EAGAIN;
		goto exit;
	}

	/*
	 * Init H2C command
	 */
	if (priv->fops->init_reg_hmtfr)
		rtl8xxxu_write8(priv, REG_HMTFR, 0x0f);
exit:
	return ret;
}

static __attribute__((unused)) int rtl8xxxu_auto_llt_table(struct rtw_8723bu_private *priv)
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
		r23_delay(priv,2);
	}

	if (!i) {
		ret = -EBUSY;
		dev_warn(priv, "LLT table init failed\n");
	}

	return ret;
}

static __attribute__((unused)) void rtl8xxxu_init_queue_reserved_page(struct rtw_8723bu_private *priv)
{
	const struct rtl8xxxu_fileops *fops = priv->fops;
	u32 hq, lq, nq, eq, pubq;
	u32 val32;

	hq = 0;
	lq = 0;
	nq = 0;
	eq = 0;
	pubq = 0;

	if (priv->ep_tx_high_queue)
		hq = fops->page_num_hi;
	if (priv->ep_tx_low_queue)
		lq = fops->page_num_lo;
	if (priv->ep_tx_normal_queue)
		nq = fops->page_num_norm;

	val32 = (nq << RQPN_NPQ_SHIFT) | (eq << RQPN_EPQ_SHIFT);
	rtl8xxxu_write32(priv, REG_RQPN_NPQ, val32);

	pubq = fops->total_page_num - hq - lq - nq - 1;

	val32 = RQPN_LOAD;
	val32 |= (hq << RQPN_HI_PQ_SHIFT);
	val32 |= (lq << RQPN_LO_PQ_SHIFT);
	val32 |= (pubq << RQPN_PUB_PQ_SHIFT);

	rtl8xxxu_write32(priv, REG_RQPN, val32);
}

static __attribute__((unused)) int rtl8xxxu_init_queue_priority(struct rtw_8723bu_private *priv)
{
	u16 val16, hi, lo;
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
		vop = voq ^ 3;
		break;
	default:
		ret = -EINVAL;
	}

	/*
	 * None of the vendor drivers are configuring the beacon
	 * queue here .... why?
	 */
	if (!ret) {
		/* Only RTL8192F seems to do it like this. */
		if (priv->rtl_chip == RTL8192F) {
			val32 = rtl8xxxu_read32(priv, REG_TRXDMA_CTRL);
			val32 &= 0x7;
			val32 |= (voq << TRXDMA_CTRL_VOQ_SHIFT_8192F) |
				 (viq << TRXDMA_CTRL_VIQ_SHIFT_8192F) |
				 (beq << TRXDMA_CTRL_BEQ_SHIFT_8192F) |
				 (bkq << TRXDMA_CTRL_BKQ_SHIFT_8192F) |
				 (mgq << TRXDMA_CTRL_MGQ_SHIFT_8192F) |
				 (hiq << TRXDMA_CTRL_HIQ_SHIFT_8192F);
			rtl8xxxu_write32(priv, REG_TRXDMA_CTRL, val32);
		} else {
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
			usb_sndbulkpipe(priv->udev, priv->out_ep[vop]);
		priv->pipe_out[TXDESC_QUEUE_VI] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[vip]);
		priv->pipe_out[TXDESC_QUEUE_BE] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[bep]);
		priv->pipe_out[TXDESC_QUEUE_BK] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[bkp]);
		priv->pipe_out[TXDESC_QUEUE_BEACON] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[0]);
		priv->pipe_out[TXDESC_QUEUE_MGNT] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[mgp]);
		priv->pipe_out[TXDESC_QUEUE_HIGH] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[hip]);
		priv->pipe_out[TXDESC_QUEUE_CMD] =
			usb_sndbulkpipe(priv->udev, priv->out_ep[0]);
	}

	return ret;
}

static __attribute__((unused)) void rtl8xxxu_init_burst(struct rtw_8723bu_private *priv)
{
	u8 val8;

	/*
	 * For USB high speed set 512B packets
	 */
	val8 = rtl8xxxu_read8(priv, REG_RXDMA_PRO_8723B);
	u8p_replace_bits(&val8, 1, RXDMA_PRO_DMA_BURST_SIZE);
	u8p_replace_bits(&val8, 3, RXDMA_PRO_DMA_BURST_CNT);
	val8 |= RXDMA_PRO_DMA_MODE;
	rtl8xxxu_write8(priv, REG_RXDMA_PRO_8723B, val8);

	/*
	 * Enable single packet AMPDU
	 */
	val8 = rtl8xxxu_read8(priv, REG_HT_SINGLE_AMPDU_8723B);
	val8 |= HT_SINGLE_AMPDU_ENABLE;
	rtl8xxxu_write8(priv, REG_HT_SINGLE_AMPDU_8723B, val8);

	rtl8xxxu_write16(priv, REG_MAX_AGGR_NUM, priv->fops->max_aggr_num);
	rtl8xxxu_write8(priv, REG_AMPDU_MAX_TIME_8723B,
			priv->fops->ampdu_max_time);
	rtl8xxxu_write32(priv, REG_AGGLEN_LMT, 0xffffffff);
	rtl8xxxu_write8(priv, REG_RX_PKT_LIMIT, 0x18);
	rtl8xxxu_write8(priv, REG_PIFS, 0x00);
	if (priv->rtl_chip == RTL8188F || priv->rtl_chip == RTL8710B ||
	    priv->rtl_chip == RTL8192F) {
		rtl8xxxu_write8(priv, REG_FWHW_TXQ_CTRL, FWHW_TXQ_CTRL_AMPDU_RETRY);
		rtl8xxxu_write32(priv, REG_FAST_EDCA_CTRL, 0x03086666);
	}
	rtl8xxxu_write8(priv, REG_USTIME_TSF_8723B, priv->fops->ustime_tsf_edca);
	rtl8xxxu_write8(priv, REG_USTIME_EDCA, priv->fops->ustime_tsf_edca);

	/* to prevent mac is reseted by bus. */
	val8 = rtl8xxxu_read8(priv, REG_RSV_CTRL);
	val8 |= RSV_CTRL_WLOCK_1C | RSV_CTRL_DIS_PRST;
	rtl8xxxu_write8(priv, REG_RSV_CTRL, val8);
}

static __attribute__((unused)) void rtl8xxxu_gen2_usb_quirks(struct rtw_8723bu_private *priv)
{
	u32 val32;

	val32 = rtl8xxxu_read32(priv, REG_TXDMA_OFFSET_CHK);
	val32 |= TXDMA_OFFSET_DROP_DATA_EN;
	rtl8xxxu_write32(priv, REG_TXDMA_OFFSET_CHK, val32);
}

static __attribute__((unused)) void rtl8xxxu_gen2_config_channel(struct rtw_8723bu_hw *hw)
{
	struct rtw_8723bu_private *priv = hw->priv;
	u32 val32;
	u8 val8, subchannel;
	u16 rf_mode_bw;
	bool ht = true;
	int sec_ch_above, channel;
	int i;

	rf_mode_bw = rtl8xxxu_read16(priv, REG_WMAC_TRXPTCL_CTL);
	rf_mode_bw &= ~WMAC_TRXPTCL_CTL_BW_MASK;
	channel = hw->conf.chandef.chan->hw_value;

/* Hack */
	subchannel = 0;

	switch (hw->conf.chandef.width) {
	case NL80211_CHAN_WIDTH_20_NOHT:
		ht = false;
		fallthrough;
	case NL80211_CHAN_WIDTH_20:
		rf_mode_bw |= WMAC_TRXPTCL_CTL_BW_20;
		subchannel = 0;

		val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
		val32 &= ~FPGA_RF_MODE;
		rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

		val32 = rtl8xxxu_read32(priv, REG_FPGA1_RF_MODE);
		val32 &= ~FPGA_RF_MODE;
		rtl8xxxu_write32(priv, REG_FPGA1_RF_MODE, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT);
		val32 &= ~(BIT(30) | BIT(31));
		rtl8xxxu_write32(priv, REG_OFDM0_TX_PSDO_NOISE_WEIGHT, val32);

		break;
	case NL80211_CHAN_WIDTH_40:
		rf_mode_bw |= WMAC_TRXPTCL_CTL_BW_40;

		if (hw->conf.chandef.center_freq1 >
		    hw->conf.chandef.chan->center_freq) {
			sec_ch_above = 1;
			channel += 2;
			subchannel = 2;
		} else {
			sec_ch_above = 0;
			channel -= 2;
			subchannel = 1;
		}

		val32 = rtl8xxxu_read32(priv, REG_FPGA0_RF_MODE);
		val32 |= FPGA_RF_MODE;
		rtl8xxxu_write32(priv, REG_FPGA0_RF_MODE, val32);

		val32 = rtl8xxxu_read32(priv, REG_FPGA1_RF_MODE);
		val32 |= FPGA_RF_MODE;
		rtl8xxxu_write32(priv, REG_FPGA1_RF_MODE, val32);

		/*
		 * Set Control channel to upper or lower. These settings
		 * are required only for 40MHz
		 */
		val32 = rtl8xxxu_read32(priv, REG_CCK0_SYSTEM);
		val32 &= ~CCK0_SIDEBAND;
		if (!sec_ch_above)
			val32 |= CCK0_SIDEBAND;
		rtl8xxxu_write32(priv, REG_CCK0_SYSTEM, val32);

		val32 = rtl8xxxu_read32(priv, REG_OFDM1_LSTF);
		val32 &= ~OFDM_LSTF_PRIME_CH_MASK; /* 0xc00 */
		if (sec_ch_above)
			val32 |= OFDM_LSTF_PRIME_CH_LOW;
		else
			val32 |= OFDM_LSTF_PRIME_CH_HIGH;
		rtl8xxxu_write32(priv, REG_OFDM1_LSTF, val32);

		val32 = rtl8xxxu_read32(priv, REG_FPGA0_POWER_SAVE);
		val32 &= ~(FPGA0_PS_LOWER_CHANNEL | FPGA0_PS_UPPER_CHANNEL);
		if (sec_ch_above)
			val32 |= FPGA0_PS_UPPER_CHANNEL;
		else
			val32 |= FPGA0_PS_LOWER_CHANNEL;
		rtl8xxxu_write32(priv, REG_FPGA0_POWER_SAVE, val32);
		break;
	case NL80211_CHAN_WIDTH_80:
		rf_mode_bw |= WMAC_TRXPTCL_CTL_BW_80;
		break;
	default:
		break;
	}

	for (i = RF_A; i < priv->rf_paths; i++) {
		val32 = rtl8xxxu_read_rfreg(priv, i, RF6052_REG_MODE_AG);
		val32 &= ~MODE_AG_CHANNEL_MASK;
		val32 |= channel;
		rtl8xxxu_write_rfreg(priv, i, RF6052_REG_MODE_AG, val32);
	}

	rtl8xxxu_write16(priv, REG_WMAC_TRXPTCL_CTL, rf_mode_bw);
	rtl8xxxu_write8(priv, REG_DATA_SUBCHANNEL, subchannel);

	if (ht)
		val8 = 0x0e;
	else
		val8 = 0x0a;

	rtl8xxxu_write8(priv, REG_SIFS_CCK + 1, val8);
	rtl8xxxu_write8(priv, REG_SIFS_OFDM + 1, val8);

	rtl8xxxu_write16(priv, REG_R2T_SIFS, 0x0808);
	rtl8xxxu_write16(priv, REG_T2T_SIFS, 0x0a0a);

	for (i = RF_A; i < priv->rf_paths; i++) {
		val32 = rtl8xxxu_read_rfreg(priv, i, RF6052_REG_MODE_AG);
		val32 &= ~MODE_AG_BW_MASK;
		switch(hw->conf.chandef.width) {
		case NL80211_CHAN_WIDTH_80:
			val32 |= MODE_AG_BW_80MHZ_8723B;
			break;
		case NL80211_CHAN_WIDTH_40:
			val32 |= MODE_AG_BW_40MHZ_8723B;
			break;
		default:
			val32 |= MODE_AG_BW_20MHZ_8723B;
			break;
		}
		rtl8xxxu_write_rfreg(priv, i, RF6052_REG_MODE_AG, val32);
	}
}

static __attribute__((unused)) void rtl8723bu_update_bt_link_info(struct rtw_8723bu_private *priv, u8 bt_info)
{
	struct rtl8xxxu_btcoex *btcoex = &priv->bt_coex;

	if (bt_info & BT_INFO_8723B_1ANT_B_INQ_PAGE)
		btcoex->c2h_bt_inquiry = true;
	else
		btcoex->c2h_bt_inquiry = false;

	if (!(bt_info & BT_INFO_8723B_1ANT_B_CONNECTION)) {
		btcoex->bt_status = BT_8723B_1ANT_STATUS_NON_CONNECTED_IDLE;
		btcoex->has_sco = false;
		btcoex->has_hid = false;
		btcoex->has_pan = false;
		btcoex->has_a2dp = false;
	} else {
		if ((bt_info & 0x1f) == BT_INFO_8723B_1ANT_B_CONNECTION)
			btcoex->bt_status = BT_8723B_1ANT_STATUS_CONNECTED_IDLE;
		else if ((bt_info & BT_INFO_8723B_1ANT_B_SCO_ESCO) ||
			 (bt_info & BT_INFO_8723B_1ANT_B_SCO_BUSY))
			btcoex->bt_status = BT_8723B_1ANT_STATUS_SCO_BUSY;
		else if (bt_info & BT_INFO_8723B_1ANT_B_ACL_BUSY)
			btcoex->bt_status = BT_8723B_1ANT_STATUS_ACL_BUSY;
		else
			btcoex->bt_status = BT_8723B_1ANT_STATUS_MAX;

		if (bt_info & BT_INFO_8723B_1ANT_B_FTP)
			btcoex->has_pan = true;
		else
			btcoex->has_pan = false;

		if (bt_info & BT_INFO_8723B_1ANT_B_A2DP)
			btcoex->has_a2dp = true;
		else
			btcoex->has_a2dp = false;

		if (bt_info & BT_INFO_8723B_1ANT_B_HID)
			btcoex->has_hid = true;
		else
			btcoex->has_hid = false;

		if (bt_info & BT_INFO_8723B_1ANT_B_SCO_ESCO)
			btcoex->has_sco = true;
		else
			btcoex->has_sco = false;
	}

	if (!btcoex->has_a2dp && !btcoex->has_sco &&
	    !btcoex->has_pan && btcoex->has_hid)
		btcoex->hid_only = true;
	else
		btcoex->hid_only = false;

	if (!btcoex->has_sco && !btcoex->has_pan &&
	    !btcoex->has_hid && btcoex->has_a2dp)
		btcoex->a2dp_only = true;
	else
		btcoex->a2dp_only = false;

	if (btcoex->bt_status == BT_8723B_1ANT_STATUS_SCO_BUSY ||
	    btcoex->bt_status == BT_8723B_1ANT_STATUS_ACL_BUSY)
		btcoex->bt_busy = true;
	else
		btcoex->bt_busy = false;
}

static __attribute__((unused)) void rtl8723bu_set_coex_with_type(struct rtw_8723bu_private *priv, u8 type)
{
	switch (type) {
	case 0:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x55555555);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0x55555555);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	case 1:
	case 3:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x55555555);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0x5a5a5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	case 2:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x5a5a5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0x5a5a5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	case 4:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x5a5a5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0xaaaa5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	case 5:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x5a5a5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0xaa5a5a5a);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	case 6:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0x55555555);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0xaaaaaaaa);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	case 7:
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE1, 0xaaaaaaaa);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE2, 0xaaaaaaaa);
		rtl8xxxu_write32(priv, REG_BT_COEX_TABLE3, 0x00ffffff);
		rtl8xxxu_write8(priv, REG_BT_COEX_TABLE4, 0x03);
		break;
	default:
		break;
	}
}

static __attribute__((unused)) void rtl8723bu_handle_bt_info(struct rtw_8723bu_private *priv)
{
	struct rtl8xxxu_btcoex *btcoex;

	btcoex = &priv->bt_coex;

	if (rtl8xxxu_is_assoc(priv)) {
		u32 val32 = 0;
		u32 high_prio_tx = 0, high_prio_rx = 0;

		val32 = rtl8xxxu_read32(priv, 0x770);
		high_prio_tx = val32 & 0x0000ffff;
		high_prio_rx = (val32  & 0xffff0000) >> 16;

		if (btcoex->bt_busy) {
			if (btcoex->hid_only) {
				rtl8723bu_set_ps_tdma(priv, 0x61, 0x20,
						      0x3, 0x11, 0x11);
				rtl8723bu_set_coex_with_type(priv, 5);
			} else if (btcoex->a2dp_only) {
				rtl8723bu_set_ps_tdma(priv, 0x61, 0x35,
						      0x3, 0x11, 0x11);
				rtl8723bu_set_coex_with_type(priv, 4);
			} else if ((btcoex->has_a2dp && btcoex->has_pan) ||
				   (btcoex->has_hid && btcoex->has_a2dp &&
				    btcoex->has_pan)) {
				rtl8723bu_set_ps_tdma(priv, 0x51, 0x21,
						      0x3, 0x10, 0x10);
				rtl8723bu_set_coex_with_type(priv, 4);
			} else if (btcoex->has_hid && btcoex->has_a2dp) {
				rtl8723bu_set_ps_tdma(priv, 0x51, 0x21,
						      0x3, 0x10, 0x10);
				rtl8723bu_set_coex_with_type(priv, 3);
			} else {
				rtl8723bu_set_ps_tdma(priv, 0x61, 0x35,
						      0x3, 0x11, 0x11);
				rtl8723bu_set_coex_with_type(priv, 4);
			}
		} else {
			rtl8723bu_set_ps_tdma(priv, 0x8, 0x0, 0x0, 0x0, 0x0);
			if (high_prio_tx + high_prio_rx <= 60)
				rtl8723bu_set_coex_with_type(priv, 2);
			else
				rtl8723bu_set_coex_with_type(priv, 7);
		}
	} else {
		rtl8723bu_set_ps_tdma(priv, 0x8, 0x0, 0x0, 0x0, 0x0);
		rtl8723bu_set_coex_with_type(priv, 0);
	}
}

static __attribute__((unused)) void rtl8723bu_handle_bt_inquiry(struct rtw_8723bu_private *priv)
{
	struct rtl8xxxu_btcoex *btcoex;

	btcoex = &priv->bt_coex;

	if (!rtl8xxxu_is_assoc(priv)) {
		rtl8723bu_set_ps_tdma(priv, 0x8, 0x0, 0x0, 0x0, 0x0);
		rtl8723bu_set_coex_with_type(priv, 0);
	} else if (btcoex->has_sco || btcoex->has_hid || btcoex->has_a2dp) {
		rtl8723bu_set_ps_tdma(priv, 0x61, 0x35, 0x3, 0x11, 0x11);
		rtl8723bu_set_coex_with_type(priv, 4);
	} else if (btcoex->has_pan) {
		rtl8723bu_set_ps_tdma(priv, 0x61, 0x3f, 0x3, 0x11, 0x11);
		rtl8723bu_set_coex_with_type(priv, 4);
	} else {
		rtl8723bu_set_ps_tdma(priv, 0x8, 0x0, 0x0, 0x0, 0x0);
		rtl8723bu_set_coex_with_type(priv, 7);
	}
}

