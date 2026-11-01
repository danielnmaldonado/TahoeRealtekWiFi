/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/DFSRegulatory.h"
#include <stddef.h>
struct dfs_regulatory_row {uint8_t country[2];int16_t limit_mbm[15];};
#include "DFSRegulatoryTables.inc"
int16_t rtw_dfs_regulatory_limit_mbm(const uint8_t country[2],uint8_t channel) {
    static const uint8_t centers[15]={52,56,60,64,100,104,108,112,116,120,124,128,132,136,140};
    size_t slot,row=0;
    for(slot=0;slot<15;slot++)if(centers[slot]==channel)break;
    if(slot==15)return RTW_DFS_REGULATORY_UNAVAILABLE;
    /* World row00 sorts first in the validated, generated database. */
    if(country)for(size_t i=0;i<sizeof(dfs_regulatory_rows)/sizeof(dfs_regulatory_rows[0]);i++)
        if(dfs_regulatory_rows[i].country[0]==country[0]&&dfs_regulatory_rows[i].country[1]==country[1]){row=i;break;}
    return dfs_regulatory_rows[row].limit_mbm[slot];
}
int rtw_dfs_regulatory_channel(const uint8_t country[2],uint8_t channel) {
    return rtw_dfs_regulatory_limit_mbm(country,channel)!=RTW_DFS_REGULATORY_UNAVAILABLE;
}
