#include "rtw/Beacon.hpp"
#include "rtw/Frame.hpp"
#include "rtw/LinkState.hpp"
#include "rtw/USBIDs.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace {
int assertions = 0;
#define CHECK(x) do { ++assertions; if (!(x)) { std::cerr << "FAIL " << __FILE__ << ':' << __LINE__ << " : " << #x << '\n'; std::exit(1); } } while (0)

constexpr rtw::Mac station{0x02,0x11,0x22,0x33,0x44,0x55};
constexpr rtw::Mac bssid{0x02,0xaa,0xbb,0xcc,0xdd,0xee};
constexpr rtw::Mac foreign{0x02,0x10,0x20,0x30,0x40,0x50};

std::vector<std::uint8_t> ethernet(std::uint16_t ethertype = 0x0800) {
    std::vector<std::uint8_t> x{0xff,0xff,0xff,0xff,0xff,0xff};
    x.insert(x.end(), station.begin(), station.end());
    x.push_back(std::uint8_t(ethertype >> 8));
    x.push_back(std::uint8_t(ethertype & 255));
    x.insert(x.end(), {0x45,0x00,0xab,0xcd});
    return x;
}

std::vector<std::uint8_t> downlink(bool qos = false) {
    auto frame = rtw::ethernetToWifi(ethernet().data(), ethernet().size(), station, bssid, 300);
    CHECK(frame);
    // Reconstruct a genuinely downlink (FromDS) data header, not an
    // incorrectly relabelled uplink frame: addr1=station, addr2=AP, addr3=source.
    auto& b = frame.bytes;
    b[1] = 0x02;
    for (unsigned i=0; i<6; ++i) {
        b[4+i] = station[i]; b[10+i] = bssid[i]; b[16+i] = foreign[i];
    }
    if (qos) {
        b[0] = 0x88;
        b.insert(b.begin() + 24, {0x00, 0x00});
    }
    return b;
}

std::vector<std::uint8_t> beacon() {
    std::vector<std::uint8_t> b(36, 0);
    b[0] = 0x80;
    for (unsigned i=0; i<6; ++i) b[16+i] = bssid[i];
    b[34] = 0x10; // privacy capability
    b.insert(b.end(), {0,4,'t','e','s','t', 3,1,6});
    // RSN version 1, group CCMP, one pairwise CCMP, one AKM PSK, PMF capable
    b.insert(b.end(), {48,20, 1,0,
            0,15,172,4, 1,0, 0,15,172,4,
            1,0, 0,15,172,2, 0x80,0x00});
    return b;
}

