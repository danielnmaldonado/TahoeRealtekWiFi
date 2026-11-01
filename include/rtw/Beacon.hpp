#pragma once

#include "rtw/Frame.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rtw {

enum class Security {
    Open, LegacyOrUnknown, WPA2Personal, WPA3Personal,
    WPA2WPA3Transition, EnterpriseOrOther
};

struct Beacon {
    Mac bssid{};
    std::vector<std::uint8_t> ssid; // SSIDs are octet strings, not necessarily UTF-8.
    bool ssidPresent = false;
    bool hidden = false;
    std::uint8_t dsChannel = 0; // 0 if absent; channel should come from RX PHY metadata.
    bool privacyCapability = false;
    bool hasRSN = false;
    bool pmfCapable = false;
    bool pmfRequired = false;
    Security security = Security::LegacyOrUnknown;
};

enum class BeaconError { None, NullInput, NotBeaconOrProbeResponse, Truncated, TooLarge, InvalidIE, InvalidRSN };

struct BeaconResult {
    BeaconError error = BeaconError::None;
    Beacon beacon;
    explicit operator bool() const noexcept { return error == BeaconError::None; }
};

// Only parses well-formed beacon/probe response management MPDUs without FCS.
// This is NOT a Wi-Fi scanner; an actual radio RX implementation feeds frames.
BeaconResult parseBeacon(const std::uint8_t* data, std::size_t length);
const char* toString(Security security) noexcept;
const char* toString(BeaconError error) noexcept;

} // namespace rtw
