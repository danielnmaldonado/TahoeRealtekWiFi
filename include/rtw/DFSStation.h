/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef RTW_DFS_STATION_H
#define RTW_DFS_STATION_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_DFS_OBSERVATIONS 32U
#define RTW_DFS_BEACON_FRESH_MS 30000U
#define RTW_DFS_AP_POWER_UNSPECIFIED 127
#define RTW_DFS_AP_POWER_INVALID (-129)
typedef struct {
 uint64_t seen_ms,auth_seen_ms; /* power evidence / visible AUTH evidence */
 uint8_t bssid[6],ssid[32],ssid_length,channel,valid,power_reduction;
 uint8_t has_power_limit,auth_valid;
 int8_t max_power_dbm;
} rtw_dfs_observation;
typedef struct {rtw_dfs_observation observations[RTW_DFS_OBSERVATIONS];uint8_t next,blocked,pending,target_channel,mode,count;} rtw_dfs_station;
enum {RTW_DFS_IGNORED=0,RTW_DFS_OBSERVED=1,RTW_DFS_SWITCH=2};
int rtw_dfs_channel(uint8_t channel);
void rtw_dfs_txpause_guard(uint8_t held,uint8_t dual_mac,uint16_t address,uint8_t *bytes,size_t length);
void rtw_dfs_station_reset(rtw_dfs_station *);
/* RX channel must come from a successfully tuned backend; nonzero physical
 * evidence must agree. Only full broadcast ESS beacons with the desired SSID
 * establish/refresh an AUTH lease. A bound AP may update power metadata
 * while hiding/changing its SSID; that never creates/refreshes an AUTH lease.
 * This never grants active-scan/AP permission. */
int rtw_dfs_station_observe(rtw_dfs_station *,const uint8_t *frame,size_t length,
 uint8_t tuned_channel,uint8_t physical_channel,const uint8_t *ssid,size_t ssid_length,
 const uint8_t local[6],const uint8_t associated_bssid[6],uint64_t now_ms);
/* Signed dBm AP Country triplet cap,127 if no Country IE, -129 if stale
 * or the advertised Country ranges omit this channel. No country override. */
int rtw_dfs_station_power_limit(const rtw_dfs_station *,const uint8_t bssid[6],
 const uint8_t *ssid,size_t ssid_length,uint8_t channel,uint64_t now_ms);
/* Fresh bound-AP power evidence is usable even with no visible AUTH lease.
 * Returns0..255 dB reduction, -1 if stale or Country ranges omit the channel. */
int rtw_dfs_station_power_reduction(const rtw_dfs_station *,const uint8_t bssid[6],
 const uint8_t *ssid,size_t ssid_length,uint8_t channel,uint64_t now_ms);
/* Requires a fresh visible matching beacon; hidden power updates cannot
 * establish or renew this AUTH authority. */
int rtw_dfs_station_authorize(const rtw_dfs_station *,const uint8_t bssid[6],
 const uint8_t *ssid,size_t ssid_length,uint8_t channel,uint64_t now_ms);
#ifdef __cplusplus
}
#endif
#endif
