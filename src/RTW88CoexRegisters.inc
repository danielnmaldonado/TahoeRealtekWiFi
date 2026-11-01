/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 * Copyright(c)2018-2019 Realtek Corporation. Generated from pinned Linux.
 * This port selects BSD-3-Clause; see RTW88Coex-Upstream.json. */
#ifndef __RTW_REG_DEF_H__
#define __RTW_REG_DEF_H__
#endif

#ifndef REG_SYS_FUNC_EN
#define REG_SYS_FUNC_EN		0x0002
#endif

#ifndef BIT_FEN_EN_25_1
#define BIT_FEN_EN_25_1		BIT(13)
#endif

#ifndef BIT_FEN_ELDR
#define BIT_FEN_ELDR		BIT(12)
#endif

#ifndef BIT_FEN_PCIEA
#define BIT_FEN_PCIEA		BIT(6)
#endif

#ifndef BIT_FEN_CPUEN
#define BIT_FEN_CPUEN		BIT(2)
#endif

#ifndef BIT_FEN_USBA
#define BIT_FEN_USBA		BIT(2)
#endif

#ifndef BIT_FEN_BB_GLB_RST
#define BIT_FEN_BB_GLB_RST	BIT(1)
#endif

#ifndef BIT_FEN_BB_RSTB
#define BIT_FEN_BB_RSTB		BIT(0)
#endif

#ifndef BIT_R_DIS_PRST
#define BIT_R_DIS_PRST		BIT(6)
#endif

#ifndef BIT_WLOCK_1C_B6
#define BIT_WLOCK_1C_B6		BIT(5)
#endif

#ifndef REG_SYS_PW_CTRL
#define REG_SYS_PW_CTRL		0x0004
#endif

#ifndef BIT_PFM_WOWL
#define BIT_PFM_WOWL		BIT(3)
#endif

#ifndef BIT_APFM_OFFMAC
#define BIT_APFM_OFFMAC		BIT(9)
#endif

#ifndef REG_APS_FSMCO
#define REG_APS_FSMCO		0x0004
#endif

#ifndef APS_FSMCO_MAC_ENABLE
#define APS_FSMCO_MAC_ENABLE	BIT(8)
#endif

#ifndef APS_FSMCO_MAC_OFF
#define APS_FSMCO_MAC_OFF	BIT(9)
#endif

#ifndef APS_FSMCO_HW_POWERDOWN
#define APS_FSMCO_HW_POWERDOWN	BIT(15)
#endif

#ifndef REG_SYS_CLK_CTRL
#define REG_SYS_CLK_CTRL	0x0008
#endif

#ifndef BIT_CPU_CLK_EN
#define BIT_CPU_CLK_EN		BIT(14)
#endif

#ifndef REG_SYS_CLKR
#define REG_SYS_CLKR		0x0008
#endif

#ifndef BIT_ANA8M
#define BIT_ANA8M		BIT(1)
#endif

#ifndef BIT_WAKEPAD_EN
#define BIT_WAKEPAD_EN		BIT(3)
#endif

#ifndef BIT_LOADER_CLK_EN
#define BIT_LOADER_CLK_EN	BIT(5)
#endif

#ifndef REG_RSV_CTRL
#define REG_RSV_CTRL		0x001C
#endif

#ifndef DISABLE_PI
#define DISABLE_PI		0x3
#endif

#ifndef ENABLE_PI
#define ENABLE_PI		0x2
#endif

#ifndef BITS_RFC_DIRECT
#define BITS_RFC_DIRECT		(BIT(31) | BIT(30))
#endif

#ifndef BIT_WLMCU_IOIF
#define BIT_WLMCU_IOIF		BIT(0)
#endif

#ifndef REG_RF_CTRL
#define REG_RF_CTRL		0x001F
#endif

#ifndef BIT_RF_SDM_RSTB
#define BIT_RF_SDM_RSTB		BIT(2)
#endif

#ifndef BIT_RF_RSTB
#define BIT_RF_RSTB		BIT(1)
#endif

#ifndef BIT_RF_EN
#define BIT_RF_EN		BIT(0)
#endif

#ifndef REG_RF_CTRL1
#define REG_RF_CTRL1		0x0020
#endif

#ifndef REG_RF_CTRL2
#define REG_RF_CTRL2		0x0021
#endif

#ifndef REG_AFE_CTRL1
#define REG_AFE_CTRL1		0x0024
#endif

#ifndef BIT_MAC_CLK_SEL
#define BIT_MAC_CLK_SEL		(BIT(20) | BIT(21))
#endif

#ifndef REG_EFUSE_CTRL
#define REG_EFUSE_CTRL		0x0030
#endif

#ifndef BIT_EF_FLAG
#define BIT_EF_FLAG		BIT(31)
#endif

#ifndef BIT_SHIFT_EF_ADDR
#define BIT_SHIFT_EF_ADDR	8
#endif

#ifndef BIT_MASK_EF_ADDR
#define BIT_MASK_EF_ADDR	0x3ff
#endif

#ifndef BIT_MASK_EF_DATA
#define BIT_MASK_EF_DATA	0xff
#endif

#ifndef BITS_EF_ADDR
#define BITS_EF_ADDR		(BIT_MASK_EF_ADDR << BIT_SHIFT_EF_ADDR)
#endif

#ifndef BITS_PLL
#define BITS_PLL		0xf0
#endif

#ifndef REG_AFE_XTAL_CTRL
#define REG_AFE_XTAL_CTRL	0x24
#endif

#ifndef REG_AFE_PLL_CTRL
#define REG_AFE_PLL_CTRL	0x28
#endif

#ifndef REG_AFE_CTRL3
#define REG_AFE_CTRL3		0x2c
#endif

#ifndef BIT_MASK_XTAL
#define BIT_MASK_XTAL		0x00FFF000
#endif

#ifndef BIT_XTAL_GMP_BIT4
#define BIT_XTAL_GMP_BIT4	BIT(28)
#endif

#ifndef REG_LDO_EFUSE_CTRL
#define REG_LDO_EFUSE_CTRL	0x0034
#endif

#ifndef BIT_MASK_EFUSE_BANK_SEL
#define BIT_MASK_EFUSE_BANK_SEL	(BIT(8) | BIT(9))
#endif

#ifndef BIT_LDO25_VOLTAGE_V25
#define BIT_LDO25_VOLTAGE_V25	0x03
#endif

#ifndef BIT_MASK_LDO25_VOLTAGE
#define BIT_MASK_LDO25_VOLTAGE	GENMASK(6, 4)
#endif

#ifndef BIT_SHIFT_LDO25_VOLTAGE
#define BIT_SHIFT_LDO25_VOLTAGE	4
#endif

#ifndef BIT_LDO25_EN
#define BIT_LDO25_EN		BIT(7)
#endif

#ifndef REG_ACLK_MON
#define REG_ACLK_MON		0x3e
#endif

#ifndef REG_GPIO_MUXCFG
#define REG_GPIO_MUXCFG		0x0040
#endif

#ifndef BIT_FSPI_EN
#define BIT_FSPI_EN		BIT(19)
#endif

#ifndef BIT_EN_SIC
#define BIT_EN_SIC		BIT(12)
#endif

#ifndef BIT_PO_BT_PTA_PINS
#define BIT_PO_BT_PTA_PINS	BIT(9)
#endif

#ifndef BIT_BT_PTA_EN
#define BIT_BT_PTA_EN		BIT(5)
#endif

#ifndef BIT_WLRFE_4_5_EN
#define BIT_WLRFE_4_5_EN	BIT(2)
#endif

#ifndef REG_GPIO_PIN_CTRL
#define REG_GPIO_PIN_CTRL	0x0044
#endif

#ifndef REG_LED_CFG
#define REG_LED_CFG		0x004C
#endif

#ifndef BIT_LNAON_SEL_EN
#define BIT_LNAON_SEL_EN	BIT(26)
#endif

#ifndef BIT_PAPE_SEL_EN
#define BIT_PAPE_SEL_EN		BIT(25)
#endif

#ifndef BIT_DPDT_WL_SEL
#define BIT_DPDT_WL_SEL		BIT(24)
#endif

#ifndef BIT_DPDT_SEL_EN
#define BIT_DPDT_SEL_EN		BIT(23)
#endif

#ifndef BIT_GPIO13_14_WL_CTRL_EN
#define BIT_GPIO13_14_WL_CTRL_EN	BIT(22)
#endif

#ifndef BIT_LED2_SV
#define BIT_LED2_SV		BIT(19)
#endif

#ifndef BIT_LED2_CM
#define BIT_LED2_CM		GENMASK(18, 16)
#endif

#ifndef BIT_LED1_SV
#define BIT_LED1_SV		BIT(11)
#endif

#ifndef BIT_LED1_CM
#define BIT_LED1_CM		GENMASK(10, 8)
#endif

#ifndef BIT_LED0_SV
#define BIT_LED0_SV		BIT(3)
#endif

#ifndef BIT_LED0_CM
#define BIT_LED0_CM		GENMASK(2, 0)
#endif

#ifndef BIT_LED_MODE_SW_CTRL
#define BIT_LED_MODE_SW_CTRL	0
#endif

#ifndef BIT_LED_MODE_RX
#define BIT_LED_MODE_RX		6
#endif

#ifndef BIT_LED_MODE_TX
#define BIT_LED_MODE_TX		4
#endif

#ifndef BIT_LED_MODE_TRX
#define BIT_LED_MODE_TRX	2
#endif

#ifndef REG_LEDCFG2
#define REG_LEDCFG2		0x004E
#endif

#ifndef REG_GPIO_PIN_CTRL_2
#define REG_GPIO_PIN_CTRL_2	0x0060
#endif

#ifndef REG_PAD_CTRL1
#define REG_PAD_CTRL1		0x0064
#endif

#ifndef BIT_BT_BTG_SEL
#define BIT_BT_BTG_SEL		BIT(31)
#endif

#ifndef BIT_PAPE_WLBT_SEL
#define BIT_PAPE_WLBT_SEL	BIT(29)
#endif

#ifndef BIT_LNAON_WLBT_SEL
#define BIT_LNAON_WLBT_SEL	BIT(28)
#endif

#ifndef BIT_BTGP_JTAG_EN
#define BIT_BTGP_JTAG_EN	BIT(24)
#endif

#ifndef BIT_BTGP_SPI_EN
#define BIT_BTGP_SPI_EN		BIT(20)
#endif

#ifndef BIT_LED1DIS
#define BIT_LED1DIS		BIT(15)
#endif

#ifndef BIT_SW_DPDT_SEL_DATA
#define BIT_SW_DPDT_SEL_DATA	BIT(0)
#endif

#ifndef REG_WL_BT_PWR_CTRL
#define REG_WL_BT_PWR_CTRL	0x0068
#endif

#ifndef BIT_BT_FUNC_EN
#define BIT_BT_FUNC_EN		BIT(18)
#endif

#ifndef BIT_BT_DIG_CLK_EN
#define BIT_BT_DIG_CLK_EN	BIT(8)
#endif

#ifndef REG_SYS_SDIO_CTRL
#define REG_SYS_SDIO_CTRL	0x0070
#endif

#ifndef BIT_DBG_GNT_WL_BT
#define BIT_DBG_GNT_WL_BT	BIT(27)
#endif

#ifndef BIT_LTE_MUX_CTRL_PATH
#define BIT_LTE_MUX_CTRL_PATH	BIT(26)
#endif

#ifndef REG_HCI_OPT_CTRL
#define REG_HCI_OPT_CTRL	0x0074
#endif

#ifndef BIT_USB_SUS_DIS
#define BIT_USB_SUS_DIS		BIT(8)
#endif

#ifndef BIT_SDIO_PAD_E5
#define BIT_SDIO_PAD_E5		BIT(18)
#endif

#ifndef REG_RF_B_CTRL
#define REG_RF_B_CTRL		0x76
#endif

#ifndef REG_RF_CTRL3
#define REG_RF_CTRL3		0x0076
#endif

#ifndef REG_AFE_CTRL_4
#define REG_AFE_CTRL_4		0x0078
#endif

#ifndef BIT_CK320M_AFE_EN
#define BIT_CK320M_AFE_EN	BIT(4)
#endif

#ifndef BIT_EN_SYN
#define BIT_EN_SYN		BIT(15)
#endif

#ifndef REG_LDO_SWR_CTRL
#define REG_LDO_SWR_CTRL	0x007C
#endif

#ifndef LDO_SEL
#define LDO_SEL			0xC3
#endif

#ifndef SPS_SEL
#define SPS_SEL			0x83
#endif

#ifndef BIT_XTA1
#define BIT_XTA1		BIT(29)
#endif

#ifndef BIT_XTA0
#define BIT_XTA0		BIT(28)
#endif

#ifndef REG_MCUFW_CTRL
#define REG_MCUFW_CTRL		0x0080
#endif

#ifndef BIT_ANA_PORT_EN
#define BIT_ANA_PORT_EN		BIT(22)
#endif

#ifndef BIT_MAC_PORT_EN
#define BIT_MAC_PORT_EN		BIT(21)
#endif

#ifndef BIT_BOOT_FSPI_EN
#define BIT_BOOT_FSPI_EN	BIT(20)
#endif

#ifndef BIT_ROM_DLEN
#define BIT_ROM_DLEN		BIT(19)
#endif

#ifndef BIT_ROM_PGE
#define BIT_ROM_PGE		GENMASK(18, 16)	/* legacy only */
#endif

#ifndef BIT_SHIFT_ROM_PGE
#define BIT_SHIFT_ROM_PGE	16
#endif

#ifndef BIT_FW_INIT_RDY
#define BIT_FW_INIT_RDY		BIT(15)
#endif

#ifndef BIT_FW_DW_RDY
#define BIT_FW_DW_RDY		BIT(14)
#endif

#ifndef BIT_CPU_CLK_SEL
#define BIT_CPU_CLK_SEL		(BIT(12) | BIT(13))
#endif

#ifndef BIT_RPWM_TOGGLE
#define BIT_RPWM_TOGGLE		BIT(7)
#endif

#ifndef BIT_RAM_DL_SEL
#define BIT_RAM_DL_SEL		BIT(7)	/* legacy only */
#endif

#ifndef BIT_DMEM_CHKSUM_OK
#define BIT_DMEM_CHKSUM_OK	BIT(6)
#endif

#ifndef BIT_WINTINI_RDY
#define BIT_WINTINI_RDY		BIT(6)	/* legacy only */
#endif

#ifndef BIT_DMEM_DW_OK
#define BIT_DMEM_DW_OK		BIT(5)
#endif

#ifndef BIT_IMEM_CHKSUM_OK
#define BIT_IMEM_CHKSUM_OK	BIT(4)
#endif

#ifndef BIT_IMEM_DW_OK
#define BIT_IMEM_DW_OK		BIT(3)
#endif

#ifndef BIT_IMEM_BOOT_LOAD_CHECKSUM_OK
#define BIT_IMEM_BOOT_LOAD_CHECKSUM_OK BIT(2)
#endif

#ifndef BIT_FWDL_CHK_RPT
#define BIT_FWDL_CHK_RPT	BIT(2)	/* legacy only */
#endif

#ifndef BIT_MCUFWDL_RDY
#define BIT_MCUFWDL_RDY		BIT(1)	/* legacy only */
#endif

#ifndef BIT_MCUFWDL_EN
#define BIT_MCUFWDL_EN		BIT(0)
#endif

#ifndef BIT_CHECK_SUM_OK
#define BIT_CHECK_SUM_OK	(BIT(4) | BIT(6))
#endif

#ifndef FW_READY
#define FW_READY		(BIT_FW_INIT_RDY | BIT_FW_DW_RDY |             \
				 BIT_IMEM_DW_OK | BIT_DMEM_DW_OK |             \
				 BIT_CHECK_SUM_OK)
#endif

#ifndef FW_READY_LEGACY
#define FW_READY_LEGACY		(BIT_MCUFWDL_RDY | BIT_FWDL_CHK_RPT |	       \
				 BIT_WINTINI_RDY | BIT_RAM_DL_SEL)
#endif

#ifndef FW_READY_MASK
#define FW_READY_MASK		(0xffff & ~BIT_CPU_CLK_SEL)
#endif

#ifndef REG_MCU_TST_CFG
#define REG_MCU_TST_CFG		0x84
#endif

#ifndef VAL_FW_TRIGGER
#define VAL_FW_TRIGGER		0x1
#endif

#ifndef REG_PMC_DBG_CTRL1
#define REG_PMC_DBG_CTRL1	0xa8
#endif

#ifndef BITS_PMC_BT_IQK_STS
#define BITS_PMC_BT_IQK_STS	GENMASK(22, 21)
#endif

#ifndef REG_HIMR0
#define REG_HIMR0		0xb0
#endif

#ifndef REG_HISR0
#define REG_HISR0		0xb4
#endif

#ifndef REG_HIMR1
#define REG_HIMR1		0xb8
#endif

#ifndef REG_HISR1
#define REG_HISR1		0xbc
#endif

#ifndef REG_PAD_CTRL2
#define REG_PAD_CTRL2		0x00C4
#endif

#ifndef BIT_RSM_EN_V1
#define BIT_RSM_EN_V1		BIT(16)
#endif

#ifndef BIT_NO_PDN_CHIPOFF_V1
#define BIT_NO_PDN_CHIPOFF_V1	BIT(17)
#endif

#ifndef BIT_MASK_USB23_SW_MODE_V1
#define BIT_MASK_USB23_SW_MODE_V1	GENMASK(19, 18)
#endif

#ifndef BIT_USB3_USB2_TRANSITION
#define BIT_USB3_USB2_TRANSITION	BIT(20)
#endif

#ifndef BIT_USB_MODE_U2
#define BIT_USB_MODE_U2		1
#endif

#ifndef BIT_USB_MODE_U3
#define BIT_USB_MODE_U3		2
#endif

#ifndef REG_EFUSE_ACCESS
#define REG_EFUSE_ACCESS	0x00CF
#endif

#ifndef EFUSE_ACCESS_ON
#define EFUSE_ACCESS_ON		0x69
#endif

#ifndef EFUSE_ACCESS_OFF
#define EFUSE_ACCESS_OFF	0x00
#endif

#ifndef REG_WLRF1
#define REG_WLRF1		0x00EC
#endif

#ifndef REG_WIFI_BT_INFO
#define REG_WIFI_BT_INFO	0x00AA
#endif

#ifndef BIT_BT_INT_EN
#define BIT_BT_INT_EN		BIT(15)
#endif

#ifndef REG_SYS_CFG1
#define REG_SYS_CFG1		0x00F0
#endif

#ifndef BIT_RTL_ID
#define	BIT_RTL_ID		BIT(23)
#endif

#ifndef BIT_LDO
#define BIT_LDO			BIT(24)
#endif

#ifndef BIT_RF_TYPE_ID
#define BIT_RF_TYPE_ID		BIT(27)
#endif

#ifndef BIT_SHIFT_VENDOR_ID
#define BIT_SHIFT_VENDOR_ID	16
#endif

#ifndef BIT_MASK_VENDOR_ID
#define BIT_MASK_VENDOR_ID	0xf
#endif

