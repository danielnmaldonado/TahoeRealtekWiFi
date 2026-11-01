/* SPDX-License-Identifier: GPL-2.0-only */
/* Linux rtl8xxxu 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * Copyright(c)2014-2017 Jes Sorensen <Jes.Sorensen@gmail.com>.
 * Copyright(c)Realtek Corporation. Retain the accompanying provenance/license.
 * Generated register/table/algorithm subset, checked I/O supplied by the port. */
struct rtl8723au_idx {
#ifdef __LITTLE_ENDIAN
	int	a:4;
	int	b:4;
#else
	int	b:4;
	int	a:4;
#endif
} __attribute__((packed));
struct rtl8723bu_pwr_idx {
#ifdef __LITTLE_ENDIAN
	int	ht20:4;
	int	ht40:4;
	int	ofdm:4;
	int	cck:4;
#else
	int	cck:4;
	int	ofdm:4;
	int	ht40:4;
	int	ht20:4;
#endif
} __attribute__((packed));
struct rtl8723bu_efuse_tx_power {
	u8 cck_base[6];
	u8 ht40_base[5];
	struct rtl8723au_idx ht20_ofdm_1s_diff;
	struct rtl8723bu_pwr_idx pwr_diff[3];
	u8 dummy5g[24]; /* max channel group (14) + power diff offset (10) */
};
struct rtl8723bu_efuse {
	__le16 rtl_id;
	u8 res0[0x0e];
	struct rtl8723bu_efuse_tx_power tx_power_index_A;	/* 0x10 */
	struct rtl8723bu_efuse_tx_power tx_power_index_B;	/* 0x3a */
	struct rtl8723bu_efuse_tx_power tx_power_index_C;	/* 0x64 */
	struct rtl8723bu_efuse_tx_power tx_power_index_D;	/* 0x8e */
	u8 channel_plan;		/* 0xb8 */
	u8 xtal_k;
	u8 thermal_meter;
	u8 iqk_lck;
	u8 pa_type;			/* 0xbc */
	u8 lna_type_2g;			/* 0xbd */
	u8 res2[3];
	u8 rf_board_option;
	u8 rf_feature_option;
	u8 rf_bt_setting;
	u8 eeprom_version;
	u8 eeprom_customer_id;
	u8 res3[2];
	u8 tx_pwr_calibrate_rate;
	u8 rf_antenna_option;		/* 0xc9 */
	u8 rfe_option;
	u8 res4[9];
	u8 usb_optional_function;
	u8 res5[0x1e];
	u8 res6[2];
	u8 serial[0x0b];		/* 0xf5 */
	u8 vid;				/* 0x100 */
	u8 res7;
	u8 pid;
	u8 res8[4];
	u8 mac_addr[ETH_ALEN];		/* 0x107 */
	u8 res9[2];
	u8 vendor_name[0x07];
	u8 res10[2];
	u8 device_name[0x14];
	u8 res11[0xcf];
	u8 package_type;		/* 0x1fb */
	u8 res12[0x4];
} __attribute__((packed));
struct h2c_cmd {
	union {
		struct {
			u8 cmd;
			u8 data[7];
		} __packed cmd;
		struct {
			__le32 data;
			__le16 ext;
		} __packed raw;
		struct {
			__le32 data;
			__le32 ext;
		} __packed raw_wide;
		struct {
			u8 cmd;
			u8 data;
		} __packed joinbss;
		struct {
			u8 cmd;
			__le16 mask_hi;
			u8 arg;
			__le16 mask_lo;
		} __packed ramask;
		struct {
			u8 cmd;
			u8 parm;
			u8 macid;
			u8 macid_end;
		} __packed media_status_rpt;
		struct {
			u8 cmd;
			u8 macid;
			/*
			 * [0:4] - RAID
			 * [7]   - SGI
			 */
			u8 data1;
			/*
			 * [0:1] - Bandwidth
			 * [3]   - No Update
			 * [4:5] - VHT enable
			 * [6]   - DISPT
			 * [7]   - DISRA
			 */
			u8 data2;
			u8 ramask0;
			u8 ramask1;
			u8 ramask2;
			u8 ramask3;
		} __packed b_macid_cfg;
		struct {
			u8 cmd;
			u8 data1;
			u8 data2;
			u8 data3;
			u8 data4;
			u8 data5;
		} __packed b_type_dma;
		struct {
			u8 cmd;
			u8 data;
		} __packed bt_info;
		struct {
			u8 cmd;
			u8 operreq;
			u8 opcode;
			u8 data;
			u8 addr;
		} __packed bt_mp_oper;
		struct {
			u8 cmd;
			u8 data;
		} __packed bt_wlan_calibration;
		struct {
			u8 cmd;
			u8 data;
		} __packed ignore_wlan;
		struct {
			u8 cmd;
			u8 ant_inverse;
			u8 int_switch_type;
		} __packed ant_sel_rsv;
		struct {
			u8 cmd;
			u8 data;
		} __packed bt_grant;
		struct {
			u8 cmd;
			u8 macid;
			u8 unknown0;
			u8 rssi;
			/*
			 * [0]   - is_rx
			 * [1]   - stbc_en
			 * [2]   - noisy_decision
			 * [6]   - bf_en
			 */
			u8 data;
			/*
			 * [0:6] - ra_th_offset
			 * [7]   - ra_offset_direction
			 */
			u8 ra_th_offset;
			u8 unknown1;
			u8 unknown2;
		} __packed rssi_report;
	};
} __attribute__((packed));
struct rtl8xxxu_cfo_tracking {
	bool adjust;
	bool atc_status;
	int cfo_tail[2];
	u8 crystal_cap;
	u32 packet_count;
	u32 packet_count_pre;
} __attribute__((packed));
struct rtl8xxxu_firmware_header {
	__le16	signature;		/*  92C0: test chip; 92C,
					    88C0: test chip;
					    88C1: MP A-cut;
					    92C1: MP A-cut */
	u8	category;		/*  AP/NIC and USB/PCI */
	u8	function;

