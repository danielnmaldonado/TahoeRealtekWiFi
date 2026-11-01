#ifndef RTW_NET80211_BA_HOST_HPP
#define RTW_NET80211_BA_HOST_HPP
#include "net80211_ht_host/platform.hpp"
#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_priv.h>
/* Model the next MAC-stage queue, not the BA algorithm or cipher. */
struct mbuf_list { std::vector<mbuf_t> packets; };
#define MBUF_LIST_INITIALIZER() {}
int if_input(_ifnet *,mbuf_list *);
void ieee80211_input_ba(ieee80211com *,mbuf_t,ieee80211_node *,int,ieee80211_rxinfo *,mbuf_list *);
void ieee80211_input_ba_seq(ieee80211com *,ieee80211_node *,uint8_t,uint16_t,mbuf_list *);
void ieee80211_input_ba_flush(ieee80211com *,ieee80211_node *,ieee80211_rx_ba *,mbuf_list *);
int ieee80211_input_ba_gap_skip(ieee80211_rx_ba *);
void ieee80211_input_ba_gap_timeout(void *);
void ieee80211_ba_move_window(ieee80211com *,ieee80211_node *,uint8_t,uint16_t,mbuf_list *);
void ieee80211_recv_bar(ieee80211com *,mbuf_t,ieee80211_node *);
void ieee80211_bar_tid(ieee80211com *,ieee80211_node *,uint8_t,uint16_t);
void ieee80211_recv_addba_req(ieee80211com *,mbuf_t,ieee80211_node *);
void ieee80211_recv_addba_resp(ieee80211com *,mbuf_t,ieee80211_node *);
#endif
