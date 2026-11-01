static void
__cfg_txpwr_lmt_by_alt(struct rtw_hal *hal, u8 regd, u8 regd_alt, u8 bw, u8 rs);
static bool __rtw8822c_dac_cal_restore(struct rtw_dev *rtwdev);
static void __rtw8822c_dac_iq_sort(struct rtw_dev *rtwdev, u32 *v1, u32 *v2);
static void __rtw8822c_pwr_track(struct rtw_dev *rtwdev);
static void
__rtw_phy_tx_power_limit_config(struct rtw_hal *hal, u8 regd, u8 bw, u8 rs);
static bool _rtw8822c_txgapk_gain_valid(struct rtw_dev *rtwdev, u32 gain);
static void _rtw8822c_txgapk_write_gain_bb_table(struct rtw_dev *rtwdev,
						 u8 band, u8 path);
static bool check_rf_status(struct rtw_dev *rtwdev, u8 status);
static void rtw8821c_cck_tx_filter_srrc(struct rtw_dev *rtwdev, u8 channel, u8 bw);
static void rtw8821c_do_iqk(struct rtw_dev *rtwdev);
static u32 rtw8821c_get_bb_swing(struct rtw_dev *rtwdev, u8 channel);
static u8 rtw8821c_get_swing_index(struct rtw_dev *rtwdev);
static void rtw8821c_phy_calibration(struct rtw_dev *rtwdev);
static void rtw8821c_phy_pwrtrack(struct rtw_dev *rtwdev);
static void rtw8821c_pwr_track(struct rtw_dev *rtwdev);
static void rtw8821c_pwrtrack_init(struct rtw_dev *rtwdev);
static void rtw8821c_pwrtrack_set(struct rtw_dev *rtwdev);
static void rtw8821c_pwrtrack_set_pwr(struct rtw_dev *rtwdev, u8 pwr_idx_offset,
				      s8 pwr_idx_offset_lower);
static void rtw8821c_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx);
static void rtw8821c_set_channel_bb(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				    u8 primary_ch_idx);
static void rtw8821c_set_channel_bb_swing(struct rtw_dev *rtwdev, u8 channel,
					  u8 bw, u8 primary_ch_idx);
static void rtw8821c_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw);
static void rtw8821c_set_channel_rxdfir(struct rtw_dev *rtwdev, u8 bw);
static void rtw8821c_set_tx_power_index(struct rtw_dev *rtwdev);
static void
rtw8821c_set_tx_power_index_by_rate(struct rtw_dev *rtwdev, u8 path,
				    u8 rs, u32 *phy_pwr_idx);
static void rtw8821c_switch_rf_set(struct rtw_dev *rtwdev, u8 rf_set);
static void
rtw8821c_txagc_swing_offset(struct rtw_dev *rtwdev, u8 pwr_idx_offset,
			    s8 pwr_idx_offset_lower,
			    s8 *txagc_idx, u8 *swing_idx);
static void rtw8822b_config_trx_mode(struct rtw_dev *rtwdev, u8 tx_path,
				     u8 rx_path, bool is_tx2_path);
static void rtw8822b_do_iqk(struct rtw_dev *rtwdev);
static void rtw8822b_get_cca_val(const struct cca_ccut *cca_ccut, u8 col,
				 u32 *reg82c, u32 *reg830, u32 *reg838);
static u8 rtw8822b_get_swing_index(struct rtw_dev *rtwdev);
static void rtw8822b_phy_calibration(struct rtw_dev *rtwdev);
static void rtw8822b_phy_pwrtrack(struct rtw_dev *rtwdev);
static void rtw8822b_phy_pwrtrack_path(struct rtw_dev *rtwdev,
				       struct rtw_swing_table *swing_table,
				       u8 path);
static void rtw8822b_phy_rfe_init(struct rtw_dev *rtwdev);
static void rtw8822b_pwr_track(struct rtw_dev *rtwdev);
static void rtw8822b_pwrtrack_init(struct rtw_dev *rtwdev);
static void rtw8822b_pwrtrack_set(struct rtw_dev *rtwdev, u8 path);
static void rtw8822b_pwrtrack_set_pwr(struct rtw_dev *rtwdev, u8 path,
				      u8 pwr_idx_offset);