#ifndef BIT_VENDOR_ID
#define BIT_VENDOR_ID(x) (((x) & BIT_MASK_VENDOR_ID) << BIT_SHIFT_VENDOR_ID)
#endif

#ifndef BITS_VENDOR_ID
#define BITS_VENDOR_ID		(BIT_MASK_VENDOR_ID << BIT_SHIFT_VENDOR_ID)
#endif

#ifndef BIT_CLEAR_VENDOR_ID
#define BIT_CLEAR_VENDOR_ID(x)	((x) & (~BITS_VENDOR_ID))
#endif

#ifndef BIT_GET_VENDOR_ID
#define BIT_GET_VENDOR_ID(x) (((x) >> BIT_SHIFT_VENDOR_ID) & BIT_MASK_VENDOR_ID)
#endif

#ifndef BIT_SHIFT_CHIP_VER
#define BIT_SHIFT_CHIP_VER	12
#endif

#ifndef BIT_MASK_CHIP_VER
#define BIT_MASK_CHIP_VER	0xf
#endif

#ifndef BIT_CHIP_VER
#define BIT_CHIP_VER(x)	 (((x) & BIT_MASK_CHIP_VER) << BIT_SHIFT_CHIP_VER)
#endif

#ifndef BITS_CHIP_VER
#define BITS_CHIP_VER		(BIT_MASK_CHIP_VER << BIT_SHIFT_CHIP_VER)
#endif

#ifndef BIT_CLEAR_CHIP_VER
#define BIT_CLEAR_CHIP_VER(x)	((x) & (~BITS_CHIP_VER))
#endif

#ifndef BIT_GET_CHIP_VER
#define BIT_GET_CHIP_VER(x) (((x) >> BIT_SHIFT_CHIP_VER) & BIT_MASK_CHIP_VER)
#endif

#ifndef REG_SYS_STATUS1
#define REG_SYS_STATUS1		0x00F4
#endif

#ifndef REG_SYS_STATUS2
#define REG_SYS_STATUS2		0x00F8
#endif

#ifndef REG_SYS_CFG2
#define REG_SYS_CFG2		0x00FC
#endif

#ifndef BIT_WLRF1_BBRF_EN
#define BIT_WLRF1_BBRF_EN	(BIT(24) | BIT(25) | BIT(26))
#endif

#ifndef REG_CR
#define REG_CR			0x0100
#endif

#ifndef BIT_32K_CAL_TMR_EN
#define BIT_32K_CAL_TMR_EN	BIT(10)
#endif

#ifndef BIT_MAC_SEC_EN
#define BIT_MAC_SEC_EN		BIT(9)
#endif

#ifndef BIT_ENSWBCN
#define BIT_ENSWBCN		BIT(8)
#endif

#ifndef BIT_MACRXEN
#define BIT_MACRXEN		BIT(7)
#endif

#ifndef BIT_MACTXEN
#define BIT_MACTXEN		BIT(6)
#endif

#ifndef BIT_SCHEDULE_EN
#define BIT_SCHEDULE_EN		BIT(5)
#endif

#ifndef BIT_PROTOCOL_EN
#define BIT_PROTOCOL_EN		BIT(4)
#endif

#ifndef BIT_RXDMA_EN
#define BIT_RXDMA_EN		BIT(3)
#endif

#ifndef BIT_TXDMA_EN
#define BIT_TXDMA_EN		BIT(2)
#endif

#ifndef BIT_HCI_RXDMA_EN
#define BIT_HCI_RXDMA_EN	BIT(1)
#endif

#ifndef BIT_HCI_TXDMA_EN
#define BIT_HCI_TXDMA_EN	BIT(0)
#endif

#ifndef MAC_TRX_ENABLE
#define MAC_TRX_ENABLE	(BIT_HCI_TXDMA_EN | BIT_HCI_RXDMA_EN | BIT_TXDMA_EN | \
			BIT_RXDMA_EN | BIT_PROTOCOL_EN | BIT_SCHEDULE_EN | \
			BIT_MACTXEN | BIT_MACRXEN)
#endif

#ifndef REG_PBP
#define REG_PBP			0x104
#endif

#ifndef PBP_RX_MASK
#define PBP_RX_MASK		0x0f
#endif

#ifndef PBP_TX_MASK
#define PBP_TX_MASK		0xf0
#endif

#ifndef PBP_64
#define PBP_64			0x0
#endif

#ifndef PBP_128
#define PBP_128			0x1
#endif

#ifndef PBP_256
#define PBP_256			0x2
#endif

#ifndef PBP_512
#define PBP_512			0x3
#endif

#ifndef PBP_1024
#define PBP_1024		0x4
#endif

#ifndef BIT_SHIFT_TXDMA_VOQ_MAP
#define BIT_SHIFT_TXDMA_VOQ_MAP	4
#endif

#ifndef BIT_MASK_TXDMA_VOQ_MAP
#define BIT_MASK_TXDMA_VOQ_MAP	0x3
#endif

#ifndef BIT_TXDMA_VOQ_MAP
#define BIT_TXDMA_VOQ_MAP(x)                                                   \
	(((x) & BIT_MASK_TXDMA_VOQ_MAP) << BIT_SHIFT_TXDMA_VOQ_MAP)
#endif

#ifndef BIT_SHIFT_TXDMA_VIQ_MAP
#define BIT_SHIFT_TXDMA_VIQ_MAP	6
#endif

#ifndef BIT_MASK_TXDMA_VIQ_MAP
#define BIT_MASK_TXDMA_VIQ_MAP	0x3
#endif

#ifndef BIT_TXDMA_VIQ_MAP
#define BIT_TXDMA_VIQ_MAP(x)                                                   \
	(((x) & BIT_MASK_TXDMA_VIQ_MAP) << BIT_SHIFT_TXDMA_VIQ_MAP)
#endif

#ifndef REG_TXDMA_PQ_MAP
#define REG_TXDMA_PQ_MAP	0x010C
#endif

#ifndef BIT_RXDMA_ARBBW_EN
#define BIT_RXDMA_ARBBW_EN	BIT(0)
#endif

#ifndef BIT_RXSHFT_EN
#define BIT_RXSHFT_EN		BIT(1)
#endif

#ifndef BIT_RXDMA_AGG_EN
#define BIT_RXDMA_AGG_EN	BIT(2)
#endif

#ifndef BIT_TXDMA_BW_EN
#define BIT_TXDMA_BW_EN		BIT(3)
#endif

#ifndef BIT_SHIFT_TXDMA_BEQ_MAP
#define BIT_SHIFT_TXDMA_BEQ_MAP	8
#endif

#ifndef BIT_MASK_TXDMA_BEQ_MAP
#define BIT_MASK_TXDMA_BEQ_MAP	0x3
#endif

#ifndef BIT_TXDMA_BEQ_MAP
#define BIT_TXDMA_BEQ_MAP(x)                                                   \
	(((x) & BIT_MASK_TXDMA_BEQ_MAP) << BIT_SHIFT_TXDMA_BEQ_MAP)
#endif

#ifndef BIT_SHIFT_TXDMA_BKQ_MAP
#define BIT_SHIFT_TXDMA_BKQ_MAP	10
#endif

#ifndef BIT_MASK_TXDMA_BKQ_MAP
#define BIT_MASK_TXDMA_BKQ_MAP	0x3
#endif

#ifndef BIT_TXDMA_BKQ_MAP
#define BIT_TXDMA_BKQ_MAP(x)                                                   \
	(((x) & BIT_MASK_TXDMA_BKQ_MAP) << BIT_SHIFT_TXDMA_BKQ_MAP)
#endif

#ifndef BIT_SHIFT_TXDMA_MGQ_MAP
#define BIT_SHIFT_TXDMA_MGQ_MAP	12
#endif

#ifndef BIT_MASK_TXDMA_MGQ_MAP
#define BIT_MASK_TXDMA_MGQ_MAP	0x3
#endif

#ifndef BIT_TXDMA_MGQ_MAP
#define BIT_TXDMA_MGQ_MAP(x)                                                   \
	(((x) & BIT_MASK_TXDMA_MGQ_MAP) << BIT_SHIFT_TXDMA_MGQ_MAP)
#endif

#ifndef BIT_SHIFT_TXDMA_HIQ_MAP
#define BIT_SHIFT_TXDMA_HIQ_MAP	14
#endif

#ifndef BIT_MASK_TXDMA_HIQ_MAP
#define BIT_MASK_TXDMA_HIQ_MAP	0x3
#endif

#ifndef BIT_TXDMA_HIQ_MAP
#define BIT_TXDMA_HIQ_MAP(x)                                                   \
	(((x) & BIT_MASK_TXDMA_HIQ_MAP) << BIT_SHIFT_TXDMA_HIQ_MAP)
#endif

#ifndef BIT_SHIFT_TXSC_40M
#define BIT_SHIFT_TXSC_40M	4
#endif

#ifndef BIT_MASK_TXSC_40M
#define BIT_MASK_TXSC_40M	0xf
#endif

#ifndef BIT_TXSC_40M
#define BIT_TXSC_40M(x)							       \
	(((x) & BIT_MASK_TXSC_40M) << BIT_SHIFT_TXSC_40M)
#endif

#ifndef BIT_SHIFT_TXSC_20M
#define BIT_SHIFT_TXSC_20M	0
#endif

#ifndef BIT_MASK_TXSC_20M
#define BIT_MASK_TXSC_20M	0xf
#endif

#ifndef BIT_TXSC_20M
#define BIT_TXSC_20M(x)							       \
	(((x) & BIT_MASK_TXSC_20M) << BIT_SHIFT_TXSC_20M)
#endif

#ifndef BIT_SHIFT_MAC_CLK_SEL
#define BIT_SHIFT_MAC_CLK_SEL	20
#endif

#ifndef MAC_CLK_HW_DEF_80M
#define MAC_CLK_HW_DEF_80M	0
#endif

#ifndef MAC_CLK_HW_DEF_40M
#define MAC_CLK_HW_DEF_40M	1
#endif

#ifndef MAC_CLK_HW_DEF_20M
#define MAC_CLK_HW_DEF_20M	2
#endif

#ifndef MAC_CLK_SPEED
#define MAC_CLK_SPEED		80
#endif

#ifndef REG_TRXFF_BNDY
#define REG_TRXFF_BNDY		0x0114
#endif

#ifndef REG_RXFF_BNDY
#define REG_RXFF_BNDY		0x011C
#endif

#ifndef REG_FE1IMR
#define REG_FE1IMR		0x0120
#endif

#ifndef BIT_FS_RXDONE
#define BIT_FS_RXDONE		BIT(16)
#endif

#ifndef REG_CPWM
#define REG_CPWM		0x012C
#endif

#ifndef REG_FWIMR
#define REG_FWIMR		0x0130
#endif

#ifndef BIT_FS_H2CCMD_INT_EN
#define BIT_FS_H2CCMD_INT_EN	BIT(4)
#endif

#ifndef BIT_FS_HRCV_INT_EN
#define BIT_FS_HRCV_INT_EN	BIT(5)
#endif

#ifndef REG_FWISR
#define REG_FWISR		0x0134
#endif

#ifndef BIT_FS_H2CCMD_INT
#define BIT_FS_H2CCMD_INT	BIT(4)
#endif

#ifndef BIT_FS_HRCV_INT
#define BIT_FS_HRCV_INT		BIT(5)
#endif

#ifndef REG_PKTBUF_DBG_CTRL
#define REG_PKTBUF_DBG_CTRL	0x0140
#endif

#ifndef REG_C2HEVT
#define REG_C2HEVT		0x01A0
#endif

#ifndef REG_MCUTST_1
#define REG_MCUTST_1		0x01C0
#endif

#ifndef REG_MCUTST_II
#define REG_MCUTST_II		0x01C4
#endif

#ifndef REG_WOWLAN_WAKE_REASON
#define REG_WOWLAN_WAKE_REASON	0x01C7
#endif

#ifndef REG_HMETFR
#define REG_HMETFR		0x01CC
#endif

#ifndef BIT_INT_BOX0
#define BIT_INT_BOX0		BIT(0)
#endif

#ifndef BIT_INT_BOX1
#define BIT_INT_BOX1		BIT(1)
#endif

#ifndef BIT_INT_BOX2
#define BIT_INT_BOX2		BIT(2)
#endif

#ifndef BIT_INT_BOX3
#define BIT_INT_BOX3		BIT(3)
#endif

#ifndef BIT_INT_BOX_ALL
#define BIT_INT_BOX_ALL		(BIT_INT_BOX0 | BIT_INT_BOX1 | BIT_INT_BOX2 | \
				 BIT_INT_BOX3)
#endif

#ifndef REG_HMEBOX0
#define REG_HMEBOX0		0x01D0
#endif

#ifndef REG_HMEBOX1
#define REG_HMEBOX1		0x01D4
#endif

#ifndef REG_HMEBOX2
#define REG_HMEBOX2		0x01D8
#endif

#ifndef REG_HMEBOX3
#define REG_HMEBOX3		0x01DC
#endif

#ifndef REG_LLT_INIT
#define REG_LLT_INIT		0x01E0
#endif

#ifndef BIT_LLT_WRITE_ACCESS
#define BIT_LLT_WRITE_ACCESS	BIT(30)
#endif

#ifndef REG_HMEBOX0_EX
#define REG_HMEBOX0_EX		0x01F0
#endif

#ifndef REG_HMEBOX1_EX
#define REG_HMEBOX1_EX		0x01F4
#endif

#ifndef REG_HMEBOX2_EX
#define REG_HMEBOX2_EX		0x01F8
#endif

#ifndef REG_HMEBOX3_EX
#define REG_HMEBOX3_EX		0x01FC
#endif

#ifndef REG_RQPN
#define REG_RQPN		0x0200
#endif

#ifndef BIT_MASK_HPQ
#define BIT_MASK_HPQ		0xff
#endif

#ifndef BIT_SHIFT_HPQ
#define BIT_SHIFT_HPQ		0
#endif

#ifndef BIT_RQPN_HPQ
#define BIT_RQPN_HPQ(x)		(((x) & BIT_MASK_HPQ) << BIT_SHIFT_HPQ)
#endif

#ifndef BIT_MASK_LPQ
#define BIT_MASK_LPQ		0xff
#endif

#ifndef BIT_SHIFT_LPQ
#define BIT_SHIFT_LPQ		8
#endif

#ifndef BIT_RQPN_LPQ
#define BIT_RQPN_LPQ(x)		(((x) & BIT_MASK_LPQ) << BIT_SHIFT_LPQ)
#endif

#ifndef BIT_MASK_PUBQ
#define BIT_MASK_PUBQ		0xff
#endif

#ifndef BIT_SHIFT_PUBQ
#define BIT_SHIFT_PUBQ		16
#endif

#ifndef BIT_RQPN_PUBQ
#define BIT_RQPN_PUBQ(x)	(((x) & BIT_MASK_PUBQ) << BIT_SHIFT_PUBQ)
#endif

#ifndef BIT_RQPN_HLP
#define BIT_RQPN_HLP(h, l, p)	(BIT_LD_RQPN | BIT_RQPN_HPQ(h) |	       \
				 BIT_RQPN_LPQ(l) | BIT_RQPN_PUBQ(p))
#endif

#ifndef REG_FIFOPAGE_CTRL_2
#define REG_FIFOPAGE_CTRL_2	0x0204
#endif

#ifndef BIT_BCN_VALID_V1
#define BIT_BCN_VALID_V1	BIT(15)
#endif

#ifndef BIT_MASK_BCN_HEAD_1_V1
#define BIT_MASK_BCN_HEAD_1_V1	0xfff
#endif

#ifndef REG_AUTO_LLT_V1
#define REG_AUTO_LLT_V1		0x0208
#endif

#ifndef BIT_AUTO_INIT_LLT_V1
#define BIT_AUTO_INIT_LLT_V1	BIT(0)
#endif

#ifndef BIT_MASK_BLK_DESC_NUM
#define BIT_MASK_BLK_DESC_NUM	GENMASK(7, 4)
#endif

#ifndef REG_DWBCN0_CTRL
#define REG_DWBCN0_CTRL		0x0208
#endif

#ifndef BIT_BCN_VALID
#define BIT_BCN_VALID		BIT(16)
#endif

#ifndef REG_TXDMA_OFFSET_CHK
#define REG_TXDMA_OFFSET_CHK	0x020C
#endif

#ifndef BIT_DROP_DATA_EN
#define BIT_DROP_DATA_EN	BIT(9)
#endif

#ifndef REG_TXDMA_STATUS
#define REG_TXDMA_STATUS	0x0210
#endif

#ifndef BTI_PAGE_OVF
#define BTI_PAGE_OVF		BIT(2)
#endif

#ifndef REG_RQPN_NPQ
#define REG_RQPN_NPQ		0x0214
#endif

#ifndef BIT_MASK_NPQ
#define BIT_MASK_NPQ		0xff
#endif

#ifndef BIT_SHIFT_NPQ
#define BIT_SHIFT_NPQ		0
#endif

#ifndef BIT_MASK_EPQ
#define BIT_MASK_EPQ		0xff
#endif

#ifndef BIT_SHIFT_EPQ
#define BIT_SHIFT_EPQ		16
#endif

#ifndef BIT_RQPN_NPQ
#define BIT_RQPN_NPQ(x)		(((x) & BIT_MASK_NPQ) << BIT_SHIFT_NPQ)
#endif

#ifndef BIT_RQPN_EPQ
#define BIT_RQPN_EPQ(x)		(((x) & BIT_MASK_EPQ) << BIT_SHIFT_EPQ)
#endif

#ifndef BIT_RQPN_NE
#define BIT_RQPN_NE(n, e)	(BIT_RQPN_NPQ(n) | BIT_RQPN_EPQ(e))
#endif

#ifndef REG_AUTO_LLT
#define REG_AUTO_LLT		0x0224
#endif

#ifndef BIT_AUTO_INIT_LLT
#define BIT_AUTO_INIT_LLT	BIT(16)
#endif

#ifndef REG_DWBCN1_CTRL
#define REG_DWBCN1_CTRL		0x0228
#endif

#ifndef REG_RQPN_CTRL_1
#define REG_RQPN_CTRL_1		0x0228
#endif

#ifndef REG_RQPN_CTRL_2
#define REG_RQPN_CTRL_2		0x022C
#endif

#ifndef BIT_LD_RQPN
#define BIT_LD_RQPN		BIT(31)
#endif

#ifndef REG_FIFOPAGE_INFO_1
#define REG_FIFOPAGE_INFO_1	0x0230
#endif

#ifndef REG_FIFOPAGE_INFO_2
#define REG_FIFOPAGE_INFO_2	0x0234
#endif

#ifndef REG_FIFOPAGE_INFO_3
#define REG_FIFOPAGE_INFO_3	0x0238
#endif

#ifndef REG_FIFOPAGE_INFO_4
#define REG_FIFOPAGE_INFO_4	0x023C
#endif

#ifndef REG_FIFOPAGE_INFO_5
#define REG_FIFOPAGE_INFO_5	0x0240
#endif

