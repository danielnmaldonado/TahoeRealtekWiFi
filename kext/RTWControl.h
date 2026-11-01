/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef RTW_CONTROL_H
#define RTW_CONTROL_H

#include <stddef.h>
#include <stdint.h>

/* Fixed-layout, little-endian v1 ABI shared by the kext and its local client.
 * No pointers, passwords, variable structures, or raw USB/register operations.
 * Every reserved byte, including unused JOIN SSID storage, must be zero.
 * JOIN supports WPA2-Personal with CCMP only. Its PMK is already derived in
 * user space; neither passphrase storage nor key derivation is a kernel API. */
#define RTW_CONTROL_VERSION 1U
#define RTW_CONTROL_USER_CLIENT_TYPE 0U
#define RTW_CONTROL_SERVICE_CLASS "RTWUSBController"
#define RTW_CONTROL_BUNDLE_ID "org.research.TahoeRealtekWiFi"
#define RTW_CONTROL_MAX_SSID 32U
#define RTW_CONTROL_MAX_NETWORKS 64U

enum RTWControlSelector {
    RTW_CONTROL_STATUS = 0,
    RTW_CONTROL_SCAN = 1,
    RTW_CONTROL_JOIN = 2,
    RTW_CONTROL_LEAVE = 3,
    RTW_CONTROL_SCAN_RESULTS = 4,
    RTW_CONTROL_CHANNEL_MAP = 5, /* additive read-only exact registered map */
    RTW_CONTROL_SELECTOR_COUNT = 6
};

enum RTWControlStatusFlags {
    RTW_CONTROL_DEVICE = 1U << 0,
    RTW_CONTROL_RADIO = 1U << 1,
    RTW_CONTROL_RUNNING = 1U << 2,
    RTW_CONTROL_AUTHORIZED = 1U << 3,
    RTW_CONTROL_SCANNING = 1U << 4,
    /* Optional output capabilities; ABI v1 layout and reserved bytes unchanged. */
    RTW_CONTROL_SIGNAL_TRACKED = 1U << 5, /* RSSI has driver-enforced freshness. */
    RTW_CONTROL_SIGNAL_HELD = 1U << 6,   /* Valid sample is at least 2 seconds old. */
    /* Independent live station channel groups, not silicon-family claims.
     * Older drivers omit both flags and retain channels 1-11 only. Neither
     * group authorizes DFS, wider channels or the other 5-GHz group. */
    RTW_CONTROL_BAND_5GHZ = 1U << 7, /* low 36/40/44/48 */
    RTW_CONTROL_BAND_5GHZ_HIGH = 1U << 8 /* upper 149/153/157/161/165 */,
    RTW_CONTROL_BAND_2GHZ = 1U << 9,
    RTW_CONTROL_BANDS_EXPLICIT = 1U << 10 /* absence retains legacy 2.4 GHz */,
    /* Managed-station DFS groups. No AP operation or channel144 permission.
     * Advertised only when the registered backend covers the complete group. */
    RTW_CONTROL_BAND_5GHZ_DFS_LOW = 1U << 11, /* 52/56/60/64 */
    RTW_CONTROL_BAND_5GHZ_DFS_MID = 1U << 12  /* 100..140 in steps of4 */
};

enum RTWControlSecurityFlags {
    RTW_CONTROL_SECURITY_PRIVACY = 1U << 0,
    RTW_CONTROL_SECURITY_WPA2_PSK = 1U << 1,
    RTW_CONTROL_SECURITY_CCMP = 1U << 2,
    RTW_CONTROL_SECURITY_UNSUPPORTED = 1U << 3
};

typedef struct RTWControlRequest {
    uint32_t version;
    uint32_t reserved[3];
} RTWControlRequest;

typedef struct RTWControlReply {
    uint32_t version;
    uint32_t reserved[3];
} RTWControlReply;

typedef struct RTWControlJoin {
    uint32_t version;
    uint32_t ssidLength;
    uint8_t ssid[RTW_CONTROL_MAX_SSID];
    uint8_t pmk[32];
    uint32_t reserved[2];
} RTWControlJoin;

