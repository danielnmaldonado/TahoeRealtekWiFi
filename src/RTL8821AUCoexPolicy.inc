/* SPDX-License-Identifier: BSD-3-Clause */
/* Pinned Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation.
 * Dual GPL-2.0 OR BSD-3-Clause; this port selects BSD-3-Clause.
 * See RTL8821AUCoex-Upstream.json and RTL8812AUPHY-LICENSE.txt. */
static u8 rtw_coex_next_rssi_state(struct rtw_dev *rtwdev, u8 pre_state,
				   u8 rssi, u8 rssi_thresh)
{
	(void)rtwdev;
	(void)pre_state;
	(void)rssi;
	(void)rssi_thresh;
	const struct rtw_chip_info *chip = rtwdev->chip;
	u8 tol = chip->rssi_tolerance;
	u8 next_state;

	if (pre_state == COEX_RSSI_STATE_LOW ||
	    pre_state == COEX_RSSI_STATE_STAY_LOW) {
		if (rssi >= (rssi_thresh + tol))
			next_state = COEX_RSSI_STATE_HIGH;
		else
			next_state = COEX_RSSI_STATE_STAY_LOW;
	} else {
		if (rssi < rssi_thresh)
			next_state = COEX_RSSI_STATE_LOW;
		else
			next_state = COEX_RSSI_STATE_STAY_HIGH;
	}

	return next_state;
}

static void rtw_coex_limited_tx(struct rtw_dev *rtwdev,
				bool tx_limit_en, bool ampdu_limit_en)
{
	(void)rtwdev;
	(void)tx_limit_en;
	(void)ampdu_limit_en;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 num_of_active_port = 1;

	if (!chip->scbd_support)
		return;

	/* force max tx retry limit = 8 */
	if (coex_stat->wl_tx_limit_en == tx_limit_en &&
	    coex_stat->wl_ampdu_limit_en == ampdu_limit_en)
		return;

	if (!coex_stat->wl_tx_limit_en) {
		coex_stat->darfrc = rtw_read32(rtwdev, REG_DARFRC);
		coex_stat->darfrch = rtw_read32(rtwdev, REG_DARFRCH);
		coex_stat->retry_limit = rtw_read16(rtwdev, REG_RETRY_LIMIT);
	}

	if (!coex_stat->wl_ampdu_limit_en)
		coex_stat->ampdu_max_time =
				rtw_read8(rtwdev, REG_AMPDU_MAX_TIME_V1);

	coex_stat->wl_tx_limit_en = tx_limit_en;
	coex_stat->wl_ampdu_limit_en = ampdu_limit_en;

	if (tx_limit_en) {
		/* set BT polluted packet on for tx rate adaptive,
		 * not including tx retry broken by PTA
		 */
		rtw_write8_set(rtwdev, REG_TX_HANG_CTRL, BIT_EN_GNT_BT_AWAKE);

		/* set queue life time to avoid can't reach tx retry limit
		 * if tx is always broken by GNT_BT
		 */
		if (num_of_active_port <= 1)
			rtw_write8_set(rtwdev, REG_LIFETIME_EN, 0xf);
		rtw_write16(rtwdev, REG_RETRY_LIMIT, 0x0808);

		/* auto rate fallback step within 8 retries */
		rtw_write32(rtwdev, REG_DARFRC, 0x1000000);
		rtw_write32(rtwdev, REG_DARFRCH, 0x4030201);
	} else {
		rtw_write8_clr(rtwdev, REG_TX_HANG_CTRL, BIT_EN_GNT_BT_AWAKE);
		rtw_write8_clr(rtwdev, REG_LIFETIME_EN, 0xf);

		rtw_write16(rtwdev, REG_RETRY_LIMIT, coex_stat->retry_limit);
		rtw_write32(rtwdev, REG_DARFRC, coex_stat->darfrc);
		rtw_write32(rtwdev, REG_DARFRCH, coex_stat->darfrch);
	}

	if (ampdu_limit_en)
		rtw_write8(rtwdev, REG_AMPDU_MAX_TIME_V1, 0x20);
	else
		rtw_write8(rtwdev, REG_AMPDU_MAX_TIME_V1,
			   coex_stat->ampdu_max_time);
}

static void rtw_coex_limited_wl(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	bool tx_limit = false;
	bool tx_agg_ctrl = false;

	if (!coex->under_5g && coex_dm->bt_status != COEX_BTSTATUS_NCON_IDLE) {
		tx_limit = true;
		tx_agg_ctrl = true;
	}

	rtw_coex_limited_tx(rtwdev, tx_limit, tx_agg_ctrl);
}

static bool rtw_coex_freerun_check(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 bt_rssi;
	u8 ant_distance = 10;

	if (coex_stat->bt_disabled)
		return false;

	if (efuse->share_ant || ant_distance <= 5 || !coex_stat->wl_gl_busy)
		return false;

	if (ant_distance >= 40 || coex_stat->bt_hid_pair_num >= 2)
		return true;

	/* ant_distance = 5 ~ 40  */
	if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[1]) &&
	    COEX_RSSI_HIGH(coex_dm->bt_rssi_state[0]))
		return true;

	if (coex_stat->wl_tput_dir == COEX_WL_TPUT_TX)
		bt_rssi = coex_dm->bt_rssi_state[0];
	else
		bt_rssi = coex_dm->bt_rssi_state[1];

	if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[3]) &&
	    COEX_RSSI_HIGH(bt_rssi) &&
	    coex_stat->cnt_wl[COEX_CNT_WL_SCANAP] <= 5)
		return true;

	return false;
}

static void rtw_coex_wl_slot_extend(struct rtw_dev *rtwdev, bool enable)
{
	(void)rtwdev;
	(void)enable;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 para[6] = {0};

	para[0] = COEX_H2C69_WL_LEAKAP;
	para[1] = PARA1_H2C69_DIS_5MS;

	if (enable)
		para[1] = PARA1_H2C69_EN_5MS;
	else
		coex_stat->cnt_wl[COEX_CNT_WL_5MS_NOEXTEND] = 0;

	coex_stat->wl_slot_extend = enable;
	rtw_fw_bt_wifi_control(rtwdev, para[0], &para[1]);
}

static void rtw_coex_wl_ccklock_action(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex->manual_control || coex->stop_dm)
		return;


	if (coex_stat->tdma_timer_base == 3 && coex_stat->wl_slot_extend) {
		(void)0;
		rtw_coex_wl_slot_extend(rtwdev, false);
		return;
	}

	if (coex_stat->wl_slot_extend && coex_stat->wl_force_lps_ctrl &&
	    !coex_stat->wl_cck_lock_ever) {
		if (coex_stat->wl_fw_dbg_info[7] <= 5)
			coex_stat->cnt_wl[COEX_CNT_WL_5MS_NOEXTEND]++;
		else
			coex_stat->cnt_wl[COEX_CNT_WL_5MS_NOEXTEND] = 0;

		(void)0;

		if (coex_stat->cnt_wl[COEX_CNT_WL_5MS_NOEXTEND] == 7) {
			(void)0;
			rtw_coex_wl_slot_extend(rtwdev, false);
		}
	} else if (!coex_stat->wl_slot_extend && coex_stat->wl_cck_lock) {
		(void)0;

		rtw_coex_wl_slot_extend(rtwdev, true);
	}
}

static void rtw_coex_wl_ccklock_detect(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;

	bool is_cck_lock_rate = false;

	if (coex_stat->wl_coex_mode != COEX_WLINK_2G1PORT &&
	    coex_stat->wl_coex_mode != COEX_WLINK_2GFREE)
		return;

	if (coex_dm->bt_status == COEX_BTSTATUS_INQ_PAGE ||
	    coex_stat->bt_setup_link) {
		coex_stat->wl_cck_lock = false;
		coex_stat->wl_cck_lock_pre = false;
		return;
	}

	if (coex_stat->wl_rx_rate <= COEX_CCK_2 ||
	    coex_stat->wl_rts_rx_rate <= COEX_CCK_2)
		is_cck_lock_rate = true;

	if (coex_stat->wl_connected && coex_stat->wl_gl_busy &&
	    COEX_RSSI_HIGH(coex_dm->wl_rssi_state[3]) &&
	    (coex_dm->bt_status == COEX_BTSTATUS_ACL_BUSY ||
	     coex_dm->bt_status == COEX_BTSTATUS_ACL_SCO_BUSY ||
	     coex_dm->bt_status == COEX_BTSTATUS_SCO_BUSY)) {
		if (is_cck_lock_rate) {
			coex_stat->wl_cck_lock = true;

			(void)0;

		} else {
			coex_stat->wl_cck_lock = false;

			(void)0;
		}
	} else {
		coex_stat->wl_cck_lock = false;
	}

	/* CCK lock identification */
	if (coex_stat->wl_cck_lock && !coex_stat->wl_cck_lock_pre)
		ieee80211_queue_delayed_work(rtwdev->hw, &coex->wl_ccklock_work,
					     3 * HZ);

	coex_stat->wl_cck_lock_pre = coex_stat->wl_cck_lock;
}

static void rtw_coex_wl_noisy_detect(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u32 cnt_cck;
	bool wl_cck_lock = false;

	/* wifi noisy environment identification */
	cnt_cck = dm_info->cck_ok_cnt + dm_info->cck_err_cnt;

	if (!coex_stat->wl_gl_busy && !wl_cck_lock) {
		if (cnt_cck > 250) {
			if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY2] < 5)
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY2]++;

			if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY2] == 5) {
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY0] = 0;
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY1] = 0;
			}
		} else if (cnt_cck < 100) {
			if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY0] < 5)
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY0]++;

			if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY0] == 5) {
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY1] = 0;
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY2] = 0;
			}
		} else {
			if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY1] < 5)
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY1]++;

			if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY1] == 5) {
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY0] = 0;
				coex_stat->cnt_wl[COEX_CNT_WL_NOISY2] = 0;
			}
		}

		if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY2] == 5)
			coex_stat->wl_noisy_level = 2;
		else if (coex_stat->cnt_wl[COEX_CNT_WL_NOISY1] == 5)
			coex_stat->wl_noisy_level = 1;
		else
			coex_stat->wl_noisy_level = 0;

		(void)0;
	}
}

static void rtw_coex_tdma_timer_base(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 para[6] = {0};
	u8 times;
	u16 tbtt_interval = coex_stat->wl_beacon_interval;

	if (coex_stat->tdma_timer_base == type)
		return;

	coex_stat->tdma_timer_base = type;

	para[0] = COEX_H2C69_TDMA_SLOT;

	(void)0;

	if (type == TDMA_TIMER_TYPE_4SLOT && tbtt_interval < 120) {
		para[1] = PARA1_H2C69_TDMA_4SLOT; /* 4-slot */
	} else if (tbtt_interval < 80 && tbtt_interval > 0) {
		times = 100 / tbtt_interval;
		if (100 % tbtt_interval != 0)
			times++;

		para[1] = FIELD_PREP(PARA1_H2C69_TBTT_TIMES, times);
	} else if (tbtt_interval >= 180) {
		times = tbtt_interval / 100;
		if (tbtt_interval % 100 <= 80)
			times--;

		para[1] = FIELD_PREP(PARA1_H2C69_TBTT_TIMES, times) |
			  FIELD_PREP(PARA1_H2C69_TBTT_DIV100, 1);
	} else {
		para[1] = PARA1_H2C69_TDMA_2SLOT;
	}

	rtw_fw_bt_wifi_control(rtwdev, para[0], &para[1]);

	(void)0;

	/* no 5ms_wl_slot_extend for 4-slot mode  */
	if (coex_stat->tdma_timer_base == 3)
		rtw_coex_wl_ccklock_action(rtwdev);
}

static void rtw_coex_set_wl_pri_mask(struct rtw_dev *rtwdev, u8 bitmap,
				     u8 data)
{
	(void)rtwdev;
	(void)bitmap;
	(void)data;
	u32 addr;

	addr = REG_BT_COEX_TABLE_H + (bitmap / 8);
	bitmap = bitmap % 8;

	rtw_write8_mask(rtwdev, addr, BIT(bitmap), data);
}

static void rtw_coex_write_scbd(struct rtw_dev *rtwdev, u16 bitpos, bool set)
{
	(void)rtwdev;
	(void)bitpos;
	(void)set;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u16 val = 0x2;

	if (!chip->scbd_support)
		return;

	val |= coex_stat->score_board;

	/* for 8822b, scbd[10] is CQDDR on
	 * for 8822c, scbd[10] is no fix 2M
	 */
	if (!chip->new_scbd10_def && (bitpos & COEX_SCBD_FIX2M)) {
		if (set)
			val &= ~COEX_SCBD_FIX2M;
		else
			val |= COEX_SCBD_FIX2M;
	} else {
		if (set)
			val |= bitpos;
		else
			val &= ~bitpos;
	}

	if (val != coex_stat->score_board) {
		coex_stat->score_board = val;
		val |= BIT_BT_INT_EN;
		rtw_write16(rtwdev, REG_WIFI_BT_INFO, val);
	}
}


