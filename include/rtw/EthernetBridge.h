#ifndef RTW_ETHERNET_BRIDGE_H
#define RTW_ETHERNET_BRIDGE_H
/* SPDX-License-Identifier: GPL-2.0-only
 * Allocation-free Ethernet/802.11 adapter for a future RTL8192EU driver.
 * No USB I/O, descriptor encoding, firmware, radio or live TX is implemented.
 * No entry point proves that hardware, association, or keys work. Do not
 * publish a network interface merely because these offline helpers pass.
 *
 * Caller owns all storage and MUST serialize every call, including state
 * setters, with RX/TX completion. Callbacks must not reenter or mutate this
 * bridge, the callback table, input aggregate, or scratch. No internal locks.
 */
#include "rtw/StationPort.h"
#include "rtw/RTL8192EU.h"
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

#define RTW_BRIDGE_MAX_AGGREGATE_BYTES 65536U

typedef enum {
    RTW_BRIDGE_OK=0,
    RTW_BRIDGE_BAD_ARGUMENT,
    RTW_BRIDGE_DOWN,
    RTW_BRIDGE_NOT_ASSOCIATED,
    RTW_BRIDGE_UNAUTHORIZED,
    RTW_BRIDGE_NEEDS_ENCRYPTION,
    RTW_BRIDGE_FRAME_REJECTED,
    RTW_BRIDGE_MALFORMED_AGGREGATE,
    RTW_BRIDGE_NO_CONSUMER,
    RTW_BRIDGE_CONSUMER_FAILED
} rtw_bridge_status;

typedef enum {
    RTW_BRIDGE_FCS_ABSENT=0,
    RTW_BRIDGE_FCS_INCLUDED
} rtw_bridge_fcs;

typedef struct {
    rtw_station_port port;
    uint16_t tx_sequence;
    uint8_t admin_enabled;
    uint8_t real_datapath_ready;
    rtw_bridge_fcs rx_fcs;
} rtw_ethernet_bridge;

/* Return nonzero only when the consumer accepted the frame synchronously.
 * Bytes are borrowed ONLY until the callback returns. Copy to retain them.
 * EAPOL callbacks receive Ethernet-II; management receives the full raw MPDU
 * with FCS removed, validated header and supported body framing. Management
 * bytes remain unauthenticated observations and MUST NOT authorize the port.
 */
typedef int (*rtw_bridge_consumer)(void* context, const uint8_t* frame,
                                    size_t length);
typedef struct {
    void* context;
    rtw_bridge_consumer host_ethernet;
    rtw_bridge_consumer supplicant_eapol;
    rtw_bridge_consumer management_mpdu; /* optional */
} rtw_bridge_callbacks;

typedef struct {
    size_t descriptors;
    size_t dropped;
    size_t c2h_skipped;
    size_t host_delivered;
    size_t eapol_delivered;
    size_t management_delivered;
    size_t bytes_consumed;
    rtw_packet_error aggregate_error;
    rtw_frame_error frame_error; /* most recent codec rejection, if any */
    rtw_port_error port_error;   /* most recent controlled-port rejection */
} rtw_bridge_rx_stats;

/* Reset/detach erase configuration, readiness and association. Disabling,
 * losing real readiness, and disconnect clear association/authorization and
 * TX sequence. Reenable/ready never restores a previous association. */
void rtw_bridge_reset(rtw_ethernet_bridge* bridge);
void rtw_bridge_detach(rtw_ethernet_bridge* bridge);
void rtw_bridge_disconnect(rtw_ethernet_bridge* bridge);
void rtw_bridge_set_admin(rtw_ethernet_bridge* bridge, int enabled);
/* True only from the external driver after actual radio + data-path checks;
 * firmware inspection, mock tests or a management parser are not evidence. */
void rtw_bridge_set_real_ready(rtw_ethernet_bridge* bridge, int ready);
/* Always revoke association/authorization and reset sequence, even on failure.
 * Configuring retains admin/readiness, supports MAC aliases into bridge.port.
 * FCS presence must come from the actual configured RX hardware format. */
rtw_bridge_status rtw_bridge_configure(rtw_ethernet_bridge* bridge,
    const uint8_t station[6], const uint8_t bssid[6],
    rtw_port_security security, rtw_bridge_fcs rx_fcs);
/* The positive evidence flags are supplied ONLY by a real external driver:
 * actual AP association and actual port authorization (including verified
 * installed keys/MIC/replay for RSN). Parsed StationMgmt state is insufficient.
 * False association disconnects; false authorization revokes authorization. */
rtw_bridge_status rtw_bridge_set_associated(rtw_ethernet_bridge* bridge,
                                            int real_association_evidence);
rtw_bridge_status rtw_bridge_authorize(rtw_ethernet_bridge* bridge,
                                       int real_authorization_evidence);
/* OPEN requires admin + real readiness + association + authorization. RSN
 * always stays down because this module has no CCMP transmit implementation. */
int rtw_bridge_carrier(const rtw_ethernet_bridge* bridge);

/* Emits a raw unprotected MPDU without FCS or any USB TX descriptor. This is
 * buffer preparation only, never live USB TX. Sequence advances only on OK,
 * wraps at 4096. RSN follows StationPort's conservative EAPOL/CCMP TX policy.
 * Input/output must not overlap; output must not overlap bridge storage.
 * All output/control objects must be separate from each other and input. */
rtw_bridge_status rtw_bridge_tx(rtw_ethernet_bridge* bridge,
    const uint8_t* ethernet, size_t length, uint8_t* output, size_t capacity,
    size_t* written, rtw_frame_error* frame_error);

/* At most 64 KiB of RTL8192EU descriptor/MPDU records. Two-pass decoding
 * validates the COMPLETE aggregate before any callback: malformed/truncated
 * aggregates deliver nothing (stats bytes_consumed=0). Decoder-defined final
 * padding is allowed. Individual bad/unsupported records are dropped and
 * counted; successful traversal returns OK even if all records were dropped.
 * Missing data/EAPOL consumers, too-small scratch, or a false callback return
 * stop processing. Earlier successful callbacks remain delivered; the failed
 * callback is not counted as delivered. bytes_consumed includes that record.
 * Optional missing management consumer only drops its record.
 *
 * CRC/ICV failures, C2H, descriptor fragmentation, security/SWDEC metadata,
 * protected frames, and all Retry frames are rejected. Rejecting every Retry
 * intentionally trades packet loss for safety until real duplicate/replay
 * tracking exists. Only OPEN authorized data reaches host_ethernet; EAPOL
 * always goes solely to supplicant_eapol (RSN requires association, never IP).
 * Management observations may be reported before association but cannot
 * change bridge state. Only supported response/beacon body framing is parsed.
 *
 * Scratch must not overlap the aggregate, bridge, callbacks, stats or control
 * objects. Direct scratch/aggregate/bridge/table/stats overlap is rejected.
 * Scratch plaintext produced by this call is zeroed before return. No output
 * length is exposed for rejected plaintext. stats is required, reset on entry.
 * An empty aggregate is valid; nonempty NULL inputs are invalid. */
rtw_bridge_status rtw_bridge_rx(const rtw_ethernet_bridge* bridge,
    const uint8_t* aggregate, size_t length,
    uint8_t* ethernet_scratch, size_t scratch_capacity,
    const rtw_bridge_callbacks* callbacks, rtw_bridge_rx_stats* stats);

const char* rtw_bridge_status_string(rtw_bridge_status status);
#ifdef __cplusplus
}
#endif
#endif
