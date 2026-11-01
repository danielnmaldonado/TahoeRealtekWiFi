/* SPDX-License-Identifier: GPL-2.0-only */
/* Optional lab-only 8051 firmware stage utility. Does not load a kext,
   detach a kernel driver, calibrate RF, associate, or enable Wi-Fi TX. */
#include "rtw/LegacyFirmwareStage.h"
#include <libusb.h>
#include <chrono>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {
struct Usb {
    libusb_context* context = nullptr;
    libusb_device_handle* handle = nullptr;
    int interface_number = -1;
    ~Usb() {
        if (interface_number >= 0 && handle)
            (void)libusb_release_interface(handle, interface_number);
        if (handle) libusb_close(handle);
        if (context) libusb_exit(context);
    }
    Usb() = default;
    Usb(const Usb&) = delete;
    Usb& operator=(const Usb&) = delete;
};
void help() {
    std::puts("Usage:\n"
        "  rtw-legacy-fwdiag --device VID:PID [--firmware FILE]\n"
        "  rtw-legacy-fwdiag --device VID:PID --firmware FILE "
        "--boot --confirm-unstable-hardware\n"
        "Generic USB firmware startup is disabled for all families.\n"
        "FU/8710BU/8723BU support offline layout inspection only.\n"
        "Default is read-only metadata/firmware planning. No automatic driver detach.\n"
        "The typed kext backends have their own chipset-specific boot sequences.\n"
        "This does not provide a functional Wi-Fi station backend.");
}
bool parse_hex_device(const char* text, uint16_t& vendor, uint16_t& product) {
    if (!text || std::strlen(text) != 9 || text[4] != ':') return false;
    for (unsigned i = 0; i < 9; ++i) {
        if (i == 4) continue;
        const char c = text[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    auto hex = [](char c) -> uint16_t {
        if (c >= '0' && c <= '9') return static_cast<uint16_t>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<uint16_t>(c - 'a' + 10);
        return static_cast<uint16_t>(c - 'A' + 10);
    };
    vendor = 0; product = 0;
    for (unsigned i = 0; i < 4; ++i)
        vendor = static_cast<uint16_t>(vendor * 16U + hex(text[i]));
    for (unsigned i = 5; i < 9; ++i)
        product = static_cast<uint16_t>(product * 16U + hex(text[i]));
    return vendor != 0U && product != 0U;
}
bool make_descriptor(const libusb_config_descriptor* cfg,
                     std::vector<uint8_t>& bytes) {
    bytes.clear();
    if (!cfg || !cfg->interface || cfg->bNumInterfaces == 0 ||
        cfg->bConfigurationValue == 0)
        return false;
    bytes = {9, 2, 0, 0, cfg->bNumInterfaces,
             cfg->bConfigurationValue, 0, cfg->bmAttributes, cfg->MaxPower};
    for (unsigned i = 0; i < cfg->bNumInterfaces; ++i) {
        const auto& iface = cfg->interface[i];
        if (iface.num_altsetting <= 0 || !iface.altsetting) return false;
        for (int altidx = 0; altidx < iface.num_altsetting; ++altidx) {
            const auto& alt = iface.altsetting[altidx];
            if ((alt.bNumEndpoints > 0 && !alt.endpoint) ||
                bytes.size() > 65535U - 9U) return false;
            bytes.insert(bytes.end(), {
                9, 4, alt.bInterfaceNumber, alt.bAlternateSetting,
                alt.bNumEndpoints, alt.bInterfaceClass,
                alt.bInterfaceSubClass, alt.bInterfaceProtocol, 0
            });
            for (unsigned e = 0; e < alt.bNumEndpoints; ++e) {
                const auto& ep = alt.endpoint[e];
                if (bytes.size() > 65535U - 7U) return false;
                bytes.insert(bytes.end(), {
                    7, 5, ep.bEndpointAddress, ep.bmAttributes,
                    static_cast<uint8_t>(ep.wMaxPacketSize),
                    static_cast<uint8_t>(ep.wMaxPacketSize >> 8U),
                    ep.bInterval
                });
            }
        }
    }
    if (bytes.size() > 65535U) return false;
    bytes[2] = static_cast<uint8_t>(bytes.size());
    bytes[3] = static_cast<uint8_t>(bytes.size() >> 8U);
    return true;
}
rtw_usb_error read_control(void* context, const rtw_usb_setup* request,
                           uint8_t* buffer, uint16_t* actual) {
    auto& usb = *static_cast<Usb*>(context);
    if (actual) *actual = 0;
    if (!request || !actual || !buffer || !usb.handle ||
        request->bm_request_type != 0xc0U || request->b_request != 5 ||
        (request->w_length != 1 && request->w_length != 2 &&
         request->w_length != 4))
        return RTW_USB_INVALID_ARGUMENT;
    const int result = libusb_control_transfer(
        usb.handle, request->bm_request_type, request->b_request,
        request->w_value, request->w_index, buffer,
        request->w_length, request->timeout_ms);
    if (result < 0 || result > request->w_length) return RTW_USB_IO_ERROR;
    *actual = static_cast<uint16_t>(result);
    return RTW_USB_OK;
}
rtw_usb_error write_control(void* context, const rtw_usb_setup* request,
                            const uint8_t* data, uint16_t* actual) {
    auto& usb = *static_cast<Usb*>(context);
    if (actual) *actual = 0;
    if (!request || !actual || !data || !usb.handle ||
        request->bm_request_type != 0x40U || request->b_request != 5 ||
        !request->w_length ||
        request->w_length > RTW_USB_MAX_VENDOR_PAYLOAD)
        return RTW_USB_INVALID_ARGUMENT;
    uint8_t copy[RTW_USB_MAX_VENDOR_PAYLOAD];
    std::memcpy(copy, data, request->w_length);
    const int result = libusb_control_transfer(
        usb.handle, request->bm_request_type, request->b_request,
        request->w_value, request->w_index, copy, request->w_length,
        request->timeout_ms);
    if (result < 0 || result > request->w_length) return RTW_USB_IO_ERROR;
    *actual = static_cast<uint16_t>(result);
    return RTW_USB_OK;
}
int wait_us(void*, uint32_t micros) {
    if (!micros || micros > 10000U) return 0;
    std::this_thread::sleep_for(std::chrono::microseconds(micros));
    return 1;
}
int count_chunk(void* context, uint8_t, uint16_t,
                const uint8_t*, uint16_t count) {
    auto* sum = static_cast<size_t*>(context);
    *sum += count;
    return 1;
}
bool load_firmware(const std::string& path, std::vector<uint8_t>& data) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) return false;
    const auto count = input.tellg();
    if (count < 33 || count > 32 + 32768) return false;
    data.resize(static_cast<size_t>(count));
    input.seekg(0);
    return static_cast<bool>(input.read(
        reinterpret_cast<char*>(data.data()), count));
}
}
int main(int argc, char** argv) {
    bool boot = false, confirm = false;
    const char* device_arg = nullptr;
    std::string firmware_file;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--help") || !std::strcmp(argv[i], "-h")) {
            help(); return 0;
        }
        if (!std::strcmp(argv[i], "--device") && i + 1 < argc) {
            if (device_arg) { help(); return 2; }
            device_arg = argv[++i]; continue;
        }
        if (!std::strcmp(argv[i], "--firmware") && i + 1 < argc) {
            if (!firmware_file.empty()) { help(); return 2; }
            firmware_file = argv[++i]; continue;
        }
        if (!std::strcmp(argv[i], "--boot")) { boot = true; continue; }
        if (!std::strcmp(argv[i], "--confirm-unstable-hardware")) {
            confirm = true; continue;
        }
        std::fprintf(stderr, "Invalid option: %s\n", argv[i]);
        help(); return 2;
    }
    if (!device_arg || (boot && (!confirm || firmware_file.empty())) ||
        (confirm && !boot)) {
        help(); return 2;
    }
    uint16_t vendor = 0, product = 0;
    if (!parse_hex_device(device_arg, vendor, product)) {
        std::fputs("Device must be four hex digits VID:PID.\n", stderr);
        return 2;
    }
    std::vector<uint8_t> file;
    if (!firmware_file.empty() && !load_firmware(firmware_file, file)) {
        std::fputs("Firmware unreadable or exceeds 8-page limit.\n", stderr);
        return 2;
    }
    Usb usb;
    if (libusb_init(&usb.context) != LIBUSB_SUCCESS) {
        std::fputs("Unable to initialize libusb.\n", stderr); return 3;
    }
    usb.handle = libusb_open_device_with_vid_pid(usb.context, vendor, product);
    if (!usb.handle) {
        std::fputs("Requested USB adapter not found or inaccessible.\n", stderr);
        return 3;
    }
    libusb_device_descriptor device{};
    if (libusb_get_device_descriptor(libusb_get_device(usb.handle), &device) !=
        LIBUSB_SUCCESS) return 3;
    libusb_config_descriptor* config = nullptr;
    if (libusb_get_active_config_descriptor(
            libusb_get_device(usb.handle), &config) != LIBUSB_SUCCESS ||
        !config) {
        std::fputs("Unable to inspect active USB configuration.\n", stderr);
        return 3;
    }
    std::vector<uint8_t> descriptors;
    const bool formatted = make_descriptor(config, descriptors);
    libusb_free_config_descriptor(config);
    if (!formatted) {
        std::fputs("USB descriptors malformed or too large.\n", stderr);
        return 3;
    }
    rtw_backend_usb_plan usb_plan{};
    const auto state = rtw_backend_usb_preflight(
        vendor, product, device.bcdDevice,
        descriptors.data(), descriptors.size(), 0, &usb_plan);
    std::printf("USB %04x:%04x: %s\n", vendor, product,
                rtw_backend_preflight_status_name(state));
    if (state != RTW_BACKEND_PREFLIGHT_DIAGNOSTICS || !usb_plan.backend) {
        std::fputs("Refusing firmware path without verified diagnostic profile.\n", stderr);
        return 3;
    }
    std::printf("Chip family: %s\n",
                rtw_usb_wifi_family_name(usb_plan.backend->family));
    if (file.empty()) {
        std::puts("Read-only inspection complete; no USB control writes performed.");
        return 0;
    }
    rtw_legacy_fw_plan plan{};
    const auto prepared = rtw_legacy_fw_prepare(
        &usb_plan, file.data(), file.size(), &plan);
    if (prepared != RTW_LEGACY_FW_OK) {
        std::fprintf(stderr, "Firmware rejected: %s\n",
                     rtw_legacy_fw_status_name(prepared));
        return 3;
    }
    size_t bytes = 0, transfers = 0;
    if (rtw_legacy_fw_walk(&plan, &count_chunk, &bytes, &transfers) !=
        RTW_LEGACY_FW_OK)
        return 3;
    std::printf("Firmware layout: %zu bytes, %u pages, %zu vendor writes, "
                "window 0x%04x, MCU 0x%04x\n",
                bytes, plan.pages, transfers,
                plan.firmware_window_register, plan.mcu_download_register);
    if (!boot) {
        std::puts("Dry run complete; no USB control writes performed.");
        return 0;
    }
    if (!plan.boot_experimental_available) {
        std::fputs("Firmware boot not implemented for this chip family.\n", stderr);
        return 3;
    }
    /* Unlike libusb_set_auto_detach_kernel_driver, claiming must fail when
       an existing kernel driver owns the adapter. No forced detach. */
    if (libusb_claim_interface(usb.handle, usb_plan.profile->interface_number) != LIBUSB_SUCCESS) {
        std::fputs("USB interface busy or inaccessible; no driver detached.\n", stderr);
        return 3;
    }
    usb.interface_number = usb_plan.profile->interface_number;
    rtw_usb_rw_transport transport{&usb, &read_control, &write_control};
    rtw_waiter waiter{nullptr, &wait_us};
    rtw_legacy_fw_options options{1, 1, 200, 100};
    rtw_legacy_fw_report report{};
    std::fputs("EXPERIMENTAL: writes may require power cycling the adapter.\n", stderr);
    const auto result = rtw_legacy_fw_boot_experimental(
        &plan, &transport, &waiter, &options, &report);
    std::printf("Firmware upload: %s; bytes=%zu writes=%zu "
                "checksum=%u 8051_ready=%u download_disabled=%u\n",
                rtw_legacy_fw_status_name(result), report.bytes_written,
                report.vendor_writes, report.checksum_seen,
                report.firmware_ready, report.download_disabled);
    return result == RTW_LEGACY_FW_OK ? 0 : 4;
}
