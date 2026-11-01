// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>

static NSString* const RTWDriverMachService = @"org.research.TahoeUSBWiFi.driver";
static NSString* const RTWDriverDaemonPlist = @"org.research.TahoeUSBWiFi.driver.plist";

// The daemon only implements the fixed, validated driver ABI. There is no
// command execution, file access, keychain access or arbitrary IOKit selector.
@protocol RTWDriverService
- (void)performRequest:(NSData*)request reply:(void (^)(NSData* response))reply;
@end
