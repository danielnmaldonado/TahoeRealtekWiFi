#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace rtw {

using Mac = std::array<std::uint8_t, 6>;

enum class FrameError {
    None,
    NullInput,
    TooShort,
    TooLarge,
    InvalidEtherType,
    InvalidAddress,
    WrongDirection,
    UnexpectedBSSID,
    UnsupportedFrame,
    UnsupportedEncapsulation,
    UnsupportedAggregation,
    Fragmented,
    EncryptedNotVerified,
    BufferTooSmall,
};

const char* toString(FrameError error) noexcept;

struct FrameResult {
    FrameError error = FrameError::None;
    std::vector<std::uint8_t> bytes;
    explicit operator bool() const noexcept { return error == FrameError::None; }
};

// Convert an Ethernet-II frame (without FCS) to an unencrypted 802.11
// infrastructure station -> AP data frame (ToDS=1). Sequence is 12 bits.
// The caller MUST NOT transmit this directly on a protected Wi-Fi link:
// encryption and the complete station/firmware TX path are not implemented.
FrameResult ethernetToWifi(const std::uint8_t* ethernet, std::size_t length,
                           const Mac& station, const Mac& bssid,
                           std::uint16_t sequence);

// Convert a validated, FCS-stripped AP -> station infrastructure data MPDU
// (FromDS=1) to Ethernet-II. 'verifiedDecrypted' must only be true after
// hardware/software has authenticated, decrypted AND checked replay/MIC.
// No A-MSDU, fragmentation, WDS or private encapsulations are supported.
FrameResult wifiToEthernet(const std::uint8_t* frame, std::size_t length,
                           const Mac& station, const Mac& bssid,
                           bool verifiedDecrypted);

bool isUnicast(const Mac& address) noexcept;

} // namespace rtw
