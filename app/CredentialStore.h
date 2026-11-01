// SPDX-License-Identifier: GPL-2.0-only
#pragma once
#import <Foundation/Foundation.h>
#import <Security/Security.h>

NS_ASSUME_NONNULL_BEGIN
// Only this application's WPA2 items are addressed. No system Wi-Fi passwords
// are imported. All methods are synchronous; call them on the credential queue.
@protocol RTWCredentialStoring <NSObject>
- (nullable NSMutableData*)passwordForSSID:(NSData*)ssid error:(NSError* _Nullable * _Nullable)error;
@optional
- (nullable NSMutableData*)passwordForSSID:(NSData*)ssid allowInteraction:(BOOL)allowInteraction
                                   error:(NSError* _Nullable * _Nullable)error;
@required
- (BOOL)savePassword:(NSData*)password forSSID:(NSData*)ssid error:(NSError* _Nullable * _Nullable)error;
- (BOOL)deletePasswordForSSID:(NSData*)ssid error:(NSError* _Nullable * _Nullable)error;
- (nullable NSArray<NSData*>*)savedSSIDsAndReturnError:(NSError* _Nullable * _Nullable)error;
@end

// Injectable Security entry points keep the regression tests out of the user's
// real keychain. Production always uses the system functions.
struct RTWKeychainAPI {
    OSStatus (*copyMatching)(CFDictionaryRef, CFTypeRef _Nullable * _Nullable);
    OSStatus (*add)(CFDictionaryRef, CFTypeRef _Nullable * _Nullable);
    OSStatus (*update)(CFDictionaryRef, CFDictionaryRef);
    OSStatus (*remove)(CFDictionaryRef);
};

@interface RTWCredentialStore : NSObject <RTWCredentialStoring>
- (instancetype)init;
- (instancetype)initWithService:(NSString*)service api:(RTWKeychainAPI)api NS_DESIGNATED_INITIALIZER;
@end

FOUNDATION_EXPORT void RTWWipePassword(NSMutableData* _Nullable password);
NS_ASSUME_NONNULL_END
