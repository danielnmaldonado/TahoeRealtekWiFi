// Read-only macOS USB discovery and bounded RTL8192EU / RTL8812AU register probes.
// Does not claim an interface, reset the device, send writes or install drivers.
#include "rtw/USBIDs.hpp"
#include "rtw/RTL8192EU.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#endif
#ifdef RTW_WITH_LIBUSB
#include <libusb.h>
#endif

namespace {
struct Device {
    std::uint16_t vendor = 0;
    std::uint16_t product = 0;
    std::uint32_t location = 0;
    std::string label;
};

std::string hex16(std::uint16_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(4) << std::setfill('0') << value;
    return out.str();
}
std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
    return out.str();
}
std::string jsonQuote(const std::string& s) {
    std::string out = "\"";
    const char* hex = "0123456789abcdef";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c < 0x20) {
            out += "\\u00";
            out += hex[c >> 4];
            out += hex[c & 0x0f];
        } else out += char(c);
    }
    return out + '"';
}

#ifdef __APPLE__
bool readNumber(io_registry_entry_t entry, CFStringRef property, std::uint32_t& number) {
    CFTypeRef value = IORegistryEntryCreateCFProperty(entry, property, kCFAllocatorDefault, 0);
    if (!value) return false;
    bool ok = false;
    if (CFGetTypeID(value) == CFNumberGetTypeID()) {
        std::int64_t x = 0;
        if (CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberSInt64Type, &x) &&
            x >= 0 && x <= 0xFFFFFFFFLL) {
            number = static_cast<std::uint32_t>(x);
            ok = true;
        }
    }
    CFRelease(value);
    return ok;
}
std::string readString(io_registry_entry_t entry, CFStringRef property) {
    CFTypeRef value = IORegistryEntryCreateCFProperty(entry, property, kCFAllocatorDefault, 0);
    if (!value) return {};
    std::string result;
    if (CFGetTypeID(value) == CFStringGetTypeID()) {
        char buf[512] = {};
        if (CFStringGetCString(static_cast<CFStringRef>(value), buf, sizeof(buf), kCFStringEncodingUTF8))
            result = buf;
    }
    CFRelease(value);
    return result;
}
std::vector<Device> listMacDevices() {
    std::vector<Device> result;
    CFMutableDictionaryRef match = IOServiceMatching("IOUSBHostDevice");
    if (!match) return result;
    io_iterator_t iterator = IO_OBJECT_NULL;
    const kern_return_t status = IOServiceGetMatchingServices(kIOMainPortDefault, match, &iterator);
    // The call consumes 'match' even if matching fails.
    if (status != KERN_SUCCESS) return result;
    io_service_t service = IO_OBJECT_NULL;
    while ((service = IOIteratorNext(iterator)) != IO_OBJECT_NULL) {
        std::uint32_t vid = 0, pid = 0, location = 0;
        const bool hasVid = readNumber(service, CFSTR("idVendor"), vid);
        const bool hasPid = readNumber(service, CFSTR("idProduct"), pid);
        (void) readNumber(service, CFSTR("locationID"), location);
        if (hasVid && hasPid && vid <= 0xFFFF && pid <= 0xFFFF) {
            Device d;
            d.vendor = static_cast<std::uint16_t>(vid);
            d.product = static_cast<std::uint16_t>(pid);
            d.location = location;
            d.label = readString(service, CFSTR("USB Product Name"));
            if (d.label.empty()) d.label = readString(service, CFSTR("product"));
            result.push_back(d);
        }
        IOObjectRelease(service);
    }
    IOObjectRelease(iterator);
    return result;
}
#endif

