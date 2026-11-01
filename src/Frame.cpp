#include "rtw/Frame.hpp"
#include "rtw/FrameWire.h"

#include <algorithm>

namespace rtw {
namespace {
FrameError fromWire(rtw_frame_error error) noexcept {
    return static_cast<FrameError>(static_cast<unsigned>(error));
}
}

const char* toString(FrameError error) noexcept {
    switch (error) {
    case FrameError::None: return "ok";
    case FrameError::NullInput: return "null input";
    case FrameError::TooShort: return "truncated frame";
    case FrameError::TooLarge: return "oversized frame";
    case FrameError::InvalidEtherType: return "not Ethernet II";
    case FrameError::InvalidAddress: return "invalid source or station address";
    case FrameError::WrongDirection: return "not AP to station infrastructure traffic";
    case FrameError::UnexpectedBSSID: return "different BSSID or destination";
    case FrameError::UnsupportedFrame: return "unsupported 802.11 frame subtype";
    case FrameError::UnsupportedEncapsulation: return "unsupported LLC/SNAP payload";
    case FrameError::UnsupportedAggregation: return "A-MSDU is not supported";
    case FrameError::Fragmented: return "fragment assembly is not implemented";
    case FrameError::EncryptedNotVerified: return "encrypted frame was not authenticated/decrypted";
    case FrameError::BufferTooSmall: return "destination buffer is too small";
    }
    return "unknown error";
}

bool isUnicast(const Mac& address) noexcept {
    return !(address[0] & 1U) &&
        std::any_of(address.begin(), address.end(), [](std::uint8_t c) { return c != 0; });
}

FrameResult ethernetToWifi(const std::uint8_t* ethernet, std::size_t length,
                           const Mac& station, const Mac& bssid,
                           std::uint16_t sequence) {
    FrameResult result;
    if (length > 1518) return {FrameError::TooLarge, {}};
    result.bytes.resize(length + 18);
    std::size_t written = 0;
    const auto rc = rtw_ethernet_to_sta_mpdu(ethernet, length, station.data(),
                             bssid.data(), sequence, result.bytes.data(),
                             result.bytes.size(), &written);
    result.error = fromWire(rc);
    if (rc != RTW_FRAME_OK) result.bytes.clear();
    else result.bytes.resize(written);
    return result;
}

FrameResult wifiToEthernet(const std::uint8_t* frame, std::size_t length,
                           const Mac& station, const Mac& bssid,
                           bool verifiedDecrypted) {
    FrameResult result;
    if (length > 4096) return {FrameError::TooLarge, {}};
    result.bytes.resize(length + 14);
    std::size_t written = 0;
    const auto rc = rtw_sta_mpdu_to_ethernet(frame, length, station.data(),
                         bssid.data(), verifiedDecrypted ? 1 : 0,
                         result.bytes.data(), result.bytes.size(), &written);
    result.error = fromWire(rc);
    if (rc != RTW_FRAME_OK) result.bytes.clear();
    else result.bytes.resize(written);
    return result;
}

} // namespace rtw
