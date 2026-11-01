/* SPDX-License-Identifier: BSD-3-Clause */
/* Pinned Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation.
 * Dual GPL-2.0 OR BSD-3-Clause; this port selects BSD-3-Clause.
 * See RTL8821AUCoex-Upstream.json and RTL8812AUPHY-LICENSE.txt. */
/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright(c) 2018-2019  Realtek Corporation
 */


#define COEX_CCK_2	0x1
#define COEX_RESP_ACK_BY_WL_FW	0x1
#define COEX_REQUEST_TIMEOUT	msecs_to_jiffies(10)

#define COEX_MIN_DELAY		10 /* delay unit in ms */
#define COEX_RFK_TIMEOUT	600 /* RFK timeout in ms */
#define COEX_BT_GAMEHID_CNT	800

#define COEX_RF_OFF	0x0
#define COEX_RF_ON	0x1

#define COEX_H2C69_WL_LEAKAP	0xc
#define PARA1_H2C69_DIS_5MS	0x1
#define PARA1_H2C69_EN_5MS	0x0

#define COEX_H2C69_TDMA_SLOT	0xb
#define PARA1_H2C69_TDMA_4SLOT	0xc1
#define PARA1_H2C69_TDMA_2SLOT	0x1
#define PARA1_H2C69_TBTT_TIMES	GENMASK(5, 0)
#define PARA1_H2C69_TBTT_DIV100	BIT(7)

#define COEX_H2C69_TOGGLE_TABLE_A 0xd
#define COEX_H2C69_TOGGLE_TABLE_B 0x7

#define TDMA_4SLOT	BIT(8)

#define TDMA_TIMER_TYPE_2SLOT 0
#define TDMA_TIMER_TYPE_4SLOT 3

#define COEX_RSSI_STEP		4

#define COEX_RSSI_HIGH(rssi) ((rssi)==COEX_RSSI_STATE_HIGH || (rssi)==COEX_RSSI_STATE_STAY_HIGH)

#define COEX_RSSI_MEDIUM(rssi) ((rssi)==COEX_RSSI_STATE_MEDIUM || (rssi)==COEX_RSSI_STATE_STAY_MEDIUM)

#define COEX_RSSI_LOW(rssi) ((rssi)==COEX_RSSI_STATE_LOW || (rssi)==COEX_RSSI_STATE_STAY_LOW)

#define GET_COEX_RESP_BT_SUPP_VER(payload)				\
	le64_get_bits(cx_le64(payload), GENMASK_ULL(39, 32))
#define GET_COEX_RESP_BT_SUPP_FEAT(payload)				\
	le64_get_bits(cx_le64(payload), GENMASK_ULL(39, 24))
#define GET_COEX_RESP_BT_PATCH_VER(payload)				\
	le64_get_bits(cx_le64(payload), GENMASK_ULL(55, 24))
#define GET_COEX_RESP_BT_REG_VAL(payload)				\
	le64_get_bits(cx_le64(payload), GENMASK_ULL(39, 24))
#define GET_COEX_RESP_BT_SCAN_TYPE(payload)				\
	le64_get_bits(cx_le64(payload), GENMASK(31, 24))

enum coex_mp_info_op {
	BT_MP_INFO_OP_PATCH_VER	= 0x00,
	BT_MP_INFO_OP_READ_REG	= 0x11,
	BT_MP_INFO_OP_SUPP_FEAT	= 0x2a,
	BT_MP_INFO_OP_SUPP_VER	= 0x2b,
	BT_MP_INFO_OP_SCAN_TYPE	= 0x2d,
	BT_MP_INFO_OP_LNA_CONSTRAINT	= 0x32,
};

enum coex_set_ant_phase {
	COEX_SET_ANT_INIT,
	COEX_SET_ANT_WONLY,
	COEX_SET_ANT_WOFF,
	COEX_SET_ANT_2G,
	COEX_SET_ANT_5G,
	COEX_SET_ANT_POWERON,
	COEX_SET_ANT_2G_WLBT,
	COEX_SET_ANT_2G_FREERUN,