static u16 rtw_coex_read_scbd(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;

	if (!chip->scbd_support)
		return 0;

	return (rtw_read16(rtwdev, REG_WIFI_BT_INFO)) & ~(BIT_BT_INT_EN);
}

static void rtw_coex_check_rfk(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	u8 cnt = 0;
	u32 wait_cnt;
	bool btk, wlk;

	if (coex_rfe->wlg_at_btg && chip->scbd_support &&
	    coex_stat->bt_iqk_state != 0xff) {
		(void)0;

		wait_cnt = COEX_RFK_TIMEOUT / COEX_MIN_DELAY;
		do {
			/* BT RFK */
			btk = !!(rtw_coex_read_scbd(rtwdev) & COEX_SCBD_BT_RFK);

			/* WL RFK */
			wlk = !!(rtw_read8(rtwdev, REG_ARFR4) & BIT_WL_RFK);

			if (!btk && !wlk)
				break;

			(void)0;

			mdelay(COEX_MIN_DELAY);
		} while (++cnt < wait_cnt);

		if (cnt >= wait_cnt)
			coex_stat->bt_iqk_state = 0xff;
	}
}

static void rtw_coex_query_bt_info(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex_stat->bt_disabled)
		return;

	(void)0;

	rtw_fw_query_bt_info(rtwdev);
}

static void rtw_coex_gnt_workaround(struct rtw_dev *rtwdev, bool force, u8 mode)
{
	(void)rtwdev;
	(void)force;
	(void)mode;
	rtw_coex_set_gnt_fix(rtwdev);
}

static void rtw_coex_monitor_bt_ctr(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u32 tmp;

	tmp = rtw_read32(rtwdev, REG_BT_ACT_STATISTICS);
	coex_stat->hi_pri_tx = FIELD_GET(MASKLWORD, tmp);
	coex_stat->hi_pri_rx = FIELD_GET(MASKHWORD, tmp);

	tmp = rtw_read32(rtwdev, REG_BT_ACT_STATISTICS_1);
	coex_stat->lo_pri_tx = FIELD_GET(MASKLWORD, tmp);
	coex_stat->lo_pri_rx = FIELD_GET(MASKHWORD, tmp);

	rtw_write8(rtwdev, REG_BT_COEX_ENH_INTR_CTRL,
		   BIT_R_GRANTALL_WLMASK | BIT_STATIS_BT_EN);

	(void)0;

	if (coex_stat->wl_under_lps || coex_stat->wl_under_ips ||
	    (coex_stat->hi_pri_rx > 60000 && coex_stat->hi_pri_tx == 60000 &&
	     coex_stat->lo_pri_rx > 60000 && coex_stat->lo_pri_tx == 60000))
		coex_stat->bt_ctr_ok = false;
	else
		coex_stat->bt_ctr_ok = true;
}

static void rtw_coex_monitor_bt_enable(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	bool bt_disabled = false;
	bool bt_active = true;
	u16 score_board;

	if (chip->scbd_support) {
		score_board = rtw_coex_read_scbd(rtwdev);
		bt_disabled = !(score_board & COEX_SCBD_ONOFF);
	} else {
		if (coex_stat->hi_pri_tx == 0 && coex_stat->hi_pri_rx == 0 &&
		    coex_stat->lo_pri_tx == 0 && coex_stat->lo_pri_rx == 0)
			bt_active = false;

		if (coex_stat->hi_pri_tx == 0xffff && coex_stat->hi_pri_rx == 0xffff &&
		    coex_stat->lo_pri_tx == 0xffff && coex_stat->lo_pri_rx == 0xffff)
			bt_active = false;

		if (bt_active) {
			coex_stat->bt_disable_cnt = 0;
			bt_disabled = false;
		} else {
			if (coex_stat->bt_disable_cnt < 10) coex_stat->bt_disable_cnt++;
			if (coex_stat->bt_disable_cnt >= 10)
				bt_disabled = true;
		}
	}

	if (coex_stat->bt_disabled != bt_disabled) {
		(void)0;

		coex_stat->bt_disabled = bt_disabled;
		coex_stat->bt_ble_scan_type = 0;
		coex_dm->cur_bt_lna_lvl = 0;

		if (!coex_stat->bt_disabled) {
			coex_stat->bt_reenable = true;
			ieee80211_queue_delayed_work(rtwdev->hw,
						     &coex->bt_reenable_work,
						     15 * HZ);
		} else {
			coex_stat->bt_mailbox_reply = false;
			coex_stat->bt_reenable = false;
		}
	}
}

static void rtw_coex_update_wl_link_info(struct rtw_dev *rtwdev, u8 reason)
{
	(void)rtwdev;
	(void)reason;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_traffic_stats *stats = &rtwdev->stats;
	bool is_5G = false;
	bool wl_busy = false;
	bool scan = false, link = false;
	int i;
	u8 rssi_state;
	u8 rssi_step;
	u8 rssi;

	scan = test_bit(RTW_FLAG_SCANNING, rtwdev->flags);
	coex_stat->wl_connected = !!rtwdev->sta_cnt;

	wl_busy = test_bit(RTW_FLAG_BUSY_TRAFFIC, rtwdev->flags);
	if (wl_busy != coex_stat->wl_gl_busy) {
		if (wl_busy)
			coex_stat->wl_gl_busy = true;
		else
			ieee80211_queue_delayed_work(rtwdev->hw,
						     &coex->wl_remain_work,
						     12 * HZ);
	}

	if (stats->tx_throughput > stats->rx_throughput)
		coex_stat->wl_tput_dir = COEX_WL_TPUT_TX;
	else
		coex_stat->wl_tput_dir = COEX_WL_TPUT_RX;

	if (scan || link || reason == COEX_RSN_2GCONSTART ||
	    reason == COEX_RSN_2GSCANSTART || reason == COEX_RSN_2GSWITCHBAND)
		coex_stat->wl_linkscan_proc = true;
	else
		coex_stat->wl_linkscan_proc = false;

	rtw_coex_wl_noisy_detect(rtwdev);

	for (i = 0; i < 4; i++) {
		rssi_state = coex_dm->wl_rssi_state[i];
		rssi_step = chip->wl_rssi_step[i];
		rssi = rtwdev->dm_info.min_rssi;
		rssi_state = rtw_coex_next_rssi_state(rtwdev, rssi_state,
						      rssi, rssi_step);
		coex_dm->wl_rssi_state[i] = rssi_state;
	}

	if (coex_stat->wl_linkscan_proc || coex_stat->wl_hi_pri_task1 ||
	    coex_stat->wl_hi_pri_task2 || coex_stat->wl_gl_busy)
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_SCAN, true);
	else
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_SCAN, false);

	switch (reason) {
	case COEX_RSN_5GSCANSTART:
	case COEX_RSN_5GSWITCHBAND:
	case COEX_RSN_5GCONSTART:

		is_5G = true;
		break;
	case COEX_RSN_2GSCANSTART:
	case COEX_RSN_2GSWITCHBAND:
	case COEX_RSN_2GCONSTART:

		is_5G = false;
		break;
	default:
		if (rtwdev->hal.current_band_type == RTW_BAND_5G)
			is_5G = true;
		else
			is_5G = false;
		break;
	}

	coex->under_5g = is_5G;
}

static inline u8 *get_payload_from_coex_resp(struct sk_buff *resp)
{
	(void)resp;
	struct rtw_c2h_cmd *c2h;
	u32 pkt_offset;

	pkt_offset = *((u32 *)resp->cb);
	c2h = (struct rtw_c2h_cmd *)(resp->data + pkt_offset);

	return c2h->payload;
}
static void rtw_coex_info_response(struct rtw_dev *rtwdev,struct sk_buff *skb){
	(void)rtwdev;
	(void)skb;cx_info_response(rtwdev,skb);}

static struct sk_buff *rtw_coex_info_request(struct rtw_dev *rtwdev,struct rtw_coex_info_req *req){
	(void)rtwdev;
	(void)req;return cx_info_request(rtwdev,req);}


static bool rtw_coex_get_bt_scan_type(struct rtw_dev *rtwdev, u8 *scan_type)
{
	(void)rtwdev;
	(void)scan_type;
	struct rtw_coex_info_req req = {0};
	struct sk_buff *skb;
	u8 *payload;

	req.op_code = BT_MP_INFO_OP_SCAN_TYPE;
	skb = rtw_coex_info_request(rtwdev, &req);
	if (!skb)
		return false;

	payload = get_payload_from_coex_resp(skb);
	*scan_type = GET_COEX_RESP_BT_SCAN_TYPE(payload);
	dev_kfree_skb_any(skb);
	return true;
}

static bool rtw_coex_set_lna_constrain_level(struct rtw_dev *rtwdev,
					     u8 lna_constrain_level)
{
	(void)rtwdev;
	(void)lna_constrain_level;
	struct rtw_coex_info_req req = {0};
	struct sk_buff *skb;

	req.op_code = BT_MP_INFO_OP_LNA_CONSTRAINT;
	req.para1 = lna_constrain_level;
	skb = rtw_coex_info_request(rtwdev, &req);
	if (!skb)
		return false;

	dev_kfree_skb_any(skb);
	return true;
}

#define case_BTSTATUS(src) \
	case COEX_BTSTATUS_##src: return #src


static void rtw_coex_update_bt_link_info(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	u8 i;
	u8 rssi_state;
	u8 rssi_step;
	u8 rssi;

	/* update wl/bt rssi by btinfo */
	for (i = 0; i < COEX_RSSI_STEP; i++) {
		rssi_state = coex_dm->bt_rssi_state[i];
		rssi_step = chip->bt_rssi_step[i];
		rssi = coex_stat->bt_rssi;
		rssi_state = rtw_coex_next_rssi_state(rtwdev, rssi_state, rssi,
						      rssi_step);
		coex_dm->bt_rssi_state[i] = rssi_state;
	}

	if (coex_stat->bt_ble_scan_en &&
	    coex_stat->cnt_bt[COEX_CNT_BT_INFOUPDATE] % 3 == 0) {
		u8 scan_type;

		if (rtw_coex_get_bt_scan_type(rtwdev, &scan_type)) {
			coex_stat->bt_ble_scan_type = scan_type;
			if ((coex_stat->bt_ble_scan_type & 0x1) == 0x1)
				coex_stat->bt_init_scan = true;
			else
				coex_stat->bt_init_scan = false;
		}
	}

	coex_stat->bt_profile_num = 0;

	/* set link exist status */
	if (!(coex_stat->bt_info_lb2 & COEX_INFO_CONNECTION)) {
		coex_stat->bt_link_exist = false;
		coex_stat->bt_pan_exist = false;
		coex_stat->bt_a2dp_exist = false;
		coex_stat->bt_hid_exist = false;
		coex_stat->bt_hfp_exist = false;
	} else {
		/* connection exists */
		coex_stat->bt_link_exist = true;
		if (coex_stat->bt_info_lb2 & COEX_INFO_FTP) {
			coex_stat->bt_pan_exist = true;
			coex_stat->bt_profile_num++;
		} else {
			coex_stat->bt_pan_exist = false;
		}

		if (coex_stat->bt_info_lb2 & COEX_INFO_A2DP) {
			coex_stat->bt_a2dp_exist = true;
			coex_stat->bt_profile_num++;
		} else {
			coex_stat->bt_a2dp_exist = false;
		}

		if (coex_stat->bt_info_lb2 & COEX_INFO_HID) {
			coex_stat->bt_hid_exist = true;
			coex_stat->bt_profile_num++;
		} else {
			coex_stat->bt_hid_exist = false;
		}

		if (coex_stat->bt_info_lb2 & COEX_INFO_SCO_ESCO) {
			coex_stat->bt_hfp_exist = true;
			coex_stat->bt_profile_num++;
		} else {
			coex_stat->bt_hfp_exist = false;
		}
	}

	if (coex_stat->bt_info_lb2 & COEX_INFO_INQ_PAGE) {
		coex_dm->bt_status = COEX_BTSTATUS_INQ_PAGE;
	} else if (!(coex_stat->bt_info_lb2 & COEX_INFO_CONNECTION)) {
		coex_dm->bt_status = COEX_BTSTATUS_NCON_IDLE;
		coex_stat->bt_multi_link_remain = false;
	} else if (coex_stat->bt_info_lb2 == COEX_INFO_CONNECTION) {
		coex_dm->bt_status = COEX_BTSTATUS_CON_IDLE;
	} else if ((coex_stat->bt_info_lb2 & COEX_INFO_SCO_ESCO) ||
		   (coex_stat->bt_info_lb2 & COEX_INFO_SCO_BUSY)) {
		if (coex_stat->bt_info_lb2 & COEX_INFO_ACL_BUSY)
			coex_dm->bt_status = COEX_BTSTATUS_ACL_SCO_BUSY;
		else
			coex_dm->bt_status = COEX_BTSTATUS_SCO_BUSY;
	} else if (coex_stat->bt_info_lb2 & COEX_INFO_ACL_BUSY) {
		coex_dm->bt_status = COEX_BTSTATUS_ACL_BUSY;
	} else {
		coex_dm->bt_status = COEX_BTSTATUS_MAX;
	}

	coex_stat->cnt_bt[COEX_CNT_BT_INFOUPDATE]++;

	(void)0;
}

