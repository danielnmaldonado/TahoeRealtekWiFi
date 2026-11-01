// SPDX-License-Identifier: GPL-2.0-only
#pragma once
/* Host boundary for real scan ordering and BSS selection. No radio, USB,
 * credential, net80211 scan algorithm or BSS choice is implemented here. */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/tree.h>

#define XYLog(...) ((void)0)
#define DPRINTF(...) ((void)0)
#define IFF_DEBUG 4
#define IFM_AUTO 0
#define IFM_MODE(value) (value)
#define IEEE80211_CHAN_MAX 255
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
#define IEEE80211_CHAN_CCK 0x0020
#define IEEE80211_CHAN_OFDM 0x0040
#define IEEE80211_CHAN_2GHZ 0x0080
#define IEEE80211_CHAN_5GHZ 0x0100
#define IEEE80211_CHAN_PASSIVE 0x0200
#define IEEE80211_CHAN_DYN 0x0400
#define IEEE80211_CHAN_HT20 0x00010000
#define IEEE80211_CHAN_VHT20 0x01000000
#define IEEE80211_IS_CHAN_2GHZ(c) (((c)->ic_flags & IEEE80211_CHAN_2GHZ) != 0)
#define IEEE80211_IS_CHAN_5GHZ(c) (((c)->ic_flags & IEEE80211_CHAN_5GHZ) != 0)
#define IEEE80211_C_SHSLOT 0x00000080
#define IEEE80211_C_SHPREAMBLE 0x00000100
#define IEEE80211_C_QOS 0x00000800
#define IEEE80211_C_RSN 0x00001000
#define IEEE80211_C_SCANALLBAND 0x00008000
#define IEEE80211_F_ASCAN 0x00000001
#define IEEE80211_F_BGSCAN 0x08000000
#define IEEE80211_F_DISABLE_BG_AUTO_CONNECT 0x40000000
#define IEEE80211_F_TX_MGMT_ONLY 1
#define IEEE80211_NODE_HT 0x0400
#define IEEE80211_NODE_VHT 0x10000
#define IEEE80211_NODE_HE 0x200000
#define IEEE80211_M_STA 1
#define IEEE80211_EVT_SCAN_DONE 4
#define IEEE80211_INACT_SCAN 10
#define IEEE80211_BGSCAN_FAIL_MAX 360
#define IEEE80211_RSSI_THRES_RATIO_2GHZ 50
#define IEEE80211_RSSI_THRES_RATIO_5GHZ 40
#define IEEE80211_RSSI_THRES_2GHZ (-60)
#define IEEE80211_RSSI_THRES_5GHZ (-70)
#define IEEE80211_FC0_SUBTYPE_DEAUTH 0xc0
#define IEEE80211_REASON_AUTH_LEAVE 3
#define IEEE80211_ADDR_COPY(dst, src) std::memcpy(dst, src, 6)
#define IEEE80211_SEND_MGMT(ic, ni, type, reason) scan_send_mgmt(ic, ni, type, reason)

