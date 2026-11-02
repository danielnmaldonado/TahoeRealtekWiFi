// SPDX-License-Identifier: GPL-2.0-only
#import "DiagnosticCLI.h"
#import "ControlDiagnostics.h"
#import "DriverConnection.h"
#include "Transport.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOReturn.h>
#include <atomic>
#include <cstdio>
#include <unistd.h>

namespace {
NSDictionary* registrySnapshot(uint64_t registryID) {
    NSMutableDictionary* record=[@{@"registry_id":@(registryID)} mutableCopy];
    auto result=[&](uint32_t code){record[@"registry_result"]=@(code);
        record[@"registry_result_hex"]=[NSString stringWithFormat:@"0x%08x",code];};
    CFMutableDictionaryRef match=IORegistryEntryIDMatching(registryID);
    io_service_t service=match?IOServiceGetMatchingService(kIOMainPortDefault,match):IO_OBJECT_NULL;
    if(!service){result(uint32_t(kIOReturnNotFound));return record;}
    uint64_t actualID=0;IORegistryEntryGetRegistryEntryID(service,&actualID);
    CFTypeRef bundle=IORegistryEntryCreateCFProperty(service,CFSTR("CFBundleIdentifier"),kCFAllocatorDefault,0);
    const bool target=actualID==registryID&&IOObjectConformsTo(service,RTW_CONTROL_SERVICE_CLASS)&&bundle&&
        CFGetTypeID(bundle)==CFStringGetTypeID()&&CFEqual(bundle,CFSTR(RTW_CONTROL_BUNDLE_ID));
    if(bundle)CFRelease(bundle);
    if(!target){IOObjectRelease(service);result(uint32_t(kIOReturnBadArgument));return record;}
    CFMutableDictionaryRef raw=nullptr;
    const kern_return_t code=IORegistryEntryCreateCFProperties(service,&raw,kCFAllocatorDefault,0);
    IOObjectRelease(service);result(uint32_t(code));
    if(code!=KERN_SUCCESS||!raw){if(raw)CFRelease(raw);return record;}
    NSDictionary* properties=CFBridgingRelease(raw);
    NSMutableDictionary* safe=[NSMutableDictionary dictionary];
    // Numeric diagnostic counters only. Never copy MAC/BSSID/SSID, packet
    // bytes, USB serial strings, or arbitrary string/data properties.
    NSSet* numeric=[NSSet setWithArray:@[@"RTWStartResult",@"IOLinkStatus",@"IOLinkSpeed",
        @"RTW8814Cut",@"RTW8814RFE",@"RTW8814TXStreams",@"RTW8814PhysicalRFPaths"]];
    for(id key in properties){
        if(![key isKindOfClass:NSString.class])continue;
        if(![key hasPrefix:@"RTWDiag"]&&![numeric containsObject:key])continue;
        id value=properties[key];
        if([value isKindOfClass:NSNumber.class])safe[key]=value;
    }
    if([properties[@"RTWStartStage"] isEqual:@"ready"])safe[@"RTWStartStage"]=@"ready";
    record[@"properties"]=safe;return record;
}
NSDictionary* statusDetails(const RTWControlStatus& status) {
    return @{@"version":@(status.version),@"flags":@(status.flags),@"state":@(status.state),
        @"channel":@(status.channel),@"rssi":@(status.rssi),@"usb_error":@(status.usbError),
        @"usb_error_hex":[NSString stringWithFormat:@"0x%08x",status.usbError],
        @"tx_packets":@(status.txPackets),@"rx_packets":@(status.rxPackets),
        @"tx_errors":@(status.txErrors),@"rx_errors":@(status.rxErrors)};
}
void keepRunLoopAlive(void*) {}
}
int RTWRunAdapterDiagnosis(const rtwui::DiagnosticArguments& options,RTWDriverConnection* driver,
    RTWDiagnosticSnapshot snapshot,RTWDiagnosticSink sink) {
    if(options.mode!=rtwui::DiagnosticMode::Valid||!options.registryID||!driver||!snapshot||!sink||NSThread.isMainThread)return 64;
    bool failed=false;
    auto exchange=[&](uint64_t id,uint32_t selector,const void* input,size_t length,void* output,size_t capacity)->uint32_t {
        NSMutableDictionary* begin=[RTWControlExchangeRecord(id,selector,0,0,YES) mutableCopy];
        begin[@"origin"]=@"diagnostic";begin[@"selected_registry_id"]=@(options.registryID);sink(begin);
        const double started=NSProcessInfo.processInfo.systemUptime;
        const uint32_t result=[driver exchangeRegistryID:id selector:selector input:input length:length output:output capacity:capacity];
        NSMutableDictionary* event=[RTWControlExchangeRecord(id,selector,result,
            (NSProcessInfo.processInfo.systemUptime-started)*1000.0,NO) mutableCopy];
        event[@"origin"]=@"diagnostic";event[@"selected_registry_id"]=@(options.registryID);
        if(!result&&selector==RTW_CONTROL_STATUS)event[@"status"]=statusDetails(*static_cast<RTWControlStatus*>(output));
        if(!result&&selector==RTW_CONTROL_CHANNEL_MAP){
            const auto& map=*static_cast<RTWControlChannelMap*>(output);NSMutableArray* channels=[NSMutableArray array];
            for(unsigned channel=1;channel<256;++channel)if(rtw_control_channel_map_contains(&map,channel))[channels addObject:@(channel)];
            event[@"channels"]=channels;
        }
        if(result)failed=true;sink(event);return result;
    };
    rtwui::DeviceList devices{};
    const uint32_t listed=exchange(0,rtwui::listSelector,nullptr,0,&devices,sizeof(devices));
    if(listed)return 1;
    bool found=false;
    if(rtwui::validDevices(devices))for(uint32_t i=0;i<devices.count;++i)if(devices.devices[i].registryID==options.registryID)found=true;
    if(!found){
        NSMutableDictionary* event=[RTWControlExchangeRecord(options.registryID,rtwui::listSelector,
            uint32_t(kIOReturnNotFound),0,NO) mutableCopy];
        event[@"origin"]=@"diagnostic";event[@"event"]=@"selection";sink(event);return 1;
    }
    auto capture=[&](NSString* phase){
        NSMutableDictionary* event=[RTWControlExchangeRecord(options.registryID,RTW_CONTROL_STATUS,0,0,YES) mutableCopy];
        event[@"event"]=@"registry_snapshot";event[@"phase"]=phase;
        event[@"snapshot"]=snapshot(options.registryID);sink(event);
    };
    capture(@"before");
    RTWControlRequest request{RTW_CONTROL_VERSION,{0,0,0}};
    RTWControlStatus status{};RTWControlChannelMap map{};
    exchange(options.registryID,RTW_CONTROL_STATUS,&request,sizeof(request),&status,sizeof(status));
    exchange(options.registryID,RTW_CONTROL_CHANNEL_MAP,&request,sizeof(request),&map,sizeof(map));
    if(options.scan){
        RTWControlReply reply{};
        exchange(options.registryID,RTW_CONTROL_SCAN,&request,sizeof(request),&reply,sizeof(reply));
        exchange(options.registryID,RTW_CONTROL_STATUS,&request,sizeof(request),&status,sizeof(status));
    }
    capture(@"after");[driver close];return failed?1:0;
}
int RTWHandleAdapterDiagnosticCLI(int argc,const char* const* argv) {
    const auto options=rtwui::parseDiagnosticArguments(argc,argv);
    if(options.mode==rtwui::DiagnosticMode::NotRequested)return -1;
    if(options.mode!=rtwui::DiagnosticMode::Valid){
        fputs("Usage: TahoeUSBWiFi --diagnose-adapter <nonzero registry ID> [--scan]\n",stderr);return 64;
    }
    if(getuid()==0||geteuid()!=getuid()){
        fputs("Run adapter diagnostics as the normal logged-in user.\n",stderr);return 77;
    }
    __block int exitCode=1;__block BOOL logOkay=YES;
    std::atomic<bool> finished{false};auto* done=&finished;
    CFRunLoopRef loop=CFRunLoopGetCurrent();
    CFRunLoopSourceContext context{};context.perform=keepRunLoopAlive;
    CFRunLoopSourceRef keepAlive=CFRunLoopSourceCreate(kCFAllocatorDefault,0,&context);
    CFRunLoopAddSource(loop,keepAlive,kCFRunLoopDefaultMode);
    // No NSApplication, instance lock, preference reads, Keychain, service
    // registration, or privileged prompt. The signed app uses its existing
    // authenticated XPC path; main remains able to service run-loop callbacks.
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_UTILITY,0),^{@autoreleasepool{
        RTWDriverConnection* driver=[RTWDriverConnection new];
        exitCode=RTWRunAdapterDiagnosis(options,driver,^(uint64_t id){return registrySnapshot(id);},^(NSDictionary* record){
            if(!RTWAppendControlDiagnosticRecord(record))logOkay=NO;
            NSData* json=[NSJSONSerialization dataWithJSONObject:record options:NSJSONWritingSortedKeys error:nil];
            if(json){fwrite(json.bytes,1,json.length,stdout);fputc('\n',stdout);fflush(stdout);}
        });
        [driver close];done->store(true);CFRunLoopStop(loop);CFRunLoopWakeUp(loop);
    }});
    while(!finished.load())CFRunLoopRunInMode(kCFRunLoopDefaultMode,0.25,false);
    CFRunLoopRemoveSource(loop,keepAlive,kCFRunLoopDefaultMode);CFRelease(keepAlive);
    if(!logOkay){fputs("Could not append the private diagnostic log; JSON results remain on stdout.\n",stderr);return 74;}
    return exitCode;
}
