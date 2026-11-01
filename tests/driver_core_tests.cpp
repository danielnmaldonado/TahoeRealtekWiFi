#include "rtw/USBControl.h"
#include "rtw/USBDescriptors.h"
#include "rtw/StationPort.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace {
unsigned assertions = 0;
#define CHECK(expr) do { ++assertions; if (!(expr)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " " << #expr << '\n'; std::exit(1); } } while (false)

struct FakeDevice {
    rtw_usb_setup last{};
    unsigned requests = 0;
    std::uint32_t value = 0x04411137;
    std::uint16_t actualOverride = 0xffff;
    rtw_usb_error error = RTW_USB_OK;
};

rtw_usb_error fakeControlRead(void* ctx, const rtw_usb_setup* setup,
                              std::uint8_t* out, std::uint16_t* actual) {
    auto& dev = *static_cast<FakeDevice*>(ctx);
    dev.last = *setup;
    ++dev.requests;
    if (dev.error != RTW_USB_OK) return dev.error;
    for (unsigned i=0;i<setup->w_length;++i)
        out[i] = static_cast<std::uint8_t>(dev.value >> (8U*i));
    *actual = dev.actualOverride == 0xffff ? setup->w_length : dev.actualOverride;
    return RTW_USB_OK;
}

void testUSBControl() {
    FakeDevice dev{};
    rtw_usb_transport transport{&dev, fakeControlRead};
    std::uint32_t result = 0;
    CHECK(rtw_usb_probe_syscfg(&transport,&result)==RTW_USB_OK);
    CHECK(result==0x04411137);
    CHECK(dev.requests==1 && dev.last.bm_request_type==0xc0);
    CHECK(dev.last.b_request==0x05 && dev.last.w_value==0xf0);
    CHECK(dev.last.w_index==0 && dev.last.w_length==4);
    CHECK(dev.last.timeout_ms==1000);
    CHECK(rtw_usb_read_register(&transport,0x34,1,&result)==RTW_USB_OK);
    CHECK(result==0x37 && dev.last.w_length==1);
    CHECK(rtw_usb_read_register(&transport,0x34,2,&result)==RTW_USB_OK);
    CHECK(result==0x1137 && dev.last.w_length==2);
    const auto before = dev.requests;
    CHECK(rtw_usb_read_register(&transport,0x23,3,&result)==RTW_USB_INVALID_ARGUMENT);
    CHECK(rtw_usb_read_register(&transport,0xffff,2,&result)==RTW_USB_INVALID_REGISTER);
    CHECK(rtw_usb_read_register(nullptr,0xf0,4,&result)==RTW_USB_INVALID_ARGUMENT);
    CHECK(rtw_usb_read_register(&transport,0xf0,4,nullptr)==RTW_USB_INVALID_ARGUMENT);
    CHECK(dev.requests==before);
    dev.actualOverride=2;
    result=1234;
    CHECK(rtw_usb_probe_syscfg(&transport,&result)==RTW_USB_SHORT_TRANSFER && result==0);
    dev.actualOverride=0xffff;
    dev.error=RTW_USB_IO_ERROR;
    CHECK(rtw_usb_probe_syscfg(&transport,&result)==RTW_USB_IO_ERROR);
    dev.error=RTW_USB_OK;
    dev.value=0;
    CHECK(rtw_usb_probe_syscfg(&transport,&result)==RTW_USB_IMPLAUSIBLE_CHIP && result==0);
    dev.value=0xffffffff;
    CHECK(rtw_usb_probe_syscfg(&transport,&result)==RTW_USB_IMPLAUSIBLE_CHIP);
}

