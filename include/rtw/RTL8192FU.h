/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8192FU_H
#define RTW_RTL8192FU_H
#include <stddef.h>
#include <stdint.h>
#include "USBRegisterIO.h"
#include "FirmwareBoot.h"
#include "RTL8192FUDataPath.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rtw_r92fu rtw_r92fu;
typedef struct {uint8_t bulk_in,bulk_out[3],bulk_out_count;uint16_t max_packet;} rtw_r92fu_usb;
typedef struct {
 uint8_t mac[6],rfe,crystal,thermal,channel_plan,cut,vendor,rom_revision;
 uint16_t vid,pid;uint8_t txpower[2][42];
} rtw_r92fu_board;
typedef struct {uint16_t version,payload_bytes;uint8_t subversion;} rtw_r92fu_firmware;
typedef struct {
 uint8_t identified,powered,firmware,mac,phy,calibrated,tx_enabled;
 uint8_t primary,center,width_mhz,max_bw,tx_iqk_mask,rx_iqk_mask,thermal;
 uint32_t thermal_samples,thermal_recalibrations;int error;
} rtw_r92fu_status;
typedef struct {uint8_t primary,center,width_mhz,ht;} rtw_r92fu_operating;
typedef struct {uint32_t mask;uint8_t rate_id,initial,ht,width_mhz,sgi;} rtw_r92fu_rate;
typedef struct {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop;} rtw_r92fu_edca;
size_t rtw_r92fu_size(void);
int rtw_r92fu_init(void*,size_t,const rtw_usb_rw_transport*,const rtw_waiter*,const rtw_r92fu_usb*,rtw_r92fu**);
int rtw_r92fu_decode_rom(const uint8_t*,size_t,rtw_r92fu_board*);
int rtw_r92fu_validate_firmware(const uint8_t*,size_t,rtw_r92fu_firmware*);
int rtw_r92fu_identify(rtw_r92fu*);
int rtw_r92fu_board_info(const rtw_r92fu*,rtw_r92fu_board*);
int rtw_r92fu_status_read(const rtw_r92fu*,rtw_r92fu_status*);
/* Exact FU boot includes its own power/LLT/0x4000-FW/MAC/BB/RF and measured
 * RFK. TX remains paused until an actual operating profile is resumed. */
int rtw_r92fu_boot(rtw_r92fu*,const uint8_t*,size_t);
int rtw_r92fu_set_operating(rtw_r92fu*,const rtw_r92fu_operating*);
int rtw_r92fu_channel_supported(const rtw_r92fu_board*,const rtw_r92fu_operating*);
int rtw_r92fu_resume(rtw_r92fu*);
int rtw_r92fu_invalidate(rtw_r92fu*);
void rtw_r92fu_forget(rtw_r92fu*);
int rtw_r92fu_pause(rtw_r92fu*);
int rtw_r92fu_stop(rtw_r92fu*);
int rtw_r92fu_decode_phy(const rtw_r92fu*,uint8_t,const uint8_t*,size_t,int*);
int rtw_r92fu_make_tx(const rtw_r92fu_tx_options*,const rtw_r92fu_rate*,uint8_t[40]);
int rtw_r92fu_tick(rtw_r92fu*);
int rtw_r92fu_read(rtw_r92fu*,uint16_t,uint8_t,uint32_t*);
int rtw_r92fu_write(rtw_r92fu*,uint16_t,uint8_t,uint32_t);
int rtw_r92fu_update(rtw_r92fu*,uint16_t,uint8_t,uint32_t,uint32_t);
int rtw_r92fu_tx_endpoint(const rtw_r92fu*,uint8_t,unsigned*);
int rtw_r92fu_h2c(rtw_r92fu*,const uint8_t[8]);
int rtw_r92fu_rate_build(uint16_t,const uint8_t[2],int ht,unsigned width,int local_sgi,int peer_sgi,rtw_r92fu_rate*);
int rtw_r92fu_rate_command(const rtw_r92fu_rate*,int reset,uint8_t[8]);
int rtw_r92fu_station(rtw_r92fu*,int associated,const uint8_t bssid[6],uint16_t aid,uint16_t interval,uint16_t basic_rates,int short_slot,int ht);
int rtw_r92fu_edca_update(rtw_r92fu*,int short_slot,const rtw_r92fu_edca[4]);
#ifdef __cplusplus
}
#endif
#endif
