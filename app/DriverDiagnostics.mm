// SPDX-License-Identifier: GPL-2.0-only
#import "DriverDiagnostics.h"
#include "RTWControl.h"
#include <IOKit/IOKitLib.h>

namespace {
bool reportNumber(CFTypeRef value,uint64_t& number) {
    if(!value||CFGetTypeID(value)!=CFNumberGetTypeID()||
       CFNumberIsFloatType(static_cast<CFNumberRef>(value)))return false;
    int64_t integer=0;
    if(!CFNumberGetValue(static_cast<CFNumberRef>(value),kCFNumberSInt64Type,&integer)||integer<0)return false;
    number=static_cast<uint64_t>(integer);return true;
}
void appendCounter(NSMutableString* report,CFDictionaryRef properties,CFStringRef key,NSString* label) {
    uint64_t number=0;
    NSString* value=reportNumber(CFDictionaryGetValue(properties,key),number)?
        [NSString stringWithFormat:@"%llu",static_cast<unsigned long long>(number)]:@"not reported";
    [report appendFormat:@"%@: %@\n",label,value];
}
}

NSString* RTWDriverDiagnosticsReport(uint64_t registryID) {
    if(!registryID)return @"No adapter selected.";
    CFMutableDictionaryRef matching=IORegistryEntryIDMatching(registryID);
    if(!matching)return @"The selected adapter is unavailable.";
    io_service_t service=IOServiceGetMatchingService(kIOMainPortDefault,matching);
    if(!service)return @"The selected adapter is unavailable.";
    if(!IOObjectConformsTo(service,RTW_CONTROL_SERVICE_CLASS)) {
        IOObjectRelease(service);return @"The selected adapter is unavailable.";
    }
    CFTypeRef before=IORegistryEntryCreateCFProperty(service,CFSTR("RTWDiagPublicationGeneration"),kCFAllocatorDefault,0);
    CFMutableDictionaryRef properties=nullptr;
    const auto result=IORegistryEntryCreateCFProperties(service,&properties,kCFAllocatorDefault,0);
    CFTypeRef after=IORegistryEntryCreateCFProperty(service,CFSTR("RTWDiagPublicationGeneration"),kCFAllocatorDefault,0);
    IOObjectRelease(service);
    uint64_t first=0,last=0,snapshot=0;
    const bool coherent=result==KERN_SUCCESS&&properties&&CFGetTypeID(properties)==CFDictionaryGetTypeID()&&
        reportNumber(before,first)&&reportNumber(after,last)&&first&&!(first&1U)&&first==last&&
        reportNumber(CFDictionaryGetValue(properties,CFSTR("RTWDiagPublicationGeneration")),snapshot)&&snapshot==first;
    if(before)CFRelease(before);if(after)CFRelease(after);
    if(!coherent) {
        if(properties)CFRelease(properties);
        return @"Driver snapshot unavailable. Counters were not exported; try Driver details again.";
    }
    NSMutableString* report=[NSMutableString stringWithFormat:@"Coherent driver snapshot · generation %llu\n",
        static_cast<unsigned long long>(snapshot)];
    // Export only these numeric counters. Never serialize the property tree,
    // addresses, SSIDs, last-frame payloads or peer bitmap diagnostics.
    appendCounter(report,properties,CFSTR("RTWDiagDriverBuild"),@"Driver build");
    appendCounter(report,properties,CFSTR("RTWDiagRXErrors"),@"RX errors");
    appendCounter(report,properties,CFSTR("RTWDiagRXBlockAckFiltered"),@"Validated BlockAck filtered");
    [report appendString:@"Validated BlockAck filtering is expected control traffic, separate from RX errors.\n\nReceive drops and recovery (cumulative)\n"];
    const struct {CFStringRef key;NSString* label;} counters[]={
        {CFSTR("RTWDiagRXAggregateErrors"),@"Aggregate errors"},
        {CFSTR("RTWDiagRXAggregateTruncated"),@"Truncated aggregate"},
        {CFSTR("RTWDiagRXAggregateMalformed"),@"Malformed aggregate"},
        {CFSTR("RTWDiagRXCRCDrops"),@"CRC"},
        {CFSTR("RTWDiagRXICVDrops"),@"ICV"},
        {CFSTR("RTWDiagRXInvalidFrameDrops"),@"Invalid frame"},
        {CFSTR("RTWDiagRXHardwareDecryptDrops"),@"Hardware decrypt"},
        {CFSTR("RTWDiagRXAllocationDrops"),@"Allocation"},
        {CFSTR("RTWDiagRXCopyDrops"),@"Copy"},
        {CFSTR("RTWDiagRXNoNodeDrops"),@"Missing node"},
        {CFSTR("RTWDiagRXRecoveryErrors"),@"Recovery errors"},
        {CFSTR("RTWDiagRXRecoveryAttempts"),@"Recovery attempts"},
        {CFSTR("RTWDiagRXRecoveryCompleted"),@"Recovery completed"},
        {CFSTR("RTWDiagRXRecoveryFailures"),@"Recovery failures"},
        {CFSTR("RTWDiagRXRecoveryExhausted"),@"Recovery exhausted"}
    };
    for(const auto& counter:counters)appendCounter(report,properties,counter.key,counter.label);
    bool hasRejectedControl=false;
    const char* labels[]={"Control subtype 0","Control subtype 1","Trigger","Control subtype 3",
        "Beamforming Report Poll","VHT NDP Announcement","Control Frame Extension","Control Wrapper",
        "BlockAck Request","BlockAck","PS-Poll","RTS","CTS","ACK","CF-End","CF-End + CF-ACK"};
    for(unsigned subtype=0;subtype<16;++subtype) {
        NSString* key=[NSString stringWithFormat:@"RTWDiagRXRejectControlSubtype%u",subtype];uint64_t value=0;
        if(!reportNumber(CFDictionaryGetValue(properties,(__bridge CFStringRef)key),value)||!value)continue;
        if(!hasRejectedControl)[report appendString:@"\nRejected control frames by subtype (nonzero)\n"];
        hasRejectedControl=true;
        [report appendFormat:@"%s: %llu\n",labels[subtype],static_cast<unsigned long long>(value)];
    }
    if(!hasRejectedControl)[report appendString:@"\nNo nonzero rejected-control subtype counters reported.\n"];
    CFRelease(properties);return report;
}