	COEX_SET_ANT_MAX
};

enum coex_runreason {
	COEX_RSN_2GSCANSTART	= 0,
	COEX_RSN_5GSCANSTART	= 1,
	COEX_RSN_SCANFINISH	= 2,
	COEX_RSN_2GSWITCHBAND	= 3,
	COEX_RSN_5GSWITCHBAND	= 4,
	COEX_RSN_2GCONSTART	= 5,
	COEX_RSN_5GCONSTART	= 6,
	COEX_RSN_2GCONFINISH	= 7,
	COEX_RSN_5GCONFINISH	= 8,
	COEX_RSN_2GMEDIA	= 9,
	COEX_RSN_5GMEDIA	= 10,
	COEX_RSN_MEDIADISCON	= 11,
	COEX_RSN_BTINFO		= 12,
	COEX_RSN_LPS		= 13,
	COEX_RSN_WLSTATUS	= 14,
	COEX_RSN_BTSTATUS	= 15,

	COEX_RSN_MAX
};

enum coex_lte_coex_table_type {
	COEX_CTT_WL_VS_LTE,
	COEX_CTT_BT_VS_LTE,
};

enum coex_gnt_setup_state {
	COEX_GNT_SET_HW_PTA	= 0x0,
	COEX_GNT_SET_SW_LOW	= 0x1,
	COEX_GNT_SET_SW_HIGH	= 0x3,
};

enum coex_ext_ant_switch_pos_type {
	COEX_SWITCH_TO_BT,
	COEX_SWITCH_TO_WLG,
	COEX_SWITCH_TO_WLA,
	COEX_SWITCH_TO_NOCARE,
	COEX_SWITCH_TO_WLG_BT,

	COEX_SWITCH_TO_MAX
};

enum coex_ext_ant_switch_ctrl_type {
	COEX_SWITCH_CTRL_BY_BBSW,
	COEX_SWITCH_CTRL_BY_PTA,
	COEX_SWITCH_CTRL_BY_ANTDIV,
	COEX_SWITCH_CTRL_BY_MAC,
	COEX_SWITCH_CTRL_BY_BT,
	COEX_SWITCH_CTRL_BY_FW,

	COEX_SWITCH_CTRL_MAX
};

enum coex_algorithm {
	COEX_ALGO_NOPROFILE	= 0,
	COEX_ALGO_HFP		= 1,
	COEX_ALGO_HID		= 2,
	COEX_ALGO_A2DP		= 3,
	COEX_ALGO_PAN		= 4,
	COEX_ALGO_A2DP_HID	= 5,
	COEX_ALGO_A2DP_PAN	= 6,
	COEX_ALGO_PAN_HID	= 7,
	COEX_ALGO_A2DP_PAN_HID	= 8,

	COEX_ALGO_MAX
};

enum coex_bt_profile {
	BPM_NOPROFILE		= 0,
	BPM_HFP			= BIT(0),
	BPM_HID			= BIT(1),
	BPM_A2DP		= BIT(2),
	BPM_PAN			= BIT(3),
	BPM_HID_HFP		= BPM_HID | BPM_HFP,
	BPM_A2DP_HFP		= BPM_A2DP | BPM_HFP,
	BPM_A2DP_HID		= BPM_A2DP | BPM_HID,
	BPM_A2DP_HID_HFP	= BPM_A2DP | BPM_HID | BPM_HFP,
	BPM_PAN_HFP		= BPM_PAN | BPM_HFP,
	BPM_PAN_HID		= BPM_PAN | BPM_HID,
	BPM_PAN_HID_HFP		= BPM_PAN | BPM_HID | BPM_HFP,
	BPM_PAN_A2DP		= BPM_PAN | BPM_A2DP,
	BPM_PAN_A2DP_HFP	= BPM_PAN | BPM_A2DP | BPM_HFP,
	BPM_PAN_A2DP_HID	= BPM_PAN | BPM_A2DP | BPM_HID,
	BPM_PAN_A2DP_HID_HFP	= BPM_PAN | BPM_A2DP | BPM_HID | BPM_HFP,
};

