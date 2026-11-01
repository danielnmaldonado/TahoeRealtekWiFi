/* SPDX-License-Identifier: GPL-2.0-only.
 * Exercise the same preflight/prefix boundary helper used by the macOS kext.
 * Real family codecs are used, not a mock descriptor parser. */
#include "rtw/USBPackets.h"
#include "rtw/RTL8192EU.h"
#include "rtw/RTL8192CDataPath.h"
#include "rtw/RTL8814AUDataPath.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static unsigned checks=0;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::abort();}} while(0)
static void put32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(n>>(8*i));}
static rtw_packet_error next92e(rtw_rx_iterator*i,void*){
    rtw_8192eu_rx_frame f{};return rtw_8192eu_rx_next(i,&f);
}
static rtw_packet_error next92c(rtw_rx_iterator*i,void*){
    rtw_92c_rx_frame f{};return rtw_92c_rx_next(i,0,&f);
}
static rtw_packet_error next88e(rtw_rx_iterator*i,void*){
    rtw_92c_rx_frame f{};return rtw_92c_rx_next(i,1,&f);
}
static rtw_packet_error next8814(rtw_rx_iterator*i,void*){
    rtw_8814au_rx_frame f{};return rtw_8814au_rx_next(i,&f);
}
static rtw_packet_error no_progress(rtw_rx_iterator*,void*){return RTW_PACKETS_OK;}
static rtw_packet_error no_frame(rtw_rx_iterator*i,void*){++i->cursor;return RTW_PACKETS_OK;}
static rtw_packet_error out_of_bounds(rtw_rx_iterator*i,void*){i->cursor=i->length+1;i->frames++;return RTW_PACKETS_OK;}
static rtw_packet_error fake_too_many(rtw_rx_iterator*,void*){return RTW_PACKETS_TOO_MANY;}

static void inspect(const std::vector<uint8_t>&b,size_t n,
    rtw_rx_preflight_decoder codec,rtw_packet_error end,size_t prefix,size_t frames){
    rtw_rx_preflight_result r{};
    CHECK(rtw_rx_preflight_transfer(b.data(),n,codec,nullptr,&r)==end);
    CHECK(r.valid_bytes==prefix&&r.valid_frames==frames);
    if(end!=RTW_PACKETS_DONE){CHECK(r.error_offset==prefix&&r.error_frames==frames);}
    /* The second decode sees no malformed suffix. */
    rtw_rx_iterator it{};rtw_rx_iterator_init(&it,b.data(),prefix);
    size_t count=0;rtw_packet_error e;
    while((e=codec(&it,nullptr))==RTW_PACKETS_OK)++count;
    CHECK(e==RTW_PACKETS_DONE);
    CHECK(count==frames);
    CHECK(it.cursor==prefix);
}

static void gen2() {
    std::vector<uint8_t>b(96,0);
    put32(&b[0],8);b[24]=0x08;
    put32(&b[32],8);b[56]=0x08;
    /* Two fully valid frames + one malformed descriptor and nonzero tail. */
    b[64+5]=0x80;
    inspect(b,96,next92e,RTW_PACKETS_MALFORMED,64,2);
    inspect(b,64,next92e,RTW_PACKETS_DONE,64,2);
    inspect(b,32,next92e,RTW_PACKETS_DONE,32,1);
    inspect(b,96,next8814,RTW_PACKETS_MALFORMED,64,2);
    /* A nonzero transfer remainder smaller than one RX descriptor. */
    b.resize(33);b[32]=0x81;
    inspect(b,b.size(),next92e,RTW_PACKETS_TRUNCATED,32,1);
    inspect(b,b.size(),next8814,RTW_PACKETS_TRUNCATED,32,1);
    /* Zero alignment padding is not treated as a damaged aggregate. */
    b[32]=0;
    inspect(b,b.size(),next92e,RTW_PACKETS_DONE,32,1);
    inspect(b,b.size(),next8814,RTW_PACKETS_DONE,32,1);
    /* Failure in first descriptor never accepts any unvalidated prefix. */
    b.resize(32);b[0]=0;b[1]=0;b[2]=1;
    inspect(b,b.size(),next92e,RTW_PACKETS_MALFORMED,0,0);
}
static void gen1() {
    std::vector<uint8_t>b(160,0);
    put32(b.data(),24);b[24]=0x08;
    /* CU and 88E use 128-byte aggregate slot alignment. */
    put32(b.data()+128,0);b[133]=0x91;
    inspect(b,160,next92c,RTW_PACKETS_MALFORMED,128,1);
    inspect(b,160,next88e,RTW_PACKETS_MALFORMED,128,1);
    inspect(b,128,next92c,RTW_PACKETS_DONE,128,1);
    inspect(b,128,next88e,RTW_PACKETS_DONE,128,1);
    b.resize(129);b[128]=0x91;
    inspect(b,129,next92c,RTW_PACKETS_TRUNCATED,128,1);
    inspect(b,129,next88e,RTW_PACKETS_TRUNCATED,128,1);
}
static void bad_callbacks() {
    std::array<uint8_t,32>b{};
    rtw_rx_preflight_result r{42,42,42,42};
    /* The preflight report must never overwrite a borrowed USB transfer. */
    rtw_rx_preflight_result aliased{};
    std::memset(&aliased, 0xA5, sizeof(aliased));
    const std::array<uint8_t, sizeof(aliased)> original = [] {
        std::array<uint8_t, sizeof(rtw_rx_preflight_result)> b{};
        b.fill(0xA5);
        return b;
    }();
    CHECK(rtw_rx_preflight_transfer(reinterpret_cast<const uint8_t*>(&aliased),
        sizeof(aliased), next92e, nullptr, &aliased) == RTW_PACKETS_BAD_ARGUMENT);
    CHECK(std::memcmp(&aliased, original.data(), original.size()) == 0);
    CHECK(rtw_rx_preflight_transfer(
        reinterpret_cast<const uint8_t*>(&aliased)+3U, sizeof(aliased)-3U,
        next92e, nullptr, &aliased) == RTW_PACKETS_BAD_ARGUMENT);
    CHECK(std::memcmp(&aliased, original.data(), original.size()) == 0);
    CHECK(rtw_rx_preflight_transfer(nullptr,32,next92e,nullptr,&r)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(!r.valid_bytes&&!r.valid_frames&&!r.error_offset&&!r.error_frames);
    CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),nullptr,nullptr,&r)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),next92e,nullptr,nullptr)==RTW_PACKETS_BAD_ARGUMENT);
    CHECK(rtw_rx_preflight_transfer(nullptr,0,next92e,nullptr,&r)==RTW_PACKETS_DONE);
    CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),no_progress,nullptr,&r)==RTW_PACKETS_MALFORMED);
    CHECK(!r.valid_bytes&&!r.valid_frames);
    CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),no_frame,nullptr,&r)==RTW_PACKETS_MALFORMED);
    CHECK(!r.valid_bytes&&!r.valid_frames);
    CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),out_of_bounds,nullptr,&r)==RTW_PACKETS_MALFORMED);
    CHECK(!r.valid_bytes&&!r.valid_frames);
    CHECK(rtw_rx_preflight_transfer(b.data(),b.size(),fake_too_many,nullptr,&r)==RTW_PACKETS_TOO_MANY);
    CHECK(!r.valid_bytes&&!r.valid_frames);
}
int main(){gen2();gen1();bad_callbacks();std::printf("rx preflight: %u checks passed\n",checks);return 0;}
