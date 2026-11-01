// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace rtwui {
inline std::string credentialAccount(const uint8_t* ssid,std::size_t length) {
    if(!ssid||!length||length>32)return {};
    static constexpr char hex[]="0123456789abcdef";
    std::string account="wpa2:";
    account.reserve(5+length*2);
    for(std::size_t i=0;i<length;++i) {
        account.push_back(hex[ssid[i]>>4]);account.push_back(hex[ssid[i]&15]);
    }
    return account;
}
inline int credentialHexDigit(char c) {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    return -1;
}
inline std::vector<uint8_t> credentialSSID(const std::string& account) {
    if(account.compare(0,5,"wpa2:")||account.size()<7||account.size()>69||
       ((account.size()-5)%2))return {};
    std::vector<uint8_t> ssid;
    ssid.reserve((account.size()-5)/2);
    for(std::size_t i=5;i<account.size();i+=2) {
        const int hi=credentialHexDigit(account[i]),lo=credentialHexDigit(account[i+1]);
        if(hi<0||lo<0)return {};
        ssid.push_back(static_cast<uint8_t>((hi<<4)|lo));
    }
    return ssid;
}
// A denied/locked update must never turn into delete+add. Such a fallback
// destroys an existing password or weakens its ACL. Retry only a creation race.
template<class Status,class Update,class Add>
Status upsertCredential(Status notFound,Status duplicate,Update update,Add add) {
    Status status=update();
    if(status==notFound) {
        status=add();
        if(status==duplicate)status=update();
    }
    return status;
}
}