enum coex_wl_link_mode {
	COEX_WLINK_2G1PORT	= 0x0,
	COEX_WLINK_5G		= 0x3,
	COEX_WLINK_2GFREE	= 0x7,
	COEX_WLINK_MAX
};

enum coex_wl2bt_scoreboard {
	COEX_SCBD_ACTIVE	= BIT(0),
	COEX_SCBD_ONOFF		= BIT(1),
	COEX_SCBD_SCAN		= BIT(2),
	COEX_SCBD_UNDERTEST	= BIT(3),
	COEX_SCBD_RXGAIN	= BIT(4),
	COEX_SCBD_BT_RFK	= BIT(5),
	COEX_SCBD_WLBUSY	= BIT(6),
	COEX_SCBD_EXTFEM	= BIT(8),
	COEX_SCBD_TDMA		= BIT(9),
	COEX_SCBD_FIX2M		= BIT(10),
	COEX_SCBD_ALL		= GENMASK(15, 0),
};

enum coex_power_save_type {
	COEX_PS_WIFI_NATIVE	= 0,
	COEX_PS_LPS_ON		= 1,
	COEX_PS_LPS_OFF		= 2,
};

enum coex_rssi_state {
	COEX_RSSI_STATE_HIGH,
	COEX_RSSI_STATE_MEDIUM,
	COEX_RSSI_STATE_LOW,
	COEX_RSSI_STATE_STAY_HIGH,
	COEX_RSSI_STATE_STAY_MEDIUM,
	COEX_RSSI_STATE_STAY_LOW,
};

enum coex_notify_type_ips {
	COEX_IPS_LEAVE		= 0x0,
	COEX_IPS_ENTER		= 0x1,
};

enum coex_notify_type_lps {
	COEX_LPS_DISABLE	= 0x0,
	COEX_LPS_ENABLE		= 0x1,
};

enum coex_notify_type_scan {
	COEX_SCAN_FINISH,
	COEX_SCAN_START,
	COEX_SCAN_START_2G,
	COEX_SCAN_START_5G,
};

enum coex_notify_type_switchband {
	COEX_NOT_SWITCH,
	COEX_SWITCH_TO_24G,
	COEX_SWITCH_TO_5G,
	COEX_SWITCH_TO_24G_NOFORSCAN,
};

enum coex_notify_type_associate {
	COEX_ASSOCIATE_FINISH,
	COEX_ASSOCIATE_START,
	COEX_ASSOCIATE_5G_FINISH,
	COEX_ASSOCIATE_5G_START,
};

enum coex_notify_type_media_status {
	COEX_MEDIA_DISCONNECT,
	COEX_MEDIA_CONNECT,
	COEX_MEDIA_CONNECT_5G,
};

enum coex_bt_status {
	COEX_BTSTATUS_NCON_IDLE		= 0,
	COEX_BTSTATUS_CON_IDLE		= 1,
	COEX_BTSTATUS_INQ_PAGE		= 2,
	COEX_BTSTATUS_ACL_BUSY		= 3,
	COEX_BTSTATUS_SCO_BUSY		= 4,
	COEX_BTSTATUS_ACL_SCO_BUSY	= 5,

	COEX_BTSTATUS_MAX
};

enum coex_wl_tput_dir {
	COEX_WL_TPUT_TX			= 0x0,
	COEX_WL_TPUT_RX			= 0x1,
	COEX_WL_TPUT_MAX
};