#ifndef REG_H2C_HEAD
#define REG_H2C_HEAD		0x0244
#endif

#ifndef REG_H2C_TAIL
#define REG_H2C_TAIL		0x0248
#endif

#ifndef REG_H2C_READ_ADDR
#define REG_H2C_READ_ADDR	0x024C
#endif

#ifndef REG_H2C_INFO
#define REG_H2C_INFO		0x0254
#endif

#ifndef REG_RXDMA_AGG_PG_TH
#define REG_RXDMA_AGG_PG_TH	0x0280
#endif

#ifndef BIT_RXDMA_AGG_PG_TH
#define BIT_RXDMA_AGG_PG_TH	GENMASK(7, 0)
#endif

#ifndef BIT_DMA_AGG_TO_V1
#define BIT_DMA_AGG_TO_V1	GENMASK(15, 8)
#endif

#ifndef BIT_EN_PRE_CALC
#define BIT_EN_PRE_CALC		BIT(29)
#endif

#ifndef REG_RXPKT_NUM
#define REG_RXPKT_NUM		0x0284
#endif

#ifndef BIT_RXDMA_REQ
#define BIT_RXDMA_REQ		BIT(19)
#endif

#ifndef BIT_RW_RELEASE
#define BIT_RW_RELEASE		BIT(18)
#endif

#ifndef BIT_RXDMA_IDLE
#define BIT_RXDMA_IDLE		BIT(17)
#endif

#ifndef REG_RXDMA_STATUS
#define REG_RXDMA_STATUS	0x0288
#endif

#ifndef REG_RXDMA_DPR
#define REG_RXDMA_DPR		0x028C
#endif

#ifndef REG_RXDMA_MODE
#define REG_RXDMA_MODE		0x0290
#endif

#ifndef BIT_DMA_MODE
#define BIT_DMA_MODE		BIT(1)
#endif

#ifndef BIT_DMA_BURST_CNT
#define BIT_DMA_BURST_CNT	GENMASK(3, 2)
#endif

#ifndef BIT_DMA_BURST_SIZE
#define BIT_DMA_BURST_SIZE	GENMASK(5, 4)
#endif

#ifndef BIT_DMA_BURST_SIZE_64
#define BIT_DMA_BURST_SIZE_64	2
#endif

#ifndef BIT_DMA_BURST_SIZE_512
#define BIT_DMA_BURST_SIZE_512	1
#endif

#ifndef BIT_DMA_BURST_SIZE_1024
#define BIT_DMA_BURST_SIZE_1024	0
#endif

#ifndef REG_RXPKTNUM
#define REG_RXPKTNUM		0x02B0
#endif

#ifndef REG_EARLY_MODE_CONTROL
#define REG_EARLY_MODE_CONTROL	0x02BC
#endif

#ifndef REG_INT_MIG
#define REG_INT_MIG		0x0304
#endif

#ifndef REG_HCI_MIX_CFG
#define REG_HCI_MIX_CFG		0x03FC
#endif

#ifndef BIT_PCIE_EMAC_PDN_AUX_TO_FAST_CLK
#define BIT_PCIE_EMAC_PDN_AUX_TO_FAST_CLK BIT(26)
#endif

#ifndef REG_BCNQ_INFO
#define REG_BCNQ_INFO		0x0418
#endif

#ifndef BIT_MGQ_CPU_EMPTY
#define BIT_MGQ_CPU_EMPTY	BIT(24)
#endif

#ifndef REG_TXPKT_EMPTY
#define REG_TXPKT_EMPTY		0x041A
#endif

#ifndef REG_FWHW_TXQ_CTRL
#define REG_FWHW_TXQ_CTRL	0x0420
#endif

#ifndef BIT_EN_BCNQ_DL
#define BIT_EN_BCNQ_DL		BIT(22)
#endif

#ifndef BIT_EN_WR_FREE_TAIL
#define BIT_EN_WR_FREE_TAIL	BIT(20)
#endif

#ifndef REG_HWSEQ_CTRL
#define REG_HWSEQ_CTRL		0x0423
#endif

#ifndef REG_BCNQ_BDNY_V1
#define REG_BCNQ_BDNY_V1	0x0424
#endif

#ifndef REG_BCNQ_BDNY
#define REG_BCNQ_BDNY		0x0424
#endif

#ifndef REG_MGQ_BDNY
#define REG_MGQ_BDNY		0x0425
#endif

#ifndef REG_LIFETIME_EN
#define REG_LIFETIME_EN		0x0426
#endif

#ifndef BIT_BA_PARSER_EN
#define BIT_BA_PARSER_EN	BIT(5)
#endif

#ifndef REG_SPEC_SIFS
#define REG_SPEC_SIFS		0x0428
#endif

#ifndef REG_RETRY_LIMIT
#define REG_RETRY_LIMIT		0x042a
#endif

#ifndef REG_DARFRC
#define REG_DARFRC		0x0430
#endif

#ifndef REG_DARFRCH
#define REG_DARFRCH		0x0434
#endif

#ifndef REG_RARFRCH
#define REG_RARFRCH		0x043C
#endif

#ifndef REG_RRSR
#define REG_RRSR		0x0440
#endif

#ifndef BITS_RRSR_RSC
#define BITS_RRSR_RSC		GENMASK(22, 21)
#endif

#ifndef REG_ARFR0
#define REG_ARFR0		0x0444
#endif

#ifndef REG_ARFRH0
#define REG_ARFRH0		0x0448
#endif

#ifndef REG_ARFR1_V1
#define REG_ARFR1_V1		0x044C
#endif

#ifndef REG_ARFRH1_V1
#define REG_ARFRH1_V1		0x0450
#endif

#ifndef REG_CCK_CHECK
#define REG_CCK_CHECK		0x0454
#endif

#ifndef BIT_CHECK_CCK_EN
#define BIT_CHECK_CCK_EN	BIT(7)
#endif

#ifndef REG_AMPDU_MAX_TIME_V1
#define REG_AMPDU_MAX_TIME_V1	0x0455
#endif

#ifndef REG_BCNQ1_BDNY_V1
#define REG_BCNQ1_BDNY_V1	0x0456
#endif

#ifndef REG_AMPDU_MAX_TIME
#define REG_AMPDU_MAX_TIME	0x0456
#endif

#ifndef REG_AMPDU_MAX_LENGTH
#define REG_AMPDU_MAX_LENGTH	0x0458
#endif

#ifndef REG_WMAC_LBK_BF_HD
#define REG_WMAC_LBK_BF_HD	0x045D
#endif

#ifndef REG_TX_HANG_CTRL
#define REG_TX_HANG_CTRL	0x045E
#endif

#ifndef BIT_EN_GNT_BT_AWAKE
#define BIT_EN_GNT_BT_AWAKE	BIT(3)
#endif

#ifndef BIT_EN_EOF_V1
#define BIT_EN_EOF_V1		BIT(2)
#endif

#ifndef REG_FAST_EDCA_CTRL
#define REG_FAST_EDCA_CTRL	0x0460
#endif

#ifndef REG_DATA_SC
#define REG_DATA_SC		0x0483
#endif

#ifndef REG_ARFR2_V1
#define REG_ARFR2_V1		0x048C
#endif

#ifndef REG_ARFRH2_V1
#define REG_ARFRH2_V1		0x0490
#endif

#ifndef REG_ARFR3_V1
#define REG_ARFR3_V1		0x0494
#endif

#ifndef BIT_EXC_CODE
#define BIT_EXC_CODE		GENMASK(6, 2)
#endif

#ifndef REG_ARFRH3_V1
#define REG_ARFRH3_V1		0x0498
#endif

#ifndef REG_ARFR4
#define REG_ARFR4		0x049C
#endif

#ifndef BIT_WL_RFK
#define BIT_WL_RFK		BIT(0)
#endif

#ifndef REG_ARFRH4
#define REG_ARFRH4		0x04A0
#endif

#ifndef REG_ARFR5
#define REG_ARFR5		0x04A4
#endif

#ifndef REG_ARFRH5
#define REG_ARFRH5		0x04A8
#endif

#ifndef REG_SW_AMPDU_BURST_MODE_CTRL
#define REG_SW_AMPDU_BURST_MODE_CTRL 0x04BC
#endif

#ifndef BIT_PRE_TX_CMD
#define BIT_PRE_TX_CMD		BIT(6)
#endif

#ifndef REG_QUEUE_CTRL
#define REG_QUEUE_CTRL		0x04C6
#endif

#ifndef BIT_PTA_WL_TX_EN
#define BIT_PTA_WL_TX_EN	BIT(4)
#endif

#ifndef BIT_PTA_EDCCA_EN
#define BIT_PTA_EDCCA_EN	BIT(5)
#endif

#ifndef REG_SINGLE_AMPDU_CTRL
#define REG_SINGLE_AMPDU_CTRL	0x04C7
#endif

#ifndef BIT_EN_SINGLE_APMDU
#define BIT_EN_SINGLE_APMDU	BIT(7)
#endif

#ifndef REG_PROT_MODE_CTRL
#define REG_PROT_MODE_CTRL	0x04C8
#endif

#ifndef REG_MAX_AGGR_NUM
#define REG_MAX_AGGR_NUM	0x04CA
#endif

#ifndef REG_BAR_MODE_CTRL
#define REG_BAR_MODE_CTRL	0x04CC
#endif

#ifndef REG_PRECNT_CTRL
#define REG_PRECNT_CTRL		0x04E5
#endif

#ifndef BIT_BTCCA_CTRL
#define BIT_BTCCA_CTRL		(BIT(0) | BIT(1))
#endif

#ifndef BIT_EN_PRECNT
#define BIT_EN_PRECNT		BIT(11)
#endif

#ifndef REG_TX_RPT_CTRL
#define REG_TX_RPT_CTRL		0x04EC
#endif

#ifndef REG_TX_RPT_TIME
#define REG_TX_RPT_TIME		0x04F0
#endif

#ifndef REG_DUMMY_PAGE4_V1
#define REG_DUMMY_PAGE4_V1	0x04FC
#endif

#ifndef REG_EDCA_VO_PARAM
#define REG_EDCA_VO_PARAM	0x0500
#endif

#ifndef REG_EDCA_VI_PARAM
#define REG_EDCA_VI_PARAM	0x0504
#endif

#ifndef REG_EDCA_BE_PARAM
#define REG_EDCA_BE_PARAM	0x0508
#endif

#ifndef REG_EDCA_BK_PARAM
#define REG_EDCA_BK_PARAM	0x050C
#endif

#ifndef BIT_MASK_TXOP_LMT
#define BIT_MASK_TXOP_LMT	GENMASK(26, 16)
#endif

#ifndef BIT_MASK_CWMAX
#define BIT_MASK_CWMAX		GENMASK(15, 12)
#endif

#ifndef BIT_MASK_CWMIN
#define BIT_MASK_CWMIN		GENMASK(11, 8)
#endif

#ifndef BIT_MASK_AIFS
#define BIT_MASK_AIFS		GENMASK(7, 0)
#endif

#ifndef REG_BCNTCFG
#define REG_BCNTCFG		0x0510
#endif

#ifndef REG_PIFS
#define REG_PIFS		0x0512
#endif

#ifndef REG_SIFS
#define REG_SIFS		0x0514
#endif

#ifndef BIT_SHIFT_SIFS_OFDM_CTX
#define BIT_SHIFT_SIFS_OFDM_CTX	8
#endif

#ifndef BIT_SHIFT_SIFS_CCK_TRX
#define BIT_SHIFT_SIFS_CCK_TRX	16
#endif

#ifndef BIT_SHIFT_SIFS_OFDM_TRX
#define BIT_SHIFT_SIFS_OFDM_TRX	24
#endif

#ifndef REG_AGGR_BREAK_TIME
#define REG_AGGR_BREAK_TIME	0x051A
#endif

#ifndef REG_SLOT
#define REG_SLOT		0x051B
#endif

#ifndef REG_TX_PTCL_CTRL
#define REG_TX_PTCL_CTRL	0x0520
#endif

#ifndef BIT_DIS_EDCCA
#define BIT_DIS_EDCCA		BIT(15)
#endif

#ifndef BIT_SIFS_BK_EN
#define BIT_SIFS_BK_EN		BIT(12)
#endif

#ifndef REG_TXPAUSE
#define REG_TXPAUSE		0x0522
#endif

#ifndef BIT_AC_QUEUE
#define BIT_AC_QUEUE		GENMASK(7, 0)
#endif

#ifndef BIT_HIGH_QUEUE
#define BIT_HIGH_QUEUE		BIT(5)
#endif

#ifndef REG_RD_CTRL
#define REG_RD_CTRL		0x0524
#endif

#ifndef BIT_EDCCA_MSK_CNTDOWN_EN
#define BIT_EDCCA_MSK_CNTDOWN_EN BIT(11)
#endif

#ifndef BIT_DIS_TXOP_CFE
#define BIT_DIS_TXOP_CFE	BIT(10)
#endif

#ifndef BIT_DIS_LSIG_CFE
#define BIT_DIS_LSIG_CFE	BIT(9)
#endif

#ifndef BIT_DIS_STBC_CFE
#define BIT_DIS_STBC_CFE	BIT(8)
#endif

#ifndef REG_TBTT_PROHIBIT
#define REG_TBTT_PROHIBIT	0x0540
#endif

#ifndef BIT_SHIFT_TBTT_HOLD_TIME_AP
#define BIT_SHIFT_TBTT_HOLD_TIME_AP 8
#endif

#ifndef REG_RD_NAV_NXT
#define REG_RD_NAV_NXT		0x0544
#endif

#ifndef REG_NAV_PROT_LEN
#define REG_NAV_PROT_LEN	0x0546
#endif

#ifndef REG_BCN_CTRL
#define REG_BCN_CTRL		0x0550
#endif

#ifndef BIT_DIS_TSF_UDT
#define BIT_DIS_TSF_UDT		BIT(4)
#endif

#ifndef BIT_EN_BCN_FUNCTION
#define BIT_EN_BCN_FUNCTION	BIT(3)
#endif

#ifndef BIT_EN_TXBCN_RPT
#define BIT_EN_TXBCN_RPT	BIT(2)
#endif

#ifndef REG_BCN_CTRL_CLINT0
#define REG_BCN_CTRL_CLINT0	0x0551
#endif

#ifndef REG_DRVERLYINT
#define REG_DRVERLYINT		0x0558
#endif

#ifndef REG_BCNDMATIM
#define REG_BCNDMATIM		0x0559
#endif

#ifndef REG_ATIMWND
#define REG_ATIMWND		0x055A
#endif

#ifndef REG_USTIME_TSF
#define REG_USTIME_TSF		0x055C
#endif

#ifndef REG_BCN_MAX_ERR
#define REG_BCN_MAX_ERR		0x055D
#endif

#ifndef REG_RXTSF_OFFSET_CCK
#define REG_RXTSF_OFFSET_CCK	0x055E
#endif

#ifndef REG_MISC_CTRL
#define REG_MISC_CTRL		0x0577
#endif

#ifndef BIT_EN_FREE_CNT
#define BIT_EN_FREE_CNT		BIT(3)
#endif

#ifndef BIT_DIS_SECOND_CCA
#define BIT_DIS_SECOND_CCA	(BIT(0) | BIT(1))
#endif

#ifndef REG_HIQ_NO_LMT_EN
#define REG_HIQ_NO_LMT_EN	0x5A7
#endif

#ifndef REG_DTIM_COUNTER_ROOT
#define REG_DTIM_COUNTER_ROOT	0x5A8
#endif

#ifndef BIT_HIQ_NO_LMT_EN_ROOT
#define BIT_HIQ_NO_LMT_EN_ROOT	BIT(0)
#endif

#ifndef REG_TIMER0_SRC_SEL
#define REG_TIMER0_SRC_SEL	0x05B4
#endif

#ifndef BIT_TSFT_SEL_TIMER0
#define BIT_TSFT_SEL_TIMER0	(BIT(4) | BIT(5) | BIT(6))
#endif

#ifndef REG_TCR
#define REG_TCR			0x0604
#endif

#ifndef BIT_PWRMGT_HWDATA_EN
#define BIT_PWRMGT_HWDATA_EN	BIT(7)
#endif

#ifndef BIT_TCR_UPDATE_TIMIE
#define BIT_TCR_UPDATE_TIMIE	BIT(5)
#endif

#ifndef BIT_TCR_UPDATE_HGQMD
#define BIT_TCR_UPDATE_HGQMD	BIT(4)
#endif

#ifndef REG_RCR
#define REG_RCR			0x0608
#endif

#ifndef BIT_APP_FCS
#define BIT_APP_FCS		BIT(31)
#endif

#ifndef BIT_APP_MIC
#define BIT_APP_MIC		BIT(30)
#endif

#ifndef BIT_APP_ICV
#define BIT_APP_ICV		BIT(29)
#endif

#ifndef BIT_APP_PHYSTS
#define BIT_APP_PHYSTS		BIT(28)
#endif

#ifndef BIT_APP_BASSN
#define BIT_APP_BASSN		BIT(27)
#endif

#ifndef BIT_VHT_DACK
#define BIT_VHT_DACK		BIT(26)
#endif

#ifndef BIT_TCPOFLD_EN
#define BIT_TCPOFLD_EN		BIT(25)
#endif

#ifndef BIT_ENMBID
#define BIT_ENMBID		BIT(24)
#endif

#ifndef BIT_LSIGEN
#define BIT_LSIGEN		BIT(23)
#endif

#ifndef BIT_MFBEN
#define BIT_MFBEN		BIT(22)
#endif

#ifndef BIT_DISCHKPPDLLEN
#define BIT_DISCHKPPDLLEN	BIT(21)
#endif

#ifndef BIT_PKTCTL_DLEN
#define BIT_PKTCTL_DLEN		BIT(20)
#endif

#ifndef BIT_DISGCLK
#define BIT_DISGCLK		BIT(19)
#endif

#ifndef BIT_TIM_PARSER_EN
#define BIT_TIM_PARSER_EN	BIT(18)
#endif

#ifndef BIT_BC_MD_EN
#define BIT_BC_MD_EN		BIT(17)
#endif

#ifndef BIT_UC_MD_EN
#define BIT_UC_MD_EN		BIT(16)
#endif

#ifndef BIT_RXSK_PERPKT
#define BIT_RXSK_PERPKT		BIT(15)
#endif

#ifndef BIT_HTC_LOC_CTRL
#define BIT_HTC_LOC_CTRL	BIT(14)
#endif

#ifndef BIT_RPFM_CAM_ENABLE
#define BIT_RPFM_CAM_ENABLE	BIT(12)
#endif

#ifndef BIT_TA_BCN
#define BIT_TA_BCN		BIT(11)
#endif

#ifndef BIT_RCR_ADF
#define BIT_RCR_ADF		BIT(11)
#endif

#ifndef BIT_DISDECMYPKT
#define BIT_DISDECMYPKT		BIT(10)
#endif

#ifndef BIT_AICV
#define BIT_AICV		BIT(9)
#endif

#ifndef BIT_ACRC32
#define BIT_ACRC32		BIT(8)
#endif

#ifndef BIT_CBSSID_BCN
#define BIT_CBSSID_BCN		BIT(7)
#endif

