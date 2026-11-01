// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Model.hpp"
#include <array>
#include <iomanip>
#include <locale>
#include <sstream>
#include <string>

namespace rtwui {
// In-memory, bounded diagnostics. Deliberately excludes network names,
// addresses, registry IDs, credentials, packet contents and wall-clock dates.
struct SignalHistory {
    struct Entry {
        double uptime=0;
        uint32_t statusError=0, flags=0, state=0, channel=0, usbError=0;
        int raw=0, filtered=0;
        unsigned bars=0;
        uint64_t tx=0, rx=0, txErrors=0, rxErrors=0;
    };
    static constexpr size_t capacity=128;
    std::array<Entry,capacity> entries{};
    size_t next=0, count=0;
    void clear() { entries={};next=0;count=0; }
    void record(const RTWControlStatus& status,const SignalMemory& signal,
                double now,uint32_t error) {
        if(!std::isfinite(now) || now<0)return;
        Entry e{};
        e.uptime=now;e.statusError=error;
        if(!error) {
            e.flags=status.flags;e.state=status.state;e.channel=status.channel;
            e.usbError=status.usbError;e.raw=status.rssi;
            e.tx=status.txPackets;e.rx=status.rxPackets;
            e.txErrors=status.txErrors;e.rxErrors=status.rxErrors;
        }
        e.filtered=signal.value(now);e.bars=signal.level(now);
        entries[next]=e;next=(next+1)%capacity;
        if(count<capacity)++count;
    }
    std::string report() const {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out<<"Tahoe Wi-Fi USB signal diagnostics\n"
              "Raw: last valid driver RSSI; filtered: median of three status polls.\n"
              "Zero means unavailable, not 0 dBm. Bars use 3 dB hysteresis.\n"
              "tracked=1: driver expires PHY samples after 10 seconds.\n"
              "held=1: last PHY sample is at least 2 seconds old.\n"
              "A status error is not a measured signal drop. WPA2 state is not filtered.\n"
              "No SSID, BSSID, MAC, serial number, password or traffic is recorded.\n"
              "History is bounded to 128 polls and resets on adapter/session changes.\n\n"
              "uptime_s,status_error,flags,state,channel,raw_dbm,filtered_dbm,bars,"
              "tracked,held,usb_error,tx_packets,rx_packets,tx_errors,rx_errors\n";
        out<<std::fixed<<std::setprecision(3);
        for(size_t i=0;i<count;++i) {
            const auto& e=entries[(next+capacity-count+i)%capacity];
            out<<e.uptime<<','<<e.statusError<<','<<e.flags<<','<<e.state<<','
               <<e.channel<<','<<e.raw<<','<<e.filtered<<','<<e.bars<<','
               <<bool(e.flags&RTW_CONTROL_SIGNAL_TRACKED)<<','
               <<bool(e.flags&RTW_CONTROL_SIGNAL_HELD)<<','<<e.usbError<<','
               <<e.tx<<','<<e.rx<<','<<e.txErrors<<','<<e.rxErrors<<'\n';
        }
        return out.str();
    }
};
}