enum coex_wl_priority_mask {
	COEX_WLPRI_RX_RSP	= 2,
	COEX_WLPRI_TX_RSP	= 3,
	COEX_WLPRI_TX_BEACON	= 4,
	COEX_WLPRI_TX_OFDM	= 11,
	COEX_WLPRI_TX_CCK	= 12,
	COEX_WLPRI_TX_BEACONQ	= 27,
	COEX_WLPRI_RX_CCK	= 28,
	COEX_WLPRI_RX_OFDM	= 29,
	COEX_WLPRI_MAX
};

enum coex_commom_chip_setup {
	COEX_CSETUP_INIT_HW		= 0x0,
	COEX_CSETUP_ANT_SWITCH		= 0x1,
	COEX_CSETUP_GNT_FIX		= 0x2,
	COEX_CSETUP_GNT_DEBUG		= 0x3,
	COEX_CSETUP_RFE_TYPE		= 0x4,
	COEX_CSETUP_COEXINFO_HW		= 0x5,
	COEX_CSETUP_WL_TX_POWER		= 0x6,
	COEX_CSETUP_WL_RX_GAIN		= 0x7,
	COEX_CSETUP_WLAN_ACT_IPS	= 0x8,
	COEX_CSETUP_MAX
};

enum coex_indirect_reg_type {
	COEX_INDIRECT_1700		= 0x0,
	COEX_INDIRECT_7C0		= 0x1,
	COEX_INDIRECT_MAX
};

enum coex_pstdma_type {
	COEX_PSTDMA_FORCE_LPSOFF	= 0x0,
	COEX_PSTDMA_FORCE_LPSON		= 0x1,
	COEX_PSTDMA_MAX
};

enum coex_btrssi_type {
	COEX_BTRSSI_RATIO		= 0x0,
	COEX_BTRSSI_DBM			= 0x1,
	COEX_BTRSSI_MAX
};

struct coex_table_para {
	u32 bt;
	u32 wl;
};

struct coex_tdma_para {
	u8 para[5];
};

struct coex_5g_afh_map {
	u32 wl_5g_ch;
	u8 bt_skip_ch;
	u8 bt_skip_span;
};

struct coex_rf_para {
	u8 wl_pwr_dec_lvl;
	u8 bt_pwr_dec_lvl;
	bool wl_low_gain_en;
	u8 bt_lna_lvl;
};

enum rtw_coex_bt_state_cnt {
	COEX_CNT_BT_RETRY,
	COEX_CNT_BT_REINIT,
	COEX_CNT_BT_REENABLE,
	COEX_CNT_BT_POPEVENT,
	COEX_CNT_BT_SETUPLINK,
	COEX_CNT_BT_IGNWLANACT,
	COEX_CNT_BT_INQ,
	COEX_CNT_BT_PAGE,
	COEX_CNT_BT_ROLESWITCH,
	COEX_CNT_BT_AFHUPDATE,
	COEX_CNT_BT_INFOUPDATE,
	COEX_CNT_BT_IQK,
	COEX_CNT_BT_IQKFAIL,

	COEX_CNT_BT_MAX
};

enum rtw_coex_wl_state_cnt {
	COEX_CNT_WL_SCANAP,
	COEX_CNT_WL_CONNPKT,
	COEX_CNT_WL_COEXRUN,
	COEX_CNT_WL_NOISY0,
	COEX_CNT_WL_NOISY1,
	COEX_CNT_WL_NOISY2,
	COEX_CNT_WL_5MS_NOEXTEND,
	COEX_CNT_WL_FW_NOTIFY,

	COEX_CNT_WL_MAX
};

struct rtw_coex_rfe {
	bool ant_switch_exist;
	bool ant_switch_diversity;
	bool ant_switch_with_bt;
	u8 rfe_module_type;
	u8 ant_switch_polarity;

	/* true if WLG at BTG, else at WLAG */
	bool wlg_at_btg;
};

#define COEX_WL_TDMA_PARA_LENGTH	5