	__le16	major_version;		/*  FW Version */
	u8	minor_version;		/*  FW Subversion, default 0x00 */
	u8	reserved1;

	u8	month;			/*  Release time Month field */
	u8	date;			/*  Release time Date field */
	u8	hour;			/*  Release time Hour field */
	u8	minute;			/*  Release time Minute field */

	__le16	ramcodesize;		/*  Size of RAM code */
	u16	reserved2;

	__le32	svn_idx;		/*  SVN entry index */
	u32	reserved3;

	u32	reserved4;
	u32	reserved5;

	u8	data[];
} __attribute__((packed));
struct rtl8xxxu_btcoex {
	u8      bt_status;
	bool	bt_busy;
	bool	has_sco;
	bool	has_a2dp;
	bool    has_hid;
	bool    has_pan;
	bool	hid_only;
	bool	a2dp_only;
	bool    c2h_bt_inquiry;
};
struct rtl8xxxu_reg8val {
	u16 reg;
	u8 val;
};
struct rtl8xxxu_reg32val {
	u16 reg;
	u32 val;
};
struct rtl8xxxu_rfregval {
	u8 reg;
	u32 val;
};
struct rtl8xxxu_rfregs {
	u16 hssiparm1;
	u16 hssiparm2;
	u16 lssiparm;
	u16 hspiread;
	u16 lssiread;
	u16 rf_sw_ctrl;
};
enum rtl8xxxu_rfpath {
	RF_A = 0,
	RF_B = 1,
};
enum rtl8xxxu_rtl_chip {
	RTL8192S = 0x81920,
	RTL8191S = 0x81910,
	RTL8192C = 0x8192c,
	RTL8191C = 0x8191c,
	RTL8188C = 0x8188c,
	RTL8188R = 0x81889,
	RTL8192D = 0x8192d,
	RTL8723A = 0x8723a,
	RTL8188E = 0x8188e,
	RTL8812  = 0x88120,
	RTL8821  = 0x88210,
	RTL8192E = 0x8192e,
	RTL8191E = 0x8191e,
	RTL8723B = 0x8723b,
	RTL8814A = 0x8814a,
	RTL8881A = 0x8881a,
	RTL8821B = 0x8821b,
	RTL8822B = 0x8822b,
	RTL8703B = 0x8703b,
	RTL8195A = 0x8195a,
	RTL8188F = 0x8188f,
	RTL8710B = 0x8710b,
	RTL8192F = 0x8192f,
};
enum c2h_evt_8723b {
	C2H_8723B_DEBUG = 0,
	C2H_8723B_TSF = 1,
	C2H_8723B_AP_RPT_RSP = 2,
	C2H_8723B_CCX_TX_RPT = 3,
	C2H_8723B_BT_RSSI = 4,
	C2H_8723B_BT_OP_MODE = 5,
	C2H_8723B_EXT_RA_RPT = 6,
	C2H_8723B_BT_INFO = 9,
	C2H_8723B_HW_INFO_EXCH = 0x0a,
	C2H_8723B_BT_MP_INFO = 0x0b,
	C2H_8723B_RA_REPORT = 0x0c,
	C2H_8723B_FW_DEBUG = 0xff,
};
enum h2c_cmd_8723b {
	/*
	 * Common Class: 000
	 */
	H2C_8723B_RSVD_PAGE = 0x00,
	H2C_8723B_MEDIA_STATUS_RPT = 0x01,
	H2C_8723B_SCAN_ENABLE = 0x02,
	H2C_8723B_KEEP_ALIVE = 0x03,
	H2C_8723B_DISCON_DECISION = 0x04,
	H2C_8723B_PSD_OFFLOAD = 0x05,
	H2C_8723B_AP_OFFLOAD = 0x08,
	H2C_8723B_BCN_RSVDPAGE = 0x09,
	H2C_8723B_PROBERSP_RSVDPAGE = 0x0A,
	H2C_8723B_FCS_RSVDPAGE = 0x10,
	H2C_8723B_FCS_INFO = 0x11,
	H2C_8723B_AP_WOW_GPIO_CTRL = 0x13,

