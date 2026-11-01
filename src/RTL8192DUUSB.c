/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2007-2011 Realtek Corporation.
 * Derived from lwfinger/rtl8192du hal/usb_ops_linux.c, immutable commit
 * dc2c6951a9fd28fdb20549140052bbdac3490865, usb_read_reg/usb_write_reg. */
#include "rtw/RTL8192DUUSB.h"
#include <string.h>
rtw_usb_error rtw_92du_usb_address(uint8_t mac,uint16_t logical,uint16_t *physical) {
    uint16_t value=logical;
    if(mac>1U || !physical)return RTW_USB_INVALID_ARGUMENT;
    if(mac) {
        if(value<0x1000U)value|=0x4000U;
        else if((value&0x2000U) && !(value&0x8000U))value&=0x0fffU;
    }
    *physical=value;return RTW_USB_OK;
}
static int request_valid(const rtw_92du_usb_map *m,const rtw_usb_setup *s,const void *bytes) {
    return m && m->mac_index<=1U && m->raw.control_in && m->raw.control_out &&
        s && s->b_request==5U && !s->w_index && s->w_length &&
        s->w_length<=RTW_USB_MAX_VENDOR_PAYLOAD && s->timeout_ms && bytes &&
        (unsigned)s->w_value+s->w_length<=65536U;
}
static rtw_usb_error mapped_in(void *context,const rtw_usb_setup *s,uint8_t *bytes,uint16_t *actual) {
    rtw_92du_usb_map *m=context;rtw_usb_setup next;
    if(!actual)return RTW_USB_INVALID_ARGUMENT;
    *actual=0;
    if(!request_valid(m,s,bytes) || s->bm_request_type!=0xc0U)return RTW_USB_INVALID_ARGUMENT;
    next=*s;(void)rtw_92du_usb_address(m->mac_index,s->w_value,&next.w_value);
    return m->raw.control_in(m->raw.context,&next,bytes,actual);
}
static rtw_usb_error mapped_out(void *context,const rtw_usb_setup *s,const uint8_t *bytes,uint16_t *actual) {
    rtw_92du_usb_map *m=context;rtw_usb_setup next;
    if(!actual)return RTW_USB_INVALID_ARGUMENT;
    *actual=0;
    if(!request_valid(m,s,bytes) || s->bm_request_type!=0x40U)return RTW_USB_INVALID_ARGUMENT;
    next=*s;(void)rtw_92du_usb_address(m->mac_index,s->w_value,&next.w_value);
    return m->raw.control_out(m->raw.context,&next,bytes,actual);
}
rtw_usb_error rtw_92du_usb_map_init(rtw_92du_usb_map *out,const rtw_usb_rw_transport *raw,uint8_t mac) {
    rtw_92du_usb_map next;
    if(!out || !raw || !raw->control_in || !raw->control_out || mac>1U ||
        (raw->context==out && (raw->control_in==mapped_in || raw->control_out==mapped_out)))
        return RTW_USB_INVALID_ARGUMENT;
    memset(&next,0,sizeof(next));next.raw=*raw;next.mac_index=mac;
    next.mapped.context=out;next.mapped.control_in=mapped_in;next.mapped.control_out=mapped_out;
    *out=next;return RTW_USB_OK;
}
const rtw_usb_rw_transport *rtw_92du_usb_bus(const rtw_92du_usb_map *m) {
    return m && m->mac_index<=1U && m->mapped.context==m &&
        m->mapped.control_in==mapped_in && m->mapped.control_out==mapped_out &&
        m->raw.control_in && m->raw.control_out ? &m->mapped:NULL;
}
rtw_92du_result rtw_92du_usb_interface_endpoints(const rtw_backend_usb_plan *primary,
    const rtw_92du_board *board,const uint8_t *cfg,size_t length,rtw_usb_endpoints *out) {
    rtw_usb_endpoints ports[2];size_t pos,end;unsigned seen=0,count,mac,a,b;
    uint8_t addresses[32]={0};
    if(!primary || !board || !cfg || !out)return RTW_92DU_BAD_ARGUMENT;
    if(rtw_92du_validate_usb_plan(primary)!=RTW_92DU_OK || board->profile!=primary->profile ||
       board->mac_index>1U || (board->mode!=RTW_92DU_SINGLE_MAC && board->mode!=RTW_92DU_DUAL_MAC) ||
       (board->mode==RTW_92DU_SINGLE_MAC && (board->mac_index || board->rf_paths!=2U)) ||
       (board->mode==RTW_92DU_DUAL_MAC && board->rf_paths!=1U))return RTW_92DU_IDENTITY_UNVERIFIED;
    count=board->mode==RTW_92DU_DUAL_MAC?2U:1U;mac=board->mac_index;
    if(length<9U || cfg[0]<9U || cfg[1]!=2U || cfg[4]!=count || cfg[5]!=primary->profile->configuration)
        return RTW_92DU_BAD_PROM;
    end=(size_t)cfg[2]|((size_t)cfg[3]<<8U);
    if(end>length || end<cfg[0])return RTW_92DU_BAD_PROM;
    for(pos=cfg[0];pos<end;) {
        unsigned n;if(end-pos<2U || (n=cfg[pos])<2U || n>end-pos)return RTW_92DU_BAD_PROM;
        if(cfg[pos+1U]==4U) {
            unsigned index;
            if(n<9U || (index=cfg[pos+2U])>=count || cfg[pos+3U] || cfg[pos+5U]!=0xffU ||
               cfg[pos+6U]!=0xffU || cfg[pos+7U]!=0xffU || (seen&(1U<<index)))return RTW_92DU_BAD_PROM;
            seen|=1U<<index;
        } else if(cfg[pos+1U]==5U) {
            unsigned address,key;
            if(!seen || n<7U || !(address=cfg[pos+2U]) || !(address&15U) || (address&0x70U))return RTW_92DU_BAD_PROM;
            key=(address&15U)|((address&0x80U)?16U:0U);
            if(addresses[key])return RTW_92DU_BAD_PROM;
            addresses[key]=1;
        }
        pos+=n;
    }
    if(seen!=((1U<<count)-1U))return RTW_92DU_BAD_PROM;
    for(a=0;a<count;++a) {
        if(rtw_find_usb_bulk_endpoints(cfg,end,(uint8_t)a,0,&ports[a])!=RTW_DESC_OK ||
           ports[a].num_bulk_in!=1U || ports[a].num_bulk_out<1U || ports[a].num_bulk_out>3U)
            return RTW_92DU_BAD_PROM;
    }
    if(memcmp(&ports[0],&primary->endpoints,sizeof(ports[0])))return RTW_92DU_IDENTITY_UNVERIFIED;
    if(count==2U) {
        if(ports[0].bulk_in[0]==ports[1].bulk_in[0])return RTW_92DU_BAD_PROM;
        for(a=0;a<ports[0].num_bulk_out;++a)for(b=0;b<ports[1].num_bulk_out;++b)
            if(ports[0].bulk_out[a]==ports[1].bulk_out[b])return RTW_92DU_BAD_PROM;
    }
    *out=ports[mac];return RTW_92DU_OK;
}