struct rtw_coex_dm {
	bool cur_ps_tdma_on;
	bool cur_wl_rx_low_gain_en;
	bool ignore_wl_act;

	u8 reason;
	u8 bt_rssi_state[4];
	u8 wl_rssi_state[4];
	u8 wl_ch_info[3];
	u8 cur_ps_tdma;
	u8 cur_table;
	u8 ps_tdma_para[5];
	u8 cur_bt_pwr_lvl;
	u8 cur_bt_lna_lvl;
	u8 cur_wl_pwr_lvl;
	u8 bt_status;
	u32 cur_ant_pos_type;
	u32 cur_switch_status;
	u32 setting_tdma;
	u8 fw_tdma_para[COEX_WL_TDMA_PARA_LENGTH];
};

#define COEX_BTINFO_SRC_WL_FW	0x0
#define COEX_BTINFO_SRC_BT_RSP	0x1
#define COEX_BTINFO_SRC_BT_ACT	0x2
#define COEX_BTINFO_SRC_BT_IQK	0x3
#define COEX_BTINFO_SRC_BT_SCBD	0x4
#define COEX_BTINFO_SRC_H2C60	0x5
#define COEX_BTINFO_SRC_MAX	0x6

#define COEX_INFO_FTP		BIT(7)
#define COEX_INFO_A2DP		BIT(6)
#define COEX_INFO_HID		BIT(5)
#define COEX_INFO_SCO_BUSY	BIT(4)
#define COEX_INFO_ACL_BUSY	BIT(3)
#define COEX_INFO_INQ_PAGE	BIT(2)
#define COEX_INFO_SCO_ESCO	BIT(1)
#define COEX_INFO_CONNECTION	BIT(0)
#define COEX_BTINFO_LENGTH_MAX	10
#define COEX_BTINFO_LENGTH	7

#define COEX_BT_HIDINFO_LIST	0x0
#define COEX_BT_HIDINFO_A	0x1
#define COEX_BT_HIDINFO_NAME	3

#define COEX_BT_HIDINFO_LENGTH	6
#define COEX_BT_HIDINFO_HANDLE_NUM	4
#define COEX_BT_HIDINFO_C2H_HANDLE	0
#define COEX_BT_HIDINFO_C2H_VENDOR	1
#define COEX_BT_BLE_HANDLE_THRS	0x10
#define COEX_BT_HIDINFO_NOTCON	0xff

struct rtw_coex_hid {
	u8 hid_handle;
	u8 hid_vendor;
	u8 hid_name[COEX_BT_HIDINFO_NAME];
	bool hid_info_completed;
	bool is_game_hid;
};

struct rtw_coex_hid_handle_list {
	u8 cmd_id;
	u8 len;
	u8 subid;
	u8 handle_cnt;
	u8 handle[COEX_BT_HIDINFO_HANDLE_NUM];
} __packed;

struct rtw_coex_hid_info_a {
	u8 cmd_id;
	u8 len;
	u8 subid;
	u8 handle;
	u8 vendor;
	u8 name[COEX_BT_HIDINFO_NAME];
} __packed;

struct rtw_coex_stat {
	bool bt_disabled;
	bool bt_disabled_pre;
	bool bt_link_exist;
	bool bt_whck_test;
	bool bt_inq_page;
	bool bt_inq_remain;
	bool bt_inq;
	bool bt_page;
	bool bt_ble_voice;
	bool bt_ble_exist;
	bool bt_hfp_exist;
	bool bt_a2dp_exist;
	bool bt_hid_exist;
	bool bt_pan_exist; /* PAN or OPP */
	bool bt_opp_exist; /* OPP only */
	bool bt_acl_busy;
	bool bt_fix_2M;
	bool bt_setup_link;
	bool bt_multi_link;
	bool bt_multi_link_pre;
	bool bt_multi_link_remain;
	bool bt_a2dp_sink;
	bool bt_a2dp_active;
	bool bt_reenable;
	bool bt_ble_scan_en;
	bool bt_init_scan;
	bool bt_slave;
	bool bt_418_hid_exist;
	bool bt_ble_hid_exist;
	bool bt_game_hid_exist;
	bool bt_hid_handle_cnt;
	bool bt_mailbox_reply;
	bool bt_ctr_ok;

