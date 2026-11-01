/* SPDX-License-Identifier: GPL-2.0-only
 * Generated from tobyw121/Openwrt_RTL@85ef7eb7ccbd4336cd6e628a743ddc211333901c.
 * Vendor notices and primary algorithms preserved; checked host adapter only. */
enum channel_width {
 CHANNEL_WIDTH_20 = 0,
 CHANNEL_WIDTH_40 = 1,
 CHANNEL_WIDTH_80 = 2,
 CHANNEL_WIDTH_160 = 3,
 CHANNEL_WIDTH_80_80 = 4,
 CHANNEL_WIDTH_5 = 5,
 CHANNEL_WIDTH_10 = 6,
 CHANNEL_WIDTH_MAX = 7,
};
enum rf_type {
 RF_1T1R = 0,
 RF_1T2R = 1,
 RF_2T2R = 2,
 RF_2T3R = 3,
 RF_2T4R = 4,
 RF_3T3R = 5,
 RF_3T4R = 6,
 RF_4T4R = 7,
 RF_TYPE_MAX,
};
enum bb_path {
 BB_PATH_NON = 0,
 BB_PATH_A = 0x00000001,
 BB_PATH_B = 0x00000002,
 BB_PATH_C = 0x00000004,
 BB_PATH_D = 0x00000008,
 BB_PATH_AB = (BB_PATH_A | BB_PATH_B),
 BB_PATH_AC = (BB_PATH_A | BB_PATH_C),
 BB_PATH_AD = (BB_PATH_A | BB_PATH_D),
 BB_PATH_BC = (BB_PATH_B | BB_PATH_C),
 BB_PATH_BD = (BB_PATH_B | BB_PATH_D),
 BB_PATH_CD = (BB_PATH_C | BB_PATH_D),
 BB_PATH_ABC = (BB_PATH_A | BB_PATH_B | BB_PATH_C),
 BB_PATH_ABD = (BB_PATH_A | BB_PATH_B | BB_PATH_D),
 BB_PATH_ACD = (BB_PATH_A | BB_PATH_C | BB_PATH_D),
 BB_PATH_BCD = (BB_PATH_B | BB_PATH_C | BB_PATH_D),
 BB_PATH_ABCD = (BB_PATH_A | BB_PATH_B | BB_PATH_C | BB_PATH_D),
 BB_PATH_AUTO = 0xff
};
enum rf_path {
 RF_PATH_A = 0,
 RF_PATH_B = 1,
 RF_PATH_C = 2,
 RF_PATH_D = 3,
 RF_PATH_AB,
 RF_PATH_AC,
 RF_PATH_AD,
 RF_PATH_BC,
 RF_PATH_BD,
 RF_PATH_CD,
 RF_PATH_ABC,
 RF_PATH_ABD,
 RF_PATH_ACD,
 RF_PATH_BCD,
 RF_PATH_ABCD,
};
enum rf_syn {
 RF_SYN0 = 0,
 RF_SYN1 = 1,
};
enum rfc_mode {
 rfc_4x4 = 0,
 rfc_2x2 = 1,
};
enum wireless_set {
 WIRELESS_CCK = 0x00000001,
 WIRELESS_OFDM = 0x00000002,
 WIRELESS_HT = 0x00000004,
 WIRELESS_VHT = 0x00000008,
};
struct rssi_info {
 s8 rssi;
 s8 rssi_cck;
 s8 rssi_ofdm;
 u8 packet_map;
 u8 ofdm_pkt_cnt;
 u8 cck_pkt_cnt;
 u16 cck_sum_power;
 u8 is_send_rssi;
 u8 valid_bit;
 s16 rssi_acc;
};
struct ra_sta_info {
 u8 rate_id;
 u8 rssi_level;
 u8 is_first_connect:1;
 u8 is_support_sgi:1;
 u8 is_vht_enable:2;
 u8 disable_ra:1;
 u8 disable_pt:1;
 u8 txrx_state:2;
 u8 is_noisy:1;
 u8 curr_tx_rate;
 enum channel_width ra_bw_mode;
 enum channel_width curr_tx_bw;
 u8 curr_retry_ratio;
 u64 ramask;
};
struct dtp_info {
 u8 dyn_tx_power;
 u8 last_tx_power;
 u8 sta_tx_high_power_lvl:4;
 u8 sta_last_dtp_lvl:4;
};
struct cmn_sta_info {
 u16 dm_ctrl;
 enum channel_width bw_mode;
 u8 mac_id;
 u8 mac_addr[6];
 u16 aid;
 enum rf_type mimo_type;
 struct rssi_info rssi_stat;
 struct ra_sta_info ra_info;
 u16 tx_moving_average_tp;
 u16 rx_moving_average_tp;
 u16 tx_current_tp;
 u16 rx_current_tp;
 u8 stbc_en:2;
 u8 ldpc_en:2;
 enum wireless_set support_wireless_set;
 u8 sm_ps:2;
 struct dtp_info dtp_stat;
 u8 iot_peer;
};
struct phydm_phyinfo_struct {
 bool physts_rpt_valid;
 u8 rx_pwdb_all;
 u8 signal_quality;
 u8 rx_mimo_signal_strength[4];
 s8 rx_mimo_signal_quality[4];
 u8 rx_mimo_evm_dbm[4];
 s16 cfo_short[4];
 s16 cfo_tail[4];
 s8 rx_power;
 s8 recv_signal_power;
 u8 bt_rx_rssi_percentage;
 u8 signal_strength;
 s8 rx_pwr[4];
 s8 rx_snr[4];
 u8 ant_idx[4];
 u8 rx_count:2;
 u8 band_width:2;
 u8 rxsc:4;
 u8 channel;
 u8 is_mu_packet:1;
 u8 is_beamformed:1;
 u8 cnt_pw2cca;
 u8 cnt_cca2agc_rdy;
};
struct phydm_perpkt_info_struct {
 u8 data_rate;
 u8 station_id;
 u8 is_cck_rate: 1;
 u8 rate_ss:3;
 u8 is_packet_match_bssid:1;
 u8 is_packet_to_self:1;
 u8 is_packet_beacon:1;
 u8 is_to_self:1;
 u8 ppdu_cnt;
};
enum phydm_h2c_cmd {
 PHYDM_H2C_RA_MASK = 0x40,
 PHYDM_H2C_TXBF = 0x41,
 ODM_H2C_RSSI_REPORT = 0x42,
 ODM_H2C_IQ_CALIBRATION = 0x45,
 PHYDM_RA_MASK_ABOVE_3SS = 0x46,
 ODM_H2C_RA_PARA_ADJUST = 0x47,
 PHYDM_H2C_DYNAMIC_TX_PATH = 0x48,
 PHYDM_H2C_FW_TRACE_EN = 0x49,
 ODM_H2C_WIFI_CALIBRATION = 0x6d,
 PHYDM_H2C_MU = 0x4a,
 PHYDM_H2C_FW_GENERAL_INIT = 0x4c,
 PHYDM_H2C_FW_CLM_MNTR = 0x4d,
 PHYDM_H2C_MCC = 0x4f,
 PHYDM_H2C_RESP_TX_PATH_CTRL = 0x50,
 PHYDM_H2C_RESP_TX_ANT_CTRL = 0x51,
 PHYDM_H2C_FW_DM_CTRL = 0x55,
 ODM_MAX_H2CCMD
};
enum phydm_c2h_evt {
 PHYDM_C2H_DBG = 0,
 PHYDM_C2H_LB = 1,
 PHYDM_C2H_XBF = 2,
 PHYDM_C2H_TX_REPORT = 3,
 PHYDM_C2H_INFO = 9,
 PHYDM_C2H_BT_MP = 11,
 PHYDM_C2H_RA_RPT = 12,
 PHYDM_C2H_RA_PARA_RPT = 14,
 PHYDM_C2H_DYNAMIC_TX_PATH_RPT = 15,
 PHYDM_C2H_IQK_FINISH = 17,
 PHYDM_C2H_CLM_MONITOR = 0x2a,
 PHYDM_C2H_DBG_CODE = 0xFE,
 PHYDM_C2H_EXTEND = 0xFF,
};
enum phydm_extend_c2h_evt {
 PHYDM_EXTEND_C2H_DBG_PRINT = 0
};
enum phydm_halmac_param {
 PHYDM_HALMAC_CMD_MAC_W8 = 0,
 PHYDM_HALMAC_CMD_MAC_W16 = 1,
 PHYDM_HALMAC_CMD_MAC_W32 = 2,
 PHYDM_HALMAC_CMD_BB_W8,
 PHYDM_HALMAC_CMD_BB_W16,
 PHYDM_HALMAC_CMD_BB_W32,
 PHYDM_HALMAC_CMD_RF_W,
 PHYDM_HALMAC_CMD_DELAY_US,
 PHYDM_HALMAC_CMD_DELAY_MS,
 PHYDM_HALMAC_CMD_END = 0XFF,
};
u8 odm_read_1byte(struct dm_struct *dm, u32 reg_addr);
u16 odm_read_2byte(struct dm_struct *dm, u32 reg_addr);
u32 odm_read_4byte(struct dm_struct *dm, u32 reg_addr);
void odm_write_1byte(struct dm_struct *dm, u32 reg_addr, u8 data);
void odm_write_2byte(struct dm_struct *dm, u32 reg_addr, u16 data);
void odm_write_4byte(struct dm_struct *dm, u32 reg_addr, u32 data);
void odm_set_mac_reg(struct dm_struct *dm, u32 reg_addr, u32 bit_mask,
       u32 data);
u32 odm_get_mac_reg(struct dm_struct *dm, u32 reg_addr, u32 bit_mask);
void odm_set_bb_reg(struct dm_struct *dm, u32 reg_addr, u32 bit_mask, u32 data);
u32 odm_get_bb_reg(struct dm_struct *dm, u32 reg_addr, u32 bit_mask);
void odm_set_rf_reg(struct dm_struct *dm, u8 e_rf_path, u32 reg_addr,
      u32 bit_mask, u32 data);
u32 odm_get_rf_reg(struct dm_struct *dm, u8 e_rf_path, u32 reg_addr,
     u32 bit_mask);
void odm_allocate_memory(struct dm_struct *dm, void **ptr, u32 length);
void odm_free_memory(struct dm_struct *dm, void *ptr, u32 length);
void odm_move_memory(struct dm_struct *dm, void *dest, void *src, u32 length);
s32 odm_compare_memory(struct dm_struct *dm, void *buf1, void *buf2,
         u32 length);
void odm_memory_set(struct dm_struct *dm, void *pbuf, s8 value, u32 length);
void odm_acquire_spin_lock(struct dm_struct *dm, enum rt_spinlock_type type);
void odm_release_spin_lock(struct dm_struct *dm, enum rt_spinlock_type type);
void ODM_delay_ms(u32 ms);
void ODM_delay_us(u32 us);
void ODM_sleep_ms(u32 ms);
void ODM_sleep_us(u32 us);
void odm_set_timer(struct dm_struct *dm, struct phydm_timer_list *timer,
     u32 ms_delay);
void odm_initialize_timer(struct dm_struct *dm, struct phydm_timer_list *timer,
     void *call_back_func, void *context,
     const char *sz_id);
void odm_cancel_timer(struct dm_struct *dm, struct phydm_timer_list *timer);
void odm_release_timer(struct dm_struct *dm, struct phydm_timer_list *timer);
enum hal_status
phydm_set_reg_by_fw(struct dm_struct *dm, enum phydm_halmac_param config_type,
      u32 offset, u32 data, u32 mask, enum rf_path e_rf_path,
      u32 delay_time);
void odm_fill_h2c_cmd(struct dm_struct *dm, u8 element_id, u32 cmd_len,
        u8 *cmd_buffer);
u8 phydm_c2H_content_parsing(void *dm_void, u8 c2h_cmd_id, u8 c2h_cmd_len,
        u8 *tmp_buf);
u64 odm_get_current_time(struct dm_struct *dm);
u64 odm_get_progressing_time(struct dm_struct *dm, u64 start_time);
void phydm_set_hw_reg_handler_interface(struct dm_struct *dm, u8 reg_Name,
     u8 *val);
void phydm_get_hal_def_var_handler_interface(struct dm_struct *dm,
          enum _HAL_DEF_VARIABLE e_variable,
          void *value);
void odm_set_tx_power_index_by_rate_section(struct dm_struct *dm,
         enum rf_path path, u8 channel,
         u8 rate_section);
u8 odm_get_tx_power_index(struct dm_struct *dm, enum rf_path path, u8 tx_rate,
     u8 band_width, u8 channel);
u8 odm_efuse_one_byte_read(struct dm_struct *dm, u16 addr, u8 *data,
      boolean b_pseu_do_test);
void odm_efuse_logical_map_read(struct dm_struct *dm, u8 type, u16 offset,
    u32 *data);
enum hal_status
odm_iq_calibrate_by_fw(struct dm_struct *dm, u8 clear, u8 segment);
enum hal_status
odm_dpk_by_fw(struct dm_struct *dm);
void phydm_cmn_sta_info_hook(struct dm_struct *dm, u8 index,
        struct cmn_sta_info *pcmn_sta_info);
void phydm_macid2sta_idx_table(struct dm_struct *dm, u8 entry_idx,
          struct cmn_sta_info *pcmn_sta_info);
void phydm_add_interrupt_mask_handler(struct dm_struct *dm, u8 interrupt_type);
void phydm_enable_rx_related_interrupt_handler(struct dm_struct *dm);
void phydm_iqk_wait(struct dm_struct *dm, u32 timeout);
u8 phydm_get_hwrate_to_mrate(struct dm_struct *dm, u8 rate);
void phydm_set_crystalcap(struct dm_struct *dm, u8 crystal_cap);
void phydm_run_in_thread_cmd(struct dm_struct *dm, void (*func)(void *),
        void *context);
u8 phydm_get_tx_rate(struct dm_struct *dm);
u8 phydm_get_tx_power_dbm(struct dm_struct *dm, u8 rf_path,
     u8 rate, u8 bandwidth, u8 channel);