#ifndef BIT_CBSSID_DATA
#define BIT_CBSSID_DATA		BIT(6)
#endif

#ifndef BIT_APWRMGT
#define BIT_APWRMGT		BIT(5)
#endif

#ifndef BIT_ADD3
#define BIT_ADD3		BIT(4)
#endif

#ifndef BIT_AB
#define BIT_AB			BIT(3)
#endif

#ifndef BIT_AM
#define BIT_AM			BIT(2)
#endif

#ifndef BIT_APM
#define BIT_APM			BIT(1)
#endif

#ifndef BIT_AAP
#define BIT_AAP			BIT(0)
#endif

#ifndef REG_RX_PKT_LIMIT
#define REG_RX_PKT_LIMIT	0x060C
#endif

#ifndef REG_RX_DRVINFO_SZ
#define REG_RX_DRVINFO_SZ	0x060F
#endif

#ifndef REG_MAR
#define REG_MAR			0x0620
#endif

#ifndef REG_USTIME_EDCA
#define REG_USTIME_EDCA		0x0638
#endif

#ifndef REG_ACKTO_CCK
#define REG_ACKTO_CCK		0x0639
#endif

#ifndef REG_MAC_SPEC_SIFS
#define REG_MAC_SPEC_SIFS	0x063A
#endif

#ifndef REG_RESP_SIFS_CCK
#define REG_RESP_SIFS_CCK	0x063C
#endif

#ifndef REG_RESP_SIFS_OFDM
#define REG_RESP_SIFS_OFDM	0x063E
#endif

#ifndef REG_ACKTO
#define REG_ACKTO		0x0640
#endif

#ifndef REG_EIFS
#define REG_EIFS		0x0642
#endif

#ifndef REG_NAV_CTRL
#define REG_NAV_CTRL		0x0650
#endif

#ifndef REG_WMAC_TRXPTCL_CTL
#define REG_WMAC_TRXPTCL_CTL	0x0668
#endif

#ifndef BIT_RFMOD
#define BIT_RFMOD		(BIT(7) | BIT(8))
#endif

#ifndef BIT_RFMOD_80M
#define BIT_RFMOD_80M		BIT(8)
#endif

#ifndef BIT_RFMOD_40M
#define BIT_RFMOD_40M		BIT(7)
#endif

#ifndef REG_WMAC_TRXPTCL_CTL_H
#define REG_WMAC_TRXPTCL_CTL_H	0x066C
#endif

#ifndef REG_WKFMCAM_CMD
#define REG_WKFMCAM_CMD		0x0698
#endif

#ifndef BIT_WKFCAM_POLLING_V1
#define BIT_WKFCAM_POLLING_V1	BIT(31)
#endif

#ifndef BIT_WKFCAM_CLR_V1
#define BIT_WKFCAM_CLR_V1	BIT(30)
#endif

#ifndef BIT_WKFCAM_WE
#define BIT_WKFCAM_WE		BIT(16)
#endif

#ifndef BIT_SHIFT_WKFCAM_ADDR_V2
#define BIT_SHIFT_WKFCAM_ADDR_V2	8
#endif

#ifndef BIT_MASK_WKFCAM_ADDR_V2
#define BIT_MASK_WKFCAM_ADDR_V2		0xff
#endif

#ifndef BIT_WKFCAM_ADDR_V2
#define BIT_WKFCAM_ADDR_V2(x)						       \
	(((x) & BIT_MASK_WKFCAM_ADDR_V2) << BIT_SHIFT_WKFCAM_ADDR_V2)
#endif

#ifndef REG_WKFMCAM_RWD
#define REG_WKFMCAM_RWD         0x069C
#endif

#ifndef BIT_WKFMCAM_VALID
#define BIT_WKFMCAM_VALID	BIT(31)
#endif

#ifndef BIT_WKFMCAM_BC
#define BIT_WKFMCAM_BC		BIT(26)
#endif

#ifndef BIT_WKFMCAM_MC
#define BIT_WKFMCAM_MC		BIT(25)
#endif

#ifndef BIT_WKFMCAM_UC
#define BIT_WKFMCAM_UC		BIT(24)
#endif

#ifndef REG_RXFLTMAP0
#define REG_RXFLTMAP0		0x06A0
#endif

#ifndef REG_RXFLTMAP1
#define REG_RXFLTMAP1		0x06A2
#endif

#ifndef REG_RXFLTMAP2
#define REG_RXFLTMAP2		0x06A4
#endif

#ifndef REG_RXFLTMAP4
#define REG_RXFLTMAP4		0x068A
#endif

#ifndef REG_BT_COEX_TABLE0
#define REG_BT_COEX_TABLE0	0x06C0
#endif

#ifndef REG_BT_COEX_TABLE1
#define REG_BT_COEX_TABLE1	0x06C4
#endif

#ifndef REG_BT_COEX_BRK_TABLE
#define REG_BT_COEX_BRK_TABLE	0x06C8
#endif

#ifndef REG_BT_COEX_TABLE_H
#define REG_BT_COEX_TABLE_H	0x06CC
#endif

#ifndef REG_BT_COEX_TABLE_H1
#define REG_BT_COEX_TABLE_H1	0x06CD
#endif

#ifndef REG_BT_COEX_TABLE_H2
#define REG_BT_COEX_TABLE_H2	0x06CE
#endif

#ifndef REG_BT_COEX_TABLE_H3
#define REG_BT_COEX_TABLE_H3	0x06CF
#endif

#ifndef REG_BBPSF_CTRL
#define REG_BBPSF_CTRL		0x06DC
#endif

#ifndef REG_BT_COEX_V2
#define REG_BT_COEX_V2		0x0762
#endif

#ifndef BIT_GNT_BT_POLARITY
#define BIT_GNT_BT_POLARITY	BIT(12)
#endif

#ifndef BIT_LTE_COEX_EN
#define BIT_LTE_COEX_EN		BIT(7)
#endif

#ifndef REG_GNT_BT
#define REG_GNT_BT		0x0765
#endif

#ifndef BIT_PTA_SW_CTL
#define BIT_PTA_SW_CTL		GENMASK(4, 3)
#endif

#ifndef REG_BT_COEX_ENH_INTR_CTRL
#define REG_BT_COEX_ENH_INTR_CTRL	0x76E
#endif

#ifndef BIT_R_GRANTALL_WLMASK
#define BIT_R_GRANTALL_WLMASK	BIT(3)
#endif

#ifndef BIT_STATIS_BT_EN
#define BIT_STATIS_BT_EN	BIT(2)
#endif

#ifndef REG_BT_ACT_STATISTICS
#define REG_BT_ACT_STATISTICS	0x0770
#endif

#ifndef REG_BT_ACT_STATISTICS_1
#define REG_BT_ACT_STATISTICS_1	0x0774
#endif

#ifndef REG_BT_STAT_CTRL
#define REG_BT_STAT_CTRL	0x0778
#endif

#ifndef REG_BT_TDMA_TIME
#define REG_BT_TDMA_TIME	0x0790
#endif

#ifndef BIT_MASK_SAMPLE_RATE
#define BIT_MASK_SAMPLE_RATE	GENMASK(5, 0)
#endif

#ifndef REG_LTR_IDLE_LATENCY
#define REG_LTR_IDLE_LATENCY	0x0798
#endif

#ifndef REG_LTR_ACTIVE_LATENCY
#define REG_LTR_ACTIVE_LATENCY	0x079C
#endif

#ifndef REG_LTR_CTRL_BASIC
#define REG_LTR_CTRL_BASIC	0x07A4
#endif

#ifndef REG_WMAC_OPTION_FUNCTION
#define REG_WMAC_OPTION_FUNCTION 0x07D0
#endif

#ifndef REG_WMAC_OPTION_FUNCTION_1
#define REG_WMAC_OPTION_FUNCTION_1 0x07D4
#endif

#ifndef REG_FPGA0_RFMOD
#define REG_FPGA0_RFMOD		0x0800
#endif

#ifndef BIT_CCKEN
#define BIT_CCKEN		BIT(24)
#endif

#ifndef BIT_OFDMEN
#define BIT_OFDMEN		BIT(25)
#endif

#ifndef REG_CCK_RPT_FORMAT
#define REG_CCK_RPT_FORMAT	0x0804
#endif

#ifndef BIT_CCK_RPT_FORMAT
#define BIT_CCK_RPT_FORMAT	BIT(16)
#endif

#ifndef REG_RXPSEL
#define REG_RXPSEL		0x0808
#endif

#ifndef BIT_RX_PSEL_RST
#define BIT_RX_PSEL_RST		(BIT(28) | BIT(29))
#endif

#ifndef REG_TXPSEL
#define REG_TXPSEL		0x080C
#endif

#ifndef REG_RX_GAIN_EN
#define REG_RX_GAIN_EN		0x081c
#endif

#ifndef REG_CCASEL
#define REG_CCASEL		0x082C
#endif

#ifndef REG_PDMFTH
#define REG_PDMFTH		0x0830
#endif

#ifndef REG_BWINDICATION
#define REG_BWINDICATION	0x0834
#endif

#ifndef REG_CCA2ND
#define REG_CCA2ND		0x0838
#endif

#ifndef REG_L1PKTH
#define REG_L1PKTH		0x0848
#endif

#ifndef REG_CLKTRK
#define REG_CLKTRK		0x0860
#endif

#ifndef REG_CSI_MASK_SETTING1
#define REG_CSI_MASK_SETTING1	0x0874
#endif

#ifndef REG_NBI_SETTING
#define REG_NBI_SETTING		0x087c
#endif

#ifndef BIT_NBI_ENABLE
#define BIT_NBI_ENABLE		BIT(13)
#endif

#ifndef REG_CSI_FIX_MASK0
#define REG_CSI_FIX_MASK0	0x0880
#endif

#ifndef REG_CSI_FIX_MASK1
#define REG_CSI_FIX_MASK1	0x0884
#endif

#ifndef REG_CSI_FIX_MASK6
#define REG_CSI_FIX_MASK6	0x0898
#endif

#ifndef REG_CSI_FIX_MASK7
#define REG_CSI_FIX_MASK7	0x089c
#endif

#ifndef REG_ADCCLK
#define REG_ADCCLK		0x08AC
#endif

#ifndef REG_HSSI_READ
#define REG_HSSI_READ		0x08B0
#endif

#ifndef REG_FPGA0_XCD_RF_PARA
#define REG_FPGA0_XCD_RF_PARA	0x08B4
#endif

#ifndef REG_RX_MCS_LIMIT
#define REG_RX_MCS_LIMIT	0x08BC
#endif

#ifndef REG_ADC160
#define REG_ADC160		0x08C4
#endif

#ifndef REG_DBGSEL
#define REG_DBGSEL		0x08fc
#endif

#ifndef REG_ANTSEL_SW
#define REG_ANTSEL_SW		0x0900
#endif

#ifndef REG_DAC_RSTB
#define REG_DAC_RSTB		0x090c
#endif

#ifndef REG_PSD
#define REG_PSD			0x0910
#endif

#ifndef BIT_PSD_INI
#define BIT_PSD_INI		GENMASK(23, 22)
#endif

#ifndef REG_SINGLE_TONE_CONT_TX
#define REG_SINGLE_TONE_CONT_TX	0x0914
#endif

#ifndef REG_AGC_TABLE
#define REG_AGC_TABLE		0x0958
#endif

#ifndef REG_RFE_CTRL_E
#define REG_RFE_CTRL_E		0x0974
#endif

#ifndef REG_2ND_CCA_CTRL
#define REG_2ND_CCA_CTRL	0x0976
#endif

#ifndef REG_IQK_COM00
#define REG_IQK_COM00		0x0978
#endif

#ifndef REG_IQK_COM32
#define REG_IQK_COM32		0x097c
#endif

#ifndef REG_IQK_COM64
#define REG_IQK_COM64		0x0980
#endif

#ifndef REG_IQK_COM96
#define REG_IQK_COM96		0x0984
#endif

#ifndef REG_FAS
#define REG_FAS			0x09a4
#endif

#ifndef REG_RXSB
#define REG_RXSB		0x0a00
#endif

#ifndef BIT_RXSB_ANA_DIV
#define BIT_RXSB_ANA_DIV	BIT(15)
#endif

#ifndef REG_CCK_RX
#define REG_CCK_RX		0x0a04
#endif

#ifndef REG_CCK_PD_TH
#define REG_CCK_PD_TH		0x0a0a
#endif

#ifndef REG_PRECTRL
#define REG_PRECTRL		0x0a14
#endif

#ifndef BIT_DIS_CO_PATHSEL
#define BIT_DIS_CO_PATHSEL	BIT(7)
#endif

#ifndef BIT_IQ_WGT
#define BIT_IQ_WGT		GENMASK(9, 8)
#endif

#ifndef REG_CCA_MF
#define REG_CCA_MF		0x0a20
#endif

#ifndef BIT_MBC_WIN
#define BIT_MBC_WIN		GENMASK(5, 4)
#endif

#ifndef REG_CCK0_TX_FILTER1
#define REG_CCK0_TX_FILTER1	0x0a20
#endif

#ifndef REG_CCK0_TX_FILTER2
#define REG_CCK0_TX_FILTER2	0x0a24
#endif

#ifndef REG_CCK0_DEBUG_PORT
#define REG_CCK0_DEBUG_PORT	0x0a28
#endif

#ifndef REG_CCK0_FAREPORT
#define REG_CCK0_FAREPORT	0x0a2c
#endif

#ifndef BIT_CCK0_2RX
#define BIT_CCK0_2RX		BIT(18)
#endif

#ifndef BIT_CCK0_MRC
#define BIT_CCK0_MRC		BIT(22)
#endif

#ifndef REG_FA_CCK
#define REG_FA_CCK		0x0a5c
#endif

#ifndef REG_DIS_DPD
#define REG_DIS_DPD		0x0a70
#endif

#ifndef DIS_DPD_MASK
#define DIS_DPD_MASK		GENMASK(9, 0)
#endif

#ifndef DIS_DPD_RATE6M
#define DIS_DPD_RATE6M		BIT(0)
#endif

#ifndef DIS_DPD_RATE9M
#define DIS_DPD_RATE9M		BIT(1)
#endif

#ifndef DIS_DPD_RATEMCS0
#define DIS_DPD_RATEMCS0	BIT(2)
#endif

#ifndef DIS_DPD_RATEMCS1
#define DIS_DPD_RATEMCS1	BIT(3)
#endif

#ifndef DIS_DPD_RATEMCS8
#define DIS_DPD_RATEMCS8	BIT(4)
#endif

#ifndef DIS_DPD_RATEMCS9
#define DIS_DPD_RATEMCS9	BIT(5)
#endif

#ifndef DIS_DPD_RATEVHT1SS_MCS0
#define DIS_DPD_RATEVHT1SS_MCS0	BIT(6)
#endif

#ifndef DIS_DPD_RATEVHT1SS_MCS1
#define DIS_DPD_RATEVHT1SS_MCS1	BIT(7)
#endif

#ifndef DIS_DPD_RATEVHT2SS_MCS0
#define DIS_DPD_RATEVHT2SS_MCS0	BIT(8)
#endif

#ifndef DIS_DPD_RATEVHT2SS_MCS1
#define DIS_DPD_RATEVHT2SS_MCS1	BIT(9)
#endif

#ifndef DIS_DPD_RATEALL
#define DIS_DPD_RATEALL		GENMASK(9, 0)
#endif

#ifndef REG_CCA
#define REG_CCA			0x0a70
#endif

#ifndef BIT_CCA_CO
#define BIT_CCA_CO		BIT(7)
#endif

#ifndef REG_ANTSEL
#define REG_ANTSEL		0x0a74
#endif

#ifndef BIT_ANT_BYCO
#define BIT_ANT_BYCO		BIT(8)
#endif

#ifndef REG_CCKTX
#define REG_CCKTX		0x0a84
#endif

#ifndef BIT_CMB_CCA_2R
#define BIT_CMB_CCA_2R		BIT(28)
#endif

#ifndef REG_CNTRST
#define REG_CNTRST		0x0b58
#endif

#ifndef REG_3WIRE_SWA
#define REG_3WIRE_SWA		0x0c00
#endif

#ifndef REG_RX_IQC_AB_A
#define REG_RX_IQC_AB_A		0x0c10
#endif

#ifndef REG_RX_IQC_CD_A
#define REG_RX_IQC_CD_A		0x0c14
#endif

#ifndef REG_TXSCALE_A
#define REG_TXSCALE_A		0x0c1c
#endif

#ifndef BB_SWING_MASK
#define BB_SWING_MASK		GENMASK(31, 21)
#endif

#ifndef REG_TX_AGC_A_CCK_11_CCK_1
#define REG_TX_AGC_A_CCK_11_CCK_1		0xc20
#endif

#ifndef REG_TX_AGC_A_OFDM18_OFDM6
#define REG_TX_AGC_A_OFDM18_OFDM6		0xc24
#endif

#ifndef REG_TX_AGC_A_OFDM54_OFDM24
#define REG_TX_AGC_A_OFDM54_OFDM24		0xc28
#endif

#ifndef REG_TX_AGC_A_MCS3_MCS0
#define REG_TX_AGC_A_MCS3_MCS0			0xc2c
#endif

#ifndef REG_TX_AGC_A_MCS7_MCS4
#define REG_TX_AGC_A_MCS7_MCS4			0xc30
#endif

#ifndef REG_TX_AGC_A_MCS11_MCS8
#define REG_TX_AGC_A_MCS11_MCS8			0xc34
#endif

#ifndef REG_TX_AGC_A_MCS15_MCS12
#define REG_TX_AGC_A_MCS15_MCS12		0xc38
#endif

#ifndef REG_TX_AGC_A_NSS1_INDEX3_NSS1_INDEX0
#define REG_TX_AGC_A_NSS1_INDEX3_NSS1_INDEX0	0xc3c
#endif

#ifndef REG_TX_AGC_A_NSS1_INDEX7_NSS1_INDEX4
#define REG_TX_AGC_A_NSS1_INDEX7_NSS1_INDEX4	0xc40
#endif

#ifndef REG_TX_AGC_A_NSS2_INDEX1_NSS1_INDEX8
#define REG_TX_AGC_A_NSS2_INDEX1_NSS1_INDEX8	0xc44
#endif

#ifndef REG_TX_AGC_A_NSS2_INDEX5_NSS2_INDEX2
#define REG_TX_AGC_A_NSS2_INDEX5_NSS2_INDEX2	0xc48
#endif

#ifndef REG_TX_AGC_A_NSS2_INDEX9_NSS2_INDEX6
#define REG_TX_AGC_A_NSS2_INDEX9_NSS2_INDEX6	0xc4c
#endif

#ifndef REG_RXIGI_A
#define REG_RXIGI_A		0x0c50
#endif

#ifndef REG_TX_PWR_TRAINING_A
#define REG_TX_PWR_TRAINING_A	0x0c54
#endif

#ifndef REG_CK_MONHA
#define REG_CK_MONHA		0x0c5c
#endif

#ifndef REG_AFE_PWR1_A
#define REG_AFE_PWR1_A		0x0c60
#endif

