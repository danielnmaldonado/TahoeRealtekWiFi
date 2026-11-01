#include "rtw/USBIDs.hpp"

namespace rtw {
ChipFamily identifyChip(std::uint16_t vendor, std::uint16_t product) noexcept {
    // TP-Link USB VID 2357 / PID 0108 is associated with the RTL8192EU.
    // The exact chip cut/bonding must still be read from hardware registers.
    if (vendor == 0x2357 && product == 0x0108)
        return ChipFamily::RTL8192EU_Candidate;
    if (vendor != 0x0BDA) return ChipFamily::NotRealtek;
    if (product == 0x8812) return ChipFamily::RTL8812AU;
    return ChipFamily::UnknownRealtek;
}
const char* toString(ChipFamily chip) noexcept {
    switch (chip) {
    case ChipFamily::RTL8812AU: return "RTL8812AU (first research target)";
    case ChipFamily::RTL8192EU_Candidate:
        return "RTL8192EU (2357:0108, verify cut/bonding by read-only probe)";
    case ChipFamily::UnknownRealtek: return "Realtek; chipset requires verification";
    case ChipFamily::NotRealtek: return "not a directly recognized Realtek USB VID";
    }
    return "unknown";
}
} // namespace rtw
