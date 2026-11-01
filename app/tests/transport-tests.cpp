// SPDX-License-Identifier: GPL-2.0-only
#include "Transport.hpp"
#include "Model.hpp"
#include <CommonCrypto/CommonKeyDerivation.h>
#include <IOKit/IOReturn.h>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <thread>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

int main() {
    RTWControlJoin join{};
    assert(rtwui::makeJoin(reinterpret_cast<const uint8_t*>("IEEE"),4,"password",8,join));
    const uint8_t pmk[]={0xf4,0x2c,0x6f,0xc5,0x2d,0xf0,0xeb,0xef,0x9e,0xbb,0x4b,0x90,0xb3,0x8a,0x5f,0x90,0x2e,0x83,0xfe,0x1b,0x13,0x5a,0x70,0xe2,0x3a,0xed,0x76,0x2e,0x97,0x10,0xa1,0x2e};
    assert(!memcmp(join.pmk,pmk,32));
    assert(!rtwui::makeJoin(reinterpret_cast<const uint8_t*>("IEEE"),4,"short",5,join));
    const RTWControlJoin zero{};assert(!memcmp(&join,&zero,sizeof(join)));
    int pair[2];assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));
    rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
    assert(rtwui::matchesPeer(pair[0],getuid(),getpid()));
    assert(!rtwui::matchesPeer(pair[0],getuid()+1,getpid()));
    assert(!rtwui::matchesPeer(pair[0],getuid(),getpid()+1));
    std::thread responder([&]{
        rtwui::Frame request{};assert(rtwui::readExact(pair[1],&request,sizeof(request)));
        assert(request.selector==0 && request.length==16 && request.registryID==1234 && request.version==2);
        RTWControlRequest in{};assert(rtwui::readExact(pair[1],&in,sizeof(in)));assert(in.version==1);
        auto reply=rtwui::makeFrame(0,1234,120);
        RTWControlStatus status{};status.version=1;status.flags=15;
        // Bytewise writes exercise stream fragmentation, not packet assumptions.
        auto* p=reinterpret_cast<uint8_t*>(&reply);
        for (size_t i=0;i<sizeof(reply);++i) assert(rtwui::writeExact(pair[1],p+i,1));
        assert(rtwui::writeExact(pair[1],&status,sizeof(status)));
        close(pair[1]);
    });
    RTWControlRequest request{};request.version=1;RTWControlStatus status{};
    assert(rtwui::exchange(pair[0],1234,0,&request,sizeof(request),&status,sizeof(status))==0);
    assert(status.flags==15);
    responder.join();close(pair[0]);
    // A truncated reply can write some untrusted bytes into an output buffer
    // before readExact observes EOF. Returning an error must erase all bytes.
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));
    rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
    auto partialFrame=rtwui::makeFrame(0,1234,sizeof(status));
    RTWControlStatus partialStatus{};partialStatus.version=1;partialStatus.flags=15;
    assert(rtwui::writeExact(pair[1],&partialFrame,sizeof(partialFrame)));
    assert(rtwui::writeExact(pair[1],&partialStatus,sizeof(partialStatus)/2));
    assert(!shutdown(pair[1],SHUT_WR));
    memset(&status,0xa5,sizeof(status));
    assert(rtwui::exchange(pair[0],1234,0,&request,sizeof(request),&status,sizeof(status))==rtwui::transportError);
    RTWControlStatus cleared{};
    assert(!memcmp(&status,&cleared,sizeof(status)));
    close(pair[0]);close(pair[1]);
    // Semantic protocol failures must never expose the rejected payload.
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));
    rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
    partialStatus.version=42;
    assert(rtwui::writeExact(pair[1],&partialFrame,sizeof(partialFrame)));
    assert(rtwui::writeExact(pair[1],&partialStatus,sizeof(partialStatus)));
    memset(&status,0xa5,sizeof(status));
    assert(rtwui::exchange(pair[0],1234,0,&request,sizeof(request),&status,sizeof(status))==rtwui::protocolError);
    assert(!memcmp(&status,&cleared,sizeof(status)));
    close(pair[0]);close(pair[1]);
    // A declared remote error carries no payload and also clears prior status.
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));
    rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
    auto remoteError=rtwui::makeFrame(0,1234,0,static_cast<uint32_t>(kIOReturnNotReady));
    assert(rtwui::writeExact(pair[1],&remoteError,sizeof(remoteError)));
    memset(&status,0xa5,sizeof(status));
    assert(rtwui::exchange(pair[0],1234,0,&request,sizeof(request),&status,sizeof(status))==uint32_t(kIOReturnNotReady));
    assert(!memcmp(&status,&cleared,sizeof(status)));
    close(pair[0]);close(pair[1]);
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));
    auto bad=rtwui::makeFrame(0,1234,3096);
    assert(rtwui::writeExact(pair[1],&bad,sizeof(bad)));
    assert(rtwui::exchange(pair[0],1234,0,&request,sizeof(request),&status,sizeof(status))==rtwui::protocolError);
    close(pair[0]);close(pair[1]);
    // A response from another registry ID can never satisfy this request.
    for(unsigned mode=0;mode<2;mode++){
        assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
        auto wrong=rtwui::makeFrame(0,mode?1234:5678,120);if(mode)wrong.version=1;
        assert(rtwui::writeExact(pair[1],&wrong,sizeof(wrong)));
        assert(rtwui::exchange(pair[0],1234,0,&request,sizeof(request),&status,sizeof(status))==rtwui::protocolError);
        close(pair[0]);close(pair[1]);
    }
    assert(!socketpair(AF_UNIX,SOCK_STREAM,0,pair));rtwui::configureSocket(pair[0]);rtwui::configureSocket(pair[1]);
    rtwui::DeviceList devices{};devices.version=2;devices.count=2;devices.devices[0].registryID=1234;devices.devices[1].registryID=5678;
    auto listed=rtwui::makeFrame(rtwui::listSelector,0,sizeof(devices));
    assert(rtwui::writeExact(pair[1],&listed,sizeof(listed))&&rtwui::writeExact(pair[1],&devices,sizeof(devices)));
    rtwui::DeviceList received{};assert(!rtwui::exchange(pair[0],0,rtwui::listSelector,nullptr,0,&received,sizeof(received)));
    assert(received.count==2&&received.devices[1].registryID==5678);close(pair[0]);close(pair[1]);
    assert(rtwui::exchange(-1,1234,99,&request,sizeof(request),&status,sizeof(status))==rtwui::protocolError);
    // Local validation failures must clear stale response data too.
    RTWControlRequest invalidRequest{}; // zero version is never valid
    memset(&status,0xa5,sizeof(status));
    assert(rtwui::exchange(-1,1234,RTW_CONTROL_STATUS,&invalidRequest,
        sizeof(invalidRequest),&status,sizeof(status))==rtwui::protocolError);
    assert(!memcmp(&status,&cleared,sizeof(status)));
    rtwui::DeviceList staleList{};
    memset(&staleList,0xa5,sizeof(staleList));
    assert(rtwui::exchange(-1,0,rtwui::listSelector,&request,0,&staleList,
        sizeof(staleList))==rtwui::protocolError);
    const rtwui::DeviceList emptyList{};
    assert(!memcmp(&staleList,&emptyList,sizeof(staleList)));
    memset(&status,0xa5,sizeof(status));
    assert(rtwui::exchange(-1,1234,RTW_CONTROL_STATUS,&request,
        static_cast<size_t>(UINT32_MAX)+1,&status,sizeof(status))==rtwui::protocolError);
    assert(!memcmp(&status,&cleared,sizeof(status)));
    std::string error;
    assert(rtwui::connectHelper("/tmp/not-our-socket",getuid(),getpid(),error)==-1);
    assert(!error.empty());
    rtwui::Session repeated;
    for (unsigned i=0;i<128;++i) {
        const auto previous=repeated.path;
        assert(repeated.create(error));
        if(!previous.empty()) {
            struct stat retired{};
            assert(lstat(previous.c_str(),&retired)==-1 && errno==ENOENT);
            const auto oldDirectory=previous.substr(0,previous.find_last_of('/'));
            assert(lstat(oldDirectory.c_str(),&retired)==-1 && errno==ENOENT);
        }
    }
    rtwui::Session cancelled;assert(cancelled.create(error));
    const auto path=cancelled.path;
    struct stat st{};assert(!lstat(path.c_str(),&st));assert((st.st_mode&0777)==0600);
    cancelled.cancelled=true;
    assert(cancelled.acceptHelper("/usr/bin/true",error)==-1);
    assert(lstat(path.c_str(),&st)==-1);
    std::puts("transport/crypto: passed");
}
