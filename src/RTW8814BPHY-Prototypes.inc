/* SPDX-License-Identifier: GPL-2.0-only
 * Generated from tobyw121/Openwrt_RTL@85ef7eb7ccbd4336cd6e628a743ddc211333901c.
 * Vendor notices and primary algorithms preserved; checked host adapter only. */
static void halrf_b_sort(void *dm_void, u32 *iv, u32 *qv);
static void halrf_bp(void *dm_void, u32 *bp_reg, u32 *bp, u32 num);
static void halrf_bprf(void *dm_void, u32 *bp_reg, u32 bp[][4], u32 num, u8 ss);
static void halrf_bubble(void *dm_void, u32 *v1, u32 *v2);
static boolean halrf_compare(void *dm_void, u32 value);
static void halrf_get_fw_version(void *dm_void);
static void halrf_iqk_dbg(void *dm_void);
static void halrf_iqk_hwtx_check(void *dm_void, boolean is_check);
static void halrf_minmax_compare(void *dm_void, u32 value, u32 *min,
     u32 *max);
static void halrf_mode(void *dm_void, u32 *i_value, u32 *q_value);
static u32 halrf_psd_log2base(u32 val);
static void halrf_reload_bp(void *dm_void, u32 *bp_reg, u32 *bp, u32 num);
static void halrf_reload_bprf(void *dm_void, u32 *bp_reg, u32 bp[][4], u32 num,
         u8 ss);
static void halrf_swap(void *dm_void, u32 *v1, u32 *v2);
static void phydm_get_iqk_cfir(void *dm_void, u8 idx, u8 path, boolean debug);
static void config_phydm_cca_dyn_pw_th(struct dm_struct *dm, enum channel_width bw);
static boolean
config_phydm_parameter_init_8814b(struct dm_struct *dm,
      enum odm_parameter_init type);
static void
config_phydm_path_adda_setting_8814b(struct dm_struct *dm);
static u32 config_phydm_read_rf_reg_8814b(struct dm_struct *dm, enum rf_path path,
       u32 reg_addr, u32 bit_mask);
static u32 config_phydm_read_syn_reg_8814b(struct dm_struct *dm, enum rf_syn syn_path,
        u32 reg_addr, u32 bit_mask);
static boolean
config_phydm_switch_band_8814b(struct dm_struct *dm,
          u8 central_ch);
static boolean
config_phydm_switch_bw_8814b(struct dm_struct *dm, u8 pri_ch,
        enum channel_width bandwidth);
static boolean
config_phydm_switch_channel_8814b(struct dm_struct *dm, u8 central_ch);
static boolean
config_phydm_trx_mode_8814b(struct dm_struct *dm, enum bb_path tx_path,
       enum bb_path rx_path);
static boolean
config_phydm_write_rf_reg_8814b(struct dm_struct *dm, enum rf_path path,
    u32 reg_addr, u32 bit_mask, u32 data);
static u32 config_phydm_write_rf_syn_8814b(struct dm_struct *dm,
        enum rf_syn syn_path, u32 reg_addr,
        u32 bit_mask, u32 data);
static boolean
config_phydm_write_txagc_diff_8814b(struct dm_struct *dm, s8 power_index1,
        s8 power_index2, s8 power_index3,
        s8 power_index4, u8 hw_rate);
static boolean
config_phydm_write_txagc_ref_8814b(struct dm_struct *dm, u8 power_index,
       enum rf_path path,
       enum PDM_RATE_TYPE mod_type);
static void
phydm_agc_tab_sel_8814b(struct dm_struct *dm, u8 central_ch);
static void phydm_bb_reset_8814b(struct dm_struct *dm);
static void phydm_ccapar_by_rfe_8814b(struct dm_struct *dm);
static void
phydm_cck_tx_shaping_filter_8814b(struct dm_struct *dm, u8 central_ch);
static u32 phydm_check_bit_mask_8814b(u32 bit_mask, u32 data_original, u32 data);
static boolean
phydm_config_cck_rx_path_8814b(struct dm_struct *dm, enum bb_path rx_path);
static boolean
phydm_config_cck_tx_path_8814b(struct dm_struct *dm, enum bb_path tx_path);
static boolean
phydm_config_ofdm_rx_path_8814b(struct dm_struct *dm,
    enum bb_path rx_path);