#ifdef __APPLE__
struct Native8192USB {
    IOUSBDeviceInterface500** device=nullptr;
    bool opened=false;
    ~Native8192USB() {
        if(device){if(opened)(*device)->USBDeviceClose(device);(*device)->Release(device);}
    }
};
bool openNative8192USB(Native8192USB& usb) {
    auto match=IOServiceMatching(kIOUSBDeviceClassName);
    if(!match)return false;
    const int values[]={RTW_8192EU_USB_VID,RTW_8192EU_USB_PID};
    const CFStringRef keys[]={CFSTR("idVendor"),CFSTR("idProduct")};
    for(unsigned i=0;i<2;i++){
        auto value=CFNumberCreate(nullptr,kCFNumberIntType,&values[i]);
        if(!value){CFRelease(match);return false;}
        CFDictionarySetValue(match,keys[i],value);CFRelease(value);
    }
    io_iterator_t iterator=IO_OBJECT_NULL;
    if(IOServiceGetMatchingServices(kIOMainPortDefault,match,&iterator)!=KERN_SUCCESS)return false;
    auto service=IOIteratorNext(iterator),second=IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    // A diagnostic must never choose the first of several identical adapters.
    if(!service||second){if(service)IOObjectRelease(service);if(second)IOObjectRelease(second);return false;}
    IOCFPlugInInterface** plugin=nullptr;SInt32 score=0;
    auto status=IOCreatePlugInInterfaceForService(service,kIOUSBDeviceUserClientTypeID,
        kIOCFPlugInInterfaceID,&plugin,&score);
    IOObjectRelease(service);
    if(status!=KERN_SUCCESS||!plugin){if(plugin)(*plugin)->Release(plugin);return false;}
    auto query=(*plugin)->QueryInterface(plugin,CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID500),
        reinterpret_cast<LPVOID*>(&usb.device));
    (*plugin)->Release(plugin);
    if(query||!usb.device)return false;
    UInt16 vendor=0,product=0;
    if((*usb.device)->GetDeviceVendor(usb.device,&vendor)!=kIOReturnSuccess||
       (*usb.device)->GetDeviceProduct(usb.device,&product)!=kIOReturnSuccess||
       vendor!=RTW_8192EU_USB_VID||product!=RTW_8192EU_USB_PID)return false;
    // Ordinary open only: never seize, reset, set configuration or detach.
    if((*usb.device)->USBDeviceOpen(usb.device)!=kIOReturnSuccess)return false;
    usb.opened=true;return true;
}
rtw_usb_error native8192ReadAdapter(void* context,const rtw_usb_setup* setup,
                                  uint8_t* data,uint16_t* actual) {
    if(actual)*actual=0;
    if(!context||!setup||!data||!actual||setup->bm_request_type!=0xc0||
       setup->b_request!=5||setup->w_index||setup->w_length!=4||
       !setup->timeout_ms||setup->timeout_ms>1000)return RTW_USB_INVALID_ARGUMENT;
    switch(setup->w_value){
    case RTW_8192EU_REG_SYS_CFG:case RTW_8192EU_REG_HPON_FSM:
    case RTW_8192EU_REG_SYS_CFG2:case RTW_8192EU_REG_MCU_FW_DL:break;
    default:return RTW_USB_INVALID_REGISTER;
    }
    auto* usb=static_cast<Native8192USB*>(context);
    if(!usb->device||!usb->opened)return RTW_USB_INVALID_ARGUMENT;
    IOUSBDevRequestTO request{};
    request.bmRequestType=setup->bm_request_type;request.bRequest=setup->b_request;
    request.wValue=setup->w_value;request.wIndex=setup->w_index;
    request.wLength=setup->w_length;request.pData=data;
    request.noDataTimeout=setup->timeout_ms;request.completionTimeout=setup->timeout_ms;
    if((*usb->device)->DeviceRequestTO(usb->device,&request)!=kIOReturnSuccess||
       request.wLenDone>setup->w_length)return RTW_USB_IO_ERROR;
    *actual=static_cast<uint16_t>(request.wLenDone);return RTW_USB_OK;
}
#endif

