/* SPDX-License-Identifier: GPL-2.0-only */
#include "Transport.hpp"
#include <IOKit/IOReturn.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>
int main() {
    RTWControlRequest request{};request.version=1;
    assert(rtwui::validFrame(rtwui::makeFrame(5,42,16),false));
    for(unsigned mode=0;mode<7;++mode) {
        int pair[2];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));
        rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
        RTWControlChannelMap payload{};payload.version=1;payload.channels[132/8]|=1U<<(132%8);
        if(mode==1)payload.version=2;
        if(mode==2)payload.reserved[2]=1;
        if(mode==3)payload.channels[0]=1;
        if(mode==4)payload.channels[144/8]|=1U<<(144%8);
        auto frame=rtwui::makeFrame(5,mode==5?77:42,sizeof(payload));
        if(mode==6){frame.result=static_cast<uint32_t>(kIOReturnUnsupported);frame.length=0;}
        assert(rtwui::writeExact(pair[1],&frame,sizeof(frame)));
        if(frame.length)assert(rtwui::writeExact(pair[1],&payload,sizeof(payload)));
        RTWControlChannelMap output;std::memset(&output,0xa5,sizeof(output));
        const auto result=rtwui::exchange(pair[0],42,5,&request,sizeof(request),&output,sizeof(output));
        if(!mode)assert(!result&&rtw_control_channel_map_contains(&output,132));
        else {
            assert(result==(mode==6?static_cast<uint32_t>(kIOReturnUnsupported):rtwui::protocolError));
            const RTWControlChannelMap empty{};assert(!std::memcmp(&output,&empty,sizeof(output)));
        }
        close(pair[0]);close(pair[1]);
    }
    std::puts("Actual socket transport: exact map, wrong epoch ID, malformed/reserved/144 payloads, Unsupported fallback and output erasure passed");
}