s16 phydm_get_tx_power_mdbm(struct dm_struct *dm, u8 rf_path,
     u8 rate, u8 bandwidth, u8 channel);
u64 phydm_division64(u64 x, u64 y);
enum hal_status {
 HAL_STATUS_SUCCESS,
 HAL_STATUS_FAILURE,
};
enum rt_spinlock_type {
 RT_TX_SPINLOCK = 1,
 RT_RX_SPINLOCK = 2,
 RT_RM_SPINLOCK = 3,
 RT_CAM_SPINLOCK = 4,
 RT_SCAN_SPINLOCK = 5,
 RT_LOG_SPINLOCK = 7,
 RT_BW_SPINLOCK = 8,
 RT_CHNLOP_SPINLOCK = 9,
 RT_RF_OPERATE_SPINLOCK = 10,
 RT_INITIAL_SPINLOCK = 11,
 RT_RF_STATE_SPINLOCK = 12,
 RT_PORT_SPINLOCK = 16,
 RT_VNIC_SPINLOCK = 17,
 RT_HVL_SPINLOCK = 18,
 RT_H2C_SPINLOCK = 20,
 rt_bt_data_spinlock = 25,
 RT_WAPI_OPTION_SPINLOCK = 26,
 RT_WAPI_RX_SPINLOCK = 27,
 RT_CCK_PAGEA_SPINLOCK = 28,
 RT_BUFFER_SPINLOCK = 29,
 RT_CHANNEL_AND_BANDWIDTH_SPINLOCK = 30,
 RT_GEN_TEMP_BUF_SPINLOCK = 31,
 RT_AWB_SPINLOCK = 32,
 RT_FW_PS_SPINLOCK = 33,
 RT_HW_TIMER_SPIN_LOCK = 34,
 RT_MPT_WI_SPINLOCK = 35,
 RT_P2P_SPIN_LOCK = 36,
 RT_DBG_SPIN_LOCK = 37,
 RT_IQK_SPINLOCK = 38,
 RT_PENDED_OID_SPINLOCK = 39,
 RT_CHNLLIST_SPINLOCK = 40,
 RT_INDIC_SPINLOCK = 41,
 RT_RFD_SPINLOCK = 42,
 RT_SYNC_IO_CNT_SPINLOCK = 43,
 RT_LAST_SPINLOCK,
};
struct _halrf_psd_data {
 u32 point;
 u32 start_point;
 u32 stop_point;
 u32 average;
 u32 buf_size;
 u32 psd_data[256];
 u32 psd_progress;
};
u32
halrf_psd_init(
 void *dm_void);
void
_halrf_iqk_psd_init_8723f(
 void *dm_void,
 bool onoff);
u64
halrf_get_iqk_psd_data(
 void *dm_void,
 u32 point);
u32
halrf_psd_query(
 void *dm_void,
 u32 *outbuf,
 u32 buf_size);
u32
halrf_psd_init_query(
 void *dm_void,
 u32 *outbuf,
 u32 point,
 u32 start_point,
 u32 stop_point,
 u32 average,
 u32 buf_size);
enum PDM_RATE_TYPE {
 PDM_1SS = 1,
 PDM_2SS = 2,
 PDM_3SS = 3,
 PDM_4SS = 4,
 PDM_CCK = 11,
 PDM_OFDM = 12
};
enum ODM_MGN_RATE {
 ODM_MGN_1M = 0x02,
 ODM_MGN_2M = 0x04,
 ODM_MGN_5_5M = 0x0B,
 ODM_MGN_6M = 0x0C,
 ODM_MGN_9M = 0x12,
 ODM_MGN_11M = 0x16,
 ODM_MGN_12M = 0x18,
 ODM_MGN_18M = 0x24,
 ODM_MGN_24M = 0x30,
 ODM_MGN_36M = 0x48,
 ODM_MGN_48M = 0x60,
 ODM_MGN_54M = 0x6C,
 ODM_MGN_MCS32 = 0x7F,
 ODM_MGN_MCS0 = 0x80,
 ODM_MGN_MCS1,
 ODM_MGN_MCS2,
 ODM_MGN_MCS3,
 ODM_MGN_MCS4,
 ODM_MGN_MCS5,
 ODM_MGN_MCS6,
 ODM_MGN_MCS7 = 0x87,
 ODM_MGN_MCS8,
 ODM_MGN_MCS9,
 ODM_MGN_MCS10,
 ODM_MGN_MCS11,
 ODM_MGN_MCS12,
 ODM_MGN_MCS13,
 ODM_MGN_MCS14,
 ODM_MGN_MCS15,
 ODM_MGN_MCS16 = 0x90,
 ODM_MGN_MCS17,
 ODM_MGN_MCS18,
 ODM_MGN_MCS19,
 ODM_MGN_MCS20,
 ODM_MGN_MCS21,
 ODM_MGN_MCS22,
 ODM_MGN_MCS23,
 ODM_MGN_MCS24 = 0x98,
 ODM_MGN_MCS25,
 ODM_MGN_MCS26,
 ODM_MGN_MCS27,
 ODM_MGN_MCS28,
 ODM_MGN_MCS29,
 ODM_MGN_MCS30,
 ODM_MGN_MCS31,
 ODM_MGN_VHT1SS_MCS0 = 0xa0,
 ODM_MGN_VHT1SS_MCS1,
 ODM_MGN_VHT1SS_MCS2,
 ODM_MGN_VHT1SS_MCS3,
 ODM_MGN_VHT1SS_MCS4,
 ODM_MGN_VHT1SS_MCS5,
 ODM_MGN_VHT1SS_MCS6,
 ODM_MGN_VHT1SS_MCS7,
 ODM_MGN_VHT1SS_MCS8,
 ODM_MGN_VHT1SS_MCS9,
 ODM_MGN_VHT2SS_MCS0 = 0xaa,
 ODM_MGN_VHT2SS_MCS1 = 0xab,
 ODM_MGN_VHT2SS_MCS2,
 ODM_MGN_VHT2SS_MCS3,
 ODM_MGN_VHT2SS_MCS4,
 ODM_MGN_VHT2SS_MCS5 = 0xaf,
 ODM_MGN_VHT2SS_MCS6 = 0xb0,
 ODM_MGN_VHT2SS_MCS7,
 ODM_MGN_VHT2SS_MCS8,
 ODM_MGN_VHT2SS_MCS9 = 0xb3,
 ODM_MGN_VHT3SS_MCS0 = 0xb4,
 ODM_MGN_VHT3SS_MCS1,
 ODM_MGN_VHT3SS_MCS2,
 ODM_MGN_VHT3SS_MCS3,
 ODM_MGN_VHT3SS_MCS4,
 ODM_MGN_VHT3SS_MCS5,
 ODM_MGN_VHT3SS_MCS6,
 ODM_MGN_VHT3SS_MCS7 = 0xbb,
 ODM_MGN_VHT3SS_MCS8 = 0xbc,
 ODM_MGN_VHT3SS_MCS9 = 0xbd,
 ODM_MGN_VHT4SS_MCS0 = 0xbe,
 ODM_MGN_VHT4SS_MCS1,
 ODM_MGN_VHT4SS_MCS2,
 ODM_MGN_VHT4SS_MCS3,
 ODM_MGN_VHT4SS_MCS4,
 ODM_MGN_VHT4SS_MCS5,
 ODM_MGN_VHT4SS_MCS6,
 ODM_MGN_VHT4SS_MCS7,
 ODM_MGN_VHT4SS_MCS8,
 ODM_MGN_VHT4SS_MCS9 = 0xc7,
 ODM_MGN_UNKNOWN
};
enum phydm_ctrl_info_rate {
 ODM_RATE1M = 0x00,
 ODM_RATE2M = 0x01,
 ODM_RATE5_5M = 0x02,
 ODM_RATE11M = 0x03,
 ODM_RATE6M = 0x04,
 ODM_RATE9M = 0x05,
 ODM_RATE12M = 0x06,
 ODM_RATE18M = 0x07,
 ODM_RATE24M = 0x08,
 ODM_RATE36M = 0x09,
 ODM_RATE48M = 0x0A,
 ODM_RATE54M = 0x0B,
 ODM_RATEMCS0 = 0x0C,
 ODM_RATEMCS1 = 0x0D,
 ODM_RATEMCS2 = 0x0E,
 ODM_RATEMCS3 = 0x0F,
 ODM_RATEMCS4 = 0x10,
 ODM_RATEMCS5 = 0x11,
 ODM_RATEMCS6 = 0x12,
 ODM_RATEMCS7 = 0x13,
 ODM_RATEMCS8 = 0x14,
 ODM_RATEMCS9 = 0x15,
 ODM_RATEMCS10 = 0x16,
 ODM_RATEMCS11 = 0x17,
 ODM_RATEMCS12 = 0x18,
 ODM_RATEMCS13 = 0x19,
 ODM_RATEMCS14 = 0x1A,
 ODM_RATEMCS15 = 0x1B,
 ODM_RATEMCS16 = 0x1C,
 ODM_RATEMCS17 = 0x1D,
 ODM_RATEMCS18 = 0x1E,
 ODM_RATEMCS19 = 0x1F,
 ODM_RATEMCS20 = 0x20,
 ODM_RATEMCS21 = 0x21,
 ODM_RATEMCS22 = 0x22,
 ODM_RATEMCS23 = 0x23,
 ODM_RATEMCS24 = 0x24,
 ODM_RATEMCS25 = 0x25,
 ODM_RATEMCS26 = 0x26,
 ODM_RATEMCS27 = 0x27,
 ODM_RATEMCS28 = 0x28,
 ODM_RATEMCS29 = 0x29,
 ODM_RATEMCS30 = 0x2A,
 ODM_RATEMCS31 = 0x2B,
 ODM_RATEVHTSS1MCS0 = 0x2C,
 ODM_RATEVHTSS1MCS1 = 0x2D,
 ODM_RATEVHTSS1MCS2 = 0x2E,
 ODM_RATEVHTSS1MCS3 = 0x2F,
 ODM_RATEVHTSS1MCS4 = 0x30,
 ODM_RATEVHTSS1MCS5 = 0x31,
 ODM_RATEVHTSS1MCS6 = 0x32,
 ODM_RATEVHTSS1MCS7 = 0x33,
 ODM_RATEVHTSS1MCS8 = 0x34,
 ODM_RATEVHTSS1MCS9 = 0x35,
 ODM_RATEVHTSS2MCS0 = 0x36,
 ODM_RATEVHTSS2MCS1 = 0x37,
 ODM_RATEVHTSS2MCS2 = 0x38,
 ODM_RATEVHTSS2MCS3 = 0x39,
 ODM_RATEVHTSS2MCS4 = 0x3A,
 ODM_RATEVHTSS2MCS5 = 0x3B,
 ODM_RATEVHTSS2MCS6 = 0x3C,
 ODM_RATEVHTSS2MCS7 = 0x3D,
 ODM_RATEVHTSS2MCS8 = 0x3E,
 ODM_RATEVHTSS2MCS9 = 0x3F,
 ODM_RATEVHTSS3MCS0 = 0x40,
 ODM_RATEVHTSS3MCS1 = 0x41,
 ODM_RATEVHTSS3MCS2 = 0x42,
 ODM_RATEVHTSS3MCS3 = 0x43,
 ODM_RATEVHTSS3MCS4 = 0x44,
 ODM_RATEVHTSS3MCS5 = 0x45,
 ODM_RATEVHTSS3MCS6 = 0x46,
 ODM_RATEVHTSS3MCS7 = 0x47,
 ODM_RATEVHTSS3MCS8 = 0x48,
 ODM_RATEVHTSS3MCS9 = 0x49,
 ODM_RATEVHTSS4MCS0 = 0x4A,
 ODM_RATEVHTSS4MCS1 = 0x4B,
 ODM_RATEVHTSS4MCS2 = 0x4C,
 ODM_RATEVHTSS4MCS3 = 0x4D,
 ODM_RATEVHTSS4MCS4 = 0x4E,
 ODM_RATEVHTSS4MCS5 = 0x4F,
 ODM_RATEVHTSS4MCS6 = 0x50,
 ODM_RATEVHTSS4MCS7 = 0x51,
 ODM_RATEVHTSS4MCS8 = 0x52,
 ODM_RATEVHTSS4MCS9 = 0x53,
};
enum phydm_legacy_spec_rate {
 PHYDM_SPEC_RATE_6M = 0xb,
 PHYDM_SPEC_RATE_9M = 0xf,
 PHYDM_SPEC_RATE_12M = 0xa,
 PHYDM_SPEC_RATE_18M = 0xe,
 PHYDM_SPEC_RATE_24M = 0x9,
 PHYDM_SPEC_RATE_36M = 0xd,
 PHYDM_SPEC_RATE_48M = 0x8,
 PHYDM_SPEC_RATE_54M = 0xc
};
enum odm_interface {
 ODM_ITRF_PCIE = 0x1,
 ODM_ITRF_USB = 0x2,
 ODM_ITRF_SDIO = 0x4,
 ODM_ITRF_ALL = 0x7,
};
enum phydm_api_host {
 RUN_IN_FW = 0,
 RUN_IN_DRIVER = 1,
};
enum phydm_ic {
 ODM_RTL8188E = BIT(0),
 ODM_RTL8812 = BIT(1),
 ODM_RTL8821 = BIT(2),
 ODM_RTL8192E = BIT(3),
 ODM_RTL8723B = BIT(4),
 ODM_RTL8814A = BIT(5),
 ODM_RTL8881A = BIT(6),
 ODM_RTL8822B = BIT(7),
 ODM_RTL8703B = BIT(8),
 ODM_RTL8195A = BIT(9),
 ODM_RTL8188F = BIT(10),
 ODM_RTL8723D = BIT(11),
 ODM_RTL8197F = BIT(12),
 ODM_RTL8821C = BIT(13),
 ODM_RTL8814B = BIT(14),
 ODM_RTL8198F = BIT(15),
 ODM_RTL8710B = BIT(16),
 ODM_RTL8192F = BIT(17),
 ODM_RTL8822C = BIT(18),
 ODM_RTL8195B = BIT(19),
 ODM_RTL8812F = BIT(20),
 ODM_RTL8197G = BIT(21),
 ODM_RTL8721D = BIT(22),
 ODM_RTL8710C = BIT(23),
 ODM_RTL8723F = BIT(24)
};
enum phydm_ic_ip {
 PHYDM_IC_N = 0,
 PHYDM_IC_AC = 1,
 PHYDM_IC_JGR3 = 2
};
enum phydm_phy_sts_type {
 PHYDM_PHYSTS_TYPE_1 = 1,
 PHYDM_PHYSTS_TYPE_2 = 2,
 PHYDM_PHYSTS_TYPE_3 = 3
};
enum odm_cut_version {
 ODM_CUT_A = 0,
 ODM_CUT_B = 1,
 ODM_CUT_C = 2,
 ODM_CUT_D = 3,
 ODM_CUT_E = 4,
 ODM_CUT_F = 5,
 ODM_CUT_G = 6,
 ODM_CUT_H = 7,
 ODM_CUT_I = 8,
 ODM_CUT_J = 9,
 ODM_CUT_K = 10,
 ODM_CUT_L = 11,
 ODM_CUT_M = 12,
 ODM_CUT_N = 13,
 ODM_CUT_O = 14,
 ODM_CUT_TEST = 15,
};
enum odm_fab {
 ODM_TSMC = 0,
 ODM_UMC = 1,
};
enum odm_operation_mode {
 ODM_NO_LINK = BIT(0),
 ODM_LINK = BIT(1),
 ODM_SCAN = BIT(2),
 ODM_POWERSAVE = BIT(3),
 ODM_AP_MODE = BIT(4),
 ODM_CLIENT_MODE = BIT(5),
 ODM_AD_HOC = BIT(6),
 ODM_WIFI_DIRECT = BIT(7),
 ODM_WIFI_DISPLAY = BIT(8),
};
enum odm_wireless_mode {
 ODM_WM_UNKNOW = 0x0,
 ODM_WM_B = BIT(0),
 ODM_WM_G = BIT(1),
 ODM_WM_A = BIT(2),
 ODM_WM_N24G = BIT(3),
 ODM_WM_N5G = BIT(4),
 ODM_WM_AUTO = BIT(5),
 ODM_WM_AC = BIT(6),
};
enum odm_band_type {
 ODM_BAND_2_4G = 0,
 ODM_BAND_5G,
 ODM_BAND_ON_BOTH,
 ODM_BANDMAX
};
enum odm_rf_band {
 ODM_RF_BAND_2G = 0,
 ODM_RF_BAND_5G_LOW = 1,
 ODM_RF_BAND_5G_MID = 2,
 ODM_RF_BAND_5G_HIGH = 3,
};
enum phydm_sec_chnl_offset {
 PHYDM_DONT_CARE = 0,
 PHYDM_BELOW = 1,
 PHYDM_ABOVE = 2
};
enum odm_security {
 ODM_SEC_OPEN = 0,
 ODM_SEC_WEP40 = 1,
 ODM_SEC_TKIP = 2,
 ODM_SEC_RESERVE = 3,
 ODM_SEC_AESCCMP = 4,
 ODM_SEC_WEP104 = 5,
 ODM_WEP_WPA_MIXED = 6,
 ODM_SEC_SMS4 = 7,
};
enum odm_board_type {
 ODM_BOARD_DEFAULT = 0,
 ODM_BOARD_MINICARD = BIT(0),
 ODM_BOARD_SLIM = BIT(1),
 ODM_BOARD_BT = BIT(2),
 ODM_BOARD_EXT_PA = BIT(3),
 ODM_BOARD_EXT_LNA = BIT(4),
 ODM_BOARD_EXT_TRSW = BIT(5),
 ODM_BOARD_EXT_PA_5G = BIT(6),
 ODM_BOARD_EXT_LNA_5G = BIT(7),
};
enum odm_package_type {
 ODM_PACKAGE_DEFAULT = 0,
 ODM_PACKAGE_QFN68 = BIT(0),
 ODM_PACKAGE_TFBGA90 = BIT(1),
 ODM_PACKAGE_TFBGA79 = BIT(2),
};
enum odm_type_gpa {
 TYPE_GPA0 = 0x0000,
 TYPE_GPA1 = 0x0055,
 TYPE_GPA2 = 0x00AA,
 TYPE_GPA3 = 0x00FF,
 TYPE_GPA4 = 0x5500,
 TYPE_GPA5 = 0x5555,
 TYPE_GPA6 = 0x55AA,
 TYPE_GPA7 = 0x55FF,
 TYPE_GPA8 = 0xAA00,
 TYPE_GPA9 = 0xAA55,
 TYPE_GPA10 = 0xAAAA,
 TYPE_GPA11 = 0xAAFF,
 TYPE_GPA12 = 0xFF00,
 TYPE_GPA13 = 0xFF55,
 TYPE_GPA14 = 0xFFAA,
 TYPE_GPA15 = 0xFFFF,
};
enum odm_type_apa {
 TYPE_APA0 = 0x0000,
 TYPE_APA1 = 0x0055,
 TYPE_APA2 = 0x00AA,
 TYPE_APA3 = 0x00FF,
 TYPE_APA4 = 0x5500,
 TYPE_APA5 = 0x5555,
 TYPE_APA6 = 0x55AA,
 TYPE_APA7 = 0x55FF,
 TYPE_APA8 = 0xAA00,
 TYPE_APA9 = 0xAA55,
 TYPE_APA10 = 0xAAAA,
 TYPE_APA11 = 0xAAFF,
 TYPE_APA12 = 0xFF00,
 TYPE_APA13 = 0xFF55,
 TYPE_APA14 = 0xFFAA,
 TYPE_APA15 = 0xFFFF,
};
enum odm_type_glna {
 TYPE_GLNA0 = 0x0000,
 TYPE_GLNA1 = 0x0055,
 TYPE_GLNA2 = 0x00AA,
 TYPE_GLNA3 = 0x00FF,
 TYPE_GLNA4 = 0x5500,
 TYPE_GLNA5 = 0x5555,
 TYPE_GLNA6 = 0x55AA,
 TYPE_GLNA7 = 0x55FF,
 TYPE_GLNA8 = 0xAA00,
 TYPE_GLNA9 = 0xAA55,
 TYPE_GLNA10 = 0xAAAA,
 TYPE_GLNA11 = 0xAAFF,
 TYPE_GLNA12 = 0xFF00,
 TYPE_GLNA13 = 0xFF55,
 TYPE_GLNA14 = 0xFFAA,
 TYPE_GLNA15 = 0xFFFF,
};
enum odm_type_alna {
 TYPE_ALNA0 = 0x0000,
 TYPE_ALNA1 = 0x0055,
 TYPE_ALNA2 = 0x00AA,
 TYPE_ALNA3 = 0x00FF,
 TYPE_ALNA4 = 0x5500,
 TYPE_ALNA5 = 0x5555,
 TYPE_ALNA6 = 0x55AA,
 TYPE_ALNA7 = 0x55FF,
 TYPE_ALNA8 = 0xAA00,
 TYPE_ALNA9 = 0xAA55,
 TYPE_ALNA10 = 0xAAAA,
 TYPE_ALNA11 = 0xAAFF,
 TYPE_ALNA12 = 0xFF00,
 TYPE_ALNA13 = 0xFF55,
 TYPE_ALNA14 = 0xFFAA,
 TYPE_ALNA15 = 0xFFFF,
};
enum odm_parameter_init {
 ODM_PRE_SETTING = 0,
 ODM_POST_SETTING = 1,
 ODM_INIT_FW_SETTING = 2,
 ODM_PRE_RF_SET = 3,
 ODM_POST_RF_SET = 4
};
enum phydm_pause_type {
 PHYDM_PAUSE = 1,
 PHYDM_PAUSE_NO_SET = 2,
 PHYDM_RESUME = 3
};
enum phydm_backup_type {
 PHYDM_BACKUP = 1,
 PHYDM_RESTORE = 2
};
enum phydm_pause_level {
 PHYDM_PAUSE_RELEASE = -1,
 PHYDM_PAUSE_LEVEL_0 = 0,
 PHYDM_PAUSE_LEVEL_1 = 1,
 PHYDM_PAUSE_LEVEL_2 = 2,
 PHYDM_PAUSE_LEVEL_3 = 3,
 PHYDM_PAUSE_MAX_NUM = 4
};
enum phydm_dis_hw_fun {
 HW_FUN_DIS = 0,
 HW_FUN_RESUME = 1
};
enum phystat_rpt {
 PHY_PWDB = 0,
 PHY_EVM = 1,
 PHY_CFO = 2,
 PHY_RXSNR = 3,
 PHY_LGAIN = 4,
 PHY_HT_AAGC_GAIN = 5,
};
struct phydm_api_stuc {
 u32 rxiqc_reg1;
 u32 rxiqc_reg2;
 u8 tx_queue_bitmap;
 u8 ccktx_path;
 u8 pri_ch_idx;
};
enum channel_width phydm_rxsc_2_bw(void *dm_void, u8 rxsc);
void phydm_reset_bb_hw_cnt(void *dm_void);
void phydm_dynamic_ant_weighting(void *dm_void);
void phydm_trx_antenna_setting_init(void *dm_void, u8 num_rf_path);
void phydm_config_ofdm_rx_path(void *dm_void, enum bb_path path);
void phydm_config_cck_rx_path(void *dm_void, enum bb_path path);
void phydm_config_cck_rx_antenna_init(void *dm_void);
void phydm_config_trx_path(void *dm_void, char input[][16], u32 *_used,
      char *output, u32 *_out_len);