void testFrameConversion() {
    const auto eth = ethernet();
    auto wifi = rtw::ethernetToWifi(eth.data(), eth.size(), station, bssid, 0x1234);
    CHECK(wifi);
    CHECK(wifi.bytes.size() == eth.size() + 18);
    CHECK(wifi.bytes[0] == 0x08 && wifi.bytes[1] == 0x01);
    CHECK(wifi.bytes[22] == 0x40 && wifi.bytes[23] == 0x23); // seq 0x234, frag 0
    CHECK(wifi.bytes[24] == 0xaa && wifi.bytes[25] == 0xaa && wifi.bytes[26] == 0x03);
    CHECK(wifi.bytes[30] == 0x08 && wifi.bytes[31] == 0x00);
    CHECK(wifi.bytes[32] == 0x45);
    CHECK(rtw::ethernetToWifi(nullptr, eth.size(), station, bssid, 0).error == rtw::FrameError::NullInput);
    CHECK(rtw::ethernetToWifi(eth.data(), 13, station, bssid, 0).error == rtw::FrameError::TooShort);
    auto oversized = eth;
    oversized.resize(1519,0);
    CHECK(rtw::ethernetToWifi(oversized.data(), oversized.size(), station, bssid, 0).error == rtw::FrameError::TooLarge);
    auto invalid = ethernet(1500); // 802.3 length, not EtherType
    CHECK(rtw::ethernetToWifi(invalid.data(),invalid.size(), station, bssid,0).error == rtw::FrameError::InvalidEtherType);
    invalid = eth; invalid[6] = 0x0a;
    CHECK(rtw::ethernetToWifi(invalid.data(),invalid.size(), station, bssid,0).error == rtw::FrameError::InvalidAddress);
    auto bcast = station; bcast[0] |= 1;
    CHECK(rtw::ethernetToWifi(eth.data(),eth.size(), bcast, bssid,0).error == rtw::FrameError::InvalidAddress);

    const auto down = downlink();
    auto e = rtw::wifiToEthernet(down.data(), down.size(), station, bssid, false);
    CHECK(e);
    CHECK(e.bytes.size() == eth.size());
    CHECK(e.bytes[0] == station[0] && e.bytes[5] == station[5]);
    CHECK(e.bytes[6] == foreign[0] && e.bytes[11] == foreign[5]);
    CHECK(e.bytes[12] == 0x08 && e.bytes[13] == 0x00 && e.bytes[14] == 0x45);

    auto qos = downlink(true);
    CHECK(rtw::wifiToEthernet(qos.data(), qos.size(), station, bssid, false));
    qos[24] = 0x80; // A-MSDU bit
    CHECK(rtw::wifiToEthernet(qos.data(), qos.size(), station, bssid, false).error == rtw::FrameError::UnsupportedAggregation);

    auto encrypted = down;
    encrypted[1] |= 0x40;
    CHECK(rtw::wifiToEthernet(encrypted.data(), encrypted.size(), station, bssid, false).error == rtw::FrameError::EncryptedNotVerified);
    CHECK(rtw::wifiToEthernet(encrypted.data(), encrypted.size(), station, bssid, true));
    encrypted = down; encrypted[1] |= 0x04;
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::Fragmented);
    encrypted = down; encrypted[22] |= 0x01;
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::Fragmented);
    encrypted = down; encrypted[1] = 0x03;
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::WrongDirection);
    encrypted = down; encrypted[1] |= 0x80; // Order/HT Control field unsupported
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::UnsupportedFrame);
    encrypted = down; encrypted[0] = 0x80;
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::UnsupportedFrame);
    encrypted = down; encrypted[10] ^= 0x20;
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::UnexpectedBSSID);
    encrypted = down; encrypted[24] = 0;
    CHECK(rtw::wifiToEthernet(encrypted.data(),encrypted.size(),station,bssid,false).error == rtw::FrameError::UnsupportedEncapsulation);
    CHECK(rtw::wifiToEthernet(nullptr,0,station,bssid,false).error == rtw::FrameError::NullInput);
    CHECK(rtw::wifiToEthernet(down.data(),20,station,bssid,false).error == rtw::FrameError::TooShort);
    auto longframe=down; longframe.resize(4097);
    CHECK(rtw::wifiToEthernet(longframe.data(),longframe.size(),station,bssid,false).error == rtw::FrameError::TooLarge);
}

void testBeacon() {
    const auto b = beacon();
    auto r = rtw::parseBeacon(b.data(),b.size());
    CHECK(r);
    CHECK(r.beacon.ssid.size()==4 && r.beacon.ssid[0]=='t');
    CHECK(r.beacon.dsChannel==6);
    CHECK(r.beacon.security==rtw::Security::WPA2Personal);
    CHECK(r.beacon.pmfCapable && !r.beacon.pmfRequired);
    CHECK(r.beacon.privacyCapability);
    CHECK(r.beacon.bssid == bssid);

    auto malformed=b; malformed.back() = 0xFF; // cap fields still allowed
    CHECK(rtw::parseBeacon(malformed.data(),malformed.size()));
    malformed=b; malformed.pop_back();
    CHECK(rtw::parseBeacon(malformed.data(),malformed.size()).error==rtw::BeaconError::InvalidIE);
    malformed=b; malformed[43]=255; // channel IE length (index 43)
    CHECK(rtw::parseBeacon(malformed.data(),malformed.size()).error==rtw::BeaconError::InvalidIE);
    malformed=b; malformed[46]=17; // malformed RSN length; still enough remaining? 46 = length field
    CHECK(rtw::parseBeacon(malformed.data(),malformed.size()).error != rtw::BeaconError::None);

    auto open = std::vector<std::uint8_t>(36,0);
    open[0] = 0x50; // probe response
    open.insert(open.end(), {0,0}); // hidden
    const auto x=rtw::parseBeacon(open.data(),open.size());
    CHECK(x && x.beacon.hidden && x.beacon.security==rtw::Security::Open);
    open[0]=0x08;
    CHECK(rtw::parseBeacon(open.data(),open.size()).error==rtw::BeaconError::NotBeaconOrProbeResponse);
    CHECK(rtw::parseBeacon(nullptr,0).error==rtw::BeaconError::NullInput);
    CHECK(rtw::parseBeacon(b.data(),35).error==rtw::BeaconError::Truncated);
    auto hugeBeacon = b; hugeBeacon.resize(4097);
    CHECK(rtw::parseBeacon(hugeBeacon.data(),hugeBeacon.size()).error == rtw::BeaconError::TooLarge);
    auto duplicateSSID = b; duplicateSSID.insert(duplicateSSID.end(), {0,1,'x'});
    CHECK(rtw::parseBeacon(duplicateSSID.data(),duplicateSSID.size()).error == rtw::BeaconError::InvalidIE);
    auto duplicateRSN = b; duplicateRSN.insert(duplicateRSN.end(), b.begin()+45, b.end());
    CHECK(rtw::parseBeacon(duplicateRSN.data(),duplicateRSN.size()).error == rtw::BeaconError::InvalidRSN);

    auto transition=b;
    // RSN AKM count 2, insert SAE as second suite after PSK; increment IE length.
    // Locate AKM count (IE beginning at offset 45; RSN body at 47).
    // 47+2 version +4 group +2 pair count +4 pairwise =59, AKM count at 59.
    transition[59]=2;
    transition.insert(transition.end()-2,{0,15,172,8});
    transition[46]=24;
    auto t=rtw::parseBeacon(transition.data(),transition.size());
    CHECK(t && t.beacon.security==rtw::Security::WPA2WPA3Transition);
}

