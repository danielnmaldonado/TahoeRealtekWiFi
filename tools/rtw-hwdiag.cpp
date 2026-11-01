/* SPDX-License-Identifier: GPL-2.0-only */
// Optional, explicit RTL8812AU USB diagnostic for a locally attached device.
// Build only if libusb development headers are available. NO SIP changes,
// NO kext installation. --boot-pre-rf is a dangerous, manual development
// operation: the adapter may require PHYSICAL unplugging afterwards.
#include "rtw/HAL8812.h"
#include "rtw/RXMonitor.h"
#include "rtw/RFReadback.h"
#include "rtw/RFCalibration.h"
#include "rtw/RTL8812AUPassive.h"
#include "rtw/Chip.h"
#include "rtw/Firmware.h"
#include <libusb.h>
#include <chrono>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace {
struct Usb {
    libusb_context* ctx=nullptr;
    libusb_device_handle* device=nullptr;
    bool claimed=false;
    ~Usb(){
        if(claimed && device) (void)libusb_release_interface(device,0);
        if(device) libusb_close(device);
        if(ctx) libusb_exit(ctx);
    }
    Usb(const Usb&)=delete;Usb& operator=(const Usb&)=delete;
    Usb()=default;
};
rtw_usb_error in(void* p,const rtw_usb_setup* req,uint8_t* bytes,uint16_t* actual){
    auto& state=*static_cast<Usb*>(p);
    if(actual) *actual=0;
    if(!state.device || !req || !bytes || !actual ||
       req->bm_request_type!=0xc0 || req->b_request!=5 ||
       !(req->w_length==1 || req->w_length==2 || req->w_length==4))
        return RTW_USB_INVALID_ARGUMENT;
    int ret=libusb_control_transfer(state.device,req->bm_request_type,req->b_request,
                      req->w_value,req->w_index,bytes,req->w_length,req->timeout_ms);
    if(ret<0 || ret>req->w_length) return RTW_USB_IO_ERROR;
    *actual=static_cast<uint16_t>(ret);
    return RTW_USB_OK;
}
rtw_usb_error out(void* p,const rtw_usb_setup* req,const uint8_t* bytes,uint16_t* actual){
    auto& state=*static_cast<Usb*>(p);
    if(actual) *actual=0;
    if(!state.device || !req || !bytes || !actual ||
       req->bm_request_type!=0x40 || req->b_request!=5 ||
       !req->w_length || req->w_length>RTW_USB_MAX_VENDOR_PAYLOAD)
        return RTW_USB_INVALID_ARGUMENT;
    uint8_t staging[RTW_USB_MAX_VENDOR_PAYLOAD];
    std::memcpy(staging,bytes,req->w_length);
    int ret=libusb_control_transfer(state.device,req->bm_request_type,req->b_request,
                      req->w_value,req->w_index,staging,req->w_length,req->timeout_ms);
    if(ret<0 || ret>req->w_length) return RTW_USB_IO_ERROR;
    *actual=static_cast<uint16_t>(ret);
    return RTW_USB_OK;
}
int wait_us(void*,uint32_t time){
    if(!time || time>10000) return 0;
    std::this_thread::sleep_for(std::chrono::microseconds(time));
    return 1;
}
struct ScanContext {
    std::set<std::array<uint8_t,6>> seen;
    unsigned parsed=0;
    unsigned invalid=0;
};
int found_beacon(void* ctx, const rtw_r12au_ap* ap,
                 const rtw_r12au_rx_frame*) {
    auto& scan=*static_cast<ScanContext*>(ctx);
    if (!ap) return 0;
    ++scan.parsed;
    std::array<uint8_t,6> address{};
    std::memcpy(address.data(),ap->bssid,address.size());
    if(scan.seen.size()>=64U || !scan.seen.insert(address).second)return 1;
    const char *security=ap->security==RTW_R12AU_SECURITY_OPEN
        ? "open" : ap->security==RTW_R12AU_SECURITY_WPA2_PSK_CCMP
        ? "WPA2-PSK/CCMP (handshake NOT implemented)" : "unsupported";
    std::printf("BSSID %02x:%02x:%02x:%02x:%02x:%02x channel %u security %s SSID ",
                ap->bssid[0],ap->bssid[1],ap->bssid[2],
                ap->bssid[3],ap->bssid[4],ap->bssid[5],
                static_cast<unsigned>(ap->primary_channel),security);
    for(unsigned i=0;i<ap->ssid_length;++i) {
        const uint8_t byte=ap->ssid[i];
        if(byte>=32U && byte<=126U && byte!='\\' && byte!='"')std::putchar(byte);
        else std::printf("\\x%02x",static_cast<unsigned>(byte));
    }
    std::putchar('\n');
    return 1;
}
void help(){
    std::puts("rtw-hwdiag [--probe]\n"
              "rtw-hwdiag --boot-pre-rf --firmware PATH --confirm-unstable-hardware\n"
              "rtw-hwdiag --passive-scan-6 --firmware PATH --confirm-unstable-hardware [--seconds 1..30]\n"
              "Only 0BDA:8812 B-cut/TSMC/USB/RFE3. Scan listens on 2.4GHz channel 6 only.\n"
              "No SSID association, WPA2/CCMP, packet injection, or Ethernet interface.\n"
              "WARNING: scan and boot modes program USB/RF registers and firmware.\n"
              "Do NOT run with another Realtek driver attached; physical replug may be necessary.");
}
}
int main(int argc,char** argv){
    bool boot=false,confirm=false,scan=false,secondsProvided=false;
    int seconds=8;
    std::string path;
    for(int i=1;i<argc;++i){
        if(std::strcmp(argv[i],"--help")==0 || std::strcmp(argv[i],"-h")==0){help();return 0;}
        if(std::strcmp(argv[i],"--probe")==0) continue;
        if(std::strcmp(argv[i],"--boot-pre-rf")==0){if(scan){help();return 2;}boot=true;continue;}
        if(std::strcmp(argv[i],"--passive-scan-6")==0){if(boot && !scan){help();return 2;}boot=scan=true;continue;}
        if(std::strcmp(argv[i],"--seconds")==0 && i+1<argc){
            char* end=nullptr;
            errno=0;
            long number=std::strtol(argv[++i],&end,10);
            if(errno!=0 || !end || *end || number<1 || number>30){
                std::fputs("--seconds expects an integer in 1..30\n",stderr);return 2;
            }
            seconds=static_cast<int>(number);secondsProvided=true;continue;
        }
        if(std::strcmp(argv[i],"--confirm-unstable-hardware")==0){confirm=true;continue;}
        if(std::strcmp(argv[i],"--firmware")==0 && i+1<argc){path=argv[++i];continue;}
        std::fprintf(stderr,"unknown or incomplete argument: %s\n",argv[i]);help();return 2;
    }
    if(boot && (!confirm || path.empty())) {
        std::fputs("Refusing write access: pass --firmware and --confirm-unstable-hardware.\n",stderr);
        return 2;
    }
    if(!scan && secondsProvided){
        std::fputs("--seconds is valid only with --passive-scan-6.\n",stderr);
        return 2;
    }
    if(!boot && (!path.empty() || confirm)){
        std::fputs("--firmware/--confirm are valid only with --boot-pre-rf.\n",stderr);
        return 2;
    }
    std::vector<uint8_t> firmware;
    if(boot) {
        std::ifstream source(path,std::ios::binary);
        if(!source) {std::fprintf(stderr,"cannot read firmware: %s\n",path.c_str());return 2;}
        source.seekg(0,std::ios::end);
        const auto size=source.tellg();
        if(size<=32 || size>32+32768) {
            std::fputs("Firmware must contain 33..32800 bytes.\n",stderr);return 2;
        }
        source.seekg(0,std::ios::beg);
        firmware.resize(static_cast<size_t>(size));
        if(!source.read(reinterpret_cast<char*>(firmware.data()),size)) {
            std::fputs("firmware read failed\n",stderr);return 2;
        }
        rtw_fw_image header{};
        if(rtw_fw_inspect_8812au(firmware.data(),firmware.size(),&header)!=RTW_FW_OK) {
            std::fputs("invalid RTL8812AU firmware image\n",stderr);return 2;
        }
        std::fprintf(stderr,"WARNING: experimental hardware writes enabled. Firmware %u.%u, %zu bytes.\n",
                     static_cast<unsigned>(header.version),static_cast<unsigned>(header.subversion),header.payload_length);
    }
    Usb usb;
    int ret=libusb_init(&usb.ctx);
    if(ret!=LIBUSB_SUCCESS){std::fprintf(stderr,"libusb_init: %s\n",libusb_error_name(ret));return 3;}
    usb.device=libusb_open_device_with_vid_pid(usb.ctx,0x0BDA,0x8812);
    if(!usb.device) {
        std::fputs("RTL8812AU (0BDA:8812) not found / inaccessible.\n",stderr);return 3;
    }
    /* Validate the real USB endpoint topology before trusting the HAL's
       three-output-pipe MAC queue map. Never assume an adapter variant. */
    libusb_config_descriptor* conf=nullptr;
    ret=libusb_get_active_config_descriptor(libusb_get_device(usb.device),&conf);
    if(ret!=LIBUSB_SUCCESS || !conf){
        std::fprintf(stderr,"cannot read USB configuration: %s\n",libusb_error_name(ret));return 3;
    }
    unsigned bulkIn=0,bulkOut=0;
    uint8_t bulkInEndpoint=0;
    uint32_t endpointMask=0;
    bool validProfile=false;
    for(unsigned i=0;i<conf->bNumInterfaces;++i){
        const auto& interface=conf->interface[i];
        for(int j=0;j<interface.num_altsetting;++j){
            const auto& alt=interface.altsetting[j];
            if(alt.bInterfaceNumber!=0 || alt.bAlternateSetting!=0)continue;
            validProfile=true;
            for(unsigned e=0;e<alt.bNumEndpoints;++e){
                const auto& ep=alt.endpoint[e];
                if((ep.bmAttributes & 3U)!=LIBUSB_TRANSFER_TYPE_BULK)continue;
                if((ep.bEndpointAddress & 0x70U) || !(ep.bEndpointAddress & 0x0fU)){
                    validProfile=false;break;
                }
                const unsigned endpoint=ep.bEndpointAddress & 0x0fU;
                const unsigned shift=endpoint+(ep.bEndpointAddress&0x80U?16U:0U);
                if(endpointMask&(1U<<shift)){validProfile=false;break;}
                endpointMask|=1U<<shift;
                if(ep.bEndpointAddress & 0x80U) {
                    ++bulkIn;
                    if(!bulkInEndpoint)bulkInEndpoint=ep.bEndpointAddress;
                } else ++bulkOut;
            }
        }
    }
    libusb_free_config_descriptor(conf);
    if(!validProfile || !bulkIn || !bulkOut){
        std::fputs("invalid USB bulk endpoint topology\n",stderr);return 3;
    }
    std::printf("USB bulk endpoints: IN=%u OUT=%u\n",bulkIn,bulkOut);
    if(scan && (bulkIn!=1 || bulkOut!=3 || !bulkInEndpoint)) {
        std::fputs("Refusing passive capture: only a 1 bulk-IN, 3 bulk-OUT profile is supported.\n",stderr);
        return 3;
    }
    if(boot && bulkOut!=3){
        std::fputs("Unsupported USB endpoint profile for pre-RF writes; expected exactly 3 BULK OUT endpoints.\n",stderr);
        return 3;
    }
    /* Never forcibly detach another kernel driver. */
    ret=libusb_claim_interface(usb.device,0);
    if(ret!=LIBUSB_SUCCESS){
        std::fprintf(stderr,"USB interface busy/unavailable: %s. No detach performed.\n",
                     libusb_error_name(ret));return 3;
    }
    usb.claimed=true;
    auto bus=rtw_usb_rw_transport{&usb,&in,&out};
    uint32_t cfg=0;
    if(rtw_usb_rw_read(&bus,0x00f0,4,&cfg)!=RTW_USB_OK) {
        std::fputs("USB SYS_CFG register read failed.\n",stderr);return 4;
    }
    rtw_chip_info chip{};
    if(!rtw_chip_decode_syscfg(cfg,&chip)){
        std::fputs("SYS_CFG was invalid.\n",stderr);return 4;
    }
    std::printf("SYS_CFG=0x%08x cut=%u USB=%s TSMC=%s test=%s\n",
                static_cast<unsigned>(cfg),static_cast<unsigned>(chip.cut),
                chip.is_usb?"yes":"no",chip.is_umc?"no":"yes",chip.is_test_chip?"yes":"no");
    if(!boot){std::puts("Read-only probe complete; no registers written.");return 0;}
    auto timer=rtw_waiter{nullptr,&wait_us};
    rtw_hal_environment env{&bus,&timer,firmware.data(),firmware.size(),3,1};
    rtw_hal_report status{};
    const auto result=rtw_hal_boot_pre_rf_8812au(&env,&status);
    std::printf("pre-RF result: %s (%u), last successful stage=%u, failed stage=%u\n",
                rtw_hal_error_string(result),static_cast<unsigned>(result),
                static_cast<unsigned>(status.last_success),static_cast<unsigned>(status.failed_at));
    std::printf("Power steps=%zu, LLT entries=%zu, firmware bytes=%zu, MAC registers=%zu\n",
                status.power_steps,status.llt_entries,status.firmware.bytes_written,status.mac_registers);
    if(result!=RTW_HAL_OK) {
        std::puts("Hardware initialization incomplete. No capture or network interface.");
        return 5;
    }
    if(!scan){
        std::puts("Pre-RF stage finished. Radio and Ethernet still disabled.");
        return 0;
    }
    rtw_monitor_report mon{};
    const auto configure=rtw_monitor_prepare_channel6(&bus,&timer,&status,1,&mon);
    std::printf("passive monitor: %s, table commands=%zu, RX writes=%zu\n",
                rtw_monitor_error_string(configure),mon.phy.executed,
                mon.rx_register_operations);
    if(configure!=RTW_MONITOR_OK || !mon.monitor_registers_written) {
        std::fputs("Monitor register configuration failed; do not expect a working adapter.\n",stderr);
        if(mon.rx_register_operations || mon.monitor_registers_written) {
            const auto stopped=rtw_monitor_quiesce_rx_channel6(&bus,&mon);
            if(stopped!=RTW_MONITOR_OK)
                std::fprintf(stderr,"WARNING: MAC RX shutdown incomplete: %s. Unplug USB.\n",
                             rtw_monitor_error_string(stopped));
        }
        return 6;
    }
    const auto stopRx = [&]() -> bool {
        const auto stopped=rtw_monitor_quiesce_rx_channel6(&bus,&mon);
        if(stopped!=RTW_MONITOR_OK) {
            std::fprintf(stderr,
                "WARNING: MAC receive shutdown failed: %s. Physically unplug USB.\n",
                rtw_monitor_error_string(stopped));
            return false;
        }
        return true;
    };
    const auto verified=rtw_monitor_verify_channel6(&bus,&mon);
    if(verified!=RTW_MONITOR_OK) {
        std::fprintf(stderr,"Refusing passive receive; register verification: %s\n",
                     rtw_monitor_error_string(verified));
        (void)stopRx();
        return 6;
    }
    rtw_rf_readback_report rf{};
    const auto rfStatus=rtw_rf_readback_channel6_8812au(&bus,&timer,&mon,1,&rf);
    std::printf("RF[0x18] channel readback: A=0x%05x/0x%05x (%s), "
                "B=0x%05x/0x%05x (%s), samples=%zu, "
                "BB selector restored=%s\n",
                static_cast<unsigned>(rf.rf18_a_first),
                static_cast<unsigned>(rf.rf18_a_second),
                rf.path_a_confirmed?"ch6":"unverified",
                static_cast<unsigned>(rf.rf18_b_first),
                static_cast<unsigned>(rf.rf18_b_second),
                rf.path_b_confirmed?"ch6":"inconclusive",
                rf.successful_samples,
                rf.bb_registers_restored?"yes":"NO");
    if(rfStatus!=RTW_RF_READBACK_OK) {
        std::fprintf(stderr,"Refusing passive receive; RF verification: %s\n",
                     rtw_rf_readback_error_string(rfStatus));
        (void)stopRx();
        return 6;
    }
    if(rf.path_b_inconclusive)
        std::puts("RF path B readback inconclusive; this does not establish B-chain failure.");
    rtw_rf_cal_report thermal{};
    const auto thermalResult=rtw_rf_thermal_preflight_8812au(
        &bus,&timer,&status,&mon,&rf,1,&thermal);
    std::printf("RF[0x42] thermal: samples=%zu, codes=%u/%u, factory=%s",
                thermal.samples_completed,
                static_cast<unsigned>(thermal.first_code),
                static_cast<unsigned>(thermal.second_code),
                thermal.baseline_valid?"available":"unavailable");
    if(thermal.baseline_valid)
        std::printf(" (%u), drift=%u codes, IQK threshold exceeded=%s",
                    static_cast<unsigned>(thermal.factory_code),
                    static_cast<unsigned>(thermal.drift_codes),
                    thermal.delta_exceeds_iqk_threshold?"yes":"no");
    std::printf("; %s; selector restored=%s\n",
                rtw_rf_cal_error_string(thermalResult),
                thermal.bb_registers_restored?"yes":"NO");
    /* Thermal measurement is advisory for passive RX. Never claim that
     * this executes IQK or that a low temperature delta means calibrated. */
    if(thermalResult==RTW_RF_CAL_RESTORE_FAILED ||
       thermalResult==RTW_RF_CAL_USB_ERROR ||
       thermalResult==RTW_RF_CAL_DELAY_FAILED ||
       thermalResult==RTW_RF_CAL_PROFILE_UNVERIFIED) {
        std::fputs("Refusing passive receive: RF diagnostic state is uncertain.\n",stderr);
        (void)stopRx();
        return 6;
    }
    std::puts("Temperature is an RF hardware code, not degrees Celsius. "
              "IQK/LCK are NOT executed; no TX authorization.");
    std::puts("MAC/BB and RF path A channel-6 checks passed (RF calibration remains unverified).");
    std::puts("WARNING: passive laboratory receive on channel 6 ONLY. No TX packets are sent.");
    const auto until=std::chrono::steady_clock::now()+std::chrono::seconds(seconds);
    ScanContext networks{};
    rtw_r12au_passive_stats stats{};
    std::array<unsigned char,32768> buffer{};
    unsigned usbErrors=0, usbTransfers=0;
    bool transferLengthInvalid=false;
    while(std::chrono::steady_clock::now()<until){
        int actual=0;
        const int rx=libusb_bulk_transfer(usb.device,bulkInEndpoint,
                       buffer.data(),static_cast<int>(buffer.size()),&actual,250);
        if(rx==LIBUSB_ERROR_TIMEOUT)continue;
        if(rx!=LIBUSB_SUCCESS){
            std::fprintf(stderr,"bulk receive error: %s\n",libusb_error_name(rx));
            if(++usbErrors>=3U)break;
            continue;
        }
        ++usbTransfers;
        if(actual<0 || static_cast<size_t>(actual)>buffer.size()){
            std::fputs("invalid USB transfer length\n",stderr);
            transferLengthInvalid=true;
            break; /* Always quiesce MAC RX before returning. */
        }
        if(actual==0)continue;
        const auto decoded=rtw_r12au_passive_channel6(buffer.data(),
              static_cast<size_t>(actual),found_beacon,&networks,&stats);
        if(decoded!=RTW_R12AU_PASSIVE_OK) {
            std::fprintf(stderr,"dropped USB RX aggregate: %s\n",
                         rtw_r12au_passive_result_string(decoded));
        }
    }
    std::printf("USB transfers=%u; MPDUs=%zu; beacons/probes=%zu; "
                "CRC drops=%zu; ICV drops=%zu; FCS drops=%zu; C2H=%zu; "
                "channel drops=%zu; unsupported APs=%zu; invalid APs=%zu; "
                "non-beacons=%zu; malformed USB aggregates=%zu; "
                "distinct BSSIDs=%zu; parsed=%u\n",
                usbTransfers,stats.frames,stats.accepted_beacons,
                stats.crc_drops,stats.icv_drops,stats.fcs_drops,stats.c2h,
                stats.wrong_channel,stats.unsupported,stats.invalid_beacons,
                stats.non_beacons,stats.malformed_aggregates,
                networks.seen.size(),networks.parsed);
    const bool stopped=stopRx();
    std::puts("No macOS Ethernet interface was published. RF remains powered; "
              "unplug/replug USB before regular use.");
    if(!stopped)return 8;
    if(transferLengthInvalid)return 6;
    return usbErrors>=3U || !usbTransfers ? 7 : 0;
}