void phydm_config_ofdm_tx_path(void *dm_void, enum bb_path path);
void phydm_config_cck_tx_path(void *dm_void, enum bb_path path);
void phydm_tx_2path(void *dm_void);
void phydm_stop_3_wire(void *dm_void, u8 set_type);
u8 phydm_stop_ic_trx(void *dm_void, u8 set_type);
void phydm_dis_cck_trx(void *dm_void, u8 set_type);
void phydm_bw_fixed_enable(void *dm_void, bool enable);
void phydm_bw_fixed_setting(void *dm_void);
void phydm_set_ext_switch(void *dm_void, u32 ext_ant_switch);
void phydm_nbi_enable(void *dm_void, u32 enable);
u8 phydm_csi_mask_setting(void *dm_void, u32 enable, u32 ch, u32 bw, u32 f_intf,
     u32 sec_ch);
u8 phydm_nbi_setting(void *dm_void, u32 enable, u32 ch, u32 bw, u32 f_intf,
       u32 sec_ch);
void phydm_nbi_debug(void *dm_void, char input[][16], u32 *_used,
       char *output, u32 *_out_len);
void phydm_csi_debug(void *dm_void, char input[][16], u32 *_used,
       char *output, u32 *_out_len);
void phydm_stop_ck320(void *dm_void, u8 enable);
bool
phydm_set_bb_txagc_offset(void *dm_void, s8 power_offset, u8 add_half_db);
u8 phydm_csi_mask_setting_jgr3(void *dm_void, u32 enable, u32 ch, u32 bw,
          u32 f_intf, u32 sec_ch, u8 wgt);
void phydm_set_csi_mask_jgr3(void *dm_void, u32 tone_idx_tmp, u8 tone_direction,
        u8 wgt);
u8 phydm_nbi_setting_jgr3(void *dm_void, u32 enable, u32 ch, u32 bw, u32 f_intf,
     u32 sec_ch, u8 path);
void phydm_set_nbi_reg_jgr3(void *dm_void, u32 tone_idx_tmp, u8 tone_direction,
       u8 path);
void phydm_nbi_enable_jgr3(void *dm_void, u32 enable, u8 path);
u8 phydm_phystat_rpt_jgr3(void *dm_void, enum phystat_rpt info,
     enum rf_path ant_path);
void phydm_user_position_for_sniffer(void *dm_void, u8 user_position);
void phydm_reset_txagc(void *dm_void);
bool
phydm_api_shift_txagc(void *dm_void, u32 pwr_offset, enum rf_path path,
        bool is_positive);
bool
phydm_api_set_txagc(void *dm_void, u32 power_index, enum rf_path path,
      u8 hw_rate, bool is_single_rate);
u8 phydm_api_get_txagc(void *dm_void, enum rf_path path, u8 hw_rate);
bool
phydm_api_switch_bw_channel(void *dm_void, u8 central_ch, u8 primary_ch_idx,
       enum channel_width bandwidth);
bool
phydm_api_trx_mode(void *dm_void, enum bb_path tx_path, enum bb_path rx_path,
     enum bb_path tx_path_ctrl);