void testLink() {
    rtw::LinkState link;
    CHECK(link.phase() == rtw::LinkPhase::Detached && !link.carrierUp());
    CHECK(!link.dispatch(rtw::LinkEvent::BeginScan));
    CHECK(link.dispatch(rtw::LinkEvent::USBConnected));
    CHECK(link.dispatch(rtw::LinkEvent::BeginScan));
    CHECK(link.dispatch(rtw::LinkEvent::SelectBSSID));
    CHECK(link.dispatch(rtw::LinkEvent::Authenticated));
    CHECK(!link.dispatch(rtw::LinkEvent::KeysVerified));
    CHECK(link.dispatch(rtw::LinkEvent::AssociatedSecured));
    CHECK(!link.carrierUp());
    CHECK(link.dispatch(rtw::LinkEvent::KeysVerified));
    CHECK(link.carrierUp());
    CHECK(link.dispatch(rtw::LinkEvent::Suspend) && !link.carrierUp());
    CHECK(link.dispatch(rtw::LinkEvent::Resume));
    CHECK(link.phase() == rtw::LinkPhase::Idle); // no stale WPA keys after sleep
    CHECK(link.dispatch(rtw::LinkEvent::BeginScan));
    CHECK(link.dispatch(rtw::LinkEvent::SelectBSSID));
    CHECK(link.dispatch(rtw::LinkEvent::Authenticated));
    CHECK(link.dispatch(rtw::LinkEvent::AssociatedOpen));
    CHECK(link.carrierUp());
    CHECK(link.dispatch(rtw::LinkEvent::USBDisconnected));
    CHECK(!link.carrierUp());
    CHECK(!link.dispatch(rtw::LinkEvent::Disconnected));
}

void testUSBIDs() {
    CHECK(rtw::identifyChip(0x0bda,0x8812)==rtw::ChipFamily::RTL8812AU);
    CHECK(rtw::identifyChip(0x0bda,0xb812)==rtw::ChipFamily::UnknownRealtek);
    CHECK(rtw::identifyChip(0x2357,0x0101)==rtw::ChipFamily::NotRealtek);
}

void testUntrustedInputs() {
    // Malformed-frame regression test. ASan/UBSan protect against overreads.
    std::mt19937 rng(0x8812);
    std::vector<std::uint8_t> buf(4098);
    for (unsigned run=0; run<2500; ++run) {
        const auto n=std::size_t(rng() % (buf.size()+1));
        for (std::size_t i=0;i<n;++i) buf[i]=std::uint8_t(rng());
        (void)rtw::wifiToEthernet(buf.data(),n,station,bssid,false);
        (void)rtw::parseBeacon(buf.data(),n);
        if (n<=1518) (void)rtw::ethernetToWifi(buf.data(),n,station,bssid,0);
    }
    CHECK(true);
}
}

int main() {
    testFrameConversion();
    testBeacon();
    testLink();
    testUSBIDs();
    testUntrustedInputs();
    std::cout << "PASS: " << assertions << " assertions and 2500 malformed-frame cases\n";
}
