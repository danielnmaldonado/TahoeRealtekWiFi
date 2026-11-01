// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
namespace rtwui {
inline int acquireInstanceLock(const char* directory) {
    if(!directory||!*directory)return -1;
    if(mkdir(directory,0700)&&errno!=EEXIST)return -1;
    struct stat info{};
    if(lstat(directory,&info)||!S_ISDIR(info.st_mode)||info.st_uid!=getuid()||
       (info.st_mode&0077))return -1;
    const int dir=open(directory,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(dir<0)return -1;
    if(fstat(dir,&info)||!S_ISDIR(info.st_mode)||info.st_uid!=getuid()||(info.st_mode&0077)) {
        close(dir);return -1;
    }
    const int fd=openat(dir,"instance.lock",O_CREAT|O_RDWR|O_CLOEXEC|O_NOFOLLOW,0600);
    close(dir);
    if(fd<0)return -1;
    if(fstat(fd,&info)||!S_ISREG(info.st_mode)||info.st_uid!=getuid()||
       info.st_nlink!=1||(info.st_mode&0077)||flock(fd,LOCK_EX|LOCK_NB)) {
        close(fd);return -1;
    }
    // Keep the same inode across launches: never unlink a live lock file.
    return fd;
}
}