static void rtw_coex_update_wl_ch_info(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_coex_dm *coex_dm = &rtwdev->coex.dm;
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;
	u8 link = 0;
	u8 center_chan = 0;
	u8 bw;
	int i;

	bw = rtwdev->hal.current_band_width;

	if (type != COEX_MEDIA_DISCONNECT)
		center_chan = rtwdev->hal.current_channel;

	if (center_chan == 0 ||
	    (efuse->share_ant && center_chan <= 14 &&
	     coex_stat->wl_coex_mode != COEX_WLINK_2GFREE)) {
		link = 0;
		center_chan = 0;
		bw = 0;
	} else if (center_chan <= 14) {
		link = 0x1;

		if (bw == RTW_CHANNEL_WIDTH_40)
			bw = chip->bt_afh_span_bw40;
		else
			bw = chip->bt_afh_span_bw20;
	} else if (chip->afh_5g_num > 1) {
		for (i = 0; i < chip->afh_5g_num; i++) {
			if (center_chan == chip->afh_5g[i].wl_5g_ch) {
				link = 0x3;
				center_chan = chip->afh_5g[i].bt_skip_ch;
				bw = chip->afh_5g[i].bt_skip_span;
				break;
			}
		}
	}

	coex_dm->wl_ch_info[0] = link;
	coex_dm->wl_ch_info[1] = center_chan;
	coex_dm->wl_ch_info[2] = bw;

	rtw_fw_wl_ch_info(rtwdev, link, center_chan, bw);
	(void)0;
}

static void rtw_coex_set_bt_tx_power(struct rtw_dev *rtwdev, u8 bt_pwr_dec_lvl)
{
	(void)rtwdev;
	(void)bt_pwr_dec_lvl;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;

	if (bt_pwr_dec_lvl == coex_dm->cur_bt_pwr_lvl)
		return;

	coex_dm->cur_bt_pwr_lvl = bt_pwr_dec_lvl;

	rtw_fw_force_bt_tx_power(rtwdev, bt_pwr_dec_lvl);
}
static void rtw_coex_set_bt_rx_gain(struct rtw_dev *rtwdev, u8 bt_lna_lvl)
{
	(void)rtwdev;
	(void)bt_lna_lvl;
 struct rtw_coex_dm *coex_dm = &rtwdev->coex.dm;

 if (rtwdev->error || !rtwdev->active)
  return;

 /* The primary blocking MP wait admitted every LNA request before caching
  * this level. A busy asynchronous slot retains only the latest decision. */
 rtwdev->mp.lna_desired = bt_lna_lvl;
 rtwdev->mp.lna_pending = false;
 if (bt_lna_lvl == coex_dm->cur_bt_lna_lvl)
  return;

 if (bt_lna_lvl < 7) {
  if (rtwdev->mp.pending && rtwdev->now_ms < rtwdev->mp.deadline) {
   rtwdev->mp.lna_pending = true;
   return;
  }
  rtw_coex_set_lna_constrain_level(rtwdev, bt_lna_lvl);
  if (rtwdev->error)
   return;
  rtw_coex_write_scbd(rtwdev, COEX_SCBD_RXGAIN, true);
 } else {
  rtw_coex_write_scbd(rtwdev, COEX_SCBD_RXGAIN, false);
 }
 /* Like the primary API, successful H2C admission is sufficient: the ACK's
  * return value was ignored there. Failed or deferred writes never apply. */
 if (!rtwdev->error)
  coex_dm->cur_bt_lna_lvl = bt_lna_lvl;
}


static void rtw_coex_set_rf_para(struct rtw_dev *rtwdev,
				 struct coex_rf_para para)
{
	(void)rtwdev;
	(void)para;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 offset = 0;

	if (coex->freerun && coex_stat->cnt_wl[COEX_CNT_WL_SCANAP] <= 5)
		offset = 3;

	rtw_coex_set_wl_tx_power(rtwdev, para.wl_pwr_dec_lvl);
	rtw_coex_set_bt_tx_power(rtwdev, para.bt_pwr_dec_lvl + offset);
	rtw_coex_set_wl_rx_gain(rtwdev, para.wl_low_gain_en);
	rtw_coex_set_bt_rx_gain(rtwdev, para.bt_lna_lvl);
}

static u32 rtw_coex_read_indirect_reg(struct rtw_dev *rtwdev, u16 addr)
{
	(void)rtwdev;
	(void)addr;
	u32 val;

	if (!ltecoex_read_reg(rtwdev, addr, &val)) {
		rtw_err(rtwdev, "failed to read indirect register\n");
		return 0;
	}

	return val;
}


static void rtw_coex_write_indirect_reg(struct rtw_dev *rtwdev, u16 addr,
				 u32 mask, u32 val)
{
	(void)rtwdev;
	(void)addr;
	(void)mask;
	(void)val;
	u32 shift = __ffs(mask);
	u32 tmp;

	tmp = rtw_coex_read_indirect_reg(rtwdev, addr);
	tmp = (tmp & (~mask)) | ((val << shift) & mask);

	if (!ltecoex_reg_write(rtwdev, addr, tmp))
		rtw_err(rtwdev, "failed to write indirect register\n");
}


static void rtw_coex_coex_ctrl_owner(struct rtw_dev *rtwdev, bool wifi_control)
{
	(void)rtwdev;
	(void)wifi_control;
	const struct rtw_chip_info *chip = rtwdev->chip;
	const struct rtw_hw_reg *btg_reg = chip->btg_reg;

	if (wifi_control) {
		rtw_write8_set(rtwdev, REG_SYS_SDIO_CTRL + 3,
			       BIT_LTE_MUX_CTRL_PATH >> 24);
		if (btg_reg)
			rtw_write8_set(rtwdev, btg_reg->addr, btg_reg->mask);
	} else {
		rtw_write8_clr(rtwdev, REG_SYS_SDIO_CTRL + 3,
			       BIT_LTE_MUX_CTRL_PATH >> 24);
		if (btg_reg)
			rtw_write8_clr(rtwdev, btg_reg->addr, btg_reg->mask);
	}
}

static void rtw_coex_set_gnt_bt(struct rtw_dev *rtwdev, u8 state)
{
	(void)rtwdev;
	(void)state;
	if (!rtwdev->chip->ltecoex_addr)
		return;

	rtw_coex_write_indirect_reg(rtwdev, LTE_COEX_CTRL, 0xc000, state);
	rtw_coex_write_indirect_reg(rtwdev, LTE_COEX_CTRL, 0x0c00, state);
}

static void rtw_coex_set_gnt_wl(struct rtw_dev *rtwdev, u8 state)
{
	(void)rtwdev;
	(void)state;
	if (!rtwdev->chip->ltecoex_addr)
		return;

	rtw_coex_write_indirect_reg(rtwdev, LTE_COEX_CTRL, 0x3000, state);
	rtw_coex_write_indirect_reg(rtwdev, LTE_COEX_CTRL, 0x0300, state);
}

static void rtw_coex_mimo_ps(struct rtw_dev *rtwdev, bool force, bool state)
{
	(void)rtwdev;
	(void)force;
	(void)state;
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	if (!force && state == coex_stat->wl_mimo_ps)
		return;

	coex_stat->wl_mimo_ps = state;

	rtw_set_txrx_1ss(rtwdev, state);

	rtw_coex_update_wl_ch_info(rtwdev, (u8)coex_stat->wl_connected);

	(void)0;
}

static void rtw_btc_wltoggle_table_a(struct rtw_dev *rtwdev, bool force,
				     u8 table_case)
{
	(void)rtwdev;
	(void)force;
	(void)table_case;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 h2c_para[6] = {0};
	u32 table_wl = 0x5a5a5a5a;

	h2c_para[0] = COEX_H2C69_TOGGLE_TABLE_A;
	/* no definition */
	h2c_para[1] = 0x1;

	if (efuse->share_ant) {
		if (table_case < chip->table_sant_num)
			table_wl = chip->table_sant[table_case].wl;
	} else {
		if (table_case < chip->table_nsant_num)
			table_wl = chip->table_nsant[table_case].wl;
	}

	/* tell WL FW WL slot toggle table-A*/
	h2c_para[2] = (u8)u32_get_bits(table_wl, GENMASK(7, 0));
	h2c_para[3] = (u8)u32_get_bits(table_wl, GENMASK(15, 8));
	h2c_para[4] = (u8)u32_get_bits(table_wl, GENMASK(23, 16));
	h2c_para[5] = (u8)u32_get_bits(table_wl, GENMASK(31, 24));

	rtw_fw_bt_wifi_control(rtwdev, h2c_para[0], &h2c_para[1]);

	(void)0;
}

#define COEX_WL_SLOT_TOGLLE 0x5a5a5aaa
static void rtw_btc_wltoggle_table_b(struct rtw_dev *rtwdev, bool force,
				     u8 interval, u32 table)
{
	(void)rtwdev;
	(void)force;
	(void)interval;
	(void)table;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 cur_h2c_para[6] = {0};
	u8 i;

	cur_h2c_para[0] = COEX_H2C69_TOGGLE_TABLE_B;
	cur_h2c_para[1] = interval;
	cur_h2c_para[2] = (u8)u32_get_bits(table, GENMASK(7, 0));
	cur_h2c_para[3] = (u8)u32_get_bits(table, GENMASK(15, 8));
	cur_h2c_para[4] = (u8)u32_get_bits(table, GENMASK(23, 16));
	cur_h2c_para[5] = (u8)u32_get_bits(table, GENMASK(31, 24));

	coex_stat->wl_toggle_interval = interval;

	for (i = 0; i <= 5; i++)
		coex_stat->wl_toggle_para[i] = cur_h2c_para[i];

	rtw_fw_bt_wifi_control(rtwdev, cur_h2c_para[0], &cur_h2c_para[1]);

	(void)0;
}

static void rtw_coex_set_table(struct rtw_dev *rtwdev, bool force, u32 table0,
			       u32 table1)
{
	(void)rtwdev;
	(void)force;
	(void)table0;
	(void)table1;
#define DEF_BRK_TABLE_VAL 0xf0ffffff
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;

	/* If last tdma is wl slot toggle, force write table*/
	if (!force && coex_dm->reason != COEX_RSN_LPS) {
		if (table0 == rtw_read32(rtwdev, REG_BT_COEX_TABLE0) &&
		    table1 == rtw_read32(rtwdev, REG_BT_COEX_TABLE1))
			return;
	}
	rtw_write32(rtwdev, REG_BT_COEX_TABLE0, table0);
	rtw_write32(rtwdev, REG_BT_COEX_TABLE1, table1);
	rtw_write32(rtwdev, REG_BT_COEX_BRK_TABLE, DEF_BRK_TABLE_VAL);

	(void)0;
}

static void rtw_coex_table(struct rtw_dev *rtwdev, bool force, u8 type)
{
	(void)rtwdev;
	(void)force;
	(void)type;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	coex_dm->cur_table = type;

	(void)0;

	if (efuse->share_ant) {
		if (type < chip->table_sant_num)
			rtw_coex_set_table(rtwdev, force,
					   chip->table_sant[type].bt,
					   chip->table_sant[type].wl);
	} else {
		type = type - 100;
		if (type < chip->table_nsant_num)
			rtw_coex_set_table(rtwdev, force,
					   chip->table_nsant[type].bt,
					   chip->table_nsant[type].wl);
	}
	if (coex_stat->wl_slot_toggle_change)
		rtw_btc_wltoggle_table_a(rtwdev, true, type);
}

static void rtw_coex_ignore_wlan_act(struct rtw_dev *rtwdev, bool enable)
{
	(void)rtwdev;
	(void)enable;
	struct rtw_coex *coex = &rtwdev->coex;

	if (coex->manual_control || coex->stop_dm)
		return;

	rtw_fw_bt_ignore_wlan_action(rtwdev, enable);
}

