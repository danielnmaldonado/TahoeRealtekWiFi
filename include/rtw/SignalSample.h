/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_SIGNAL_SAMPLE_H
#define RTW_SIGNAL_SAMPLE_H
#include <stddef.h>
#include <stdint.h>

/* Zero is "unavailable", never a received power measurement.
 * The cache is per association/channel and expires in monotonic time.
 * Only real PHY samples refresh its timestamp. Reading STATUS never does. */
#define RTW_SIGNAL_MAX_AGE_MS 10000U
#define RTW_SIGNAL_HELD_AGE_MS 2000U

typedef struct rtw_signal_sample {
    uint8_t bssid[6];
    uint16_t channel;
    int32_t dbm;
    uint64_t sampled_ms;
    uint64_t valid_samples, missing_phy, invalid_phy, foreign_frames;
    uint8_t associated, valid;
} rtw_signal_sample;

static inline int rtw_signal_valid_dbm(int dbm) {
    return dbm >= -127 && dbm < 0;
}
static inline int rtw_signal_same_address(const uint8_t *a, const uint8_t *b) {
    if (!a || !b) return 0;
    for (size_t i = 0; i < 6; ++i) if (a[i] != b[i]) return 0;
    return 1;
}
static inline int rtw_signal_valid_bssid(const uint8_t *bssid) {
    if (!bssid || (bssid[0] & 1U)) return 0;
    unsigned nonzero = 0;
    for (size_t i = 0; i < 6; ++i) nonzero |= bssid[i];
    return nonzero != 0;
}
static inline int rtw_signal_from_ap(const uint8_t *frame, size_t length,
                                      const uint8_t *bssid) {
    if (!frame || length < 24 || !rtw_signal_valid_bssid(bssid) ||
        (frame[0] & 3U) || !rtw_signal_same_address(frame + 10, bssid)) return 0;
    const unsigned type = frame[0] & 0x0cU;
    const unsigned direction = frame[1] & 3U;
    if (type == 0) /* Management: transmitter and BSSID must both match. */
        return direction == 0 && rtw_signal_same_address(frame + 16, bssid);
    return type == 8 && direction == 2; /* Downlink data, not uplink/WDS. */
}
static inline void rtw_signal_reset(rtw_signal_sample *sample) {
    if (!sample) return;
    for (size_t i = 0; i < 6; ++i) sample->bssid[i] = 0;
    sample->channel = 0;
    sample->dbm = 0;
    sample->sampled_ms = 0;
    sample->associated = sample->valid = 0;
    /* Diagnostic counters remain cumulative for the lifetime of the driver. */
}
static inline void rtw_signal_observe(rtw_signal_sample *sample,
    const uint8_t *bssid, uint16_t channel, const uint8_t *frame, size_t length,
    int phy_present, int dbm, uint64_t now_ms) {
    if (!sample) return;
    if (!rtw_signal_valid_bssid(bssid) || !channel) {
        rtw_signal_reset(sample);
        return;
    }
    if (!sample->associated || sample->channel != channel ||
        !rtw_signal_same_address(sample->bssid, bssid)) {
        rtw_signal_reset(sample);
        for (size_t i = 0; i < 6; ++i) sample->bssid[i] = bssid[i];
        sample->channel = channel;
        sample->associated = 1;
    }
    if (sample->valid && now_ms < sample->sampled_ms) {
        sample->valid = 0;
        sample->dbm = 0;
    }
    if (!rtw_signal_from_ap(frame, length, bssid)) {
        ++sample->foreign_frames;
        return;
    }
    if (!phy_present) {
        ++sample->missing_phy;
        return;
    }
    if (!rtw_signal_valid_dbm(dbm)) {
        ++sample->invalid_phy;
        return;
    }
    sample->dbm = dbm;
    sample->sampled_ms = now_ms;
    sample->valid = 1;
    ++sample->valid_samples;
}
static inline int rtw_signal_value(const rtw_signal_sample *sample,
    const uint8_t *bssid, uint16_t channel, uint64_t now_ms) {
    if (!sample || !sample->associated || !sample->valid ||
        sample->channel != channel || !rtw_signal_same_address(sample->bssid, bssid) ||
        now_ms < sample->sampled_ms || now_ms - sample->sampled_ms > RTW_SIGNAL_MAX_AGE_MS)
        return 0;
    return sample->dbm;
}
static inline uint8_t rtw_signal_node_value(int dbm) {
    if (!rtw_signal_valid_dbm(dbm)) return 0;
    /* net80211 has a positive 1..100 scale; retain exact dBm in the cache. */
    return (uint8_t)(dbm <= -99 ? 1 : dbm + 100);
}
#endif
