/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_RTL8710BU_H
#define RTW_RTL8710BU_H
#include <stddef.h>
#include <stdint.h>
#include "USBRegisterIO.h"
#include "FirmwareBoot.h"
#include "RTL8710BUDataPath.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct rtw_r10bu rtw_r10bu;
typedef struct {uint8_t bulk_in,bulk_out[3],bulk_out_count;uint16_t max_packet;} rtw_r10bu_usb;
typedef struct {
 uint8_t mac[6],crystal,thermal,channel_plan,cut,vendor,rom_revision,package,boot_eeprom,has_eeprom;
 uint16_t vid,pid;uint8_t txpower[12];
 uint8_t logical[512]; /* complete immutable OTP identity, including reserved bytes */
} rtw_r10bu_board;
typedef struct {uint16_t version,payload_bytes;uint8_t subversion;} rtw_r10bu_firmware;
typedef struct {
 uint8_t identified,powered,firmware,mac,phy,calibrated,tx_enabled;
 uint8_t primary,center,width_mhz,max_bw,tx_iqk_mask,rx_iqk_mask,thermal;
 uint32_t thermal_samples,thermal_recalibrations;int error;
} rtw_r10bu_status;
typedef struct {uint8_t primary,center,width_mhz,ht;} rtw_r10bu_operating;
typedef struct {uint32_t mask;uint8_t rate_id,initial,ht,width_mhz,sgi;} rtw_r10bu_rate;
typedef struct {uint8_t aifsn,ecwmin,ecwmax;uint16_t txop;} rtw_r10bu_edca;
size_t rtw_r10bu_size(void);
int rtw_r10bu_init(void*,size_t,const rtw_usb_rw_transport*,const rtw_waiter*,const rtw_r10bu_usb*,rtw_r10bu**);
int rtw_r10bu_decode_rom(const uint8_t*,size_t,rtw_r10bu_board*);
int rtw_r10bu_validate_firmware(const uint8_t*,size_t,rtw_r10bu_firmware*);
int rtw_r10bu_identify(rtw_r10bu*);
int rtw_r10bu_board_info(const rtw_r10bu*,rtw_r10bu_board*);
int rtw_r10bu_status_read(const rtw_r10bu*,rtw_r10bu_status*);
/* Exact8710BU boot includes indirect SYSON, its own power/LLT/0x1000-FW/MAC/BB/RF and measured
 * RFK. TX remains paused until an actual operating profile is resumed. */
enum {RTW_R10BU_SMIC=0,RTW_R10BU_UMC=2};
/* The caller selects the firmware blob by actual identified silicon vendor. */
int rtw_r10bu_boot(rtw_r10bu*,const uint8_t*,size_t,uint8_t firmware_vendor);
int rtw_r10bu_set_operating(rtw_r10bu*,const rtw_r10bu_operating*);
int rtw_r10bu_channel_supported(const rtw_r10bu_board*,const rtw_r10bu_operating*);
int rtw_r10bu_resume(rtw_r10bu*);
int rtw_r10bu_invalidate(rtw_r10bu*);
void rtw_r10bu_forget(rtw_r10bu*);
int rtw_r10bu_pause(rtw_r10bu*);
#define RTW_R10BU_CHECKED_STOP 1
/* Checked teardown preserves the original error and returns cleanup only. */
int rtw_r10bu_stop_checked(rtw_r10bu*);
int rtw_r10bu_stop(rtw_r10bu*);
int rtw_r10bu_decode_phy(const rtw_r10bu*,uint8_t,const uint8_t*,size_t,int*);
int rtw_r10bu_make_tx(const rtw_r10bu_tx_options*,const rtw_r10bu_rate*,uint8_t[40]);
int rtw_r10bu_tick(rtw_r10bu*);
int rtw_r10bu_read(rtw_r10bu*,uint16_t,uint8_t,uint32_t*);
int rtw_r10bu_write(rtw_r10bu*,uint16_t,uint8_t,uint32_t);
int rtw_r10bu_update(rtw_r10bu*,uint16_t,uint8_t,uint32_t,uint32_t);
int rtw_r10bu_tx_endpoint(const rtw_r10bu*,uint8_t,unsigned*);
int rtw_r10bu_h2c(rtw_r10bu*,const uint8_t[8]);
/* Unlatched EBUSY means firmware still owns the box; hard I/O stays sticky. */
int rtw_r10bu_report_rssi(rtw_r10bu*,uint8_t percent);
int rtw_r10bu_rate_build(uint16_t,const uint8_t[2],int ht,unsigned width,int local_sgi,int peer_sgi,rtw_r10bu_rate*);
int rtw_r10bu_rate_command(const rtw_r10bu_rate*,int reset,uint8_t[8]);
int rtw_r10bu_station(rtw_r10bu*,int associated,const uint8_t bssid[6],uint16_t aid,uint16_t interval,uint16_t basic_rates,int short_slot,int ht);
int rtw_r10bu_edca_update(rtw_r10bu*,int short_slot,const rtw_r10bu_edca[4]);
#ifdef __cplusplus
}
#endif
#endif