#ifndef REG_AFE_PWR2_A
#define REG_AFE_PWR2_A		0x0c64
#endif

#ifndef REG_RX_WAIT_CCA_TX_CCK_RFON_A
#define REG_RX_WAIT_CCA_TX_CCK_RFON_A	0x0c68
#endif

#ifndef REG_OFDM0_XA_TX_IQ_IMBALANCE
#define REG_OFDM0_XA_TX_IQ_IMBALANCE	0x0c80
#endif

#ifndef REG_OFDM0_A_TX_AFE
#define REG_OFDM0_A_TX_AFE	0x0c84
#endif

#ifndef REG_OFDM0_XB_TX_IQ_IMBALANCE
#define REG_OFDM0_XB_TX_IQ_IMBALANCE	0x0c88
#endif

#ifndef REG_TSSI_TRK_SW
#define REG_TSSI_TRK_SW		0x0c8c
#endif

#ifndef REG_LSSI_WRITE_A
#define REG_LSSI_WRITE_A	0x0c90
#endif

#ifndef REG_PREDISTA
#define REG_PREDISTA		0x0c90
#endif

#ifndef REG_TXAGCIDX
#define REG_TXAGCIDX		0x0c94
#endif

#ifndef REG_TX_AGC_A
#define REG_TX_AGC_A		0x0c94
#endif

#ifndef REG_RFE_PINMUX_A
#define REG_RFE_PINMUX_A	0x0cb0
#endif

#ifndef REG_RFE_INV_A
#define REG_RFE_INV_A		0x0cb4
#endif

#ifndef REG_RFE_CTRL8
#define REG_RFE_CTRL8		0x0cb4
#endif

#ifndef BIT_MASK_RFE_SEL89
#define BIT_MASK_RFE_SEL89	GENMASK(7, 0)
#endif

#ifndef PTA_CTRL_PIN
#define PTA_CTRL_PIN		0x66
#endif

#ifndef DPDT_CTRL_PIN
#define DPDT_CTRL_PIN		0x77
#endif

#ifndef RFE_INV_MASK
#define RFE_INV_MASK		0x3ff00000
#endif

#ifndef REG_RFECTL_A
#define REG_RFECTL_A		0x0cb8
#endif

#ifndef REG_RFE_INV0
#define REG_RFE_INV0		0x0cbc
#endif

#ifndef REG_RFE_INV8
#define REG_RFE_INV8		0x0cbd
#endif

#ifndef BIT_MASK_RFE_INV89
#define BIT_MASK_RFE_INV89	GENMASK(1, 0)
#endif

#ifndef REG_RFE_INV16
#define REG_RFE_INV16		0x0cbe
#endif

#ifndef BIT_RFE_BUF_EN
#define BIT_RFE_BUF_EN		BIT(3)
#endif

#ifndef REG_IQK_DPD_CFG
#define REG_IQK_DPD_CFG		0x0cc4
#endif

#ifndef REG_CFG_PMPD
#define REG_CFG_PMPD		0x0cc8
#endif

#ifndef REG_IQC_Y
#define REG_IQC_Y		0x0ccc
#endif

#ifndef REG_IQC_X
#define REG_IQC_X		0x0cd4
#endif

#ifndef REG_INTPO_SETA
#define REG_INTPO_SETA		0x0ce8
#endif

#ifndef REG_IQKA_END
#define REG_IQKA_END		0x0d00
#endif

#ifndef REG_PI_READ_A
#define REG_PI_READ_A		0x0d04
#endif

#ifndef REG_SI_READ_A
#define REG_SI_READ_A		0x0d08
#endif

#ifndef REG_IQKB_END
#define REG_IQKB_END		0x0d40
#endif

#ifndef REG_PI_READ_B
#define REG_PI_READ_B		0x0d44
#endif

#ifndef REG_SI_READ_B
#define REG_SI_READ_B		0x0d48
#endif

#ifndef REG_3WIRE_SWB
#define REG_3WIRE_SWB		0x0e00
#endif

#ifndef REG_RX_IQC_AB_B
#define REG_RX_IQC_AB_B		0x0e10
#endif

#ifndef REG_RX_IQC_CD_B
#define REG_RX_IQC_CD_B		0x0e14
#endif

#ifndef REG_TXSCALE_B
#define REG_TXSCALE_B		0x0e1c
#endif

#ifndef REG_TX_AGC_B_CCK_11_CCK_1
#define REG_TX_AGC_B_CCK_11_CCK_1		0xe20
#endif

#ifndef REG_TX_AGC_B_OFDM18_OFDM6
#define REG_TX_AGC_B_OFDM18_OFDM6		0xe24
#endif

#ifndef REG_TX_AGC_B_OFDM54_OFDM24
#define REG_TX_AGC_B_OFDM54_OFDM24		0xe28
#endif

#ifndef REG_TX_AGC_B_MCS3_MCS0
#define REG_TX_AGC_B_MCS3_MCS0			0xe2c
#endif

#ifndef REG_TX_AGC_B_MCS7_MCS4
#define REG_TX_AGC_B_MCS7_MCS4			0xe30
#endif

#ifndef REG_TX_AGC_B_MCS11_MCS8
#define REG_TX_AGC_B_MCS11_MCS8			0xe34
#endif

#ifndef REG_TX_AGC_B_MCS15_MCS12
#define REG_TX_AGC_B_MCS15_MCS12		0xe38
#endif

#ifndef REG_TX_AGC_B_NSS1_INDEX3_NSS1_INDEX0
#define REG_TX_AGC_B_NSS1_INDEX3_NSS1_INDEX0	0xe3c
#endif

#ifndef REG_TX_AGC_B_NSS1_INDEX7_NSS1_INDEX4
#define REG_TX_AGC_B_NSS1_INDEX7_NSS1_INDEX4	0xe40
#endif

#ifndef REG_TX_AGC_B_NSS2_INDEX1_NSS1_INDEX8
#define REG_TX_AGC_B_NSS2_INDEX1_NSS1_INDEX8	0xe44
#endif

#ifndef REG_TX_AGC_B_NSS2_INDEX5_NSS2_INDEX2
#define REG_TX_AGC_B_NSS2_INDEX5_NSS2_INDEX2	0xe48
#endif

#ifndef REG_TX_AGC_B_NSS2_INDEX9_NSS2_INDEX6
#define REG_TX_AGC_B_NSS2_INDEX9_NSS2_INDEX6	0xe4c
#endif

#ifndef REG_RXIGI_B
#define REG_RXIGI_B		0x0e50
#endif

#ifndef REG_TX_PWR_TRAINING_B
#define REG_TX_PWR_TRAINING_B	0x0e54
#endif

#ifndef REG_CK_MONHB
#define REG_CK_MONHB		0x0e5c
#endif

#ifndef REG_AFE_PWR1_B
#define REG_AFE_PWR1_B		0x0e60
#endif

#ifndef REG_AFE_PWR2_B
#define REG_AFE_PWR2_B		0x0e64
#endif

#ifndef REG_RX_WAIT_CCA_TX_CCK_RFON_B
#define REG_RX_WAIT_CCA_TX_CCK_RFON_B	0x0e68
#endif

#ifndef REG_TXTONEB
#define REG_TXTONEB		0x0e80
#endif

#ifndef REG_RXTONEB
#define REG_RXTONEB		0x0e84
#endif

#ifndef REG_TXPITMB
#define REG_TXPITMB		0x0e88
#endif

#ifndef REG_RXPITMB
#define REG_RXPITMB		0x0e8c
#endif

#ifndef REG_LSSI_WRITE_B
#define REG_LSSI_WRITE_B	0x0e90
#endif

#ifndef REG_PREDISTB
#define REG_PREDISTB		0x0e90
#endif

#ifndef REG_INIDLYB
#define REG_INIDLYB		0x0e94
#endif

#ifndef REG_TX_AGC_B
#define REG_TX_AGC_B		0x0e94
#endif

#ifndef REG_RFE_PINMUX_B
#define REG_RFE_PINMUX_B	0x0eb0
#endif

#ifndef REG_RFE_INV_B
#define REG_RFE_INV_B		0x0eb4
#endif

#ifndef REG_RFECTL_B
#define REG_RFECTL_B		0x0eb8
#endif

#ifndef REG_BPBDB
#define REG_BPBDB		0x0ec4
#endif

#ifndef REG_PHYTXONB
#define REG_PHYTXONB		0x0ec8
#endif

#ifndef REG_IQKYB
#define REG_IQKYB		0x0ecc
#endif

#ifndef REG_IQKXB
#define REG_IQKXB		0x0ed4
#endif

#ifndef REG_INTPO_SETB
#define REG_INTPO_SETB		0x0ee8
#endif

#ifndef REG_CRC_CCK
#define REG_CRC_CCK		0x0f04
#endif

#ifndef REG_CCA_OFDM
#define REG_CCA_OFDM		0x0f08
#endif

#ifndef REG_CRC_VHT
#define REG_CRC_VHT		0x0f0c
#endif

#ifndef REG_CRC_HT
#define REG_CRC_HT		0x0f10
#endif

#ifndef REG_CRC_OFDM
#define REG_CRC_OFDM		0x0f14
#endif

#ifndef REG_FA_OFDM
#define REG_FA_OFDM		0x0f48
#endif

#ifndef REG_DBGRPT
#define REG_DBGRPT		0x0fa0
#endif

#ifndef REG_CCA_CCK
#define REG_CCA_CCK		0x0fcc
#endif

#ifndef REG_SYS_CFG3_8814A
#define REG_SYS_CFG3_8814A	0x1000
#endif

#ifndef REG_ANAPARSW_MAC_0
#define REG_ANAPARSW_MAC_0	0x1010
#endif

#ifndef BIT_CF_L_V2
#define BIT_CF_L_V2		GENMASK(29, 28)
#endif

#ifndef REG_ANAPAR_XTAL_0
#define REG_ANAPAR_XTAL_0	0x1040
#endif

#ifndef BIT_XCAP_0
#define BIT_XCAP_0		GENMASK(23, 10)
#endif

#ifndef REG_CPU_DMEM_CON
#define REG_CPU_DMEM_CON	0x1080
#endif

#ifndef BIT_WL_PLATFORM_RST
#define BIT_WL_PLATFORM_RST	BIT(16)
#endif

#ifndef BIT_WL_SECURITY_CLK
#define BIT_WL_SECURITY_CLK	BIT(15)
#endif

#ifndef BIT_DDMA_EN
#define BIT_DDMA_EN		BIT(8)
#endif

#ifndef REG_SW_MDIO
#define REG_SW_MDIO		0x10C0
#endif

#ifndef REG_H2C_PKT_READADDR
#define REG_H2C_PKT_READADDR	0x10D0
#endif

#ifndef REG_H2C_PKT_WRITEADDR
#define REG_H2C_PKT_WRITEADDR	0x10D4
#endif

#ifndef REG_FW_DBG6
#define REG_FW_DBG6		0x10F8
#endif

#ifndef REG_FW_DBG7
#define REG_FW_DBG7		0x10FC
#endif

#ifndef FW_KEY_MASK
#define FW_KEY_MASK		0xffffff00
#endif

#ifndef REG_CR_EXT
#define REG_CR_EXT		0x1100
#endif

#ifndef REG_FT1IMR
#define REG_FT1IMR		0x1138
#endif

#ifndef BIT_FS_H2C_CMD_OK_INT_EN
#define BIT_FS_H2C_CMD_OK_INT_EN BIT(25)
#endif

#ifndef REG_FT1ISR
#define REG_FT1ISR		0x113c
#endif

#ifndef BIT_FS_H2C_CMD_OK_INT
#define BIT_FS_H2C_CMD_OK_INT	BIT(25)
#endif

#ifndef REG_DDMA_CH0SA
#define REG_DDMA_CH0SA		0x1200
#endif

#ifndef REG_DDMA_CH0DA
#define REG_DDMA_CH0DA		0x1204
#endif

#ifndef REG_DDMA_CH0CTRL
#define REG_DDMA_CH0CTRL	0x1208
#endif

#ifndef BIT_DDMACH0_OWN
#define BIT_DDMACH0_OWN		BIT(31)
#endif

#ifndef BIT_DDMACH0_CHKSUM_EN
#define BIT_DDMACH0_CHKSUM_EN	BIT(29)
#endif

#ifndef BIT_DDMACH0_CHKSUM_STS
#define BIT_DDMACH0_CHKSUM_STS	BIT(27)
#endif

#ifndef BIT_DDMACH0_DDMA_MODE
#define BIT_DDMACH0_DDMA_MODE	BIT(26)
#endif

#ifndef BIT_DDMACH0_RESET_CHKSUM_STS
#define BIT_DDMACH0_RESET_CHKSUM_STS BIT(25)
#endif

#ifndef BIT_DDMACH0_CHKSUM_CONT
#define BIT_DDMACH0_CHKSUM_CONT	BIT(24)
#endif

#ifndef BIT_MASK_DDMACH0_DLEN
#define BIT_MASK_DDMACH0_DLEN	0x3ffff
#endif

#ifndef REG_H2CQ_CSR
#define REG_H2CQ_CSR		0x1330
#endif

#ifndef BIT_H2CQ_FULL
#define BIT_H2CQ_FULL		BIT(31)
#endif

#ifndef REG_FAST_EDCA_VOVI_SETTING
#define REG_FAST_EDCA_VOVI_SETTING 0x1448
#endif

#ifndef REG_FAST_EDCA_BEBK_SETTING
#define REG_FAST_EDCA_BEBK_SETTING 0x144C
#endif

#ifndef REG_RXPSF_CTRL
#define REG_RXPSF_CTRL		0x1610
#endif

#ifndef BIT_RXGCK_FIFOTHR_EN
#define BIT_RXGCK_FIFOTHR_EN	BIT(28)
#endif

#ifndef BIT_SHIFT_RXGCK_VHT_FIFOTHR
#define BIT_SHIFT_RXGCK_VHT_FIFOTHR 26
#endif

#ifndef BIT_MASK_RXGCK_VHT_FIFOTHR
#define BIT_MASK_RXGCK_VHT_FIFOTHR 0x3
#endif

#ifndef BIT_RXGCK_VHT_FIFOTHR
#define BIT_RXGCK_VHT_FIFOTHR(x)                                               \
	(((x) & BIT_MASK_RXGCK_VHT_FIFOTHR) << BIT_SHIFT_RXGCK_VHT_FIFOTHR)
#endif

#ifndef BITS_RXGCK_VHT_FIFOTHR
#define BITS_RXGCK_VHT_FIFOTHR                                                 \
	(BIT_MASK_RXGCK_VHT_FIFOTHR << BIT_SHIFT_RXGCK_VHT_FIFOTHR)
#endif

#ifndef BIT_SHIFT_RXGCK_HT_FIFOTHR
#define BIT_SHIFT_RXGCK_HT_FIFOTHR 24
#endif

#ifndef BIT_MASK_RXGCK_HT_FIFOTHR
#define BIT_MASK_RXGCK_HT_FIFOTHR 0x3
#endif

#ifndef BIT_RXGCK_HT_FIFOTHR
#define BIT_RXGCK_HT_FIFOTHR(x)                                                \
	(((x) & BIT_MASK_RXGCK_HT_FIFOTHR) << BIT_SHIFT_RXGCK_HT_FIFOTHR)
#endif

#ifndef BITS_RXGCK_HT_FIFOTHR
#define BITS_RXGCK_HT_FIFOTHR                                                  \
	(BIT_MASK_RXGCK_HT_FIFOTHR << BIT_SHIFT_RXGCK_HT_FIFOTHR)
#endif

#ifndef BIT_SHIFT_RXGCK_OFDM_FIFOTHR
#define BIT_SHIFT_RXGCK_OFDM_FIFOTHR 22
#endif

#ifndef BIT_MASK_RXGCK_OFDM_FIFOTHR
#define BIT_MASK_RXGCK_OFDM_FIFOTHR 0x3
#endif

#ifndef BIT_RXGCK_OFDM_FIFOTHR
#define BIT_RXGCK_OFDM_FIFOTHR(x)                                              \
	(((x) & BIT_MASK_RXGCK_OFDM_FIFOTHR) << BIT_SHIFT_RXGCK_OFDM_FIFOTHR)
#endif

#ifndef BITS_RXGCK_OFDM_FIFOTHR
#define BITS_RXGCK_OFDM_FIFOTHR                                                \
	(BIT_MASK_RXGCK_OFDM_FIFOTHR << BIT_SHIFT_RXGCK_OFDM_FIFOTHR)
#endif

#ifndef BIT_SHIFT_RXGCK_CCK_FIFOTHR
#define BIT_SHIFT_RXGCK_CCK_FIFOTHR 20
#endif

#ifndef BIT_MASK_RXGCK_CCK_FIFOTHR
#define BIT_MASK_RXGCK_CCK_FIFOTHR 0x3
#endif

#ifndef BIT_RXGCK_CCK_FIFOTHR
#define BIT_RXGCK_CCK_FIFOTHR(x)                                               \
	(((x) & BIT_MASK_RXGCK_CCK_FIFOTHR) << BIT_SHIFT_RXGCK_CCK_FIFOTHR)
#endif

#ifndef BITS_RXGCK_CCK_FIFOTHR
#define BITS_RXGCK_CCK_FIFOTHR                                                 \
	(BIT_MASK_RXGCK_CCK_FIFOTHR << BIT_SHIFT_RXGCK_CCK_FIFOTHR)
#endif

#ifndef BIT_RXGCK_OFDMCCA_EN
#define BIT_RXGCK_OFDMCCA_EN BIT(16)
#endif

#ifndef BIT_SHIFT_RXPSF_PKTLENTHR
#define BIT_SHIFT_RXPSF_PKTLENTHR 13
#endif

#ifndef BIT_MASK_RXPSF_PKTLENTHR
#define BIT_MASK_RXPSF_PKTLENTHR 0x7
#endif

#ifndef BIT_RXPSF_PKTLENTHR
#define BIT_RXPSF_PKTLENTHR(x)                                                 \
	(((x) & BIT_MASK_RXPSF_PKTLENTHR) << BIT_SHIFT_RXPSF_PKTLENTHR)
#endif

#ifndef BITS_RXPSF_PKTLENTHR
#define BITS_RXPSF_PKTLENTHR                                                   \
	(BIT_MASK_RXPSF_PKTLENTHR << BIT_SHIFT_RXPSF_PKTLENTHR)
#endif

#ifndef BIT_CLEAR_RXPSF_PKTLENTHR
#define BIT_CLEAR_RXPSF_PKTLENTHR(x) ((x) & (~BITS_RXPSF_PKTLENTHR))
#endif

#ifndef BIT_SET_RXPSF_PKTLENTHR
#define BIT_SET_RXPSF_PKTLENTHR(x, v)                                          \
	(BIT_CLEAR_RXPSF_PKTLENTHR(x) | BIT_RXPSF_PKTLENTHR(v))
#endif

#ifndef BIT_RXPSF_CTRLEN
#define BIT_RXPSF_CTRLEN	BIT(12)
#endif

