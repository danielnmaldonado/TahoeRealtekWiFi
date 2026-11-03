// SPDX-License-Identifier: GPL-2.0-only
// Deterministic synthetic CPU/allocation fixtures; no driver, app or device I/O.
#include "Model.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <sys/resource.h>

static bool countAllocations=false;
static uint64_t allocations=0;
void* operator new(std::size_t size) {
    if(countAllocations)++allocations;
    if(void* memory=std::malloc(size?size:1))return memory;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {return ::operator new(size);}
void operator delete(void* memory) noexcept {std::free(memory);}
void operator delete[](void* memory) noexcept {std::free(memory);}
void operator delete(void* memory,std::size_t) noexcept {std::free(memory);}
void operator delete[](void* memory,std::size_t) noexcept {std::free(memory);}

static uint32_t randomValue(uint32_t& state) {
    state^=state<<13;state^=state>>17;state^=state<<5;return state;
}
static const unsigned allowed[]={1,2,3,4,5,6,7,8,9,10,11,
    36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140,
    149,153,157,161,165};
static uint64_t digest=1469598103934665603ULL;
static void hashBytes(const void* data,size_t length) {
    const auto* bytes=static_cast<const uint8_t*>(data);
    for(size_t i=0;i<length;++i){digest^=bytes[i];digest*=1099511628211ULL;}
}
template<class T> static void hashValue(T value) {hashBytes(&value,sizeof(value));}
static void equivalenceFixtures() {
    uint32_t state=0x243f6a88;
    const uint32_t flags[]={0,RTW_CONTROL_BANDS_EXPLICIT,RTW_CONTROL_BAND_2GHZ,
        RTW_CONTROL_BAND_5GHZ,RTW_CONTROL_BAND_5GHZ_HIGH,
        RTW_CONTROL_BAND_5GHZ_DFS_LOW,RTW_CONTROL_BAND_5GHZ_DFS_MID,0x80000000U,UINT32_MAX};
    const int16_t readings[]={-128,-127,-99,-30,0,INT16_MAX,INT16_MIN};
    const uint8_t lengths[]={0,3,32,33};
    for(unsigned fixture=0;fixture<2048;++fixture) {
        RTWControlChannelMap map{};map.version=1;
        for(unsigned channel:allowed)if(randomValue(state)&1U)map.channels[channel/8]|=uint8_t(1U<<(channel%8));
        switch(fixture%8) {
        case 0:map.reserved[fixture%3]=randomValue(state)|1U;break;
        case 1:map.version=UINT32_MAX;break;
        case 2:map.channels[144/8]|=1U<<(144%8);break;
        case 3:map.channels[255/8]|=1U<<(255%8);break;
        default:break;
        }
        RTWControlScanResults scan{};scan.version=fixture%17?1:UINT32_MAX;
        scan.count=fixture%23?64:65;scan.truncated=fixture%19?0:2;
        for(unsigned row=0;row<64;++row) {
            auto& record=scan.records[row];record.ssidLength=lengths[randomValue(state)%4];
            for(auto& byte:record.ssid)byte=uint8_t(randomValue(state)%8);
            // Many equal 3/32-byte names make grouping and usable-BSS selection
            // sensitive to RSSI, security, exact maps and legacy flags.
            if(row%3)std::fill(std::begin(record.ssid),std::end(record.ssid),uint8_t(row%4));
            record.channel=uint8_t(randomValue(state)%256);
            if(row%2)record.channel=uint8_t(allowed[randomValue(state)%35]);
            record.rssi=readings[randomValue(state)%7];
            record.securityFlags=uint8_t((row%4)?7:randomValue(state));
            for(auto& byte:record.bssid)byte=uint8_t(randomValue(state));
        }
        const auto* channels=fixture%5?&map:nullptr;
        const uint32_t stationFlags=flags[fixture%9];
        hashValue(rtw_control_channel_map_valid(&map));
        for(const auto& row:scan.records)hashValue(rtwui::supported(row,stationFlags,channels));
        const auto rows=rtwui::networks(scan,stationFlags,channels);
        hashValue(rows.size());for(const auto& row:rows)hashBytes(&row,sizeof(row));
        for(size_t length:{size_t(0),size_t(3),size_t(32),size_t(33)}) {
            const uint8_t ssid[33]{};
            hashValue(rtwui::observedSSIDSupported(scan,ssid,length,stationFlags,channels));
            hashValue(rtwui::savedProfileReconnectAllowed(scan,ssid,length,stationFlags,channels));
        }
        hashValue(rtwui::observedNetworkBands(scan,scan.records[0],stationFlags,channels));
        const auto bands=rtwui::stationBandDescription(stationFlags,channels);
        const auto details=rtwui::stationChannelDescription(stationFlags,channels);
        hashBytes(bands.data(),bands.size());hashBytes(details.data(),details.size());
    }
    printf("{\"case\":\"equivalence_digest\",\"fixtures\":2048,\"digest\":\"%016llx\"}\n",static_cast<unsigned long long>(digest));
}

static volatile uint64_t consumed=0;
template<class Work> static void bench(const char* name,unsigned iterations,Work work) {
    for(unsigned i=0;i<100;++i)consumed+=work(i);
    // Multiple batches expose scheduling spread; every timed operation uses
    // the same fixture sequence in both independently built binaries.
    constexpr unsigned batches=15;
    std::array<double,batches> times{};
    uint64_t totalAllocations=0;
    for(unsigned batch=0;batch<batches;++batch) {
        allocations=0;countAllocations=true;
        const auto start=std::chrono::steady_clock::now();
        uint64_t sum=0;
        for(unsigned i=0;i<iterations;++i)sum+=work(i);
        const auto end=std::chrono::steady_clock::now();
        countAllocations=false;consumed+=sum;totalAllocations+=allocations;
        times[batch]=std::chrono::duration<double,std::nano>(end-start).count()/iterations;
    }
    std::sort(times.begin(),times.end());
    printf("{\"case\":\"%s\",\"batches\":%u,\"iterations_per_batch\":%u,\"min_ns\":%.4f,\"median_ns\":%.4f,\"max_ns\":%.4f,\"allocations_per_operation\":%.6f}\n",
        name,batches,iterations,times.front(),times[batches/2],times.back(),
        static_cast<double>(totalAllocations)/(iterations*batches));
}
int main() {
    equivalenceFixtures();
    std::array<RTWControlChannelMap,64> maps{};
    uint32_t state=0x85a308d3;
    for(auto& map:maps) {
        map.version=1;
        for(unsigned channel:allowed)if(randomValue(state)&1U)map.channels[channel/8]|=uint8_t(1U<<(channel%8));
    }
    bench("valid_map_subsets",100000,[&](unsigned i){return rtw_control_channel_map_valid(&maps[i%64]);});
    for(auto& map:maps)map.channels[255/8]|=1U<<(255%8);
    bench("invalid_map_last_bit",100000,[&](unsigned i){return rtw_control_channel_map_valid(&maps[i%64]);});
    for(auto& map:maps)map.channels[0]|=1U;
    bench("invalid_map_first_bit",100000,[&](unsigned i){return rtw_control_channel_map_valid(&maps[i%64]);});
    RTWControlChannelMap map{};map.version=1;
    for(unsigned channel:allowed)map.channels[channel/8]|=uint8_t(1U<<(channel%8));
    std::array<RTWControlScanResults,64> scans{};
    for(unsigned fixture=0;fixture<scans.size();++fixture) {
        auto& scan=scans[fixture];scan.version=1;scan.count=64;
        for(unsigned row=0;row<scan.count;++row) {
            auto& record=scan.records[row];record.ssidLength=10;
            std::snprintf(reinterpret_cast<char*>(record.ssid),sizeof(record.ssid),"Bench-%04u",row%8);
            record.channel=uint8_t(allowed[(fixture+row)%35]);
            record.rssi=-30-static_cast<int>((fixture+row)%70);record.securityFlags=7;
        }
    }
    bench("networks_64_bss_8_ssids",5000,[&](unsigned i){const auto rows=rtwui::networks(scans[i%64],0,&map);return rows.size()+unsigned(rows[0].rssi);});
    for(auto& scan:scans)for(unsigned row=0;row<scan.count;++row)
        std::snprintf(reinterpret_cast<char*>(scan.records[row].ssid),sizeof(scan.records[row].ssid),"Bench-%04u",row);
    bench("networks_64_unique",5000,[&](unsigned i){const auto rows=rtwui::networks(scans[i%64],UINT32_MAX,&map);return rows.size()+unsigned(rows[0].rssi);});
    for(auto& scan:scans)for(auto& record:scan.records) {
        record.ssidLength=3;std::memcpy(record.ssid,"Lab",3);record.channel=6;
    }
    map={};map.version=1;map.channels[11/8]|=1U<<(11%8);
    bench("observed_bands_64_denied",5000,[&](unsigned i){const auto& scan=scans[i%64];return rtwui::observedNetworkBands(scan,scan.records[0],UINT32_MAX,&map);});
    const auto ssid=reinterpret_cast<const uint8_t*>("Lab");
    bench("saved_profile_64_denied",5000,[&](unsigned i){return rtwui::savedProfileReconnectAllowed(scans[i%64],ssid,3,UINT32_MAX,&map);});
    rusage usage{};getrusage(RUSAGE_SELF,&usage);
    printf("{\"case\":\"process_peak_rss\",\"bytes\":%ld,\"consumed\":%llu}\n",usage.ru_maxrss,static_cast<unsigned long long>(consumed));
}
