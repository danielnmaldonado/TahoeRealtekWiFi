/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2009-2012, 2024 Realtek Corporation.
 * Adapted from Linux rtlwifi rtl8192du/{hw,phy,fw}.c,
 * rtl8192d/{hw,phy,fw}_common.c and efuse.c at
 * 0e135910deec43ffe97eca0b6af1f8f847285b59. See docs/RTL8192DU-BOOT.md.
 * Cold boot only; no RF writes and no automatic hardware attachment. */
#include "rtw/RTL8192DUBoot.h"
#include "rtw/USBWiFiFirmware.h"
#include <string.h>

enum {
    SYS_ISO=0x0000, SYS_FUNC_HI=0x0003, APS_FSMCO=0x0004,
    SPS0_CTRL=0x0011, POWER_OFF=0x0017, FW_MAC0_READY=0x0018,
    FW_MAC1_READY=0x001a, RSV_CTRL=0x001c, LDOV12D_CTRL=0x0021,
    MAC1=0x0053, MCUFWDL=0x0080, MAC0=0x0081, SYS_CFG=0x00f0,
    MAC_PHY_CTRL=0x00f8, CR=0x0100, RX_BOUNDARY=0x0116,
    TXPAUSE=0x0522, DMC=0x05f0, RX_PKT_LIMIT=0x0609,
    FW_WINDOW=0x1000, PAGE_SIZE=4096, MAX_PAYLOAD=32768,
    FW_DOWNLOAD=1, FW_READY=2, FW_CHECKSUM=4, FW_RAM=0x80
};

static int mac_valid(const uint8_t *mac) {
    unsigned i, any=0;
    if (mac[0]&1U) return 0;
    for (i=0;i<6U;++i) any|=mac[i];
    return any!=0U;
}
static int profile_valid(const rtw_usb_wifi_profile *profile) {
    size_t i;
    /* Compare before dereferencing: callers may not fabricate catalog entries. */
    for (i=0;i<rtw_usb_wifi_profile_count();++i) {
        if (profile==rtw_usb_wifi_profile_at(i))
            return profile->family==RTW_WIFI_RTL8192DU &&
                profile->primary_verified && profile->vendor && profile->product;
    }
    return 0;
}
static int sys_cfg_valid(uint32_t value) {
    /* DU's normal/test chip discriminator differs from EU's SYS_CFG bit23. */
    return value!=0xffffffffU && (value&0x000f0000U)!=0;
}
static int board_valid(const rtw_92du_board *board) {
    if (!board || !profile_valid(board->profile) ||
        !sys_cfg_valid(board->sys_cfg) || !mac_valid(board->mac) ||
        board->mac_index>1U) return 0;
    if (board->mode==RTW_92DU_SINGLE_MAC)
        return board->mac_index==0U && board->rf_paths==2U &&
            board->initial_band==RTW_WIFI_BAND_2GHZ;
    if (board->mode==RTW_92DU_DUAL_MAC)
        return board->rf_paths==1U && board->initial_band==
            (board->mac_index ? RTW_WIFI_BAND_2GHZ:RTW_WIFI_BAND_5GHZ);
    return 0;
}
static int endpoints_valid(const rtw_backend_usb_plan *usb) {
    const rtw_usb_endpoints *e=&usb->endpoints;
    uint32_t seen=0;
    unsigned i;
    if (e->interface_number!=usb->profile->interface_number ||
        e->alternate_setting || !e->num_bulk_in || !e->num_bulk_out ||
        e->num_bulk_in>RTW_MAX_BULK_PIPES ||
        e->num_bulk_out>RTW_MAX_BULK_PIPES ||
        e->num_endpoints<e->num_bulk_in+e->num_bulk_out) return 0;
    for (i=0;i<(unsigned)e->num_bulk_in+e->num_bulk_out;++i) {
        const int input=i<e->num_bulk_in;
        const uint8_t ep=input ? e->bulk_in[i]:e->bulk_out[i-e->num_bulk_in];
        const unsigned slot=(ep&15U)+(input ? 16U:0U);
        if (!(ep&15U) || (ep&0x70U) || !!(ep&0x80U)!=input ||
            (seen&(1UL<<slot))) return 0;
        seen|=1UL<<slot;
    }
    return 1;
}
rtw_92du_result rtw_92du_validate_usb_plan(const rtw_backend_usb_plan *usb) {
    if (!usb) return RTW_92DU_BAD_ARGUMENT;
    const rtw_family_backend *backend=rtw_backend_for_family(RTW_WIFI_RTL8192DU);
    const uint8_t station=(uint8_t)(backend && (backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION)!=0U);
    if (!profile_valid(usb->profile) || usb->backend!=backend ||
        usb->station_backend_integrated!=station || usb->usb_preflight_only!=(uint8_t)!station ||
        !endpoints_valid(usb)) return RTW_92DU_IDENTITY_UNVERIFIED;
    return RTW_92DU_OK;
}
rtw_92du_result rtw_92du_identify(const rtw_backend_usb_plan *usb,
    uint32_t sys_cfg, const uint8_t *prom, size_t length,
    uint8_t mac_index, rtw_92du_board *out) {
    rtw_92du_board next;
    size_t mac_offset;
    if (!usb || !prom || !out || mac_index>1U) return RTW_92DU_BAD_ARGUMENT;
    if (rtw_92du_validate_usb_plan(usb)!=RTW_92DU_OK)
        return RTW_92DU_IDENTITY_UNVERIFIED;
    if (!sys_cfg_valid(sys_cfg)) return RTW_92DU_UNSUPPORTED_CHIP;
    /* Require the logical image through BOTH MACs, including in single mode.
     * An erased mode byte is not a usable manufacturing configuration. */
    if (length<0x61U || prom[0]!=0x29U || prom[1]!=0x81U ||
        prom[0x10U]==0xffU) return RTW_92DU_BAD_PROM;
    memset(&next,0,sizeof(next));
    next.profile=usb->profile; next.sys_cfg=sys_cfg; next.mac_index=mac_index;
    next.mode=(prom[0x10U]&1U) ? RTW_92DU_DUAL_MAC:RTW_92DU_SINGLE_MAC;
    if (next.mode==RTW_92DU_SINGLE_MAC && mac_index) return RTW_92DU_BAD_PROM;
    next.rf_paths=next.mode==RTW_92DU_SINGLE_MAC ? 2U:1U;
    next.initial_band=next.mode==RTW_92DU_DUAL_MAC && !mac_index ?
        RTW_WIFI_BAND_5GHZ:RTW_WIFI_BAND_2GHZ;
    mac_offset=mac_index ? 0x5bU:0x19U;
    memcpy(next.mac,prom+mac_offset,6U);
    if (!mac_valid(next.mac)) return RTW_92DU_BAD_PROM;
    *out=next;
    return RTW_92DU_OK;
}