static boolean
phydm_config_ofdm_tx_path_8814b(struct dm_struct *dm,
    enum bb_path tx_path);
static boolean
phydm_config_set_rfc_8814b(struct dm_struct *dm, enum rfc_mode mode);
static void phydm_dis_cck_trx_8814b(void *dm_void, u8 set_type);
static void phydm_dyn_dpd_8814b(struct dm_struct *dm, u8 central_ch);
static void phydm_dynamic_spur_det_eliminate_8814b(struct dm_struct *dm);
static void phydm_igi_toggle_8814b(struct dm_struct *dm);
static boolean
phydm_rfe_8814b(struct dm_struct *dm, u8 channel);
static void phydm_rfe_8814b_efem(struct dm_struct *dm, u8 channel);
static void phydm_rfe_8814b_ifem(struct dm_struct *dm, u8 channel);
static void phydm_rfe_8814b_init(struct dm_struct *dm);
static void phydm_rfe_type_6(struct dm_struct *dm, u8 channel);
static void
phydm_sco_trk_fc_setting_8814b(struct dm_struct *dm, u8 central_ch);
static void phydm_set_txagc_by_table_8814b(struct dm_struct *dm,
        struct txagc_table_8814b *tab);
static void odm_config_bb_agc_8814b(struct dm_struct *dm, u32 addr, u32 bitmask,
        u32 data);
static void odm_config_bb_phy_8814b(struct dm_struct *dm, u32 addr, u32 bitmask,
        u32 data);
static void odm_config_rf_radio_a_8814b(struct dm_struct *dm, u32 addr, u32 data);
static void odm_config_rf_radio_b_8814b(struct dm_struct *dm, u32 addr, u32 data);
static void odm_config_rf_radio_c_8814b(struct dm_struct *dm, u32 addr, u32 data);
static void odm_config_rf_radio_d_8814b(struct dm_struct *dm, u32 addr, u32 data);
static void odm_config_rf_radio_syn0_8814b(struct dm_struct *dm, u32 addr, u32 data);
static void odm_config_rf_radio_syn1_8814b(struct dm_struct *dm, u32 addr, u32 data);
static void odm_config_rf_reg_8814b(struct dm_struct *dm, u32 addr, u32 data,
        enum rf_path rf_path, u32 reg_addr);
static void odm_config_syn_reg_8814b(struct dm_struct *dm, u32 addr, u32 data,
         enum rf_syn syn_path, u32 reg_addr);
static boolean
check_positive_halhwimg8814b_bb(struct dm_struct *dm,
        const u32 condition1,
        const u32 condition2,
        const u32 condition3,
        const u32 condition4
);
static void
odm_read_and_config_mp_8814b_agc_tab(struct dm_struct *dm);
static void
odm_read_and_config_mp_8814b_phy_reg(struct dm_struct *dm);
static boolean
check_positive_halhwimg8814b_rf(struct dm_struct *dm,
        const u32 condition1,
        const u32 condition2,
        const u32 condition3,
        const u32 condition4
);
static void
odm_read_and_config_mp_8814b_radioa(struct dm_struct *dm);
static void
odm_read_and_config_mp_8814b_radiob(struct dm_struct *dm);
static void
odm_read_and_config_mp_8814b_radioc(struct dm_struct *dm);
static void
odm_read_and_config_mp_8814b_radiod(struct dm_struct *dm);
static void
odm_read_and_config_mp_8814b_radiosyn0(struct dm_struct *dm);
static void
odm_read_and_config_mp_8814b_radiosyn1(struct dm_struct *dm);
static void _phy_rtc_8814b(struct dm_struct *dm);
static void halrf_dac_cal_8814b(void *dm_void);
static void halrf_dack1_8814b(struct dm_struct *dm, u8 path, u32 *iadc, u32 *qadc,
               u32 *v1, u32 *v2);