static void rtw_coex_power_save_state(struct rtw_dev *rtwdev, u8 ps_type,
				      u8 lps_val, u8 rpwm_val)
{
	(void)rtwdev;
	(void)ps_type;
	(void)lps_val;
	(void)rpwm_val;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 lps_mode = 0x0;

	lps_mode = rtwdev->lps_conf.mode;

	switch (ps_type) {
	case COEX_PS_WIFI_NATIVE:
		/* recover to original 32k low power setting */
		coex_stat->wl_force_lps_ctrl = false;
		(void)0;
		rtw_leave_lps(rtwdev);
		break;
	case COEX_PS_LPS_OFF:
		coex_stat->wl_force_lps_ctrl = true;
		if (lps_mode)
			rtw_fw_coex_tdma_type(rtwdev, 0, 0, 0, 0, 0);

		rtw_leave_lps(rtwdev);
		(void)0;
		break;
	default:
		break;
	}
}

static void rtw_coex_set_tdma(struct rtw_dev *rtwdev, u8 byte1, u8 byte2,
			      u8 byte3, u8 byte4, u8 byte5)
{
	(void)rtwdev;
	(void)byte1;
	(void)byte2;
	(void)byte3;
	(void)byte4;
	(void)byte5;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 ps_type = COEX_PS_WIFI_NATIVE;
	bool ap_enable = false;

	if (ap_enable && (byte1 & BIT(4) && !(byte1 & BIT(5)))) {
		(void)0;

		byte1 &= ~BIT(4);
		byte1 |= BIT(5);

		byte5 |= BIT(5);
		byte5 &= ~BIT(6);

		ps_type = COEX_PS_WIFI_NATIVE;
		rtw_coex_power_save_state(rtwdev, ps_type, 0x0, 0x0);
	} else if ((byte1 & BIT(4) && !(byte1 & BIT(5))) ||
		   coex_stat->wl_coex_mode == COEX_WLINK_2GFREE) {
		(void)0;

		if (chip->pstdma_type == COEX_PSTDMA_FORCE_LPSOFF)
			ps_type = COEX_PS_LPS_OFF;
		else
			ps_type = COEX_PS_LPS_ON;
		rtw_coex_power_save_state(rtwdev, ps_type, 0x50, 0x4);
	} else {
		(void)0;

		ps_type = COEX_PS_WIFI_NATIVE;
		rtw_coex_power_save_state(rtwdev, ps_type, 0x0, 0x0);
	}

	coex_dm->ps_tdma_para[0] = byte1;
	coex_dm->ps_tdma_para[1] = byte2;
	coex_dm->ps_tdma_para[2] = byte3;
	coex_dm->ps_tdma_para[3] = byte4;
	coex_dm->ps_tdma_para[4] = byte5;

	rtw_fw_coex_tdma_type(rtwdev, byte1, byte2, byte3, byte4, byte5);

	if (byte1 & BIT(2)) {
		coex_stat->wl_slot_toggle = true;
		coex_stat->wl_slot_toggle_change = false;
	} else {
		coex_stat->wl_slot_toggle_change = coex_stat->wl_slot_toggle;
		coex_stat->wl_slot_toggle = false;
	}
}

static void rtw_coex_tdma(struct rtw_dev *rtwdev, bool force, u32 tcase)
{
	(void)rtwdev;
	(void)force;
	(void)tcase;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 n, type;
	bool turn_on;
	bool wl_busy = false;

	if (tcase & TDMA_4SLOT) /* 4-slot (50ms) mode */
		rtw_coex_tdma_timer_base(rtwdev, TDMA_TIMER_TYPE_4SLOT);
	else
		rtw_coex_tdma_timer_base(rtwdev, TDMA_TIMER_TYPE_2SLOT);

	type = (u8)(tcase & 0xff);

	turn_on = (type == 0 || type == 100) ? false : true;

	if (!force && turn_on == coex_dm->cur_ps_tdma_on &&
	    type == coex_dm->cur_ps_tdma) {
		(void)0;

		return;
	}
	wl_busy = test_bit(RTW_FLAG_BUSY_TRAFFIC, rtwdev->flags);

	if ((coex_stat->bt_a2dp_exist &&
	     (coex_stat->bt_inq_remain || coex_stat->bt_multi_link)) ||
	    !wl_busy)
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_TDMA, false);
	else
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_TDMA, true);

	/* update pre state */
	coex_dm->cur_ps_tdma_on = turn_on;
	coex_dm->cur_ps_tdma = type;

	if (efuse->share_ant) {
		if (type < chip->tdma_sant_num)
			rtw_coex_set_tdma(rtwdev,
					  chip->tdma_sant[type].para[0],
					  chip->tdma_sant[type].para[1],
					  chip->tdma_sant[type].para[2],
					  chip->tdma_sant[type].para[3],
					  chip->tdma_sant[type].para[4]);
	} else {
		n = type - 100;
		if (n < chip->tdma_nsant_num)
			rtw_coex_set_tdma(rtwdev,
					  chip->tdma_nsant[n].para[0],
					  chip->tdma_nsant[n].para[1],
					  chip->tdma_nsant[n].para[2],
					  chip->tdma_nsant[n].para[3],
					  chip->tdma_nsant[n].para[4]);
	}


	(void)0;
}

static void rtw_coex_set_ant_path(struct rtw_dev *rtwdev, bool force, u8 phase)
{
	(void)rtwdev;
	(void)force;
	(void)phase;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	u8 ctrl_type = COEX_SWITCH_CTRL_MAX;
	u8 pos_type = COEX_SWITCH_TO_MAX;

	if (!force && coex_dm->cur_ant_pos_type == phase)
		return;

	coex_dm->cur_ant_pos_type = phase;

	/* avoid switch coex_ctrl_owner during BT IQK */
	rtw_coex_check_rfk(rtwdev);

	(void)0;

	switch (phase) {
	case COEX_SET_ANT_POWERON:
		(void)0;
		/* set path control owner to BT at power-on */
		if (coex_stat->bt_disabled)
			rtw_coex_coex_ctrl_owner(rtwdev, true);
		else
			rtw_coex_coex_ctrl_owner(rtwdev, false);

		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
		pos_type = COEX_SWITCH_TO_BT;
		break;
	case COEX_SET_ANT_INIT:
		(void)0;
		if (coex_stat->bt_disabled) {
			/* set GNT_BT to SW low */
			rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_SW_LOW);

			/* set GNT_WL to SW high */
			rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_SW_HIGH);
		} else {
			/* set GNT_BT to SW high */
			rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_SW_HIGH);

			/* set GNT_WL to SW low */
			rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_SW_LOW);
		}

		/* set path control owner to wl at initial step */
		rtw_coex_coex_ctrl_owner(rtwdev, true);

		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
		pos_type = COEX_SWITCH_TO_BT;
		break;
	case COEX_SET_ANT_WONLY:
		(void)0;
		/* set GNT_BT to SW Low */
		rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_SW_LOW);

		/* set GNT_WL to SW high */
		rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_SW_HIGH);

		/* set path control owner to wl at initial step */
		rtw_coex_coex_ctrl_owner(rtwdev, true);

		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
		pos_type = COEX_SWITCH_TO_WLG;
		break;
	case COEX_SET_ANT_WOFF:
		(void)0;
		/* set path control owner to BT */
		rtw_coex_coex_ctrl_owner(rtwdev, false);

		ctrl_type = COEX_SWITCH_CTRL_BY_BT;
		pos_type = COEX_SWITCH_TO_NOCARE;
		break;
	case COEX_SET_ANT_2G:
		(void)0;
		/* set GNT_BT to PTA */
		rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_HW_PTA);

		/* set GNT_WL to PTA */
		rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_HW_PTA);

		/* set path control owner to wl at runtime step */
		rtw_coex_coex_ctrl_owner(rtwdev, true);

		ctrl_type = COEX_SWITCH_CTRL_BY_PTA;
		pos_type = COEX_SWITCH_TO_NOCARE;
		break;
	case COEX_SET_ANT_5G:
		(void)0;

		/* set GNT_BT to HW PTA */
		rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_HW_PTA);

		/* set GNT_WL to SW high */
		rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_SW_HIGH);

		/* set path control owner to wl at runtime step */
		rtw_coex_coex_ctrl_owner(rtwdev, true);

		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
		pos_type = COEX_SWITCH_TO_WLA;
		break;
	case COEX_SET_ANT_2G_FREERUN:
		(void)0;

		/* set GNT_BT to HW PTA */
		rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_HW_PTA);

		/* Set GNT_WL to SW high */
		rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_SW_HIGH);

		/* set path control owner to wl at runtime step */
		rtw_coex_coex_ctrl_owner(rtwdev, true);

		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
		pos_type = COEX_SWITCH_TO_WLG_BT;
		break;
	case COEX_SET_ANT_2G_WLBT:
		(void)0;
		/* set GNT_BT to HW PTA */
		rtw_coex_set_gnt_bt(rtwdev, COEX_GNT_SET_HW_PTA);

		/* Set GNT_WL to HW PTA */
		rtw_coex_set_gnt_wl(rtwdev, COEX_GNT_SET_HW_PTA);

		/* set path control owner to wl at runtime step */
		rtw_coex_coex_ctrl_owner(rtwdev, true);

		ctrl_type = COEX_SWITCH_CTRL_BY_BBSW;
		pos_type = COEX_SWITCH_TO_WLG_BT;
		break;
	default:
		WARN(1, "unknown phase when setting antenna path\n");
		return;
	}

	if (ctrl_type < COEX_SWITCH_CTRL_MAX && pos_type < COEX_SWITCH_TO_MAX &&
	    coex_rfe->ant_switch_exist)
		rtw_coex_set_ant_switch(rtwdev, ctrl_type, pos_type);
}

#define case_ALGO(src) \
	case COEX_ALGO_##src: return #src


#define case_BT_PROFILE(src) \
	case BPM_##src: return #src


static u8 rtw_coex_algorithm(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 algorithm = COEX_ALGO_NOPROFILE;
	u8 profile_map = 0;

	if (coex_stat->bt_hfp_exist)
		profile_map |= BPM_HFP;
	if (coex_stat->bt_hid_exist)
		profile_map |= BPM_HID;
	if (coex_stat->bt_a2dp_exist)
		profile_map |= BPM_A2DP;
	if (coex_stat->bt_pan_exist)
		profile_map |= BPM_PAN;

	switch (profile_map) {
	case BPM_HFP:
		algorithm = COEX_ALGO_HFP;
		break;
	case           BPM_HID:
	case BPM_HFP | BPM_HID:
		algorithm = COEX_ALGO_HID;
		break;
	case BPM_HFP           | BPM_A2DP:
	case           BPM_HID | BPM_A2DP:
	case BPM_HFP | BPM_HID | BPM_A2DP:
		algorithm = COEX_ALGO_A2DP_HID;
		break;
	case BPM_HFP                      | BPM_PAN:
	case           BPM_HID            | BPM_PAN:
	case BPM_HFP | BPM_HID            | BPM_PAN:
		algorithm = COEX_ALGO_PAN_HID;
		break;
	case BPM_HFP           | BPM_A2DP | BPM_PAN:
	case           BPM_HID | BPM_A2DP | BPM_PAN:
	case BPM_HFP | BPM_HID | BPM_A2DP | BPM_PAN:
		algorithm = COEX_ALGO_A2DP_PAN_HID;
		break;
	case                                BPM_PAN:
		algorithm = COEX_ALGO_PAN;
		break;
	case                     BPM_A2DP | BPM_PAN:
		algorithm = COEX_ALGO_A2DP_PAN;
		break;
	case                     BPM_A2DP:
		if (coex_stat->bt_multi_link) {
			if (coex_stat->bt_hid_pair_num > 0)
				algorithm = COEX_ALGO_A2DP_HID;
			else
				algorithm = COEX_ALGO_A2DP_PAN;
		} else {
			algorithm = COEX_ALGO_A2DP;
		}
		break;
	default:
		algorithm = COEX_ALGO_NOPROFILE;
		break;
	}

	(void)0;
	return algorithm;
}

static void rtw_coex_action_coex_all_off(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 2;
		tdma_case = 0;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_freerun(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 level = 0;
	bool bt_afh_loss = true;

	(void)0;

	if (efuse->share_ant)
		return;

	coex->freerun = true;

	if (bt_afh_loss)
		rtw_coex_update_wl_ch_info(rtwdev, COEX_MEDIA_CONNECT);

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G_FREERUN);

	rtw_coex_write_scbd(rtwdev, COEX_SCBD_FIX2M, false);

	if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[0]))
		level = 2;
	else if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[1]))
		level = 3;
	else if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[2]))
		level = 4;
	else
		level = 5;

	if (level > chip->wl_rf_para_num - 1)
		level = chip->wl_rf_para_num - 1;

	if (coex_stat->wl_tput_dir == COEX_WL_TPUT_TX)
		rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_tx[level]);
	else
		rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[level]);

	rtw_coex_table(rtwdev, false, 100);
	rtw_coex_tdma(rtwdev, false, 100);
}

