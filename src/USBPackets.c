#include "rtw/USBPackets.h"
#include <string.h>

/* A mutable preflight report may not overlap its borrowed USB transfer. */
static int overlaps(const void* a, size_t an, const void* b, size_t bn) {
    uintptr_t ap, bp;
    if (!a || !b || !an || !bn) return 0;
    ap=(uintptr_t)a; bp=(uintptr_t)b;
    return ap <= bp ? bp-ap < an : ap-bp < bn;
}

static uint32_t load32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) |
           ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
}
static void put32(uint8_t* p, uint32_t word) {
    p[0]=(uint8_t)word; p[1]=(uint8_t)(word>>8U);
    p[2]=(uint8_t)(word>>16U); p[3]=(uint8_t)(word>>24U);
}

void rtw_rx_iterator_init(rtw_rx_iterator* it, const uint8_t* data,
                          size_t length) {
    if (!it) return;
    it->buffer=data;
    it->length=length;
    it->cursor=0;
    it->frames=0;
}

static int zeroes(const uint8_t* p, size_t n) {
    size_t i;
    for(i=0;i<n;++i) if(p[i]!=0) return 0;
    return 1;
}

rtw_packet_error rtw_rx_next(rtw_rx_iterator* it, rtw_rx_packet* packet) {
    uint32_t w0;
    size_t remain, phy_len, shift, mpdu_len, start, consumed, padded;
    const uint8_t* base;
    if (packet) memset(packet,0,sizeof(*packet));
    if (!it || !packet || (!it->buffer && it->length != 0) || it->cursor > it->length)
        return RTW_PACKETS_BAD_ARGUMENT;
    if (it->cursor == it->length) return RTW_PACKETS_DONE;
    base=it->buffer+it->cursor;
    remain=it->length-it->cursor;
    if (remain < RTW_RX_DESC_BYTES) {
        /* max 7 bytes of transfer padding after a full unit are tolerated. */
        if (remain<=7U && zeroes(base,remain)) {
            it->cursor=it->length;
            return RTW_PACKETS_DONE;
        }
        return RTW_PACKETS_TRUNCATED;
    }
    if (it->frames >= RTW_MAX_RX_FRAMES) return RTW_PACKETS_TOO_MANY;
    w0=load32(base);
    mpdu_len=(size_t)(w0 & 0x3FFFU);
    if (!mpdu_len) {
        if (!zeroes(base,remain)) return RTW_PACKETS_MALFORMED;
        it->cursor=it->length;
        return RTW_PACKETS_DONE;
    }
    phy_len=(size_t)((w0>>16U)&0x0FU)*8U;
    shift=(size_t)((w0>>24U)&3U);
    if (phy_len > remain-RTW_RX_DESC_BYTES ||
        shift > remain-RTW_RX_DESC_BYTES-phy_len ||
        mpdu_len > remain-RTW_RX_DESC_BYTES-phy_len-shift)
        return RTW_PACKETS_TRUNCATED;
    start=RTW_RX_DESC_BYTES+phy_len+shift;
    consumed=start+mpdu_len;
    padded=(consumed+7U)&~((size_t)7U);
    packet->mpdu=base+start;
    packet->mpdu_length=mpdu_len;
    packet->phy_info=base+RTW_RX_DESC_BYTES;
    packet->phy_info_length=phy_len;
    packet->crc_error=(uint8_t)((w0>>14U)&1U);
    packet->icv_error=(uint8_t)((w0>>15U)&1U);
    packet->phy_status_present=(uint8_t)((w0>>26U)&1U);
    packet->rate_index=(uint8_t)(load32(base+12U)&0x7FU);
    packet->bandwidth_code=(uint8_t)((load32(base+16U)>>4U)&3U);
    packet->timestamp=load32(base+20U);
    if (padded > remain) {
        /* Last MPDU can finish exactly at USB transfer end without padding. */
        if (consumed != remain) return RTW_PACKETS_TRUNCATED;
        it->cursor=it->length;
    } else {
        /* The hardware may use arbitrary pad content; don't inspect it. */
        it->cursor += padded;
    }
    ++it->frames;
    return RTW_PACKETS_OK;
}

