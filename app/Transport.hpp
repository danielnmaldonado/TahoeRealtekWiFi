// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include "Protocol.hpp"
#include <atomic>
#include <string>
#include <sys/types.h>

namespace rtwui {
constexpr uint32_t protocolError=0xe1000001,transportError=0xe1000002;
void configureSocket(int fd);
bool readExact(int fd,void* data,size_t length);
bool writeExact(int fd,const void* data,size_t length);
bool matchesPeer(int fd,uid_t uid,pid_t pid);
uint32_t exchange(int fd,uint64_t registryID,uint32_t selector,const void* in,size_t length,void* out,size_t size);
bool makeJoin(const uint8_t* ssid,size_t length,const char* password,size_t passwordLength,RTWControlJoin& out);
int connectHelper(const std::string& path,uid_t uid,pid_t pid,std::string& error);
class Session {
    int listener_=-1;
    std::string directory_;
    void retireEndpoint();
public:
    std::string path;
    std::atomic<bool> cancelled{false};
    ~Session();
    bool create(std::string& error);
    int acceptHelper(const std::string& helperPath,std::string& error);
};
}
