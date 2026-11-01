// SPDX-License-Identifier: GPL-2.0-only
#include "InstanceLock.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/wait.h>
int main() {
    char pattern[]="/tmp/tahoe-lock-test.XXXXXXXX";
    char* parent=mkdtemp(pattern);assert(parent);
    const std::string directory=std::string(parent)+"/private";
    int first=rtwui::acquireInstanceLock(directory.c_str());assert(first>=0);
    assert(rtwui::acquireInstanceLock(directory.c_str())<0);
    pid_t child=fork();assert(child>=0);
    if(child==0) {
        close(first);
        int second=rtwui::acquireInstanceLock(directory.c_str());
        if(second>=0){close(second);_exit(1);}
        _exit(0);
    }
    int status=0;assert(waitpid(child,&status,0)==child);
    assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);
    close(first);
    for(unsigned i=0;i<2000;++i) {
        int fd=rtwui::acquireInstanceLock(directory.c_str());assert(fd>=0);
        assert(fcntl(fd,F_GETFD)&FD_CLOEXEC);close(fd);
    }
    assert(chmod(directory.c_str(),0777)==0);
    assert(rtwui::acquireInstanceLock(directory.c_str())<0);
    assert(chmod(directory.c_str(),0700)==0);
    const std::string lock=directory+"/instance.lock";
    assert(unlink(lock.c_str())==0);
    assert(symlink("/dev/null",lock.c_str())==0);
    assert(rtwui::acquireInstanceLock(directory.c_str())<0);
    assert(unlink(lock.c_str())==0);
    assert(rmdir(directory.c_str())==0);assert(rmdir(parent)==0);
    puts("instance-lock: mutual exclusion, fork, 2000 reopen cycles, symlink and mode checks passed");
}