static void rtw_coex_action_rf4ce(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 9;
		tdma_case = 16;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_whql_test(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 2;
		tdma_case = 0;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_relink(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) { /* Shared-Ant */
		if (coex_stat->wl_gl_busy) {
			table_case = 26;
			if (coex_stat->bt_hid_exist &&
			    coex_stat->bt_profile_num == 1) {
				slot_type = TDMA_4SLOT;
				tdma_case = 20;
			} else {
				tdma_case = 20;
			}
		} else {
			table_case = 1;
			tdma_case = 0;
		}
	} else { /* Non-Shared-Ant */
		if (coex_stat->wl_gl_busy)
			table_case = 115;
		else
			table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case | slot_type);
}

static void rtw_coex_action_bt_idle(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_coex_rfe *coex_rfe = &coex->rfe;
	u8 table_case = 0xff, tdma_case = 0xff;

	(void)0;
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (coex_rfe->ant_switch_with_bt &&
	    coex_dm->bt_status == COEX_BTSTATUS_NCON_IDLE) {
		if (efuse->share_ant &&
		    COEX_RSSI_HIGH(coex_dm->wl_rssi_state[3]) &&
		    coex_stat->wl_gl_busy) {
			table_case = 0;
			tdma_case = 0;
		} else if (!efuse->share_ant) {
			table_case = 100;
			tdma_case = 100;
		}
	}

	if (table_case != 0xff && tdma_case != 0xff) {
		rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G_FREERUN);
		goto exit;
	}

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);

	if (efuse->share_ant) {
		/* Shared-Ant */
		if (!coex_stat->wl_gl_busy) {
			table_case = 10;
			tdma_case = 3;
		} else if (coex_dm->bt_status == COEX_BTSTATUS_NCON_IDLE) {
			table_case = 11;

			if (coex_stat->lo_pri_rx + coex_stat->lo_pri_tx > 250)
				tdma_case = 17;
			else
				tdma_case = 7;
		} else {
			table_case = 12;
			tdma_case = 7;
		}
	} else {
		/* Non-Shared-Ant */
		if (!coex_stat->wl_gl_busy) {
			table_case = 112;
			tdma_case = 104;
		} else if ((coex_stat->bt_ble_scan_type & 0x2) &&
			   coex_dm->bt_status == COEX_BTSTATUS_NCON_IDLE) {
			table_case = 114;
			tdma_case = 103;
		} else {
			table_case = 112;
			tdma_case = 103;
		}
	}

exit:
	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_inquiry(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	bool wl_hi_pri = false;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (coex_stat->wl_linkscan_proc || coex_stat->wl_hi_pri_task1 ||
	    coex_stat->wl_hi_pri_task2)
		wl_hi_pri = true;

	if (efuse->share_ant) {
		/* Shared-Ant */
		if (wl_hi_pri) {
			(void)0;
			table_case = 15;

			if (coex_stat->bt_profile_num > 0)
				tdma_case = 10;
			else if (coex_stat->wl_hi_pri_task1)
				tdma_case = 6;
			else if (!coex_stat->bt_page)
				tdma_case = 8;
			else
				tdma_case = 9;
		} else if (coex_stat->wl_gl_busy) {
			(void)0;
			if (coex_stat->bt_profile_num == 0) {
				table_case = 12;
				tdma_case = 18;
			} else if (coex_stat->bt_profile_num == 1 &&
				   !coex_stat->bt_a2dp_exist) {
				slot_type = TDMA_4SLOT;
				table_case = 12;
				tdma_case = 20;
			} else {
				slot_type = TDMA_4SLOT;
				table_case = 12;
				tdma_case = 26;
			}
		} else if (coex_stat->wl_connected) {
			(void)0;
			table_case = 9;
			tdma_case = 27;
		} else {
			(void)0;
			table_case = 1;
			tdma_case = 0;
		}
	} else {
		/* Non_Shared-Ant */
		if (wl_hi_pri) {
			(void)0;
			table_case = 114;

			if (coex_stat->bt_profile_num > 0)
				tdma_case = 110;
			else if (coex_stat->wl_hi_pri_task1)
				tdma_case = 106;
			else if (!coex_stat->bt_page)
				tdma_case = 108;
			else
				tdma_case = 109;
		}  else if (coex_stat->wl_gl_busy) {
			(void)0;
			table_case = 114;
			tdma_case = 121;
		} else if (coex_stat->wl_connected) {
			(void)0;
			table_case = 101;
			tdma_case = 100;
		} else {
			(void)0;
			table_case = 101;
			tdma_case = 100;
		}
	}

	(void)0;

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case | slot_type);
}

static void rtw_coex_action_bt_game_hid(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);

	if (efuse->share_ant) {
		coex_stat->wl_coex_mode = COEX_WLINK_2GFREE;
		if (coex_stat->bt_whck_test)
			table_case = 2;
		else if (coex_stat->wl_linkscan_proc || coex_stat->bt_hid_exist)
			table_case = 33;
		else if (coex_stat->bt_setup_link || coex_stat->bt_inq_page)
			table_case = 0;
		else if (coex_stat->bt_a2dp_exist)
			table_case = 34;
		else
			table_case = 33;

		tdma_case = 0;
	} else {
		if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[1]))
			tdma_case = 112;
		else
			tdma_case = 113;

		table_case = 121;
	}

	if (coex_stat->wl_coex_mode == COEX_WLINK_2GFREE) {
		if (coex_stat->wl_tput_dir == COEX_WL_TPUT_TX)
			rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_tx[6]);
		else
			rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[5]);
	} else {
		rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_hfp(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 10;
		tdma_case = 5;
	} else {
		/* Non-Shared-Ant */
		if (coex_stat->bt_multi_link) {
			table_case = 112;
			tdma_case = 117;
		} else {
			table_case = 105;
			tdma_case = 100;
		}
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_hid(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	bool is_bt_ctr_hi = false, is_toggle_table = false;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (coex_stat->bt_ctr_ok &&
	    coex_stat->lo_pri_rx + coex_stat->lo_pri_tx > 360)
		is_bt_ctr_hi = true;

	if (efuse->share_ant) {
		/* Shared-Ant */
		if (coex_stat->bt_ble_exist) {
			/* RCU */
			if (coex_stat->cnt_wl[COEX_CNT_WL_SCANAP] > 5) {
				table_case = 26;
				tdma_case = 2;
			} else {
				table_case = 27;
				tdma_case = 9;
			}
		} else {
			/* Legacy HID  */
			if (coex_stat->bt_a2dp_active) {
				table_case = 9;
				tdma_case = 18;
			} else if (coex_stat->bt_profile_num == 1 &&
				   (coex_stat->bt_multi_link &&
				    (is_bt_ctr_hi || coex_stat->bt_slave ||
				     coex_stat->bt_multi_link_remain))) {
				if (coex_stat->wl_gl_busy &&
				    (coex_stat->wl_rx_rate <= 3 ||
				     coex_stat->wl_rts_rx_rate <= 3))
					table_case = 13;
				else
					table_case = 12;

				tdma_case = 26;
			} else if (coex_stat->bt_418_hid_exist &&
				   coex_stat->wl_gl_busy) {
				is_toggle_table = true;
				slot_type = TDMA_4SLOT;
				table_case = 32;
				tdma_case = 27;
			} else if (coex_stat->bt_ble_hid_exist &&
				   coex_stat->wl_gl_busy) {
				table_case = 36;
				tdma_case = 0;
			} else {
				table_case = 9;
				tdma_case = 9;
			}
		}
	} else {
		/* Non-Shared-Ant */
		if (coex_stat->bt_ble_exist) {
			/* BLE */
			if (coex_stat->cnt_wl[COEX_CNT_WL_SCANAP] > 5) {
				table_case = 121;
				tdma_case = 102;
			} else {
				table_case = 122;
				tdma_case = 109;
			}
		} else if (coex_stat->bt_a2dp_active) {
			table_case = 113;
			tdma_case = 118;
		} else {
			table_case = 113;
			tdma_case = 104;
		}
	}

	rtw_coex_table(rtwdev, false, table_case);
	if (is_toggle_table) {
		rtw_btc_wltoggle_table_a(rtwdev, true, table_case);
		rtw_btc_wltoggle_table_b(rtwdev, false, 1, COEX_WL_SLOT_TOGLLE);
	}

	rtw_coex_tdma(rtwdev, false, tdma_case | slot_type);
}

static void rtw_coex_action_bt_a2dp(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	slot_type = TDMA_4SLOT;

	if (efuse->share_ant) {
		/* Shared-Ant */
		if (coex_stat->wl_gl_busy && coex_stat->wl_noisy_level == 0)
			table_case = 12;
		else
			table_case = 9;

		if (coex_stat->wl_connecting || !coex_stat->wl_gl_busy)
			tdma_case = 14;
		else
			tdma_case = 13;
	} else {
		/* Non-Shared-Ant */
		table_case = 112;

		if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[1]))
			tdma_case = 112;
		else
			tdma_case = 113;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case | slot_type);
}

