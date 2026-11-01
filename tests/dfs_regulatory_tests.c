/* SPDX-License-Identifier: GPL-2.0-only */
#include <stdint.h>
#include <stdio.h>
#if __has_include("rtw/DFSRegulatory.h")
#include "rtw/DFSRegulatory.h"
#else
#include "rtw/StationChannels.h"
/* Before the factory-country gate, registration considered only geometry. */
static int rtw_dfs_regulatory_channel(const uint8_t cc[2],uint8_t ch) {
    (void)cc;return rtw_station_is_dfs_channel(ch);
}
#endif
#define CHECK(x) do{if(!(x)){fprintf(stderr,"DFS country failure line%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    const uint8_t eg[2]={'E','G'},us[2]={'U','S'},br[2]={'B','R'},world[2]={'0','0'},blank[2]={255,255};
    CHECK(!rtw_dfs_regulatory_channel(eg,100));
    CHECK(rtw_dfs_regulatory_channel(eg,52));
    CHECK(rtw_dfs_regulatory_channel(us,100));
    CHECK(rtw_dfs_regulatory_channel(br,140));
    CHECK(rtw_dfs_regulatory_channel(world,64));
    CHECK(rtw_dfs_regulatory_channel(blank,120));
    CHECK(rtw_dfs_regulatory_channel(0,120));
    for(unsigned ch=0;ch<256;++ch) {
        const int dfs=(ch>=52&&ch<=64&&ch%4==0)||(ch>=100&&ch<=140&&ch%4==0);
        CHECK(rtw_dfs_regulatory_channel(world,(uint8_t)ch)==dfs);
    }
#if __has_include("rtw/DFSRegulatory.h")
    CHECK(rtw_dfs_regulatory_limit_mbm(world,100)==2000);
    CHECK(rtw_dfs_regulatory_limit_mbm(br,100)==2700);
    CHECK(rtw_dfs_regulatory_limit_mbm(eg,100)==RTW_DFS_REGULATORY_UNAVAILABLE);
    CHECK(rtw_dfs_regulatory_limit_mbm(eg,52)==2000);
#endif
    puts("DFS factory-country/world HT20 limits passed");return 0;
}
