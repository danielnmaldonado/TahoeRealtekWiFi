/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/EthernetBridge.h"
#include "rtw/FCS.h"
#include <string.h>

/* Difference comparisons avoid end-pointer overflow for hostile capacities. */
static int overlaps(const void* a,size_t an,const void* b,size_t bn) {
    uintptr_t ap=(uintptr_t)a, bp=(uintptr_t)b;
    if (!a || !b || !an || !bn) return 0;
    return ap<=bp ? bp-ap<an : ap-bp<bn;
}
static rtw_bridge_status port_status(rtw_port_error error) {
    switch(error) {
    case RTW_PORT_OK:return RTW_BRIDGE_OK;
    case RTW_PORT_BAD_ARGUMENT:return RTW_BRIDGE_BAD_ARGUMENT;
    case RTW_PORT_NOT_ASSOCIATED:return RTW_BRIDGE_NOT_ASSOCIATED;
    case RTW_PORT_UNAUTHORIZED:return RTW_BRIDGE_UNAUTHORIZED;
    case RTW_PORT_NEEDS_ENCRYPTION:return RTW_BRIDGE_NEEDS_ENCRYPTION;
    default:return RTW_BRIDGE_FRAME_REJECTED;
    }
}
static int active(const rtw_ethernet_bridge* b) {
    return b->admin_enabled && b->real_datapath_ready;
}
void rtw_bridge_reset(rtw_ethernet_bridge* b) {
    if (b) memset(b,0,sizeof(*b));
}
void rtw_bridge_detach(rtw_ethernet_bridge* b) {
    rtw_bridge_reset(b);
}
void rtw_bridge_disconnect(rtw_ethernet_bridge* b) {
    if (!b) return;
    b->port.associated=0;
    b->port.authorized=0;
    b->tx_sequence=0;
}
void rtw_bridge_set_admin(rtw_ethernet_bridge* b,int enabled) {
    if (!b) return;
    if (!enabled) rtw_bridge_disconnect(b);
    b->admin_enabled=(uint8_t)(enabled!=0);
}
void rtw_bridge_set_real_ready(rtw_ethernet_bridge* b,int ready) {
    if (!b) return;
    if (!ready) rtw_bridge_disconnect(b);
    b->real_datapath_ready=(uint8_t)(ready!=0);
}
rtw_bridge_status rtw_bridge_configure(rtw_ethernet_bridge* b,
    const uint8_t station[6],const uint8_t bssid[6],
    rtw_port_security security,rtw_bridge_fcs rx_fcs) {
    uint8_t station_copy[6],bssid_copy[6];
    if (!b) return RTW_BRIDGE_BAD_ARGUMENT;
    /* Snapshot first: station/bssid may refer to the previous port. */
    if (station) memcpy(station_copy,station,sizeof(station_copy));
    if (bssid) memcpy(bssid_copy,bssid,sizeof(bssid_copy));
    rtw_port_reset(&b->port);
    b->tx_sequence=0;
    b->rx_fcs=RTW_BRIDGE_FCS_ABSENT;
    if (!station || !bssid ||
        (rx_fcs!=RTW_BRIDGE_FCS_ABSENT && rx_fcs!=RTW_BRIDGE_FCS_INCLUDED))
        return RTW_BRIDGE_BAD_ARGUMENT;
    if (rtw_port_configure(&b->port,station_copy,bssid_copy,security)!=RTW_PORT_OK)
        return RTW_BRIDGE_BAD_ARGUMENT;
    b->rx_fcs=rx_fcs;
    return RTW_BRIDGE_OK;
}
rtw_bridge_status rtw_bridge_set_associated(rtw_ethernet_bridge* b,
                                            int evidence) {
    if (!b) return RTW_BRIDGE_BAD_ARGUMENT;
    rtw_bridge_disconnect(b);
    if (!evidence) return RTW_BRIDGE_OK;
    if (!b->port.configured) return RTW_BRIDGE_BAD_ARGUMENT;
    if (!active(b)) return RTW_BRIDGE_DOWN;
    return port_status(rtw_port_associated(&b->port));
}
rtw_bridge_status rtw_bridge_authorize(rtw_ethernet_bridge* b,int evidence) {
    if (!b) return RTW_BRIDGE_BAD_ARGUMENT;
    b->port.authorized=0;
    if (!evidence) return RTW_BRIDGE_UNAUTHORIZED;
    if (!active(b)) return RTW_BRIDGE_DOWN;
    return port_status(rtw_port_authorize(&b->port,1));
}
int rtw_bridge_carrier(const rtw_ethernet_bridge* b) {
    return b && active(b) && b->port.security==RTW_PORT_OPEN &&
           rtw_port_carrier(&b->port);
}
rtw_bridge_status rtw_bridge_tx(rtw_ethernet_bridge* b,
    const uint8_t* ethernet,size_t length,uint8_t* output,size_t capacity,
    size_t* written,rtw_frame_error* frame_error) {
    rtw_port_error result;
    if (written) *written=0;
    if (frame_error) *frame_error=RTW_FRAME_OK;
    if (!b || !ethernet || !output || !written || !frame_error ||
        overlaps(ethernet,length,output,capacity) ||
        overlaps(b,sizeof(*b),output,capacity)) return RTW_BRIDGE_BAD_ARGUMENT;
    if (!active(b)) return RTW_BRIDGE_DOWN;
    result=rtw_port_tx(&b->port,ethernet,length,b->tx_sequence,
                       output,capacity,written,frame_error);
    if (result==RTW_PORT_OK)
        b->tx_sequence=(uint16_t)((b->tx_sequence+1U)&0x0FFFU);
    return port_status(result);
}