static void rtw_coex_action_bt_a2dpsink(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;
	bool ap_enable = false;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) { /* Shared-Ant */
		if (ap_enable) {
			table_case = 2;
			tdma_case = 0;
		} else if (coex_stat->wl_gl_busy) {
			table_case = 28;
			tdma_case = 20;
		} else {
			table_case = 28;
			tdma_case = 26;
		}
	} else { /* Non-Shared-Ant */
		if (ap_enable) {
			table_case = 100;
			tdma_case = 100;
		} else {
			table_case = 119;
			tdma_case = 120;
		}
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_pan(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		if (coex_stat->wl_gl_busy && coex_stat->wl_noisy_level == 0)
			table_case = 14;
		else
			table_case = 10;

		if (coex_stat->wl_gl_busy)
			tdma_case = 17;
		else
			tdma_case = 20;
	} else {
		/* Non-Shared-Ant */
		table_case = 112;

		if (coex_stat->wl_gl_busy)
			tdma_case = 117;
		else
			tdma_case = 119;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_a2dp_hid(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case, interval = 0;
	u32 slot_type = 0;
	bool is_toggle_table = false;

	slot_type = TDMA_4SLOT;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		if (coex_stat->bt_ble_exist) {
			table_case = 26; /* for RCU */
		} else if (coex_stat->bt_418_hid_exist) {
			table_case = 9;
			interval = 1;
		} else {
			table_case = 9;
		}

		if (coex_stat->wl_connecting || !coex_stat->wl_gl_busy) {
			tdma_case = 14;
		} else if (coex_stat->bt_418_hid_exist) {
			is_toggle_table = true;
			tdma_case = 23;
		} else {
			tdma_case = 13;
		}
	} else {
		/* Non-Shared-Ant */
		if (coex_stat->bt_ble_exist)
			table_case = 121;
		else
			table_case = 113;

		if (COEX_RSSI_HIGH(coex_dm->wl_rssi_state[1]))
			tdma_case = 112;
		else
			tdma_case = 113;
	}

	rtw_coex_table(rtwdev, false, table_case);
	if (is_toggle_table) {
		rtw_btc_wltoggle_table_a(rtwdev, true, table_case);
		rtw_btc_wltoggle_table_b(rtwdev, false, interval, COEX_WL_SLOT_TOGLLE);
	}
	rtw_coex_tdma(rtwdev, false, tdma_case | slot_type);
}

static void rtw_coex_action_bt_a2dp_pan(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);
	if (efuse->share_ant) {
		/* Shared-Ant */
		if (coex_stat->wl_gl_busy &&
		    coex_stat->wl_noisy_level == 0)
			table_case = 14;
		else
			table_case = 10;

		if (coex_stat->wl_gl_busy)
			tdma_case = 15;
		else
			tdma_case = 20;
	} else {
		/* Non-Shared-Ant */
		table_case = 112;

		if (coex_stat->wl_gl_busy)
			tdma_case = 115;
		else
			tdma_case = 120;
	}

	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);
	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_pan_hid(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 9;

		if (coex_stat->wl_gl_busy)
			tdma_case = 18;
		else
			tdma_case = 19;
	} else {
		/* Non-Shared-Ant */
		table_case = 113;

		if (coex_stat->wl_gl_busy)
			tdma_case = 117;
		else
			tdma_case = 119;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_bt_a2dp_pan_hid(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 10;

		if (coex_stat->wl_gl_busy)
			tdma_case = 15;
		else
			tdma_case = 20;
	} else {
		/* Non-Shared-Ant */
		table_case = 113;

		if (coex_stat->wl_gl_busy)
			tdma_case = 115;
		else
			tdma_case = 120;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_wl_under5g(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 table_case, tdma_case;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_5G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	rtw_coex_write_scbd(rtwdev, COEX_SCBD_FIX2M, false);

	if (coex_stat->bt_game_hid_exist && coex_stat->wl_linkscan_proc)
		coex_stat->wl_coex_mode = COEX_WLINK_2GFREE;

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 0;
		tdma_case = 0;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_wl_only(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 2;
		tdma_case = 0;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_wl_native_lps(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 table_case, tdma_case;

	if (coex->under_5g)
		return;

	(void)0;

	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 28;
		tdma_case = 0;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	if (coex_stat->bt_game_hid_exist) {
		coex_stat->wl_coex_mode = COEX_WLINK_2GFREE;
		if (coex_stat->wl_tput_dir == COEX_WL_TPUT_TX)
			rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_tx[6]);
		else
			rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[5]);
	} else {
		rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_wl_linkscan(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;
	u32 slot_type = 0;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) { /* Shared-Ant */
		if (coex_stat->bt_a2dp_exist) {
			slot_type = TDMA_4SLOT;
			tdma_case = 11;
			if (coex_stat->wl_gl_busy)
				table_case = 26;
			else
				table_case = 9;
		} else {
			table_case = 9;
			tdma_case = 7;
		}
	} else { /* Non-Shared-Ant */
		if (coex_stat->bt_a2dp_exist) {
			slot_type = TDMA_4SLOT;
			table_case = 112;
			tdma_case = 111;
		} else {
			table_case = 112;
			tdma_case = 107;
		}
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case | slot_type);
}

static void rtw_coex_action_wl_not_connected(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	u8 table_case, tdma_case;

	(void)0;
	rtw_coex_set_ant_path(rtwdev, false, COEX_SET_ANT_2G);
	rtw_coex_set_rf_para(rtwdev, chip->wl_rf_para_rx[0]);

	if (efuse->share_ant) {
		/* Shared-Ant */
		table_case = 1;
		tdma_case = 0;
	} else {
		/* Non-Shared-Ant */
		table_case = 100;
		tdma_case = 100;
	}

	rtw_coex_table(rtwdev, false, table_case);
	rtw_coex_tdma(rtwdev, false, tdma_case);
}

static void rtw_coex_action_wl_connected(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 algorithm;

	(void)0;

	algorithm = rtw_coex_algorithm(rtwdev);

	switch (algorithm) {
	case COEX_ALGO_HFP:
		rtw_coex_action_bt_hfp(rtwdev);
		break;
	case COEX_ALGO_HID:
		if (rtw_coex_freerun_check(rtwdev))
			rtw_coex_action_freerun(rtwdev);
		else
			rtw_coex_action_bt_hid(rtwdev);
		break;
	case COEX_ALGO_A2DP:
		if (rtw_coex_freerun_check(rtwdev))
			rtw_coex_action_freerun(rtwdev);
		else if (coex_stat->bt_a2dp_sink)
			rtw_coex_action_bt_a2dpsink(rtwdev);
		else
			rtw_coex_action_bt_a2dp(rtwdev);
		break;
	case COEX_ALGO_PAN:
		rtw_coex_action_bt_pan(rtwdev);
		break;
	case COEX_ALGO_A2DP_HID:
		if (rtw_coex_freerun_check(rtwdev))
			rtw_coex_action_freerun(rtwdev);
		else
			rtw_coex_action_bt_a2dp_hid(rtwdev);
		break;
	case COEX_ALGO_A2DP_PAN:
		rtw_coex_action_bt_a2dp_pan(rtwdev);
		break;
	case COEX_ALGO_PAN_HID:
		rtw_coex_action_bt_pan_hid(rtwdev);
		break;
	case COEX_ALGO_A2DP_PAN_HID:
		rtw_coex_action_bt_a2dp_pan_hid(rtwdev);
		break;
	default:
	case COEX_ALGO_NOPROFILE:
		rtw_coex_action_bt_idle(rtwdev);
		break;
	}
}

static void rtw_coex_run_coex(struct rtw_dev *rtwdev, u8 reason)
{
	(void)rtwdev;
	(void)reason;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	bool rf4ce_en = false;

	

	if (!test_bit(RTW_FLAG_RUNNING, rtwdev->flags))
		return;

	coex_dm->reason = reason;

	(void)0;

	/* update wifi_link_info_ext variable */
	rtw_coex_update_wl_link_info(rtwdev, reason);

	rtw_coex_monitor_bt_enable(rtwdev);

	if (coex->manual_control) {
		(void)0;
		return;
	}

	if (coex->stop_dm) {
		(void)0;
		return;
	}

	if (coex_stat->wl_under_ips) {
		(void)0;
		return;
	}

	if (coex->freeze && coex_dm->reason == COEX_RSN_BTINFO &&
	    !coex_stat->bt_setup_link) {
		(void)0;
		return;
	}

	coex_stat->cnt_wl[COEX_CNT_WL_COEXRUN]++;
	coex->freerun = false;

	/* Pure-5G Coex Process */
	if (coex->under_5g) {
		coex_stat->wl_coex_mode = COEX_WLINK_5G;
		rtw_coex_action_wl_under5g(rtwdev);
		goto exit;
	}

	(void)0;
	coex_stat->wl_coex_mode = COEX_WLINK_2G1PORT;

	if (coex_stat->bt_disabled) {
		if (coex_stat->wl_connected && rf4ce_en)
			rtw_coex_action_rf4ce(rtwdev);
		else if (!coex_stat->wl_connected)
			rtw_coex_action_wl_not_connected(rtwdev);
		else
			rtw_coex_action_wl_only(rtwdev);
		goto exit;
	}

	if (coex_stat->wl_under_lps && !coex_stat->wl_force_lps_ctrl) {
		rtw_coex_action_wl_native_lps(rtwdev);
		goto exit;
	}

	if (coex_stat->bt_game_hid_exist && coex_stat->wl_connected) {
		rtw_coex_action_bt_game_hid(rtwdev);
		goto exit;
	}

	if (coex_stat->bt_whck_test) {
		rtw_coex_action_bt_whql_test(rtwdev);
		goto exit;
	}

	if (coex_stat->bt_setup_link) {
		rtw_coex_action_bt_relink(rtwdev);
		goto exit;
	}

	if (coex_stat->bt_inq_page) {
		rtw_coex_action_bt_inquiry(rtwdev);
		goto exit;
	}

	if ((coex_dm->bt_status == COEX_BTSTATUS_NCON_IDLE ||
	     coex_dm->bt_status == COEX_BTSTATUS_CON_IDLE) &&
	     coex_stat->wl_connected) {
		rtw_coex_action_bt_idle(rtwdev);
		goto exit;
	}

	if (coex_stat->wl_linkscan_proc && !coex->freerun) {
		rtw_coex_action_wl_linkscan(rtwdev);
		goto exit;
	}

	if (coex_stat->wl_connected) {
		rtw_coex_action_wl_connected(rtwdev);
		goto exit;
	} else {
		rtw_coex_action_wl_not_connected(rtwdev);
		goto exit;
	}

exit:

	if (chip->wl_mimo_ps_support) {
		if (coex_stat->wl_coex_mode == COEX_WLINK_2GFREE) {
			if (coex_dm->reason == COEX_RSN_2GMEDIA)
				rtw_coex_mimo_ps(rtwdev, true, true);
			else
				rtw_coex_mimo_ps(rtwdev, false, true);
		} else {
			rtw_coex_mimo_ps(rtwdev, false, false);
		}
	}

	rtw_coex_gnt_workaround(rtwdev, false, coex_stat->wl_coex_mode);
	rtw_coex_limited_wl(rtwdev);
}

static void rtw_coex_init_coex_var(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	u8 i;

	memset(coex_dm, 0, sizeof(*coex_dm));
	memset(coex_stat, 0, sizeof(*coex_stat));

	for (i = 0; i < COEX_CNT_WL_MAX; i++)
		coex_stat->cnt_wl[i] = 0;

	for (i = 0; i < COEX_CNT_BT_MAX; i++)
		coex_stat->cnt_bt[i] = 0;

	for (i = 0; i < ARRAY_SIZE(coex_dm->bt_rssi_state); i++)
		coex_dm->bt_rssi_state[i] = COEX_RSSI_STATE_LOW;

	for (i = 0; i < ARRAY_SIZE(coex_dm->wl_rssi_state); i++)
		coex_dm->wl_rssi_state[i] = COEX_RSSI_STATE_LOW;

	coex_stat->wl_coex_mode = COEX_WLINK_MAX;
	coex_stat->wl_rx_rate = DESC_RATE5_5M;
	coex_stat->wl_rts_rx_rate = DESC_RATE5_5M;
}

static void __rtw_coex_init_hw_config(struct rtw_dev *rtwdev, bool wifi_only)
{
	(void)rtwdev;
	(void)wifi_only;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	(void)0;

	rtw_coex_init_coex_var(rtwdev);

	coex_stat->kt_ver = u8_get_bits(rtw_read8(rtwdev, 0xf1), GENMASK(7, 4));

	rtw_coex_monitor_bt_enable(rtwdev);
	rtw_coex_wl_slot_extend(rtwdev, coex_stat->wl_slot_extend);

	rtw_write8_set(rtwdev, REG_BCN_CTRL, BIT_EN_BCN_FUNCTION);

	rtw_coex_set_rfe_type(rtwdev);
	rtw_coex_set_init(rtwdev);

	/* set Tx response = Hi-Pri (ex: Transmitting ACK,BA,CTS) */
	rtw_coex_set_wl_pri_mask(rtwdev, COEX_WLPRI_TX_RSP, 1);

	/* set Tx beacon = Hi-Pri */
	rtw_coex_set_wl_pri_mask(rtwdev, COEX_WLPRI_TX_BEACON, 1);

	/* set Tx beacon queue = Hi-Pri */
	rtw_coex_set_wl_pri_mask(rtwdev, COEX_WLPRI_TX_BEACONQ, 1);

	/* antenna config */
	if (coex->wl_rf_off) {
		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_WOFF);
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ALL, false);
		coex->stop_dm = true;

		(void)0;
	} else if (wifi_only) {
		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_WONLY);
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE | COEX_SCBD_ONOFF,
				    true);
		coex->stop_dm = true;
	} else {
		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_INIT);
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE | COEX_SCBD_ONOFF,
				    true);
		coex->stop_dm = false;
		coex->freeze = true;
	}

	/* PTA parameter */
	rtw_coex_table(rtwdev, true, 1);
	rtw_coex_tdma(rtwdev, true, 0);
	rtw_coex_query_bt_info(rtwdev);
}

static void rtw_coex_power_on_setting(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	u8 table_case = 1;

	(void)0;

	coex->stop_dm = true;
	coex->wl_rf_off = false;

	/* enable BB, we can write 0x948 */
	rtw_write8_set(rtwdev, REG_SYS_FUNC_EN,
		       BIT_FEN_BB_GLB_RST | BIT_FEN_BB_RSTB);

	rtw_coex_monitor_bt_enable(rtwdev);
	rtw_coex_set_rfe_type(rtwdev);

	/* set antenna path to BT */
	rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_POWERON);

	rtw_coex_table(rtwdev, true, table_case);
	/* red x issue */
	rtw_write8(rtwdev, 0xff1a, 0x0);
	rtw_coex_set_gnt_debug(rtwdev);
}


static void rtw_coex_power_off_setting(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	rtw_write16(rtwdev, REG_WIFI_BT_INFO, BIT_BT_INT_EN);
}


static void rtw_coex_init_hw_config(struct rtw_dev *rtwdev, bool wifi_only)
{
	(void)rtwdev;
	(void)wifi_only;
	__rtw_coex_init_hw_config(rtwdev, wifi_only);
}


static void rtw_coex_ips_notify(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex->manual_control || coex->stop_dm)
		return;

	if (type == COEX_IPS_ENTER) {
		(void)0;

		coex_stat->wl_under_ips = true;

		/* for lps off */
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ALL, false);

		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_WOFF);
		rtw_coex_action_coex_all_off(rtwdev);
	} else if (type == COEX_IPS_LEAVE) {
		(void)0;

		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE | COEX_SCBD_ONOFF, true);
		/* run init hw config (exclude wifi only) */
		__rtw_coex_init_hw_config(rtwdev, false);

		coex_stat->wl_under_ips = false;
	}
}

