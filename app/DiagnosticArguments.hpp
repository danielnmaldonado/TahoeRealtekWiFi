// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cstdint>
#include <cstring>

namespace rtwui {
enum class DiagnosticMode { NotRequested, Valid, Invalid };
struct DiagnosticArguments {
    DiagnosticMode mode=DiagnosticMode::NotRequested;
    uint64_t registryID=0;
    bool scan=false;
};
inline DiagnosticArguments parseDiagnosticArguments(int argc,const char* const* argv) {
    bool requested=false;
    for(int i=1;i<argc;++i)if(argv[i]&&!std::strcmp(argv[i],"--diagnose-adapter"))requested=true;
    if(!requested)return {};
    DiagnosticArguments out{DiagnosticMode::Invalid,0,false};
    if((argc!=3&&argc!=4)||!argv[1]||std::strcmp(argv[1],"--diagnose-adapter")||!argv[2])return out;
    if(argc==4&&(!argv[3]||std::strcmp(argv[3],"--scan")))return out;
    const char* digits=argv[2];unsigned base=10;
    if(digits[0]=='0'&&digits[1]=='x'){base=16;digits+=2;}
    if(!*digits)return out;
    uint64_t value=0;
    for(;*digits;++digits){
        unsigned digit=*digits>='0'&&*digits<='9'?unsigned(*digits-'0'):
            *digits>='a'&&*digits<='f'?unsigned(*digits-'a'+10):
            *digits>='A'&&*digits<='F'?unsigned(*digits-'A'+10):base;
        if(digit>=base||value>(UINT64_MAX-digit)/base)return out;
        value=value*base+digit;
    }
    if(!value)return out;
    return {DiagnosticMode::Valid,value,argc==4};
}
}
