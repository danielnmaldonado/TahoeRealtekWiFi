/* SPDX-License-Identifier: BSD-3-Clause
 * Original allocation-free encoder of the rtw88 hardware descriptor format.
 * Field definitions: Linux tx.h/tx.c; checksum spans: rtw8821c.c, rtw8822b.c,
 * rtw8822c.c at 0e135910deec43ffe97eca0b6af1f8f847285b59.
 * See RTW88USBDataPath-Upstream.json for immutable references and hashes. */
#include "rtw/RTW88USBDataPath.h"
#include <string.h>

static void put32(uint8_t *p,uint32_t v) {
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8U);
    p[2]=(uint8_t)(v>>16U);p[3]=(uint8_t)(v>>24U);
}
static int overlap(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=y ? y-x<an : x-y<bn;
}

rtw88_usb_tx_result rtw88_usb_make_tx_descriptor(rtw_wifi_family family,
    size_t length,const rtw88_usb_tx_options *o,uint8_t *out,size_t out_length) {
    uint8_t d[RTW88_USB_TX_DESC_BYTES];
    uint32_t w[12]={0};
    uint16_t checksum=0;
    unsigned chains,checksum_length,i,ht,vht;
    uint8_t flags[12];
    if(!o||!out||out_length<sizeof(d)||!length||length>11454U||
       overlap(out,sizeof(d),o,sizeof(*o)))return RTW88_USB_TX_INVALID_ARGUMENT;
    switch(family){
    case RTW_WIFI_RTL8821CU:chains=1;checksum_length=32;break;
    case RTW_WIFI_RTL8822BU:chains=2;checksum_length=32;break;
    case RTW_WIFI_RTL8822CU:chains=2;checksum_length=48;break;
    default:return RTW88_USB_TX_UNSUPPORTED_FAMILY;
    }
    flags[0]=o->band_5ghz;flags[1]=o->multicast;flags[2]=o->firmware_rate;
    flags[3]=o->short_gi;flags[4]=o->ldpc;flags[5]=o->stbc;
    flags[6]=o->ampdu;flags[7]=o->request_report;flags[8]=o->disable_fallback;
    flags[9]=o->rts;flags[10]=o->hardware_sequence;flags[11]=o->disable_queue_sequence;
    for(i=0;i<sizeof(flags);i++)if(flags[i]>1U)return RTW88_USB_TX_INVALID_ARGUMENT;
    if(o->sequence>4095U||o->report_tag>4095U||o->queue>19U||
       o->rate_id>31U||o->bandwidth>2U||o->ampdu_density>7U||
       o->ampdu_factor>31U)return RTW88_USB_TX_INVALID_ARGUMENT;
    ht=o->rate>=12U&&o->rate<12U+chains*8U;
    vht=o->rate>=44U&&o->rate<44U+chains*10U;
    if(!(o->rate<=11U||ht||vht)||
       (o->band_5ghz&&o->rate<4U)||(vht&&!o->band_5ghz)||
       (o->stbc&&chains<2U)||
       (o->bandwidth&&!(ht||vht))||
       (o->bandwidth==2U&&(!vht||!o->band_5ghz))||
       (o->short_gi&&((o->rate>=4U&&o->rate<=11U)||o->rate==0U))||
       ((o->ampdu||o->ldpc||o->stbc)&&!(ht||vht)))
        return RTW88_USB_TX_UNSUPPORTED_RATE;
    w[0]=(uint32_t)length|((uint32_t)sizeof(d)<<16U)|
        ((uint32_t)o->multicast<<24U)|(1U<<26U)|
        ((uint32_t)o->disable_queue_sequence<<31U);
    w[1]=(uint32_t)o->mac_id|((uint32_t)o->queue<<8U)|
        ((uint32_t)o->rate_id<<16U)|(o->queue==17U ? 1U<<29U : 0U);
    w[2]=((uint32_t)o->ampdu<<12U)|((uint32_t)o->request_report<<19U)|
        ((uint32_t)o->ampdu_density<<20U);
    w[3]=((uint32_t)!o->firmware_rate<<8U)|
        ((uint32_t)o->disable_fallback<<10U)|((uint32_t)o->rts<<12U)|
        ((uint32_t)o->ampdu_factor<<17U);
    w[4]=o->rate;
    w[5]=((uint32_t)o->short_gi<<4U)|((uint32_t)o->bandwidth<<5U)|
        ((uint32_t)o->ldpc<<7U)|((uint32_t)o->stbc<<8U);
    w[6]=o->report_tag;
    w[8]=(uint32_t)o->hardware_sequence<<15U;
    w[9]=(uint32_t)o->sequence<<12U;
    if(o->rts){w[4]|=8U<<24U;w[5]|=1U<<12U;}
    for(i=0;i<12U;i++)put32(d+i*4U,w[i]);
    for(i=0;i<checksum_length;i+=2U)
        checksum^=(uint16_t)((uint16_t)d[i]|((uint16_t)d[i+1U]<<8U));
    d[28]=(uint8_t)checksum;d[29]=(uint8_t)(checksum>>8U);
    memcpy(out,d,sizeof(d));return RTW88_USB_TX_OK;
}

rtw88_usb_tx_result rtw88_usb_make_control_descriptor(rtw_wifi_family family,
    size_t length,rtw88_usb_control_kind kind,uint8_t*out,size_t out_length) {
    uint8_t d[RTW88_USB_TX_DESC_BYTES]={0};unsigned checksum_length,i;uint16_t sum=0;
    if(!out||out_length<sizeof(d)||!length||length>4097U||
       (kind!=RTW88_USB_CONTROL_RESERVED&&kind!=RTW88_USB_CONTROL_H2C)||
       (kind==RTW88_USB_CONTROL_H2C&&length!=32U))return RTW88_USB_TX_INVALID_ARGUMENT;
    if(family==RTW_WIFI_RTL8822CU)checksum_length=48;
    else if(family==RTW_WIFI_RTL8821CU||family==RTW_WIFI_RTL8822BU)checksum_length=32;
    else return RTW88_USB_TX_UNSUPPORTED_FAMILY;
    put32(d,(uint32_t)length|(kind==RTW88_USB_CONTROL_RESERVED?0x04300000U:0U));
    put32(d+4,(uint32_t)(kind==RTW88_USB_CONTROL_RESERVED?16U:19U)<<8U);
    for(i=0;i<checksum_length;i+=2U)sum^=(uint16_t)((uint16_t)d[i]|((uint16_t)d[i+1U]<<8U));
    d[28]=(uint8_t)sum;d[29]=(uint8_t)(sum>>8U);memcpy(out,d,sizeof(d));return RTW88_USB_TX_OK;
}
