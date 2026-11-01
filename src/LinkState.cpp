#include "rtw/LinkState.hpp"

namespace rtw {

bool LinkState::dispatch(LinkEvent event) noexcept {
    if (event == LinkEvent::USBDisconnected) {
        phase_ = LinkPhase::Detached;
        return true;
    }
    if (event == LinkEvent::Disconnected && phase_ != LinkPhase::Detached) {
        phase_ = LinkPhase::Idle;
        return true;
    }
    if (event == LinkEvent::Suspend && phase_ != LinkPhase::Detached && phase_ != LinkPhase::Suspended) {
        phase_ = LinkPhase::Suspended;
        return true;
    }
    switch (phase_) {
    case LinkPhase::Detached:
        if (event == LinkEvent::USBConnected) { phase_ = LinkPhase::Idle; return true; }
        break;
    case LinkPhase::Idle:
        if (event == LinkEvent::BeginScan) { phase_ = LinkPhase::Scanning; return true; }
        break;
    case LinkPhase::Scanning:
        if (event == LinkEvent::SelectBSSID) { phase_ = LinkPhase::Authenticating; return true; }
        break;
    case LinkPhase::Authenticating:
        if (event == LinkEvent::Authenticated) { phase_ = LinkPhase::Associating; return true; }
        break;
    case LinkPhase::Associating:
        if (event == LinkEvent::AssociatedOpen) { phase_ = LinkPhase::Online; return true; }
        if (event == LinkEvent::AssociatedSecured) { phase_ = LinkPhase::FourWayHandshake; return true; }
        break;
    case LinkPhase::FourWayHandshake:
        if (event == LinkEvent::KeysVerified) { phase_ = LinkPhase::Online; return true; }
        break;
    case LinkPhase::Online:
        // Reassociation is handled by disconnect -> scan to avoid stale keys.
        break;
    case LinkPhase::Suspended:
        if (event == LinkEvent::Resume) { phase_ = LinkPhase::Idle; return true; }
        break;
    }
    return false;
}

const char* toString(LinkPhase phase) noexcept {
    switch (phase) {
    case LinkPhase::Detached: return "detached";
    case LinkPhase::Idle: return "idle";
    case LinkPhase::Scanning: return "scanning";
    case LinkPhase::Authenticating: return "authenticating";
    case LinkPhase::Associating: return "associating";
    case LinkPhase::FourWayHandshake: return "four-way handshake";
    case LinkPhase::Online: return "online";
    case LinkPhase::Suspended: return "suspended";
    }
    return "unknown";
}

} // namespace rtw