std::vector<std::uint8_t> descriptors() {
    return {
        9,2,39,0,1,1,0,0x80,50,              // configuration
        9,4,0,0,3,0xff,0xff,0xff,0,          // interface 0 alternate 0
        7,5,0x81,2,0,2,0,                   // bulk IN
        7,5,0x02,2,0,2,0,                   // bulk OUT
        7,5,0x83,3,0x40,0,1                // interrupt IN
    };
}
void testEndpoints() {
    const auto data=descriptors();
    rtw_usb_endpoints eps{};
    CHECK(rtw_find_usb_bulk_endpoints(data.data(),data.size(),0,0,&eps)==RTW_DESC_OK);
    CHECK(eps.num_bulk_in==1 && eps.bulk_in[0]==0x81);
    CHECK(eps.num_bulk_out==1 && eps.bulk_out[0]==0x02);
    CHECK(eps.num_endpoints==3);
    CHECK(rtw_find_usb_bulk_endpoints(data.data(),data.size(),0,1,&eps)==RTW_DESC_INTERFACE_NOT_FOUND);
    CHECK(rtw_find_usb_bulk_endpoints(data.data(),data.size(),1,0,&eps)==RTW_DESC_INTERFACE_NOT_FOUND);
    CHECK(rtw_find_usb_bulk_endpoints(data.data(),data.size()-1,0,0,&eps)==RTW_DESC_MALFORMED);
    CHECK(rtw_find_usb_bulk_endpoints(nullptr,0,0,0,&eps)==RTW_DESC_INVALID_ARGUMENT);
    CHECK(rtw_find_usb_bulk_endpoints(data.data(),data.size(),0,0,nullptr)==RTW_DESC_INVALID_ARGUMENT);
    auto broken=data;
    broken[18]=0;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_MALFORMED);
    broken=data;broken[27]=0x81;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_DUPLICATE_ENDPOINT);
    broken=data;broken[21]=3;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_NO_BULK_PAIR);
    broken=data; broken[13]=2;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_MALFORMED);
    broken=data;broken[2]=0xff;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_MALFORMED);
    broken=data;broken[20]=0x00;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_MALFORMED);
    // Ignore unrelated interface descriptors, but detect target with alt setting.
    broken=data;
    broken.insert(broken.end(),{9,4,1,0,1,0xff,0xff,0xff,0, 7,5,0x84,2,0,2,0});
    broken[2]=55;
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),0,0,&eps)==RTW_DESC_OK);
    CHECK(eps.num_bulk_in==1 && eps.num_bulk_out==1);
    CHECK(rtw_find_usb_bulk_endpoints(broken.data(),broken.size(),1,0,&eps)==RTW_DESC_NO_BULK_PAIR);
}

constexpr std::array<std::uint8_t,6> sta{2,0x11,0x22,0x33,0x44,0x55};
constexpr std::array<std::uint8_t,6> ap{2,0xaa,0xbb,0xcc,0xdd,0xee};
constexpr std::array<std::uint8_t,6> remote{2,1,2,3,4,5};

std::vector<std::uint8_t> ethernet(std::uint16_t ethertype) {
    std::vector<std::uint8_t> packet(14+16);
    std::memcpy(packet.data(),remote.data(),6);
    std::memcpy(packet.data()+6,sta.data(),6);
    packet[12]=static_cast<std::uint8_t>(ethertype>>8);
    packet[13]=static_cast<std::uint8_t>(ethertype);
    for(unsigned i=14;i<packet.size();++i) packet[i]=static_cast<std::uint8_t>(i);
    return packet;
}
std::vector<std::uint8_t> fromAP(std::uint16_t ethertype, bool protectedBit) {
    std::vector<std::uint8_t> packet(24+8+16,0);
    packet[0]=0x08;
    packet[1]=static_cast<std::uint8_t>(0x02 | (protectedBit ? 0x40 : 0));
    std::memcpy(packet.data()+4,sta.data(),6);
    std::memcpy(packet.data()+10,ap.data(),6);
    std::memcpy(packet.data()+16,remote.data(),6);
    const std::uint8_t snap[6]={0xaa,0xaa,3,0,0,0};
    std::memcpy(packet.data()+24,snap,6);
    packet[30]=static_cast<std::uint8_t>(ethertype>>8);
    packet[31]=static_cast<std::uint8_t>(ethertype);
    return packet;
}

