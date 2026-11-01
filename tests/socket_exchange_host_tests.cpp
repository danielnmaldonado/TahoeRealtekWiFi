// SPDX-License-Identifier: GPL-2.0-only
// Socketpair regression for the actual, extracted rtwui::exchange implementation.
#include "app/Transport.hpp"
#include "app/Model.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

namespace {
struct Connection {
    int ends[2]{-1,-1};
    Connection() { assert(socketpair(AF_UNIX, SOCK_STREAM, 0, ends)==0); }
    ~Connection() { close(ends[0]); close(ends[1]); }
};
RTWControlRequest request() { RTWControlRequest r{};r.version=RTW_CONTROL_VERSION;return r; }
void assertErased(const RTWControlStatus& result) {
    RTWControlStatus empty{};
    assert(std::memcmp(&result,&empty,sizeof(result))==0);
}
void poison(RTWControlStatus& result) { std::memset(&result,0xa5,sizeof(result)); }
}

int main() {
    const auto in=request();
    RTWControlStatus output{};
    {
        Connection s;
        auto frame=rtwui::makeFrame(RTW_CONTROL_STATUS,1234,sizeof(output));
        RTWControlStatus valid{};valid.version=RTW_CONTROL_VERSION;valid.flags=15;
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)));
        assert(rtwui::writeExact(s.ends[1],&valid,sizeof(valid)));
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,sizeof(in),&output,sizeof(output))==0);
        assert(output.flags==15);
    }
    {
        Connection s;
        auto frame=rtwui::makeFrame(RTW_CONTROL_STATUS,1234,sizeof(output));
        RTWControlStatus partial{};partial.version=RTW_CONTROL_VERSION;partial.flags=15;
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)));
        assert(rtwui::writeExact(s.ends[1],&partial,sizeof(partial)/2));
        assert(shutdown(s.ends[1],SHUT_WR)==0);
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,sizeof(in),&output,sizeof(output))==rtwui::transportError);
        assertErased(output);
    }
    {
        Connection s;
        auto frame=rtwui::makeFrame(RTW_CONTROL_STATUS,1234,sizeof(output));
        RTWControlStatus invalid{};invalid.version=99;
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)));
        assert(rtwui::writeExact(s.ends[1],&invalid,sizeof(invalid)));
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,sizeof(in),&output,sizeof(output))==rtwui::protocolError);
        assertErased(output);
    }
    {
        Connection s;
        const auto frame=rtwui::makeFrame(RTW_CONTROL_STATUS,1234,0,0xe1000020U);
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)));
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,sizeof(in),&output,sizeof(output))==0xe1000020U);
        assertErased(output);
    }
    {
        Connection s;
        auto frame=rtwui::makeFrame(RTW_CONTROL_STATUS,9999,sizeof(output));
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)));
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,sizeof(in),&output,sizeof(output))==rtwui::protocolError);
        assertErased(output);
    }
    {
        Connection s;
        auto frame=rtwui::makeFrame(RTW_CONTROL_STATUS,1234,sizeof(output));
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)/2));
        assert(shutdown(s.ends[1],SHUT_WR)==0);
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,sizeof(in),&output,sizeof(output))==rtwui::transportError);
        assertErased(output);
    }
    {
        Connection s;
        auto frame=rtwui::makeFrame(rtwui::listSelector,0,sizeof(rtwui::DeviceList));
        rtwui::DeviceList invalid{};invalid.version=rtwui::ipcVersion;invalid.count=rtwui::maxDevices+1;
        assert(rtwui::writeExact(s.ends[1],&frame,sizeof(frame)));
        assert(rtwui::writeExact(s.ends[1],&invalid,sizeof(invalid)));
        rtwui::DeviceList list;
        std::memset(&list,0xa5,sizeof(list));
        assert(rtwui::exchange(s.ends[0],0,rtwui::listSelector,nullptr,0,&list,sizeof(list))==rtwui::protocolError);
        rtwui::DeviceList empty{};assert(std::memcmp(&list,&empty,sizeof(list))==0);
    }
    // A locally invalid request must not leave the previous successful
    // status visible to a caller that subsequently checks the result buffer.
    {
        Connection s;
        RTWControlRequest malformed{}; // missing the mandatory ABI version
        poison(output);
        assert(rtwui::exchange(s.ends[0], 1234, RTW_CONTROL_STATUS,
               &malformed, sizeof(malformed), &output, sizeof(output)) == rtwui::protocolError);
        assertErased(output);
    }
    // An invalid LIST request must also clear a previously populated list.
    {
        Connection s;
        rtwui::DeviceList list;
        std::memset(&list, 0xa5, sizeof(list));
        assert(rtwui::exchange(s.ends[0], 0, rtwui::listSelector,
               &in, 0, &list, sizeof(list)) == rtwui::protocolError);
        const rtwui::DeviceList empty{};
        assert(std::memcmp(&list, &empty, sizeof(list)) == 0);
    }
    // Even an oversized, unrepresentable request length must clear the result
    // without ever attempting to read that input buffer.
    {
        Connection s;
        poison(output);
        assert(rtwui::exchange(s.ends[0],1234,RTW_CONTROL_STATUS,&in,
               static_cast<size_t>(UINT32_MAX)+1,&output,sizeof(output))==rtwui::protocolError);
        assertErased(output);
    }
    std::puts("socket exchange: 10 real-function scenarios passed");
}
