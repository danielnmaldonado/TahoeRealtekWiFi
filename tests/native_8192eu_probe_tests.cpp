/* Native IOKit transport tests: the production CLI runs against an in-process
 * USB vtable. No real USB, driver, firmware, reset or radio operation occurs. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

static IOUSBDeviceInterface500 deviceTable{};
static IOUSBDeviceInterface500 *devicePointer=&deviceTable;
static IOCFPlugInInterface pluginTable{};
static IOCFPlugInInterface *pluginPointer=&pluginTable;
static unsigned matches=1,iterated=0,opens=0,closes=0,reads=0,releases=0;
static bool shortRead=false,oversizeRead=false,failRead=false,wrongIdentity=false;
static IOReturn openResult=kIOReturnSuccess;
static kern_return_t fakeServices(mach_port_t,CFDictionaryRef matching,io_iterator_t* out) {
    int vendor=0,product=0;
    auto vid=static_cast<CFNumberRef>(CFDictionaryGetValue(matching,CFSTR("idVendor")));
    auto pid=static_cast<CFNumberRef>(CFDictionaryGetValue(matching,CFSTR("idProduct")));
    assert(vid&&pid&&CFNumberGetValue(vid,kCFNumberIntType,&vendor)&&CFNumberGetValue(pid,kCFNumberIntType,&product));
    assert(vendor==0x2357&&product==0x0108);CFRelease(matching);iterated=0;*out=900;return KERN_SUCCESS;
}
static io_object_t fakeNext(io_iterator_t) {return iterated++<matches?100+iterated:IO_OBJECT_NULL;}
static kern_return_t fakeRelease(io_object_t) {return KERN_SUCCESS;}
[[maybe_unused]] static kern_return_t fakePlugin(io_service_t,CFUUIDRef,CFUUIDRef,IOCFPlugInInterface*** out,SInt32*) {*out=&pluginPointer;return KERN_SUCCESS;}
#define IOServiceGetMatchingServices fakeServices
#define IOIteratorNext fakeNext
#define IOObjectRelease fakeRelease
#define IOCreatePlugInInterfaceForService fakePlugin
#define main production_probe_main
#include "../tools/rtw-probe.cpp"
#undef main
#undef IOServiceGetMatchingServices
#undef IOIteratorNext
#undef IOObjectRelease
#undef IOCreatePlugInInterfaceForService

static ULONG releaseDevice(void*) {++releases;return 0;}
static ULONG releasePlugin(void*) {return 0;}
static HRESULT query(void*,REFIID,LPVOID* out) {*out=&devicePointer;return S_OK;}
static IOReturn vendor(void*,UInt16* out) {*out=0x2357;return kIOReturnSuccess;}
static IOReturn product(void*,UInt16* out) {*out=wrongIdentity?0x0106:0x0108;return kIOReturnSuccess;}
static IOReturn openDevice(void*) {++opens;return openResult;}
static IOReturn closeDevice(void*) {++closes;return kIOReturnSuccess;}
static IOReturn request(void*,IOUSBDevRequestTO* r) {
    ++reads;assert(r&&r->bmRequestType==0xc0&&r->bRequest==5&&r->wIndex==0&&r->wLength==4);
    assert(r->noDataTimeout&&r->noDataTimeout<=1000&&r->completionTimeout&&r->completionTimeout<=1000);
    uint32_t value=0;
    switch(r->wValue){
    case RTW_8192EU_REG_SYS_CFG:value=0x04011234;break;
    case RTW_8192EU_REG_HPON_FSM:value=0x00400000;break;
    case RTW_8192EU_REG_SYS_CFG2:value=0x113;break;
    case RTW_8192EU_REG_MCU_FW_DL:value=0x604c6;break;
    default:assert(false);
    }
    if(failRead)return kIOReturnNoDevice;
    for(unsigned i=0;i<4;i++)static_cast<uint8_t*>(r->pData)[i]=uint8_t(value>>(i*8));
    r->wLenDone=oversizeRead?5:shortRead?3:4;return kIOReturnSuccess;
}
static void reset() {
    matches=1;iterated=opens=closes=reads=releases=0;shortRead=oversizeRead=failRead=wrongIdentity=false;openResult=kIOReturnSuccess;
    deviceTable={};pluginTable={};pluginTable.QueryInterface=query;pluginTable.Release=releasePlugin;
    deviceTable.Release=releaseDevice;deviceTable.GetDeviceVendor=vendor;deviceTable.GetDeviceProduct=product;
    deviceTable.USBDeviceOpen=openDevice;deviceTable.USBDeviceClose=closeDevice;deviceTable.DeviceRequestTO=request;
    // All setters/reset/firmware/interface vtable entries intentionally remain
    // null, so any unexpected mutation/claim immediately fails this test.
}
int main() {
    reset();assert(readRTL8192EUIdentity()==0);assert(opens==1&&closes==1&&reads==4&&releases==1);
    reset();matches=2;assert(readRTL8192EUIdentity()!=0);assert(!opens&&!reads);
    reset();matches=0;assert(readRTL8192EUIdentity()!=0);assert(!opens&&!reads);
    reset();wrongIdentity=true;assert(readRTL8192EUIdentity()!=0);assert(!opens&&!reads&&releases==1);
    reset();openResult=kIOReturnExclusiveAccess;assert(readRTL8192EUIdentity()!=0);assert(opens==1&&!closes&&!reads&&releases==1);
    reset();shortRead=true;assert(readRTL8192EUIdentity()!=0);assert(opens==1&&closes==1&&reads==1&&releases==1);
    reset();failRead=true;assert(readRTL8192EUIdentity()!=0);assert(opens==1&&closes==1&&reads==1&&releases==1);
    reset();oversizeRead=true;assert(readRTL8192EUIdentity()!=0);assert(opens==1&&closes==1&&reads==1&&releases==1);
    reset();{
        Native8192USB usb;assert(openNative8192USB(usb));
        const rtw_usb_setup valid{0xc0,5,RTW_8192EU_REG_SYS_CFG,0,4,1000};
        uint8_t bytes[4]{};uint16_t actual=99;
        for(unsigned bad=0;bad<7;bad++){
            auto setup=valid;
            switch(bad){case 0:setup.bm_request_type=0x40;break;case 1:setup.b_request=6;break;
            case 2:setup.w_value=0x30;break;case 3:setup.w_index=1;break;
            case 4:setup.w_length=8;break;case 5:setup.timeout_ms=0;break;
            case 6:setup.timeout_ms=1001;break;}
            actual=99;assert(native8192ReadAdapter(&usb,&setup,bytes,&actual)!=RTW_USB_OK);
            assert(actual==0&&reads==0);
        }
        assert(native8192ReadAdapter(nullptr,&valid,bytes,&actual)!=RTW_USB_OK&&reads==0);
        assert(native8192ReadAdapter(&usb,nullptr,bytes,&actual)!=RTW_USB_OK&&reads==0);
        assert(native8192ReadAdapter(&usb,&valid,nullptr,&actual)!=RTW_USB_OK&&reads==0);
        assert(native8192ReadAdapter(&usb,&valid,bytes,nullptr)!=RTW_USB_OK&&reads==0);
    }assert(opens==1&&closes==1&&releases==1);
    std::puts("Native RTL8192EU probe: eight lifecycle cases plus eleven rejected requests passed");
}
