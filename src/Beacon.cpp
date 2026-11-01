#include "rtw/Beacon.hpp"

#include <algorithm>

namespace rtw {
namespace {
constexpr std::uint8_t rsnOui[] = {0x00, 0x0F, 0xAC};

std::uint16_t little16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8));
}
bool rsnSuite(const std::uint8_t* suite, std::uint8_t type) {
    return std::equal(rsnOui, rsnOui + 3, suite) && suite[3] == type;
}
struct RSNResult {
    bool valid = false;
    bool psk = false;
    bool sae = false;
    bool enterprise = false;
    bool pmfCapable = false;
    bool pmfRequired = false;
};
RSNResult parseRSN(const std::uint8_t* p, std::size_t length) {
    RSNResult result;
    if (length < 2 + 4 + 2 + 4 + 2 + 4) return result;
    std::size_t offset = 0;
    if (little16(p + offset) != 1) return result;
    offset += 2; // RSN version
    offset += 4; // group data cipher suite
    const std::uint16_t pairwiseCount = little16(p + offset);
    offset += 2;
    if (!pairwiseCount || pairwiseCount > (length - offset) / 4) return result;
    offset += std::size_t(pairwiseCount) * 4;
    if (length - offset < 2) return result;
    const std::uint16_t akmCount = little16(p + offset);
    offset += 2;
    if (!akmCount || akmCount > (length - offset) / 4) return result;
    for (std::uint16_t i = 0; i < akmCount; ++i) {
        const auto* suite = p + offset + std::size_t(i) * 4;
        result.psk |= rsnSuite(suite, 2) || rsnSuite(suite, 6);
        result.sae |= rsnSuite(suite, 8) || rsnSuite(suite, 9);
        result.enterprise |= rsnSuite(suite, 1) || rsnSuite(suite, 5);
    }
    offset += std::size_t(akmCount) * 4;
    if (length - offset >= 2) {
        const std::uint16_t caps = little16(p + offset);
        result.pmfRequired = (caps & (1U << 6)) != 0; // MFPR
        result.pmfCapable = (caps & (1U << 7)) != 0; // MFPC
    }
    // Extra optional RSN fields (PMKID list, group management cipher) are
    // not interpreted; unrecognized portions do not mean a usable network.
    result.valid = true;
    return result;
}
}

BeaconResult parseBeacon(const std::uint8_t* data, std::size_t length) {
    if (!data) return {BeaconError::NullInput, {}};
    if (length < 36) return {BeaconError::Truncated, {}};
    if (length > 4096) return {BeaconError::TooLarge, {}};
    const std::uint16_t fc = little16(data);
    const unsigned type = (fc >> 2) & 3;
    const unsigned subtype = (fc >> 4) & 15;
    if (type != 0 || (subtype != 8 && subtype != 5))
        return {BeaconError::NotBeaconOrProbeResponse, {}};
    // A beacon/probe response must be a management frame with no DS bits.
    if ((fc & 0x0300U) != 0) return {BeaconError::NotBeaconOrProbeResponse, {}};
    Beacon out;
    std::copy(data + 16, data + 22, out.bssid.begin());
    out.privacyCapability = (little16(data + 34) & 0x0010U) != 0;

    std::size_t offset = 36;
    while (offset < length) {
        if (length - offset < 2) return {BeaconError::InvalidIE, {}};
        const std::uint8_t id = data[offset];
        const std::size_t ieLength = data[offset + 1];
        offset += 2;
        if (ieLength > length - offset) return {BeaconError::InvalidIE, {}};
        const auto* ie = data + offset;
        if (id == 0) { // SSID
            if (ieLength > 32) return {BeaconError::InvalidIE, {}};
            if (out.ssidPresent) return {BeaconError::InvalidIE, {}};
            {
                out.ssid.assign(ie, ie + ieLength);
                out.ssidPresent = true;
                out.hidden = out.ssid.empty() ||
                    std::all_of(out.ssid.begin(), out.ssid.end(), [](std::uint8_t b) { return b == 0; });
            }
        } else if (id == 3 && ieLength == 1) { // DS parameter set
            out.dsChannel = ie[0];
        } else if (id == 48) { // RSN
            if (out.hasRSN) return {BeaconError::InvalidRSN, {}};
            const auto rsn = parseRSN(ie, ieLength);
            if (!rsn.valid) return {BeaconError::InvalidRSN, {}};
            out.hasRSN = true;
            out.pmfCapable = rsn.pmfCapable;
            out.pmfRequired = rsn.pmfRequired;
            if (rsn.psk && rsn.sae) out.security = Security::WPA2WPA3Transition;
            else if (rsn.sae) out.security = Security::WPA3Personal;
            else if (rsn.psk) out.security = Security::WPA2Personal;
            else out.security = Security::EnterpriseOrOther;
        }
        offset += ieLength;
    }
    if (!out.hasRSN) out.security = out.privacyCapability ? Security::LegacyOrUnknown : Security::Open;
    // A secure RSN network without the privacy bit is inconsistent metadata;
    // don't reinterpret it as open just because a malformed AP advertised it.
    return {BeaconError::None, out};
}

const char* toString(Security security) noexcept {
    switch (security) {
    case Security::Open: return "Open";
    case Security::LegacyOrUnknown: return "Legacy/unknown (not supported)";
    case Security::WPA2Personal: return "WPA2-Personal (detected, not implemented)";
    case Security::WPA3Personal: return "WPA3-Personal (detected, not implemented)";
    case Security::WPA2WPA3Transition: return "WPA2/WPA3 transition (detected, not implemented)";
    case Security::EnterpriseOrOther: return "Enterprise/other (not implemented)";
    }
    return "Unknown";
}

const char* toString(BeaconError error) noexcept {
    switch (error) {
    case BeaconError::None: return "ok";
    case BeaconError::NullInput: return "null input";
    case BeaconError::NotBeaconOrProbeResponse: return "not a beacon/probe response";
    case BeaconError::Truncated: return "truncated management frame";
    case BeaconError::TooLarge: return "oversized management frame";
    case BeaconError::InvalidIE: return "malformed information element";
    case BeaconError::InvalidRSN: return "malformed RSN element";
    }
    return "unknown error";
}

} // namespace rtw