#if defined(RTW_WITH_LIBUSB) && !defined(__APPLE__)
rtw_usb_error libusbReadAdapter(void* context, const rtw_usb_setup* setup,
                                  uint8_t* data, uint16_t* actual) {
    if (actual) *actual=0;
    if (!context || !setup || !data || !actual ||
        setup->bm_request_type!=0xC0U || setup->b_request!=0x05U ||
        setup->w_index!=0U || setup->w_length!=4U)
        return RTW_USB_INVALID_ARGUMENT;
    const int result=libusb_control_transfer(
        static_cast<libusb_device_handle*>(context),setup->bm_request_type,
        setup->b_request,setup->w_value,setup->w_index,data,
        setup->w_length,static_cast<unsigned>(setup->timeout_ms));
    if (result<0) return RTW_USB_IO_ERROR;
    if (result>setup->w_length) return RTW_USB_IO_ERROR;
    *actual=static_cast<uint16_t>(result);
    return RTW_USB_OK;
}
#endif
int readRTL8192EUIdentity() {
#ifdef __APPLE__
    Native8192USB handle;
    if(!openNative8192USB(handle)){
        std::cerr << "Unable to open exactly one 2357:0108: absent, ambiguous, busy or inaccessible USB.\n"
                     "No driver will be detached; no USB reset or write performed.\n";
        return 3;
    }
    const rtw_usb_transport transport{&handle,&native8192ReadAdapter};
#elif !defined(RTW_WITH_LIBUSB)
    std::cerr << "libusb not found during build; install libusb and rebuild.\n";
    return 2;
#else
    libusb_context* context=nullptr;
    const int init=libusb_init(&context);
    if (init<0) {
        std::cerr << "libusb_init: " << libusb_error_name(init) << '\n';
        return 2;
    }
    libusb_device_handle* handle=libusb_open_device_with_vid_pid(
        context,RTW_8192EU_USB_VID,RTW_8192EU_USB_PID);
    if (!handle) {
        std::cerr << "Unable to open 2357:0108: missing device, permissions or busy USB.\n"
                     "No driver will be detached; no USB reset or write performed.\n";
        libusb_exit(context);
        return 3;
    }
    const rtw_usb_transport transport{handle,&libusbReadAdapter};
#endif
#if defined(__APPLE__) || defined(RTW_WITH_LIBUSB)
    rtw_8192eu_identity identity{};
    const auto state=rtw_8192eu_probe_registers(
        &transport,RTW_8192EU_USB_VID,RTW_8192EU_USB_PID,&identity);
    int code=0;
    if (state!=RTW_8192EU_OK) {
        std::cerr << "RTL8192EU read-only probe failed: "
                  << rtw_8192eu_result_string(state) << '\n';
        code=4;
    } else {
        std::cout << "USB 2357:0108 (RTL8192EU reference identity)\n"
                  << "  SYS_CFG:   " << hex32(identity.sys_cfg) << '\n'
                  << "  HPON_FSM:  " << hex32(identity.hpon_fsm) << '\n'
                  << "  SYS_CFG2:  " << hex32(identity.sys_cfg2) << '\n'
                  << "  MCU_FW_DL: " << hex32(identity.mcu_fw_dl) << '\n'
                  << "  Silicon cut: " << unsigned(identity.silicon_cut)
                  << ", foundry code: " << unsigned(identity.foundry_code) << '\n'
                  << "  Bonding: " << (identity.is_8191eu_bonding ? "8191EU (1 TX)" : "8192EU (2 TX inferred)") << '\n'
                  << "  Probe uses only USB vendor READ; not a Wi-Fi connection.\n";
    }
#ifndef __APPLE__
    libusb_close(handle);
    libusb_exit(context);
#endif
    return code;
#endif
}