static __attribute__((unused)) void rtw_coex_lps_notify(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex->manual_control || coex->stop_dm)
		return;

	if (type == COEX_LPS_ENABLE) {
		(void)0;

		coex_stat->wl_under_lps = true;

		if (coex_stat->wl_force_lps_ctrl) {
			/* for ps-tdma */
			rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE, true);
		} else {
			/* for native ps */
			rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE, false);
			rtw_coex_write_scbd(rtwdev, COEX_SCBD_WLBUSY, false);

			rtw_coex_run_coex(rtwdev, COEX_RSN_LPS);
		}
	} else if (type == COEX_LPS_DISABLE) {
		(void)0;

		coex_stat->wl_under_lps = false;

		/* for lps off */
		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE, true);

		if (!coex_stat->wl_force_lps_ctrl)
			rtw_coex_query_bt_info(rtwdev);

		rtw_coex_run_coex(rtwdev, COEX_RSN_LPS);
	}
}

static void rtw_coex_scan_notify(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex->manual_control || coex->stop_dm)
		return;

	coex->freeze = false;
	rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE | COEX_SCBD_ONOFF, true);

	if (type == COEX_SCAN_START_5G) {
		(void)0;

		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_5G);
		rtw_coex_run_coex(rtwdev, COEX_RSN_5GSCANSTART);
	} else if ((type == COEX_SCAN_START_2G) || (type == COEX_SCAN_START)) {
		(void)0;

		coex_stat->wl_hi_pri_task2 = true;

		/* Force antenna setup for no scan result issue */
		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_2G);
		rtw_coex_run_coex(rtwdev, COEX_RSN_2GSCANSTART);
	} else {
		/* Preserve the caller's actual scan-cache AP count. */

		(void)0;

		coex_stat->wl_hi_pri_task2 = false;
		rtw_coex_run_coex(rtwdev, COEX_RSN_SCANFINISH);
	}
}

static void rtw_coex_switchband_notify(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;

	if (coex->manual_control || coex->stop_dm)
		return;

	if (type == COEX_SWITCH_TO_5G) {
		(void)0;
	} else if (type == COEX_SWITCH_TO_24G_NOFORSCAN) {
		(void)0;
	} else {
		(void)0;
	}

	if (type == COEX_SWITCH_TO_5G)
		rtw_coex_run_coex(rtwdev, COEX_RSN_5GSWITCHBAND);
	else if (type == COEX_SWITCH_TO_24G_NOFORSCAN)
		rtw_coex_run_coex(rtwdev, COEX_RSN_2GSWITCHBAND);
	else
		rtw_coex_scan_notify(rtwdev, COEX_SCAN_START_2G);
}

static void rtw_coex_connect_notify(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex->manual_control || coex->stop_dm)
		return;

	rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE | COEX_SCBD_ONOFF, true);

	if (type == COEX_ASSOCIATE_5G_START) {
		(void)0;

		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_5G);
		rtw_coex_run_coex(rtwdev, COEX_RSN_5GCONSTART);
	} else if (type == COEX_ASSOCIATE_5G_FINISH) {
		(void)0;

		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_5G);
		rtw_coex_run_coex(rtwdev, COEX_RSN_5GCONFINISH);
	} else if (type == COEX_ASSOCIATE_START) {
		coex_stat->wl_hi_pri_task1 = true;
		coex_stat->wl_connecting = true;
		coex_stat->cnt_wl[COEX_CNT_WL_CONNPKT] = 2;
		coex_stat->wl_connecting = true;
		ieee80211_queue_delayed_work(rtwdev->hw,
					     &coex->wl_connecting_work, 2 * HZ);

		(void)0;
		/* Force antenna setup for no scan result issue */
		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_2G);

		rtw_coex_run_coex(rtwdev, COEX_RSN_2GCONSTART);

		/* To keep TDMA case during connect process,
		 * to avoid changed by Btinfo and runcoexmechanism
		 */
		coex->freeze = true;
		ieee80211_queue_delayed_work(rtwdev->hw, &coex->defreeze_work,
					     5 * HZ);
	} else {
		coex_stat->wl_hi_pri_task1 = false;
		coex->freeze = false;
		coex_stat->wl_connecting = false;

		(void)0;
		rtw_coex_run_coex(rtwdev, COEX_RSN_2GCONFINISH);
	}
}

static void rtw_coex_media_status_notify(struct rtw_dev *rtwdev, u8 type)
{
	(void)rtwdev;
	(void)type;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	if (coex->manual_control || coex->stop_dm)
		return;

	if (type == COEX_MEDIA_CONNECT_5G) {
		(void)0;

		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE, true);

		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_5G);
		rtw_coex_run_coex(rtwdev, COEX_RSN_5GMEDIA);
	} else if (type == COEX_MEDIA_CONNECT) {
		(void)0;

		coex_stat->wl_connecting = false;

		rtw_coex_write_scbd(rtwdev, COEX_SCBD_ACTIVE, true);

		/* Force antenna setup for no scan result issue */
		rtw_coex_set_ant_path(rtwdev, true, COEX_SET_ANT_2G);

		/* Set CCK Rx high Pri */
		rtw_coex_set_wl_pri_mask(rtwdev, COEX_WLPRI_RX_CCK, 1);
		rtw_coex_run_coex(rtwdev, COEX_RSN_2GMEDIA);
	} else {
		(void)0;
		rtw_coex_set_wl_pri_mask(rtwdev, COEX_WLPRI_RX_CCK, 0);
		rtw_coex_run_coex(rtwdev, COEX_RSN_MEDIADISCON);
	}

	rtw_coex_update_wl_ch_info(rtwdev, type);
}

static void rtw_coex_bt_info_notify(struct rtw_dev *rtwdev, u8 *buf, u8 length)
{
	(void)rtwdev;
	(void)buf;
	(void)length;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_dm *coex_dm = &coex->dm;
	u32 bt_relink_time;
	u8 i, rsp_source = 0, type;
	bool inq_page = false;

	rsp_source = buf[0] & 0xf;
	if (rsp_source >= COEX_BTINFO_SRC_MAX)
		return;
	coex_stat->cnt_bt_info_c2h[rsp_source]++;

	if (rsp_source == COEX_BTINFO_SRC_BT_IQK) {
		coex_stat->bt_iqk_state = buf[1];
		if (coex_stat->bt_iqk_state == 0)
			coex_stat->cnt_bt[COEX_CNT_BT_IQK]++;
		else if (coex_stat->bt_iqk_state == 2)
			coex_stat->cnt_bt[COEX_CNT_BT_IQKFAIL]++;

		(void)0;

		return;
	}

	if (rsp_source == COEX_BTINFO_SRC_BT_SCBD) {
		(void)0;

		rtw_coex_monitor_bt_enable(rtwdev);
		if (coex_stat->bt_disabled != coex_stat->bt_disabled_pre) {
			coex_stat->bt_disabled_pre = coex_stat->bt_disabled;
			rtw_coex_run_coex(rtwdev, COEX_RSN_BTINFO);
		}
		return;
	}

	if (rsp_source == COEX_BTINFO_SRC_H2C60) {
		(void)0;

		for (i = 1; i <= COEX_WL_TDMA_PARA_LENGTH; i++)
			coex_dm->fw_tdma_para[i - 1] = buf[i];
		return;
	}

	if (rsp_source == COEX_BTINFO_SRC_WL_FW) {
		(void)0;

		rtw_coex_update_bt_link_info(rtwdev);
		return;
	}

	if (rsp_source == COEX_BTINFO_SRC_BT_RSP ||
	    rsp_source == COEX_BTINFO_SRC_BT_ACT) {
		if (coex_stat->bt_disabled) {
			coex_stat->bt_disabled = false;
			coex_stat->bt_reenable = true;
			ieee80211_queue_delayed_work(rtwdev->hw,
						     &coex->bt_reenable_work,
						     15 * HZ);
			(void)0;
		}
	}

	if (length != COEX_BTINFO_LENGTH) {
		(void)0;

		return;
	}

	(void)0;

	for (i = 0; i < COEX_BTINFO_LENGTH; i++)
		coex_stat->bt_info_c2h[rsp_source][i] = buf[i];

	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A)
		coex_stat->bt_info_c2h[rsp_source][5] = 0;

	/* get the same info from bt, skip it */
	if (coex_stat->bt_info_c2h[rsp_source][1] == coex_stat->bt_info_lb2 &&
	    coex_stat->bt_info_c2h[rsp_source][2] == coex_stat->bt_info_lb3 &&
	    coex_stat->bt_info_c2h[rsp_source][3] == coex_stat->bt_info_hb0 &&
	    coex_stat->bt_info_c2h[rsp_source][4] == coex_stat->bt_info_hb1 &&
	    coex_stat->bt_info_c2h[rsp_source][5] == coex_stat->bt_info_hb2 &&
	    coex_stat->bt_info_c2h[rsp_source][6] == coex_stat->bt_info_hb3) {
		(void)0;
		return;
	}

	coex_stat->bt_info_lb2 = coex_stat->bt_info_c2h[rsp_source][1];
	coex_stat->bt_info_lb3 = coex_stat->bt_info_c2h[rsp_source][2];
	coex_stat->bt_info_hb0 = coex_stat->bt_info_c2h[rsp_source][3];
	coex_stat->bt_info_hb1 = coex_stat->bt_info_c2h[rsp_source][4];
	coex_stat->bt_info_hb2 = coex_stat->bt_info_c2h[rsp_source][5];
	coex_stat->bt_info_hb3 = coex_stat->bt_info_c2h[rsp_source][6];

	/* 0xff means BT is under WHCK test */
	coex_stat->bt_whck_test = (coex_stat->bt_info_lb2 == 0xff);

	inq_page = ((coex_stat->bt_info_lb2 & BIT(2)) == BIT(2));

	if (inq_page != coex_stat->bt_inq_page) {
		cancel_delayed_work_sync(&coex->bt_remain_work);
		coex_stat->bt_inq_page = inq_page;

		if (inq_page)
			coex_stat->bt_inq_remain = true;
		else
			ieee80211_queue_delayed_work(rtwdev->hw,
						     &coex->bt_remain_work,
						     4 * HZ);
	}
	coex_stat->bt_acl_busy = ((coex_stat->bt_info_lb2 & BIT(3)) == BIT(3));
	if (chip->ble_hid_profile_support) {
		if (coex_stat->bt_info_lb2 & BIT(5)) {
			if (coex_stat->bt_info_hb1 & BIT(0)) {
				/*BLE HID*/
				coex_stat->bt_ble_hid_exist = true;
			} else {
				coex_stat->bt_ble_hid_exist = false;
			}
			coex_stat->bt_ble_exist = false;
		} else if (coex_stat->bt_info_hb1 & BIT(0)) {
			/*RCU*/
			coex_stat->bt_ble_hid_exist = false;
			coex_stat->bt_ble_exist = true;
		} else {
			coex_stat->bt_ble_hid_exist = false;
			coex_stat->bt_ble_exist = false;
		}
	} else {
		if (coex_stat->bt_info_hb1 & BIT(0)) {
			if (coex_stat->bt_hid_slot == 1 &&
			    coex_stat->hi_pri_rx + 100 < coex_stat->hi_pri_tx &&
			    coex_stat->hi_pri_rx < 100) {
				coex_stat->bt_ble_hid_exist = true;
				coex_stat->bt_ble_exist = false;
			} else {
				coex_stat->bt_ble_hid_exist = false;
				coex_stat->bt_ble_exist = true;
			}
		} else {
			coex_stat->bt_ble_hid_exist = false;
			coex_stat->bt_ble_exist = false;
		}
	}

	coex_stat->cnt_bt[COEX_CNT_BT_RETRY] = coex_stat->bt_info_lb3 & 0xf;
	if (coex_stat->cnt_bt[COEX_CNT_BT_RETRY] >= 1)
		coex_stat->cnt_bt[COEX_CNT_BT_POPEVENT]++;

	coex_stat->bt_fix_2M = ((coex_stat->bt_info_lb3 & BIT(4)) == BIT(4));
	coex_stat->bt_inq = ((coex_stat->bt_info_lb3 & BIT(5)) == BIT(5));
	if (coex_stat->bt_inq)
		coex_stat->cnt_bt[COEX_CNT_BT_INQ]++;

	coex_stat->bt_page = ((coex_stat->bt_info_lb3 & BIT(7)) == BIT(7));
	if (coex_stat->bt_page)
		coex_stat->cnt_bt[COEX_CNT_BT_PAGE]++;

	/* unit: % (value-100 to translate to unit: dBm in coex info) */
	if (chip->bt_rssi_type == COEX_BTRSSI_RATIO) {
		coex_stat->bt_rssi = coex_stat->bt_info_hb0 * 2 + 10;
	} else {
		if (coex_stat->bt_info_hb0 <= 127)
			coex_stat->bt_rssi = 100;
		else if (256 - coex_stat->bt_info_hb0 <= 100)
			coex_stat->bt_rssi = 100 - (256 - coex_stat->bt_info_hb0);
		else
			coex_stat->bt_rssi = 0;
	}

	if (coex_stat->bt_info_hb1 & BIT(1))
		coex_stat->cnt_bt[COEX_CNT_BT_REINIT]++;

	if (coex_stat->bt_info_hb1 & BIT(2)) {
		coex_stat->cnt_bt[COEX_CNT_BT_SETUPLINK]++;
		coex_stat->bt_setup_link = true;
		if (coex_stat->bt_reenable)
			bt_relink_time = 6 * HZ;
		else
			bt_relink_time = 1 * HZ;

		ieee80211_queue_delayed_work(rtwdev->hw,
					     &coex->bt_relink_work,
					     bt_relink_time);

		(void)0;
	}

	if (coex_stat->bt_info_hb1 & BIT(3))
		coex_stat->cnt_bt[COEX_CNT_BT_IGNWLANACT]++;

	coex_stat->bt_ble_voice = ((coex_stat->bt_info_hb1 & BIT(4)) == BIT(4));
	coex_stat->bt_ble_scan_en = ((coex_stat->bt_info_hb1 & BIT(5)) == BIT(5));
	if (coex_stat->bt_info_hb1 & BIT(6))
		coex_stat->cnt_bt[COEX_CNT_BT_ROLESWITCH]++;

	coex_stat->bt_multi_link = ((coex_stat->bt_info_hb1 & BIT(7)) == BIT(7));
	/* for multi_link = 0 but bt pkt remain exist */
	/* Use PS-TDMA to protect WL RX */
	if (!coex_stat->bt_multi_link && coex_stat->bt_multi_link_pre) {
		coex_stat->bt_multi_link_remain = true;
		ieee80211_queue_delayed_work(rtwdev->hw,
					     &coex->bt_multi_link_remain_work,
					     3 * HZ);
	}
	coex_stat->bt_multi_link_pre = coex_stat->bt_multi_link;

	/* resend wifi info to bt, it is reset and lost the info */
	if (coex_stat->bt_info_hb1 & BIT(1)) {
		(void)0;

		if (coex_stat->wl_connected)
			type = COEX_MEDIA_CONNECT;
		else
			type = COEX_MEDIA_DISCONNECT;
		rtw_coex_update_wl_ch_info(rtwdev, type);
	}

	/* if ignore_wlan_act && not set_up_link */
	if ((coex_stat->bt_info_hb1 & BIT(3)) &&
	    (!(coex_stat->bt_info_hb1 & BIT(2)))) {
		(void)0;
		rtw_coex_ignore_wlan_act(rtwdev, false);
	}

	coex_stat->bt_opp_exist = ((coex_stat->bt_info_hb2 & BIT(0)) == BIT(0));
	if (coex_stat->bt_info_hb2 & BIT(1))
		coex_stat->cnt_bt[COEX_CNT_BT_AFHUPDATE]++;

	coex_stat->bt_a2dp_active = (coex_stat->bt_info_hb2 & BIT(2)) == BIT(2);
	coex_stat->bt_slave = ((coex_stat->bt_info_hb2 & BIT(3)) == BIT(3));
	coex_stat->bt_hid_slot = (coex_stat->bt_info_hb2 & 0x30) >> 4;
	coex_stat->bt_hid_pair_num = (coex_stat->bt_info_hb2 & 0xc0) >> 6;
	if (coex_stat->bt_hid_pair_num > 0 && coex_stat->bt_hid_slot >= 2)
		coex_stat->bt_418_hid_exist = true;
	else if (coex_stat->bt_hid_pair_num == 0 || coex_stat->bt_hid_slot == 1)
		coex_stat->bt_418_hid_exist = false;

	if ((coex_stat->bt_info_lb2 & 0x49) == 0x49)
		coex_stat->bt_a2dp_bitpool = (coex_stat->bt_info_hb3 & 0x7f);
	else
		coex_stat->bt_a2dp_bitpool = 0;

	coex_stat->bt_a2dp_sink = ((coex_stat->bt_info_hb3 & BIT(7)) == BIT(7));

	rtw_coex_update_bt_link_info(rtwdev);
	rtw_coex_run_coex(rtwdev, COEX_RSN_BTINFO);
}

