/* SPDX-License-Identifier: BSD-3-Clause */
/* Portable RTL8814AU host contract. Register algorithms are adapted from
 * Linux rtw88 (GPL-2.0 OR BSD-3-Clause), Copyright Realtek Corporation.
 * No successful bootstrap operation implies that the PHY is ready. */
#ifndef RTW_RTL8814AU_H
#define RTW_RTL8814AU_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

enum rtw_8814au_speed { RTW_8814AU_FULL_SPEED=1, RTW_8814AU_HIGH_SPEED=2,
    RTW_8814AU_SUPER_SPEED=3 };
enum { RTW_8814AU_QSEL_BEACON=16, RTW_8814AU_QSEL_HIGH=17,
    RTW_8814AU_QSEL_MGMT=18, RTW_8814AU_QSEL_H2C=19 };
struct rtw_8814au_bus_ops {
    void *cookie;
    /* Width is 1, 2 or 4 bytes. Return a positive errno on failure. */
    int (*read)(void *, uint16_t, unsigned, uint32_t *);
    int (*write)(void *, uint16_t, unsigned, uint32_t);
    /* Complete USB bytes, including TX40 descriptor; qsel selects a pipe.
     * Must return only after a bounded transfer has completed. */
    int (*bulk_write)(void *, uint8_t qsel, const uint8_t *, size_t);
    void (*delay_us)(void *, uint32_t);
};
struct rtw_8814au_board {
    uint8_t logical[512];
    uint32_t sys_cfg, sys_cfg2, hpon;
    uint8_t cut, vendor, mp_chip, rfe_option, mac[6];
    uint8_t crystal_cap, thermal_meter, channel_plan, country_code[2];
    uint8_t rf_board_option, bt_setting, btcoex, share_ant, regd;
    uint8_t pa_type_2g, pa_type_5g, lna_type_2g, lna_type_5g;
    uint8_t ext_pa_2g, ext_pa_5g, ext_lna_2g, ext_lna_5g;
    uint8_t tx_bb_swing_setting_2g, tx_bb_swing_setting_5g;
    uint8_t physical_rf_paths, tx_streams, rx_streams, antenna_tx, antenna_rx;
    uint8_t txpwr_idx_table[4][42];
};
struct rtw_8814au_fifo {
    uint16_t rsvd_boundary, rsvd_pg_num, rsvd_drv_pg_num, txff_pg_num;
    uint16_t acq_pg_num, rsvd_drv_addr, rsvd_h2c_info_addr;
    uint16_t rsvd_h2c_sta_info_addr, rsvd_h2cq_addr;
    uint16_t rsvd_cpu_instr_addr, rsvd_fw_txbuf_addr;
};
struct rtw_8814au_fw_segment { size_t offset, size; uint32_t address; };
struct rtw_8814au_fw_info {
    uint16_t version; uint8_t subversion;
    struct rtw_8814au_fw_segment dmem, imem, emem;
};
struct rtw_8814au_phy;
struct rtw_8814au {
    struct rtw_8814au_bus_ops bus;
    struct rtw_8814au_board board;
    struct rtw_8814au_fifo fifo;
    struct rtw_8814au_phy *phy;
    int io_error;
    enum rtw_8814au_speed speed;
    uint8_t output_endpoints, h2c_box;
    uint16_t h2c_seq;
    uint8_t identified, board_valid, powered, fw_running, mac_initialized;
    uint8_t phy_initialized; /* Owned by the PHY layer, never set by boot. */
};

void rtw_8814au_init(struct rtw_8814au *, const struct rtw_8814au_bus_ops *);
int rtw_8814au_read(struct rtw_8814au *, uint16_t, unsigned, uint32_t *);
int rtw_8814au_write(struct rtw_8814au *, uint16_t, unsigned, uint32_t);
int rtw_8814au_update(struct rtw_8814au *, uint16_t, unsigned, uint32_t mask, uint32_t value);
int rtw_8814au_poll(struct rtw_8814au *, uint16_t, unsigned, uint32_t mask,
    uint32_t expected, uint32_t delay_us, unsigned attempts);
void rtw_8814au_delay(struct rtw_8814au *, uint32_t);
int rtw_8814au_decode_efuse(const uint8_t *, size_t, uint8_t *, size_t);
int rtw_8814au_parse_board(struct rtw_8814au *, const uint8_t logical[512], enum rtw_8814au_speed);
/* OTP access writes only its access/clock/address registers. It does not run
 * card-enable, MAC_TRX, BB/RF enable, tables or firmware. */
int rtw_8814au_identify_read_board(struct rtw_8814au *, enum rtw_8814au_speed);
int rtw_8814au_power_on(struct rtw_8814au *);
int rtw_8814au_init_mac(struct rtw_8814au *, unsigned endpoints, enum rtw_8814au_speed);
int rtw_8814au_inspect_fw(const uint8_t *, size_t, struct rtw_8814au_fw_info *);
int rtw_8814au_download_fw(struct rtw_8814au *, const uint8_t *, size_t);
int rtw_8814au_stop(struct rtw_8814au *);
int rtw_8814au_h2c_mailbox(struct rtw_8814au *, const uint8_t command[8]);
int rtw_8814au_h2c_packet(struct rtw_8814au *, const uint8_t packet[32]);
/* After MAC queues, PHY tables and RX transfers are established. */
int rtw_8814au_send_startup_info(struct rtw_8814au *);
#ifdef __cplusplus
}
#endif
#endif
