/* SPDX-License-Identifier: BSD-3-Clause
 * Bounded board-identification diagnostic for one2357:0106. The only OUT
 * requests select/read OTP; no OTP programming, reset, RF, firmware or power-on.
 * Access/clock/bank snapshots are restored by the shared production reader. */
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include "rtw/RTL8814AU.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <thread>
struct USBBoard {
    IOUSBDeviceInterface500 **device;
    std::chrono::steady_clock::time_point deadline;
    unsigned reads=0,writes=0;
    uint32_t original[4]{};bool captured[4]{};
};
static int indexOf(uint16_t a) {return a==0xcf?0:a==2?1:a==8?2:a==0x34?3:-1;}
static unsigned widthOf(int i) {return i==0?1:i==3?4:2;}
static int request(USBBoard *b,bool input,uint16_t a,unsigned width,uint32_t *value) {
    uint8_t bytes[4]={uint8_t(*value),uint8_t(*value>>8),uint8_t(*value>>16),uint8_t(*value>>24)};
    IOUSBDevRequestTO r{};r.bmRequestType=input?0xc0:0x40;r.bRequest=5;r.wValue=a;
    r.wLength=uint16_t(width);r.pData=bytes;r.noDataTimeout=500;r.completionTimeout=1000;
    auto result=(*b->device)->DeviceRequestTO(b->device,&r);
    if(result!=kIOReturnSuccess||r.wLenDone!=width){std::fprintf(stderr,"USB_ERROR=0x%x ADDRESS=0x%04x IN=%u\n",result,a,input);return EIO;}
    if(input){*value=bytes[0];if(width>=2)*value|=uint32_t(bytes[1])<<8;if(width==4)*value|=uint32_t(bytes[2])<<16|uint32_t(bytes[3])<<24;}
    return 0;
}
static int readBoard(void *p,uint16_t a,unsigned width,uint32_t *value) {
    auto *b=static_cast<USBBoard*>(p);*value=0;
    if(++b->reads>10000||std::chrono::steady_clock::now()>b->deadline)return ETIMEDOUT;
    int i=indexOf(a);
    if(!((i>=0&&width==widthOf(i))||(a==0x30&&width==4)||((a==0xf0||a==0xfc||a==4)&&width==4)))return EPERM;
    int error=request(b,true,a,width,value);
    if(!error&&i>=0&&!b->captured[i]){b->original[i]=*value;b->captured[i]=true;}
    return error;
}
static int writeBoard(void *p,uint16_t a,unsigned width,uint32_t value) {
    auto *b=static_cast<USBBoard*>(p);int i=indexOf(a);
    if(++b->writes>5000)return EOVERFLOW;
    if(a==0x30){if(width!=4||(value&0x80000000U))return EPERM;}
    else {
        if(i<0||!b->captured[i]||width!=widthOf(i))return EPERM;
        const uint32_t mask=i==0?0xff:i==1?0x1000:i==2?0x22:0x300;
        if((value^b->original[i])&~mask)return EPERM;
        if(i==0&&value!=0x69&&value!=b->original[i])return EPERM;
    }
    return request(b,false,a,width,&value);
}
static void delayBoard(void*,uint32_t us){std::this_thread::sleep_for(std::chrono::microseconds(us));}
static void number(CFMutableDictionaryRef d,CFStringRef key,int value){auto n=CFNumberCreate(nullptr,kCFNumberIntType,&value);CFDictionarySetValue(d,key,n);CFRelease(n);}
#ifndef RTW_BOARD_HELPER_ONLY
int main(int argc,char **argv) {
    if(argc!=2||std::strcmp(argv[1],"--read-board")){std::fputs("Usage: rtw-8814au-board --read-board\n",stderr);return 1;}
    auto match=IOServiceMatching(kIOUSBDeviceClassName);if(!match)return 1;
    number(match,CFSTR("idVendor"),0x2357);number(match,CFSTR("idProduct"),0x0106);
    io_iterator_t iterator=0;auto result=IOServiceGetMatchingServices(kIOMainPortDefault,match,&iterator);
    if(result!=KERN_SUCCESS)return 2;
    io_service_t service=IOIteratorNext(iterator);auto second=IOIteratorNext(iterator);IOObjectRelease(iterator);
    if(!service||second){if(service)IOObjectRelease(service);if(second)IOObjectRelease(second);return 2;}
    IOCFPlugInInterface **plugin=nullptr;SInt32 score=0;
    result=IOCreatePlugInInterfaceForService(service,kIOUSBDeviceUserClientTypeID,kIOCFPlugInInterfaceID,&plugin,&score);IOObjectRelease(service);
    if(result!=KERN_SUCCESS||!plugin)return 3;
    IOUSBDeviceInterface500 **device=nullptr;
    auto query=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500),reinterpret_cast<LPVOID*>(&device));(*plugin)->Release(plugin);
    if(query||!device)return 3;
    UInt16 vendor=0,product=0;UInt8 speed=0,configuration=0;
    result=(*device)->GetDeviceVendor(device,&vendor);
    if(result==kIOReturnSuccess)result=(*device)->GetDeviceProduct(device,&product);
    if(result==kIOReturnSuccess)result=(*device)->GetDeviceSpeed(device,&speed);
    if(result==kIOReturnSuccess)result=(*device)->GetConfiguration(device,&configuration);
    if(result!=kIOReturnSuccess||vendor!=0x2357||product!=0x0106||configuration!=1||speed!=kUSBDeviceSpeedHigh){(*device)->Release(device);return 3;}
    result=(*device)->USBDeviceOpen(device);
    if(result!=kIOReturnSuccess){std::printf("OPEN_ERROR=0x%x\n",result);(*device)->Release(device);return 4;}
    USBBoard usb{device,std::chrono::steady_clock::now()+std::chrono::seconds(30)};
    rtw_8814au_bus_ops ops{&usb,readBoard,writeBoard,nullptr,delayBoard};rtw_8814au context{};
    rtw_8814au_init(&context,&ops);
    int error=rtw_8814au_identify_read_board(&context,RTW_8814AU_HIGH_SPEED);
    const auto &b=context.board;
    std::printf("USB=2357:0106 OTP_READ_ONLY=1 RESULT=%d IO_ERROR=%d READS=%u WRITES=%u\n",error,context.io_error,usb.reads,usb.writes);
    if(!error)std::printf("CUT=%u MP=%u VENDOR_RAW=%u RFE=%u RF_PATHS=%u TX_STREAMS=%u RX_STREAMS=%u ANT_TX=0x%x ANT_RX=0x%x XTAL=%u THERMAL=%u CHANNEL_PLAN=0x%x REGD=%u\n",b.cut,b.mp_chip,b.vendor,b.rfe_option,b.physical_rf_paths,b.tx_streams,b.rx_streams,b.antenna_tx,b.antenna_rx,b.crystal_cap,b.thermal_meter,b.channel_plan,b.regd);
    (*device)->USBDeviceClose(device);(*device)->Release(device);
    return error?5:0;
}
#endif
