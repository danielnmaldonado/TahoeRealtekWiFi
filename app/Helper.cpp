// SPDX-License-Identifier: GPL-2.0-only
#include "Transport.hpp"
#include "Model.hpp"
#include "Devices.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <poll.h>
#include <sys/resource.h>
#include <unistd.h>

namespace {
uint32_t invoke(uint64_t registryID,uint32_t selector,const void* input,size_t n,void* output) {
    if(selector==rtwui::listSelector)return rtwui::listDevices(*static_cast<rtwui::DeviceList*>(output));
    io_connect_t handle=IO_OBJECT_NULL;
    auto error=rtwui::openDevice(registryID,handle);if(error)return error;
    size_t length=rtw_control_output_size(selector);
    error=IOConnectCallStructMethod(handle,selector,input,n,output,&length);
    IOServiceClose(handle);
    if(!error&&(length!=rtw_control_output_size(selector)||!rtwui::validPayload(selector,output)))return rtwui::protocolError;
    return error;
}
bool number(const char* text,unsigned long& value) {
    if (!text || !*text) return false;
    for (const char* p=text;*p;++p) if (*p<'0'||*p>'9') return false;
    errno=0;char* end=nullptr;value=strtoul(text,&end,10);
    return !errno && end && !*end && value>0 && value<=INT_MAX;
}
}
int main(int argc,char** argv) {
    rlimit core{0,0};setrlimit(RLIMIT_CORE,&core);
    unsigned long uid=0,pid=0;
    if (argc!=7 || strcmp(argv[1],"--socket") || strcmp(argv[3],"--uid") ||
        strcmp(argv[5],"--pid") || !number(argv[4],uid) || !number(argv[6],pid)) {
        std::fputs("This private helper is launched by Tahoe Wi-Fi USB.\n",stderr);return 64;
    }
    std::string error;
    int fd=rtwui::connectHelper(argv[2],static_cast<uid_t>(uid),static_cast<pid_t>(pid),error);
    if(fd<0) {std::fputs("Private helper session rejected.\n",stderr);return 77;}
    using Clock=std::chrono::steady_clock;
    const auto started=Clock::now();
    auto lastRequest=started;
    while (Clock::now()-started<std::chrono::hours(12)) {
        pollfd waiting{fd,POLLIN,0};
        const int ready=poll(&waiting,1,1000);
        if (ready<0 && errno==EINTR) continue;
        if (ready<0 || (waiting.revents&(POLLHUP|POLLERR|POLLNVAL))) break;
        if (!ready) {if(Clock::now()-lastRequest>std::chrono::minutes(10))break;continue;}
        lastRequest=Clock::now();
        rtwui::Frame frame{};
        if (!rtwui::readExact(fd,&frame,sizeof(frame)) || !rtwui::validFrame(frame,false)) break;
        uint8_t input[sizeof(RTWControlJoin)]{};
        alignas(rtwui::DeviceList) uint8_t output[sizeof(RTWControlScanResults)]{};
        if (!rtwui::readExact(fd,input,frame.length) ||
            (frame.selector!=rtwui::listSelector&&rtw_control_validate_request(frame.selector,input,frame.length)!=RTW_CONTROL_VALID)) {
            rtwui::wipe(input,sizeof(input));break;
        }
        frame.result=invoke(frame.registryID,frame.selector,input,frame.length,output);
        rtwui::wipe(input,sizeof(input));
        frame.length=frame.result?0:static_cast<uint32_t>(rtwui::outputSize(frame.selector));
        const bool sent=rtwui::writeExact(fd,&frame,sizeof(frame)) &&
            (!frame.length || rtwui::writeExact(fd,output,frame.length));
        rtwui::wipe(output,sizeof(output));
        if (!sent) break;
    }
    close(fd);return 0;
}