#ifndef BIT_RXPSF_VHTCHKEN
#define BIT_RXPSF_VHTCHKEN	BIT(11)
#endif

#ifndef BIT_RXPSF_HTCHKEN
#define BIT_RXPSF_HTCHKEN	BIT(10)
#endif

#ifndef BIT_RXPSF_OFDMCHKEN
#define BIT_RXPSF_OFDMCHKEN	BIT(9)
#endif

#ifndef BIT_RXPSF_CCKCHKEN
#define BIT_RXPSF_CCKCHKEN	BIT(8)
#endif

#ifndef BIT_RXPSF_OFDMRST
#define BIT_RXPSF_OFDMRST	BIT(7)
#endif

#ifndef BIT_RXPSF_CCKRST
#define BIT_RXPSF_CCKRST	BIT(6)
#endif

#ifndef BIT_RXPSF_MHCHKEN
#define BIT_RXPSF_MHCHKEN	BIT(5)
#endif

#ifndef BIT_RXPSF_CONT_ERRCHKEN
#define BIT_RXPSF_CONT_ERRCHKEN	BIT(4)
#endif

#ifndef BIT_RXPSF_ALL_ERRCHKEN
#define BIT_RXPSF_ALL_ERRCHKEN	BIT(3)
#endif

#ifndef BIT_SHIFT_RXPSF_ERRTHR
#define BIT_SHIFT_RXPSF_ERRTHR 0
#endif

#ifndef BIT_MASK_RXPSF_ERRTHR
#define BIT_MASK_RXPSF_ERRTHR 0x7
#endif

#ifndef BIT_RXPSF_ERRTHR
#define BIT_RXPSF_ERRTHR(x)                                                    \
	(((x) & BIT_MASK_RXPSF_ERRTHR) << BIT_SHIFT_RXPSF_ERRTHR)
#endif

#ifndef BITS_RXPSF_ERRTHR
#define BITS_RXPSF_ERRTHR (BIT_MASK_RXPSF_ERRTHR << BIT_SHIFT_RXPSF_ERRTHR)
#endif

#ifndef BIT_CLEAR_RXPSF_ERRTHR
#define BIT_CLEAR_RXPSF_ERRTHR(x) ((x) & (~BITS_RXPSF_ERRTHR))
#endif

#ifndef BIT_GET_RXPSF_ERRTHR
#define BIT_GET_RXPSF_ERRTHR(x)                                                \
	(((x) >> BIT_SHIFT_RXPSF_ERRTHR) & BIT_MASK_RXPSF_ERRTHR)
#endif

#ifndef BIT_SET_RXPSF_ERRTHR
#define BIT_SET_RXPSF_ERRTHR(x, v)                                             \
	(BIT_CLEAR_RXPSF_ERRTHR(x) | BIT_RXPSF_ERRTHR(v))
#endif

#ifndef REG_RXPSF_TYPE_CTRL
#define REG_RXPSF_TYPE_CTRL	0x1614
#endif

#ifndef REG_GENERAL_OPTION
#define REG_GENERAL_OPTION	0x1664
#endif

#ifndef BIT_DUMMY_FCS_READY_MASK_EN
#define BIT_DUMMY_FCS_READY_MASK_EN BIT(9)
#endif

#ifndef REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1
#define REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1		0x1700
#endif

#ifndef REG_WL2LTECOEX_INDIRECT_ACCESS_WRITE_DATA_V1
#define REG_WL2LTECOEX_INDIRECT_ACCESS_WRITE_DATA_V1	0x1704
#endif

#ifndef REG_WL2LTECOEX_INDIRECT_ACCESS_READ_DATA_V1
#define REG_WL2LTECOEX_INDIRECT_ACCESS_READ_DATA_V1	0x1708
#endif

#ifndef LTECOEX_READY
#define LTECOEX_READY		BIT(29)
#endif

#ifndef LTECOEX_ACCESS_CTRL
#define LTECOEX_ACCESS_CTRL REG_WL2LTECOEX_INDIRECT_ACCESS_CTRL_V1
#endif

#ifndef LTECOEX_WRITE_DATA
#define LTECOEX_WRITE_DATA REG_WL2LTECOEX_INDIRECT_ACCESS_WRITE_DATA_V1
#endif

#ifndef LTECOEX_READ_DATA
#define LTECOEX_READ_DATA REG_WL2LTECOEX_INDIRECT_ACCESS_READ_DATA_V1
#endif

#ifndef REG_RX_IQC_AB_C
#define REG_RX_IQC_AB_C		0x1810
#endif

#ifndef REG_RX_IQC_CD_C
#define REG_RX_IQC_CD_C		0x1814
#endif

#ifndef REG_TXSCALE_C
#define REG_TXSCALE_C		0x181c
#endif

#ifndef REG_CK_MONHC
#define REG_CK_MONHC		0x185c
#endif

#ifndef REG_AFE_PWR1_C
#define REG_AFE_PWR1_C		0x1860
#endif

#ifndef REG_IGN_GNT_BT1
#define REG_IGN_GNT_BT1	0x1860
#endif

#ifndef REG_TX_AGC_C
#define REG_TX_AGC_C		0x1894
#endif

#ifndef REG_RFE_PINMUX_C
#define REG_RFE_PINMUX_C	0x18b4
#endif

#ifndef REG_RFESEL_CTRL
#define REG_RFESEL_CTRL	0x1990
#endif

#ifndef REG_AGC_TBL
#define REG_AGC_TBL		0x1998
#endif

#ifndef REG_RX_IQC_AB_D
#define REG_RX_IQC_AB_D		0x1a10
#endif

#ifndef REG_RX_IQC_CD_D
#define REG_RX_IQC_CD_D		0x1a14
#endif

#ifndef REG_TXSCALE_D
#define REG_TXSCALE_D		0x1a1c
#endif

#ifndef REG_CK_MONHD
#define REG_CK_MONHD		0x1a5c
#endif

#ifndef REG_AFE_PWR1_D
#define REG_AFE_PWR1_D		0x1a60
#endif

#ifndef REG_TX_AGC_D
#define REG_TX_AGC_D		0x1a94
#endif

#ifndef REG_RFE_PINMUX_D
#define REG_RFE_PINMUX_D	0x1ab4
#endif

#ifndef REG_RFE_INVSEL_D
#define REG_RFE_INVSEL_D	0x1abc
#endif

#ifndef BIT_RFE_SELSW0_D
#define BIT_RFE_SELSW0_D	GENMASK(27, 20)
#endif

#ifndef REG_NOMASK_TXBT
#define REG_NOMASK_TXBT	0x1ca7
#endif

#ifndef REG_ANAPAR
#define REG_ANAPAR	0x1c30
#endif

#ifndef BIT_ANAPAR_BTPS
#define BIT_ANAPAR_BTPS	BIT(22)
#endif

#ifndef REG_RSTB_SEL
#define REG_RSTB_SEL	0x1c38
#endif

#ifndef BIT_DAC_OFF_ENABLE
#define BIT_DAC_OFF_ENABLE	BIT(4)
#endif

#ifndef BIT_PI_IGNORE_GNT_BT
#define BIT_PI_IGNORE_GNT_BT	BIT(3)
#endif

#ifndef BIT_NOMASK_TXBT_ENABLE
#define BIT_NOMASK_TXBT_ENABLE	BIT(3)
#endif

#ifndef REG_HRCV_MSG
#define REG_HRCV_MSG	0x1cf
#endif

#ifndef REG_EDCCA_REPORT
#define REG_EDCCA_REPORT	0x2d38
#endif

#ifndef BIT_EDCCA_FLAG
#define BIT_EDCCA_FLAG		BIT(24)
#endif

#ifndef REG_IGN_GNTBT4
#define REG_IGN_GNTBT4	0x4160
#endif

#ifndef REG_USB_MOD
#define REG_USB_MOD	0xf008
#endif

#ifndef REG_USB3_RXITV
#define REG_USB3_RXITV	0xf050
#endif

#ifndef REG_USB2_PHY_ADR
#define REG_USB2_PHY_ADR	0xfe40
#endif

#ifndef REG_USB2_PHY_DAT
#define REG_USB2_PHY_DAT	0xfe41
#endif

#ifndef REG_USB2_PHY_CMD
#define REG_USB2_PHY_CMD	0xfe42
#endif

#ifndef BIT_USB2_PHY_CMD_TRG
#define BIT_USB2_PHY_CMD_TRG	0x81
#endif

#ifndef REG_USB_HRPWM
#define REG_USB_HRPWM	0xfe58
#endif

#ifndef REG_USB3_PHY_ADR
#define REG_USB3_PHY_ADR	0xff0c
#endif

#ifndef REG_USB3_PHY_DAT_L
#define REG_USB3_PHY_DAT_L	0xff0d
#endif

#ifndef REG_USB3_PHY_DAT_H
#define REG_USB3_PHY_DAT_H	0xff0e
#endif

#ifndef BIT_USB3_PHY_ADR_WR
#define BIT_USB3_PHY_ADR_WR	BIT(7)
#endif

#ifndef BIT_USB3_PHY_ADR_RD
#define BIT_USB3_PHY_ADR_RD	BIT(6)
#endif

#ifndef BIT_USB3_PHY_ADR_MASK
#define BIT_USB3_PHY_ADR_MASK	GENMASK(5, 0)
#endif

#ifndef RF_MODE
#define RF_MODE		0x00
#endif

#ifndef RF_MODOPT
#define RF_MODOPT	0x01
#endif

#ifndef RF_WLINT
#define RF_WLINT	0x01
#endif

#ifndef RF_WLSEL
#define RF_WLSEL	0x02
#endif

#ifndef RF_DTXLOK
#define RF_DTXLOK	0x08
#endif

#ifndef RF_CFGCH
#define RF_CFGCH	0x18
#endif

#ifndef BIT_BAND
#define BIT_BAND	GENMASK(18, 16)
#endif

#ifndef RF18_BAND_MASK
#define RF18_BAND_MASK	(BIT(16) | BIT(9) | BIT(8))
#endif

#ifndef RF18_CHANNEL_MASK
#define RF18_CHANNEL_MASK	(MASKBYTE0)
#endif

#ifndef RF18_RFSI_MASK
#define RF18_RFSI_MASK	(BIT(18) | BIT(17))
#endif

#ifndef RF_RCK1_V1
#define RF_RCK1_V1	0x1c
#endif

#ifndef RF_RCK
#define RF_RCK		0x1d
#endif

#ifndef RF_MODE_TABLE_ADDR
#define RF_MODE_TABLE_ADDR	0x30
#endif

#ifndef RF_MODE_TABLE_DATA0
#define RF_MODE_TABLE_DATA0	0x31
#endif

#ifndef RF_MODE_TABLE_DATA1
#define RF_MODE_TABLE_DATA1	0x32
#endif

#ifndef RF_LUTWA
#define RF_LUTWA	0x33
#endif

#ifndef RF_LUTWD1
#define RF_LUTWD1	0x3e
#endif

#ifndef RF_LUTWD0
#define RF_LUTWD0	0x3f
#endif

#ifndef BIT_GAIN_EXT
#define BIT_GAIN_EXT	BIT(12)
#endif

#ifndef BIT_DATA_L
#define BIT_DATA_L	GENMASK(11, 0)
#endif

#ifndef RF_T_METER
#define RF_T_METER	0x42
#endif

#ifndef RF_BSPAD
#define RF_BSPAD	0x54
#endif

#ifndef RF_GAINTX
#define RF_GAINTX	0x56
#endif

#ifndef RF_TXMOD
#define RF_TXMOD	0x58
#endif

#ifndef RF_TXATANK
#define RF_TXATANK	0x64
#endif

#ifndef RF_TXA_PREPAD
#define RF_TXA_PREPAD	0x65
#endif

#ifndef RF_TRXIQ
#define RF_TRXIQ	0x66
#endif

#ifndef RF_RXIQGEN
#define RF_RXIQGEN	0x8d
#endif

#ifndef RF_RXBB2
#define RF_RXBB2	0x8f
#endif

#ifndef RF_SYN_PFD
#define RF_SYN_PFD	0xb0
#endif

#ifndef RF_LCK
#define RF_LCK		0xb4
#endif

#ifndef RF_XTALX2
#define RF_XTALX2	0xb8
#endif

#ifndef RF_SYN_CTRL
#define RF_SYN_CTRL	0xbb
#endif

#ifndef RF_MALSEL
#define RF_MALSEL	0xbe
#endif

#ifndef RF_SYN_AAC
#define RF_SYN_AAC	0xc9
#endif

#ifndef RF_AAC_CTRL
#define RF_AAC_CTRL	0xca
#endif

#ifndef RF_FAST_LCK
#define RF_FAST_LCK	0xcc
#endif

#ifndef RF_RCKD
#define RF_RCKD		0xde
#endif

#ifndef RF_TXADBG
#define RF_TXADBG	0xde
#endif

#ifndef RF_LUTDBG
#define RF_LUTDBG	0xdf
#endif

#ifndef BIT_TXA_TANK
#define BIT_TXA_TANK	BIT(4)
#endif

#ifndef RF_LUTWE2
#define RF_LUTWE2	0xee
#endif

#ifndef RF_LUTWE
#define RF_LUTWE	0xef
#endif

#ifndef LTE_COEX_CTRL
#define LTE_COEX_CTRL	0x38
#endif

#ifndef LTE_WL_TRX_CTRL
#define LTE_WL_TRX_CTRL	0xa0
#endif

#ifndef LTE_BT_TRX_CTRL
#define LTE_BT_TRX_CTRL	0xa4
#endif

#ifndef __RTW8821C_H__
#define __RTW8821C_H__
#endif

#ifndef RCR_VHT_ACK
#define RCR_VHT_ACK		BIT(26)
#endif

#ifndef rtw_write32s_mask
#define rtw_write32s_mask(rtwdev, addr, mask, data)			       \
	do {								       \
		BUILD_BUG_ON((addr) < 0xC00 || (addr) >= 0xD00);	       \
									       \
		_rtw_write32s_mask(rtwdev, addr, mask, data);		       \
	} while (0)
#endif

#ifndef WLAN_SLOT_TIME
#define WLAN_SLOT_TIME		0x09
#endif

#ifndef WLAN_PIFS_TIME
#define WLAN_PIFS_TIME		0x19
#endif

#ifndef WLAN_SIFS_CCK_CONT_TX
#define WLAN_SIFS_CCK_CONT_TX	0xA
#endif

#ifndef WLAN_SIFS_OFDM_CONT_TX
#define WLAN_SIFS_OFDM_CONT_TX	0xE
#endif

#ifndef WLAN_SIFS_CCK_TRX
#define WLAN_SIFS_CCK_TRX	0x10
#endif

#ifndef WLAN_SIFS_OFDM_TRX
#define WLAN_SIFS_OFDM_TRX	0x10
#endif

#ifndef WLAN_VO_TXOP_LIMIT
#define WLAN_VO_TXOP_LIMIT	0x186
#endif

#ifndef WLAN_VI_TXOP_LIMIT
#define WLAN_VI_TXOP_LIMIT	0x3BC
#endif

#ifndef WLAN_RDG_NAV
#define WLAN_RDG_NAV		0x05
#endif

#ifndef WLAN_TXOP_NAV
#define WLAN_TXOP_NAV		0x1B
#endif

#ifndef WLAN_CCK_RX_TSF
#define WLAN_CCK_RX_TSF		0x30
#endif

#ifndef WLAN_OFDM_RX_TSF
#define WLAN_OFDM_RX_TSF	0x30
#endif

#ifndef WLAN_TBTT_PROHIBIT
#define WLAN_TBTT_PROHIBIT	0x04
#endif

#ifndef WLAN_TBTT_HOLD_TIME
#define WLAN_TBTT_HOLD_TIME	0x064
#endif

#ifndef WLAN_DRV_EARLY_INT
#define WLAN_DRV_EARLY_INT	0x04
#endif

#ifndef WLAN_BCN_DMA_TIME
#define WLAN_BCN_DMA_TIME	0x02
#endif

#ifndef WLAN_RX_FILTER0
#define WLAN_RX_FILTER0		0xFFFF
#endif

#ifndef WLAN_RX_FILTER1
#define WLAN_RX_FILTER1		0x0FFF
#endif

#ifndef WLAN_RX_FILTER2
#define WLAN_RX_FILTER2		0xFFFF
#endif

#ifndef WLAN_RCR_CFG
#define WLAN_RCR_CFG		0xE400220E
#endif

#ifndef WLAN_RXPKT_MAX_SZ
#define WLAN_RXPKT_MAX_SZ	12288
#endif

#ifndef WLAN_RXPKT_MAX_SZ_512
#define WLAN_RXPKT_MAX_SZ_512	(WLAN_RXPKT_MAX_SZ >> 9)
#endif

#ifndef WLAN_AMPDU_MAX_TIME
#define WLAN_AMPDU_MAX_TIME		0x70
#endif

#ifndef WLAN_RTS_LEN_TH
#define WLAN_RTS_LEN_TH			0xFF
#endif

#ifndef WLAN_RTS_TX_TIME_TH
#define WLAN_RTS_TX_TIME_TH		0x08
#endif

#ifndef WLAN_MAX_AGG_PKT_LIMIT
#define WLAN_MAX_AGG_PKT_LIMIT		0x20
#endif

#ifndef WLAN_RTS_MAX_AGG_PKT_LIMIT
#define WLAN_RTS_MAX_AGG_PKT_LIMIT	0x20
#endif

#ifndef FAST_EDCA_VO_TH
#define FAST_EDCA_VO_TH		0x06
#endif

#ifndef FAST_EDCA_VI_TH
#define FAST_EDCA_VI_TH		0x06
#endif

#ifndef FAST_EDCA_BE_TH
#define FAST_EDCA_BE_TH		0x06
#endif

#ifndef FAST_EDCA_BK_TH
#define FAST_EDCA_BK_TH		0x06
#endif

#ifndef WLAN_BAR_RETRY_LIMIT
#define WLAN_BAR_RETRY_LIMIT		0x01
#endif

#ifndef WLAN_RA_TRY_RATE_AGG_LIMIT
#define WLAN_RA_TRY_RATE_AGG_LIMIT	0x08
#endif

#ifndef WLAN_TX_FUNC_CFG1
#define WLAN_TX_FUNC_CFG1		0x30
#endif

#ifndef WLAN_TX_FUNC_CFG2
#define WLAN_TX_FUNC_CFG2		0x30
#endif

#ifndef WLAN_MAC_OPT_NORM_FUNC1
#define WLAN_MAC_OPT_NORM_FUNC1		0x98
#endif

#ifndef WLAN_MAC_OPT_LB_FUNC1
#define WLAN_MAC_OPT_LB_FUNC1		0x80
#endif

#ifndef WLAN_MAC_OPT_FUNC2
#define WLAN_MAC_OPT_FUNC2		0xb0810041
#endif

#ifndef WLAN_SIFS_CFG
#define WLAN_SIFS_CFG	(WLAN_SIFS_CCK_CONT_TX | \
			(WLAN_SIFS_OFDM_CONT_TX << BIT_SHIFT_SIFS_OFDM_CTX) | \
			(WLAN_SIFS_CCK_TRX << BIT_SHIFT_SIFS_CCK_TRX) | \
			(WLAN_SIFS_OFDM_TRX << BIT_SHIFT_SIFS_OFDM_TRX))