enum phydm_dig_mode {
 PHYDM_DIG_PERFORAMNCE_MODE = 0,
 PHYDM_DIG_COVERAGE_MODE = 1,
};
enum phydm_dig_trend {
 DIG_STABLE = 0,
 DIG_INCREASING = 1,
 DIG_DECREASING = 2
};
struct phydm_mcc_dig {
 u8 mcc_rssi_A;
 u8 mcc_rssi_B;
};
struct phydm_dig_struct {
 bool is_dbg_fa_th;
 u8 cur_ig_value;
 bool igi_dyn_up_hit;
 u8 igi_trend;
 u32 rvrt_val;
 u8 igi_backup;
 u8 rx_gain_range_max;
 u8 rx_gain_range_min;
 u8 dm_dig_max;
 u8 dm_dig_min;
 u8 dig_max_of_min;
 u32 ant_div_rssi_max;
 u8 *is_p2p_in_process;
 u16 fa_th[3];
 u8 rf_gain_idx;
 u8 agc_table_idx;
 u8 big_jump_lmt[16];
 u8 enable_adjust_big_jump:1;
 u8 big_jump_step1:3;
 u8 big_jump_step2:2;
 u8 big_jump_step3:2;
 u8 upcheck_init_val;
 u8 lv0_ratio_reciprocal;
 u8 lv1_ratio_reciprocal;
};
struct phydm_fa_struct {
 u32 cnt_parity_fail;
 u32 cnt_rate_illegal;
 u32 cnt_crc8_fail;
 u32 cnt_crc8_fail_vhta;
 u32 cnt_crc8_fail_vhtb;
 u32 cnt_mcs_fail;
 u32 cnt_mcs_fail_vht;
 u32 cnt_ofdm_fail;
 u32 cnt_ofdm_fail_pre;
 u32 cnt_cck_fail;
 u32 cnt_all;
 u32 cnt_all_accumulated;
 u32 cnt_all_pre;
 u32 cnt_fast_fsync;
 u32 cnt_sb_search_fail;
 u32 cnt_ofdm_cca;
 u32 cnt_cck_cca;
 u32 cnt_cca_all;
 u32 cnt_bw_usc;
 u32 cnt_bw_lsc;
 u32 cnt_cck_crc32_error;
 u32 cnt_cck_crc32_ok;
 u32 cnt_ofdm_crc32_error;
 u32 cnt_ofdm_crc32_ok;
 u32 cnt_ht_crc32_error;
 u32 cnt_ht_crc32_ok;
 u32 cnt_ht_crc32_error_agg;
 u32 cnt_ht_crc32_ok_agg;
 u32 cnt_vht_crc32_error;
 u32 cnt_vht_crc32_ok;
 u32 cnt_crc32_error_all;
 u32 cnt_crc32_ok_all;
 u32 time_fa_all;
 bool cck_block_enable;
 bool ofdm_block_enable;
 u32 dbg_port0;
 bool edcca_flag;
 u8 ofdm2_rate_idx;
 u32 cnt_ofdm2_crc32_error;
 u32 cnt_ofdm2_crc32_ok;
 u8 ofdm2_pcr;
 u8 ht2_rate_idx;
 u32 cnt_ht2_crc32_error;
 u32 cnt_ht2_crc32_ok;
 u8 ht2_pcr;
 u8 vht2_rate_idx;
 u32 cnt_vht2_crc32_error;
 u32 cnt_vht2_crc32_ok;
 u8 vht2_pcr;
};
void phydm_write_dig_reg(void *dm_void, u8 igi);
void odm_write_dig(void *dm_void, u8 current_igi);
u8 phydm_get_igi(void *dm_void, enum bb_path path);
void phydm_set_dig_val(void *dm_void, u32 *val_buf, u8 val_len);
void odm_pause_dig(void *dm_void, enum phydm_pause_type pause_type,
     enum phydm_pause_level pause_level, u8 igi_value);
void phydm_dig_init(void *dm_void);
void phydm_dig(void *dm_void);
void phydm_dig_lps_32k(void *dm_void);
void phydm_dig_by_rssi_lps(void *dm_void);
void phydm_false_alarm_counter_statistics(void *dm_void);
void phydm_set_ofdm_agc_tab(void *dm_void, u8 tab_sel);
void phydm_dig_debug(void *dm_void, char input[][16], u32 *_used, char *output,
       u32 *_out_len);
void phydm_crc32_cnt_dbg(void *dm_void, char input[][16], u32 *_used,
    char *output, u32 *_out_len);
