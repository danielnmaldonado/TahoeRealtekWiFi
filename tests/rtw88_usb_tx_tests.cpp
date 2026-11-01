/* SPDX-License-Identifier: BSD-3-Clause */
#include "rtw/RTW88USBDataPath.h"
#include "rtw/FamilyBackend.h"
#include <array>
#include <cassert>
#include <cstring>
#include <cstdio>
static const uint8_t mgmt32[48]={
0x24,0x00,0x30,0x84,0x00,0x12,0x06,0x00,0,0,0,0,0x00,0x05,0,0,0x04,0,0,0,0,0,0,0,0,0,0,0,0x16,0x93,0,0,0x00,0x80,0,0,0x00,0xc0,0xab,0x00,0,0,0,0,0,0,0,0};
static const uint8_t mgmt48[48]={
0x24,0x00,0x30,0x84,0x00,0x12,0x06,0x00,0,0,0,0,0x00,0x05,0,0,0x04,0,0,0,0,0,0,0,0,0,0,0,0xbd,0xd3,0,0,0x00,0x80,0,0,0x00,0xc0,0xab,0x00,0,0,0,0,0,0,0,0};
static const uint8_t vht32[48]={
0xdc,0x05,0x30,0x04,0x03,0x05,0x09,0x00,0x00,0x10,0x38,0x00,0x00,0x01,0x3e,0x00,0x3f,0,0,0,0xd0,0,0,0,0xfc,0,0,0,0xf3,0x15,0,0,0,0,0,0,0x00,0x30,0x12,0x00,0,0,0,0,0,0,0,0};
static const uint8_t vht48[48]={
0xdc,0x05,0x30,0x04,0x03,0x05,0x09,0x00,0x00,0x10,0x38,0x00,0x00,0x01,0x3e,0x00,0x3f,0,0,0,0xd0,0,0,0,0xfc,0,0,0,0xe1,0x25,0,0,0,0,0,0,0x00,0x30,0x12,0x00,0,0,0,0,0,0,0,0};
static rtw88_usb_tx_options mgmt(){rtw88_usb_tx_options o{};o.sequence=0xabc;o.queue=18;o.rate_id=6;o.rate=4;o.band_5ghz=1;o.disable_fallback=1;o.hardware_sequence=1;o.disable_queue_sequence=1;return o;}
static unsigned xor16(const uint8_t*d,unsigned n){unsigned x=0;for(unsigned i=0;i<n;i+=2)x^=d[i]|unsigned(d[i+1])<<8;return x;}
int main(){
    std::array<uint8_t,64>d;auto o=mgmt();
    for(auto f:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        d.fill(0xa5);assert(rtw88_usb_make_tx_descriptor(f,36,&o,d.data(),d.size())==RTW88_USB_TX_OK);
        assert(!std::memcmp(d.data(),f==RTW_WIFI_RTL8822CU?mgmt48:mgmt32,48));
        assert(d[48]==0xa5&&d[63]==0xa5);assert(xor16(d.data(),f==RTW_WIFI_RTL8822CU?48:32)==0);
    }
    d.fill(0xa5);auto before=d;
    assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8814BU,36,&o,d.data(),48)==RTW88_USB_TX_UNSUPPORTED_FAMILY);assert(d==before);
    assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8821CU,36,&o,d.data(),47)==RTW88_USB_TX_INVALID_ARGUMENT);assert(d==before);
    assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8821CU,0,&o,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(d==before);
    assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8821CU,65536,&o,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(d==before);
    o.rate=63;o.bandwidth=2;
    assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8821CU,1500,&o,d.data(),48)==RTW88_USB_TX_UNSUPPORTED_RATE);assert(d==before);
    o=mgmt();o.rate=3;assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8822BU,36,&o,d.data(),48)==RTW88_USB_TX_UNSUPPORTED_RATE);assert(d==before);
    o=mgmt();o.queue=20;assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8822BU,36,&o,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(d==before);
    o=mgmt();o.sequence=4096;assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8822BU,36,&o,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(d==before);
    o={};o.sequence=0x123;o.report_tag=0xfc;o.mac_id=3;o.queue=5;o.rate_id=9;o.rate=63;
    o.bandwidth=2;o.band_5ghz=1;o.short_gi=1;o.ldpc=1;o.ampdu=1;o.ampdu_density=3;o.ampdu_factor=31;o.request_report=1;
    for(auto f:{RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        assert(!rtw88_usb_make_tx_descriptor(f,1500,&o,d.data(),48));
        assert(!std::memcmp(d.data(),f==RTW_WIFI_RTL8822BU?vht32:vht48,48));
    }
    for(unsigned rate=0;rate<128;rate++){
        auto q=mgmt();q.rate=uint8_t(rate);q.hardware_sequence=0;
        bool valid=(rate>=4&&rate<=11)||(rate>=12&&rate<=19)||(rate>=44&&rate<=53);
        d.fill(0xa5);before=d;
        auto result=rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8821CU,100,&q,d.data()+1,48);
        assert((result==RTW88_USB_TX_OK)==valid);if(!valid)assert(d==before);
        else assert(d[0]==0xa5&&d[49]==0xa5);
    }
    for(unsigned field=0;field<8;field++){
        auto q=mgmt();switch(field){case 0:q.rate_id=32;break;case 1:q.report_tag=4096;break;
        case 2:q.bandwidth=3;break;case 3:q.ampdu_density=8;break;case 4:q.ampdu_factor=32;break;
        case 5:q.band_5ghz=2;break;case 6:q.hardware_sequence=2;break;case 7:q.firmware_rate=2;break;}
        d.fill(0xa5);before=d;assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8822BU,100,&q,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(d==before);
    }
    {alignas(rtw88_usb_tx_options) uint8_t storage[96]{};auto q=mgmt();std::memcpy(storage,&q,sizeof(q));
     std::array<uint8_t,96> snapshot;std::memcpy(snapshot.data(),storage,96);
     assert(rtw88_usb_make_tx_descriptor(RTW_WIFI_RTL8822BU,100,reinterpret_cast<const rtw88_usb_tx_options*>(storage),storage+4,48)==RTW88_USB_TX_INVALID_ARGUMENT);
     assert(!std::memcmp(snapshot.data(),storage,96));}
    for(auto f:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        const auto* backend=rtw_backend_for_family(f);assert(backend);
        assert(backend->capabilities&RTW_BACKEND_CAP_TX_WIRE);
        assert(backend->capabilities&RTW_BACKEND_CAP_KEXT_STATION);
    }
    const uint8_t reserved[48]={0x28,0,0x30,4,0,0x10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x18,0x14,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    const uint8_t h2c[48]={0x20,0,0,0,0,0x13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x20,0x13,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    for(auto f:{RTW_WIFI_RTL8821CU,RTW_WIFI_RTL8822BU,RTW_WIFI_RTL8822CU}){
        assert(!rtw88_usb_make_control_descriptor(f,40,RTW88_USB_CONTROL_RESERVED,d.data(),48));assert(!std::memcmp(d.data(),reserved,48));
        assert(!rtw88_usb_make_control_descriptor(f,32,RTW88_USB_CONTROL_H2C,d.data(),48));assert(!std::memcmp(d.data(),h2c,48));
        before=d;assert(rtw88_usb_make_control_descriptor(f,31,RTW88_USB_CONTROL_H2C,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(before==d);
        assert(rtw88_usb_make_control_descriptor(f,4098,RTW88_USB_CONTROL_RESERVED,d.data(),48)==RTW88_USB_TX_INVALID_ARGUMENT);assert(before==d);
    }
    std::puts("rtw88 USB TX and control golden descriptors and input boundaries passed");
}