static void rtw8822b_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx);
static void rtw8822b_set_channel_bb(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				    u8 primary_ch_idx);
static void rtw8822b_set_channel_cca(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				     const struct rtw8822b_rfe_info *rfe_info);
static void rtw8822b_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw);
static void rtw8822b_set_channel_rfe_efem(struct rtw_dev *rtwdev, u8 channel);
static void rtw8822b_set_channel_rfe_ifem(struct rtw_dev *rtwdev, u8 channel);
static void rtw8822b_set_channel_rxdfir(struct rtw_dev *rtwdev, u8 bw);
static void rtw8822b_set_tx_power_index(struct rtw_dev *rtwdev);
static void
rtw8822b_set_tx_power_index_by_rate(struct rtw_dev *rtwdev, u8 path,
				    u8 rs, u32 *phy_pwr_idx);
static void rtw8822b_toggle_igi(struct rtw_dev *rtwdev);
static void rtw8822b_txagc_swing_offset(struct rtw_dev *rtwdev, u8 path,
					u8 tx_pwr_idx_offset,
					s8 *txagc_idx, u8 *swing_idx);
static void rtw8822c_bb_reset(struct rtw_dev *rtwdev);
static void rtw8822c_config_cck_rx_path(struct rtw_dev *rtwdev, u8 rx_path);
static void rtw8822c_config_cck_tx_path(struct rtw_dev *rtwdev, u8 tx_path,
					bool is_tx2_path);
static void rtw8822c_config_ofdm_rx_path(struct rtw_dev *rtwdev, u8 rx_path);
static void rtw8822c_config_ofdm_tx_path(struct rtw_dev *rtwdev, u8 tx_path,
					 enum rtw_bb_path tx_path_sel_1ss);
static void rtw8822c_config_rx_path(struct rtw_dev *rtwdev, u8 rx_path);
static void rtw8822c_config_trx_mode(struct rtw_dev *rtwdev, u8 tx_path,
				     u8 rx_path, bool is_tx2_path);
static void rtw8822c_config_tx_path(struct rtw_dev *rtwdev, u8 tx_path,
				    enum rtw_bb_path tx_path_sel_1ss,
				    enum rtw_bb_path tx_path_cck,
				    bool is_tx2_path);
static void rtw8822c_dac_backup_reg(struct rtw_dev *rtwdev,
				    struct rtw_backup_info *backup,
				    struct rtw_backup_info *backup_rf);
static void rtw8822c_dac_bb_setting(struct rtw_dev *rtwdev);
static void rtw8822c_dac_cal_adc(struct rtw_dev *rtwdev,
				 u8 path, u32 *adc_ic, u32 *adc_qc);
