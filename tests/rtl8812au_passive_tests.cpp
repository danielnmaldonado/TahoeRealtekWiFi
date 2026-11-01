/* SPDX-License-Identifier: BSD-2-Clause */
#include "rtw/RTL8812AUPassive.h"
#include "rtw/FCS.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
unsigned checks = 0U;
#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        std::abort(); \
    } \
} while (0)

void append16(std::vector<uint8_t> &data, size_t offset, uint16_t value) {
    data[offset] = static_cast<uint8_t>(value);
    data[offset+1] = static_cast<uint8_t>(value >> 8U);
}
std::vector<uint8_t> make_beacon(uint8_t channel=6U) {
    std::vector<uint8_t> packet(36U, 0U);
    packet[0] = 0x80U;
    std::fill(packet.begin()+4U,packet.begin()+10U,0xffU);
    packet[10] = packet[16] = 0x02U;
    packet[15] = packet[21] = 0x11U;
    append16(packet, 32U, 100U);
    append16(packet, 34U, 0x0401U);
    packet.insert(packet.end(), {0U,4U,'T','e','s','t',
        1U,4U,0x82U,0x84U,0x8bU,0x96U,3U,1U,channel});
    const uint32_t crc=rtw_fcs32(packet.data(),packet.size());
    for (unsigned n=0U;n<4U;++n)
        packet.push_back(static_cast<uint8_t>(crc>>(8U*n)));
    return packet;
}
std::vector<uint8_t> envelope(const std::vector<uint8_t> &packet,
                               bool crc_error=false, bool c2h=false) {
    std::vector<uint8_t> frame(24U+packet.size(),0U);
    const uint32_t len=static_cast<uint32_t>(packet.size());
    CHECK(len<=RTW_R12AU_MAX_MPDU);
    frame[0]=static_cast<uint8_t>(len);
    frame[1]=static_cast<uint8_t>((len>>8U) | (crc_error?0x40U:0U));
    if(c2h) frame[11]=0x10U; /* word 2, bit 28 */
    else frame[12]=4U; /* Realtek legacy OFDM6 rate */
    std::copy(packet.begin(),packet.end(),frame.begin()+24U);
    frame.resize((frame.size()+7U)&~size_t(7U),0U);
    return frame;
}
struct Seen {
    unsigned callback_count = 0U;
    bool stop = false;
    uint8_t channel = 0U;
    uint8_t ssid_length = 0U;
};
int collected(void* context, const rtw_r12au_ap* ap,
              const rtw_r12au_rx_frame* frame) {
    auto &seen=*static_cast<Seen*>(context);
    CHECK(ap && frame && frame->payload_length>=40U);
    CHECK(frame->phy_length==0U && !frame->is_c2h);
    ++seen.callback_count;
    seen.channel=ap->primary_channel;
    seen.ssid_length=ap->ssid_length;
    return seen.stop?0:1;
}
void main_tests() {
    rtw_r12au_passive_stats stats{};
    Seen seen{};
    auto beacon=make_beacon();
    auto bulk=envelope(beacon);
    CHECK(rtw_r12au_passive_channel6(bulk.data(),bulk.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.frames==1U && stats.aggregates==1U &&
          stats.accepted_beacons==1U && stats.malformed_aggregates==0U);
    CHECK(seen.callback_count==1 && seen.channel==6U && seen.ssid_length==4U);

    auto second=envelope(beacon);
    bulk.insert(bulk.end(),second.begin(),second.end());
    CHECK(rtw_r12au_passive_channel6(bulk.data(),bulk.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.frames==3U && stats.accepted_beacons==3U &&
          stats.aggregates==2U && seen.callback_count==3);

    auto corrupted_tail=bulk;
    corrupted_tail.insert(corrupted_tail.end(), {1U,2U,3U,4U,5U});
    const unsigned previous=seen.callback_count;
    const size_t accepted=stats.accepted_beacons, frames=stats.frames;
    CHECK(rtw_r12au_passive_channel6(corrupted_tail.data(),corrupted_tail.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_BAD_AGGREGATE);
    CHECK(stats.malformed_aggregates==1U && stats.frames==frames &&
          stats.accepted_beacons==accepted && seen.callback_count==previous);
    CHECK(rtw_r12au_passive_channel6(nullptr,1U,collected,&seen,&stats)==
          RTW_R12AU_PASSIVE_BAD_ARGUMENT);
    CHECK(rtw_r12au_passive_channel6(bulk.data(),bulk.size(),nullptr,&seen,&stats)==
          RTW_R12AU_PASSIVE_BAD_ARGUMENT);
    CHECK(rtw_r12au_passive_channel6(bulk.data(),65537U,collected,&seen,&stats)==
          RTW_R12AU_PASSIVE_BAD_ARGUMENT);
    CHECK(rtw_r12au_passive_channel6(nullptr,0U,collected,&seen,&stats)==
          RTW_R12AU_PASSIVE_OK);
    CHECK(rtw_r12au_passive_channel6(bulk.data(),bulk.size(),collected,&seen,nullptr)==
          RTW_R12AU_PASSIVE_BAD_ARGUMENT);

    auto damaged=beacon;
    damaged[40]^=0x10U; /* FCS error */
    auto bad_fcs=envelope(damaged);
    CHECK(rtw_r12au_passive_channel6(bad_fcs.data(),bad_fcs.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.fcs_drops==1U && seen.callback_count==previous);
    auto bad_ch=envelope(make_beacon(11U));
    CHECK(rtw_r12au_passive_channel6(bad_ch.data(),bad_ch.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.wrong_channel==1U);
    auto crc=envelope(beacon,true);
    CHECK(rtw_r12au_passive_channel6(crc.data(),crc.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.crc_drops==1U);
    auto icv=envelope(beacon);
    icv[1]|=0x80U; /* RX descriptor ICV bit15 */
    CHECK(rtw_r12au_passive_channel6(icv.data(),icv.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.icv_drops==1U);
    auto c2h=envelope({1U,2U},false,true);
    CHECK(rtw_r12au_passive_channel6(c2h.data(),c2h.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.c2h==1U);
    auto not_beacon=beacon;
    not_beacon[0]=0x08U;
    const auto sum=rtw_fcs32(not_beacon.data(),not_beacon.size()-4U);
    for(unsigned i=0U;i<4U;++i)
        not_beacon[not_beacon.size()-4U+i]=static_cast<uint8_t>(sum>>(8U*i));
    auto other=envelope(not_beacon);
    CHECK(rtw_r12au_passive_channel6(other.data(),other.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.non_beacons==1U);
    auto unsupported=make_beacon();
    unsupported[34]|=0x10U;
    const auto newcrc=rtw_fcs32(unsupported.data(),unsupported.size()-4U);
    for(unsigned i=0U;i<4U;++i)
        unsupported[unsupported.size()-4U+i]=static_cast<uint8_t>(newcrc>>(8U*i));
    auto other_ap=envelope(unsupported);
    CHECK(rtw_r12au_passive_channel6(other_ap.data(),other_ap.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.unsupported==1U);
    CHECK(seen.callback_count==previous+1U);
    seen.stop=true;
    CHECK(rtw_r12au_passive_channel6(bulk.data(),bulk.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_CALLBACK_STOP);
    CHECK(seen.callback_count==previous+2U);

    /* A malformed USB aggregate must not overwrite itself when callers
     * accidentally alias its backing bytes with the statistics object. */
    rtw_r12au_passive_stats aliased{};
    std::memset(&aliased,0xa5,sizeof(aliased));
    std::array<uint8_t,sizeof(aliased)> snapshot{};
    std::memcpy(snapshot.data(),&aliased,sizeof(aliased));
    CHECK(rtw_r12au_passive_channel6(
          reinterpret_cast<const uint8_t*>(&aliased),sizeof(aliased),
          collected,&seen,&aliased)==RTW_R12AU_PASSIVE_BAD_ARGUMENT);
    CHECK(std::memcmp(snapshot.data(),&aliased,sizeof(aliased))==0);
    CHECK(rtw_r12au_passive_channel6(
          reinterpret_cast<const uint8_t*>(&aliased)+5U,sizeof(aliased)-5U,
          collected,&seen,&aliased)==RTW_R12AU_PASSIVE_BAD_ARGUMENT);
    CHECK(std::memcmp(snapshot.data(),&aliased,sizeof(aliased))==0);

    /* Saturating counters protect long captures against wraparound. */
    stats.fcs_drops=SIZE_MAX;
    CHECK(rtw_r12au_passive_channel6(bad_fcs.data(),bad_fcs.size(),
          collected,&seen,&stats)==RTW_R12AU_PASSIVE_OK);
    CHECK(stats.fcs_drops==SIZE_MAX);
}
void hostile_usb_tests() {
    uint32_t state=0xa376bd19U;
    std::array<uint8_t,251U> bytes{};
    for(unsigned round=0U;round<20000U;++round) {
        for(auto &byte:bytes) {
            state^=state<<13U;
            state^=state>>17U;
            state^=state<<5U;
            byte=static_cast<uint8_t>(state);
        }
        Seen seen{};
        rtw_r12au_passive_stats stats{};
        const size_t length=round%bytes.size();
        const auto result=rtw_r12au_passive_channel6(
            bytes.data(),length,collected,&seen,&stats);
        CHECK(result==RTW_R12AU_PASSIVE_OK ||
              result==RTW_R12AU_PASSIVE_BAD_AGGREGATE ||
              result==RTW_R12AU_PASSIVE_TOO_MANY);
        if (result!=RTW_R12AU_PASSIVE_OK)
            CHECK(seen.callback_count==0U && stats.frames==0U);
    }
}
}
int main() {
    main_tests();
    hostile_usb_tests();
    std::printf("rtw-r12au-passive-tests: %u checks; no physical radio tested\n",checks);
}