extern const u16 phy_rate_table[84];
struct phydm_bb_ram_per_sta {
 bool hw_igi_en;
 bool tx_pwr_offset0_en;
 bool tx_pwr_offset1_en;
 u8 macid_addr;
 u8 hw_igi;
 s8 tx_pwr_offset0;
 s8 tx_pwr_offset1;
};
struct phydm_bb_ram_ctrl {
 struct phydm_bb_ram_per_sta pram_sta_ctrl[MACID_NUM_SW_LIMIT];
 bool tx_pwr_ofst_reg0_en;
 u8 tx_pwr_ofst_reg0;
 bool tx_pwr_ofst_reg1_en;
 u8 tx_pwr_ofst_reg1;
};
struct phydm_phystatus_statistic {
 u32 rssi_cck_sum;
 u32 rssi_cck_cnt;
 u32 rssi_beacon_sum[4];
 u32 rssi_beacon_cnt;
 u32 rssi_cck_sum_abv_2ss[4 - 1];
 u32 rssi_ofdm_sum[4];
 u32 rssi_ofdm_cnt;
 u32 evm_ofdm_sum;
 u32 snr_ofdm_sum[4];
 u16 evm_ofdm_hist[12];
 u16 snr_ofdm_hist[12];
 u32 rssi_1ss_cnt;
 u32 rssi_1ss_sum[4];
 u32 evm_1ss_sum;
 u32 snr_1ss_sum[4];
 u16 evm_1ss_hist[12];
 u16 snr_1ss_hist[12];
 u32 rssi_2ss_cnt;
 u32 rssi_2ss_sum[4];
 u32 evm_2ss_sum[2];
 u32 snr_2ss_sum[4];
 u16 evm_2ss_hist[2][12];
 u16 snr_2ss_hist[2][12];
 u32 rssi_3ss_cnt;
 u32 rssi_3ss_sum[4];
 u32 evm_3ss_sum[3];
 u32 snr_3ss_sum[4];
 u16 evm_3ss_hist[3][12];
 u16 snr_3ss_hist[3][12];
 u32 rssi_4ss_cnt;
 u32 rssi_4ss_sum[4];
 u32 evm_4ss_sum[4];
 u32 snr_4ss_sum[4];
 u16 evm_4ss_hist[4][12];
 u16 snr_4ss_hist[4][12];
};
struct phydm_phystatus_avg {
 u8 rssi_cck_avg;
 u8 rssi_beacon_avg[4];
 u8 rssi_cck_avg_abv_2ss[4 - 1];
 u8 rssi_ofdm_avg[4];
 u8 evm_ofdm_avg;
 u8 snr_ofdm_avg[4];
 u8 rssi_1ss_avg[4];
 u8 evm_1ss_avg;
 u8 snr_1ss_avg[4];
 u8 rssi_2ss_avg[4];
 u8 evm_2ss_avg[2];
 u8 snr_2ss_avg[4];
 u8 rssi_3ss_avg[4];
 u8 evm_3ss_avg[3];
 u8 snr_3ss_avg[4];
 u8 rssi_4ss_avg[4];
 u8 evm_4ss_avg[4];
 u8 snr_4ss_avg[4];
};
struct odm_phy_dbg_info {
 u32 num_qry_phy_status_cck;
 u32 num_qry_phy_status_ofdm;
 u32 num_qry_mu_pkt;
 u32 num_qry_bf_pkt;
 u16 num_mu_vht_pkt[40];
 bool is_ldpc_pkt;
 bool is_stbc_pkt;
 u8 num_of_ppdu[4];
 u8 gid_num[4];
 u32 condi_num;
 u8 condi_num_cdf[10];
 u8 num_qry_beacon_pkt;
 u8 beacon_cnt_in_period;
 u8 beacon_phy_rate;
 u8 show_phy_sts_all_pkt;
 u16 show_phy_sts_max_cnt;
 u16 show_phy_sts_cnt;
 u16 num_qry_legacy_pkt[12];
 u16 num_qry_ht_pkt[32];
 u16 num_qry_pkt_sc_20m[40];
 bool ht_pkt_not_zero;
 bool low_bw_20_occur;
 u16 num_qry_vht_pkt[40];
 u16 num_qry_pkt_sc_40m[40];
 bool vht_pkt_not_zero;
 bool low_bw_40_occur;
 u16 snr_hist_th[(12 - 1)];
 u16 evm_hist_th[(12 - 1)];
 u16 cn_hist_th[(12 - 1)];
 u8 condition_num_seg0;
 u8 eigen_val[4];
 s16 cfo_tail[4];
 struct phydm_phystatus_statistic physts_statistic_info;
 struct phydm_phystatus_avg phystatus_statistic_avg;
};
enum odm_cmninfo {
 ODM_CMNINFO_PLATFORM = 0,
 ODM_CMNINFO_ABILITY,
 ODM_CMNINFO_INTERFACE,
 ODM_CMNINFO_MP_TEST_CHIP,
 ODM_CMNINFO_IC_TYPE,
 ODM_CMNINFO_CUT_VER,
 ODM_CMNINFO_FAB_VER,
 ODM_CMNINFO_FW_VER,
 ODM_CMNINFO_FW_SUB_VER,
 ODM_CMNINFO_RF_TYPE,
 ODM_CMNINFO_RFE_TYPE,
 ODM_CMNINFO_DPK_EN,
 ODM_CMNINFO_BOARD_TYPE,
 ODM_CMNINFO_PACKAGE_TYPE,
 ODM_CMNINFO_EXT_LNA,
 ODM_CMNINFO_5G_EXT_LNA,
 ODM_CMNINFO_EXT_PA,
 ODM_CMNINFO_5G_EXT_PA,
 ODM_CMNINFO_GPA,
 ODM_CMNINFO_APA,
 ODM_CMNINFO_GLNA,
 ODM_CMNINFO_ALNA,
 ODM_CMNINFO_TDMA,
 ODM_CMNINFO_EXT_TRSW,
 ODM_CMNINFO_EXT_LNA_GAIN,
 ODM_CMNINFO_PATCH_ID,
 ODM_CMNINFO_BINHCT_TEST,
 ODM_CMNINFO_BWIFI_TEST,
 ODM_CMNINFO_SMART_CONCURRENT,
 ODM_CMNINFO_CONFIG_BB_RF,
 ODM_CMNINFO_IQKPAOFF,
 ODM_CMNINFO_HUBUSBMODE,
 ODM_CMNINFO_FWDWRSVDPAGEINPROGRESS,
 ODM_CMNINFO_TX_TP,
 ODM_CMNINFO_RX_TP,
 ODM_CMNINFO_SOUNDING_SEQ,
 ODM_CMNINFO_REGRFKFREEENABLE,
 ODM_CMNINFO_RFKFREEENABLE,
 ODM_CMNINFO_NORMAL_RX_PATH_CHANGE,
 ODM_CMNINFO_VALID_PATH_SET,
 ODM_CMNINFO_EFUSE0X3D8,
 ODM_CMNINFO_EFUSE0X3D7,
 ODM_CMNINFO_SOFT_AP_SPECIAL_SETTING,
 ODM_CMNINFO_X_CAP_SETTING,
 ODM_CMNINFO_ADVANCE_OTA,
 ODM_CMNINFO_HP_HWID,
 ODM_CMNINFO_TSSI_ENABLE,
 ODM_CMNINFO_DIS_DPD,
 ODM_CMNINFO_POWER_VOLTAGE,
 ODM_CMNINFO_ANTDIV_GPIO,
 ODM_CMNINFO_EN_AUTO_BW_TH,
 ODM_CMNINFO_TX_UNI,
 ODM_CMNINFO_RX_UNI,
 ODM_CMNINFO_BAND,
 ODM_CMNINFO_SEC_CHNL_OFFSET,
 ODM_CMNINFO_SEC_MODE,
 ODM_CMNINFO_BW,
 ODM_CMNINFO_CHNL,
 ODM_CMNINFO_FORCED_RATE,
 ODM_CMNINFO_ANT_DIV,
 ODM_CMNINFO_PATH_DIV,
 ODM_CMNINFO_ADAPTIVE_SOML,
 ODM_CMNINFO_ADAPTIVITY,
 ODM_CMNINFO_SCAN,
 ODM_CMNINFO_POWER_SAVING,
 ODM_CMNINFO_ONE_PATH_CCA,
 ODM_CMNINFO_DRV_STOP,
 ODM_CMNINFO_PNP_IN,
 ODM_CMNINFO_INIT_ON,
 ODM_CMNINFO_ANT_TEST,
 ODM_CMNINFO_NET_CLOSED,
 ODM_CMNINFO_P2P_LINK,
 ODM_CMNINFO_FCS_MODE,
 ODM_CMNINFO_IS1ANTENNA,
 ODM_CMNINFO_RFDEFAULTPATH,
 ODM_CMNINFO_DFS_MASTER_ENABLE,
 ODM_CMNINFO_FORCE_TX_ANT_BY_TXDESC,
 ODM_CMNINFO_SET_S0S1_DEFAULT_ANTENNA,
 ODM_CMNINFO_SOFT_AP_MODE,
 ODM_CMNINFO_MP_MODE,
 ODM_CMNINFO_INTERRUPT_MASK,
 ODM_CMNINFO_BB_OPERATION_MODE,
 ODM_CMNINFO_BF_ANTDIV_DECISION,
 ODM_CMNINFO_MANUAL_SUPPORTABILITY,
 ODM_CMNINFO_EN_DYM_BW_INDICATION,
 ODM_CMNINFO_WIFI_DIRECT,
 ODM_CMNINFO_WIFI_DISPLAY,
 ODM_CMNINFO_LINK_IN_PROGRESS,
 ODM_CMNINFO_LINK,
 ODM_CMNINFO_CMW500LINK,
 ODM_CMNINFO_STATION_STATE,
 ODM_CMNINFO_RSSI_MIN,
 ODM_CMNINFO_RSSI_MIN_BY_PATH,
 ODM_CMNINFO_DBG_COMP,
 ODM_CMNINFO_RA_THRESHOLD_HIGH,
 ODM_CMNINFO_RA_THRESHOLD_LOW,
 ODM_CMNINFO_RF_ANTENNA_TYPE,
 ODM_CMNINFO_WITH_EXT_ANTENNA_SWITCH,
 ODM_CMNINFO_BE_FIX_TX_ANT,
 ODM_CMNINFO_BT_ENABLED,
 ODM_CMNINFO_BT_HS_CONNECT_PROCESS,
 ODM_CMNINFO_BT_HS_RSSI,
 ODM_CMNINFO_BT_OPERATION,
 ODM_CMNINFO_BT_LIMITED_DIG,
 ODM_CMNINFO_AP_TOTAL_NUM,
 ODM_CMNINFO_POWER_TRAINING,
 ODM_CMNINFO_DFS_REGION_DOMAIN,
 ODM_CMNINFO_BT_CONTINUOUS_TURN,
 ODM_CMNINFO_IS_DOWNLOAD_FW,
 ODM_CMNINFO_PHYDM_PATCH_ID,
 ODM_CMNINFO_RRSR_VAL,
 ODM_CMNINFO_LINKED_BF_SUPPORT,
 ODM_CMNINFO_FLATNESS_TYPE,
 ODM_CMNINFO_STA_STATUS,
 ODM_CMNINFO_MAX,
};
enum phydm_rfe_bb_source_sel {
 PAPE_2G = 0,
 PAPE_5G = 1,
 LNA0N_2G = 2,
 LNAON_5G = 3,
 TRSW = 4,
 TRSW_B = 5,
 GNT_BT = 6,
 ZERO = 7,
 ANTSEL_0 = 8,
 ANTSEL_1 = 9,
 ANTSEL_2 = 0xa,
 ANTSEL_3 = 0xb,
 ANTSEL_4 = 0xc,
 ANTSEL_5 = 0xd,
 ANTSEL_6 = 0xe,
 ANTSEL_7 = 0xf
};
enum phydm_info_query {
 PHYDM_INFO_FA_OFDM,
 PHYDM_INFO_FA_CCK,
 PHYDM_INFO_FA_TOTAL,
 PHYDM_INFO_CCA_OFDM,
 PHYDM_INFO_CCA_CCK,
 PHYDM_INFO_CCA_ALL,
 PHYDM_INFO_CRC32_OK_VHT,
 PHYDM_INFO_CRC32_OK_HT,
 PHYDM_INFO_CRC32_OK_LEGACY,
 PHYDM_INFO_CRC32_OK_CCK,
 PHYDM_INFO_CRC32_ERROR_VHT,
 PHYDM_INFO_CRC32_ERROR_HT,
 PHYDM_INFO_CRC32_ERROR_LEGACY,
 PHYDM_INFO_CRC32_ERROR_CCK,
 PHYDM_INFO_EDCCA_FLAG,
 PHYDM_INFO_OFDM_ENABLE,
 PHYDM_INFO_CCK_ENABLE,
 PHYDM_INFO_CRC32_OK_HT_AGG,
 PHYDM_INFO_CRC32_ERROR_HT_AGG,
 PHYDM_INFO_DBG_PORT_0,
 PHYDM_INFO_CURR_IGI,
 PHYDM_INFO_RSSI_MIN,
 PHYDM_INFO_RSSI_MAX,
 PHYDM_INFO_CLM_RATIO,
 PHYDM_INFO_NHM_RATIO,
 PHYDM_INFO_NHM_NOISE_PWR,
};
enum phydm_api {
 PHYDM_API_NBI = 1,
 PHYDM_API_CSI_MASK = 2,
};
enum phydm_func_idx {
 F00_DIG = 0,
 F01_RA_MASK = 1,
 F02_DYN_TXPWR = 2,
 F03_FA_CNT = 3,
 F04_RSSI_MNTR = 4,
 F05_CCK_PD = 5,
 F06_ANT_DIV = 6,
 F07_SMT_ANT = 7,
 F08_PWR_TRAIN = 8,
 F09_RA = 9,
 F10_PATH_DIV = 10,
 F11_DFS = 11,
 F12_DYN_ARFR = 12,
 F13_ADPTVTY = 13,
 F14_CFO_TRK = 14,
 F15_ENV_MNTR = 15,
 F16_PRI_CCA = 16,
 F17_ADPTV_SOML = 17,
 F18_LNA_SAT_CHK = 18,
};
enum odm_ability {
 ODM_BB_DIG = BIT(F00_DIG),
 ODM_BB_RA_MASK = BIT(F01_RA_MASK),
 ODM_BB_DYNAMIC_TXPWR = BIT(F02_DYN_TXPWR),
 ODM_BB_FA_CNT = BIT(F03_FA_CNT),
 ODM_BB_RSSI_MONITOR = BIT(F04_RSSI_MNTR),
 ODM_BB_CCK_PD = BIT(F05_CCK_PD),
 ODM_BB_ANT_DIV = BIT(F06_ANT_DIV),
 ODM_BB_SMT_ANT = BIT(F07_SMT_ANT),
 ODM_BB_PWR_TRAIN = BIT(F08_PWR_TRAIN),
 ODM_BB_RATE_ADAPTIVE = BIT(F09_RA),
 ODM_BB_PATH_DIV = BIT(F10_PATH_DIV),
 ODM_BB_DFS = BIT(F11_DFS),
 ODM_BB_DYNAMIC_ARFR = BIT(F12_DYN_ARFR),
 ODM_BB_ADAPTIVITY = BIT(F13_ADPTVTY),
 ODM_BB_CFO_TRACKING = BIT(F14_CFO_TRK),
 ODM_BB_ENV_MONITOR = BIT(F15_ENV_MNTR),
 ODM_BB_PRIMARY_CCA = BIT(F16_PRI_CCA),
 ODM_BB_ADAPTIVE_SOML = BIT(F17_ADPTV_SOML),
 ODM_BB_LNA_SAT_CHK = BIT(F18_LNA_SAT_CHK),
};
enum phydm_dbg_comp {
 DBG_DIG = BIT(F00_DIG),
 DBG_RA_MASK = BIT(F01_RA_MASK),
 DBG_DYN_TXPWR = BIT(F02_DYN_TXPWR),
 DBG_FA_CNT = BIT(F03_FA_CNT),
 DBG_RSSI_MNTR = BIT(F04_RSSI_MNTR),
 DBG_CCKPD = BIT(F05_CCK_PD),
 DBG_ANT_DIV = BIT(F06_ANT_DIV),
 DBG_SMT_ANT = BIT(F07_SMT_ANT),
 DBG_PWR_TRAIN = BIT(F08_PWR_TRAIN),
 DBG_RA = BIT(F09_RA),
 DBG_PATH_DIV = BIT(F10_PATH_DIV),
 DBG_DFS = BIT(F11_DFS),
 DBG_DYN_ARFR = BIT(F12_DYN_ARFR),
 DBG_ADPTVTY = BIT(F13_ADPTVTY),
 DBG_CFO_TRK = BIT(F14_CFO_TRK),
 DBG_ENV_MNTR = BIT(F15_ENV_MNTR),
 DBG_PRI_CCA = BIT(F16_PRI_CCA),
 DBG_ADPTV_SOML = BIT(F17_ADPTV_SOML),
 DBG_LNA_SAT_CHK = BIT(F18_LNA_SAT_CHK),
 DBG_PHY_STATUS = BIT(20),
 DBG_TMP = BIT(21),
 DBG_FW_TRACE = BIT(22),
 DBG_TXBF = BIT(23),
 DBG_COMMON_FLOW = BIT(24),
 DBG_COMP_MCC = BIT(25),
 DBG_DM_SUMMARY = BIT(27),
 ODM_PHY_CONFIG = BIT(28),
 ODM_COMP_INIT = BIT(29),
 DBG_CMN = BIT(30),
 ODM_COMP_API = BIT(31)
};
enum odm_cca_path {
 ODM_CCA_2R = 0,
 ODM_CCA_1R_A = 1,
 ODM_CCA_1R_B = 2,
};
enum phy_reg_pg_type {
 PHY_REG_PG_RELATIVE_VALUE = 0,
 PHY_REG_PG_EXACT_VALUE = 1
};
enum phydm_offload_ability {
 PHYDM_PHY_PARAM_OFFLOAD = BIT(0),
 PHYDM_RF_IQK_OFFLOAD = BIT(1),
 PHYDM_RF_DPK_OFFLOAD = BIT(2),
};
enum phydm_init_result {
 PHYDM_INIT_SUCCESS = 0,
 PHYDM_INIT_FAIL_BBRF_REG_INVALID = 1
};
struct phydm_pause_lv {
 s8 lv_dig;
 s8 lv_cckpd;
 s8 lv_antdiv;
 s8 lv_adapt;
 s8 lv_adsl;
};
struct phydm_func_poiner {
 void (*pause_phydm_handler)(void *dm_void, u32 *val_buf, u8 val_len);
};
struct pkt_process_info {
 u8 lna_idx;
 u8 vga_idx;
};
struct phydm_iot_center {
 bool is_linked_cmw500;
 u8 win_patch_id;
 bool patch_id_100f0401;
 bool patch_id_10120200;
 u32 phydm_patch_id;
};
struct dm_struct {
 void *adapter;
 struct rtl8192cd_priv *priv;
 bool odm_ready;
 enum phy_reg_pg_type phy_reg_pg_value_type;
 u8 phy_reg_pg_version;
 u64 support_ability;
 u64 pause_ability;
 u64 debug_components;
 u8 cmn_dbg_msg_period;
 u8 cmn_dbg_msg_cnt;
 u32 fw_debug_components;
 u32 num_qry_phy_status_all;
 u32 last_num_qry_phy_status_all;
 u32 rx_pwdb_ave;
 bool is_init_hw_info_by_rfe;
 u8 en_tssi_mode;
 bool is_cck_high_power;
 u8 rf_path_rx_enable;
 u8 support_platform;
 u8 normal_rx_path;
 u8 valid_path_set;
 bool brxagcswitch;
 u8 support_interface;
 u32 support_ic_type;
 enum phydm_api_host run_in_drv_fw;
 u8 ic_ip_series;
 enum phydm_phy_sts_type ic_phy_sts_type;
 u8 cut_version;
 u8 fab_version;
 u8 fw_version;
 u8 fw_sub_version;
 u8 rf_type;
 u8 rfe_type;
 u8 board_type;
 u8 package_type;
 u16 type_glna;
 u16 type_gpa;
 u16 type_alna;
 u16 type_apa;
 u8 ext_lna;
 u8 ext_lna_5g;
 u8 ext_pa;
 u8 ext_pa_5g;
 u8 efuse0x3d7;
 u8 efuse0x3d8;
 u8 ext_trsw;
 u8 ext_lna_gain;
 bool is_in_hct_test;
 u8 wifi_test;
 bool is_dual_mac_smart_concurrent;
 u32 bk_support_ability;
 u8 with_extenal_ant_switch;
 bool cck_new_agc;
 s8 cck_lna_gain_table[8];
 u8 cck_sat_cnt_th_init;
 u32 phydm_sys_up_time;
 u8 num_rf_path;
 u32 soft_ap_special_setting;
 bool boolean_dummy;
 s8 s8_dummy;
 u8 u8_dummy;
 u16 u16_dummy;
 u32 u32_dummy;
 u8 rfe_hwsetting_band;
 u8 p_advance_ota;
 bool hp_hw_id;
 bool BOOLEAN_temp;
 bool is_dfs_band;
 u8 is_rx_blocking_en;
 u16 fw_offload_ability;
 bool is_download_fw;
 bool en_dis_dpd;
 u16 dis_dpd_rate;
 u8 en_auto_bw_th;
 u8 txagc_buff[4][(ODM_RATEVHTSS4MCS9 + 1)];
 u32 bp_0x9b0;
 u64 *num_tx_bytes_unicast;
 u64 *num_rx_bytes_unicast;
 u8 *band_type;
 u8 *sec_ch_offset;
 u8 *security;
 u8 *band_width;
 u8 *channel;
 bool *is_scan_in_process;
 bool *is_power_saving;
 bool *is_tdma;
 u8 *one_path_cca;
 u8 *antenna_test;
 bool *is_net_closed;
 bool *is_fcs_mode_enable;
 bool *is_1_antenna;
 u8 *rf_default_path;
 u16 *forced_data_rate;
 u8 *enable_antdiv;
 u8 *enable_pathdiv;
 u8 *en_adap_soml;
 u8 *edcca_mode;
 u8 *hub_usb_mode;
 bool *is_fw_dw_rsvd_page_in_progress;
 u32 *current_tx_tp;
 u32 *current_rx_tp;
 u8 *sounding_seq;
 u32 *soft_ap_mode;
 u8 *mp_mode;
 u32 *interrupt_mask;
 u8 *bb_op_mode;
 u32 *manual_supportability;
 u8 *dis_dym_bw_indication;
 u8 disable_phydm_watchdog;
 bool is_link_in_process;
 bool is_wifi_direct;
 bool is_wifi_display;
 bool is_linked;
 bool pre_is_linked;
 bool first_connect;
 bool first_disconnect;
 bool bsta_state;
 u8 rssi_min;
 u8 rssi_min_h_state;
 u8 rssi_min_macid;
 u8 pre_rssi_min;
 u8 rssi_max;
 u8 rssi_max_macid;
 u8 rssi_min_by_path;
 bool use_new_agc;
 bool ra_turbo_mode;
 bool is_mp_chip;
 bool is_one_entry_only;
 u32 one_entry_macid;
 u32 one_entry_tp;
 u32 pre_one_entry_tp;
 u8 pre_number_linked_client;
 u8 number_linked_client;
 u8 pre_number_active_client;
 u8 number_active_client;
 bool is_disable_phy_api;
 u8 rssi_a;
 u8 rssi_b;
 u8 rssi_c;
 u8 rssi_d;
 s8 rxsc_80;
 s8 rxsc_40;
 s8 rxsc_20;
 s8 rxsc_l;
 u64 rssi_trsw;
 u64 rssi_trsw_h;
 u64 rssi_trsw_l;
 u64 rssi_trsw_iso;
 u8 tx_ant_status;
 u8 rx_ant_status;
 enum bb_path tx_4ss_status;
 enum bb_path tx_3ss_status;
 enum bb_path tx_2ss_status;
 enum bb_path tx_1ss_status;
 u8 cck_lna_idx;
 u8 cck_vga_idx;
 u8 curr_station_id;
 u8 ofdm_agc_idx[4];
 u8 rx_rate;
 u8 rate_ss;
 u8 tx_rate;
 u8 linked_interval;
 u8 pre_channel;
 u32 txagc_offset_value_a;
 bool is_txagc_offset_positive_a;
 u32 txagc_offset_value_b;
 bool is_txagc_offset_positive_b;
 u8 ap_total_num;
 bool flatness_type;
 u8 traffic_load;
 u8 pre_traffic_load;
 u32 tx_tp;
 u32 rx_tp;
 u32 total_tp;
 u8 txrx_state_all;
 u64 cur_tx_ok_cnt;
 u64 cur_rx_ok_cnt;
 u64 last_tx_ok_cnt;
 u64 last_rx_ok_cnt;
 u16 consecutive_idlel_time;
 bool is_bb_swing_offset_positive_a;
 bool is_bb_swing_offset_positive_b;
 bool MPDIG_2G;
 u8 times_2g;
 u8 force_igi;
 u8 tdma_dig_timer_ms;
 u8 tdma_dig_state_number;
 u8 tdma_dig_low_upper_bond;
 u8 force_tdma_low_igi;
 u8 force_tdma_high_igi;
    u8 tdma_adptv_en;
 u8 tdma_adptv_trb_flag;
 u8 tdma_dig_adptv_trb_state_number;
 u8 tdma_dig_adptv_trb_swtch_th;
 u8 fix_expire_to_zero;
 bool original_dig_restore;
 bool tdmadig_active_en;
 bool edcca_by_igi;
 u8 ant_div_type;
 u8 antdiv_rssi;
 u8 fat_comb_a;
 u8 fat_comb_b;
 u8 antdiv_intvl;
 u8 antdiv_delay;
 u8 ant_type;
 u8 ant_type2;
 u8 pre_ant_type;
 u8 pre_ant_type2;
 u8 antdiv_period;
 u8 evm_antdiv_period;
 u8 antdiv_select;
 u8 antdiv_train_num;
 u8 stop_antdiv_rssi_th;
 u16 stop_antdiv_tp_diff_th;
 u16 stop_antdiv_tp_th;
 u8 antdiv_tp_period;
 u16 tp_active_th;
 u8 tp_active_occur;
 u8 path_select;
 u8 antdiv_evm_en;
 u8 bdc_holdstate;
 u8 antdiv_counter;
 u8 ndpa_period;
 bool h2c_rarpt_connect;
 bool cck_agc_report_type;
 u8 print_agc;
 u8 la_mode;
 u8 current_rf_set_8821c;
 u8 default_rf_set_8821c;
 u8 current_ant_num_8821c;
 u8 default_ant_num_8821c;
 u8 rfe_type_expand;
 s8 TH_L2H_default;
 s8 th_edcca_hl_diff_default;
 s8 th_l2h_ini;
 s8 th_edcca_hl_diff;
 bool carrier_sense_enable;
 u8 pre_dbg_priority;
 u8 nbi_set_result;
 u8 c2h_cmd_start;
 u8 fw_debug_trace[60];
 u8 pre_c2h_seq;
 bool fw_buff_is_enpty;
 u32 data_frame_num;
 bool en_reg_mntr_bb;
 bool en_reg_mntr_rf;
 bool en_reg_mntr_mac;
 bool en_reg_mntr_byte;
 u8 dsde_sel;
 u8 nbi_path_sel;
 u8 csi_wgt;
 bool is_noisy_state;
 bool noisy_decision;
 bool pre_b_noisy;
 u32 noisy_decision_smooth;
 bool is_stop_dym_ant_weighting;
 bool is_disable_dym_ecs;
 bool is_disable_dym_ant_weighting;
 struct cmn_sta_info *phydm_sta_info[MACID_NUM_SW_LIMIT];
 u8 phydm_macid_table[MACID_NUM_SW_LIMIT];
 bool ra_support88e;
 bool *is_driver_stopped;
 bool *is_driver_is_going_to_pnp_set_power_sleep;
 bool *pinit_adpt_in_progress;
 bool is_user_assign_level;
 u8 RSSI_BT;
 bool is_psd_in_process;
 bool is_psd_active;
 bool bsomlenabled;
 u8 no_ndp_cnts;
 u8 ndp_cnt_pre;
 bool is_beamformed;
 u8 linked_bf_support;
 bool bhtstfdisabled;
 u32 n_iqk_cnt;
 u32 n_iqk_ok_cnt;
 u32 n_iqk_fail_cnt;
 bool is_disable_power_training;
 bool is_bt_continuous_turn;
 u8 enhance_pwr_th[3];
 u8 set_pwr_th[3];
 struct phydm_bb_ram_ctrl p_bb_ram_ctrl;
 u8 dynamic_tx_high_power_lvl;
 void (*fill_desc_dyntxpwr)(void *dm, u8 *desc, u8 dyn_tx_power);
 u8 last_dtp_lvl;
 u8 min_power_index;
 u32 tx_agc_ofdm_18_6;
 u8 rx_pkt_type;
 struct rtw_timer_list mpt_dig_timer;
 struct rtw_timer_list fast_ant_training_timer;
 struct rtw_timer_list sbdcnt_timer;
 struct phydm_func_poiner phydm_func_handler;
 struct phydm_iot_center iot_table;
 struct pkt_process_info pkt_proc_struct;
 struct phydm_adaptivity_struct adaptivity;
 struct odm_noise_monitor noise_level;
 u32 retry_cnt;
 struct odm_phy_dbg_info phy_dbg_info;
 struct phydm_bf_rate_info_jgr3 bf_rate_info_jgr3;
 struct _hal_rf_ rf_table;
 struct dm_rf_calibration_struct rf_calibrate_info;
 struct dm_iqk_info IQK_info;
 struct dm_dpk_info dpk_info;
 struct dm_dack_info dack_info;
 struct phydm_dig_struct dm_dig_table;
 struct ra_table dm_ra_table;
 struct phydm_fa_struct false_alm_cnt;
 struct phydm_cfo_track_struct dm_cfo_track;
 struct ccx_info dm_ccx_info;
 struct odm_power_trim_data power_trim_data;
 struct phydm_pause_lv pause_lv_table;
 struct phydm_api_stuc api_table;
};
enum phydm_adv_ota {
 PHYDM_PATHB_1RCCA = BIT(0),
 PHYDM_HP_OTA_SETTING_A = BIT(1),
 PHYDM_HP_OTA_SETTING_B = BIT(2),
 PHYDM_ASUS_OTA_SETTING = BIT(3),
 PHYDM_ASUS_OTA_SETTING_CCK_PATH = BIT(4),
 PHYDM_HP_OTA_SETTING_CCK_PATH = BIT(5),
 PHYDM_LENOVO_OTA_SETTING_NBI_CSI = BIT(6),
};
enum phydm_bb_op_mode {
 PHYDM_PERFORMANCE_MODE = 0,
 PHYDM_BALANCE_MODE = 1,
};
enum phydm_structure_type {
 PHYDM_FALSEALMCNT,
 PHYDM_CFOTRACK,
 PHYDM_ADAPTIVITY,
 PHYDM_DFS,
 PHYDM_ROMINFO,
};
enum odm_bb_config_type {
 CONFIG_BB_PHY_REG,
 CONFIG_BB_AGC_TAB,
 CONFIG_BB_AGC_TAB_2G,
 CONFIG_BB_AGC_TAB_5G,
 CONFIG_BB_PHY_REG_PG,
 CONFIG_BB_PHY_REG_MP,
 CONFIG_BB_AGC_TAB_DIFF,
 CONFIG_BB_RF_CAL_INIT,
};
enum odm_rf_config_type {
 CONFIG_RF_RADIO,
 CONFIG_RF_TXPWR_LMT,
 CONFIG_RF_SYN_RADIO,
};
enum odm_fw_config_type {
 CONFIG_FW_NIC,
 CONFIG_FW_NIC_2,
 CONFIG_FW_AP,
 CONFIG_FW_AP_2,
 CONFIG_FW_MP,
 CONFIG_FW_WOWLAN,
 CONFIG_FW_WOWLAN_2,
 CONFIG_FW_AP_WOWLAN,
 CONFIG_FW_BT,
};
enum rt_status {
 RT_STATUS_SUCCESS,
 RT_STATUS_FAILURE,
 RT_STATUS_PENDING,
 RT_STATUS_RESOURCE,
 RT_STATUS_INVALID_CONTEXT,
 RT_STATUS_INVALID_PARAMETER,
 RT_STATUS_NOT_SUPPORT,
 RT_STATUS_OS_API_FAILED,
};
void
phydm_watchdog_lps(struct dm_struct *dm);
void
phydm_watchdog_lps_32k(struct dm_struct *dm);
void
phydm_txcurrentcalibration(struct dm_struct *dm);
void
phydm_dm_early_init(struct dm_struct *dm);
enum phydm_init_result
odm_dm_init(struct dm_struct *dm);
void
odm_dm_reset(struct dm_struct *dm);
void
phydm_fwoffload_ability_init(struct dm_struct *dm,
        enum phydm_offload_ability offload_ability);