#endif

#ifndef WLAN_TBTT_TIME
#define WLAN_TBTT_TIME	(WLAN_TBTT_PROHIBIT |\
			(WLAN_TBTT_HOLD_TIME << BIT_SHIFT_TBTT_HOLD_TIME_AP))
#endif

#ifndef WLAN_NAV_CFG
#define WLAN_NAV_CFG		(WLAN_RDG_NAV | (WLAN_TXOP_NAV << 16))
#endif

#ifndef WLAN_RX_TSF_CFG
#define WLAN_RX_TSF_CFG		(WLAN_CCK_RX_TSF | (WLAN_OFDM_RX_TSF) << 8)
#endif

#ifndef WLAN_PRE_TXCNT_TIME_TH
#define WLAN_PRE_TXCNT_TIME_TH		0x1E4
#endif

#ifndef GET_PHY_STAT_P0_PWDB
#define GET_PHY_STAT_P0_PWDB(phy_stat)                                         \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#endif

#ifndef GET_PHY_STAT_P0_VGA
#define GET_PHY_STAT_P0_VGA(phy_stat)                                          \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), GENMASK(12, 8))
#endif

#ifndef GET_PHY_STAT_P0_LNA_L
#define GET_PHY_STAT_P0_LNA_L(phy_stat)                                        \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), GENMASK(15, 13))
#endif

#ifndef GET_PHY_STAT_P0_LNA_H
#define GET_PHY_STAT_P0_LNA_H(phy_stat)                                        \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), BIT(23))
#endif

#ifndef BIT_LNA_H_MASK
#define BIT_LNA_H_MASK BIT(3)
#endif

#ifndef BIT_LNA_L_MASK
#define BIT_LNA_L_MASK GENMASK(2, 0)
#endif

#ifndef GET_PHY_STAT_P1_PWDB_A
#define GET_PHY_STAT_P1_PWDB_A(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#endif

#ifndef GET_PHY_STAT_P1_PWDB_B
#define GET_PHY_STAT_P1_PWDB_B(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(23, 16))
#endif

#ifndef GET_PHY_STAT_P1_RF_MODE
#define GET_PHY_STAT_P1_RF_MODE(phy_stat)                                      \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x03), GENMASK(29, 28))
#endif

#ifndef GET_PHY_STAT_P1_L_RXSC
#define GET_PHY_STAT_P1_L_RXSC(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(11, 8))
#endif

#ifndef GET_PHY_STAT_P1_HT_RXSC
#define GET_PHY_STAT_P1_HT_RXSC(phy_stat)                                      \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(15, 12))
#endif

#ifndef GET_PHY_STAT_P1_RXEVM_A
#define GET_PHY_STAT_P1_RXEVM_A(phy_stat)                                      \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(7, 0))
#endif

#ifndef GET_PHY_STAT_P1_RXEVM_B
#define GET_PHY_STAT_P1_RXEVM_B(phy_stat)                                      \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(15, 8))
#endif

#ifndef GET_PHY_STAT_P1_CFO_TAIL_A
#define GET_PHY_STAT_P1_CFO_TAIL_A(phy_stat)                                 \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x05), GENMASK(7, 0))
#endif

#ifndef GET_PHY_STAT_P1_CFO_TAIL_B
#define GET_PHY_STAT_P1_CFO_TAIL_B(phy_stat)                                 \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x05), GENMASK(15, 8))
#endif

#ifndef GET_PHY_STAT_P1_RXSNR_A
#define GET_PHY_STAT_P1_RXSNR_A(phy_stat)                                      \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x06), GENMASK(7, 0))
#endif

#ifndef GET_PHY_STAT_P1_RXSNR_B
#define GET_PHY_STAT_P1_RXSNR_B(phy_stat)                                      \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x06), GENMASK(15, 8))
#endif

#ifndef REG_SYS_CTRL
#define REG_SYS_CTRL	0x000
#endif

#ifndef BIT_FEN_EN
#define BIT_FEN_EN	BIT(26)
#endif

#ifndef REG_INIRTS_RATE_SEL
#define REG_INIRTS_RATE_SEL 0x0480
#endif

#ifndef REG_HTSTFWT
#define REG_HTSTFWT	0x800
#endif

#ifndef REG_RXCCAMSK
#define REG_RXCCAMSK	0x814
#endif

#ifndef REG_L1WT
#define REG_L1WT	0x83c
#endif

#ifndef REG_L1PKWT
#define REG_L1PKWT	0x840
#endif

#ifndef REG_MRC
#define REG_MRC		0x850
#endif

#ifndef REG_ADC40
#define REG_ADC40	0x8c8
#endif

#ifndef REG_CHFIR
#define REG_CHFIR	0x8f0
#endif

#ifndef REG_CDDTXP
#define REG_CDDTXP	0x93c
#endif

#ifndef REG_TXPSEL1
#define REG_TXPSEL1	0x940
#endif

#ifndef REG_ACBB0
#define REG_ACBB0	0x948
#endif

#ifndef REG_ACBBRXFIR
#define REG_ACBBRXFIR	0x94c
#endif

#ifndef REG_ACGG2TBL
#define REG_ACGG2TBL	0x958
#endif

#ifndef REG_ADCINI
#define REG_ADCINI	0xa04
#endif

#ifndef REG_PWRTH
#define REG_PWRTH	0xa08
#endif

#ifndef REG_CCA_FLTR
#define REG_CCA_FLTR	0xa20
#endif

#ifndef REG_TXSF2
#define REG_TXSF2	0xa24
#endif

#ifndef REG_TXSF6
#define REG_TXSF6	0xa28
#endif

#ifndef REG_RXDESC
#define REG_RXDESC	0xa2c
#endif

#ifndef REG_ENTXCCK
#define REG_ENTXCCK	0xa80
#endif

#ifndef BTG_LNA
#define BTG_LNA		0xfc84
#endif

#ifndef WLG_LNA
#define WLG_LNA		0x7532
#endif

#ifndef REG_ENRXCCA
#define REG_ENRXCCA	0xa84
#endif

#ifndef BTG_CCA
#define BTG_CCA		0x0e
#endif

#ifndef WLG_CCA
#define WLG_CCA		0x12
#endif

#ifndef REG_PWRTH2
#define REG_PWRTH2	0xaa8
#endif

#ifndef REG_CSRATIO
#define REG_CSRATIO	0xaaa
#endif

#ifndef REG_TXFILTER
#define REG_TXFILTER	0xaac
#endif

#ifndef REG_AGCTR_A
#define REG_AGCTR_A	0xc08
#endif

#ifndef REG_TXDFIR
#define REG_TXDFIR	0xc20
#endif

#ifndef REG_TRSW
#define REG_TRSW	0xca0
#endif

#ifndef REG_RFESEL0
#define REG_RFESEL0	0xcb0
#endif

#ifndef REG_RFESEL8
#define REG_RFESEL8	0xcb4
#endif

#ifndef REG_RFECTL
#define REG_RFECTL	0xcb8
#endif

#ifndef B_BTG_SWITCH
#define B_BTG_SWITCH	BIT(16)
#endif

#ifndef B_CTRL_SWITCH
#define B_CTRL_SWITCH	BIT(18)
#endif

#ifndef B_WL_SWITCH
#define B_WL_SWITCH	(BIT(20) | BIT(22))
#endif

#ifndef B_WLG_SWITCH
#define B_WLG_SWITCH	BIT(21)
#endif

#ifndef B_WLA_SWITCH
#define B_WLA_SWITCH	BIT(23)
#endif

#ifndef REG_RFEINV
#define REG_RFEINV	0xcbc
#endif

#ifndef REG_AGCTR_B
#define REG_AGCTR_B	0xe08
#endif

#ifndef REG_DMEM_CTRL
#define REG_DMEM_CTRL	0x1080
#endif

#ifndef BIT_WL_RST
#define BIT_WL_RST	BIT(16)
#endif

#ifndef REG_ANTWT
#define REG_ANTWT	0x1904
#endif

#ifndef REG_IQKFAILMSK
#define REG_IQKFAILMSK	0x1bf0
#endif

#ifndef BIT_MASK_R_RFE_SEL_15
#define BIT_MASK_R_RFE_SEL_15	GENMASK(31, 28)
#endif

#ifndef BIT_SDIO_INT
#define BIT_SDIO_INT BIT(18)
#endif

#ifndef BT_CNT_ENABLE
#define BT_CNT_ENABLE	0x1
#endif

#ifndef BIT_BCN_QUEUE
#define BIT_BCN_QUEUE	BIT(3)
#endif

#ifndef BCN_PRI_EN
#define BCN_PRI_EN	0x1
#endif

#ifndef ANTDIC_CTRL_PIN
#define ANTDIC_CTRL_PIN	0x88
#endif

#ifndef REG_CTRL_TYPE
#define REG_CTRL_TYPE	0x67
#endif

#ifndef BIT_CTRL_TYPE1
#define BIT_CTRL_TYPE1	BIT(5)
#endif

#ifndef BIT_CTRL_TYPE2
#define BIT_CTRL_TYPE2	BIT(4)
#endif

#ifndef CTRL_TYPE_MASK
#define CTRL_TYPE_MASK	GENMASK(15, 8)
#endif

#ifndef RF18_BAND_2G
#define RF18_BAND_2G		(0)
#endif

#ifndef RF18_BAND_5G
#define RF18_BAND_5G		(BIT(16) | BIT(8))
#endif

#ifndef RF18_RFSI_GE
#define RF18_RFSI_GE		(BIT(17))
#endif

#ifndef RF18_RFSI_GT
#define RF18_RFSI_GT		(BIT(18))
#endif

#ifndef RF18_BW_MASK
#define RF18_BW_MASK		(BIT(11) | BIT(10))
#endif

#ifndef RF18_BW_20M
#define RF18_BW_20M		(BIT(11) | BIT(10))
#endif

#ifndef RF18_BW_40M
#define RF18_BW_40M		(BIT(11))
#endif

#ifndef RF18_BW_80M
#define RF18_BW_80M		(BIT(10))
#endif

#ifndef __RTW8822B_H__
#define __RTW8822B_H__
#endif

#ifndef RTW8822B_EDCCA_MAX
#define RTW8822B_EDCCA_MAX	0x7f
#endif

#ifndef RTW8822B_EDCCA_SRC_DEF
#define RTW8822B_EDCCA_SRC_DEF	1
#endif

#ifndef REG_EDCCA_POW_MA
#define REG_EDCCA_POW_MA	0x8a0
#endif

#ifndef BIT_MA_LEVEL
#define BIT_MA_LEVEL	GENMASK(1, 0)
#endif

#ifndef REG_EDCCA_DECISION
#define REG_EDCCA_DECISION	0x8dc
#endif

#ifndef BIT_EDCCA_OPTION
#define BIT_EDCCA_OPTION	BIT(5)
#endif

#ifndef REG_EDCCA_SOURCE
#define REG_EDCCA_SOURCE	0x944
#endif

#ifndef BIT_SOURCE_OPTION
#define BIT_SOURCE_OPTION	GENMASK(29, 28)
#endif

#ifndef __RTW8822C_H__
#define __RTW8822C_H__
#endif

#ifndef RTW_DECL_TABLE_DPK
#define RTW_DECL_TABLE_DPK(name)			\
const struct rtw_table name ## _tbl = {			\
	.data = name,					\
	.size = ARRAY_SIZE(name),			\
	.parse = rtw8822c_parse_tbl_dpk,		\
}
#endif

#ifndef DACK_PATH_8822C
#define DACK_PATH_8822C		2
#endif

#ifndef DACK_REG_8822C
#define DACK_REG_8822C		16
#endif

#ifndef DACK_RF_8822C
#define DACK_RF_8822C		1
#endif

#ifndef DACK_SN_8822C
#define DACK_SN_8822C		100
#endif

#ifndef GET_PHY_STAT_P0_PWDB_A
#define GET_PHY_STAT_P0_PWDB_A(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(15, 8))
#endif

#ifndef GET_PHY_STAT_P0_PWDB_B
#define GET_PHY_STAT_P0_PWDB_B(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(7, 0))
#endif

#ifndef GET_PHY_STAT_P0_GAIN_A
#define GET_PHY_STAT_P0_GAIN_A(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x00), GENMASK(21, 16))
#endif

#ifndef GET_PHY_STAT_P0_CHANNEL
#define GET_PHY_STAT_P0_CHANNEL(phy_stat)				       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(23, 16))
#endif

#ifndef GET_PHY_STAT_P0_GAIN_B
#define GET_PHY_STAT_P0_GAIN_B(phy_stat)                                       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x04), GENMASK(29, 24))
#endif

#ifndef GET_PHY_STAT_P1_CHANNEL
#define GET_PHY_STAT_P1_CHANNEL(phy_stat)				       \
	le32_get_bits(*((__le32 *)(phy_stat) + 0x01), GENMASK(23, 16))
#endif

#ifndef RTW8822C_EDCCA_MAX
#define RTW8822C_EDCCA_MAX	0x7f
#endif

#ifndef REG_ANAPARLDO_POW_MAC
#define REG_ANAPARLDO_POW_MAC	0x0029
#endif

#ifndef BIT_LDOE25_PON
#define BIT_LDOE25_PON		BIT(0)
#endif

#ifndef XCAP_MASK
#define XCAP_MASK		GENMASK(6, 0)
#endif

#ifndef CFO_TRK_ENABLE_TH
#define CFO_TRK_ENABLE_TH	20
#endif

#ifndef CFO_TRK_STOP_TH
#define CFO_TRK_STOP_TH		10
#endif

#ifndef CFO_TRK_ADJ_TH
#define CFO_TRK_ADJ_TH		10
#endif

#ifndef REG_TXDFIR0
#define REG_TXDFIR0		0x808
#endif

#ifndef REG_DFIRBW
#define REG_DFIRBW		0x810
#endif

#ifndef REG_ANTMAP0
#define REG_ANTMAP0		0x820
#endif

#ifndef BIT_ANT_PATH
#define BIT_ANT_PATH		GENMASK(1, 0)
#endif

#ifndef REG_ANTMAP
#define REG_ANTMAP		0x824
#endif

#ifndef REG_DYMPRITH
#define REG_DYMPRITH		0x86c
#endif

#ifndef REG_DYMENTH0
#define REG_DYMENTH0		0x870
#endif

#ifndef REG_DYMENTH
#define REG_DYMENTH		0x874
#endif

#ifndef REG_SBD
#define REG_SBD			0x88c
#endif

#ifndef BITS_SUBTUNE
#define BITS_SUBTUNE		GENMASK(15, 12)
#endif

#ifndef REG_DYMTHMIN
#define REG_DYMTHMIN		0x8a4
#endif

#ifndef REG_TXBWCTL
#define REG_TXBWCTL		0x9b0
#endif

#ifndef REG_TXCLK
#define REG_TXCLK		0x9b4
#endif

#ifndef REG_SCOTRK
#define REG_SCOTRK		0xc30
#endif

#ifndef REG_MRCM
#define REG_MRCM		0xc38
#endif

#ifndef REG_AGCSWSH
#define REG_AGCSWSH		0xc44
#endif

#ifndef REG_ANTWTPD
#define REG_ANTWTPD		0xc54
#endif

#ifndef REG_PT_CHSMO
#define REG_PT_CHSMO		0xcbc
#endif

#ifndef BIT_PT_OPT
#define BIT_PT_OPT		BIT(21)
#endif

#ifndef REG_ORITXCODE
#define REG_ORITXCODE		0x1800
#endif

#ifndef BIT_PATH_EN
#define BIT_PATH_EN		BIT(31)
#endif

#ifndef REG_3WIRE
#define REG_3WIRE		0x180c
#endif

#ifndef BIT_DIS_SHARERX_TXGAT
#define BIT_DIS_SHARERX_TXGAT	BIT(27)
#endif

#ifndef BIT_3WIRE_TX_EN
#define BIT_3WIRE_TX_EN		BIT(0)
#endif

#ifndef BIT_3WIRE_RX_EN
#define BIT_3WIRE_RX_EN		BIT(1)
#endif

#ifndef BIT_3WIRE_EN
#define BIT_3WIRE_EN		GENMASK(1, 0)
#endif

#ifndef BIT_3WIRE_PI_ON
#define BIT_3WIRE_PI_ON		BIT(28)
#endif

#ifndef REG_ANAPAR_A
#define REG_ANAPAR_A		0x1830
#endif

#ifndef BIT_ANAPAR_UPDATE
#define BIT_ANAPAR_UPDATE	BIT(29)
#endif

#ifndef REG_RFTXEN_GCK_A
#define REG_RFTXEN_GCK_A	0x1864
#endif

#ifndef BIT_RFTXEN_GCK_FORCE_ON
#define BIT_RFTXEN_GCK_FORCE_ON	BIT(31)
#endif

#ifndef REG_DIS_SHARE_RX_A
#define REG_DIS_SHARE_RX_A	0x186c
#endif

#ifndef BIT_TX_SCALE_0DB
#define BIT_TX_SCALE_0DB	BIT(7)
#endif

#ifndef REG_RXAGCCTL0
#define REG_RXAGCCTL0		0x18ac
#endif

#ifndef BITS_RXAGC_CCK
#define BITS_RXAGC_CCK		GENMASK(15, 12)
#endif

#ifndef BITS_RXAGC_OFDM
#define BITS_RXAGC_OFDM		GENMASK(8, 4)
#endif

#ifndef REG_DCKA_I_0
#define REG_DCKA_I_0		0x18bc
#endif

#ifndef REG_DCKA_I_1
#define REG_DCKA_I_1		0x18c0
#endif

#ifndef REG_DCKA_Q_0
#define REG_DCKA_Q_0		0x18d8
#endif

#ifndef REG_DCKA_Q_1
#define REG_DCKA_Q_1		0x18dc
#endif

#ifndef REG_CCKSB
#define REG_CCKSB		0x1a00
#endif

#ifndef BIT_BBMODE
#define BIT_BBMODE		GENMASK(2, 1)
#endif

#ifndef REG_RXCCKSEL
#define REG_RXCCKSEL		0x1a04
#endif

#ifndef REG_BGCTRL
#define REG_BGCTRL		0x1a14
#endif

#ifndef BITS_RX_IQ_WEIGHT
#define BITS_RX_IQ_WEIGHT	(BIT(8) | BIT(9))
#endif

#ifndef REG_TXF0
#define REG_TXF0		0x1a20
#endif

#ifndef REG_TXF1
#define REG_TXF1		0x1a24
#endif

#ifndef REG_TXF2
#define REG_TXF2		0x1a28
#endif

#ifndef REG_CCANRX
#define REG_CCANRX		0x1a2c
#endif

#ifndef BIT_CCK_FA_RST
#define BIT_CCK_FA_RST		(BIT(14) | BIT(15))
#endif

#ifndef BIT_OFDM_FA_RST
#define BIT_OFDM_FA_RST		(BIT(12) | BIT(13))
#endif