static int eapol_frame(const uint8_t* ethernet,size_t length) {
    return length>=14U && ethernet[12]==0x88U && ethernet[13]==0x8EU;
}
/* Conservative subset of AP-originated management frames. Validate complete
 * fixed fields and IE boundaries; this never authenticates their contents. */
static int management_valid(const rtw_ethernet_bridge* b,
                             const uint8_t* mpdu,size_t length) {
    uint16_t fc;
    unsigned subtype;
    size_t fixed,i;
    if (length<24U || length>4096U) return 0;
    fc=(uint16_t)((unsigned)mpdu[0]|((unsigned)mpdu[1]<<8U));
    if ((fc&0x000FU) || (fc&0xCF00U) || (mpdu[22]&15U)) return 0;
    if (memcmp(mpdu+10,b->port.bssid,6U) ||
        memcmp(mpdu+16,b->port.bssid,6U) ||
        (memcmp(mpdu+4,b->port.station,6U) && !(mpdu[4]&1U))) return 0;
    subtype=(fc>>4U)&15U;
    switch(subtype) {
    case 1U:case 3U:fixed=6U;break; /* association/reassociation response */
    case 5U:case 8U:fixed=12U;break; /* probe response/beacon */
    case 10U:case 12U:return length==26U; /* disassociation/deauthentication */
    case 11U:fixed=6U;break; /* authentication */
    default:return 0;
    }
    if (length<24U+fixed) return 0;
    i=24U+fixed;
    while (i<length) {
        if (length-i<2U || (size_t)mpdu[i+1U]>length-i-2U) return 0;
        i+=2U+(size_t)mpdu[i+1U];
    }
    return 1;
}

