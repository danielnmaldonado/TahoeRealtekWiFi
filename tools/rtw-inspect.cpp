// Offline-only diagnostic for RTL8812AU. Never opens or writes a USB device.
#include "rtw/Chip.h"
#include "rtw/Efuse.h"
#include "rtw/Firmware.h"
#include "rtw/USBPackets.h"
#include "rtw/RTL8192EU.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> load(const std::string& path, std::size_t max) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("cannot open binary file");
    const auto raw=file.tellg();
    if(raw<0 || static_cast<std::uintmax_t>(raw)>max)
        throw std::runtime_error("file exceeds permitted size");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(raw));
    file.seekg(0, std::ios::beg);
    if (!bytes.empty() &&
        !file.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("file could not be read completely");
    return bytes;
}
void showMac(const std::uint8_t mac[6]) {
    std::cout << std::hex << std::setfill('0');
    for(int i=0;i<6;++i) {
        if(i) std::cout << ':';
        std::cout << std::setw(2) << static_cast<unsigned>(mac[i]);
    }
    std::cout << std::dec << '\n';
}
int efuse(const std::string& path) {
    const auto input=load(path,RTW_EFUSE_BYTES);
    std::array<std::uint8_t,RTW_EFUSE_BYTES> map{};
    std::size_t words=0;
    const auto err=rtw_efuse_decode(input.data(),input.size(),
                                     map.data(),map.size(),&words);
    if (err!=RTW_EFUSE_OK) {
        std::cerr << "EFUSE: " << rtw_efuse_error_string(err) << '\n';
        return 1;
    }
    rtw_efuse_identity id{};
    const auto ie=rtw_efuse_extract_identity(map.data(),map.size(),&id);
    std::cout << "EFUSE decoded words: " << words << '\n';
    if (ie!=RTW_EFUSE_OK) {
        std::cerr << "EFUSE identity: " << rtw_efuse_error_string(ie) << '\n';
        return 1;
    }
    std::cout << "EEPROM VID:PID: " << std::hex << std::setw(4)
              << std::setfill('0') << id.eeprom_vendor << ':'
              << std::setw(4) << id.eeprom_product << std::dec << '\n';
    std::cout << "Station MAC: ";showMac(id.mac);
    std::cout << "RFE type: " << static_cast<unsigned>(id.rfe_type)
              << ", board option: " << static_cast<unsigned>(id.rf_board_option)
              << ", channel plan: " << static_cast<unsigned>(id.channel_plan) << '\n';
    return 0;
}
int firmware(const std::string& path) {
    const auto data=load(path,RTW_FW_PAGE_SIZE*RTW_FW_MAX_PAGES+RTW_FW_HEADER_SIZE);
    rtw_fw_image info{};
    const auto err=rtw_fw_inspect_8812au(data.data(),data.size(),&info);
    if (err!=RTW_FW_OK) {
        std::cerr << "Firmware: " << rtw_fw_error_string(err) << '\n';
        return 1;
    }
    std::cout << "RTL8812AU firmware header: signature=0x"
              << std::hex << std::setfill('0') << std::setw(4)
              << info.signature << std::dec << ", version="
              << info.version << '.' << static_cast<unsigned>(info.subversion)
              << ", payload=" << info.payload_length
              << " bytes, pages=" << static_cast<unsigned>(info.pages)
              << " (no firmware was uploaded)\n";
    return 0;
}
int rx(const std::string& path) {
    const auto data=load(path,65536);
    rtw_rx_iterator it{};
    rtw_rx_iterator_init(&it,data.data(),data.size());
    rtw_rx_packet packet{};
    std::size_t crc=0,icv=0;
    while (true) {
        const auto status=rtw_rx_next(&it,&packet);
        if(status==RTW_PACKETS_DONE) break;
        if(status!=RTW_PACKETS_OK) {
            std::cerr << "USB RX at offset " << it.cursor << ": "
                      << rtw_packet_error_string(status) << '\n';
            return 1;
        }
        crc+=packet.crc_error;icv+=packet.icv_error;
        if(it.frames<=20)
            std::cout << "RX #" << it.frames << ": mpdu=" << packet.mpdu_length
                      << " rate-index=" << static_cast<unsigned>(packet.rate_index)
                      << " CRC-bad=" << static_cast<unsigned>(packet.crc_error)
                      << " ICV-bad=" << static_cast<unsigned>(packet.icv_error) << '\n';
    }
    std::cout << "RX frames=" << it.frames << " CRC-bad=" << crc
              << " ICV-bad=" << icv << " (headers only; no frames forwarded)\n";
    return 0;
}
int efuse8192(const std::string& path) {
    const auto bytes=load(path,RTW_8192EU_EFUSE_BYTES);
    rtw_8192eu_efuse_identity id{};
    const auto error=rtw_8192eu_parse_physical_efuse(
        bytes.data(),bytes.size(),&id);
    if (error!=RTW_8192EU_OK) {
        std::cerr << "RTL8192EU efuse: " << rtw_8192eu_result_string(error) << '\n';
        return 1;
    }
    std::cout << "RTL8192EU efuse: RTL ID=0x" << std::hex << id.rtl_id
              << std::dec << ", MAC: ";showMac(id.mac);
    std::cout << "Channel plan: " << unsigned(id.channel_plan)
              << ", crystal cap: " << unsigned(id.crystal_cap)
              << ", thermal meter: " << unsigned(id.thermal_meter)
              << ", board: " << unsigned(id.rf_board_option) << '\n';
    return 0;
}
int firmware8192(const std::string& path) {
    const auto bytes=load(path,RTW_8192EU_FW_HEADER_BYTES+RTW_8192EU_FW_MAX_PAYLOAD);
    rtw_8192eu_fw_info fw{};
    const auto error=rtw_8192eu_inspect_firmware(bytes.data(),bytes.size(),&fw);
    if (error!=RTW_8192EU_OK) {
        std::cerr << "RTL8192EU firmware: " << rtw_8192eu_result_string(error) << '\n';
        return 1;
    }
    std::cout << "RTL8192EU firmware signature=0x" << std::hex
              << fw.signature << std::dec << ", version "
              << fw.major_version << '.' << unsigned(fw.minor_version)
              << ", size=" << fw.payload_length << ", declared RAM="
              << fw.reported_ramcode_size << ", SVN=" << fw.svn_index
              << " (not uploaded)\n";
    return 0;
}
int rx8192(const std::string& path) {
    const auto bytes=load(path,65536U);
    rtw_rx_iterator it{};
    rtw_rx_iterator_init(&it,bytes.data(),bytes.size());
    rtw_8192eu_rx_frame frame{};
    size_t c2h=0,crc=0,icv=0;
    while (true) {
        const auto state=rtw_8192eu_rx_next(&it,&frame);
        if (state==RTW_PACKETS_DONE) break;
        if (state!=RTW_PACKETS_OK) {
            std::cerr << "RTL8192EU RX at offset " << it.cursor << ": "
                      << rtw_packet_error_string(state) << '\n';
            return 1;
        }
        c2h+=frame.report_is_c2h;crc+=frame.payload.crc_error;
        icv+=frame.payload.icv_error;
        if (it.frames<=20) {
            std::cout << "RX #" << it.frames << " size=" << frame.payload.mpdu_length
                      << " C2H=" << unsigned(frame.report_is_c2h)
                      << " CRC=" << unsigned(frame.payload.crc_error)
                      << " ICV=" << unsigned(frame.payload.icv_error)
                      << " security=" << unsigned(frame.security_code) << '\n';
        }
    }
    std::cout << "RTL8192EU RX total=" << it.frames << " C2H=" << c2h
              << " CRC=" << crc << " ICV=" << icv
              << " (offline, no network frames forwarded)\n";
    return 0;
}
void usage() {
    std::cout << "rtw-inspect [--efuse|--firmware|--rx|--8192eu-efuse|--8192eu-firmware|--8192eu-rx] <binary-file>\n"
              << "Offline parser: never opens a USB device, installs a kext, "
              << "or modifies hardware.\n";
}
}
int main(int argc, char** argv) {
    if(argc==2 && (std::string(argv[1])=="--help" || std::string(argv[1])=="-h")) {
        usage();return 0;
    }
    if (argc!=3) {usage();return 2;}
    try {
        const std::string mode=argv[1];
        if(mode=="--efuse") return efuse(argv[2]);
        if(mode=="--firmware") return firmware(argv[2]);
        if(mode=="--rx") return rx(argv[2]);
        if(mode=="--8192eu-efuse") return efuse8192(argv[2]);
        if(mode=="--8192eu-firmware") return firmware8192(argv[2]);
        if(mode=="--8192eu-rx") return rx8192(argv[2]);
        usage();return 2;
    } catch(const std::exception& e) {
        std::cerr << "Input refused: " << e.what() << '\n';
        return 2;
    }
}