#ifndef REG_CCK_FACNT
#define REG_CCK_FACNT		0x1a5c
#endif

#ifndef REG_CCKTXONLY
#define REG_CCKTXONLY		0x1a80
#endif

#ifndef BIT_BB_CCK_CHECK_EN
#define BIT_BB_CCK_CHECK_EN	BIT(18)
#endif

#ifndef REG_TXF3
#define REG_TXF3		0x1a98
#endif

#ifndef REG_TXF4
#define REG_TXF4		0x1a9c
#endif

#ifndef REG_TXF5
#define REG_TXF5		0x1aa0
#endif

#ifndef REG_TXF6
#define REG_TXF6		0x1aac
#endif

#ifndef REG_TXF7
#define REG_TXF7		0x1ab0
#endif

#ifndef REG_CCK_SOURCE
#define REG_CCK_SOURCE		0x1abc
#endif

#ifndef BIT_NBI_EN
#define BIT_NBI_EN		BIT(30)
#endif

#ifndef REG_NCTL0
#define REG_NCTL0		0x1b00
#endif

#ifndef BIT_SEL_PATH
#define BIT_SEL_PATH		GENMASK(2, 1)
#endif

#ifndef BIT_SUBPAGE
#define BIT_SUBPAGE		GENMASK(3, 0)
#endif

#ifndef REG_DPD_CTL0_S0
#define REG_DPD_CTL0_S0		0x1b04
#endif

#ifndef BIT_GS_PWSF
#define BIT_GS_PWSF		GENMASK(27, 0)
#endif

#ifndef REG_DPD_CTL1_S0
#define REG_DPD_CTL1_S0		0x1b08
#endif

#ifndef BIT_DPD_EN
#define BIT_DPD_EN		BIT(31)
#endif

#ifndef BIT_PS_EN
#define BIT_PS_EN		BIT(7)
#endif

#ifndef REG_IQKSTAT
#define REG_IQKSTAT		0x1b10
#endif

#ifndef REG_IQK_CTL1
#define REG_IQK_CTL1		0x1b20
#endif

#ifndef BIT_TX_CFIR
#define BIT_TX_CFIR		GENMASK(31, 30)
#endif

#ifndef BIT_CFIR_EN
#define BIT_CFIR_EN		GENMASK(26, 24)
#endif

#ifndef BIT_BYPASS_DPD
#define BIT_BYPASS_DPD		BIT(25)
#endif

#ifndef REG_TX_TONE_IDX
#define REG_TX_TONE_IDX		0x1b2c
#endif

#ifndef REG_DPD_LUT0
#define REG_DPD_LUT0		0x1b44
#endif

#ifndef BIT_GLOSS_DB
#define BIT_GLOSS_DB		GENMASK(14, 12)
#endif

#ifndef REG_DPD_CTL0_S1
#define REG_DPD_CTL0_S1		0x1b5c
#endif

#ifndef REG_DPD_CTL1_S1
#define REG_DPD_CTL1_S1		0x1b60
#endif

#ifndef REG_DPD_AGC
#define REG_DPD_AGC		0x1b67
#endif

#ifndef REG_TABLE_SEL
#define REG_TABLE_SEL		0x1b98
#endif

#ifndef BIT_I_GAIN
#define BIT_I_GAIN		GENMASK(19, 16)
#endif

#ifndef BIT_GAIN_RST
#define BIT_GAIN_RST		BIT(15)
#endif

#ifndef BIT_Q_GAIN_SEL
#define BIT_Q_GAIN_SEL		GENMASK(14, 12)
#endif

#ifndef BIT_Q_GAIN
#define BIT_Q_GAIN		GENMASK(11, 0)
#endif

#ifndef REG_TX_GAIN_SET
#define REG_TX_GAIN_SET		0x1b9c
#endif

#ifndef BIT_GAPK_RPT_IDX
#define BIT_GAPK_RPT_IDX	GENMASK(11, 8)
#endif

#ifndef REG_DPD_CTL0
#define REG_DPD_CTL0		0x1bb4
#endif

#ifndef REG_SINGLE_TONE_SW
#define REG_SINGLE_TONE_SW	0x1bb8
#endif

#ifndef BIT_IRQ_TEST_MODE
#define BIT_IRQ_TEST_MODE	BIT(20)
#endif

#ifndef REG_R_CONFIG
#define REG_R_CONFIG		0x1bcc
#endif

#ifndef BIT_INNER_LB
#define BIT_INNER_LB		BIT(21)
#endif

#ifndef BIT_IQ_SWITCH
#define BIT_IQ_SWITCH		GENMASK(5, 0)
#endif

#ifndef BIT_2G_SWING
#define BIT_2G_SWING		0x2d
#endif

#ifndef BIT_5G_SWING
#define BIT_5G_SWING		0x36
#endif

#ifndef REG_RXSRAM_CTL
#define REG_RXSRAM_CTL		0x1bd4
#endif

#ifndef BIT_RPT_EN
#define BIT_RPT_EN		BIT(21)
#endif

#ifndef BIT_RPT_SEL
#define BIT_RPT_SEL		GENMASK(20, 16)
#endif

#ifndef BIT_DPD_CLK
#define BIT_DPD_CLK		GENMASK(7, 4)
#endif

#ifndef REG_DPD_CTL11
#define REG_DPD_CTL11		0x1be4
#endif

#ifndef REG_DPD_CTL12
#define REG_DPD_CTL12		0x1be8
#endif

#ifndef REG_DPD_CTL15
#define REG_DPD_CTL15		0x1bf4
#endif

#ifndef REG_DPD_CTL16
#define REG_DPD_CTL16		0x1bf8
#endif

#ifndef REG_STAT_RPT
#define REG_STAT_RPT		0x1bfc
#endif

#ifndef BIT_RPT_DGAIN
#define BIT_RPT_DGAIN		GENMASK(27, 16)
#endif

#ifndef BIT_GAPK_RPT0
#define BIT_GAPK_RPT0		GENMASK(3, 0)
#endif

#ifndef BIT_GAPK_RPT1
#define BIT_GAPK_RPT1		GENMASK(7, 4)
#endif

#ifndef BIT_GAPK_RPT2
#define BIT_GAPK_RPT2		GENMASK(11, 8)
#endif

#ifndef BIT_GAPK_RPT3
#define BIT_GAPK_RPT3		GENMASK(15, 12)
#endif

#ifndef BIT_GAPK_RPT4
#define BIT_GAPK_RPT4		GENMASK(19, 16)
#endif

#ifndef BIT_GAPK_RPT5
#define BIT_GAPK_RPT5		GENMASK(23, 20)
#endif

#ifndef BIT_GAPK_RPT6
#define BIT_GAPK_RPT6		GENMASK(27, 24)
#endif

#ifndef BIT_GAPK_RPT7
#define BIT_GAPK_RPT7		GENMASK(31, 28)
#endif

#ifndef REG_TXANT
#define REG_TXANT		0x1c28
#endif

#ifndef REG_IQK_CTRL
#define REG_IQK_CTRL		0x1c38
#endif

#ifndef REG_ENCCK
#define REG_ENCCK		0x1c3c
#endif

#ifndef BIT_CCK_BLK_EN
#define BIT_CCK_BLK_EN		BIT(1)
#endif

#ifndef BIT_CCK_OFDM_BLK_EN
#define BIT_CCK_OFDM_BLK_EN	(BIT(0) | BIT(1))
#endif

#ifndef REG_CCAMSK
#define REG_CCAMSK		0x1c80
#endif

#ifndef REG_RSTB
#define REG_RSTB		0x1c90
#endif

#ifndef BIT_RSTB_3WIRE
#define BIT_RSTB_3WIRE		BIT(8)
#endif

#ifndef REG_CH_DELAY_EXTR2
#define REG_CH_DELAY_EXTR2	0x1cd0
#endif

#ifndef BIT_TST_IQK2SET_SRC
#define BIT_TST_IQK2SET_SRC	BIT(31)
#endif

#ifndef BIT_EN_IOQ_IQK_DPK
#define BIT_EN_IOQ_IQK_DPK	BIT(30)
#endif

#ifndef BIT_IQK_DPK_RESET_SRC
#define BIT_IQK_DPK_RESET_SRC	BIT(29)
#endif

#ifndef BIT_IQK_DPK_CLOCK_SRC
#define BIT_IQK_DPK_CLOCK_SRC	BIT(28)
#endif

#ifndef REG_RX_BREAK
#define REG_RX_BREAK		0x1d2c
#endif

#ifndef BIT_COM_RX_GCK_EN
#define BIT_COM_RX_GCK_EN	BIT(31)
#endif

#ifndef REG_RXFNCTL
#define REG_RXFNCTL		0x1d30
#endif

#ifndef REG_CCA_OFF
#define REG_CCA_OFF		0x1d58
#endif

#ifndef BIT_CCA_ON_BY_PW
#define BIT_CCA_ON_BY_PW	GENMASK(11, 3)
#endif

#ifndef REG_RXIGI
#define REG_RXIGI		0x1d70
#endif

#ifndef REG_ENFN
#define REG_ENFN		0x1e24
#endif

#ifndef BIT_IQK_DPK_EN
#define BIT_IQK_DPK_EN		BIT(17)
#endif

#ifndef REG_TXANTSEG
#define REG_TXANTSEG		0x1e28
#endif

#ifndef BIT_ANTSEG
#define BIT_ANTSEG		GENMASK(3, 0)
#endif

#ifndef REG_TXLGMAP
#define REG_TXLGMAP		0x1e2c
#endif

#ifndef REG_CCKPATH
#define REG_CCKPATH		0x1e5c
#endif

#ifndef REG_TX_FIFO
#define REG_TX_FIFO		0x1e70
#endif

#ifndef BIT_STOP_TX
#define BIT_STOP_TX		GENMASK(3, 0)
#endif

#ifndef REG_CNT_CTRL
#define REG_CNT_CTRL		0x1eb4
#endif

#ifndef BIT_ALL_CNT_RST
#define BIT_ALL_CNT_RST		BIT(25)
#endif

#ifndef REG_OFDM_FACNT
#define REG_OFDM_FACNT		0x2d00
#endif

#ifndef REG_OFDM_FACNT1
#define REG_OFDM_FACNT1		0x2d04
#endif

#ifndef REG_OFDM_FACNT2
#define REG_OFDM_FACNT2		0x2d08
#endif

#ifndef REG_OFDM_FACNT3
#define REG_OFDM_FACNT3		0x2d0c
#endif

#ifndef REG_OFDM_FACNT4
#define REG_OFDM_FACNT4		0x2d10
#endif

#ifndef REG_OFDM_FACNT5
#define REG_OFDM_FACNT5		0x2d20
#endif

#ifndef REG_RPT_CIP
#define REG_RPT_CIP		0x2d9c
#endif

#ifndef BIT_RPT_CIP_STATUS
#define BIT_RPT_CIP_STATUS	GENMASK(7, 0)
#endif

#ifndef REG_OFDM_TXCNT
#define REG_OFDM_TXCNT		0x2de0
#endif

#ifndef REG_ORITXCODE2
#define REG_ORITXCODE2		0x4100
#endif

#ifndef REG_3WIRE2
#define REG_3WIRE2		0x410c
#endif

#ifndef REG_ANAPAR_B
#define REG_ANAPAR_B		0x4130
#endif

#ifndef REG_RFTXEN_GCK_B
#define REG_RFTXEN_GCK_B	0x4164
#endif

#ifndef REG_DIS_SHARE_RX_B
#define REG_DIS_SHARE_RX_B	0x416c
#endif

#ifndef BIT_EXT_TIA_BW
#define BIT_EXT_TIA_BW		BIT(1)
#endif

#ifndef REG_RXAGCCTL
#define REG_RXAGCCTL		0x41ac
#endif

#ifndef REG_DCKB_I_0
#define REG_DCKB_I_0		0x41bc
#endif

#ifndef REG_DCKB_I_1
#define REG_DCKB_I_1		0x41c0
#endif

#ifndef REG_DCKB_Q_0
#define REG_DCKB_Q_0		0x41d8
#endif

#ifndef REG_DCKB_Q_1
#define REG_DCKB_Q_1		0x41dc
#endif

#ifndef RF_MODE_TRXAGC
#define RF_MODE_TRXAGC		0x00
#endif

#ifndef BIT_RF_MODE
#define BIT_RF_MODE		GENMASK(19, 16)
#endif

#ifndef BIT_RXAGC
#define BIT_RXAGC		GENMASK(9, 5)
#endif

#ifndef BIT_TXAGC
#define BIT_TXAGC		GENMASK(4, 0)
#endif

#ifndef RF_RXAGC_OFFSET
#define RF_RXAGC_OFFSET		0x19
#endif

#ifndef RF_BW_TRXBB
#define RF_BW_TRXBB		0x1a
#endif

#ifndef BIT_TX_CCK_IND
#define BIT_TX_CCK_IND		BIT(16)
#endif

#ifndef BIT_BW_TXBB
#define BIT_BW_TXBB		GENMASK(14, 12)
#endif

#ifndef BIT_BW_RXBB
#define BIT_BW_RXBB		GENMASK(11, 10)
#endif

#ifndef BIT_DBG_CCK_CCA
#define BIT_DBG_CCK_CCA		BIT(1)
#endif

#ifndef RF_TX_GAIN_OFFSET
#define RF_TX_GAIN_OFFSET	0x55
#endif

#ifndef BIT_BB_GAIN
#define BIT_BB_GAIN		GENMASK(18, 14)
#endif

#ifndef BIT_RF_GAIN
#define BIT_RF_GAIN		GENMASK(4, 2)
#endif

#ifndef RF_TX_GAIN
#define RF_TX_GAIN		0x56
#endif

#ifndef BIT_GAIN_TXBB
#define BIT_GAIN_TXBB		GENMASK(4, 0)
#endif

#ifndef RF_IDAC
#define RF_IDAC			0x58
#endif

#ifndef BIT_TX_MODE
#define BIT_TX_MODE		GENMASK(19, 8)
#endif

#ifndef RF_TX_RESULT
#define RF_TX_RESULT		0x5f
#endif

#ifndef BIT_GAIN_TX_PAD_H
#define BIT_GAIN_TX_PAD_H	GENMASK(11, 8)
#endif

#ifndef BIT_GAIN_TX_PAD_L
#define BIT_GAIN_TX_PAD_L	GENMASK(7, 4)
#endif

#ifndef RF_PA
#define RF_PA			0x60
#endif

#ifndef RF_PABIAS_2G_MASK
#define RF_PABIAS_2G_MASK	GENMASK(15, 12)
#endif

#ifndef RF_PABIAS_5G_MASK
#define RF_PABIAS_5G_MASK	GENMASK(19, 16)
#endif

#ifndef RF_TXA_LB_SW
#define RF_TXA_LB_SW		0x63
#endif

#ifndef BIT_TXA_LB_ATT
#define BIT_TXA_LB_ATT		GENMASK(15, 14)
#endif

#ifndef BIT_LB_SW
#define BIT_LB_SW		GENMASK(13, 12)
#endif

#ifndef BIT_LB_ATT
#define BIT_LB_ATT		GENMASK(4, 2)
#endif

#ifndef RF_RXG_GAIN
#define RF_RXG_GAIN		0x87
#endif

#ifndef BIT_RXG_GAIN
#define BIT_RXG_GAIN		BIT(18)
#endif

#ifndef RF_RXA_MIX_GAIN
#define RF_RXA_MIX_GAIN		0x8a
#endif

#ifndef BIT_RXA_MIX_GAIN
#define BIT_RXA_MIX_GAIN	GENMASK(4, 3)
#endif

#ifndef RF_EXT_TIA_BW
#define RF_EXT_TIA_BW		0x8f
#endif

#ifndef BIT_PW_EXT_TIA
#define BIT_PW_EXT_TIA		BIT(1)
#endif

#ifndef RF_DIS_BYPASS_TXBB
#define RF_DIS_BYPASS_TXBB	0x9e
#endif

#ifndef BIT_TXBB
#define BIT_TXBB		BIT(10)
#endif

#ifndef BIT_TIA_BYPASS
#define BIT_TIA_BYPASS		BIT(5)
#endif

#ifndef RF_DEBUG
#define RF_DEBUG		0xde
#endif

#ifndef BIT_DE_PWR_TRIM
#define BIT_DE_PWR_TRIM		BIT(19)
#endif

#ifndef BIT_DE_TX_GAIN
#define BIT_DE_TX_GAIN		BIT(16)
#endif

#ifndef BIT_DE_TRXBW
#define BIT_DE_TRXBW		BIT(2)
#endif

#ifndef PPG_THERMAL_B
#define PPG_THERMAL_B		0x1b0
#endif

#ifndef RF_THEMAL_MASK
#define RF_THEMAL_MASK		GENMASK(19, 16)
#endif

#ifndef PPG_2GH_TXAB
#define PPG_2GH_TXAB		0x1d2
#endif

#ifndef PPG_2G_A_MASK
#define PPG_2G_A_MASK		GENMASK(3, 0)
#endif

#ifndef PPG_2G_B_MASK
#define PPG_2G_B_MASK		GENMASK(7, 4)
#endif

#ifndef PPG_2GL_TXAB
#define PPG_2GL_TXAB		0x1d4
#endif

#ifndef PPG_PABIAS_2GB
#define PPG_PABIAS_2GB		0x1d5
#endif

#ifndef PPG_PABIAS_2GA
#define PPG_PABIAS_2GA		0x1d6
#endif

#ifndef PPG_PABIAS_MASK
#define PPG_PABIAS_MASK		GENMASK(3, 0)
#endif

#ifndef PPG_PABIAS_5GB
#define PPG_PABIAS_5GB		0x1d7
#endif

#ifndef PPG_PABIAS_5GA
#define PPG_PABIAS_5GA		0x1d8
#endif

#ifndef PPG_5G_MASK
#define PPG_5G_MASK		GENMASK(4, 0)
#endif

#ifndef PPG_5GH1_TXB
#define PPG_5GH1_TXB		0x1db
#endif

#ifndef PPG_5GH1_TXA
#define PPG_5GH1_TXA		0x1dc
#endif

#ifndef PPG_5GM2_TXB
#define PPG_5GM2_TXB		0x1df
#endif

#ifndef PPG_5GM2_TXA
#define PPG_5GM2_TXA		0x1e0
#endif

#ifndef PPG_5GM1_TXB
#define PPG_5GM1_TXB		0x1e3
#endif

#ifndef PPG_5GM1_TXA
#define PPG_5GM1_TXA		0x1e4
#endif

#ifndef PPG_5GL2_TXB
#define PPG_5GL2_TXB		0x1e7
#endif

#ifndef PPG_5GL2_TXA
#define PPG_5GL2_TXA		0x1e8
#endif

#ifndef PPG_5GL1_TXB
#define PPG_5GL1_TXB		0x1eb
#endif

#ifndef PPG_5GL1_TXA
#define PPG_5GL1_TXA		0x1ec
#endif

#ifndef PPG_2GM_TXAB
#define PPG_2GM_TXAB		0x1ee
#endif

#ifndef PPG_THERMAL_A
#define PPG_THERMAL_A		0x1ef
#endif
