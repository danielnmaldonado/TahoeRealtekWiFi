/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2009-2012, 2024 Realtek Corporation.
 * Adapted from pinned Linux rtlwifi/efuse.c and rtl8192d/hw_common.c.
 * See RTL8192DUBoot-Upstream.json. No OTP programming operation exists here. */
#include "rtw/RTL8192DUEfuse.h"
#include <string.h>

static int overlap(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x,y;
    if (!a || !b || !an || !bn) return 0;
    x=(uintptr_t)a;y=(uintptr_t)b;
    return x<=y ? y-x<an:x-y<bn;
}
rtw_92du_result rtw_92du_unpack_rom(const uint8_t *p,size_t n,
    uint8_t *out,size_t cap) {
    size_t pos=0;
    if (!p || !out || !n || n>RTW_92DU_PHYSICAL_BYTES ||
        cap<RTW_92DU_LOGICAL_BYTES ||
        overlap(p,n,out,RTW_92DU_LOGICAL_BYTES)) return RTW_92DU_BAD_ARGUMENT;
    memset(out,0xff,RTW_92DU_LOGICAL_BYTES);
    while (pos<n) {
        const uint8_t header=p[pos++];
        unsigned block,mask,word;
        if (header==0xffU) return RTW_92DU_OK;
        if ((header&31U)==15U) {
            uint8_t ext;
            if (pos==n) goto bad;
            ext=p[pos++];mask=ext&15U;
            if (mask==15U) continue;
            block=((unsigned)(ext&0xf0U)>>1U)|((unsigned)header>>5U);
        } else {block=header>>4U;mask=header&15U;}
        if (block>=32U) goto bad;
        for (word=0;word<4U;++word) {
            const size_t offset=(size_t)block*8U+word*2U;
            if (mask&(1U<<word)) continue;
            if (n-pos<2U) goto bad;
            out[offset]=p[pos++];out[offset+1U]=p[pos++];
        }
    }
    return RTW_92DU_OK;
bad:
    memset(out,0xff,RTW_92DU_LOGICAL_BYTES);return RTW_92DU_BAD_PROM;
}
typedef struct {
    const rtw_usb_rw_transport *bus;
    const rtw_waiter *waiter;
    uint16_t max_polls;
    rtw_92du_result error;
} du_reader;
static int readreg(du_reader *r,uint16_t reg,uint8_t width,uint32_t *v) {
    if (rtw_usb_rw_read(r->bus,reg,width,v)==RTW_USB_OK) return 1;
    r->error=RTW_92DU_IO_ERROR;return 0;
}
static int writereg(du_reader *r,uint16_t reg,uint8_t width,uint32_t v) {
    if (rtw_usb_write_register(r->bus,reg,width,v)==RTW_USB_OK) return 1;
    r->error=RTW_92DU_IO_ERROR;return 0;
}
static int waitfor(du_reader *r,uint32_t us) {
    if (r->waiter->wait_us(r->waiter->context,us)) return 1;
    r->error=RTW_92DU_WAIT_ERROR;return 0;
}
static int otpbyte(du_reader *r,uint16_t addr,uint8_t *out) {
    uint32_t v=0;unsigned i;
    if (!writereg(r,0x31U,1U,addr&255U) || !readreg(r,0x32U,1U,&v) ||
        !writereg(r,0x32U,1U,(v&0xfcU)|((unsigned)addr>>8U)) ||
        !readreg(r,0x33U,1U,&v) || !writereg(r,0x33U,1U,v&0x7fU)) return 0;
    for (i=0;i<r->max_polls;++i) {
        if (!readreg(r,0x30U,4U,&v)) return 0;
        if (v&0x80000000U) {
            /* The common DU PROM reader always settles and rereads data. */
            if (!waitfor(r,50U) || !readreg(r,0x30U,4U,&v)) return 0;
            if (!(v&0x80000000U)) {r->error=RTW_92DU_POLL_TIMEOUT;return 0;}
            *out=(uint8_t)v;return 1;
        }
        if (i+1U<r->max_polls && !waitfor(r,1U)) return 0;
    }
    r->error=RTW_92DU_POLL_TIMEOUT;return 0;
}
rtw_92du_result rtw_92du_read_rom(const rtw_backend_usb_plan *usb,
    const rtw_usb_rw_transport *bus,const rtw_waiter *waiter,
    const rtw_92du_boot_options *options,uint8_t index,rtw_92du_rom *out) {
    rtw_92du_rom next;
    du_reader r;
    uint32_t syscfg,v,mac0,mac1;
    uint8_t cut_hi,cut_lo;
    size_t i;
    int gate=0;
    if (!out) return RTW_92DU_BAD_ARGUMENT;
    if (overlap(out,sizeof(*out),usb,sizeof(*usb)) ||
        overlap(out,sizeof(*out),bus,sizeof(*bus)) ||
        overlap(out,sizeof(*out),waiter,sizeof(*waiter)) ||
        overlap(out,sizeof(*out),options,sizeof(*options))) return RTW_92DU_BAD_ARGUMENT;
    memset(out,0,sizeof(*out));
    if (!bus || !bus->control_in || !bus->control_out ||
        !waiter || !waiter->wait_us || !options ||
        !options->max_polls || options->max_polls>10U || index>1U)
        return RTW_92DU_BAD_ARGUMENT;
    if (options->confirm_experimental_io!=1U || options->exclusive_whole_device!=1U)
        return RTW_92DU_IO_NOT_CONFIRMED;
    if (rtw_92du_validate_usb_plan(usb)!=RTW_92DU_OK)
        return RTW_92DU_IDENTITY_UNVERIFIED;
    memset(&next,0,sizeof(next));
    r.bus=bus;r.waiter=waiter;r.max_polls=options->max_polls;r.error=RTW_92DU_OK;
    if (!readreg(&r,0xf0U,4U,&syscfg) || !readreg(&r,0x0aU,1U,&v) ||
        !readreg(&r,0x81U,1U,&mac0) || !readreg(&r,0x53U,1U,&mac1)) goto cleanup;
    if (syscfg==0xffffffffU || !(syscfg&0x000f0000U))
        return RTW_92DU_UNSUPPORTED_CHIP;
    if (!(v&0x20U)) return RTW_92DU_BAD_PROM;
    if (v&0x10U) return RTW_92DU_UNSUPPORTED_CHIP; /* serial EEPROM not ported */
    if ((mac0&0x80U) || (mac1&1U)) return RTW_92DU_MCU_BUSY;
    gate=1; /* an ambiguous completion may have opened access */
    if (!writereg(&r,0xcfU,1U,0x69U) || !readreg(&r,2U,2U,&v) ||
        (!(v&0x1000U) && !writereg(&r,2U,2U,v|0x1000U))) goto cleanup;
    /* DU maps both EFUSE_LOADER_CLK_EN and EFUSE_ANA8M to zero: there is
     * no AU loader-clock mask or read-programming voltage to enable. */
    for (i=0;i<RTW_92DU_PHYSICAL_BYTES;++i)
        if (!otpbyte(&r,(uint16_t)i,next.physical+i)) goto cleanup;
    if (!otpbyte(&r,0x3faU,&next.pa_bias) ||
        !otpbyte(&r,0x3feU,&cut_hi) || !otpbyte(&r,0x3ffU,&cut_lo)) goto cleanup;
    next.cut_word=(uint16_t)(((unsigned)cut_hi<<8U)|cut_lo);
    switch (next.cut_word) {
    case 0xaa55U: next.cut=RTW_92DU_CUT_C;break;
    case 0x9966U: next.cut=RTW_92DU_CUT_D;break;
    case 0xcc33U: case 0x33ccU: next.cut=RTW_92DU_CUT_E;break;
    default: r.error=RTW_92DU_UNSUPPORTED_CHIP;goto cleanup;
    }
    r.error=rtw_92du_unpack_rom(next.physical,sizeof(next.physical),
        next.logical,sizeof(next.logical));
    if (r.error!=RTW_92DU_OK) goto cleanup;
    r.error=rtw_92du_identify(usb,syscfg,next.logical,sizeof(next.logical),index,&next.board);
    if (r.error!=RTW_92DU_OK) goto cleanup;
    next.crystal=next.logical[0xbdU];
    next.thermal=next.logical[0xc3U]&31U;
    next.thermal_valid=(uint8_t)(next.thermal>=6U && next.thermal<=0x1cU);
    next.regulatory=next.logical[0xc4U]&7U;
    if (next.cut==RTW_92DU_CUT_C) next.pa_valid=1U;
    else {
        next.pa_valid=(uint8_t)(next.logical[0xbeU]!=0xffU && next.logical[0xbfU]!=0xffU);
        next.internal_pa_5g[0]=(uint8_t)(!(next.logical[0xbeU]&0x40U));
        next.internal_pa_5g[1]=(uint8_t)(!(next.logical[0xbfU]&0x40U));
    }
cleanup:
    if (gate && !writereg(&r,0xcfU,1U,0U)) {
        (void)rtw_usb_write_register(bus,0xcfU,1U,0U);
        r.error=RTW_92DU_IO_ERROR;
    }
    if (r.error==RTW_92DU_OK) *out=next;
    return r.error;
}