	/*
	 * PoweSave Class: 001
	 */
	H2C_8723B_SET_PWR_MODE = 0x20,
	H2C_8723B_PS_TUNING_PARA = 0x21,
	H2C_8723B_PS_TUNING_PARA2 = 0x22,
	H2C_8723B_P2P_LPS_PARAM = 0x23,
	H2C_8723B_P2P_PS_OFFLOAD = 0x24,
	H2C_8723B_PS_SCAN_ENABLE = 0x25,
	H2C_8723B_SAP_PS_ = 0x26,
	H2C_8723B_INACTIVE_PS_ = 0x27,
	H2C_8723B_FWLPS_IN_IPS_ = 0x28,

	/*
	 * Dynamic Mechanism Class: 010
	 */
	H2C_8723B_MACID_CFG_RAID = 0x40,
	H2C_8723B_TXBF = 0x41,
	H2C_8723B_RSSI_SETTING = 0x42,
	H2C_8723B_AP_REQ_TXRPT = 0x43,
	H2C_8723B_INIT_RATE_COLLECT = 0x44,

	/*
	 * BT Class: 011
	 */
	H2C_8723B_B_TYPE_TDMA = 0x60,
	H2C_8723B_BT_INFO = 0x61,
	H2C_8723B_FORCE_BT_TXPWR = 0x62,
	H2C_8723B_BT_IGNORE_WLANACT = 0x63,
	H2C_8723B_DAC_SWING_VALUE = 0x64,
	H2C_8723B_ANT_SEL_RSV = 0x65,
	H2C_8723B_WL_OPMODE = 0x66,
	H2C_8723B_BT_MP_OPER = 0x67,
	H2C_8723B_BT_CONTROL = 0x68,
	H2C_8723B_BT_WIFI_CTRL = 0x69,
	H2C_8723B_BT_FW_PATCH = 0x6a,
	H2C_8723B_BT_WLAN_CALIBRATION = 0x6d,
	H2C_8723B_BT_GRANT = 0x6e,

	/*
	 * WOWLAN Class: 100
	 */
	H2C_8723B_WOWLAN = 0x80,
	H2C_8723B_REMOTE_WAKE_CTRL = 0x81,
	H2C_8723B_AOAC_GLOBAL_INFO = 0x82,
	H2C_8723B_AOAC_RSVD_PAGE = 0x83,
	H2C_8723B_AOAC_RSVD_PAGE2 = 0x84,
	H2C_8723B_D0_SCAN_OFFLOAD_CTRL = 0x85,
	H2C_8723B_D0_SCAN_OFFLOAD_INFO = 0x86,
	H2C_8723B_CHNL_SWITCH_OFFLOAD = 0x87,

