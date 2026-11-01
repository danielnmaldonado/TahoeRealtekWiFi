/* SPDX-License-Identifier: BSD-3-Clause */
/* Pinned Linuxrtw88 Copyright(c)2018-2019,2024 Realtek Corporation.
 * Dual GPL-2.0 OR BSD-3-Clause; this port selects BSD-3-Clause.
 * See RTL8821AUCoex-Upstream.json and RTL8812AUPHY-LICENSE.txt. */
static inline void rtw_coex_set_init(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	chip->ops->coex_set_init(rtwdev);
}

static inline
void rtw_coex_set_ant_switch(struct rtw_dev *rtwdev, u8 ctrl_type, u8 pos_type)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	if (!chip->ops->coex_set_ant_switch)
		return;

	chip->ops->coex_set_ant_switch(rtwdev, ctrl_type, pos_type);
}

static inline void rtw_coex_set_gnt_fix(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	chip->ops->coex_set_gnt_fix(rtwdev);
}

static inline void rtw_coex_set_gnt_debug(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	chip->ops->coex_set_gnt_debug(rtwdev);
}

static inline  void rtw_coex_set_rfe_type(struct rtw_dev *rtwdev)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	chip->ops->coex_set_rfe_type(rtwdev);
}

static inline void rtw_coex_set_wl_tx_power(struct rtw_dev *rtwdev, u8 wl_pwr)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	chip->ops->coex_set_wl_tx_power(rtwdev, wl_pwr);
}

static inline
void rtw_coex_set_wl_rx_gain(struct rtw_dev *rtwdev, bool low_gain)
{
	const struct rtw_chip_info *chip = rtwdev->chip;

	chip->ops->coex_set_wl_rx_gain(rtwdev, low_gain);
}

static void rtw_coex_info_response(struct rtw_dev *rtwdev, struct sk_buff *skb);
static u32 rtw_coex_read_indirect_reg(struct rtw_dev *rtwdev, u16 addr);
static void rtw_coex_write_indirect_reg(struct rtw_dev *rtwdev, u16 addr,
				 u32 mask, u32 val);
static void rtw_coex_write_scbd(struct rtw_dev *rtwdev, u16 bitpos, bool set);
static void rtw_coex_query_bt_info(struct rtw_dev *rtwdev);

static void rtw_coex_bt_relink_work(struct work_struct *work);
static void rtw_coex_bt_reenable_work(struct work_struct *work);
static void rtw_coex_defreeze_work(struct work_struct *work);
static void rtw_coex_wl_remain_work(struct work_struct *work);
static void rtw_coex_bt_remain_work(struct work_struct *work);
static void rtw_coex_wl_connecting_work(struct work_struct *work);
static void rtw_coex_bt_multi_link_remain_work(struct work_struct *work);
static void rtw_coex_wl_ccklock_work(struct work_struct *work);

static void rtw_coex_power_on_setting(struct rtw_dev *rtwdev);
static void rtw_coex_power_off_setting(struct rtw_dev *rtwdev);
static void rtw_coex_init_hw_config(struct rtw_dev *rtwdev, bool wifi_only);
static void rtw_coex_ips_notify(struct rtw_dev *rtwdev, u8 type);
static void rtw_coex_lps_notify(struct rtw_dev *rtwdev, u8 type);
static void rtw_coex_scan_notify(struct rtw_dev *rtwdev, u8 type);
static void rtw_coex_connect_notify(struct rtw_dev *rtwdev, u8 type);
static void rtw_coex_media_status_notify(struct rtw_dev *rtwdev, u8 type);
static void rtw_coex_bt_info_notify(struct rtw_dev *rtwdev, u8 *buf, u8 length);
static void rtw_coex_bt_hid_info_notify(struct rtw_dev *rtwdev, u8 *buf, u8 length);
static void rtw_coex_wl_fwdbginfo_notify(struct rtw_dev *rtwdev, u8 *buf, u8 length);
static void rtw_coex_switchband_notify(struct rtw_dev *rtwdev, u8 type);
static void rtw_coex_wl_status_change_notify(struct rtw_dev *rtwdev, u32 type);
static void rtw_coex_wl_status_check(struct rtw_dev *rtwdev);
static void rtw_coex_query_bt_hid_list(struct rtw_dev *rtwdev);


static inline bool rtw_coex_disabled(struct rtw_dev *rtwdev)
{
	struct rtw_coex *coex = &rtwdev->coex;
	struct rtw_coex_stat *coex_stat = &coex->stat;

	return coex_stat->bt_disabled;
}

static inline void rtw_coex_active_query_bt_info(struct rtw_dev *rtwdev)
{
	/* The RTL8821AU firmware doesn't send C2H_BT_INFO by itself
	 * when bluetooth headphones are disconnected, so we have to
	 * ask for it regularly.
	 */
	if (rtwdev->chip->id == RTW_CHIP_TYPE_8821A && rtwdev->efuse.btcoex)
		rtw_coex_query_bt_info(rtwdev);
}