static void halrf_dack2_8814b(struct dm_struct *dm, u8 path, u32 *idac, u32 *qdac,
         u32 v1, u32 v2);
static void halrf_dack_reload_8814b(struct dm_struct *dm);
static void halrf_dack_setbb_8814b(struct dm_struct *dm);
static void halrf_get_efuse_thermal_pwrtype_8814b(
 void *dm_void);
static u32 halrf_pathsel1_8814b(struct dm_struct *dm, u8 path);
static u32 halrf_pathsel2_8814b(struct dm_struct *dm, u8 path);
static void halrf_tssi_get_efuse_8814b(
 void *dm_void);
static void phy_lc_calibrate_8814b(void *dm_void);
static void _iq_calibrate_8814b_init(struct dm_struct *dm);
static void _iqk_backup_iqk_8814b(struct dm_struct *dm, u8 step, u8 path);
static void _iqk_backup_iqk_8814b_subfunction(struct dm_struct *dm);
static void _iqk_bp_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
    u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg);
static void _iqk_bp_rf_8814b(struct dm_struct *dm, u32 rf_bp[][SS_8814B],
        u32 *bp_reg);
static void _iqk_cal_path_off_8814b(struct dm_struct *dm);
static boolean
_iqk_check_cal_8814b(struct dm_struct *dm, u8 path, u8 cmd);
static void _iqk_clean_cfir_8814b(struct dm_struct *dm, u8 mode, u8 path);
static void _iqk_fail_count_8814b(void *dm_void);
static void _iqk_fill_iqk_report_8814b(void *dm_void, u8 ch);
static void _iqk_iqk_by_path_8814b(void *dm_void, boolean segment_iqk);
static void _iqk_lok_setting_8814b(struct dm_struct *dm, u8 path);
static void _iqk_lok_tune_8814b(void *dm_void, u8 path);
static boolean
_iqk_nbtxk_oneshot_8814b(void *dm_void, u8 path);
static boolean
_iqk_one_shot_8814b(void *dm_void, u8 path, u8 idx);
static void _iqk_reload_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
        u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg);
static void _iqk_reload_rf_8814b(struct dm_struct *dm, u32 *reg,
      u32 data[][SS_8814B]);
static boolean
_iqk_rx_iqk_by_path_8814b(void *dm_void, u8 path);
static void _iqk_rxiqk(void *dm_void, u8 rf_path);
static void _iqk_rxk2_setting_8814b(struct dm_struct *dm, u8 path, boolean is_gs);
static boolean
_iqk_rxk_gsearch_fail_8814b(struct dm_struct *dm, u8 path, u8 step);
static void _iqk_set_afe_8814b(struct dm_struct *dm);
static void _iqk_set_macbb_8814b(struct dm_struct *dm);
static void _iqk_set_rfe_8814b(struct dm_struct *dm);
static void _iqk_start_iqk_8814b(struct dm_struct *dm, boolean segment_iqk);
static void _iqk_summy_report_8814b(void *dm_void);
static void _iqk_txcfir_rate_en_8814b(struct dm_struct *dm);
static void _iqk_txk_setting_8814b(struct dm_struct *dm, u8 path);
static boolean
_lok_check(void *dm_void, u8 path);
static boolean
_lok_one_shot_8814b(void *dm_void, u8 path, boolean for_rxk);
static void _phy_iq_calibrate_8814b(struct dm_struct *dm, boolean reset,
        boolean segment_iqk);
static void phy_get_iqk_cfir_8814b(void *dm_void, u8 idx, u8 path, boolean debug);
static void phy_iq_calibrate_8814b(void *dm_void, boolean clear, boolean segment_iqk);
static boolean _do_dpk_8814b(void *dm_void);
static void _dpk_bp_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
    u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg);
static void _dpk_bp_rf_8814b(struct dm_struct *dm, u32 rf_bp[][SS_8814B],
        u32 *bp_reg);