	H2C_8723B_RESET_TSF = 0xC0,
};
enum _BT_8723B_1ANT_STATUS {
	BT_8723B_1ANT_STATUS_NON_CONNECTED_IDLE      = 0x0,
	BT_8723B_1ANT_STATUS_CONNECTED_IDLE          = 0x1,
	BT_8723B_1ANT_STATUS_INQ_PAGE                = 0x2,
	BT_8723B_1ANT_STATUS_ACL_BUSY                = 0x3,
	BT_8723B_1ANT_STATUS_SCO_BUSY                = 0x4,
	BT_8723B_1ANT_STATUS_ACL_SCO_BUSY            = 0x5,
	BT_8723B_1ANT_STATUS_MAX
};
enum bt_info_src_8723b {
	BT_INFO_SRC_8723B_WIFI_FW = 0x0,
        BT_INFO_SRC_8723B_BT_RSP = 0x1,
        BT_INFO_SRC_8723B_BT_ACTIVE_SEND = 0x2,
};
enum bt_mp_oper_opcode_8723b {
	BT_MP_OP_GET_BT_VERSION	= 0x00,
	BT_MP_OP_RESET = 0x01,
	BT_MP_OP_TEST_CTRL = 0x02,
	BT_MP_OP_SET_BT_MODE = 0x03,
	BT_MP_OP_SET_CHNL_TX_GAIN = 0x04,
	BT_MP_OP_SET_PKT_TYPE_LEN = 0x05,
	BT_MP_OP_SET_PKT_CNT_L_PL_TYPE = 0x06,
	BT_MP_OP_SET_PKT_CNT_H_PKT_INTV = 0x07,
	BT_MP_OP_SET_PKT_HEADER = 0x08,
	BT_MP_OP_SET_WHITENCOEFF = 0x09,
	BT_MP_OP_SET_BD_ADDR_L = 0x0a,
	BT_MP_OP_SET_BD_ADDR_H = 0x0b,
	BT_MP_OP_WRITE_REG_ADDR = 0x0c,
	BT_MP_OP_WRITE_REG_VALUE = 0x0d,
	BT_MP_OP_GET_BT_STATUS = 0x0e,
	BT_MP_OP_GET_BD_ADDR_L = 0x0f,
	BT_MP_OP_GET_BD_ADDR_H = 0x10,
	BT_MP_OP_READ_REG = 0x11,
	BT_MP_OP_SET_TARGET_BD_ADDR_L = 0x12,
	BT_MP_OP_SET_TARGET_BD_ADDR_H = 0x13,
	BT_MP_OP_SET_TX_POWER_CALIBRATION = 0x14,
	BT_MP_OP_GET_RX_PKT_CNT_L = 0x15,
	BT_MP_OP_GET_RX_PKT_CNT_H = 0x16,
	BT_MP_OP_GET_RX_ERROR_BITS_L = 0x17,
	BT_MP_OP_GET_RX_ERROR_BITS_H = 0x18,
	BT_MP_OP_GET_RSSI = 0x19,
	BT_MP_OP_GET_CFO_HDR_QUALITY_L = 0x1a,
	BT_MP_OP_GET_CFO_HDR_QUALITY_H = 0x1b,
	BT_MP_OP_GET_TARGET_BD_ADDR_L = 0x1c,
	BT_MP_OP_GET_TARGET_BD_ADDR_H = 0x1d,
	BT_MP_OP_GET_AFH_MAP_L = 0x1e,
	BT_MP_OP_GET_AFH_MAP_M = 0x1f,
	BT_MP_OP_GET_AFH_MAP_H = 0x20,
	BT_MP_OP_GET_AFH_STATUS = 0x21,
	BT_MP_OP_SET_TRACKING_INTERVAL = 0x22,
	BT_MP_OP_SET_THERMAL_METER = 0x23,
	BT_MP_OP_ENABLE_CFO_TRACKING = 0x24,
};
