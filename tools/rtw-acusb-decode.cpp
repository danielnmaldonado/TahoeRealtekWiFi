// SPDX-License-Identifier: BSD-2-Clause
// Offline RTL8812AU / RTL8821AU USB bulk-RX descriptor inspector.
// Reads a user-supplied capture; never opens or writes to a USB device.
#include "rtw/RTL8812AUDataPath.h"
#include "rtw/RTL8812AUStation.h"
#include <array>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static const char *label(rtw_r12au_result result) {
    switch (result) {
    case RTW_R12AU_FRAME: return "frame";
    case RTW_R12AU_END: return "end";
    case RTW_R12AU_BAD_ARGUMENT: return "invalid input";
    case RTW_R12AU_TRUNCATED: return "truncated transfer";
    case RTW_R12AU_MALFORMED: return "malformed descriptor";
    case RTW_R12AU_TOO_MANY: return "too many frames in transfer";
    }
    return "unrecognized result";
}
int main(int argc, char **argv) {
    if (argc < 3 || argc > 6 || !argv[1] || !argv[2]) {
        std::cerr << "Usage: rtw-acusb-decode rtl8812au|rtl8821au capture.bin "
                     "[--channel N] [--fcs]\n"
                  << "Channel must come from captured PHY tuning metadata. "
                     "No USB access or transmission.\n";
        return 2;
    }
    const std::string family=argv[1];
    const unsigned chains=family=="rtl8812au" ? 2U :
                          family=="rtl8821au" ? 1U : 0U;
    if (!chains) {
        std::cerr << "Unsupported family: " << family << '\n';
        return 2;
    }
    unsigned primary_channel=0;
    bool fcs_present=false;
    for (int i=3; i<argc; ++i) {
        if (std::strcmp(argv[i],"--fcs")==0) {
            if (fcs_present) return 2;
            fcs_present=true;
        } else if (std::strcmp(argv[i],"--channel")==0 && i+1<argc) {
            if (primary_channel) return 2;
            errno=0;
            char *endptr=nullptr;
            const auto number=std::strtoul(argv[++i],&endptr,10);
            if (errno || !endptr || *endptr || number==0 || number>255U) {
                std::cerr << "Invalid physical channel argument\n";
                return 2;
            }
            primary_channel=static_cast<unsigned>(number);
        } else {
            std::cerr << "Unknown or incomplete argument: "<<argv[i]<<'\n';
            return 2;
        }
    }
    if (fcs_present && !primary_channel) {
        std::cerr << "--fcs requires --channel\n";
        return 2;
    }
    std::ifstream file(argv[2],std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Cannot open capture file\n";
        return 2;
    }
    const auto end=file.tellg();
    if (end < 0 || static_cast<unsigned long long>(end) > RTW_R12AU_MAX_BULK) {
        std::cerr << "Capture length exceeds the 64-KiB transfer limit\n";
        return 2;
    }
    std::vector<uint8_t> buffer(static_cast<size_t>(end));
    file.seekg(0);
    if (!buffer.empty() &&
        !file.read(reinterpret_cast<char*>(buffer.data()),buffer.size())) {
        std::cerr << "Failed to read capture\n";
        return 2;
    }
    rtw_r12au_rx_iterator it{buffer.data(),buffer.size(),0,0};
    rtw_r12au_rx_frame frame{};
    unsigned errors=0;
    for (;;) {
        const size_t pos=it.cursor;
        const auto result=rtw_r12au_rx_next(&it,chains,&frame);
        if (result==RTW_R12AU_END) break;
        if (result!=RTW_R12AU_FRAME) {
            std::cerr<<"Decode error at byte "<<pos<<": "<<label(result)<<'\n';
            ++errors; break;
        }
        std::cout<<"offset="<<pos<<" bytes="<<frame.payload_length;
        if (frame.is_c2h)
            std::cout<<" c2h=0x"<<std::hex<<static_cast<unsigned>(frame.c2h_id)
                     <<" seq="<<std::dec<<static_cast<unsigned>(frame.c2h_sequence);
        else
            std::cout<<" rate="<<static_cast<unsigned>(frame.rate)
                     <<" width="<<static_cast<unsigned>(frame.bandwidth_mhz)
                     <<"MHz phy="<<frame.phy_length<<" crc="<<static_cast<unsigned>(frame.crc_error)
                     <<" icv="<<static_cast<unsigned>(frame.icv_error);
        if (primary_channel && !frame.is_c2h && !frame.crc_error) {
            rtw_r12au_ap ap{};
            const auto parsed=rtw_r12au_scan_parse(
                &frame,static_cast<uint8_t>(primary_channel),
                fcs_present?1:0,&ap);
            if (parsed==RTW_R12AU_SCAN_OK) {
                static const char *security[]={
                    "open", "wpa2-psk-ccmp", "unsupported"};
                std::cout<<" beacon_channel="
                         <<static_cast<unsigned>(ap.primary_channel)
                         <<" security="<<security[ap.security]
                         <<" ssid_hex=";
                constexpr char hexdigits[]="0123456789abcdef";
                for (unsigned i=0;i<ap.ssid_length;i++) {
                    const auto ch=ap.ssid[i];
                    std::cout<<hexdigits[ch>>4]<<hexdigits[ch&15U];
                }
                /* Append new metadata after existing output fields so the
                 * established decoder text contract remains compatible. */
                std::cout<<" advertised_width="
                         <<static_cast<unsigned>(ap.operating_width_mhz)
                         <<"MHz center="<<static_cast<unsigned>(ap.center_channel)
                         <<" ht="<<static_cast<unsigned>(ap.ht_capable)
                         <<" vht="<<static_cast<unsigned>(ap.vht_capable);
            }
        }
        std::cout<<'\n';
    }
    std::cout<<"frames="<<it.frames<<" errors="<<errors<<'\n';
    return errors?1:0;
}