rtw_92du_result rtw_92du_boot_init(rtw_92du_boot *out,
    const rtw_92du_board *board, const rtw_usb_rw_transport *bus,
    const rtw_waiter *waiter, const rtw_92du_boot_options *options) {
    rtw_92du_boot next;
    if (!out || !board || !bus || !bus->control_in || !bus->control_out ||
        !waiter || !waiter->wait_us || !options ||
        !options->max_polls || options->max_polls>1000U)
        return RTW_92DU_BAD_ARGUMENT;
    if (!board_valid(board)) return RTW_92DU_IDENTITY_UNVERIFIED;
    if (options->confirm_experimental_io!=1U ||
        options->exclusive_whole_device!=1U) return RTW_92DU_IO_NOT_CONFIRMED;
    memset(&next,0,sizeof(next));
    next.board=*board; next.bus=*bus; next.waiter=*waiter;
    next.max_polls=options->max_polls;
    *out=next;
    return RTW_92DU_OK;
}
static int context_valid(const rtw_92du_boot *c) {
    return c && board_valid(&c->board) && c->bus.control_in &&
        c->bus.control_out && c->waiter.wait_us &&
        c->max_polls && c->max_polls<=1000U;
}
static int read_reg(rtw_92du_boot *c,uint16_t reg,uint8_t width,uint32_t *out) {
    c->last_register=reg;
    if (rtw_usb_rw_read(&c->bus,reg,width,out)==RTW_USB_OK) return 1;
    c->last_error=RTW_92DU_IO_ERROR; return 0;
}
static int write_reg(rtw_92du_boot *c,uint16_t reg,uint8_t width,uint32_t value) {
    c->last_register=reg;
    if (rtw_usb_write_register(&c->bus,reg,width,value)==RTW_USB_OK) return 1;
    c->last_error=RTW_92DU_IO_ERROR; return 0;
}
static int update(rtw_92du_boot *c,uint16_t reg,uint8_t width,
                  uint32_t clear,uint32_t set) {
    uint32_t value;
    return read_reg(c,reg,width,&value) &&
        write_reg(c,reg,width,(value&~clear)|set);
}
static int wait_us(rtw_92du_boot *c,uint32_t us) {
    if (c->waiter.wait_us(c->waiter.context,us)) return 1;
    c->last_error=RTW_92DU_WAIT_ERROR; return 0;
}
static int poll(rtw_92du_boot *c,uint16_t reg,uint8_t width,
                uint32_t mask,uint32_t expect,uint32_t delay) {
    unsigned i; uint32_t value;
    for (i=0;i<c->max_polls;++i) {
        if (!read_reg(c,reg,width,&value)) return 0;
        if ((value&mask)==expect) return 1;
        if (i+1U<c->max_polls && !wait_us(c,delay)) return 0;
    }
    c->last_error=RTW_92DU_POLL_TIMEOUT; return 0;
}
static rtw_92du_result abort_boot(rtw_92du_boot *c,int pause) {
    uint32_t value;
    /* Best effort, preserve the original failed register and result. Never
     * reset a CPU, zero MCUFWDL+1 (MAC0_ON), unpause or retry firmware data. */
    if (c->download_active &&
        rtw_usb_rw_read(&c->bus,MCUFWDL,1U,&value)==RTW_USB_OK)
        (void)rtw_usb_write_register(&c->bus,MCUFWDL,1U,value&~(uint32_t)FW_DOWNLOAD);
    c->download_active=0;
    if (pause) (void)rtw_usb_write_register(&c->bus,TXPAUSE,1U,0xffU);
    c->power_ready=0; c->firmware_ready=0; c->firmware_reused=0;
    return c->last_error;
}
rtw_92du_result rtw_92du_power_on(rtw_92du_boot *c) {
    uint32_t cfg,mac0,mac1;
    uint16_t mac_reg;
    if (!context_valid(c)) return RTW_92DU_BAD_ARGUMENT;
    if (c->last_error!=RTW_92DU_OK) return c->last_error;
    if (c->power_ready || c->firmware_ready) return RTW_92DU_WRONG_STATE;
    if (!read_reg(c,SYS_CFG,4U,&cfg) || !read_reg(c,MAC0,1U,&mac0) ||
        !read_reg(c,MAC1,1U,&mac1)) return abort_boot(c,0);
    if (!sys_cfg_valid(cfg) || cfg!=c->board.sys_cfg) {
        c->last_error=RTW_92DU_UNSUPPORTED_CHIP; return c->last_error;
    }
    /* Avoid clearing global reset/mode bits beneath an already active MAC. */
    if ((mac0&0x80U) || (mac1&1U)) {
        c->last_error=RTW_92DU_MCU_BUSY; return c->last_error;
    }
    mac_reg=c->board.mac_index ? MAC1:MAC0;
    if (!write_reg(c,TXPAUSE,1U,0xffU) ||
        !write_reg(c,RSV_CTRL,1U,0U) ||
        !update(c,SYS_FUNC_HI,1U,0x80U,0U) ||
        !update(c,MAC_PHY_CTRL,1U,7U,
                c->board.mode==RTW_92DU_SINGLE_MAC ? 4U:3U) ||
        !update(c,mac_reg,1U,2U,
                c->board.initial_band==RTW_WIFI_BAND_2GHZ ? 2U:0U) ||
        !update(c,mac_reg,1U,0U,c->board.mac_index ? 1U:0x80U))
        return abort_boot(c,1);
    if (c->board.mode==RTW_92DU_DUAL_MAC &&
        !poll(c,POWER_OFF,1U,0x80U,0U,500U)) return abort_boot(c,1);
    if (!poll(c,APS_FSMCO,1U,2U,2U,1U) ||
        !write_reg(c,RSV_CTRL,1U,0U) ||
        !write_reg(c,SPS0_CTRL,1U,0x2bU) || !wait_us(c,1000U))
        return abort_boot(c,1);
    if (!read_reg(c,LDOV12D_CTRL,1U,&cfg)) return abort_boot(c,1);
    if (!(cfg&1U) && (!write_reg(c,LDOV12D_CTRL,1U,cfg|1U) ||
        !wait_us(c,1000U) || !update(c,SYS_ISO,1U,1U,0U)))
        return abort_boot(c,1);
    if (!update(c,APS_FSMCO,2U,0U,0x100U) ||
        !poll(c,APS_FSMCO,2U,0x100U,0U,1U) ||
        !update(c,SYS_ISO,2U,0x200U,0U) ||
        !update(c,CR,2U,0U,0x2ffU) ||
        /* The pre-power pause may not survive the MAC power/reset edge. */
        !write_reg(c,TXPAUSE,1U,0xffU) ||
        !write_reg(c,DMC,1U,0U) ||
        !write_reg(c,RX_PKT_LIMIT,1U,c->board.mode==RTW_92DU_SINGLE_MAC ? 0x10U:8U) ||
        !write_reg(c,RX_BOUNDARY,2U,c->board.mode==RTW_92DU_SINGLE_MAC ? 0x27ffU:0x13ffU))
        return abort_boot(c,1);
    c->power_ready=1U;
    return RTW_92DU_OK;
}