void
phydm_fwoffload_ability_clear(struct dm_struct *dm,
         enum phydm_offload_ability offload_ability);
void
phydm_supportability_en(void *dm_void, char input[][16], u32 *_used,
   char *output, u32 *_out_len);
void
phydm_pause_dm_watchdog(void *dm_void, enum phydm_pause_type pause_type);
void
phydm_watchdog(struct dm_struct *dm);
void
phydm_watchdog_mp(struct dm_struct *dm);
void
phydm_pause_func_init(void *dm_void);
u8
phydm_pause_func(void *dm_void, enum phydm_func_idx pause_func,
   enum phydm_pause_type pause_type,
   enum phydm_pause_level pause_lv, u8 val_lehgth, u32 *val_buf);
void
phydm_pause_func_console(void *dm_void, char input[][16], u32 *_used,
    char *output, u32 *_out_len);
void phydm_pause_dm_by_asso_pkt(struct dm_struct *dm,
    enum phydm_pause_type pause_type, u8 rssi);
void
odm_cmn_info_init(struct dm_struct *dm, enum odm_cmninfo cmn_info, u64 value);
void
odm_cmn_info_hook(struct dm_struct *dm, enum odm_cmninfo cmn_info, void *value);
void
odm_cmn_info_update(struct dm_struct *dm, u32 cmn_info, u64 value);
u32
phydm_cmn_info_query(struct dm_struct *dm, enum phydm_info_query info_type);
void
odm_init_all_timers(struct dm_struct *dm);
void
odm_cancel_all_timers(struct dm_struct *dm);
void
odm_release_all_timers(struct dm_struct *dm);
void *
phydm_get_structure(struct dm_struct *dm, u8 structure_type);
void
phydm_dc_cancellation(struct dm_struct *dm);
void
phydm_receiver_blocking(void *dm_void);
void
phydm_dyn_bw_indication(void *dm_void);
void
phydm_iot_patch_id_update(void *dm_void, u32 iot_idx, bool en);
void
odm_dtc(struct dm_struct *dm);
enum halrf_func_idx {
 RF00_PWR_TRK = 0,
 RF01_IQK = 1,
 RF02_LCK = 2,
 RF03_DPK = 3,
 RF04_TXGAPK = 4,
 RF05_DACK = 5,
 RF06_DPK_TRK = 6,
 RF07_2GBAND_SHIFT = 7,
 RF08_RXDCK = 8,
 RF09_RFK = 9
};
enum halrf_ability {
 HAL_RF_TX_PWR_TRACK = BIT(RF00_PWR_TRK),
 HAL_RF_IQK = BIT(RF01_IQK),
 HAL_RF_LCK = BIT(RF02_LCK),
 HAL_RF_DPK = BIT(RF03_DPK),
 HAL_RF_TXGAPK = BIT(RF04_TXGAPK),
 HAL_RF_DACK = BIT(RF05_DACK),
 HAL_RF_DPK_TRACK = BIT(RF06_DPK_TRK),
 HAL_2GBAND_SHIFT = BIT(RF07_2GBAND_SHIFT),
 HAL_RF_RXDCK = BIT(RF08_RXDCK)
};
enum halrf_shift_band {
 HAL_RF_2P4 = 0,
 HAL_RF_2P3 = 1,
 HAL_RF_2P5 = 2
};
enum halrf_dbg_comp {
 DBG_RF_TX_PWR_TRACK = BIT(RF00_PWR_TRK),
 DBG_RF_IQK = BIT(RF01_IQK),
 DBG_RF_LCK = BIT(RF02_LCK),
 DBG_RF_DPK = BIT(RF03_DPK),
 DBG_RF_TXGAPK = BIT(RF04_TXGAPK),
 DBG_RF_DACK = BIT(RF05_DACK),
 DBG_RF_DPK_TRACK = BIT(RF06_DPK_TRK),
 DBG_RF_RFK = BIT(RF09_RFK),
 DBG_RF_MP = BIT(29),
 DBG_RF_TMP = BIT(30),
 DBG_RF_INIT = BIT(31)
};
enum halrf_cmninfo_init {
 HALRF_CMNINFO_ABILITY = 0,
 HALRF_CMNINFO_DPK_EN = 1,
 HALRF_CMNINFO_EEPROM_THERMAL_VALUE,
 HALRF_CMNINFO_RFK_FORBIDDEN,
 HALRF_CMNINFO_IQK_SEGMENT,
 HALRF_CMNINFO_RATE_INDEX,
 HALRF_CMNINFO_PWT_TYPE,
 HALRF_CMNINFO_MP_PSD_POINT,
 HALRF_CMNINFO_MP_PSD_START_POINT,
 HALRF_CMNINFO_MP_PSD_STOP_POINT,
 HALRF_CMNINFO_MP_PSD_AVERAGE,
 HALRF_CMNINFO_IQK_TIMES,
 HALRF_CMNINFO_MP_POWER_TRACKING_TYPE,
 HALRF_CMNINFO_POWER_TRACK_CONTROL
};
enum halrf_cmninfo_hook {
 HALRF_CMNINFO_CON_TX,
 HALRF_CMNINFO_SINGLE_TONE,
 HALRF_CMNINFO_CARRIER_SUPPRESSION,
 HALRF_CMNINFO_MP_RATE_INDEX,
 HALRF_CMNINFO_MANUAL_RF_SUPPORTABILITY
};
enum halrf_lna_set {
 HALRF_LNA_DISABLE = 0,
 HALRF_LNA_ENABLE = 1,
};
enum halrf_k_segment_time {
 SEGMENT_FREE = 0,
 SEGMENT_10MS = 10,
 SEGMENT_30MS = 30,
 SEGMENT_50MS = 50,
};
struct _halrf_tssi_data {
 s32 cck_offset_patha;
 s32 cck_offset_pathb;
 s32 tssi_trk_txagc_offset[4];
 s32 delta_tssi_txagc_offset[4];
 s16 txagc_codeword[84];
 u16 tssi_codeword[84];
 s8 tssi_efuse[4][25];
 s8 tssi_de_diff_efuse[4][10];
 s8 tssi_kfree_efuse[4][4];
 u8 thermal[4];
 u32 index[4][14];
 u8 do_tssi;
 u8 get_thermal;
 u8 tssi_finish_bit[4];
 u8 thermal_trigger;
 s8 tssi_de;
};
struct _halrf_txgapk_info {
 u32 txgapk_rf3f_bp[5][12][4];
 bool txgapk_bp_done;
 s8 offset[12][4];
 s8 fianl_offset[12][4];
 u8 read_txgain;
};
struct _hal_rf_ {
 u8 *test1;
 u32 rf_supportability;
 u8 rf_shift_band;
 u8 eeprom_thermal;
 u8 dpk_en;
 bool dpk_done;
 u64 dpk_progressing_time;
 u64 iqk_progressing_time;
 u32 fw_ver;
 bool *is_con_tx;
 bool *is_single_tone;
 bool *is_carrier_suppresion;
 bool is_dpk_in_progress;
 bool is_tssi_in_progress;
 bool is_bt_iqk_timeout;
 bool is_rfk_h2c_timeout;
 bool aac_checked;
 bool is_txgapk_in_progress;
 u8 *mp_rate_index;
 u32 *manual_rf_supportability;
 u32 p_rate_index;
 u8 pwt_type;
 u32 rf_dbg_comp;
 u8 rfk_type;
 u32 gnt_control;
 u8 ext_lna;
 u8 ext_lna_5g;
 u8 ext_pa;
 u8 ext_pa_5g;
 struct _halrf_psd_data halrf_psd_data;
 struct _halrf_tssi_data halrf_tssi_data;
 struct _halrf_txgapk_info halrf_txgapk_info;
 u8 power_track_type;
 u8 mp_pwt_type;
 u8 pre_band_type;
};
void halrf_iqk_info_dump(void *dm_void, u32 *_used, char *output,
    u32 *_out_len);
