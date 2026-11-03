/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8723BU_H
#define RTW_RTL8723BU_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_8723BU_EFUSE_BYTES 512U
#define RTW_8723BU_FW_HEADER_BYTES 32U
#define RTW_8723BU_FW_MAX_BYTES 32768U
typedef struct {
 void *cookie;
 int (*read)(void*,uint16_t,unsigned,uint32_t*);
 int (*write)(void*,uint16_t,unsigned,uint32_t);
 int (*write_region)(void*,uint16_t,const uint8_t*,size_t);
 void (*delay_us)(void*,uint32_t);
} rtw_8723bu_bus;
typedef struct {
 uint8_t bulk_in,bulk_out[4],bulk_out_count,high_speed;
 uint16_t bulk_max_packet;
} rtw_8723bu_usb;
typedef struct {
 uint8_t logical[RTW_8723BU_EFUSE_BYTES],mac[6],cut,vendor,rom_revision;
 uint8_t crystal_cap,thermal_meter,channel_plan,rf_board_option,bt_setting;
 uint8_t has_wifi,has_bluetooth,has_gps,antenna_option,rfe_option,package_type;
 uint8_t cck_power[6],ht40_power[5];int8_t ofdm_delta,ht20_delta;
} rtw_8723bu_board;
typedef struct rtw_8723bu {
 rtw_8723bu_bus bus;rtw_8723bu_usb usb;rtw_8723bu_board board;
 void *private_state;uint16_t vid,pid,firmware_version;
 uint8_t firmware_subversion,identified,board_valid,powered,firmware_running;
 uint8_t mac_initialized,phy_loaded,calibrated,channel,associated;
 uint8_t tx_rate,tx_sgi,rssi_valid,rssi_percent,mailbox;
 uint32_t epoch;int last_error,cleanup_error;
} rtw_8723bu;
size_t rtw_8723bu_private_size(void);
/* Strict verified USB identities; local initialization never performs I/O.
 * Caller storage remains at the same address for the lifetime of this object. */
int rtw_8723bu_init(rtw_8723bu*,uint16_t,uint16_t,const rtw_8723bu_bus*,const rtw_8723bu_usb*,void*,size_t);
int rtw_8723bu_identify(rtw_8723bu*);
int rtw_8723bu_decode_efuse(const uint8_t*,size_t,uint8_t*,size_t);
int rtw_8723bu_parse_board(rtw_8723bu*,const uint8_t*,size_t);
int rtw_8723bu_read_board(rtw_8723bu*);
int rtw_8723bu_channel_supported(const rtw_8723bu*,uint8_t,unsigned);
int rtw_8723bu_inspect_firmware(const uint8_t*,size_t);
int rtw_8723bu_boot(rtw_8723bu*,const uint8_t*,size_t);
int rtw_8723bu_set_channel(rtw_8723bu*,uint8_t,unsigned);
/* Successful RFK keeps TX paused. The station MAC writes response rates
 * before explicitly allowing active scan/AUTH/RUN transmission. */
int rtw_8723bu_calibrate(rtw_8723bu*);
int rtw_8723bu_h2c(rtw_8723bu*,const uint8_t*,size_t);
/* Unlatched EBUSY means firmware still owns the box; hard I/O stays sticky. */
int rtw_8723bu_report_rssi(rtw_8723bu*,uint8_t percent);
int rtw_8723bu_c2h(rtw_8723bu*,const uint8_t*,size_t);
int rtw_8723bu_tick(rtw_8723bu*);
#define RTW_8723BU_CHECKED_STOP 1
int rtw_8723bu_stop_checked(rtw_8723bu*);
int rtw_8723bu_stop(rtw_8723bu*);
/* Port0 station parameters. No key installation or controlled-port authority. */
typedef struct {
 uint8_t bssid[6],channel,short_slot,short_preamble,ht,sgi20,mcs;
 uint16_t aid,beacon_tu,basic_rates,legacy_rates;
 uint8_t passive_scan;
} rtw_8723bu_station;
typedef struct {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop_32us;} rtw_8723bu_edca;
typedef struct {uint32_t mask;uint8_t raid,sgi,command[8];} rtw_8723bu_rates;
int rtw_8723bu_prepare_rates(uint16_t,uint8_t,int,int,rtw_8723bu_rates*);
int rtw_8723bu_station_state(rtw_8723bu*,unsigned,const rtw_8723bu_station*); /*0INIT1SCAN2AUTH3RUN*/
int rtw_8723bu_set_edca(rtw_8723bu*,int,const rtw_8723bu_edca[4]);
int rtw_8723bu_rssi(uint8_t,const uint8_t*,size_t,int*);
int rtw_8723bu_tx_endpoint(const rtw_8723bu*,uint8_t,uint8_t*);
#ifdef __cplusplus
}
#endif
#endif
