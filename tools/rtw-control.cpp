/* SPDX-License-Identifier: GPL-2.0-only */
#include "RTWControl.h"

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cerrno>
#include <cstdlib>
#include <limits>

#ifdef __APPLE__
#include <CommonCrypto/CommonCryptoError.h>
#include <CommonCrypto/CommonKeyDerivation.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include "../app/Devices.hpp"
#include <readpassphrase.h>
#include <sys/resource.h>
#endif

namespace {
void usage(FILE* stream) {
    std::fputs("Usage: rtw-control list\n"
               "       rtw-control [--device <registry-ID>] status|scan|networks|leave\n"
               "       rtw-control [--device <registry-ID>] join <SSID>\n"
               "       rtw-control --help\n"
               "JOIN supports WPA2-Personal/CCMP. The passphrase is prompted\n"
               "on the terminal with echo disabled; credentials are not saved.\n"
               "Control commands require administrator access to the loaded driver.\n", stream);
}

enum class Command { List, Status, Scan, Networks, Join, Leave, Help, Invalid };
struct Arguments {Command command=Command::Invalid;uint64_t registryID=0;const char* ssid=nullptr;};

Arguments parse(int argc,char** argv) {
    Arguments out;int first=1;
    if(argc>1&&!std::strcmp(argv[1],"--device")){
        if(argc<4||!argv[2][0])return out;
        for(const char*p=argv[2];*p;p++)if(*p<'0'||*p>'9')return out;
        errno=0;char*end=nullptr;unsigned long long id=std::strtoull(argv[2],&end,10);
        if(errno||!end||*end||!id||id>std::numeric_limits<uint64_t>::max())return out;
        out.registryID=static_cast<uint64_t>(id);first=3;
    }
    const int remaining=argc-first;
    if(remaining==1){
        const char* c=argv[first];
        if(!std::strcmp(c,"list")&&!out.registryID)out.command=Command::List;
        else if(!std::strcmp(c,"status"))out.command=Command::Status;
        else if(!std::strcmp(c,"scan"))out.command=Command::Scan;
        else if(!std::strcmp(c,"networks"))out.command=Command::Networks;
        else if(!std::strcmp(c,"leave"))out.command=Command::Leave;
        else if((!std::strcmp(c,"--help")||!std::strcmp(c,"-h"))&&!out.registryID)out.command=Command::Help;
    }else if(remaining==2&&!std::strcmp(argv[first],"join")){
        const size_t length=std::strlen(argv[first+1]);
        if(length&&length<=RTW_CONTROL_MAX_SSID){out.command=Command::Join;out.ssid=argv[first+1];}
    }
    return out;
}

#ifdef __APPLE__
void wipe(void* memory, size_t size) {
    volatile uint8_t* bytes = static_cast<volatile uint8_t*>(memory);
    while (size--) *bytes++ = 0;
}

struct JoinSecrets {
    /* 64-byte inputs are deliberately retained and rejected, so excessively
     * long input can never be silently truncated to a valid 63-byte password. */
    char password[65]{};
    RTWControlJoin request{};
    ~JoinSecrets() { wipe(this, sizeof(*this)); }
};

struct Connection {
    io_connect_t handle = IO_OBJECT_NULL;
    ~Connection() { if (handle != IO_OBJECT_NULL) IOServiceClose(handle); }
};

int list() {
    rtwui::DeviceList devices{};auto error=rtwui::listDevices(devices);
    if(error){std::fprintf(stderr,"Adapter enumeration failed: 0x%08x%s\n",error,error==uint32_t(kIOReturnNoResources)?" (more than 16 adapters)":"");return 3;}
    for(uint32_t i=0;i<devices.count;i++)std::printf("%llu\t%s\n",static_cast<unsigned long long>(devices.devices[i].registryID),devices.devices[i].family);
    if(!devices.count)std::puts("No matching TahoeRealtekWiFi adapters.");
    return 0;
}
bool connect(Connection& connection,uint64_t id) {
    if(!id){
        rtwui::DeviceList devices{};auto error=rtwui::listDevices(devices);
        if(error){std::fprintf(stderr,"Adapter enumeration failed: 0x%08x\n",error);return false;}
        if(devices.count!=1){std::fputs("Select an adapter: rtw-control list, then --device <registry-ID>.\n",stderr);return false;}
        id=devices.devices[0].registryID;
    }
    const auto opened=rtwui::openDevice(id,connection.handle);
    if(opened){std::fprintf(stderr,"Unable to open selected adapter %llu: 0x%08x\n",static_cast<unsigned long long>(id),opened);return false;}
    return true;
}
bool call(Connection& connection, uint32_t selector, const void* input,
          size_t inputLength, void* output, size_t expectedOutput) {
    size_t outputLength = expectedOutput;
    const kern_return_t result = IOConnectCallStructMethod(connection.handle,
        selector, input, inputLength, output, &outputLength);
    if (result != KERN_SUCCESS) {
        std::fprintf(stderr, "Driver request failed: 0x%08x\n", unsigned(result));
        return false;
    }
    if (outputLength != expectedOutput ||
        rtw_control_load32(static_cast<const uint8_t*>(output)) != RTW_CONTROL_VERSION) {
        std::fputs("Driver returned an incompatible control response.\n", stderr);
        return false;
    }
    return true;
}

void printBytes(const uint8_t* bytes, size_t length) {
    std::fputc('"', stdout);
    for (size_t i = 0; i < length; ++i) {
        const unsigned c = bytes[i];
        if (c >= 0x20 && c <= 0x7e && c != '"' && c != '\\') std::fputc(int(c), stdout);
        else std::printf("\\x%02x", c);
    }
    std::fputc('"', stdout);
}

void printMAC(const uint8_t* bytes) {
    for (unsigned i = 0; i < 6; ++i) std::printf("%s%02x", i ? ":" : "", unsigned(bytes[i]));
}

int status(Connection& connection) {
    RTWControlRequest request{}; request.version = RTW_CONTROL_VERSION;
    RTWControlStatus value{};
    if (!call(connection, RTW_CONTROL_STATUS, &request, sizeof(request), &value, sizeof(value))) return 4;
    if (value.ssidLength > RTW_CONTROL_MAX_SSID ||
        !rtw_control_zeroes(reinterpret_cast<const uint8_t*>(value.reserved), sizeof(value.reserved))) {
        std::fputs("Driver returned malformed status.\n", stderr); return 4;
    }
    std::printf("device=%s radio=%s running=%s authorized=%s scanning=%s\n",
        value.flags & RTW_CONTROL_DEVICE ? "yes" : "no",
        value.flags & RTW_CONTROL_RADIO ? "yes" : "no",
        value.flags & RTW_CONTROL_RUNNING ? "yes" : "no",
        value.flags & RTW_CONTROL_AUTHORIZED ? "yes" : "no",
        value.flags & RTW_CONTROL_SCANNING ? "yes" : "no");
    std::printf("state=%u channel=%u rssi=%d dBm usbError=0x%08x\n",
        value.state, value.channel, value.rssi, value.usbError);
    std::fputs("MAC=", stdout); printMAC(value.mac);
    std::fputs(" BSSID=", stdout); printMAC(value.bssid);
    std::fputs(" SSID=", stdout); printBytes(value.ssid, value.ssidLength);
    std::printf("\nTX packets=%llu errors=%llu RX packets=%llu errors=%llu\n",
        static_cast<unsigned long long>(value.txPackets),
        static_cast<unsigned long long>(value.txErrors),
        static_cast<unsigned long long>(value.rxPackets),
        static_cast<unsigned long long>(value.rxErrors));
    return 0;
}

int networks(Connection& connection) {
    RTWControlRequest request{}; request.version = RTW_CONTROL_VERSION;
    RTWControlScanResults results{};
    if (!call(connection, RTW_CONTROL_SCAN_RESULTS, &request, sizeof(request), &results, sizeof(results))) return 4;
    if (results.count > RTW_CONTROL_MAX_NETWORKS || results.truncated > 1 ||
        !rtw_control_zeroes(reinterpret_cast<const uint8_t*>(results.reserved), sizeof(results.reserved))) {
        std::fputs("Driver returned malformed scan results.\n", stderr); return 4;
    }
    for (uint32_t i = 0; i < results.count; ++i) {
        const RTWControlScanRecord& row = results.records[i];
        if (row.ssidLength > RTW_CONTROL_MAX_SSID || row.reserved) {
            std::fputs("Driver returned a malformed network record.\n", stderr); return 4;
        }
    }
    std::printf("%u networks%s\n", results.count, results.truncated ? " (results truncated)" : "");
    for (uint32_t i = 0; i < results.count; ++i) {
        const RTWControlScanRecord& row = results.records[i];
        printMAC(row.bssid);
        std::printf(" channel=%u rssi=%d security=0x%08x SSID=",
            unsigned(row.channel), int(row.rssi), row.securityFlags);
        printBytes(row.ssid, row.ssidLength); std::fputc('\n', stdout);
    }
    return 0;
}

int simple(Connection& connection, uint32_t selector) {
    RTWControlRequest request{}; request.version = RTW_CONTROL_VERSION;
    RTWControlReply reply{};
    if (!call(connection, selector, &request, sizeof(request), &reply, sizeof(reply))) return 4;
    if (!rtw_control_zeroes(reinterpret_cast<const uint8_t*>(reply.reserved), sizeof(reply.reserved))) {
        std::fputs("Driver returned a malformed response.\n", stderr); return 4;
    }
    std::fputs(selector == RTW_CONTROL_SCAN ? "Scan requested; use networks to read results.\n" : "Leave requested.\n", stdout);
    return 0;
}

int join(Connection& connection, const char* ssid) {
    /* Disallow credential-bearing core files for this process. */
    const rlimit noCore{0, 0};
    if (setrlimit(RLIMIT_CORE, &noCore) != 0) {
        std::fputs("Unable to disable core dumps for passphrase input.\n", stderr); return 5;
    }
    JoinSecrets secrets;
    if (!readpassphrase("WPA2 passphrase: ", secrets.password, sizeof(secrets.password),
                        RPP_ECHO_OFF | RPP_REQUIRE_TTY)) {
        std::fputs("Unable to read passphrase from a terminal.\n", stderr); return 5;
    }
    const size_t passwordLength = std::strlen(secrets.password);
    bool printable = true;
    for (size_t i = 0; i < passwordLength; ++i) {
        const unsigned char c = static_cast<unsigned char>(secrets.password[i]);
        if (c < 0x20 || c > 0x7e) printable = false;
    }
    if (passwordLength < 8 || passwordLength > 63 || !printable) {
        std::fputs("WPA2 passphrase must contain 8 to 63 printable ASCII bytes.\n", stderr); return 2;
    }
    secrets.request.version = RTW_CONTROL_VERSION;
    secrets.request.ssidLength = static_cast<uint32_t>(std::strlen(ssid));
    std::memcpy(secrets.request.ssid, ssid, secrets.request.ssidLength);
    const int derived = CCKeyDerivationPBKDF(kCCPBKDF2, secrets.password,
        passwordLength, secrets.request.ssid, secrets.request.ssidLength,
        kCCPRFHmacAlgSHA1, 4096, secrets.request.pmk, sizeof(secrets.request.pmk));
    wipe(secrets.password, sizeof(secrets.password));
    if (derived != kCCSuccess) {
        std::fputs("Unable to derive the WPA2 key.\n", stderr); return 5;
    }
    RTWControlReply reply{};
    const bool ok = call(connection, RTW_CONTROL_JOIN, &secrets.request,
        sizeof(secrets.request), &reply, sizeof(reply));
    wipe(&secrets.request, sizeof(secrets.request));
    if (!ok) return 4;
    if (!rtw_control_zeroes(reinterpret_cast<const uint8_t*>(reply.reserved), sizeof(reply.reserved))) {
        std::fputs("Driver returned a malformed response.\n", stderr); return 4;
    }
    std::fputs("Join requested; use status to check authorization and link state.\n", stdout);
    return 0;
}
#endif
}

int main(int argc, char** argv) {
    /* Parse completely before service discovery, so help/invalid arguments
     * cannot open a driver or initiate a scan or connection. */
    const Arguments arguments=parse(argc,argv);
    const Command command=arguments.command;
    if (command == Command::Help) { usage(stdout); return 0; }
    if (command == Command::Invalid) { usage(stderr); return 2; }
#ifdef __APPLE__
    if(command==Command::List)return list();
    Connection connection;
    if (!connect(connection,arguments.registryID)) return 3;
    switch (command) {
        case Command::Status: return status(connection);
        case Command::Scan: return simple(connection, RTW_CONTROL_SCAN);
        case Command::Networks: return networks(connection);
        case Command::Join: return join(connection,arguments.ssid);
        case Command::Leave: return simple(connection, RTW_CONTROL_LEAVE);
        default: return 2;
    }
#else
    std::fputs("rtw-control requires macOS and its IOKit control service.\n", stderr);
    return 3;
#endif
}
