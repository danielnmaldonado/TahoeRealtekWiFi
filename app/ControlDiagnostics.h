// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#include <cstdint>

// Metadata only: callers never pass request/output payloads to the logger.
NSDictionary* RTWControlExchangeRecord(uint64_t registryID,uint32_t selector,
    uint32_t result,double durationMS,BOOL beginning);
// Default directory is ~/Library/Logs/Tahoe-WiFi-USB. Explicit directories are
// useful for hardware-free file tests; command-line users cannot override it.
BOOL RTWAppendControlDiagnosticRecord(NSDictionary* record,NSString* directory=nil);
void RTWLogControlExchange(uint64_t registryID,uint32_t selector,uint32_t result,
    double durationMS,BOOL beginning);

enum class RTWLifecycleReason : uint8_t {
    SystemSleep,SystemWake,ScreenSleep,ScreenWake,SessionInactive,SessionActive,
    AppTermination,ManualDisconnect
};
// Lifecycle callers supply only a bounded enum and selected adapter ID.
NSDictionary* RTWLifecycleDiagnosticRecord(RTWLifecycleReason reason,uint64_t registryID);
void RTWLogLifecycle(RTWLifecycleReason reason,uint64_t registryID);
