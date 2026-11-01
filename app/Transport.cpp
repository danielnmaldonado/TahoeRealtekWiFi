// SPDX-License-Identifier: GPL-2.0-only
#include "Transport.hpp"
#include "Model.hpp"
#include <CommonCrypto/CommonKeyDerivation.h>
#include <CommonCrypto/CommonCryptoError.h>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <fcntl.h>
#include <libproc.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace rtwui {
void configureSocket(int fd) {
    int yes=1;setsockopt(fd,SOL_SOCKET,SO_NOSIGPIPE,&yes,sizeof(yes));
    timeval timeout{5,0};setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
    fcntl(fd,F_SETFD,FD_CLOEXEC);
}
bool readExact(int fd,void* p,size_t n) {
    auto* b=static_cast<uint8_t*>(p);
    while(n) {ssize_t got=recv(fd,b,n,0);if(got<0 && errno==EINTR)continue;if(got<=0)return false;b+=got;n-=got;}
    return true;
}
bool writeExact(int fd,const void* p,size_t n) {
    auto* b=static_cast<const uint8_t*>(p);
    while(n) {ssize_t sent=send(fd,b,n,0);if(sent<0 && errno==EINTR)continue;if(sent<=0)return false;b+=sent;n-=sent;}
    return true;
}
uint32_t exchange(int fd,uint64_t registryID,uint32_t selector,const void* in,size_t n,void* out,size_t cap) {
    if(!out || cap!=outputSize(selector))return protocolError;
    if(n>UINT32_MAX) {wipe(out,cap);return protocolError;}
    Frame f=makeFrame(selector,registryID,static_cast<uint32_t>(n));
    if(!validFrame(f,false)||
       (selector==listSelector?(in||n):rtw_control_validate_request(selector,in,n)!=RTW_CONTROL_VALID)) {
        // Local protocol errors must obey the same no-stale-output guarantee
        // as truncated replies and remote protocol errors.
        wipe(out,cap);
        return protocolError;
    }
    // An unsuccessful exchange must not expose a stale or partially received
    // status/device list to its caller. Preserve input/output aliasing until
    // the outgoing request has been sent, then clear the response buffer.
    if(!writeExact(fd,&f,sizeof(f))||!writeExact(fd,in,n)) {
        wipe(out,cap);
        return transportError;
    }
    wipe(out,cap);
    if(!readExact(fd,&f,sizeof(f)))return transportError;
    if(!validFrame(f,true)||f.selector!=selector||f.registryID!=registryID)return protocolError;
    if(f.result)return f.result;
    if(!readExact(fd,out,cap)) {
        wipe(out,cap);
        return transportError;
    }
    if(!validPayload(selector,out)) {
        wipe(out,cap);
        return protocolError;
    }
    return 0;
}
bool makeJoin(const uint8_t* ssid,size_t n,const char* password,size_t pn,RTWControlJoin& out) {
    wipe(&out,sizeof(out));
    if (!ssid || !n || n>32 || !validPassword(password,pn)) return false;
    out.version=1;out.ssidLength=static_cast<uint32_t>(n);memcpy(out.ssid,ssid,n);
    if (CCKeyDerivationPBKDF(kCCPBKDF2,password,pn,ssid,n,kCCPRFHmacAlgSHA1,4096,out.pmk,32)!=kCCSuccess) {
        wipe(&out,sizeof(out));return false;
    }
    return true;
}
bool matchesPeer(int fd,uid_t uid,pid_t pid) {
    uid_t actual;gid_t group;pid_t process=0;socklen_t n=sizeof(process);
    return !getpeereid(fd,&actual,&group) && actual==uid &&
        !getsockopt(fd,SOL_LOCAL,LOCAL_PEERPID,&process,&n) && n==sizeof(process) && process==pid;
}
static sockaddr_un address(const std::string& path) {
    sockaddr_un a{};a.sun_family=AF_UNIX;a.sun_len=sizeof(a);
    if (path.size()<sizeof(a.sun_path)) memcpy(a.sun_path,path.c_str(),path.size()+1);
    return a;
}
Session::~Session() {retireEndpoint();}
void Session::retireEndpoint() {
    if (listener_>=0) {close(listener_);listener_=-1;}
    if (!path.empty()) unlink(path.c_str());
    if (!directory_.empty()) rmdir(directory_.c_str());
    path.clear();directory_.clear();
}
bool Session::create(std::string& error) {
    retireEndpoint();cancelled=false;
    if (geteuid()==0) {error="Run the app as a normal user.";return false;}
    char pattern[]="/private/tmp/tahoe-wifi.XXXXXXXX";
    char* dir=mkdtemp(pattern);
    if (!dir) {error="Could not create the private channel.";return false;}
    directory_=dir;
    if(chmod(dir,0700)) {error="Could not protect the private channel.";retireEndpoint();return false;}
    path=directory_+"/control.sock";
    listener_=socket(AF_UNIX,SOCK_STREAM,0);
    if (listener_<0) {error="Could not create the socket.";retireEndpoint();return false;}
    configureSocket(listener_);auto a=address(path);
    if (bind(listener_,reinterpret_cast<sockaddr*>(&a),sizeof(a)) || chmod(path.c_str(),0600) || listen(listener_,1)) {
        error="Could not open the private channel.";retireEndpoint();return false;
    }
    return true;
}
int Session::acceptHelper(const std::string& helperPath,std::string& error) {
    char expected[PATH_MAX];
    if (!realpath(helperPath.c_str(),expected)) {error="The helper is missing from the app.";retireEndpoint();return -1;}
    for (int i=0;i<900 && !cancelled;++i) {
        pollfd p{listener_,POLLIN,0};const int ready=poll(&p,1,200);
        if (ready<0 && errno==EINTR) continue;
        if (ready<0) break;
        if (!ready) continue;
        const int fd=accept(listener_,nullptr,nullptr);
        if (fd<0) continue;
        configureSocket(fd);
        uid_t uid;gid_t group;pid_t pid=0;socklen_t n=sizeof(pid);char actual[PROC_PIDPATHINFO_MAXSIZE]{};
        if (!getpeereid(fd,&uid,&group) && uid==0 &&
            !getsockopt(fd,SOL_LOCAL,LOCAL_PEERPID,&pid,&n) && n==sizeof(pid) && pid>1 &&
            proc_pidpath(pid,actual,sizeof(actual))>0 && !strcmp(actual,expected)) {retireEndpoint();return fd;}
        close(fd);
    }
    error=cancelled?"Authorization canceled. Choose Authorize access to try again.":"Authorization timed out. Try again.";
    retireEndpoint();
    return -1;
}
int connectHelper(const std::string& path,uid_t uid,pid_t pid,std::string& error) {
    if (geteuid()!=0 || !uid || pid<=1 || path.rfind("/private/tmp/tahoe-wifi.",0)!=0 ||
        path.size()>=sizeof(sockaddr_un::sun_path) || path.substr(path.find_last_of('/'))!="/control.sock") {
        error="Invalid helper invocation";return -1;
    }
    const auto parent=path.substr(0,path.find_last_of('/'));struct stat dir{},sock{};
    if (lstat(parent.c_str(),&dir) || !S_ISDIR(dir.st_mode) || dir.st_uid!=uid || (dir.st_mode&0777)!=0700 ||
        lstat(path.c_str(),&sock) || !S_ISSOCK(sock.st_mode) || sock.st_uid!=uid || (sock.st_mode&0777)!=0600) {
        error="Invalid private endpoint";return -1;
    }
    int fd=socket(AF_UNIX,SOCK_STREAM,0);
    if (fd<0) {error="Socket failed";return -1;}
    configureSocket(fd);auto a=address(path);
    if (connect(fd,reinterpret_cast<sockaddr*>(&a),sizeof(a)) || !matchesPeer(fd,uid,pid)) {
        close(fd);error="Peer rejected";return -1;
    }
    return fd;
}
}