enum ieee80211_state { IEEE80211_S_INIT, IEEE80211_S_SCAN, IEEE80211_S_AUTH, IEEE80211_S_ASSOC, IEEE80211_S_RUN };
enum ieee80211_phymode { IEEE80211_MODE_AUTO, IEEE80211_MODE_11A, IEEE80211_MODE_11B, IEEE80211_MODE_11G, IEEE80211_MODE_11N, IEEE80211_MODE_11AC, IEEE80211_MODE_11AX };
struct ieee80211_channel { unsigned ic_flags=0; uint16_t ic_freq=0; };
struct _ifnet { unsigned if_flags=0; char if_xname[16]{}; };
struct scan_media_entry { int ifm_media=IFM_AUTO; };
struct ieee80211_node {
    RB_ENTRY(ieee80211_node) ni_node;
    ieee80211_channel *ni_chan=nullptr;
    unsigned ni_fails=0,ni_rssi=0,ni_flags=0,ni_refcnt=0,ni_inact=0;
    uint8_t ni_macaddr[6]{},ni_bssid[6]{};
    void *ni_unref_arg=nullptr;
    size_t ni_unref_arg_size=0;
    void (*ni_unref_cb)(struct ieee80211com *,ieee80211_node *)=nullptr;
};
RB_HEAD(ieee80211_tree,ieee80211_node);
int ieee80211_node_cmp(ieee80211_node *,ieee80211_node *);
RB_PROTOTYPE(ieee80211_tree,ieee80211_node,ni_node,ieee80211_node_cmp)
struct ieee80211com {
    _ifnet ic_if;
    unsigned ic_caps=0,ic_flags=0,ic_xflags=0,ic_max_rssi=100,ic_bgscan_fail=0;
    ieee80211_state ic_state=IEEE80211_S_SCAN;
    int ic_opmode=IEEE80211_M_STA;
    ieee80211_phymode ic_curmode=IEEE80211_MODE_AUTO;
    struct { scan_media_entry *ifm_cur; } ic_media{};
    struct { unsigned is_scan_active=0; } ic_stats;
    ieee80211_channel ic_channels[IEEE80211_CHAN_MAX+1]{};
    uint8_t ic_chan_active[howmany(IEEE80211_CHAN_MAX+1,NBBY)]{};
    uint8_t ic_chan_scan[howmany(IEEE80211_CHAN_MAX+1,NBBY)]{};
    unsigned ic_scan_count=0;
    ieee80211_node *ic_bss=nullptr;
    ieee80211_tree ic_tree{};
    void (*ic_event_handler)(ieee80211com *,int,void *)=nullptr;
    int (*ic_node_checkrssi)(ieee80211com *,const ieee80211_node *)=nullptr;
    void *test_context=nullptr;
};
struct ieee80211_node_switch_bss_arg { uint8_t cur_macaddr[6],sel_macaddr[6]; };
inline void *malloc(size_t size,int,int) { return std::malloc(size); }
inline const char *ether_sprintf(const uint8_t *) { return "fixture"; }
unsigned au_scan_capabilities();
void register_8814_scan_channels(ieee80211com &);
void register_8192du_scan_channels(ieee80211com &);
void register_8812_scan_channels(ieee80211com &);
void register_88_scan_channels(ieee80211com &);
inline unsigned ieee80211_ieee2mhz(unsigned channel,unsigned band){return band&IEEE80211_CHAN_5GHZ?5000+5*channel:2407+5*channel;}
inline int rtw_8814au_phy_channel_supported(const int *,uint8_t,unsigned width){return width==20;}
inline int rtw_r12au_phy_channel_supported(const int *,uint8_t,unsigned width){return width==20;}
int ieee80211_node_checkrssi(ieee80211com *,const ieee80211_node *);
unsigned ieee80211_chan2ieee(ieee80211com *,const ieee80211_channel *);
void ieee80211_reset_scan(_ifnet *);
void ieee80211_begin_scan(_ifnet *);
void ieee80211_next_scan(_ifnet *);
void ieee80211_end_scan(_ifnet *);
ieee80211_node *ieee80211_node_choose_bss(ieee80211com *,int,ieee80211_node **);
void ieee80211_node_cleanup(ieee80211com *,ieee80211_node *);
void ieee80211_node_raise_inact(void *,ieee80211_node *);
void ieee80211_iterate_nodes(ieee80211com *,void (*)(void *,ieee80211_node *),void *);
void ieee80211_clean_inactive_nodes(ieee80211com *,int);
void ieee80211_free_node(ieee80211com *,ieee80211_node *);
int ieee80211_match_bss(ieee80211com *,ieee80211_node *,int);
void ieee80211_switch_ess(ieee80211com *);
void ieee80211_node_join_bss(ieee80211com *,ieee80211_node *);
void ieee80211_node_switch_bss(ieee80211com *,ieee80211_node *);
void ieee80211_stop_ampdu_tx(ieee80211com *,ieee80211_node *,int);
int scan_send_mgmt(ieee80211com *,ieee80211_node *,int,int);
void ieee80211_new_state(ieee80211com *,ieee80211_state,int);
int ieee80211_setmode(ieee80211com *,ieee80211_phymode);
ieee80211_phymode ieee80211_next_mode(_ifnet *);
ieee80211_phymode ieee80211_chan2mode(ieee80211com *,ieee80211_channel *);