rtw_bridge_status rtw_bridge_rx(const rtw_ethernet_bridge* b,
    const uint8_t* aggregate,size_t length,uint8_t* scratch,size_t capacity,
    const rtw_bridge_callbacks* callbacks,rtw_bridge_rx_stats* stats) {
    rtw_rx_iterator iterator;
    rtw_8192eu_rx_frame frame;
    rtw_packet_error decode;
    /* Reject aliases before resetting stats, preserving immutable input. */
    if (!stats || overlaps(stats,sizeof(*stats),b,b?sizeof(*b):0U) ||
        overlaps(stats,sizeof(*stats),aggregate,length) ||
        overlaps(stats,sizeof(*stats),scratch,capacity) ||
        overlaps(stats,sizeof(*stats),callbacks,callbacks?sizeof(*callbacks):0U))
        return RTW_BRIDGE_BAD_ARGUMENT;
    memset(stats,0,sizeof(*stats));
    if (!b || (!aggregate && length) || !scratch ||
        length>RTW_BRIDGE_MAX_AGGREGATE_BYTES ||
        overlaps(scratch,capacity,aggregate,length) ||
        overlaps(scratch,capacity,b,sizeof(*b)) ||
        overlaps(scratch,capacity,callbacks,callbacks?sizeof(*callbacks):0U))
        return RTW_BRIDGE_BAD_ARGUMENT;
    if (!active(b)) return RTW_BRIDGE_DOWN;
    if (!b->port.configured) return RTW_BRIDGE_NOT_ASSOCIATED;
    if (b->rx_fcs!=RTW_BRIDGE_FCS_ABSENT && b->rx_fcs!=RTW_BRIDGE_FCS_INCLUDED)
        return RTW_BRIDGE_BAD_ARGUMENT;
    /* No partial delivery from structurally invalid aggregates. */
    rtw_rx_iterator_init(&iterator,aggregate,length);
    do {decode=rtw_8192eu_rx_next(&iterator,&frame);} while (decode==RTW_PACKETS_OK);
    if (decode!=RTW_PACKETS_DONE) {
        stats->aggregate_error=decode;
        return RTW_BRIDGE_MALFORMED_AGGREGATE;
    }
    rtw_rx_iterator_init(&iterator,aggregate,length);
    while ((decode=rtw_8192eu_rx_next(&iterator,&frame))==RTW_PACKETS_OK) {
        const uint8_t* mpdu=frame.payload.mpdu;
        size_t mpdu_length=frame.payload.mpdu_length,written=0;
        rtw_port_error result;
        rtw_frame_error error;
        rtw_bridge_consumer consumer;
        int is_eapol;
        unsigned type;
        ++stats->descriptors;
        stats->bytes_consumed=iterator.cursor;
        if (frame.payload.crc_error || frame.payload.icv_error) {
            ++stats->dropped;continue;
        }
        if (frame.report_is_c2h) {++stats->c2h_skipped;continue;}
        if (frame.fragmented || frame.security_code || frame.sw_decrypt_required) {
            ++stats->dropped;continue;
        }
        if (b->rx_fcs==RTW_BRIDGE_FCS_INCLUDED) {
            if (!rtw_fcs_valid(mpdu,mpdu_length)) {++stats->dropped;continue;}
            mpdu_length-=4U;
        }
        /* Reject every Retry until real per-peer/TID duplicate tracking exists.
         * Protected input is never trusted even when SWDEC is clear. */
        if (mpdu_length<24U || mpdu_length>4096U || (mpdu[0]&3U) ||
            (mpdu[1]&0x4CU) || (mpdu[22]&15U)) {
            ++stats->dropped;continue;
        }
        type=(mpdu[0]>>2U)&3U;
        if (type==0U) {
            if (!management_valid(b,mpdu,mpdu_length) ||
                !callbacks || !callbacks->management_mpdu) {
                ++stats->dropped;continue;
            }
            if (!callbacks->management_mpdu(callbacks->context,mpdu,mpdu_length)) {
                ++stats->dropped;return RTW_BRIDGE_CONSUMER_FAILED;
            }
            ++stats->management_delivered;continue;
        }
        if (type!=2U) {++stats->dropped;continue;}
        if (!b->port.associated) {
            ++stats->dropped;return RTW_BRIDGE_NOT_ASSOCIATED;
        }
        result=rtw_port_rx(&b->port,mpdu,mpdu_length,0,
                           scratch,capacity,&written,&error);
        if (result!=RTW_PORT_OK) {
            stats->port_error=result;
            stats->frame_error=error;
            ++stats->dropped;
            if (written) memset(scratch,0,written);
            if (error==RTW_FRAME_BUFFER_TOO_SMALL) return RTW_BRIDGE_FRAME_REJECTED;
            continue;
        }
        is_eapol=eapol_frame(scratch,written);
        /* Defense in depth: RSN plaintext can only be dedicated EAPOL. */
        if (!is_eapol && (b->port.security!=RTW_PORT_OPEN ||
                          !rtw_bridge_carrier(b))) {
            memset(scratch,0,written);++stats->dropped;continue;
        }
        consumer=callbacks ? (is_eapol ? callbacks->supplicant_eapol :
                                         callbacks->host_ethernet) : NULL;
        if (!consumer) {
            memset(scratch,0,written);++stats->dropped;
            return RTW_BRIDGE_NO_CONSUMER;
        }
        if (!consumer(callbacks->context,scratch,written)) {
            memset(scratch,0,written);++stats->dropped;
            return RTW_BRIDGE_CONSUMER_FAILED;
        }
        memset(scratch,0,written);
        if (is_eapol) ++stats->eapol_delivered;
        else ++stats->host_delivered;
    }
    /* The preflight and nonmutation contract make this unreachable for valid
     * callers, but retain a deterministic failure if an iterator ever fails. */
    if (decode!=RTW_PACKETS_DONE) {
        stats->aggregate_error=decode;return RTW_BRIDGE_MALFORMED_AGGREGATE;
    }
    stats->bytes_consumed=length;
    return RTW_BRIDGE_OK;
}
const char* rtw_bridge_status_string(rtw_bridge_status status) {
    switch(status) {
    case RTW_BRIDGE_OK:return "ok";
    case RTW_BRIDGE_BAD_ARGUMENT:return "invalid bridge argument";
    case RTW_BRIDGE_DOWN:return "administratively down or real datapath not ready";
    case RTW_BRIDGE_NOT_ASSOCIATED:return "real association required";
    case RTW_BRIDGE_UNAUTHORIZED:return "real authorization required";
    case RTW_BRIDGE_NEEDS_ENCRYPTION:return "CCMP TX not implemented";
    case RTW_BRIDGE_FRAME_REJECTED:return "malformed frame or insufficient scratch";
    case RTW_BRIDGE_MALFORMED_AGGREGATE:return "malformed or truncated RTL8192EU aggregate";
    case RTW_BRIDGE_NO_CONSUMER:return "frame consumer unavailable";
    case RTW_BRIDGE_CONSUMER_FAILED:return "frame consumer rejected delivery";
    default:return "unknown bridge status";
    }
}