rtw_92du_result rtw_92du_firmware_start(rtw_92du_boot *c,
    const uint8_t *firmware,size_t length) {
    rtw_8192eu_fw_info image;
    uint32_t value;
    size_t offset=0;
    if (!context_valid(c)) return RTW_92DU_BAD_ARGUMENT;
    if (c->last_error!=RTW_92DU_OK) return c->last_error;
    if (!c->power_ready || c->firmware_ready) return RTW_92DU_WRONG_STATE;
    if (!rtw_usb_wifi_inspect_firmware(RTW_WIFI_RTL8192DU,firmware,length,&image) ||
        image.payload_length>MAX_PAYLOAD) return RTW_92DU_BAD_FIRMWARE;
    if (!read_reg(c,MCUFWDL,1U,&value)) return abort_boot(c,0);
    if ((value&FW_READY) && !(value&FW_DOWNLOAD)) {
        if (!poll(c,c->board.mac_index ? FW_MAC1_READY:FW_MAC0_READY,
                  1U,1U,1U,5U)) return abort_boot(c,1);
        c->firmware_ready=1U; c->firmware_reused=1U;
        c->firmware_version=image.major_version;c->firmware_subversion=image.minor_version;
        return RTW_92DU_OK;
    }
    /* We only support cold download: do not accept stale checksum as proof
     * that these payload bytes were accepted, or apply EU's reset to DU. */
    if (value&(FW_DOWNLOAD|FW_CHECKSUM|FW_RAM)) {
        c->last_error=RTW_92DU_MCU_BUSY; return c->last_error;
    }
    if (!read_reg(c,c->board.mac_index ? FW_MAC1_READY:FW_MAC0_READY,1U,&value))
        return abort_boot(c,0);
    if (value&1U) {
        /* Without RDY, an already-set per-MAC flag cannot prove this cold
         * download started. Do not invent a write-to-clear/reset protocol. */
        c->last_error=RTW_92DU_MCU_BUSY; return c->last_error;
    }
    if (!update(c,SYS_FUNC_HI,1U,0U,4U)) return abort_boot(c,1);
    c->download_active=1U; /* even a short enable write may have taken effect */
    if (!update(c,MCUFWDL,1U,0U,FW_DOWNLOAD) ||
        !update(c,MCUFWDL+2,1U,8U,0U)) return abort_boot(c,1);
    while (offset<image.payload_length) {
        const size_t remaining=image.payload_length-offset;
        const size_t page_length=remaining>PAGE_SIZE ? PAGE_SIZE:remaining;
        size_t in_page=0;
        if (!update(c,MCUFWDL+2,1U,7U,(uint32_t)(offset/PAGE_SIZE)))
            return abort_boot(c,1);
        while (in_page<page_length) {
            const size_t available=page_length-in_page;
            const uint16_t chunk=available>=64U ? 64U:(available>=8U ? 8U:1U);
            c->last_register=(uint16_t)(FW_WINDOW+in_page);
            if (rtw_usb_write_block(&c->bus,c->last_register,
                image.payload+offset+in_page,chunk)!=RTW_USB_OK) {
                c->last_error=RTW_92DU_IO_ERROR; return abort_boot(c,1);
            }
            in_page+=chunk; c->firmware_bytes+=chunk;
        }
        offset+=page_length;
    }
    if (!update(c,MCUFWDL,1U,FW_DOWNLOAD,0U)) return abort_boot(c,1);
    c->download_active=0;
    if (!poll(c,MCUFWDL,4U,FW_CHECKSUM,FW_CHECKSUM,5U) ||
        !update(c,MCUFWDL,4U,0U,FW_READY) ||
        !poll(c,c->board.mac_index ? FW_MAC1_READY:FW_MAC0_READY,
              1U,1U,1U,5U)) return abort_boot(c,1);
    c->firmware_ready=1U;
    c->firmware_version=image.major_version;c->firmware_subversion=image.minor_version;
    return RTW_92DU_OK;
}
const char *rtw_92du_result_string(rtw_92du_result result) {
    switch (result) {
    case RTW_92DU_OK: return "ok";
    case RTW_92DU_BAD_ARGUMENT: return "invalid boot argument";
    case RTW_92DU_IDENTITY_UNVERIFIED: return "RTL8192DU identity unverified";
    case RTW_92DU_UNSUPPORTED_CHIP: return "unsupported RTL8192DU silicon";
    case RTW_92DU_BAD_PROM: return "invalid RTL8192DU logical PROM";
    case RTW_92DU_IO_NOT_CONFIRMED: return "exclusive experimental I/O unconfirmed";
    case RTW_92DU_WRONG_STATE: return "boot state does not allow this operation";
    case RTW_92DU_MCU_BUSY: return "cold boot required; MAC or MCU already active";
    case RTW_92DU_BAD_FIRMWARE: return "invalid RTL8192DU firmware image";
    case RTW_92DU_IO_ERROR: return "USB transfer failed or was short";
    case RTW_92DU_WAIT_ERROR: return "boot delay failed";
    case RTW_92DU_POLL_TIMEOUT: return "boot register polling timed out";
    }
    return "unknown RTL8192DU boot result";
}
