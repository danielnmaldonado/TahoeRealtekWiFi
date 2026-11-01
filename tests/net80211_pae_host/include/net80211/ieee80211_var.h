#ifndef RTW_PAE_HOST_VAR_H
#define RTW_PAE_HOST_VAR_H
#include "../../platform.hpp"
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_crypto.h>
#include <net80211/ieee80211_node.h>
#include <net80211/ieee80211_proto.h>
#define IEEE80211_PROTO_RSN (1 << 0)
#define IEEE80211_PROTO_WPA (1 << 1)
#define IEEE80211_GROUP_NKID 6
#define IEEE80211_C_RSN 0x00001000
#define IEEE80211_C_MFP 0x00002000
#define IEEE80211_F_WEPON 0x00000100
#define IEEE80211_F_RSNON 0x00200000
#define IEEE80211_F_PSK 0x00400000
#define IEEE80211_F_MFPR 0x01000000
#define IEEE80211_F_PBAR 0x04000000
enum { IEEE80211_M_STA=1, IEEE80211_M_MONITOR=8 };
struct ieee80211com {
    _ifnet ic_if;
    uint32_t ic_caps,ic_flags,ic_rsnprotos,ic_rsnakms,ic_rsnciphers;
    ieee80211_cipher ic_rsngroupcipher,ic_rsngroupmgmtcipher;
    uint8_t ic_myaddr[6],ic_psk[32],ic_nonce[32],ic_globalcnt[32];
    int ic_opmode,ic_def_txkey,ic_igtk_kid;
    ieee80211_state ic_state;
    ieee80211_node *ic_bss;
    ieee80211_key ic_nw_keys[IEEE80211_GROUP_NKID];
    TAILQ_HEAD(,ieee80211_pmk) ic_pmksa;
    int (*ic_set_key)(ieee80211com *,ieee80211_node *,ieee80211_key *);
    void (*ic_delete_key)(ieee80211com *,ieee80211_node *,ieee80211_key *);
    int (*ic_send_mgmt)(ieee80211com *,ieee80211_node *,int,int,int);
    int (*ic_newstate)(ieee80211com *,ieee80211_state,int);
    struct {
        unsigned is_rx_eapol_badmic,is_rx_eapol_key,is_rx_eapol_replay;
        unsigned is_rx_nombuf,is_rx_remmicfail,is_rx_elem_toosmall;
    } ic_stats;
};
void ieee80211_disable_rsn(ieee80211com *);
void ieee80211_node_leave_rsn(ieee80211com *,ieee80211_node *);
void ieee80211_node_cleanup(ieee80211com *,ieee80211_node *);
void ieee80211_ba_del(ieee80211_node *);
void ieee80211_ba_free(ieee80211_node *);
void ieee80211_count_rekeysta(void *,ieee80211_node *);
void ieee80211_setkeysdone(ieee80211com *);
typedef void ieee80211_iter_func(void *,ieee80211_node *);
void ieee80211_iterate_nodes(ieee80211com *,ieee80211_iter_func *,void *);
#endif