void halrf_iqk_hwtx_check(void *dm_void, bool is_check);
u8 halrf_match_iqk_version(void *dm_void);
void halrf_support_ability_debug(void *dm_void, char input[][16], u32 *_used,
     char *output, u32 *_out_len);
void halrf_cmn_info_init(void *dm_void, enum halrf_cmninfo_init cmn_info,
    u32 value);
void halrf_cmn_info_hook(void *dm_void, enum halrf_cmninfo_hook cmn_info,
    void *value);
void halrf_cmn_info_set(void *dm_void, u32 cmn_info, u64 value);
u64 halrf_cmn_info_get(void *dm_void, u32 cmn_info);
void halrf_watchdog(void *dm_void);
void halrf_supportability_init(void *dm_void);
void halrf_init(void *dm_void);
void halrf_iqk_trigger(void *dm_void, bool is_recovery);
void halrf_rfk_handshake(void *dm_void, bool is_before_k);
void halrf_rf_k_connect_trigger(void *dm_void, bool is_recovery,
    enum halrf_k_segment_time seg_time);
void halrf_segment_iqk_trigger(void *dm_void, bool clear,
          bool segment_iqk);
void halrf_lck_trigger(void *dm_void);
void halrf_iqk_debug(void *dm_void, u32 *const dm_value, u32 *_used,
       char *output, u32 *_out_len);
void phydm_get_iqk_cfir(void *dm_void, u8 idx, u8 path, bool debug);
void halrf_iqk_xym_read(void *dm_void, u8 path, u8 xym_type);
void halrf_rf_lna_setting(void *dm_void, enum halrf_lna_set type);
void halrf_do_imr_test(void *dm_void, u8 data);
u32 halrf_psd_log2base(u32 val);
void halrf_dpk_trigger(void *dm_void);
void halrf_txgapk_trigger(void *dm_void);
u8 halrf_dpk_result_check(void *dm_void);
void halrf_dpk_sram_read(void *dm_void);
void halrf_dpk_enable_disable(void *dm_void);
void halrf_dpk_track(void *dm_void);
void halrf_dpk_reload(void *dm_void);
void halrf_dpk_switch(void *dm_void, u8 enable);
void halrf_dpk_debug_cmd(void *dm_void, char input[][16], u32 *_used,
    char *output, u32 *_out_len);
void halrf_dpk_c2h_report_transfer(void *dm_void, bool is_ok, u8 *buf, u8 buf_size);
void halrf_dpk_info_rsvd_page(void *dm_void, u8 *buf, u32 *buf_size);
void halrf_reload_bp(void *dm_void, u32 *bp_reg, u32 *bp, u32 num);
void halrf_reload_bprf(void *dm_void, u32 *bp_reg, u32 bp[][4], u32 num,
         u8 ss);
void halrf_bp(void *dm_void, u32 *bp_reg, u32 *bp, u32 num);
void halrf_bprf(void *dm_void, u32 *bp_reg, u32 bp[][4], u32 num, u8 ss);
void halrf_mode(void *dm_void, u32 *i_value, u32 *q_value);
bool halrf_compare(void *dm_void, u32 value);
u32 halrf_delta(void *dm_void, u32 v1, u32 v2);
void halrf_minmax_compare(void *dm_void, u32 value, u32 *min, u32 *max);
void halrf_b_sort(void *dm_void, u32 *iv, u32 *qv);
void halrf_bubble(void *dm_void, u32 *v1, u32 *v2);
void halrf_swap(void *dm_void, u32 *v1, u32 *v2);
enum hal_status
halrf_config_rfk_with_header_file(void *dm_void, u32 config_type);
void halrf_iqk_dbg(void *dm_void);
void halrf_tssi_get_efuse(void *dm_void);
void halrf_do_tssi(void *dm_void);
void halrf_set_tssi_enable(void *dm_void, bool enable);
void halrf_do_thermal(void *dm_void);
u32 halrf_set_tssi_value(void *dm_void, u32 tssi_value);
void halrf_set_tssi_power(void *dm_void, s8 power);
void halrf_tssi_set_de_for_tx_verify(void *dm_void, u32 tssi_de, u8 path);
u32 halrf_query_tssi_value(void *dm_void);
void halrf_tssi_cck(void *dm_void);
void halrf_thermal_cck(void *dm_void);
void halrf_tssi_set_de(void *dm_void);
void halrf_tssi_dck(void *dm_void, u8 direct_do);
void halrf_calculate_tssi_codeword(void *dm_void);
void halrf_set_tssi_codeword(void *dm_void);
u8 halrf_get_tssi_codeword_for_txindex(void *dm_void);
void halrf_tssi_clean_de(void *dm_void);
u32 halrf_tssi_trigger_de(void *dm_void, u8 path);
u32 halrf_tssi_get_de(void *dm_void, u8 path);
u32 halrf_get_online_tssi_de(void *dm_void, u8 path, s32 pout);
void halrf_tssi_trigger(void *dm_void);
void halrf_txgapk_write_gain_table(void *dm_void);
void halrf_txgapk_reload_tx_gain(void *dm_void);
void halrf_txgap_enable_disable(void *dm_void, u8 enable);
void halrf_set_dpk_track(void *dm_void, u8 enable);
void halrf_set_dpkbychannel(void *dm_void, bool dpk_by_ch);
void halrf_set_dpkenable(void *dm_void, bool is_dpk_enable);
bool halrf_get_dpkbychannel(void *dm_void);
bool halrf_get_dpkenable(void *dm_void);
void _iqk_check_if_reload(void *dm_void);
void halrf_do_rxbb_dck(void *dm_void);
void config_halrf_path_adda_setting_trigger(void *dm_void);
void halrf_reload_iqk(void *dm_void, bool reset);
void halrf_dack_dbg(void *dm_void);
void halrf_dack_trigger(void *dm_void, bool force);
void halrf_dack_restore(void *dm_void);
void halrf_iqk_info_rsvd_page(void *dm_void, u8 *buf, u32 *buf_size);
void halrf_set_rfsupportability(void *dm_void);
void halrf_rxdck(void *dm_void);
void halrf_delay_10us(u16 v1);
void halrf_dump_rfk_reg(void *dm_void, char input[][16], u32 *_used,
         char *output, u32 *_out_len);
void halrf_rfk_power_save(void *dm_void, bool is_power_save);
struct dm_dack_info {
 bool dack_en;
 u16 msbk_d[2][2][15];
 u8 dck_d[2][2][2];
 u16 biask_d[2][2];
};
struct dm_iqk_info {
 bool lok_fail[4];
 bool iqk_fail[2][4];
 u32 iqc_matrix[2][4];
 u8 iqk_times;
 u32 rf_reg18;
 u32 rf_reg08;
 u32 lna_idx;
 u8 iqk_step;
 u8 rxiqk_step;
 u8 tmp1bcc;
 u8 txgain;
 u32 txgain56;
 u8 kcount;
 u8 rfk_ing;
 bool rfk_forbidden;
 u8 rxbb;
 u32 rf_reg58;
 bool segment_iqk;
 bool is_tssi_mode;
 u8 iqk_band;
 u8 iqk_ch;
 u8 iqk_bw;
 u32 iqk_channel[2];
 bool iqk_fail_report[2][4][2];
 u16 iqk_cfir_real[3][4][2][19];
 u16 iqk_cfir_imag[3][4][2][19];
 u8 retry_count[2][4][3];
 u8 gs_retry_count[2][4][2];
 u8 rxiqk_fail_code[2][4];
 u32 lok_idac[2][4];
 u16 rxiqk_agc[2][4];
 u32 bypass_iqk[2][4];
 u32 txgap_result[8];
 u32 tmp_gntwl;
 bool is_btg;
 bool isbnd;
 bool is_reload;
 bool is_hwtx;
 bool xym_read;
 bool trximr_enable;
 u32 rx_xym[2][10];
 u32 tx_xym[2][10];
 u32 gs1_xym[2][6];
 u32 gs2_xym[2][6];
 u32 rxk1_xym[2][6];
 u32 nbtxk_1b38[2];
 u32 nbrxk_1b3c[2];
};
struct dm_dpk_info {
 bool is_dpk_enable;
 bool is_dpk_pwr_on;
 bool is_dpk_by_channel;
 bool is_tssi_mode;
 u8 dpk_status;
 u16 dpk_path_ok;
 u8 thermal_dpk[4];
 u8 thermal_dpk_avg[4][8];
 u8 pre_pwsf[4];
 u8 thermal_dpk_avg_index;
 u32 gnt_control;
 u32 gnt_value;
 u8 dpk_ch;
 u8 dpk_band;
 u8 dpk_bw;
 u32 dpk_rf18[2];
 u32 dpk_cal_cnt;
 u32 dpk_ok_cnt;
 u32 dpk_reload_cnt;
 u8 dpk_result[4][3];
 u8 pwsf_2g[4][3];
 u32 lut_2g_even[4][3][64];
 u32 lut_2g_odd[4][3][64];
 s16 tmp_pas_i[32];
 s16 tmp_pas_q[32];
 u8 dpk_5g_result[4][6];
 u8 pwsf_5g[4][6];
 u32 lut_5g[4][6][64];
 u32 lut_2g[4][3][64];
 u8 rxbb[4];
 u8 txbb[4];
 u8 tx_gain;
};
 extern u32 ofdm_swing_table[43];
 extern u8 cck_swing_table_ch1_ch13[33][8];
 extern u8 cck_swing_table_ch14[33][8];
 extern u32 ofdm_swing_table_new[43];
 extern u8 cck_swing_table_ch1_ch13_new[33][8];
 extern u8 cck_swing_table_ch14_new[33][8];
 extern u8 cck_swing_table_ch1_ch14_88f[21][16];
 extern u8 cck_swing_table_ch1_ch13_88f[21][16];
 extern u8 cck_swing_table_ch14_88f[21][16];
 extern u32 cck_swing_table_ch1_ch14_8192f[41];
extern u8 delta_swing_table_idx_2ga_p_default[30];
extern u8 delta_swing_table_idx_2ga_n_default[30];
extern u8 delta_swing_table_idx_2ga_p_8188e[];
extern u8 delta_swing_table_idx_2ga_n_8188e[];
 extern u32 tx_scaling_table_jaguar[37];