	bool wl_under_lps;
	bool wl_under_ips;
	bool wl_hi_pri_task1;
	bool wl_hi_pri_task2;
	bool wl_force_lps_ctrl;
	bool wl_gl_busy;
	bool wl_linkscan_proc;
	bool wl_ps_state_fail;
	bool wl_tx_limit_en;
	bool wl_ampdu_limit_en;
	bool wl_connected;
	bool wl_slot_extend;
	bool wl_cck_lock;
	bool wl_cck_lock_pre;
	bool wl_cck_lock_ever;
	bool wl_connecting;
	bool wl_slot_toggle;
	bool wl_slot_toggle_change; /* if toggle to no-toggle */
	bool wl_mimo_ps;

	u32 bt_supported_version;
	u32 bt_supported_feature;
	u32 hi_pri_tx;
	u32 hi_pri_rx;
	u32 lo_pri_tx;
	u32 lo_pri_rx;
	u32 patch_ver;
	u16 bt_reg_vendor_ae;
	u16 bt_reg_vendor_ac;
	s8 bt_rssi;
	u8 kt_ver;
	u8 gnt_workaround_state;
	u8 tdma_timer_base;
	u8 bt_profile_num;
	u8 bt_info_c2h[COEX_BTINFO_SRC_MAX][COEX_BTINFO_LENGTH_MAX];
	u8 bt_info_lb2;
	u8 bt_info_lb3;
	u8 bt_info_hb0;
	u8 bt_info_hb1;
	u8 bt_info_hb2;
	u8 bt_info_hb3;
	u8 bt_ble_scan_type;
	u8 bt_hid_pair_num;
	u8 bt_hid_slot;
	u8 bt_a2dp_bitpool;
	u8 bt_iqk_state;
	u8 bt_disable_cnt;

	u16 wl_beacon_interval;
	u8 wl_noisy_level;
	u8 wl_fw_dbg_info[10];
	u8 wl_fw_dbg_info_pre[10];
	u8 wl_rx_rate;
	u8 wl_tx_rate;
	u8 wl_rts_rx_rate;
	u8 wl_coex_mode;
	u8 wl_iot_peer;
	u8 ampdu_max_time;
	u8 wl_tput_dir;

	u8 wl_toggle_para[6];
	u8 wl_toggle_interval;

	u16 score_board;
	u16 retry_limit;

	/* counters to record bt states */
	u32 cnt_bt[COEX_CNT_BT_MAX];

	/* counters to record wifi states */
	u32 cnt_wl[COEX_CNT_WL_MAX];

	/* counters to record bt c2h data */
	u32 cnt_bt_info_c2h[COEX_BTINFO_SRC_MAX];

	u32 darfrc;
	u32 darfrch;

	struct rtw_coex_hid hid_info[COEX_BT_HIDINFO_HANDLE_NUM];
	struct rtw_coex_hid_handle_list hid_handle_list;
};

struct rtw_coex {



	bool under_5g;
	bool stop_dm;
	bool freeze;
	bool freerun;
	bool wl_rf_off;
	bool manual_control;

	struct rtw_coex_stat stat;
	struct rtw_coex_dm dm;
	struct rtw_coex_rfe rfe;

	struct delayed_work bt_relink_work;
	struct delayed_work bt_reenable_work;
	struct delayed_work defreeze_work;
	struct delayed_work wl_remain_work;
	struct delayed_work bt_remain_work;
	struct delayed_work wl_connecting_work;
	struct delayed_work bt_multi_link_remain_work;
	struct delayed_work wl_ccklock_work;

};