typedef struct RTWControlStatus {
    uint32_t version;
    uint32_t flags;
    uint32_t state;                 /* net80211 IEEE80211_S_* state value */
    uint32_t channel;               /* IEEE channel number; 0 if unknown */
    int32_t rssi;                   /* dBm; 0 if unavailable */
    uint32_t usbError;              /* most recent IOReturn, or 0 */
    uint8_t mac[6];
    uint8_t bssid[6];
    uint32_t ssidLength;
    uint8_t ssid[RTW_CONTROL_MAX_SSID];
    uint64_t txPackets;
    uint64_t rxPackets;
    uint64_t txErrors;
    uint64_t rxErrors;
    uint32_t reserved[4];
} RTWControlStatus;

typedef struct RTWControlScanRecord {
    uint8_t ssid[RTW_CONTROL_MAX_SSID];
    uint8_t bssid[6];
    uint8_t ssidLength;
    uint8_t channel;
    int16_t rssi;                   /* dBm; 0 if unavailable */
    uint16_t reserved;
    uint32_t securityFlags;
} RTWControlScanRecord;

typedef struct RTWControlScanResults {
    uint32_t version;
    uint32_t count;                 /* number of initialized records, <=64 */
    uint32_t truncated;             /* exactly 0 or 1 */
    uint32_t reserved[3];
    RTWControlScanRecord records[RTW_CONTROL_MAX_NETWORKS];
} RTWControlScanResults;

/* Registered station geometry, not a fresh-BSS/TX lease. Existing v1 layouts
 * and full-group status bits are unchanged. Older drivers reject selector5. */
#define RTW_CONTROL_CHANNEL_MAP_BYTES 32U
typedef struct RTWControlChannelMap {
    uint32_t version;
    uint32_t reserved[3];
    uint8_t channels[RTW_CONTROL_CHANNEL_MAP_BYTES]; /* bitN is IEEE channelN */
} RTWControlChannelMap;

static inline int rtw_control_channel_map_contains(const RTWControlChannelMap* map,uint32_t channel) {
    return map && channel<256U && (map->channels[channel/8U]&(1U<<(channel%8U)))!=0;
}
static inline int rtw_control_channel_map_valid(const RTWControlChannelMap* map) {
    if(!map||map->version!=RTW_CONTROL_VERSION||map->reserved[0]||map->reserved[1]||map->reserved[2])return 0;
    for(uint32_t channel=0;channel<256U;++channel)if(rtw_control_channel_map_contains(map,channel)) {
        if(!((channel>=1U&&channel<=11U)||
             (channel>=36U&&channel<=64U&&channel%4U==0U)||
             (channel>=100U&&channel<=140U&&channel%4U==0U)||
             (channel>=149U&&channel<=165U&&(channel-149U)%4U==0U)))return 0;
    }
    return 1;
}

enum RTWControlValidation {
    RTW_CONTROL_VALID = 0,
    RTW_CONTROL_BAD_SELECTOR,
    RTW_CONTROL_BAD_SIZE,
    RTW_CONTROL_BAD_VERSION,
    RTW_CONTROL_BAD_RESERVED,
    RTW_CONTROL_BAD_SSID
};

static inline size_t rtw_control_input_size(uint32_t selector) {
    if (selector >= RTW_CONTROL_SELECTOR_COUNT) return 0;
    return selector == RTW_CONTROL_JOIN ? sizeof(RTWControlJoin) : sizeof(RTWControlRequest);
}

/* Keep user-space connection eligibility aligned with this capability's
 * narrow channel contract. Geometry, DFS and other 5-GHz groups do not imply
 * permission to connect; later extensions need an explicit capability. */
static inline int rtw_control_channel_supported(uint32_t channel, uint32_t flags) {
    return (channel >= 1U && channel <= 11U &&
            (!(flags & RTW_CONTROL_BANDS_EXPLICIT) || (flags & RTW_CONTROL_BAND_2GHZ))) ||
        ((flags & RTW_CONTROL_BAND_5GHZ) &&
         (channel == 36U || channel == 40U || channel == 44U || channel == 48U)) ||
        ((flags & RTW_CONTROL_BAND_5GHZ_HIGH) &&
         (channel == 149U || channel == 153U || channel == 157U || channel == 161U || channel == 165U)) ||
        ((flags & RTW_CONTROL_BAND_5GHZ_DFS_LOW) &&
         channel>=52U && channel<=64U && channel%4U==0U) ||
        ((flags & RTW_CONTROL_BAND_5GHZ_DFS_MID) &&
         channel>=100U && channel<=140U && channel%4U==0U);
}