#define COEX_BT_HIDINFO_MTK	0x46
static const u8 coex_bt_hidinfo_ps[] = {0x57, 0x69, 0x72};
static const u8 coex_bt_hidinfo_xb[] = {0x58, 0x62, 0x6f};

static void rtw_coex_bt_hid_info_notify(struct rtw_dev *rtwdev, u8 *buf, u8 length)
{
	(void)rtwdev;
	(void)buf;
	(void)length;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_hid *hidinfo;
	struct rtw_coex_hid_info_a *hida;
	struct rtw_coex_hid_handle_list *hl, *bhl;
	u8 sub_id = buf[2], gamehid_cnt = 0, handle, i;
	bool cur_game_hid_exist, complete;

	if (!chip->wl_mimo_ps_support &&
	    (sub_id == COEX_BT_HIDINFO_LIST || sub_id == COEX_BT_HIDINFO_A))
		return;

	(void)0;

	switch (sub_id) {
	case COEX_BT_HIDINFO_LIST:
		hl = &coex_stat->hid_handle_list;
		bhl = (struct rtw_coex_hid_handle_list *)buf;
		if (!memcmp(hl, bhl, sizeof(*hl)))
			return;
		coex_stat->hid_handle_list = *bhl;
		memset(&coex_stat->hid_info, 0, sizeof(coex_stat->hid_info));
		for (i = 0; i < COEX_BT_HIDINFO_HANDLE_NUM; i++) {
			hidinfo = &coex_stat->hid_info[i];
			if (hl->handle[i] != COEX_BT_HIDINFO_NOTCON &&
			    hl->handle[i] != 0)
				hidinfo->hid_handle = hl->handle[i];
		}
		break;
	case COEX_BT_HIDINFO_A:
		hida = (struct rtw_coex_hid_info_a *)buf;
		handle = hida->handle;
		for (i = 0; i < COEX_BT_HIDINFO_HANDLE_NUM; i++) {
			hidinfo = &coex_stat->hid_info[i];
			if (hidinfo->hid_handle == handle) {
				hidinfo->hid_vendor = hida->vendor;
				memcpy(hidinfo->hid_name, hida->name,
				       sizeof(hidinfo->hid_name));
				hidinfo->hid_info_completed = true;
				break;
			}
		}
		break;
	}
	for (i = 0; i < COEX_BT_HIDINFO_HANDLE_NUM; i++) {
		hidinfo = &coex_stat->hid_info[i];
		complete = hidinfo->hid_info_completed;
		handle = hidinfo->hid_handle;
		if (!complete || handle == COEX_BT_HIDINFO_NOTCON ||
		    handle == 0 || handle >= COEX_BT_BLE_HANDLE_THRS) {
			hidinfo->is_game_hid = false;
			continue;
		}

		if (hidinfo->hid_vendor == COEX_BT_HIDINFO_MTK) {
			if ((memcmp(hidinfo->hid_name,
				    coex_bt_hidinfo_ps,
				    COEX_BT_HIDINFO_NAME)) == 0)
				hidinfo->is_game_hid = true;
			else if ((memcmp(hidinfo->hid_name,
					 coex_bt_hidinfo_xb,
					 COEX_BT_HIDINFO_NAME)) == 0)
				hidinfo->is_game_hid = true;
			else
				hidinfo->is_game_hid = false;
		} else {
			hidinfo->is_game_hid = false;
		}
		if (hidinfo->is_game_hid)
			gamehid_cnt++;
	}

	if (gamehid_cnt > 0)
		cur_game_hid_exist = true;
	else
		cur_game_hid_exist = false;

	if (cur_game_hid_exist != coex_stat->bt_game_hid_exist) {
		coex_stat->bt_game_hid_exist = cur_game_hid_exist;
		(void)0;
		rtw_coex_run_coex(rtwdev, COEX_RSN_BTSTATUS);
	}
}

static __attribute__((unused)) void rtw_coex_query_bt_hid_list(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	const struct rtw_chip_info *chip = rtwdev->chip;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	struct rtw_coex_hid *hidinfo;
	u8 i, handle;
	bool complete;

	if (!chip->wl_mimo_ps_support || coex_stat->wl_under_ips ||
	    (coex_stat->wl_under_lps && !coex_stat->wl_force_lps_ctrl))
		return;

	if (!coex_stat->bt_hid_exist &&
	    !((coex_stat->bt_info_lb2 & COEX_INFO_CONNECTION) &&
	      (coex_stat->hi_pri_tx + coex_stat->hi_pri_rx >
	       COEX_BT_GAMEHID_CNT)))
		return;

	rtw_fw_coex_query_hid_info(rtwdev, COEX_BT_HIDINFO_LIST, 0);

	for (i = 0; i < COEX_BT_HIDINFO_HANDLE_NUM; i++) {
		hidinfo = &coex_stat->hid_info[i];
		complete = hidinfo->hid_info_completed;
		handle = hidinfo->hid_handle;
		if (handle == 0 || handle == COEX_BT_HIDINFO_NOTCON ||
		    handle >= COEX_BT_BLE_HANDLE_THRS || complete)
			continue;

		rtw_fw_coex_query_hid_info(rtwdev,
					   COEX_BT_HIDINFO_A,
					   handle);
	}
}

static void rtw_coex_wl_fwdbginfo_notify(struct rtw_dev *rtwdev, u8 *buf, u8 length)
{
	(void)rtwdev;
	(void)buf;
	(void)length;
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;
	u8 val;
	int i;

	(void)0;
	if (WARN(length < 8, "invalid wl info c2h length\n"))
		return;

	if (buf[0] != 0x08)
		return;

	for (i = 1; i < 8; i++) {
		val = coex_stat->wl_fw_dbg_info_pre[i];
		if (buf[i] >= val)
			coex_stat->wl_fw_dbg_info[i] = buf[i] - val;
		else
			coex_stat->wl_fw_dbg_info[i] = 255 - val + buf[i];

		coex_stat->wl_fw_dbg_info_pre[i] = buf[i];
	}

	coex_stat->cnt_wl[COEX_CNT_WL_FW_NOTIFY]++;
	rtw_coex_wl_ccklock_action(rtwdev);
	rtw_coex_wl_ccklock_detect(rtwdev);
}

static void rtw_coex_wl_status_change_notify(struct rtw_dev *rtwdev, u32 type)
{
	(void)rtwdev;
	(void)type;
	rtw_coex_run_coex(rtwdev, COEX_RSN_WLSTATUS);
}

static void rtw_coex_wl_status_check(struct rtw_dev *rtwdev)
{
	(void)rtwdev;
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	if ((coex_stat->wl_under_lps && !coex_stat->wl_force_lps_ctrl) ||
	    coex_stat->wl_under_ips)
		return;

	rtw_coex_monitor_bt_ctr(rtwdev);
}

static void rtw_coex_bt_relink_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.bt_relink_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->bt_setup_link = false;
	rtw_coex_run_coex(rtwdev, COEX_RSN_WLSTATUS);
	
}

static void rtw_coex_bt_reenable_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.bt_reenable_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->bt_reenable = false;
	
}

static void rtw_coex_defreeze_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.defreeze_work.work);
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex->freeze = false;
	coex_stat->wl_hi_pri_task1 = false;
	rtw_coex_run_coex(rtwdev, COEX_RSN_WLSTATUS);
	
}

static void rtw_coex_wl_remain_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.wl_remain_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->wl_gl_busy = test_bit(RTW_FLAG_BUSY_TRAFFIC, rtwdev->flags);
	rtw_coex_run_coex(rtwdev, COEX_RSN_WLSTATUS);
	
}

static void rtw_coex_bt_remain_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.bt_remain_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->bt_inq_remain = coex_stat->bt_inq_page;
	rtw_coex_run_coex(rtwdev, COEX_RSN_BTSTATUS);
	
}

static void rtw_coex_wl_connecting_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.wl_connecting_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->wl_connecting = false;
	(void)0;
	rtw_coex_run_coex(rtwdev, COEX_RSN_WLSTATUS);
	
}

static void rtw_coex_bt_multi_link_remain_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
		coex.bt_multi_link_remain_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->bt_multi_link_remain = false;
	
}

static void rtw_coex_wl_ccklock_work(struct work_struct *work)
{
	(void)work;
	struct rtw_dev *rtwdev = container_of(work, struct rtw_dev,
					      coex.wl_ccklock_work.work);
	struct rtw_coex_stat *coex_stat = &rtwdev->coex.stat;

	
	coex_stat->wl_cck_lock = false;
	
}