static void rtw8822c_dac_cal_backup(struct rtw_dev *rtwdev);
static void rtw8822c_dac_cal_backup_dck(struct rtw_dev *rtwdev);
static void rtw8822c_dac_cal_backup_path(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dac_cal_backup_vec(struct rtw_dev *rtwdev,
					u8 path, u8 vec, u32 w_addr, u32 r_addr);
static void rtw8822c_dac_cal_iq_sample(struct rtw_dev *rtwdev, u32 *iv, u32 *qv);
static void rtw8822c_dac_cal_iq_search(struct rtw_dev *rtwdev,
				       u32 *iv, u32 *qv,
				       u32 *i_value, u32 *q_value);
static bool rtw8822c_dac_cal_restore(struct rtw_dev *rtwdev);
static void rtw8822c_dac_cal_restore_dck(struct rtw_dev *rtwdev);
static bool rtw8822c_dac_cal_restore_path(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dac_cal_restore_prepare(struct rtw_dev *rtwdev);
static bool rtw8822c_dac_cal_restore_wait(struct rtw_dev *rtwdev,
					  u32 target_addr, u32 toggle_addr);
static void rtw8822c_dac_cal_rf_mode(struct rtw_dev *rtwdev,
				     u32 *i_value, u32 *q_value);
static void rtw8822c_dac_cal_step1(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dac_cal_step2(struct rtw_dev *rtwdev,
				   u8 path, u32 *ic_out, u32 *qc_out);
static void rtw8822c_dac_cal_step3(struct rtw_dev *rtwdev, u8 path,
				   u32 adc_ic, u32 adc_qc,
				   u32 *ic_in, u32 *qc_in,
				   u32 *i_out, u32 *q_out);
static void rtw8822c_dac_cal_step4(struct rtw_dev *rtwdev, u8 path);
static bool rtw8822c_dac_iq_check(struct rtw_dev *rtwdev, u32 value);
static void rtw8822c_dac_iq_offset(struct rtw_dev *rtwdev, u32 *vec, u32 *val);
static void rtw8822c_dac_iq_sort(struct rtw_dev *rtwdev, u32 *iv, u32 *qv);
static void rtw8822c_dac_restore_reg(struct rtw_dev *rtwdev,
				     struct rtw_backup_info *backup,
				     struct rtw_backup_info *backup_rf);
static void rtw8822c_do_dpk(struct rtw_dev *rtwdev);
static void rtw8822c_do_gapk(struct rtw_dev *rtwdev);
static void rtw8822c_do_iqk(struct rtw_dev *rtwdev);
static void rtw8822c_do_lck(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_afe_setting(struct rtw_dev *rtwdev, bool is_do_dpk);
static u8 rtw8822c_dpk_agc_gain_chk(struct rtw_dev *rtwdev, u8 path,
				    u8 limited_pga);
static u8 rtw8822c_dpk_agc_loss_chk(struct rtw_dev *rtwdev, u8 path);
static void
rtw8822c_dpk_backup_registers(struct rtw_dev *rtwdev, u32 *reg,
			      u32 reg_num, struct rtw_backup_info *bckp);
static void rtw8822c_dpk_backup_rf_registers(struct rtw_dev *rtwdev,
					     u32 *rf_reg,
					     u32 rf_reg_bak[][2]);
static u8 rtw8822c_dpk_by_path(struct rtw_dev *rtwdev, u32 tx_agc, u8 path);
static void rtw8822c_dpk_cal_coef1(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_cal_gs(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_calibrate(struct rtw_dev *rtwdev, u8 path);
static bool rtw8822c_dpk_check_pass(struct rtw_dev *rtwdev, bool is_fail,
				    u32 dpk_txagc, u8 path);
static bool rtw8822c_dpk_coef_iq_check(struct rtw_dev *rtwdev,
				       u16 coef_i, u16 coef_q);
static u8 rtw8822c_dpk_coef_read(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_coef_tbl_apply(struct rtw_dev *rtwdev, u8 path);
static u32 rtw8822c_dpk_coef_transfer(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_coef_write(struct rtw_dev *rtwdev, u8 path, u8 result);
static u8 rtw8822c_dpk_dc_corr_check(struct rtw_dev *rtwdev, u8 path);
static u16 rtw8822c_dpk_dgain_read(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_enable_disable(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_fill_result(struct rtw_dev *rtwdev, u32 dpk_txagc,
				     u8 path, u8 result);
static u32 rtw8822c_dpk_gainloss(struct rtw_dev *rtwdev, u8 path);
static u8 rtw8822c_dpk_gainloss_result(struct rtw_dev *rtwdev, u8 path);
static u16 rtw8822c_dpk_get_cmd(struct rtw_dev *rtwdev, u8 action, u8 path);
static void rtw8822c_dpk_get_coef(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_information(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_mac_bb_setting(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_on(struct rtw_dev *rtwdev, u8 path);
static u8 rtw8822c_dpk_one_shot(struct rtw_dev *rtwdev, u8 path, u8 action);
static u8 rtw8822c_dpk_pas_agc(struct rtw_dev *rtwdev, u8 path,
			       bool gain_only, bool loss_only);
static u32 rtw8822c_dpk_pas_read(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_path_select(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_pre_setting(struct rtw_dev *rtwdev);
static bool rtw8822c_dpk_reload(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_reload_data(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_reload_rf_registers(struct rtw_dev *rtwdev,
					     u32 *rf_reg,
					     u32 rf_reg_bak[][2]);
static void
rtw8822c_dpk_restore_registers(struct rtw_dev *rtwdev, u32 reg_num,
			       struct rtw_backup_info *bckp);
static void rtw8822c_dpk_result_reset(struct rtw_dev *rtwdev);
static u32 rtw8822c_dpk_rf_setting(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_rxbb_dc_cal(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_set_gnt_wl(struct rtw_dev *rtwdev, bool is_before_k);
static u8 rtw8822c_dpk_thermal_read(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_dpk_track(struct rtw_dev *rtwdev);
static void rtw8822c_dpk_tx_pause(struct rtw_dev *rtwdev);
static u8 rtw8822c_gain_check_state(struct rtw_dev *rtwdev,
				    struct rtw8822c_dpk_data *data);
static u8 rtw8822c_gain_large_state(struct rtw_dev *rtwdev,
				    struct rtw8822c_dpk_data *data);
static u8 rtw8822c_gain_less_state(struct rtw_dev *rtwdev,
				   struct rtw8822c_dpk_data *data);
static u32 rtw8822c_get_path_read_addr(u8 path);
static u32 rtw8822c_get_path_write_addr(u8 path);
static u8 rtw8822c_gl_large_state(struct rtw_dev *rtwdev,
				  struct rtw8822c_dpk_data *data);
static u8 rtw8822c_gl_less_state(struct rtw_dev *rtwdev,
				 struct rtw8822c_dpk_data *data);
static u8 rtw8822c_gl_state(struct rtw_dev *rtwdev,
			    struct rtw8822c_dpk_data *data, u8 is_large);
static u8 rtw8822c_loss_check_state(struct rtw_dev *rtwdev,
				    struct rtw8822c_dpk_data *data);
static void rtw8822c_pa_bias(struct rtw_dev *rtwdev);
static void rtw8822c_parse_tbl_dpk(struct rtw_dev *rtwdev,
			    const struct rtw_table *tbl);
static void rtw8822c_phy_calibration(struct rtw_dev *rtwdev);
static void rtw8822c_power_trim(struct rtw_dev *rtwdev);
static u32 rtw8822c_psd_log2base(u32 val);
static void rtw8822c_pwr_track(struct rtw_dev *rtwdev);
static void rtw8822c_pwr_track_path(struct rtw_dev *rtwdev,
				    struct rtw_swing_table *swing_table,
				    u8 path);
static void rtw8822c_pwr_track_stats(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_pwrtrack_init(struct rtw_dev *rtwdev);
static void rtw8822c_pwrtrack_set(struct rtw_dev *rtwdev, u8 rf_path);
static void rtw8822c_rf_dac_cal(struct rtw_dev *rtwdev);
static void rtw8822c_rf_init(struct rtw_dev *rtwdev);
static void rtw8822c_rf_minmax_cmp(struct rtw_dev *rtwdev, u32 value,
				   u32 *min, u32 *max);
static void rtw8822c_rf_x2_check(struct rtw_dev *rtwdev);
static void rtw8822c_rfk_handshake(struct rtw_dev *rtwdev, bool is_before_k);
static void rtw8822c_rfk_power_save(struct rtw_dev *rtwdev,
				    bool is_power_save);
static void rtw8822c_rstb_3wire(struct rtw_dev *rtwdev, bool enable);
static void rtw8822c_set_channel(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				 u8 primary_chan_idx);
static void rtw8822c_set_channel_bb(struct rtw_dev *rtwdev, u8 channel, u8 bw,
				    u8 primary_ch_idx);
static void rtw8822c_set_channel_rf(struct rtw_dev *rtwdev, u8 channel, u8 bw);
static void rtw8822c_set_power_trim(struct rtw_dev *rtwdev, s8 bb_gain[2][8]);
static void rtw8822c_set_tx_power_diff(struct rtw_dev *rtwdev, u8 rate,
				       s8 *diff_idx);
static void rtw8822c_set_tx_power_index(struct rtw_dev *rtwdev);
static void
rtw8822c_set_write_tx_power_ref(struct rtw_dev *rtwdev, u8 *tx_pwr_ref_cck,
				u8 *tx_pwr_ref_ofdm);
static void rtw8822c_thermal_trim(struct rtw_dev *rtwdev);
static void rtw8822c_toggle_igi(struct rtw_dev *rtwdev);
static void rtw8822c_txgapk(struct rtw_dev *rtwdev);
static void rtw8822c_txgapk_afe_dpk(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_txgapk_afe_dpk_restore(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_txgapk_backup_bb_reg(struct rtw_dev *rtwdev, const u32 reg[],
					  u32 reg_backup[], u32 reg_num);
static void rtw8822c_txgapk_bb_dpk(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_txgapk_bb_dpk_restore(struct rtw_dev *rtwdev, u8 path);
static u32 rtw8822c_txgapk_cal_gain(struct rtw_dev *rtwdev, u32 gain, s8 offset);
static void rtw8822c_txgapk_calculate_offset(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_txgapk_read_offset(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_txgapk_reload_bb_reg(struct rtw_dev *rtwdev,
					  const u32 reg[], u32 reg_backup[],
					  u32 reg_num);
static void rtw8822c_txgapk_rf_restore(struct rtw_dev *rtwdev, u8 path);
static void rtw8822c_txgapk_save_all_tx_gain_table(struct rtw_dev *rtwdev);
static void rtw8822c_txgapk_tx_pause(struct rtw_dev *rtwdev);
static void rtw8822c_txgapk_write_gain_bb_table(struct rtw_dev *rtwdev);
static void rtw8822c_txgapk_write_tx_gain(struct rtw_dev *rtwdev);
static void
rtw_cfg_txpwr_lmt_by_alt(struct rtw_dev *rtwdev, u8 regd, u8 regd_alt);
static int rtw_channel_to_idx(u8 band, u8 channel);
static u8 rtw_get_channel_group(u8 channel, u8 rate);
static void rtw_get_tx_power_params(struct rtw_dev *rtwdev, u8 path, u8 rate, u8 bw,
			     u8 ch, u8 regd, struct rtw_power_params *pwr_param);
static void rtw_parse_tbl_bb_pg(struct rtw_dev *rtwdev, const struct rtw_table *tbl);
static void rtw_parse_tbl_txpwr_lmt(struct rtw_dev *rtwdev,
			     const struct rtw_table *tbl);
static void rtw_phy_config_swing_table(struct rtw_dev *rtwdev,
				struct rtw_swing_table *swing_table);
static s16 rtw_phy_get_2g_tx_power_index(struct rtw_dev *rtwdev,
					struct rtw_2g_txpwr_idx *pwr_idx_2g,
					enum rtw_bandwidth bandwidth,
					u8 rate, u8 group);
static s16 rtw_phy_get_5g_tx_power_index(struct rtw_dev *rtwdev,
					struct rtw_5g_txpwr_idx *pwr_idx_5g,
					enum rtw_bandwidth bandwidth,
					u8 rate, u8 group);
static s8 rtw_phy_get_dis_dpd_by_rate_diff(struct rtw_dev *rtwdev, u16 rate);
static void
rtw_phy_get_rate_values_of_txpwr_by_rate(struct rtw_dev *rtwdev,
					 u32 addr, u32 mask, u32 val, u8 *rate,
					 u8 *pwr_by_rate, u8 *rate_num);
static u8
rtw_phy_get_tx_power_index(struct rtw_dev *rtwdev, u8 rf_path, u8 rate,
			   enum rtw_bandwidth bandwidth, u8 channel, u8 regd);
static s8 rtw_phy_get_tx_power_limit(struct rtw_dev *rtwdev, u8 band,
				     enum rtw_bandwidth bw, u8 rf_path,
				     u8 rate, u8 channel, u8 regd);
static s32 rtw_phy_get_tx_power_sar(struct rtw_dev *rtwdev, u8 sar_band,
				   u8 rf_path, u8 rate);
static void rtw_phy_init_tx_power(struct rtw_dev *rtwdev);
static void rtw_phy_init_tx_power_limit(struct rtw_dev *rtwdev,
					u8 regd, u8 bw, u8 rs);
static void rtw_phy_pwrtrack_avg(struct rtw_dev *rtwdev, u8 thermal, u8 path);
static u8 rtw_phy_pwrtrack_get_delta(struct rtw_dev *rtwdev, u8 path);
static s8 rtw_phy_pwrtrack_get_pwridx(struct rtw_dev *rtwdev,
			       struct rtw_swing_table *swing_table,
			       u8 tbl_path, u8 therm_path, u8 delta);
static bool rtw_phy_pwrtrack_need_iqk(struct rtw_dev *rtwdev);
static bool rtw_phy_pwrtrack_need_lck(struct rtw_dev *rtwdev);
static bool rtw_phy_pwrtrack_thermal_changed(struct rtw_dev *rtwdev, u8 thermal,
				      u8 path);
static u8 rtw_phy_rate_to_rate_section(u8 rate);
static void rtw_phy_set_tx_power_limit(struct rtw_dev *rtwdev, u8 regd, u8 band,
				       u8 bw, u8 rs, u8 ch, s8 pwr_limit);
static void rtw_phy_store_tx_power_by_rate(struct rtw_dev *rtwdev,
					   u32 band, u32 rfpath, u32 txnum,
					   u32 regaddr, u32 bitmask, u32 data);
static void rtw_phy_tx_power_by_rate_config(struct rtw_hal *hal);
static void
rtw_phy_tx_power_by_rate_config_by_path(struct rtw_hal *hal, u8 path,
					u8 rs, u8 size, const u8 *rates);
static void rtw_phy_tx_power_limit_config(struct rtw_hal *hal);
static void rtw_set_channel_mac(struct rtw_dev *rtwdev, u8 channel, u8 bw,
			 u8 primary_ch_idx);
static void
rtw_xref_5g_txpwr_lmt(struct rtw_dev *rtwdev, u8 regd,
		      u8 bw, u8 ch_idx, u8 rs_ht, u8 rs_vht);
static void
rtw_xref_5g_txpwr_lmt_by_ch(struct rtw_dev *rtwdev, u8 regd, u8 bw);
static void rtw_xref_txpwr_lmt(struct rtw_dev *rtwdev);
static void
rtw_xref_txpwr_lmt_by_bw(struct rtw_dev *rtwdev, u8 regd);
static void
rtw_xref_txpwr_lmt_by_rs(struct rtw_dev *rtwdev, u8 regd, u8 bw, u8 ch_idx);
static u8 tbl_to_dec_pwr_by_rate(struct rtw_dev *rtwdev, u32 hex, u8 i);