rtw_packet_error rtw_rx_preflight_transfer(const uint8_t* transfer, size_t length,
    rtw_rx_preflight_decoder decode, void* context, rtw_rx_preflight_result* out) {
    rtw_rx_iterator it;
    rtw_packet_error status;
    size_t previous_cursor, previous_frames;
    if (out && overlaps(out, sizeof(*out), transfer, length))
        return RTW_PACKETS_BAD_ARGUMENT;
    if (out) memset(out, 0, sizeof(*out));
    if (!out || !decode || (!transfer && length)) return RTW_PACKETS_BAD_ARGUMENT;
    rtw_rx_iterator_init(&it, transfer, length);
    for (;;) {
        previous_cursor=it.cursor;
        previous_frames=it.frames;
        status=decode(&it, context);
        if (status!=RTW_PACKETS_OK) break;
        /* The callback must never claim progress outside this transfer or
         * produce a packet without advancing one complete frame. */
        if (it.cursor<=previous_cursor || it.cursor>length ||
            it.frames!=previous_frames+1U) {
            status=RTW_PACKETS_MALFORMED;
            break;
        }
        out->valid_bytes=it.cursor;
        out->valid_frames=it.frames;
    }
    if (status!=RTW_PACKETS_DONE) {
        out->error_offset=previous_cursor;
        out->error_frames=previous_frames;
    }
    return status;
}

rtw_packet_error rtw_tx_make_descriptor(
    size_t length, uint8_t rate, uint8_t queue, int multicast,
    uint8_t desc[RTW_TX_DESC_BYTES]) {
    uint32_t word;
    uint16_t checksum=0;
    size_t i;
    if (!desc || length == 0 || length > 0xFFFFU || rate > 0x7FU ||
        queue > 0x1FU || (multicast != 0 && multicast != 1))
        return RTW_PACKETS_BAD_ARGUMENT;
    memset(desc,0,RTW_TX_DESC_BYTES);
    /* OWN | FIRST | LAST | BMC (if group), 40-byte offset, frame length. */
    word=0x8C280000UL | (uint32_t)length | (multicast ? 0x01000000UL : 0U);
    put32(desc,word);
    /* MACID=1; queue and rate-id G=8 (observed vendor values). */
    put32(desc+4U,0x00080001UL | ((uint32_t)queue << 8U));
    /* use explicitly selected data rate */
    put32(desc+12U,0x00000100UL);
    /* retry-limit enabled (6), rate index */
    put32(desc+16U,0x001A0000UL | rate);
    /* Sequence number assigned by hardware */
    put32(desc+32U,0x00008000UL);
    /* XOR the first 16 16-bit words, DW7 checksum field initially 0. */
    for(i=0;i<32U;i+=2U)
        checksum ^= (uint16_t)((unsigned)desc[i] | ((unsigned)desc[i+1U]<<8U));
    desc[28U]=(uint8_t)checksum;
    desc[29U]=(uint8_t)(checksum>>8U);
    return RTW_PACKETS_OK;
}

rtw_packet_error rtw_tx_total_transfer_size_for_descriptor(size_t descriptor_bytes,
    size_t payload_len,uint16_t mps,size_t* total_size) {
    size_t length;
    if(total_size)*total_size=0;
    if(!total_size || (descriptor_bytes!=32U && descriptor_bytes!=40U) ||
       !payload_len || payload_len>0xffffU || (mps!=64U && mps!=512U && mps!=1024U))
        return RTW_PACKETS_BAD_ARGUMENT;
    length=descriptor_bytes+payload_len;
    if(!(length%mps))++length;
    *total_size=length;return RTW_PACKETS_OK;
}
rtw_packet_error rtw_tx_total_transfer_size(size_t payload_len,uint16_t mps,size_t* total_size) {
    return rtw_tx_total_transfer_size_for_descriptor(RTW_TX_DESC_BYTES,payload_len,mps,total_size);
}

const char* rtw_packet_error_string(rtw_packet_error e) {
    switch(e) {
        case RTW_PACKETS_OK:return "ok";
        case RTW_PACKETS_DONE:return "end of transfer";
        case RTW_PACKETS_BAD_ARGUMENT:return "invalid argument";
        case RTW_PACKETS_TRUNCATED:return "truncated RX transfer";
        case RTW_PACKETS_MALFORMED:return "malformed RX transfer";
        case RTW_PACKETS_TOO_MANY:return "too many aggregated RX frames";
        default:return "unknown";
    }
}
