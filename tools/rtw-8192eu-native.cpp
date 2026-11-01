// SPDX-License-Identifier: GPL-2.0-only
// Native RTL8192EU USB bring-up laboratory tool. No Apple Wi-Fi stack, OCLP,
// vendor kext, EFI or system modification. Firmware is supplied by caller.
#include "rtw/RTL8192EUHardware.h"
#include "rtw/RTL8192EU.h"
#include <libusb.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {
struct Session {
    libusb_context* context=nullptr;
    libusb_device_handle* handle=nullptr;
    bool claimed=false;
    ~Session(){
        if(claimed && handle) libusb_release_interface(handle,0);
        if(handle) libusb_close(handle);
        if(context) libusb_exit(context);
    }
};
static rtw_usb_error control(void* context,const rtw_usb_setup* request,
                             uint8_t* data,uint16_t* actual){
    if(actual) *actual=0;
    if(!context || !data || !request || !actual || request->bm_request_type!=0xC0 ||
       request->b_request!=0x05 || request->w_index!=0 ||
       (request->w_length!=1 && request->w_length!=2 && request->w_length!=4))
       return RTW_USB_INVALID_ARGUMENT;
    const int received=libusb_control_transfer(
        static_cast<libusb_device_handle*>(context),request->bm_request_type,
        request->b_request,request->w_value,request->w_index,data,
        request->w_length,request->timeout_ms);
    if(received<0) return RTW_USB_IO_ERROR;
    if(received!=request->w_length) return RTW_USB_SHORT_TRANSFER;
    *actual=static_cast<uint16_t>(received);
    return RTW_USB_OK;
}
static rtw_usb_error controlWrite(void* context,const rtw_usb_setup* request,
                                 const uint8_t* data,uint16_t* actual){
    if(actual) *actual=0;
    if(!context || !request || !data || !actual || request->bm_request_type!=0x40 ||
       request->b_request!=0x05 || request->w_index!=0 ||
       request->w_length==0 || request->w_length>RTW_USB_MAX_VENDOR_PAYLOAD)
       return RTW_USB_INVALID_ARGUMENT;
    // libusb expects a mutable buffer; never cast away const from callers.
    uint8_t scratch[RTW_USB_MAX_VENDOR_PAYLOAD];
    std::memcpy(scratch,data,request->w_length);
    const int written=libusb_control_transfer(
        static_cast<libusb_device_handle*>(context),request->bm_request_type,
        request->b_request,request->w_value,request->w_index,scratch,
        request->w_length,request->timeout_ms);
    std::memset(scratch,0,sizeof(scratch));
    if(written<0) return RTW_USB_IO_ERROR;
    if(written!=request->w_length) return RTW_USB_SHORT_TRANSFER;
    *actual=static_cast<uint16_t>(written);
    return RTW_USB_OK;
}
static int sleepUs(void*,uint32_t delay){
    std::this_thread::sleep_for(std::chrono::microseconds(delay));
    return 1;
}
static void usage(){
    std::puts("rtw-8192eu-native [--probe] [--firmware PATH --confirm-hardware-write]");
    std::puts("Defaults to register-only detection. No EFI, OCLP or third-party drivers.");
    std::puts("--firmware explicitly uploads original RTL8192EU NIC firmware to volatile RAM.");
    std::puts("WARNING: experimental, can hang the adapter until physically unplugged.");
    std::puts("A successful firmware boot does NOT mean Wi-Fi connectivity.");
}
static bool readFile(const std::string& filename,std::vector<uint8_t>& out){
    std::ifstream stream(filename,std::ios::binary|std::ios::ate);
    if(!stream) return false;
    auto sz=stream.tellg();
    if(sz<33 || sz>static_cast<std::streamoff>(32U+RTW_8192EU_FW_MAX_PAYLOAD))
        return false;
    out.resize(static_cast<size_t>(sz));
    stream.seekg(0);
    return bool(stream.read(reinterpret_cast<char*>(out.data()),sz));
}
}
int main(int argc,char** argv){
    std::string filename;
    bool confirm=false;
    for(int i=1;i<argc;++i){
        if(std::strcmp(argv[i],"--help")==0 || std::strcmp(argv[i],"-h")==0){usage();return 0;}
        if(std::strcmp(argv[i],"--probe")==0)continue;
        if(std::strcmp(argv[i],"--firmware")==0 && i+1<argc){filename=argv[++i];continue;}
        if(std::strcmp(argv[i],"--confirm-hardware-write")==0){confirm=true;continue;}
        std::fprintf(stderr,"Invalid option: %s\n",argv[i]);usage();return 2;
    }
    if((!filename.empty())!=confirm){
        std::fputs("Firmware upload requires BOTH --firmware and --confirm-hardware-write.\n",stderr);
        return 2;
    }
    std::vector<uint8_t> image;
    rtw_8192eu_fw_info info{};
    if(!filename.empty()){
        if(!readFile(filename,image) ||
           rtw_8192eu_inspect_firmware(image.data(),image.size(),&info)!=RTW_8192EU_OK){
            std::fputs("Not a valid bounded RTL8192EU firmware 0x92Ex image.\n",stderr);
            return 3;
        }
        std::fprintf(stderr,"RTL8192EU firmware %u.%u, %zu bytes: USB WRITES requested.\n",
                     static_cast<unsigned>(info.major_version), static_cast<unsigned>(info.minor_version), info.payload_length);
    }
    Session session;
    int result=libusb_init(&session.context);
    if(result!=LIBUSB_SUCCESS){
        std::fprintf(stderr,"libusb initialization: %s\n",libusb_error_name(result));return 4;
    }
    session.handle=libusb_open_device_with_vid_pid(session.context,
                                                   RTW_8192EU_USB_VID,RTW_8192EU_USB_PID);
    if(!session.handle){
        std::fputs("USB 2357:0108 not found. Do not substitute a different Realtek device.\n",stderr);
        return 4;
    }
    libusb_device_descriptor descriptor{};
    if(libusb_get_device_descriptor(libusb_get_device(session.handle),&descriptor)!=LIBUSB_SUCCESS ||
       descriptor.idVendor!=RTW_8192EU_USB_VID || descriptor.idProduct!=RTW_8192EU_USB_PID){
        std::fputs("Device descriptor/VID:PID mismatch.\n",stderr);return 4;
    }
    libusb_config_descriptor* config=nullptr;
    result=libusb_get_active_config_descriptor(libusb_get_device(session.handle),&config);
    if(result!=LIBUSB_SUCCESS || !config){
        std::fprintf(stderr,"USB configuration unavailable: %s\n",libusb_error_name(result));
        return 4;
    }
    unsigned numIn=0,numOut=0;
    bool valid=false;
    for(uint8_t i=0;i<config->bNumInterfaces;++i) {
        const libusb_interface& intf=config->interface[i];
        for(int a=0;a<intf.num_altsetting;++a) {
            const libusb_interface_descriptor& alt=intf.altsetting[a];
            if(alt.bInterfaceNumber!=0 || alt.bAlternateSetting!=0 ||
               alt.bInterfaceClass!=0xff || alt.bInterfaceSubClass!=0xff ||
               alt.bInterfaceProtocol!=0xff)continue;
            valid=true;
            for(uint8_t ep=0;ep<alt.bNumEndpoints;++ep) {
                const libusb_endpoint_descriptor& e=alt.endpoint[ep];
                if((e.bmAttributes&0x3)!=LIBUSB_TRANSFER_TYPE_BULK)continue;
                if(e.bEndpointAddress&LIBUSB_ENDPOINT_IN)++numIn;
                else ++numOut;
            }
        }
    }
    libusb_free_config_descriptor(config);
    if(!valid || numIn<1 || numOut<1) {
        std::fputs("Unexpected USB interface descriptor (expect FF/FF/FF + bulk).\n",stderr);
        return 4;
    }
    result=libusb_claim_interface(session.handle,0);
    if(result!=LIBUSB_SUCCESS){
        std::fprintf(stderr,"Cannot claim interface: %s (no kernel driver detachment attempted).\n",
                     libusb_error_name(result));return 4;
    }
    session.claimed=true;
    const rtw_usb_rw_transport bus{session.handle,&control,&controlWrite};
    const rtw_usb_transport readBus{session.handle,&control};
    rtw_8192eu_identity identity{};
    const auto probed=rtw_8192eu_probe_registers(&readBus,descriptor.idVendor,
                                                 descriptor.idProduct,&identity);
    if(probed!=RTW_8192EU_OK){
        std::fprintf(stderr,"RTL8192EU register probe failed: %s\n",rtw_8192eu_result_string(probed));
        return 5;
    }
    std::printf("2357:0108 SYSCFG=0x%08x cut=%u foundry=%u paths=%u, FWDL=0x%08x, bulk=%u/%u\n",
                identity.sys_cfg,static_cast<unsigned>(identity.silicon_cut),static_cast<unsigned>(identity.foundry_code),
                static_cast<unsigned>(identity.inferred_tx_paths),identity.mcu_fw_dl,numIn,numOut);
    if(filename.empty()){
        std::puts("Read-only identification complete: NO REGISTERS WRITTEN.");
        return 0;
    }
    const rtw_waiter waiter{nullptr,&sleepUs};
    const rtw_92e_boot_config settings{500U,100U};
    rtw_92e_boot_report report{};
    const rtw_92e_boot_result boot=rtw_8192eu_boot_8051(&bus,&waiter,
        descriptor.idVendor,descriptor.idProduct,image.data(),image.size(),
        &settings,&report);
    std::printf("8051 boot: %s | stage=%u | pages=%u | bytes=%zu | transfers=%zu | status=0x%08x\n",
                rtw_8192eu_boot_result_string(boot),static_cast<unsigned>(report.last_stage),static_cast<unsigned>(report.pages),
                report.firmware_bytes,report.usb_firmware_writes,report.last_mcu_status);
    std::fill(image.begin(),image.end(),0);
    if(boot!=RTW_92E_BOOT_OK){
        std::fputs("Failed. Do not repeat without unplug/replug and checking the report.\n",stderr);
        return 6;
    }
    std::puts("The USB 8051 boot stage succeeded. PHY/RF, association and Ethernet are NOT active.");
    return 0;
}
