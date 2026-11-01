/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_DFS_REGULATORY_H
#define RTW_DFS_REGULATORY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define RTW_DFS_REGULATORY_UNAVAILABLE (-32768)
/* Factory-country DFS20 metadata. Missing/unrecognized ROM country uses the
 * pinned Linux world baseline; no locale/IP or AP country override. A true
 * result is NOT a complete TX authority: backend power and observed-BSS
 * station checks remain required. Non-DFS/ch144 return unavailable. */
int rtw_dfs_regulatory_channel(const uint8_t country[2],uint8_t channel);
int16_t rtw_dfs_regulatory_limit_mbm(const uint8_t country[2],uint8_t channel);
#ifdef __cplusplus
}
#endif
#endif