static boolean _dpk_by_path_8814b(struct dm_struct *dm, u8 tx_agc_search, u8 path,
         u8 group);
static boolean _dpk_check_fail_8814b(struct dm_struct *dm, boolean is_fail, u8 path,
    u8 group);
static boolean
_dpk_check_nctl_done_8814b(struct dm_struct *dm, u8 path, u32 IQK_CMD);
static void _dpk_con_tx_8814b(struct dm_struct *dm, boolean is_contx);
static u8 _dpk_gainloss_8814b(struct dm_struct *dm, u8 path);
static void _dpk_init_8814b(struct dm_struct *dm, boolean start_dpk);
static boolean _dpk_lut_sram_read_8814b(void *dm_void, u8 group, u8 path);
static void _dpk_lut_sram_write_8814b(void *dm_void, u8 group, u8 path);
static void _dpk_mac_bb_setting_8814b(struct dm_struct *dm);
static void _dpk_manual_txagc_8814b(struct dm_struct *dm, boolean is_manual);
static void _dpk_on_8814b(struct dm_struct *dm, u8 path);
static boolean _dpk_one_shot_8814b(struct dm_struct *dm, u8 path, u8 action);
static boolean _dpk_pas_agc_8814b(
 struct dm_struct *dm,
 u8 path,
 u8 gs);
static void _dpk_pas_dpk_read_8814b(struct dm_struct *dm, u8 path);
static u32 _dpk_pas_get_iq_8814b(
 struct dm_struct *dm,
 u8 gs_type);
static void _dpk_pas_gl_read_8814b(struct dm_struct *dm, u8 path);
static s32 _dpk_pas_iq_check_8814b(
 struct dm_struct *dm,
 u8 path,
 u8 gs_type);
static boolean _dpk_pas_rxbbagc_8814b(
 struct dm_struct *dm,
 u8 path,
 u8 gs_type);
static void _dpk_reload_8814b(void *dm_void, u8 group, u8 path);
static void _dpk_reload_macbb_8814b(struct dm_struct *dm, u32 *mac_bp,
        u32 *bb_bp, u32 *bp_mac_reg, u32 *bp_bb_reg);
static void _dpk_reload_rf_8814b(struct dm_struct *dm, u32 *reg,
      u32 data[][SS_8814B]);
static void _dpk_rfe_setting_8814b(struct dm_struct *dm);
static void _dpk_rfsetting_8814b(struct dm_struct *dm, u8 path);
static void _dpk_set_afe_8814b(struct dm_struct *dm, boolean is_do_dpk);
static void _dpk_thermal_read_8814b(
 void *dm_void,
 u8 path);
static void do_dpk_8814b(void *dm_void);
static void dpk_by_group_8814b(void *dm_void);
static u8 dpk_channel_transfer_8814b(void *dm_void);
static boolean dpk_get_dpkbychannel_8814b(void *dm_void);
static void dpk_result_summary_8814b(void *dm_void);
static void dpk_thermal_read_8814b(void *dm_void);
static void dpk_track_8814b(void *dm_void);
static void _phy_txgapk_calibrate_8814b(void *dm_void, boolean is_cck);
static void _txgapk_afe_setting_8814b(struct dm_struct *dm, boolean do_txgapk);
static void _txgapk_bp_rf_gain_table_8814b(struct dm_struct *dm);
static u64 _txgapk_one_shot_8814b(struct dm_struct *dm, u8 path);
static void _txgapk_phy_8814b(struct dm_struct *dm, u8 path);
static void _txgapk_phy_restore_8814b(struct dm_struct *dm);
static void _txgapk_reload_rf_gain_table_8814b(struct dm_struct *dm, boolean is_cck, u8 rf_path);
static void _txgapk_rf_restore_8814b(struct dm_struct *dm);
static void _txgapk_rf_setting_8814b(struct dm_struct *dm,
         u8 path);
static void phy_txgap_calibrate_8814b(void *dm_void,
          boolean clear);
static void odm_read_and_config_mp_8814b_cal_init(void *dm_void);
