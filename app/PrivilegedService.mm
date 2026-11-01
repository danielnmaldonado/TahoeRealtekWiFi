// SPDX-License-Identifier: GPL-2.0-only
#import "ServiceProtocol.h"
#import "CodeIdentity.h"
#include "DriverInvocation.hpp"
#include "Model.hpp"
#import <SystemConfiguration/SystemConfiguration.h>
#include <atomic>
#include <sys/resource.h>
#include <unistd.h>

static_assert(sizeof(RTWControlScanResults)>=sizeof(rtwui::DeviceList),
    "The IPC output buffer must fit the device list.");
static_assert(sizeof(RTWControlScanResults)>=sizeof(RTWControlStatus),
    "The IPC output buffer must fit status.");
static_assert(sizeof(RTWControlScanResults)>=sizeof(RTWControlChannelMap),
    "The IPC output buffer must fit the registered channel map.");
static dispatch_queue_t driverQueue;
static std::atomic<unsigned> pendingRequests{0};

static BOOL activeConsoleUser(uid_t uid) {
    uid_t console=0;gid_t group=0;
    CFStringRef name=SCDynamicStoreCopyConsoleUser(nullptr,&console,&group);
    const BOOL allowed=name&&uid!=0&&uid==console&&
        !CFEqual(name,CFSTR("loginwindow"));
    if(name)CFRelease(name);
    return allowed;
}
static NSData* failureResponse(const rtwui::Frame& request,uint32_t error) {
    auto frame=rtwui::makeFrame(request.selector,request.registryID,0,error);
    return [NSData dataWithBytes:&frame length:sizeof(frame)];
}
@interface RTWRequestHandler : NSObject <RTWDriverService>
@property(nonatomic,assign) uid_t uid;
@end
@implementation RTWRequestHandler
- (void)performRequest:(NSData*)request reply:(void (^)(NSData*))reply {
    if(!reply)return;
    rtwui::Frame frame{};
    if(![request isKindOfClass:NSData.class]||request.length<sizeof(frame)||
       request.length>sizeof(frame)+sizeof(RTWControlJoin)) {
        reply([NSData data]);return;
    }
    memcpy(&frame,request.bytes,sizeof(frame));
    const auto* bytes=static_cast<const uint8_t*>(request.bytes)+sizeof(frame);
    if(!rtwui::validFrame(frame,false)||request.length!=sizeof(frame)+frame.length||
       (frame.selector!=rtwui::listSelector&&
        rtw_control_validate_request(frame.selector,bytes,frame.length)!=RTW_CONTROL_VALID)) {
        reply(failureResponse(frame,rtwui::protocolError));return;
    }
    const uid_t uid=self.uid;
    if(!activeConsoleUser(uid)) {
        reply(failureResponse(frame,kIOReturnNotPrivileged));return;
    }
    // Bound outstanding work even if a legitimate client malfunctions.
    if(pendingRequests.fetch_add(1)>=8) {
        pendingRequests.fetch_sub(1);
        reply(failureResponse(frame,kIOReturnBusy));return;
    }
    NSMutableData* input=[request mutableCopy];
    if(!input){pendingRequests.fetch_sub(1);reply(failureResponse(frame,kIOReturnNoMemory));return;}
    dispatch_async(driverQueue,^{
        @autoreleasepool {
            alignas(RTWControlJoin) uint8_t argument[sizeof(RTWControlJoin)]{};
            alignas(rtwui::DeviceList) uint8_t output[sizeof(RTWControlScanResults)]{};
            const auto* data=static_cast<const uint8_t*>(input.bytes)+sizeof(frame);
            memcpy(argument,data,frame.length);
            rtwui::wipe(input.mutableBytes,input.length);
            [input setLength:0];
            const uint32_t code=activeConsoleUser(uid)?
                rtwui::invokeDriver(frame.registryID,frame.selector,argument,frame.length,output):
                static_cast<uint32_t>(kIOReturnNotPrivileged);
            rtwui::wipe(argument,sizeof(argument));
            const uint32_t length=code?0:rtwui::outputSize(frame.selector);
            auto response=rtwui::makeFrame(frame.selector,frame.registryID,length,code);
            NSMutableData* message=[NSMutableData dataWithBytes:&response length:sizeof(response)];
            if(length)[message appendBytes:output length:length];
            rtwui::wipe(output,sizeof(output));
            pendingRequests.fetch_sub(1);
            reply(message);
        }
    });
}
@end
@interface RTWServiceDelegate : NSObject <NSXPCListenerDelegate>
@end
@implementation RTWServiceDelegate
- (BOOL)listener:(NSXPCListener*)listener shouldAcceptNewConnection:(NSXPCConnection*)connection {
    (void)listener;
    if(!activeConsoleUser(connection.effectiveUserIdentifier))return NO;
    RTWRequestHandler* handler=[RTWRequestHandler new];
    handler.uid=connection.effectiveUserIdentifier;
    connection.exportedInterface=[NSXPCInterface interfaceWithProtocol:@protocol(RTWDriverService)];
    connection.exportedObject=handler;
    // No connection->handler->connection retain cycle. launchd/XPC owns the
    // connection lifetime; driver handles are opened and closed per request.
    [connection resume];
    return YES;
}
@end
int main(int argc,const char** argv) {
    (void)argc;(void)argv;
    rlimit core{0,0};setrlimit(RLIMIT_CORE,&core);
    if(geteuid()!=0)return 77;
    @autoreleasepool {
        NSString* path=@"/Applications/Tahoe Wi-Fi USB.app";
        if(!RTWProtectedBundle(path))return 78;
        NSString* requirement=RTWRequirementForURL(
            [NSURL fileURLWithPath:[path stringByAppendingPathComponent:@"Contents/MacOS/TahoeUSBWiFi"]]);
        if(!requirement.length)return 78;
        driverQueue=dispatch_queue_create("org.research.TahoeUSBWiFi.driver.serial",
            dispatch_queue_attr_make_with_autorelease_frequency(DISPATCH_QUEUE_SERIAL,
                DISPATCH_AUTORELEASE_FREQUENCY_WORK_ITEM));
        __attribute__((objc_precise_lifetime)) NSXPCListener* listener=
            [[NSXPCListener alloc]initWithMachServiceName:RTWDriverMachService];
        // Use the actual signed executable's designated requirement, including
        // cdhash for ad-hoc builds. Never trust a bundle identifier alone.
        [listener setConnectionCodeSigningRequirement:requirement];
        __attribute__((objc_precise_lifetime)) RTWServiceDelegate* delegate=[RTWServiceDelegate new];
        listener.delegate=delegate;
        [listener resume];
        // A command-line daemon need not have any run-loop sources.
        // dispatch_main keeps the XPC listener alive even before the first client.
        dispatch_main();
    }
    return 0;
}