extern u32 cck_swing_table_ch1_ch14_8723d[41];
extern u32 cck_swing_table_ch1_ch14_8710b[41];
struct iqk_matrix_regs_setting {
 bool is_iqk_done;
 s32 value[1][8];
};
struct dm_rf_calibration_struct {
 u32 rega24;
 s32 rege94;
 s32 rege9c;
 s32 regeb4;
 s32 regebc;
 u8 tx_powercount;
 bool is_txpowertracking_init;
 bool is_txpowertracking;
 u8 txpowertrack_control;
 u8 tm_trigger;
 u8 internal_pa_5g[2];
 u8 thermal_meter[2];
 u8 thermal_value;
 u8 thermal_value_path[4];
 u8 thermal_value_lck;
 u8 thermal_value_iqk;
 s8 thermal_value_delta;
 u8 thermal_value_avg[8];
 u8 thermal_value_avg_path[4][8];
 u8 thermal_value_avg_index;
 u8 thermal_value_avg_index_path[4];
 s8 power_index_offset_path[4];
 u8 thermal_value_rx_gain;
 u8 thermal_value_crystal;
 u8 thermal_value_dpk_store;
 u8 thermal_value_dpk_track;
 bool txpowertracking_in_progress;
 bool is_reloadtxpowerindex;
 u8 is_rf_pi_enable;
 u32 txpowertracking_callback_cnt;
 u8 is_cck_in_ch14;
 u8 CCK_index;
 u8 OFDM_index[4];
 s8 power_index_offset;
 s8 delta_power_index;
 s8 delta_power_index_path[4];
 s8 delta_power_index_last;
 s8 delta_power_index_last_path[4];
 bool is_tx_power_changed;
 struct iqk_matrix_regs_setting iqk_matrix_reg_setting[(14+24+21)];
 u8 delta_lck;
 u8 delta_swing_table_idx_2g_cck_a_p[30];
 u8 delta_swing_table_idx_2g_cck_a_n[30];
 u8 delta_swing_table_idx_2g_cck_b_p[30];
 u8 delta_swing_table_idx_2g_cck_b_n[30];
 u8 delta_swing_table_idx_2g_cck_c_p[30];
 u8 delta_swing_table_idx_2g_cck_c_n[30];
 u8 delta_swing_table_idx_2g_cck_d_p[30];
 u8 delta_swing_table_idx_2g_cck_d_n[30];
 u8 delta_swing_table_idx_2ga_p[30];
 u8 delta_swing_table_idx_2ga_n[30];
 u8 delta_swing_table_idx_2gb_p[30];
 u8 delta_swing_table_idx_2gb_n[30];
 u8 delta_swing_table_idx_2gc_p[30];
 u8 delta_swing_table_idx_2gc_n[30];
 u8 delta_swing_table_idx_2gd_p[30];
 u8 delta_swing_table_idx_2gd_n[30];
 u8 delta_swing_table_idx_5ga_p[3][30];
 u8 delta_swing_table_idx_5ga_n[3][30];
 u8 delta_swing_table_idx_5gb_p[3][30];
 u8 delta_swing_table_idx_5gb_n[3][30];
 u8 delta_swing_table_idx_5gc_p[3][30];
 u8 delta_swing_table_idx_5gc_n[3][30];
 u8 delta_swing_table_idx_5gd_p[3][30];
 u8 delta_swing_table_idx_5gd_n[3][30];
 u8 delta_swing_tssi_table_2g_cck_a[61];
 u8 delta_swing_tssi_table_2g_cck_b[61];
 u8 delta_swing_tssi_table_2g_cck_c[61];
 u8 delta_swing_tssi_table_2g_cck_d[61];
 u8 delta_swing_tssi_table_2ga[61];
 u8 delta_swing_tssi_table_2gb[61];
 u8 delta_swing_tssi_table_2gc[61];
 u8 delta_swing_tssi_table_2gd[61];
 u8 delta_swing_tssi_table_5ga[3][61];
 u8 delta_swing_tssi_table_5gb[3][61];
 u8 delta_swing_tssi_table_5gc[3][61];
 u8 delta_swing_tssi_table_5gd[3][61];
 s8 delta_swing_table_xtal_p[30];
 s8 delta_swing_table_xtal_n[30];
 u8 delta_swing_table_idx_2ga_p_8188e[30];
 u8 delta_swing_table_idx_2ga_n_8188e[30];
 u8 bb_swing_idx_ofdm[4];
 u8 bb_swing_idx_ofdm_current;
 u8 bb_swing_idx_ofdm_base[4];
 bool bb_swing_flag_ofdm;
 u8 bb_swing_idx_cck;
 u8 bb_swing_idx_cck_current;
 u8 bb_swing_idx_cck_base;
 u8 default_ofdm_index;
 u8 default_cck_index;
 s8 default_txagc_index;
 bool bb_swing_flag_cck;
 s8 absolute_ofdm_swing_idx[4];
 s8 remnant_ofdm_swing_idx[4];
 s8 absolute_cck_swing_idx[4];
 s8 remnant_cck_swing_idx;
 s8 modify_tx_agc_value;
 bool modify_tx_agc_flag_path_a;
 bool modify_tx_agc_flag_path_b;
 bool modify_tx_agc_flag_path_c;
 bool modify_tx_agc_flag_path_d;
 bool modify_tx_agc_flag_path_a_cck;
 bool modify_tx_agc_flag_path_b_cck;
 s8 kfree_offset[4];
 u32 regc04;
 u32 reg874;
 u32 regc08;
 u32 regb68;
 u32 regb6c;
 u32 reg870;
 u32 reg860;
 u32 reg864;
 bool is_iqk_initialized;
 bool is_lck_in_progress;
 bool is_antenna_detected;
 bool is_need_iqk;
 bool is_iqk_in_progress;
 bool is_iqk_pa_off;
 u8 delta_iqk;
 u32 ADDA_backup[16];
 u32 IQK_MAC_backup[4];
 u32 IQK_BB_backup_recover[9];
 u32 IQK_BB_backup[9];
 u32 tx_iqc_8723b[2][3][2];
 u32 rx_iqc_8723b[2][2][2];
 u32 tx_iqc_8703b[3][2];
 u32 rx_iqc_8703b[2][2];
 u64 iqk_start_time;
 u64 iqk_total_progressing_time;
 u64 iqk_progressing_time;
 u64 lck_progressing_time;
 u32 lok_result;
 u8 iqk_step;
 u8 kcount;
 u8 retry_count[4][2];
 bool is_mp_mode;
 u32 ap_koutput[2][2];
 u8 is_ap_kdone;
 u8 is_apk_thermal_meter_ignore;
 u8 is_dp_done;
 s8 modify_tx_agc_value_ofdm;
 s8 modify_tx_agc_value_cck;
 u8 reg_rf_kfree_enable;
 u8 rf_kfree_enable;
 u32 tx_lok[2];
};
void
odm_txpowertracking_check_ap(
 void *dm_void
);
void
odm_txpowertracking_check(
 void *dm_void
);
void
odm_txpowertracking_thermal_meter_init(
 void *dm_void
);
void
odm_txpowertracking_init(
 void *dm_void
);
void
odm_txpowertracking_check_mp(
 void *dm_void
);
void
odm_txpowertracking_check_ce(
 void *dm_void
);
struct txagc_table_8814b {
 u8 ref_pow_cck[4];
 u8 ref_pow_ofdm[4];
 s8 diff_t[(ODM_RATEVHTSS4MCS9 + 1)];
};
bool phydm_chk_pkg_set_valid_8814b(struct dm_struct *dm,
          u8 ver_bb, u8 ver_rf);
void phydm_rfe_8814b_init(struct dm_struct *dm);
bool
phydm_rfe_8814b(struct dm_struct *dm, u8 channel);
void
phydm_bb_reset_8814b(struct dm_struct *dm);
u32 config_phydm_read_syn_reg_8814b(struct dm_struct *dm, enum rf_syn syn_path,
        u32 reg_addr, u32 bit_mask);
u32 config_phydm_read_rf_reg_8814b(struct dm_struct *dm, enum rf_path path,
       u32 reg_addr, u32 bit_mask);
u32 config_phydm_write_rf_syn_8814b(struct dm_struct *dm,
        enum rf_syn syn_path, u32 reg_addr,
        u32 bit_mask, u32 data);
bool
config_phydm_write_rf_reg_8814b(struct dm_struct *dm, enum rf_path path,
    u32 reg_addr, u32 bit_mask, u32 data);
bool
phydm_write_txagc_1byte_8814b(struct dm_struct *dm, s8 pw_idx, u8 hw_rate);
bool
config_phydm_write_txagc_ref_8814b(struct dm_struct *dm, u8 power_index,
       enum rf_path path,
       enum PDM_RATE_TYPE mod_type);
bool
config_phydm_write_txagc_diff_8814b(struct dm_struct *dm,
        s8 power_index1, s8 power_index2,
        s8 power_index3, s8 power_index4,
        u8 hw_rate);
s8 config_phydm_read_txagc_diff_8814b(struct dm_struct *dm, u8 hw_rate);
u8 config_phydm_read_txagc_8814b(struct dm_struct *dm, enum rf_path path,
     u8 hw_rate, enum PDM_RATE_TYPE mod_type);
void config_phydm_path_adda_setting_8814b(struct dm_struct *dm);
bool
config_phydm_trx_mode_8814b(struct dm_struct *dm, enum bb_path tx_path,
       enum bb_path rx_path);
bool
config_phydm_trx_mode_ext_8814b(struct dm_struct *dm, enum bb_path tx_path,
    enum bb_path rx_path,
    enum bb_path tx_path_sel_1ss,
    enum bb_path tx_path_sel_2ss,
    enum bb_path tx_path_sel_3ss);
bool
config_phydm_trx_80p80_8814b(struct dm_struct *dm, enum bb_path tx_path,
        enum bb_path rx_path, u8 primary_ch_idx);
bool
config_phydm_switch_band_8814b(struct dm_struct *dm, u8 central_ch);
bool
config_phydm_switch_channel_8814b(struct dm_struct *dm, u8 central_ch);
bool
config_phydm_switch_bw_8814b(struct dm_struct *dm, u8 primary_ch_idx,
        enum channel_width bandwidth);
bool
config_phydm_ch_80p80_8814b(struct dm_struct *dm, u8 central_ch1,
       u8 central_ch2, u8 primary_ch_idx);
bool
config_phydm_switch_channel_bw_8814b(struct dm_struct *dm, u8 central_ch,
         u8 primary_ch_idx, enum channel_width bw);
bool
config_phydm_parameter_init_8814b(struct dm_struct *dm,
      enum odm_parameter_init type);
bool
config_phydm_syn_ch_8814b(struct dm_struct *dm, u8 central_ch,
     enum rf_syn syn_path);
bool
config_phydm_mu_anapar_8814b(struct dm_struct *dm, u8 mu_bfee_cnt);
void
config_phydm_set_txagc_to_hw_8814b(struct dm_struct *dm);
bool
config_phydm_write_txagc_8814b(struct dm_struct *dm,
          u32 power_index, enum rf_path path, u8 hw_rate);
void phydm_dynamic_spur_det_eliminate_8814b(struct dm_struct *dm);
void phydm_init_hw_info_by_rfe_type_8814b(struct dm_struct *dm);
s32 phydm_get_condition_number_8814b(struct dm_struct *dm);
u8 halrf_do_tssi_8814b(void *dm_void, u8 rf_path);
void halrf_trigger_tssi_8814b( void *dm_void, u8 rf_path);
u8 halrf_get_tssi_result_8814b(void *dm_void, u8 rf_path);
void halrf_calculate_tssi_codeword_8814b(void *dm_void, u8 rf_path);
void halrf_set_tssi_codeword_8814b(void *dm_void, u16 *tssi_value);
u8 halrf_get_tssi_codeword_8814b(void *dm_void);
u32 halrf_query_tssi_value_8814b(void *dm_void);
u32 halrf_set_tssi_value_8814b(void *dm_void, u32 tssi_codeowrd);
void halrf_tssi_get_efuse_8814b(void *dm_void);
void halrf_get_efuse_thermal_pwrtype_8814b(void *dm_void);
void halrf_tssi_set_de_8814b(void *dm_void);
void halrf_tssi_set_de_for_tx_verify_8814b(
 void *dm_void, u32 tssi_de, u8 path);
void halrf_tssi_clean_de_8814b(void *dm_void);
u32 halrf_tssi_trigger_de_8814b(void *dm_void, u8 path);
u32 halrf_tssi_get_de_8814b(void *dm_void, u8 path);
void halrf_tssi_dck_8814b(void *dm_void, u8 direct_do);
void configure_txpower_track_8814b(struct txpwrtrack_cfg *config);
void odm_tx_pwr_track_set_pwr8814b(void *dm_void, enum pwrtrack_method method,
       u8 rf_path, u8 channel_mapped_index);
void get_delta_swing_table_8814b(void *dm_void,
     u8 **temperature_up_a,
     u8 **temperature_down_a,
     u8 **temperature_up_b,
     u8 **temperature_down_b);
void halrf_dac_cal_8814b(void *dm_void);
void phy_lc_calibrate_8814b(void *dm_void);
void halrf_rf_lna_setting_8814b(struct dm_struct *dm, enum halrf_lna_set type);
void halrf_do_rxbb_dck_8814b(void *dm_void);
void phy_set_rf_path_switch_8814b(struct dm_struct *dm,
      bool is_main);
void do_iqk_8814b(void *dm_void, u8 delta_thermal_index, u8 thermal_value,
    u8 threshold);
void phy_iq_calibrate_8814b(void *dm_void, bool clear, bool segment_iqk);
void do_imr_test_8814b(void *dm_void);
void phy_get_iqk_cfir_8814b(void *dm_void, u8 idx, u8 path, bool debug);
void phy_iqk_dbg_cfir_backup_8814b(void *dm_void);
u8 dpk_channel_transfer_8814b(void *dm_void);
void dpk_sram_read_8814b(void *dm_void);
void dpk_enable_disable_8814b(void *dm_void);
void dpk_reload_8814b(void *dm_void);
void do_dpk_8814b(void *dm_void);
void dpk_track_8814b(void *dm_void);
void dpk_result_summary_8814b(void *dm_void);
void dpk_by_group_8814b(void *dm_void);
void dpk_set_dpkbychannel_8814b(void *dm_void, bool dpk_by_ch);
bool dpk_get_dpkbychannel_8814b(void *dm_void);
void dpk_set_is_dpk_enable_8814b(void *dm_void, bool is_dpk_enable);
bool dpk_get_is_dpk_enable_8814b(void *dm_void);
void dpk_thermal_read_8814b(void *dm_void);
void do_txgapk_8814b(void *dm_void,
       u8 delta_thermal_index,
       u8 thermal_value,
       u8 threshold);
void phy_txgap_calibrate_8814b(void *dm_void,
          bool clear);
void odm_read_and_config_mp_8814b_cal_init(void *dm_void);
