#pragma once

namespace rtw {

// All events must run on a single serialized driver workloop / command gate.
enum class LinkPhase {
    Detached, Idle, Scanning, Authenticating, Associating,
    FourWayHandshake, Online, Suspended
};

enum class LinkEvent {
    USBConnected, BeginScan, SelectBSSID, Authenticated,
    AssociatedOpen, AssociatedSecured, KeysVerified,
    Disconnected, Suspend, Resume, USBDisconnected
};

class LinkState {
public:
    bool dispatch(LinkEvent event) noexcept;
    LinkPhase phase() const noexcept { return phase_; }
    // Link can be advertised to macOS only after association and, if required,
    // verification of the four-way handshake (keys installed, MIC/replay rules).
    bool carrierUp() const noexcept { return phase_ == LinkPhase::Online; }
private:
    LinkPhase phase_ = LinkPhase::Detached;
};

const char* toString(LinkPhase phase) noexcept;

} // namespace rtw
