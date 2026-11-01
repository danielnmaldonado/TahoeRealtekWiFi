// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>

// ServiceManagement, not a saved checkbox, is the source of truth.
typedef NS_ENUM(NSInteger, RTWLoginItemState) {
    RTWLoginItemStateNotRegistered,
    RTWLoginItemStateEnabled,
    RTWLoginItemStateRequiresApproval,
    RTWLoginItemStateNotFound
};

@protocol RTWLoginItemManaging <NSObject>
@property (nonatomic, readonly) RTWLoginItemState state;
- (BOOL)setEnabled:(BOOL)enabled error:(NSError**)error;
- (BOOL)repairRegistrationAndReturnError:(NSError**)error;
- (void)openSystemSettings;
@end

// Narrow, injectable boundary for tests without changing real login items.
@protocol RTWLoginItemBackend <NSObject>
@property (nonatomic, readonly) RTWLoginItemState state;
- (BOOL)prepareBundleAndReturnError:(NSError**)error;
- (BOOL)registerAndReturnError:(NSError**)error;
- (BOOL)unregisterAndReturnError:(NSError**)error;
- (void)openSystemSettings;
@end

@interface RTWLoginItemManager : NSObject <RTWLoginItemManaging>
- (instancetype)init;
- (instancetype)initWithBackend:(id<RTWLoginItemBackend>)backend;
@end