void testPort() {
    rtw_station_port port{};
    const auto ip=ethernet(0x0800);
    const auto eapol=ethernet(0x888e);
    std::array<std::uint8_t,2048> out{};
    std::size_t written=123;
    rtw_frame_error frameError=RTW_FRAME_OK;
    CHECK(rtw_port_tx(&port,ip.data(),ip.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_NOT_ASSOCIATED);
    CHECK(written==0);
    CHECK(rtw_port_configure(&port,sta.data(),ap.data(),RTW_PORT_OPEN)==RTW_PORT_OK);
    CHECK(!rtw_port_carrier(&port));
    CHECK(rtw_port_associated(&port)==RTW_PORT_OK);
    CHECK(!rtw_port_carrier(&port));
    CHECK(rtw_port_tx(&port,ip.data(),ip.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_UNAUTHORIZED);
    CHECK(rtw_port_authorize(&port,0)==RTW_PORT_OK);
    CHECK(rtw_port_carrier(&port));
    CHECK(rtw_port_tx(&port,ip.data(),ip.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_OK);
    CHECK(written==ip.size()+18 && out[0]==0x08 && out[1]==0x01);
    auto incoming=fromAP(0x0800,false);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),0,out.data(),out.size(),&written,&frameError)==RTW_PORT_OK);
    CHECK(written==14+16);
    incoming=fromAP(0x0800,true);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_UNAUTHORIZED);
    CHECK(written==0);
    // Under WPA2, unauthorized traffic can only carry handshake EtherType.
    CHECK(rtw_port_configure(&port,sta.data(),ap.data(),RTW_PORT_RSN)==RTW_PORT_OK);
    CHECK(!rtw_port_carrier(&port));
    CHECK(rtw_port_associated(&port)==RTW_PORT_OK);
    CHECK(rtw_port_authorize(&port,0)==RTW_PORT_UNAUTHORIZED);
    CHECK(rtw_port_tx(&port,ip.data(),ip.size(),4,out.data(),out.size(),&written,&frameError)==RTW_PORT_UNAUTHORIZED);
    CHECK(rtw_port_tx(&port,eapol.data(),eapol.size(),4,out.data(),out.size(),&written,&frameError)==RTW_PORT_OK);
    CHECK(written>0 && out[1]==1); // unprotected 802.11 ToDS EAPOL
    incoming=fromAP(0x888e,false);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),0,out.data(),out.size(),&written,&frameError)==RTW_PORT_OK);
    CHECK(written==30);
    incoming=fromAP(0x0800,false);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),0,out.data(),out.size(),&written,&frameError)==RTW_PORT_UNAUTHORIZED);
    CHECK(written==0 && out[14]==0);
    incoming=fromAP(0x0800,true);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_UNAUTHORIZED);
    CHECK(rtw_port_authorize(&port,1)==RTW_PORT_OK);
    CHECK(rtw_port_carrier(&port));
    CHECK(rtw_port_tx(&port,ip.data(),ip.size(),4,out.data(),out.size(),&written,&frameError)==RTW_PORT_NEEDS_ENCRYPTION);
    CHECK(written==0);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),0,out.data(),out.size(),&written,&frameError)==RTW_PORT_FRAME_REJECTED);
    CHECK(frameError==RTW_FRAME_ENCRYPTED_NOT_VERIFIED);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_OK);
    CHECK(written==30);
    incoming=fromAP(0x0800,false);
    CHECK(rtw_port_rx(&port,incoming.data(),incoming.size(),0,out.data(),out.size(),&written,&frameError)==RTW_PORT_UNAUTHORIZED);
    CHECK(written==0);
    rtw_port_reset(&port);
    CHECK(!rtw_port_carrier(&port));
    CHECK(rtw_port_tx(&port,ip.data(),ip.size(),1,out.data(),out.size(),&written,&frameError)==RTW_PORT_NOT_ASSOCIATED);
    CHECK(rtw_port_configure(&port,sta.data(),ap.data(),static_cast<rtw_port_security>(55))==RTW_PORT_BAD_ARGUMENT);
    CHECK(!rtw_port_carrier(&port));
    CHECK(rtw_port_configure(&port,sta.data(),sta.data(),RTW_PORT_OPEN)==RTW_PORT_BAD_ARGUMENT);
    CHECK(!rtw_port_carrier(&port));
    CHECK(rtw_port_configure(&port,sta.data(),ap.data(),RTW_PORT_OPEN)==RTW_PORT_OK);
    CHECK(rtw_port_associated(&port)==RTW_PORT_OK);
    CHECK(rtw_port_authorize(&port,0)==RTW_PORT_OK);
    CHECK(rtw_port_carrier(&port));
    CHECK(rtw_port_configure(&port,nullptr,ap.data(),RTW_PORT_OPEN)==RTW_PORT_BAD_ARGUMENT);
    CHECK(!rtw_port_carrier(&port));
}

