#ifndef RTW_HT_HOST_VAR_H
#define RTW_HT_HOST_VAR_H
#include "../../platform.hpp"
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_crypto.h>
#include <net80211/ieee80211_node.h>
#include <net80211/ieee80211_proto.h>
#include <netinet/ip.h>
struct llc {
    uint8_t llc_dsap,llc_ssap,llc_control;
    struct { uint8_t org_code[3]; uint16_t ether_type; } __packed llc_snap;
} __packed;
#define LLC_SNAPFRAMELEN 8
#define LLC_SNAP_LSAP 0xaa
#define LLC_UI 3
#define IEEE80211_PROTO_RSN 1
#define IEEE80211_C_AMSDU_IN_AMPDU 0x00020000
#define IEEE80211_C_TX_AMPDU 0x00010000
#define IEEE80211_C_TX_AMPDU_SETUP_IN_RS 0x00100000
#define IEEE80211_F_WEPON 0x00000100
#define IEEE80211_F_RSNON 0x00200000
#define IEEE80211_F_QOS 0x00080000
#define IEEE80211_F_COUNTERM 0x00800000
#define IEEE80211_F_BGSCAN 0x08000000
#define IEEE80211_F_DISABLE_BG_AUTO_CONNECT 0x40000000
#define IEEE80211_F_TX_MGMT_ONLY 1
#define IEEE80211_F_PBAR 0x04000000
#define IEEE80211_CHAN_ANYC ((ieee80211_channel *)-1)
enum { IEEE80211_M_STA=1 };
struct ieee80211com {
    _ifnet ic_if;
    uint32_t ic_caps,ic_flags,ic_htcaps,ic_xflags;
    ieee80211_state ic_state;
    unsigned ic_tid_noack;
    int ic_opmode;
    ieee80211_node *ic_bss;
    ieee80211_tree ic_tree;
    unsigned ic_nnodes;
    CTimeout *ic_rsn_timeout;
    void (*ic_delete_key)(ieee80211com *,ieee80211_node *,ieee80211_key *);
    void (*ic_node_free)(ieee80211com *,ieee80211_node *);
    int (*ic_ampdu_tx_start)(ieee80211com *,ieee80211_node *,uint8_t);
    int (*ic_ampdu_rx_start)(ieee80211com *,ieee80211_node *,uint8_t);
    int (*ic_send_mgmt)(ieee80211com *,ieee80211_node *,int,int,int);
    struct {
        unsigned is_tx_nombuf,is_tx_nonode,is_tx_noauth;
        unsigned is_rx_nombuf,is_ccmp_replays,is_ccmp_dec_errs;
        unsigned is_ht_rx_frame_below_ba_winstart,is_ht_rx_ba_no_buf;
        unsigned is_ht_rx_frame_above_ba_winend,is_ht_rx_ba_window_jump;
        unsigned is_ht_rx_ba_window_slide,is_ht_rx_ba_frame_lost;
        unsigned is_ht_rx_ba_window_gap_timeout,is_ht_rx_ba_agreements;
        unsigned is_ht_tx_ba_agreements,is_pbac_errs;
    } ic_stats;
};
inline unsigned ieee80211_get_hdrlen(const ieee80211_frame *wh) {
    return (ieee80211_has_addr4(wh) ? 30 : 24) + (ieee80211_has_qos(wh) ? 2 : 0) + (ieee80211_has_htc(wh) ? 4 : 0);
}
void ieee80211_del_ess(ieee80211com *,char *,int,int);
void ieee80211_node_set_timeouts(ieee80211_node *);
void ieee80211_node_cleanup(ieee80211com *,ieee80211_node *);
void ieee80211_node_free(ieee80211com *,ieee80211_node *);
void ieee80211_node_copy(ieee80211com *,ieee80211_node *,const ieee80211_node *);
void ieee80211_ba_del(ieee80211_node *);
void ieee80211_ba_free(ieee80211_node *);
void ieee80211_free_node(ieee80211com *,ieee80211_node *);
void ieee80211_free_allnodes(ieee80211com *,int);
void ieee80211_node_leave_ht(ieee80211com *,ieee80211_node *);
mbuf_t ieee80211_encap(_ifnet *,mbuf_t,ieee80211_node **);
int ieee80211_can_use_ampdu(ieee80211com *,ieee80211_node *);
#endif
