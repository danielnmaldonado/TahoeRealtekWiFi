// SPDX-License-Identifier: GPL-2.0-only
// Exercise the actual JSONL writer in a fresh process so an unexpected
// filesystem block is caught by a deadline rather than hanging the app suite.
#import "../ControlDiagnostics.h"
#include "../Protocol.hpp"
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern char** environ;

static bool concurrentTimestamps() {
    constexpr size_t count=256;
    NSMutableArray<NSString*>* values=[NSMutableArray arrayWithCapacity:count];
    for(size_t i=0;i<count;++i)[values addObject:@""];
    NSDate* before=NSDate.date;
    dispatch_apply(count,dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^(size_t index) {
        @autoreleasepool {
            NSDictionary* record=RTWControlExchangeRecord(index+1,RTW_CONTROL_STATUS,0,1,NO);
            @synchronized(values) {values[index]=record[@"timestamp"]?:@"";}
        }
    });
    NSDate* after=NSDate.date;
    NSISO8601DateFormatter* parser=[NSISO8601DateFormatter new];
    parser.formatOptions=NSISO8601DateFormatWithInternetDateTime|NSISO8601DateFormatWithFractionalSeconds;
    for(NSString* value in values) {
        NSDate* parsed=[parser dateFromString:value];
        // Millisecond formatting can round either boundary by one millisecond.
        if(!parsed||[parsed timeIntervalSinceDate:before]<-0.002||
           [parsed timeIntervalSinceDate:after]>0.002)return false;
    }
    return true;
}

static bool appendBeforeDeadline(const char* executable,NSString* directory,bool expected) {
    char* arguments[]={const_cast<char*>(executable),const_cast<char*>("--append"),
        const_cast<char*>(directory.fileSystemRepresentation),const_cast<char*>(expected?"success":"skip"),nullptr};
    pid_t child=0;
    if(posix_spawn(&child,executable,nullptr,nullptr,arguments,environ))return false;
    const double deadline=NSProcessInfo.processInfo.systemUptime+1.0;
    int status=0;
    while(NSProcessInfo.processInfo.systemUptime<deadline){
        const pid_t result=waitpid(child,&status,WNOHANG);
        if(result==child)return WIFEXITED(status)&&WEXITSTATUS(status)==0;
        if(result<0)return false;
        usleep(10000);
    }
    kill(child,SIGKILL);waitpid(child,&status,0);return false;
}
int main(int argc,const char** argv) {
    @autoreleasepool {
        if(argc==4&&!strcmp(argv[1],"--append")){
            const BOOL wrote=RTWAppendControlDiagnosticRecord(
                RTWControlExchangeRecord(4294968492ULL,RTW_CONTROL_STATUS,0,1,NO),
                [NSString stringWithUTF8String:argv[2]]);
            return wrote==(!strcmp(argv[3],"success"))?0:1;
        }
        NSString* directory=[NSTemporaryDirectory() stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
        if(![NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:NO attributes:nil error:nil])return 1;
        NSString* path=[directory stringByAppendingPathComponent:@"control.jsonl"];
        bool okay=concurrentTimestamps();
        if(!okay)fputs("FAIL: concurrent diagnostic timestamp content or wall-clock bounds\n",stderr);
        // Existing owned FIFO with no reader: open must return immediately;
        // fstat cannot protect a blocking open.
        if(mkfifo(path.fileSystemRepresentation,0600)||!appendBeforeDeadline(argv[0],directory,false)){
            fputs("FAIL: FIFO diagnostic destination blocked or was written\n",stderr);okay=false;
        }
        unlink(path.fileSystemRepresentation);
        const int locked=open(path.fileSystemRepresentation,O_WRONLY|O_APPEND|O_CREAT|O_CLOEXEC,0600);
        if(locked<0||flock(locked,LOCK_EX))return 1;
        if(!appendBeforeDeadline(argv[0],directory,false)){
            fputs("FAIL: held diagnostic flock blocked command execution\n",stderr);okay=false;
        }
        flock(locked,LOCK_UN);close(locked);
        if(!appendBeforeDeadline(argv[0],directory,true)){
            fputs("FAIL: regular unlocked JSONL append failed\n",stderr);okay=false;
        }
        struct stat st{};
        if(stat(path.fileSystemRepresentation,&st)||(st.st_mode&0777)!=0600){
            fputs("FAIL: private diagnostic file permissions\n",stderr);okay=false;
        }
        NSString* line=[NSString stringWithContentsOfFile:path encoding:NSUTF8StringEncoding error:nil];
        NSData* json=[[line stringByTrimmingCharactersInSet:NSCharacterSet.newlineCharacterSet] dataUsingEncoding:NSUTF8StringEncoding];
        NSDictionary* record=json?[NSJSONSerialization JSONObjectWithData:json options:0 error:nil]:nil;
        if(![record[@"registry_id"] isEqual:@4294968492ULL]||![record[@"result_hex"] isEqual:@"0x00000000"]){
            fputs("FAIL: regular diagnostic JSONL content\n",stderr);okay=false;
        }
        // The next record resets a full log and remains parseable and bounded.
        const int full=open(path.fileSystemRepresentation,O_WRONLY|O_CLOEXEC);
        if(full<0||ftruncate(full,2*1024*1024))return 1;
        close(full);
        if(!appendBeforeDeadline(argv[0],directory,true)||stat(path.fileSystemRepresentation,&st)||st.st_size>2*1024*1024){
            fputs("FAIL: diagnostic log size cap\n",stderr);okay=false;
        }
        [NSFileManager.defaultManager removeItemAtPath:directory error:nil];
        if(okay)puts("Control diagnostics: FIFO, held flock, private JSONL and cap passed with child deadlines");
        return okay?0:1;
    }
}
