/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtw/RTL8192DUUSB.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

#include <cstring>
struct Bus {
    rtw_usb_setup seen{};unsigned calls=0;rtw_usb_error error=RTW_USB_OK;bool short_io=false;
    static rtw_usb_error in(void* p,const rtw_usb_setup* s,uint8_t* b,uint16_t* actual) {
        auto& f=*static_cast<Bus*>(p);f.seen=*s;++f.calls;
        if(f.error!=RTW_USB_OK)return f.error;
        memset(b,0xab,s->w_length);*actual=f.short_io?uint16_t(s->w_length-1):s->w_length;return RTW_USB_OK;
    }
    static rtw_usb_error out(void* p,const rtw_usb_setup* s,const uint8_t*,uint16_t* actual) {
        auto& f=*static_cast<Bus*>(p);f.seen=*s;++f.calls;
        if(f.error!=RTW_USB_OK)return f.error;
        *actual=f.short_io?uint16_t(s->w_length-1):s->w_length;return RTW_USB_OK;
    }
};
int main() {
    for(uint8_t mac=0;mac<2;++mac)for(unsigned reg=0;reg<65536;++reg) {
        uint16_t mapped=0;unsigned expected=reg;
        if(mac){if(reg<0x1000)expected|=0x4000;else if((reg&0x2000) && !(reg&0x8000))expected&=0xfff;}
        assert(rtw_92du_usb_address(mac,uint16_t(reg),&mapped)==RTW_USB_OK && mapped==expected);
    }
    uint16_t unchanged=0x5a5a;
    assert(rtw_92du_usb_address(2,0,&unchanged)==RTW_USB_INVALID_ARGUMENT && unchanged==0x5a5a);
    assert(rtw_92du_usb_address(0,0,nullptr)==RTW_USB_INVALID_ARGUMENT);
    for(uint8_t mac=0;mac<2;++mac) {
        Bus b;rtw_usb_rw_transport raw{&b,Bus::in,Bus::out};rtw_92du_usb_map m{};
        assert(rtw_92du_usb_map_init(&m,&raw,mac)==RTW_USB_OK && !b.calls);
        const auto* bus=rtw_92du_usb_bus(&m);assert(bus);
        for(uint16_t reg:{uint16_t(0x522),uint16_t(0x1000),uint16_t(0x2840),uint16_t(0x4840),uint16_t(0xfe56)}) {
            uint16_t physical=0;assert(rtw_92du_usb_address(mac,reg,&physical)==RTW_USB_OK);
            uint32_t result=0;
            assert(rtw_usb_rw_read(bus,reg,1,&result)==RTW_USB_OK && result==0xab && b.seen.w_value==physical);
            assert(rtw_usb_write_register(bus,reg,1,0xff)==RTW_USB_OK && b.seen.w_value==physical);
        }
        b.short_io=true;uint32_t result=0;
        assert(rtw_usb_rw_read(bus,0x522,1,&result)==RTW_USB_SHORT_TRANSFER);
        assert(rtw_usb_write_register(bus,0x522,1,0xff)==RTW_USB_SHORT_TRANSFER);
        b.short_io=false;b.error=RTW_USB_IO_ERROR;
        assert(rtw_usb_rw_read(bus,0x522,1,&result)==RTW_USB_IO_ERROR);
        assert(rtw_usb_write_register(bus,0x522,1,0xff)==RTW_USB_IO_ERROR);
        const auto before=b.calls;uint8_t bytes[4]{};uint16_t actual=99;
        rtw_usb_setup invalid{0xc0,5,0x522,1,1,1000};
        assert(bus->control_in(bus->context,&invalid,bytes,&actual)==RTW_USB_INVALID_ARGUMENT && !actual && b.calls==before);
    }
    puts("RTL8192DU per-MAC vendor address mapping: 131072 primary-source cases verified");
}
