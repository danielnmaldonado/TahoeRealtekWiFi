// SPDX-License-Identifier: GPL-2.0-only
#import "DriverConnection.h"
#import "ServiceProtocol.h"
#import "CodeIdentity.h"
#import <ServiceManagement/ServiceManagement.h>
#include "Transport.hpp"
#include "Model.hpp"
#include <mutex>
#include <memory>

namespace {
struct ResponseState {
    std::mutex mutex;
    dispatch_semaphore_t ready=dispatch_semaphore_create(0);
    NSData* response=nil;
    bool finished=false;
    void complete(NSData* value) {
        std::lock_guard<std::mutex> lock(mutex);
        if(finished)return;
        finished=true;response=value;
        dispatch_semaphore_signal(ready);
    }
};
}
@implementation RTWDriverConnection {
    NSXPCConnection* _connection;
}
- (BOOL)open {
    if(_connection)return YES;
    NSURL* helper=[NSBundle.mainBundle.bundleURL URLByAppendingPathComponent:
        @"Contents/Helpers/TahoeWiFiHelper"];
    NSString* requirement=RTWRequirementForURL(helper);
    if(!requirement.length)return NO;
    _connection=[[NSXPCConnection alloc]initWithMachServiceName:RTWDriverMachService
        options:NSXPCConnectionPrivileged];
    _connection.remoteObjectInterface=[NSXPCInterface interfaceWithProtocol:@protocol(RTWDriverService)];
    [_connection setCodeSigningRequirement:requirement];
    [_connection resume];
    return YES;
}
- (void)close {
    [_connection invalidate];_connection=nil;
}
- (void)dealloc {[_connection invalidate];}
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity {
    if(capacity!=rtwui::outputSize(selector)||!output)
        return rtwui::protocolError;
    if(length>UINT32_MAX) {
        // Reject oversized request lengths without retaining previous results.
        memset(output,0,capacity);
        return rtwui::protocolError;
    }
    auto frame=rtwui::makeFrame(selector,registryID,static_cast<uint32_t>(length));
    if(!rtwui::validFrame(frame,false)||
       (selector==rtwui::listSelector?(input||length):
        rtw_control_validate_request(selector,input,length)!=RTW_CONTROL_VALID)) {
        // Keep XPC and legacy socket transport identical on local failures:
        // do not expose a stale status or device list after a rejected call.
        memset(output,0,capacity);
        return rtwui::protocolError;
    }
    memset(output,0,capacity);
    if(![self open])return rtwui::transportError;
    auto state=std::make_shared<ResponseState>();
    NSMutableData* message=[NSMutableData dataWithBytes:&frame length:sizeof(frame)];
    if(length)[message appendBytes:input length:length];
    id<RTWDriverService> remote=[_connection remoteObjectProxyWithErrorHandler:^(NSError* error) {
        (void)error;state->complete(nil);
    }];
    [remote performRequest:message reply:^(NSData* response) {state->complete(response);}];
    // Wait off the main thread with a bounded deadline. Callback state remains
    // alive if XPC delivers a late reply after this method times out.
    const long timedOut=dispatch_semaphore_wait(state->ready,
        dispatch_time(DISPATCH_TIME_NOW,8LL*NSEC_PER_SEC));
    rtwui::wipe(message.mutableBytes,message.length);
    [message setLength:0];
    NSData* response=nil;
    {
        std::lock_guard<std::mutex> lock(state->mutex);
        state->finished=true;
        if(!timedOut)response=state->response;
        state->response=nil;
    }
    if(![response isKindOfClass:NSData.class]||response.length<sizeof(frame)) {
        [self close];return rtwui::transportError;
    }
    rtwui::Frame answer{};memcpy(&answer,response.bytes,sizeof(answer));
    if(!rtwui::validFrame(answer,true)||answer.selector!=selector||
       answer.registryID!=registryID||response.length!=sizeof(answer)+answer.length) {
        [self close];return rtwui::protocolError;
    }
    if(answer.result)return answer.result;
    memcpy(output,static_cast<const uint8_t*>(response.bytes)+sizeof(answer),capacity);
    const bool validPayload=rtwui::validPayload(selector,output);
    if(!validPayload) {
        // An XPC reply with valid framing can still contain an invalid
        // payload. Never leave that data in a caller's result buffer or reuse
        // the connection that delivered a malformed response.
        memset(output,0,capacity);
        [self close];
        return rtwui::protocolError;
    }
    return 0;
}
@end
@implementation RTWDriverServiceManager
- (SMAppService*)service {
    return [SMAppService daemonServiceWithPlistName:RTWDriverDaemonPlist];
}
- (RTWLoginItemState)state {
    switch(self.service.status) {
    case SMAppServiceStatusEnabled:return RTWLoginItemStateEnabled;
    case SMAppServiceStatusRequiresApproval:return RTWLoginItemStateRequiresApproval;
    case SMAppServiceStatusNotRegistered:return RTWLoginItemStateNotRegistered;
    case SMAppServiceStatusNotFound:return RTWLoginItemStateNotFound;
    }
    return RTWLoginItemStateNotFound;
}
- (BOOL)enableAndReturnError:(NSError**)error {
    if(error)*error=nil;
    NSString* path=NSBundle.mainBundle.bundleURL.URLByResolvingSymlinksInPath.path;
    if(!RTWProtectedBundle(path)) {
        if(error)*error=[NSError errorWithDomain:@"org.research.TahoeUSBWiFi.Driver"
            code:1 userInfo:@{NSLocalizedDescriptionKey:
                [NSString stringWithFormat:
                @"Install the app using the .pkg package and open /Applications/Tahoe Wi-Fi USB.app. The service requires the complete app with protected files.\n\nRunning copy: %@\n\nTo build the installer from source, run build-pkg.command.",path]}];
        return NO;
    }
    if(self.state==RTWLoginItemStateEnabled||self.state==RTWLoginItemStateRequiresApproval)return YES;
    NSError* registrationError=nil;
    const BOOL accepted=[self.service registerAndReturnError:&registrationError];
    if(self.state==RTWLoginItemStateEnabled||self.state==RTWLoginItemStateRequiresApproval)return YES;
    if(error)*error=registrationError?:[NSError errorWithDomain:@"org.research.TahoeUSBWiFi.Driver"
        code:2 userInfo:@{NSLocalizedDescriptionKey:accepted?
            @"macOS received the request but has not confirmed the service. Check Login Items.":
            @"macOS denied service registration. Check the installation and code signature."}];
    return NO;
}
- (BOOL)disableAndReturnError:(NSError**)error {
    if(error)*error=nil;
    if(self.state==RTWLoginItemStateNotRegistered)return YES;
    if(![self.service unregisterAndReturnError:error])return NO;
    if(self.state==RTWLoginItemStateEnabled) {
        if(error)*error=[NSError errorWithDomain:@"org.research.TahoeUSBWiFi.Driver"
            code:3 userInfo:@{NSLocalizedDescriptionKey:
                @"macOS still reports the service as enabled. Check Login Items."}];
        return NO;
    }
    return YES;
}
- (void)openSystemSettings {[SMAppService openSystemSettingsLoginItems];}
@end