int readRTL8812SYSConfig() {
#ifndef RTW_WITH_LIBUSB
    std::cerr << "libusb not found during build; install libusb and rebuild.\n";
    return 2;
#else
    libusb_context* context = nullptr;
    const int initResult = libusb_init(&context);
    if (initResult < 0) {
        std::cerr << "libusb_init: " << libusb_error_name(initResult) << '\n';
        return 2;
    }
    libusb_device_handle* device = libusb_open_device_with_vid_pid(context, 0x0BDA, 0x8812);
    if (!device) {
        std::cerr << "Cannot open 0BDA:8812 (missing, already claimed, or no USB permissions).\n";
        libusb_exit(context);
        return 3;
    }
    // Vendor control READ: direction IN / vendor / device, request 0x05,
    // wValue = RTL8812AU REG_SYS_CFG 0x00F0, wIndex = 0, length 4.
    // These constants and this transaction are cross-checked against the
    // rtl8812au-macos hardware-verified GPL-2.0 implementation.
    unsigned char raw[4] = {};
    const int size = libusb_control_transfer(device, 0xC0, 0x05,
                                             0x00F0, 0x0000, raw, 4, 750);
    int result = 0;
    if (size == 4) {
        const std::uint32_t reg = std::uint32_t(raw[0]) |
           (std::uint32_t(raw[1]) << 8) | (std::uint32_t(raw[2]) << 16) |
           (std::uint32_t(raw[3]) << 24);
        std::cout << "RTL8812AU REG_SYS_CFG [0x00f0]: " << hex32(reg) << '\n';
        if (reg == 0 || reg == 0xffffffffU)
            std::cout << "WARNING: implausible register value; device may be powered down.\n";
    } else {
        std::cerr << "Read failed";
        if (size < 0) std::cerr << ": " << libusb_error_name(size);
        else std::cerr << ": received " << size << " / 4 bytes";
        std::cerr << '\n';
        result = 4;
    }
    libusb_close(device);
    libusb_exit(context);
    return result;
#endif
}
} // namespace

int main(int argc, char** argv) {
    bool all = false, json = false, readSysCfg = false, read8192 = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--all") all = true;
        else if (a == "--json") json = true;
        else if (a == "--read-syscfg") readSysCfg = true;
        else if (a == "--read-8192eu") read8192 = true;
        else if (a == "--help" || a == "-h") {
            std::cout << "rtw-probe [--all] [--json] [--read-syscfg] [--read-8192eu]\n"
                      << "  Default: show Realtek VID 0BDA and known OEM Wi-Fi adapters.\n"
                      << "  --all: show all USB devices (helps identify OEM VID/PIDs).\n"
                      << "  --read-syscfg: read one register on 0BDA:8812; never writes.\n"
                      << "  --read-8192eu: read 4 registers on 2357:0108; never writes.\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << a << '\n';
            return 2;
        }
    }
    if (read8192 && readSysCfg) {
        std::cerr << "Choose only one register-read mode.\n";
        return 2;
    }
    if ((read8192 || readSysCfg) && json) {
        std::cerr << "--json is for inventory only; do not combine with register-read mode.\n";
        return 2;
    }
#ifdef __APPLE__
    const auto devices = listMacDevices();
    if (json) std::cout << "[\n";
    bool first = true;
    std::size_t shown = 0;
    for (const auto& d : devices) {
        const auto chip = rtw::identifyChip(d.vendor, d.product);
        if (!all && d.vendor != 0x0BDA && chip != rtw::ChipFamily::RTL8192EU_Candidate)
            continue;
        if (json) {
            if (!first) std::cout << ",\n";
            std::cout << "  {\"vendor\":" << d.vendor << ",\"product\":" << d.product
                      << ",\"location\":" << d.location << ",\"name\":" << jsonQuote(d.label)
                      << ",\"classification\":" << jsonQuote(rtw::toString(chip)) << "}";
        } else {
            std::cout << hex16(d.vendor) << ':' << hex16(d.product)
                      << "  location=" << hex32(d.location)
                      << "  " << (d.label.empty() ? "(unnamed)" : d.label)
                      << " | " << rtw::toString(chip) << '\n';
        }
        ++shown;
        first = false;
    }
    if (json) std::cout << "\n]\n";
    else if (!shown) std::cout << "No matching USB device was enumerated. Try --all.\n";
#else
    (void) all;
    (void) json;
    std::cerr << "Native USB inventory requires macOS (IOKit).\n";
    if (!readSysCfg && !read8192) return 1;
#endif
    if (read8192) return readRTL8192EUIdentity();
    if (readSysCfg) return readRTL8812SYSConfig();
    return 0;
}
