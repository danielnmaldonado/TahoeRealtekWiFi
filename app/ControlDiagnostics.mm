// SPDX-License-Identifier: GPL-2.0-only
#import "ControlDiagnostics.h"
#include "Protocol.hpp"
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cmath>

namespace {
NSString* operation(uint32_t selector) {
    switch(selector){
    case rtwui::listSelector:return @"LIST";
    case RTW_CONTROL_STATUS:return @"STATUS";
    case RTW_CONTROL_CHANNEL_MAP:return @"MAP";
    case RTW_CONTROL_SCAN:return @"SCAN";
    case RTW_CONTROL_SCAN_RESULTS:return @"SCAN_RESULTS";
    case RTW_CONTROL_JOIN:return @"JOIN";
    case RTW_CONTROL_LEAVE:return @"LEAVE";
    default:return @"INVALID";
    }
}
NSString* timestamp() {
    // Formatting is on every request/result path. Construct this immutable
    // formatter once; serialize use so concurrent CLI/control callers do not
    // depend on undocumented formatter thread-safety guarantees.
    static NSISO8601DateFormatter* formatter;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        formatter=[NSISO8601DateFormatter new];
        formatter.formatOptions=NSISO8601DateFormatWithInternetDateTime|NSISO8601DateFormatWithFractionalSeconds;
    });
    @synchronized(formatter) {
        return [formatter stringFromDate:NSDate.date];
    }
}
}
NSDictionary* RTWControlExchangeRecord(uint64_t registryID,uint32_t selector,
    uint32_t result,double durationMS,BOOL beginning) {
    NSMutableDictionary* record=[@{@"event":@"control",@"phase":beginning?@"request":@"result",
        @"timestamp":timestamp(),@"monotonic_ms":@(NSProcessInfo.processInfo.systemUptime*1000.0),
        @"pid":@(getpid()),@"registry_id":@(registryID),@"selector":@(selector),
        @"operation":operation(selector)} mutableCopy];
    if(!beginning){record[@"result"]=@(result);record[@"result_hex"]=[NSString stringWithFormat:@"0x%08x",result];
        record[@"duration_ms"]=@(std::isfinite(durationMS)&&durationMS>=0?durationMS:0);}
    return record;
}
BOOL RTWAppendControlDiagnosticRecord(NSDictionary* record,NSString* directory) {
    // This path never elevates privileges or creates root-owned user logs.
    if(getuid()==0||geteuid()!=getuid()||!record)return NO;
    if(!directory)directory=[NSHomeDirectory() stringByAppendingPathComponent:@"Library/Logs/Tahoe-WiFi-USB"];
    if(![NSFileManager.defaultManager createDirectoryAtPath:directory withIntermediateDirectories:YES
        attributes:@{NSFilePosixPermissions:@0700} error:nil])return NO;
    struct stat dir{};
    if(lstat(directory.fileSystemRepresentation,&dir)||!S_ISDIR(dir.st_mode)||dir.st_uid!=getuid())return NO;
    NSData* json=[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingSortedKeys error:nil];
    if(!json||json.length>65536)return NO;
    NSString* path=[directory stringByAppendingPathComponent:@"control.jsonl"];
    // A pre-existing FIFO can block inside open before fstat can reject it.
    const int fd=open(path.fileSystemRepresentation,O_WRONLY|O_APPEND|O_CREAT|O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK,0600);
    if(fd<0)return NO;
    struct stat st{};
    bool okay=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_uid==getuid()&&st.st_nlink==1;
    // Logging is best effort: another app/CLI process must never hold up a
    // driver request while it owns this file's advisory lock.
    if(okay)okay=!fchmod(fd,0600)&&!flock(fd,LOCK_EX|LOCK_NB);
    // Bound the rolling diagnostic file to 2 MiB, including frequent STATUS
    // polling. Locking covers the whole JSON line for app/CLI coexistence.
    if(okay)okay=!fstat(fd,&st);
    if(okay&&st.st_size+static_cast<off_t>(json.length)+1>2*1024*1024)okay=!ftruncate(fd,0);
    if(okay){
        NSMutableData* line=[json mutableCopy];const char newline='\n';[line appendBytes:&newline length:1];
        const auto* cursor=static_cast<const uint8_t*>(line.bytes);size_t remaining=line.length;
        while(remaining){const ssize_t sent=write(fd,cursor,remaining);
            if(sent<0&&errno==EINTR)continue;
            if(sent<=0){okay=false;break;}cursor+=sent;remaining-=static_cast<size_t>(sent);}
    }
    close(fd);return okay;
}
void RTWLogControlExchange(uint64_t registryID,uint32_t selector,uint32_t result,
    double durationMS,BOOL beginning) {
    (void)RTWAppendControlDiagnosticRecord(RTWControlExchangeRecord(registryID,selector,result,durationMS,beginning));
}
