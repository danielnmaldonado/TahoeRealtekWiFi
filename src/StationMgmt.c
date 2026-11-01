/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/StationMgmt.h"
#include <string.h>

static int valid_unicast(const uint8_t* mac) {
    unsigned i,nonzero=0;
    if(!mac || (mac[0]&1U)) return 0;
    for(i=0;i<6U;++i)nonzero|=mac[i];
    return nonzero!=0;
}
static uint16_t le16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8U));
}
static void set16(uint8_t* p,uint16_t x) {
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8U);
}
static void header(const rtw_sta_session* s,uint8_t subtype,
                   uint16_t sequence,uint8_t* p) {
    memset(p,0,24U);
    p[0]=subtype;
    memcpy(p+4U,s->bssid,6U);    /* Addr1: AP / receiver */
    memcpy(p+10U,s->station,6U); /* Addr2: station / transmitter */
    memcpy(p+16U,s->bssid,6U);   /* Addr3: BSSID */
    set16(p+22U,(uint16_t)(sequence<<4U));
}
static rtw_sta_error validate_response(const rtw_sta_session* s,
    const uint8_t* frame,size_t n,uint8_t subtype,size_t minimum) {
    if(!frame || n<minimum) return RTW_STA_MALFORMED_RESPONSE;
    /* Protocol 0, management frame, exact subtype; no protected frames,
       ToDS/FromDS, More Fragments or nonzero fragment number. */
    if(frame[0]!=subtype || (frame[1]&0x47U) || (frame[22]&0x0FU))
        return RTW_STA_MALFORMED_RESPONSE;
    if(memcmp(frame+4U,s->station,6U) ||
       memcmp(frame+10U,s->bssid,6U) ||
       memcmp(frame+16U,s->bssid,6U)) return RTW_STA_WRONG_PEER;
    return RTW_STA_OK;
}
void rtw_sta_reset(rtw_sta_session* s) {
    if(s) memset(s,0,sizeof(*s));
}
rtw_sta_error rtw_sta_configure_open(rtw_sta_session* s,
    const uint8_t station[6],const uint8_t bssid[6],
    const uint8_t* ssid,size_t ssid_len,uint8_t channel,
    int privacy_capability,int has_rsn) {
    rtw_sta_session next;
    size_t i;
    int nonzero=0;
    if(!s) return RTW_STA_BAD_ARGUMENT;
    /* Build a validated temporary copy first so callers may safely supply
     * a station/BSSID/SSID pointer originating in the prior session. */
    memset(&next,0,sizeof(next));
    if(!station || !bssid || !ssid || !ssid_len || ssid_len>32U ||
       !valid_unicast(station) || !valid_unicast(bssid) ||
       !memcmp(station,bssid,6U) ||
       (privacy_capability!=0 && privacy_capability!=1) ||
       (has_rsn!=0 && has_rsn!=1)) {
        rtw_sta_reset(s);
        return RTW_STA_BAD_ARGUMENT;
    }
    for(i=0;i<ssid_len;++i)nonzero|=ssid[i];
    if(!nonzero) {
        rtw_sta_reset(s);
        return RTW_STA_BAD_ARGUMENT;
    }
    if(privacy_capability || has_rsn || channel!=6U) {
        rtw_sta_reset(s);
        return RTW_STA_UNSUPPORTED_AP;
    }
    memcpy(next.station,station,6U);
    memcpy(next.bssid,bssid,6U);
    memcpy(next.ssid,ssid,ssid_len);
    next.ssid_length=(uint8_t)ssid_len;
    next.state=RTW_STA_CONFIGURED;
    *s=next;
    return RTW_STA_OK;
}
rtw_sta_error rtw_sta_build_auth_request(rtw_sta_session* s,
    uint16_t seq,uint8_t* out,size_t cap,size_t* written) {
    if(written)*written=0;
    if(!s || !out || !written || seq>4095U) return RTW_STA_BAD_ARGUMENT;
    if(s->state!=RTW_STA_CONFIGURED)return RTW_STA_BAD_STATE;
    if(cap<30U)return RTW_STA_BUFFER_TOO_SMALL;
    header(s,0xB0U,seq,out);
    /* Open System algorithm=0, transaction=1, status=0 */
    set16(out+24U,0U);set16(out+26U,1U);set16(out+28U,0U);
    s->status=0;
    s->state=RTW_STA_AUTH_PENDING;
    *written=30U;
    return RTW_STA_OK;
}
rtw_sta_error rtw_sta_accept_auth_response(rtw_sta_session* s,
    const uint8_t* frame,size_t n) {
    rtw_sta_error e;
    if(!s)return RTW_STA_BAD_ARGUMENT;
    if(s->state!=RTW_STA_AUTH_PENDING)return RTW_STA_BAD_STATE;
    e=validate_response(s,frame,n,0xB0U,30U);
    if(e!=RTW_STA_OK)return e;
    if(le16(frame+24U)!=0U || le16(frame+26U)!=2U)
        return RTW_STA_MALFORMED_RESPONSE;
    s->status=le16(frame+28U);
    if(s->status){s->state=RTW_STA_REJECTED;return RTW_STA_AP_REJECTED;}
    s->state=RTW_STA_AUTHENTICATED;
    return RTW_STA_OK;
}
rtw_sta_error rtw_sta_build_assoc_request(rtw_sta_session* s,
    uint16_t seq,uint16_t listen,const uint8_t* rates,size_t rate_count,
    uint8_t* out,size_t cap,size_t* written) {
    size_t i,need;
    if(written)*written=0;
    if(!s || !out || !written || !rates || !rate_count ||
       rate_count>RTW_STA_RATES_MAX || seq>4095U ||
       listen==0U || listen>1000U)return RTW_STA_BAD_ARGUMENT;
    if(s->state!=RTW_STA_AUTHENTICATED)return RTW_STA_BAD_STATE;
    /* Limit to known 802.11b/g rate units (500 kbps), not MCS indices. */
    for(i=0;i<rate_count;++i) {
        const uint8_t r=(uint8_t)(rates[i]&0x7FU);
        static const uint8_t allowed[]={2,4,11,22,12,18,24,36,48,72,96,108};
        size_t j;
        for(j=0;j<sizeof(allowed);++j) if(r==allowed[j])break;
        if(j==sizeof(allowed))return RTW_STA_BAD_ARGUMENT;
    }
    need=24U+4U+2U+s->ssid_length+2U+rate_count;
    if(cap<need)return RTW_STA_BUFFER_TOO_SMALL;
    header(s,0x00U,seq,out);
    set16(out+24U,0x0001U); /* ESS, no privacy bit */
    set16(out+26U,listen);
    out[28U]=0U;out[29U]=s->ssid_length;
    memcpy(out+30U,s->ssid,s->ssid_length);
    i=30U+s->ssid_length;
    out[i++]=1U;out[i++]=(uint8_t)rate_count;
    memcpy(out+i,rates,rate_count);
    s->state=RTW_STA_ASSOC_PENDING;
    *written=need;
    return RTW_STA_OK;
}
rtw_sta_error rtw_sta_accept_assoc_response(rtw_sta_session* s,
    const uint8_t* frame,size_t n) {
    rtw_sta_error e;
    uint16_t aid;
    if(!s)return RTW_STA_BAD_ARGUMENT;
    if(s->state!=RTW_STA_ASSOC_PENDING)return RTW_STA_BAD_STATE;
    e=validate_response(s,frame,n,0x10U,30U);
    if(e!=RTW_STA_OK)return e;
    s->status=le16(frame+26U);
    if(s->status){s->state=RTW_STA_REJECTED;return RTW_STA_AP_REJECTED;}
    aid=le16(frame+28U);
    if((aid&0xC000U)!=0xC000U || !(aid&0x3FFFU) ||
       (aid&0x3FFFU)>2007U)return RTW_STA_MALFORMED_RESPONSE;
    /* A successful association response must contain the Supported Rates IE.
     * Parse the entire IE list to reject truncated, duplicated and invalid
     * advertisements. The caller must still validate RF capabilities. */
    {
        size_t cursor=30U;
        unsigned supported=0;
        while(cursor<n) {
            uint8_t id,ie_len;
            if(n-cursor<2U)return RTW_STA_MALFORMED_RESPONSE;
            id=frame[cursor++];ie_len=frame[cursor++];
            if((size_t)ie_len>n-cursor)return RTW_STA_MALFORMED_RESPONSE;
            if(id==1U) {
                if(supported || !ie_len || ie_len>RTW_STA_RATES_MAX)
                    return RTW_STA_MALFORMED_RESPONSE;
                supported=1;
            }
            cursor+=(size_t)ie_len;
        }
        if(!supported)return RTW_STA_MALFORMED_RESPONSE;
    }
    s->aid=(uint16_t)(aid&0x3FFFU);
    s->state=RTW_STA_ASSOCIATED;
    return RTW_STA_OK;
}
rtw_sta_error rtw_sta_accept_disconnect(rtw_sta_session* s,
    const uint8_t* frame,size_t n) {
    rtw_sta_error e;
    if(!s)return RTW_STA_BAD_ARGUMENT;
    if(s->state!=RTW_STA_AUTHENTICATED &&
       s->state!=RTW_STA_ASSOC_PENDING &&
       s->state!=RTW_STA_ASSOCIATED) return RTW_STA_BAD_STATE;
    if(!frame || n<26U) return RTW_STA_MALFORMED_RESPONSE;
    if(frame[0]!=0xC0U && frame[0]!=0xA0U)
        return RTW_STA_MALFORMED_RESPONSE;
    e=validate_response(s,frame,n,frame[0],26U);
    if(e!=RTW_STA_OK)return e;
    if(le16(frame+24U)==0U)return RTW_STA_MALFORMED_RESPONSE;
    s->status=le16(frame+24U);
    s->aid=0;
    s->state=RTW_STA_DISCONNECTED;
    return RTW_STA_OK;
}
const char* rtw_sta_error_string(rtw_sta_error e) {
    switch(e) {
        case RTW_STA_OK:return "ok";
        case RTW_STA_BAD_ARGUMENT:return "invalid management input";
        case RTW_STA_BAD_STATE:return "invalid station transition";
        case RTW_STA_UNSUPPORTED_AP:return "only open channel-6 APs supported";
        case RTW_STA_WRONG_PEER:return "unexpected source/BSSID/destination";
        case RTW_STA_MALFORMED_RESPONSE:return "invalid management response";
        case RTW_STA_AP_REJECTED:return "AP rejected management exchange";
        case RTW_STA_BUFFER_TOO_SMALL:return "insufficient output capacity";
    }
    return "unknown management error";
}
