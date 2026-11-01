#pragma once

#include <cstdint>

namespace rtw {

enum class ChipFamily { RTL8812AU, RTL8192EU_Candidate, UnknownRealtek, NotRealtek };
ChipFamily identifyChip(std::uint16_t vendor, std::uint16_t product) noexcept;
const char* toString(ChipFamily chip) noexcept;

} // namespace rtw
