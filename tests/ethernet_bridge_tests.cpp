/* SPDX-License-Identifier: GPL-2.0-only. Offline bridge contract tests. */
#include "rtw/EthernetBridge.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static size_t checks=0;
#define CHECK(x) do {++checks; if (!(x)) {std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);std::abort();}} while(0)
namespace {
constexpr uint8_t sta[6]={0x02,0x11,0x22,0x33,0x44,0x55};
constexpr uint8_t ap[6]={0x02,0xaa,0xbb,0xcc,0xdd,0xee};
// These are independent wire fixtures, not produced by the bridge/codec.
const std::vector<uint8_t> arp={
    0x02,0x11,0x22,0x33,0x44,0x55, 0x02,0x66,0x77,0x88,0x99,0x00,
    0x08,0x06, 0x00,0x01,0x08,0x00,0x06,0x04,0x00,0x01,
    0x02,0x66,0x77,0x88,0x99,0x00,0xc0,0xa8,0x01,0x02,
    0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xa8,0x01,0x01};
const std::vector<uint8_t> arp_mpdu={
    0x08,0x02,0x00,0x00, 0x02,0x11,0x22,0x33,0x44,0x55,
    0x02,0xaa,0xbb,0xcc,0xdd,0xee, 0x02,0x66,0x77,0x88,0x99,0x00,
    0x10,0x00, 0xaa,0xaa,0x03,0x00,0x00,0x00,0x08,0x06,
    0x00,0x01,0x08,0x00,0x06,0x04,0x00,0x01,
    0x02,0x66,0x77,0x88,0x99,0x00,0xc0,0xa8,0x01,0x02,
    0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xa8,0x01,0x01};
const std::vector<uint8_t> auth_response={
    0xb0,0x00,0x00,0x00, 0x02,0x11,0x22,0x33,0x44,0x55,
    0x02,0xaa,0xbb,0xcc,0xdd,0xee, 0x02,0xaa,0xbb,0xcc,0xdd,0xee,
    0x20,0x00, 0x00,0x00,0x02,0x00,0x00,0x00};
struct Sink {
    std::vector<std::vector<uint8_t>> host,eapol,mgmt;
    unsigned attempts=0,fail_at=0;
};
int host(void* p,const uint8_t* frame,size_t n) {
    auto& s=*static_cast<Sink*>(p);
    if (++s.attempts==s.fail_at) return 0;
    s.host.emplace_back(frame,frame+n);return 1;
}
int eapol(void* p,const uint8_t* frame,size_t n) {
    auto& s=*static_cast<Sink*>(p);
    if (++s.attempts==s.fail_at) return 0;
    s.eapol.emplace_back(frame,frame+n);return 1;
}
int mgmt(void* p,const uint8_t* frame,size_t n) {
    auto& s=*static_cast<Sink*>(p);
    if (++s.attempts==s.fail_at) return 0;
    s.mgmt.emplace_back(frame,frame+n);return 1;
}
rtw_bridge_callbacks callbacks(Sink& s) {return {&s,host,eapol,mgmt};}
void put32(uint8_t* p,uint32_t n) {for(unsigned i=0;i<4;++i)p[i]=uint8_t(n>>(8*i));}
std::vector<uint8_t> record(const std::vector<uint8_t>& m,uint32_t flags=0,
                            uint32_t w1=0,uint32_t w2=0) {
    std::vector<uint8_t> data((24+m.size()+7)&~size_t(7),0);
    put32(data.data(),uint32_t(m.size())|flags);put32(data.data()+4,w1);
    put32(data.data()+8,w2);std::memcpy(data.data()+24,m.data(),m.size());
    return data;
}
std::vector<uint8_t> concat(std::vector<uint8_t> a,const std::vector<uint8_t>& b) {
    a.insert(a.end(),b.begin(),b.end());return a;
}
rtw_ethernet_bridge configured(rtw_port_security security=RTW_PORT_OPEN,
                               rtw_bridge_fcs fcs=RTW_BRIDGE_FCS_ABSENT) {
    rtw_ethernet_bridge b{};rtw_bridge_reset(&b);
    CHECK(rtw_bridge_configure(&b,sta,ap,security,fcs)==RTW_BRIDGE_OK);
    rtw_bridge_set_admin(&b,1);rtw_bridge_set_real_ready(&b,1);return b;
}
void link(rtw_ethernet_bridge& b) {
    CHECK(rtw_bridge_set_associated(&b,1)==RTW_BRIDGE_OK);
    CHECK(rtw_bridge_authorize(&b,1)==RTW_BRIDGE_OK);
}
rtw_bridge_status receive(rtw_ethernet_bridge& b,const std::vector<uint8_t>& usb,
                         Sink& sink,rtw_bridge_rx_stats& stats,size_t scratch_size=1518) {
    std::array<uint8_t,1518> scratch{};
    const auto cb=callbacks(sink);
    return rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch_size,&cb,&stats);
}
void open_data_and_gates() {
    auto b=configured();Sink sink;rtw_bridge_rx_stats stats{};const auto usb=record(arp_mpdu);
    CHECK(!rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_NOT_ASSOCIATED && sink.host.empty());
    CHECK(rtw_bridge_set_associated(&b,1)==RTW_BRIDGE_OK);
    CHECK(!rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_OK && stats.dropped==1 && sink.host.empty());
    CHECK(rtw_bridge_authorize(&b,0)==RTW_BRIDGE_UNAUTHORIZED);
    CHECK(rtw_bridge_authorize(&b,1)==RTW_BRIDGE_OK && rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_OK);
    CHECK(sink.host.size()==1 && sink.host[0]==arp && sink.eapol.empty());
    CHECK(stats.descriptors==1 && stats.host_delivered==1 && stats.bytes_consumed==usb.size());
    CHECK(rtw_bridge_authorize(&b,0)==RTW_BRIDGE_UNAUTHORIZED && !rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_OK && stats.host_delivered==0);
    link(b);
    rtw_bridge_set_admin(&b,0);CHECK(!rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_DOWN && stats.descriptors==0);
    CHECK(rtw_bridge_set_associated(&b,1)==RTW_BRIDGE_DOWN);
    rtw_bridge_set_admin(&b,1);CHECK(!rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_NOT_ASSOCIATED);link(b);
    rtw_bridge_set_real_ready(&b,0);CHECK(!rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_DOWN);
    rtw_bridge_set_real_ready(&b,1);CHECK(!rtw_bridge_carrier(&b));link(b);
    rtw_bridge_disconnect(&b);CHECK(!rtw_bridge_carrier(&b));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_NOT_ASSOCIATED);link(b);
    rtw_bridge_detach(&b);CHECK(!rtw_bridge_carrier(&b));
    rtw_bridge_set_admin(&b,1);rtw_bridge_set_real_ready(&b,1);
    CHECK(rtw_bridge_set_associated(&b,1)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_NOT_ASSOCIATED);
}
void tx_wire_sequence_and_failures() {
    auto b=configured();link(b);
    auto outbound=arp;std::memcpy(outbound.data()+6,sta,6);
    std::array<uint8_t,1600> raw{};size_t written=999;rtw_frame_error error{};
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_OK);
    CHECK(written==60 && raw[0]==0x08 && raw[1]==0x01);
    CHECK(!std::memcmp(raw.data()+4,ap,6) && !std::memcmp(raw.data()+10,sta,6));
    CHECK(!std::memcmp(raw.data()+16,arp.data(),6));
    CHECK(raw[22]==0 && raw[23]==0 && raw[30]==0x08 && raw[31]==0x06);
    CHECK(!std::memcmp(raw.data()+32,arp.data()+14,28));
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),1,&written,&error)==RTW_BRIDGE_FRAME_REJECTED);
    CHECK(written==0 && error==RTW_FRAME_BUFFER_TOO_SMALL);
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_OK);
    CHECK(raw[22]==0x10 && raw[23]==0);
    for(unsigned i=2;i<4096;++i)
        CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_OK);
    CHECK(raw[22]==0xf0 && raw[23]==0xff);
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_OK);
    CHECK(raw[22]==0 && raw[23]==0);
    rtw_bridge_set_admin(&b,0);written=123;
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_DOWN && written==0);
    rtw_bridge_set_admin(&b,1);link(b);
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_OK && raw[22]==0);
    CHECK(rtw_bridge_tx(&b,nullptr,outbound.size(),raw.data(),raw.size(),&written,&error)==RTW_BRIDGE_BAD_ARGUMENT && written==0);
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),outbound.data(),outbound.size(),&written,&error)==RTW_BRIDGE_BAD_ARGUMENT && written==0);
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),nullptr,&error)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_tx(&b,outbound.data(),outbound.size(),raw.data(),raw.size(),&written,nullptr)==RTW_BRIDGE_BAD_ARGUMENT && written==0);
}
void rsn_eapol_is_separate_and_never_carrier() {
    auto b=configured(RTW_PORT_RSN);CHECK(rtw_bridge_set_associated(&b,1)==RTW_BRIDGE_OK);
    Sink sink;rtw_bridge_rx_stats stats{};
    CHECK(receive(b,record(arp_mpdu),sink,stats)==RTW_BRIDGE_OK && sink.host.empty());
    auto ep=arp_mpdu;ep[30]=0x88;ep[31]=0x8e;
    CHECK(receive(b,record(ep),sink,stats)==RTW_BRIDGE_OK);
    auto eth=arp;eth[12]=0x88;eth[13]=0x8e;
    CHECK(sink.eapol.size()==1 && sink.eapol[0]==eth && sink.host.empty());
    std::memcpy(eth.data()+6,sta,6);
    std::array<uint8_t,1600> raw{};size_t n=999;rtw_frame_error err{};
    CHECK(rtw_bridge_tx(&b,eth.data(),eth.size(),raw.data(),raw.size(),&n,&err)==RTW_BRIDGE_OK);
    CHECK(rtw_bridge_authorize(&b,1)==RTW_BRIDGE_OK && !rtw_bridge_carrier(&b));
    CHECK(receive(b,record(ep),sink,stats)==RTW_BRIDGE_OK && sink.eapol.size()==2);
    CHECK(receive(b,record(arp_mpdu),sink,stats)==RTW_BRIDGE_OK && sink.host.empty());
    CHECK(rtw_bridge_tx(&b,eth.data(),eth.size(),raw.data(),raw.size(),&n,&err)==RTW_BRIDGE_NEEDS_ENCRYPTION && n==0);
    auto usb=record(ep);std::array<uint8_t,1518> scratch{};auto cb=callbacks(sink);
    cb.supplicant_eapol=nullptr;
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_NO_CONSUMER);
    CHECK(stats.eapol_delivered==0 && sink.eapol.size()==2 && sink.host.empty());
    // The OPEN path must also keep EAPOL out of host IP input.
    auto open=configured();link(open);Sink open_sink;
    CHECK(receive(open,usb,open_sink,stats)==RTW_BRIDGE_OK && open_sink.host.empty() && open_sink.eapol.size()==1);
    ep[1]|=0x40;
    CHECK(receive(b,record(ep),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1 && sink.eapol.size()==2);
}
void rx_fail_closed() {
    auto b=configured();link(b);Sink sink;rtw_bridge_rx_stats stats{};
    const uint32_t descriptor_flags[]={1U<<14,1U<<15,1U<<20,4U<<20,1U<<27};
    for(auto f:descriptor_flags) {
        CHECK(receive(b,record(arp_mpdu,f),sink,stats)==RTW_BRIDGE_OK);
        CHECK(stats.dropped==1 && sink.host.empty());
    }
    CHECK(receive(b,record({0x01,0x02},0,0,1U<<28),sink,stats)==RTW_BRIDGE_OK);
    CHECK(stats.c2h_skipped==1 && stats.dropped==0 && sink.host.empty());
    CHECK(receive(b,record(arp_mpdu,0,1U<<27),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1);
    CHECK(receive(b,record(arp_mpdu,0,0,1U<<12),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1);
    for(unsigned which=0;which<7;++which) {
        auto m=arp_mpdu;
        switch(which) {
            case 0:m[1]|=0x40;break; // protected (no verification exists)
            case 1:m[10]^=2;break; // foreign BSSID
            case 2:m[1]|=4;break; // More Fragments
            case 3:m[22]|=1;break; // nonzero fragment
            case 4:m[1]|=8;break; // Retry, no duplicate tracking
            case 5:m[0]|=1;break; // protocol version
            default:m[24]=0xab;break; // malformed LLC/SNAP
        }
        CHECK(receive(b,record(m),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1 && sink.host.empty());
    }
    CHECK(receive(b,record(arp_mpdu),sink,stats,13)==RTW_BRIDGE_FRAME_REJECTED);
    CHECK(stats.frame_error==RTW_FRAME_BUFFER_TOO_SMALL && sink.host.empty());
    auto usb=record(arp_mpdu);std::array<uint8_t,1518> scratch{};
    auto cb=callbacks(sink);cb.host_ethernet=nullptr;
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_NO_CONSUMER);
    CHECK(stats.host_delivered==0 && stats.dropped==1 && sink.host.empty());
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),nullptr,&stats)==RTW_BRIDGE_NO_CONSUMER);
    cb=callbacks(sink);
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),usb.data(),usb.size(),&cb,&stats)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_rx(&b,nullptr,usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_rx(nullptr,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),nullptr,scratch.size(),&cb,&stats)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_rx(&b,usb.data(),RTW_BRIDGE_MAX_AGGREGATE_BYTES+1U,scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,nullptr)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_rx(&b,nullptr,0,scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_OK && stats.descriptors==0);
}
void aggregates_are_preflighted_and_callback_failure_stops() {
    auto b=configured();link(b);Sink sink;rtw_bridge_rx_stats stats{};
    auto usb=concat(record(arp_mpdu),record(arp_mpdu));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_OK && sink.host.size()==2 && stats.host_delivered==2);
    sink=Sink{};auto truncated=usb;truncated.resize(usb.size()-20);
    CHECK(receive(b,truncated,sink,stats)==RTW_BRIDGE_MALFORMED_AGGREGATE);
    CHECK(sink.host.empty() && stats.host_delivered==0 && stats.bytes_consumed==0 && stats.aggregate_error==RTW_PACKETS_TRUNCATED);
    auto malformed=record(arp_mpdu);malformed.resize(malformed.size()+24,0);malformed.back()=1;
    CHECK(receive(b,malformed,sink,stats)==RTW_BRIDGE_MALFORMED_AGGREGATE && sink.host.empty());
    sink.fail_at=2;usb=concat(usb,record(arp_mpdu));
    CHECK(receive(b,usb,sink,stats)==RTW_BRIDGE_CONSUMER_FAILED);
    CHECK(sink.host.size()==1 && sink.attempts==2 && stats.host_delivered==1 && stats.descriptors==2 && stats.dropped==1);
    // No plaintext remains in scratch after either accepted or failed callback.
    std::array<uint8_t,1518> scratch{};scratch.fill(0xa5);auto cb=callbacks(sink);sink=Sink{};sink.fail_at=1;
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_CONSUMER_FAILED);
    for(size_t i=0;i<arp.size();++i) CHECK(scratch[i]==0);
}
void fcs_is_explicit() {
    // Independent standard IEEE CRC32 fixture; FCS bytes computed externally.
    auto b=configured(RTW_PORT_OPEN,RTW_BRIDGE_FCS_INCLUDED);link(b);Sink sink;rtw_bridge_rx_stats stats{};
    auto with_fcs=arp_mpdu;with_fcs.insert(with_fcs.end(),{0xfc,0xbd,0xb0,0x6c});
    CHECK(receive(b,record(with_fcs),sink,stats)==RTW_BRIDGE_OK && sink.host.size()==1 && sink.host[0]==arp);
    with_fcs.back()^=0x80;
    CHECK(receive(b,record(with_fcs),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1 && sink.host.size()==1);
    CHECK(receive(b,record(arp_mpdu),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1);
    // Absent means do not guess/strip a matching suffix: those bytes are payload.
    CHECK(rtw_bridge_configure(&b,sta,ap,RTW_PORT_OPEN,RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_OK);link(b);
    with_fcs.back()^=0x80;
    CHECK(receive(b,record(with_fcs),sink,stats)==RTW_BRIDGE_OK && sink.host.size()==2);
    CHECK(sink.host.back().size()==arp.size()+4 && sink.host.back().back()==0x6c);
}
void management_cannot_create_carrier() {
    auto b=configured();Sink sink;rtw_bridge_rx_stats stats{};
    CHECK(receive(b,record(auth_response),sink,stats)==RTW_BRIDGE_OK);
    CHECK(stats.management_delivered==1 && sink.mgmt.size()==1 && sink.mgmt[0]==auth_response);
    CHECK(!rtw_bridge_carrier(&b) && !b.port.associated && !b.port.authorized);
    for(unsigned which=0;which<7;++which) {
        auto m=auth_response;
        switch(which) {
            case 0:m[0]|=1;break;
            case 1:m[1]|=8;break;
            case 2:m[1]|=0x40;break;
            case 3:m[16]^=2;break;
            case 4:m.resize(29);break;
            case 5:m.push_back(1);break; // truncated IE
            default:m[1]|=1;break;
        }
        CHECK(receive(b,record(m),sink,stats)==RTW_BRIDGE_OK && stats.dropped==1 && sink.mgmt.size()==1);
    }
    auto usb=record(auth_response);std::array<uint8_t,1518> scratch{};auto cb=callbacks(sink);cb.management_mpdu=nullptr;
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_OK && stats.dropped==1);
    CHECK(!rtw_bridge_carrier(&b));
    cb=callbacks(sink);sink.fail_at=sink.attempts+1;
    CHECK(rtw_bridge_rx(&b,usb.data(),usb.size(),scratch.data(),scratch.size(),&cb,&stats)==RTW_BRIDGE_CONSUMER_FAILED);
    CHECK(stats.management_delivered==0 && stats.dropped==1 && !rtw_bridge_carrier(&b));
}
void reconfigure_clears_state_even_on_failure_and_aliases() {
    auto b=configured();link(b);
    CHECK(rtw_bridge_configure(&b,b.port.station,b.port.bssid,RTW_PORT_OPEN,RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_OK);
    CHECK(!rtw_bridge_carrier(&b) && !std::memcmp(b.port.station,sta,6));link(b);
    const uint8_t multicast[6]={1,2,3,4,5,6};
    CHECK(rtw_bridge_configure(&b,sta,multicast,RTW_PORT_OPEN,RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(!rtw_bridge_carrier(&b) && !b.port.configured && !b.port.associated && !b.port.authorized);
    CHECK(rtw_bridge_configure(&b,sta,ap,RTW_PORT_OPEN,static_cast<rtw_bridge_fcs>(9))==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(!b.port.configured);
    CHECK(rtw_bridge_configure(&b,sta,sta,RTW_PORT_OPEN,RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_configure(&b,sta,ap,static_cast<rtw_port_security>(9),RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_configure(&b,nullptr,ap,RTW_PORT_OPEN,RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_configure(nullptr,sta,ap,RTW_PORT_OPEN,RTW_BRIDGE_FCS_ABSENT)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_set_associated(nullptr,1)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(rtw_bridge_authorize(nullptr,1)==RTW_BRIDGE_BAD_ARGUMENT);
    CHECK(!rtw_bridge_carrier(nullptr));
    rtw_bridge_reset(nullptr);rtw_bridge_detach(nullptr);rtw_bridge_disconnect(nullptr);
    rtw_bridge_set_admin(nullptr,0);rtw_bridge_set_real_ready(nullptr,0);
    CHECK(std::strlen(rtw_bridge_status_string(RTW_BRIDGE_OK))>0);
}
}
int main() {
    open_data_and_gates();tx_wire_sequence_and_failures();rsn_eapol_is_separate_and_never_carrier();
    rx_fail_closed();aggregates_are_preflighted_and_callback_failure_stops();fcs_is_explicit();
    management_cannot_create_carrier();reconfigure_clears_state_even_on_failure_and_aliases();
    std::printf("ethernet-bridge-tests: %zu assertions, offline raw MPDU only, no hardware\n",checks);
}