static inline size_t rtw_control_output_size(uint32_t selector) {
    if (selector >= RTW_CONTROL_SELECTOR_COUNT) return 0;
    if (selector == RTW_CONTROL_STATUS) return sizeof(RTWControlStatus);
    if (selector == RTW_CONTROL_SCAN_RESULTS) return sizeof(RTWControlScanResults);
    if (selector == RTW_CONTROL_CHANNEL_MAP) return sizeof(RTWControlChannelMap);
    return sizeof(RTWControlReply);
}

/* Read bytes rather than dereferencing possibly unaligned external input. */
static inline uint32_t rtw_control_load32(const uint8_t* bytes) {
    return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U) |
        ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}

static inline int rtw_control_zeroes(const uint8_t* bytes, size_t count) {
    for (size_t i = 0; i < count; ++i) if (bytes[i]) return 0;
    return 1;
}

static inline enum RTWControlValidation rtw_control_validate_request(
    uint32_t selector, const void* input, size_t inputLength) {
    const size_t expected = rtw_control_input_size(selector);
    if (!expected) return RTW_CONTROL_BAD_SELECTOR;
    if (!input || inputLength != expected) return RTW_CONTROL_BAD_SIZE;
    const uint8_t* bytes = (const uint8_t*)input;
    if (rtw_control_load32(bytes) != RTW_CONTROL_VERSION) return RTW_CONTROL_BAD_VERSION;
    if (selector == RTW_CONTROL_JOIN) {
        const uint32_t length = rtw_control_load32(bytes + offsetof(RTWControlJoin, ssidLength));
        if (!length || length > RTW_CONTROL_MAX_SSID) return RTW_CONTROL_BAD_SSID;
        if (!rtw_control_zeroes(bytes + offsetof(RTWControlJoin, ssid) + length,
                               RTW_CONTROL_MAX_SSID - length) ||
            !rtw_control_zeroes(bytes + offsetof(RTWControlJoin, reserved),
                               sizeof(((RTWControlJoin*)0)->reserved)))
            return RTW_CONTROL_BAD_RESERVED;
    } else if (!rtw_control_zeroes(bytes + offsetof(RTWControlRequest, reserved),
                                   sizeof(((RTWControlRequest*)0)->reserved))) {
        return RTW_CONTROL_BAD_RESERVED;
    }
    return RTW_CONTROL_VALID;
}

#if defined(__cplusplus)
#define RTW_CONTROL_ASSERT(c, m) static_assert((c), m)
#define RTW_CONTROL_ALIGNOF(t) alignof(t)
#else
#define RTW_CONTROL_ASSERT(c, m) _Static_assert((c), m)
#define RTW_CONTROL_ALIGNOF(t) _Alignof(t)
#endif
RTW_CONTROL_ASSERT(sizeof(RTWControlRequest) == 16, "request ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlRequest) == 4, "request ABI alignment");
RTW_CONTROL_ASSERT(sizeof(RTWControlReply) == 16, "reply ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlReply) == 4, "reply ABI alignment");
RTW_CONTROL_ASSERT(sizeof(RTWControlJoin) == 80, "join ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlJoin) == 4, "join ABI alignment");
RTW_CONTROL_ASSERT(offsetof(RTWControlJoin, pmk) == 40, "PMK ABI offset");
RTW_CONTROL_ASSERT(sizeof(RTWControlStatus) == 120, "status ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlStatus) == 8, "status ABI alignment");
RTW_CONTROL_ASSERT(offsetof(RTWControlStatus, txPackets) == 72, "counter ABI offset");
RTW_CONTROL_ASSERT(sizeof(RTWControlScanRecord) == 48, "scan record ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlScanRecord) == 4, "scan record ABI alignment");
RTW_CONTROL_ASSERT(sizeof(RTWControlScanResults) == 3096, "scan results ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlScanResults) == 4, "scan results ABI alignment");
RTW_CONTROL_ASSERT(offsetof(RTWControlScanResults, records) == 24, "scan records ABI offset");
RTW_CONTROL_ASSERT(sizeof(RTWControlChannelMap) == 48, "channel map ABI size");
RTW_CONTROL_ASSERT(RTW_CONTROL_ALIGNOF(RTWControlChannelMap) == 4, "channel map ABI alignment");
RTW_CONTROL_ASSERT(offsetof(RTWControlChannelMap, channels) == 16, "channel bitmap ABI offset");
#undef RTW_CONTROL_ASSERT
#undef RTW_CONTROL_ALIGNOF

#endif
