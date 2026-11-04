// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#import "LoginItem.h"
#include <cstdint>
#include <cstddef>

@interface RTWDriverConnection : NSObject
// All transport methods are called on the app's serial control queue.
- (uint32_t)exchangeRegistryID:(uint64_t)registryID selector:(uint32_t)selector
                        input:(const void*)input length:(size_t)length
                       output:(void*)output capacity:(size_t)capacity;
- (void)close;
@end
@interface RTWDriverServiceManager : NSObject
@property(nonatomic,readonly) RTWLoginItemState state;
- (BOOL)enableAndReturnError:(NSError**)error;
- (BOOL)disableAndReturnError:(NSError**)error;
// Only repairs an already-enabled registration. Completion and the validity
// predicate run on the main queue; cancellation prevents late re-registration.
- (void)repairWithValidityCheck:(BOOL (^)(void))valid
                    completion:(void (^)(NSError*))completion;
- (void)openSystemSettings;
@end
