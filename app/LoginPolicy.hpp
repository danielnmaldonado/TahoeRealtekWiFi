// SPDX-License-Identifier: GPL-2.0-only
#pragma once
namespace rtwui {
enum class LoginAction { Enable, Disable, OpenSettings };
inline LoginAction loginToggleAction(bool enabled,bool requiresApproval) {
    if(requiresApproval)return LoginAction::OpenSettings;
    return enabled?LoginAction::Disable:LoginAction::Enable;
}
inline bool loginRegistrationConfirmed(bool enabled,bool requiresApproval) {
    return enabled||requiresApproval;
}
}