void testFrameProtocolVersion() {
    std::array<std::uint8_t,64> out{};
    for (const std::uint8_t subtype : {0x08,0x88}) {
        auto incoming=fromAP(0x0800,false);
        incoming[0]=subtype;
        if (subtype==0x88) incoming.insert(incoming.begin()+24,2,0);
        std::size_t written=0;
        CHECK(rtw_sta_mpdu_to_ethernet(incoming.data(),incoming.size(),sta.data(),ap.data(),0,
                                       out.data(),out.size(),&written)==RTW_FRAME_OK);
        CHECK(written==30);
        for (std::uint8_t version=1;version<=3;++version) {
            incoming[0]=static_cast<std::uint8_t>(subtype | version);
            out.fill(0xa5);
            const auto before=out;
            written=123;
            CHECK(rtw_sta_mpdu_to_ethernet(incoming.data(),incoming.size(),sta.data(),ap.data(),0,
                                           out.data(),out.size(),&written)==RTW_FRAME_UNSUPPORTED_FRAME);
            CHECK(written==0 && out==before);
        }
    }
}

void testPortAliasedReconfiguration() {
    for (unsigned aliases=1;aliases<=3;++aliases) {
        rtw_station_port port{};
        CHECK(rtw_port_configure(&port,sta.data(),ap.data(),RTW_PORT_OPEN)==RTW_PORT_OK);
        CHECK(rtw_port_associated(&port)==RTW_PORT_OK);
        CHECK(rtw_port_authorize(&port,0)==RTW_PORT_OK);
        const auto* station=(aliases & 1U) ? port.station : sta.data();
        const auto* bssid=(aliases & 2U) ? port.bssid : ap.data();
        CHECK(rtw_port_configure(&port,station,bssid,RTW_PORT_RSN)==RTW_PORT_OK);
        CHECK(!std::memcmp(port.station,sta.data(),6));
        CHECK(!std::memcmp(port.bssid,ap.data(),6));
        CHECK(port.configured && port.security==RTW_PORT_RSN);
        CHECK(!port.associated && !port.authorized && !rtw_port_carrier(&port));
        CHECK(rtw_port_associated(&port)==RTW_PORT_OK);
        CHECK(rtw_port_authorize(&port,1)==RTW_PORT_OK);
        // Invalid aliased inputs must still clear the previous authorization.
        CHECK(rtw_port_configure(&port,port.station,port.station,RTW_PORT_OPEN)==RTW_PORT_BAD_ARGUMENT);
        const std::array<std::uint8_t,6> zero{};
        CHECK(!port.configured && !port.associated && !port.authorized);
        CHECK(!std::memcmp(port.station,zero.data(),6));
        CHECK(!std::memcmp(port.bssid,zero.data(),6));
        CHECK(!rtw_port_carrier(&port));
    }
}

void testFuzz() {
    std::mt19937 gen(0x881202);
    std::array<std::uint8_t,128> input{};
    rtw_usb_endpoints ep{};
    for (int i=0;i<6000;++i) {
        std::size_t n = gen()% (input.size()+1);
        for (auto& v:input) v=static_cast<std::uint8_t>(gen());
        (void)rtw_find_usb_bulk_endpoints(input.data(),n,0,0,&ep);
    }
    CHECK(true);
}
}
int main() {
    testUSBControl();
    testEndpoints();
    testPort();
    testFrameProtocolVersion();
    testPortAliasedReconfiguration();
    testFuzz();
    std::cout << "PASS: " << assertions << " bridge+USB assertions and 6000 malformed descriptor cases\n";
}
