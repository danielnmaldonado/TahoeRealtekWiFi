#ifndef RTW_HOST_NET80211_VAR_H
#define RTW_HOST_NET80211_VAR_H
#include "../../host_platform.hpp"
#include <net80211/ieee80211.h>
#include <net80211/ieee80211_crypto.h>

#define IEEE80211_ADDR_COPY(dst, src) memcpy((dst), (src), IEEE80211_ADDR_LEN)
struct ieee80211com {
    struct {
        unsigned is_tx_nombuf;
        unsigned is_rx_nombuf;
        unsigned is_ccmp_replays;
        unsigned is_ccmp_dec_errs;
    } ic_stats;
};

inline unsigned ieee80211_get_hdrlen(const struct ieee80211_frame *wh) {
    unsigned length = ieee80211_has_addr4(wh) ? 30 : 24;
    if (ieee80211_has_qos(wh)) length += 2;
    if (ieee80211_has_htc(wh)) length += 4;
    return length;
}
#endif
